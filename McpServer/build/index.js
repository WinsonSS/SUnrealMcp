import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import { readFile, readdir } from "fs/promises";
import { dirname, join } from "path";
import { fileURLToPath, pathToFileURL } from "url";
import { z } from "zod";
import { PROTOCOL_VERSION } from "./protocol.js";
import { UnrealTransportClient } from "./transport/unreal_client.js";
const ConfigFileSchema = z.object({
    host: z.string().min(1).optional(),
    port: z.number().int().positive().optional(),
    timeoutMs: z.number().int().positive().optional(),
    taskTimeoutMs: z.number().int().positive().optional(),
    taskStatusRetryLimit: z.number().int().nonnegative().optional(),
}).strict();
function createLogger() {
    const write = (level) => (message, meta) => {
        const payload = meta ? ` ${JSON.stringify(meta)}` : "";
        console.error(`[SUnrealMcp][${level}] ${message}${payload}`);
    };
    return {
        debug: write("debug"),
        info: write("info"),
        warn: write("warn"),
        error: write("error"),
    };
}
async function readJsonConfigFile(filePath) {
    try {
        const raw = await readFile(filePath, "utf-8");
        const parsed = JSON.parse(raw);
        return ConfigFileSchema.parse(parsed);
    }
    catch (error) {
        if (error.code === "ENOENT") {
            return {};
        }
        throw new Error(`Failed to read config file "${filePath}": ${error instanceof Error ? error.message : String(error)}`);
    }
}
function toFiniteNumber(value, fallback) {
    const parsed = Number.parseInt(value ?? "", 10);
    return Number.isFinite(parsed) ? parsed : fallback;
}
async function readConfig() {
    const __dirname = dirname(fileURLToPath(import.meta.url));
    const rootDir = join(__dirname, "..");
    const defaultConfigPath = join(rootDir, "config.json");
    const localConfigPath = join(rootDir, "config.local.json");
    const fileConfig = await readJsonConfigFile(defaultConfigPath);
    const localConfig = await readJsonConfigFile(localConfigPath);
    const mergedConfig = {
        ...fileConfig,
        ...localConfig,
    };
    return {
        host: mergedConfig.host ?? "127.0.0.1",
        port: toFiniteNumber(mergedConfig.port !== undefined ? String(mergedConfig.port) : undefined, 55557),
        timeoutMs: toFiniteNumber(mergedConfig.timeoutMs !== undefined ? String(mergedConfig.timeoutMs) : undefined, 5000),
        taskTimeoutMs: toFiniteNumber(mergedConfig.taskTimeoutMs !== undefined ? String(mergedConfig.taskTimeoutMs) : undefined, 30000),
        taskStatusRetryLimit: toFiniteNumber(mergedConfig.taskStatusRetryLimit !== undefined ? String(mergedConfig.taskStatusRetryLimit) : undefined, 3),
    };
}
const logger = createLogger();
const server = new McpServer({ name: "SUnrealMcp", version: "2.0.0" });
server.registerTool("ping", {
    description: "测试 MCP Server 本身是否可用",
}, async () => {
    return {
        content: [
            {
                type: "text",
                text: JSON.stringify({
                    tool: "ping",
                    ok: true,
                    server: "SUnrealMcp",
                    version: "2.0.0",
                    protocolVersion: PROTOCOL_VERSION,
                    configSources: ["config.json", "config.local.json"],
                }, null, 2),
            },
        ],
    };
});
async function collectToolModulePaths(rootDir) {
    const entries = await readdir(rootDir, { withFileTypes: true });
    const modulePaths = [];
    for (const entry of entries.sort((left, right) => left.name.localeCompare(right.name))) {
        const fullPath = join(rootDir, entry.name);
        if (entry.isDirectory()) {
            modulePaths.push(...(await collectToolModulePaths(fullPath)));
            continue;
        }
        if (entry.isFile() && entry.name.endsWith(".js")) {
            modulePaths.push(fullPath);
        }
    }
    return modulePaths;
}
async function registerAllTools(target, serverContext) {
    const __dirname = dirname(fileURLToPath(import.meta.url));
    const toolsDir = join(__dirname, "tools");
    const modulePaths = await collectToolModulePaths(toolsDir);
    const registeredModules = [];
    for (const modulePath of modulePaths) {
        const relativeModulePath = modulePath.slice(toolsDir.length + 1).replaceAll("\\", "/");
        const imported = (await import(pathToFileURL(modulePath).href));
        if (typeof imported.register !== "function") {
            throw new Error(`Tool module "${relativeModulePath}" does not export register(server, context)`);
        }
        imported.register(target, serverContext);
        registeredModules.push(relativeModulePath);
    }
    return registeredModules;
}
async function main() {
    const config = await readConfig();
    const client = new UnrealTransportClient({
        ...config,
        logger,
    });
    const context = {
        client,
        logger,
        config,
    };
    logger.info("Starting server", {
        host: config.host,
        port: config.port,
        timeoutMs: config.timeoutMs,
        taskTimeoutMs: config.taskTimeoutMs,
        taskStatusRetryLimit: config.taskStatusRetryLimit,
    });
    const registeredModules = await registerAllTools(server, context);
    logger.info("Registered tool modules", { modules: registeredModules });
    const transport = new StdioServerTransport();
    await server.connect(transport);
    logger.info("Server connected to stdio transport");
}
main().catch((error) => {
    logger.error("Fatal error in main()", {
        error: error instanceof Error ? error.message : String(error),
    });
    process.exit(1);
});
