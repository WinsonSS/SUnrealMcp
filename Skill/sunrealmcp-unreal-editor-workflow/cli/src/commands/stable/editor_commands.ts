import { registerCommand, registerFamily } from "../../runtime/command_helpers.js";
import { CliCommandRegistry } from "../../types.js";

const FAMILY_NAME = "editor" as const;
const FAMILY_DESCRIPTION = "Level actor queries, spawning, deletion, transforms, and actor properties.";
const FAMILY_ORDER = 30;

export function register(registry: CliCommandRegistry): void {
    registerFamily(registry, {
        name: FAMILY_NAME,
        description: FAMILY_DESCRIPTION,
        order: FAMILY_ORDER,
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "get_actors_in_level",
        unrealCommand: "get_actors_in_level",
        description: "List all actors in the current level.",
        parameters: [],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "find_actors_by_label",
        unrealCommand: "find_actors_by_label",
        description: "Find actors by Actor Label pattern.",
        parameters: [
            { name: "pattern", type: "string", description: "Actor Label match pattern.", required: true },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "spawn_actor",
        unrealCommand: "spawn_actor",
        description: "Spawn a new actor in the current level.",
        parameters: [
            { name: "actor_label", type: "string", description: "Actor Label to assign after spawning.", required: true },
            {
                name: "actor_class",
                type: "string",
                description: "Actor class name or class path, for example StaticMeshActor or /Script/Engine.PointLight.",
                required: true,
            },
            { name: "location", type: "vec3", description: "[X, Y, Z] world location.", defaultValue: "0,0,0" },
            { name: "rotation", type: "vec3", description: "[Pitch, Yaw, Roll] rotation.", defaultValue: "0,0,0" },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "delete_actor",
        unrealCommand: "delete_actor",
        description: "Delete an actor by Actor Object Path.",
        parameters: [
            { name: "actor_path", type: "string", description: "Actor Object Path.", required: true },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "set_actor_transform",
        unrealCommand: "set_actor_transform",
        description: "Set an actor transform; location, rotation, and scale are all optional.",
        parameters: [
            { name: "actor_path", type: "string", description: "Actor Object Path.", required: true },
            { name: "location", type: "vec3", description: "[X, Y, Z] location." },
            { name: "rotation", type: "vec3", description: "[Pitch, Yaw, Roll] rotation." },
            { name: "scale", type: "vec3", description: "[X, Y, Z] scale." },
        ],
        mapParams(values) {
            const params: Record<string, unknown> = {
                actor_path: values.actor_path,
            };
            if (values.location !== undefined) params.location = values.location;
            if (values.rotation !== undefined) params.rotation = values.rotation;
            if (values.scale !== undefined) params.scale = values.scale;
            return params;
        },
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "get_actor_properties",
        unrealCommand: "get_actor_properties",
        description: "Get all properties of an actor.",
        parameters: [
            { name: "actor_path", type: "string", description: "Actor Object Path.", required: true },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "set_actor_property",
        unrealCommand: "set_actor_property",
        description: "Set a single property on an actor.",
        parameters: [
            { name: "actor_path", type: "string", description: "Actor Object Path.", required: true },
            { name: "property_name", type: "string", description: "Property name.", required: true },
            { name: "property_value", type: "json", description: "Property value.", required: true },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "spawn_blueprint_actor",
        unrealCommand: "spawn_blueprint_actor",
        description: "Spawn an actor instance from a Blueprint.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Blueprint asset path.", required: true },
            { name: "actor_label", type: "string", description: "Actor Label to assign after spawning.", required: true },
            { name: "location", type: "vec3", description: "[X, Y, Z] world location.", defaultValue: "0,0,0" },
            { name: "rotation", type: "vec3", description: "[Pitch, Yaw, Roll] rotation.", defaultValue: "0,0,0" },
        ],
    });
}
