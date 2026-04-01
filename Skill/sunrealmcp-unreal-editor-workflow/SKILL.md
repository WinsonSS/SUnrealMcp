---
name: sunrealmcp-unreal-editor-workflow
description: Use this when a user task is related to a UE project and must interact with Unreal Editor assets or editor objects. Default to SUnrealMcp first, use the existing MCP tool and plugin command surface, automatically add the minimum missing tool or command when capability is missing, rebuild or Live Code, refresh discovery, and then resume the original task until it is complete.
---

# SUnrealMcp Unreal Editor Workflow

## Purpose

Use this skill for UE-project tasks that need real interaction with Unreal Editor, Unreal assets, or editor objects.

The main rule is simple:

1. If the task must touch Unreal Editor state or UE asset semantics, do not default to file-first analysis.
2. Default to `SUnrealMcp` and try the current MCP `tool` surface first.
3. If the required Unreal Editor operation is missing, add the minimum missing capability, rebuild or hot-reload it, refresh discovery, and then continue the original task.
4. Stay in that loop until the original task is actually finished.

This skill is not only for "use existing tools". It is also the default recovery workflow for "the task is valid, but the current `McpServer` or plugin command surface is incomplete".

## Trigger Conditions

Use this skill when both are true:

- the user task is related to a UE project
- the task needs interaction with Unreal Editor, UE assets, or editor-managed objects

Typical triggers:

- read, inspect, summarize, query, modify, create, compile, or connect Blueprint, Widget Blueprint, AnimBP, Data Asset, Level, Actor, Component, graph node, input mapping, or other UE asset/editor object
- inspect graph logic, object metadata, references, hierarchy, generated classes, exposed properties, or editor state that cannot be trusted from raw file text alone
- perform an Editor action such as spawn actor, inspect actors in level, compile Blueprint, add nodes, bind widget events, or other editor-executed operations
- continue a UE task after discovering that the current SUnrealMcp capability is missing or incomplete

## Do Not Use This Skill

Do not use this skill by default when any of these is true:

- the task is only about normal source files, docs, configs, scripts, or build logic with no Unreal Editor interaction
- the target is only plain text such as `md`, `json`, `toml`, `ini`, or ordinary source code and does not need UE asset semantics
- the user explicitly wants static analysis only and does not want editor interaction or workflow execution
- the real task is a protocol redesign, transport rewrite, or architecture refactor rather than completing a UE-editor-facing operation

## Core Operating Model

Treat `SUnrealMcp` as a tool-command bridge:

- `McpServer/src/tools/*` exposes MCP `tools`
- `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/*` implements Unreal-side `commands`
- the command registry is the runtime source of truth for what the Editor can execute

Default mental model:

- the user asked for a UE task
- the agent should try the current `tool` surface
- if the needed operation is missing, the agent should extend the bridge instead of abandoning the task
- after the bridge is fixed, the agent should immediately return to the original UE task

## Hard Constraints

These rules are mandatory for this skill.

### Constraint 0. In this repository, the default source of truth is already known

When this skill is being used inside the `SUnrealMcp` repository, do not behave as if the implementation location is unknown by default.

Default source of truth in this repo:

- `McpServer/src/tools/*` for MCP tool exposure
- `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/*` for Unreal command implementation
- the matching runtime registry and nearby helper files under the same plugin source tree

Mandatory rules:

- inspect these locations before claiming that the server-side or plugin-side entry point is unknown
- do not say “I cannot add a new MCP function entry myself” before checking these locations
- do not downgrade into peripheral inference just because the first search did not immediately reveal the exact file
- if these locations exist and are writable, treat capability extension as available work, not as an external blocker

Only classify the source of truth as ambiguous when there is direct evidence that the running Editor is using a different plugin copy or a different `McpServer` tree.

### Constraint 1. `SUnrealMcp` has no MCP resources

`SUnrealMcp` is not a resource-oriented MCP server.
It has no `resources` and no `resource templates`.
Its usable interaction surface is the `tool` to Unreal `command` bridge.

Mandatory rules:

- use MCP `tools` first
- inspect `McpServer/src/tools/*` first
- inspect Unreal plugin `Commands/*` first
- do not call `list_mcp_resources`
- do not call `list_mcp_resource_templates`
- do not plan around a future resource interface unless the user explicitly asks to design one

If the current tool surface cannot complete the task, return directly to tool-command gap analysis.
Do not create a resource-discovery branch, and do not infer from missing resources that raw asset inspection is the correct fallback.

### Constraint 2. Confirmed UE assets must not degrade into raw `.uasset` analysis

Once the target has been confirmed to be a Blueprint, Widget Blueprint, AnimBP, Data Asset, or another UE editor-managed asset:

- do not make `strings` on `.uasset` the main workflow
- do not make repository-wide grep the main workflow
- do not present byte-level clues as if they were editor-semantic truth

Allowed use of raw asset inspection:

- quick orientation when object identity is still uncertain
- emergency clue gathering before designing a missing command
- sanity checks that support, but do not replace, Editor-semantic operations

If the user needs graph semantics, node relationships, editor metadata, or asset mutation, the correct next step is to use or extend `SUnrealMcp`, not to deepen `.uasset` text extraction.

### Constraint 3. Asset-semantic requests must not degrade into peripheral inference

When the user is asking about the asset itself, such as:

- what logic exists inside this Blueprint
- what nodes, graphs, variables, bindings, or metadata it has
- why this asset behaves a certain way
- how to modify this asset safely

then source-code references, call sites, tags, comments, naming clues, or nearby usage patterns are not a valid substitute for reading the asset semantically.

Mandatory rules:

- do not treat surrounding C++ references as a successful answer to an asset-semantic request
- do not stop at “I can infer how it is probably used”
- do not reframe “read the Blueprint” into “analyze who references the Blueprint”
- do not treat ALS flow inference as completion when the user asked for the Blueprint's own logic

Allowed use of peripheral inference:

- to estimate what missing command should be added
- to prioritize which graph, function, or metadata view is needed first
- to provide temporary orientation while implementing the missing Unreal capability

If the requested answer depends on the Blueprint's own graphs, nodes, properties, bindings, or editor metadata, and the current tool surface cannot read them, the workflow must classify this as missing capability and extend `SUnrealMcp`.

## Execution Loop

Use this loop as the primary workflow, not as a fallback.

### Step 1. Freeze the original task

Before making capability changes, capture the original user goal in one sentence and keep it stable.

Examples:

- inspect why a Blueprint graph behaves a certain way
- add a widget binding in a Widget Blueprint
- create or modify an actor in the current level
- read or change properties on a Blueprint or component

All capability work exists only to unblock that original task.

### Step 2. Decide whether this is an Unreal Editor workflow

If the target is a UE asset or editor object, enter this workflow immediately.

Default to `unknown Unreal object` when the user gives only a UE-style name. Identify the object type first instead of assuming it is a normal file or C++ symbol.

Priority order:

1. determine whether it is a Blueprint, Widget Blueprint, AnimBP, Data Asset, Actor, Level object, graph node target, or another editor-managed object
2. determine which Editor operation is required
3. only use generic source search as support when the Unreal object identity is still unclear

If it is confirmed to be a UE asset or editor object, stop treating raw file search as the primary path.

### Step 3. Establish the runtime target

Before editing code, identify the active execution target:

- the UE project involved in the task
- the `McpServer` instance or source tree that provides the MCP tools
- the `SUnrealMcp` plugin copy that is actually loaded by the running Unreal Editor
- whether the Editor is open and whether Live Coding is available

If multiple plugin copies exist, default to the copy that the running Editor is really using.
Do not silently edit mirror copies or backup copies.

When the active source of truth is unclear, pause only long enough to resolve that ambiguity.
Inside this repository, do not call it unclear until the default `McpServer/src/tools` and `UnrealPlugin/SUnrealMcp/.../Commands` locations have actually been checked.

### Step 4. Try existing capability first

Always attempt the smallest existing capability path first:

1. inspect the current `tool` surface
2. see whether an existing tool already solves the task
3. see whether an existing command already exists under a nearby name or family
4. prefer parameter or `mode` extension over a brand new tool

Do not add a new tool just because the first obvious tool name does not exist.
Do not switch to `list_mcp_resources`, `list_mcp_resource_templates`, `grep`, or `strings` as the new primary path just because the first tool attempt failed.

### Step 5. Classify the gap

If the task cannot proceed, classify the failure before coding.

Use one of these buckets:

- `no_gap`: the current tools already support the task and the issue is only wrong tool choice or wrong parameters
- `server_only`: Unreal plugin command exists, but `McpServer` does not expose the needed tool or mapping
- `plugin_only`: `McpServer` expects the operation, but the Unreal plugin lacks the command or runtime behavior
- `both_missing`: neither side has the required capability
- `reload_only`: code exists, but the running Editor session or registry has not picked it up yet
- `wrong_abstraction`: the requested operation should be solved by extending an existing tool or command rather than creating a separate one

Design only the minimum capability needed to complete the frozen original task.

Failure routing rules:

- if a tool call fails because the command is unknown or absent, classify it as `plugin_only` or `both_missing`
- if the plugin command exists but no MCP tool exposes it cleanly, classify it as `server_only`
- if a task suggests using MCP resources, reject that path for `SUnrealMcp` and return to gap classification
- if raw `.uasset` inspection only yields names, strings, or partial metadata but not the editor semantics the task needs, classify the situation as missing Unreal capability rather than “analysis complete”
- if surrounding code search only yields references, call sites, asset paths, or inferred behavior but not the asset's own editor semantics, classify the situation as missing Unreal capability rather than “analysis complete”
- if the agent has not yet inspected the default tool and command source trees in this repository, it must not claim that capability extension is unavailable

### Step 6. Apply the minimum capability change

Use this priority order:

1. `reuse`
2. `extend`
3. `merge`
4. `add`
5. `deprecate` or `delete` only when it is clearly safe and directly helpful

Default rules:

- prefer capability-shaped names, not task-shaped names
- prefer adding one parameter or one `mode` when that keeps the abstraction clean
- keep the MCP `tool` contract aligned with the Unreal `command` contract
- keep changes narrow enough that they can be validated quickly and then reused immediately

### Step 7. Edit both sides when needed

When a capability change is required, update the correct surface:

For `McpServer`:

- add or extend the tool definition in `McpServer/src/tools/core`, `stable`, `candidate`, or `temporary`
- keep naming consistent with existing families such as `blueprint`, `editor`, `node`, `umg`, `project`, or `system`
- use a clear schema and stable parameter names
- map tool parameters to the exact Unreal command contract

For Unreal plugin:

- add or extend the command implementation under `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/...`
- keep command names exactly aligned with what the `McpServer` expects
- register the command so the runtime registry can execute it
- return structured, actionable errors

Do not change protocol or transport skeletons unless the task is truly blocked there.

## Build And Reload Policy

After changing capability, do not jump back to the user task until the new path is actually loadable.

### Preferred order

1. build or validate `McpServer`
2. apply Unreal-side code changes
3. use `Live Coding` first when the Editor is already open and the change is suitable for hot reload
4. if Live Coding is not enough, close the Editor and do a full rebuild
5. refresh command registration
6. refresh MCP tool discovery if needed
7. rerun the blocked part of the original task

### Current-session execution policy

Treat “formal capability exists in code” and “current session can invoke it as an MCP tool” as separate questions.

Mandatory rules:

- if the current session can refresh the new capability, do so and continue through the formal MCP tool path
- if the current session cannot refresh the new MCP tool surface, still complete the formal code change first
- after the formal code change is in place, use a temporary execution bridge in the current session when needed instead of downgrading to peripheral inference

The preferred temporary bridge is a local script or PowerShell invocation that directly exercises the newly added capability through the real project runtime.
This bridge is for continuing the blocked task in the current session only.
It does not replace the formal `McpServer` tool definition.

### Live Coding first

Prefer Live Coding when:

- the Editor is already open
- the change is a normal command implementation or nearby helper change
- there is no evidence that a full restart is required

### Escalate to full rebuild when needed

Use close-editor plus full rebuild when any of these is true:

- Live Coding fails
- the change is not safely hot-reloadable
- registration or module initialization is not refreshed correctly after Live Coding
- the running Editor still cannot execute the command after a reload attempt

If a full rebuild will interrupt the user's current Editor session, align with the user first.
Otherwise, default to finishing the unblock automatically.

### When MCP tool discovery cannot refresh in-place

If the Unreal-side command is ready but the current session still cannot see the newly added MCP tool:

1. confirm the formal tool and command implementation is correct
2. use a temporary script or shell bridge to invoke the new capability in the current session
3. continue the original task with that bridge
4. report clearly that the formal MCP tool will require session refresh or rediscovery for future direct use

Do not treat “current session cannot see the new tool yet” as permission to switch to raw asset inspection or peripheral inference.

## Runtime Refresh Rules

After Unreal-side changes, refresh runtime state explicitly.

Minimum expectations:

- the command is compiled into the active plugin
- the command is registered in the runtime command registry
- the MCP side can see the intended tool surface
- a small smoke call proves the bridge works

Use the existing `reload_command_registry` capability when appropriate.
Do not assume a newly added command is available until it has been reloaded or rebuilt into the running session.

If the current session cannot discover the new MCP tool after formal refresh attempts:

- verify the command itself through a temporary local bridge
- verify the end-to-end task path through that bridge if needed
- keep the formal MCP tool code in place for the next refreshed session

The inability to refresh discovery in-place is a session limitation, not evidence that the capability design was wrong.

## Resume Rule

After the missing capability has been validated, immediately return to the original task.

Do not stop after reporting that:

- a new tool was added
- a new command was added
- Live Coding succeeded
- the registry reloaded successfully

Those are intermediate milestones only. The workflow is not done until the original UE task is done.

If the original task was to read, inspect, or explain a Blueprint or other UE asset semantically, the workflow is still incomplete until that asset has actually been read semantically through `SUnrealMcp` and the answer is based on that result.

If the formal MCP tool is not yet callable in the current session, resume the original task through the temporary execution bridge rather than stopping early.

## Search And Inspection Rules

Use code search as support, not as the primary execution path, when the task is really about UE assets.

If the target is already confirmed to be a Blueprint, Widget Blueprint, graph, or other editor object:

- raw `.uasset` inspection is only a clue
- generic file scanning is only a clue
- normal text search is only a clue
- resource-style MCP entry points are not applicable to `SUnrealMcp`
- surrounding source references are only clues unless the user explicitly asked for reference analysis

Prefer Editor-semantic operations through `SUnrealMcp` whenever the task depends on actual asset meaning or editor state.

When the agent discovers an asset path such as `Content/.../*.uasset`, the next decision must be:

1. is there already a tool or command that can inspect or mutate this asset semantically?
2. if not, what is the minimum tool or command addition needed?

That discovery must not automatically trigger a raw asset parsing workflow.
That discovery also must not automatically trigger a “look at surrounding C++ logic instead” workflow when the user's target is the asset itself.

Default search boundaries when code inspection is necessary:

- include `McpServer/src/tools`
- include `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp`
- exclude `Intermediate`, `Binaries`, `Saved`, `.vs`, `DerivedDataCache`, `node_modules`, and irrelevant mirror copies unless direct evidence says otherwise

## Tool Lifecycle And Convergence

Do not grow the tool surface carelessly.

### Lifecycle guidance

- `core`: essential system or workflow plumbing
- `stable`: well-shaped, reusable capability that should be a normal default
- `candidate`: useful but still settling capability
- `temporary`: short-lived unblocker created mainly to finish the current task

### Convergence rule

Whenever a new tool or command is introduced, decide whether it should be:

- `keep`
- `merge`
- `promote`
- `deprecate`
- `delete`

Default behavior:

- avoid creating project-specific one-off names
- prefer folding overlap back into an existing family
- do not promote to `stable` unless the capability is clearly reusable and well-shaped

## When To Pause And Align With The User

Continue automatically by default. Pause only for decisions with real downside.

Ask or align when any of these is true:

- the active UE project or active plugin copy is ambiguous
- the task would close the user's live Unreal Editor session and that interruption matters
- the change would be destructive to user content, not just tooling
- the change would broaden into protocol redesign or large-scale refactor
- the best abstraction choice has non-obvious long-term consequences

Do not pause merely because the exact MCP tool file or command file was not obvious on the first pass.
Search the default source-of-truth directories first and keep moving.

Do not pause just because a new tool or command must be added.
That is a normal part of this workflow.

## Completion Criteria

This workflow is complete only when all of these are true:

- the original user task is complete, or only a real external blocker remains
- the needed Unreal operation works through the active SUnrealMcp bridge
- the `tool` contract and plugin `command` behavior match
- any new or changed command is compiled, registered, and discoverable
- any temporary capability introduced during the turn has an explicit convergence decision
- the agent has returned to the original task instead of stopping at infrastructure completion
- asset-semantic requests are answered from actual asset-semantic reads, not only from peripheral inference
- if the current session could not directly consume the new MCP tool, the original task was still completed through a temporary execution bridge and that limitation was stated explicitly

## Compact Pseudocode

```text
if task is UE-project-related and needs Unreal Editor interaction:
    freeze original task
    identify active UE project, active plugin copy, and active McpServer surface
    while original task is not complete:
        try existing SUnrealMcp tools and commands
        if blocked by missing capability:
            classify the gap
            implement the minimum tool and/or command change
            Live Code or rebuild as needed
            reload command registry and refresh discovery
            if current session can call the new MCP tool:
                run a smoke check through the formal tool
            else:
                use a temporary script or shell bridge
                run a smoke check through that bridge
        resume the original task
```
