import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { z } from "zod";
import { registerCommandTool, vec3Schema } from "../../runtime/tool_helpers.js";
import { ServerContext } from "../../types.js";

export function register(server: McpServer, context: ServerContext): void {
    registerCommandTool(server, context, {
        name: "create_blueprint",
        description: "Create a new Blueprint class",
        inputSchema: {
            blueprint_path: z.string().describe("Full asset path for the new Blueprint"),
            parent_class: z.string().describe("Parent class name or class path, for example Actor, Pawn, or Character"),
        },
    });

    registerCommandTool(server, context, {
        name: "add_component_to_blueprint",
        description: "Add a component to a Blueprint",
        inputSchema: {
            blueprint_path: z.string().describe("Target Blueprint asset path"),
            component_type: z.string().describe("Component type or class path, for example StaticMeshComponent without the U prefix"),
            component_name: z.string().describe("Name of the new component"),
            location: vec3Schema().optional().describe("[X, Y, Z] component location"),
            rotation: vec3Schema().optional().describe("[Pitch, Yaw, Roll] component rotation"),
            scale: vec3Schema().optional().describe("[X, Y, Z] component scale"),
            component_properties: z.record(z.unknown()).optional().describe("Additional component properties"),
        },
        mapParams: ({
            blueprint_path,
            component_type,
            component_name,
            location,
            rotation,
            scale,
            component_properties,
        }) => {
            const params: Record<string, unknown> = {
                blueprint_path,
                component_type,
                component_name,
                location: location ?? [0.0, 0.0, 0.0],
                rotation: rotation ?? [0.0, 0.0, 0.0],
                scale: scale ?? [1.0, 1.0, 1.0],
            };

            if (component_properties && Object.keys(component_properties).length > 0) {
                params.component_properties = component_properties;
            }

            return params;
        },
    });

    registerCommandTool(server, context, {
        name: "compile_blueprint",
        description: "Compile a Blueprint",
        inputSchema: {
            blueprint_path: z.string().describe("Blueprint asset path"),
        },
    });

    registerCommandTool(server, context, {
        name: "set_blueprint_property",
        description: "Set a property on the Blueprint class default object",
        inputSchema: {
            blueprint_path: z.string().describe("Target Blueprint asset path"),
            property_name: z.string().describe("Property name"),
            property_value: z.unknown().describe("Property value"),
        },
    });

    registerCommandTool(server, context, {
        name: "set_component_property",
        description: "Set a property on a component inside a Blueprint",
        inputSchema: {
            blueprint_path: z.string().describe("Target Blueprint asset path"),
            component_name: z.string().describe("Component name"),
            property_name: z.string().describe("Property name"),
            property_value: z.unknown().describe("Property value"),
        },
    });
}
