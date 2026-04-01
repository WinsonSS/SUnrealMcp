import { z } from "zod";
import { registerCommandTool, vec2Schema } from "../../runtime/tool_helpers.js";
export function register(server, context) {
    registerCommandTool(server, context, {
        name: "add_blueprint_event_node",
        description: "Add an event node to a Blueprint event graph, such as ReceiveBeginPlay or ReceiveTick",
        inputSchema: {
            blueprint_path: z.string().describe("Target Blueprint asset path"),
            event_name: z.string().describe("Event name, for example ReceiveBeginPlay or ReceiveTick"),
            node_position: vec2Schema().default([0, 0]).describe("[X, Y] node position in the graph"),
        },
    });
    registerCommandTool(server, context, {
        name: "add_blueprint_function_node",
        description: "Add a function call node to a Blueprint event graph",
        inputSchema: {
            blueprint_path: z.string().describe("Target Blueprint asset path"),
            target: z.string().describe("Function target reference, such as self, a component name, or an object path"),
            function_name: z.string().describe("Function name"),
            params: z.record(z.unknown()).default({}).describe("Parameters for the function node"),
            node_position: vec2Schema().default([0, 0]).describe("[X, Y] node position in the graph"),
        },
    });
    registerCommandTool(server, context, {
        name: "connect_blueprint_nodes",
        description: "Connect two nodes in a Blueprint event graph",
        inputSchema: {
            blueprint_path: z.string().describe("Target Blueprint asset path"),
            source_node_id: z.string().describe("Source node ID"),
            source_pin: z.string().describe("Source node output pin name"),
            target_node_id: z.string().describe("Target node ID"),
            target_pin: z.string().describe("Target node input pin name"),
        },
    });
    registerCommandTool(server, context, {
        name: "add_blueprint_variable",
        description: "Add a variable to a Blueprint",
        inputSchema: {
            blueprint_path: z.string().describe("Target Blueprint asset path"),
            variable_name: z.string().describe("Variable name"),
            variable_type: z.enum([
                "Boolean",
                "Integer",
                "Float",
                "Vector",
                "Rotator",
                "String",
                "Name",
                "Text",
            ]).describe("Variable type"),
            is_exposed: z.boolean().default(false).describe("Whether the variable is exposed in the editor"),
        },
    });
    registerCommandTool(server, context, {
        name: "find_blueprint_nodes",
        description: "Find nodes in a Blueprint event graph",
        inputSchema: {
            blueprint_path: z.string().describe("Target Blueprint asset path"),
            node_type: z.string().optional().describe("Node type, for example Event, Function, or Variable"),
            event_type: z.string().optional().describe("Specific event type, for example BeginPlay or Tick"),
        },
    });
}
