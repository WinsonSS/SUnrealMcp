## 1. 扩展 `FSUnrealMcpCommandRegistry`

- [x] 1.1 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Public/Mcp/SUnrealMcpCommandRegistry.h` 声明 `TArray<FString> ListCommandNames() const;` 与 `bool HasCommand(const FString& Name) const;`
- [x] 1.2 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Mcp/SUnrealMcpCommandRegistry.cpp` 实现两个访问器：`ListCommandNames` 遍历 `Commands` 取 Key、`Sort` 后返回；`HasCommand` 直接 `return Commands.Contains(Name);`

## 2. 打通 `FSUnrealMcpExecutionContext`

- [x] 2.1 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Public/Mcp/ISUnrealMcpCommand.h`：前向声明 `FSUnrealMcpCommandRegistry`，在 `FSUnrealMcpExecutionContext` 加字段 `TSharedRef<FSUnrealMcpCommandRegistry> CommandRegistry;`
- [x] 2.2 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Mcp/SUnrealMcpServer.cpp:247` 构造 context 处补充 `CommandRegistry.ToSharedRef()`

## 3. 实现 `check_command_exists`

- [x] 3.1 新文件 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/CheckCommandExistsCommand.cpp`：匿名 namespace + `FSUnrealMcpCommandAutoRegistrar` 模式，`GetCommandName` 返回 `"check_command_exists"`
- [x] 3.2 `Execute` 解析 `Request.Params->TryGetStringField(TEXT("name"), Name)`，空/缺失 → `INVALID_PARAMS` 且 message 含 `name`；否则 `Data->SetBoolField(TEXT("exists"), Context.CommandRegistry->HasCommand(Name))`

## 4. 实现 `diff_commands`

- [x] 4.1 新文件 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/DiffCommandsCommand.cpp`：同上模式，`GetCommandName` 返回 `"diff_commands"`
- [x] 4.2 `Execute` 解析 `Request.Params->TryGetArrayField(TEXT("cli_names"), ArrayPtr)`，缺失/类型错 → `INVALID_PARAMS` 且 message 含 `cli_names`
- [x] 4.3 把 JSON 数组转成 `TSet<FString>` CliNamesSet；取 `Context.CommandRegistry->ListCommandNames()` 作为 UnrealNames；计算 `only_in_unreal = UnrealNames - CliNamesSet`、`only_in_cli = CliNamesSet - UnrealNames`，两者分别 `Sort`
- [x] 4.4 把两个数组分别构造为 `TArray<TSharedPtr<FJsonValue>>` 后 `Data->SetArrayField`

## 5. 扩展 CLI 注册表接口与执行上下文

- [x] 5.1 `Skill/sunrealmcp-unreal-editor-workflow/cli/src/types.ts`：`CliCommandRegistry` 接口追加 `getAll: () => CliCommandDefinition[];`；`CliCommandExecutionContext` 追加 `registry: CliCommandRegistry;`
- [x] 5.2 `Skill/sunrealmcp-unreal-editor-workflow/cli/src/index.ts` 中 `executeCommand` 走到 `definition.execute(...)` 的构造点，把 `registry` 传进去
- [x] 5.3 自检：`CommandRegistry` 类已有 `getAll` 实现，无需补齐

## 6. CLI 侧注册两条命令

- [x] 6.1 `Skill/sunrealmcp-unreal-editor-workflow/cli/src/commands/core/system_commands.ts`：新增 `check_command_exists` 命令，`parameters: [{name:"name", type:"string", required:true, description:"Command name to check."}]`，不写 `mapParams`
- [x] 6.2 同文件新增 `diff_commands` 命令，`parameters: []`，提供 `execute:` handler：从 `context.registry.getAll()` 过滤 `family !== "raw" && typeof unrealCommand === "string"`，取 `unrealCommand` 去重排序，构造 `UnrealTransportClient`（参考 `index.ts:126-130` 的构造方式），发送 `diff_commands` + `{cli_names}`，返回 `{ok, data}`/`{ok, error}` 精简信封
- [x] 6.3 自检：`execute:` handler 返回结构与默认路径一致，不额外输出 `target`/`cli`/`unreal`/`raw`

## 7. 测试 / 自验证

- [x] 7.1 编译 Unreal 插件，确认无未引用头文件 / 未解析符号
- [x] 7.2 构建 CLI（`tsc`），确认改动打包进 `dist/`
- [x] 7.3 手动冒烟：
  - `sunrealmcp-cli system check_command_exists --name ping` → `{ok:true, data:{exists:true}}`
  - `sunrealmcp-cli system check_command_exists --name nope_xyz` → `{ok:true, data:{exists:false}}`
  - `sunrealmcp-cli system check_command_exists` → `INVALID_PARAMS`
  - `sunrealmcp-cli system diff_commands` → `{ok:true, data:{only_in_unreal:[...], only_in_cli:[...]}}`，健康时两数组均为 `[]`

## 8. 归档前收尾

- [x] 8.1 `openspec validate list-commands-core-command` 通过
