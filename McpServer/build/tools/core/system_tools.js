import { registerCommandTool } from "../../runtime/tool_helpers.js";
export function register(server, context) {
    registerCommandTool(server, context, {
        name: "reload_command_registry",
        description: "Rebuild the active Unreal plugin command registry so commands added or changed after Live Coding become available at runtime",
    });
}
