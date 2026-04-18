## 1. 更新 OpenSpec 规格

- [x] 1.1 在 `openspec/changes/2026-04-18-narrow-command-context/specs/command-registry-introspection/spec.md` 删除“execution context 直接暴露 registry”的要求，改成“通过 context 方法暴露最小能力面”
- [x] 1.2 `openspec validate 2026-04-18-narrow-command-context` 通过

## 2. Unreal 侧收紧 `FSUnrealMcpExecutionContext`

- [x] 2.1 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Public/Mcp/ISUnrealMcpCommand.h`：把 `FSUnrealMcpExecutionContext` 从 public 字段 struct 调整为带 private 成员与 public 方法的类型
- [x] 2.2 为当前命令补齐最小方法面：`HasCommand`、`ListCommandNames`、`GetTaskStatusResponse`、`CancelTask`、`EnqueueTask`、`MakeAcceptedTaskResponse`、`ReloadCommandRegistry`
- [x] 2.3 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Mcp/SUnrealMcpServer.cpp`：构造 context 时注入必要引用 / 回调，而不是把 registry 直接作为 public 字段传给命令

## 3. Unreal 命令切换到 context 方法

- [x] 3.1 `CheckCommandExistsCommand.cpp` 与 `DiffCommandsCommand.cpp` 改为调用 `Context.HasCommand(...)` / `Context.ListCommandNames()`
- [x] 3.2 `GetTaskStatusCommand.cpp`、`CancelTaskCommand.cpp`、`StartMockTaskCommand.cpp` 改为调用 task 相关 context 方法
- [x] 3.3 `ReloadCommandRegistryCommand.cpp` 改为调用 `Context.ReloadCommandRegistry(...)`，不再直接 lookup `FSUnrealMcpModule`

## 4. CLI 侧移除 `registry` 暴露

- [x] 4.1 `Skill/sunrealmcp-unreal-editor-workflow/cli/src/types.ts`：删除 `CliCommandExecutionContext.registry`，改为 `listRegisteredUnrealCommands(): string[]`
- [x] 4.2 `Skill/sunrealmcp-unreal-editor-workflow/cli/src/index.ts`：在构造 execution context 时提供 `listRegisteredUnrealCommands` 实现
- [x] 4.3 `Skill/sunrealmcp-unreal-editor-workflow/cli/src/commands/core/system_commands.ts`：`diff_commands` 改用 `context.listRegisteredUnrealCommands()`

## 5. 自验证

- [x] 5.1 CLI `tsc` 构建通过，`dist/` 产物更新
- [x] 5.2 Unreal 插件编译通过
- [x] 5.3 冒烟以下命令保持原行为：
  - `sunrealmcp-cli system check_command_exists --name ping`
  - `sunrealmcp-cli system diff_commands`
  - `sunrealmcp-cli system get_task_status --task_id <id>`
  - `sunrealmcp-cli system cancel_task --task_id <id>`
  - `sunrealmcp-cli system reload_command_registry`
