## 1. 修正 Unreal 侧 handler 读取 `task_id`

- [x] 1.1 修改 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/GetTaskStatusCommand.cpp`，将 `Request.Params` 读取的 key 从 `taskId` 改为 `task_id`；当字段缺失、为空或非字符串时，错误消息改为 `Missing task_id.`。
- [x] 1.2 修改 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/CancelTaskCommand.cpp`，同样将 `taskId` 改为 `task_id`，错误消息改为 `Missing task_id.`。
- [x] 1.3 在 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Core/` 目录下搜索字面量 `taskId`，确认没有其它 core 命令还残留该旧字段名。
- [x] 1.4 修改 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Mcp/SUnrealMcpTaskRegistry.cpp` 中的 `BuildTaskData`，将 `Data->SetStringField(TEXT("taskId"), ...)` 改为 `task_id`、`taskName` 改为 `task_name`；以及 `CancelTask` 中的 `Data->SetBoolField(TEXT("cancelRequested"), true)` 改为 `cancel_requested`。`MakeAcceptedResponse` 写入的 `accepted` 字段已是 snake_case，无需改动。
- [x] 1.5 在 `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Mcp/` 下重新搜索字面量 `taskId`、`taskName`、`cancelRequested`，确认 framework 代码内已经全部清理（envelope 的 `requestId` / `protocolVersion` 在 `SUnrealMcpProtocol.cpp` 内属于豁免，可保留）。

## 2. 校验 CLI 侧已经符合规范

- [x] 2.1 检查 `Skill/sunrealmcp-unreal-editor-workflow/cli/src/commands/core/system_commands.ts`，确认 `get_task_status` 与 `cancel_task` 都以 `task_id` 注册参数，且都未设置会 rename 字段的 `mapParams` 回调。
- [x] 2.2 在 `Skill/sunrealmcp-unreal-editor-workflow/cli/` 下执行 `npm run build`，确认编译后的 `dist/` 仍然发送 `params.task_id`。

## 3. 针对运行中的编辑器做端到端验证

- [x] 3.1 启动一个加载了 SUnrealMcp 插件的 Unreal 编辑器，先执行 `system ping` 确认服务器就绪。
- [x] 3.2 执行 `system get_task_status --task_id task-does-not-exist`，验证返回的是结构化 `TASK_NOT_FOUND` 错误而不是 `INVALID_PARAMS`，以证明 `task_id` 字段已被正确解析。
- [x] 3.3 执行不带 `--task_id` 的 `system get_task_status`，验证返回 `INVALID_PARAMS`，且错误消息中包含字面子串 `task_id`。
- [x] 3.4 执行 `system cancel_task --task_id task-does-not-exist`，验证返回结构化 `TASK_NOT_FOUND` 错误。
- [x] 3.5 如果 `Commands/Temporary/System/` 下的 `start_mock_task` 等长耗时任务可用，先入队一个任务，用 `system get_task_status --task_id <id>` 查询状态，再用 `system cancel_task --task_id <id>` 取消，验证两次调用均返回 `ok:true`。

## 4. 发布 capability spec

- [x] 4.1 通过 `/opsx:archive` 归档本 change，使 `openspec/changes/fix-task-id-param-naming/specs/core-command-wire-protocol/spec.md` 被提升到 `openspec/specs/core-command-wire-protocol/spec.md`。
- [x] 4.2 确认 `openspec/specs/core-command-wire-protocol/spec.md` 存在且包含本次定义的四条 requirements：wire-参数 snake_case 约定、`get_task_status` 契约、`cancel_task` 契约、CLI 透传契约。
