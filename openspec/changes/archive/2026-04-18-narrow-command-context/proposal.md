## Why

目前 CLI 与 Unreal 两侧的 command handler 在执行期都能直接摸到内部 owner 对象：

- Unreal 侧 `FSUnrealMcpExecutionContext` 直接暴露 `CommandRegistry`，命令理论上能拿到完整注册表类型，而不只是它真正需要的查询能力。
- `reload_command_registry` 更进一步，绕过 context，直接通过 `FModuleManager` 抓 `FSUnrealMcpModule` 再调用 `RebuildCommandRegistry()`。
- CLI 侧 `CliCommandExecutionContext` 直接暴露 `registry`，虽然当前只有 `system diff_commands` 在用，但所有 `execute:` handler 都能拿到完整注册表。

这会让 command 层和内部 ownership 结构强耦合，也把不必要的可变面暴露到了执行期。当前真实需求其实很小：命令只需要少量显式命名的方法，而不是 `Registry` / `Module` / `Subsystem` 本体。

## What Changes

- 把两侧 execution context 都改成“方法边界”，不再通过 public 字段把内部 owner 直接暴露给 command。
- Unreal 侧：
  - `FSUnrealMcpExecutionContext` 不再公开 `CommandRegistry` 字段。
  - 改为提供当前命令所需的最小方法面：命令查询、任务查询/控制、`reload_command_registry` 需要的显式 reload 入口。
  - `check_command_exists`、`diff_commands`、`get_task_status`、`cancel_task`、`reload_command_registry`、`start_mock_task` 改用这些 context 方法。
- CLI 侧：
  - `CliCommandExecutionContext` 不再公开 `registry` 字段。
  - 改为提供 `listRegisteredUnrealCommands()` 这类直接服务当前需求的方法。
  - `system diff_commands` 通过该方法收集本地静态注册命令名，而不是自己遍历 registry。
- 本 change 不抽 `FSUnrealMcpModule -> Subsystem`；先把 command 和内部 owner 的边界收紧，后续如果还要做 runtime owner 重组，再单独开 change。

## Capabilities

### New Capabilities
无。

### Modified Capabilities
- `command-registry-introspection`: 不再要求 CLI / Unreal 的 execution context 把 registry 直接暴露给 command，而是改成最小方法面；`system diff_commands` 通过 context 方法拿本地清单。

## Impact

- Unreal 代码：
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Public/Mcp/ISUnrealMcpCommand.h`
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Mcp/SUnrealMcpServer.cpp`
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/CheckCommandExistsCommand.cpp`
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/DiffCommandsCommand.cpp`
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/GetTaskStatusCommand.cpp`
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/CancelTaskCommand.cpp`
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/ReloadCommandRegistryCommand.cpp`
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Temporary/System/StartMockTaskCommand.cpp`
- CLI 代码：
  - `Skill/sunrealmcp-unreal-editor-workflow/cli/src/types.ts`
  - `Skill/sunrealmcp-unreal-editor-workflow/cli/src/index.ts`
  - `Skill/sunrealmcp-unreal-editor-workflow/cli/src/commands/core/system_commands.ts`
- Specs：
  - `openspec/specs/command-registry-introspection/spec.md` 的 context 暴露方式会被调整，但 wire 命令名、参数名、响应体不变。
