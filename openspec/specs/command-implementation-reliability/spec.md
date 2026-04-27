# command-implementation-reliability Specification

## Purpose
定义稳定 command 的实现可靠性底线：成功必须代表目标变更真实完成，失败不能静默吞掉编译错误，也不能留下本次调用创建的明显半成品资产、graph 或 node。后续 Agent 新增或修改 `Skill/sunrealmcp-unreal-editor-workflow/cli/src/commands/**` 与 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/**` 时，应把本 spec 当作 command handler 的主要质量门槛。

## Requirements

### Requirement: `compile_blueprint` 必须报告真实编译失败

当目标 Blueprint 编译没有成功时，`compile_blueprint` SHALL 返回 `ok:false`。错误 SHALL 使用结构化错误码，例如 `BLUEPRINT_COMPILE_FAILED`。只有 Blueprint 编译成功时，命令才允许返回 `data.status = "compiled"`。

#### Scenario: Blueprint 编译失败

- **WHEN** `compile_blueprint` 被用于一个会产生编译错误的现有 Blueprint
- **THEN** command 返回 `ok:false`
- **AND** `error.code == "BLUEPRINT_COMPILE_FAILED"`

#### Scenario: Blueprint 编译成功

- **WHEN** `compile_blueprint` 被用于一个可成功编译的现有 Blueprint
- **THEN** command 返回 `ok:true`
- **AND** `data.status == "compiled"`

### Requirement: 会编译 Blueprint 资产的 command 不得隐藏编译失败

会修改 Blueprint 或 UMG 资产并触发编译的 command SHALL 检查编译结果后再返回成功。如果编译失败，command SHALL 返回结构化编译错误，而不是报告目标修改已成功完成。

#### Scenario: Blueprint 修改导致后续编译失败

- **WHEN** Blueprint 修改类 command 已应用变更，但后续编译失败
- **THEN** command 返回 `ok:false`
- **AND** error code 能表达这是编译失败

#### Scenario: Widget 修改后成功编译

- **WHEN** UMG widget tree command 修改 Widget Blueprint 且编译成功
- **THEN** command 可以返回 `ok:true` 与正常 data payload

### Requirement: node 创建失败不得留下半成品 node

当 `add_blueprint_function_node` 因传入的 pin 默认值非法或 pin 名不存在而返回 `ok:false` 时，SHALL NOT 在 graph 中留下本次调用新建的 function node。

#### Scenario: function node 参数引用不存在的 pin

- **WHEN** `add_blueprint_function_node` 收到的 `params` 包含不存在的 pin 名
- **THEN** command 返回 `ok:false`
- **AND** Blueprint graph 中不存在该失败调用新加入的 function node

### Requirement: Widget event bind 失败不得留下半成品 graph artifact

当 `bind_widget_event` 在创建中间 binding artifact 后返回 `ok:false` 时，SHALL NOT 留下本次调用新建的 event node、function graph 或 function call node。

#### Scenario: event 到 function 的 exec pin 无法连接

- **WHEN** `bind_widget_event` 已创建部分中间 artifact，但无法完成最终 event-to-function 连接
- **THEN** command 返回 `ok:false`
- **AND** 仅由本次失败调用创建的 artifact 被移除或从未提交

### Requirement: 资产创建 command 必须可靠拒绝已有资产

`create_blueprint` 与 `create_umg_widget_blueprint` SHALL 能检测目标资产已存在的情况，包括资产已经加载、仅存在于 Asset Registry 或磁盘、以及调用方使用无 `.AssetName` 后缀路径的情况。`/Game/Path/Asset` 与 `/Game/Path/Asset.Asset` SHALL 被视为同一个目标资产。

#### Scenario: Blueprint 已存在但未加载

- **WHEN** `create_blueprint` 被调用到一个已存在但当前未加载的目标资产路径
- **THEN** command 返回 `ok:false`
- **AND** `error.code == "BLUEPRINT_ALREADY_EXISTS"`

#### Scenario: Widget Blueprint 使用规范化 object path 查重

- **WHEN** `/Game/UI/WBP_Menu.WBP_Menu` 已存在，而调用方传入 `/Game/UI/WBP_Menu`
- **THEN** `create_umg_widget_blueprint` 返回 `ok:false`
- **AND** `error.code == "WIDGET_ALREADY_EXISTS"`

### Requirement: Enhanced Input mapping 对相同 action/key 幂等

当目标 `InputMappingContext` 已经包含相同 `InputAction` 与 `FKey` 的 mapping 时，`add_enhanced_input_mapping` SHALL NOT 追加重复 mapping。重复调用 SHALL 返回 `ok:true`，并在 data 中表达该 mapping 已存在。

#### Scenario: mapping 已存在

- **WHEN** `add_enhanced_input_mapping` 被用于一个已存在的 action/key pair
- **THEN** command 返回 `ok:true`
- **AND** mapping count 不增加
- **AND** `data.already_existed == true`

### Requirement: `spawn_actor` 的 class 契约必须一致

`spawn_actor` 的 CLI wrapper 与 Unreal handler SHALL 对可接受的 `actor_class` 格式保持一致。如果 CLI help 宣称支持 `StaticMeshActor` 这类短类名，handler SHALL 能把该短类名解析为 `AActor` 子类；如果未来选择不支持短类名，则 CLI help MUST 同步收窄。

#### Scenario: 使用引擎短类名 spawn actor

- **WHEN** Agent 调用 `spawn_actor` 且 `actor_class = "StaticMeshActor"`
- **THEN** handler 解析到一个 `AActor` 子类
- **AND** 如果解析失败，错误信息与 CLI help 中承诺的契约一致

### Requirement: `add_blueprint_variable` 必须确认变量实际创建

`add_blueprint_variable` SHALL 仅在确认请求变量已出现在 `Blueprint->NewVariables` 后返回成功。如果 Unreal 拒绝该变量名或因为其他原因没有创建变量，command SHALL 返回 `ok:false`，不得报告变量已创建。

#### Scenario: Unreal 拒绝变量创建

- **WHEN** `add_blueprint_variable` 使用了 Unreal 没有加入 `Blueprint->NewVariables` 的变量名
- **THEN** command 返回 `ok:false`
- **AND** 不报告该变量已创建

### Requirement: UMG command 必须保持 source widget GUID 有效

会创建或挂接 source widget 的 UMG command SHALL 确保 `WidgetBlueprint->WidgetVariableNameToGuidMap` 中存在对应 widget 的 GUID entry。该要求覆盖 Unreal Widget Blueprint compiler 会通过 `ForEachSourceWidget` 校验的 widget，包括 `RootCanvas`、命令创建的变量 widget，以及按钮内部 text 这类非变量 source widget。

即使 `Widget->bIsVariable == false`，command 也 SHOULD 为该 source widget 维护 GUID entry，因为 UE 5.6 的 compiler 会独立校验 source-widget GUID tracking，而不是只校验暴露为 generated class variable 的 widget。

#### Scenario: Button command 创建嵌套 text widget

- **WHEN** `add_button_to_widget` 创建一个 button 和其内部 text widget
- **THEN** Widget Blueprint 可以编译且不会触发 `WidgetVariableNameToGuidMap` ensure
- **AND** 本次 command 创建的 source widgets 都有 GUID entry

#### Scenario: command 创建或解析 RootCanvas

- **WHEN** UMG command 创建或解析 Widget Blueprint 的 root canvas
- **THEN** root canvas 在 Widget Blueprint 被 structurally modified 并编译前已经有 GUID entry
