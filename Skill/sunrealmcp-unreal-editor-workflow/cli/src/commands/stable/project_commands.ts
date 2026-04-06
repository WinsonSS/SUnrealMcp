import { registerCommand, registerFamily } from "../../runtime/command_helpers.js";
import { CliCommandRegistry } from "../../types.js";

const FAMILY_NAME = "project" as const;
const FAMILY_DESCRIPTION = "Project-level assets and configuration-like UE objects.";
const FAMILY_ORDER = 70;

export function register(registry: CliCommandRegistry): void {
    registerFamily(registry, {
        name: FAMILY_NAME,
        description: FAMILY_DESCRIPTION,
        order: FAMILY_ORDER,
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "add_enhanced_input_mapping",
        unrealCommand: "add_enhanced_input_mapping",
        description: "Add a key mapping to an Enhanced Input Mapping Context.",
        parameters: [
            { name: "mapping_context_path", type: "string", description: "InputMappingContext asset path.", required: true },
            { name: "input_action_path", type: "string", description: "InputAction asset path.", required: true },
            { name: "key", type: "string", description: "Key to bind, for example SpaceBar or LeftMouseButton.", required: true },
        ],
    });
}
