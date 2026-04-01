name: sunrealmcp-unreal-editor-workflow
description: Use this as the primary Unreal Editor workflow when a task needs to inspect, modify, generate, validate, or otherwise interact with Unreal Editor content. Default to SUnrealMcp as the interface layer for editor interaction, then reuse, extend, merge, deprecate, delete, or add tools and matching commands as needed, validate the stack, refresh tool discovery, and continue the original task instead of stopping at analysis.
---

# SUnrealMcp Unreal Editor Workflow

## Purpose

Use this as the primary workflow whenever a task needs to interact with Unreal Editor content or state.

The default expectation is:

1. Recognize that the task belongs to Unreal Editor automation or editor interaction.
2. Prefer `SUnrealMcp` as the default interface layer for interacting with the editor.
3. If the current `SUnrealMcp` surface is sufficient, use it directly.
4. If a real capability gap exists, extend the stack and then continue the original task.
5. If the `tool` surface has become duplicated, over-narrow, or stale, converge it before closing the task.

The goal is not to accumulate `tools` endlessly. The goal is to keep `SUnrealMcp` as the default Unreal Editor automation surface while preserving a small, composable, maintainable, and accurate `tool` set.

## Core Intent

Follow this loop:

1. Identify whether the user task requires interaction with Unreal Editor, Unreal assets, Blueprint graphs, UMG, actors, project settings, or other editor-managed state.
2. Default to `SUnrealMcp` as the first interaction surface for that work.
3. Try to solve the user's task with existing `tools`.
4. Identify whether a real capability gap exists.
5. Decide whether to reuse, extend, merge, deprecate, delete, or add a `tool`.
6. Make the minimum necessary changes in:
   - `McpServer`
   - the active in-project plugin copy at `Plugins/SUnrealMcp`
7. Rebuild and validate.
8. Refresh the `tool` view.
9. Return to the original task and finish it.
10. If cleanup triggers apply, converge the `tool` surface before closing the task.

## When To Use

Use this workflow when the task clearly involves Unreal Editor interaction, Unreal content automation, or Unreal editor-side inspection or mutation.

This includes situations where the agent has not yet called `SUnrealMcp`, but should strongly consider it first.

Typical triggers:

- Blueprint creation, mutation, graph editing, or graph inspection
- UMG widget creation, mutation, binding, or viewport-related work
- Actor, level, or editor-world inspection and mutation
- Project-side Unreal asset or input configuration changes
- Tasks that need to inspect or modify editor-managed assets or state
- Tasks where `SUnrealMcp` is the most natural interface even if the exact needed `tool` is not exposed yet

Typical examples:

- The task needs Unreal Editor interaction, but the currently visible `SUnrealMcp` `tools` do not clearly cover it yet.
- A Blueprint workflow needs a node operation that is not currently exposed.
- `McpServer` has the `tool` concept, but the plugin lacks the matching `command`.
- The plugin has the capability, but `McpServer` does not expose it as a `tool`.
- A long-running workflow needs a task-aware `tool`, but no such capability exists yet.
- Several overlapping `tools` now point at the same capability and should be merged or deprecated.
- A temporary `tool` added to unblock a task should now be reviewed for cleanup.

## When Not To Use

Do not use this skill when:

- The task does not require Unreal Editor interaction at all.
- Existing `tools` can already solve the problem in combination.
- The issue is only wrong parameters, wrong call sequence, or misunderstanding.
- The required work is a protocol-level redesign rather than `tool` / `command` extension or convergence.
- The user explicitly asked for analysis only.

## Scope

This skill supports different repository layouts. The paths below are common examples, not hardcoded requirements:

- `McpServers/SUnrealMcp/McpServer`
- `McpServers/SUnrealMcp/UnrealPlugin/SUnrealMcp`
- `Plugins/SUnrealMcp`

Before making changes, first identify these logical targets in the current workspace:

- the `McpServer` directory
- the in-project `SUnrealMcp` plugin directory that is actually compiled and used
- any external repository copy or mirror plugin directory that may also exist

For plugin implementation work, default to editing only the in-project copy. Do not update the external repository copy by default unless the user explicitly asks for it.

### Capability Audit Scope Rule

When checking whether `SUnrealMcp` already has the needed capability but has not exposed it correctly, default to auditing only these two implementation surfaces:

- `McpServer/src/tools`
- `Plugins/SUnrealMcp/Source/SUnrealMcp` command implementations and command registration

Treat these as the primary and usually sufficient audit targets: `tools` in `McpServer`, and `Command` implementations or registration in the active `SUnrealMcp` plugin.

Do not expand the search scope by default to other project directories just to confirm capability presence.

In particular, do not scan unrelated or high-cost directories such as:

- `Binaries`
- `Intermediate`
- `Saved`
- `.vs`
- `node_modules`
- unrelated plugins
- unrelated MCP servers
- external repository copies or mirror plugin directories

Only widen the audit scope if the user explicitly says the active `SUnrealMcp` implementation is elsewhere, or if direct evidence inside the default `tools` or `Command` locations points to another required source-of-truth location.

### Source Of Truth Rule

If both an external repository copy and a local active plugin copy exist, do not edit both by default.

Use this default order:

1. Edit the active in-project plugin copy that actually participates in compilation.
2. Leave any external repository copy or mirror plugin unchanged unless the user explicitly asks to update it too.
3. Sync both only if the user explicitly wants both updated in the same task.

In other words:

- The active in-project plugin is the default implementation target.
- The external repository plugin is a manual sync target.

### Domain Categories Are Not Fixed Taxonomy

Directory or family names such as `Blueprint`, `Editor`, `Node`, `UMG`, `Project`, or `System` should be treated as the current organizational grouping for maintainability, not as immutable long-term taxonomy.

The lifecycle layer is the stable structure:

- `core`
- `stable`
- `candidate`
- `temporary`

The domain grouping is allowed to evolve over time. It may be regrouped, merged, split, or renamed when that makes the capability surface clearer and easier to maintain.

## Core Workflow

### 1. Try Existing Capabilities First

Before adding anything, do these checks:

- Restrict capability-audit search to `McpServer/src/tools` and the active plugin's `Command` implementations or registration under `Plugins/SUnrealMcp/Source/SUnrealMcp` by default.
- Enumerate the MCP `tools` relevant to the current task.
- Check whether the task can be solved by combining existing `tools`.
- Check whether the plugin already has a similar `Command` under a different name or without MCP exposure.
- Check whether the problem can be solved by extending an existing `tool` with a new parameter or `mode` instead of adding a new `tool`.

Do not add a new `tool` just because it seems convenient.
Do not turn this step into a workspace-wide repository scan.

### 2. Perform Gap And Convergence Analysis

Classify the missing capability or cleanup target as one of the following:

- `server_only`
  The plugin can conceptually do it, but `McpServer` does not expose it.
- `plugin_only`
  `McpServer` can have the `tool` shape, but the plugin lacks the `command`.
- `both_missing`
  Both sides are missing it.
- `wrong_abstraction`
  The stack already has related capability, but the abstraction is too narrow, fragmented, overlapping, or stale.

Design only the minimum capability needed to complete the original user task. Do not casually turn this into a broad framework unless the current task truly requires that.

### 3. Choose The Smallest Correct Change

Use this priority order:

1. Reuse an existing `tool`.
2. Extend an existing `tool` with backward-compatible parameters or `mode`.
3. Merge two or more overly narrow `tools` into a more general `tool`.
4. Deprecate or delete a stale `tool` if another `tool` already covers its value.
5. If the above are not enough, add a new `tool` and matching `command`.

If a new `tool` is required, it should be:

- narrowly scoped
- clearly named
- consistent with existing `tool` families
- easy to discover and understand from the name alone

## Tool Hygiene And Convergence Policy

Too many `tools` degrade the agent's tool selection quality. Every `tool` change must be treated as both a capability decision and a maintenance decision.

### Tool Lifecycle Classes

`core`

- broadly useful across many tasks
- foundational to other workflows
- not a likely deletion candidate

`stable`

- repeatedly proven valuable across multiple independent tasks
- naming, `schema`, and behavior are stable enough to be part of the long-term capability surface
- should be maintained as long-term supported `tool` / `command` surface

`candidate`

- survived at least one convergence review and should not be deleted immediately
- likely useful in future tasks, but not yet proven enough to become long-term stable capability
- is an observation layer between `temporary` and `stable`, not a permanent parking lot

`temporary`

- mainly added to unblock the current task
- narrow, project-shaped, or likely replaceable
- must be reviewed before the task closes or immediately after the original task is complete
- does not imply long-term support by default

### Default Rule

Default to not adding a new `tool`.

If the problem can be solved by:

- a better prompt
- using a different existing `tool`
- a small parameter extension
- combining multiple existing `tools`

then do that instead.

### Convergence Triggers

Run a convergence review when any of the following happens:

- A new `tool` is about to be added.
- Multiple `tools` overlap semantically.
- A `tool` name has become project-shaped instead of capability-shaped.
- The number of `tools` in a domain becomes hard to scan.
- A temporary `tool` was added during the current task.
- The original user task has finished and this turn added or changed temporary capability.
- A `tool` silently became a thin wrapper around another `tool` or workflow.

### Convergence Decision Rule

When a trigger fires, explicitly decide whether each candidate `tool` should:

- `keep`
- `merge`
- `deprecate`
- `delete`

Do not leave the answer implicit.

The purpose of a convergence review is to decide whether a capability should keep existing, and in what form.

A convergence review by itself is not enough to promote something directly to `stable`. At most, one review can show that a `tool`:

- should not keep existing
- should be merged
- should be deprecated
- should remain under observation

If a `temporary` `tool` survives the review, default to moving it to `candidate`, not directly to `stable`.

### Tool Inventory Rule

`Tool` classification and convergence decisions must not live only in short-term reasoning.

When a task adds, merges, deprecates, or deletes a `tool`, record the decision in a lightweight `tool inventory` note if the workspace already has one for `SUnrealMcp`. Prefer a file named `tool-inventory.md` near the persisted `tool` definitions or in the `SUnrealMcp` root. If no such file exists, do one of the following:

- add a minimal inventory file if the current task is already changing the repository that should persist `tool` definitions
- otherwise include the classification and convergence recommendation explicitly in the final summary

At minimum, persist:

- `tool` name
- classification: `core`, `stable`, `candidate`, or `temporary`
- action: `keep`, `merge`, `deprecate`, or `delete`
- short reason

Preferred minimal entry format:

```md
- tool: <name>
  classification: <core|stable|candidate|temporary>
  action: <keep|merge|deprecate|delete>
  reason: <short reason>
```

### Convergence Questions

For each candidate `tool`, ask at least:

- Will other future tasks likely need it?
- Is the name project-specific instead of capability-specific?
- Can existing `tools` compose to replace it?
- Is it only wrapping a sequence that adds almost no value?
- Does the maintenance cost on both sides exceed the benefit?
- Would removing or merging it make the overall `tool` surface easier for future agents to choose from?
- Are the input/output `schema` and behavior actually stable, or likely to change again soon?
- Was it only useful in this one task, or has it already shown cross-task reuse value?
- Does validation go beyond "worked once" and include a credible smoke path?

If the answers suggest low reuse, high overlap, and high maintenance cost, prefer `merge`, `deprecate`, or `delete` over `keep`.

If the answers only justify "keep observing this" but do not justify "make this part of the long-term surface", classify it as `candidate`, not `stable`.

### Promotion Rules

Default promotion path:

`temporary` -> `candidate` -> `stable`

#### `temporary` -> `candidate`

Require all of the following:

- the current convergence review does not conclude `merge`, `deprecate`, or `delete`
- there is no strong semantic overlap with an existing `stable` or `candidate` capability
- it is not just a one-task hardcoded workflow script
- it has at least a minimal validation path, including a credible `smoke test`

#### `candidate` -> `stable`

Require all of the following:

- it has proven useful across multiple independent tasks
- the name is capability-shaped rather than project-shaped or task-shaped
- the input/output `schema` and behavior are mostly stable
- the `McpServer` `tool` and Unreal plugin `command` contract are clear and unlikely to churn
- maintainers are willing to treat it as part of the long-term supported surface

If that evidence is missing, keep it as `candidate` instead of promoting it just because it looked useful once.

### Allowed Convergence Outcomes

Preferred order:

1. Merge when possible.
2. Use `deprecate` when immediate deletion is still uncertain.
3. Delete only when there is no expected dependency.

Do not casually delete or rename widely used `tools`.

### End-Of-Task Convergence Rule

Before closing a task that added or materially changed a `tool`, perform a final convergence pass.

At minimum:

- review all `temporary` `tools` touched in this task
- review whether any `candidate` `tools` touched in this task now satisfy promotion criteria
- review overlapping `tools` in the same family
- decide whether each one should `keep`, `merge`, `deprecate`, or `delete`
- decide whether any `temporary -> candidate` or `candidate -> stable` promotion should happen
- persist or report the result

## McpServer-Side Rules

When editing `McpServer`:

- Keep naming aligned with existing Blueprint, Editor, Node, Project, and UMG `tool` families.
- Keep parameter names stable and clear.
- Make the `schema` strong enough to guide the agent, but do not over-constrain valid inputs without backend need.
- Ensure the `tool` definition matches the real plugin `command` contract.
- Keep convergence-safe migration in mind when changing or retiring a `tool`.
- Do not touch the `protocol` and `transport` skeleton unless the task explicitly requires it.

Default allowed server-side write scope:

- `tool definitions`
- `runtime tool helpers`
- `tool registration`
- `tests` directly related to the new capability or convergence change

Do not change these skeleton-level parts by default:

- `transport framing`
- `request/response envelope` structure
- `connection lifecycle`
- `protocol versioning`

If a `tool` returns a `task handle`, make sure the `runtime helper` behavior and response expectations still match.

## Unreal Plugin-Side Rules

When editing the Unreal plugin:

- The `command` string must match what `McpServer` expects.
- Request parsing, validation, and response shape must stay consistent with the MCP server contract.
- Prefer the minimum necessary `command` implementation instead of speculative abstraction.
- Follow the existing `SUnrealMcp` `module` / `command` / `helper` style.
- Keep errors structured and actionable.
- When converging `tools`, preserve or intentionally bridge any remaining plugin-side dependency until the migration is complete.

Default allowed plugin-side write scope:

- `command registration`
- `command implementation`
- `helper utilities` directly supporting the `command`
- module dependency updates needed for the new `command`

Do not casually refactor these skeleton-level parts:

- `server skeleton`
- `task registry model`
- core `transport` behavior

If a `command` edits assets or Blueprint graphs, make sure it actually performs the behavior promised by the `tool` description instead of only returning a success-like response.

## Validation Workflow

After implementation, validate first and only then return to the original task.

### McpServer Validation

At minimum, usually run:

```powershell
cd <McpServer directory>
npm run build
```

If relevant tests exist in the touched area, run them too.

### Unreal Plugin Validation

Build the plugin or owning project using the Unreal build workflow for the current workspace.

At minimum, confirm:

- the code compiles
- module dependencies are correct
- the new or changed `command` is registered

### Editor Rebuild Coordination Rule

When the new or changed `tool` / `command` requires rebuilding the Unreal Editor target, do not assume the editor can be interrupted safely.

If the editor may currently be in active use, first ask the user whether they want the agent to:

1. automatically close the editor
2. trigger the required build
3. reopen the editor
4. continue the original task after the editor is back

Use this coordinated rebuild path when:

- the active editor process must be closed for the build to succeed
- the updated plugin or module will not be picked up reliably without restart
- continuing the task depends on the rebuilt editor session

If the user approves this flow, treat it as one continuous task rather than a stopping point. After reopening the editor, continue validation, refresh any affected runtime assumptions, and resume the original task.

If the user does not approve automatic editor interruption, stop at the safe handoff point, explain what rebuild or restart is still required, and do not force-close the editor.

### Runtime Smoke Validation

Before declaring the extension successful, at minimum confirm:

- the MCP server can start
- the agent can see the new, changed, deprecated, or merged `tool`
- a small `smoke call` succeeds
- if a `task` flow is involved, the `status` path still works

### Tool Refresh Rule

After changing `tool` definitions, do not assume the current agent will automatically understand the new `tool surface`.

Treat tool refresh as a separate step:

1. rebuild the `server`
2. if the MCP server is already running, restart it
3. ensure the host reconnects to the `server`
4. run a fresh `tool discovery`
5. before resuming the original task, explicitly tell the agent which `tool` was added, removed, merged, deprecated, or changed

This is especially important when:

- a new `tool` was added
- the `tool` kept the same name but changed parameters
- the parameters stayed the same but the behavior changed
- a `tool` was merged, deprecated, or deleted

The current agent may still be carrying stale `tool` memory. Without an explicit refresh, it may continue acting on outdated assumptions.

For hosts that cache or delay `tool` discovery, use this practical refresh sequence:

1. rebuild the `server`
2. restart the MCP server process
3. reconnect the host
4. start a fresh `tool discovery`
5. continue the original task with a short `reorientation note`

If the current session cannot reliably refresh the `tool` view, treat that as a `workflow limitation` and use a fresh session or explicit reconnect instead of assuming the system will refresh implicitly.

### Tool Change Strategy

Prefer incremental, low-surprise changes.

Prefer to:

- add a clearly named new `tool` rather than silently changing the meaning of an old one
- extend `schema` in backward-compatible ways
- avoid reusing an old `tool` name for materially different behavior
- converge by `merge` or `deprecate` before `delete` when future callers may still exist

If an existing `tool` must change in a breaking way, treat it as a workflow that requires explicit refresh and reorientation rather than a silent replacement.

## Return To The Original Task

This skill is not complete just because the new `tool` compiles.

After validation, immediately return to the original user task and continue with the new capability until the original task is actually complete.

Do not stop at states like:

- "`tool` has been added"
- "the plugin compiles now"
- "you can manually call this `tool` now"

This workflow is successful only when the original task is complete or a real external `blocker` remains.

## Failure Handling

If the failure is still under the agent's control, do not stop at the first failed attempt.

Default behavior:

- if `McpServer` `build` fails, fix the server-side issue and rebuild
- if Unreal plugin compilation fails, fix the plugin-side issue and rebuild
- if both sides compile but the `smoke test` fails, inspect the contract mismatch and iterate
- if the new capability works but the original task still fails, continue the original task
- if convergence exposes overlapping or stale `tools`, keep iterating until the correct `keep`, `merge`, `deprecate`, or `delete` outcome is clear

Stop and escalate only when:

- the `blocker` is outside the workspace
- the required change has become a protocol or architecture redesign
- the `source of truth` is ambiguous and risky to guess
- the current host cannot provide the `refresh` / `reconnect` capability the workflow depends on
- rebuilding requires interrupting an editor session that may currently be in use and the user has not approved automatic close and reopen

## Escalation Rules

Pause and align with the user first when:

- a protocol-level redesign is required
- multiple widely used `tools` must be renamed or deleted
- the correct `source of truth` is unclear
- the change would create maintenance consequences far beyond the current task
- the required editor rebuild may interrupt the user's active Unreal Editor session

But do not stop just because a small `tool` / `command` pair needs to be added, or because a clearly stale temporary `tool` should be deprecated. The normal path is to make the smallest correct change, perform the convergence review, and keep going.

## Execution Checklist

Whenever a capability gap or convergence trigger appears, follow this checklist:

1. Restate the original task and the missing capability or overlapping `tool`.
2. Check whether an existing `tool` already solves it.
3. Decide `keep`, `reuse`, `extend`, `merge`, `deprecate`, `delete`, or `add`.
4. Classify each affected `tool` as `core`, `reusable`, or `temporary`.
5. Make the minimum necessary changes in `McpServer`.
6. Make the matching `command` changes in the Unreal plugin.
7. Rebuild `McpServer`.
8. If rebuilding the Unreal plugin or project may interrupt an active editor session, ask whether to automatically close the editor, build, reopen it, and continue.
9. Rebuild the Unreal plugin or project.
10. If the coordinated editor rebuild path was used, reopen the editor and restore the task flow before continuing.
11. Run a `smoke test` for the new or converged capability.
12. Refresh `tool discovery` and explicitly reorient the agent to the new `tool surface`.
13. Return to and finish the original task.
14. Run an end-of-task convergence pass for temporary or overlapping `tools`.
15. Persist or report the classification and convergence outcome.

## Completion Criteria

This workflow is complete only when all of the following are true:

- The original user task is complete, or only a real external `blocker` remains.
- The added, changed, merged, deprecated, or deleted MCP capability is implemented consistently on both sides.
- The `tool` contract and plugin `command` behavior are consistent.
- The refreshed session can see the intended `tool surface`.
- All temporary or overlapping `tools` touched by the task have been reviewed for convergence.
- Any needed classification or convergence conclusion has been persisted or explicitly reported.
- The user receives a concise summary covering:
  - what capability was missing or overlapping
  - what was added, changed, merged, deprecated, or deleted
  - how it was validated
  - whether any temporary items or follow-up cleanup remain
