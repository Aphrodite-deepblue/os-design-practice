# os-design-practice

操作系统设计综合实践课程项目（26271-操作系统设计综合实践）

- 时间：26271 小学期，第 1~3 周，2026-08-31 ~ 2026-09-20
- 平台：[Xv6](https://pdos.csail.mit.edu/6.828/2026/xv6.html) (RISC-V)
- 基线版本：xv6-riscv 官方仓库最新版（2026-09-01 拉取）

## 项目说明

本仓库用于操作系统设计综合实践课程，基于 Xv6 教学内核进行二次开发与扩展，
内容包括内核源码、用户态测试程序、课程设计报告、运行截图与测试日志等交付物。

项目实现的是静态优先级调度与 Aging 防饥饿机制，不扩展为 MLFQ 或其他复杂调度器。
统一约定如下：

- 优先级范围为 `0`～`31`，数值越小，优先级越高；
- 新进程默认优先级为 `10`，fork 后子进程继承父进程优先级；
- 调度器优先选择高优先级的 `RUNNABLE` 进程，并维持同优先级进程的基本公平；
- 长期等待的可运行进程通过 Aging 逐级提升，避免持续饥饿；
- 用户可通过 `setpriority`、`getpriority` 系统调用和 `nice` 命令查询或修改优先级。

## 目录结构

```
os-design-practice/
├── kernel/            # Xv6 内核源码（调度、内存、文件系统等）
├── user/              # 用户程序及 prioritytest、agingtest、nice
├── mkfs/              # 文件系统镜像生成工具
├── tests/             # 主机端 QEMU 回归与结果汇总工具
├── docs/              # 课程任务、开发计划与 Bug 记录
├── report/            # 课程设计报告 LaTeX 源文件
├── presentation/      # 汇报演示稿、讲稿与素材
├── test-xv6.py        # xv6 自动化测试脚本
├── Makefile           # 构建脚本
├── README.md          # 本文件
└── README             # xv6 官方说明
```

## 环境搭建与构建运行

需要安装 RISC-V 交叉编译工具链和 QEMU。项目要求 **QEMU >= 7.2**，Makefile
会在启动前检查版本，同时兼容标准 QEMU 和 xPack QEMU 的版本输出。

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y gcc-riscv64-linux-gnu qemu-system-misc make perl bc python3

# 如果发行版只提供旧版 QEMU，可使用 xPack QEMU，并将其 bin 目录加入 PATH。
qemu-system-riscv64 --version
riscv64-linux-gnu-gcc --version

# 构建镜像并启动 xv6
make kernel/kernel fs.img
make qemu
```

Windows 用户建议在 WSL 中运行以上命令，以保持与 Linux CI 一致。启动后进入 xv6
shell，可用下面的命令演示优先级接口：

```text
$ nice 1
pid 1 priority = 10
$ nice 1 3
pid 1 priority changed to 3
```

`nice pid` 用于查询，`nice pid priority` 用于设置。内核会再次检查 PID 是否存在以及
优先级是否合法，因此其他用户程序直接调用系统调用时也不能绕过范围限制。

## 测试

日常修改后可以先执行快速回归：

```bash
make kernel/kernel fs.img
python3 test-xv6.py -q usertests
```

提交前执行完整 xv6 回归：

```bash
python3 test-xv6.py usertests
python3 test-xv6.py crash
```

调度专项测试会启动真实 QEMU guest，覆盖系统调用边界、fork 继承、同优先级公平、
不同优先级调度、Aging 和进程退出后的系统稳定性：

```bash
python3 tests/run_scheduler_tests.py \
  --cpus 1 --repeat 1 --regression quick \
  --out tests/results/local-single
```

完整参数、结果文件和 CI 对应关系见 [`tests/README.md`](tests/README.md)。每个 `--out`
目录只能使用一次，避免新结果覆盖旧日志。

### 已验证状态

- `make kernel/kernel fs.img` 构建成功，xv6 能正常进入 shell；
- `nice` 查询、合法边界 `0`/`31`、非法参数和不存在 PID 均已验证；
- 单 CPU 与 3 CPU 配置下，`prioritytest`、`agingtest` 和 `usertests -q` 均通过；
- 原有完整 `usertests` 与 `crash` 回归通过；
- Linux 和 macOS GitHub Actions 均用于持续检查构建或回归。

这些结论只描述已经执行并保留日志的验证，不把单次 QEMU 运行数据作为稳定性能基准。

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

本项目在代码核对、成员 4 系统调用与用户命令开发辅助、测试命令整理、CI 检查和文档修订中
使用了 OpenAI Codex。AI 输出只作为分析或草稿，功能语义由小组约定决定，代码由成员检查后
提交，构建、QEMU 输出和测试结果均来自真实执行，没有使用 AI 生成的实验数据代替验证。

具体使用场景、采用情况、人工修改、验证方式和关联提交记录在课程报告“AI 使用情况”章节及
`docs/进度汇报.md` 中。

## 参考链接

- [Xv6, a simple Unix-like teaching operating system](https://pdos.csail.mit.edu/6.828/2026/xv6.html)
- [操作系统综合实践课程设计任务书（基于XV6-RISC-V）](https://docs.qq.com/markdown/DUmFydVdaYWx0bkhC?)
- 课程设计任务书：`docs/课程设计任务书.md`
