## 1. 类型定义更新

- [x] 1.1 在 `cli/src/types.ts` 的 `CliGlobalOptions` 接口中新增 `verbose: boolean` 字段

## 2. 选项解析

- [x] 2.1 在 `cli/src/index.ts` 的 `parseOptions` 中识别 `--verbose` 标志并设置 `global.verbose = true`，默认值为 `false`

## 3. 信封裁剪

- [x] 3.1 修改 `cli/src/index.ts` 的 `executeCommand` 函数，在返回前根据 `global.verbose` 裁剪输出：默认仅保留 `{ok, data}` / `{ok, error}`，verbose 模式追加 `target`、`cli`、`unreal`、`raw`

## 4. 文档同步

- [x] 4.1 更新 SKILL.md 中对 CLI 响应格式的描述，标注 BREAKING 变更和 `--verbose` 用法
