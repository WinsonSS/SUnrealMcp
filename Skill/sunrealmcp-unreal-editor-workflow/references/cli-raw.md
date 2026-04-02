# CLI Raw Commands

Use this file only when a formal CLI wrapper does not exist yet and the agent must call a newly added Unreal command in the current session.

CLI help for this family should be available through:

- `sunrealmcp-cli help raw`
- `sunrealmcp-cli raw --help`

## Core

### `raw send`

Purpose:
send any Unreal command directly.

Parameters:

- `--command <name>`
- `--params-json <json>`

## Rules

- prefer a family-specific command when one exists
- use `raw send` immediately after adding a new Unreal command if the CLI wrapper is not ready yet
- treat `raw send` as a bridge, not as the final long-term abstraction
