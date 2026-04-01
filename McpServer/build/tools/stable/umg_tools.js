import { z } from "zod";
import { registerCommandTool } from "../../runtime/tool_helpers.js";
export function register(server, context) {
    registerCommandTool(server, context, {
        name: "create_umg_widget_blueprint",
        description: "Create a new UMG Widget Blueprint",
        inputSchema: {
            widget_path: z.string().describe("Full Widget Blueprint asset path"),
            parent_class: z.string().default("UserWidget").describe("Parent class, default is UserWidget"),
        },
    });
}
