import { registerCommand, registerFamily } from "../../runtime/command_helpers.js";
const FAMILY_NAME = "system";
const FAMILY_DESCRIPTION = "Connectivity checks, command registry reload, and task control.";
const FAMILY_ORDER = 20;
export function register(registry) {
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
        cliCommand: "reload-command-registry",
        unrealCommand: "reload_command_registry",
        description: "Reload the Unreal command registry after Live Coding or rebuild.",
        parameters: [],
    });
    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "core",
        cliCommand: "get-task-status",
        unrealCommand: "get_task_status",
        description: "Query a long-running Unreal task.",
        parameters: [
            { name: "task-id", type: "string", description: "Task id.", required: true },
        ],
    });
    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "core",
        cliCommand: "cancel-task",
        unrealCommand: "cancel_task",
        description: "Cancel a long-running Unreal task.",
        parameters: [
            { name: "task-id", type: "string", description: "Task id.", required: true },
        ],
    });
}
