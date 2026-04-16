## ADDED Requirements

### Requirement: `root-help` JSON 不含 globalOptions 和 helpOptions

`printRootHelp` 的 JSON 输出 SHALL 仅包含 `kind`（值为 `"root-help"`）和 `families` 两个字段。MUST NOT 包含 `globalOptions` 或 `helpOptions` 字段。

#### Scenario: root-help JSON 输出精简

- **WHEN** Agent 执行 `sunrealmcp-cli help --json`
- **THEN** stdout 输出的 JSON 包含 `kind`（`"root-help"`）和 `families` 两个字段
- **AND** 输出中不存在 `globalOptions` 或 `helpOptions` 字段

### Requirement: `family-help` JSON 不含 globalOptions 和 helpOptions

`printFamilyHelp` 的 JSON 输出 SHALL 包含 `kind`（值为 `"family-help"`）、`family`、`description`、`sections` 四个字段。MUST NOT 包含 `globalOptions` 或 `helpOptions` 字段。

#### Scenario: family-help JSON 输出精简

- **WHEN** Agent 执行 `sunrealmcp-cli help <family> --json`
- **THEN** stdout 输出的 JSON 包含 `kind`、`family`、`description`、`sections`
- **AND** 输出中不存在 `globalOptions` 或 `helpOptions` 字段

### Requirement: `family-help` JSON 过滤空 lifecycle section

`printFamilyHelp` 的 JSON 输出中 `sections` 对象 SHALL 仅包含有命令的 lifecycle key。空数组对应的 lifecycle key MUST NOT 出现在 `sections` 中。

#### Scenario: 无 stable 命令时 sections 不含 stable key

- **WHEN** Agent 执行 `sunrealmcp-cli help <family> --json` 且该 family 没有 `stable` lifecycle 的命令
- **THEN** `sections` 对象中不存在 `"stable"` key

#### Scenario: 所有 lifecycle 都有命令时全部保留

- **WHEN** Agent 执行 `sunrealmcp-cli help <family> --json` 且该 family 在 `core`、`stable`、`temporary` 三个 lifecycle 下都有命令
- **THEN** `sections` 对象包含 `"core"`、`"stable"`、`"temporary"` 三个 key

### Requirement: `family-help` 命令条目不含 lifecycle 字段

`printFamilyHelp` 的 JSON 输出中 `sections` 下每条命令条目 SHALL 仅包含 `cliCommand` 和 `description` 两个字段。MUST NOT 包含 `lifecycle` 字段。

#### Scenario: 命令条目无 lifecycle 冗余

- **WHEN** Agent 执行 `sunrealmcp-cli help <family> --json`
- **THEN** `sections` 中每条命令条目只有 `cliCommand` 和 `description` 两个字段
- **AND** 命令条目中不存在 `lifecycle` 字段

### Requirement: `command-help` JSON 不含 globalOptions 和 helpOptions

`printCommandHelp` 的 JSON 输出 SHALL 包含 `kind`（值为 `"command-help"`）、`family`、`command`、`lifecycle`、`description`、`unrealCommand`、`parameters`、`examples` 字段。MUST NOT 包含 `globalOptions` 或 `helpOptions` 字段。

#### Scenario: command-help JSON 输出精简

- **WHEN** Agent 执行 `sunrealmcp-cli help <family> <command> --json`
- **THEN** stdout 输出的 JSON 包含 `kind`、`family`、`command`、`lifecycle`、`description`、`unrealCommand`、`parameters`、`examples`
- **AND** 输出中不存在 `globalOptions` 或 `helpOptions` 字段
