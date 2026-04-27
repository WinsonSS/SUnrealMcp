## 1. Shared Helpers

- [x] 1.1 在 Blueprint/UMG shared utils 中增加规范化 asset object path 与存在性检查 helper，覆盖 `/Game/Foo/Asset` 与 `/Game/Foo/Asset.Asset`。
- [x] 1.2 增加 Blueprint/Widget Blueprint compile result helper，统一判断 `FKismetEditorUtilities::CompileBlueprint` 后是否成功，并生成结构化错误信息。
- [x] 1.3 增加或复用 actor class 短名解析 helper，使 `spawn_actor` 能解析常见 `AActor` 子类短名。

## 2. Compile Result Fixes

- [x] 2.1 修复 `compile_blueprint`：编译失败时返回 `BLUEPRINT_COMPILE_FAILED`，成功时才返回 `data.status = "compiled"`。
- [x] 2.2 将 Blueprint 修改类 command 接入编译结果检查：`add_component_to_blueprint`、`set_component_property`、`set_blueprint_property`、`set_physics_properties`、`set_static_mesh_properties`。
- [x] 2.3 将 UMG 修改类 command 接入编译结果检查：`create_umg_widget_blueprint`、`add_text_block_to_widget`、`add_button_to_widget`、`bind_widget_event`、`set_text_block_binding`。

## 3. Failure Rollback Fixes

- [x] 3.1 修复 `add_blueprint_function_node`：在提交 node 前预校验 `params` pin 名和默认值，或在失败路径移除本次创建的 node。
- [x] 3.2 修复 `bind_widget_event`：记录本次创建的 event node、function graph、function call node，并在后续失败路径回滚。
- [x] 3.3 验证失败路径：人为构造 missing pin / invalid pin value / event bind connect failure 场景，确认 `ok:false` 后不会留下半成品 graph artifact。

## 4. Asset Creation Fixes

- [x] 4.1 修复 `create_blueprint`：使用规范化 object path 与 Asset Registry / load 检查已有资产，正确返回 `BLUEPRINT_ALREADY_EXISTS`。
- [x] 4.2 修复 `create_umg_widget_blueprint`：使用同样策略检查已有 Widget Blueprint，正确返回 `WIDGET_ALREADY_EXISTS`。
- [x] 4.3 验证无 `.AssetName` 后缀路径和完整 object path 都指向同一目标资产。

## 5. Idempotency And Contract Fixes

- [x] 5.1 修复 `add_enhanced_input_mapping`：重复 action/key 不再追加 mapping，返回 `ok:true` 且 `data.already_existed = true`。
- [x] 5.2 修复 `spawn_actor`：支持 CLI 文案中的短类名，或同步收窄 CLI description；优先实现短类名解析。
- [x] 5.3 修复 `add_blueprint_variable`：调用 `AddMemberVariable` 后确认变量实际存在，否则返回结构化错误。

## 6. Verification

- [x] 6.1 运行 TypeScript CLI build，确认 wrapper 文案或类型变更无编译错误。
- [x] 6.2 运行可用的 Unreal/C++ 编译或至少做静态编译风险检查；若当前环境无法编译 Unreal plugin，记录未验证项。
- [x] 6.3 如有可用 Editor 测试工程，运行非破坏性与小型资产验证：compile failure、existing asset、duplicate mapping、short actor class、rollback 场景。
- [x] 6.4 汇总修复结果与剩余风险，确认本 change 覆盖所有“建议处理”的 findings。
