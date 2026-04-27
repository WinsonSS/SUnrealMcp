## Context

本 change 的目标不是新增 command，也不是立即修复已发现的问题，而是把 CLI 与 UnrealPlugin command 实现 review 变成一个可恢复的工作流。review 的核心对象是 TypeScript CLI command wrapper 与 Unreal C++ command handler：它们如何读取参数、调用 Unreal API、处理失败路径、构造响应数据，以及是否会在真实编辑器状态下产生错误行为。

这类 review 容易在一次长会话中膨胀：命令数量多、跨语言、每个 handler 都有不同 Unreal 语义，并且 findings 需要绑定到具体文件与行号。设计上需要把它拆成多个短任务，每个任务都能独立得出“有问题 / 暂无问题 / 需要后续验证”的结论。

## Goals / Non-Goals

**Goals:**

- 建立 CLI command wrapper 与 UnrealPlugin command handler 的逐项实现 review 顺序。
- 每个 review 小任务都记录范围、事实来源、检查项和产出格式，方便下次会话续接。
- 覆盖参数读取、类型校验、缺省值、路径/asset 引用、Unreal API 调用前置条件、对象生命周期、事务/保存/编译副作用、错误处理、返回数据结构等 command 实现风险。
- 只在判断实现正确性需要时核对 command 名称、payload 字段和现有 specs。
- 明确 review 阶段不得修改应用代码；发现问题后以 findings 或后续 change 承接。

**Non-Goals:**

- 不在本 change 中实现 bugfix、重构或新增 command。
- 不运行需要真实 Unreal Editor 状态的 destructive 操作。
- 不系统审查 CLI help、transport、parser、`dist/**` 同步、通用 registry 机制；除非它们直接影响某个 command 实现结论。
- 不替代后续实现 change 的测试计划。

## Decisions

1. 使用 OpenSpec change 作为 review 载体。

   选择 OpenSpec 而不是临时 markdown 笔记，是因为用户希望跨会话续接；OpenSpec 的 `tasks.md` 能提供稳定的任务编号和完成状态。没有选择直接继续当前聊天 review，是因为 command 面横跨多组文件，长会话容易让上下文和 findings 混在一起。

2. 先建立最小命令索引，再做 family-by-family 深查。

   review 应先生成足够定位的 CLI wrapper 与 Unreal handler 索引，然后按 `system`、`editor`、`blueprint`、`node`、`umg`、`project` 分段阅读实现。没有选择先系统检查 help/transport/registry，是因为用户当前最关心的是 command 里的实现 bug，外围机制会稀释注意力。

3. 事实来源按可信度分层。

   优先级为：Unreal C++ command handler 与 shared command utils > CLI `src/commands/**` wrapper > 现有 `openspec/specs/**` 规范 > 必要的 MCP runtime helper。没有把 CLI help JSON 或 `dist/**` 放在主路径，是因为它们更适合做包装/发布检查，而不是判断 handler 业务逻辑是否正确。

4. findings 与 no-findings 都要记录。

   每个任务完成时必须写出检查结论。没有发现问题时也要写“暂无 findings”，否则下次会话无法判断该范围是否已经看过。

5. review 阶段只读。

   本 change 的 apply 阶段可以运行命令、读取文件、生成报告，但不得改 CLI 或 UnrealPlugin 源码。没有选择边看边修，是因为用户明确想先分任务 review，并用 proposal 防止中途 token 不足；修复应在 findings 明确后另行确认范围。

## Risks / Trade-offs

- [Risk] 静态 review 可能漏掉只有真实 Unreal Editor 会话才暴露的问题。→ Mitigation：把运行态验证列为独立可选任务，并要求标注“需要 Unreal Editor 验证”的 residual risk。
- [Risk] `rg` 在当前 Codex desktop 环境可能被 WindowsApps 权限拦截。→ Mitigation：允许使用 PowerShell `Get-ChildItem` / `Select-String` 作为 fallback。
- [Risk] 过度检查外围框架会吞掉 command 实现 review 的上下文预算。→ Mitigation：tasks 只保留必要索引与 family-by-family command 实现检查，外围问题仅在影响具体 command 时记录。
- [Risk] review 输出过长。→ Mitigation：每个任务只输出该任务 findings，并维护简短总表；详细证据用文件路径与行号引用。

## Migration Plan

不需要迁移。该 change 只增加 review 计划与规范，不改变运行时代码或协议。

## Open Questions

- 后续 review findings 是否要直接转成一个修复型 OpenSpec change，还是先汇总给用户人工筛选优先级？
- 是否有可用的真实 Unreal Editor 测试工程用于运行 `system diff_commands`、`system ping` 与非破坏性 command 验证？
