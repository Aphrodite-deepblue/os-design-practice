# xv6 优先级调度与 Aging 机制开发计划

> 课程：26271-操作系统设计综合实践  
> 项目方向：基于 xv6-RISC-V 的静态优先级调度与 Aging 防饥饿机制  
> 团队规模：5 人  
> 目标：控制工作量，保证项目可编译、可运行、可测试；每位成员均有清晰、可核查的代码贡献和 Git 提交记录。

---

## 1. 项目范围

本项目在现有 xv6-RISC-V 基础上进行二次开发，不从零实现操作系统。

计划完成以下功能：

1. 为进程增加静态优先级属性；
2. 修改 xv6 调度器，使高优先级进程优先获得 CPU；
3. 保留同优先级进程之间的基本公平性；
4. 增加 Aging 机制，降低低优先级进程长期饥饿的风险；
5. 增加 `setpriority` / `getpriority` 系统调用及简单用户命令；
6. 编写优先级调度和 Aging 测试程序；
7. 保留测试日志、运行截图、开发记录和 AI 使用记录。

### 1.1 不计划实现的内容

为控制工作量，本项目不实现：

- MLFQ / 多级反馈队列；
- CFS、红黑树等复杂调度器；
- 图形化监控界面；
- 完整性能分析平台；
- 动态 I/O 优先级；
- 复杂的进程监控系统；
- 与本课题无关的大规模 xv6 重构。

---

## 2. 五人职责划分

| 成员 | 主要职责 | 核心代码区域 | 主要个人成果 |
|---|---|---|---|
| 成员 1 | 进程优先级数据模型 | `kernel/proc.h`、`kernel/proc.c` | priority 字段、默认值、fork 继承、边界处理 |
| 成员 2 | 静态优先级调度 | `kernel/proc.c` | 优先级 scheduler、同级公平 |
| 成员 3 | Aging 防饥饿机制 | `kernel/proc.c`，必要时 `kernel/trap.c` | wait ticks、Aging 提升逻辑 |
| 成员 4 | 系统调用与用户命令 | syscall 相关文件、`user/`、`Makefile` | `setpriority`、`getpriority`、`nice` |
| 成员 5 | 用户态测试与集成验证 | `user/`、`tests/` | `prioritytest`、`agingtest`、测试日志 |

原则：

- 每个人至少保留 2 个有意义的 Git commit；
- 每个人必须有实际代码贡献，不能只负责报告或 PPT；
- 每个模块尽量由单一负责人维护，避免多人同时修改同一段代码；
- 公共 bug 可以协作解决，但最终提交应能看出主要责任人。

---

# 阶段 A：基线确认与协作准备

## A.1 目标

在开始修改内核前，保证 5 名成员使用同一份可运行的 xv6 基线，并统一开发规则。

## A.2 工作内容

全员完成：

```bash
git clone <repository-url>
cd os-design-practice
make qemu
```

确认 xv6 能正常启动，并至少执行：

```text
$ ls
$ echo hello
$ usertests
```

仓库负责人创建稳定开发分支：

```text
main
└── develop
```

后续功能从 `develop` 创建独立分支：

```text
feature/proc-priority
feature/priority-scheduler
feature/aging
feature/priority-syscall
feature/scheduler-tests
```

## A.3 本阶段代码产出

本阶段原则上不修改核心功能代码，仅允许：

- 补充必要的项目说明；
- 修复明显的环境或构建问题；
- 创建开发分支。

## A.4 非代码产出

需要保存：

- 原版 xv6 启动截图；
- `usertests` 基线测试结果；
- 团队成员与模块分工表；
- 当前基线 commit hash；
- AI 使用记录文件或报告中的 AI 使用章节初始化。

建议在仓库中保留：

```text
docs/
├── team-work.md
├── test-log/
└── images/
```

如果希望减少文档数量，也可以将这些内容最终统一整理到课程设计报告中。

## A.5 验收条件

阶段 A 完成的标准：

- 5 人都能在本地启动 xv6；
- 已确认统一 baseline；
- `main` 不直接用于日常开发；
- 已建立 `develop` 和个人功能分支；
- 每个人明确知道自己负责的模块。

---

# 阶段 B：优先级数据模型与基础接口

## B.1 目标

先建立所有后续功能依赖的进程优先级数据模型。

本阶段主要由成员 1 负责，其他成员暂时不要自行修改 `struct proc`。

## B.2 设计约定

统一采用：

```text
priority = 0   -> 最高优先级
priority = 31  -> 最低优先级
priority = 10  -> 默认优先级
```

进程至少新增：

```c
int priority;
uint wait_ticks;
```

其中：

- `priority`：当前静态优先级；
- `wait_ticks`：为后续 Aging 提供等待时间记录。

## B.3 工作内容

成员 1 负责：

1. 修改 `struct proc`；
2. 在进程创建时设置默认优先级；
3. 规定 `fork()` 后子进程继承父进程优先级；
4. 在进程释放/复用时正确重置相关字段；
5. 处理非法 priority 范围。

建议提交：

```text
feat(proc): add process priority attributes
feat(proc): inherit priority on fork
```

## B.4 并行准备

成员 2、3、4 可以在这一阶段阅读代码和写设计草稿，但不要提前各自增加重复字段。

建议重点阅读：

```text
kernel/proc.c
kernel/proc.h
kernel/trap.c
kernel/syscall.c
kernel/sysproc.c
user/usys.pl
```

成员 5 可以提前设计测试场景，但暂不依赖未完成接口编写最终测试。

## B.5 非代码产出

记录以下设计决定：

- priority 范围；
- 数字越小还是越大代表优先级越高；
- 默认优先级；
- fork 是否继承；
- `wait_ticks` 什么时候增加、什么时候清零。

这些约定必须在后续成员开始开发前统一，否则容易出现接口含义不一致。

## B.6 验收条件

- xv6 可以正常编译、启动；
- 原有程序不受明显影响；
- `fork()` 正常；
- priority 初始化和继承逻辑明确；
- 成员 1 的代码合并到 `develop`。

---

# 阶段 C：调度核心、Aging 与系统调用并行开发

## C.1 目标

在阶段 B 的基础上，由成员 2、3、4 并行完成三个相对独立的功能模块。

---

## C.2 成员 2：静态优先级调度

修改 scheduler，使：

```text
RUNNABLE 进程
      ↓
选择当前最高优先级
      ↓
获得 CPU
```

要求：

1. 优先选择 priority 数字更小的进程；
2. 同一优先级下尽量保持公平；
3. 不删除或绕过 xv6 原有必要的锁；
4. 不在本模块中顺手实现 Aging。

建议提交：

```text
feat(sched): implement static priority scheduling
fix(sched): preserve fairness for equal priority processes
```

---

## C.3 成员 3：Aging 防饥饿

目标是解决静态优先级调度中的 starvation 问题。

基本规则：

```text
RUNNABLE 且长期未得到 CPU
        ↓
wait_ticks 增加
        ↓
达到 AGING_INTERVAL
        ↓
priority 减 1
        ↓
获得更高调度优先级
```

例如：

```c
#define AGING_INTERVAL 50
```

注意：

- priority 不得小于 0；
- 进程真正得到运行机会后，合理重置 `wait_ticks`；
- 不要实现复杂动态优先级算法；
- Aging 只作为简单防饥饿机制。

建议提交：

```text
feat(sched): implement aging for waiting processes
fix(sched): reset aging state after scheduling
```

---

## C.4 成员 4：系统调用与用户命令

实现：

```c
int setpriority(int pid, int priority);
int getpriority(int pid);
```

涉及：

```text
kernel/syscall.h
kernel/syscall.c
kernel/sysproc.c
user/user.h
user/usys.pl
Makefile
```

增加简单用户命令，例如：

```text
$ nice 5
pid 5 priority = 10

$ nice 5 3
pid 5 priority changed to 3
```

建议提交：

```text
feat(syscall): add priority control syscalls
feat(user): add nice command
```

---

## C.5 合并顺序

虽然 C 阶段是并行开发，但不要同时无序合并。

建议由仓库负责人按以下顺序集成：

```text
B 阶段基础
    ↓
成员 2 scheduler
    ↓
编译 + 启动 + 基本测试
    ↓
成员 3 Aging
    ↓
编译 + 启动 + 基本测试
    ↓
成员 4 syscall / nice
    ↓
完整集成测试
```

每合入一个模块，立刻验证一次。

## C.6 开发注意事项

### 1. `kernel/proc.c` 是高冲突文件

成员 2 和成员 3 都可能修改该文件，应提前约定修改区域，并尽量避免大范围格式化或无关重构。

### 2. 保持锁语义

不要为了“让程序先跑起来”而随意移除：

```c
acquire(&p->lock);
release(&p->lock);
```

priority、state、wait_ticks 等进程状态的修改要特别注意并发环境。

### 3. 不扩大功能范围

本阶段如果功能已经满足需求，不临时增加：

- MLFQ；
- 动态时间片；
- 多级队列；
- 新的复杂统计模块。

### 4. 小步提交

一个 commit 只解决一个清晰问题。

避免：

```text
update
修改
final
final2
```

建议：

```text
feat(sched): implement static priority scheduling
fix(sched): handle equal-priority fairness
```

## C.7 非代码产出

每个成员至少记录一次：

- 自己修改了哪些内核路径；
- 实现思路；
- 一次实际调试过程或 bug；
- 是否使用 AI；
- AI 给出的建议是否采用；
- 对应 commit。

## C.8 验收条件

- 静态优先级调度工作；
- 高优先级任务能够更早或更频繁获得 CPU；
- Aging 能逐步提升长期等待进程；
- `setpriority/getpriority` 可从用户态使用；
- `nice` 能正常运行；
- xv6 仍可正常启动。

---

# 阶段 D：测试、集成与回归

## D.1 目标

由成员 5 主负责测试，其他成员负责各自模块的 bug 修复。

代码功能完成不代表项目完成。本阶段必须证明功能真的可用。

## D.2 测试程序

至少实现两个用户态测试程序。

### 1. `prioritytest`

创建多个 CPU 密集进程，例如：

```text
P1 priority = 3
P2 priority = 10
P3 priority = 20
```

验证高优先级进程相对于低优先级进程获得明显的调度优势。

测试应有明确输出：

```text
prioritytest: PASS
```

或：

```text
prioritytest: FAIL
reason: ...
```

### 2. `agingtest`

构造：

```text
多个高优先级 CPU-bound 进程
+
一个低优先级进程
```

验证低优先级进程不会永久得不到运行机会。

输出：

```text
agingtest: PASS
```

## D.3 原版功能回归

至少重新验证：

```text
make qemu
usertests
```

如果 `usertests` 中有与调度修改天然冲突的测试，需要记录具体情况和原因，而不是直接删除测试。

## D.4 建议测试场景

至少覆盖：

1. 单个普通进程；
2. 多个相同优先级进程；
3. 多个不同优先级 CPU-bound 进程；
4. priority 边界值 0 和 31；
5. 非法 priority；
6. fork 后优先级继承；
7. Aging 生效；
8. 进程退出后系统仍稳定。

不要求构建复杂性能分析平台。

## D.5 建议保存的测试材料

```text
tests/
├── logs/
│   ├── baseline.txt
│   ├── prioritytest.txt
│   └── agingtest.txt
└── README.md

docs/images/
├── boot.png
├── nice.png
├── prioritytest.png
└── agingtest.png
```

目录名称可调整，重点是材料能对应到实际运行结果。

## D.6 建议提交

成员 5：

```text
test(sched): add priority scheduling test
test(sched): add aging starvation test
```

其他成员针对测试发现的问题提交：

```text
fix(proc): ...
fix(sched): ...
fix(syscall): ...
```

不要由成员 5 一个人替所有人修全部模块。

## D.7 验收条件

- `prioritytest` 可重复通过；
- `agingtest` 可重复通过；
- 用户态 `nice` 可演示；
- 没有明显 panic；
- 原版基础功能仍能运行；
- 保存了测试日志和关键截图；
- 五名成员均已有可核查 commit。

---

# 阶段 E：最终交付与材料整理

## E.1 目标

冻结代码，不再继续增加功能，集中整理课程要求的交付物。

## E.2 必须准备的项目成果

根据课程要求，至少整理：

1. 可编译运行的 xv6 改造源码；
2. 用户态测试程序；
3. 完整课程设计报告；
4. 运行截图；
5. 测试日志；
6. AI 使用说明及交互记录（本组已使用 AI，因此需要保留）。

答辩 PPT 和演示视频是否强制提交，以教师最终通知为准；如需要，应从现有报告和测试材料中直接整理，避免重复劳动。

## E.3 课程设计报告建议结构

```text
1. 项目背景与目标
2. xv6 原有调度机制简介
3. 需求分析
4. 总体设计
5. 进程优先级数据结构设计
6. 静态优先级调度实现
7. Aging 防饥饿机制
8. 系统调用与用户命令
9. 测试设计与运行结果
10. 开发中遇到的问题与解决过程
11. 小组分工与 Git 提交说明
12. AI 工具使用说明
13. 项目不足与总结
```

## E.4 AI 使用记录

由于课程明确允许但要求声明 AI 使用情况，应至少记录：

- AI 工具名称；
- 模型名称；
- 使用日期；
- 使用成员；
- 使用场景；
- 主要问题；
- AI 给出的建议；
- 是否采用；
- 人工修改与验证；
- 关联 commit 或模块；
- 必要的交互记录。

示例：

```text
日期：2026-09-10
成员：成员 2
工具：ChatGPT
模型：GPT-5.6 Sol
场景：分析 xv6 scheduler 修改位置
AI 建议：在 RUNNABLE 进程中比较 priority，并保留同级公平
处理：采用总体思路，实际代码由成员实现，并通过 prioritytest 验证
关联 commit：<commit hash>
```

## E.5 个人贡献检查

最终每位成员都应至少拥有：

```text
2 个左右有效 Git commit
+
一个明确负责的代码模块
+
对应的设计说明
+
对应测试证据
+
必要的调试记录
+
AI 使用记录（若使用）
```

推荐在报告中加入成员贡献表：

| 成员 | 负责模块 | 主要文件 | 主要 commit | 测试/验证 |
|---|---|---|---|---|
| 成员 1 | priority 数据模型 | `proc.h/proc.c` | ... | ... |
| 成员 2 | scheduler | `proc.c` | ... | ... |
| 成员 3 | Aging | `proc.c` | ... | ... |
| 成员 4 | syscall / nice | syscall + `user/` | ... | ... |
| 成员 5 | tests | `user/`、`tests/` | ... | ... |

## E.6 最终验收

代码冻结前执行一次完整验收：

```text
[ ] clean build 成功
[ ] QEMU 启动成功
[ ] 基础 xv6 命令正常
[ ] setpriority/getpriority 正常
[ ] nice 正常
[ ] prioritytest 通过
[ ] agingtest 通过
[ ] 无明显 panic
[ ] 测试日志已保存
[ ] 关键截图已保存
[ ] 课程设计报告内容齐全
[ ] AI 使用记录齐全
[ ] 五人均有明确 Git 提交
```

建议最后创建一个提交或 tag：

```bash
git tag v1.0-course-final
```

在此之后原则上只修阻塞性 bug，不再增加新功能。

---

## 3. 推荐整体时间顺序

| 阶段 | 建议时间 | 主要结果 |
|---|---|---|
| A | 第 1 天 | baseline、分支、分工、开发规则 |
| B | 第 2 天 | priority 数据模型合并 |
| C | 第 3～4 天 | scheduler、Aging、syscall 并行完成 |
| D | 第 5～6 天 | 测试、集成、修 bug、截图日志 |
| E | 后续时间 | 报告、AI 记录、PPT/视频（如需要） |

不追求严格按天完成。核心原则是：**上一阶段达到验收条件后再进入依赖它的下一阶段。**

---

## 4. Git 协作约定

### 4.1 分支

```text
main        稳定版本
develop     日常集成版本
feature/*   各模块开发分支
```

### 4.2 提交信息

建议采用：

```text
feat: 新功能
fix: bug 修复
test: 测试
docs: 文档
refactor: 不改变功能的重构
```

例如：

```text
feat(proc): add process priority attributes
feat(sched): implement static priority scheduling
feat(sched): implement aging mechanism
feat(syscall): add priority control syscalls
feat(user): add nice command
test(sched): add priority scheduling test
```

### 4.3 合并原则

- 不直接在 `main` 上开发；
- 功能完成并自行测试后再合入 `develop`；
- 修改 `kernel/proc.c` 前先同步最新 `develop`；
- 遇到冲突时由相关模块负责人共同确认，不盲目选择 `ours/theirs`；
- 一个功能合并后先测试，再合下一个功能。

---

## 5. 开发过程中的核心注意事项

1. **控制范围。** 完成既定功能后停止增加新需求。
2. **先保证能跑。** 每次较大修改后先编译、启动，再继续开发。
3. **不要破坏锁。** 调度器和 `struct proc` 涉及并发与 `p->lock`。
4. **避免大范围无关重构。** 减少 merge conflict，也方便老师检查个人贡献。
5. **每个人负责自己的 bug。** 测试负责人发现问题后，由对应模块负责人修复。
6. **保存真实调试记录。** 不需要人为制造 bug。
7. **代码、报告、截图必须一致。** 报告中不要描述最终代码不存在的功能。
8. **AI 输出必须人工验证。** AI 建议不能代替编译、运行和测试。
9. **不要最后一次性提交。** Git 历史应能看出项目逐步演进过程。
10. **最终冻结版本。** 完成交付后不再临时加入高风险功能。

---

## 6. 项目完成定义

当满足以下条件时，本项目视为完成：

- xv6 能正常编译和启动；
- 已实现静态优先级调度；
- 同优先级进程具备基本公平性；
- 已实现 Aging 防饥饿机制；
- 已实现 `setpriority/getpriority`；
- 已提供简单用户命令；
- 已提供可运行的用户态测试；
- 已保存运行截图和测试日志；
- 五名成员都有独立、明确的 Git 贡献；
- 课程设计报告已完整描述设计、实现、测试、分工和 AI 使用情况。

项目完成后不再以增加功能数量为目标，优先保证现有功能稳定、可解释、可复现。
