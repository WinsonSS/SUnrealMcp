import { z } from "zod";
import { registerCommandTool } from "../../runtime/tool_helpers.js";
export function register(server, context) {
    registerCommandTool(server, context, {
        name: "add_enhanced_input_mapping",
        description: "Add a key mapping to an Enhanced Input Mapping Context",
        inputSchema: {
            mapping_context_path: z.string().describe("InputMappingContext asset path, for example /Game/Input/IMC_Player.IMC_Player"),
            input_action_path: z.string().describe("InputAction asset path, for example /Game/Input/IA_Jump.IA_Jump"),
            key: z.string().describe("Key to bind, for example SpaceBar or LeftMouseButton"),
        },
    });
}
