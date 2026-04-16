## Context

当前 `executeCommand`（`cli/src/index.ts:136-152`）在每次成功调用后返回一个 7 字段的 JSON 信封：

```jsonc
{
  "ok": true,
  "target": { /* projectRoot, pluginPath, host, port, configSources, timeoutMs */ },
  "cli": { "family": "…", "command": "…", "lifecycle": "…" },
  "unreal": { "command": "…", "requestId": "…", "params": {…} },
  "data": {…},
  "error": undefined,
  "raw": { /* Unreal 原始响应，raw.data === data */ }
}
```

`raw.data` 完整复制了外层 `data`，`target` / `cli` / `unreal.params` 全部是 Agent 自己已知的信息。对于 Agent 消费场景，这些字段纯粹浪费 token。

CLI 是 one-shot 进程（每次命令新建 TCP 连接），输出直接打到 stdout 供 Agent 读取，因此信封格式只影响 CLI 侧代码，不涉及 Unreal Plugin 侧改动。

## Goals / Non-Goals

**Goals:**

- 默认信封只包含 `{ok, data}` 或 `{ok, error}`，将 Agent 每次调用的 token 开销减至最低。
- 提供 `--verbose` 全局选项，人工调试时可恢复完整诊断信息。
- 保持 `ok` / `data` / `error` 的语义与当前完全一致，Agent 侧无需改变读取逻辑。

**Non-Goals:**

- 不改变 Unreal 侧 TCP 响应格式（wire protocol 不受影响）。
- 不调整 help 输出格式（属于 punch list #3 的范畴）。
- 不引入响应压缩或分页（属于 punch list #4）。

## Decisions

### D1: 默认精简，`--verbose` 还原

默认输出 `{ok, data}` / `{ok, error}`。`--verbose` 启用后追加 `target`、`cli`、`unreal`、`raw` 四个字段。

**为什么不选 opt-out（默认全量 + `--slim`）**：CLI 的主要消费者是 Agent，Agent 永远不需要诊断字段，因此 opt-in 冗余（`--verbose`）比 opt-out 精简（`--slim`）更合理——忘记加 `--slim` 就会持续浪费 token，而忘记加 `--verbose` 只影响人工调试体验。

### D2: 在 `executeCommand` 末尾做字段裁剪，不改变中间变量

`executeCommand` 内部继续构建完整的 result 对象（方便调试、日志），最后根据 `global.verbose` 决定输出哪些字段。这比在构建过程中就分支处理更简单，也更容易回退。

**为什么不在 `main` 的 `JSON.stringify` 层做裁剪**：`executeCommand` 的返回类型是公开接口（`CliCommandDefinition.execute` 的自定义命令也走这条路），在函数内部裁剪可以保证所有路径一致，不依赖调用方记得裁剪。

### D3: `--verbose` 作为 `CliGlobalOptions` 的布尔字段

在 `CliGlobalOptions` 接口中新增 `verbose: boolean`（默认 `false`），与 `--pretty`、`--timeout` 同级。`parseOptions` 识别 `--verbose` 时将其设为 `true`。

**为什么不用环境变量 `SUNREALMCP_VERBOSE`**：CLI 是 Agent 驱动的 one-shot 进程，Agent 没有设置环境变量的方式；`--verbose` 作为显式命令行参数更透明。

### D4: `error` 字段在精简模式下保持原结构

失败时 `error` 对象的 `{code, message, details}` 结构不变。Agent 需要 `error.code` 来做条件重试，裁剪 error 内部字段没有收益。

## Risks / Trade-offs

- **BREAKING 变更** → 任何依赖 `raw`、`target`、`cli`、`unreal` 字段的外部脚本会立即失效。缓解：当前唯一已知消费者是 Agent prompt，同步更新 SKILL.md 即可；在 CHANGELOG 中标注 BREAKING。
- **调试体验退化** → 默认输出不再包含 `requestId` 和 `target`，排查连接问题时需要记得加 `--verbose`。缓解：CLI 的 `--help` 输出中提示 `--verbose` 的存在。
