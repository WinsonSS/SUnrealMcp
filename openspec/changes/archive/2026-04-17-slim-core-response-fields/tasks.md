## 1. PingCommand 响应精简

- [x] 1.1 修改 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/PingCommand.cpp`，删除 `server` / `plugin` / `status` 三个 `SetStringField` 调用，把 `protocolVersion` 字段名改为 `protocol_version`
- [x] 1.2 自检：确认 `data` 对象只剩 `protocol_version` 一个键且类型为 number

## 2. ReloadCommandRegistryCommand 响应精简

- [x] 2.1 修改 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/ReloadCommandRegistryCommand.cpp`，在成功分支删除 `SetBoolField("reloaded", true)` 与 `SetStringField("command", "reload_command_registry")` 两行，保持 `Data` 为空对象后直接 `MakeSuccess`
- [x] 2.2 自检：错误分支（`MODULE_NOT_LOADED` / `RELOAD_FAILED`）未被改动

## 3. SUnrealMcpTaskRegistry 响应精简

- [x] 3.1 修改 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Mcp/SUnrealMcpTaskRegistry.cpp` 中的 `BuildTaskData`，删除 `task_name` 字段的 `SetStringField` 以及派生字段 `completed` 的 `SetBoolField` 调用
- [x] 3.2 修改 `MakeAcceptedResponse`，删除 `Data->SetBoolField(TEXT("accepted"), true);` 这一行
- [x] 3.3 修改 `CancelTask`，删除 `Data->SetBoolField(TEXT("cancel_requested"), true);` 这一行
- [x] 3.4 自检：`BuildTaskData` 仍然输出 `task_id` + `status`，可选 `payload`、`error`；`Cancel()` 调用顺序不变

## 4. 测试 / 自验证

- [x] 4.1 编译 Unreal 插件，确认无未使用变量 / 未引用头文件警告
- [x] 4.2 若 `UnrealPlugin` 下存在 wire-protocol / task registry 单元测试，更新其对被删字段的断言（搜索 `task_name` / `completed` / `accepted` / `cancel_requested` / `protocolVersion` data-level 断言）
- [x] 4.3 手动冒烟：通过 CLI 依次执行 `system ping`、`system reload_command_registry`（若可用）、一次长耗时 task 的 enqueue + `get_task_status` + `cancel_task`，断言响应符合新 spec

## 5. 归档前收尾

- [x] 5.1 检查 Skill/CLI 源码中是否仍有读取 `data.completed` / `data.accepted` / `data.cancel_requested` / `data.task_name` / `data.protocolVersion` 的地方，如有则同步改用 `status` 判断或 envelope 层 `protocolVersion`
- [x] 5.2 `openspec validate slim-core-response-fields` 通过
