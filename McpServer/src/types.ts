import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { type ZodTypeAny } from "zod";
import { UnrealTransportClient } from "./transport/unreal_client.js";
import { NormalizedUnrealResponse, UnrealCommandParams } from "./protocol.js";

export interface Logger {
    debug: (message: string, meta?: Record<string, unknown>) => void;
    info: (message: string, meta?: Record<string, unknown>) => void;
    warn: (message: string, meta?: Record<string, unknown>) => void;
    error: (message: string, meta?: Record<string, unknown>) => void;
}

export interface ServerConfig {
    host: string;
    port: number;
    timeoutMs: number;
}

export interface PartialServerConfig {
    host?: string;
    port?: number;
    timeoutMs?: number;
}

export interface ServerContext {
    client: UnrealTransportClient;
    logger: Logger;
    config: ServerConfig;
}

export interface ToolModule {
    register: (server: McpServer, context: ServerContext) => void;
}

export type ToolInputShape = Record<string, ZodTypeAny>;

export interface CommandToolDefinition {
    name: string;
    description: string;
    command?: string;
    inputSchema?: ToolInputShape;
    mapParams?: (input: Record<string, unknown>) => UnrealCommandParams;
    formatResponse?: (response: NormalizedUnrealResponse, input: Record<string, unknown>) => unknown;
}
