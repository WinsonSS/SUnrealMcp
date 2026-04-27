## 1. 建立最小实现索引

- [x] 1.1 读取与 command 实现直接相关的 specs：`core-command-wire-protocol`、`command-registry-introspection`、`slim-core-responses`、`large-response-error`、`cpp-runtime-refresh`，只提炼会影响 command handler/wrapper 正确性的约束。
- [x] 1.2 建立 CLI wrapper 索引：从 `Skill/sunrealmcp-unreal-editor-workflow/cli/src/commands/**` 提取每个 command 的 `family`、`cliCommand`、`unrealCommand`、参数定义、`mapParams`、自定义 `execute`。
- [x] 1.3 建立 Unreal handler 索引：从 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/**` 提取每个 handler 的 `GetCommandName()`、参数读取字段、主要 Unreal API 调用、成功响应字段、错误响应路径。
- [x] 1.4 输出索引结论：列出需要逐个 review 的 command 列表；只记录会影响 command 实现判断的 CLI/Unreal 对应关系问题。

## 2. Review System/Core Command 实现

- [x] 2.1 Review `ping`、`reload_command_registry`、`refresh_cpp_runtime` 的 handler/wrapper 实现，检查 task 创建、reload 时机、成功/失败响应是否可能错误。
- [x] 2.2 Review `get_task_status` 与 `cancel_task` 的 handler/wrapper 实现，检查 `task_id` 校验、状态转换、payload/error 返回是否可能错误。
- [x] 2.3 Review `check_command_exists` 与 `diff_commands` 的 handler/wrapper 实现，检查 `name`、`cli_names`、差集计算、排序和响应字段是否可能错误。

## 3. Review Editor Command 实现

- [x] 3.1 Review actor 查询类 command：`get_actors_in_level`、`find_actors_by_label`、`get_actor_properties`，检查世界/actor 获取、过滤逻辑、属性读取和返回结构。
- [x] 3.2 Review actor 创建/删除类 command：`spawn_actor`、`spawn_blueprint_actor`、`delete_actor`，检查 class/asset 解析、transform、事务、失败路径和副作用。
- [x] 3.3 Review actor 修改类 command：`set_actor_transform`、`set_actor_property`，检查参数校验、类型转换、事务/dirty 标记和错误返回。

## 4. Review Blueprint Command 实现

- [x] 4.1 Review Blueprint asset 生命周期 command：`create_blueprint`、`compile_blueprint`，检查路径处理、父类解析、资产创建/保存/编译结果处理。
- [x] 4.2 Review Blueprint component command：`add_component_to_blueprint`、`set_component_property`，检查 component 查找/创建、类型校验、属性写入和编译/dirty 处理。
- [x] 4.3 Review Blueprint property/physics/static mesh command：`set_blueprint_property`、`set_physics_properties`、`set_static_mesh_properties`，检查字段映射、对象类型前置条件和失败路径。

## 5. Review Node Command 实现

- [x] 5.1 Review node 创建 command：event/function/input/self/component reference/variable 相关 command，检查 graph 查找、节点类型、坐标、命名冲突和返回 node id。
- [x] 5.2 Review node 连接与查询 command：`connect_blueprint_nodes`、`find_blueprint_nodes`、`inspect_blueprint_graph`，检查 pin 匹配、连接方向、图遍历和返回结构。
- [x] 5.3 对 node family 输出 findings / 暂无 findings / 需要真实 Editor 验证的缺口。

## 6. Review UMG 与 Project Command 实现

- [x] 6.1 Review UMG asset/widget tree command：`create_umg_widget_blueprint`、`add_button_to_widget`、`add_text_block_to_widget`，检查 widget blueprint 创建、widget tree 修改、命名和返回结构。
- [x] 6.2 Review UMG runtime/binding command：`bind_widget_event`、`set_text_block_binding`、`add_widget_to_viewport`，检查事件/绑定目标、函数创建、viewport 副作用和失败路径。
- [x] 6.3 Review project command：`add_enhanced_input_mapping`，检查 asset 查找、mapping 写入、重复项处理和保存/dirty 逻辑。

## 7. 汇总 Command 实现 Findings

- [x] 7.1 汇总确定性 command 实现 findings：按严重程度排序，包含文件路径、行号、影响、触发条件和建议修复方向。
- [x] 7.2 汇总已完成且暂无 findings 的 command/family，避免下次重复 review。
- [x] 7.3 汇总需要真实 Unreal Editor 验证的 command，并说明需要的工程状态或资产条件。
- [x] 7.4 给出后续建议：哪些 findings 应进入修复型 OpenSpec change，哪些可以作为低优先级改进或运行态验证项。
