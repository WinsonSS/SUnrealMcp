export const PROTOCOL_VERSION = 1;
const sessionToken = `session-${Date.now()}`;
let nextRequestSequence = 1;
export function createRequestId() {
    const requestId = `${sessionToken}-req-${nextRequestSequence}`;
    nextRequestSequence += 1;
    return requestId;
}
export function createCommandRequest(command, params, requestId = createRequestId()) {
    return {
        protocolVersion: PROTOCOL_VERSION,
        requestId,
        command,
        params,
    };
}
export function encodeDelimitedMessage(payload) {
    return `${JSON.stringify(payload)}\n`;
}
export function extractDelimitedMessages(buffer) {
    const lines = buffer.split("\n");
    const remainder = lines.pop() ?? "";
    const messages = [];
    for (const line of lines) {
        const trimmed = line.trim();
        if (!trimmed) {
            continue;
        }
        try {
            messages.push(JSON.parse(trimmed));
        }
        catch {
            messages.push({
                malformed: true,
                raw: trimmed,
            });
        }
    }
    return { messages, remainder };
}
export function normalizeUnrealResponse(raw, fallbackRequestId) {
    if (!isRecord(raw)) {
        return invalidResponse(raw, fallbackRequestId, "Unreal response was not an object.");
    }
    if (raw.protocolVersion !== PROTOCOL_VERSION ||
        typeof raw.requestId !== "string" ||
        typeof raw.ok !== "boolean") {
        return invalidResponse(raw, fallbackRequestId, "Unreal response did not match the protocol envelope.");
    }
    if (raw.ok) {
        return {
            requestId: raw.requestId,
            ok: true,
            data: raw.data,
            raw,
        };
    }
    return {
        requestId: raw.requestId,
        ok: false,
        error: isRecord(raw.error)
            ? {
                code: String(raw.error.code ?? "UNREAL_COMMAND_FAILED"),
                message: String(raw.error.message ?? "Unreal command failed."),
                details: raw.error.details,
            }
            : {
                code: "UNREAL_COMMAND_FAILED",
                message: "Unreal command failed.",
            },
        raw,
    };
}
function invalidResponse(raw, requestId, message) {
    return {
        requestId,
        ok: false,
        error: {
            code: "INVALID_UNREAL_RESPONSE",
            message,
            details: raw,
        },
        raw,
    };
}
function isRecord(value) {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}
