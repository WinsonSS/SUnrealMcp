# SUnrealMcp

中文 | [English](#english)

SUnrealMcp 当前推荐的使用方式是：

- Unreal 侧使用 `UnrealPlugin/SUnrealMcp`
- Agent 侧使用 `Skill/sunrealmcp-unreal-editor-workflow` 内置的 CLI

也就是说，这个仓库现在的主方向是：

- `UnrealPlugin`
  负责在 Unreal Editor 内执行 command，并通过 TCP JSON 协议对外提供能力
- `Skill`
  负责告诉 Agent 何时进入 Unreal 工作流、如何选择命令类别、如何扩展能力、以及如何通过内置 CLI 连接目标编辑器

根目录里的 `McpServer/` 目前保留为迁移参考，不再是推荐主执行面。

## 当前结构

```text
SUnrealMcp/
├─ Skill/
│  └─ sunrealmcp-unreal-editor-workflow/
│     ├─ SKILL.md
│     ├─ SKILL_cn.md
│     ├─ cli/
│     │  ├─ package.json
│     │  ├─ bin/
│     │  └─ dist/
│     └─ references/
├─ UnrealPlugin/
│  └─ SUnrealMcp/
└─ McpServer/                  # legacy reference during migration
```

## CLI-First Workflow

当前推荐链路：

1. Agent 触发 `sunrealmcp-unreal-editor-workflow`
2. Skill 判断这是 Unreal Editor 任务
3. Skill 引导 Agent 使用内置 CLI
4. CLI 根据目标工程的插件和 ini 配置解析 host / port
5. CLI 通过 TCP JSON 调用 Unreal 插件 command
6. 如果能力不足，Agent 同时扩展 Unreal 插件和 CLI
7. 重编或 Live Coding 后继续原任务

这套模式的目标是：

- 不依赖 MCP tool discovery
- 同一会话能更快消费新增能力
- 允许多个 Unreal Editor 通过不同端口并行存在
- Skill 和 CLI 一起打包后可以直接迁移到 Agent 的 skills 目录

## 多编辑器支持

CLI 以 Unreal 工程为主要目标标识，并从工程里的 `SUnrealMcp` 配置解析端口。

关键设置节：

```ini
[/Script/SUnrealMcp.SUnrealMcpSettings]
BindAddress=127.0.0.1
Port=55557
```

默认规则：

- 优先传 `--project <path>`
- 由 CLI 自动检查插件是否安装
- 由 CLI 自动读取 ini 并解析 `BindAddress` 和 `Port`
- 只有必要时才手动传 `--host` / `--port`

## 内置 CLI

当前 skill 内置 CLI 位置：

```text
Skill/sunrealmcp-unreal-editor-workflow/cli/
├─ package.json
├─ bin/sunrealmcp-cli.ps1
└─ dist/sunrealmcp-cli.js
```

### 直接使用

Windows PowerShell:

```powershell
.\Skill\sunrealmcp-unreal-editor-workflow\cli\bin\sunrealmcp-cli.ps1 help
```

Node:

```bash
node ./Skill/sunrealmcp-unreal-editor-workflow/cli/dist/sunrealmcp-cli.js help
```

### Help 系统

CLI 自带帮助系统，方便 Agent 运行时校验，也方便手动调试：

```bash
sunrealmcp-cli help
sunrealmcp-cli help node
sunrealmcp-cli help node inspect-graph
sunrealmcp-cli help --json
```

## Skill 文档

主 Skill 文档位置：

- [SKILL.md](Skill/sunrealmcp-unreal-editor-workflow/SKILL.md)
- [SKILL_cn.md](Skill/sunrealmcp-unreal-editor-workflow/SKILL_cn.md)

命令参考文档按类别拆分在：

- [cli-command-catalog.md](Skill/sunrealmcp-unreal-editor-workflow/references/cli-command-catalog.md)
- [cli-discovery.md](Skill/sunrealmcp-unreal-editor-workflow/references/cli-discovery.md)
- [cli-system.md](Skill/sunrealmcp-unreal-editor-workflow/references/cli-system.md)
- [cli-editor.md](Skill/sunrealmcp-unreal-editor-workflow/references/cli-editor.md)
- [cli-blueprint.md](Skill/sunrealmcp-unreal-editor-workflow/references/cli-blueprint.md)
- [cli-node.md](Skill/sunrealmcp-unreal-editor-workflow/references/cli-node.md)
- [cli-umg.md](Skill/sunrealmcp-unreal-editor-workflow/references/cli-umg.md)
- [cli-project.md](Skill/sunrealmcp-unreal-editor-workflow/references/cli-project.md)
- [cli-raw.md](Skill/sunrealmcp-unreal-editor-workflow/references/cli-raw.md)

## Unreal Plugin

将 `UnrealPlugin/SUnrealMcp` 复制到你的 Unreal 项目：

```text
YourProject/
└─ Plugins/
   └─ SUnrealMcp/
```

插件设置类：

- [SUnrealMcpSettings.h](UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Public/SUnrealMcpSettings.h)

默认配置：

- `BindAddress = 127.0.0.1`
- `Port = 55557`
- `bAutoStartServer = true`
- `CompletedTaskRetentionSeconds = 300`

## 迁移状态

当前仓库已经完成这些重构方向：

- Skill 设计切到 CLI-first
- CLI 被内置到 skill 目录，并改成 TypeScript 工程
- CLI 入口改成扫描命令模块，family 元数据和 command 元数据一起动态注册
- 文档改成渐进式披露
- CLI 加入了分层 help 设计和首版实现
- 已先迁入用于验证架构的 `core` 命令面：`discovery`、`system`、`raw`

当前仍处于迁移中的部分：

- `McpServer/` 尚未移除
- `editor / blueprint / node / umg / project` 等类别的命令还会继续迁入同一套 TypeScript 命令模块架构
- 内置 CLI 还会继续替代剩余的老 MCP server 职责

## English

SUnrealMcp is currently moving toward a `skill-embedded CLI + Unreal plugin` architecture.

Recommended runtime path:

- `UnrealPlugin/SUnrealMcp` handles Unreal-side command execution over TCP JSON
- `Skill/sunrealmcp-unreal-editor-workflow` teaches the agent how to use the embedded CLI
- the embedded CLI resolves the target project and port, then talks directly to the plugin

The root `McpServer/` folder is still kept as a migration reference, but it is no longer the recommended primary execution surface.
