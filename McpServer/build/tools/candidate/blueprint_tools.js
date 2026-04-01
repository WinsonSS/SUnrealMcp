import { z } from "zod";
import { registerCommandTool } from "../../runtime/tool_helpers.js";
export function register(server, context) {
    registerCommandTool(server, context, {
        name: "set_static_mesh_properties",
        description: "Set static mesh properties on a StaticMeshComponent",
        inputSchema: {
            blueprint_path: z.string().describe("Target Blueprint asset path"),
            component_name: z.string().describe("StaticMeshComponent name"),
            static_mesh: z.string().default("/Engine/BasicShapes/Cube.Cube").describe("Static mesh asset path"),
        },
    });
    registerCommandTool(server, context, {
        name: "set_physics_properties",
        description: "Set physics properties on a component",
        inputSchema: {
            blueprint_path: z.string().describe("Target Blueprint asset path"),
            component_name: z.string().describe("Component name"),
            simulate_physics: z.boolean().default(true).describe("Whether to enable physics simulation"),
            gravity_enabled: z.boolean().default(true).describe("Whether gravity is enabled"),
            mass: z.number().default(1.0).describe("Mass in kilograms"),
            linear_damping: z.number().default(0.01).describe("Linear damping"),
            angular_damping: z.number().default(0.0).describe("Angular damping"),
        },
    });
}
