# core-command-wire-protocol Specification

## Purpose
TBD - created by archiving change fix-task-id-param-naming. Update Purpose after archive.
## Requirements
### Requirement: Wire 参数字段命名约定

请求 `params` 对象和响应 `data` 对象内部的字段名 SHALL 采用 `snake_case`（仅包含小写 ASCII 字母、数字和下划线；不以数字开头；不出现连续下划线）。该约定 SHALL 同时适用于 CLI 侧的参数注册（`CliParameterDefinition.name`）以及 Unreal 侧命令 handler 读取 `Request.Params` 字段或拼装 `data` 字段时使用的 key。

**例外 1（envelope）**：由 `ProtocolVersion = 1` 定义的 envelope 字段——具体为 `protocolVersion`、`requestId`、`ok`、`data`、`error`、`error.code`、`error.message`、`error.details`——不在本规则范围内，继续保持现有的 camelCase 形式。**补充**：CLI 侧输出信封的顶层字段 `target`、`cli`、`unreal`、`raw` 仅在 `--verbose` 模式下出现，同样属于 envelope 层级，不受 snake_case 约束。

**例外 2（task payload）**：长耗时任务通过 `ISUnrealMcpTask::BuildPayload` 返回的 JSON 内容（在响应中位于 `data.payload`）属于具体 task 实现的私有命名空间，不受本规则强制约束。Framework 拼装的 `data.payload` 这一 key 名本身仍然必须是 snake_case，但 payload 对象**内部**的字段命名由 task 作者自决。

#### Scenario: 新 core 命令按 snake_case 注册参数

- **WHEN** 新增一个 Unreal 侧 core 命令并在 `Skill/sunrealmcp-unreal-editor-workflow/cli/src/commands/core/` 下编写对应的 CLI 注册
- **THEN** CLI 通过 `CliParameterDefinition.name` 注册的每个参数名都使用 snake_case 书写
- **AND** Unreal 侧 handler 从 `Request.Params` 读取该字段时使用完全一致的字面字符串，不做任何大小写转换
- **AND** CLI 侧不因"仅为了改大小写"而引入 `mapParams` 回调

#### Scenario: 发现非 snake_case 字段时拒绝合入

- **WHEN** 审阅者在某个 core 命令里发现 `params` 字段使用了 camelCase、PascalCase 或其它非 snake_case 形式
- **THEN** 该字段被视为规范违规，合入前必须 rename 为 snake_case

### Requirement: `get_task_status` 读取 `task_id`

Unreal core 命令 `get_task_status` SHALL 从 `params.task_id`（snake_case）读取任务标识。当 `task_id` 缺失、为空字符串或非字符串类型时，命令 SHALL 返回结构化错误，`error.code` 为 `INVALID_PARAMS`，`error.message` 必须包含字面子串 `task_id`。成功响应的 `data` 对象 SHALL 使用 snake_case 字段名（`task_id`、`task_name`、`status`、`completed`、`payload`、`error`）；MUST NOT 出现 `taskId`、`taskName` 或其它任何 camelCase 形式的 framework 字段。

#### Scenario: 合法 task_id 返回任务状态

- **WHEN** CLI 发送 `{command:"get_task_status", params:{task_id:"task-1"}}` 且 `task-1` 已在任务注册表中
- **THEN** Unreal 侧返回 `ok:true`，`data.task_id == "task-1"`，`data` 中同时包含 `task_name`、`status`、`completed` 字段
- **AND** `data` 中不存在 `taskId`、`taskName` 或 `cancelRequested` 字段

#### Scenario: 缺失 task_id 时错误消息使用 snake_case 字段名

- **WHEN** CLI 发送 `{command:"get_task_status", params:{}}`
- **THEN** Unreal 侧返回 `ok:false`，`error.code = "INVALID_PARAMS"`，`error.message` 包含字面子串 `task_id`

#### Scenario: task_id 为空字符串时被拒绝

- **WHEN** CLI 发送 `{command:"get_task_status", params:{task_id:""}}`
- **THEN** Unreal 侧返回 `ok:false`，`error.code = "INVALID_PARAMS"`，`error.message` 包含字面子串 `task_id`

### Requirement: `cancel_task` 读取 `task_id`

Unreal core 命令 `cancel_task` SHALL 从 `params.task_id`（snake_case）读取任务标识。当 `task_id` 缺失、为空字符串或非字符串类型时，命令 SHALL 返回结构化错误，`error.code` 为 `INVALID_PARAMS`，`error.message` 必须包含字面子串 `task_id`。成功响应的 `data` 对象 SHALL 使用 snake_case 字段名，包括取消语义元数据 `cancel_requested`；MUST NOT 出现 `cancelRequested`、`taskId`、`taskName` 或其它 camelCase 形式的 framework 字段。

#### Scenario: 合法 task_id 取消正在运行的任务

- **WHEN** CLI 发送 `{command:"cancel_task", params:{task_id:"task-2"}}` 且 `task-2` 处于 `pending` 或 `running` 状态
- **THEN** Unreal 侧调用该任务的 `Cancel()` 方法，并返回 `ok:true`，`data.task_id == "task-2"`，`data.cancel_requested == true`
- **AND** `data` 中不存在 `cancelRequested`、`taskId` 或 `taskName` 字段

#### Scenario: 缺失 task_id 时错误消息使用 snake_case 字段名

- **WHEN** CLI 发送 `{command:"cancel_task", params:{}}`
- **THEN** Unreal 侧返回 `ok:false`，`error.code = "INVALID_PARAMS"`，`error.message` 包含字面子串 `task_id`

### Requirement: CLI `system` task 命令透传 `task_id`

CLI 命令 `system get_task_status` 和 `system cancel_task` SHALL 在 `CliParameterDefinition` 中以 `task_id` 为名注册选项，SHALL NOT 定义会重命名该字段的 `mapParams` 回调，因此 SHALL 将用户/Agent 输入的 `task_id` 原样放到 wire 上。

#### Scenario: Agent 调用 system get_task_status

- **WHEN** Agent 执行 `sunrealmcp-cli system get_task_status --task_id task-1`
- **THEN** 通过 TCP 连接发送的请求 JSON 中 `params.task_id = "task-1"`，且 `params` 中不存在 `taskId` 字段

