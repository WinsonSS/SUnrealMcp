## Why

当前 `sunrealmcp-unreal-editor-workflow` 的文档已经明确要求：扩充 Unreal C++ 命令后，Agent 应先尝试 Live Coding，让改动在不打断编辑器会话的前提下生效；只有 Live Coding 不适合当前环境时，才升级到完整重编。

但仓库现状里还缺少一条由 Agent 可调用的“让 C++ 改动生效”的正式能力：

- 插件侧只有 `reload_command_registry`，它只能在代码已经进入进程之后刷新命令表，不能触发 Live Coding。
- Agent 因此无法按 Skill 规定“先 Live Coding，再继续原任务”，只能停在“让用户手动编译”这一步。
- 这会把本来属于 Agent 的工作过早转嫁给用户，尤其是在只是普通代码/构建错误、重新修代码后再次 Live Coding 就能解决的场景里。

同时，Live Coding 失败并不只有一种含义：

- **代码/构建错误**：应由 Agent 继续修代码并重试，不应立即要求用户手动编译。
- **轻量编辑器状态阻塞**：例如用户正在 PIE / 运行游戏，此时应默认失败并提示用户先停止 PIE，再重试。
- **重量环境阻塞**：例如当前环境无法执行 Live Coding、必须关闭编辑器后完整重编。这类路径可能影响用户未保存内容，必须先征得用户允许，不能由 Agent 自动执行。

因此需要的不是一个单纯“按一下 Live Coding”的底层按钮，而是一条**工作流能力**：尝试让 Unreal C++ 改动生效，并把失败原因结构化地分流给 Agent。

## What Changes

- 新增一条 Unreal core workflow 命令，用于启动“刷新 C++ 运行态”的异步任务。暂定命令名为 `refresh_cpp_runtime`。
- 该命令不是同步 RPC，而是复用现有 `TaskRegistry` 机制，返回 `task_id`，由 `get_task_status` 轮询结果。
- 任务的最小成功路径为：
  1. 检查当前编辑器状态是否允许执行 Live Coding；
  2. 触发 Live Coding；
  3. 等待 Live Coding 完成；
  4. 若成功，则自动执行 `reload_command_registry`；
  5. 整体成功后返回 `succeeded`。
- 任务失败时不只返回 `failed`，而是返回结构化错误，至少区分三类：
  - **Agent 可继续处理**：例如代码/构建错误，Agent 应修代码并重试；
  - **轻量环境阻塞**：例如 `PIE_ACTIVE`，用户先停止 PIE，再重试；
  - **需要用户授权的重路径**：例如当前环境无法 Live Coding，需要关闭编辑器并完整重编；任务应明确告诉 Agent 这一步不能自动执行。
- CLI 侧在 `system` family 下注册对应 wrapper，使 Skill 可以正式把“扩命令后优先 Live Coding”落到命令链路里。
- Skill 文档同步更新，把“能 Live Coding 就优先 Live Coding”的原则描述收敛成正式工作流：先调用 `refresh_cpp_runtime`，再根据结构化结果决定是继续修代码重试、提示用户退出 PIE，还是请求用户允许进入完整重编路径。

## Capabilities

### New Capabilities
- `cpp-runtime-refresh`: 定义一条基于 task 的工作流命令，用于尝试通过 Live Coding 让 Unreal C++ 改动生效，并在成功后自动刷新命令表；失败时以结构化方式告诉 Agent 下一步应继续修代码、提示用户退出 PIE，还是请求用户允许进入完整重编流程。

### Modified Capabilities
- `command-registry-introspection`: `refresh_cpp_runtime` 成功后会自动触发 `reload_command_registry`，因此“新命令能否被 `check_command_exists` / `diff_commands` 观测到”将与这条工作流能力联动。

## Impact

- Unreal 代码：
  - 新增一条 core workflow 命令与对应 task 实现；
  - 接入 Unreal 编辑器的 Live Coding 触发/状态监听入口；
  - 在任务成功路径中调用现有的 `reload_command_registry` 能力；
  - 为任务失败补充结构化错误分类，尤其覆盖：
    - 代码/构建错误
    - PIE 运行中
    - 当前环境无法 Live Coding
- CLI / Skill：
  - `Skill/sunrealmcp-unreal-editor-workflow/cli/src/commands/core/system_commands.ts` 增加 `refresh_cpp_runtime` wrapper；
  - `Skill/sunrealmcp-unreal-editor-workflow/SKILL.md` 与 `SKILL_cn.md` 一起更新；
  - Skill 文档从“原则上优先 Live Coding”升级为“先调用正式命令，按结构化结果决定下一步”。
- 用户体验：
  - Agent 不再在普通编译错误上过早要求用户手动编译；
  - 当用户正在 PIE 时，系统会先提示“停止游戏后重试”，而不是直接升级为重编；
  - 只有当流程确实需要关闭编辑器 / 完整重编时，才要求用户授权。
