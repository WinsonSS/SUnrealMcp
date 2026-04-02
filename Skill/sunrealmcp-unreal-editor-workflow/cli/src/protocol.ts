export type UnrealCommandParams = Record<string, unknown>;
export const PROTOCOL_VERSION = 1 as const;

export interface UnrealCommandRequest {
    protocolVersion: typeof PROTOCOL_VERSION;
    requestId: string;
    command: string;
    params: UnrealCommandParams;
}

export interface UnrealSuccessResponse {
    protocolVersion: typeof PROTOCOL_VERSION;
    requestId: string;
    ok: true;
    data: unknown;
}

export interface UnrealErrorPayload {
    code: string;
    message: string;
    details?: unknown;
}

export interface UnrealErrorResponse {
    protocolVersion: typeof PROTOCOL_VERSION;
    requestId: string;
    ok: false;
    error: UnrealErrorPayload;
}

export type UnrealResponseEnvelope = UnrealSuccessResponse | UnrealErrorResponse;

export interface NormalizedUnrealResponse {
    requestId: string;
    ok: boolean;
    data?: unknown;
    error?: UnrealErrorPayload;
    raw: unknown;
}

const sessionToken = `session-${Date.now()}`;
let nextRequestSequence = 1;

export function createRequestId(): string {
    const requestId = `${sessionToken}-req-${nextRequestSequence}`;
    nextRequestSequence += 1;
    return requestId;
}

export function createCommandRequest(
    command: string,
    params: UnrealCommandParams,
    requestId = createRequestId(),
): UnrealCommandRequest {
    return {
        protocolVersion: PROTOCOL_VERSION,
        requestId,
        command,
        params,
    };
}

export function encodeDelimitedMessage(payload: unknown): string {
    return `${JSON.stringify(payload)}\n`;
}

export function extractDelimitedMessages(buffer: string): { messages: unknown[]; remainder: string } {
    const lines = buffer.split("\n");
    const remainder = lines.pop() ?? "";
    const messages: unknown[] = [];

    for (const line of lines) {
        const trimmed = line.trim();
        if (!trimmed) {
            continue;
        }

        try {
            messages.push(JSON.parse(trimmed) as unknown);
        } catch {
            messages.push({
                malformed: true,
                raw: trimmed,
            });
        }
    }

    return { messages, remainder };
}

export function normalizeUnrealResponse(raw: unknown, fallbackRequestId: string): NormalizedUnrealResponse {
    if (!isRecord(raw)) {
        return invalidResponse(raw, fallbackRequestId, "Unreal response was not an object.");
    }

    if (
        raw.protocolVersion !== PROTOCOL_VERSION ||
        typeof raw.requestId !== "string" ||
        typeof raw.ok !== "boolean"
    ) {
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

function invalidResponse(raw: unknown, requestId: string, message: string): NormalizedUnrealResponse {
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

function isRecord(value: unknown): value is Record<string, any> {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}
