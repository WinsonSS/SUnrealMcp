## Why

当 Unreal 侧命令产生的响应超过 `MaxPendingSendBytesPerConnection`（默认 8 MiB）时，`SendResponseLine` 返回 `false`，服务端仅记录日志后关闭连接。Agent/CLI 侧看到的是 TCP 连接意外断开（broken pipe 或 socket hang up），无法区分是网络故障还是响应过大，也无法据此做出重试或分页决策。

## What Changes

- 在 `FSUnrealMcpServer` 的请求处理流程中，序列化响应后、调用 `SendResponseLine` 前，检测 UTF-8 字节数是否超过 `MaxPendingSendBytesPerConnection`。
- 超限时，构造一条 `RESPONSE_TOO_LARGE` 结构化错误响应（包含原始字节数和限制值），发送该错误响应后正常关闭连接。Agent 得到一条可解析的错误而非 broken pipe。
- 错误响应本身的体积远小于限制，因此不会触发二次溢出。

## Capabilities

### New Capabilities
- `large-response-error`: 定义响应过大时的结构化错误行为，包括错误码、错误消息格式和 `details` 中携带的诊断字段。

### Modified Capabilities
（无）

## Impact

- **Plugin 代码**: `SUnrealMcpServer.cpp` 的请求处理循环（`ProcessReceivedLines` 区域，约 L267 附近）需要在发送前插入大小检测逻辑。
- **协议**: 新增错误码 `RESPONSE_TOO_LARGE`，属于 envelope 层面的错误，不影响现有命令的 `error.code` 枚举。
- **CLI 侧**: 无代码改动。CLI 已经能处理 `ok:false` 响应，Agent 可根据 `error.code === "RESPONSE_TOO_LARGE"` 决定后续策略。
