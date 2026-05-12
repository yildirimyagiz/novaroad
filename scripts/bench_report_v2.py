#!/usr/bin/env python3
"""
Nova benchmarks runner + Markdown/JSON report generator (v2).

Usage:
  python3 scripts/bench_report_v2.py \
    --bench benchmarks/unit_bench_working.zn \
    --runs 10 \
    --warmup 2 \
    --out BENCHMARK_RESULTS.md \
    --json BENCHMARK_SNAPSHOT.json

Regression guard (CI):
  python3 scripts/bench_report_v2.py \
    --bench benchmarks/unit_bench_working.zn \
    --runs 10 \
    --warmup 2 \
    --out BENCHMARK_RESULTS.md \
    --json BENCHMARK_SNAPSHOT.json \
    --baseline-json BENCHMARK_BASELINE.json \
    --max-regression-pct 3.0 \
    --fail-on-regression

With plotting:
  python3 scripts/bench_report_v2.py \
    --bench benchmarks/unit_bench_working.zn \
    --runs 10 \
    --warmup 2 \
    --out BENCHMARK_RESULTS.md \
    --plot BENCHMARK_PLOT.png
"""

from __future__ import annotations

import argparse
import dataclasses
import datetime as dt
import json
import platform
import re
import shutil
import statistics
import subprocess
import sys
import time
from pathlib import Path
from typing import Any, Optional

try:
    from zoneinfo import ZoneInfo  # py3.9+
except Exception:  # pragma: no cover
    ZoneInfo = None  # type: ignore[assignment]

# Optional matplotlib for plotting
try:
    import matplotlib.pyplot as plt
    import matplotlib
    matplotlib.use('Agg')  # Non-interactive backend
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


_TIME_RE = re.compile(r"^(real|user|sys)\s+([0-9.]+)\s*$", re.MULTILINE)
_EXEC_TIME_RE = re.compile(
    r"Execution Time:\s*([0-9]*\.?[0-9]+)s\s*\(([\d,_. ]+)\s*operations\)",
    re.IGNORECASE,
)
_THROUGHPUT_RE = re.compile(
    r"Throughput:\s*~?\s*([\d,_. ]+)\s*(?:ops/s|işlem/saniye|operations/s)",
    re.IGNORECASE,
)


@dataclasses.dataclass(frozen=True)
class EnvInfo:
    timestamp_istanbul: str
    os: str
    machine: str
    python: str
    cpu: Optional[str]


@dataclasses.dataclass(frozen=True)
class RunResult:
    real_s: float
    user_s: float
    sys_s: float
    stdout: str
    stderr: str
    parsed_exec_time_s: Optional[float] = None
    parsed_operations: Optional[int] = None
    parsed_throughput_ops_s: Optional[float] = None


@dataclasses.dataclass(frozen=True)
class SummaryStats:
    mean: float
    median: float
    stdev: float
    min: float
    max: float
    cv_pct: float  # coefficient of variation (%)

    def to_dict(self) -> dict[str, float]:
        return dataclasses.asdict(self)


@dataclasses.dataclass(frozen=True)
class BenchSummary:
    bench_path: str
    runs: int
    warmup: int
    real: SummaryStats
    user: SummaryStats
    sys: SummaryStats
    exec_time: Optional[SummaryStats]
    operations_mean: Optional[float]
    throughput_mean: Optional[float]
    sample_stdout: str
    regression_vs_baseline_pct: Optional[float] = None
    all_real_times: Optional[list[float]] = None  # For plotting


def _now_istanbul_iso() -> str:
    if ZoneInfo is None:
        return dt.datetime.now().isoformat(timespec="seconds")
    return dt.datetime.now(tz=ZoneInfo("Europe/Istanbul")).isoformat(timespec="seconds")


def _run_cmd_text(cmd: list[str]) -> tuple[int, str, str]:
    proc = subprocess.run(cmd, text=True, capture_output=True, check=False)
    return proc.returncode, proc.stdout or "", proc.stderr or ""


def _cpu_info_linux() -> Optional[str]:
    cpuinfo = Path("/proc/cpuinfo")
    if not cpuinfo.exists():
        return None
    try:
        text = cpuinfo.read_text(encoding="utf-8", errors="ignore")
    except Exception:
        return None

    model = None
    mhz = None
    for line in text.splitlines():
        if model is None and line.lower().startswith("model name"):
            _, v = line.split(":", 1)
            model = v.strip()
        if mhz is None and line.lower().startswith("cpu mhz"):
            _, v = line.split(":", 1)
            mhz = v.strip()
        if model and mhz:
            break

    if not model and not mhz:
        return None
    if model and mhz:
        return f"{model} @ {mhz} MHz"
    return model or mhz


def _cpu_info_macos() -> Optional[str]:
    # Apple Silicon: machdep.cpu.brand_string often returns "Apple M1/M2/..."
    for key in ("machdep.cpu.brand_string", "hw.model"):
        rc, out, _ = _run_cmd_text(["sysctl", "-n", key])
        if rc == 0:
            val = out.strip()
            if val:
                return val
    return None


def _read_cpu_info() -> Optional[str]:
    system = platform.system().lower()
    if system == "linux":
        return _cpu_info_linux()
    if system == "darwin":
        return _cpu_info_macos()
    val = (platform.processor() or "").strip()
    return val or None


def _env_info() -> EnvInfo:
    return EnvInfo(
        timestamp_istanbul=_now_istanbul_iso(),
        os=f"{platform.system()} {platform.release()}",
        machine=platform.machine(),
        python=sys.version.split()[0],
        cpu=_read_cpu_info(),
    )


def _ensure_nova(nova: str) -> str:
    if "/" in nova or nova.startswith("./"):
        p = Path(nova)
        if not p.exists():
            raise FileNotFoundError(f"nova executable not found: {nova}")
        return str(p.resolve())

    found = shutil.which(nova)
    if not found:
        raise FileNotFoundError(f"Executable not found in PATH: {nova}")
    return found


def _run_once(nova_exe: str, bench_file: str) -> RunResult:
    usr_time = Path("/usr/bin/time")
    use_usr_time = usr_time.exists()

    cmd = [nova_exe, "run", bench_file]
    if use_usr_time:
        full_cmd = [str(usr_time), "-p", *cmd]
        rc, stdout, stderr = _run_cmd_text(full_cmd)

        times = dict(_TIME_RE.findall(stderr))
        real_s = float(times.get("real", "0") or "0")
        user_s = float(times.get("user", "0") or "0")
        sys_s = float(times.get("sys", "0") or "0")
    else:
        t0 = time.perf_counter()
        rc, stdout, stderr = _run_cmd_text(cmd)
        real_s = time.perf_counter() - t0
        user_s = 0.0
        sys_s = 0.0

    if rc != 0:
        raise RuntimeError(
            f"Benchmark failed (exit={rc}).\n"
            f"CMD: {' '.join(cmd)}\n"
            f"--- STDOUT ---\n{stdout[-2000:]}\n"
            f"--- STDERR ---\n{stderr[-2000:]}\n"
        )

    parsed_exec_time_s = None
    parsed_operations = None
    em = _EXEC_TIME_RE.search(stdout)
    if em:
        parsed_exec_time_s = float(em.group(1))
        ops_raw = re.sub(r"[^\d]", "", em.group(2))
        if ops_raw:
            parsed_operations = int(ops_raw)

    parsed_throughput_ops_s = None
    tm = _THROUGHPUT_RE.search(stdout)
    if tm:
        t_raw = re.sub(r"[^\d.]", "", tm.group(1).replace(",", ""))
        if t_raw:
            try:
                parsed_throughput_ops_s = float(t_raw)
            except ValueError:
                parsed_throughput_ops_s = None

    return RunResult(
        real_s=real_s,
        user_s=user_s,
        sys_s=sys_s,
        stdout=stdout,
        stderr=stderr,
        parsed_exec_time_s=parsed_exec_time_s,
        parsed_operations=parsed_operations,
        parsed_throughput_ops_s=parsed_throughput_ops_s,
    )


def _stats(xs: list[float]) -> SummaryStats:
    if not xs:
        return SummaryStats(mean=0.0, median=0.0, stdev=0.0, min=0.0, max=0.0, cv_pct=0.0)
    mean = statistics.fmean(xs)
    median = statistics.median(xs)
    stdev = statistics.pstdev(xs) if len(xs) > 1 else 0.0
    mn = min(xs)
    mx = max(xs)
    cv = (stdev / mean * 100.0) if mean > 0 else 0.0
    return SummaryStats(mean=mean, median=median, stdev=stdev, min=mn, max=mx, cv_pct=cv)


def _trim_sample(text: str, limit_chars: int = 3000) -> str:
    t = (text or "").strip()
    if len(t) <= limit_chars:
        return t
    return t[:limit_chars] + "\n… (truncated)\n"


def _render_report(env: EnvInfo, summaries: list[BenchSummary]) -> str:
    lines: list[str] = []
    lines.append("# Nova Unit Algebra - Benchmark Results (v2)")
    lines.append("")
    lines.append(f"**Date (Europe/Istanbul):** {env.timestamp_istanbul}")
    lines.append(f"**OS:** {env.os} ({env.machine})")
    lines.append(f"**Python:** {env.python}")
    if env.cpu:
        lines.append(f"**CPU:** {env.cpu}")
    lines.append("")

    for s in summaries:
        lines.append("---")
        lines.append("")
        lines.append(f"## Benchmark: `{s.bench_path}`")
        lines.append("")
        lines.append(f"**Warmup:** {s.warmup} (excluded)  •  **Measured runs:** {s.runs}")
        if s.regression_vs_baseline_pct is not None:
            emoji = "✅" if s.regression_vs_baseline_pct <= 3.0 else "⚠️"
            lines.append(f"**Regression vs baseline (median real):** {emoji} {s.regression_vs_baseline_pct:.2f}%")
        lines.append("")
        lines.append("### Timing")
        lines.append("")
        lines.append("| Metric | Mean (s) | Median (s) | StdDev (s) | Min (s) | Max (s) | CV % |")
        lines.append("|---|---:|---:|---:|---:|---:|---:|")
        lines.append(
            f"| real | {s.real.mean:.6f} | {s.real.median:.6f} | {s.real.stdev:.6f} | "
            f"{s.real.min:.6f} | {s.real.max:.6f} | {s.real.cv_pct:.2f} |"
        )
        lines.append(
            f"| user | {s.user.mean:.6f} | {s.user.median:.6f} | {s.user.stdev:.6f} | "
            f"{s.user.min:.6f} | {s.user.max:.6f} | {s.user.cv_pct:.2f} |"
        )
        lines.append(
            f"| sys  | {s.sys.mean:.6f} | {s.sys.median:.6f} | {s.sys.stdev:.6f} | "
            f"{s.sys.min:.6f} | {s.sys.max:.6f} | {s.sys.cv_pct:.2f} |"
        )
        lines.append("")

        if s.exec_time is not None:
            lines.append("### Parsed benchmark output")
            lines.append("")
            lines.append("| Field | Mean | Median | StdDev |")
            lines.append("|---|---:|---:|---:|")
            lines.append(
                f"| Execution Time (s) | {s.exec_time.mean:.6f} | {s.exec_time.median:.6f} | "
                f"{s.exec_time.stdev:.6f} |"
            )
            if s.operations_mean is not None:
                lines.append(f"| Operations | {s.operations_mean:.0f} | - | - |")
            if s.throughput_mean is not None:
                lines.append(f"| Throughput (ops/s) | {s.throughput_mean:.0f} | - | - |")
            lines.append("")

        lines.append("<details>")
        lines.append("<summary>Sample stdout (first measured run)</summary>")
        lines.append("")
        lines.append("```text")
        lines.append(_trim_sample(s.sample_stdout))
        lines.append("```")
        lines.append("</details>")
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def _bench_summary(bench_path: str, warmup: int, measured: list[RunResult]) -> BenchSummary:
    real_s = [r.real_s for r in measured]
    user_s = [r.user_s for r in measured]
    sys_s = [r.sys_s for r in measured]

    exec_s = [r.parsed_exec_time_s for r in measured if r.parsed_exec_time_s is not None]
    ops = [r.parsed_operations for r in measured if r.parsed_operations is not None]
    thr = [r.parsed_throughput_ops_s for r in measured if r.parsed_throughput_ops_s is not None]

    exec_stats = _stats(exec_s) if exec_s else None
    ops_mean = statistics.fmean(ops) if ops else None
    thr_mean = statistics.fmean(thr) if thr else None

    return BenchSummary(
        bench_path=bench_path,
        runs=len(measured),
        warmup=warmup,
        real=_stats(real_s),
        user=_stats(user_s),
        sys=_stats(sys_s),
        exec_time=exec_stats,
        operations_mean=ops_mean,
        throughput_mean=thr_mean,
        sample_stdout=measured[0].stdout if measured else "",
        all_real_times=real_s,
    )


def _summary_to_json(env: EnvInfo, summaries: list[BenchSummary]) -> dict[str, Any]:
    return {
        "env": dataclasses.asdict(env),
        "benchmarks": [
            {
                "bench_path": s.bench_path,
                "warmup": s.warmup,
                "runs": s.runs,
                "timing": {
                    "real": s.real.to_dict(),
                    "user": s.user.to_dict(),
                    "sys": s.sys.to_dict(),
                },
                "parsed": None
                if s.exec_time is None
                else {
                    "execution_time": s.exec_time.to_dict(),
                    "operations_mean": s.operations_mean,
                    "throughput_mean": s.throughput_mean,
                },
                "regression_vs_baseline_pct": s.regression_vs_baseline_pct,
            }
            for s in summaries
        ],
    }


def _load_baseline(path: str) -> dict[str, Any]:
    p = Path(path)
    if not p.exists():
        raise FileNotFoundError(f"Baseline JSON not found: {path}")
    return json.loads(p.read_text(encoding="utf-8"))


def _baseline_map(baseline: dict[str, Any]) -> dict[str, float]:
    out: dict[str, float] = {}
    for b in baseline.get("benchmarks", []):
        bench_path = str(b.get("bench_path", ""))
        median_real = (
            b.get("timing", {})
            .get("real", {})
            .get("median", None)
        )
        if bench_path and isinstance(median_real, (int, float)):
            out[bench_path] = float(median_real)
            out[Path(bench_path).name] = float(median_real)
    return out


def _apply_regression(
    summaries: list[BenchSummary],
    baseline_json: dict[str, Any],
) -> list[BenchSummary]:
    bm = _baseline_map(baseline_json)
    updated: list[BenchSummary] = []
    for s in summaries:
        base = bm.get(s.bench_path) or bm.get(Path(s.bench_path).name)
        if base is None or base <= 0:
            updated.append(s)
            continue
        reg = (s.real.median - base) / base * 100.0
        updated.append(dataclasses.replace(s, regression_vs_baseline_pct=reg))
    return updated


def _plot_benchmarks(summaries: list[BenchSummary], out_path: str) -> None:
    if not HAS_MATPLOTLIB:
        print("⚠️  matplotlib not available, skipping plot", file=sys.stderr)
        return

    fig, ax = plt.subplots(figsize=(10, 6))
    
    for s in summaries:
        if not s.all_real_times:
            continue
        name = Path(s.bench_path).name
        runs = list(range(1, len(s.all_real_times) + 1))
        ax.plot(runs, s.all_real_times, marker='o', label=name, linewidth=2)
        
        # Add median line
        ax.axhline(y=s.real.median, linestyle='--', alpha=0.5, 
                   label=f'{name} median: {s.real.median:.6f}s')
    
    ax.set_xlabel('Run Number', fontsize=12)
    ax.set_ylabel('Time (seconds)', fontsize=12)
    ax.set_title('Nova Unit Algebra Benchmark - Run Times', fontsize=14, fontweight='bold')
    ax.legend(loc='best', fontsize=10)
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(out_path, dpi=150)
    print(f"✅ Saved plot: {out_path}")


def _parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Nova benchmark runner v2 with warmup, regression guard, and plotting")
    p.add_argument(
        "--bench",
        action="append",
        required=True,
        help="Benchmark .zn file path (repeatable)",
    )
    p.add_argument("--runs", type=int, default=5, help="Measured runs per benchmark")
    p.add_argument("--warmup", type=int, default=1, help="Warmup runs per benchmark (excluded)")
    p.add_argument("--nova", default="./nova", help="Path to nova executable")
    p.add_argument("--out", default="BENCHMARK_RESULTS.md", help="Output markdown path")
    p.add_argument("--json", default=None, help="Optional JSON snapshot output")
    p.add_argument("--baseline-json", default=None, help="Optional baseline snapshot JSON")
    p.add_argument("--max-regression-pct", type=float, default=3.0, help="Allowed regression in percent")
    p.add_argument("--fail-on-regression", action="store_true", help="Exit non-zero if regression exceeds threshold")
    p.add_argument("--plot", default=None, help="Optional PNG plot output path")
    return p.parse_args(argv)


def main(argv: Optional[list[str]] = None) -> int:
    args = _parse_args(argv)
    env = _env_info()
    nova_exe = _ensure_nova(args.nova)

    summaries: list[BenchSummary] = []
    for bench in args.bench:
        bench_path = str(Path(bench).as_posix())
        if not Path(bench_path).exists():
            raise FileNotFoundError(f"Benchmark file not found: {bench_path}")

        print(f"Running {Path(bench_path).name} ({args.warmup} warmup + {args.runs} measured)...", flush=True)
        
        # Warmup runs
        for i in range(max(args.warmup, 0)):
            print(f"  Warmup {i+1}/{args.warmup}...", end=" ", flush=True)
            _run_once(nova_exe, bench_path)
            print("✓")

        # Measured runs
        measured: list[RunResult] = []
        for i in range(max(args.runs, 1)):
            print(f"  Run {i+1}/{args.runs}...", end=" ", flush=True)
            measured.append(_run_once(nova_exe, bench_path))
            print("✓")

        summaries.append(_bench_summary(bench_path, args.warmup, measured))

    baseline_json = None
    if args.baseline_json:
        baseline_json = _load_baseline(args.baseline_json)
        summaries = _apply_regression(summaries, baseline_json)

    report = _render_report(env, summaries)
    Path(args.out).write_text(report, encoding="utf-8")
    print(f"\n✅ Wrote report: {args.out}")

    if args.json:
        snapshot = _summary_to_json(env, summaries)
        Path(args.json).write_text(json.dumps(snapshot, indent=2, ensure_ascii=False), encoding="utf-8")
        print(f"✅ Wrote JSON snapshot: {args.json}")

    if args.plot:
        _plot_benchmarks(summaries, args.plot)

    if args.fail_on_regression:
        for s in summaries:
            if s.regression_vs_baseline_pct is None:
                continue
            if s.regression_vs_baseline_pct > args.max_regression_pct:
                print(
                    f"❌ Regression exceeded: {s.bench_path} "
                    f"{s.regression_vs_baseline_pct:.2f}% > {args.max_regression_pct:.2f}%",
                    file=sys.stderr,
                )
                return 2

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
