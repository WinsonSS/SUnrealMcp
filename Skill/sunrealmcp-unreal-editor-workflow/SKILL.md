---
name: sunrealmcp-unreal-editor-workflow
description: Use this when a user task is related to a UE project and must interact with Unreal Editor assets or editor objects. Drive the task through the embedded SUnrealMcp CLI, inspect the live CLI help surface to discover command families and parameters, extend the Unreal plugin and CLI when capability is missing, and continue the original task until it is complete.
---

# SUnrealMcp Unreal Editor Workflow

## Purpose

Use this skill for tasks that need real interaction with Unreal Editor, UE assets, or editor-managed objects.

The skill owns workflow and decision-making.
The CLI owns connection, command execution, port lookup, returned results, and acts as the capability source of truth.

## Embedded CLI

Preferred entrypoints:

- Windows PowerShell:
  `cli/bin/sunrealmcp-cli.ps1`
- Direct Node entry:
  `cli/dist/sunrealmcp-cli.js`

## When To Use

Use this skill when both are true:

- the task is related to a UE project
- the task needs interaction with Unreal Editor, UE assets, or editor-managed objects

Typical cases:

- Blueprint, Widget Blueprint, AnimBP, Data Asset, Actor, Level, Component, graph node, or widget binding work
- reading graph semantics, editor metadata, or object state
- creating, editing, compiling, or inspecting UE assets
- operating multiple Unreal Editor instances as long as they use different ports

## Do Not Use

Do not use this skill by default when:

- the task is only about normal source files, docs, configs, or build scripts
- the target does not require Unreal Editor semantics
- the user explicitly wants static analysis only and does not want real editor interaction

## Core Rules

### 1. Unreal tasks default to the embedded CLI

If the task needs Unreal Editor interaction, do not default to raw file analysis and do not use any `SUnrealMcp` MCP resource path.

Use the embedded CLI first.

### 2. Asset-semantic requests require asset-semantic reads

If the user asks about Blueprint logic, graph structure, widget bindings, or editor metadata:

- do not treat `.uasset` string extraction as the main answer
- do not treat surrounding C++ references as the main answer
- do not stop at peripheral inference

If the current CLI and plugin cannot read the needed semantics, classify that as missing capability and extend them.

### 3. CLI help is the command catalog

Do not treat static markdown command catalogs as the capability source of truth.

When the agent needs command information, call CLI help in this fixed order:

1. If the correct family is not known yet, run `sunrealmcp-cli help --json`
2. Read the returned family list and choose the family that best matches the task
3. Run `sunrealmcp-cli help <family> --json`
4. Read the returned command list in that family and choose the command that best matches the task
5. Before actual execution, run `sunrealmcp-cli help <family> <command> --json`
6. Read the returned parameter definition and construct the real invocation from that schema

If CLI help does not show the required family or command, treat that capability as missing.

### 4. Missing capability is part of the main workflow

Judge capability existence only by the command surface currently exposed by the CLI.

If the CLI does not expose a command needed for the task:

- treat that capability as missing
- add the minimum Unreal command and the minimum CLI wrapper
- rebuild or use Live Coding
- continue the original task

If implementation work reveals that the Unreal side already has an older command:

- reuse, replace, wrap, or converge it
- but keep the new CLI command surface as the agent-facing truth

Do not stop at “the capability was added”.
Return to the original task and finish it.

## Target Project And Port Discovery

The CLI supports multiple Unreal Editor instances.
Specify the UE project directory through `--project <project_path>`.

## Execution Loop

### 1. Freeze the original task

Capture the real user task in one sentence.

All capability work exists only to finish that task.

### 2. Locate the target project

Locate the UE project and obtain its project-root path.

### 3. Discover the live CLI surface

Use CLI help to inspect the currently available families and commands.

Follow the exact call order from “Core Rule 3. CLI help is the command catalog”.

Do not guess command names from memory.

Construct the final invocation from the parameter definitions returned by help.

### 4. Prefer mature commands first

Inside the families and commands currently exposed by the CLI, prefer in this order:

1. use `core` for infrastructural work
2. prefer `stable` for formal capabilities
3. use `temporary` only as a temporary capability

### 5. Extend when needed

If the CLI does not expose a command that can complete the task:

- inspect the CLI command surface
- add the minimum Unreal command and the minimum CLI wrapper
- add the new CLI command and Unreal command as `temporary`

Default source-of-truth locations in this repository:

- Unreal plugin:
  `ProjectRootPath/Plugins/SUnrealMcp/Source/SUnrealMcp/Private/Commands`
- Skill-embedded CLI:
  `Skill/sunrealmcp-unreal-editor-workflow/cli`

### 6. Rebuild, reload, continue

After adding capability:

#### Unreal side (C++ command)

- first call `sunrealmcp-cli system refresh_cpp_runtime`
- poll the returned task with `sunrealmcp-cli system get_task_status --task_id <id>`
- branch on the structured result:
  - `succeeded`: continue the original task immediately
  - `failed` with `error.details.resolution = "agent_retry"`: fix the code and retry `refresh_cpp_runtime`
  - `failed` with `error.details.resolution = "needs_user_editor_action"`: ask the user to do the lightweight editor action first, such as stopping PIE, then retry
  - `failed` with `error.details.resolution = "needs_user_approval"`: explain why the heavy path is needed and ask the user before closing the editor or doing a full rebuild
- only fall back to a full rebuild after `refresh_cpp_runtime` reports that Live Coding is unavailable for the current environment
- do not automatically close the editor or trigger a full rebuild without explicit user approval

#### CLI side (TypeScript wrapper)

If you added or modified TypeScript files under `cli/src/`, you must compile before the changes take effect:

```bash
cd Skill/sunrealmcp-unreal-editor-workflow/cli && npm run build
```

This compiles `src/` into `dist/`. The CLI reads from `dist/` at runtime.

#### Then continue

Once both sides are ready, continue the original task immediately. The newly added commands are now available.

### 7. Finish when the original task is finished

This workflow is complete only when the original user task is complete.

## CLI Response Format

The default command output is a minimal JSON envelope:

- Success: `{“ok”: true, “data”: {…}}`
- Failure: `{“ok”: false, “error”: {“code”: “…”, “message”: “…”}}`

Diagnostic fields (`target`, `cli`, `unreal`, `raw`) are omitted by default to reduce token cost. To include them, pass `--verbose`:

```bash
sunrealmcp-cli <family> <command> --verbose
```

Agent workflows should use the default (slim) format. Use `--verbose` only for human debugging.

## CLI Help

When the goal is agent execution, prefer `--json` help output.

Follow the exact usage rules from “Core Rule 3. CLI help is the command catalog”.

Use non-JSON help only for human-readable manual debugging.

### Global options reference

JSON help output only contains command-specific information. Global options are listed here for reference:

- `--project <path>` — Project root or .uproject path
- `--host <string>` — Override resolved bind address
- `--port <number>` — Override resolved port
- `--timeout_ms <number>` — Socket timeout override. Use the value exposed by `sunrealmcp-cli help` and the CLI runtime as the default source of truth; only pass this explicitly when the default timeout is not sufficient for the current command.
- `--pretty` — Pretty-print JSON command output
- `--verbose` — Include diagnostic fields in command output

Help-only option: `--json` — Emit JSON help output.
