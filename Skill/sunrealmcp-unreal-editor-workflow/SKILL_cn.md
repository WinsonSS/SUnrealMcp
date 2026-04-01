# SUnrealMcp Unreal Editor 工作流

## 目的

这是 Unreal Editor 相关任务的主工作流。

只要任务需要与 Unreal Editor、UE 资产或编辑器对象交互，就默认先走这条流程，并优先使用 `SUnrealMcp`。

这个工作流只做三件事：

1. 先判断任务是不是 Unreal Editor 工作流
2. 只要是，就优先通过 `SUnrealMcp` 交互
3. 现有能力不够时，再扩展并收敛 `tool` / `command`

## 何时使用

出现下面任一情况时就使用这份 Skill：

- Blueprint、Widget Blueprint、AnimBP、Ability、Data Asset、Actor、Level、Input Mapping 等 UE 资产或编辑器对象需要被读取、检查、分析、修改、创建、编译或查询
- 任务需要理解 Unreal 编辑器里的图逻辑、对象状态、引用关系或元数据
- 任务明显属于 Unreal Editor 自动化，即使 Agent 还没有真正调用 `SUnrealMcp`

## 何时不要使用

以下情况默认不要走这份 Skill：

- 任务与 Unreal Editor 交互无关
- 目标只是普通文本或普通文件，而不是 UE 资产语义对象
- 用户明确要求只做分析，不要实际推进工作流
- 需要的是协议级或架构级重设计，而不是 `tool` / `command` 扩展或收敛

## 默认原则

### 1. UE 资产默认走 `SUnrealMcp`

一旦目标被确认是 Unreal 资产或编辑器对象，默认必须通过 `SUnrealMcp` 去交互。

这里的“交互”包括：

- 读取
- 检查
- 分析
- 查询引用
- 获取摘要或元数据
- 修改
- 创建
- 编译

下面这些默认不要求走 `SUnrealMcp`：

- `ini`、`json`、`toml`、`md` 等纯文本配置或文档
- `jpg`、`png`、`wav` 等与 UE 资产语义无关的普通文件
- 用户明确要求按普通文件处理

如果目标本质上是 Unreal 资产，但当前 `SUnrealMcp` 没有对应能力，默认先判断是否应该扩展 `tool` / `command`，而不是退回普通文件工作流。

### 2. 未知 Unreal 对象先判定类型

当用户只给出一个 Unreal 风格名称，但没有说明它是 Blueprint、C++ 类、数据资产、配置对象还是其他资源时：

- 先把它视为 `unknown Unreal object`
- 不要立刻假设它是 C++ 符号
- 不要立刻做无边界全仓库源码搜索

默认顺序：

1. 先判断它是不是 Unreal 资产、Blueprint、Widget、Ability、数据资产或其他编辑器对象
2. 再看直接引用、父类、生成类、图表入口或元数据线索
3. 只有这些都不够时，才转向源码搜索

如果已经确认它是 UE 资产，就立即切回 Unreal Editor 工作流，不再把它当普通文件或普通源码对象处理。

### 3. 普通文件搜索只是辅助

如果目标已经确认是 Blueprint、Widget Blueprint、AnimBP、Ability 资产或其他 Unreal 资产：

- `.uasset` 字符串提取
- 原始字节扫描
- 普通文本搜索

都只能作为辅助线索，不能作为主分析路径。

用户要求“分析逻辑”时，默认优先：

1. 检查 `SUnrealMcp` 是否已经具备读取、检查、摘要或图表分析能力
2. 如果没有，再判断是否应该扩一条对应能力

### 4. `SUnrealMcp` 默认先看 tool / command，不先看 resource

`SUnrealMcp` 当前默认应被视为以 `tool` / `command` 为主的 MCP 接口面，而不是以 `resource` / `resource template` 为主的接口面。

因此当任务已经进入 Unreal Editor workflow 时：

- 默认先检查当前有哪些 `tools` 能直接或间接解决问题
- 默认先检查插件实现里是否已经存在对应 `command`，只是尚未暴露
- 不要把 `list_mcp_resources` 或 `list_mcp_resource_templates` 当作 `SUnrealMcp` 的主入口

只有在已经有直接证据说明某个目标 MCP server 本身就是资源型 server，或者当前任务明确依赖 MCP `resource` / `resource template` 时，才优先走 `resource` 接口。

## 搜索边界与超时

### 默认边界

无论是对象定位还是能力确认，默认排除：

- `Intermediate`
- `Binaries`
- `Saved`
- `.vs`
- `DerivedDataCache`
- `node_modules`
- 无关插件
- 无关 MCP Server
- 外部仓库副本或镜像插件目录

### 默认超时

未知对象首次定位默认用低成本搜索，建议短超时，约 `8s`。

如果超时：

1. 不要继续盲搜
2. 优先请求能显著缩小范围的信息
3. 用户不给信息时，再做一次更大范围的兜底搜索，建议约 `20s`

超时后的默认动作是切换策略，而不是继续等待。

## 范围与 Source Of Truth

开始动手前，先识别：

- `McpServer` 所在目录
- 接进工程、实际参与编译和运行的 `SUnrealMcp` 插件目录
- 可能存在的外部仓库或镜像插件目录

默认只改工程内实际生效的那份插件副本，不默认同步外部仓库副本。

能力确认时，默认只检查这两个实现面：

- `McpServer/src/tools`
- `Plugins/SUnrealMcp/Source/SUnrealMcp` 下的 `Command` 实现和注册

只有出现直接证据指向别处，或用户明确说明 source of truth 在别处时，才扩大范围。

## 核心流程

### 1. 先尝试现有能力

先做这些检查：

- 当前任务是否属于 Unreal Editor 工作流
- 当前对象是否应视为 UE 资产或编辑器对象
- 现有 `SUnrealMcp` `tools` 能不能直接解决
- 插件里是否已经有对应 `Command`，只是名字不同或尚未暴露
- 是否能通过扩参数或 `mode` 解决，而不是新增 `tool`

### 2. 做缺口归因

把缺口归类为：

- `server_only`
- `plugin_only`
- `both_missing`
- `wrong_abstraction`

只围绕完成当前原始任务所需的最小能力来设计。

### 3. 选择最小改法

优先级顺序：

1. `reuse`
2. `extend`
3. `merge`
4. `deprecate` / `delete`
5. `add`

### 4. 修改、验证、刷新、回到原任务

默认顺序：

1. 改 `McpServer`
2. 改当前生效插件的 `command`
3. 重建并验证
4. 刷新 `tool discovery`
5. 回到原始任务继续做完

## Tool 生命周期与收敛

### 生命周期层级

- `core`
- `stable`
- `candidate`
- `temporary`

领域分类如 `Blueprint`、`Editor`、`Node`、`UMG`、`Project`、`System` 只是当前组织方式，不是固定 taxonomy，可以随着命令面演化而重组。

### 默认规则

默认不新增新 `tool`。

如果问题可以通过：

- 更好的提示
- 另一个已有 `tool`
- 小范围参数扩展
- 组合多个已有 `tools`

解决，就优先这么做。

### 收敛触发条件

出现以下任一情况时做收敛检查：

- 准备新增 `tool`
- 多个 `tools` 语义重叠
- 命名已经更像项目词而不是能力词
- 某个领域下的 `tools` 变得难以扫描
- 引入了 `temporary` 能力
- 原始任务完成后仍存在临时或重叠能力

### 收敛结论

每个候选 `tool` 必须显式决定：

- `keep`
- `merge`
- `deprecate`
- `delete`

收敛检查本身不等于直接升 `stable`。

默认晋升路径：

`temporary` -> `candidate` -> `stable`

## 实现约束

### McpServer

- `tool` 命名与现有家族风格一致
- 参数名稳定、清晰
- `schema` 足够强，但不过度约束
- `tool` 定义必须和真实插件 `command` 契约一致
- 不默认改 `protocol` / `transport` 骨架

### Unreal 插件

- `command` 字符串必须与 `McpServer` 期待的一致
- 请求解析、校验、响应结构与契约一致
- 优先做最小必要实现
- 遵循当前 `module` / `command` / `helper` 风格
- 错误信息应结构化、可行动

## 验证

### 至少要验证

- `McpServer` 能构建
- Unreal 插件或所属工程能编译
- 新增或修改后的 `command` 已注册
- Agent 能看到预期的 `tool surface`
- 至少有一次小的 `smoke call` 成功

### Editor 重编译

如果改动需要 Unreal Editor 重编译：

- 默认优先尝试 `Live Coding`
- 只有在 `Live Coding` 不可靠或不适用时，才升级到关闭 Editor 后完整重编

如果关闭 Editor / 重编 / 重开会打断用户当前会话，先和用户对齐。

## 完成标准

只有在这些条件都满足时，这个工作流才算完成：

- 原始用户任务已经完成，或只剩真实外部阻塞
- 两端实现保持一致
- `tool` 契约与插件 `command` 行为一致
- 刷新后的会话能看到正确的 `tool surface`
- 本轮临时或重叠能力已经做过收敛检查
- 必要的分类和收敛结论已经持久化或明确汇报
