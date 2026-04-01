# SUnrealMcp Unreal Editor 工作流

## 目的

凡是任务需要和 Unreal Editor 的内容、状态或资产交互时，都应优先把这份 Skill 视为主工作流。

默认预期是：

1. 先识别这个任务属于 Unreal Editor 自动化或编辑器交互任务。
2. 优先把 `SUnrealMcp` 作为和编辑器交互的默认接口层。
3. 如果当前 `SUnrealMcp` 能力足够，就直接使用。
4. 如果存在真实能力缺口，就补齐整条链路，再继续原始任务。
5. 如果 `tool` 面已经出现重复、过窄、临时性过强或陈旧问题，就在任务关闭前做收敛。

目标不是无限堆积 `tools`，而是在把 `SUnrealMcp` 维持为默认 Unreal Editor 自动化接口面的同时，让 `tool` 面保持小而强、可组合、可维护、可理解。

## 核心意图

遵循下面这个闭环：

1. 先识别用户任务是否需要和 Unreal Editor、Unreal 资产、Blueprint 图、UMG、Actor、项目设置或其他编辑器托管状态交互。
2. 默认优先使用 `SUnrealMcp` 作为第一交互面。
3. 先尝试用现有 `tools` 完成用户任务。
4. 识别是否真的存在能力缺口。
5. 决定应该 `reuse`、`extend`、`merge`、`deprecate`、`delete` 还是 `add`。
6. 只在最小必要范围内修改：
   - `McpServer`
   - 工程内实际生效的 `Plugins/SUnrealMcp`
7. 重建并验证。
8. 刷新 `tool` 视图。
9. 回到原始任务并做完。
10. 如果触发收敛条件，在关闭任务前完成一次 `tool` 收敛检查。

## 何时使用

当任务明显涉及 Unreal Editor 交互、Unreal 内容自动化，或需要对 Unreal 编辑器侧状态做读取、修改、创建、验证时，就使用这份工作流。

这也包括一种情况：Agent 还没有真正调用 `SUnrealMcp`，但按任务性质本来就应该优先考虑它。

典型触发条件：

- Blueprint 创建、修改、图编辑或图分析
- UMG Widget 创建、修改、绑定或视口相关工作
- Actor、关卡、编辑器世界对象的读取或修改
- Unreal 项目侧资产或输入配置修改
- 需要读取或修改编辑器托管资产或运行状态的任务
- `SUnrealMcp` 明显是最自然的接口层，即使当前还没暴露出精确所需的 `tool`

典型场景：

- 任务需要和 Unreal Editor 交互，但当前可见的 `SUnrealMcp` `tools` 还没有明显覆盖入口。
- Blueprint 工作流需要一个当前没有暴露出来的节点操作。
- `McpServer` 有 `tool` 概念，但插件没有对应的 `command`。
- 插件里有能力，但 `McpServer` 没暴露成 `tool`。
- 长任务需要 task-aware 的 `tool`，而当前没有这类能力。
- 多个 `tools` 已经语义重叠，应该合并或废弃。
- 某个为了解阻塞而临时加上的 `tool`，在任务结束前需要检查是否该收敛。

## 何时不要使用

以下情况不要使用：

- 任务本身根本不需要和 Unreal Editor 交互。
- 现有 `tools` 组合起来已经能解决问题。
- 只是参数传错、调用顺序不对、或者理解偏差。
- 需要的是协议级重构，而不是 `tool` / `command` 扩展或收敛。
- 用户明确要求只做分析，不要实际改代码。

## 范围

这份 Skill 允许不同项目使用不同目录结构。下面这些路径只是常见示例，不是硬编码要求：

- `McpServers/SUnrealMcp/McpServer`
- `McpServers/SUnrealMcp/UnrealPlugin/SUnrealMcp`
- `Plugins/SUnrealMcp`

开始动手前，先在当前工作区里识别这几个“逻辑目标”：

- `McpServer` 所在目录
- 接进工程、实际参与编译和运行的 `SUnrealMcp` 插件目录
- 可能存在的外部仓库或镜像插件目录

插件实现默认只改接进工程的那份，不默认改外部仓库副本。外部仓库副本保持不动，除非用户明确要求同步更新。

### 能力确认范围规则

当需要确认 `SUnrealMcp` 是否已经具备某个能力，只是还没有正确暴露出来时，默认只检查这两个实现面：

- `McpServer/src/tools`
- `Plugins/SUnrealMcp/Source/SUnrealMcp` 下的 `Command` 实现和 `Command` 注册

把它们视为能力确认时默认且通常足够的目标：也就是 `McpServer` 里的 `tools`，以及当前生效 `SUnrealMcp` 插件里的 `Command` 实现或注册。

不要为了确认能力是否存在，就默认把搜索范围扩大到项目里的其他目录。

尤其不要扫描这些无关或高成本目录：

- `Binaries`
- `Intermediate`
- `Saved`
- `.vs`
- `node_modules`
- 无关插件
- 无关 MCP Server
- 外部仓库副本或镜像插件目录

只有在用户明确说明当前生效的 `SUnrealMcp` 实现在别处，或者在这些默认的 `tools` / `Command` 位置里已经出现直接证据指向另一个 source of truth 时，才扩大检查范围。

### Source Of Truth 规则

如果工程里同时存在外部仓库副本和本地激活插件副本，默认不要两边一起改。

默认顺序如下：

1. 改“接进工程、实际参与编译”的那份激活插件副本。
2. 外部仓库或镜像插件副本保持不动，除非用户明确要求一起更新。
3. 只有当用户明确要求时，才同时同步两份。

也就是说：

- 工程内激活插件是默认实现目标。
- 外部仓库插件是手动同步目标。

### 领域分类不是固定 Taxonomy

`Blueprint`、`Editor`、`Node`、`UMG`、`Project`、`System` 等目录或家族名，默认只视为当前工作区里方便理解和维护的组织方式。

不要把它们当成不可变的长期 taxonomy。

默认稳定的是生命周期层级：

- `core`
- `stable`
- `candidate`
- `temporary`

领域分类本身可以随着命令面演化而重组、合并、拆分或重命名，只要这样做能让能力边界更清晰、收敛质量更高、维护成本更低。

## 核心流程

### 1. 先尝试现有能力

在新增任何东西之前，先做这些检查：

- 默认把能力确认搜索限制在 `McpServer/src/tools`，以及当前生效插件 `Plugins/SUnrealMcp/Source/SUnrealMcp` 下的 `Command` 实现或注册。
- 枚举和当前任务相关的 MCP `tools`。
- 判断是否可以通过组合现有 `tools` 解决。
- 判断插件里是否已经有类似 `Command`，只是命令名不同或尚未暴露成 MCP `tool`。
- 判断是否能通过给现有 `tool` 增加参数或 `mode` 来解决，而不是新增一个全新的 `tool`。

不要因为“新增一个看起来方便”就直接加。
不要把这一步执行成整个工作区范围的仓库扫描。

### 2. 做缺口与收敛归因

把缺失能力或收敛目标归类为以下之一：

- `server_only`
  插件侧概念上能做，但 `McpServer` 没暴露出来。
- `plugin_only`
  `McpServer` 的 `tool` 形状可以有，但插件没有对应 `command`。
- `both_missing`
  两边都缺。
- `wrong_abstraction`
  栈里已有相关能力，但抽象太窄、太碎、重叠，或者已经陈旧。

只围绕完成原始用户任务所需的最小能力来设计，不要顺手做成一个大框架，除非当前任务真的需要。

### 3. 选择最小且正确的改法

优先级顺序：

1. 直接复用已有 `tool`。
2. 向现有 `tool` 兼容性地扩展参数或 `mode`。
3. 把两个或多个过窄的 `tools` 合并成一个更通用的 `tool`。
4. 如果现有能力已经覆盖其价值，则废弃或删除陈旧 `tool`。
5. 上述都不够时，再新增一个 `tool` 和匹配的 `command`。

如果确实要新增 `tool`，需要满足：

- 作用域小
- 名字清晰
- 与现有 `tool` 家族风格一致
- 光看名字就容易被发现和理解

## Tool Hygiene 与收敛策略

`tool` 太多会降低 Agent 的选择质量。每一次 `tool` 变更，都同时是能力决策和维护决策。

### Tool 生命周期层级

`core`

- 在很多任务里都通用
- 是其他工作流的基础
- 一般不是删除候选

`stable`

- 已经被多个独立任务反复证明有价值
- 命名、`schema` 和行为已经稳定到可以作为长期能力面的一部分
- 需要被当作长期支持的 `tool` / `command` 来维护

`candidate`

- 已经通过至少一次收敛检查，证明不应立即删除
- 可能在多个后续任务里复用，但还不够证实为长期稳定能力
- 是 `temporary` 升向 `stable` 前的观察层，而不是永久层

`temporary`

- 主要是为了给当前任务解阻塞
- 比较窄、比较项目化，或者未来很可能被替换
- 必须在任务结束前或原任务完成后立刻复查
- 默认不承诺长期存在

### 默认规则

默认不新增新的 `tool`。

如果问题可以通过以下方式解决：

- 更好的提示
- 使用另一个已有 `tool`
- 小范围参数扩展
- 组合多个已有 `tools`

那就优先这么做。

### 收敛触发条件

当出现以下任意情况时，执行一次收敛检查：

- 准备新增一个 `tool`
- 多个 `tools` 在语义上重叠
- 某个 `tool` 的命名已经更像项目名，而不是能力名
- 某个领域下的 `tools` 多到难以扫描
- 当前任务里引入了临时 `tool`
- 原始任务已经完成，而这轮新增或修改了临时能力
- 某个 `tool` 实际上已经只是另一个 `tool` 或一段流程的薄包装

### 收敛决策规则

当触发条件成立时，必须显式决定每个候选 `tool` 应该是：

- `keep`
- `merge`
- `deprecate`
- `delete`

不要把这个决定只留在临时推理里。

收敛检查的职责，是判断某个能力是否应该继续存在，以及应以什么形式继续存在。

收敛检查本身不等于“可以直接升为 `stable`”。一次收敛最多只能说明：

- 不该继续保留
- 应该被合并
- 应该被废弃
- 可以继续保留观察

如果一个 `temporary` `tool` 通过了收敛检查，默认先进入 `candidate`，而不是直接进入 `stable`。

### Tool Inventory 规则

`tool` 的分类和收敛结论不能只停留在短期上下文里。

当某个任务新增、合并、废弃或删除了 `tool` 时，如果工作区里已经有面向 `SUnrealMcp` 的 `tool inventory`，就把决定记进去。优先使用名为 `tool-inventory.md` 的文件，并尽量放在持久化 `tool` 定义附近，或放在 `SUnrealMcp` 根目录下。如果没有，则执行以下二选一：

- 如果当前任务本来就在改那个应该长期保存 `tool` 定义的仓库，就加一个最小 `inventory` 文件。
- 否则就在最终总结里明确写出分类和收敛建议。

至少要持久化这些信息：

- `tool` 名称
- 分类：`core`、`stable`、`candidate`、`temporary`
- 动作：`keep`、`merge`、`deprecate`、`delete`
- 简短原因

推荐使用的最小记录格式：

```md
- tool: <name>
  classification: <core|stable|candidate|temporary>
  action: <keep|merge|deprecate|delete>
  reason: <short reason>
```

### 收敛检查问题

对每个候选 `tool`，至少问这些问题：

- 未来其他任务还会不会需要它？
- 这个名字是不是更像项目特定词，而不是能力名？
- 现有 `tools` 组合起来能不能替代它？
- 它是不是只是在包装一串几乎没有新增价值的调用？
- 两端维护它的成本是不是已经大于收益？
- 移除或合并它后，未来 Agent 的 `tool` 选择是否会更清晰？
- 它的输入输出 `schema` 和行为是否已经稳定，而不是下轮很可能继续改？
- 它是否只是在这一次任务里碰巧有用，而不是已经跨任务体现出复用价值？
- 当前验证是否只是“这次跑通”，还是已经有可信的 `smoke path`？

如果答案体现出低复用、高重叠、高维护成本，就优先考虑 `merge`、`deprecate` 或 `delete`，而不是 `keep`。

如果答案只能说明“这次值得留下继续观察”，但还不足以说明“应纳入长期能力面”，就把它放进 `candidate`，而不是 `stable`。

### 晋升规则

默认晋升路径：

`temporary` -> `candidate` -> `stable`

#### `temporary` -> `candidate`

至少同时满足：

- 本次收敛检查没有得出 `merge`、`deprecate` 或 `delete`
- 它和现有 `stable` / `candidate` 能力没有明显语义重叠
- 它不是只为当前单一任务硬编码出来的任务脚本
- 它已经有最基本的验证路径，至少存在可信的 `smoke test`

#### `candidate` -> `stable`

至少同时满足：

- 已在多个相互独立的任务里反复证明有价值
- 命名已经抽象到能力层，而不是项目层或任务层
- 输入输出 `schema` 与行为已经基本稳定
- `McpServer` `tool` 与 Unreal 插件 `command` 契约已经清晰且不太可能继续频繁改动
- 维护者愿意把它视为长期支持面的一部分，而不是继续观察

如果缺少这些证据，就继续保持 `candidate`，不要因为“一次看起来有用”就升到 `stable`。

### 允许的收敛结果

优先顺序：

1. 能 `merge` 就优先 `merge`
2. 不能立刻删但已经可疑时，用 `deprecate`
3. 只有在没有预期依赖时，才 `delete`

不要随意删除或重命名广泛使用的 `tools`。

### 任务结束前收敛规则

只要本轮任务新增或实质性修改了 `tool`，在关闭任务前都要做一次最终收敛检查。

至少要做：

- 复查本轮涉及的所有 `temporary` `tools`
- 复查本轮涉及的所有 `candidate` `tools` 是否满足晋升条件
- 复查同一家族里语义重叠的 `tools`
- 决定每个候选项是 `keep`、`merge`、`deprecate` 还是 `delete`
- 如有需要，决定是否执行 `temporary -> candidate` 或 `candidate -> stable`
- 把结果持久化或显式汇报给用户

## McpServer 侧规则

修改 `McpServer` 时：

- 命名要与现有 Blueprint、Editor、Node、Project、UMG 等 `tool` 家族保持一致
- 参数名要稳定、清晰
- `schema` 要足够强，能指导 agent，但不要无端过度约束
- `tool` 定义必须和真实插件 `command` 契约一致
- 当 `tool` 发生合并、废弃或删除时，要考虑收敛过程中的迁移安全
- 除非任务明确要求，否则不要碰 `protocol` 和 `transport` 骨架

默认允许改动的 server 侧范围：

- `tool definitions`
- `runtime tool helpers`
- `tool registration`
- 与新增能力或收敛改动直接相关的 `tests`

默认不要修改这些骨架级部分：

- `transport framing`
- `request/response envelope` 结构
- `connection lifecycle`
- `protocol versioning`

如果某个 `tool` 返回 `task handle`，要确保 `runtime helper` 行为和响应预期仍然匹配。

## Unreal 插件侧规则

修改 Unreal 插件时：

- `command` 字符串必须和 `McpServer` 期待的一致
- 请求解析、校验、响应结构要与 MCP server 契约保持一致
- 优先做最小必要实现，而不是顺手抽象出大框架
- 遵循现有 `SUnrealMcp` 的 `module` / `command` / `helper` 风格
- 错误信息要结构化、可行动
- 当进行 `tool` 收敛时，要在迁移完成前保留或显式桥接必要的插件侧依赖

默认允许改动的插件侧范围：

- `command registration`
- `command implementation`
- 直接支撑该 `command` 的 `helper utilities`
- 该 `command` 需要的模块依赖更新

默认不要随意重构这些骨架级部分：

- `server skeleton`
- `task registry model`
- 核心 `transport` 行为

如果某个 `command` 会编辑资产或 Blueprint 图，要确保它真的完成了 `tool` 描述里承诺的行为，而不是只返回一个“看起来成功”的响应。

## 验证流程

实现完成后，先验证，再回到原始任务。

### McpServer 验证

通常至少运行：

```powershell
cd <McpServer directory>
npm run build
```

如果改动区域有相关测试，也要一起跑。

### Unreal 插件验证

使用当前工作区对应的 Unreal 构建流程来编译插件或所属工程。

至少确认：

- 代码能编译
- 模块依赖正确
- 新增或修改后的 `command` 已注册

### Editor 重编译协调规则

当新的或修改后的 `tool` / `command` 需要重编译 Unreal Editor Target 时，不要假设可以无条件打断当前 Editor 会话。

如果 Editor 可能正在被用户使用，先询问用户是否希望 Agent：

1. 自动关闭 Editor
2. 触发所需编译
3. 重新打开 Editor
4. 在 Editor 恢复后继续原始任务

以下情况应优先走这个协调后的重编译流程：

- 必须关闭当前 Editor 进程，编译才会成功
- 不重启 Editor，更新后的插件或模块无法被可靠加载
- 原始任务依赖重编译并重新启动后的 Editor 会话才能继续

如果用户批准这条流程，就把它视为同一个连续任务的一部分，而不是停止点。重新打开 Editor 后，继续完成验证、刷新受影响的运行时假设，并恢复原始任务。

如果用户不批准自动打断 Editor，会在安全交接点停止，说明还需要哪些重编译或重启步骤，不要强制关闭 Editor。

### 运行时冒烟验证

在宣布扩展成功前，至少确认：

- MCP server 能启动
- Agent 能看到新增、修改、废弃或合并后的 `tool`
- 一个小的 `smoke call` 可以成功
- 如果涉及 `task` 流程，`status` 路径仍然可用

### Tool Refresh 规则

修改 `tool` 定义后，不要假设当前 Agent 会自动理解新的 `tool surface`。

把刷新视为单独步骤：

1. 重建 `server`
2. 如果 MCP server 已在运行，就重启它
3. 确保 host 重新连接到 `server`
4. 重新做一次 `tool discovery`
5. 在回到原任务前，明确告诉 Agent 哪些 `tool` 被新增、移除、合并、废弃或修改了

以下情况尤其重要：

- 新增了 `tool`
- `tool` 名字没变，但参数变了
- 参数没变，但行为变了
- 某个 `tool` 被合并、废弃或删除了

当前 Agent 可能仍携带旧的 `tool` 记忆。如果没有显式刷新，它可能继续基于过时假设行动。

对于会缓存或延迟更新 `tool discovery` 的 host，可采用这个实用刷新顺序：

1. 重建 `server`
2. 重启 MCP server 进程
3. 重新连接 host
4. 启动一次新的 `tool discovery`
5. 带着一段简短 `reorientation note` 继续原始任务

如果当前会话无法可靠刷新 `tool` 视图，就把它当作一种 `workflow limitation`，改用新会话或显式重连，而不是默认系统会隐式刷新。

### Tool Change 策略

优先做低意外、低惊讶的增量改动。

优先：

- 新增一个命名清晰的新 `tool`，而不是偷偷改变旧 `tool` 的含义
- 以向后兼容方式扩展 `schema`
- 避免让旧 `tool` 名承载完全不同的行为
- 当未来调用方可能还存在时，优先 `merge` 或 `deprecate`，再考虑 `delete`

如果某个现有 `tool` 必须发生 breaking change，就把它视为一个需要显式刷新和重新定向的工作流，而不是静默替换。

## 回到原始任务

这个 Skill 不是因为“新 `tool` 编译通过了”就算完成。

验证完成后，必须立刻回到原始用户任务，继续用新能力把原任务真正做完。

不要停在这些状态：

- “`tool` 已经加好了”
- “插件现在能编译了”
- “你现在可以手动调用这个 `tool` 了”

只有当原始任务完成，或者只剩真实外部阻塞时，这个工作流才算成功。

## 失败处理

只要失败还在 Agent 可控范围内，就不要第一次失败就停下。

默认行为：

- 如果 `McpServer` `build` 失败，就修 server 侧问题再重建
- 如果 Unreal 插件编译失败，就修插件侧问题再重建
- 如果两端都能编译，但 `smoke test` 失败，就排查契约不匹配并继续迭代
- 如果新能力可用了，但原任务还没完成，就继续原任务
- 如果收敛后暴露出重叠或陈旧 `tools`，就继续迭代，直到 `keep`、`merge`、`deprecate` 或 `delete` 结论足够明确

只有在以下情况才停止并升级：

- `blocker` 在工作区之外
- 需要的改动已经演变成协议或架构重设计
- `source of truth` 不明确，贸然猜测风险过高
- 当前 host 无法提供这个工作流依赖的 `refresh` / `reconnect`
- 需要重编译，但这会打断用户当前正在使用的 Editor 会话，而用户尚未批准自动关闭和重新打开

## 升级规则

以下情况先暂停并和用户对齐：

- 需要协议级重设计
- 多个被广泛使用的 `tools` 必须重命名或删除
- 正确的 `source of truth` 不清楚
- 这次改动会带来远超当前任务的维护后果
- 所需的 Editor 重编译可能会打断用户当前正在使用的 Unreal Editor 会话

但不要因为要加一个小的 `tool` / `command` 对，或者因为一个明显过期的临时 `tool` 应该被 `deprecate`，就停下来。正常路径是做最小正确改动，完成收敛检查，然后继续推进。

## 执行清单

每当出现能力缺口或收敛触发条件时，按下面执行：

1. 重述原始任务，以及缺失能力或重叠 `tool`
2. 检查现有 `tool` 是否已经能解决
3. 决定 `keep`、`reuse`、`extend`、`merge`、`deprecate`、`delete` 或 `add`
4. 将受影响的每个 `tool` 分类为 `core`、`reusable` 或 `temporary`
5. 在 `McpServer` 做最小必要改动
6. 在 Unreal 插件做匹配的 `command` 改动
7. 重建 `McpServer`
8. 如果重建 Unreal 插件或工程可能打断当前 Editor 会话，先询问是否自动关闭 Editor、编译、重新打开并继续
9. 重建 Unreal 插件或工程
10. 如果走了协调后的 Editor 重编译路径，在继续前重新打开 Editor 并恢复任务流程
11. 对新能力或收敛后的能力做一次 `smoke test`
12. 刷新 `tool discovery`，并明确让 Agent 重新理解新的 `tool surface`
13. 回到并完成原始任务
14. 对临时或重叠 `tools` 做一次任务结束前收敛检查
15. 持久化或汇报分类和收敛结论

## 完成标准

只有在以下条件都满足时，这个工作流才算完成：

- 原始用户任务已经完成，或者只剩真实外部 `blocker`
- 新增、修改、合并、废弃或删除的 MCP 能力，已经在两端保持一致实现
- `tool` 契约与插件 `command` 行为一致
- 刷新后的会话能看到预期的 `tool surface`
- 本轮涉及的所有临时或重叠 `tools` 都已经做过收敛复查
- 必要的分类和收敛结论已经持久化或显式汇报
- 用户收到了一份简明总结，说明：
  - 缺失或重叠的能力是什么
  - 新增、修改、合并、废弃或删除了什么
  - 如何完成验证
  - 是否还有临时项或后续清理工作
