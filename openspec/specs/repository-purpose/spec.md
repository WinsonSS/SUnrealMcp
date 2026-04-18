# repository-purpose Specification

## Purpose
定义 `SUnrealMcp` 这个仓库在仓库级别上的定位、目标与使用边界，供后续 Agent 在进入 Unreal 工作流前快速恢复背景，而不需要每次依赖用户重复口头说明。

## Requirements

### Requirement: 仓库定位为 Agent-first 的 Unreal Editor 工作流骨架

`SUnrealMcp` SHALL 被视为一套面向 Agent 的 Unreal Editor 工作流骨架，而不是单纯面向人类手工操作的插件仓库或 CLI 仓库。该骨架的核心目的，是让 Agent 能够在真实 Unreal Editor 会话中：

- 判断任务是否需要 Unreal Editor 语义；
- 连接目标 Unreal 工程；
- 调用已存在能力；
- 在能力不足时补最小 command；
- 让新增能力在当前会话中尽快生效；
- 然后继续完成原始任务。

#### Scenario: Agent 读取仓库级定位

- **WHEN** Agent 首次进入本仓库并需要判断这是一个什么项目
- **THEN** 它将 `SUnrealMcp` 理解为一套服务 Unreal Editor 自动化任务的 Agent-first 工作流骨架
- **AND** 它不会把该仓库误判为“仅供人类手工调试的独立 CLI 工具”

### Requirement: 推荐运行路径是 `Skill + embedded CLI + Unreal plugin`

本仓库的推荐运行路径 SHALL 是：

1. `Skill` 负责工作流约束、任务分流与决策；
2. embedded `CLI` 负责目标工程解析、命令 help、参数拼装、TCP JSON 调用与最小运行时桥接；
3. `UnrealPlugin/SUnrealMcp` 负责在 Unreal Editor 内执行 command 并对外暴露能力。

该路径 SHALL 被视为默认主链路。Agent SHOULD 优先通过这条链路工作，而不是优先依赖静态文档、MCP tool discovery 或外围源码猜测。

#### Scenario: Agent 判断应该通过哪条链路与 Unreal 交互

- **WHEN** Agent 需要在真实 Unreal Editor 中读取或修改资产、图语义或编辑器对象
- **THEN** 它优先进入 `Skill + embedded CLI + Unreal plugin` 这条主链路
- **AND** 它不把静态 markdown 文档当作主要能力真相源

### Requirement: 仓库的主要消费者是 Agent，人类主要负责必要调试与审批

`SUnrealMcp` 的主要消费者 SHALL 是 Agent。人类用户在这套体系中的默认职责，是：

- 提供任务目标；
- 在必要时指定目标工程；
- 在需要 Unreal Editor 内轻量操作时配合；
- 在必须升级到更高成本动作时提供审批；
- 在出现异常时做人工调试。

本仓库的接口、输出与默认行为 SHOULD 优先服务 Agent 执行闭环，而不是优先优化成人类手工频繁调用的命令行体验。

#### Scenario: 遇到需要用户配合的运行态限制

- **WHEN** Agent 在执行过程中遇到必须由人类完成的编辑器操作或审批动作
- **THEN** 它将用户视为“必要时参与的调试与审批者”
- **AND** 在除此之外的大多数正常流程中，默认由 Agent 自主推进

### Requirement: 能力补全属于完成原任务的一部分

当当前命令面无法完成原任务时，Agent SHALL 将“补能力”视为完成原任务过程中的中间步骤，而不是独立终点。推荐闭环 SHALL 是：

1. 固定原任务；
2. 探测当前可用命令面；
3. 若能力足够，则直接执行；
4. 若能力不足，则补最小 Unreal command 与最小 CLI wrapper；
5. 让新增能力在当前 Editor 会话中生效；
6. 回到原任务并继续执行，直到原任务完成。

#### Scenario: 当前命令面缺少所需能力

- **WHEN** Agent 发现现有命令无法直接完成用户任务
- **THEN** 它将新增最小必要能力视为主流程的一部分
- **AND** 它不会把“命令已经补好”误当作任务结束
- **AND** 它会在能力生效后继续回到原始用户目标

### Requirement: Token 成本是一等设计约束

由于本仓库的主要消费者是 Agent，`SUnrealMcp` 的运行时接口、帮助信息与默认输出 SHALL 把 token 成本视为一等设计约束。具体而言：

- 默认输出 SHOULD 优先返回完成任务所需的最小结构化信息；
- 诊断字段、详细上下文与人工调试信息 SHOULD 采用按需暴露；
- 能被运行时 help 或结构化元数据替代的静态长文档，不应成为 Agent 每次执行前的必读依赖；
- 任何新增工作流能力在设计时 SHOULD 评估其对调用次数、响应体积与上下文噪音的影响。

#### Scenario: Agent 以默认模式执行命令

- **WHEN** Agent 使用默认命令输出和默认 help 路径工作
- **THEN** 系统默认返回足以驱动下一步决策的最小结构化结果
- **AND** 只有在明确需要诊断时才读取更冗长的信息

### Requirement: Unreal 语义任务默认走运行态语义读取，而不是外围推断

当任务涉及 Blueprint、Widget Blueprint、节点图、编辑器对象、关卡 Actor、UMG 绑定或其他 Unreal Editor 运行态语义时，Agent SHALL 优先通过这套工作流获取真实语义信息。除非用户明确要求只做静态分析，否则 Agent SHOULD NOT 仅依赖：

- `.uasset` 的字符串提取；
- 外围 C++ 引用关系；
- 纯源码层的间接推断；
- 与当前 Editor 状态脱节的离线分析。

#### Scenario: 用户询问 Blueprint 图语义

- **WHEN** 用户的问题需要读取 Blueprint 图结构、节点关系或 Widget 绑定这类 Unreal 语义
- **THEN** Agent 优先通过运行态工作流读取真实语义
- **AND** 如果当前工作流缺少该能力，则将其识别为缺能力并补齐

### Requirement: 本仓库不以通用 MCP 发现链路为前提

本仓库的主目标 SHALL 不是构建一个必须依赖通用 MCP tool discovery 才能工作的系统。`SUnrealMcp` SHOULD 支持 Agent 在单次会话中通过内嵌 `Skill` 与 embedded `CLI` 直接消费新增能力，并在必要时面向多个 Unreal Editor 实例按工程与端口进行路由。

#### Scenario: 同一会话中新增命令后继续任务

- **WHEN** Agent 在当前会话中为完成任务新增了一个 Unreal command 和对应 CLI wrapper
- **THEN** 这套体系的设计目标是让 Agent 在能力生效后继续使用该命令
- **AND** 它不要求 Agent 先依赖外部 MCP discovery 重新建立整条工具发现链路
