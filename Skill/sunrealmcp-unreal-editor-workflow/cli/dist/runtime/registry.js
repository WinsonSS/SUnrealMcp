export class CommandRegistry {
    families = new Map();
    commands = new Map();
    registerFamily(definition) {
        if (this.families.has(definition.name)) {
            throw new Error(`Duplicate CLI family registration detected for "${definition.name}".`);
        }
        this.families.set(definition.name, definition);
    }
    registerCommand(definition) {
        if (!this.families.has(definition.family)) {
            throw new Error(`Refusing to register command "${definition.family}:${definition.cliCommand}" before its family metadata is registered.`);
        }
        const key = `${definition.family}:${definition.cliCommand}`;
        if (this.commands.has(key)) {
            throw new Error(`Duplicate CLI command registration detected for "${key}".`);
        }
        this.commands.set(key, definition);
    }
    getAll() {
        return [...this.commands.values()];
    }
    getFamilies() {
        return [...this.families.values()].sort((left, right) => {
            const leftOrder = left.order ?? Number.MAX_SAFE_INTEGER;
            const rightOrder = right.order ?? Number.MAX_SAFE_INTEGER;
            if (leftOrder !== rightOrder) {
                return leftOrder - rightOrder;
            }
            return left.name.localeCompare(right.name);
        });
    }
    get(family, cliCommand) {
        return this.commands.get(`${family}:${cliCommand}`);
    }
}
