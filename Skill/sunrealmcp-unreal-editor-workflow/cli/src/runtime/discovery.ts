import * as fs from "fs";
import * as path from "path";
import { CliGlobalOptions, CliTarget } from "../types.js";

export const DEFAULT_HOST = "127.0.0.1";
export const DEFAULT_PORT = 55557;
export const DEFAULT_TIMEOUT_MS = 30000;
const SETTINGS_SECTION = "/Script/SUnrealMcp.SUnrealMcpSettings";
const IGNORED_SCAN_DIRS = new Set([
    ".git",
    ".vs",
    "Binaries",
    "Build",
    "DerivedDataCache",
    "Intermediate",
    "node_modules",
    "Saved",
]);

export interface TargetProbeResult {
    projectRoot: string;
    pluginPath: string;
    host: string;
    port: number;
    configSources: string[];
}

export async function listTargets(root: string, depth: number): Promise<TargetProbeResult[]> {
    const rootPath = path.resolve(root);
    const projects: string[] = [];
    await scanForProjects(rootPath, 0, depth, projects);

    return projects
        .map((projectRoot) => {
            const plugin = findPluginLocation(projectRoot);
            if (!plugin.exists) {
                return null;
            }

            const config = resolveConfig(projectRoot, {});
            return {
                projectRoot,
                pluginPath: plugin.path,
                host: config.host,
                port: config.port,
                configSources: config.sources,
            } satisfies TargetProbeResult;
        })
        .filter((value): value is TargetProbeResult => value !== null);
}

export async function resolveTarget(global: CliGlobalOptions): Promise<CliTarget> {
    const projectPath = global.project;
    if (!projectPath) {
        const auto = await autoResolveSingleProject(process.cwd());
        if (!auto) {
            throw new Error("No target project was provided and auto-discovery did not find exactly one Unreal project with SUnrealMcp. Pass --project.");
        }

        return buildResolvedTarget(auto.projectRoot, global);
    }

    return buildResolvedTarget(resolveProjectRoot(projectPath), global);
}

function buildResolvedTarget(projectRoot: string, global: CliGlobalOptions): CliTarget {
    const plugin = findPluginLocation(projectRoot);
    if (!plugin.exists) {
        throw new Error(`SUnrealMcp plugin was not found under "${projectRoot}".`);
    }

    const config = resolveConfig(projectRoot, {
        host: global.host,
        port: global.port,
    });

    return {
        projectRoot,
        projectFile: findProjectFile(projectRoot),
        pluginPath: plugin.path,
        host: config.host,
        port: config.port,
        configSources: config.sources,
        timeoutMs: global.timeoutMs ?? DEFAULT_TIMEOUT_MS,
    };
}

function resolveProjectRoot(projectPath: string): string {
    const fullPath = path.resolve(projectPath);
    if (fs.existsSync(fullPath) && fs.statSync(fullPath).isFile() && fullPath.endsWith(".uproject")) {
        return path.dirname(fullPath);
    }
    if (fs.existsSync(fullPath) && fs.statSync(fullPath).isDirectory()) {
        return fullPath;
    }
    throw new Error(`Project path "${projectPath}" does not exist.`);
}

function findProjectFile(projectRoot: string): string | undefined {
    const entries = safeReadDir(projectRoot);
    const projectFile = entries.find((entry) => entry.isFile() && entry.name.endsWith(".uproject"));
    return projectFile ? path.join(projectRoot, projectFile.name) : undefined;
}

function findPluginLocation(projectRoot: string): { exists: boolean; path: string } {
    const pluginPath = path.join(projectRoot, "Plugins", "SUnrealMcp", "SUnrealMcp.uplugin");
    return {
        exists: fs.existsSync(pluginPath),
        path: pluginPath,
    };
}

function resolveConfig(
    projectRoot: string,
    overrides: {
        host?: string;
        port?: number;
    },
): {
    host: string;
    port: number;
    sources: string[];
} {
    const config = {
        host: DEFAULT_HOST,
        port: DEFAULT_PORT,
        sources: [] as string[],
    };

    for (const filePath of collectConfigCandidates(projectRoot)) {
        if (!fs.existsSync(filePath)) {
            continue;
        }

        const section = readIniSection(filePath, SETTINGS_SECTION);
        if (!section) {
            continue;
        }

        if (section.BindAddress) {
            config.host = section.BindAddress;
        }
        if (section.Port && Number.isFinite(Number(section.Port))) {
            config.port = Number(section.Port);
        }
        config.sources.push(filePath);
    }

    if (overrides.host !== undefined) {
        config.host = overrides.host;
        config.sources.push("override:--host");
    }
    if (overrides.port !== undefined) {
        config.port = overrides.port;
        config.sources.push("override:--port");
    }

    return config;
}

function collectConfigCandidates(projectRoot: string): string[] {
    const files: string[] = [];
    files.push(path.join(projectRoot, "Plugins", "SUnrealMcp", "Config", "DefaultSUnrealMcp.ini"));
    files.push(path.join(projectRoot, "Config", "DefaultSUnrealMcp.ini"));
    return files;
}

function readIniSection(filePath: string, targetSection: string): Record<string, string> | null {
    const text = fs.readFileSync(filePath, "utf8");
    let currentSection = "";
    const result: Record<string, string> = {};

    for (const rawLine of text.split(/\r?\n/)) {
        const line = rawLine.trim();
        if (!line || line.startsWith(";") || line.startsWith("#")) {
            continue;
        }
        if (line.startsWith("[") && line.endsWith("]")) {
            currentSection = line.slice(1, -1);
            continue;
        }
        if (currentSection !== targetSection) {
            continue;
        }
        const separator = line.indexOf("=");
        if (separator === -1) {
            continue;
        }
        const key = line.slice(0, separator).trim();
        const value = line.slice(separator + 1).trim();
        result[key] = value;
    }

    return Object.keys(result).length > 0 ? result : null;
}

async function autoResolveSingleProject(root: string): Promise<{ projectRoot: string } | null> {
    const projects: string[] = [];
    await scanForProjects(root, 0, 4, projects);
    const resolved = projects
        .map((projectRoot) => ({ projectRoot, pluginPath: findPluginLocation(projectRoot) }))
        .filter((entry) => entry.pluginPath.exists);
    return resolved.length === 1 ? { projectRoot: resolved[0].projectRoot } : null;
}

async function scanForProjects(currentPath: string, currentDepth: number, maxDepth: number, output: string[]): Promise<void> {
    if (currentDepth > maxDepth) {
        return;
    }

    const entries = safeReadDir(currentPath);
    const hasProject = entries.some((entry) => entry.isFile() && entry.name.endsWith(".uproject"));
    if (hasProject) {
        output.push(currentPath);
        return;
    }

    for (const entry of entries) {
        if (!entry.isDirectory() || IGNORED_SCAN_DIRS.has(entry.name)) {
            continue;
        }
        await scanForProjects(path.join(currentPath, entry.name), currentDepth + 1, maxDepth, output);
    }
}

function safeReadDir(directoryPath: string): fs.Dirent[] {
    try {
        return fs.readdirSync(directoryPath, { withFileTypes: true });
    } catch {
        return [];
    }
}
