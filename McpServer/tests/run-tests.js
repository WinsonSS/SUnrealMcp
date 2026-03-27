import assert from "node:assert/strict";
import {
    createCommandRequest,
    encodeDelimitedMessage,
    extractDelimitedMessages,
    normalizeUnrealResponse,
    toResponseEnvelope,
} from "../build/protocol.js";

function run(name, fn) {
    try {
        fn();
        console.log(`PASS ${name}`);
    } catch (error) {
        console.error(`FAIL ${name}`);
        throw error;
    }
}

run("extractDelimitedMessages parses newline-delimited JSON messages", () => {
    const first = createCommandRequest("one", {});
    const second = createCommandRequest("two", { value: 2 });
    const combined = `${encodeDelimitedMessage(first)}${encodeDelimitedMessage(second)}partial`;

    const extracted = extractDelimitedMessages(combined);

    assert.equal(extracted.messages.length, 2);
    assert.deepEqual(extracted.messages[0], first);
    assert.deepEqual(extracted.messages[1], second);
    assert.equal(extracted.remainder, "partial");
});

run("normalizeUnrealResponse understands enveloped errors", () => {
    const normalized = normalizeUnrealResponse(
        {
            protocolVersion: 1,
            requestId: "req-1",
            ok: false,
            error: {
                code: "EDITOR_NOT_CONNECTED",
                message: "Editor is offline",
            },
        },
        "fallback",
    );

    assert.equal(normalized.ok, false);
    assert.equal(normalized.requestId, "req-1");
    assert.equal(normalized.error?.code, "EDITOR_NOT_CONNECTED");
});

run("normalizeUnrealResponse rejects non-enveloped payloads", () => {
    const normalized = normalizeUnrealResponse(
        {
            success: true,
            data: { legacy: true },
        },
        "fallback-id",
    );

    assert.equal(normalized.ok, false);
    assert.equal(normalized.requestId, "fallback-id");
    assert.equal(normalized.error?.code, "INVALID_UNREAL_RESPONSE");
});

run("toResponseEnvelope preserves successful normalized payloads", () => {
    const envelope = toResponseEnvelope({
        requestId: "req-2",
        ok: true,
        data: { actors: ["Cube"] },
        raw: { actors: ["Cube"] },
    });

    assert.deepEqual(envelope, {
        protocolVersion: 1,
        requestId: "req-2",
        ok: true,
        data: { actors: ["Cube"] },
    });
});
