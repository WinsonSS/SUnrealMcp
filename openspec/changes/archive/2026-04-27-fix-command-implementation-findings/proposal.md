## Why

`review-cli-unreal-commands` 已确认多条 command 存在“返回成功但实际失败”或“返回失败但已经部分修改资产”的问题。这会直接误导 Agent 的下一步决策，也会污染 Blueprint、UMG 和 Enhanced Input 资产，因此需要集中修复建议处理的 findings。

## What Changes

- 修复 `compile_blueprint` 及 Blueprint/UMG 修改类 command 的编译结果处理，避免编译失败时返回 `ok:true`。
- 修复 `add_blueprint_function_node` 与 `bind_widget_event` 的失败路径，避免命令返回 `ok:false` 时留下半成品 node、function graph 或 binding graph 修改。
- 修复 `create_blueprint` 与 `create_umg_widget_blueprint` 的资产存在性检查，覆盖未加载资产与无 `.AssetName` 后缀路径。
- 修复 `add_enhanced_input_mapping` 的重复 mapping 问题，使重复调用成为可预测的 no-op 或返回已存在状态。
- 修复 `spawn_actor` 的 CLI/handler 契约漂移：支持短类名解析，或收窄 CLI 文案到完整 class path。
- 修复 `add_blueprint_variable` 的添加结果确认，避免变量未创建却返回成功。
- 暂不处理低价值 raw-call 鲁棒性问题，例如 `diff_commands` 对非字符串数组元素的严格校验，以及坏 vec/color 数组 fallback。

## Capabilities

### New Capabilities
- `command-implementation-reliability`: 定义 command 实现的成功/失败语义、失败回滚、资产存在性检查、幂等写入和 wrapper/handler 契约一致性要求。

### Modified Capabilities

## Impact

- 主要影响 UnrealPlugin command 实现：
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Stable/Blueprint/**`
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Stable/Node/AddBlueprintFunctionNodeCommand.cpp`
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Stable/UMG/**`
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Stable/Project/AddEnhancedInputMappingCommand.cpp`
  - `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Stable/Editor/SpawnActorCommand.cpp`
  - shared command utils as needed
- 可能影响 CLI wrapper 文案：
  - `Skill/sunrealmcp-unreal-editor-workflow/cli/src/commands/stable/editor_commands.ts`
- 不改变 TCP envelope、request/response protocol version 或 command 名称。
