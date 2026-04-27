# command-runtime-contracts Specification

## Purpose
定义 CLI 与 Unreal command runtime 的长期契约：wire 字段命名、CLI 默认输出、help JSON、timeout 默认值、core command 响应与超大响应错误。后续 Agent 在新增或修改 CLI/Unreal command 时，应优先遵守本 spec，而不是翻阅历史 change。

## Requirements

### Requirement: Wire 参数字段使用 snake_case

请求 `params` 对象和响应 `data` 对象内部的字段名 SHALL 使用 `snake_case`。该规则适用于 CLI 参数注册、`mapParams` 结果、Unreal handler 读取的 `Request.Params` 字段，以及 handler 拼装的 `data` key。

Envelope 字段不受该规则约束，包括 `protocolVersion`、`requestId`、`ok`、`data`、`error`、`error.code`、`error.message`、`error.details`，以及 CLI `--verbose` 下的 `target`、`cli`、`unreal`、`raw`。长任务 `data.payload` 内部字段属于 task 私有命名空间，也不强制 snake_case。

#### Scenario: 新 command 复用同名 snake_case 参数

- **WHEN** 新增 CLI wrapper 与 Unreal handler
- **THEN** CLI `CliParameterDefinition.name` 使用 snake_case
- **AND** Unreal handler 用完全相同的 snake_case key 读取参数
- **AND** 不为单纯大小写转换新增 `mapParams`

### Requirement: Task command 使用 `task_id`

`get_task_status` 与 `cancel_task` SHALL 从 `params.task_id` 读取任务标识。缺失、空字符串或类型错误时 SHALL 返回 `ok:false`，`error.code == "INVALID_PARAMS"`，且 `error.message` 包含 `task_id`。

成功响应 `data` SHALL 包含 `task_id` 与 `status`；当 task payload 或 error 存在时，可额外包含 `payload` 或 `error`。`status` 取值 SHALL 属于 `{pending, running, succeeded, failed, cancelled}`。

#### Scenario: 查询 task 状态

- **WHEN** CLI 发送 `{command:"get_task_status", params:{task_id:"task-1"}}`
- **THEN** Unreal 返回 `ok:true`
- **AND** `data.task_id == "task-1"`
- **AND** `data.status` 属于 `{pending, running, succeeded, failed, cancelled}`

### Requirement: CLI 默认输出使用精简 envelope

CLI 默认 stdout JSON SHALL 只输出成功时的 `{ok,data}` 或失败时的 `{ok,error}`。默认模式 MUST NOT 输出 `target`、`cli`、`unreal`、`raw`。

当传入 `--verbose` 时，CLI SHALL 在精简 envelope 基础上追加 `target`、`cli`、`unreal`、`raw` 诊断字段。

#### Scenario: 默认成功输出

- **WHEN** Agent 执行任意 command 且 Unreal 返回 `ok:true`
- **THEN** CLI stdout 顶层仅包含 `ok` 和 `data`

#### Scenario: verbose 输出

- **WHEN** Agent 执行 command 并传入 `--verbose`
- **THEN** CLI stdout 包含 `target`、`cli`、`unreal`、`raw`

### Requirement: CLI JSON help 保持精简

CLI JSON help SHALL 只返回当前 help 层级需要的信息，不重复静态全局选项。

- `root-help` SHALL 仅包含 `kind` 与 `families`。
- `family-help` SHALL 包含 `kind`、`family`、`description`、`sections`，且 `sections` 只包含非空 lifecycle。
- `family-help` 下每条 command SHALL 仅包含 `cliCommand` 与 `description`。
- `command-help` SHALL 包含 `kind`、`family`、`command`、`lifecycle`、`description`、`unrealCommand`、`parameters`、`examples`。

#### Scenario: command-help JSON 不含全局选项

- **WHEN** Agent 执行 `sunrealmcp-cli help <family> <command> --json`
- **THEN** 输出中不存在 `globalOptions` 或 `helpOptions`

### Requirement: CLI 默认 timeout 为 30000

当调用方未显式传入 `--timeout_ms` 时，CLI SHALL 使用 `30000` 作为连接与响应等待的默认 timeout，并在 `CliGlobalOptions.timeoutMs` 与 `CliTarget.timeoutMs` 中体现。显式传入 `--timeout_ms` 时 SHALL 覆盖默认值。

Skill 文档 SHOULD 避免硬编码 timeout 默认值；如需说明，SHOULD 指向 CLI help 或运行时行为作为真相源。

#### Scenario: 未传 timeout 时使用默认值

- **WHEN** 调用方执行任意 CLI command 且未传入 `--timeout_ms`
- **THEN** 运行时 timeout 为 `30000`

### Requirement: Core command 响应保持低噪声

`ping` 成功响应的 `data` SHALL 仅包含 `protocol_version`，其值等于当前协议版本。`reload_command_registry` 成功响应的 `data` SHALL 为空对象 `{}`。

#### Scenario: ping 响应只含协议版本

- **WHEN** CLI 发送 `{command:"ping", params:{}}`
- **THEN** Unreal 返回 `ok:true`
- **AND** `data` key 集合恰为 `{protocol_version}`

### Requirement: 超大响应返回结构化错误

当命令响应序列化后的 UTF-8 字节数超过 `MaxPendingSendBytesPerConnection` 时，server SHALL 发送替代错误响应，`error.code == "RESPONSE_TOO_LARGE"`，而不是静默断开连接。

`error.details` SHALL 至少包含 `response_bytes` 与 `limit_bytes`。`error.message` SHALL 包含触发超限的 command 名。

#### Scenario: 响应超过限制

- **WHEN** command 响应超过发送缓冲区限制
- **THEN** CLI 收到 `ok:false`
- **AND** `error.code == "RESPONSE_TOO_LARGE"`
- **AND** `error.details.response_bytes` 与 `error.details.limit_bytes` 为整数
