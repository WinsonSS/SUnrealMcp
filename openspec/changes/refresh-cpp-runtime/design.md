## Context

`sunrealmcp-unreal-editor-workflow` 已经把“补完 Unreal C++ 命令后，优先让改动在当前编辑器会话里生效”定义成标准流程，但当前仓库缺的是一个正式的、可被 Agent 调用的工作流入口：

- 插件侧只有 `reload_command_registry`，它只能刷新已经进入进程的命令表，不能触发 Live Coding。
- CLI 侧也没有对应的 `system` wrapper，所以 Skill 只能停留在原则描述，不能落到真实命令链路。
- 一旦 Live Coding 失败，Agent 现在很容易直接要求用户手动编译，但这会把本应由 Agent 继续处理的代码问题过早转嫁给用户。

同时，Live Coding 失败并不都是同一种含义。对 Agent 来说，失败后的下一步责任归属比“失败”这个字面状态更重要：

```text
refresh_cpp_runtime
        |
        v
   +----+----+
   |         |
   v         v
success    failed
              |
   +----------+-----------+
   |          |           |
   v          v           v
agent_retry  stop_pie   ask_approval
```

这次 change 的目标就是把这条分流能力收敛成一个 task-backed workflow command。

## Goals / Non-Goals

**Goals:**
- 增加一条工作流命令 `refresh_cpp_runtime`，供 Agent 在扩充 Unreal C++ 命令后调用。
- 复用现有 `TaskRegistry`，让 Live Coding 与后续 registry reload 通过异步 task 生命周期表达。
- 明确区分三类失败：
  - Agent 可继续修代码并重试；
  - 用户先调整编辑器状态（例如退出 PIE）再重试；
  - 只有在用户批准后才能进入“关闭编辑器并完整重编”的重路径。
- 在任务成功路径中自动执行 `reload_command_registry`，让新命令对 `check_command_exists` / `diff_commands` 立即可见。
- 为 CLI 和 Skill 提供正式入口，而不是继续依赖口头规则。

**Non-Goals:**
- 不在本 change 中自动执行“关闭编辑器 -> 完整重编 -> 重开工程”的重路径。
- 不为尚未确认存在的 Live Coding 取消边界预先设计新状态，例如 `cancel_ignored`。
- 不尝试覆盖所有 Unreal Live Coding 边角行为，只先覆盖当前 Skill 真正需要的工作流分流。
- 不改变现有 task 基础协议（`pending/running/succeeded/failed/cancelled` 五态仍沿用）。

## Decisions

### Decision 1: 提供工作流命令 `refresh_cpp_runtime`，而不是底层 `trigger_live_coding`

替代方案：
- (a) 只暴露一个“按下 Live Coding 按钮”的原子命令
- (b) 暴露一条更高层的工作流命令，内部编排 Live Coding 与 registry reload

选 (b)。Agent 的真实目标不是“触发一次编译”，而是“让 C++ 改动生效并知道下一步该谁处理”。如果只给原子命令，Agent 还要自己推导：

- 何时检查编辑器状态
- 何时 reload command registry
- 何时把失败视为代码问题，何时升级为用户介入

这些都更适合在 Unreal 侧集中定义成正式工作流。

### Decision 2: `refresh_cpp_runtime` 采用 task 形式，命令本身只负责入队

替代方案：
- (a) 同步 RPC，一次调用里阻塞直到 Live Coding 完成
- (b) task-backed 异步命令，返回 `task_id` 后由 `get_task_status` 轮询

选 (b)。Live Coding 与 registry reload 都不是一帧内完成的同步动作，当前仓库也已经有成熟的 `ISUnrealMcpTask` / `FSUnrealMcpTaskRegistry` 机制。沿用 task 可避免再造一套长耗时命令协议。

命令级行为收口为：

- 请求：`{command:"refresh_cpp_runtime", params:{}}`
- 响应：`ok:true`，`data` 使用现有 accepted-task 结构，至少包含 `task_id` 与 `status`

### Decision 3: 成功路径必须把 `reload_command_registry` 纳入任务内部

替代方案：
- (a) `refresh_cpp_runtime` 只负责编译，registry reload 仍由 Agent 单独调用
- (b) 成功路径里自动 reload，只有 reload 成功后任务才算 `succeeded`

选 (b)。Skill 关心的是“新增命令能否立刻可用”，而不是“编译过程是否单独成功”。如果把 reload 留给 Agent 单独编排，会出现“Live Coding 成功但命令仍不可见”的半完成状态，增加工作流歧义。

因此这次把成功语义定义为：

```text
Live Coding 成功
  -> reload_command_registry 成功
      -> task = succeeded
```

若 reload 失败，则任务整体失败，错误码归为 `REGISTRY_RELOAD_FAILED`。

### Decision 4: PIE 运行中作为显式前置阻塞，而不是等待底层自行报错

Epic 官方文档说明 Live Coding 在技术上可以在 PIE 期间工作；因此“PIE 中默认失败”不是引擎限制，而是本项目的工作流决策。原因是：

- Agent 触发运行态重编时，用户可能正在主动测试游戏；
- 即便引擎支持，边编边玩也会让“这次改动为何生效/未生效”的责任边界变得模糊；
- 对当前工作流来说，更安全、也更可解释的策略是先要求用户停止当前 Play Session。

因此任务在真正触发 Live Coding 前 SHALL 做前置检查：

- 若检测到 PIE 正在运行，任务直接失败；
- 错误码为 `PIE_ACTIVE`；
- 错误 details 指向“用户先停止 PIE，再重试”。

### Decision 5: 失败结果按“下一步责任归属”做结构化分类

替代方案：
- (a) 所有失败都统一成 `LIVE_CODING_FAILED`
- (b) 按下一步归属拆成可机读的错误分类

选 (b)。Agent 的关键问题不是“失败了吗”，而是“下一步应该自己继续修，还是先请用户做动作，还是先请求用户授权做重操作”。因此这次要求至少覆盖以下分类：

| 错误码 | 含义 | 下一步 |
| --- | --- | --- |
| `LIVE_CODING_COMPILE_FAILED` | 代码/构建问题 | Agent 修代码并重试 |
| `PIE_ACTIVE` | 用户当前正在 PIE | 提示用户先停止 PIE，再重试 |
| `LIVE_CODING_BUSY` | 另一轮 Live Coding 已在进行中 | 等当前编译结束后重试 |
| `LIVE_CODING_UNAVAILABLE` | 当前环境无法执行 Live Coding | 请求用户批准进入关闭编辑器/完整重编路径 |
| `REGISTRY_RELOAD_FAILED` | Live Coding 成功，但 registry reload 失败 | Agent/维护者检查插件侧重载逻辑 |

为了让 Skill / Agent 侧不依赖字符串猜测，错误 `details` 至少要包含：

- `resolution`：`agent_retry` / `needs_user_editor_action` / `needs_user_approval`
- `suggested_action`：例如 `fix_code_and_retry`、`stop_pie_and_retry`、`close_editor_and_rebuild`

对于 `PIE_ACTIVE`，再额外包含：

- `required_user_action = "stop_pie"`

### Decision 6: 当前阶段不把“完整重编”自动塞进 `refresh_cpp_runtime`

替代方案：
- (a) `refresh_cpp_runtime` 内部自动 fallback 到关闭编辑器并完整重编
- (b) `refresh_cpp_runtime` 只负责 Live Coding 路径，重路径由用户批准后走别的流程

选 (b)。关闭编辑器可能影响用户未保存内容，也会打断当前会话；这已经超出“默认安全自动化”的范围。当前命令应当只负责把“需要升级到重路径”明确告诉 Agent，而不是自动执行。

这意味着：

- 代码/构建错误：Agent 继续修代码，再次调用 `refresh_cpp_runtime`
- 环境阻塞：Agent 根据错误 details 向用户解释需要的动作
- 只有在用户明确允许后，Agent 才可以进入关闭编辑器/完整重编的流程

### Decision 7: 取消语义维持现有 task 协议，不预先扩展

替代方案：
- (a) 先设计细粒度取消结果，例如 `cancel_ignored`
- (b) 沿用现有 task cancel 语义，等接到真实 Live Coding API 后再看是否需要扩展

选 (b)。当前没有证据表明新增 richer cancel 语义是立即必要的。为了避免过度设计，这次只要求：

- `refresh_cpp_runtime` 作为普通 task 可被 `cancel_task` 命令请求取消；
- 若后续真实接线发现 Live Coding 无法可靠中断，再单独开 change 调整协议。

## Request / Response Contract

### Command Request

当前阶段 `refresh_cpp_runtime` 不接收业务参数，最小请求固定为：

```json
{
  "command": "refresh_cpp_runtime",
  "params": {}
}
```

保留未来扩展空间，但这次不预支 `expected_command_names` 等增强参数。

### Accepted Response

命令入队成功后使用现有 accepted-task 结构：

```json
{
  "ok": true,
  "data": {
    "task_id": "task-7",
    "status": "running"
  }
}
```

### Terminal Success Shape

任务成功时，`get_task_status` 返回：

```json
{
  "ok": true,
  "data": {
    "task_id": "task-7",
    "status": "succeeded",
    "payload": {
      "live_coding": "succeeded",
      "command_registry_reloaded": true
    }
  }
}
```

### Terminal Failure Shape

任务失败时，通过 task `error` 返回结构化信息：

```json
{
  "ok": true,
  "data": {
    "task_id": "task-7",
    "status": "failed",
    "error": {
      "code": "PIE_ACTIVE",
      "message": "Cannot refresh C++ runtime while PIE is running.",
      "details": {
        "resolution": "needs_user_editor_action",
        "suggested_action": "stop_pie_and_retry",
        "required_user_action": "stop_pie"
      }
    }
  }
}
```

## Workflow State Machine

任务内部不新增新的公开 `status` 枚举，但实现上按以下阶段推进：

```text
pending
  -> running
     -> preflight editor state
        -> fail(PIE_ACTIVE)
     -> start live coding
        -> fail(LIVE_CODING_UNAVAILABLE)
        -> fail(LIVE_CODING_COMPILE_FAILED)
        -> fail(LIVE_CODING_TIMEOUT)
        -> success
     -> reload command registry
        -> fail(REGISTRY_RELOAD_FAILED)
        -> succeed
```

外部消费方始终只看现有五态：

- `pending`
- `running`
- `succeeded`
- `failed`
- `cancelled`

需要更细粒度信息时，通过 `payload` / `error.details` 获取。

## CLI / Skill Integration

CLI 侧新增 `system refresh_cpp_runtime` wrapper，约定如下：

- `family = "system"`
- `lifecycle = "core"`
- `cliCommand = "refresh_cpp_runtime"`
- `unrealCommand = "refresh_cpp_runtime"`
- `parameters = []`

Skill 文档随之升级为一条正式流程：

1. 扩充 Unreal C++ 命令后，先调用 `system refresh_cpp_runtime`
2. 根据 task 终态分流：
   - `succeeded`：继续原任务
   - `failed` + `resolution=agent_retry`：修代码并重试
   - `failed` + `resolution=needs_user_editor_action`：提示用户退出 PIE 等轻量操作
   - `failed` + `resolution=needs_user_approval`：请求用户批准后再走重编流程

## Risks / Trade-offs

- Unreal Live Coding 的具体 API/回调入口可能分散在 editor modules 或 delegates 中，接线成本比普通同步命令高。
- 把 registry reload 纳入成功路径会让任务定义更强，但也会把两段失败来源合并到一条命令里；因此必须用独立错误码区分 `LIVE_CODING_*` 与 `REGISTRY_RELOAD_FAILED`。
- 这次把 PIE 视为项目工作流阻塞，意味着与 Epic 官方“PIE 可用”的能力相比更保守，但它换来的是更清晰的用户会话边界。

## Migration Plan

1. 新增 OpenSpec capability `cpp-runtime-refresh`，定义命令、task 和错误分流。
2. 为 `command-registry-introspection` 增加与 `refresh_cpp_runtime` 成功路径联动的要求。
3. Unreal 侧增加：
   - `refresh_cpp_runtime` core 命令
   - 对应 task 类
   - Live Coding 触发与完成/失败观察逻辑
   - PIE 前置检查
4. CLI 侧增加 `system refresh_cpp_runtime` wrapper。
5. Skill 文档更新为“先调正式命令，再按结构化结果决策”。

## Open Questions

- 真实 Unreal Live Coding API 是否能提供稳定、可区分“编译失败”与“环境不可用”的信号源；若不能，可能需要在实现阶段补一层适配。
- 如果后续需要非阻塞轮询式的 Live Coding 结果观测，再单独讨论是否补 `LIVE_CODING_TIMEOUT` 这类错误码。
