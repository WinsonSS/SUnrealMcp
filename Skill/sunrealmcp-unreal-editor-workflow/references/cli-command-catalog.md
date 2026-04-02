# SUnrealMcp CLI Command Index

This file is the compact navigation layer for the skill-embedded CLI.

Read this file first.
Then load only the command-family reference that matches the task.

Current implementation status:

- the TypeScript CLI architecture is in place
- the currently migrated runtime command surface is `discovery`, `system`, and `raw`
- the other family documents describe the intended structure and migration targets

## Global Options

Common options for most commands:

- `--project <path>`: Unreal project root or `.uproject` path
- `--host <value>`: override resolved bind address
- `--port <number>`: override resolved port
- `--timeout-ms <number>`: socket or task timeout override
- `--json`: emit JSON output
- `--pretty`: pretty-print JSON

Default rules:

- prefer `--project` over `--port`
- resolve host and port automatically when `--project` is present
- print machine-readable payload to stdout

## Family Routing

Load only one of the following files unless the task truly spans multiple families.

- help:
  Use the CLI itself when the agent or user needs live command discovery, parameter lookup, or validation against the current implementation.
  Preferred forms:
  - `sunrealmcp-cli help`
  - `sunrealmcp-cli help <family>`
  - `sunrealmcp-cli help <family> <command>`
  - `sunrealmcp-cli help --json`

- discovery:
  [cli-discovery.md](./cli-discovery.md)
  Use when the agent needs to locate Unreal projects, detect whether `SUnrealMcp` is installed, or resolve host and port from ini files.

- system:
  [cli-system.md](./cli-system.md)
  Use for connectivity checks, command-registry reload, task polling, and task cancelation.

- editor:
  [cli-editor.md](./cli-editor.md)
  Use for level actors and scene editing.

- blueprint:
  [cli-blueprint.md](./cli-blueprint.md)
  Use for Blueprint asset creation, compilation, component changes, and Blueprint property edits.

- node:
  [cli-node.md](./cli-node.md)
  Use for Blueprint graph editing and graph inspection.

- umg:
  [cli-umg.md](./cli-umg.md)
  Use for Widget Blueprint creation and widget tree or binding operations.

- project:
  [cli-project.md](./cli-project.md)
  Use for project-level assets such as Enhanced Input mappings.

- raw:
  [cli-raw.md](./cli-raw.md)
  Use only when a formal CLI wrapper does not exist yet and the agent must call a newly added Unreal command in the current session.

## Progressive Disclosure Rule

The skill should not inline the full CLI catalog.

The main skill should contain only:

- when to use the CLI workflow
- how to identify the target Unreal project
- how to resolve or override the port
- how to choose a command family
- when to extend the plugin or CLI
- when to load one of the family reference files above

Command details such as subcommands and parameters should stay in the family reference files, not in the main skill body.

When the agent needs to confirm the live CLI shape instead of the planned design shape, prefer the CLI's built-in `help` output over the docs.
