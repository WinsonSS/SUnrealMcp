## 1. 精简 root-help JSON

- [x] 1.1 修改 `cli/src/index.ts` 的 `printRootHelp`，JSON 分支输出移除 `globalOptions` 和 `helpOptions` 字段

## 2. 精简 family-help JSON

- [x] 2.1 修改 `cli/src/index.ts` 的 `printFamilyHelp`，JSON 分支输出移除 `globalOptions` 和 `helpOptions` 字段
- [x] 2.2 过滤 `sections` 中的空 lifecycle section（仅保留有命令的 key）
- [x] 2.3 移除 `sections` 中每条命令条目的 `lifecycle` 字段，仅保留 `cliCommand` 和 `description`

## 3. 精简 command-help JSON

- [x] 3.1 修改 `cli/src/index.ts` 的 `printCommandHelp`，JSON 分支输出移除 `globalOptions` 和 `helpOptions` 字段

## 4. 文档同步

- [x] 4.1 更新 SKILL.md 和 SKILL_cn.md，补充全局选项列表并说明 JSON help 不再包含 `globalOptions`/`helpOptions`
