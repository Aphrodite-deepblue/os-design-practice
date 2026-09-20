# 实验报告工作日志

## 2026-09-21 15:30:00 CST

- 将队友提交的 `7c22218`、`0851624` 快进合并到 `main`，包含 `prioritytest`、`agingtest` 和 QEMU 回归运行器。
- 在当前 `main` 快照的干净临时副本中执行单 CPU、3 CPU 回归；两组的专项测试和 `usertests -q` 均通过，结果来自真实 QEMU 日志。
- 修复测试运行器在 macOS 上无条件读取 Linux `/etc/os-release` 的兼容问题，并在 `docs/bugs.md` 记录。
- 更新报告的测试与验证章节、总结与展望，补充实际覆盖范围、运行配置和结果；未把一次本地运行当作性能基准。

## 2026-09-20 11:46:07 CST

- 用户要求再次编译报告。
- 确认当前 LaTeX 工作目录已调整为 `report/`，并从该目录执行 `latexmk -g -xelatex -interaction=nonstopmode -halt-on-error tjumain.tex` 强制完整重编译。
- 编译成功，生成 14 页 A4 PDF；未发现 LaTeX 错误、未解析引用或页面溢出。
- 抽取 PDF 文本确认“参考资料”及相关条目未重新出现，渲染最后一页检查排版正常。
- 将最新 PDF 同步至两份交付文件并核对 SHA-256 一致。
