import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { z } from "zod";
import { registerCommandTool } from "../../runtime/tool_helpers.js";
import { ServerContext } from "../../types.js";

export function register(server: McpServer, context: ServerContext): void {
    registerCommandTool(server, context, {
        name: "add_enhanced_input_mapping",
        description: "向 Enhanced Input Mapping Context 添加按键映射",
        inputSchema: {
            mapping_context_path: z.string().describe("InputMappingContext 资产路径，如 /Game/Input/IMC_Player.IMC_Player"),
            input_action_path: z.string().describe("InputAction 资产路径，如 /Game/Input/IA_Jump.IA_Jump"),
            key: z.string().describe("绑定的按键，如 SpaceBar、LeftMouseButton"),
        },
    });
}
