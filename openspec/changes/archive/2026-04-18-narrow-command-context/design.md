## Context

当前实现里，execution context 更像“内部对象转发器”而不是“命令可观察的宿主视图”：

- Unreal 侧 `FSUnrealMcpExecutionContext` 把 `TaskRegistry`、`CommandRegistry` 两个 `TSharedRef` 直接作为 public 字段暴露。
- CLI 侧 `CliCommandExecutionContext` 把 `registry` 直接暴露给所有 `execute:` handler。
- `reload_command_registry` 没走 context，直接 lookup module。

这导致 command 层知道了过深的内部结构。更麻烦的是，后续如果真的把 Unreal runtime owner 从 `FSUnrealMcpModule` 迁到 `Subsystem`，所有直接摸 module / registry 的命令都会跟着动。

这次不预支未来架构，只按当前需求收口：command 只能通过 `Context` 的显式方法获取能力。

## Goals / Non-Goals

**Goals:**
- Unreal / CLI 两侧统一成“command 只依赖 context 方法”的模型。
- 只提供当前命令真正需要的最小能力，不预先设计大而全的 `Catalog` / `Services` / `Host`。
- 不让 command 在执行期直接拿到 `Registry` / `Module` / `Subsystem` / `Owner`。
- 保持现有 wire 协议和用户可见命令行为不变。

**Non-Goals:**
- 不在本 change 中把 `FSUnrealMcpModule` 重构成空壳 + `Subsystem` runtime owner。
- 不引入通用 service locator，也不把所有潜在未来能力都塞进 `Context`。
- 不改 `check_command_exists` / `diff_commands` / task 命令的外部请求或响应结构。

## Decisions

### Decision 1：顶层名字继续叫 `Context`

替代方案：
- (a) 改成 `Catalog`
- (b) 改成 `Services` / `Host`
- (c) 继续使用 `Context`

选 (c)。当前调用形式已经是 `Execute(Request, Context)` / `execute(context)`，迁移成本最低，也最符合“命令执行时可观察的宿主视图”。`Catalog` 对当前需求太窄，因为除了命令查询，还要承载任务与 reload；`Services` / `Host` 又太宽，容易被当成新的 god object。

### Decision 2：Unreal `Context` 直接暴露最小方法，而不是继续公开底层对象

替代方案：
- (a) 保留 `Context.CommandRegistry` / `Context.TaskRegistry`
- (b) 在 `Context` 上公开 `Commands` / `Tasks` / `Runtime` 子 view
- (c) `Context` 直接提供当前命令需要的方法

选 (c)。当前能力面很小，直接方法最清楚，也最能避免命令继续穿透到底层 owner。选 (b) 只有在方法数量明显增多时才有收益；现在做会把结构复杂度提早引入。选 (a) 正是这次要消掉的问题。

当前 Unreal 侧最小方法面按现有命令需求收口为：

- `bool HasCommand(const FString& Name) const`
- `TArray<FString> ListCommandNames() const`
- `FSUnrealMcpResponse GetTaskStatusResponse(const FString& RequestId, const FString& TaskId) const`
- `FSUnrealMcpResponse CancelTask(const FString& RequestId, const FString& TaskId)`
- `FString EnqueueTask(const TSharedRef<ISUnrealMcpTask>& Task)`
- `FSUnrealMcpResponse MakeAcceptedTaskResponse(const FString& RequestId, const FString& TaskId) const`
- `bool ReloadCommandRegistry(FString* OutError = nullptr)`

其中后四项不是纯只读，但都是已有命令已经在做的显式动作；把它们收敛成命名方法，仍然明显优于暴露整个 owner。

### Decision 3：CLI `Context` 直接提供 `listRegisteredUnrealCommands()`，不再公开 `registry`

替代方案：
- (a) 保留 `registry` 字段
- (b) 在 `register(registry)` 时让 `diff_commands` 闭包捕获 registry
- (c) 给 `Context` 加一个面向意图的方法 `listRegisteredUnrealCommands()`

选 (c)。`system diff_commands` 的真实需求不是“获得 registry 本体”，而是“获得当前 CLI 静态注册的 Unreal 命令名数组”。直接暴露意图方法可以把过滤 `raw`、读取 `unrealCommand`、去重排序这些内部细节都藏在 context 里。闭包方案 (b) 虽然可行，但把 handler 和注册期状态绑得过紧，不如 context 明确。

### Decision 4：`Context` 内部持有 private 引用 / 回调，不复制大块 owner 数据

替代方案：
- (a) 每次构造 context 时拷贝一整份命令表 / 状态快照
- (b) `Context` private 持有必要引用或回调，对外只暴露方法

选 (b)。当前构造点只需组装少量句柄，不存在真正的“复制一大堆东西”问题。private 引用 / 回调既能保持轻量，也能把权限边界收回到方法级别。`Context` 的对外 contract 是方法集合，不是它内部怎么拿到这些数据。

### Decision 5：`reload_command_registry` 通过 `Context.ReloadCommandRegistry()` 访问 runtime owner

替代方案：
- (a) 维持 `ReloadCommandRegistryCommand` 里直接 `FModuleManager::GetModulePtr`
- (b) 把完整 module / subsystem 暴露给 command
- (c) 由 `Context` 提供一个显式 reload 方法，内部再调用 owner

选 (c)。这样 command 不再依赖 runtime owner 的具体类型；未来不管 reload 最终落在 `Module` 还是 `Subsystem`，命令侧都不需要知道。选 (a) 会继续把 command 和 `FSUnrealMcpModule` 绑死；选 (b) 则把边界问题重新带回来。

### Decision 6：这次明确不做 `Module -> Subsystem` 提取

替代方案：
- (a) 在同一个 change 里同时修 context 边界和 runtime owner 架构
- (b) 先只修 command/context 边界，owner 重组后续再做

选 (b)。把 `FSUnrealMcpModule` 逻辑迁移到 `Subsystem` 是合理方向，但它会牵涉 plugin 生命周期、editor 启停时机、console command 注册点等额外问题；当前用户要解决的是“command 不该拿到 registry / module 本体”，两者不是同一个 blast radius。先把边界修干净，再决定 owner 放哪层。

## Risks / Trade-offs

- `FSUnrealMcpExecutionContext` 会从“数据 struct”变成“有行为的小对象”，头文件复杂度上升。
- 任务相关方法直接返回 `FSUnrealMcpResponse`，会让 context 保留一些协议层味道；但当前这些响应本就由 `TaskRegistry` 负责生成，继续复用比让命令重复拼装 JSON 更稳。
- 如果后续命令大量增长，`Context` 方法数也会跟着增长。Mitigation：只有当方法开始显著分组时，再拆 `Commands` / `Tasks` / `Runtime` 子 view。
- 暂不做 subsystem 提取意味着 `ReloadCommandRegistry` 的内部实现短期内仍可能调用 `FSUnrealMcpModule`，只是这层耦合被藏到 context 内部，不再暴露给 command。

## Migration Plan

1. 更新 OpenSpec：把 `command-registry-introspection` 对 context 暴露 registry 的要求改成方法边界。
2. Unreal 侧先改 `FSUnrealMcpExecutionContext`：
   - 去掉 public `CommandRegistry` 字段；
   - 以 private 成员 + public 方法封装命令查询、任务、reload；
   - `SUnrealMcpServer` 构造 context 时注入所需引用 / 回调。
3. 更新依赖这些能力的 Unreal 命令：
   - `CheckCommandExistsCommand`
   - `DiffCommandsCommand`
   - `GetTaskStatusCommand`
   - `CancelTaskCommand`
   - `ReloadCommandRegistryCommand`
   - `StartMockTaskCommand`
4. CLI 侧移除 `registry` 暴露：
   - `CliCommandExecutionContext` 改成 `listRegisteredUnrealCommands()`
   - `index.ts` 构造该方法
   - `system diff_commands` 改用该方法
5. 自验证：
   - `openspec validate 2026-04-18-narrow-command-context`
   - CLI `tsc`
   - Unreal 插件编译
   - 相关命令冒烟

## Open Questions

无。
