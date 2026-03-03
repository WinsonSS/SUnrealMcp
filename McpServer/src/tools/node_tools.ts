import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { z } from "zod";
import { sendCommand } from "../unreal_connection.js";

const vec2 = () => z.tuple([z.number(), z.number()]);

function toText(response: Record<string, unknown>): { content: [{ type: "text"; text: string }] } {
    return { content: [{ type: "text", text: JSON.stringify(response) }] };
}

export function register(server: McpServer): void {
    server.registerTool(
        "add_blueprint_event_node",
        {
            description: "向 Blueprint 事件图添加事件节点，如 ReceiveBeginPlay、ReceiveTick",
            inputSchema: {
                blueprint_name: z.string().describe("目标 Blueprint 名称"),
                event_name: z.string().describe("事件名称，如 ReceiveBeginPlay、ReceiveTick"),
                node_position: vec2().default([0, 0]).describe("[X, Y] 节点在图中的位置"),
            },
        },
        async ({ blueprint_name, event_name, node_position }) => {
            const response = await sendCommand("add_blueprint_event_node", { blueprint_name, event_name, node_position });
            return toText(response);
        }
    );

    server.registerTool(
        "add_blueprint_input_action_node",
        {
            description: "向 Blueprint 事件图添加输入动作事件节点",
            inputSchema: {
                blueprint_name: z.string().describe("目标 Blueprint 名称"),
                action_name: z.string().describe("输入动作名称"),
                node_position: vec2().default([0, 0]).describe("[X, Y] 节点在图中的位置"),
            },
        },
        async ({ blueprint_name, action_name, node_position }) => {
            const response = await sendCommand("add_blueprint_input_action_node", { blueprint_name, action_name, node_position });
            return toText(response);
        }
    );

    server.registerTool(
        "add_blueprint_function_node",
        {
            description: "向 Blueprint 事件图添加函数调用节点",
            inputSchema: {
                blueprint_name: z.string().describe("目标 Blueprint 名称"),
                target: z.string().describe("函数目标对象，组件名称或 self"),
                function_name: z.string().describe("函数名称"),
                params: z.record(z.unknown()).default({}).describe("函数节点上的参数"),
                node_position: vec2().default([0, 0]).describe("[X, Y] 节点在图中的位置"),
            },
        },
        async ({ blueprint_name, target, function_name, params, node_position }) => {
            const response = await sendCommand("add_blueprint_function_node", { blueprint_name, target, function_name, params, node_position });
            return toText(response);
        }
    );

    server.registerTool(
        "connect_blueprint_nodes",
        {
            description: "连接 Blueprint 事件图中的两个节点",
            inputSchema: {
                blueprint_name: z.string().describe("目标 Blueprint 名称"),
                source_node_id: z.string().describe("源节点 ID"),
                source_pin: z.string().describe("源节点输出引脚名称"),
                target_node_id: z.string().describe("目标节点 ID"),
                target_pin: z.string().describe("目标节点输入引脚名称"),
            },
        },
        async ({ blueprint_name, source_node_id, source_pin, target_node_id, target_pin }) => {
            const response = await sendCommand("connect_blueprint_nodes", { blueprint_name, source_node_id, source_pin, target_node_id, target_pin });
            return toText(response);
        }
    );

    server.registerTool(
        "add_blueprint_variable",
        {
            description: "向 Blueprint 添加变量",
            inputSchema: {
                blueprint_name: z.string().describe("目标 Blueprint 名称"),
                variable_name: z.string().describe("变量名称"),
                variable_type: z.string().describe("变量类型，如 Boolean、Integer、Float、Vector"),
                is_exposed: z.boolean().default(false).describe("是否在编辑器中公开"),
            },
        },
        async ({ blueprint_name, variable_name, variable_type, is_exposed }) => {
            const response = await sendCommand("add_blueprint_variable", { blueprint_name, variable_name, variable_type, is_exposed });
            return toText(response);
        }
    );

    server.registerTool(
        "add_blueprint_get_self_component_reference",
        {
            description: "添加获取 Blueprint 自身组件引用的节点",
            inputSchema: {
                blueprint_name: z.string().describe("目标 Blueprint 名称"),
                component_name: z.string().describe("要获取引用的组件名称"),
                node_position: vec2().default([0, 0]).describe("[X, Y] 节点在图中的位置"),
            },
        },
        async ({ blueprint_name, component_name, node_position }) => {
            const response = await sendCommand("add_blueprint_get_self_component_reference", { blueprint_name, component_name, node_position });
            return toText(response);
        }
    );

    server.registerTool(
        "add_blueprint_self_reference",
        {
            description: "向 Blueprint 事件图添加 Get Self 节点",
            inputSchema: {
                blueprint_name: z.string().describe("目标 Blueprint 名称"),
                node_position: vec2().default([0, 0]).describe("[X, Y] 节点在图中的位置"),
            },
        },
        async ({ blueprint_name, node_position }) => {
            const response = await sendCommand("add_blueprint_self_reference", { blueprint_name, node_position });
            return toText(response);
        }
    );

    server.registerTool(
        "find_blueprint_nodes",
        {
            description: "查找 Blueprint 事件图中的节点",
            inputSchema: {
                blueprint_name: z.string().describe("目标 Blueprint 名称"),
                node_type: z.string().optional().describe("节点类型，如 Event、Function、Variable"),
                event_type: z.string().optional().describe("具体事件类型，如 BeginPlay、Tick"),
            },
        },
        async ({ blueprint_name, node_type, event_type }) => {
            const response = await sendCommand("find_blueprint_nodes", { blueprint_name, node_type, event_type });
            return toText(response);
        }
    );
}
