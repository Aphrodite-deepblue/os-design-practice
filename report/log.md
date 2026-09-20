# 实验报告工作日志

## 2026-09-20 11:46:07 CST

- 用户要求再次编译报告。
- 确认当前 LaTeX 工作目录已调整为 `report/`，并从该目录执行 `latexmk -g -xelatex -interaction=nonstopmode -halt-on-error tjumain.tex` 强制完整重编译。
- 编译成功，生成 14 页 A4 PDF；未发现 LaTeX 错误、未解析引用或页面溢出。
- 抽取 PDF 文本确认“参考资料”及相关条目未重新出现，渲染最后一页检查排版正常。
- 将最新 PDF 同步至两份交付文件并核对 SHA-256 一致。
