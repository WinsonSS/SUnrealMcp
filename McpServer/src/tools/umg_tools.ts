import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { z } from "zod";
import { sendCommand } from "../unreal_connection.js";

const vec2 = () => z.tuple([z.number(), z.number()]);
const color = () => z.tuple([z.number(), z.number(), z.number(), z.number()]);

function toText(response: Record<string, unknown>): { content: [{ type: "text"; text: string }] } {
    return { content: [{ type: "text", text: JSON.stringify(response) }] };
}

export function register(server: McpServer): void {
    server.registerTool(
        "create_umg_widget_blueprint",
        {
            description: "创建新的 UMG Widget Blueprint",
            inputSchema: {
                widget_name: z.string().describe("Widget Blueprint 名称"),
                parent_class: z.string().default("UserWidget").describe("父类，默认 UserWidget"),
                path: z.string().default("/Game/UI").describe("内容浏览器路径"),
            },
        },
        async ({ widget_name, parent_class, path }) => {
            const response = await sendCommand("create_umg_widget_blueprint", { widget_name, parent_class, path });
            return toText(response);
        }
    );

    server.registerTool(
        "add_text_block_to_widget",
        {
            description: "向 UMG Widget Blueprint 添加文本块",
            inputSchema: {
                widget_name: z.string().describe("目标 Widget Blueprint 名称"),
                text_block_name: z.string().describe("文本块名称"),
                text: z.string().default("").describe("初始文本内容"),
                position: vec2().default([0.0, 0.0]).describe("[X, Y] 在画布中的位置"),
                size: vec2().default([200.0, 50.0]).describe("[Width, Height] 尺寸"),
                font_size: z.number().int().default(12).describe("字体大小（磅）"),
                color: color().default([1.0, 1.0, 1.0, 1.0]).describe("[R, G, B, A] 颜色，0.0~1.0"),
            },
        },
        async ({ widget_name, text_block_name, text, position, size, font_size, color }) => {
            const response = await sendCommand("add_text_block_to_widget", { widget_name, text_block_name, text, position, size, font_size, color });
            return toText(response);
        }
    );

    server.registerTool(
        "add_button_to_widget",
        {
            description: "向 UMG Widget Blueprint 添加按钮",
            inputSchema: {
                widget_name: z.string().describe("目标 Widget Blueprint 名称"),
                button_name: z.string().describe("按钮名称"),
                text: z.string().default("").describe("按钮显示文本"),
                position: vec2().default([0.0, 0.0]).describe("[X, Y] 在画布中的位置"),
                size: vec2().default([200.0, 50.0]).describe("[Width, Height] 尺寸"),
                font_size: z.number().int().default(12).describe("按钮文字字体大小"),
                color: color().default([1.0, 1.0, 1.0, 1.0]).describe("[R, G, B, A] 文字颜色"),
                background_color: color().default([0.1, 0.1, 0.1, 1.0]).describe("[R, G, B, A] 背景颜色"),
            },
        },
        async ({ widget_name, button_name, text, position, size, font_size, color, background_color }) => {
            const response = await sendCommand("add_button_to_widget", { widget_name, button_name, text, position, size, font_size, color, background_color });
            return toText(response);
        }
    );

    server.registerTool(
        "bind_widget_event",
        {
            description: "将 Widget 组件事件绑定到函数",
            inputSchema: {
                widget_name: z.string().describe("目标 Widget Blueprint 名称"),
                widget_component_name: z.string().describe("Widget 组件名称（如按钮名）"),
                event_name: z.string().describe("事件名称，如 OnClicked"),
                function_name: z.string().default("").describe("绑定的函数名，为空则自动生成为 {组件名}_{事件名}"),
            },
        },
        async ({ widget_name, widget_component_name, event_name, function_name }) => {
            const resolvedFunctionName = function_name || `${widget_component_name}_${event_name}`;
            const response = await sendCommand("bind_widget_event", { widget_name, widget_component_name, event_name, function_name: resolvedFunctionName });
            return toText(response);
        }
    );

    server.registerTool(
        "add_widget_to_viewport",
        {
            description: "将 Widget Blueprint 实例添加到游戏视口",
            inputSchema: {
                widget_name: z.string().describe("Widget Blueprint 名称"),
                z_order: z.number().int().default(0).describe("Z 顺序，数值越大越靠前"),
            },
        },
        async ({ widget_name, z_order }) => {
            const response = await sendCommand("add_widget_to_viewport", { widget_name, z_order });
            return toText(response);
        }
    );

    server.registerTool(
        "set_text_block_binding",
        {
            description: "为文本块设置属性绑定",
            inputSchema: {
                widget_name: z.string().describe("目标 Widget Blueprint 名称"),
                text_block_name: z.string().describe("文本块名称"),
                binding_property: z.string().describe("要绑定的属性名称"),
                binding_type: z.string().default("Text").describe("绑定类型，如 Text、Visibility"),
            },
        },
        async ({ widget_name, text_block_name, binding_property, binding_type }) => {
            const response = await sendCommand("set_text_block_binding", { widget_name, text_block_name, binding_property, binding_type });
            return toText(response);
        }
    );
}
