## Context

`FSUnrealMcpCommandRegistry` 目前只对外提供 `RegisterCommand`、`Execute`、`GetRegistrationErrorSummary` 三条 API，没有「遍历当前已注册命令」的出口；命令 handler 拿到的 `FSUnrealMcpExecutionContext` 只持有 `TaskRegistry`，没有访问注册表的合法路径。CLI 侧 `CliCommandExecutionContext`（命令 handler 在运行时拿到的上下文）也没有注册表的引用，因此 CLI-side 的 `execute` handler 目前读不到自己注册了哪些命令。本 change 要把这两条路径都打通，并在此基础上引入两个目标各自最小的 core 命令。

需要说明这条通道的用户不是「运行时的 Agent」——它以 CLI help 为准。真正的消费者是：

1. **Agent 扩展命令时的重名检查**：一次只查一个目标名字。
2. **维护者 debug 双侧漂移**：关心的是差集，不是全量清单。

两个场景收益差异明显，合并成一个 `list_commands` 会让两边都不舒服。因此拆分。

## Goals / Non-Goals

**Goals:**
- 提供 `check_command_exists`：只查单个名字的存在性，返回布尔。
- 提供 `diff_commands`：从 CLI 一次发起，拿回「CLI 有而 Unreal 无」+「Unreal 有而 CLI 无」两个数组。健康状态下两个都为空。
- 建立基础通道：`FSUnrealMcpExecutionContext.CommandRegistry` + `CliCommandExecutionContext.registry`（或等效），未来类似的 meta 命令可以复用。

**Non-Goals:**
- 不做一个笼统的「列全部命令」命令。理由见 proposal。
- 不返回 `RegistrationErrors` 或命令描述 / 参数签名。
- 不在 `diff_commands` 响应里返回「两边共有」那一项——它的大小等于整个注册表，违背「只输出差集」的初衷，且消费方用不到。
- 不对「raw 命令透传」场景做命令名补全：CLI 侧 `raw/send` 的 `unrealCommand` 是运行时参数，不是静态注册项；采集 CLI 名单时跳过该族。

## Decisions

### Decision 1：拆成两条命令，而不是一条大命令

替代方案：
- (a) 单一 `list_commands` + 客户端/消费方自己做过滤或 diff；
- (b) 拆成 `check_command_exists` + `diff_commands`。

选 (b)。(a) 对两个用例都不匹配：AI 查重名时得拉回一整张表再做 `includes`，浪费；人类 debug 时看一张表也比看差集累。拆开之后每个命令的响应大小和语义都是目标匹配的。

### Decision 2：`diff_commands` 在 CLI 侧自动打包本地清单

替代方案：
- (a) `diff_commands` 在 Unreal 侧直接返回全量 `commands` 数组，CLI / Agent 自己和本地对比；
- (b) CLI 命令 handler 运行时从本地 `CommandRegistry` 收集所有 `unrealCommand` 名字，作为 `cli_names` 参数发给 Unreal；Unreal 侧做差集并返回。

选 (b)。(a) 的响应大小 = 全量注册表，违背「只返回差集」的目标；(b) 健康状态下响应 `{only_in_unreal:[], only_in_cli:[]}` 极小。

代价：需要给 CLI 的命令 handler 一条读取本地注册表的路径，见 Decision 3。

### Decision 3：`CliCommandExecutionContext` 加 `registry: CliCommandRegistry`

替代方案：
- (a) 把 `CliCommandRegistry` 通过模块级闭包捕获到 `execute` handler 里（`register(registry)` 里建闭包）；
- (b) 在 `CliCommandExecutionContext` 里显式加 `registry` 字段；
- (c) 拓宽 `CliCommandRegistry` 接口加 `getAllCommandNames()` 方法。

选 (b) + 为 `CliCommandRegistry` 接口追加 `getAll()` 方法（目前 `CommandRegistry` 类已有，只是接口里没暴露）。闭包方案 (a) 能工作但是把 diff_commands 的 handler 和注册顺序耦合在一起，不如显式 context 清晰；纯 (c) 不够——context 里还是拿不到 registry。

### Decision 4：`FSUnrealMcpCommandRegistry` 同时提供 `HasCommand` 与 `ListCommandNames`

替代方案：
- (a) 只加 `ListCommandNames`，`check_command_exists` 的实现自己 `Contains`；
- (b) 同时提供两个访问器。

选 (b)。查单个存在性时不必构造整个 `TArray<FString>` 副本再查找——直接 `Commands.Contains(Name)` O(1)。代码层面 `HasCommand` 是一行 wrapper，没成本。两个访问器语义独立、都直接。

### Decision 5：`diff_commands` 响应字段名

替代方案：
- (a) `{added: [...], removed: [...]}`（像 diff 工具）；
- (b) `{only_in_unreal: [...], only_in_cli: [...]}`。

选 (b)。"added/removed" 要先看版本前后谁是基准才知道方向，`only_in_X` 零歧义，消费方一眼就知道这是哪边独有。

### Decision 6：采集 CLI 名单时跳过 `raw` 族

`raw/send` 的 `unrealCommand` 是运行时参数不是静态字段。把它纳入 `cli_names` 会导致 `only_in_cli` 永远多出一项假信号，或者 CLI 想不出合理名字只能塞空。直接在采集时 `family !== "raw"` 过滤掉。其它 family 的命令若没定义 `unrealCommand`（理论上不应该）也一并跳过。

## Risks / Trade-offs

- [`FSUnrealMcpExecutionContext` 加字段破坏所有 struct 聚合初始化点] → 项目里构造 context 的地方只有 `SUnrealMcpServer.cpp` 一处。Mitigation：tasks 里显式标注该位置。
- [`CliCommandRegistry` 接口拓宽一个 `getAll` 方法] → 目前实现类已经有该方法，只是接口里没有。拓宽之后所有实现者（当前只有 `CommandRegistry` 一个）必须实现。现状没有其它实现者，低风险。Mitigation：同时更新 types.ts 与 registry.ts。
- [`diff_commands` 如果 Unreal 侧未注册 `reload_command_registry` 就跑，结果会把 `reload_command_registry` 列在 `only_in_cli`] → 这是正确的诊断输出，不是 bug。但需要在 SKILL.md 文档化「先保持注册表最新」这一点。本 change 不要求同步改 SKILL.md。
- [CLI 侧 `diff_commands` 产生的 `cli_names` 可能含重复（理论上不会，因为 `registerCommand` 拒绝重复）] → 保险起见 CLI 在发送前去重并排序。

## Migration Plan

1. 先写 spec。
2. Unreal 侧按依赖顺序：
   - `ISUnrealMcpCommand.h`：`FSUnrealMcpExecutionContext` 加字段 + 前向声明。
   - `SUnrealMcpCommandRegistry.h/.cpp`：加 `ListCommandNames` 与 `HasCommand`。
   - `SUnrealMcpServer.cpp`：构造 context 时补传 `CommandRegistry.ToSharedRef()`。
   - 新增 `CheckCommandExistsCommand.cpp` 与 `DiffCommandsCommand.cpp`。
3. CLI 侧：
   - `types.ts`：`CliCommandRegistry` 接口追加 `getAll()`；`CliCommandExecutionContext` 追加 `registry`。
   - `index.ts`：`executeCommand` 构造 context 时传 `registry`。
   - `system_commands.ts`：注册两条命令，`diff_commands` 用 `execute:` 自定义 handler。
4. 无 wire 版本位或 feature flag。

## Open Questions

无。
