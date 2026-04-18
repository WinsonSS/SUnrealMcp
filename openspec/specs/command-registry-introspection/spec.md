# command-registry-introspection Specification

## Purpose
定义 CLI 与 Unreal 两侧对命令注册表的最小内省通道：一条只查单个命令名是否存在的 `check_command_exists`，一条返回双向差集的 `diff_commands`。前者服务 Agent 扩展命令时的重名检查，后者服务维护者排查「CLI 注册文件漏改」「Live Coding 未 reload」等双侧漂移。两条命令 + `FSUnrealMcpCommandRegistry` 的只读访问器 + `FSUnrealMcpExecutionContext` / `CliCommandExecutionContext` 的注册表引用一起构成基础通道。
## Requirements
### Requirement: Unreal core 命令 `check_command_exists` 查单个命令名

Unreal core 命令 `check_command_exists` SHALL 从 `params.name`（snake_case，string）读取目标命令名。当 `name` 缺失、为空字符串或非字符串类型时，命令 SHALL 返回结构化错误，`error.code` 为 `INVALID_PARAMS`，`error.message` 必须包含字面子串 `name`。成功响应的 `data` 对象 SHALL 恰好包含一个字段 `exists`（boolean），值由 `FSUnrealMcpCommandRegistry::HasCommand(Name)` 决定。

#### Scenario: 名字命中已注册命令

- **WHEN** CLI 发送 `{command:"check_command_exists", params:{name:"ping"}}` 且 `ping` 已注册
- **THEN** Unreal 侧返回 `ok:true`，`data.exists == true`
- **AND** `data` 对象的键集合恰为 `{exists}`

#### Scenario: 名字未命中

- **WHEN** CLI 发送 `{command:"check_command_exists", params:{name:"does_not_exist_xyz"}}`
- **THEN** Unreal 侧返回 `ok:true`，`data.exists == false`

#### Scenario: 缺失 name 参数

- **WHEN** Unreal 收到 `{command:"check_command_exists", params:{}}`
- **THEN** Unreal 侧返回 `ok:false`，`error.code = "INVALID_PARAMS"`，`error.message` 包含字面子串 `name`

#### Scenario: name 为空字符串

- **WHEN** CLI 发送 `{command:"check_command_exists", params:{name:""}}`
- **THEN** Unreal 侧返回 `ok:false`，`error.code = "INVALID_PARAMS"`，`error.message` 包含字面子串 `name`

### Requirement: Unreal core 命令 `diff_commands` 返回双向差集

Unreal core 命令 `diff_commands` SHALL 从 `params.cli_names`（snake_case，字符串数组）读取 CLI 侧声称自己已注册的命令名。当 `cli_names` 缺失或不是数组类型时，命令 SHALL 返回结构化错误，`error.code` 为 `INVALID_PARAMS`，`error.message` 必须包含字面子串 `cli_names`。成功响应的 `data` 对象 SHALL 恰好包含两个字段：

- `only_in_unreal`：Unreal 已注册但 `cli_names` 未出现的命令名数组，按字典序升序排列。
- `only_in_cli`：`cli_names` 出现但 Unreal 未注册的命令名数组，按字典序升序排列。

两个数组在健康状态下都应为空 `[]`。

#### Scenario: 注册表与 CLI 一致

- **WHEN** CLI 发送 `{command:"diff_commands", params:{cli_names:["ping","list"]}}` 且 Unreal 恰好注册了 `ping`、`list` 两条命令
- **THEN** Unreal 侧返回 `ok:true`，`data.only_in_unreal == []`，`data.only_in_cli == []`
- **AND** `data` 对象的键集合恰为 `{only_in_unreal, only_in_cli}`

#### Scenario: 双向都存在差异

- **WHEN** CLI 发送 `{command:"diff_commands", params:{cli_names:["a","b","c"]}}` 且 Unreal 注册了 `a`、`c`、`d`
- **THEN** Unreal 侧返回 `ok:true`，`data.only_in_unreal == ["d"]`，`data.only_in_cli == ["b"]`

#### Scenario: cli_names 缺失

- **WHEN** Unreal 收到 `{command:"diff_commands", params:{}}`
- **THEN** Unreal 侧返回 `ok:false`，`error.code = "INVALID_PARAMS"`，`error.message` 包含字面子串 `cli_names`

#### Scenario: cli_names 类型错误

- **WHEN** Unreal 收到 `{command:"diff_commands", params:{cli_names:"ping"}}`（字符串而非数组）
- **THEN** Unreal 侧返回 `ok:false`，`error.code = "INVALID_PARAMS"`，`error.message` 包含字面子串 `cli_names`

### Requirement: `FSUnrealMcpCommandRegistry` 暴露只读访问器

`FSUnrealMcpCommandRegistry` SHALL 提供以下两个 public const 成员函数，供命令 handler 内省注册表使用：

- `TArray<FString> ListCommandNames() const`：返回当前所有成功注册命令的名字副本，按字典序升序排序，不暴露内部 `TMap` 或 `TSharedRef` 句柄。
- `bool HasCommand(const FString& Name) const`：当 `Name` 对应的命令已成功注册时返回 `true`，否则返回 `false`；比较大小写敏感。

#### Scenario: ListCommandNames 返回排序副本

- **WHEN** 代码依次成功注册 `zeta`、`alpha`、`mu`，随后调用 `ListCommandNames()`
- **THEN** 返回的 `TArray<FString>` 内容为 `{alpha, mu, zeta}`，长度 3

#### Scenario: HasCommand 命中已注册命令

- **WHEN** `ping` 已通过 `RegisterCommand` 成功注册
- **THEN** `HasCommand(TEXT("ping"))` 返回 `true`，`HasCommand(TEXT("Ping"))` 返回 `false`

#### Scenario: 注册失败不影响访问器

- **WHEN** 代码先成功注册 `ping`，再尝试重复注册另一个名字为 `ping` 的命令（该次返回 false 并写入 `RegistrationErrors`）
- **THEN** `ListCommandNames()` 内 `ping` 恰好出现一次，`HasCommand(TEXT("ping"))` 返回 `true`

### Requirement: CLI `system check_command_exists` 透传

CLI `system` family SHALL 注册命令 `check_command_exists`，`lifecycle: "core"`、`unrealCommand: "check_command_exists"`、参数列表为 `[{ name: "name", type: "string", required: true, description: ... }]`，不定义 `mapParams` 回调，因此用户/Agent 传入的 `name` SHALL 被原样放到 wire 的 `params.name` 上。

#### Scenario: Agent 调用 system check_command_exists

- **WHEN** Agent 执行 `sunrealmcp-cli system check_command_exists --name ping`
- **THEN** CLI 向 Unreal 发送 `{command:"check_command_exists", params:{name:"ping"}}`
- **AND** stdout 输出遵循 `slim-envelope` 默认规范

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

