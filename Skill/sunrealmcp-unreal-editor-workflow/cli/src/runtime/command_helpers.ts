import { CliCommandDefinition, CliCommandRegistry, CliFamilyDefinition } from "../types.js";

export function registerFamily(registry: CliCommandRegistry, definition: CliFamilyDefinition): void {
    registry.registerFamily(definition);
}

export function registerCommand(registry: CliCommandRegistry, definition: CliCommandDefinition): void {
    registry.registerCommand(definition);
}
