import { registerCommand, registerFamily } from "../../runtime/command_helpers.js";
import { CliCommandRegistry } from "../../types.js";

const FAMILY_NAME = "blueprint" as const;
const FAMILY_DESCRIPTION = "Blueprint asset creation, compilation, component setup, and Blueprint property editing.";
const FAMILY_ORDER = 40;

export function register(registry: CliCommandRegistry): void {
    registerFamily(registry, {
        name: FAMILY_NAME,
        description: FAMILY_DESCRIPTION,
        order: FAMILY_ORDER,
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "create_blueprint",
        unrealCommand: "create_blueprint",
        description: "Create a new Blueprint class.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Full asset path for the new Blueprint.", required: true },
            {
                name: "parent_class",
                type: "string",
                description: "Parent class name or class path, for example Actor, Pawn, or Character.",
                required: true,
            },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "add_component_to_blueprint",
        unrealCommand: "add_component_to_blueprint",
        description: "Add a component to a Blueprint.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Target Blueprint asset path.", required: true },
            {
                name: "component_type",
                type: "string",
                description: "Component type or class path, for example StaticMeshComponent without the U prefix.",
                required: true,
            },
            { name: "component_name", type: "string", description: "Name of the new component.", required: true },
            { name: "location", type: "vec3", description: "[X, Y, Z] component location." },
            { name: "rotation", type: "vec3", description: "[Pitch, Yaw, Roll] component rotation." },
            { name: "scale", type: "vec3", description: "[X, Y, Z] component scale." },
            { name: "component_properties", type: "json", description: "Additional component properties." },
        ],
        mapParams(values) {
            const params: Record<string, unknown> = {
                blueprint_path: values.blueprint_path,
                component_type: values.component_type,
                component_name: values.component_name,
                location: values.location ?? [0.0, 0.0, 0.0],
                rotation: values.rotation ?? [0.0, 0.0, 0.0],
                scale: values.scale ?? [1.0, 1.0, 1.0],
            };

            const componentProperties = values.component_properties as Record<string, unknown> | undefined;
            if (componentProperties && Object.keys(componentProperties).length > 0) {
                params.component_properties = componentProperties;
            }

            return params;
        },
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "compile_blueprint",
        unrealCommand: "compile_blueprint",
        description: "Compile a Blueprint.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Blueprint asset path.", required: true },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "set_blueprint_property",
        unrealCommand: "set_blueprint_property",
        description: "Set a property on the Blueprint class default object.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Target Blueprint asset path.", required: true },
            { name: "property_name", type: "string", description: "Property name.", required: true },
            { name: "property_value", type: "json", description: "Property value.", required: true },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "set_component_property",
        unrealCommand: "set_component_property",
        description: "Set a property on a component inside a Blueprint.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Target Blueprint asset path.", required: true },
            { name: "component_name", type: "string", description: "Component name.", required: true },
            { name: "property_name", type: "string", description: "Property name.", required: true },
            { name: "property_value", type: "json", description: "Property value.", required: true },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "set_static_mesh_properties",
        unrealCommand: "set_static_mesh_properties",
        description: "Set static mesh properties on a StaticMeshComponent.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Target Blueprint asset path.", required: true },
            { name: "component_name", type: "string", description: "StaticMeshComponent name.", required: true },
            {
                name: "static_mesh",
                type: "string",
                description: "Static mesh asset path.",
                defaultValue: "/Engine/BasicShapes/Cube.Cube",
            },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "set_physics_properties",
        unrealCommand: "set_physics_properties",
        description: "Set physics properties on a component.",
        parameters: [
            { name: "blueprint_path", type: "string", description: "Target Blueprint asset path.", required: true },
            { name: "component_name", type: "string", description: "Component name.", required: true },
            { name: "simulate_physics", type: "boolean", description: "Whether to enable physics simulation.", defaultValue: true },
            { name: "gravity_enabled", type: "boolean", description: "Whether gravity is enabled.", defaultValue: true },
            { name: "mass", type: "number", description: "Mass in kilograms.", defaultValue: 1.0 },
            { name: "linear_damping", type: "number", description: "Linear damping.", defaultValue: 0.01 },
            { name: "angular_damping", type: "number", description: "Angular damping.", defaultValue: 0.0 },
        ],
    });
}
