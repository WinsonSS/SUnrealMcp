import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import { readdir } from "fs/promises";
import { fileURLToPath, pathToFileURL } from "url";
import { dirname, join } from "path";
const server = new McpServer({ name: "SUnrealMcp", version: "1.0.0" });
server.registerTool("ping", {
    description: "测试连通性"
}, async () => {
    return { content: [{ type: "text", text: "pong" }] };
});
async function registerAllTools(server) {
    const __dirname = dirname(fileURLToPath(import.meta.url));
    const toolsDir = join(__dirname, "tools");
    let files;
    try {
        files = await readdir(toolsDir);
    }
    catch {
        return;
    }
    for (const file of files) {
        if (!file.endsWith(".js"))
            continue;
        const mod = await import(pathToFileURL(join(toolsDir, file)).href);
        if (typeof mod.register === "function") {
            mod.register(server);
        }
    }
}
// Start the server
async function main() {
    await registerAllTools(server);
    const transport = new StdioServerTransport();
    await server.connect(transport);
    console.error("SUnrealMCP Server running on stdio");
}
main().catch((error) => {
    console.error("Fatal error in main():", error);
    process.exit(1);
});
