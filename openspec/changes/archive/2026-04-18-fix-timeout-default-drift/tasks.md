## 1. 收口默认值真相源

- [x] 1.1 将 `Skill/sunrealmcp-unreal-editor-workflow/cli/src/runtime/discovery.ts` 中的 `DEFAULT_TIMEOUT_MS` 调整为 `30000`
- [x] 1.2 确认 `Skill/sunrealmcp-unreal-editor-workflow/cli/src/index.ts` 中 help 暴露的 `--timeout_ms` 默认值继续来自同一个 `DEFAULT_TIMEOUT_MS`

## 2. 同步 Agent 工作流文档

- [x] 2.1 修改 `Skill/sunrealmcp-unreal-editor-workflow/SKILL_cn.md` 中 `--timeout_ms` 的说明，移除硬编码默认数字，并把 CLI help 作为默认值来源
- [x] 2.2 修改 `Skill/sunrealmcp-unreal-editor-workflow/SKILL.md` 中 `--timeout_ms` 的说明，移除硬编码默认数字，并把 CLI help 作为默认值来源

## 3. 验证一致性

- [x] 3.1 运行 `sunrealmcp-cli help` 或等价 Node 入口，确认 help 中 `--timeout_ms` 默认值显示为 `30000`
- [x] 3.2 搜索活跃 Skill 文档与 CLI 源码，确认 `30000` 只由运行时与 help 持有，Skill 文档中不再硬编码默认 `timeout_ms` 数字
