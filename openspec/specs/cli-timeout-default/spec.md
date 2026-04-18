# cli-timeout-default Specification

## Purpose
TBD - created by archiving change fix-timeout-default-drift. Update Purpose after archive.
## Requirements
### Requirement: CLI 默认 `timeout_ms` 为 30000

当调用方未显式传入 `--timeout_ms` 时，`sunrealmcp-cli` SHALL 使用 `30000` 作为目标连接与响应等待的默认 timeout，并在解析后的 `CliGlobalOptions.timeoutMs` 与最终 `CliTarget.timeoutMs` 中体现该值。

#### Scenario: 未显式传入 timeout 时使用默认值

- **WHEN** 调用方执行任意 `sunrealmcp-cli <family> <command>` 且未传入 `--timeout_ms`
- **THEN** CLI 内部使用的默认 `timeout_ms` 为 `30000`
- **AND** 由该默认值解析出的 `CliTarget.timeoutMs` 也为 `30000`

#### Scenario: 显式传入 timeout 时覆盖默认值

- **WHEN** 调用方执行命令并传入 `--timeout_ms 12000`
- **THEN** CLI MUST 使用 `12000` 作为本次调用的 timeout
- **AND** 本次调用的 `CliTarget.timeoutMs` 不再是默认值 `30000`

### Requirement: CLI help 暴露的默认 `timeout_ms` 必须与运行时一致

CLI help 中展示给 Agent 和人类调用者的全局选项默认值 SHALL 与运行时真实默认值一致。对于 `--timeout_ms`，help 输出中的默认值 MUST 为 `30000`，不得继续暴露 `5000` 或其他与运行时不一致的数字。

#### Scenario: 根 help 显示正确默认值

- **WHEN** 调用方执行 `sunrealmcp-cli help`
- **THEN** help 中 `--timeout_ms` 的默认值显示为 `30000`

#### Scenario: 命令 help 显示正确默认值

- **WHEN** 调用方执行 `sunrealmcp-cli help system ping`
- **THEN** help 中列出的全局选项里 `--timeout_ms` 的默认值显示为 `30000`

### Requirement: Skill 文档不得硬编码 `timeout_ms` 默认数字

`Skill/sunrealmcp-unreal-editor-workflow/SKILL.md` 与 `Skill/sunrealmcp-unreal-editor-workflow/SKILL_cn.md` SHALL NOT 硬编码 `--timeout_ms` 的具体默认数字。Skill 文档如果需要解释该选项，MUST 将 CLI help 或 CLI 运行时行为视为默认值真相源，并优先描述“何时需要显式覆盖 timeout”这类工作流信息。

#### Scenario: 中文 Skill 文档不再写死默认值

- **WHEN** Agent 或维护者读取 `Skill/sunrealmcp-unreal-editor-workflow/SKILL_cn.md` 中 `--timeout_ms` 的说明
- **THEN** 文档中不存在把 `--timeout_ms` 默认值写成 `30000` 或 `5000` 的硬编码说明
- **AND** 文档将默认值来源指向 CLI help、CLI 运行时或等价的单一事实来源

#### Scenario: 英文 Skill 文档不再写死默认值

- **WHEN** Agent 或维护者读取 `Skill/sunrealmcp-unreal-editor-workflow/SKILL.md` 中 `--timeout_ms` 的说明
- **THEN** 文档中不存在把 `--timeout_ms` 默认值写成 `30000` 或 `5000` 的硬编码说明
- **AND** 文档将默认值来源指向 CLI help、CLI 运行时或等价的单一事实来源

