## MODIFIED Requirements

### Requirement: `get_task_status` 读取 `task_id`

Unreal core 命令 `get_task_status` SHALL 从 `params.task_id`（snake_case）读取任务标识。当 `task_id` 缺失、为空字符串或非字符串类型时，命令 SHALL 返回结构化错误，`error.code` 为 `INVALID_PARAMS`，`error.message` 必须包含字面子串 `task_id`。

成功响应的 `data` 对象 SHALL 恰好包含以下字段：

- `task_id`（string）：请求中查询的任务标识。
- `status`（string）：任务当前状态，取值属于枚举 `{pending, running, succeeded, failed, cancelled}`。`succeeded` / `failed` / `cancelled` 三态等价于「任务已完成」；消费方以 `status ∈ {succeeded, failed, cancelled}` 作为完成判据。
- `payload`（JSON 对象，可选）：当且仅当 task 实现的 `BuildPayload` 返回有效对象时出现。该对象内部字段由 task 作者自决（承接 Requirement 1 的例外条款）。
- `error`（JSON 对象，可选）：当且仅当 task 进入失败终态且记录了结构化错误时出现，字段为 `code`、`message`、可选 `details`。

#### Scenario: 合法 task_id 返回任务状态

- **WHEN** CLI 发送 `{command:"get_task_status", params:{task_id:"task-1"}}` 且 `task-1` 已在任务注册表中
- **THEN** Unreal 侧返回 `ok:true`，`data.task_id == "task-1"`，`data.status` 属于 `{pending, running, succeeded, failed, cancelled}`
- **AND** `data` 的键集合是 `{task_id, status}`，或再加上 `payload` / `error` 中实际有值的那一项

#### Scenario: 已完成任务通过 status 表达终态

- **WHEN** CLI 发送 `{command:"get_task_status", params:{task_id:"task-1"}}` 且 `task-1` 已进入 `succeeded` 状态
- **THEN** Unreal 侧返回 `ok:true`，`data.status == "succeeded"`

#### Scenario: 缺失 task_id 时错误消息使用 snake_case 字段名

- **WHEN** CLI 发送 `{command:"get_task_status", params:{}}`
- **THEN** Unreal 侧返回 `ok:false`，`error.code = "INVALID_PARAMS"`，`error.message` 包含字面子串 `task_id`

#### Scenario: task_id 为空字符串时被拒绝

- **WHEN** CLI 发送 `{command:"get_task_status", params:{task_id:""}}`
- **THEN** Unreal 侧返回 `ok:false`，`error.code = "INVALID_PARAMS"`，`error.message` 包含字面子串 `task_id`

### Requirement: `cancel_task` 读取 `task_id`

Unreal core 命令 `cancel_task` SHALL 从 `params.task_id`（snake_case）读取任务标识。当 `task_id` 缺失、为空字符串或非字符串类型时，命令 SHALL 返回结构化错误，`error.code` 为 `INVALID_PARAMS`，`error.message` 必须包含字面子串 `task_id`。

命令 SHALL 在找到任务后调用其 `Cancel()` 方法（当任务处于 `pending` 或 `running` 时），并返回 `ok:true`。成功响应的 `data` 对象使用与 `get_task_status` 相同的结构：恰好包含 `task_id` 与 `status`，并在对应数据存在时包含可选 `payload` / `error`。取消语义通过 `ok:true`（请求被受理）与后续 `status` 的演化（由 `running` 过渡到 `cancelled`）共同表达。

#### Scenario: 合法 task_id 取消正在运行的任务

- **WHEN** CLI 发送 `{command:"cancel_task", params:{task_id:"task-2"}}` 且 `task-2` 处于 `pending` 或 `running` 状态
- **THEN** Unreal 侧调用该任务的 `Cancel()` 方法，并返回 `ok:true`，`data.task_id == "task-2"`，`data.status` 属于 `{pending, running, cancelled}`
- **AND** `data` 的键集合是 `{task_id, status}`，或再加上 `payload` / `error` 中实际有值的那一项

#### Scenario: 缺失 task_id 时错误消息使用 snake_case 字段名

- **WHEN** CLI 发送 `{command:"cancel_task", params:{}}`
- **THEN** Unreal 侧返回 `ok:false`，`error.code = "INVALID_PARAMS"`，`error.message` 包含字面子串 `task_id`
