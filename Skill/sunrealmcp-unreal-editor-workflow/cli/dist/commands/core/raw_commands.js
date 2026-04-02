import { registerCommand, registerFamily } from "../../runtime/command_helpers.js";
const FAMILY_NAME = "raw";
const FAMILY_DESCRIPTION = "Low-level escape hatch for direct Unreal command execution.";
const FAMILY_ORDER = 90;
export function register(registry) {
    registerFamily(registry, {
        name: FAMILY_NAME,
        description: FAMILY_DESCRIPTION,
        order: FAMILY_ORDER,
    });
    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "core",
        cliCommand: "send",
        description: "Send any Unreal command directly when a friendly CLI wrapper does not exist yet.",
        parameters: [
            { name: "command", type: "string", description: "Unreal command name.", required: true },
            { name: "params-json", type: "json", description: "Raw params JSON.", defaultValue: {} },
        ],
        examples: [
            "sunrealmcp-cli raw send --project D:\\Projects\\MyGame --command inspect_blueprint_graph --params-json '{\"blueprint_path\":\"/Game/BP_Test.BP_Test\"}'",
        ],
        mapParams(values) {
            return values["params-json"] ?? {};
        },
    });
}
