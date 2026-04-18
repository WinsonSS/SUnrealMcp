## REMOVED Requirements

### Requirement: `FSUnrealMcpExecutionContext` 携带命令注册表引用

`FSUnrealMcpExecutionContext` SHALL 包含字段 `CommandRegistry: TSharedRef<FSUnrealMcpCommandRegistry>`，使得任意命令 handler 在 `Execute` 时能够通过 `Context.CommandRegistry->ListCommandNames()` 或 `Context.CommandRegistry->HasCommand(...)` 访问当前注册表内容。`SUnrealMcpServer` 在分发请求前 SHALL 用当前服务端持有的 `CommandRegistry` 构造该字段。

#### Scenario: 服务端构造 context 时提供两套 registry

- **WHEN** `SUnrealMcpServer` 接收到一个合法请求并调用 `CommandRegistry->Execute(Request, Context)`
- **THEN** 传入的 `Context` 同时包含有效的 `TaskRegistry` 与 `CommandRegistry` 两个 `TSharedRef`

### Requirement: CLI `CliCommandExecutionContext` 暴露本地注册表

`CliCommandExecutionContext` SHALL 包含字段 `registry: CliCommandRegistry`，指向当前进程内构造的 `CommandRegistry` 实例，使 `execute:` 风格的命令 handler 能在运行时读到所有已注册命令。`CliCommandRegistry` 接口 SHALL 新增 `getAll(): CliCommandDefinition[]` 方法，返回已注册命令定义数组。`index.ts` 的 `executeCommand` 在构造 `CliCommandExecutionContext` 时 SHALL 传入 `registry`。

#### Scenario: execute handler 读到本地注册表

- **WHEN** 一个命令以 `execute:` 方式注册，其 handler 在运行时访问 `context.registry.getAll()`
- **THEN** 返回的数组包含当前 `CommandRegistry` 中所有已 `registerCommand` 过的 `CliCommandDefinition`

### Requirement: CLI `system diff_commands` 自动采集本地清单

CLI `system` family SHALL 注册命令 `diff_commands`，`lifecycle: "core"`、`unrealCommand: "diff_commands"`、`parameters: []`，且 SHALL 提供一个自定义 `execute:` handler。该 handler 在运行时 SHALL：

1. 从 `context.registry.getAll()` 读取所有已注册命令定义；
2. 过滤掉 `family === "raw"` 的条目（其 `unrealCommand` 非静态）；
3. 对剩余条目读取 `unrealCommand` 字段（若缺省则跳过该条目）；
4. 对得到的字符串集合去重后按字典序升序排序，作为 `cli_names` 参数；
5. 向 Unreal 发送 `{command:"diff_commands", params:{cli_names:<array>}}`；
6. 遵循 `slim-envelope` 默认规范输出响应。

#### Scenario: Agent 调用 system diff_commands

- **WHEN** Agent 执行 `sunrealmcp-cli system diff_commands`
- **THEN** CLI 向 Unreal 发送的请求 `params.cli_names` 是字符串数组，内容为 CLI 侧所有非 `raw` family 命令的 `unrealCommand` 值去重排序后的结果
- **AND** stdout 输出顶层只有 `ok` 和 `data`，`data` 键集为 `{only_in_unreal, only_in_cli}`

## ADDED Requirements

### Requirement: `FSUnrealMcpExecutionContext` 通过方法暴露命令查询能力

`FSUnrealMcpExecutionContext` SHALL 通过 public 方法而不是 public `CommandRegistry` 字段向 command 暴露当前请求可见的命令查询能力。至少 SHALL 提供：

- `bool HasCommand(const FString& Name) const`
- `TArray<FString> ListCommandNames() const`

这些方法的结果 SHALL 基于服务端当前用于分发该请求的注册表视图计算；command 实现 MUST NOT 通过 execution context 直接拿到 `FSUnrealMcpCommandRegistry` 本体。

#### Scenario: `check_command_exists` 通过 context 查询单个命令

- **WHEN** `check_command_exists` 在 `Execute` 中需要判断 `ping` 是否已注册
- **THEN** 它通过 `Context.HasCommand(TEXT("ping"))` 完成查询
- **AND** command 不需要也拿不到 `Context.CommandRegistry`

#### Scenario: `diff_commands` 通过 context 获取命令名列表

- **WHEN** `diff_commands` 在 `Execute` 中需要当前 Unreal 已注册命令名集合
- **THEN** 它通过 `Context.ListCommandNames()` 获取排序后的字符串数组
- **AND** command 不需要也拿不到 `Context.CommandRegistry`

### Requirement: CLI `CliCommandExecutionContext` 通过方法暴露本地 Unreal 命令清单

`CliCommandExecutionContext` SHALL 通过 `listRegisteredUnrealCommands(): string[]` 方法向 `execute:` handler 暴露当前 CLI 静态注册的 Unreal 命令名清单，而不是公开 `registry` 字段。该方法 SHALL：

- 从进程内实际构造完成的命令注册表推导结果；
- 跳过 `family === "raw"` 的命令；
- 跳过未定义静态 `unrealCommand` 的条目；
- 对结果去重并按字典序升序排序。

`execute:` handler MUST NOT 通过 `CliCommandExecutionContext` 直接拿到 `CliCommandRegistry` 本体。

#### Scenario: `diff_commands` handler 获取本地命令名

- **WHEN** `system diff_commands` 的 `execute:` handler 调用 `context.listRegisteredUnrealCommands()`
- **THEN** 返回值是字符串数组，内容为当前 CLI 侧所有非 `raw` family 命令的静态 `unrealCommand` 值去重排序后的结果

### Requirement: CLI `system diff_commands` 通过 context 方法自动采集本地清单

CLI `system` family SHALL 注册命令 `diff_commands`，`lifecycle: "core"`、`unrealCommand: "diff_commands"`、`parameters: []`，且 SHALL 提供一个自定义 `execute:` handler。该 handler 在运行时 SHALL：

1. 调用 `context.listRegisteredUnrealCommands()` 获取本地命令名数组；
2. 将返回值原样作为 `cli_names` 参数；
3. 向 Unreal 发送 `{command:"diff_commands", params:{cli_names:<array>}}`；
4. 遵循 `slim-envelope` 默认规范输出响应。

#### Scenario: Agent 调用 system diff_commands

- **WHEN** Agent 执行 `sunrealmcp-cli system diff_commands`
- **THEN** CLI 向 Unreal 发送的请求 `params.cli_names` 等于 `context.listRegisteredUnrealCommands()` 的返回值
- **AND** stdout 输出顶层只有 `ok` 和 `data`，`data` 键集为 `{only_in_unreal, only_in_cli}`
