import { z } from "zod";
import { registerCommandTool, vec2Schema } from "../runtime/tool_helpers.js";
export function register(server, context) {
    registerCommandTool(server, context, {
        name: "add_blueprint_event_node",
        description: "向 Blueprint 事件图添加事件节点，如 ReceiveBeginPlay、ReceiveTick",
        inputSchema: {
            blueprint_name: z.string().describe("目标 Blueprint 名称"),
            event_name: z.string().describe("事件名称，如 ReceiveBeginPlay、ReceiveTick"),
            node_position: vec2Schema().default([0, 0]).describe("[X, Y] 节点在图中的位置"),
        },
    });
    registerCommandTool(server, context, {
        name: "add_blueprint_input_action_node",
        description: "向 Blueprint 事件图添加输入动作事件节点",
        inputSchema: {
            blueprint_name: z.string().describe("目标 Blueprint 名称"),
            action_name: z.string().describe("输入动作名称"),
            node_position: vec2Schema().default([0, 0]).describe("[X, Y] 节点在图中的位置"),
        },
    });
    registerCommandTool(server, context, {
        name: "add_blueprint_function_node",
        description: "向 Blueprint 事件图添加函数调用节点",
        inputSchema: {
            blueprint_name: z.string().describe("目标 Blueprint 名称"),
            target: z.string().describe("函数目标对象，组件名称或 self"),
            function_name: z.string().describe("函数名称"),
            params: z.record(z.unknown()).default({}).describe("函数节点上的参数"),
            node_position: vec2Schema().default([0, 0]).describe("[X, Y] 节点在图中的位置"),
        },
    });
    registerCommandTool(server, context, {
        name: "connect_blueprint_nodes",
        description: "连接 Blueprint 事件图中的两个节点",
        inputSchema: {
            blueprint_name: z.string().describe("目标 Blueprint 名称"),
            source_node_id: z.string().describe("源节点 ID"),
            source_pin: z.string().describe("源节点输出引脚名称"),
            target_node_id: z.string().describe("目标节点 ID"),
            target_pin: z.string().describe("目标节点输入引脚名称"),
        },
    });
    registerCommandTool(server, context, {
        name: "add_blueprint_variable",
        description: "向 Blueprint 添加变量",
        inputSchema: {
            blueprint_name: z.string().describe("目标 Blueprint 名称"),
            variable_name: z.string().describe("变量名称"),
            variable_type: z.string().describe("变量类型，如 Boolean、Integer、Float、Vector"),
            is_exposed: z.boolean().default(false).describe("是否在编辑器中公开"),
        },
    });
    registerCommandTool(server, context, {
        name: "add_blueprint_get_self_component_reference",
        description: "添加获取 Blueprint 自身组件引用的节点",
        inputSchema: {
            blueprint_name: z.string().describe("目标 Blueprint 名称"),
            component_name: z.string().describe("要获取引用的组件名称"),
            node_position: vec2Schema().default([0, 0]).describe("[X, Y] 节点在图中的位置"),
        },
    });
    registerCommandTool(server, context, {
        name: "add_blueprint_self_reference",
        description: "向 Blueprint 事件图添加 Get Self 节点",
        inputSchema: {
            blueprint_name: z.string().describe("目标 Blueprint 名称"),
            node_position: vec2Schema().default([0, 0]).describe("[X, Y] 节点在图中的位置"),
        },
    });
    registerCommandTool(server, context, {
        name: "find_blueprint_nodes",
        description: "查找 Blueprint 事件图中的节点",
        inputSchema: {
            blueprint_name: z.string().describe("目标 Blueprint 名称"),
            node_type: z.string().optional().describe("节点类型，如 Event、Function、Variable"),
            event_type: z.string().optional().describe("具体事件类型，如 BeginPlay、Tick"),
        },
    });
}
