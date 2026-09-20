#!/usr/bin/env python3
"""Print a Markdown summary from actual runner results (no synthetic data)."""
import argparse
from collections import Counter
import json
from pathlib import Path
import statistics


def summary(paths):
    documents = [json.loads(Path(p).read_text()) for p in paths]
    print('| 数据集 | PASS | EXPECTED_FAIL | FAIL | 总体通过 |')
    print('|---|---:|---:|---:|---|')
    for path, doc in zip(paths, documents):
        counts = Counter(r['status'] for r in doc['runs'])
        print(f"| {Path(path).parent.name} | {counts['PASS']} | {counts['EXPECTED_FAIL']} | {counts['FAIL']} | {doc['success']} |")
    benches = [r for doc in documents for r in doc['runs']
               if r['command'] == 'prioritytest bench' and r['status'] == 'PASS']
    if benches:
        print('\n单核混合优先级负载，统一释放起点；单位为 uptime tick。')
        print('\n| 策略 | 优先级 | n | 开始延迟中位数 | 完成历时中位数 | 完成历时范围 |')
        print('|---|---:|---:|---:|---:|---|')
        for policy in ('rr', 'priority', 'aging'):
            for priority in (3, 10, 20):
                points = [m for r in benches if r['policy'] == policy
                          for m in r['metrics']
                          if m['case'] == 'mixed' and m['priority'] == priority]
                if not points:
                    continue
                starts = [m['start_ticks'] for m in points]
                finishes = [m['finish_ticks'] for m in points]
                print(f'| {policy} | {priority} | {len(points)} | {statistics.median(starts):g} | {statistics.median(finishes):g} | {min(finishes)}–{max(finishes)} |')
    print('\n| 数据集/策略 | 轮次 | Aging结果 | 获得运行/超时 tick | 实测优先级 | 高优先级存活数 |')
    print('|---|---:|---|---:|---:|---:|')
    for path, doc in zip(paths, documents):
        for r in doc['runs']:
            a = r.get('aging')
            if a:
                print(f"| {Path(path).parent.name}/{r['policy']} | {r['repeat']} | {r['status']} | {a['elapsed_ticks']} | {a['observed']} | {a['high_alive']} |")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('results', nargs='+', type=Path)
    summary(parser.parse_args().results)
