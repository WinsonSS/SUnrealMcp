## Why

CLI help 的 JSON 输出是 Agent 发现命令能力的唯一入口。当前每次 `--json` help 调用都重复返回 `globalOptions`（6 项）和 `helpOptions`（1 项），`family-help` 输出包含空的 lifecycle section，每条命令条目冗余回显 `lifecycle` 字段。Agent 在一次任务中可能调用 3 次以上 help（root → family → command），重复的静态数据累积浪费大量 token。

## What Changes

- **BREAKING** `root-help` JSON 输出移除 `globalOptions` 和 `helpOptions` 字段。
- **BREAKING** `family-help` JSON 输出移除 `globalOptions` 和 `helpOptions` 字段；过滤掉空的 lifecycle section；移除每条命令条目中的 `lifecycle` 字段（已由 section key 隐含）。
- **BREAKING** `command-help` JSON 输出移除 `globalOptions` 和 `helpOptions` 字段。
- 文本模式（非 `--json`）help 输出不受影响。

## Capabilities

### New Capabilities
- `slim-help-json`: 定义 `root-help`、`family-help`、`command-help` 三种 JSON help 输出的精简结构规范。

### Modified Capabilities
（无——现有 specs 不涉及 help 输出格式的 requirement。）

## Impact

- **CLI 代码**: `cli/src/index.ts` 的 `printRootHelp`、`printFamilyHelp`、`printCommandHelp` 三个函数的 JSON 分支需要修改输出结构。
- **常量**: `GLOBAL_OPTIONS` 和 `HELP_OPTIONS` 仍保留用于文本模式 help，不删除。
- **消费者兼容性**: Agent prompt / SKILL.md 中对 help JSON 格式的描述需同步更新。依赖 `globalOptions`/`helpOptions` 字段的脚本将 **BREAKING**。
