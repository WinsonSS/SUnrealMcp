import * as net from "net";
import {
    createCommandRequest,
    encodeDelimitedMessage,
    extractDelimitedMessages,
    hasDelimitedMessageTerminator,
    normalizeUnrealResponse,
    type NormalizedUnrealResponse,
} from "../protocol.js";
import { Logger } from "../types.js";

export interface UnrealTransportClientOptions {
    host: string;
    port: number;
    timeoutMs: number;
    logger: Logger;
}

interface PendingRequest {
    command: string;
    params: Record<string, unknown>;
    requestId: string;
    startedAt: number;
    resolve: (value: NormalizedUnrealResponse) => void;
    reject: (error: Error) => void;
    timeout: NodeJS.Timeout;
}

class UnrealTransportSession {
    private readonly socket = new net.Socket();
    private readonly host: string;
    private readonly port: number;
    private readonly timeoutMs: number;
    private readonly logger: Logger;
    private buffer = "";
    private connected = false;
    private closed = false;
    private readonly pending: PendingRequest[] = [];

    constructor(options: UnrealTransportClientOptions) {
        this.host = options.host;
        this.port = options.port;
        this.timeoutMs = options.timeoutMs;
        this.logger = options.logger;

        this.socket.setNoDelay(true);
        this.socket.on("data", (chunk: Buffer) => {
            this.handleData(chunk);
        });
        this.socket.on("error", (error: Error) => {
            this.failAll(error);
        });
        this.socket.on("end", () => {
            if (this.buffer.trim().length > 0) {
                this.logger.warn("Unreal session ended with incomplete response data", {
                    remainder: this.buffer,
                });
            }
        });
        this.socket.on("close", () => {
            this.closed = true;
            if (this.pending.length > 0) {
                this.failAll(new Error("Unreal transport session closed before all responses were received."));
            }
        });
    }

    async connect(): Promise<void> {
        if (this.connected) {
            return;
        }

        await new Promise<void>((resolve, reject) => {
            const onError = (error: Error) => {
                this.socket.removeListener("connect", onConnect);
                reject(error);
            };
            const onConnect = () => {
                this.socket.removeListener("error", onError);
                this.connected = true;
                resolve();
            };

            this.socket.once("error", onError);
            this.socket.once("connect", onConnect);
            this.socket.connect(this.port, this.host);
        });
    }

    async sendCommand(command: string, params: Record<string, unknown> = {}): Promise<NormalizedUnrealResponse> {
        if (this.closed) {
            throw new Error("Unreal transport session is closed.");
        }

        await this.connect();

        const request = createCommandRequest(command, params);
        const startedAt = Date.now();

        return new Promise<NormalizedUnrealResponse>((resolve, reject) => {
            const timeout = setTimeout(() => {
                this.removePendingRequest(request.requestId);
                const error = new Error(
                    `Timeout receiving Unreal response for "${command}" after ${this.timeoutMs}ms`,
                );
                this.logger.error("Unreal command failed", {
                    command,
                    requestId: request.requestId,
                    durationMs: Date.now() - startedAt,
                    params,
                    error: error.message,
                });
                reject(error);
            }, this.timeoutMs);

            this.pending.push({
                command,
                params,
                requestId: request.requestId,
                startedAt,
                resolve,
                reject,
                timeout,
            });

            this.logger.debug("Sending Unreal command", {
                command,
                requestId: request.requestId,
                params,
            });

            this.socket.write(encodeDelimitedMessage(request));
        });
    }

    async close(): Promise<void> {
        if (this.closed) {
            return;
        }

        this.closed = true;
        await new Promise<void>((resolve) => {
            this.socket.end(() => resolve());
        });
        this.socket.destroy();
    }

    private handleData(chunk: Buffer): void {
        this.buffer += chunk.toString("utf-8");

        const extracted = extractDelimitedMessages(this.buffer);
        if (extracted.messages.length === 0 && hasDelimitedMessageTerminator(this.buffer)) {
            this.logger.warn("Received malformed Unreal response line", {
                rawBuffer: this.buffer,
            });
        }
        this.buffer = extracted.remainder;

        for (const message of extracted.messages) {
            const fallbackRequestId = this.pending[0]?.requestId ?? "";
            const normalized = normalizeUnrealResponse(message, fallbackRequestId);
            const pendingRequestIndex = this.pending.findIndex(
                (pendingRequest) => pendingRequest.requestId === normalized.requestId,
            );

            if (pendingRequestIndex === -1) {
                this.logger.warn("Received Unreal response for unknown request id", {
                    requestId: normalized.requestId,
                    response: message,
                });
                continue;
            }

            const [pendingRequest] = this.pending.splice(pendingRequestIndex, 1);
            clearTimeout(pendingRequest.timeout);

            this.logger.debug("Unreal command completed", {
                command: pendingRequest.command,
                requestId: normalized.requestId,
                durationMs: Date.now() - pendingRequest.startedAt,
                ok: normalized.ok,
                params: pendingRequest.params,
                response: normalized.ok
                    ? {
                          data: normalized.data,
                      }
                    : {
                          error: normalized.error,
                      },
            });

            pendingRequest.resolve(normalized);
        }
    }

    private removePendingRequest(requestId: string): void {
        const pendingRequestIndex = this.pending.findIndex((pendingRequest) => pendingRequest.requestId === requestId);
        if (pendingRequestIndex >= 0) {
            const [pendingRequest] = this.pending.splice(pendingRequestIndex, 1);
            clearTimeout(pendingRequest.timeout);
        }
    }

    private failAll(error: Error): void {
        if (this.pending.length === 0) {
            return;
        }

        const pendingRequests = this.pending.splice(0, this.pending.length);
        for (const pendingRequest of pendingRequests) {
            clearTimeout(pendingRequest.timeout);
            this.logger.error("Unreal command failed", {
                command: pendingRequest.command,
                requestId: pendingRequest.requestId,
                durationMs: Date.now() - pendingRequest.startedAt,
                params: pendingRequest.params,
                error: error.message,
            });
            pendingRequest.reject(error);
        }
    }
}

export class UnrealTransportClient {
    private readonly options: UnrealTransportClientOptions;

    constructor(options: UnrealTransportClientOptions) {
        this.options = options;
    }

    async sendCommand(command: string, params: Record<string, unknown> = {}): Promise<NormalizedUnrealResponse> {
        return this.withSession((session) => session.sendCommand(command, params));
    }

    async withSession<T>(callback: (session: { sendCommand: (command: string, params?: Record<string, unknown>) => Promise<NormalizedUnrealResponse> }) => Promise<T>): Promise<T> {
        const session = new UnrealTransportSession(this.options);

        try {
            await session.connect();
            return await callback(session);
        } finally {
            await session.close();
        }
    }
}
