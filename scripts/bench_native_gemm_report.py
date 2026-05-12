"""
Nova native HGEMV/HGEMM benchmark runner (v4-safe, CI-friendly).

Fixes:
- Deterministic bootstrap seeding without Python's randomized hash()
- Proper regression sign conventions:
    - ms:   positive regression_pct => slower (bad)
    - gflops: positive regression_pct => lower throughput (bad)

Usage:
  python3 scripts/bench_native_gemm_report.py \
    --make-target bench-hgemv-build --bin bench/bench_hgemv \
    --make-target bench-hgemm-build --bin bench/bench_hgemm \
    --threads 1 4 \
    --runs 10 --warmup 2 \
    --trim 5 95 --bootstrap 1000 --seed 42 \
    --out NATIVE_V4_REPORT.md --json NATIVE_V4_REPORT.json

Regression:
  python3 scripts/bench_native_gemm_report.py \
    --make-target bench-hgemv-build --bin bench/bench_hgemv \
    --threads 1 4 \
    --runs 10 --warmup 2 \
    --baseline-json baselines/darwin-arm64.json \
    --max-regression-pct 3.0 --fail-on-regression
"""

from __future__ import annotations

import argparse
import dataclasses
import datetime as dt
import hashlib
import json
import math
import os
import platform
import random
import re
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


# Example lines:
# 4096x4096       51.84 GFLOPS  (   0.647 ms)
LINE_RE = re.compile(
    r"^\s*(\d+)\s*x\s*(\d+)\s+([0-9]*\.?[0-9]+)\s+GFLOPS\s+\(\s*([0-9]*\.?[0-9]+)\s*ms\)",
    re.IGNORECASE | re.MULTILINE,
)


def _run(cmd: list[str], env: Optional[dict[str, str]] = None) -> tuple[int, str, str]:
    proc = subprocess.run(cmd, text=True, capture_output=True, env=env, check=False)
    return proc.returncode, proc.stdout or "", proc.stderr or ""


def _now_istanbul_iso() -> str:
    if ZoneInfo is None:
        return dt.datetime.now().isoformat(timespec="seconds")
    return dt.datetime.now(tz=ZoneInfo("Europe/Istanbul")).isoformat(timespec="seconds")


def _stable_hash32(s: str) -> int:
    # Deterministic across processes/platforms (unlike Python hash()).
    h = hashlib.sha256(s.encode("utf-8")).digest()
    return int.from_bytes(h[:4], "little", signed=False)


def _detect_platform() -> str:
    os_s = platform.system().lower()
    if os_s.startswith("darwin") or os_s.startswith("mac"):
        os_s = "darwin"
    elif os_s.startswith("linux"):
        os_s = "linux"
    elif os_s.startswith("windows"):
        os_s = "windows"

    arch = platform.machine().lower()
    arch_map = {"amd64": "x86_64", "x86_64": "x86_64", "aarch64": "arm64", "arm64": "arm64"}
    arch = arch_map.get(arch, arch or "unknown")
    return f"{os_s}-{arch}"


def _cpu_info() -> str:
    if platform.system().lower() == "darwin":
        for key in ("machdep.cpu.brand_string", "hw.model"):
            rc, out, _ = _run(["sysctl", "-n", key])
            if rc == 0 and out.strip():
                return out.strip()
    return (platform.processor() or "unknown").strip()


def _git_meta() -> dict[str, Any]:
    def q(args: list[str]) -> Optional[str]:
        rc, out, _ = _run(["git", *args])
        if rc != 0:
            return None
        s = out.strip()
        return s or None

    sha = q(["rev-parse", "HEAD"])
    branch = q(["rev-parse", "--abbrev-ref", "HEAD"])
    rc, out, _ = _run(["git", "status", "--porcelain"])
    dirty = None if rc != 0 else bool(out.strip())
    return {"git_sha": sha, "git_branch": branch, "git_dirty": dirty}


def _parse_lines(stdout: str) -> list[tuple[str, float, float]]:
    out: list[tuple[str, float, float]] = []
    for m in LINE_RE.finditer(stdout):
        m_dim = int(m.group(1))
        k_dim = int(m.group(2))
        gflops = float(m.group(3))
        ms = float(m.group(4))
        out.append((f"{m_dim}x{k_dim}", gflops, ms))
    return out


def _quantile(xs: list[float], q: float) -> float:
    if not xs:
        return 0.0
    s = sorted(xs)
    if len(s) == 1:
        return s[0]
    pos = (len(s) - 1) * q
    lo = int(math.floor(pos))
    hi = int(math.ceil(pos))
    if lo == hi:
        return s[lo]
    w = pos - lo
    return s[lo] * (1.0 - w) + s[hi] * w


def _trim(xs: list[float], p_lo: float, p_hi: float) -> list[float]:
    if len(xs) < 5:
        return xs[:]  # avoid over-trimming small N
    lo = _quantile(xs, p_lo / 100.0)
    hi = _quantile(xs, p_hi / 100.0)
    kept = [x for x in xs if lo <= x <= hi]
    return kept if kept else xs[:]


def _bootstrap_ci_median(xs: list[float], iters: int, seed: int) -> tuple[float, float]:
    if not xs:
        return (0.0, 0.0)
    if len(xs) == 1:
        return (xs[0], xs[0])

    rng = random.Random(seed)
    n = len(xs)
    meds: list[float] = []
    for _ in range(iters):
        sample = [xs[rng.randrange(n)] for _ in range(n)]
        meds.append(statistics.median(sample))
    return (_quantile(meds, 0.025), _quantile(meds, 0.975))


@dataclasses.dataclass(frozen=True)
class Stats:
    median: float
    mean: float
    stdev: float
    cv_pct: float
    ci95: tuple[float, float]

    def to_dict(self) -> dict[str, Any]:
        return {
            "median": self.median,
            "mean": self.mean,
            "stdev": self.stdev,
            "cv_pct": self.cv_pct,
            "ci_95": [self.ci95[0], self.ci95[1]],
        }


def _stats(xs: list[float], bootstrap_iters: int, seed: int) -> Stats:
    if not xs:
        return Stats(0.0, 0.0, 0.0, 0.0, (0.0, 0.0))
    med = statistics.median(xs)
    mean = statistics.fmean(xs)
    st = statistics.pstdev(xs) if len(xs) > 1 else 0.0
    cv = (st / mean * 100.0) if mean > 0 else 0.0
    ci = _bootstrap_ci_median(xs, bootstrap_iters, seed)
    return Stats(median=med, mean=mean, stdev=st, cv_pct=cv, ci95=ci)


def _env_with_threads(threads: int) -> dict[str, str]:
    env = dict(os.environ)
    env["NOVA_NUM_THREADS"] = str(threads)
    return env


def _ensure_built(make_targets: list[str], script_dir: Path) -> None:
    for t in make_targets:
        rc, out, err = _run(["make", "-C", str(script_dir), t])
        if rc != 0:
            raise RuntimeError(f"make {t} failed\n--- stdout ---\n{out}\n--- stderr ---\n{err}\n")


def _load_baseline(path_or_dict: str | dict[str, Any]) -> dict[tuple[str, int, str], dict[str, float]]:
    """
    Returns mapping:
      (bench_name, threads, shape) -> {"gflops_median": x, "ms_median": y}
    Accepts baseline JSON produced by THIS script (snapshot["benches"] schema).
    """
    if isinstance(path_or_dict, dict):
        snap = path_or_dict
    else:
        snap = json.loads(Path(path_or_dict).read_text(encoding="utf-8"))
    
    out: dict[tuple[str, int, str], dict[str, float]] = {}

    for bench in snap.get("benches", []):
        name = bench.get("name")
        if not name:
            continue
        for th in bench.get("threads", []):
            t = th.get("threads")
            if not isinstance(t, int):
                continue
            for row in th.get("rows", []):
                shape = row.get("shape")
                if not isinstance(shape, str):
                    continue
                g = ((row.get("gflops") or {}).get("median"))
                ms = ((row.get("ms") or {}).get("median"))
                if isinstance(g, (int, float)) and isinstance(ms, (int, float)):
                    out[(name, t, shape)] = {"gflops_median": float(g), "ms_median": float(ms)}
    return out


def _regression_ms_pct(new_ms: float, base_ms: float) -> Optional[float]:
    if base_ms <= 0:
        return None
    return (new_ms - base_ms) / base_ms * 100.0  # positive => slower (bad)


def _regression_gflops_pct(new_g: float, base_g: float) -> Optional[float]:
    if base_g <= 0:
        return None
    return (base_g - new_g) / base_g * 100.0  # positive => lower throughput (bad)


def _render_md(snapshot: dict[str, Any]) -> str:
    env = snapshot["env"]
    lines: list[str] = []
    lines.append("# Nova Native GEMM Bench Results")
    lines.append("")
    lines.append(f"- **Date (Europe/Istanbul):** {env.get('timestamp_istanbul')}")
    lines.append(f"- **Platform:** `{env.get('platform')}`  •  **CPU:** `{env.get('cpu')}`")
    lines.append(f"- **Git:** `{env.get('git_sha')}` (`{env.get('git_branch')}`, dirty={env.get('git_dirty')})")
    lines.append(f"- **Trim:** p{env.get('trim_lo')}-p{env.get('trim_hi')}  •  **Bootstrap:** {env.get('bootstrap_iters')} (seed={env.get('seed')})")
    if env.get("baseline_json"):
        lines.append(f"- **Baseline:** `{env.get('baseline_json')}`")
    lines.append("")

    for bench in snapshot["benches"]:
        lines.append("---")
        lines.append("")
        lines.append(f"## `{bench['name']}`")
        lines.append("")
        for th in bench["threads"]:
            t = th["threads"]
            lines.append(f"### Threads = {t}")
            lines.append("")
            lines.append("| shape | GFLOPS median | GFLOPS CV% | GFLOPS 95% CI | GFLOPS reg% | ms median | ms CV% | ms 95% CI | ms reg% |")
            lines.append("|---|---:|---:|---:|---:|---:|---:|---:|---:|")
            for row in th["rows"]:
                g = row["gflops"]
                ms = row["ms"]
                reg = row.get("regression") or {}
                g_reg = reg.get("gflops_regression_pct")
                ms_reg = reg.get("ms_regression_pct")

                def fmt_reg(x: Any) -> str:
                    if not isinstance(x, (int, float)):
                        return "n/a"
                    badge = "✅" if x <= 0 else "⚠️"
                    return f"{x:+.2f}% {badge}"

                lines.append(
                    f"| `{row['shape']}` | {g['median']:.2f} | {g['cv_pct']:.2f} | [{g['ci_95'][0]:.2f}, {g['ci_95'][1]:.2f}] | {fmt_reg(g_reg)} | "
                    f"{ms['median']:.3f} | {ms['cv_pct']:.2f} | [{ms['ci_95'][0]:.3f}, {ms['ci_95'][1]:.3f}] | {fmt_reg(ms_reg)} |"
                )
            lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--make-target", action="append", default=[], help="Optional make target to build bench binary (repeatable).")
    ap.add_argument("--bin", action="append", required=True, help="Bench binary path (repeatable).")
    ap.add_argument("--threads", nargs="+", type=int, default=[1, 4], help="Thread counts to test.")
    ap.add_argument("--runs", type=int, default=10)
    ap.add_argument("--warmup", type=int, default=2)
    ap.add_argument("--trim", nargs=2, type=float, default=[5.0, 95.0])
    ap.add_argument("--bootstrap", type=int, default=1000)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--out", default="NATIVE_V4_REPORT.md")
    ap.add_argument("--json", default="NATIVE_V4_REPORT.json")
    ap.add_argument("--baseline-json", default=None)
    ap.add_argument("--baseline-pack", default=None, help="Path to packed baseline bundle")
    ap.add_argument("--max-regression-pct", type=float, default=3.0)
    ap.add_argument("--fail-on-regression", action="store_true")
    args = ap.parse_args()

    script_dir = Path(__file__).parent.parent / "src" / "native" / "ml" / "nova_gemm"

    if args.make_target:
        _ensure_built(args.make_target, script_dir)

    baseline_val: str | dict[str, Any] | None = args.baseline_json
    if baseline_val is None and args.baseline_pack and Path(args.baseline_pack).exists():
        try:
            pack = json.loads(Path(args.baseline_pack).read_text(encoding="utf-8"))
            native_base = pack.get("components", {}).get("native")
            if native_base:
                baseline_val = native_base
                print(f"ℹ️  Using 'native' component from pack: {args.baseline_pack}")
        except Exception as e:
            print(f"⚠️  Failed to load pack: {e}")

    baseline_map: Optional[dict[tuple[str, int, str], dict[str, float]]] = None
    if baseline_val:
        baseline_map = _load_baseline(baseline_val)
        if isinstance(baseline_val, str):
            print(f"ℹ️  Loaded baseline: {baseline_val}")

    env = {
        "timestamp_istanbul": _now_istanbul_iso(),
        "platform": _detect_platform(),
        "os": f"{platform.system()} {platform.release()}",
        "machine": platform.machine(),
        "python": sys.version.split()[0],
        "cpu": _cpu_info(),
        **_git_meta(),
        "trim_lo": float(args.trim[0]),
        "trim_hi": float(args.trim[1]),
        "bootstrap_iters": int(args.bootstrap),
        "seed": int(args.seed),
        "runs": int(args.runs),
        "warmup": int(args.warmup),
        "threads": list(map(int, args.threads)),
        "baseline_json": args.baseline_json,
    }

    worst_regression = 0.0
    benches_out: list[dict[str, Any]] = []

    for bin_path in args.bin:
        abs_p = script_dir / bin_path
        if not abs_p.exists():
            raise FileNotFoundError(f"Bench binary not found: {abs_p}")

        bench_name = abs_p.name
        bench_threads: list[dict[str, Any]] = []

        for t in args.threads:
            print(f"Benchmarking {bench_name} (threads={t})...")

            for _ in range(max(0, args.warmup)):
                rc, _, err = _run([str(abs_p)], env=_env_with_threads(t))
                if rc != 0:
                    raise RuntimeError(f"Warmup failed for {bench_name} threads={t}\n{err}")

            per_shape_g: dict[str, list[float]] = {}
            per_shape_ms: dict[str, list[float]] = {}

            for _ in range(max(1, args.runs)):
                rc, out, err = _run([str(abs_p)], env=_env_with_threads(t))
                if rc != 0:
                    raise RuntimeError(f"Run failed for {bench_name} threads={t}\n--- stderr ---\n{err}")
                rows = _parse_lines(out)
                if not rows:
                    raise RuntimeError(f"Could not parse output for {bench_name} threads={t}\n--- stdout ---\n{out}")
                for shape, gflops, ms in rows:
                    per_shape_g.setdefault(shape, []).append(gflops)
                    per_shape_ms.setdefault(shape, []).append(ms)

            rows_out: list[dict[str, Any]] = []
            for shape in sorted(per_shape_g.keys(), key=lambda s: (int(s.split("x")[0]), int(s.split("x")[1]))):
                g_raw = per_shape_g[shape]
                ms_raw = per_shape_ms[shape]

                g = _trim(g_raw, env["trim_lo"], env["trim_hi"])
                ms = _trim(ms_raw, env["trim_lo"], env["trim_hi"])

                seed_g = env["seed"] ^ _stable_hash32(f"{bench_name}|{t}|{shape}|gflops")
                seed_ms = env["seed"] ^ _stable_hash32(f"{bench_name}|{t}|{shape}|ms")

                g_stats = _stats(g, env["bootstrap_iters"], seed_g)
                ms_stats = _stats(ms, env["bootstrap_iters"], seed_ms)

                reg: dict[str, Any] = {}
                if baseline_map is not None:
                    b = baseline_map.get((bench_name, int(t), shape))
                    if b:
                        g_reg = _regression_gflops_pct(g_stats.median, b["gflops_median"])
                        ms_reg = _regression_ms_pct(ms_stats.median, b["ms_median"])
                        reg["gflops_regression_pct"] = g_reg
                        reg["ms_regression_pct"] = ms_reg

                        for v in (g_reg, ms_reg):
                            if isinstance(v, (int, float)):
                                worst_regression = max(worst_regression, float(v))

                rows_out.append(
                    {
                        "shape": shape,
                        "gflops": g_stats.to_dict(),
                        "ms": ms_stats.to_dict(),
                        "regression": reg or None,
                        "samples": {"gflops": g_raw, "ms": ms_raw},
                    }
                )

            bench_threads.append({"threads": int(t), "rows": rows_out})

        benches_out.append({"name": bench_name, "threads": bench_threads})

    snapshot = {"env": env, "benches": benches_out}
    Path(args.json).write_text(json.dumps(snapshot, indent=2, ensure_ascii=False), encoding="utf-8")
    Path(args.out).write_text(_render_md(snapshot), encoding="utf-8")
    print(f"✅ wrote {args.out}")
    print(f"✅ wrote {args.json}")

    if args.fail_on_regression and baseline_map is not None:
        if worst_regression > float(args.max_regression_pct):
            print(
                f"❌ Regression exceeded: {worst_regression:.2f}% > {args.max_regression_pct:.2f}%",
                file=sys.stderr,
            )
            return 2

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
