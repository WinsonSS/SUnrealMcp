# SUnrealMcp

中文 | [English](#english)

SUnrealMcp 是一个面向 Unreal Engine 的双端 MCP 集成方案，由两部分组成：

- `UnrealPlugin/SUnrealMcp`
  UE5 编辑器插件，负责在 Unreal Editor 内执行命令并通过 TCP JSON 协议对外暴露能力
- `McpServer`
  Node.js 编写的 MCP Server，负责连接插件并把 Unreal 能力重新发布为 MCP tools

两部分配合后，MCP 客户端就可以通过标准工具调用驱动 Unreal Editor，例如：

- 查询和编辑关卡中的 Actor
- 创建和编译 Blueprint
- 编辑 Blueprint 组件和部分属性
- 在 Blueprint 图里添加节点、连接节点
- 创建和编辑 UMG Widget
- 发起任务、轮询状态、取消任务

## 仓库结构

```text
SUnrealMcp/
├─ McpServer/                  # Node.js MCP Server
│  ├─ src/
│  │  ├─ index.ts              # Server 启动入口
│  │  ├─ protocol.ts           # 请求/响应协议定义
│  │  ├─ transport/            # 与 Unreal 插件通信的 TCP 客户端
│  │  ├─ runtime/              # Tool 注册辅助逻辑
│  │  └─ tools/                # MCP Tool 定义
│  └─ tests/
├─ UnrealPlugin/
│  └─ SUnrealMcp/              # Unreal Editor 插件
│     ├─ Source/SUnrealMcp/Public/Mcp/
│     └─ Source/SUnrealMcp/Private/
└─ README.md
```

## 功能概览

当前仓库已经覆盖的核心能力包括：

- `Editor` 类工具
  获取 Actor、按 Label 查找、生成 Actor、删除 Actor、修改 Transform、读取和设置属性
- `Blueprint` 类工具
  创建 Blueprint、添加组件、设置静态网格、设置物理属性、编译 Blueprint、修改默认对象属性
- `Node` 类工具
  添加事件节点、输入节点、函数节点、变量节点、Self/组件引用节点、连接节点、查找节点
- `UMG` 类工具
  创建 Widget Blueprint、添加文本块、添加按钮、绑定事件、设置文本绑定、将 Widget 加入视口
- `Project` 类工具
  Enhanced Input Mapping Context 映射写入
- `Task` 支持
  `get_task_status`、`cancel_task`

## 工作流程

整体调用链如下：

1. MCP 客户端调用 `McpServer` 暴露的 tool
2. `McpServer` 将 tool 调用转换为协议请求
3. 请求通过 TCP 发送到 Unreal 插件
4. Unreal 插件的 Command Registry 分发到对应命令实现
5. 结果返回给 `McpServer`
6. `McpServer` 再将结果返回给 MCP 客户端

## 部署 Unreal 插件

1. 将 `UnrealPlugin/SUnrealMcp` 整个目录复制到你的 Unreal 项目 `Plugins/` 下

```text
YourProject/
└─ Plugins/
   └─ SUnrealMcp/
```

2. 打开 Unreal 项目
3. 让 Unreal 自动编译，或者手动编译 Editor Target
4. 在 Unreal 的设置中确认插件配置

插件设置类定义见：
[SUnrealMcpSettings.h](UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Public/SUnrealMcpSettings.h)

默认配置：

- `BindAddress = 127.0.0.1`
- `Port = 55557`
- `bAutoStartServer = true`
- `CompletedTaskRetentionSeconds = 300`

## 部署 McpServer

进入 `McpServer/` 后执行：

```bash
npm install
npm run build
node ./build/index.js
```

`McpServer` 会读取：

- `config.json`
- `config.local.json`

默认配置在 [index.ts](McpServer/src/index.ts) 中定义，包括：

- host: `127.0.0.1`
- port: `55557`
- timeout: `5000ms`
- task timeout: `30000ms`

## 推荐启动顺序

1. 启动 Unreal Editor，并确保插件已启用
2. 确认插件侧 TCP Server 正常监听
3. 构建并启动 `McpServer`
4. 在 MCP 客户端中连接 `build/index.js`
5. 先调用 `ping` 验证链路

## 如何接入 Agent

这个仓库里的 `McpServer` 不是直接给人手动操作的 UI，而是给支持 MCP 的 Agent / Client 使用的。

典型接入方式是：

1. 先完成 Unreal 插件部署
2. 在 `McpServer/` 下执行：

```bash
npm install
npm run build
```

3. 在你的 MCP Client 或 Agent 配置中，把这个 server 注册进去
4. 让客户端启动 `McpServer/build/index.js`

### 接入 Codex

如果你使用的是 Codex，可以在 Codex 的 MCP 配置里添加一个 server 条目，命令指向 Node，参数指向构建后的入口文件。

示例：

```toml
[mcp_servers.SUnrealMcp]
command = "node"
args = ["D:/path/to/SUnrealMcp/McpServer/build/index.js"]
```

如果你不想在配置里写绝对路径，也可以按你自己的环境改成相对路径或启动脚本，但核心原则是不变的：

- Agent 启动的是 `McpServer/build/index.js`
- `McpServer` 再去连接 Unreal 插件监听的 TCP 地址

### 在 Codex 中部署 `sunrealmcp-unreal-editor-workflow` Skill

如果你希望 Codex 在遇到 Unreal Editor 交互任务时，默认优先走 `SUnrealMcp` 工作流，并在存在真实能力缺口时继续扩展 MCP 栈、在合适条件下收敛 `tool`，可以把这个 skill 部署到 Codex 的 skills 目录。

当前仓库中的 skill 位置：

```text
.codex/skills/sunrealmcp-unreal-editor-workflow/
├─ SKILL.md
└─ SKILL_cn.md
```

典型部署方式：

1. 将整个 `sunrealmcp-unreal-editor-workflow` 目录复制到 Codex 的 skills 根目录
2. 保持目录名不变
3. 确认目标目录下至少存在 `SKILL.md`
4. 重启 Codex，或重新加载 skills

以 Windows 上的 Codex 默认目录为例：

```text
C:\Users\<YourUser>\.codex\skills\sunrealmcp-unreal-editor-workflow\
├─ SKILL.md
└─ SKILL_cn.md
```

如果你当前就在这个仓库里使用 Codex，也可以直接复用仓库内的 skill 副本，前提是你的 Codex 环境会扫描该 skills 路径。

这个 skill 主要在下面这类场景触发：

- 任务需要和 Unreal Editor、Blueprint、UMG、Actor、关卡或其他编辑器托管状态交互
- `SUnrealMcp` 是默认应优先考虑的交互面
- 现有 `tools` 足够时直接使用，不足时继续扩展
- 目标是继续完成任务，而不是停在分析或只给手工方案

部署完成后，Codex 的行为目标会变成：

- 先识别这是不是 Unreal Editor 工作流任务
- 默认优先尝试 `SUnrealMcp`
- 先尝试复用现有 `tools`
- 必要时同时改 `McpServer` 和 Unreal 插件
- 重建、验证、刷新 `tool discovery`
- 回到原始任务继续完成
- 在满足条件时，对临时、候选或重叠 `tools` 做收敛检查和生命周期复查

### 通用 MCP Client 接入原则

对于其他支持 MCP 的客户端，通常也只需要提供以下信息：

- executable: `node`
- arguments: `[".../McpServer/build/index.js"]`
- working directory: 可选，通常设为 `McpServer/`

### 接入前提

要让 Agent 真正可用，还需要同时满足：

- Unreal Editor 已启动
- `SUnrealMcp` 插件已启用
- 插件 TCP server 已正常监听
- `McpServer` 的 host / port 与 Unreal 插件配置一致

如果 Agent 已经能看到 `ping`、`get_actors_in_level` 之类的工具，说明接入链路已经打通。

## 如何扩展 Tool

如果你要增加一个新的端到端能力，通常需要同时改 `McpServer` 和 Unreal 插件两边。

### 1. 在 McpServer 侧增加 Tool

位置：

- `McpServer/src/tools/*.ts`

注册入口辅助函数：

- [tool_helpers.ts](McpServer/src/runtime/tool_helpers.ts)

通常需要定义：

- `name`
- `description`
- `inputSchema`
- 可选的 `mapParams`
- 可选的 `executionMode: "task"`
- 可选的 `taskOptions`

示例：

```ts
registerCommandTool(server, context, {
    name: "my_command",
    description: "Do something in Unreal",
    inputSchema: {
        asset_path: z.string(),
    },
});
```

### 2. 在 Unreal 插件侧增加 Command

位置：

- `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/...`

你需要：

1. 新建一个实现 `ISUnrealMcpCommand` 的类
2. 在 `GetCommandName()` 中返回与 MCP 侧一致的命令名
3. 在 `Execute(...)` 中实现编辑器逻辑
4. 通过 `FSUnrealMcpCommandAutoRegistrar` 自动注册

示例骨架：

```cpp
class FMyCommand final : public ISUnrealMcpCommand
{
public:
    virtual FString GetCommandName() const override
    {
        return TEXT("my_command");
    }

    virtual FSUnrealMcpResponse Execute(
        const FSUnrealMcpRequest& Request,
        const FSUnrealMcpExecutionContext& Context) override
    {
        return FSUnrealMcpResponse::MakeSuccess(Request.RequestId, MakeShared<FJsonObject>());
    }
};
```

### 3. 如果是异步任务型能力

插件侧：

- 实现 `ISUnrealMcpTask`
- 交给 `FSUnrealMcpTaskRegistry` 管理
- 返回带 `taskId` 的 accepted 响应

Server 侧：

- 在 tool 定义中设置 `executionMode: "task"`
- 默认通过 `get_task_status` / `cancel_task` 管理任务生命周期

## 协议说明

`McpServer` 与 Unreal 插件之间使用的是：

- TCP
- UTF-8 JSON
- 以换行符分隔的消息
- 包含 `protocolVersion`、`requestId`、`ok` 的请求/响应 envelope

核心协议定义位置：

- [McpServer/src/protocol.ts](McpServer/src/protocol.ts)
- [SUnrealMcpProtocol.h](UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Public/Mcp/SUnrealMcpProtocol.h)

## 测试与验证

常用验证方式：

- 在 `McpServer/` 下执行 `npm run build`
- 在 `McpServer/` 下执行 `npm test`
- 在 Unreal 项目中编译插件
- 调用 `ping`
- 调用一个简单的编辑器工具，例如 `get_actors_in_level`

## 常见问题

### `npm run build` 失败

优先检查：

- Node.js 版本
- 依赖是否安装完整
- tool 文件 import 路径是否正确
- 运行的是源码目录还是构建产物

### MCP Server 启动了但无法连接 Unreal

检查：

- 插件是否正确加载
- 插件 TCP Server 是否已启动
- 双方 host / port 是否一致
- 本地防火墙或端口占用问题

### McpServer 有 tool，但 Unreal 返回 `UNKNOWN_COMMAND`

通常说明：

- 插件端没有对应命令实现
- 或者插件命令 `GetCommandName()` 与 MCP tool 实际映射名不一致

## 贡献建议

- 尽量把协议层改动控制在最小范围
- 新能力优先以新增 command/tool 的方式扩展，而不是修改基础 envelope
- 改业务能力时，同时检查：
  - MCP tool 定义
  - Unreal Command 实现
- 如果新增 task 型能力，要一起验证 accepted/status/cancel/timeout

## License

见 [LICENSE](LICENSE)。

---

## English

SUnrealMcp is a two-part Unreal Engine integration for the Model Context Protocol:

- `UnrealPlugin/SUnrealMcp`: a UE5 editor plugin that exposes Unreal editor capabilities over a lightweight TCP JSON protocol
- `McpServer`: a Node.js MCP server that connects to the plugin and republishes those capabilities as MCP tools

Together, these two parts allow an MCP client to drive Unreal Editor workflows such as actor editing, Blueprint editing, UMG creation, node wiring, task polling, and project-side input setup.

## Repository Layout

```text
SUnrealMcp/
├─ McpServer/
│  ├─ src/
│  │  ├─ index.ts
│  │  ├─ protocol.ts
│  │  ├─ transport/
│  │  ├─ runtime/
│  │  └─ tools/
│  └─ tests/
├─ UnrealPlugin/
│  └─ SUnrealMcp/
│     ├─ Source/SUnrealMcp/Public/Mcp/
│     └─ Source/SUnrealMcp/Private/
└─ README.md
```

## Features

The current stack includes:

- actor queries and editing in the current editor level
- Blueprint creation, compilation, component editing, and selected property updates
- Blueprint graph operations such as adding nodes and wiring pins
- UMG widget creation and editing
- task-style command support with polling and cancellation
- typed MCP tool definitions built with `@modelcontextprotocol/sdk`

## Deployment

### Unreal Plugin

1. Copy `UnrealPlugin/SUnrealMcp` into your Unreal project's `Plugins/` folder.
2. Open the project in Unreal Editor.
3. Let Unreal compile the plugin, or build the editor target manually.
4. Verify the plugin settings such as bind address, port, auto-start, and task retention.

Default values are defined in:
[SUnrealMcpSettings.h](UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Public/SUnrealMcpSettings.h)

### MCP Server

Inside `McpServer/`:

```bash
npm install
npm run build
node ./build/index.js
```

Configuration is loaded from:

- `config.json`
- `config.local.json`

Default server values are defined in:
[index.ts](McpServer/src/index.ts)

## Recommended Bring-Up Order

1. Start Unreal Editor with the plugin enabled.
2. Confirm the plugin TCP server is running.
3. Build and start `McpServer`.
4. Connect your MCP client to the built server entrypoint.
5. Call `ping` first.

## Connecting an Agent

`McpServer` is meant to be consumed by an MCP-capable client or agent rather than used directly by hand.

Typical setup flow:

1. deploy the Unreal plugin
2. run `npm install` and `npm run build` inside `McpServer/`
3. register the built server in your MCP client / agent configuration
4. let the client launch `McpServer/build/index.js`

### Connecting Codex

For Codex, add an MCP server entry that launches Node with the built server entrypoint.

Example:

```toml
[mcp_servers.SUnrealMcp]
command = "node"
args = ["D:/path/to/SUnrealMcp/McpServer/build/index.js"]
```

You can adapt the path to your own machine, use a wrapper script, or use a relative path if your environment supports it. The important part is that the agent launches the built MCP server entrypoint.

### Deploying the `sunrealmcp-unreal-editor-workflow` Skill in Codex

If you want Codex to continue when `SUnrealMcp` has a real capability gap, extend the MCP stack, and converge temporary or overlapping tools when appropriate, deploy the skill into Codex's skills directory.

Skill location in this repository:

```text
.codex/skills/sunrealmcp-unreal-editor-workflow/
├─ SKILL.md
└─ SKILL_cn.md
```

Typical deployment flow:

1. Copy the entire `sunrealmcp-unreal-editor-workflow` directory into Codex's skills root.
2. Keep the folder name unchanged.
3. Ensure `SKILL.md` exists in the deployed folder.
4. Restart Codex or reload skills.

Example Windows destination:

```text
C:\Users\<YourUser>\.codex\skills\sunrealmcp-unreal-editor-workflow\
├─ SKILL.md
└─ SKILL_cn.md
```

If you are already using Codex inside this repository, you may also reuse the in-repo copy as long as your Codex environment scans that skills path.

The skill is meant to activate when:

- Codex is using `SUnrealMcp` for an Unreal automation task
- existing tools are not enough to finish the task
- the gap is real rather than a misuse issue
- the expectation is to continue and finish the task instead of stopping at analysis

After deployment, the intended behavior is:

- try existing tools first
- update both `McpServer` and the Unreal plugin only when needed
- rebuild, validate, and refresh tool discovery
- return to the original task and finish it
- run convergence review for temporary or overlapping tools when applicable

### Generic MCP Client Setup

For most MCP-capable clients, the required pieces are:

- executable: `node`
- arguments: `[".../McpServer/build/index.js"]`
- working directory: optional, usually `McpServer/`

### Preconditions

For the agent connection to work end-to-end:

- Unreal Editor must already be running
- the `SUnrealMcp` plugin must be enabled
- the plugin TCP server must be listening
- the `McpServer` host / port settings must match the Unreal plugin settings

If the agent can see tools such as `ping` or `get_actors_in_level`, the MCP integration is wired up correctly.

## Extending Tools and Commands

To add a new end-to-end capability, you usually need changes on both sides.

### Add a Tool in `McpServer`

Files:

- `McpServer/src/tools/*.ts`

Helper:

- [tool_helpers.ts](McpServer/src/runtime/tool_helpers.ts)

Typical fields:

- `name`
- `description`
- `inputSchema`
- optional `mapParams`
- optional `executionMode: "task"`
- optional `taskOptions`

### Add a Command in the Unreal Plugin

Files:

- `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/...`

Steps:

1. Implement `ISUnrealMcpCommand`
2. Return the wire name in `GetCommandName()`
3. Implement editor logic in `Execute(...)`
4. Register it through `FSUnrealMcpCommandAutoRegistrar`

### Add a Task-Based Capability

Plugin side:

- implement `ISUnrealMcpTask`
- enqueue it through `FSUnrealMcpTaskRegistry`
- return an accepted response with `taskId`

Server side:

- mark the tool with `executionMode: "task"`
- use `get_task_status` / `cancel_task` by default, or override through `taskOptions`

## Protocol

Transport between `McpServer` and the Unreal plugin is:

- TCP
- UTF-8 JSON
- newline-delimited messages
- request/response envelopes carrying `protocolVersion`, `requestId`, and `ok`

Core protocol files:

- [McpServer/src/protocol.ts](McpServer/src/protocol.ts)
- [SUnrealMcpProtocol.h](UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Public/Mcp/SUnrealMcpProtocol.h)

## Validation

Useful checks:

- `npm run build`
- `npm test`
- compile the Unreal plugin inside a UE project
- call `ping`
- call a simple editor-facing tool such as `get_actors_in_level`

## Troubleshooting

### `npm run build` fails

Check:

- Node.js version
- dependency installation state
- tool module import paths
- whether you are running source files or built output

### The MCP server starts but cannot reach Unreal

Check:

- plugin load state
- plugin TCP server startup
- matching host/port values
- firewall or local socket restrictions

### A tool exists in `McpServer` but Unreal returns `UNKNOWN_COMMAND`

That usually means:

- the plugin side does not implement the command yet
- or `GetCommandName()` does not match the effective MCP command name

## Contributing

- Keep protocol-layer changes minimal unless compatibility requires more
- Prefer extending via new commands/tools instead of changing the core envelope
- When changing a capability, review both the MCP tool definition and the Unreal command implementation
- For task-style features, validate accepted/status/cancel/timeout together

## License

See [LICENSE](LICENSE).

