# CLI System Commands

Use this file only for connectivity checks, registry reloads, and task control.

CLI help for this family should be available through:

- `sunrealmcp-cli help system`
- `sunrealmcp-cli system --help`

## Core

### `system ping`

Purpose:
ping the Unreal-side server for the resolved target.

Parameters:

- global target options only

### `system reload-command-registry`

Purpose:
reload the Unreal command registry after Live Coding or rebuild.

Parameters:

- global target options only

### `system get-task-status`

Purpose:
query a task started by a long-running Unreal command.

Parameters:

- `--task-id <value>`

### `system cancel-task`

Purpose:
cancel a running Unreal task.

Parameters:

- `--task-id <value>`
