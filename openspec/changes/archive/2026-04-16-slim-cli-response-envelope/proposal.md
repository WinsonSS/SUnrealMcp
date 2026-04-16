## Why

CLI 的主要消费者是 Agent。当前 `executeCommand` 每次成功返回的 JSON 包含 7 个顶层字段（`ok`、`target`、`cli`、`unreal`、`data`、`error`、`raw`），其中 `raw` 完整复制了 Unreal 原始响应（`raw.data` 与外层 `data` 重复），`target` 包含 Agent 自己发送请求时已知的连接参数，`cli` 回显 Agent 自己输入的命令元数据。每条成功响应的 token 开销接近 2 倍实际有效载荷。对于 Agent 对话场景，这些冗余字段既浪费 token 又增加解析噪音。

## What Changes

- **BREAKING** 默认响应信封精简为 `{ok, data}` （成功）或 `{ok, error}` （失败），移除 `target`、`cli`、`unreal`、`raw` 四个顶层字段。
- 新增 `--verbose` 全局选项，启用后响应恢复包含 `target`、`cli`、`unreal`、`raw` 等诊断字段，供人工调试使用。
- 失败响应在非 verbose 模式下仅保留 `{ok, error}`，其中 `error` 保持现有结构不变。

## Capabilities

### New Capabilities
- `slim-envelope`: 定义默认精简信封格式与 `--verbose` 扩展信封格式的响应结构规范。

### Modified Capabilities
- `core-command-wire-protocol`: 响应信封结构变更，需要更新协议层面对返回值格式的约定。

## Impact

- **CLI 代码**: `cli/src/index.ts` 的 `executeCommand` 函数需重构返回值构建逻辑；`parseOptions` 及全局选项定义需新增 `--verbose`。
- **CLI 类型**: `cli/src/types.ts` 中的 `CommandResult` / 相关接口需要更新。
- **消费者兼容性**: Agent prompt / SKILL.md 中对响应格式的描述需同步更新。任何依赖 `raw`、`target`、`cli`、`unreal` 字段的外部脚本将 **BREAKING**。
