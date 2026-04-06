import { readdir } from "fs/promises";
import { dirname, join } from "path";
import { fileURLToPath, pathToFileURL } from "url";
import { CommandRegistry } from "./runtime/registry.js";
import { CliCommandDefinition, CliCommandModule, CliGlobalOptions, CliParameterDefinition, ParsedCliOptions } from "./types.js";
import { resolveTarget, DEFAULT_TIMEOUT_MS } from "./runtime/discovery.js";
import { UnrealTransportClient } from "./transport/unreal_client.js";

const LIFECYCLE_ORDER = ["core", "stable", "temporary"] as const;

const GLOBAL_OPTIONS = [
    { name: "project", type: "path", description: "Project root or .uproject path." },
    { name: "host", type: "string", description: "Override resolved bind address." },
    { name: "port", type: "number", description: "Override resolved port." },
    { name: "timeout-ms", type: "number", description: "Socket timeout override.", defaultValue: DEFAULT_TIMEOUT_MS },
    { name: "pretty", type: "flag", description: "Pretty-print JSON command output." },
];
const HELP_OPTIONS = [{ name: "json", type: "flag", description: "Emit JSON help output." }];

export async function main(argv: string[]): Promise<void> {
    const registry = new CommandRegistry();
    await registerAllCommands(registry);

    if (argv.length === 0 || argv[0] === "help" || argv.includes("--help")) {
        handleHelp(registry, argv);
        return;
    }

    const family = argv[0];
    if (!registry.getFamilies().some((entry) => entry.name === family)) {
        throw new Error(`Unknown command family "${family}". Use "sunrealmcp-cli help".`);
    }

    const cliCommand = argv[1];
    if (!cliCommand) {
        printFamilyHelp(registry, family, false);
        return;
    }

    const definition = registry.get(family, cliCommand);
    if (!definition) {
        throw new Error(`Unknown command "${family} ${cliCommand}". Use "sunrealmcp-cli help ${family}".`);
    }

    const parsed = parseOptions(argv.slice(2), definition.parameters);
    const output = await executeCommand(definition, parsed);
    writeJson(output, parsed.global.pretty);
}

async function registerAllCommands(registry: CommandRegistry): Promise<void> {
    const __dirname = dirname(fileURLToPath(import.meta.url));
    const commandsDir = join(__dirname, "commands");
    const modulePaths = await collectModulePaths(commandsDir);
    for (const modulePath of modulePaths) {
        const imported = (await import(pathToFileURL(modulePath).href)) as Partial<CliCommandModule>;
        if (typeof imported.register !== "function") {
            throw new Error(`Command module "${modulePath}" does not export register(registry).`);
        }
        imported.register(registry);
    }
}

async function collectModulePaths(rootDir: string): Promise<string[]> {
    const entries = await readdir(rootDir, { withFileTypes: true });
    const modulePaths: string[] = [];
    for (const entry of entries.sort((left, right) => left.name.localeCompare(right.name))) {
        const fullPath = join(rootDir, entry.name);
        if (entry.isDirectory()) {
            modulePaths.push(...(await collectModulePaths(fullPath)));
            continue;
        }

        if (entry.isFile() && entry.name.endsWith(".js")) {
            modulePaths.push(fullPath);
        }
    }
    return modulePaths;
}

function handleHelp(registry: CommandRegistry, argv: string[]): void {
    const jsonMode = argv.includes("--json");
    if (argv.length === 0 || argv[0] === "--help" || (argv[0] === "help" && argv.length === 1)) {
        printRootHelp(registry, jsonMode);
        return;
    }

    const tokens =
        argv[0] === "help"
            ? argv.slice(1).filter((token) => token !== "--json")
            : argv.filter((token) => token !== "--help" && token !== "--json");

    if (tokens.length === 0) {
        printRootHelp(registry, jsonMode);
        return;
    }

    const family = tokens[0];
    if (!registry.getFamilies().some((entry) => entry.name === family)) {
        printRootHelp(registry, jsonMode);
        return;
    }

    if (tokens.length === 1) {
        printFamilyHelp(registry, family, jsonMode);
        return;
    }

    printCommandHelp(registry, family, tokens[1], jsonMode);
}

async function executeCommand(definition: CliCommandDefinition, parsed: ParsedCliOptions): Promise<unknown> {
    const resolveTargetForContext = (): Promise<import("./types.js").CliTarget> =>
        resolveTarget(parsed.global);

    const target = await resolveTargetForContext();
    const client = new UnrealTransportClient({
        host: target.host,
        port: target.port,
        timeoutMs: target.timeoutMs,
    });

    const unrealCommand =
        definition.family === "raw" ? String(parsed.values.command) : String(definition.unrealCommand);
    const params = definition.mapParams ? definition.mapParams(parsed.values) : parsed.values;
    const response = await client.sendCommand(unrealCommand, params);

    return {
        ok: response.ok,
        target,
        cli: {
            family: definition.family,
            command: definition.cliCommand,
            lifecycle: definition.lifecycle,
        },
        unreal: {
            command: unrealCommand,
            requestId: response.requestId,
            params,
        },
        data: response.ok ? response.data : undefined,
        error: response.ok ? undefined : response.error,
        raw: response.raw,
    };
}

function parseOptions(tokens: string[], parameters: CliParameterDefinition[]): ParsedCliOptions {
    const values: Record<string, unknown> = {};
    const global: CliGlobalOptions = {
        timeoutMs: DEFAULT_TIMEOUT_MS,
        pretty: false,
    };

    const parameterLookup = new Map(parameters.map((parameter) => [parameter.name, parameter]));

    for (let index = 0; index < tokens.length; index += 1) {
        const token = tokens[index];
        if (!token.startsWith("--")) {
            throw new Error(`Unexpected token "${token}". Options must use --name value form.`);
        }

        const optionName = token.slice(2);
        if (optionName === "pretty") {
            global.pretty = true;
            continue;
        }
        if (optionName === "json") {
            throw new Error('Option "--json" is only supported with "help". Command execution already emits JSON by default.');
        }

        const next = tokens[index + 1];
        if (next === undefined || next.startsWith("--")) {
            throw new Error(`Missing value for option "--${optionName}".`);
        }
        index += 1;

        if (optionName === "project") {
            global.project = next;
            continue;
        }
        if (optionName === "host") {
            global.host = next;
            continue;
        }
        if (optionName === "port") {
            global.port = parseNumber(next, "--port");
            continue;
        }
        if (optionName === "timeout-ms") {
            global.timeoutMs = parseNumber(next, "--timeout-ms");
            continue;
        }

        const parameter = parameterLookup.get(optionName);
        if (!parameter) {
            throw new Error(`Unknown option "--${optionName}". Use "sunrealmcp-cli help" for the command syntax.`);
        }

        values[optionName] = parseTypedValue(parameter, next);
    }

    for (const parameter of parameters) {
        if (values[parameter.name] === undefined && parameter.defaultValue !== undefined) {
            values[parameter.name] = cloneDefault(parameter.defaultValue, parameter.type);
        }
        if (parameter.required && values[parameter.name] === undefined) {
            throw new Error(`Missing required option "--${parameter.name}".`);
        }
    }

    return { values, global };
}

function parseTypedValue(parameter: CliParameterDefinition, rawValue: string): unknown {
    switch (parameter.type) {
        case "string":
        case "path":
            return rawValue;
        case "number":
            return parseNumber(rawValue, `--${parameter.name}`);
        case "boolean":
            return parseBoolean(rawValue, `--${parameter.name}`);
        case "json":
            return parseJson(rawValue, `--${parameter.name}`);
        case "vec2":
            return parseNumberTuple(rawValue, 2, parameter.name);
        case "vec3":
            return parseNumberTuple(rawValue, 3, parameter.name);
        case "color":
            return parseNumberTuple(rawValue, 4, parameter.name);
        case "enum":
            if (!parameter.enumValues?.includes(rawValue)) {
                throw new Error(`Invalid value "${rawValue}" for "--${parameter.name}". Expected one of: ${parameter.enumValues?.join(", ")}.`);
            }
            return rawValue;
        default:
            throw new Error(`Unsupported parameter type "${parameter.type}".`);
    }
}

function cloneDefault(defaultValue: unknown, type: CliParameterDefinition["type"]): unknown {
    if (type === "json" && typeof defaultValue === "object" && defaultValue !== null) {
        return JSON.parse(JSON.stringify(defaultValue));
    }
    if (typeof defaultValue === "string" && (type === "vec2" || type === "vec3" || type === "color")) {
        const expected = type === "vec2" ? 2 : type === "vec3" ? 3 : 4;
        return parseNumberTuple(defaultValue, expected, "default");
    }
    return defaultValue;
}

function parseNumber(rawValue: string, optionName: string): number {
    const numeric = Number(rawValue);
    if (!Number.isFinite(numeric)) {
        throw new Error(`Option "${optionName}" must be a number.`);
    }
    return numeric;
}

function parseBoolean(rawValue: string, optionName: string): boolean {
    if (rawValue === "true") return true;
    if (rawValue === "false") return false;
    throw new Error(`Option "${optionName}" must be "true" or "false".`);
}

function parseJson(rawValue: string, optionName: string): unknown {
    try {
        return JSON.parse(rawValue) as unknown;
    } catch (error) {
        throw new Error(`Option "${optionName}" must be valid JSON: ${error instanceof Error ? error.message : String(error)}`);
    }
}

function parseNumberTuple(rawValue: string, length: number, optionName: string): number[] {
    const parts = rawValue.split(",").map((part) => Number(part.trim()));
    if (parts.length !== length || parts.some((value) => !Number.isFinite(value))) {
        throw new Error(`Option "--${optionName}" must be ${length} comma-separated numbers.`);
    }
    return parts;
}

function printRootHelp(registry: CommandRegistry, jsonMode: boolean): void {
    const families = registry.getFamilies().map((family) => ({
        family: family.name,
        description: family.description,
        commandCount: registry.getAll().length,
    }));

    if (jsonMode) {
        writeJson({ kind: "root-help", globalOptions: GLOBAL_OPTIONS, helpOptions: HELP_OPTIONS, families }, true);
        return;
    }

    const lines: string[] = [];
    lines.push("SUnrealMcp CLI");
    lines.push("");
    lines.push("Purpose:");
    lines.push("Drive SUnrealMcp-enabled Unreal Editor instances through a skill-embedded TypeScript CLI.");
    lines.push("");
    lines.push("Global options:");
    for (const option of GLOBAL_OPTIONS) {
        lines.push(`  --${option.name} (${option.type})${"defaultValue" in option ? ` default=${formatValue(option.defaultValue)}` : ""}  ${option.description}`);
    }
    lines.push("");
    lines.push("Help options:");
    for (const option of HELP_OPTIONS) {
        lines.push(`  --${option.name} (${option.type})  ${option.description}`);
    }
    lines.push("");
    lines.push("Families:");
    for (const family of families) {
        lines.push(`  ${family.family.padEnd(10)} ${family.description}`);
    }
    lines.push("");
    lines.push("Next steps:");
    lines.push("  sunrealmcp-cli help <family>");
    lines.push("  sunrealmcp-cli help <family> <command>");
    lines.push("  sunrealmcp-cli help --json");
    process.stdout.write(`${lines.join("\n")}\n`);
}

function printFamilyHelp(registry: CommandRegistry, family: string, jsonMode: boolean): void {
    const commands = registry.getAll().filter((command) => command.family === family);
    const familyDefinition = registry.getFamilies().find((entry) => entry.name === family);
    if (!familyDefinition) {
        throw new Error(`Unknown family "${family}".`);
    }
    const sections: Record<string, { cliCommand: string; description: string; lifecycle: string }[]> = {};
    for (const lifecycle of LIFECYCLE_ORDER) {
        sections[lifecycle] = commands
            .filter((command) => command.lifecycle === lifecycle)
            .map((command) => ({
                cliCommand: command.cliCommand,
                description: command.description,
                lifecycle: command.lifecycle,
            }));
    }

    if (jsonMode) {
        writeJson({
            kind: "family-help",
            family,
            description: familyDefinition.description,
            globalOptions: GLOBAL_OPTIONS,
            helpOptions: HELP_OPTIONS,
            sections,
        }, true);
        return;
    }

    const lines: string[] = [];
    lines.push(`Family: ${family}`);
    lines.push(familyDefinition.description);
    lines.push("");
    for (const lifecycle of LIFECYCLE_ORDER) {
        if (sections[lifecycle].length === 0) {
            continue;
        }
        lines.push(`${capitalize(lifecycle)}:`);
        for (const command of sections[lifecycle]) {
            lines.push(`  ${command.cliCommand.padEnd(28)} ${command.description}`);
        }
        lines.push("");
    }
    lines.push(`Command help: sunrealmcp-cli help ${family} <command>`);
    process.stdout.write(`${lines.join("\n")}\n`);
}

function printCommandHelp(registry: CommandRegistry, family: string, cliCommand: string, jsonMode: boolean): void {
    const definition = registry.get(family, cliCommand);
    if (!definition) {
        throw new Error(`Unknown command "${family} ${cliCommand}".`);
    }

    const payload = {
        kind: "command-help",
        family,
        command: cliCommand,
        lifecycle: definition.lifecycle,
        description: definition.description,
        unrealCommand: definition.unrealCommand ?? null,
        globalOptions: GLOBAL_OPTIONS,
        helpOptions: HELP_OPTIONS,
        parameters: definition.parameters,
        examples: definition.examples ?? [`sunrealmcp-cli ${family} ${cliCommand} ...`],
    };

    if (jsonMode) {
        writeJson(payload, true);
        return;
    }

    const lines: string[] = [];
    lines.push(`Command: ${family} ${cliCommand}`);
    lines.push(`Lifecycle: ${definition.lifecycle}`);
    lines.push(definition.description);
    lines.push("");
    if (definition.unrealCommand) {
        lines.push(`Unreal command: ${definition.unrealCommand}`);
        lines.push("");
    }
    lines.push("Parameters:");
    if (definition.parameters.length === 0) {
        lines.push("  (no command-specific parameters)");
    } else {
        for (const parameter of definition.parameters) {
            lines.push(`  --${parameter.name} (${parameter.type})${parameter.required ? " required" : " optional"}${parameter.defaultValue !== undefined ? ` default=${formatValue(parameter.defaultValue)}` : ""}  ${parameter.description}`);
        }
    }
    lines.push("");
    lines.push("Global target options:");
    for (const option of GLOBAL_OPTIONS) {
        lines.push(`  --${option.name} (${option.type})${"defaultValue" in option ? ` default=${formatValue(option.defaultValue)}` : ""}`);
    }
    lines.push("");
    lines.push('Help JSON: sunrealmcp-cli help --json');
    if (payload.examples.length > 0) {
        lines.push("");
        lines.push("Examples:");
        for (const example of payload.examples) {
            lines.push(`  ${example}`);
        }
    }
    process.stdout.write(`${lines.join("\n")}\n`);
}

function writeJson(payload: unknown, pretty: boolean): void {
    process.stdout.write(`${JSON.stringify(payload, null, pretty ? 2 : 0)}\n`);
}

function capitalize(value: string): string {
    return value.slice(0, 1).toUpperCase() + value.slice(1);
}

function formatValue(value: unknown): string {
    return typeof value === "string" ? value : JSON.stringify(value);
}
