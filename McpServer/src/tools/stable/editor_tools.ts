import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { z } from "zod";
import { registerCommandTool, vec3Schema } from "../../runtime/tool_helpers.js";
import { ServerContext } from "../../types.js";

export function register(server: McpServer, context: ServerContext): void {
    registerCommandTool(server, context, {
        name: "get_actors_in_level",
        description: "List all actors in the current level",
    });

    registerCommandTool(server, context, {
        name: "find_actors_by_label",
        description: "Find actors by Actor Label pattern",
        inputSchema: {
            pattern: z.string().describe("Actor Label match pattern"),
        },
    });

    registerCommandTool(server, context, {
        name: "spawn_actor",
        description: "Spawn a new actor in the current level",
        inputSchema: {
            actor_label: z.string().describe("Actor Label to assign after spawning"),
            actor_class: z.string().describe("Actor class name or class path, for example StaticMeshActor or /Script/Engine.PointLight"),
            location: vec3Schema().default([0.0, 0.0, 0.0]).describe("[X, Y, Z] world location"),
            rotation: vec3Schema().default([0.0, 0.0, 0.0]).describe("[Pitch, Yaw, Roll] rotation"),
        },
    });

    registerCommandTool(server, context, {
        name: "delete_actor",
        description: "Delete an actor by Actor Object Path",
        inputSchema: {
            actor_path: z.string().describe("Actor Object Path"),
        },
    });

    registerCommandTool(server, context, {
        name: "set_actor_transform",
        description: "Set an actor's transform, including location, rotation, and scale; all transform fields are optional",
        inputSchema: {
            actor_path: z.string().describe("Actor Object Path"),
            location: vec3Schema().optional().describe("[X, Y, Z] location"),
            rotation: vec3Schema().optional().describe("[Pitch, Yaw, Roll] rotation"),
            scale: vec3Schema().optional().describe("[X, Y, Z] scale"),
        },
        mapParams: ({ actor_path, location, rotation, scale }) => {
            const params: Record<string, unknown> = { actor_path };
            if (location !== undefined) params.location = location;
            if (rotation !== undefined) params.rotation = rotation;
            if (scale !== undefined) params.scale = scale;
            return params;
        },
    });

    registerCommandTool(server, context, {
        name: "get_actor_properties",
        description: "Get all properties of an actor",
        inputSchema: {
            actor_path: z.string().describe("Actor Object Path"),
        },
    });

    registerCommandTool(server, context, {
        name: "set_actor_property",
        description: "Set a single property on an actor",
        inputSchema: {
            actor_path: z.string().describe("Actor Object Path"),
            property_name: z.string().describe("Property name"),
            property_value: z.unknown().describe("Property value"),
        },
    });

    registerCommandTool(server, context, {
        name: "spawn_blueprint_actor",
        description: "Spawn an actor instance from a Blueprint",
        inputSchema: {
            blueprint_path: z.string().describe("Blueprint asset path"),
            actor_label: z.string().describe("Actor Label to assign after spawning"),
            location: vec3Schema().default([0.0, 0.0, 0.0]).describe("[X, Y, Z] world location"),
            rotation: vec3Schema().default([0.0, 0.0, 0.0]).describe("[Pitch, Yaw, Roll] rotation"),
        },
    });
}
