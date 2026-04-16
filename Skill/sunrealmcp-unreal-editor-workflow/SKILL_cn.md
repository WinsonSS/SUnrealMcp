# SUnrealMcp Unreal Editor 工作流

## 目的

这份 Skill 用于所有需要和 Unreal Editor、UE 资产、编辑器对象真实交互的任务。

Skill 负责流程和决策。
CLI 负责连接、发命令、查端口、返回结果，并且充当能力真相源。

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

### 2. 资产语义请求必须用资产语义读取来回答

如果用户问的是 Blueprint 逻辑、图结构、Widget 绑定、编辑器元数据：

- 不要把 `.uasset` 字符串提取当主答案
- 不要把外围 C++ 引用关系当主答案
- 不要停在外围推断

如果当前 CLI 和插件读不到这些语义，就把它归类成“缺能力”，然后去补。

### 3. CLI help 就是命令目录

不要把静态 markdown 命令清单当作能力真相源。

当 Agent 需要命令信息时，按下面这个固定顺序调用 CLI help：

1. 如果还不知道该看哪个类别，先执行 `sunrealmcp-cli help --json`
2. 读取返回里的 family 列表，选出最匹配任务的 family
3. 执行 `sunrealmcp-cli help <family> --json`
4. 读取该 family 下的命令列表，选出最匹配任务的 command
5. 在真正执行前，执行 `sunrealmcp-cli help <family> <command> --json`
6. 读取返回里的参数定义，再据此拼出真实调用命令

如果 CLI help 里没有出现所需 family 或 command，就直接把它视为缺能力。

### 4. 缺能力是主流程的一部分

能力是否存在，只以 CLI 当前暴露的命令面为准。

如果 CLI 里没有完成任务所需的命令：

- 就直接把它视为缺能力
- 补最小 Unreal command 和最小 CLI wrapper
- 重编或 Live Coding
- 然后继续原任务

如果补的过程中发现 Unreal 侧以前其实有旧 command：

- 可以复用、替换、包装、收敛
- 但对 Agent 来说，仍然以新的 CLI 命令面为准

不要停在“能力已经补好了”。
必须回到原任务继续做完。

## 目标工程与端口发现

CLI 支持多个 Unreal Editor。但是需要通过`--project <project_path>`指定UE工程目录。

## 执行闭环

### 1. 固定原任务

先把用户真正要完成的任务固定成一句话。

后面的能力补全，只是为了把这件事做完。

### 2. 定位目标工程

定位UE工程，获取到UE工程根目录路径。

### 3. 探测当前 CLI 命令面

通过 CLI help 去看当前真实可用的类别和命令。

具体调用顺序严格按“核心规则 3. CLI help 就是命令目录”执行。

不要凭记忆猜命令名。

需要根据 help 返回的参数定义构造最终调用。

### 4. 先用成熟能力

在 CLI 当前暴露出来的类别和命令里，默认这样选：

1. 基础设施类先看 `core`
2. 正式能力优先看 `stable`
3. `temporary` 只作为临时能力

### 5. 缺能力就补

如果 CLI 里没有能完成任务的命令：

- 查 CLI 命令面
- 补最小 Unreal command 和最小 CLI wrapper
- 新添加的cli命令和UnrealCommand作为temporary添加。

这个仓库里的默认 source of truth：

- Unreal 插件：
  `ProjectRootPath/Plugins/SUnrealMcp/Source/SUnrealMcp/Private/Commands`
- Skill 内置 CLI：
  `Skill/sunrealmcp-unreal-editor-workflow/cli`

### 6. 重编、重载、继续

补完能力后：

#### Unreal 侧（C++ command）

- 能 Live Coding 就优先 Live Coding
- 不行就完整重编
- 必要时 reload command registry

#### CLI 侧（TypeScript wrapper）

如果新增或修改了 `cli/src/` 下的 TypeScript 文件，必须编译后才能生效：

```bash
cd Skill/sunrealmcp-unreal-editor-workflow/cli && npm run build
```

��会把 `src/` 编译到 `dist/`，CLI 运行时读的是 `dist/`。

#### 然后继续

两侧都就绪后立刻继续原任务。新添加的命令可以直接使用。

### 7. 以原任务完成为结束

只有原任务完成了，这个工作流才算完成。

## CLI 响应格式

命令默认输出精简 JSON 信封：

- 成功：`{"ok": true, "data": {…}}`
- 失败：`{"ok": false, "error": {"code": "…", "message": "…"}}`

诊断字段（`target`、`cli`、`unreal`、`raw`）默认不输出，以降低 token 开销。如需包含，传 `--verbose`：

```bash
sunrealmcp-cli <family> <command> --verbose
```

Agent 工作流应使用默认（精简）格式。`--verbose` 仅用于人工调试。

## CLI Help

当目标是让 Agent 执行命令时，优先使用 `--json` 的 help 输出。

具体用法严格按”核心规则 3. CLI help 就是命令目录”执行。

非 JSON help 只用于人类手动调试。

### 全局选项参考

JSON help 输出只包含命令相关信息。全局选项列表如下供参考：

- `--project <path>` — 工程根目录或 .uproject 路径
- `--host <string>` — 覆盖解析出的绑定地址
- `--port <number>` — 覆盖解析出的端口
- `--timeout_ms <number>` — Socket 超时覆盖（默认 30000）
- `--pretty` — 美化打印 JSON 命令输出
- `--verbose` — 在命令输出中包含诊断字段

Help 专用选项：`--json` — 输出 JSON 格式的 help。
