## ADDED Requirements

### Requirement: Blueprint compile command must report real compile failure

`compile_blueprint` SHALL return `ok:false` when the target Blueprint compile does not succeed. The error SHALL use a structured code such as `BLUEPRINT_COMPILE_FAILED` and SHALL NOT return `data.status = "compiled"` unless the Blueprint compile succeeded.

#### Scenario: Blueprint compile fails

- **WHEN** `compile_blueprint` is called for an existing Blueprint that produces compile errors
- **THEN** the command returns `ok:false`
- **AND** `error.code == "BLUEPRINT_COMPILE_FAILED"`

#### Scenario: Blueprint compile succeeds

- **WHEN** `compile_blueprint` is called for an existing Blueprint and compilation succeeds
- **THEN** the command returns `ok:true`
- **AND** `data.status == "compiled"`

### Requirement: Commands that compile Blueprint assets must not hide compile failures

Blueprint and UMG commands that modify assets and then compile them SHALL check the compile result before returning success. If compilation fails, the command SHALL return a structured compile error instead of reporting successful completion.

#### Scenario: Component edit causes Blueprint compile failure

- **WHEN** a Blueprint modification command applies a change and the subsequent compile fails
- **THEN** the command returns `ok:false`
- **AND** the error code identifies a compile failure

#### Scenario: Widget edit compiles successfully

- **WHEN** a UMG widget tree command modifies a Widget Blueprint and compilation succeeds
- **THEN** the command may return `ok:true` with its normal data payload

### Requirement: Node creation failure must not leave half-created nodes

`add_blueprint_function_node` SHALL NOT leave a new function node in the graph when it returns `ok:false` because provided pin defaults are invalid or reference missing pins.

#### Scenario: Function node parameter references missing pin

- **WHEN** `add_blueprint_function_node` receives `params` containing a pin name that does not exist
- **THEN** the command returns `ok:false`
- **AND** the Blueprint graph does not contain a newly added function node from that failed call

### Requirement: Widget event binding failure must not leave half-created graph artifacts

`bind_widget_event` SHALL NOT leave newly created event nodes, function graphs, or function call nodes when it returns `ok:false` after partially creating binding artifacts.

#### Scenario: Binding cannot connect exec pins

- **WHEN** `bind_widget_event` creates intermediate binding artifacts but cannot complete the final event-to-function connection
- **THEN** the command returns `ok:false`
- **AND** artifacts created only for that failed call are removed or never committed

### Requirement: Asset creation commands must reliably reject existing assets

`create_blueprint` and `create_umg_widget_blueprint` SHALL detect existing target assets whether they are already loaded or only present in the Asset Registry. They SHALL treat `/Game/Path/Asset` and `/Game/Path/Asset.Asset` as the same target asset.

#### Scenario: Blueprint exists but is not loaded

- **WHEN** `create_blueprint` is called for an asset path that already exists on disk but is not loaded
- **THEN** the command returns `ok:false`
- **AND** `error.code == "BLUEPRINT_ALREADY_EXISTS"`

#### Scenario: Widget Blueprint exists using normalized object path

- **WHEN** `create_umg_widget_blueprint` is called with `/Game/UI/WBP_Menu` and `/Game/UI/WBP_Menu.WBP_Menu` already exists
- **THEN** the command returns `ok:false`
- **AND** `error.code == "WIDGET_ALREADY_EXISTS"`

### Requirement: Enhanced Input mapping command must be idempotent for identical action/key

`add_enhanced_input_mapping` SHALL NOT append a duplicate mapping when the target `InputMappingContext` already contains the same `InputAction` and `FKey`. Repeated calls with identical action/key SHALL return `ok:true` and indicate that the mapping already existed.

#### Scenario: Mapping already exists

- **WHEN** `add_enhanced_input_mapping` is called for an action/key pair already present in the mapping context
- **THEN** the command returns `ok:true`
- **AND** it does not increase the mapping count
- **AND** `data.already_existed == true`

### Requirement: spawn_actor class contract must be consistent

The `spawn_actor` CLI wrapper and Unreal handler SHALL agree on accepted `actor_class` formats. If CLI help advertises short class names such as `StaticMeshActor`, the handler SHALL resolve those names to `AActor` subclasses.

#### Scenario: Spawn actor using short engine class name

- **WHEN** the Agent calls `spawn_actor` with `actor_class = "StaticMeshActor"`
- **THEN** the handler resolves an `AActor` subclass or returns an error consistent with the CLI help contract

### Requirement: add_blueprint_variable must confirm creation

`add_blueprint_variable` SHALL return success only after confirming that the requested variable exists in `Blueprint->NewVariables` after the add operation. If Unreal rejects the variable name or otherwise fails to add it, the command SHALL return `ok:false`.

#### Scenario: Variable add is rejected by Unreal

- **WHEN** `add_blueprint_variable` is called with a variable name that Unreal does not add to the Blueprint
- **THEN** the command returns `ok:false`
- **AND** it does not report the variable as created
