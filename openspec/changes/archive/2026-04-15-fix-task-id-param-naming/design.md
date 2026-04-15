## Context

SUnrealMcp 工具包含两个骨架：嵌入 Claude Skill 的 TypeScript CLI，以及运行在 Unreal 编辑器内的 C++ 插件。两者通过 TCP + 换行分隔的 JSON 通信。CLI 发送 `{protocolVersion, requestId, command, params}`，插件回复 `{protocolVersion, requestId, ok, data|error}`。`params` 是一个松散的 JSON 对象，wire 层没有 schema 校验字段名。

目前代码里存在两种互相冲突的参数命名习惯：

- **CLI 侧**的选项注册（`CliParameterDefinition.name`）统一用 snake_case：`task_id`、`params_json`、`timeout_ms`，stable 命令里也都是 `blueprint_path`、`actor_label` 这类。
- **Unreal 侧**的 core 命令 handler（`GetTaskStatusCommand`、`CancelTaskCommand`）读的却是 camelCase：`taskId`。

原因在 `Skill/.../cli/src/index.ts:133` 的 `executeCommand`——除非命令显式提供 `mapParams` 回调，否则 `parsed.values` 会原样作为 `params` 发送。所以 CLI 侧的 snake_case 选项名直接变成了 wire 字段名，而 Unreal 侧读 `taskId` 就命中了 `INVALID_PARAMS`。`system get_task_status` 和 `system cancel_task` 因此彻底坏掉。

这是本项目第一次需要在约定层级上做决定，所以除了修 bug，也要把约定写进 capability spec，让后续 core 命令有规范可依。

## Goals / Non-Goals

**Goals:**

- 让 `system get_task_status` 和 `system cancel_task` 回到可用状态。
- 选定并文档化一条 wire `params` 字段命名约定，防止未来的 core 命令继续漂移。
- 修复范围严格收窄——只改这两个命令和新 spec，不做大规模 sweep。

**Non-Goals:**

- 不审计、不改 stable / temporary 命令的参数命名。它们不阻塞 task 功能，等后续 sweep change 再处理。
- 不修改 envelope 字段（`protocolVersion`、`requestId`、`ok`、`data`、`error`）——这些是 `ProtocolVersion = 1` 的一部分，动它们是一次完整的协议升级，不属于本次范围。
- 不引入运行时 schema 校验器，也不引入通过 `mapParams` 做大小写自动转换的"翻译层"。
- 不 bump `ProtocolVersion`。

## Decisions

### Decision 1：wire `params` / `data` 采用 snake_case

**选择：** 请求 `params` 对象和响应 `data` 对象内的所有字段名都使用 snake_case。

**理由：**

- CLI 侧每一个现有文件的选项名已经是 snake_case（`system_commands.ts` 和所有 stable 命令），并且会通过 `executeCommand` 直接落到 `params`。选 snake_case 意味着除了两个坏掉的 Unreal handler 以外，**CLI 侧零改动**。
- 选 camelCase 需要 sweep 每一个 stable 命令文件（要么 rename 选项、要么加 `mapParams`）——为修同一个 bug 要付出大得多的代价。
- Agent 和人类在终端里敲的 `--option` 本身就是 snake_case，wire 参数保持 snake_case 可以把两个命名世界合成一个。

**考虑过的替代方案：**

- **camelCase on wire（把 CLI 侧对齐 Unreal 侧）**。拒绝：要改每一个 CLI 命令文件，还要到处塞 `mapParams`，并且和 `--snake_case` 的 CLI 接口本身别扭。
- **在 `executeCommand` 里加 snake_case → camelCase 的自动翻译层**。拒绝：会引入一条看不见的规则，自定义 `mapParams` 的命令很容易忘，而且它只是把 convention 问题藏起来，并没有真的回答"命名规范是什么"。

**规则范围：** 约定只作用于 `params` 和 `data` 内部字段。envelope 字段（`protocolVersion`、`requestId`、`ok`、`error.code`、`error.message`、`error.details`）保持 camelCase，因为它们属于 `ProtocolVersion = 1` 的已发布契约，改动它们会把变更升级成协议破坏性变更。

### Decision 2：改 Unreal 侧，不改 CLI 侧

**选择：** 更新 `GetTaskStatusCommand.cpp` 和 `CancelTaskCommand.cpp` 读 `task_id`；`system_commands.ts` 不动。

**理由：** Decision 1 确立 snake_case 为权威，CLI 侧在新规则下本来就是对的——漂移的是 Unreal 侧的两个 handler。

### Decision 2.5：响应 `data` 的 framework 字段也要 snake_case，本次一并修

**选择：** 在 `FSUnrealMcpTaskRegistry::BuildTaskData` / `MakeAcceptedResponse` / `CancelTask` 中，把所有 framework 拼装的 `data` 字段从 camelCase 迁到 snake_case：`taskId` → `task_id`、`taskName` → `task_name`、`cancelRequested` → `cancel_requested`。

**理由：**

- Decision 1 的规则同时覆盖 `params` 和 `data`，如果只修 `params` 那一半、`data` 那一半留着违规，spec 在归档当天就和代码不一致，spec 的权威性立刻打折。
- `BuildTaskData` 是 framework 代码（`Mcp/SUnrealMcpTaskRegistry.cpp`），不属于 proposal 范围外的 stable / temporary 命令；它正好是 `get_task_status` / `cancel_task` 这两个本来就要修的命令的响应组装函数，scope 上自然延伸过来。
- 改动只有 3 个字段、1 个文件，merge 风险低；和命令侧的 `task_id` rename 放在同一个 commit 让历史更可读。

**考虑过的替代方案：**

- **缩窄 spec，只约束 `params`。** 拒绝：选 snake_case 的核心理由本来就是"让 Agent 读响应顺手"，把 `data` 排除等于自废武功；后续 #5（响应清理）也指望本 spec 作为依据。
- **新开 follow-up change 单独 sweep `data`。** 拒绝：违反"一次同类型修复在一个 commit 里"的串行节奏，且让 spec 在两次归档之间处于不一致状态。

**对 task payload 的边界：** task 自身用 `BuildPayload` 返回的内容（例如 `StartMockTaskCommand` 的 `remainingTicks`）由 task 实现拥有，不在 framework 强制范围内。spec 的命名约定只覆盖 framework 自己拼装的字段；payload 内部的字段命名留给写 task 的人，未来可以单独立约定，但本次不强加。

### Decision 3：错误消息使用 wire 字段名

**选择：** 两个 handler 返回的错误消息文字为 `Missing task_id.`，不是 `Missing taskId.`、也不是 `Missing task id.`。

**理由：** 错误消息的消费者是 Agent，它需要把错误映射回自己实际发送的字段名。直接写出线上字段名是从"看到错误"到"修好调用"的最短路径。

### Decision 3.5：Ping 的 `data.protocolVersion` 推迟到 #5 处理

**选择：** `Commands/Core/PingCommand.cpp:23` 在 `data` 里写了 `protocolVersion`（驼峰），严格说违反 Decision 1。本次 change 不改它，留到后续的「响应清理」change（#5）一起处理。

**理由：** #5 已经计划把 Ping 响应大幅瘦身——`server`/`plugin` 是常量字符串、`status` 隐含在 `ok` 里、`protocolVersion` 同样可以删（envelope 已经携带）。如果本次先 rename 成 `protocol_version`，下一次 change 又把它整个删掉，是无效 churn 且会污染 commit 历史。

**风险接受：** spec 归档之后到 #5 落地之间的窗口里，Ping 响应会处于"代码与规范不一致"的状态。窗口由"按计划立刻进入 #5 propose"控制，应当很短，且仅影响 Ping 一个命令。

### Decision 3：错误消息使用 wire 字段名

**选择：** 两个 handler 返回的错误消息文字为 `Missing task_id.`，不是 `Missing taskId.`、也不是 `Missing task id.`。

**理由：** 错误消息的消费者是 Agent，它需要把错误映射回自己实际发送的字段名。直接写出线上字段名是从"看到错误"到"修好调用"的最短路径。

### Decision 4：约定写进 capability spec，而不是代码注释

**选择：** 创建 `specs/core-command-wire-protocol/spec.md`，把 snake_case 规则写成正式的 normative requirement。

**理由：** 一条写在 `index.ts` 的注释对一个在 C++ 里写新 Unreal 命令的人来说是完全隐形的。capability spec 是本仓库 OpenSpec 设置下约定的合同位置，后续 proposal（比如响应信封瘦身、响应清理等后续 change）都可以直接引用它。

## Risks / Trade-offs

- **风险：** 如果某个外部工具绕过 CLI 直接向 Unreal 服务器发 `taskId`，它会挂。
  **缓解：** CLI 是唯一文档化入口且工具尚未稳定；错误消息明确写 `Missing task_id.`，外部调用者可立即自诊断。

- **风险：** stable 命令的 Unreal 侧 handler 可能仍然存在 camelCase 字段，本次不扫这些地方，导致规范只被部分执行。
  **缓解：** spec 以 normative 形式落地后，后续 sweep change 可以直接按规则推进，不用重新吵定义。spec 文件本身就是那次 sweep 的权威依据。

- **取舍：** envelope 用 camelCase，`params` / `data` 用 snake_case，两种风格并存。接受——envelope 是已发布协议版本，动它是一场比本次更大的迁移，不是这次 change 要承担的范围。

## Migration Plan

1. 落地 Unreal 侧两个文件的 rename。
2. 发布 capability spec。
3. 没有运行时迁移——旧行为本来就是坏的，所谓 "rollback" 就是退回坏掉的状态。无需版本 bump。

## Open Questions

_无。_
