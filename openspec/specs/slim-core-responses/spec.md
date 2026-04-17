# slim-core-responses Specification

## Purpose
TBD - created by archiving change slim-core-response-fields. Update Purpose after archive.
## Requirements
### Requirement: `ping` 响应 data 仅含 `protocol_version`

Unreal core 命令 `ping` 成功响应的 `data` 对象 SHALL 恰好包含一个字段 `protocol_version`（number），其值等于 `FSUnrealMcpRequest::ProtocolVersion`。消费方可据此在建立连接后确认协议版本匹配。

#### Scenario: ping 成功响应只含 protocol_version

- **WHEN** CLI 发送 `{command:"ping", params:{}}`
- **THEN** Unreal 侧返回 `ok:true`，`data.protocol_version == 1`
- **AND** `data` 对象的键集合恰为 `{protocol_version}`

### Requirement: `reload_command_registry` 成功响应 data 为空

Unreal core 命令 `reload_command_registry` 在命令表重建成功时 SHALL 返回 `ok:true` 且 `data` 为空对象 `{}`——成功语义由 envelope 的 `ok` 字段承载。错误路径保持结构化错误：模块未加载返回 `error.code = "MODULE_NOT_LOADED"`，重建失败返回 `error.code = "RELOAD_FAILED"`。

#### Scenario: 重建成功响应 data 为空

- **WHEN** CLI 发送 `{command:"reload_command_registry", params:{}}` 且模块加载成功重建命令表
- **THEN** Unreal 侧返回 `ok:true`，`data` 为空对象 `{}`

#### Scenario: 模块未加载返回结构化错误

- **WHEN** CLI 发送 `{command:"reload_command_registry", params:{}}` 但 `FSUnrealMcpModule` 未加载
- **THEN** Unreal 侧返回 `ok:false`，`error.code = "MODULE_NOT_LOADED"`

#### Scenario: 重建失败返回结构化错误

- **WHEN** CLI 发送 `{command:"reload_command_registry", params:{}}` 且 `RebuildCommandRegistry` 返回失败
- **THEN** Unreal 侧返回 `ok:false`，`error.code = "RELOAD_FAILED"`，`error.message` 为具体失败原因

