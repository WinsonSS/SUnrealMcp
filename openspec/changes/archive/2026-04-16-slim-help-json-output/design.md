## Context

当前 CLI help 有三个层级的 JSON 输出（`printRootHelp`、`printFamilyHelp`、`printCommandHelp`），每个都在输出中包含完整的 `globalOptions`（6 项）和 `helpOptions`（1 项）。这些静态数据在 Agent 的一次任务流中（root → family → command）至少重复 3 次。

此外 `family-help` 的 `sections` 对象固定包含所有 lifecycle key（`core`、`stable`、`temporary`），即使某些 section 为空数组也会输出；每条命令条目都带 `lifecycle` 字段，但该信息已由 section key 隐含。

变更范围仅限 CLI 侧 `cli/src/index.ts` 中三个 `print*Help` 函数的 JSON 分支，文本模式不受影响，Unreal Plugin 侧无改动。

## Goals / Non-Goals

**Goals:**

- 从三个 JSON help 输出中移除 `globalOptions` 和 `helpOptions`，消除每次调用的静态重复。
- `family-help` 的 `sections` 过滤掉空 lifecycle section，减少空数组噪音。
- `family-help` 中每条命令条目移除 `lifecycle` 字段（section key 已提供该信息）。

**Non-Goals:**

- 不改变文本模式的 help 输出格式。
- 不改变 help 的调用方式或路由逻辑（`handleHelp`）。
- 不引入新的 help 子命令（如 `help options`）来单独查询 globalOptions。Agent 可以在 SKILL.md 中获取这些信息。

## Decisions

### D1: 直接移除 `globalOptions` / `helpOptions`，不提供替代查询入口

Agent 需要知道有哪些全局选项（如 `--project`、`--verbose`、`--pretty`），但这些信息是静态的，完全可以写入 SKILL.md。不为此新增 `help options --json` 子命令。

**为什么不选新增子命令**：`globalOptions` 只在 Agent 首次接触 CLI 时有价值，之后 SKILL.md 就已提供。新增子命令增加代码和维护成本，收益不对等。

### D2: 过滤空 section 而非保留全部 lifecycle key

`family-help` 的 `sections` 只输出有命令的 lifecycle key。Agent 无需知道"没有 stable 命令"这件事。

**为什么不保留空 key**：空数组不提供任何信号，只浪费 token。Agent 遍历 sections 时检查 key 存在性比检查数组长度更自然。

### D3: 移除命令条目中的 `lifecycle` 字段

每条命令已经按 lifecycle 分组在 section 下面，`lifecycle` 字段是 100% 冗余的。

**为什么不保留**：在按 section 分组的结构里，每条命令重复自己所在的 section 名是纯粹的信息冗余。

## Risks / Trade-offs

- **BREAKING 变更** → 依赖 `globalOptions`/`helpOptions` 字段的脚本会失效。缓解：当前唯一消费者是 Agent prompt；同步更新 SKILL.md。
- **Agent 不再能动态获知全局选项** → Agent 必须依赖 SKILL.md 中的静态描述。缓解：全局选项变更频率极低，SKILL.md 同步维护即可。
