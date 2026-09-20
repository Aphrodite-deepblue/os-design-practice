# Bug 记录

## BUG-20260910-01：Homebrew QEMU 启动时动态库符号不匹配

- **类型**：环境问题
- **现象**：已安装的 QEMU 启动失败，提示找不到 `cs_close` 符号；系统中同时缺少 RISC-V 交叉编译工具链。
- **原因**：Homebrew 中原有 QEMU 11.0.1 与动态库版本不匹配，且未安装项目所需的 RISC-V 工具链。
- **处理**：安装 `riscv64-elf-gcc`、`riscv64-elf-binutils`，重装 QEMU 及其依赖，并通过 Make 参数使用 `riscv64-elf-` 工具链前缀。
- **验证**：QEMU 11.1.1 成功启动 xv6，进入 shell；`ls`、`echo`、`forktest` 和 `usertests` 均可执行。
- **状态**：已解决。

## BUG-20260910-02：fork 读取父进程优先级时缺少锁保护

- **类型**：并发/锁语义问题
- **现象**：代码复核发现 `kfork()` 直接读取 `p->priority`，但当时没有持有父进程的 `p->lock`。
- **原因**：`priority` 属于受 `p->lock` 保护的进程字段；后续加入 `setpriority` 或调度相关逻辑后，未加锁读取可能产生数据竞争或读取不一致快照。
- **处理**：在 `kfork()` 中获取父进程锁，将优先级复制到局部快照后立即释放锁；子进程根据快照继承优先级，非法值回退为默认优先级 `10`。
- **验证**：clean build 成功；QEMU 正常启动；`forktest` 通过；`usertests` 输出 `ALL TESTS PASSED`。
- **关联提交**：`9e066e5 feat(proc): inherit priority on fork`
- **状态**：已解决。

## BUG-20260917-01：xPack QEMU 版本号无法被 Makefile 识别

- **类型**：构建环境兼容性问题
- **现象**：使用项目文档推荐的 xPack QEMU 9.2.4 执行 `make qemu` 时，版本检查出现 `bc` 语法错误和 `Illegal number`，但 QEMU 随后仍会启动。
- **原因**：xPack 的版本输出以 `xPack QEMU emulator version` 开头，原有正则仅识别 `QEMU emulator version`，导致传给 `bc` 的内容不是版本号。
- **处理**：让版本解析同时接受标准 QEMU 和 xPack QEMU 的输出格式，仍只提取主、次版本参与最低版本检查。
- **验证**：使用 xPack QEMU 9.2.4 执行版本检查和启动 xv6，不再出现版本解析错误；标准 QEMU 6.2.0 仍会被最低版本要求拒绝。
- **状态**：已解决。

## BUG-20260901-aging-01：Aging 与 setpriority 的等待状态需要统一

- **类型**：跨模块接口约定
- **现象**：Aging 分支按“获得 CPU 即清零 `wait_ticks`”处理；系统调用分支还需要明确手动设置优先级时如何处理已有等待状态。
- **原因**：Aging 与 `setpriority` 分支并行开发，合并前没有统一等待计数的重置语义。
- **处理**：调度器在进程获得 CPU 时清零 `wait_ticks`；`setpriority` 成功修改优先级时也清零，使新的手动优先级从新的等待周期开始计算。
- **验证**：clean build 成功；3 CPU QEMU 的 `usertests -q` 输出 `ALL TESTS PASSED`；单 CPU QEMU 临时集成测试覆盖优先级边界、非法参数、fork 继承、静态优先级顺序和 Aging 提升并输出 `abcdtest: PASS`；`nice` 查询、设置及非法参数检查符合预期。
- **关联提交**：`631ee68 feat(sched): implement aging for waiting processes`、`43b682f feat(syscall): add priority control syscalls`
- **状态**：已解决。

## BUG-20260921-test-01：调度测试运行器在 macOS 上读取 Linux 环境信息失败

- **类型**：测试工具跨平台兼容性问题
- **现象**：在 macOS 执行 `tests/run_scheduler_tests.py` 时，尚未开始构建就因 `platform.freedesktop_os_release()` 找不到 `/etc/os-release` 和 `/usr/lib/os-release` 而退出。
- **原因**：运行器记录环境信息时直接调用 Linux 专用 API，没有为 macOS 等非 Linux 系统提供后备路径。
- **处理**：增加 `host_os_release()`，Linux 上继续使用发行版信息；其他系统回退到 `platform.system()` 和 `platform.release()`，不改变测试、构建和 QEMU 流程。
- **验证**：在 macOS 上使用 QEMU 11.1.1 完成单 CPU 与 3 CPU 回归，`prioritytest`、`agingtest` 和 `usertests -q` 均通过。
- **状态**：已解决。
