import { McpServer, type ToolCallback } from "@modelcontextprotocol/sdk/server/mcp.js";
import { z } from "zod";
import { NormalizedUnrealResponse, toResponseEnvelope } from "../protocol.js";
import { CommandTaskOptions, CommandToolDefinition, ServerContext, ToolInputShape } from "../types.js";
import { DEFAULT_TASK_STATUS_RETRY_LIMIT } from "../index.js"

const DEFAULT_TASK_STATUS_COMMAND = "get_task_status";
const DEFAULT_TASK_CANCEL_COMMAND = "cancel_task";
const DEFAULT_TASK_ID_FIELD = "taskId";
const DEFAULT_TASK_POLL_INTERVAL_MS = 300;
const DEFAULT_TASK_CANCEL_ON_TIMEOUT = true;
const DEFAULT_NON_RETRYABLE_TASK_ERROR_CODES = new Set([
    "TASK_NOT_FOUND",
    "TASK_EXPIRED",
    "INVALID_TASK_STATUS",
    "INVALID_TASK_RESPONSE",
]);

interface TaskAcceptedPayload {
    taskId: string;
    data: Record<string, unknown>;
}

export function registerCommandTool(
    server: McpServer,
    context: ServerContext,
    definition: CommandToolDefinition,
): void {
    const { name, description } = definition;
    const inputSchema = definition.inputSchema;

    context.logger.debug("Registering tool", {
        tool: name,
        command: definition.command ?? name,
        executionMode: definition.executionMode ?? "sync",
    });

    if (inputSchema) {
        registerCommandToolWithSchema(server, context, definition, inputSchema);
        return;
    }

    server.registerTool(name, { description }, async () => {
        const plainInput: Record<string, unknown> = {};
        const normalized = await executeToolCommand(context, definition, plainInput);
        const payload =
            definition.formatResponse?.(normalized, plainInput) ?? buildDefaultPayload(definition, normalized);

        return {
            content: [
                {
                    type: "text" as const,
                    text: JSON.stringify({ tool: name, payload }, null, 2),
                },
            ],
            isError: !normalized.ok,
        };
    });
}

function registerCommandToolWithSchema(
    server: McpServer,
    context: ServerContext,
    definition: CommandToolDefinition,
    inputSchema: ToolInputShape,
): void {
    const { name, description } = definition;
    const callback: ToolCallback<ToolInputShape> = async (input) => {
        const plainInput = input as Record<string, unknown>;
        const normalized = await executeToolCommand(context, definition, plainInput);
        const payload =
            definition.formatResponse?.(normalized, plainInput) ?? buildDefaultPayload(definition, normalized);

        return {
            content: [
                {
                    type: "text" as const,
                    text: JSON.stringify({ tool: name, payload }, null, 2),
                },
            ],
            isError: !normalized.ok,
        };
    };

    server.registerTool(
        name,
        {
            description,
            inputSchema,
        },
        callback,
    );
}

async function executeToolCommand(
    context: ServerContext,
    definition: CommandToolDefinition,
    input: Record<string, unknown>,
): Promise<NormalizedUnrealResponse> {
    const command = definition.command ?? definition.name;
    const params = definition.mapParams ? definition.mapParams(input) : input;
    const executionMode = definition.executionMode ?? "sync";

    if (executionMode === "task") {
        return executeTaskCommand(context, definition, command, params);
    }

    return context.client.sendCommand(command, params);
}

async function executeTaskCommand(
    context: ServerContext,
    definition: CommandToolDefinition,
    command: string,
    params: Record<string, unknown>,
): Promise<NormalizedUnrealResponse> {
    return context.client.withSession(async (session) => {
        const accepted = await session.sendCommand(command, params);
        if (!accepted.ok) {
            return accepted;
        }

        const taskOptions = definition.taskOptions ?? {};
        const acceptedTask = extractAcceptedTask(accepted, taskOptions);
        if (!acceptedTask) {
            return {
                requestId: accepted.requestId,
                ok: false,
                error: {
                    code: "INVALID_TASK_RESPONSE",
                    message: `Task command "${definition.name}" did not return a task id.`,
                    details: accepted.raw,
                },
                raw: accepted.raw,
            };
        }

        if (taskOptions.waitForCompletion === false) {
            return accepted;
        }

        return pollTaskUntilCompletion(context, definition, acceptedTask.taskId, accepted, session);
    });
}

async function pollTaskUntilCompletion(
    context: ServerContext,
    definition: CommandToolDefinition,
    taskId: string,
    acceptedResponse: NormalizedUnrealResponse,
    session: { sendCommand: (command: string, params?: Record<string, unknown>) => Promise<NormalizedUnrealResponse> },
): Promise<NormalizedUnrealResponse> {
    const taskOptions = definition.taskOptions ?? {};
    const statusCommand = taskOptions.statusCommand ?? DEFAULT_TASK_STATUS_COMMAND;
    const cancelCommand = taskOptions.cancelCommand ?? DEFAULT_TASK_CANCEL_COMMAND;
    const pollIntervalMs = taskOptions.pollIntervalMs ?? DEFAULT_TASK_POLL_INTERVAL_MS;
    const timeoutMs = taskOptions.timeoutMs ?? context.config.taskTimeoutMs;
    const cancelOnTimeout = taskOptions.cancelOnTimeout ?? DEFAULT_TASK_CANCEL_ON_TIMEOUT;
    const statusRetryLimit = taskOptions.statusRetryLimit ?? context.config.taskStatusRetryLimit ?? DEFAULT_TASK_STATUS_RETRY_LIMIT;
    const nonRetryableErrorCodes = new Set([
        ...DEFAULT_NON_RETRYABLE_TASK_ERROR_CODES,
        ...(taskOptions.nonRetryableErrorCodes ?? []),
    ]);
    const startedAt = Date.now();
    let consecutiveStatusFailures = 0;

    while (Date.now() - startedAt <= timeoutMs) {
        await sleep(pollIntervalMs);

        try {
            const statusResponse = await session.sendCommand(statusCommand, { taskId });
            if (!statusResponse.ok) {
                const errorCode = statusResponse.error?.code;
                if (typeof errorCode === "string" && nonRetryableErrorCodes.has(errorCode)) {
                    return statusResponse;
                }

                consecutiveStatusFailures += 1;

                if (consecutiveStatusFailures > statusRetryLimit) {
                    return statusResponse;
                }

                context.logger.warn("Task status request returned an error; retrying", {
                    tool: definition.name,
                    taskId,
                    statusCommand,
                    attempt: consecutiveStatusFailures,
                    retryLimit: statusRetryLimit,
                    error: statusResponse.error,
                });
                continue;
            }

            consecutiveStatusFailures = 0;

            const taskData = asRecord(statusResponse.data);
            if (taskData?.completed === true) {
                return normalizeTaskCompletion(statusResponse, taskId);
            }
        } catch (error) {
            consecutiveStatusFailures += 1;

            if (consecutiveStatusFailures > statusRetryLimit) {
                throw error;
            }

            context.logger.warn("Task status request threw an error; retrying", {
                tool: definition.name,
                taskId,
                statusCommand,
                attempt: consecutiveStatusFailures,
                retryLimit: statusRetryLimit,
                error: error instanceof Error ? error.message : String(error),
            });
            continue;
        }
    }

    context.logger.warn("Timed out waiting for Unreal task completion", {
        tool: definition.name,
        taskId,
        timeoutMs,
        cancelOnTimeout,
        cancelCommand,
    });

    let cancelAttempt: {
        attempted: boolean;
        ok: boolean;
        response?: unknown;
        error?: string;
    } = {
        attempted: false,
        ok: false,
    };

    if (cancelOnTimeout) {
        cancelAttempt.attempted = true;

        try {
            const cancelResponse = await session.sendCommand(cancelCommand, { taskId });
            cancelAttempt = {
                attempted: true,
                ok: cancelResponse.ok,
                response: cancelResponse.ok ? cancelResponse.data : cancelResponse.error,
            };
        } catch (error) {
            cancelAttempt = {
                attempted: true,
                ok: false,
                error: error instanceof Error ? error.message : String(error),
            };
        }
    }

    return {
        requestId: acceptedResponse.requestId,
        ok: false,
        error: {
            code: "TASK_TIMEOUT",
            message: `Task "${taskId}" did not complete within ${timeoutMs}ms.`,
            details: {
                taskId,
                statusCommand,
                cancelCommand,
                cancelAttempt,
            },
        },
        raw: acceptedResponse.raw,
    };
}

function extractAcceptedTask(
    response: NormalizedUnrealResponse,
    taskOptions: CommandTaskOptions,
): TaskAcceptedPayload | null {
    const data = asRecord(response.data);
    if (!data) {
        return null;
    }

    const taskIdField = taskOptions.taskIdField ?? DEFAULT_TASK_ID_FIELD;
    const taskId = data[taskIdField];
    if (typeof taskId !== "string" || taskId.length === 0) {
        return null;
    }

    return {
        taskId,
        data,
    };
}

function normalizeTaskCompletion(
    statusResponse: NormalizedUnrealResponse,
    taskId: string,
): NormalizedUnrealResponse {
    const data = asRecord(statusResponse.data);
    if (!data) {
        return {
            requestId: statusResponse.requestId,
            ok: false,
            error: {
                code: "INVALID_TASK_STATUS",
                message: `Task "${taskId}" returned an invalid status payload.`,
                details: statusResponse.raw,
            },
            raw: statusResponse.raw,
        };
    }

    const status = data.status;
    if (typeof status !== "string") {
        return {
            requestId: statusResponse.requestId,
            ok: false,
            error: {
                code: "INVALID_TASK_STATUS",
                message: `Task "${taskId}" did not report a valid string status.`,
                details: data,
            },
            raw: statusResponse.raw,
        };
    }

    if (status === "failed" || status === "cancelled") {
        const embeddedError = asRecord(data.error);
        return {
            requestId: statusResponse.requestId,
            ok: false,
            error: {
                code:
                    typeof embeddedError?.code === "string"
                        ? embeddedError.code
                        : status === "cancelled"
                          ? "TASK_CANCELLED"
                          : "TASK_FAILED",
                message:
                    typeof embeddedError?.message === "string"
                        ? embeddedError.message
                        : `Task "${taskId}" finished with status "${String(status)}".`,
                details: data,
            },
            raw: statusResponse.raw,
        };
    }

    if (status !== "succeeded") {
        return {
            requestId: statusResponse.requestId,
            ok: false,
            error: {
                code: "INVALID_TASK_STATUS",
                message: `Task "${taskId}" completed with unsupported status "${status}".`,
                details: data,
            },
            raw: statusResponse.raw,
        };
    }

    return {
        requestId: statusResponse.requestId,
        ok: true,
        data: {
            taskId,
            status,
            result: data.payload ?? null,
            task: data,
        },
        raw: statusResponse.raw,
    };
}

function buildDefaultPayload(
    definition: CommandToolDefinition,
    normalized: NormalizedUnrealResponse,
): unknown {
    if (definition.executionMode === "task" && normalized.ok) {
        return {
            protocol: toResponseEnvelope(normalized),
            task: normalized.data,
        };
    }

    return toResponseEnvelope(normalized);
}

function asRecord(value: unknown): Record<string, unknown> | null {
    return typeof value === "object" && value !== null && !Array.isArray(value)
        ? (value as Record<string, unknown>)
        : null;
}

function sleep(delayMs: number): Promise<void> {
    return new Promise((resolve) => {
        setTimeout(resolve, delayMs);
    });
}

export const __testables = {
    extractAcceptedTask,
    normalizeTaskCompletion,
};

export function vec2Schema() {
    return z.tuple([z.number(), z.number()]);
}

export function vec3Schema() {
    return z.tuple([z.number(), z.number(), z.number()]);
}

export function colorSchema() {
    return z.tuple([z.number(), z.number(), z.number(), z.number()]);
}
