---
name: sunrealmcp-unreal-editor-workflow
description: Use this when a user task is related to a UE project and must interact with Unreal Editor assets or editor objects. Route the task into the SUnrealMcp CLI workflow, resolve the target Unreal project and port from the installed plugin and ini settings, use the appropriate CLI command family, extend the Unreal plugin and CLI when capability is missing, and continue the original task until it is complete.
---

# SUnrealMcp Unreal Editor Workflow

## Purpose

Use this skill for UE-project tasks that need real interaction with Unreal Editor, Unreal assets, or editor-managed objects.

This skill is now `CLI-first`.
It does not assume MCP tool discovery.

The agent should use the embedded CLI in this skill directory to talk to Unreal Editor through the `SUnrealMcp` plugin.

Current implementation status:

- the embedded CLI is now a real TypeScript project inside the skill directory
- the current migrated command surface is the core layer needed to validate the architecture
- additional family commands should be migrated into the same command-module structure incrementally

## Embedded CLI

Preferred entrypoints:

- Windows PowerShell:
  `cli/bin/sunrealmcp-cli.ps1`
- Direct Node entry:
  `cli/dist/sunrealmcp-cli.js`

The CLI is the execution surface.
The skill is the workflow and decision surface.

The CLI architecture is module-driven:

- family metadata is registered from command modules
- command definitions are registered from command modules
- the CLI entrypoint only scans and loads modules
- family names should not be hardcoded into the entry skeleton

## When To Use

Use this skill when both are true:

- the task is related to a UE project
- the task needs interaction with Unreal Editor, UE assets, or editor-managed objects

Typical cases:

- Blueprint, Widget Blueprint, AnimBP, Data Asset, Level, Actor, Component, graph node, or widget binding work
- reading graph semantics, metadata, or editor state
- creating, editing, compiling, or inspecting UE assets
- operating multiple Unreal Editor instances that are distinguished by different ports

## Do Not Use

Do not use this skill by default when:

- the task is only about normal source files, docs, configs, or build scripts
- the target does not require Unreal Editor semantics
- the user explicitly wants static analysis only and does not want editor interaction

## Core Rules

### 1. Unreal tasks default to the embedded CLI

If the task needs Unreal Editor interaction, do not default to raw file analysis or MCP resource discovery.

Use the embedded CLI first.

### 2. `SUnrealMcp` has no MCP resources

Do not call `list_mcp_resources` or `list_mcp_resource_templates` for `SUnrealMcp`.

This workflow assumes:

- Unreal-side capability lives in the plugin command system
- agent-side execution goes through the CLI

### 3. Asset-semantic requests require asset-semantic reads

If the user asks about Blueprint logic, graph structure, widget bindings, or editor metadata:

- do not treat `.uasset` string extraction as the main answer
- do not treat surrounding C++ references as the main answer
- do not stop at peripheral inference

If the current CLI and plugin cannot read the needed semantics, classify that as missing capability and extend them.

### 4. Missing capability is normal workflow, not failure

If the task cannot be completed with the current CLI surface:

- identify the minimum missing Unreal command or CLI wrapper
- add it
- rebuild or Live Code the plugin
- continue the original task

Do not stop at “the capability was added”.
Return to the original task and finish it.

## Progressive Disclosure

Keep the main skill lean.

Default loading order:

1. read this file for workflow rules
2. read [references/cli-command-catalog.md](./references/cli-command-catalog.md) to choose a command family
3. read exactly one family reference file unless the task genuinely spans multiple families

Do not load all family reference files by default.

## Command Routing

Use [references/cli-command-catalog.md](./references/cli-command-catalog.md) as the routing index.

That file tells the agent:

- which command families exist
- what each family is for
- which family reference file to load next

Each family reference file contains:

- the commands in that family
- their parameters
- their lifecycle sections: `core`, `stable`, `candidate`, `temporary`
- in the real CLI implementation, family metadata and command metadata should arrive through the same module-registration path

At the moment, the implemented CLI surface is centered on the `core` migration path:

- `discovery`
- `system`
- `raw`

The other families remain part of the intended extension structure and should be filled into the same TypeScript command-module architecture over time.

## Target Resolution

The CLI must support multiple Unreal Editor instances.

Default targeting rules:

1. prefer `--project <path>`
2. let the CLI inspect the target project's `SUnrealMcp` plugin and ini files
3. resolve `BindAddress` and `Port` from `[/Script/SUnrealMcp.SUnrealMcpSettings]`
4. only fall back to explicit `--host` and `--port` when needed

Do not silently guess another editor target if multiple valid projects exist.

If the task does not clearly identify which project to operate on and auto-discovery finds multiple targets, align with the user.

## Execution Loop

### Step 1. Freeze the original task

Capture the original user goal in one sentence and keep it stable.

All capability work exists only to unblock that task.

### Step 2. Resolve the target project and editor

Prefer the Unreal project as the main identity.

Let the CLI discover:

- whether `SUnrealMcp` is installed in the project
- which config files contribute settings
- which host and port the editor should be using

### Step 3. Choose a command family

Use the command catalog index first.

Then load only the family reference file that matches the current object layer:

- `discovery`
- `system`
- `editor`
- `blueprint`
- `node`
- `umg`
- `project`
- `raw`

### Step 4. Prefer mature commands first

Inside a family:

1. use `core` when the task is infrastructural
2. prefer `stable`
3. use `candidate` when needed
4. use `temporary` only as a bridge

### Step 5. Extend when needed

If no existing CLI command can complete the task:

- inspect the Unreal plugin command implementation
- inspect the CLI command surface
- add the minimum missing Unreal command and CLI wrapper

Default source-of-truth locations in this repository:

- Unreal plugin:
  `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands`
- Skill-embedded CLI:
  `Skill/sunrealmcp-unreal-editor-workflow/cli`

### Step 6. Rebuild, reload, continue

After capability changes:

- prefer Live Coding when appropriate
- otherwise do a full rebuild
- reload the command registry if needed
- use the updated CLI immediately

If a polished CLI wrapper is not ready yet but the new Unreal command exists, use the CLI `raw send` path as a temporary bridge in the same session.

### Step 7. Finish the original task

The workflow is complete only when the original user task is complete, not when the infrastructure work is complete.

## CLI Help

The CLI must be self-describing for both agents and humans.

Use CLI help:

- to inspect the live command surface
- to verify parameters
- to manually debug the CLI

Typical forms:

- `sunrealmcp-cli help`
- `sunrealmcp-cli help <family>`
- `sunrealmcp-cli help <family> <command>`
- `sunrealmcp-cli help --json`

Use the docs as the main guidance surface.
Use CLI help as runtime verification and manual debugging support.

## Completion Criteria

This workflow is complete only when all are true:

- the original user task is complete, or only a real external blocker remains
- the agent used the embedded CLI rather than drifting to unrelated paths
- the correct Unreal project and port were resolved or explicitly overridden
- any missing capability needed for the task was added at the correct layer
- asset-semantic requests were answered from actual Unreal-semantic reads, not only from peripheral inference
