import * as net from "net";
import { createCommandRequest, encodeDelimitedMessage, extractDelimitedMessages, normalizeUnrealResponse, } from "../protocol.js";
class UnrealTransportSession {
    socket = new net.Socket();
    host;
    port;
    timeoutMs;
    buffer = "";
    connected = false;
    closed = false;
    constructor(options) {
        this.host = options.host;
        this.port = options.port;
        this.timeoutMs = options.timeoutMs;
        this.socket.setNoDelay(true);
    }
    async connect() {
        if (this.connected) {
            return;
        }
        await new Promise((resolve, reject) => {
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
            const finish = (callback) => {
                if (settled) {
                    return;
                }
                settled = true;
                cleanup();
                callback();
            };
            const onError = (error) => {
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
    async sendCommand(command, params = {}) {
        if (this.closed) {
            throw new Error("Unreal transport session is closed.");
        }
        await this.connect();
        const request = createCommandRequest(command, params);
        return new Promise((resolve, reject) => {
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
            const finish = (callback) => {
                if (settled) {
                    return;
                }
                settled = true;
                cleanup();
                callback();
            };
            const onError = (error) => {
                finish(() => reject(error));
            };
            const onClose = () => {
                finish(() => reject(new Error(`Connection closed before receiving Unreal response for "${command}".`)));
            };
            const onData = (chunk) => {
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
    async close() {
        if (this.closed) {
            return;
        }
        this.closed = true;
        this.socket.destroy();
    }
}
export class UnrealTransportClient {
    options;
    constructor(options) {
        this.options = options;
    }
    async sendCommand(command, params = {}) {
        const session = new UnrealTransportSession(this.options);
        try {
            return await session.sendCommand(command, params);
        }
        finally {
            await session.close();
        }
    }
}
