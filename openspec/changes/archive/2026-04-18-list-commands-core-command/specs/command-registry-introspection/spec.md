## ADDED Requirements

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

- **WHEN** CLI 发送 `{command:"check_command_exists", params:{}}`
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

- **WHEN** CLI 发送 `{command:"diff_commands", params:{}}`
- **THEN** Unreal 侧返回 `ok:false`，`error.code = "INVALID_PARAMS"`，`error.message` 包含字面子串 `cli_names`

#### Scenario: cli_names 类型错误

- **WHEN** CLI 发送 `{command:"diff_commands", params:{cli_names:"ping"}}`（字符串而非数组）
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

### Requirement: CLI `system check_command_exists` 透传

CLI `system` family SHALL 注册命令 `check_command_exists`，`lifecycle: "core"`、`unrealCommand: "check_command_exists"`、参数列表为 `[{ name: "name", type: "string", required: true, description: ... }]`，不定义 `mapParams` 回调，因此用户/Agent 传入的 `name` SHALL 被原样放到 wire 的 `params.name` 上。

#### Scenario: Agent 调用 system check_command_exists

- **WHEN** Agent 执行 `sunrealmcp-cli system check_command_exists --name ping`
- **THEN** CLI 向 Unreal 发送 `{command:"check_command_exists", params:{name:"ping"}}`
- **AND** stdout 输出遵循 `slim-envelope` 默认规范
