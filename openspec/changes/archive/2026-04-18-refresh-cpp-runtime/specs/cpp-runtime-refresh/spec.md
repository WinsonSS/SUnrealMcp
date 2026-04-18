## ADDED Requirements

### Requirement: Unreal core 命令 `refresh_cpp_runtime` 启动异步运行态刷新任务

Unreal core 命令 `refresh_cpp_runtime` SHALL 接受空 `params` 对象 `{}`，并通过现有 `FSUnrealMcpTaskRegistry` 启动一个异步 task，用于尝试让当前 Unreal C++ 改动在不关闭编辑器的前提下生效。命令成功入队时 SHALL 返回 `ok:true`，并沿用现有 accepted-task 响应结构，`data` 至少包含：

- `task_id`（string）
- `status`（string，初值属于 `{pending, running}`）

命令自身 SHALL NOT 同步阻塞等待 Live Coding 完成；最终结果 SHALL 通过 `get_task_status` 获取。

#### Scenario: 调用命令后立即拿到 task accepted 响应

- **WHEN** CLI 发送 `{command:"refresh_cpp_runtime", params:{}}`
- **THEN** Unreal 侧返回 `ok:true`
- **AND** `data.task_id` 为非空字符串
- **AND** `data.status` 属于 `{pending, running}`

### Requirement: 任务成功前必须先完成 Live Coding 与 registry reload

`refresh_cpp_runtime` 对应 task 在进入 `succeeded` 之前 SHALL 完成以下最小工作流：

1. 检查当前编辑器状态是否允许本项目执行运行态刷新；
2. 触发 Live Coding；
3. 等待 Live Coding 完成；
4. 在 Live Coding 成功后执行 `reload_command_registry`；
5. 仅当以上步骤全部成功时，任务才进入 `succeeded`。

成功终态的 `payload` SHALL 至少包含：

- `live_coding = "succeeded"`
- `command_registry_reloaded = true`

#### Scenario: Live Coding 与 reload 都成功后任务完成

- **WHEN** `refresh_cpp_runtime` task 成功触发 Live Coding，且后续 `reload_command_registry` 也成功
- **THEN** 后续 `get_task_status` 返回 `ok:true`
- **AND** `data.status == "succeeded"`
- **AND** `data.payload.live_coding == "succeeded"`
- **AND** `data.payload.command_registry_reloaded == true`

### Requirement: PIE 运行中时任务返回轻量环境阻塞错误

虽然 Unreal 官方支持在 PIE 期间执行 Live Coding，但本项目的 `refresh_cpp_runtime` workflow SHALL 在检测到 PIE 正在运行时直接失败，以避免 Agent 在用户主动 playtest 期间做运行态改动。

当 PIE 正在运行时，任务 SHALL：

- 进入 `failed`
- 返回 `error.code = "PIE_ACTIVE"`
- 在 `error.details` 中至少包含：
  - `resolution = "needs_user_editor_action"`
  - `suggested_action = "stop_pie_and_retry"`
  - `required_user_action = "stop_pie"`

#### Scenario: PIE 中调用 refresh_cpp_runtime

- **WHEN** 用户当前正在 PIE，CLI 发送 `{command:"refresh_cpp_runtime", params:{}}`
- **THEN** 任务最终进入 `failed`
- **AND** `data.error.code == "PIE_ACTIVE"`
- **AND** `data.error.details.resolution == "needs_user_editor_action"`
- **AND** `data.error.details.required_user_action == "stop_pie"`

### Requirement: 代码/构建错误归类为 Agent 可继续处理的失败

当 Live Coding 已成功启动，但因当前代码或构建错误而未能完成 patch 时，任务 SHALL 进入 `failed`，并返回：

- `error.code = "LIVE_CODING_COMPILE_FAILED"`
- `error.details.resolution = "agent_retry"`
- `error.details.suggested_action = "fix_code_and_retry"`

任务 MAY 在 `error.details` 中附带额外诊断信息（例如编译日志摘录、阶段名），但 SHALL NOT 把这类失败归类成需要用户授权的重路径。

#### Scenario: Live Coding 因当前代码错误失败

- **WHEN** `refresh_cpp_runtime` task 触发了 Live Coding，但编译阶段因当前代码错误失败
- **THEN** 后续 `get_task_status` 返回 `data.status == "failed"`
- **AND** `data.error.code == "LIVE_CODING_COMPILE_FAILED"`
- **AND** `data.error.details.resolution == "agent_retry"`

### Requirement: 无法执行 Live Coding 时要求用户授权进入重路径

当当前环境无法执行 Live Coding，导致 Agent 需要考虑“关闭编辑器并完整重编”这类会打断用户会话的重路径时，任务 SHALL 进入 `failed`，并返回：

- `error.code = "LIVE_CODING_UNAVAILABLE"`，或实现阶段确认需要时的更具体环境类错误码
- `error.details.resolution = "needs_user_approval"`
- `error.details.suggested_action = "close_editor_and_rebuild"`

这类失败 SHALL 明确表达：`refresh_cpp_runtime` 本身不会自动关闭编辑器或自动重编，后续是否升级到重路径由 Agent 征求用户允许后决定。

#### Scenario: 当前环境无法执行 Live Coding

- **WHEN** `refresh_cpp_runtime` task 发现当前环境无法执行 Live Coding
- **THEN** 后续 `get_task_status` 返回 `data.status == "failed"`
- **AND** `data.error.details.resolution == "needs_user_approval"`
- **AND** `data.error.details.suggested_action == "close_editor_and_rebuild"`

### Requirement: 已有 Live Coding 编译进行中时返回轻量环境阻塞错误

当 `refresh_cpp_runtime` 尝试触发 Live Coding 时，若编辑器里另一轮 Live Coding 编译已经在运行，任务 SHALL 进入 `failed`，并返回：

- `error.code = "LIVE_CODING_BUSY"`
- `error.details.resolution = "needs_user_editor_action"`
- `error.details.suggested_action = "wait_for_current_compile_and_retry"`

#### Scenario: 已有 Live Coding 编译在进行中

- **WHEN** `refresh_cpp_runtime` task 触发时，当前 editor session 中已有另一轮 Live Coding 编译正在进行
- **THEN** 后续 `get_task_status` 返回 `data.status == "failed"`
- **AND** `data.error.code == "LIVE_CODING_BUSY"`
- **AND** `data.error.details.resolution == "needs_user_editor_action"`

### Requirement: registry reload 失败时返回独立错误

当 Live Coding 已成功完成，但后续 `reload_command_registry` 失败时，任务 SHALL 进入 `failed`，并返回：

- `error.code = "REGISTRY_RELOAD_FAILED"`

这类失败 SHALL 与 `LIVE_CODING_*` 错误分离，使 Agent 能区分“代码未生效”与“代码已生效但命令表未刷新”两类问题。

#### Scenario: Live Coding 成功但 registry reload 失败

- **WHEN** `refresh_cpp_runtime` task 成功完成 Live Coding，但 `reload_command_registry` 失败
- **THEN** 后续 `get_task_status` 返回 `data.status == "failed"`
- **AND** `data.error.code == "REGISTRY_RELOAD_FAILED"`

### Requirement: CLI `system refresh_cpp_runtime` 透传空参数调用

CLI `system` family SHALL 注册命令 `refresh_cpp_runtime`，其定义至少满足：

- `lifecycle = "core"`
- `unrealCommand = "refresh_cpp_runtime"`
- `parameters = []`

Agent 执行 `sunrealmcp-cli system refresh_cpp_runtime` 时，CLI SHALL 向 Unreal 发送：

```json
{ "command": "refresh_cpp_runtime", "params": {} }
```

#### Scenario: Agent 调用 system refresh_cpp_runtime

- **WHEN** Agent 执行 `sunrealmcp-cli system refresh_cpp_runtime`
- **THEN** CLI 向 Unreal 发送的请求 `command == "refresh_cpp_runtime"`
- **AND** `params` 为空对象 `{}`
