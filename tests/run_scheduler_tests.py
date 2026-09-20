#!/usr/bin/env python3
"""Build, boot, and measure real xv6 guests; standard-library-only Linux runner.

The three-policy experiment changes only scheduler code in disposable copies.
No test results are synthesized. Missing tools, panics, timeouts, and unexpected
guest failures produce a non-zero host exit status with the raw log retained.
"""
import argparse
import csv
import hashlib
import json
import os
import platform
from pathlib import Path
import re
import selectors
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
METRIC = re.compile(
    r"METRIC case=(\w+) id=(\d+) pid=(\d+) priority=(\d+) "
    r"start_ticks=(\d+) finish_ticks=(\d+) work=(\d+)"
)
AGING = re.compile(
    r"AGING_RESULT elapsed_ticks=(\d+) requested=(\d+) observed=(-?\d+) "
    r"high_alive=(\d+) timeout=(\d+)"
)


def version(command):
    p = subprocess.run(command, capture_output=True, text=True, check=True)
    return p.stdout.strip().splitlines()[0]


def replace_function(source, signature, replacement):
    if source.count(signature) != 1:
        raise RuntimeError("unexpected kernel layout: " + signature)
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for i in range(brace, len(source)):
        if source[i] == "{":
            depth += 1
        elif source[i] == "}":
            depth -= 1
            if depth == 0:
                return source[:start] + replacement + source[i + 1:]
    raise RuntimeError("unclosed function")


def prepare_policy(destination, policy):
    shutil.copytree(
        ROOT, destination,
        ignore=shutil.ignore_patterns(
            ".git", "results", "__pycache__", "*.o", "*.d", "*.asm",
            "*.sym", "fs.img", "_*", "usys.S", "test-xv6.out"
        ),
    )
    path = destination / "kernel/proc.c"
    original = path.read_text()
    if policy == "priority":
        changed = replace_function(
            original, "static void\napply_aging(struct proc *p)",
            "static void\napply_aging(struct proc *p)\n{\n  (void)p;\n}",
        )
    elif policy == "rr":
        changed = replace_function(original, "static void\napply_aging(struct proc *p)", "")
        changed = replace_function(
            changed, "void\nscheduler(void)",
            (ROOT / "tests/rr_scheduler.c").read_text().strip(),
        )
    else:
        changed = original
    path.write_text(changed)
    return hashlib.sha256(changed.encode()).hexdigest()


def build(root, log):
    with log.open("w") as out:
        for cmd in (["make", "clean"], ["make", "-j2", "kernel/kernel", "fs.img"]):
            out.write("$ " + " ".join(cmd) + "\n")
            out.flush()
            subprocess.run(cmd, cwd=root, stdout=out, stderr=subprocess.STDOUT,
                           timeout=180, check=True)


def guest(root, cpus, command, timeout, log_path, expected_failure=None):
    raw = bytearray()
    status = "FAIL"
    detail = "guest did not complete"
    started = time.monotonic()
    with tempfile.TemporaryDirectory(prefix="xv6-disk-") as tmp:
        disk = Path(tmp) / "fs.img"
        shutil.copyfile(root / "fs.img", disk)
        cmd = [
            "qemu-system-riscv64", "-machine", "virt", "-bios", "none",
            "-kernel", str(root / "kernel/kernel"), "-m", "128M", "-smp", str(cpus),
            "-nographic", "-monitor", "none", "-global", "virtio-mmio.force-legacy=false",
            "-drive", f"file={disk},if=none,format=raw,id=x0",
            "-device", "virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0",
        ]
        proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, bufsize=0)
        selector = selectors.DefaultSelector()
        selector.register(proc.stdout, selectors.EVENT_READ)
        phase = "boot"
        deadline = time.monotonic() + 30
        result_offset = 0
        try:
            while time.monotonic() < deadline:
                if not selector.select(timeout=0.25):
                    if proc.poll() is not None:
                        detail = "QEMU exited early"
                        break
                    continue
                chunk = os.read(proc.stdout.fileno(), 65536)
                if not chunk:
                    detail = "QEMU stdout closed"
                    break
                raw.extend(chunk)
                text = raw.decode("utf-8", "replace").replace("\r", "")
                if re.search(r"(^|\n)panic:", text):
                    detail = "kernel panic"
                    break
                if phase == "boot" and re.search(r"\$ $", text):
                    proc.stdin.write((command + "\n").encode())
                    proc.stdin.flush()
                    phase = "test"
                    deadline = time.monotonic() + timeout
                elif phase == "test":
                    if command.startswith("usertests"):
                        passed = re.search(r"^ALL TESTS PASSED[ \t]*\n", text, re.M)
                        failed = re.search(r"^[^\n]*(FAILED|FAIL)[^\n]*\n", text, re.M)
                    else:
                        name = command.split()[0]
                        passed = re.search(r"^" + name + r": PASS[ \t]*\n", text, re.M)
                        failed = re.search(r"^" + name + r": FAIL[^\n]*\n", text, re.M)
                    if failed:
                        if expected_failure and expected_failure in failed.group():
                            status, detail = "EXPECTED_FAIL", failed.group()
                        else:
                            detail = failed.group()
                            break
                        phase = "exit"
                        result_offset = failed.end()
                    elif passed:
                        if expected_failure:
                            detail = "negative control unexpectedly passed"
                            break
                        status, detail = "PASS", passed.group()
                        phase = "exit"
                        result_offset = passed.end()
                # Require return to shell and another functioning command.
                if phase == "exit" and "$ " in text[result_offset:]:
                    proc.stdin.write(b"echo TASK5_POST_EXIT_OK\n")
                    proc.stdin.flush()
                    phase = "alive"
                    deadline = time.monotonic() + 15
                if phase == "alive" and re.search(r"^TASK5_POST_EXIT_OK[ \t]*\n", text, re.M):
                    phase = "done"
                    break
            if phase != "done":
                status = "FAIL"
                if time.monotonic() >= deadline:
                    detail = f"host timeout in phase {phase}"
        finally:
            selector.close()
            proc.terminate()
            try:
                tail, _ = proc.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
                tail, _ = proc.communicate()
            raw.extend(tail)
            log_path.write_bytes(raw)
    text = raw.decode("utf-8", "replace")
    metric_keys = ["case", "id", "pid", "priority", "start_ticks", "finish_ticks", "work"]
    metrics = [dict(zip(metric_keys, [m[0]] + list(map(int, m[1:])))) for m in METRIC.findall(text)]
    a = AGING.search(text)
    aging = dict(zip(["elapsed_ticks", "requested", "observed", "high_alive", "timeout"],
                     map(int, a.groups()))) if a else None
    if expected_failure and command == "agingtest" and status == "EXPECTED_FAIL":
        if not aging or aging["timeout"] != 1 or aging["high_alive"] != 8 or aging["observed"] != 3:
            status, detail = "FAIL", "invalid no-aging negative-control evidence"
    return {"command": command, "cpus": cpus, "status": status, "detail": detail,
            "host_seconds": round(time.monotonic() - started, 3),
            "log": log_path.name, "metrics": metrics, "aging": aging}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cpus", type=int, choices=[1, 3], default=1)
    parser.add_argument("--repeat", type=int, default=3)
    parser.add_argument("--regression", choices=["none", "quick", "full"], default="quick")
    parser.add_argument("--compare", action="store_true")
    parser.add_argument("--out", type=Path, default=ROOT / "tests/results/latest")
    args = parser.parse_args()
    if not 1 <= args.repeat <= 10:
        parser.error("--repeat must be 1..10")
    if args.compare and args.cpus != 1:
        parser.error("controlled three-policy comparison uses one CPU")
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    if (out / "results.json").exists():
        parser.error("output directory already contains results; choose a new --out")
    results = {"created_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
               "cpus": args.cpus, "repeat": args.repeat, "runs": [], "policy_sha256": {}}
    results["environment"] = {"platform": platform.platform(),
                              "os_release": platform.freedesktop_os_release()}
    results["arguments"] = {**vars(args), "out": str(out)}
    # Record exact working files even when testing before the next Git commit.
    files = sorted(p for folder in ("kernel", "user", "tests")
                   for p in (ROOT / folder).rglob("*")
                   if p.is_file() and p.suffix in (".c", ".h", ".S", ".ld", ".pl", ".py")
                   and "results" not in p.parts and "__pycache__" not in p.parts)
    files.append(ROOT / "Makefile")
    results["source_sha256"] = {
        str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in files
    }
    try:
        results["source_commit"] = version(["git", "-C", str(ROOT), "rev-parse", "HEAD"])
    except subprocess.CalledProcessError:
        results["source_commit"] = "unavailable (source archive)"
    good = True
    try:
        results["qemu"] = version(["qemu-system-riscv64", "--version"])
        results["compiler"] = version(["riscv64-linux-gnu-gcc", "--version"])
        with tempfile.TemporaryDirectory(prefix="xv6-scheduler-tests-") as tmp:
            policies = ["rr", "priority", "aging"] if args.compare else ["aging"]
            for policy in policies:
                root = Path(tmp) / policy
                results["policy_sha256"][policy] = prepare_policy(root, policy)
                build(root, out / f"build-{policy}.log")
                commands = [("prioritytest bench", None)] if args.compare else [
                    ("prioritytest" + (" --smp" if args.cpus == 3 else ""), None),
                    ("agingtest", None),
                ]
                if args.compare and policy == "priority":
                    commands.append(("agingtest", "timeout while high-priority competitors remained alive"))
                if args.compare and policy == "aging":
                    commands.append(("agingtest", None))
                for repeat in range(1, args.repeat + 1):
                    for command, expected in commands:
                        label = command.replace(" ", "-")
                        r = guest(root, args.cpus, command, 150,
                                  out / f"{policy}-{label}-{repeat}.log", expected)
                        r.update(policy=policy, repeat=repeat)
                        results["runs"].append(r)
                        good = good and r["status"] != "FAIL"
                        print(f"{policy} CPU={args.cpus} run={repeat} {command}: {r['status']} {r['detail']}", flush=True)
                if args.compare and policy == "rr":
                    r = guest(root, 1, "prioritytest", 150, out / "rr-order-negative.log", "single-CPU priority isolation")
                    r.update(policy=policy, repeat=0)
                    results["runs"].append(r)
                    good = good and r["status"] == "EXPECTED_FAIL"
                if not args.compare and args.regression != "none":
                    command = "usertests" + (" -q" if args.regression == "quick" else "")
                    r = guest(root, args.cpus, command, 900, out / "usertests.log")
                    r.update(policy=policy, repeat=0)
                    results["runs"].append(r)
                    good = good and r["status"] == "PASS"
    except Exception as exc:
        good = False
        results["error"] = f"{type(exc).__name__}: {exc}"
        print(results["error"], flush=True)
    finally:
        results["success"] = good
        (out / "results.json").write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n")
        with (out / "metrics.csv").open("w", newline="") as f:
            fields = ["policy", "cpus", "repeat", "case", "id", "pid", "priority", "start_ticks", "finish_ticks", "work"]
            writer = csv.DictWriter(f, fieldnames=fields)
            writer.writeheader()
            for run in results["runs"]:
                for metric in run["metrics"]:
                    writer.writerow({**metric, **{k: run[k] for k in ["policy", "cpus", "repeat"]}})
    return 0 if good else 1


if __name__ == "__main__":
    raise SystemExit(main())
