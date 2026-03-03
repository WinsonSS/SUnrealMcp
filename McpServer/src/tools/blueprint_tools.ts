import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { z } from "zod";
import { sendCommand } from "../unreal_connection.js";

const vec3 = () => z.tuple([z.number(), z.number(), z.number()]);

function toText(response: Record<string, unknown>): { content: [{ type: "text"; text: string }] } {
    return { content: [{ type: "text", text: JSON.stringify(response) }] };
}

export function register(server: McpServer): void {
    server.registerTool(
        "create_blueprint",
        {
            description: "创建新的 Blueprint 类",
            inputSchema: {
                name: z.string().describe("Blueprint 名称"),
                parent_class: z.string().describe("父类名称，如 Actor、Pawn、Character"),
            },
        },
        async ({ name, parent_class }) => {
            const response = await sendCommand("create_blueprint", { name, parent_class });
            return toText(response);
        }
    );

    server.registerTool(
        "add_component_to_blueprint",
        {
            description: "向 Blueprint 添加组件",
            inputSchema: {
                blueprint_name: z.string().describe("目标 Blueprint 名称"),
                component_type: z.string().describe("组件类型，如 StaticMeshComponent（不含 U 前缀）"),
                component_name: z.string().describe("新组件名称"),
                location: vec3().optional().describe("[X, Y, Z] 组件位置"),
                rotation: vec3().optional().describe("[Pitch, Yaw, Roll] 组件旋转"),
                scale: vec3().optional().describe("[X, Y, Z] 组件缩放"),
                component_properties: z.record(z.unknown()).optional().describe("额外组件属性"),
            },
        },
        async ({ blueprint_name, component_type, component_name, location, rotation, scale, component_properties }) => {
            const params: Record<string, unknown> = {
                blueprint_name,
                component_type,
                component_name,
                location: location ?? [0.0, 0.0, 0.0],
                rotation: rotation ?? [0.0, 0.0, 0.0],
                scale: scale ?? [1.0, 1.0, 1.0],
            };
            if (component_properties && Object.keys(component_properties).length > 0) {
                params.component_properties = component_properties;
            }
            const response = await sendCommand("add_component_to_blueprint", params);
            return toText(response);
        }
    );

    server.registerTool(
        "set_static_mesh_properties",
        {
            description: "设置 StaticMeshComponent 的静态网格属性",
            inputSchema: {
                blueprint_name: z.string().describe("目标 Blueprint 名称"),
                component_name: z.string().describe("StaticMeshComponent 名称"),
                static_mesh: z.string().default("/Engine/BasicShapes/Cube.Cube").describe("静态网格资产路径"),
            },
        },
        async ({ blueprint_name, component_name, static_mesh }) => {
            const response = await sendCommand("set_static_mesh_properties", { blueprint_name, component_name, static_mesh });
            return toText(response);
        }
    );

    server.registerTool(
        "set_component_property",
        {
            description: "设置 Blueprint 中组件的属性",
            inputSchema: {
                blueprint_name: z.string().describe("目标 Blueprint 名称"),
                component_name: z.string().describe("组件名称"),
                property_name: z.string().describe("属性名称"),
                property_value: z.unknown().describe("属性值"),
            },
        },
        async ({ blueprint_name, component_name, property_name, property_value }) => {
            const response = await sendCommand("set_component_property", { blueprint_name, component_name, property_name, property_value });
            return toText(response);
        }
    );

    server.registerTool(
        "set_physics_properties",
        {
            description: "设置组件的物理属性",
            inputSchema: {
                blueprint_name: z.string().describe("目标 Blueprint 名称"),
                component_name: z.string().describe("组件名称"),
                simulate_physics: z.boolean().default(true).describe("是否启用物理模拟"),
                gravity_enabled: z.boolean().default(true).describe("是否启用重力"),
                mass: z.number().default(1.0).describe("质量（kg）"),
                linear_damping: z.number().default(0.01).describe("线性阻尼"),
                angular_damping: z.number().default(0.0).describe("角阻尼"),
            },
        },
        async ({ blueprint_name, component_name, simulate_physics, gravity_enabled, mass, linear_damping, angular_damping }) => {
            const response = await sendCommand("set_physics_properties", {
                blueprint_name, component_name, simulate_physics, gravity_enabled, mass, linear_damping, angular_damping,
            });
            return toText(response);
        }
    );

    server.registerTool(
        "compile_blueprint",
        {
            description: "编译指定 Blueprint",
            inputSchema: {
                blueprint_name: z.string().describe("Blueprint 名称"),
            },
        },
        async ({ blueprint_name }) => {
            const response = await sendCommand("compile_blueprint", { blueprint_name });
            return toText(response);
        }
    );

    server.registerTool(
        "set_blueprint_property",
        {
            description: "设置 Blueprint 类默认对象上的属性",
            inputSchema: {
                blueprint_name: z.string().describe("目标 Blueprint 名称"),
                property_name: z.string().describe("属性名称"),
                property_value: z.unknown().describe("属性值"),
            },
        },
        async ({ blueprint_name, property_name, property_value }) => {
            const response = await sendCommand("set_blueprint_property", { blueprint_name, property_name, property_value });
            return toText(response);
        }
    );
}
