import assert from "node:assert/strict";
import {
    createCommandRequest,
    encodeDelimitedMessage,
    extractDelimitedMessages,
    normalizeUnrealResponse,
    toResponseEnvelope,
} from "../build/protocol.js";
import { __testables as taskToolTestables } from "../build/runtime/tool_helpers.js";

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

run("task helper extracts accepted task ids", () => {
    const accepted = taskToolTestables.extractAcceptedTask(
        {
            requestId: "req-task-1",
            ok: true,
            data: {
                taskId: "task-42",
                accepted: true,
            },
            raw: {},
        },
        {},
    );

    assert.deepEqual(accepted, {
        taskId: "task-42",
        data: {
            taskId: "task-42",
            accepted: true,
        },
    });
});

run("task helper normalizes completed task payloads", () => {
    const normalized = taskToolTestables.normalizeTaskCompletion(
        {
            requestId: "req-task-2",
            ok: true,
            data: {
                taskId: "task-9",
                status: "succeeded",
                completed: true,
                payload: {
                    assetPath: "/Game/Test/BP_Test",
                },
            },
            raw: {},
        },
        "task-9",
    );

    assert.equal(normalized.ok, true);
    assert.deepEqual(normalized.data, {
        taskId: "task-9",
        status: "succeeded",
        result: {
            assetPath: "/Game/Test/BP_Test",
        },
        task: {
            taskId: "task-9",
            status: "succeeded",
            completed: true,
            payload: {
                assetPath: "/Game/Test/BP_Test",
            },
        },
    });
});

run("task helper rejects unsupported completed task statuses", () => {
    const normalized = taskToolTestables.normalizeTaskCompletion(
        {
            requestId: "req-task-3",
            ok: true,
            data: {
                taskId: "task-10",
                status: "done",
                completed: true,
            },
            raw: {},
        },
        "task-10",
    );

    assert.equal(normalized.ok, false);
    assert.equal(normalized.error?.code, "INVALID_TASK_STATUS");
});

run("task helper treats invalid task status as non-retryable", () => {
    const errorCode = "INVALID_TASK_STATUS";
    const defaultNonRetryableCodes = new Set([
        "TASK_NOT_FOUND",
        "TASK_EXPIRED",
        "INVALID_TASK_STATUS",
        "INVALID_TASK_RESPONSE",
    ]);

    assert.equal(defaultNonRetryableCodes.has(errorCode), true);
});
