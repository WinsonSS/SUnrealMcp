---
name: sunrealmcp-self-extension
description: Extend and converge the SUnrealMcp tool surface for Unreal automation tasks that depend on SUnrealMcp and cannot be completed with existing MCP tools alone. Use when an agent must confirm a real capability gap or overlapping tool surface, then reuse, extend, merge, deprecate, delete, or add tools and matching commands, validate the stack, refresh tool discovery, and continue the original task instead of stopping at analysis.
---

# SUnrealMcp Self-Extension And Convergence Workflow

## Purpose

When an agent is solving a task through `SUnrealMcp` and discovers that the required MCP capability does not exist yet, it should not stop. It should close the loop end-to-end, then return to the original task and finish it.

The workflow has two equal responsibilities:

1. Extend the stack when a real capability gap blocks the task.
2. Converge the `tool` surface when duplicated, over-narrow, or temporary `tools` should be merged, deprecated, or removed.

The goal is not to accumulate `tools` endlessly. The goal is to keep the `tool` surface small, composable, maintainable, and accurate while still allowing the agent to unblock itself.

## Core Intent

Follow this loop:

1. Try to solve the user's task with existing `tools`.
2. Identify whether a real capability gap exists.
3. Decide whether to reuse, extend, merge, deprecate, delete, or add a `tool`.
4. Make the minimum necessary changes in:
   - `McpServer`
   - the active in-project plugin copy at `Plugins/SUnrealMcp`
5. Rebuild and validate.
6. Refresh the `tool` view.
7. Return to the original task and finish it.
8. If cleanup triggers apply, converge the `tool` surface before closing the task.

## When To Use

Use this skill only when all of the following are true:

- The current task depends on `SUnrealMcp` for Unreal automation.
- The agent has already checked whether existing `tools` can solve the problem.
- The gap is real and is not just misuse of an existing `tool`.
- The expectation is that the agent should continue and finish the task, not just hand the user a manual implementation plan.

Typical examples:

- A Blueprint workflow needs a node operation that is not currently exposed.
- `McpServer` has the `tool` concept, but the plugin lacks the matching `command`.
- The plugin has the capability, but `McpServer` does not expose it as a `tool`.
- A long-running workflow needs a task-aware `tool`, but no such capability exists yet.
- Several overlapping `tools` now point at the same capability and should be merged or deprecated.
- A temporary `tool` added to unblock a task should now be reviewed for cleanup.

## When Not To Use

Do not use this skill when:

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

### Source Of Truth Rule

If both an external repository copy and a local active plugin copy exist, do not edit both by default.

Use this default order:

1. Edit the active in-project plugin copy that actually participates in compilation.
2. Leave any external repository copy or mirror plugin unchanged unless the user explicitly asks to update it too.
3. Sync both only if the user explicitly wants both updated in the same task.

In other words:

- The active in-project plugin is the default implementation target.
- The external repository plugin is a manual sync target.

## Core Workflow

### 1. Try Existing Capabilities First

Before adding anything, do these checks:

- Enumerate the MCP `tools` relevant to the current task.
- Check whether the task can be solved by combining existing `tools`.
- Check whether the plugin already has a similar capability under a different command name or without exposure.
- Check whether the problem can be solved by extending an existing `tool` with a new parameter or `mode` instead of adding a new `tool`.

Do not add a new `tool` just because it seems convenient.

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

### Tool Classes

`core`

- broadly useful across many tasks
- foundational to other workflows
- not a likely deletion candidate

`reusable`

- useful beyond the current task
- reasonably general
- worth keeping if naming and `schema` are clean

`temporary`

- mainly added to unblock the current task
- narrow, project-shaped, or likely replaceable
- must be reviewed before the task closes or immediately after the original task is complete

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

### Tool Inventory Rule

`Tool` classification and convergence decisions must not live only in short-term reasoning.

When a task adds, merges, deprecates, or deletes a `tool`, record the decision in a lightweight `tool inventory` note if the workspace already has one for `SUnrealMcp`. Prefer a file named `tool-inventory.md` near the persisted `tool` definitions or in the `SUnrealMcp` root. If no such file exists, do one of the following:

- add a minimal inventory file if the current task is already changing the repository that should persist `tool` definitions
- otherwise include the classification and convergence recommendation explicitly in the final summary

At minimum, persist:

- `tool` name
- classification: `core`, `reusable`, or `temporary`
- action: `keep`, `merge`, `deprecate`, or `delete`
- short reason

Preferred minimal entry format:

```md
- tool: <name>
  classification: <core|reusable|temporary>
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

If the answers suggest low reuse, high overlap, and high maintenance cost, prefer `merge`, `deprecate`, or `delete` over `keep`.

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
- review overlapping `tools` in the same family
- decide whether each one should `keep`, `merge`, `deprecate`, or `delete`
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

## Escalation Rules

Pause and align with the user first when:

- a protocol-level redesign is required
- multiple widely used `tools` must be renamed or deleted
- the correct `source of truth` is unclear
- the change would create maintenance consequences far beyond the current task

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
8. Rebuild the Unreal plugin or project.
9. Run a `smoke test` for the new or converged capability.
10. Refresh `tool discovery` and explicitly reorient the agent to the new `tool surface`.
11. Return to and finish the original task.
12. Run an end-of-task convergence pass for temporary or overlapping `tools`.
13. Persist or report the classification and convergence outcome.

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
