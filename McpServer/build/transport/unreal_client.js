import * as net from "net";
import { createCommandRequest, encodeDelimitedMessage, extractDelimitedMessages, hasDelimitedMessageTerminator, normalizeUnrealResponse, } from "../protocol.js";
class UnrealTransportSession {
    socket = new net.Socket();
    host;
    port;
    timeoutMs;
    logger;
    buffer = "";
    connected = false;
    closed = false;
    pending = [];
    constructor(options) {
        this.host = options.host;
        this.port = options.port;
        this.timeoutMs = options.timeoutMs;
        this.logger = options.logger;
        this.socket.setNoDelay(true);
        this.socket.on("data", (chunk) => {
            this.handleData(chunk);
        });
        this.socket.on("error", (error) => {
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
    async connect() {
        if (this.connected) {
            return;
        }
        await new Promise((resolve, reject) => {
            const onError = (error) => {
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
    async sendCommand(command, params = {}) {
        if (this.closed) {
            throw new Error("Unreal transport session is closed.");
        }
        await this.connect();
        const request = createCommandRequest(command, params);
        const startedAt = Date.now();
        return new Promise((resolve, reject) => {
            const timeout = setTimeout(() => {
                this.removePendingRequest(request.requestId);
                const error = new Error(`Timeout receiving Unreal response for "${command}" after ${this.timeoutMs}ms`);
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
    async close() {
        if (this.closed) {
            return;
        }
        this.closed = true;
        await new Promise((resolve) => {
            this.socket.end(() => resolve());
        });
        this.socket.destroy();
    }
    handleData(chunk) {
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
            const pendingRequestIndex = this.pending.findIndex((pendingRequest) => pendingRequest.requestId === normalized.requestId);
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
    removePendingRequest(requestId) {
        const pendingRequestIndex = this.pending.findIndex((pendingRequest) => pendingRequest.requestId === requestId);
        if (pendingRequestIndex >= 0) {
            const [pendingRequest] = this.pending.splice(pendingRequestIndex, 1);
            clearTimeout(pendingRequest.timeout);
        }
    }
    failAll(error) {
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
    options;
    constructor(options) {
        this.options = options;
    }
    async sendCommand(command, params = {}) {
        return this.withSession((session) => session.sendCommand(command, params));
    }
    async withSession(callback) {
        const session = new UnrealTransportSession(this.options);
        try {
            await session.connect();
            return await callback(session);
        }
        finally {
            await session.close();
        }
    }
}
