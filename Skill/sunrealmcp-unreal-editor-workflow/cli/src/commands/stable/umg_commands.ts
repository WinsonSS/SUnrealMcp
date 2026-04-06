import { registerCommand, registerFamily } from "../../runtime/command_helpers.js";
import { CliCommandRegistry } from "../../types.js";

const FAMILY_NAME = "umg" as const;
const FAMILY_DESCRIPTION = "UMG widget blueprint creation, widget tree edits, bindings, and runtime viewport display.";
const FAMILY_ORDER = 60;

export function register(registry: CliCommandRegistry): void {
    registerFamily(registry, {
        name: FAMILY_NAME,
        description: FAMILY_DESCRIPTION,
        order: FAMILY_ORDER,
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "create_umg_widget_blueprint",
        unrealCommand: "create_umg_widget_blueprint",
        description: "Create a new UMG Widget Blueprint.",
        parameters: [
            { name: "widget_path", type: "string", description: "Full Widget Blueprint asset path.", required: true },
            { name: "parent_class", type: "string", description: "Parent class.", defaultValue: "UserWidget" },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "add_text_block_to_widget",
        unrealCommand: "add_text_block_to_widget",
        description: "Add a text block to a UMG Widget Blueprint.",
        parameters: [
            { name: "widget_path", type: "string", description: "Target Widget Blueprint asset path.", required: true },
            { name: "text_block_name", type: "string", description: "Text block name.", required: true },
            { name: "text", type: "string", description: "Initial text content.", defaultValue: "" },
            { name: "position", type: "vec2", description: "[X, Y] position on the canvas.", defaultValue: "0,0" },
            { name: "size", type: "vec2", description: "[Width, Height] size.", defaultValue: "200,50" },
            { name: "font_size", type: "number", description: "Font size in points.", defaultValue: 12 },
            { name: "color", type: "color", description: "[R, G, B, A] color in the 0.0 to 1.0 range.", defaultValue: "1,1,1,1" },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "add_button_to_widget",
        unrealCommand: "add_button_to_widget",
        description: "Add a button to a UMG Widget Blueprint.",
        parameters: [
            { name: "widget_path", type: "string", description: "Target Widget Blueprint asset path.", required: true },
            { name: "button_name", type: "string", description: "Button name.", required: true },
            { name: "text", type: "string", description: "Button label text.", defaultValue: "" },
            { name: "position", type: "vec2", description: "[X, Y] position on the canvas.", defaultValue: "0,0" },
            { name: "size", type: "vec2", description: "[Width, Height] size.", defaultValue: "200,50" },
            { name: "font_size", type: "number", description: "Button text font size.", defaultValue: 12 },
            { name: "color", type: "color", description: "[R, G, B, A] text color.", defaultValue: "1,1,1,1" },
            { name: "background_color", type: "color", description: "[R, G, B, A] background color.", defaultValue: "0.1,0.1,0.1,1" },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "bind_widget_event",
        unrealCommand: "bind_widget_event",
        description: "Bind a widget component event to a function.",
        parameters: [
            { name: "widget_path", type: "string", description: "Target Widget Blueprint asset path.", required: true },
            { name: "widget_component_name", type: "string", description: "Widget component name, for example a button name.", required: true },
            { name: "event_name", type: "string", description: "Event name, for example OnClicked.", required: true },
            {
                name: "function_name",
                type: "string",
                description: "Function name to bind; if empty it is generated as {component}_{event}.",
                defaultValue: "",
            },
        ],
        mapParams(values) {
            const widgetComponentName = String(values.widget_component_name);
            const eventName = String(values.event_name);
            const functionName = String(values.function_name ?? "");
            return {
                widget_path: values.widget_path,
                widget_component_name: widgetComponentName,
                event_name: eventName,
                function_name: functionName || `${widgetComponentName}_${eventName}`,
            };
        },
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "add_widget_to_viewport",
        unrealCommand: "add_widget_to_viewport",
        description: "Add a Widget Blueprint instance to the game viewport at runtime.",
        parameters: [
            { name: "widget_path", type: "string", description: "Widget Blueprint asset path.", required: true },
            { name: "z_order", type: "number", description: "Z order; higher values appear in front.", defaultValue: 0 },
        ],
    });

    registerCommand(registry, {
        family: FAMILY_NAME,
        lifecycle: "stable",
        cliCommand: "set_text_block_binding",
        unrealCommand: "set_text_block_binding",
        description: "Set a property binding for a text block.",
        parameters: [
            { name: "widget_path", type: "string", description: "Target Widget Blueprint asset path.", required: true },
            { name: "text_block_name", type: "string", description: "Text block name.", required: true },
            { name: "binding_property", type: "string", description: "Property name to bind.", required: true },
            { name: "binding_type", type: "string", description: "Binding type, for example Text or Visibility.", defaultValue: "Text" },
        ],
    });
}
