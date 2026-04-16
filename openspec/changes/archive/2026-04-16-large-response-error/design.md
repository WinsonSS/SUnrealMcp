## Context

`FSUnrealMcpServer::ProcessReceivedLines` 在收到请求后执行命令、序列化响应、调用 `SendResponseLine` 发送。`SendResponseLine` 内部检查 `(QueuedBytes + IncomingBytes) > Config.MaxPendingSendBytesPerConnection`，超限时返回 `false`，调用方只打日志，连接随后被清理。Agent 侧表现为 socket hang up。

当前 `SendResponseLine` 是在已经序列化完成后、将字节追加到发送缓冲区时才做检测。这意味着失败时，整条响应已经被 JSON 序列化，只是没来得及发出去。

## Goals / Non-Goals

**Goals:**

- Agent 在响应过大时收到结构化的 `RESPONSE_TOO_LARGE` 错误（包含实际字节数和限制值），而非 broken pipe。
- 错误响应本身体积可控，不会二次触发溢出。

**Non-Goals:**

- 不实现响应分页/游标机制（属于 punch list 长期项目，本次只解决诊断性问题）。
- 不改变 `MaxPendingSendBytesPerConnection` 的默认值或可配置性。
- 不改动 CLI 侧代码。

## Decisions

### D1: 在 `ProcessReceivedLines` 中序列化后、发送前做大小检测

在调用 `SendResponseLine` 之前，先用 `FTCHARToUTF8` 计算序列化后的字节数。超限时构造一条 `RESPONSE_TOO_LARGE` 错误响应替代原响应发送。

**为什么不在 `SendResponseLine` 内部处理**：`SendResponseLine` 是通用的发送方法，职责是"把给定的行写到 socket"。让它在失败时自行构造并发送替代错误会打破单一职责，且可能导致递归调用。在调用方处理更清晰。

### D2: 错误响应的 `details` 携带诊断数据

`error.details` 中包含 `response_bytes`（实际字节数）和 `limit_bytes`（配置的上限），让 Agent 能据此判断超限程度并决定后续策略（如请求分页、缩小查询范围）。

**为什么不只给 message**：纯文本 message 需要 Agent 做字符串解析才能获取数值，结构化 `details` 更可靠。

### D3: 不关闭连接，正常发送错误响应

`RESPONSE_TOO_LARGE` 错误响应本身只有几百字节，远低于 8 MiB 限制。发送后连接保持正常状态（尽管 CLI 是 one-shot 进程，连接也即将关闭）。不需要额外的"发送错误后强制断开"逻辑。

**为什么不断开连接**：CLI 是 one-shot 的，每条命令一个 TCP 连接，发完响应后连接自然结束。强制断开只会增加复杂度。

## Risks / Trade-offs

- **序列化两次的开销** → 超限时需要先序列化原始响应、检测大小、再序列化错误响应。缓解：超限是异常路径，正常响应只序列化一次（大小检测用的是同一次序列化结果的字节数）。
- **`response_bytes` 暴露内部信息** → Agent 能知道响应的精确字节数。这是有意为之的——Agent 需要这个信息来决策。
