## 1. 更新 OpenSpec 规格

- [x] 1.1 在 `openspec/changes/refresh-cpp-runtime/specs/cpp-runtime-refresh/spec.md` 定义 `refresh_cpp_runtime` 的命令契约、task 成功路径和失败分流
- [x] 1.2 在 `openspec/changes/refresh-cpp-runtime/specs/command-registry-introspection/spec.md` 增加“`refresh_cpp_runtime` 成功后自动 reload command registry”的联动要求
- [x] 1.3 `openspec validate refresh-cpp-runtime` 通过

## 2. Unreal 侧实现 `refresh_cpp_runtime` 工作流

- [x] 2.1 新增 Unreal core 命令 `refresh_cpp_runtime`，入队并返回 task accepted 响应
- [x] 2.2 新增对应 task，实现 `Start/Tick/Cancel/GetState/BuildPayload/GetError`
- [x] 2.3 在任务开始前检查当前编辑器状态；若 PIE 正在运行，直接以 `PIE_ACTIVE` 失败
- [x] 2.4 接入 Unreal Live Coding 触发入口，并观察完成/失败结果
- [x] 2.5 将 Live Coding 失败至少映射为：
  - `LIVE_CODING_COMPILE_FAILED`
  - `LIVE_CODING_UNAVAILABLE`
  - `LIVE_CODING_BUSY`（若已有另一轮编译正在进行）
- [x] 2.6 为失败结果补充结构化 `error.details`，至少包含 `resolution` 与 `suggested_action`
- [x] 2.7 成功路径中自动执行 `reload_command_registry`，若失败则返回 `REGISTRY_RELOAD_FAILED`

## 3. CLI / Skill 接入

- [x] 3.1 `Skill/sunrealmcp-unreal-editor-workflow/cli/src/commands/core/system_commands.ts` 注册 `system refresh_cpp_runtime`
- [x] 3.2 如有需要，更新 CLI help / 描述文本，使其明确这是“优先 Live Coding 的工作流命令”
- [x] 3.3 更新 `Skill/sunrealmcp-unreal-editor-workflow/SKILL.md` 与 `SKILL_cn.md`，把“优先 Live Coding”改成“调用 `refresh_cpp_runtime` 并按结构化结果分流”

## 4. 自验证

- [x] 4.1 CLI 构建通过，`dist/` 产物更新
- [x] 4.2 Unreal 插件编译通过
- [x] 4.3 冒烟成功路径：
  - 触发 `refresh_cpp_runtime`
  - 轮询到 `succeeded`
  - 新增命令可被 `check_command_exists` / `diff_commands` 观察到
- [x] 4.4 冒烟 PIE 阻塞路径：PIE 运行中调用任务，返回 `PIE_ACTIVE`
- [x] 4.5 冒烟代码错误路径：Live Coding 编译失败时返回 `LIVE_CODING_COMPILE_FAILED`，并带 `resolution=agent_retry`
- [x] 4.6 冒烟环境阻塞路径：无法 Live Coding 时返回需要用户授权的结构化错误
