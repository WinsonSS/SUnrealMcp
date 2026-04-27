# Command Implementation Review Findings

## Scope

本报告只记录 `review-cli-unreal-commands` 的 command wrapper / handler 实现 review 结论。不修改运行时代码。

## Task 1: 最小实现索引

### 1.1 相关实现约束

- `params` 与 `data` 内部字段应使用 `snake_case`；task payload 内部字段例外。
- `get_task_status` / `cancel_task` 必须读取 `task_id`，缺失、空字符串、非字符串应返回 `INVALID_PARAMS`。
- `check_command_exists` 必须读取 `name` 并返回 `{ exists }`。
- `diff_commands` 必须读取字符串数组 `cli_names` 并返回排序后的 `{ only_in_unreal, only_in_cli }`。
- `ping` 成功 data 只能包含 `protocol_version`。
- `reload_command_registry` 成功 data 必须为空对象；模块未加载为 `MODULE_NOT_LOADED`，reload 失败为 `RELOAD_FAILED`。
- `refresh_cpp_runtime` 应异步返回 accepted task；成功终态 payload 至少表达 Live Coding 成功与 registry reload 成功；PIE、Live Coding busy、compile failed、unavailable、registry reload failed 需要可区分错误码。

### 1.2 CLI wrapper 索引

- `system`: `ping`, `reload_command_registry`, `refresh_cpp_runtime`, `get_task_status`, `cancel_task`, `check_command_exists`, `diff_commands`
- `editor`: `get_actors_in_level`, `find_actors_by_label`, `spawn_actor`, `delete_actor`, `set_actor_transform`, `get_actor_properties`, `set_actor_property`, `spawn_blueprint_actor`
- `blueprint`: `create_blueprint`, `add_component_to_blueprint`, `compile_blueprint`, `set_blueprint_property`, `set_component_property`, `set_static_mesh_properties`, `set_physics_properties`
- `node`: `add_blueprint_event_node`, `add_blueprint_function_node`, `connect_blueprint_nodes`, `add_blueprint_variable`, `find_blueprint_nodes`, `add_blueprint_input_action_node`, `add_blueprint_get_self_component_reference`, `add_blueprint_self_reference`, `inspect_blueprint_graph`
- `umg`: `create_umg_widget_blueprint`, `add_text_block_to_widget`, `add_button_to_widget`, `bind_widget_event`, `add_widget_to_viewport`, `set_text_block_binding`
- `project`: `add_enhanced_input_mapping`
- `raw`: `send`

### 1.3 Unreal handler 索引

- `Core`: `ping`, `reload_command_registry`, `refresh_cpp_runtime`, `get_task_status`, `cancel_task`, `check_command_exists`, `diff_commands`
- `Stable/Editor`: `get_actors_in_level`, `find_actors_by_label`, `get_actor_properties`, `spawn_actor`, `spawn_blueprint_actor`, `delete_actor`, `set_actor_transform`, `set_actor_property`
- `Stable/Blueprint`: `create_blueprint`, `compile_blueprint`, `add_component_to_blueprint`, `set_component_property`, `set_blueprint_property`, `set_physics_properties`, `set_static_mesh_properties`
- `Stable/Node`: `add_blueprint_event_node`, `add_blueprint_function_node`, `add_blueprint_get_self_component_reference`, `add_blueprint_input_action_node`, `add_blueprint_self_reference`, `add_blueprint_variable`, `connect_blueprint_nodes`, `find_blueprint_nodes`, `inspect_blueprint_graph`
- `Stable/UMG`: `create_umg_widget_blueprint`, `add_text_block_to_widget`, `add_button_to_widget`, `bind_widget_event`, `set_text_block_binding`, `add_widget_to_viewport`
- `Stable/Project`: `add_enhanced_input_mapping`
- `Temporary/System`: `start_mock_task`

### 1.4 索引结论

- CLI wrapper 与 stable/core Unreal handler 的 command 名称覆盖基本一致。
- `raw send` 是预期的 CLI-only escape hatch。
- `start_mock_task` 是 Unreal-only temporary command；本轮不作为 stable command 实现问题处理，除非后续任务需要检查 temporary system command。
- 后续 review 将按 tasks 2-6 的 family 分组逐个检查 handler 逻辑。

## Task 2: System/Core command 实现 review

### 2.1 `ping` / `reload_command_registry` / `refresh_cpp_runtime`

暂无确定性 findings。

- `ping` 返回 data 仅包含 `protocol_version`。
- `reload_command_registry` 成功返回 `{}`，失败路径区分 `MODULE_NOT_LOADED` 与 `RELOAD_FAILED`。
- `refresh_cpp_runtime` 以 task 形式异步执行，覆盖 PIE active、Live Coding busy/unavailable/compile failed、registry reload failed，并在成功 payload 中设置 `live_coding = "succeeded"` 与 `command_registry_reloaded = true`。

### 2.2 `get_task_status` / `cancel_task`

暂无确定性 findings。

- CLI wrapper 使用 `task_id` 且无重命名 `mapParams`。
- Unreal handler 从 `params.task_id` 读取，缺失或空字符串返回 `INVALID_PARAMS`。
- Task registry 返回 `task_id`、`status`，并按 payload/error 存在情况附加对应对象。

### 2.3 `check_command_exists` / `diff_commands`

Finding CORE-001: `diff_commands` 会静默忽略非字符串数组元素。

- 文件：`D:\Projects\SUnrealMcp\UnrealPlugin\SUnrealMcp\Source\SUnrealMcp\Private\Commands\Core\DiffCommandsCommand.cpp:37`
- 触发条件：调用方发送 `params.cli_names` 为数组，但其中某些元素不是字符串，例如 `["ping", 1]`。
- 当前行为：循环中只把 `TryGetString` 成功的元素加入集合，非字符串元素被跳过，命令仍返回 `ok:true`。
- 影响：`diff_commands` 的输入契约是字符串数组。静默忽略坏元素会掩盖 CLI/raw caller 的输入错误，并可能产生错误的 `only_in_unreal` / `only_in_cli` 差集。
- 建议：遇到任何非字符串元素时返回 `INVALID_PARAMS`，错误信息包含 `cli_names`。

`check_command_exists` 暂无确定性 findings。

## Task 3: Editor command 实现 review

### 3.1 actor 查询类 command

暂无确定性 findings。

- `get_actors_in_level` 使用 editor world，并为每个 actor 返回 summary。
- `find_actors_by_label` 校验 `pattern`，支持普通 contains 与 `*` / `?` wildcard。
- `get_actor_properties` 通过 actor path 查找当前 editor world 中的 actor，并导出支持的属性值。

### 3.2 actor 创建/删除类 command

Finding EDITOR-001: `spawn_actor` 的 CLI 文档/参数契约允许短类名，但 handler 只接受完整 class path。

- CLI 文件：`D:\Projects\SUnrealMcp\Skill\sunrealmcp-unreal-editor-workflow\cli\src\commands\stable\editor_commands.ts:46`
- Handler 文件：`D:\Projects\SUnrealMcp\UnrealPlugin\SUnrealMcp\Source\SUnrealMcp\Private\Commands\Stable\Editor\SpawnActorCommand.cpp:43`
- Shared util：`D:\Projects\SUnrealMcp\UnrealPlugin\SUnrealMcp\Source\SUnrealMcp\Private\Commands\Shared\Editor\EditorCommandUtils.h:171`
- 触发条件：Agent 按 CLI help 使用 `--actor_class StaticMeshActor`。
- 当前行为：`ResolveActorClassPath` 要求输入同时包含 `/` 与 `.`，短类名直接返回 nullptr，最终 `spawn_actor` 返回 `INVALID_CLASS`。
- 影响：CLI 暴露的有效用法在 Unreal handler 中不可用，Agent 会按 help 走到失败路径。
- 建议：要么支持短类名解析（例如常见 engine actor class），要么收窄 CLI description，只承诺完整 class path。

`spawn_blueprint_actor`、`delete_actor` 暂无确定性 findings。

### 3.3 actor 修改类 command

暂无确定性主 findings。

验证/改进缺口：

- `set_actor_transform` 对 `location` / `rotation` / `scale` 使用 `TryReadVector3`。如果 raw caller 提供了字段但数组格式错误，handler 会把它当作“未提供”；当另一个 transform 字段合法时，命令仍会部分成功。CLI wrapper 会阻止这种输入，因此这不是默认路径 bug，但 raw-call 鲁棒性可以加强。
- `set_actor_property` 在属性应用失败前已经调用 `Actor->Modify()`；当前未观察到属性被实际改坏，但如果后续要做事务洁净度 review，可以单独验证。

## Task 4: Blueprint command 实现 review

### 4.1 Blueprint asset 生命周期 command

Finding BLUEPRINT-001: `compile_blueprint` 无论编译结果如何都返回成功。

- 文件：`D:\Projects\SUnrealMcp\UnrealPlugin\SUnrealMcp\Source\SUnrealMcp\Private\Commands\Stable\Blueprint\CompileBlueprintCommand.cpp:37`
- 触发条件：目标 Blueprint 存在，但编译产生错误或 Blueprint 状态未成功。
- 当前行为：调用 `FKismetEditorUtilities::CompileBlueprint(Blueprint)` 后不检查 `Blueprint->Status` / 编译错误，直接返回 `ok:true` 且 `data.status = "compiled"`。
- 影响：Agent 会把失败编译误判为成功，随后继续基于未通过编译的 Blueprint 执行后续 command。
- 建议：编译后检查 Blueprint 编译状态；失败时返回结构化错误，成功时再返回 `compiled`。

Finding BLUEPRINT-002: `create_blueprint` 的已存在检查可能漏掉磁盘上已有但未加载，或以无 `.AssetName` 形式传入的资产。

- 文件：`D:\Projects\SUnrealMcp\UnrealPlugin\SUnrealMcp\Source\SUnrealMcp\Private\Commands\Stable\Blueprint\CreateBlueprintCommand.cpp:52`
- 相关路径解析：`D:\Projects\SUnrealMcp\UnrealPlugin\SUnrealMcp\Source\SUnrealMcp\Private\Commands\Shared\Blueprint\BlueprintCommandUtils.h:25`
- 触发条件：调用方传入 `/Game/Foo/BP_Test` 这类无对象名后缀路径，或目标 Blueprint 已存在但尚未加载。
- 当前行为：`SplitAssetPath` 会接受该路径并推导 package/name，但存在性检查直接 `FindObject<UBlueprint>(nullptr, *BlueprintPath)`，没有用规范化 object path `/Game/Foo/BP_Test.BP_Test`，也没有查 Asset Registry。
- 影响：可能没有正确返回 `BLUEPRINT_ALREADY_EXISTS`，后续 `CreatePackage` / `CreateBlueprint` 在已有资产位置上行为不确定。
- 建议：基于 `PackagePath` + `AssetName` 生成规范化 object path，并用 `StaticLoadObject` 或 Asset Registry 做存在性检查。

### 4.2 Blueprint component command

暂无确定性主 findings。

验证/改进缺口：

- `add_component_to_blueprint` 对可选 `location` / `rotation` / `scale` 使用 `TryReadVector3`；raw caller 传入格式错误时会被静默视为未提供。CLI wrapper 会阻止这种输入，因此不是默认路径 bug。
- `add_component_to_blueprint`、`set_component_property` 都会编译 Blueprint，但未显式检查编译失败；如果后续希望所有“修改并编译”类 command 都阻断失败编译，可以统一修复。

### 4.3 Blueprint property / physics / static mesh command

暂无确定性主 findings。

验证/改进缺口：

- `set_blueprint_property`、`set_physics_properties`、`set_static_mesh_properties` 修改后都会编译 Blueprint，但当前没有显式检查编译失败。
- `set_physics_properties` 对 raw caller 缺失的可选字段使用 CLI 默认值语义，会覆盖 mass/damping/gravity 等属性；CLI 默认路径符合 wrapper 定义，但 raw-call 语义不是“只改提供字段”。

## Task 5: Node command 实现 review

### 5.1 node 创建 command

Finding NODE-001: `add_blueprint_function_node` 在参数默认值校验失败时会留下已经插入图中的节点。

- 文件：`D:\Projects\SUnrealMcp\UnrealPlugin\SUnrealMcp\Source\SUnrealMcp\Private\Commands\Stable\Node\AddBlueprintFunctionNodeCommand.cpp:62`
- 触发条件：调用方传入 `params` 对象，但其中某个 pin 名不存在，或 pin 默认值类型不合法。
- 当前行为：handler 先 `EventGraph->AddNode(FunctionNode, true, false)` 并分配 pins，然后遍历 `params`。遇到 `PIN_NOT_FOUND` 或 `INVALID_PIN_VALUE` 时直接返回错误，没有移除刚创建的 node。
- 影响：命令返回 `ok:false`，但 Blueprint 图已经被部分修改；Agent 可能重试并产生重复/脏节点。
- 建议：先创建但不提交，或失败时从 graph 移除 node 并清理；也可以先根据函数签名预校验 `params` 后再 `AddNode`。

Finding NODE-002: `add_blueprint_variable` 不确认变量是否实际添加成功。

- 文件：`D:\Projects\SUnrealMcp\UnrealPlugin\SUnrealMcp\Source\SUnrealMcp\Private\Commands\Stable\Node\AddBlueprintVariableCommand.cpp:98`
- 触发条件：变量名非法、与继承成员或其它 Blueprint 限制冲突，导致 `FBlueprintEditorUtils::AddMemberVariable` 未真正新增变量。
- 当前行为：handler 调用 `AddMemberVariable` 后只尝试遍历 `Blueprint->NewVariables` 设置 exposed flags；即使找不到新变量，仍然 `MarkBlueprintAsStructurallyModified` 并返回 `ok:true`。
- 影响：Agent 会认为变量已创建，但后续查找/使用变量失败。
- 建议：检查添加结果，或在调用后确认 `Blueprint->NewVariables` 中确实出现目标变量；失败时返回结构化错误。

`add_blueprint_event_node`、`add_blueprint_input_action_node`、`add_blueprint_get_self_component_reference`、`add_blueprint_self_reference` 暂无确定性 findings。

### 5.2 node 连接与查询 command

暂无确定性主 findings。

- `connect_blueprint_nodes` 通过 node guid 与 pin name 定位，并使用 schema `TryCreateConnection`。
- `find_blueprint_nodes` 支持按节点类型和 event type 过滤 event graph。
- `inspect_blueprint_graph` 支持 event/function/macro/delegate graph，并按 `include_pins` 控制输出体积。

### 5.3 node family 输出

- 确定性 findings：NODE-001、NODE-002。
- 验证缺口：node 创建类 command 普遍没有对 raw caller 的无效 `node_position` 返回错误；CLI 默认路径会保证 vec2 格式。

## Task 6: UMG 与 Project command 实现 review

### 6.1 UMG asset/widget tree command

Finding UMG-001: `create_umg_widget_blueprint` 的已存在检查可能漏掉已有但未加载，或以无 `.AssetName` 形式传入的资产。

- 文件：`D:\Projects\SUnrealMcp\UnrealPlugin\SUnrealMcp\Source\SUnrealMcp\Private\Commands\Stable\UMG\CreateUMGWidgetBlueprintCommand.cpp:46`
- 触发条件：调用方传入 `/Game/UI/WBP_Menu` 这类无对象名后缀路径，或目标 Widget Blueprint 已存在但尚未加载。
- 当前行为：存在性检查直接 `FindObject<UWidgetBlueprint>(nullptr, *WidgetPath)`，没有用规范化 object path `/Game/UI/WBP_Menu.WBP_Menu`，也没有查 Asset Registry。
- 影响：可能没有正确返回 `WIDGET_ALREADY_EXISTS`，后续 `CreatePackage` / `CreateBlueprint` 在已有资产位置上行为不确定。
- 建议：与 BLUEPRINT-002 一起统一修复资产存在性检查。

`add_text_block_to_widget`、`add_button_to_widget` 暂无确定性主 findings。

验证/改进缺口：

- 两者都在 `WidgetBlueprint->WidgetTree->FindWidget(...)` 前假设 `WidgetTree` 非空；正常 Widget Blueprint 应成立，但损坏资产或特殊对象可能导致空指针风险。
- 两者创建后会编译 Widget Blueprint，但未显式检查编译失败。

### 6.2 UMG runtime/binding command

Finding UMG-002: `bind_widget_event` 在后续失败时会留下已创建的 event/function/call 节点或函数图。

- 文件：`D:\Projects\SUnrealMcp\UnrealPlugin\SUnrealMcp\Source\SUnrealMcp\Private\Commands\Stable\UMG\BindWidgetEventCommand.cpp:94`
- 后续失败路径：`FUNCTION_NOT_FOUND` at line 124、`EXEC_PIN_NOT_FOUND` at line 161、`EVENT_BIND_CONNECT_FAILED` at line 170。
- 触发条件：bound event 创建成功，但 function graph 未生成可解析函数、exec pin 缺失，或连接失败。
- 当前行为：handler 先调用 `CreateNewBoundEventForClass` / `EnsureFunctionGraph`，后续失败时直接返回错误，没有回滚已创建的 graph/node。
- 影响：命令返回 `ok:false`，但 Widget Blueprint 已经被部分修改；重试可能产生重复函数图、事件节点或 call node。
- 建议：预校验可绑定 event 与目标 function 可创建性；或在失败路径回滚本次创建的 event node / function graph / call node。

`set_text_block_binding`、`add_widget_to_viewport` 暂无确定性主 findings。

### 6.3 project command

Finding PROJECT-001: `add_enhanced_input_mapping` 重复调用会追加重复 mapping。

- 文件：`D:\Projects\SUnrealMcp\UnrealPlugin\SUnrealMcp\Source\SUnrealMcp\Private\Commands\Stable\Project\AddEnhancedInputMappingCommand.cpp:62`
- 触发条件：对同一个 `mapping_context_path`、`input_action_path`、`key` 多次调用命令。
- 当前行为：每次都执行 `MappingContext->MapKey(InputAction, Key)`，未检查现有 `GetMappings()` 中是否已经存在相同 action/key。
- 影响：Agent 重试或幂等调用会让 mapping context 积累重复映射，后续输入触发可能重复或造成维护噪音。
- 建议：写入前检查已有 mapping；若已存在，返回 `ok:true` 并标记 `already_existed`，或返回明确的 no-op data。

## Task 7: 汇总

### 7.1 确定性 command 实现 findings

按建议修复优先级排序：

1. BLUEPRINT-001: `compile_blueprint` 无论编译是否失败都返回 `ok:true/status:"compiled"`。
2. NODE-001: `add_blueprint_function_node` 在 pin 参数失败时返回错误但留下已插入图中的节点。
3. UMG-002: `bind_widget_event` 在中后段失败时返回错误但留下已创建的 event/function/call 节点或函数图。
4. PROJECT-001: `add_enhanced_input_mapping` 重复调用会追加重复 mapping，不具备幂等性。
5. EDITOR-001: `spawn_actor` CLI 承诺短类名可用，但 handler 只接受完整 class path。
6. BLUEPRINT-002: `create_blueprint` 资产存在性检查可能漏掉已有资产。
7. UMG-001: `create_umg_widget_blueprint` 资产存在性检查可能漏掉已有资产。
8. NODE-002: `add_blueprint_variable` 不确认变量是否实际添加成功。
9. CORE-001: `diff_commands` 静默忽略 `cli_names` 里的非字符串元素。

### 7.2 已完成且暂无主 findings 的范围

- `system`: `ping`, `reload_command_registry`, `refresh_cpp_runtime`, `get_task_status`, `cancel_task`, `check_command_exists`
- `editor`: `get_actors_in_level`, `find_actors_by_label`, `get_actor_properties`, `spawn_blueprint_actor`, `delete_actor`, `set_actor_transform`, `set_actor_property`
- `blueprint`: `add_component_to_blueprint`, `set_component_property`, `set_blueprint_property`, `set_physics_properties`, `set_static_mesh_properties` 无独立主 finding，但继承“修改后编译不检查失败”的统一缺口
- `node`: `add_blueprint_event_node`, `add_blueprint_input_action_node`, `add_blueprint_get_self_component_reference`, `add_blueprint_self_reference`, `connect_blueprint_nodes`, `find_blueprint_nodes`, `inspect_blueprint_graph`
- `umg`: `add_text_block_to_widget`, `add_button_to_widget`, `set_text_block_binding`, `add_widget_to_viewport`

### 7.3 需要真实 Unreal Editor 验证的缺口

- Blueprint/UMG 修改后编译失败如何暴露：需要构造会编译失败的 Blueprint/Widget Blueprint，验证 `CompileBlueprint` 后可读取的状态字段与错误日志位置。
- Graph rollback：需要在真实 Blueprint 图中验证从 `UEdGraph` 移除刚创建 node / function graph 的安全做法。
- Widget binding：需要验证 `CreateNewBoundEventForClass` 与 `EnsureFunctionGraph` 在不同 widget event 上的副作用和可回滚点。
- Raw-call 参数鲁棒性：`node_position`、`location`、`rotation`、`scale`、UMG `position`/`size`/`color` 的坏数组当前多为 fallback/no-op，CLI 默认路径不会触发，但 raw caller 可触发。

### 7.4 后续建议

建议开一个修复型 change，优先范围：

- 先修 `compile_blueprint` 编译失败误报成功，并把 Blueprint/UMG “修改后编译”命令统一接入编译结果检查。
- 再修 graph/widget binding 的失败回滚，避免 `ok:false` 后留下半成品资产改动。
- 然后修资产存在性检查与 `add_enhanced_input_mapping` 幂等性。
- 最后修 CLI/handler 契约和输入校验类问题：`spawn_actor` 短类名、`diff_commands` 字符串数组验证、`add_blueprint_variable` 添加结果确认。
