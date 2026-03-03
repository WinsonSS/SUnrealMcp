import { z } from "zod";
import { sendCommand } from "../unreal_connection.js";
const vec3 = () => z.tuple([z.number(), z.number(), z.number()]);
function toText(response) {
    return { content: [{ type: "text", text: JSON.stringify(response) }] };
}
export function register(server) {
    server.registerTool("get_actors_in_level", {
        description: "获取当前关卡中所有 Actor 的列表",
    }, async () => {
        const response = await sendCommand("get_actors_in_level", {});
        return toText(response);
    });
    server.registerTool("find_actors_by_name", {
        description: "按名称模式查找 Actor",
        inputSchema: {
            pattern: z.string().describe("名称匹配模式"),
        },
    }, async ({ pattern }) => {
        const response = await sendCommand("find_actors_by_name", { pattern });
        return toText(response);
    });
    server.registerTool("spawn_actor", {
        description: "在当前关卡中创建新 Actor",
        inputSchema: {
            name: z.string().describe("Actor 名称（须唯一）"),
            type: z.string().describe("Actor 类型，如 StaticMeshActor、PointLight"),
            location: vec3().default([0.0, 0.0, 0.0]).describe("[X, Y, Z] 世界坐标"),
            rotation: vec3().default([0.0, 0.0, 0.0]).describe("[Pitch, Yaw, Roll] 旋转角度"),
        },
    }, async ({ name, type, location, rotation }) => {
        const response = await sendCommand("spawn_actor", { name, type, location, rotation });
        return toText(response);
    });
    server.registerTool("delete_actor", {
        description: "按名称删除 Actor",
        inputSchema: {
            name: z.string().describe("Actor 名称"),
        },
    }, async ({ name }) => {
        const response = await sendCommand("delete_actor", { name });
        return toText(response);
    });
    server.registerTool("set_actor_transform", {
        description: "设置 Actor 的变换（位置、旋转、缩放），所有参数均可选",
        inputSchema: {
            name: z.string().describe("Actor 名称"),
            location: vec3().optional().describe("[X, Y, Z] 位置"),
            rotation: vec3().optional().describe("[Pitch, Yaw, Roll] 旋转"),
            scale: vec3().optional().describe("[X, Y, Z] 缩放"),
        },
    }, async ({ name, location, rotation, scale }) => {
        const params = { name };
        if (location !== undefined)
            params.location = location;
        if (rotation !== undefined)
            params.rotation = rotation;
        if (scale !== undefined)
            params.scale = scale;
        const response = await sendCommand("set_actor_transform", params);
        return toText(response);
    });
    server.registerTool("get_actor_properties", {
        description: "获取 Actor 的所有属性",
        inputSchema: {
            name: z.string().describe("Actor 名称"),
        },
    }, async ({ name }) => {
        const response = await sendCommand("get_actor_properties", { name });
        return toText(response);
    });
    server.registerTool("set_actor_property", {
        description: "设置 Actor 的单个属性",
        inputSchema: {
            name: z.string().describe("Actor 名称"),
            property_name: z.string().describe("属性名称"),
            property_value: z.unknown().describe("属性值"),
        },
    }, async ({ name, property_name, property_value }) => {
        const response = await sendCommand("set_actor_property", { name, property_name, property_value });
        return toText(response);
    });
    server.registerTool("spawn_blueprint_actor", {
        description: "从 Blueprint 生成 Actor 实例",
        inputSchema: {
            blueprint_name: z.string().describe("Blueprint 名称"),
            actor_name: z.string().describe("生成的 Actor 名称"),
            location: vec3().default([0.0, 0.0, 0.0]).describe("[X, Y, Z] 世界坐标"),
            rotation: vec3().default([0.0, 0.0, 0.0]).describe("[Pitch, Yaw, Roll] 旋转角度"),
        },
    }, async ({ blueprint_name, actor_name, location, rotation }) => {
        const response = await sendCommand("spawn_blueprint_actor", { blueprint_name, actor_name, location, rotation });
        return toText(response);
    });
}
