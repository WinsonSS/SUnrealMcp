import { registerCommand, registerFamily } from "../../runtime/command_helpers.js";
import { CliCommandRegistry } from "../../types.js";

const FAMILY_NAME = "node" as const;
const FAMILY_DESCRIPTION = "Blueprint graph nodes, connections, variables, graph inspection, and graph-level editing.";
const FAMILY_ORDER = 50;

export function register(registry: CliCommandRegistry): void {
    registerFamily(registry, {
        name: FAMILY_NAME,
        description: FAMILY_DESCRIPTION,
        order: FAMILY_ORDER,
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "add_blueprint_event_node",
        unrealCommand: "add_blueprint_event_node",
        description: "Add an event node to a Blueprint event graph.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Target Blueprint asset path.", required: true },
            { name: "event_name", type: "string", description: "Event name, for example ReceiveBeginPlay or ReceiveTick.", required: true },
            { name: "node_position", type: "vec2", description: "[X, Y] node position in the graph.", defaultValue: "0,0" },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "add_blueprint_function_node",
        unrealCommand: "add_blueprint_function_node",
        description: "Add a function call node to a Blueprint event graph.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Target Blueprint asset path.", required: true },
            { name: "target", type: "string", description: "Function target reference, such as self, a component name, or an object path.", required: true },
            { name: "function_name", type: "string", description: "Function name.", required: true },
            { name: "params", type: "json", description: "Parameters for the function node.", defaultValue: {} },
            { name: "node_position", type: "vec2", description: "[X, Y] node position in the graph.", defaultValue: "0,0" },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "connect_blueprint_nodes",
        unrealCommand: "connect_blueprint_nodes",
        description: "Connect two nodes in a Blueprint event graph.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Target Blueprint asset path.", required: true },
            { name: "source_node_id", type: "string", description: "Source node ID.", required: true },
            { name: "source_pin", type: "string", description: "Source node output pin name.", required: true },
            { name: "target_node_id", type: "string", description: "Target node ID.", required: true },
            { name: "target_pin", type: "string", description: "Target node input pin name.", required: true },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "add_blueprint_variable",
        unrealCommand: "add_blueprint_variable",
        description: "Add a variable to a Blueprint.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Target Blueprint asset path.", required: true },
            { name: "variable_name", type: "string", description: "Variable name.", required: true },
            {
                name: "variable_type",
                type: "enum",
                description: "Variable type.",
                required: true,
                enumValues: ["Boolean", "Integer", "Float", "Vector", "Rotator", "String", "Name", "Text"],
            },
            { name: "is_exposed", type: "boolean", description: "Whether the variable is exposed in the editor.", defaultValue: false },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "find_blueprint_nodes",
        unrealCommand: "find_blueprint_nodes",
        description: "Find nodes in a Blueprint event graph.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Target Blueprint asset path.", required: true },
            { name: "node_type", type: "string", description: "Node type, for example Event, Function, or Variable." },
            { name: "event_type", type: "string", description: "Specific event type, for example BeginPlay or Tick." },
        ],
        mapParams(values) {
            const params: Record<string, unknown> = {
                blueprint_path: values.blueprint_path,
            };
            if (values.node_type !== undefined) params.node_type = values.node_type;
            if (values.event_type !== undefined) params.event_type = values.event_type;
            return params;
        },
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "add_blueprint_input_action_node",
        unrealCommand: "add_blueprint_input_action_node",
        description: "Add an input action event node to a Blueprint event graph.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Target Blueprint asset path.", required: true },
            { name: "input_action_path", type: "string", description: "InputAction asset path.", required: true },
            { name: "node_position", type: "vec2", description: "[X, Y] node position in the graph.", defaultValue: "0,0" },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "add_blueprint_get_self_component_reference",
        unrealCommand: "add_blueprint_get_self_component_reference",
        description: "Add a node that gets a reference to a component owned by the Blueprint itself.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Target Blueprint asset path.", required: true },
            { name: "component_name", type: "string", description: "Component name to reference.", required: true },
            { name: "node_position", type: "vec2", description: "[X, Y] node position in the graph.", defaultValue: "0,0" },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "add_blueprint_self_reference",
        unrealCommand: "add_blueprint_self_reference",
        description: "Add a Get Self node to a Blueprint event graph.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Target Blueprint asset path.", required: true },
            { name: "node_position", type: "vec2", description: "[X, Y] node position in the graph.", defaultValue: "0,0" },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "inspect_blueprint_graph",
        unrealCommand: "inspect_blueprint_graph",
        description: "Inspect nodes, pins, and links in a selected Blueprint graph for graph analysis.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Target Blueprint asset path.", required: true },
            { name: "graph_type", type: "string", description: "Graph type, for example Event, Function, Macro, Delegate, or Collapsed." },
            { name: "graph_name", type: "string", description: "Graph name; for Collapsed graphs this can also be the source composite node id." },
            { name: "include_pins", type: "boolean", description: "Whether to include pin and link details.", defaultValue: true },
        ],
        mapParams(values) {
            const params: Record<string, unknown> = {
                blueprint_path: values.blueprint_path,
                include_pins: values.include_pins ?? true,
            };
            if (values.graph_type !== undefined) params.graph_type = values.graph_type;
            if (values.graph_name !== undefined) params.graph_name = values.graph_name;
            return params;
        },
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "list_blueprint_graphs",
        unrealCommand: "list_blueprint_graphs",
        description: "List event, function, macro, delegate, and collapsed graphs in a Blueprint.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Target Blueprint asset path.", required: true },
            { name: "include_collapsed", type: "boolean", description: "Whether to include collapsed graphs.", defaultValue: true },
        ],
        mapParams(values) {
            return {
                blueprint_path: values.blueprint_path,
                include_collapsed: values.include_collapsed ?? true,
            };
        },
    });
}
