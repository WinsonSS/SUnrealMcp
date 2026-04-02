import { CliCommandDefinition, CliCommandRegistry, CliFamilyDefinition } from "../types.js";

export class CommandRegistry implements CliCommandRegistry {
    private readonly families = new Map<string, CliFamilyDefinition>();
    private readonly commands = new Map<string, CliCommandDefinition>();

    registerFamily(definition: CliFamilyDefinition): void {
        if (this.families.has(definition.name)) {
            throw new Error(`Duplicate CLI family registration detected for "${definition.name}".`);
        }

        this.families.set(definition.name, definition);
    }

    registerCommand(definition: CliCommandDefinition): void {
        if (!this.families.has(definition.family)) {
            throw new Error(`Refusing to register command "${definition.family}:${definition.cliCommand}" before its family metadata is registered.`);
        }

        const key = `${definition.family}:${definition.cliCommand}`;
        if (this.commands.has(key)) {
            throw new Error(`Duplicate CLI command registration detected for "${key}".`);
        }

        this.commands.set(key, definition);
    }

    getAll(): CliCommandDefinition[] {
        return [...this.commands.values()];
    }

    getFamilies(): CliFamilyDefinition[] {
        return [...this.families.values()].sort((left, right) => {
            const leftOrder = left.order ?? Number.MAX_SAFE_INTEGER;
            const rightOrder = right.order ?? Number.MAX_SAFE_INTEGER;
            if (leftOrder !== rightOrder) {
                return leftOrder - rightOrder;
            }
            return left.name.localeCompare(right.name);
        });
    }

    get(family: string, cliCommand: string): CliCommandDefinition | undefined {
        return this.commands.get(`${family}:${cliCommand}`);
    }
}
