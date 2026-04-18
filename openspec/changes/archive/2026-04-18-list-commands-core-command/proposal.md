## Why

Agent 日常使用 CLI 时以 CLI 的 help 面板为准，不需要反向查询 Unreal 实际注册了什么；但有两个场景需要从 Unreal 侧读信息：

- **Agent 扩展新命令时查重名**：Agent 按 SKILL.md 流程给项目新增一个 Unreal 命令时，要先确认目标名字在 Unreal 侧没被占用，避免实现后被 `RegisterCommand` 返回 false 吞进 `RegistrationErrors`。这个场景每次只查**一个**名字，返回全部清单纯属浪费。
- **人工 debug 双侧漂移**：CLI 的 help 是扫描 `Skill/.../cli/src/commands/**` 静态推导的，Unreal 的 `FSUnrealMcpCommandRegistry` 是运行时反射自注册的，两者在「CLI 注册文件漏改」「Live Coding 未 reload」这类情况下会不一致。人类需要的不是"Unreal 全量命令表"（一列展开看着就累），而是**差集**：哪些在 CLI 这边多、哪些在 Unreal 那边多。健康状态下差集是空，出问题时一眼看出哪条不对。

因此不做一个笼统的 `list_commands`，而是拆成两个目标各自最小的命令。

## What Changes

- 新增 Unreal core 命令 `check_command_exists`：`params: {name: string}`（snake_case），响应 `data: {exists: boolean}`，字段集恰好 `{exists}`。
- 新增 Unreal core 命令 `diff_commands`：`params: {cli_names: string[]}`（CLI 侧传入它自己已注册的 `unrealCommand` 名字数组），响应 `data: {only_in_unreal: string[], only_in_cli: string[]}`，两个数组各自按字典序排序，字段集恰好 `{only_in_unreal, only_in_cli}`。
- `FSUnrealMcpCommandRegistry` 增加只读访问器 `ListCommandNames() const -> TArray<FString>` 与 `HasCommand(const FString&) const -> bool`，供上述两个命令实现使用。
- `FSUnrealMcpExecutionContext` 增加 `CommandRegistry: TSharedRef<FSUnrealMcpCommandRegistry>` 字段；`SUnrealMcpServer` 构造 context 时同步补上。
- CLI 侧在 `system` family 下注册两条命令：
  - `system check_command_exists --name <n>`：透传 `name` 到 Unreal。
  - `system diff_commands`：**无用户参数**；CLI 在运行时自动从本地 `CommandRegistry` 收集所有 `unrealCommand` 值、去重排序后作为 `cli_names` 参数发送。Agent / 人直接 `sunrealmcp-cli system diff_commands` 就能拿到 diff。

## Capabilities

### New Capabilities
- `command-registry-introspection`: 定义两条 Unreal core 命令（`check_command_exists` / `diff_commands`）的请求/响应契约、CLI 侧的注册与 diff 自动打包行为，以及 `FSUnrealMcpCommandRegistry` 的两个只读访问器。

### Modified Capabilities
无。

## Impact

- Unreal 代码：
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Public/Mcp/SUnrealMcpCommandRegistry.h`（加两个访问器声明）
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Mcp/SUnrealMcpCommandRegistry.cpp`（加实现）
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Public/Mcp/ISUnrealMcpCommand.h`（`FSUnrealMcpExecutionContext` 加字段 + 前向声明）
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Mcp/SUnrealMcpServer.cpp`（构造 context 时传 `CommandRegistry.ToSharedRef()`）
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/CheckCommandExistsCommand.cpp`（新文件）
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/DiffCommandsCommand.cpp`（新文件）
- CLI 代码：
  - `Skill/sunrealmcp-unreal-editor-workflow/cli/src/commands/core/system_commands.ts`（加两条注册）
  - `diff_commands` 需要一条能在命令 handler 运行时读到 CLI 内部 `CommandRegistry` 全部注册项的通道；当前 `CliCommandRegistry` 已有 `getAll()` 方法，可直接复用。
- Wire/依赖：不涉及协议版本变化，`ProtocolVersion` 保持 `1`。
