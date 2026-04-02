import * as net from "net";
import {
    createCommandRequest,
    encodeDelimitedMessage,
    extractDelimitedMessages,
    normalizeUnrealResponse,
    type NormalizedUnrealResponse,
} from "../protocol.js";

export interface UnrealTransportClientOptions {
    host: string;
    port: number;
    timeoutMs: number;
}

class UnrealTransportSession {
    private readonly socket = new net.Socket();
    private readonly host: string;
    private readonly port: number;
    private readonly timeoutMs: number;
    private buffer = "";
    private connected = false;
    private closed = false;

    constructor(options: UnrealTransportClientOptions) {
        this.host = options.host;
        this.port = options.port;
        this.timeoutMs = options.timeoutMs;
        this.socket.setNoDelay(true);
    }

    async connect(): Promise<void> {
        if (this.connected) {
            return;
        }

        await new Promise<void>((resolve, reject) => {
            let settled = false;
            const timeout = setTimeout(() => {
                finish(() => {
                    this.socket.destroy();
                    reject(new Error(`Timeout connecting to Unreal at ${this.host}:${this.port} after ${this.timeoutMs}ms.`));
                });
            }, this.timeoutMs);

            const cleanup = () => {
                clearTimeout(timeout);
                this.socket.removeListener("error", onError);
                this.socket.removeListener("connect", onConnect);
            };

            const finish = (callback: () => void) => {
                if (settled) {
                    return;
                }
                settled = true;
                cleanup();
                callback();
            };

            const onError = (error: Error) => {
                finish(() => reject(error));
            };

            const onConnect = () => {
                finish(() => {
                    this.connected = true;
                    resolve();
                });
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

        return new Promise<NormalizedUnrealResponse>((resolve, reject) => {
            let settled = false;
            const timeout = setTimeout(() => {
                finish(() => {
                    this.socket.destroy();
                    reject(new Error(`Timeout receiving Unreal response for "${command}" after ${this.timeoutMs}ms.`));
                });
            }, this.timeoutMs);

            const cleanup = () => {
                clearTimeout(timeout);
                this.socket.removeListener("error", onError);
                this.socket.removeListener("close", onClose);
                this.socket.removeListener("data", onData);
            };

            const finish = (callback: () => void) => {
                if (settled) {
                    return;
                }
                settled = true;
                cleanup();
                callback();
            };

            const onError = (error: Error) => {
                finish(() => reject(error));
            };

            const onClose = () => {
                finish(() => reject(new Error(`Connection closed before receiving Unreal response for "${command}".`)));
            };

            const onData = (chunk: Buffer) => {
                this.buffer += chunk.toString("utf-8");
                const extracted = extractDelimitedMessages(this.buffer);
                this.buffer = extracted.remainder;
                if (extracted.messages.length === 0) {
                    return;
                }

                const normalized = normalizeUnrealResponse(extracted.messages[0], request.requestId);
                finish(() => resolve(normalized));
            };

            this.socket.once("error", onError);
            this.socket.once("close", onClose);
            this.socket.on("data", onData);

            this.socket.write(encodeDelimitedMessage(request));
        });
    }

    async close(): Promise<void> {
        if (this.closed) {
            return;
        }

        this.closed = true;
        this.socket.destroy();
    }
}

export class UnrealTransportClient {
    private readonly options: UnrealTransportClientOptions;

    constructor(options: UnrealTransportClientOptions) {
        this.options = options;
    }

    async sendCommand(command: string, params: Record<string, unknown> = {}): Promise<NormalizedUnrealResponse> {
        const session = new UnrealTransportSession(this.options);
        try {
            return await session.sendCommand(command, params);
        } finally {
            await session.close();
        }
    }
}
