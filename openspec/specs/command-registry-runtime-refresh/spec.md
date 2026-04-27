# command-registry-runtime-refresh Specification

## Purpose
定义 Agent 修改 Unreal plugin 后继续工作的运行态链路：命令注册表内省、CLI/Unreal command surface 差异检查，以及 `refresh_cpp_runtime` 的 Live Coding + registry reload 工作流。

## Requirements

### Requirement: Unreal command registry 暴露只读内省

`FSUnrealMcpCommandRegistry` SHALL 提供只读访问器：

- `TArray<FString> ListCommandNames() const`：返回当前成功注册的 command 名字副本，按字典序升序排序。
- `bool HasCommand(const FString& Name) const`：命中已注册 command 时返回 `true`，大小写敏感。

Command handler SHALL 通过 `FSUnrealMcpExecutionContext` 的方法查询当前请求可见的注册表视图，而不是直接持有 registry 本体。

#### Scenario: 注册表返回排序副本

- **WHEN** 已注册 `zeta`、`alpha`、`mu`
- **THEN** `ListCommandNames()` 返回 `{alpha, mu, zeta}`

### Requirement: `check_command_exists` 查询单个 command

Unreal core command `check_command_exists` SHALL 从 `params.name` 读取目标 command 名。缺失或为空时 SHALL 返回 `INVALID_PARAMS`。成功响应 `data` SHALL 只包含 `exists`，其值来自当前注册表视图。

CLI `system check_command_exists` SHALL 透传参数 `name`。

#### Scenario: 查询已注册 command

- **WHEN** Agent 执行 `sunrealmcp-cli system check_command_exists --name ping`
- **THEN** CLI 向 Unreal 发送 `{command:"check_command_exists", params:{name:"ping"}}`
- **AND** Unreal 返回 `data.exists == true`

### Requirement: `diff_commands` 返回 CLI/Unreal 双向差集

Unreal core command `diff_commands` SHALL 从 `params.cli_names` 读取 CLI 已注册 Unreal command 名数组。成功响应 `data` SHALL 只包含：

- `only_in_unreal`：Unreal 已注册但 CLI 未声明的 command 名数组，升序排序。
- `only_in_cli`：CLI 声明但 Unreal 未注册的 command 名数组，升序排序。

CLI `system diff_commands` SHALL 通过本地 registry 自动采集非 `raw` family、定义了静态 `unrealCommand` 的 command 名，去重排序后作为 `cli_names` 发送。

#### Scenario: 双侧 command surface 一致

- **WHEN** CLI 与 Unreal command 名完全一致
- **THEN** `diff_commands` 返回 `only_in_unreal == []`
- **AND** `only_in_cli == []`

### Requirement: `refresh_cpp_runtime` 启动异步运行态刷新任务

Unreal core command `refresh_cpp_runtime` SHALL 接受空 `params`，并启动异步 task。命令本身 SHALL 立即返回 accepted-task 响应，`data` 至少包含 `task_id` 与初始 `status`。

CLI `system refresh_cpp_runtime` SHALL 透传空参数调用 Unreal command。

#### Scenario: 启动刷新任务

- **WHEN** Agent 执行 `sunrealmcp-cli system refresh_cpp_runtime`
- **THEN** Unreal 返回 `ok:true`
- **AND** `data.task_id` 为非空字符串
- **AND** `data.status` 属于 `{pending, running}`

### Requirement: 刷新成功前必须完成 Live Coding 与 registry reload

`refresh_cpp_runtime` task 进入 `succeeded` 之前 SHALL 完成 preflight、触发 Live Coding、等待 Live Coding 成功、执行 `reload_command_registry`。成功终态 payload SHALL 至少包含 `live_coding == "succeeded"` 与 `command_registry_reloaded == true`。

成功后，同一 Editor 会话内新增或修改的 command SHALL 立即对 `check_command_exists` 与 `diff_commands` 可见；Agent 不需要再单独调用 `reload_command_registry`。

#### Scenario: Live Coding 与 reload 都成功

- **WHEN** `refresh_cpp_runtime` task 成功完成
- **THEN** `get_task_status` 返回 `data.status == "succeeded"`
- **AND** `data.payload.live_coding == "succeeded"`
- **AND** `data.payload.command_registry_reloaded == true`

### Requirement: 刷新失败需要可行动错误分类

`refresh_cpp_runtime` SHALL 用结构化错误区分失败原因：

- PIE 运行中：`PIE_ACTIVE`，`details.resolution == "needs_user_editor_action"`，建议停止 PIE 后重试。
- Live Coding 编译失败：`LIVE_CODING_COMPILE_FAILED`，`details.resolution == "agent_retry"`，建议修代码后重试。
- Live Coding 不可用：`LIVE_CODING_UNAVAILABLE` 或更具体环境错误，`details.resolution == "needs_user_approval"`，建议询问用户是否关闭 Editor 并完整重编。
- 已有 Live Coding 进行中：`LIVE_CODING_BUSY`，`details.resolution == "needs_user_editor_action"`。
- Live Coding 成功但 registry reload 失败：`REGISTRY_RELOAD_FAILED`。

#### Scenario: 代码错误导致 Live Coding 失败

- **WHEN** Live Coding 因当前代码错误失败
- **THEN** task 进入 `failed`
- **AND** `data.error.code == "LIVE_CODING_COMPILE_FAILED"`
- **AND** `data.error.details.resolution == "agent_retry"`
