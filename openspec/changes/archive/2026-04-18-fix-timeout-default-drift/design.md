## Context

当前 `sunrealmcp-cli` 的默认 `timeout_ms` 在仓库内存在明确漂移：`Skill/sunrealmcp-unreal-editor-workflow/cli/src/runtime/discovery.ts` 中的 `DEFAULT_TIMEOUT_MS` 为 `5000`，`cli/src/index.ts` 也使用同一个常量向 help 输出默认值；但 `Skill/sunrealmcp-unreal-editor-workflow/SKILL.md` 与 `SKILL_cn.md` 手写了 `30000`。

这类漂移对普通 CLI 项目只是文档不一致；但对当前仓库而言，`Skill` 本身就是 Agent 的工作流输入之一，因此文档漂移会直接变成 Agent 的错误背景。更进一步，如果 Skill 文档继续携带具体数字，那么每次 CLI 调整默认值都会带来额外同步面。

## Goals / Non-Goals

**Goals:**

- 为默认 `timeout_ms` 明确一个仓库内可追踪的基准值。
- 消除运行时实现、CLI help 与 Skill 文档之间的默认值漂移。
- 收敛默认值真相源，让后续维护者在修改默认 timeout 时尽量只改运行时与 help。
- 将本次 change 收敛为“修复默认值漂移”，避免顺带扩散到 transport 行为重设计。

**Non-Goals:**

- 不重做 `unreal_client` 的连接超时与接收超时模型。
- 不引入自动重试、命令级 timeout 配置或 family 级 timeout 配置。
- 不修改 Unreal 插件侧的协议字段、任务模型或错误结构。
- 不把所有文档都改为运行时动态生成。

## Decisions

### 1. 以 `30000` 作为 CLI 默认值基准

本次设计选择把 CLI 默认 `timeout_ms` 的基准值统一收敛为 `30000`，并在 apply 阶段同步运行时代码与 CLI help。

原因：

- 用户已经明确决定把默认值目标收敛到 `30000`；
- 当前 `5000` 不是文档约定值，也不是用户期望值，因此需要显式调整；
- 把默认值目标先固定下来，才能进一步讨论哪些文档应当暴露这个数字、哪些不应暴露。

为什么不选“继续保留 5000，只改文档”：

- 这与用户刚刚明确给出的目标相违背；
- 继续保留 `5000` 会让 Agent 工作流文档和实际默认行为之间长期背离；
- 即使文档暂时改回 `5000`，后续仍可能因为预期不一致再度被改回 `30000`，不如这次直接明确决策。

### 2. 把“单一事实来源”落实为代码常量 + help 输出，Skill 不再硬编码数值

本次不新增新的配置文件或生成步骤，而是把 `cli/src/runtime/discovery.ts` 中的 `DEFAULT_TIMEOUT_MS` 作为运行时单点，把它调整到 `30000`，并要求 `cli/src/index.ts` 的 help 默认值与之保持一致。`SKILL.md` / `SKILL_cn.md` 不再写死具体默认数字，而是把 CLI help 作为默认值真相源。

原因：

- 运行时代码天然承载真实默认值；
- `cli/src/index.ts` 已直接消费该常量，help 本身不需要额外设计；
- Skill 的职责更偏工作流决策，而不是维护运行时常量镜像；
- 去掉 Skill 里的硬编码数值后，未来默认值调整的同步面更小。

为什么不选“新增独立 metadata 文件给 docs 和 runtime 共用”：

- 对当前变更来说过重；
- 会引入新的维护面与读取链路；
- 不能从根本上避免人类在 Skill 文档里手写错误，仍然需要规范约束。

为什么不选“继续要求 Skill 文档也写 `30000`”：

- 这会让 Skill 再次变成一个默认值镜像；
- 未来 CLI 若改默认值，Skill 依然需要人工同步；
- Agent 真正需要的是“默认值去哪里看”和“什么时候该显式覆盖”，而不是重复记住一个常量。

### 3. 用独立 capability spec 同时记录默认值与文档职责边界

本次使用新的 `cli-timeout-default` capability spec，记录默认值为 `30000`，并明确 CLI runtime / help 持有具体数值，而 Skill 文档不再硬编码默认数字。

原因：

- 当前现有 spec 没有覆盖默认 timeout 的行为；
- 这是一个 Agent 工作流级别的约束，而不只是实现细节；
- 单独 capability 有利于后续若再次调整默认值时能先改 spec，再改实现。

为什么不选“只改 proposal / design，不补 spec”：

- 那样 apply 之后仍缺少可归档的规范性描述；
- 后续再次漂移时，Agent 无法通过 spec 恢复这个约束。

## Risks / Trade-offs

- [未来证明 `30000` 过长或过短] → 本次 spec 只固定“当前默认值与同步要求”，后续若要调到别的值，应走新的 change 修改 spec。
- [Skill 文档仍然可能描述不清 timeout 语义] → 在 Skill 中保留“何时覆盖 timeout、默认值以 CLI help 为准”的流程说明，但移除具体数字。
- [有人误以为本次 change 解决了所有 timeout 问题] → 在 proposal、design 与 tasks 中反复声明非目标，避免范围蔓延。

## Migration Plan

1. 补充 `cli-timeout-default` spec，定义默认值与同步要求。
2. 在 apply 阶段将 `cli/src/runtime/discovery.ts` 中的 `DEFAULT_TIMEOUT_MS` 调整为 `30000`。
3. 检查 `cli/src/index.ts` 的 help 默认值随之显示为 `30000`。
4. 修改 `SKILL.md` / `SKILL_cn.md` 中与 `timeout_ms` 相关的说明，去掉硬编码默认数字，并指向 CLI help 作为默认值来源。
5. 用 CLI help 验证输出默认值为 `30000`，并检查 Skill 文档不再携带过期数字。

本次变更不涉及数据迁移。若 apply 后发现设计判断有误，回滚策略是撤销默认值调整与文档/spec 变更，并重新提出“调整默认 timeout 行为”的独立 change。

## Open Questions

- 当前是否还需要把 `README.md` 中与默认 `timeout_ms` 相关的说明一并纳入同步范围；本次 change 默认只覆盖 Skill 与 CLI 主链路。
