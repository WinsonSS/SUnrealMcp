import { registerCommand, registerFamily } from "../../runtime/command_helpers.js";
import { CliCommandRegistry } from "../../types.js";

const FAMILY_NAME = "system" as const;
const FAMILY_DESCRIPTION = "Connectivity checks, command registry reload, and task control.";
const FAMILY_ORDER = 20;

export function register(registry: CliCommandRegistry): void {
    registerFamily(registry, {
        name: FAMILY_NAME,
        description: FAMILY_DESCRIPTION,
        order: FAMILY_ORDER,
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "core",
        cliCommand: "ping",
        unrealCommand: "ping",
        description: "Ping the Unreal-side server for the resolved target.",
        parameters: [],
        examples: [
            "sunrealmcp-cli system ping --project D:\\Projects\\MyGame",
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "core",
        cliCommand: "reload_command_registry",
        unrealCommand: "reload_command_registry",
        description: "Reload the Unreal command registry after Live Coding or rebuild.",
        parameters: [],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "core",
        cliCommand: "get_task_status",
        unrealCommand: "get_task_status",
        description: "Query a long-running Unreal task.",
        parameters: [
            { name: "task_id", type: "string", description: "Task id.", required: true },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "core",
        cliCommand: "cancel_task",
        unrealCommand: "cancel_task",
        description: "Cancel a long-running Unreal task.",
        parameters: [
            { name: "task_id", type: "string", description: "Task id.", required: true },
        ],
    });
}
