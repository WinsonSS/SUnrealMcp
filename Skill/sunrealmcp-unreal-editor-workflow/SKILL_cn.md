# SUnrealMcp Unreal Editor 工作流

## 目的

这份 Skill 用于所有需要和 Unreal Editor、UE 资产、编辑器对象真实交互的任务。

当前版本是 `CLI-first`：

- 不再把 MCP tool discovery 当作主执行面
- 默认通过 Skill 内置 CLI 去驱动安装了 `SUnrealMcp` 插件的 Unreal Editor

当前实现状态：

- 内置 CLI 已经是 Skill 目录里的真实 TypeScript 工程
- 目前先迁入了用于验证架构的 `core` 命令面
- 其他类别命令后续继续按同一套命令模块结构逐步迁入

Skill 负责流程和决策。
CLI 负责连接、发命令、查端口、返回结果。

CLI 架构本身也应是模块驱动的：

- 类别元数据和命令元数据一起从命令模块里注册
- CLI 入口只负责扫描和加载模块
- 不要把 `system`、`editor`、`node` 这类类别名硬编码在骨架里

## 内置 CLI

优先入口：

- Windows PowerShell：
  `cli/bin/sunrealmcp-cli.ps1`
- 直接 Node 入口：
  `cli/dist/sunrealmcp-cli.js`

## 何时使用

满足下面两个条件时就使用这份 Skill：

- 任务和某个 UE 工程有关
- 任务需要和 Unreal Editor、UE 资产或编辑器托管对象交互

常见场景：

- Blueprint、Widget Blueprint、AnimBP、Data Asset、Actor、Level、Component、图节点、Widget 绑定
- 读取图语义、编辑器元数据、对象状态
- 创建、修改、编译、检查 UE 资产
- 同时操作多个 Unreal Editor，只要它们使用不同端口

## 何时不要用

以下情况默认不要走这份 Skill：

- 任务只是普通源码、文档、配置或构建脚本修改
- 目标不需要 Unreal Editor 语义
- 用户明确要求只做静态分析，不要实际和编辑器交互

## 核心规则

### 1. Unreal 任务默认走内置 CLI

只要任务需要 Unreal Editor 交互，就不要默认走原始文件分析，也不要走 `SUnrealMcp` 的 MCP resource 路线。

默认先走内置 CLI。

### 2. `SUnrealMcp` 没有 MCP resources

不要对 `SUnrealMcp` 调用：

- `list_mcp_resources`
- `list_mcp_resource_templates`

这套流程的前提是：

- Unreal 侧能力在插件 command 系统里
- Agent 侧执行面在 CLI 里

### 3. 资产语义请求必须用资产语义读取来回答

如果用户问的是 Blueprint 逻辑、图结构、Widget 绑定、编辑器元数据：

- 不要把 `.uasset` 字符串提取当主答案
- 不要把外围 C++ 引用关系当主答案
- 不要停在外围推断

如果当前 CLI 和插件读不到这些语义，就把它归类成“缺能力”，然后去补。

### 4. 缺能力是主流程的一部分

如果当前 CLI 命令面做不完任务：

- 判断缺的是哪个 Unreal command 或 CLI wrapper
- 补最小能力
- 重编或 Live Coding
- 然后继续原任务

不要停在“能力已经补好了”。
必须回到原任务继续做完。

## 渐进式披露

主 Skill 保持精简。

默认读取顺序：

1. 先读本文件，获取工作流规则
2. 再读 [references/cli-command-catalog.md](./references/cli-command-catalog.md)，决定该看哪个类别文档
3. 默认只读一个类别文档，除非任务确实跨多个类别

不要默认把所有类别文档都读进上下文。

## 命令路由

把 [references/cli-command-catalog.md](./references/cli-command-catalog.md) 当作命令路由索引。

它负责告诉 Agent：

- 有哪些命令类别
- 每个类别是干什么的
- 下一步应该读哪个类别文档

每个类别文档内部再按下面这套生命周期分层：

- `core`
- `stable`
- `candidate`
- `temporary`

当前已经真正落地到 CLI 的命令面以 `core` 为主：

- `discovery`
- `system`
- `raw`

其他类别目前保留为目标扩展结构，后续继续按同样的 TypeScript 命令模块架构补进去。
而且这些类别在真实 CLI 里也应该和命令一起通过模块注册进来，而不是写死在入口代码里。

## 目标工程与端口发现

CLI 需要支持多个 Unreal Editor。

默认规则：

1. 优先传 `--project <path>`
2. 让 CLI 去检查该工程是否安装了 `SUnrealMcp`
3. 让 CLI 去读取 ini 里的 `BindAddress` 和 `Port`
4. 只有必要时才手动指定 `--host` 和 `--port`

设置节固定为：

```ini
[/Script/SUnrealMcp.SUnrealMcpSettings]
```

如果自动发现出了多个候选目标，而任务本身又没有明确指出具体工程，就和用户对齐。

## 执行闭环

### 1. 固定原任务

先把用户真正要完成的任务固定成一句话。

后面的能力补全，只是为了把这件事做完。

### 2. 解析目标工程和编辑器

优先按工程定位，而不是只按端口定位。

让 CLI 去确认：

- 工程里有没有 `SUnrealMcp` 插件
- 哪些配置文件在生效
- 当前编辑器应该监听哪个 host 和 port

### 3. 选择命令类别

先看命令路由索引，再加载对应类别文档。

当前类别有：

- `discovery`
- `system`
- `editor`
- `blueprint`
- `node`
- `umg`
- `project`
- `raw`

### 4. 先用成熟能力

在某个类别里默认这样选：

1. 基础设施类先看 `core`
2. 正式能力优先看 `stable`
3. 不够再看 `candidate`
4. `temporary` 只作为过桥能力

### 5. 缺能力就补

如果现有 CLI 命令不够：

- 查 Unreal 插件 command 实现
- 查 CLI 命令面
- 补最小 Unreal command 和最小 CLI wrapper

这个仓库里的默认 source of truth：

- Unreal 插件：
  `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands`
- Skill 内置 CLI：
  `Skill/sunrealmcp-unreal-editor-workflow/cli`

### 6. 重编、重载、继续

补完能力后：

- 能 Live Coding 就优先 Live Coding
- 不行就完整重编
- 必要时 reload command registry
- 然后立刻继续原任务

如果新的 Unreal command 已经有了，但更漂亮的 CLI wrapper 还没补完，就先用 CLI 的 `raw send` 作为当前会话的临时桥接。

### 7. 以原任务完成为结束

只有原任务完成了，这个工作流才算完成。

## CLI Help

CLI 本身也必须可自解释，方便 Agent 运行时校验和你手动调试。

常见用法：

- `sunrealmcp-cli help`
- `sunrealmcp-cli help <family>`
- `sunrealmcp-cli help <family> <command>`
- `sunrealmcp-cli help --json`

文档是主指导面。
CLI help 是运行时校验和调试面。

## 完成标准

只有下面这些都满足时，这个工作流才算完成：

- 原始用户任务已经完成，或只剩真实外部阻塞
- Agent 确实走了内置 CLI，而不是漂移到无关路径
- 正确的 Unreal 工程和端口已经被解析出来，或者被显式覆盖
- 本轮所需的缺失能力已经在正确层级补上
- 资产语义类请求，答案来自真实的 Unreal 语义读取，而不是外围推断
