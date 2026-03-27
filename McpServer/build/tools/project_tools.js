import { z } from "zod";
import { registerCommandTool } from "../runtime/tool_helpers.js";
export function register(server, context) {
    registerCommandTool(server, context, {
        name: "create_input_mapping",
        description: "为项目创建输入映射，将按键绑定到输入动作",
        inputSchema: {
            action_name: z.string().describe("输入动作名称"),
            key: z.string().describe("绑定的按键，如 SpaceBar、LeftMouseButton"),
            input_type: z.enum(["Action", "Axis"]).default("Action").describe("输入映射类型"),
        },
    });
}
