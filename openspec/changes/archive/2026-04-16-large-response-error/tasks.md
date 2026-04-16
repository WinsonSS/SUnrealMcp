## 1. 响应大小检测与错误替换

- [x] 1.1 在 `SUnrealMcpServer.cpp` 的 `ProcessReceivedLines` 中，`SendResponseLine` 调用前，计算序列化响应的 UTF-8 字节数，超限时构造 `RESPONSE_TOO_LARGE` 错误响应（含 `error.details.response_bytes` 和 `error.details.limit_bytes`）替代原响应发送
- [x] 1.2 `error.message` 中包含触发超限的命令名（从 `Request.Command` 获取）
