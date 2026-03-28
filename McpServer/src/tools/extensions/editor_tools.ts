import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { z } from "zod";
import { registerCommandTool, vec3Schema } from "../../runtime/tool_helpers.js";
import { ServerContext } from "../../types.js";

export function register(server: McpServer, context: ServerContext): void {
    registerCommandTool(server, context, {
        name: "get_actors_in_level",
        description: "获取当前关卡中所有 Actor 的列表",
    });

    registerCommandTool(server, context, {
        name: "find_actors_by_label",
        description: "按 Actor Label 模式查找 Actor",
        inputSchema: {
            pattern: z.string().describe("Actor Label 匹配模式"),
        },
    });

    registerCommandTool(server, context, {
        name: "spawn_actor",
        description: "在当前关卡中创建新 Actor",
        inputSchema: {
            actor_label: z.string().describe("生成后设置的 Actor Label"),
            actor_class: z.string().describe("Actor 类名或类路径，如 StaticMeshActor、/Script/Engine.PointLight"),
            location: vec3Schema().default([0.0, 0.0, 0.0]).describe("[X, Y, Z] 世界坐标"),
            rotation: vec3Schema().default([0.0, 0.0, 0.0]).describe("[Pitch, Yaw, Roll] 旋转角度"),
        },
    });

    registerCommandTool(server, context, {
        name: "delete_actor",
        description: "按 Actor Object Path 删除 Actor",
        inputSchema: {
            actor_path: z.string().describe("Actor Object Path"),
        },
    });

    registerCommandTool(server, context, {
        name: "set_actor_transform",
        description: "设置 Actor 的变换（位置、旋转、缩放），所有参数均可选",
        inputSchema: {
            actor_path: z.string().describe("Actor Object Path"),
            location: vec3Schema().optional().describe("[X, Y, Z] 位置"),
            rotation: vec3Schema().optional().describe("[Pitch, Yaw, Roll] 旋转"),
            scale: vec3Schema().optional().describe("[X, Y, Z] 缩放"),
        },
        mapParams: ({ actor_path, location, rotation, scale }) => {
            const params: Record<string, unknown> = { actor_path };
            if (location !== undefined) params.location = location;
            if (rotation !== undefined) params.rotation = rotation;
            if (scale !== undefined) params.scale = scale;
            return params;
        },
    });

    registerCommandTool(server, context, {
        name: "get_actor_properties",
        description: "获取 Actor 的所有属性",
        inputSchema: {
            actor_path: z.string().describe("Actor Object Path"),
        },
    });

    registerCommandTool(server, context, {
        name: "set_actor_property",
        description: "设置 Actor 的单个属性",
        inputSchema: {
            actor_path: z.string().describe("Actor Object Path"),
            property_name: z.string().describe("属性名称"),
            property_value: z.unknown().describe("属性值"),
        },
    });

    registerCommandTool(server, context, {
        name: "spawn_blueprint_actor",
        description: "从 Blueprint 生成 Actor 实例",
        inputSchema: {
            blueprint_path: z.string().describe("Blueprint 资产路径"),
            actor_label: z.string().describe("生成后设置的 Actor Label"),
            location: vec3Schema().default([0.0, 0.0, 0.0]).describe("[X, Y, Z] 世界坐标"),
            rotation: vec3Schema().default([0.0, 0.0, 0.0]).describe("[Pitch, Yaw, Roll] 旋转角度"),
        },
    });
}
