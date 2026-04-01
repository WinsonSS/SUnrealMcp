import { z } from "zod";
import { colorSchema, registerCommandTool, vec2Schema } from "../../runtime/tool_helpers.js";
export function register(server, context) {
    registerCommandTool(server, context, {
        name: "add_text_block_to_widget",
        description: "Add a text block to a UMG Widget Blueprint",
        inputSchema: {
            widget_path: z.string().describe("Target Widget Blueprint asset path"),
            text_block_name: z.string().describe("Text block name"),
            text: z.string().default("").describe("Initial text content"),
            position: vec2Schema().default([0.0, 0.0]).describe("[X, Y] position on the canvas"),
            size: vec2Schema().default([200.0, 50.0]).describe("[Width, Height] size"),
            font_size: z.number().int().default(12).describe("Font size in points"),
            color: colorSchema().default([1.0, 1.0, 1.0, 1.0]).describe("[R, G, B, A] color in the 0.0 to 1.0 range"),
        },
    });
    registerCommandTool(server, context, {
        name: "add_button_to_widget",
        description: "Add a button to a UMG Widget Blueprint",
        inputSchema: {
            widget_path: z.string().describe("Target Widget Blueprint asset path"),
            button_name: z.string().describe("Button name"),
            text: z.string().default("").describe("Button label text"),
            position: vec2Schema().default([0.0, 0.0]).describe("[X, Y] position on the canvas"),
            size: vec2Schema().default([200.0, 50.0]).describe("[Width, Height] size"),
            font_size: z.number().int().default(12).describe("Button text font size"),
            color: colorSchema().default([1.0, 1.0, 1.0, 1.0]).describe("[R, G, B, A] text color"),
            background_color: colorSchema().default([0.1, 0.1, 0.1, 1.0]).describe("[R, G, B, A] background color"),
        },
    });
    registerCommandTool(server, context, {
        name: "bind_widget_event",
        description: "Bind a widget component event to a function",
        inputSchema: {
            widget_path: z.string().describe("Target Widget Blueprint asset path"),
            widget_component_name: z.string().describe("Widget component name, for example a button name"),
            event_name: z.string().describe("Event name, for example OnClicked"),
            function_name: z.string().default("").describe("Function name to bind; if empty it is generated as {component}_{event}"),
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
        description: "Add a Widget Blueprint instance to the game viewport at runtime",
        inputSchema: {
            widget_path: z.string().describe("Widget Blueprint asset path"),
            z_order: z.number().int().default(0).describe("Z order; higher values appear in front"),
        },
    });
    registerCommandTool(server, context, {
        name: "set_text_block_binding",
        description: "Set a property binding for a text block",
        inputSchema: {
            widget_path: z.string().describe("Target Widget Blueprint asset path"),
            text_block_name: z.string().describe("Text block name"),
            binding_property: z.string().describe("Property name to bind"),
            binding_type: z.string().default("Text").describe("Binding type, for example Text or Visibility"),
        },
    });
}
