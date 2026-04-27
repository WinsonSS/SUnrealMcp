## Why

CLI command wrapper 与 UnrealPlugin command handler 是本仓库真正执行 Unreal 自动化能力的地方。当前命令数量已经覆盖 `system`、`editor`、`blueprint`、`node`、`umg`、`project` 等多个 family，需要一次可续接、可分段的实现 review，重点验证每个 command 的参数读取、业务逻辑、错误处理和返回数据是否有 bug。

## What Changes

- 新增一个结构化 review change，用于逐项检查 CLI command wrapper 与 UnrealPlugin command handler 的实现是否存在 bug。
- 将 review 拆成可独立执行的小任务，每个任务产出明确 findings / no findings 结论，方便下次会话从任意任务继续。
- 约定 review 时优先阅读 command 源码本身，并用现有 OpenSpec 规范作为判断实现是否正确的约束。
- 仅在验证 command 实现需要时检查注册、协议、help、transport 等外围代码；这些不是本次 review 主范围。
- 不直接修改 CLI 或 UnrealPlugin 代码；若 review 发现需要修复的问题，再另开实现 change 或在用户确认后进入 apply 阶段。

## Capabilities

### New Capabilities
- `command-surface-review`: 定义 CLI 与 UnrealPlugin command 实现的分段 review 范围、记录方式、验收口径和续接要求。

### Modified Capabilities

## Impact

- 影响 review 范围：
  - `Skill/sunrealmcp-unreal-editor-workflow/cli/src/commands/**`
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/**`
- 必要时参考但不主动展开的外围范围：
  - `Skill/sunrealmcp-unreal-editor-workflow/cli/src/index.ts`
  - `Skill/sunrealmcp-unreal-editor-workflow/cli/src/protocol.ts`
  - `Skill/sunrealmcp-unreal-editor-workflow/cli/src/transport/**`
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Mcp/**`
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Public/Mcp/**`
- 参考现有规范：
  - `core-command-wire-protocol`
  - `command-registry-introspection`
  - `slim-envelope`
  - `slim-core-responses`
  - `large-response-error`
  - `cli-timeout-default`
  - `cpp-runtime-refresh`
- 不新增运行时依赖，不改变 wire protocol，不改变默认 CLI 输出。
