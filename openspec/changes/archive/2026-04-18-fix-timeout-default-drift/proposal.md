## Why

当前 `sunrealmcp-cli` 的默认 `timeout_ms` 在仓库内缺少稳定的单一事实来源：CLI 运行时代码与 help 输出实际使用 `5000`，而 `SKILL.md` / `SKILL_cn.md` 又手写了 `30000`。一旦 Skill 文档携带具体数值，后续只要 CLI 改一次默认值，就很容易再次分叉。

## What Changes

- 为 CLI 默认 `timeout_ms` 建立仓库内明确的单一事实来源，将默认值从 `5000` 调整并收敛到 `30000`。
- 补充一份描述 CLI 默认 timeout 行为与文档边界的 spec，避免后续再次出现实现和 Agent 指令漂移。
- 调整 Skill 文档，使其不再硬编码 `timeout_ms` 的具体默认数字，而是把 CLI help 作为默认值真相源。
- 明确这是“修正默认值漂移”的 change，而不是重做 socket timeout 策略、重试策略或长耗时命令模型。

## Capabilities

### New Capabilities
- `cli-timeout-default`: 定义 `sunrealmcp-cli` 默认 `timeout_ms` 的基准值，以及运行时、help 与 Skill 文档之间的职责边界。

### Modified Capabilities

## Impact

- `Skill/sunrealmcp-unreal-editor-workflow/cli/src/runtime/discovery.ts`
- `Skill/sunrealmcp-unreal-editor-workflow/cli/src/index.ts`
- `Skill/sunrealmcp-unreal-editor-workflow/SKILL.md`
- `Skill/sunrealmcp-unreal-editor-workflow/SKILL_cn.md`
- 后续 Agent 对默认 `timeout_ms` 的背景恢复与调试判断
