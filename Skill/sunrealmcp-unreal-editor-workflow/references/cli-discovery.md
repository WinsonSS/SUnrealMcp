# CLI Discovery Commands

Use this file only when the agent needs to discover target Unreal projects or resolve the correct host and port.

CLI help for this family should be available through:

- `sunrealmcp-cli help discovery`
- `sunrealmcp-cli discovery --help`

## Core

### `discovery list-targets`

Purpose:
discover Unreal projects in or under the working tree that contain `SUnrealMcp` and resolve their target endpoints.

Parameters:

- `--root <path>`: search root; defaults to current working directory
- `--depth <number>`: optional scan depth limit

### `discovery inspect-target`

Purpose:
inspect one target project, show plugin presence, config sources, host, port, and whether the socket responds.

Parameters:

- `--project <path>`

## Rules

- prefer `--project` over `--port`
- treat the Unreal project as the primary target identity
- read `[/Script/SUnrealMcp.SUnrealMcpSettings]`
- resolve `BindAddress`, `Port`, and `bAutoStartServer`
- do not guess a different port silently when config and runtime disagree
