# slim-envelope Specification

## Purpose
定义 CLI 命令输出的精简信封格式与 `--verbose` 扩展信封格式。
## Requirements
### Requirement: 默认精简信封格式

CLI `executeCommand` 的 stdout JSON 输出在默认模式下 SHALL 仅包含顶层字段 `ok` 和 `data`（成功时）或 `ok` 和 `error`（失败时）。MUST NOT 在默认模式下输出 `target`、`cli`、`unreal`、`raw` 字段。

#### Scenario: 成功响应仅含 ok 和 data

- **WHEN** Agent 执行任意命令且 Unreal 侧返回 `ok:true`
- **THEN** CLI stdout 输出的 JSON 仅包含顶层字段 `ok`（值为 `true`）和 `data`
- **AND** 输出中不存在 `target`、`cli`、`unreal`、`raw` 字段

#### Scenario: 失败响应仅含 ok 和 error

- **WHEN** Agent 执行任意命令且 Unreal 侧返回 `ok:false`
- **THEN** CLI stdout 输出的 JSON 仅包含顶层字段 `ok`（值为 `false`）和 `error`
- **AND** `error` 对象保持现有结构（`code`、`message`、可选 `details`）
- **AND** 输出中不存在 `target`、`cli`、`unreal`、`raw`、`data` 字段

### Requirement: `--verbose` 全局选项恢复完整信封

CLI SHALL 支持 `--verbose` 全局选项。当 `--verbose` 被指定时，CLI stdout 输出 SHALL 在精简信封基础上追加 `target`、`cli`、`unreal`、`raw` 四个诊断字段，其结构与变更前的默认信封完全一致。

#### Scenario: verbose 模式下成功响应包含全部字段

- **WHEN** Agent 执行 `sunrealmcp-cli <family> <command> --verbose` 且 Unreal 侧返回 `ok:true`
- **THEN** CLI stdout 输出的 JSON 包含 `ok`、`data`、`target`、`cli`、`unreal`、`raw` 六个顶层字段
- **AND** `target` 包含 `projectRoot`、`pluginPath`、`host`、`port`、`configSources`、`timeoutMs`
- **AND** `cli` 包含 `family`、`command`、`lifecycle`
- **AND** `unreal` 包含 `command`、`requestId`、`params`
- **AND** `raw` 为 Unreal TCP 响应的原始 JSON 对象

#### Scenario: verbose 模式下失败响应包含全部字段

- **WHEN** Agent 执行 `sunrealmcp-cli <family> <command> --verbose` 且 Unreal 侧返回 `ok:false`
- **THEN** CLI stdout 输出的 JSON 包含 `ok`、`error`、`target`、`cli`、`unreal`、`raw` 六个顶层字段

### Requirement: `CliGlobalOptions` 声明 `verbose` 字段

`cli/src/types.ts` 中的 `CliGlobalOptions` 接口 SHALL 包含 `verbose: boolean` 字段，默认值为 `false`。`parseOptions` SHALL 在遇到 `--verbose` 标志时将其设为 `true`。

#### Scenario: 未指定 --verbose 时默认为 false

- **WHEN** Agent 执行命令且未传入 `--verbose`
- **THEN** `CliGlobalOptions.verbose` 为 `false`

#### Scenario: 指定 --verbose 时为 true

- **WHEN** Agent 执行命令且传入 `--verbose`
- **THEN** `CliGlobalOptions.verbose` 为 `true`
