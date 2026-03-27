import { z } from "zod";
import { toResponseEnvelope } from "../protocol.js";
export function registerCommandTool(server, context, definition) {
    const { name, description, command = name } = definition;
    const inputSchema = definition.inputSchema;
    context.logger.debug("Registering tool", { tool: name, command });
    if (inputSchema) {
        registerCommandToolWithSchema(server, context, definition, inputSchema);
        return;
    }
    server.registerTool(name, { description }, async () => {
        const normalized = await context.client.sendCommand(command, {});
        const payload = toResponseEnvelope(normalized);
        return {
            content: [
                {
                    type: "text",
                    text: JSON.stringify({ tool: name, payload }, null, 2),
                },
            ],
            isError: !normalized.ok,
        };
    });
}
function registerCommandToolWithSchema(server, context, definition, inputSchema) {
    const { name, description, command = name } = definition;
    const callback = async (input) => {
        const plainInput = input;
        const params = definition.mapParams
            ? definition.mapParams(plainInput)
            : plainInput;
        const normalized = await context.client.sendCommand(command, params);
        const payload = definition.formatResponse?.(normalized, plainInput) ?? toResponseEnvelope(normalized);
        return {
            content: [
                {
                    type: "text",
                    text: JSON.stringify({ tool: name, payload }, null, 2),
                },
            ],
            isError: !normalized.ok,
        };
    };
    server.registerTool(name, {
        description,
        inputSchema,
    }, callback);
}
export function vec2Schema() {
    return z.tuple([z.number(), z.number()]);
}
export function vec3Schema() {
    return z.tuple([z.number(), z.number(), z.number()]);
}
export function colorSchema() {
    return z.tuple([z.number(), z.number(), z.number(), z.number()]);
}
