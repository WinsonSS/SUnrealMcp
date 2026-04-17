## Context

本变更是 Skeleton Agent-efficiency punch list 里第 5 项（response cleanups）的落地。前四项已经把「传参大小写不一致」「信封冗余」「help 冗余」「大响应静默截断」四个方向处理掉，剩下的都是响应体里**具体字段**层面的冗余与概念混杂，分散在 `ping`、`reload_command_registry` 和 task 响应三处。当前状态下每次 task 轮询都会携带 `task_name` + `completed` + 可能的 `accepted`/`cancel_requested`，其中 `completed` 和 `status` 严格同义，`accepted`/`cancel_requested` 则把「动作结果」硬塞进「任务状态」里，消费方需要自己分辨这是哪一层的信息。

另一个并行问题：`PingCommand` 返回的 `protocolVersion` 是 camelCase，但它位于 `data` 字段内（不是 envelope），按照 `core-command-wire-protocol` 的 snake_case 约定本应是 `protocol_version`。这个小瑕疵此次连同精简一起修复。

## Goals / Non-Goals

**Goals:**
- `ping.data` 只剩 `protocol_version` 一个字段。
- `reload_command_registry.data` 是空对象 `{}`。
- task 响应 `data` 最小集合：`task_id` + `status`（+ 可选 `payload` + 可选 `error`）。
- 通过 spec delta 把 `task_name`、`completed`、`accepted`、`cancel_requested` 明确列为禁止字段，避免后续实现漂移回旧形态。

**Non-Goals:**
- 不改 envelope 层级（`ok`/`data`/`error`/`requestId`/`protocolVersion` 保持 camelCase，沿用 `slim-envelope` 的约定）。
- 不为 `task_name` 引入 include/opt-in 开关——简单直接地移除，Agent 发请求时自己知道在跟哪个 task 打交道。
- 不重新组织 task 响应结构（比如不把 payload 拍到顶层、不引入 `action` 子对象），保持当前扁平结构只是字段更少。
- 不修改 long-running task 的 payload 内部结构（那是 task 作者的私有命名空间，`core-command-wire-protocol` 已经明确豁免）。

## Decisions

### Decision 1：用空 `data: {}` 而不是省略 `data` 字段

`reload_command_registry` 和类似命令成功时 `data` 没有有意义的内容。两种选择：

- (a) `data` 字段不出现在响应里；
- (b) `data` 始终存在、值为空对象 `{}`。

选 (b)。原因：`FSUnrealMcpResponse::MakeSuccess` 接受一个 `TSharedPtr<FJsonObject>`，当前实现里 envelope 序列化时会写入 `data`。统一让 success 响应总是带 `data` 字段（即使为空）可以让消费方代码无需判断「有没有这个字段」，只需判断「这个字段里有没有我要的 key」。这与 `slim-envelope` 成功响应总带 `data` 的现状一致。

### Decision 2：task 响应移除 `accepted` 与 `cancel_requested`，不做替换

现状把这两个字段写在 task `data` 里：
- `MakeAcceptedResponse` 在 `BuildTaskData` 后额外 `SetBoolField("accepted", true)`；
- `CancelTask` 在 `BuildTaskData` 后额外 `SetBoolField("cancel_requested", true)`。

替代方案：
- (a) 保留两个字段，只是在 spec 里明确它们属于「动作结果」而非「任务状态」；
- (b) 把它们挪到 `data.action` 之类的子对象里；
- (c) 彻底移除。

选 (c)。原因：`ok:true` 已经承载「动作被服务端受理」的语义——如果服务端没受理，早就是 `ok:false`。`cancel_requested:true` 也是类似冗余；实际的取消结果反映在 `status` 字段上（从 `running` 变成 `cancelled` 可能需要一个或多个 tick）。消费方要判断「是否在取消中」就读 `status`，这比 `cancel_requested` 配 `status` 两个字段更不容易出错。

选 (a) 的问题：spec 里「概念上是动作结果但字段混在 data 里」长期会让消费方误解；(b) 额外引入 `action` 子对象就是为了解决「混杂」这个问题，但既然两个字段本身就冗余，还不如直接删。

### Decision 3：默认不返回 `task_name`，不提供 include 开关

`task_name` 目前无条件出现。Agent 发起 task 时自己知道在调什么命令、什么 task 类型，拉状态时重复收到同一个 `task_name` 字段属于纯冗余。

替代方案：
- (a) 保留无条件返回；
- (b) 默认不返回，加 `include_task_name` 之类的 params 开关；
- (c) 彻底移除。

选 (c)。理由：开关会让 CLI 参数表膨胀，而这个场景实际上没有已知的真实需求——调试时用 `--verbose` 信封或者直接读服务端日志更合适。如果未来确实有需要再加回来成本很低。

### Decision 4：`ping` 响应字段 `protocol_version` 用 snake_case

现状 `PingCommand` 写的是 `protocolVersion`。按 `core-command-wire-protocol` 的 Requirement 1：envelope 字段豁免 snake_case，但 `ping.data.protocolVersion` 是 data 字段不是 envelope 字段，因此应该是 `protocol_version`。此次顺路修正，避免后续 Agent 因为「envelope 里用 protocolVersion、data 里用 protocol_version」感到混乱。

## Risks / Trade-offs

- [已有消费者依赖 `task_name` / `completed` / `accepted` / `cancel_requested`] → 本项目当前无稳定外部消费者，Skill/Agent 侧逻辑可随本变更同步更新；但未来若要对接第三方工具，需要在版本说明里点出这几个字段已移除。Mitigation：在 `core-command-wire-protocol` spec delta 中把这四个字段写成「禁止出现」而非「可选」，让后续实现不会悄悄把它们加回来。
- [`status` 的枚举字符串成为唯一「完成/未完成」信号] → 所有消费方必须知道 `succeeded`/`failed`/`cancelled` 三态都属于已完成。Mitigation：`core-command-wire-protocol` 的 `get_task_status` Requirement 描述里显式枚举这三个终态。
- [`ping.protocolVersion` → `protocol_version` 是字段级破坏] → 这只是个核对版本的诊断命令，消费方通常只读值不看字段名；但若有脚本 `response.data.protocolVersion` 会坏。Mitigation：spec 中写明新字段名，变更合入时同步 Skill 侧任何引用。

## Migration Plan

1. 先落盘 spec delta（定义好新契约）。
2. 再改 Unreal 侧三个文件（PingCommand、ReloadCommandRegistryCommand、SUnrealMcpTaskRegistry）。
3. Skill/CLI 侧如果有任何地方读取将被移除的字段（`data.completed` / `data.accepted` / `data.cancel_requested` / `data.task_name` / `data.protocolVersion`），同步改为新字段。
4. 无需版本位或 feature flag：`ProtocolVersion` 保持 `1`，本变更不引入 wire 兼容层。

## Open Questions

无。
