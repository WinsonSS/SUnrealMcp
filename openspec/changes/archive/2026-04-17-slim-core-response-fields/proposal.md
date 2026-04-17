## Why

Core 命令的响应体里混了大量对 Agent 没有信息量的字段：`ping` 塞入三个常量、`reload_command_registry` 回显一个已知的命令名、task 状态数据同时返回 `status` 和由它派生的 `completed`、把 `accepted` / `cancel_requested` 这种「本次响应元信息」硬塞进 task 本身的数据字段，再加上一个大多数时候 Agent 并不需要的 `task_name`。每次调用都在稳定地烧 token，也让消费侧在理解响应时多走一步「这个字段是 task 状态还是动作结果」。

## What Changes

- `ping` 响应 `data` 字段精简为单一 `protocol_version`（顺带把原来的 `protocolVersion` 对齐到 `core-command-wire-protocol` 的 snake_case 约定），移除常量 `server`、`plugin`、`status`。
- `reload_command_registry` 响应 `data` 设为空对象 `{}`；原来的 `reloaded:true` 与 `command:"reload_command_registry"` 去除。
- task 状态/动作响应 `data` 精简为 `task_id` + `status` + 可选 `payload` + 可选 `error`：
  - 移除派生字段 `completed`（由 `status` 派生：`succeeded`/`failed`/`cancelled` 三态即完成）。
  - 移除 `task_name` 字段（默认不返回，不提供 include 开关）。
  - 移除 `accepted` 字段（由 `ok:true` 承载「被受理」语义）。
  - 移除 `cancel_requested` 字段（由 `status` 反映任务当前状态）。
- **BREAKING**：上述修改会删除现有 `core-command-wire-protocol` spec 中对 `task_name`、`completed`、`cancel_requested` 的显式要求，因此对该 spec 增加 delta；任何当前依赖这些字段的外部消费者都需要改用 `status` 字段判断状态。

## Capabilities

### New Capabilities
- `slim-core-responses`: 定义 `ping` 与 `reload_command_registry` 两个核心命令的精简响应格式（包含字段清单、禁止字段、以及 `ping` 响应的 snake_case 对齐）。

### Modified Capabilities
- `core-command-wire-protocol`: 重写 `get_task_status` / `cancel_task` 响应要求，把 task 响应 `data` 的字段集收窄到 `task_id` + `status`（以及可选 `payload` / `error`）。

## Impact

- 代码：
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/PingCommand.cpp`
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/ReloadCommandRegistryCommand.cpp`
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Mcp/SUnrealMcpTaskRegistry.cpp`（`BuildTaskData`、`MakeAcceptedResponse`、`CancelTask` 三处）
- 测试：Unreal 侧的 wire-protocol 单测（若存在针对现有字段的断言）需要同步更新。
- 下游：Skill/Agent 侧若有逻辑依赖被删字段，需改用 `status` 字符串比较；`ok:true` 即视作任务已被受理。
