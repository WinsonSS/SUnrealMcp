## Why

`system get_task_status` 和 `system cancel_task` 这两个 CLI 命令目前端到端是坏的：CLI 注册的参数名是 `task_id`（snake_case），透传到线上的 `params.task_id`；而 Unreal 侧的 handler 读的是 `params.taskId`（camelCase），导致每次调用都返回 `INVALID_PARAMS: Missing taskId.`。任何依赖长耗时 task 的工作流都被挡住。这个 mismatch 还暴露了更底层的问题——项目里没有任何地方约定 CLI↔Unreal wire 上的参数名规范，新命令完全可以往任意方向漂移。

## What Changes

- 确定 **snake_case** 作为 CLI↔Unreal wire 协议中 `params` / `data` JSON 字段名的正式约定。
- 修正 Unreal 侧 `get_task_status` 和 `cancel_task` 的 handler，把 `taskId` 改读 `task_id`，让它们与 CLI 侧注册的参数名对齐。
- 修正 `FSUnrealMcpTaskRegistry::BuildTaskData`（以及 `MakeAcceptedResponse` / `CancelTask` 添加的元数据字段），让 `get_task_status` / `cancel_task` 返回的响应 `data` 中所有 framework 字段都使用 snake_case：`taskId` → `task_id`，`taskName` → `task_name`，`cancelRequested` → `cancel_requested`。
- 在新的 `core-command-wire-protocol` capability spec 里写下 wire-参数命名约定，作为后续 core 命令的规范源。
- **BREAKING**：
  - 任何外部调用者绕过 CLI 直接发原始 JSON 用了 `taskId` 入参的，需要改成 `task_id`。CLI 是唯一官方入口，影响面应该很小。
  - 任何 Agent / 调用者从 `get_task_status` / `cancel_task` 响应中读 `taskId`/`taskName`/`cancelRequested` 的代码需要改成 snake_case。鉴于这两个命令此前完全无法工作（INVALID_PARAMS），实际"破坏面"几乎为零。

范围外：本次不审计、不改动 stable / temporary 命令；`Commands/Core/PingCommand.cpp` 的 `protocolVersion` 数据字段同样推迟到 follow-up 的"响应清理"change（#5），因为 Ping 的整个响应会在那里被进一步瘦身，本次先 rename 反而是无效 churn。

## Capabilities

### New Capabilities

- `core-command-wire-protocol`：定义嵌入式 CLI 与 Unreal 插件之间 core 命令的跨层契约——包括 `params` 对象的字段命名约定，以及 `get_task_status` / `cancel_task` 的 `task_id` 参数契约。

### Modified Capabilities

_无——目前 `openspec/specs/` 里还没有任何已发布的 spec。_

## Impact

- **代码**：
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/GetTaskStatusCommand.cpp` —— 改读 `task_id`；错误消息文字同步更新。
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/CancelTaskCommand.cpp` —— 改读 `task_id`；错误消息文字同步更新。
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Mcp/SUnrealMcpTaskRegistry.cpp` —— `BuildTaskData` 写出的 `taskId` / `taskName` 改为 `task_id` / `task_name`；`CancelTask` 写出的 `cancelRequested` 改为 `cancel_requested`。
  - `Skill/sunrealmcp-unreal-editor-workflow/cli/src/commands/core/system_commands.ts` —— 代码本身不需要改（已经是 `task_id`），但要验证。
- **协议 / 文档**：新建 capability spec 描述 wire 参数命名规则。**不** bump `ProtocolVersion`——这是单命令字段层的 rename，不是 envelope 层的变更，且命令原本就是 non-functional 的。
- **外部调用者**：任何直接发送 `taskId` 的非 CLI 客户端需要改成 `task_id`。Unreal 侧对旧名字会回 `INVALID_PARAMS: Missing task_id.`，方便自诊断。
- **CLI 侧**：`system_commands.ts` 已经是 `task_id`，终端用户的命令行语法不变。
