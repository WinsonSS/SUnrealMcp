## ADDED Requirements

### Requirement: Review 必须可跨会话续接

CLI 与 UnrealPlugin command 实现 review SHALL 被拆分为带编号的小任务。每个任务 SHALL 明确 review 的 command family、涉及的 CLI wrapper、涉及的 Unreal handler、检查项和输出结论。完成任一任务后，review 记录 MUST 足以让下一次会话不重新阅读全部上下文即可继续。

#### Scenario: 中途停止后继续 review

- **WHEN** reviewer 在完成某个任务后中断会话
- **THEN** 下一次会话可以从 `tasks.md` 中找到已完成任务与下一个待执行任务
- **AND** 已完成任务包含 findings 或“暂无 findings”的明确结论

### Requirement: Review 必须聚焦 command 实现

review SHALL 聚焦 `Skill/sunrealmcp-unreal-editor-workflow/cli/src/commands/**` 中的 command wrapper 与 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/**` 中的 command handler。review SHALL 检查参数读取、类型/空值校验、Unreal 对象查找、API 调用前置条件、错误路径、成功响应数据和副作用是否正确。注册、help、transport、`dist/**` 同步等外围机制 MUST NOT 成为主检查范围，除非它们直接影响某个 command 实现判断。

#### Scenario: 发现 handler 业务逻辑错误

- **WHEN** Unreal handler 在参数合法时仍可能调用错误对象、遗漏必要前置条件或返回错误数据
- **THEN** review 输出 SHALL 记录该实现问题为 finding
- **AND** finding SHALL 指向对应 Unreal handler 文件与相关 CLI wrapper 文件

### Requirement: Review 必须核对 CLI wrapper 与 Unreal handler 的实现契约

review SHALL 对每个 command 核对 CLI 参数定义、`mapParams`、Unreal `Request.Params` 读取字段、成功响应 `data` 字段、错误响应 `error.code` / `error.message` 是否能共同支撑该 command 的预期实现。payload 字段命名只作为实现契约的一部分检查，不扩展成全局协议 review。

#### Scenario: CLI 参数名与 Unreal 读取字段不一致

- **WHEN** CLI command 注册参数 `task_id`，但 Unreal handler 读取 `taskId`
- **THEN** review 输出 SHALL 记录该不一致为 finding
- **AND** finding SHALL 标明违反的现有 spec 名称或字段命名约定

### Requirement: Review 阶段不得修改运行时代码

执行本 review change 时，reviewer SHALL NOT 修改 `Skill/sunrealmcp-unreal-editor-workflow/cli/**` 或 `UnrealPlugin/SUnrealMcp/**` 的运行时代码。若发现需要修复的问题，review SHALL 记录 finding，并由后续用户确认是否创建修复 change 或进入实现阶段。

#### Scenario: 发现明确 bug

- **WHEN** reviewer 发现一个可以直接修复的 command bug
- **THEN** reviewer SHALL 只记录 bug、影响范围、证据和建议修复方向
- **AND** reviewer SHALL NOT 在本 review change 中直接编辑运行时代码

### Requirement: Review 输出必须区分 finding、验证缺口和后续建议

每个 review 任务的输出 SHALL 将确定性 finding、需要真实 Unreal Editor 验证的缺口、以及可选改进建议分开记录。确定性 finding MUST 包含文件路径和尽可能精确的行号；验证缺口 MUST 说明缺少的运行条件。

#### Scenario: 静态检查无法确认运行态行为

- **WHEN** 某个 command 的行为依赖真实 Unreal Editor 状态，且当前没有可用 Editor 会话
- **THEN** review 输出 SHALL 将其记录为验证缺口
- **AND** 不得把该缺口伪装成确定性 bug
