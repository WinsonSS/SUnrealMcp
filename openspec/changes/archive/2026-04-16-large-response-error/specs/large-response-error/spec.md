## ADDED Requirements

### Requirement: 响应超限时返回 RESPONSE_TOO_LARGE 结构化错误

当命令执行产生的响应序列化后 UTF-8 字节数超过 `MaxPendingSendBytesPerConnection` 时，`FSUnrealMcpServer` SHALL 发送一条 `ok:false` 的错误响应替代原始响应。错误响应的 `error.code` SHALL 为 `"RESPONSE_TOO_LARGE"`。

#### Scenario: 响应超过 8 MiB 限制

- **WHEN** Unreal 侧命令产生的响应序列化后 UTF-8 字节数超过 `MaxPendingSendBytesPerConnection`
- **THEN** Agent/CLI 收到 `ok:false` 的 JSON 响应，`error.code` 为 `"RESPONSE_TOO_LARGE"`
- **AND** 连接不会在发送前被静默关闭（Agent 不会看到 broken pipe）

#### Scenario: 响应未超限时正常发送

- **WHEN** Unreal 侧命令产生的响应序列化后 UTF-8 字节数未超过 `MaxPendingSendBytesPerConnection`
- **THEN** 响应正常发送，行为与改动前完全一致

### Requirement: RESPONSE_TOO_LARGE 错误 details 包含诊断数据

`RESPONSE_TOO_LARGE` 错误响应的 `error.details` SHALL 包含 `response_bytes`（int，实际响应的 UTF-8 字节数）和 `limit_bytes`（int，`MaxPendingSendBytesPerConnection` 的配置值）两个字段。

#### Scenario: details 中携带字节数和限制值

- **WHEN** 响应超限触发 `RESPONSE_TOO_LARGE` 错误
- **THEN** `error.details.response_bytes` 为实际响应的 UTF-8 字节数（整数）
- **AND** `error.details.limit_bytes` 为 `MaxPendingSendBytesPerConnection` 的当前配置值（整数）

### Requirement: RESPONSE_TOO_LARGE 错误消息包含命令名

`RESPONSE_TOO_LARGE` 错误响应的 `error.message` SHALL 包含触发超限的命令名，让 Agent 能定位是哪条命令的响应过大。

#### Scenario: 错误消息中出现命令名

- **WHEN** 命令 `inspect_blueprint_graph` 的响应超限
- **THEN** `error.message` 中包含字面子串 `inspect_blueprint_graph`
