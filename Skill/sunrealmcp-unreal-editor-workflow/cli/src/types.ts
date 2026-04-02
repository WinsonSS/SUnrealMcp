export type ParameterType =
    | "string"
    | "path"
    | "number"
    | "boolean"
    | "json"
    | "vec2"
    | "vec3"
    | "color"
    | "enum";

export interface CliParameterDefinition {
    name: string;
    type: ParameterType;
    description: string;
    required?: boolean;
    defaultValue?: unknown;
    enumValues?: string[];
}

export type CliParameterValues = Record<string, unknown>;

export interface CliGlobalOptions {
    project?: string;
    host?: string;
    port?: number;
    timeoutMs: number;
    pretty: boolean;
}

export interface ParsedCliOptions {
    values: CliParameterValues;
    global: CliGlobalOptions;
}

export type CliLifecycle = "core" | "stable" | "candidate" | "temporary";

export interface CliTarget {
    projectRoot: string;
    projectFile?: string;
    pluginPath: string;
    host: string;
    port: number;
    configSources: string[];
    timeoutMs: number;
}

export interface CliCommandExecutionContext {
    target?: CliTarget;
    resolveTarget: () => Promise<CliTarget>;
    values: CliParameterValues;
    global: CliGlobalOptions;
}

export interface CliFamilyDefinition {
    name: string;
    description: string;
    order?: number;
}

export interface CliCommandDefinition {
    family: string;
    lifecycle: CliLifecycle;
    cliCommand: string;
    description: string;
    parameters: CliParameterDefinition[];
    examples?: string[];
    unrealCommand?: string;
    mapParams?: (values: CliParameterValues) => Record<string, unknown>;
}

export interface CliCommandModule {
    register: (registry: CliCommandRegistry) => void;
}

export interface CliCommandRegistry {
    registerFamily: (definition: CliFamilyDefinition) => void;
    registerCommand: (definition: CliCommandDefinition) => void;
}

export interface CommandSummary {
    cliCommand: string;
    description: string;
    lifecycle: CliLifecycle;
}
