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
## BUG-20260901-aging-01：Aging 与 setpriority 未来可能的交互待确认

- **类型**：待办/接口约定
- **现象**：成员 4 的 `setpriority` 尚未合入，Aging 当前按"获得 CPU 即清零 `wait_ticks`"处理；若成员 4 后续选择"手动设置优先级时重置等待状态"，需要与其约定保持一致，避免重复重置或语义冲突。
- **原因**：跨模块接口约定的时机问题，不是当前分支的 bug。
- **处理**：本分支保持现状，待任务 D 实现后按 plan.md C.5 顺序联调时再统一确认。
- **验证**：clean build 成功；QEMU 启动正常；`ls`、`forktest` 正常；`./test-xv6.py -q usertests` 输出 `ALL TESTS PASSED`。
- **关联提交**：`631ee68 feat(sched): implement aging for waiting processes`
- **状态**：待联调确认。
