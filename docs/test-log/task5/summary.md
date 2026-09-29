| 数据集 | PASS | EXPECTED_FAIL | FAIL | 总体通过 |
|---|---:|---:|---:|---|
| single | 7 | 0 | 0 | True |
| smp | 7 | 0 | 0 | True |
| compare | 12 | 4 | 0 | True |

单核混合优先级负载，统一释放起点；单位为 uptime tick。

| 策略 | 优先级 | n | 开始延迟中位数 | 完成历时中位数 | 完成历时范围 |
|---|---:|---:|---:|---:|---|
| rr | 3 | 3 | 0 | 9 | 9–9 |
| rr | 10 | 3 | 1 | 9 | 9–9 |
| rr | 20 | 3 | 2 | 9 | 9–9 |
| priority | 3 | 3 | 0 | 4 | 4–4 |
| priority | 10 | 3 | 4 | 8 | 8–9 |
| priority | 20 | 3 | 8 | 12 | 12–14 |
| aging | 3 | 3 | 0 | 3 | 3–3 |
| aging | 10 | 3 | 3 | 6 | 6–7 |
| aging | 20 | 3 | 6 | 10 | 9–10 |

| 数据集/策略 | 轮次 | Aging结果 | 获得运行/超时 tick | 实测优先级 | 高优先级存活数 |
|---|---:|---|---:|---:|---:|
| single/aging | 1 | PASS | 136 | 0 | 8 |
| single/aging | 2 | PASS | 136 | 0 | 8 |
| single/aging | 3 | PASS | 136 | 0 | 8 |
| smp/aging | 1 | PASS | 47 | 0 | 8 |
| smp/aging | 2 | PASS | 47 | 0 | 8 |
| smp/aging | 3 | PASS | 46 | 0 | 8 |
| compare/priority | 1 | EXPECTED_FAIL | 256 | 3 | 8 |
| compare/priority | 2 | EXPECTED_FAIL | 256 | 3 | 8 |
| compare/priority | 3 | EXPECTED_FAIL | 256 | 3 | 8 |
| compare/aging | 1 | PASS | 136 | 0 | 8 |
| compare/aging | 2 | PASS | 136 | 0 | 8 |
| compare/aging | 3 | PASS | 136 | 0 | 8 |
