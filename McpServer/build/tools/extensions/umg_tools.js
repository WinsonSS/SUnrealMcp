import { z } from "zod";
import { colorSchema, registerCommandTool, vec2Schema } from "../../runtime/tool_helpers.js";
export function register(server, context) {
    registerCommandTool(server, context, {
        name: "create_umg_widget_blueprint",
        description: "创建新的 UMG Widget Blueprint",
        inputSchema: {
            widget_path: z.string().describe("Widget Blueprint 完整资产路径"),
            parent_class: z.string().default("UserWidget").describe("父类，默认 UserWidget"),
        },
    });
    registerCommandTool(server, context, {
        name: "add_text_block_to_widget",
        description: "向 UMG Widget Blueprint 添加文本块",
        inputSchema: {
            widget_path: z.string().describe("目标 Widget Blueprint 资产路径"),
            text_block_name: z.string().describe("文本块名称"),
            text: z.string().default("").describe("初始文本内容"),
            position: vec2Schema().default([0.0, 0.0]).describe("[X, Y] 在画布中的位置"),
            size: vec2Schema().default([200.0, 50.0]).describe("[Width, Height] 尺寸"),
            font_size: z.number().int().default(12).describe("字体大小（磅）"),
            color: colorSchema().default([1.0, 1.0, 1.0, 1.0]).describe("[R, G, B, A] 颜色，0.0~1.0"),
        },
    });
    registerCommandTool(server, context, {
        name: "add_button_to_widget",
        description: "向 UMG Widget Blueprint 添加按钮",
        inputSchema: {
            widget_path: z.string().describe("目标 Widget Blueprint 资产路径"),
            button_name: z.string().describe("按钮名称"),
            text: z.string().default("").describe("按钮显示文本"),
            position: vec2Schema().default([0.0, 0.0]).describe("[X, Y] 在画布中的位置"),
            size: vec2Schema().default([200.0, 50.0]).describe("[Width, Height] 尺寸"),
            font_size: z.number().int().default(12).describe("按钮文字字体大小"),
            color: colorSchema().default([1.0, 1.0, 1.0, 1.0]).describe("[R, G, B, A] 文字颜色"),
            background_color: colorSchema().default([0.1, 0.1, 0.1, 1.0]).describe("[R, G, B, A] 背景颜色"),
        },
    });
    registerCommandTool(server, context, {
        name: "bind_widget_event",
        description: "将 Widget 组件事件绑定到函数",
        inputSchema: {
            widget_path: z.string().describe("目标 Widget Blueprint 资产路径"),
            widget_component_name: z.string().describe("Widget 组件名称（如按钮名）"),
            event_name: z.string().describe("事件名称，如 OnClicked"),
            function_name: z.string().default("").describe("绑定的函数名，为空则自动生成为 {组件名}_{事件名}"),
        },
        mapParams: ({ widget_path, widget_component_name, event_name, function_name }) => ({
            widget_path,
            widget_component_name,
            event_name,
            function_name: function_name || `${widget_component_name}_${event_name}`,
        }),
    });
    registerCommandTool(server, context, {
        name: "add_widget_to_viewport",
        description: "在运行时将 Widget Blueprint 实例添加到游戏视口",
        inputSchema: {
            widget_path: z.string().describe("Widget Blueprint 资产路径"),
            z_order: z.number().int().default(0).describe("Z 顺序，数值越大越靠前"),
        },
    });
    registerCommandTool(server, context, {
        name: "set_text_block_binding",
        description: "为文本块设置属性绑定",
        inputSchema: {
            widget_path: z.string().describe("目标 Widget Blueprint 资产路径"),
            text_block_name: z.string().describe("文本块名称"),
            binding_property: z.string().describe("要绑定的属性名称"),
            binding_type: z.string().default("Text").describe("绑定类型，如 Text、Visibility"),
        },
    });
}
