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

export class UnrealTransportClient {
    private readonly host: string;
    private readonly port: number;
    private readonly timeoutMs: number;
    private readonly logger: Logger;

    constructor(options: UnrealTransportClientOptions) {
        this.host = options.host;
        this.port = options.port;
        this.timeoutMs = options.timeoutMs;
        this.logger = options.logger;
    }

    async sendCommand(command: string, params: Record<string, unknown> = {}): Promise<NormalizedUnrealResponse> {
        const startedAt = Date.now();

        return new Promise((resolve, reject) => {
            const socket = new net.Socket();
            const request = createCommandRequest(command, params);
            let settled = false;
            let buffer = "";

            const finish = (handler: () => void): void => {
                if (settled) {
                    return;
                }

                settled = true;
                socket.removeAllListeners();
                socket.destroy();
                handler();
            };

            const resolveWith = (raw: unknown): void => {
                const normalized = normalizeUnrealResponse(raw, request.requestId);
                this.logger.debug("Unreal command completed", {
                    command,
                    requestId: normalized.requestId,
                    durationMs: Date.now() - startedAt,
                    ok: normalized.ok,
                    params,
                    response: normalized.ok
                        ? {
                              data: normalized.data,
                          }
                        : {
                              error: normalized.error,
                          },
                });
                finish(() => resolve(normalized));
            };

            const rejectWith = (error: Error): void => {
                this.logger.error("Unreal command failed", {
                    command,
                    requestId: request.requestId,
                    durationMs: Date.now() - startedAt,
                    params,
                    error: error.message,
                });
                finish(() => reject(error));
            };

            socket.setNoDelay(true);
            socket.setTimeout(this.timeoutMs, () => {
                rejectWith(
                    new Error(
                        `Timeout receiving Unreal response for "${command}" after ${this.timeoutMs}ms`,
                    ),
                );
            });

            socket.on("data", (chunk: Buffer) => {
                buffer += chunk.toString("utf-8");

                const extracted = extractDelimitedMessages(buffer);
                if (extracted.messages.length === 0 && hasDelimitedMessageTerminator(buffer)) {
                    this.logger.warn("Received malformed Unreal response line", {
                        command,
                        requestId: request.requestId,
                        rawBuffer: buffer,
                    });
                }
                buffer = extracted.remainder;

                for (const message of extracted.messages) {
                    const normalized = normalizeUnrealResponse(message, request.requestId);
                    if (normalized.requestId === request.requestId) {
                        resolveWith(message);
                        return;
                    }
                }
            });

            socket.on("error", (error: Error) => {
                rejectWith(error);
            });

            socket.on("end", () => {
                if (buffer.trim().length > 0) {
                    this.logger.warn("Unreal connection ended with incomplete response data", {
                        command,
                        requestId: request.requestId,
                        remainder: buffer,
                    });
                }
            });

            socket.on("close", () => {
                if (!settled && buffer.trim().length > 0) {
                    this.logger.warn("Unreal connection closed before a full response was parsed", {
                        command,
                        requestId: request.requestId,
                        remainder: buffer,
                    });
                }
            });

            socket.connect(this.port, this.host, () => {
                this.logger.debug("Sending Unreal command", {
                    command,
                    requestId: request.requestId,
                    params,
                });

                socket.write(encodeDelimitedMessage(request));
            });
        });
    }
}
