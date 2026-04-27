## Context

`review-cli-unreal-commands` 发现的问题集中在 command 的可靠性边界：部分 command 会在失败时留下资产修改，部分 command 会在实际失败时返回成功，还有部分 command 在重复调用或资产已存在时行为不稳定。由于本项目主要消费者是 Agent，command 的成功/失败语义必须可信，否则 Agent 会在错误状态上继续构建后续步骤。

本 change 聚焦建议处理的问题，不覆盖低价值 raw-call 鲁棒性问题。修复目标是让稳定 command 的行为更接近事务式语义：成功才表示目标变更已经可靠完成，失败不能留下明显半成品状态。

## Goals / Non-Goals

**Goals:**

- `compile_blueprint` 和修改后触发 compile 的 Blueprint/UMG command 能识别编译失败并返回结构化错误。
- `add_blueprint_function_node` 与 `bind_widget_event` 在中途失败时不会留下新创建的半成品 graph/node。
- `create_blueprint` 与 `create_umg_widget_blueprint` 能可靠识别目标资产已存在，包括未加载资产与无 `.AssetName` 后缀路径。
- `add_enhanced_input_mapping` 对相同 action/key 重复调用时幂等。
- `spawn_actor` 的 CLI/handler 契约一致。
- `add_blueprint_variable` 只有确认变量实际添加后才返回成功。

**Non-Goals:**

- 不修复 `diff_commands` 非字符串数组元素校验。
- 不系统改变 raw-call 坏数组 fallback 行为。
- 不引入新的 command 名称或 protocol version。
- 不构建完整事务框架；只在本轮涉及的 command 中做局部回滚/预校验。

## Decisions

1. 引入小型 shared helper，而不是每个 command 手写一套。

   Blueprint/UMG 编译检查、资产路径规范化与存在性检查会在多个 command 中使用。优先放在现有 shared utils 中，例如 `BlueprintCommandUtils.h` 与 `UMGCommandUtils.h`。没有选择新增独立 runtime 模块，是因为当前范围小，现有 command utils 已经承载同类辅助逻辑。

2. 编译失败返回 command error，不再以 `data.status = "compiled"` 掩盖失败。

   修改类 command 在 `FKismetEditorUtilities::CompileBlueprint` 后需要检查 Blueprint 状态。失败时返回结构化错误，例如 `BLUEPRINT_COMPILE_FAILED` 或 `WIDGET_COMPILE_FAILED`。没有选择仅在 data 中加 warning，是因为 Agent 默认以 `ok` 驱动流程，warning 容易被忽略。

3. 失败回滚优先采用预校验；无法预校验时记录并清理新建对象。

   `add_blueprint_function_node` 可以先根据函数 pins 预校验 `params`，再把 node 加入 graph。`bind_widget_event` 更复杂，可能需要记录本次新建的 event node、function graph、call node，在后续失败时移除。没有选择完全依赖 Unreal undo transaction，是因为 command 是远程自动化调用，失败时应由 handler 自己恢复显式状态。

4. 资产存在性以规范化 object path + Asset Registry / load 检查为准。

   对 `/Game/Foo/BP_Test` 与 `/Game/Foo/BP_Test.BP_Test` 应推导到同一个 package/object 目标。没有继续使用 `FindObject` 作为唯一检查，是因为它只覆盖已加载对象。

5. `add_enhanced_input_mapping` 重复调用返回成功 no-op。

   对 Agent 来说，幂等成功比报错更适合重试语义。返回 data 可包含 `already_existed` 表示没有新增。没有选择每次先删除再添加，是因为这可能改变 mapping 顺序或丢失扩展设置。

6. `spawn_actor` 优先支持短类名解析。

   现有 CLI 文案已经承诺 `StaticMeshActor` 这类短名，且 Blueprint component/parent class resolver 已经支持短名风格。没有选择只改 CLI 文案，是因为支持短名能减少 Agent 调用失败，并保持接口更易用。

## Risks / Trade-offs

- [Risk] Blueprint/UMG 编译状态在不同 UE 版本中的状态枚举或日志行为可能不同。→ Mitigation：封装为 helper，并在真实 Editor 中用成功/失败 Blueprint 验证。
- [Risk] graph/node 回滚如果调用不完整，可能留下 dangling references。→ Mitigation：优先预校验；必须回滚时只清理本次新建对象，并在真实 Editor 中验证图可打开、可编译。
- [Risk] 资产存在性检查可能改变以往覆盖式创建的隐式行为。→ Mitigation：明确返回 `*_ALREADY_EXISTS`，不执行覆盖。
- [Risk] mapping 幂等 no-op 会保留已有 mapping 的 modifiers/triggers。→ Mitigation：本 change 只处理 action/key 级别重复，复杂 mapping 配置留给未来 command。

## Migration Plan

不需要数据迁移。修复后已有重复 input mappings 或已存在半成品 graph/node 不会自动清理；本 change 只保证后续调用不再引入这些问题。

## Open Questions

- 编译失败错误码是否统一使用 `BLUEPRINT_COMPILE_FAILED`，Widget Blueprint 是否单独使用 `WIDGET_COMPILE_FAILED`？
- `spawn_actor` 短类名解析是否只支持 `AActor` 子类，还是也允许 Blueprint generated class path 的 `_C` 形式继续通过完整路径加载？
