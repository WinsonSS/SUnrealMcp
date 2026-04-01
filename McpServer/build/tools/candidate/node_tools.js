import { z } from "zod";
import { registerCommandTool, vec2Schema } from "../../runtime/tool_helpers.js";
export function register(server, context) {
    registerCommandTool(server, context, {
        name: "add_blueprint_input_action_node",
        description: "Add an input action event node to a Blueprint event graph",
        inputSchema: {
            blueprint_path: z.string().describe("Target Blueprint asset path"),
            input_action_path: z.string().describe("InputAction asset path"),
            node_position: vec2Schema().default([0, 0]).describe("[X, Y] node position in the graph"),
        },
    });
    registerCommandTool(server, context, {
        name: "add_blueprint_get_self_component_reference",
        description: "Add a node that gets a reference to a component owned by the Blueprint itself",
        inputSchema: {
            blueprint_path: z.string().describe("Target Blueprint asset path"),
            component_name: z.string().describe("Component name to reference"),
            node_position: vec2Schema().default([0, 0]).describe("[X, Y] node position in the graph"),
        },
    });
    registerCommandTool(server, context, {
        name: "add_blueprint_self_reference",
        description: "Add a Get Self node to a Blueprint event graph",
        inputSchema: {
            blueprint_path: z.string().describe("Target Blueprint asset path"),
            node_position: vec2Schema().default([0, 0]).describe("[X, Y] node position in the graph"),
        },
    });
    registerCommandTool(server, context, {
        name: "inspect_blueprint_graph",
        description: "Inspect nodes, pins, and links in a selected Blueprint graph for graph analysis",
        inputSchema: {
            blueprint_path: z.string().describe("Target Blueprint asset path"),
            graph_type: z.string().optional().describe("Graph type, for example Event, Function, Macro, or Delegate"),
            graph_name: z.string().optional().describe("Graph name; when reading a function graph this is usually the function name"),
            include_pins: z.boolean().default(true).describe("Whether to include pin and link details"),
        },
    });
}
