# SUnrealMcp CLI Skill Design

## Goal

Replace the current `McpServer`-registered tool surface with a skill-embedded CLI while keeping the Unreal-side plugin and the existing TCP JSON command architecture.

The target user experience is:

- repository root only keeps `UnrealPlugin/` and `Skill/`
- the CLI ships inside the skill folder
- an agent copies the whole skill directory into its skills directory and can use the CLI immediately
- the skill tells the agent when to use the CLI, how to discover the target Unreal project, how to resolve the port, and which command family to call
- one agent can operate multiple Unreal Editor instances as long as they listen on different ports

## Core Decision

Keep these parts from the current `McpServer` design:

- TCP JSON wire protocol
- request and response envelope shape
- Unreal transport client session model
- command naming and command families
- task polling model for long-running operations

Remove these parts from the runtime path:

- MCP SDK tool registration
- MCP tool discovery dependency
- per-session MCP tool refresh as a requirement for newly added capabilities

Replace them with:

- a local CLI entrypoint inside the skill directory
- a command catalog that mirrors the current tool and command surface
- a discovery layer that resolves the target Unreal Editor by project path plus ini-driven port lookup
- a built-in help system for both agents and humans

## Context Strategy

The main skill should stay compact and delegate detailed command lookup to reference files.

Recommended loading order:

1. load the skill body for workflow rules
2. load [cli-command-catalog.md](./cli-command-catalog.md) for family routing only
3. load exactly one family reference file when the task needs command details

Do not load all command-family references by default.

## Target Repository Layout

```text
SUnrealMcp/
├─ UnrealPlugin/
│  └─ SUnrealMcp/
└─ Skill/
   └─ sunrealmcp-unreal-editor-workflow/
      ├─ SKILL.md
      ├─ SKILL_cn.md
      ├─ references/
      │  ├─ cli-design.md
      │  ├─ cli-command-catalog.md
      │  ├─ cli-discovery.md
      │  ├─ cli-system.md
      │  ├─ cli-editor.md
      │  ├─ cli-blueprint.md
      │  ├─ cli-node.md
      │  ├─ cli-umg.md
      │  ├─ cli-project.md
      │  └─ cli-raw.md
      └─ cli/
         ├─ package.json
         ├─ dist/
         │  └─ sunrealmcp-cli.js
         ├─ src/
         │  ├─ index.ts
         │  ├─ protocol.ts
         │  ├─ catalog.ts
         │  ├─ discovery/
         │  │  ├─ project_scan.ts
         │  │  ├─ plugin_scan.ts
         │  │  ├─ ini_resolver.ts
         │  │  └─ target_resolver.ts
         │  ├─ runtime/
         │  │  ├─ command_runner.ts
         │  │  └─ task_runner.ts
         │  ├─ transport/
         │  │  └─ unreal_client.ts
         │  └─ commands/
         │     ├─ system.ts
         │     ├─ discovery.ts
         │     ├─ editor.ts
         │     ├─ blueprint.ts
         │     ├─ node.ts
         │     ├─ umg.ts
         │     └─ project.ts
         └─ bin/
            └─ sunrealmcp-cli.ps1
```

## Packaging Rules

The CLI should be portable as part of the skill.

Recommended packaging rules:

- commit the built `dist/` output so the skill works immediately after copy
- keep runtime dependencies minimal; prefer Node built-ins where possible
- keep the PowerShell shim in `bin/` so Windows agents can call the CLI with a stable entrypoint
- keep TypeScript sources in `src/` for maintenance, but the agent should call the built artifact by default

This avoids a hard requirement for `npm install` inside the destination skills directory.

## Runtime Model

The CLI becomes the single agent-facing execution surface.

Flow:

1. the skill determines the Unreal project involved in the task
2. the skill resolves the target Editor instance from project path and ini settings
3. the skill chooses a CLI command family and concrete command
4. the CLI sends the corresponding Unreal command over TCP
5. the CLI returns machine-readable JSON by default
6. the agent continues the workflow from that result

The skill remains responsible for:

- deciding whether the task belongs to the Unreal Editor workflow
- deciding whether an existing CLI command is enough
- deciding whether the plugin and CLI need to be extended
- continuing the original task after the capability gap is fixed

The CLI is responsible for:

- discovery
- connection
- parameter parsing
- request sending
- task polling
- stable output formatting
- self-describing help output

## Multi-Editor Target Resolution

The CLI must support multiple Unreal Editor instances at the same time.

The target key is the Unreal project, not only the port.

### Target selection rules

Preferred selector order:

1. explicit `--project <path>`
2. explicit `--port <number>`
3. explicit `--host <value>` plus `--port <number>`
4. auto-discovery from the current workspace when there is exactly one matching Unreal project with `SUnrealMcp`

If multiple candidate projects are found and the task did not identify one clearly, the skill should pause and align with the user.

### Port discovery rules

For a selected Unreal project, resolve host and port in this order:

1. explicit CLI flags
2. project user override ini files under `Saved/Config/*Editor/EditorPerProjectUserSettings.ini`
3. project config overrides such as `Config/DefaultEditorPerProjectUserSettings.ini`
4. project-local `Config/DefaultSUnrealMcp.ini` if present
5. plugin-local `Plugins/SUnrealMcp/Config/DefaultSUnrealMcp.ini`
6. built-in defaults from the plugin settings:
   - `BindAddress=127.0.0.1`
   - `Port=55557`

The setting section to read is:

```ini
[/Script/SUnrealMcp.SUnrealMcpSettings]
```

The keys that matter for target discovery are:

- `BindAddress`
- `Port`
- `bAutoStartServer`

If `bAutoStartServer` is false or the port does not answer, the skill should report that the target Editor is not ready rather than guessing another port silently.

## Command Surface Strategy

The CLI command names should stay close to the existing command names to reduce migration cost.

Recommended family layout:

- `help`
- `discovery`
- `system`
- `editor`
- `blueprint`
- `node`
- `umg`
- `project`
- `raw`

### Why a `raw` family should exist

The CLI should include a low-level escape hatch:

```text
sunrealmcp-cli raw send --command <name> --params-json <json>
```

This solves two problems:

- the agent can use a newly added Unreal command in the same session before a nicer CLI wrapper is added
- capability extension no longer blocks on a separate discovery protocol

The `raw` path is a fallback, not the preferred long-term UX.

## Help System Requirements

The CLI must be discoverable without opening the skill docs.

It should support these levels of help:

1. root help
2. family help
3. command help
4. machine-readable help

### Root help

Examples:

```text
sunrealmcp-cli --help
sunrealmcp-cli help
```

Root help should show:

- what the CLI is for
- the common global flags
- the available command families
- one-line descriptions for each family
- the recommended next command to inspect a family

### Family help

Examples:

```text
sunrealmcp-cli help node
sunrealmcp-cli node --help
```

Family help should show:

- the family purpose
- the family lifecycle sections: `core`, `stable`, `candidate`, `temporary`
- the commands in each section
- a one-line summary for each command

### Command help

Examples:

```text
sunrealmcp-cli help node inspect-graph
sunrealmcp-cli node inspect-graph --help
```

Command help should show:

- the exact command path
- the command purpose
- all supported parameters
- which parameters are required
- defaults for optional parameters
- example invocations

### Machine-readable help

The CLI should also support a JSON help mode so the skill and agents can inspect the command surface programmatically.

Recommended forms:

```text
sunrealmcp-cli help --json
sunrealmcp-cli help node --json
sunrealmcp-cli help node inspect-graph --json
```

The JSON help payload should include:

- family
- command
- lifecycle classification
- description
- parameter names
- parameter types
- required vs optional
- defaults when present

## Documentation And Help Must Match

The CLI help output and the reference files should be generated from the same command catalog metadata where possible.

The goal is:

- the agent can read docs when needed
- the agent can inspect the CLI directly with `help`
- a human can debug the CLI without opening the skill docs

Do not maintain separate drifting truth sources for:

- help output
- skill reference docs
- command metadata

## Migration From McpServer

The migration should be mechanical where possible.

### Reuse directly

- `src/protocol.ts`
- `src/transport/unreal_client.ts`
- task polling logic from `runtime/tool_helpers.ts`
- command names and parameter names from the current tool definitions

### Rewrite

- MCP-specific server bootstrap
- Zod-first tool registration
- MCP output envelope formatting

### Replace with CLI equivalents

- argument parsing
- command catalog metadata
- discovery commands
- JSON output formatter

## Skill Behavior After CLI Migration

The skill should keep the same high-level workflow as the current Unreal workflow skill.

The only major execution change is:

- before: use MCP tool
- after: run the embedded CLI

The skill should explicitly tell the agent:

- where the CLI lives inside the skill directory
- which command families exist
- to load only the family reference file that matches the current task
- which command should be preferred for a given operation
- when to use `raw send`
- how to detect the target project and port

## Capability Extension Loop In The CLI Version

The original self-extension loop stays intact.

Loop:

1. user asks for a UE asset or Editor task
2. skill routes the task into the Unreal workflow
3. agent tries the existing CLI command surface
4. if capability is missing, the agent edits the Unreal plugin and CLI code
5. the agent rebuilds or hot reloads the plugin
6. the agent immediately uses the updated CLI or `raw send` in the same session
7. the agent returns to the original task and finishes it

This is one of the main benefits of the CLI design: the agent no longer depends on MCP tool rediscovery just to consume newly added capabilities.

## Output Rules

The CLI should default to JSON output so the skill can teach agents to parse it reliably.

Recommended flags:

- `--json`
- `--pretty`
- `--timeout-ms`
- `--project`
- `--host`
- `--port`

When the CLI reports failure, it should preserve:

- the Unreal error code when present
- the Unreal error message
- the command name
- the target host and port

## Suggested Implementation Order

1. create the skill-local CLI skeleton and move protocol plus transport code into it
2. add discovery and target resolution
3. port the current stable command families
4. add candidate command families
5. add `raw send`
6. update `SKILL.md` and `SKILL_cn.md` to teach agents the CLI-first workflow
7. remove or archive the old root `McpServer/` after the CLI version is proven
