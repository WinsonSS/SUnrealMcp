## MODIFIED Requirements

### Requirement: Wire 参数字段命名约定

请求 `params` 对象和响应 `data` 对象内部的字段名 SHALL 采用 `snake_case`（仅包含小写 ASCII 字母、数字和下划线；不以数字开头；不出现连续下划线）。该约定 SHALL 同时适用于 CLI 侧的参数注册（`CliParameterDefinition.name`）以及 Unreal 侧命令 handler 读取 `Request.Params` 字段或拼装 `data` 字段时使用的 key。

**例外 1（envelope）**：由 `ProtocolVersion = 1` 定义的 envelope 字段——具体为 `protocolVersion`、`requestId`、`ok`、`data`、`error`、`error.code`、`error.message`、`error.details`——不在本规则范围内，继续保持现有的 camelCase 形式。**补充**：CLI 侧输出信封的顶层字段 `target`、`cli`、`unreal`、`raw` 仅在 `--verbose` 模式下出现，同样属于 envelope 层级，不受 snake_case 约束。

**例外 2（task payload）**：长耗时任务通过 `ISUnrealMcpTask::BuildPayload` 返回的 JSON 内容（在响应中位于 `data.payload`）属于具体 task 实现的私有命名空间，不受本规则强制约束。Framework 拼装的 `data.payload` 这一 key 名本身仍然必须是 snake_case，但 payload 对象**内部**的字段命名由 task 作者自决。

#### Scenario: 新 core 命令按 snake_case 注册参数

- **WHEN** 新增一个 Unreal 侧 core 命令并在 `Skill/sunrealmcp-unreal-editor-workflow/cli/src/commands/core/` 下编写对应的 CLI 注册
- **THEN** CLI 通过 `CliParameterDefinition.name` 注册的每个参数名都使用 snake_case 书写
- **AND** Unreal 侧 handler 从 `Request.Params` 读取该字段时使用完全一致的字面字符串，不做任何大小写转换
- **AND** CLI 侧不因"仅为了改大小写"而引入 `mapParams` 回调

#### Scenario: 发现非 snake_case 字段时拒绝合入

- **WHEN** 审阅者在某个 core 命令里发现 `params` 字段使用了 camelCase、PascalCase 或其它非 snake_case 形式
- **THEN** 该字段被视为规范违规，合入前必须 rename 为 snake_case
