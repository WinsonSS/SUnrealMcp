## ADDED Requirements

### Requirement: `refresh_cpp_runtime` 成功后新命令立即对注册表内省可见

当 `refresh_cpp_runtime` task 进入 `succeeded` 时，它 SHALL 已在成功路径中完成 `reload_command_registry`。因此，对于同一 Unreal Editor 会话内刚通过 Live Coding 生效的新命令，后续的 `check_command_exists` 与 `diff_commands` SHALL 观察到 reload 后的注册表结果，而不要求 Agent 再单独调用一次 `reload_command_registry`。

#### Scenario: 运行态刷新成功后 check_command_exists 立即命中新命令

- **WHEN** Agent 通过 `refresh_cpp_runtime` 让某个新命令在当前 Editor 会话中生效，且该 task 已进入 `succeeded`
- **THEN** 后续调用 `check_command_exists` 查询该命令名时，返回结果基于刷新后的注册表
- **AND** Agent 不需要再额外调用一次 `reload_command_registry`

#### Scenario: 运行态刷新成功后 diff_commands 不再报告仅 Unreal 侧缺失的新增命令

- **WHEN** CLI 与 Unreal 因新增命令而暂时漂移，随后 `refresh_cpp_runtime` task 成功完成
- **THEN** 后续 `diff_commands` 的差集计算基于 reload 后的 Unreal 注册表
- **AND** 若双方其余条件一致，则该新增命令不再因“尚未 reload registry”而出现在 `only_in_cli`
