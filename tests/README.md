# 调度专项测试指南

`tests/run_scheduler_tests.py` 在主机端构建临时源码副本、启动真实 QEMU guest，
并执行 `prioritytest`、`agingtest` 及可选的 `usertests` 回归。运行器不会生成或补写
测试数据；构建失败、内核 panic、超时、测试失败或 QEMU 异常退出都会使进程返回非零状态，
同时保留已产生的原始日志。

## 环境要求

建议使用 Linux、macOS 或 Windows WSL。主机需要以下命令：

```text
python3
make
riscv64-linux-gnu-gcc
qemu-system-riscv64（版本不低于 7.2）
perl
bc
```

Ubuntu/Debian 可以安装：

```bash
sudo apt-get update
sudo apt-get install -y gcc-riscv64-linux-gnu qemu-system-misc make perl bc python3
```

如果使用 xPack QEMU，应先把对应的 `bin` 目录加入当前 shell 的 `PATH`。当前运行器会用
`riscv64-linux-gnu-gcc --version` 记录编译器环境，因此即使 Makefile 还能识别其他工具链前缀，
执行本测试指南时也应确保该命令存在。测试脚本不依赖某位成员的绝对安装路径。

## 常用命令

在仓库根目录执行。开发过程中可先跑一次单 CPU 快速回归：

```bash
python3 tests/run_scheduler_tests.py \
  --cpus 1 \
  --repeat 1 \
  --regression quick \
  --out tests/results/local-single
```

提交前建议执行与调度 CI 相同的三组测试：

```bash
python3 tests/run_scheduler_tests.py \
  --cpus 1 --repeat 3 --regression quick \
  --out tests/results/single

python3 tests/run_scheduler_tests.py \
  --cpus 3 --repeat 3 --regression full \
  --out tests/results/smp

python3 tests/run_scheduler_tests.py \
  --compare --repeat 3 \
  --out tests/results/compare
```

三组命令分别验证：

- `single`：单 CPU 下的优先级接口、静态调度、Aging 和快速 `usertests`；
- `smp`：3 CPU 下的并发行为、专项测试和完整 `usertests`；
- `compare`：在一次性临时副本中运行 RR、静态优先级和带 Aging 的调度策略，保留可复核指标。

`--repeat` 的合法范围为 1～10。`--out` 指向的目录如果已经存在 `results.json`，运行器会
拒绝覆盖；请改用新的目录名，或先由人工归档旧结果。

## 参数说明

| 参数 | 可选值 | 默认值 | 说明 |
|---|---|---|---|
| `--cpus` | `1`、`3` | `1` | QEMU 虚拟 CPU 数量；`--compare` 只允许单 CPU。 |
| `--repeat` | `1`～`10` | `3` | 每组专项测试的重复次数。 |
| `--regression` | `none`、`quick`、`full` | `quick` | 不跑、快速或完整 `usertests`。 |
| `--compare` | 开关 | 关闭 | 执行三种调度策略的受控对比。 |
| `--out` | 目录 | `tests/results/latest` | 保存日志和结构化结果；禁止覆盖已有结果。 |

## 结果文件

每次运行都会记录源码提交和相关源文件的 SHA-256，便于确认结果对应的代码状态。输出目录中
主要包含：

- `results.json`：环境、参数、源码哈希、每次运行状态和解析后的指标；
- `metrics.csv`：`prioritytest` 输出的调度指标，便于制作表格或进一步分析；
- `build-*.log`：对应策略的完整构建输出；
- `*.log`：QEMU 原始输出，包括专项测试、负向控制和 `usertests` 日志。

只有运行器退出码为 0 且 `results.json` 中 `success` 为 `true` 时，整组测试才算通过。
`EXPECTED_FAIL` 只用于验证受控负向场景，例如关闭 Aging 后低优先级进程应当超时；它不是
被忽略的普通失败。

可以把多组结果汇总为 Markdown 表格：

```bash
python3 tests/summarize_results.py \
  tests/results/single/results.json \
  tests/results/smp/results.json \
  tests/results/compare/results.json
```

## Docker 运行

仓库提供固定为 Ubuntu 24.04 的测试镜像：

```bash
docker build -t xv6-scheduler-tests -f tests/Dockerfile .
docker run --rm -v "$PWD:/src" xv6-scheduler-tests \
  python3 tests/run_scheduler_tests.py \
  --cpus 1 --repeat 1 --regression quick \
  --out tests/results/docker-single
```

PowerShell 中可将挂载参数写为 `-v "${PWD}:/src"`。容器只提供统一工具链；运行器仍会在
临时目录中构建被测策略，并把结果写回指定的 `tests/results/` 目录。

## GitHub Actions

`.github/workflows/scheduler-tests.yml` 在内核、用户程序、Makefile、测试脚本或工作流本身变化时
自动触发，也支持手动运行。矩阵中的 `single`、`smp` 和 `compare` 与上面的提交前命令一致，
每组结果都会作为 Actions artifact 上传，即使测试失败也保留已有日志供排查。

文档单独变化不会触发调度专项工作流；这是为了避免只改说明文字时重复运行耗时的 QEMU
矩阵。仓库的基础 `test.yml` 仍会按其自身触发条件执行。
