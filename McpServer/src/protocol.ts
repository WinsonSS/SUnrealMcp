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

export function tryParseJsonMessage(raw: string): unknown | null {
    const trimmed = raw.trim();
    if (!trimmed) {
        return null;
    }

    try {
        return JSON.parse(trimmed) as unknown;
    } catch {
        return null;
    }
}

export function extractDelimitedMessages(buffer: string): { messages: unknown[]; remainder: string } {
    const lines = buffer.split("\n");
    const remainder = lines.pop() ?? "";
    const messages: unknown[] = [];

    for (const line of lines) {
        const parsed = tryParseJsonMessage(line);
        if (parsed !== null) {
            messages.push(parsed);
        }
    }

    return { messages, remainder };
}

export function hasDelimitedMessageTerminator(buffer: string): boolean {
    return buffer.includes("\n");
}

export function normalizeUnrealResponse(raw: unknown, fallbackRequestId: string): NormalizedUnrealResponse {
    if (!isRecord(raw)) {
        return {
            requestId: fallbackRequestId,
            ok: false,
            error: {
                code: "INVALID_UNREAL_RESPONSE",
                message: "Unreal response was not an object",
                details: raw,
            },
            raw,
        };
    }

    if (isEnvelope(raw)) {
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
            error: raw.error,
            raw,
        };
    }

    const requestId = typeof raw.requestId === "string" ? raw.requestId : fallbackRequestId;

    return {
        requestId,
        ok: false,
        error: {
            code: "INVALID_UNREAL_RESPONSE",
            message: "Unreal response did not match the enveloped protocol",
            details: raw,
        },
        raw,
    };
}

export function toResponseEnvelope(response: NormalizedUnrealResponse): UnrealResponseEnvelope {
    if (response.ok) {
        return {
            protocolVersion: PROTOCOL_VERSION,
            requestId: response.requestId,
            ok: true,
            data: response.data,
        };
    }

    return {
        protocolVersion: PROTOCOL_VERSION,
        requestId: response.requestId,
        ok: false,
        error: response.error ?? {
            code: "UNREAL_COMMAND_FAILED",
            message: "Unreal command failed",
        },
    };
}

function isEnvelope(value: unknown): value is UnrealResponseEnvelope {
    if (!isRecord(value)) {
        return false;
    }

    if (
        value.protocolVersion !== PROTOCOL_VERSION ||
        typeof value.requestId !== "string" ||
        typeof value.ok !== "boolean"
    ) {
        return false;
    }

    if (value.ok) {
        return "data" in value;
    }

    return isErrorPayload(value.error);
}

function isRecord(value: unknown): value is Record<string, unknown> {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}

function isErrorPayload(value: unknown): value is UnrealErrorPayload {
    return isRecord(value) && typeof value.code === "string" && typeof value.message === "string";
}
