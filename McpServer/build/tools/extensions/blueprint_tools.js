import { z } from "zod";
import { registerCommandTool, vec3Schema } from "../../runtime/tool_helpers.js";
export function register(server, context) {
    registerCommandTool(server, context, {
        name: "create_blueprint",
        description: "创建新的 Blueprint 类",
        inputSchema: {
            blueprint_path: z.string().describe("新 Blueprint 的完整资产路径"),
            parent_class: z.string().describe("父类名称或类路径，如 Actor、Pawn、Character"),
        },
    });
    registerCommandTool(server, context, {
        name: "add_component_to_blueprint",
        description: "向 Blueprint 添加组件",
        inputSchema: {
            blueprint_path: z.string().describe("目标 Blueprint 资产路径"),
            component_type: z.string().describe("组件类型或类路径，如 StaticMeshComponent（不含 U 前缀）"),
            component_name: z.string().describe("新组件名称"),
            location: vec3Schema().optional().describe("[X, Y, Z] 组件位置"),
            rotation: vec3Schema().optional().describe("[Pitch, Yaw, Roll] 组件旋转"),
            scale: vec3Schema().optional().describe("[X, Y, Z] 组件缩放"),
            component_properties: z.record(z.unknown()).optional().describe("额外组件属性"),
        },
        mapParams: ({ blueprint_path, component_type, component_name, location, rotation, scale, component_properties, }) => {
            const params = {
                blueprint_path,
                component_type,
                component_name,
                location: location ?? [0.0, 0.0, 0.0],
                rotation: rotation ?? [0.0, 0.0, 0.0],
                scale: scale ?? [1.0, 1.0, 1.0],
            };
            if (component_properties && Object.keys(component_properties).length > 0) {
                params.component_properties = component_properties;
            }
            return params;
        },
    });
    registerCommandTool(server, context, {
        name: "set_static_mesh_properties",
        description: "设置 StaticMeshComponent 的静态网格属性",
        inputSchema: {
            blueprint_path: z.string().describe("目标 Blueprint 资产路径"),
            component_name: z.string().describe("StaticMeshComponent 名称"),
            static_mesh: z.string().default("/Engine/BasicShapes/Cube.Cube").describe("静态网格资产路径"),
        },
    });
    registerCommandTool(server, context, {
        name: "set_component_property",
        description: "设置 Blueprint 中组件的属性",
        inputSchema: {
            blueprint_path: z.string().describe("目标 Blueprint 资产路径"),
            component_name: z.string().describe("组件名称"),
            property_name: z.string().describe("属性名称"),
            property_value: z.unknown().describe("属性值"),
        },
    });
    registerCommandTool(server, context, {
        name: "set_physics_properties",
        description: "设置组件的物理属性",
        inputSchema: {
            blueprint_path: z.string().describe("目标 Blueprint 资产路径"),
            component_name: z.string().describe("组件名称"),
            simulate_physics: z.boolean().default(true).describe("是否启用物理模拟"),
            gravity_enabled: z.boolean().default(true).describe("是否启用重力"),
            mass: z.number().default(1.0).describe("质量（kg）"),
            linear_damping: z.number().default(0.01).describe("线性阻尼"),
            angular_damping: z.number().default(0.0).describe("角阻尼"),
        },
    });
    registerCommandTool(server, context, {
        name: "compile_blueprint",
        description: "编译指定 Blueprint",
        inputSchema: {
            blueprint_path: z.string().describe("Blueprint 资产路径"),
        },
    });
    registerCommandTool(server, context, {
        name: "set_blueprint_property",
        description: "设置 Blueprint 类默认对象上的属性",
        inputSchema: {
            blueprint_path: z.string().describe("目标 Blueprint 资产路径"),
            property_name: z.string().describe("属性名称"),
            property_value: z.unknown().describe("属性值"),
        },
    });
}
