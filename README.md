# os-design-practice

操作系统设计综合实践课程项目（26271-操作系统设计综合实践）

- 时间：26271 小学期，第 1~3 周，2026-08-31 ~ 2026-09-20
- 平台：[Xv6](https://pdos.csail.mit.edu/6.828/2026/xv6.html) (RISC-V)
- 基线版本：xv6-riscv 官方仓库最新版（2026-09-01 拉取）

## 项目说明

本仓库用于操作系统设计综合实践课程，基于 Xv6 教学内核进行二次开发与扩展，
内容包括内核源码、用户态测试程序、课程设计报告、运行截图与测试日志等交付物。

## 目录结构

```
os-design-practice/
├── kernel/            # Xv6 内核源码（调度、内存、文件系统等）
├── user/              # Xv6 用户态程序（sh、ls、usertests 等）
├── mkfs/              # 文件系统镜像生成工具
├── tests/             # 用户态测试程序（课程新增）
├── docs/              # 课程设计任务书、设计报告等文档
├── test-xv6.py        # xv6 自动化测试脚本
├── Makefile           # 构建脚本
├── README.md          # 本文件
└── README             # xv6 官方说明
```

## 环境搭建与构建运行

需要安装 RISC-V 交叉编译工具链和 QEMU（**要求 QEMU >= 7.2**，xv6 Makefile 有版本检查）：

```bash
# 安装工具链（Ubuntu/Debian）
sudo apt-get install gcc-riscv64-unknown-elf

# QEMU：Ubuntu 20.04 系统源仅提供 4.2（过旧），需使用 >= 7.2 的版本
# 方式一：更新系统源中 QEMU（需较新的 Ubuntu 发行版）
sudo apt-get install qemu-system-misc
# 方式二：使用 xPack 预编译 QEMU（本机已装到 ~/tools，免 root）
#   ~/tools/xpack-qemu-riscv-9.2.4-1/bin 已加入 PATH

# 构建并启动 xv6
make qemu
```

启动后进入 xv6 shell，可运行 `ls`、`usertests` 等用户程序测试。

### 本地验证结果（基线版，2026-09-01）

- 编译：`make kernel/kernel fs.img` 成功
- 启动：QEMU 9.2.4 正常引导，`init: starting sh`
- 命令测试：`ls`、`echo` 正常
- 自动化测试：`./test-xv6.py -q usertests` → **ALL TESTS PASSED**

## 交付成果（按课程任务书）

- 可编译运行的 Xv6 改造源码
- 用户态测试程序
- 完整课程设计报告
- 运行截图与测试日志
- 答辩 PPT（可选）

## Git 提交约定

课程评分要求"git 提交历史清晰，每一步迭代可见"，因此：
- 每个功能点独立提交，commit message 写清改动内容
- 保留 xv6 官方原始代码作为基线提交，后续改动可 diff 对照
- 小组成员用各自 GitHub 账号提交，体现分工

## AI 工具使用声明

按课程要求，凡使用 AI 工具的小组需在开发相关文档中声明所用 AI 工具/大模型的
名称、使用场景，并在 git commit 记录、设计实现文档及答辩 PPT 中单独说明
AI 工具的成果及交互记录。本项目将在此处持续更新。

## 参考链接

- [Xv6, a simple Unix-like teaching operating system](https://pdos.csail.mit.edu/6.828/2026/xv6.html)
- [操作系统综合实践课程设计任务书（基于XV6-RISC-V）](https://docs.qq.com/markdown/DUmFydVdaYWx0bkhC?)
- 课程设计任务书：`docs/课程设计任务书.md`
