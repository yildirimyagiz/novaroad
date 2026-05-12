#!/usr/bin/env python3
"""
Nova benchmarks runner v4 with:
- Platform-specific baselines
- Git metadata tracking
- Deterministic bootstrap seed
- Enhanced JSON metadata
"""

from __future__ import annotations

import argparse
import dataclasses
import datetime as dt
import json
import platform
import random
import re
import shutil
import statistics
import subprocess
import sys
import time
from pathlib import Path
from typing import Any, Optional

try:
    from zoneinfo import ZoneInfo
except Exception:
    ZoneInfo = None

try:
    import matplotlib.pyplot as plt
    import matplotlib
    matplotlib.use('Agg')
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


_TIME_RE = re.compile(r"^(real|user|sys)\s+([0-9.]+)\s*$", re.MULTILINE)


@dataclasses.dataclass(frozen=True)
class GitInfo:
    sha: Optional[str]
    branch: Optional[str]
    dirty: bool
    
    def to_dict(self) -> dict[str, Any]:
        return {"sha": self.sha, "branch": self.branch, "dirty": self.dirty}


@dataclasses.dataclass(frozen=True)
class EnvInfo:
    timestamp_istanbul: str
    os: str
    machine: str
    python: str
    cpu: Optional[str]
    platform_id: str
    git: GitInfo
    nova_version: Optional[str]
    
    def to_dict(self) -> dict[str, Any]:
        return {
            "timestamp_istanbul": self.timestamp_istanbul,
            "os": self.os,
            "machine": self.machine,
            "python": self.python,
            "cpu": self.cpu,
            "platform_id": self.platform_id,
            "git": self.git.to_dict(),
            "nova_version": self.nova_version,
        }


@dataclasses.dataclass(frozen=True)
class RunResult:
    real_s: float
    user_s: float
    sys_s: float
    stdout: str
    stderr: str


@dataclasses.dataclass(frozen=True)
class SummaryStats:
    mean: float
    median: float
    stdev: float
    min: float
    max: float
    cv_pct: float
    p5: float
    p95: float
    ci_lower: Optional[float] = None
    ci_upper: Optional[float] = None

    def to_dict(self) -> dict[str, Any]:
        return dataclasses.asdict(self)


@dataclasses.dataclass(frozen=True)
class BenchSummary:
    bench_path: str
    runs: int
    warmup: int
    real: SummaryStats
    user: SummaryStats
    sys: SummaryStats
    sample_stdout: str
    regression_vs_baseline_pct: Optional[float] = None
    all_real_times: Optional[list[float]] = None
    trim_percentiles: tuple[float, float] = (5.0, 95.0)
    bootstrap_seed: Optional[int] = None


def _run_cmd_text(cmd: list[str]) -> tuple[int, str, str]:
    proc = subprocess.run(cmd, text=True, capture_output=True, check=False)
    return proc.returncode, proc.stdout or "", proc.stderr or ""


def _git_info() -> GitInfo:
    """Extract git metadata."""
    sha = None
    rc, out, _ = _run_cmd_text(["git", "rev-parse", "HEAD"])
    if rc == 0:
        sha = out.strip()[:12]
    
    branch = None
    rc, out, _ = _run_cmd_text(["git", "rev-parse", "--abbrev-ref", "HEAD"])
    if rc == 0:
        branch = out.strip()
    
    dirty = False
    rc, out, _ = _run_cmd_text(["git", "status", "--porcelain"])
    if rc == 0 and out.strip():
        dirty = True
    
    return GitInfo(sha=sha, branch=branch, dirty=dirty)


def _nova_version(nova_exe: str) -> Optional[str]:
    """Get Nova version."""
    rc, out, _ = _run_cmd_text([nova_exe, "--version"])
    if rc == 0:
        return out.strip().split("\n")[0]
    return None


def _platform_id() -> str:
    """Platform identifier for baseline selection."""
    os_name = platform.system().lower()
    machine = platform.machine().lower()
    
    if machine in ("x86_64", "amd64"):
        arch = "x86_64"
    elif machine in ("aarch64", "arm64"):
        arch = "arm64"
    else:
        arch = machine
    
    return f"{os_name}-{arch}"


def _cpu_info_macos() -> Optional[str]:
    for key in ("machdep.cpu.brand_string", "hw.model"):
        rc, out, _ = _run_cmd_text(["sysctl", "-n", key])
        if rc == 0 and out.strip():
            return out.strip()
    return None


def _cpu_info_linux() -> Optional[str]:
    cpuinfo = Path("/proc/cpuinfo")
    if not cpuinfo.exists():
        return None
    try:
        text = cpuinfo.read_text(encoding="utf-8", errors="ignore")
        for line in text.splitlines():
            if line.lower().startswith("model name"):
                _, v = line.split(":", 1)
                return v.strip()
    except Exception:
        pass
    return None


def _read_cpu_info() -> Optional[str]:
    system = platform.system().lower()
    if system == "linux":
        return _cpu_info_linux()
    if system == "darwin":
        return _cpu_info_macos()
    val = (platform.processor() or "").strip()
    return val or None


def _now_istanbul_iso() -> str:
    if ZoneInfo is None:
        return dt.datetime.now().isoformat(timespec="seconds")
    return dt.datetime.now(tz=ZoneInfo("Europe/Istanbul")).isoformat(timespec="seconds")


def _env_info(nova_exe: str) -> EnvInfo:
    return EnvInfo(
        timestamp_istanbul=_now_istanbul_iso(),
        os=f"{platform.system()} {platform.release()}",
        machine=platform.machine(),
        python=sys.version.split()[0],
        cpu=_read_cpu_info(),
        platform_id=_platform_id(),
        git=_git_info(),
        nova_version=_nova_version(nova_exe),
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
        raise RuntimeError(f"Benchmark failed (exit={rc}).\n{stdout[-1000:]}")

    return RunResult(real_s=real_s, user_s=user_s, sys_s=sys_s, stdout=stdout, stderr=stderr)


def _percentile(xs: list[float], p: float) -> float:
    if not xs:
        return 0.0
    sorted_xs = sorted(xs)
    k = (len(sorted_xs) - 1) * (p / 100.0)
    f = int(k)
    c = int(k) + 1
    if c >= len(sorted_xs):
        return sorted_xs[-1]
    d0 = sorted_xs[f] * (c - k)
    d1 = sorted_xs[c] * (k - f)
    return d0 + d1


def _bootstrap_ci(
    xs: list[float],
    confidence: float = 0.95,
    n_bootstrap: int = 1000,
    seed: Optional[int] = None,
) -> tuple[float, float]:
    if len(xs) < 2:
        return (0.0, 0.0)
    
    if seed is not None:
        random.seed(seed)
    
    medians: list[float] = []
    for _ in range(n_bootstrap):
        sample = [random.choice(xs) for _ in range(len(xs))]
        medians.append(statistics.median(sample))
    
    medians.sort()
    alpha = 1.0 - confidence
    lower_idx = int(n_bootstrap * (alpha / 2))
    upper_idx = int(n_bootstrap * (1.0 - alpha / 2))
    
    return (medians[lower_idx], medians[upper_idx])


def _stats(
    xs: list[float],
    trim_low: float = 5.0,
    trim_high: float = 95.0,
    bootstrap_seed: Optional[int] = None,
) -> SummaryStats:
    if not xs:
        return SummaryStats(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, None, None)
    
    p_low = _percentile(xs, trim_low)
    p_high = _percentile(xs, trim_high)
    trimmed = [x for x in xs if p_low <= x <= p_high]
    
    if not trimmed:
        trimmed = xs
    
    mean = statistics.fmean(trimmed)
    median = statistics.median(trimmed)
    stdev = statistics.pstdev(trimmed) if len(trimmed) > 1 else 0.0
    mn = min(xs)
    mx = max(xs)
    cv = (stdev / mean * 100.0) if mean > 0 else 0.0
    
    ci_lower, ci_upper = None, None
    if len(trimmed) >= 5:
        ci_lower, ci_upper = _bootstrap_ci(trimmed, seed=bootstrap_seed)
    
    return SummaryStats(
        mean=mean,
        median=median,
        stdev=stdev,
        min=mn,
        max=mx,
        cv_pct=cv,
        p5=p_low,
        p95=p_high,
        ci_lower=ci_lower,
        ci_upper=ci_upper,
    )


def _render_report(env: EnvInfo, summaries: list[BenchSummary]) -> str:
    lines: list[str] = []
    lines.append("# Nova Unit Algebra - Benchmark Results (v4)")
    lines.append("")
    lines.append("**Platform-specific baselines • Git metadata • Deterministic bootstrap**")
    lines.append("")
    lines.append(f"**Date:** {env.timestamp_istanbul}")
    lines.append(f"**Platform:** {env.platform_id}")
    lines.append(f"**OS:** {env.os} ({env.machine})")
    lines.append(f"**CPU:** {env.cpu or 'unknown'}")
    lines.append(f"**Python:** {env.python}")
    if env.nova_version:
        lines.append(f"**Nova:** {env.nova_version}")
    if env.git.sha:
        lines.append(f"**Git:** {env.git.sha} ({env.git.branch}){' [dirty]' if env.git.dirty else ''}")
    lines.append("")

    for s in summaries:
        lines.append("---")
        lines.append("")
        lines.append(f"## Benchmark: `{s.bench_path}`")
        lines.append("")
        lines.append(f"**Runs:** {s.runs} (trimmed p{s.trim_percentiles[0]}-p{s.trim_percentiles[1]})  •  **Warmup:** {s.warmup}")
        if s.bootstrap_seed is not None:
            lines.append(f"**Bootstrap seed:** {s.bootstrap_seed}")
        if s.regression_vs_baseline_pct is not None:
            emoji = "✅" if s.regression_vs_baseline_pct <= 3.0 else "⚠️"
            lines.append(f"**Regression:** {emoji} {s.regression_vs_baseline_pct:+.2f}%")
        lines.append("")
        
        lines.append("### Timing")
        lines.append("")
        lines.append("| Metric | Median | Mean | StdDev | CV% | 95% CI |")
        lines.append("|--------|--------|------|--------|-----|--------|")
        
        for name, stat in [("real", s.real), ("user", s.user), ("sys", s.sys)]:
            ci_str = "-"
            if stat.ci_lower is not None and stat.ci_upper is not None:
                ci_str = f"[{stat.ci_lower:.6f}, {stat.ci_upper:.6f}]"
            lines.append(
                f"| {name} | {stat.median:.6f}s | {stat.mean:.6f}s | {stat.stdev:.6f}s | "
                f"{stat.cv_pct:.2f}% | {ci_str} |"
            )
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def _bench_summary(
    bench_path: str,
    warmup: int,
    measured: list[RunResult],
    trim_percentiles: tuple[float, float] = (5.0, 95.0),
    bootstrap_seed: Optional[int] = None,
) -> BenchSummary:
    real_s = [r.real_s for r in measured]
    user_s = [r.user_s for r in measured]
    sys_s = [r.sys_s for r in measured]

    return BenchSummary(
        bench_path=bench_path,
        runs=len(measured),
        warmup=warmup,
        real=_stats(real_s, trim_percentiles[0], trim_percentiles[1], bootstrap_seed),
        user=_stats(user_s, trim_percentiles[0], trim_percentiles[1], bootstrap_seed),
        sys=_stats(sys_s, trim_percentiles[0], trim_percentiles[1], bootstrap_seed),
        sample_stdout=measured[0].stdout if measured else "",
        all_real_times=real_s,
        trim_percentiles=trim_percentiles,
        bootstrap_seed=bootstrap_seed,
    )


def _summary_to_json(env: EnvInfo, summaries: list[BenchSummary]) -> dict[str, Any]:
    return {
        "env": env.to_dict(),
        "benchmarks": [
            {
                "bench_path": s.bench_path,
                "runs": s.runs,
                "warmup": s.warmup,
                "trim_percentiles": list(s.trim_percentiles),
                "bootstrap_seed": s.bootstrap_seed,
                "timing": {
                    "real": s.real.to_dict(),
                    "user": s.user.to_dict(),
                    "sys": s.sys.to_dict(),
                },
                "regression_vs_baseline_pct": s.regression_vs_baseline_pct,
            }
            for s in summaries
        ],
    }


def _load_baseline(path_or_dict: str | dict[str, Any]) -> dict[str, Any]:
    if isinstance(path_or_dict, dict):
        return path_or_dict
    p = Path(path_or_dict)
    if not p.exists():
        raise FileNotFoundError(f"Baseline not found: {path_or_dict}")
    return json.loads(p.read_text(encoding="utf-8"))


def _baseline_map(baseline: dict[str, Any]) -> dict[str, float]:
    out: dict[str, float] = {}
    for b in baseline.get("benchmarks", []):
        bench_path = str(b.get("bench_path", ""))
        median_real = b.get("timing", {}).get("real", {}).get("median", None)
        if bench_path and isinstance(median_real, (int, float)):
            out[bench_path] = float(median_real)
            out[Path(bench_path).name] = float(median_real)
    return out


def _apply_regression(summaries: list[BenchSummary], baseline_json: dict[str, Any]) -> list[BenchSummary]:
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


def _parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Nova benchmark runner v4")
    p.add_argument("--bench", action="append", required=True)
    p.add_argument("--runs", type=int, default=5)
    p.add_argument("--warmup", type=int, default=1)
    p.add_argument("--nova", default="./nova")
    p.add_argument("--out", default="BENCHMARK_RESULTS_V4.md")
    p.add_argument("--json", default=None)
    p.add_argument("--baseline-json", default=None)
    p.add_argument("--baseline-pack", default=None, help="Path to packed baseline bundle")
    p.add_argument("--baseline-dir", default="baselines", help="Directory for platform-specific baselines")
    p.add_argument("--max-regression-pct", type=float, default=3.0)
    p.add_argument("--fail-on-regression", action="store_true")
    p.add_argument("--bootstrap-seed", type=int, default=42, help="Seed for deterministic bootstrap")
    return p.parse_args(argv)


def main(argv: Optional[list[str]] = None) -> int:
    args = _parse_args(argv)
    nova_exe = _ensure_nova(args.nova)
    env = _env_info(nova_exe)

    # Auto-select platform-specific baseline if not specified
    baseline_val: str | dict[str, Any] | None = args.baseline_json
    if baseline_val is None and args.baseline_pack and Path(args.baseline_pack).exists():
        try:
            pack = json.loads(Path(args.baseline_pack).read_text(encoding="utf-8"))
            nova_base = pack.get("components", {}).get("nova")
            if nova_base:
                baseline_val = nova_base
                print(f"ℹ️  Using 'nova' component from pack: {args.baseline_pack}")
        except Exception as e:
            print(f"⚠️  Failed to load pack: {e}")

    if baseline_val is None and args.baseline_dir:
        platform_baseline = Path(args.baseline_dir) / f"{env.platform_id}.json"
        if platform_baseline.exists():
            baseline_val = str(platform_baseline)
            print(f"ℹ️  Using platform baseline: {baseline_val}")

    summaries: list[BenchSummary] = []
    for bench in args.bench:
        bench_path = str(Path(bench).as_posix())
        if not Path(bench_path).exists():
            raise FileNotFoundError(f"Benchmark not found: {bench_path}")

        print(f"Running {Path(bench_path).name} ({args.warmup}W + {args.runs}R)...", flush=True)
        
        for i in range(max(args.warmup, 0)):
            print(f"  Warmup {i+1}/{args.warmup}...", end=" ", flush=True)
            _run_once(nova_exe, bench_path)
            print("✓")

        measured: list[RunResult] = []
        for i in range(max(args.runs, 1)):
            print(f"  Run {i+1}/{args.runs}...", end=" ", flush=True)
            measured.append(_run_once(nova_exe, bench_path))
            print("✓")

        summaries.append(_bench_summary(bench_path, args.warmup, measured, bootstrap_seed=args.bootstrap_seed))

    if baseline_val:
        try:
            baseline_json = _load_baseline(baseline_val)
            summaries = _apply_regression(summaries, baseline_json)
        except FileNotFoundError as e:
            print(f"⚠️  {e}", file=sys.stderr)

    report = _render_report(env, summaries)
    Path(args.out).write_text(report, encoding="utf-8")
    print(f"\n✅ Report: {args.out}")

    if args.json:
        snapshot = _summary_to_json(env, summaries)
        Path(args.json).write_text(json.dumps(snapshot, indent=2), encoding="utf-8")
        print(f"✅ JSON: {args.json}")

    if args.fail_on_regression:
        for s in summaries:
            if s.regression_vs_baseline_pct is None:
                continue
            if s.regression_vs_baseline_pct > args.max_regression_pct:
                print(f"❌ Regression: {s.bench_path} {s.regression_vs_baseline_pct:+.2f}%", file=sys.stderr)
                return 2

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
