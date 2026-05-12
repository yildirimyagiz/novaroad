"""
Render a single unified PR comment / Step Summary from:
- bench_report_v4.py snapshot (default: BENCHMARK_SNAPSHOT.json)
- bench_native_gemm_report.py snapshot (default: NATIVE_V4_REPORT.json)

Usage:
  python3 scripts/render_unified_bench_comment.py \
    --nova BENCHMARK_SNAPSHOT.json \
    --native NATIVE_V4_REPORT.json \
    --out bench_comment.md
"""

from __future__ import annotations

import argparse
import json
import os
import re
from pathlib import Path
from typing import Any, Optional, Tuple


def _require_keys(doc: dict[str, Any], kind: str, required: list[str]) -> None:
    missing = [k for k in required if k not in doc]
    if missing:
        raise ValueError(f"{kind} snapshot missing keys: {missing}")


def _run_url() -> Optional[str]:
    server = os.getenv("GITHUB_SERVER_URL")
    repo = os.getenv("GITHUB_REPOSITORY")
    run_id = os.getenv("GITHUB_RUN_ID")
    if server and repo and run_id:
        return f"{server}/{repo}/actions/runs/{run_id}"
    return None


def _fmt_float(x: Any, nd: int) -> str:
    if isinstance(x, (int, float)):
        return f"{float(x):.{nd}f}"
    return "n/a"


def _fmt_pct_badge(x: Any) -> str:
    if not isinstance(x, (int, float)):
        return "n/a"
    badge = "⚠️" if float(x) > 0 else "✅"
    return f"{float(x):+.2f}% {badge}"


def _fmt_cv(x: Any) -> str:
    if isinstance(x, (int, float)):
        return f"{float(x):.2f}"
    return "n/a"


def _fmt_ci(ci: Any, nd: int) -> str:
    if ci is None:
        return "n/a"
    if isinstance(ci, (list, tuple)) and len(ci) == 2 and all(isinstance(v, (int, float)) for v in ci):
        return f"[{ci[0]:.{nd}f}, {ci[1]:.{nd}f}]"
    if isinstance(ci, dict) and "low" in ci and "high" in ci:
        try:
            lo = float(ci["low"])
            hi = float(ci["high"])
            return f"[{lo:.{nd}f}, {hi:.{nd}f}]"
        except Exception:
            return "n/a"
    return "n/a"


def _pick_env(nova: Optional[dict[str, Any]], native: Optional[dict[str, Any]]) -> dict[str, Any]:
    env: dict[str, Any] = {}
    if nova and isinstance(nova.get("env"), dict):
        env.update(nova["env"])
    if native and isinstance(native.get("env"), dict):
        for k, v in native["env"].items():
            env.setdefault(k, v)
    return env


def _parse_nova_rows(nova: dict[str, Any]) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for b in nova.get("benchmarks", []) or []:
        if not isinstance(b, dict):
            continue
        name = b.get("bench_path") or b.get("name") or "(unknown)"
        timing = b.get("timing") or {}
        real = timing.get("real") or {}
        rows.append(
            {
                "name": str(name),
                "median": real.get("median"),
                "cv": real.get("cv_pct"),
                "ci": real.get("ci_95") or real.get("ci95") or b.get("ci_95"),
                "reg": b.get("regression_vs_baseline_pct"),
            }
        )
    return rows


_SHAPE_RE = re.compile(r"^\s*(\d+)\s*x\s*(\d+)\s*$")


def _shape_key(s: str) -> tuple[int, int, str]:
    m = _SHAPE_RE.match(s)
    if not m:
        return (10**9, 10**9, s)
    return (int(m.group(1)), int(m.group(2)), s)


def _parse_native_rows(native: dict[str, Any]) -> list[dict[str, Any]]:
    out: list[dict[str, Any]] = []
    for bench in native.get("benches", []) or []:
        if not isinstance(bench, dict):
            continue
        bench_name = bench.get("name") or "(unknown)"
        for th in bench.get("threads") or []:
            if not isinstance(th, dict):
                continue
            threads = th.get("threads")
            if not isinstance(threads, int):
                continue
            for row in th.get("rows") or []:
                if not isinstance(row, dict):
                    continue
                shape = row.get("shape")
                if not isinstance(shape, str):
                    continue
                g = row.get("gflops") or {}
                ms = row.get("ms") or {}
                reg = row.get("regression") or {}
                out.append(
                    {
                        "bench": str(bench_name),
                        "threads": int(threads),
                        "shape": shape,
                        "g_median": g.get("median"),
                        "g_cv": g.get("cv_pct"),
                        "g_ci": g.get("ci_95"),
                        "g_reg": reg.get("gflops_regression_pct"),
                        "ms_median": ms.get("median"),
                        "ms_cv": ms.get("cv_pct"),
                        "ms_ci": ms.get("ci_95"),
                        "ms_reg": reg.get("ms_regression_pct"),
                    }
                )
    out.sort(key=lambda r: (r["bench"], r["threads"], _shape_key(r["shape"])))
    return out


def _top_regressions(nova: Optional[dict[str, Any]], native: Optional[dict[str, Any]], count: int = 3) -> list[tuple[float, str]]:
    """
    Returns sorted list (descending by regression %) of (pct, context).
    Only includes positive (bad) regressions.
    
    Priority: ms regressions get a tiny 'tie-breaker' boost in sorting 
    to prefer them for triage if magnitudes are identical.
    """
    regs: list[tuple[float, str, float]] = []

    if nova:
        for r in _parse_nova_rows(nova):
            v = r.get("reg")
            if isinstance(v, (int, float)) and float(v) > 0:
                # Nova benchmarks are typically execution time (ms-like)
                regs.append((float(v), f"Nova: {r['name']} (real)", float(v) + 0.001))

    if native:
        for r in _parse_native_rows(native):
            # Prioritize ms regressions slightly for triage
            for metric_key, v in (("ms", r.get("ms_reg")), ("gflops", r.get("g_reg"))):
                if isinstance(v, (int, float)) and float(v) > 0:
                    weight = float(v) + (0.001 if metric_key == "ms" else 0.0)
                    regs.append((float(v), f"Native: {r['bench']} thr={r['threads']} {r['shape']} ({metric_key})", weight))

    # Sort by weight (descending)
    regs.sort(key=lambda x: x[2], reverse=True)
    return [(v, ctx) for v, ctx, w in regs[:count]]


def _top_jitter(nova: Optional[dict[str, Any]], native: Optional[dict[str, Any]], count: int = 1) -> list[tuple[float, str]]:
    """
    Returns sorted list (descending by CV%) of (cv_pct, context).
    """
    jitters: list[tuple[float, str]] = []

    if nova:
        for r in _parse_nova_rows(nova):
            v = r.get("cv")
            if isinstance(v, (int, float)):
                jitters.append((float(v), f"Nova: {r['name']}"))

    if native:
        for r in _parse_native_rows(native):
            # Take the max CV between GFLOPS and MS for that row
            v_g = r.get("g_cv")
            v_ms = r.get("ms_cv")
            v = max(float(v_g) if isinstance(v_g, (int, float)) else 0.0,
                    float(v_ms) if isinstance(v_ms, (int, float)) else 0.0)
            jitters.append((v, f"Native: {r['bench']} thr={r['threads']} {r['shape']}"))

    jitters.sort(key=lambda x: x[0], reverse=True)
    return jitters[:count]


def render(nova: Optional[dict[str, Any]], native: Optional[dict[str, Any]]) -> str:
    env = _pick_env(nova, native)

    platform_s = env.get("platform") or f"{env.get('os','?')} {env.get('machine','?')}"
    cpu_s = env.get("cpu") or "n/a"
    sha = env.get("git_sha") or env.get("sha") or "n/a"
    branch = env.get("git_branch") or env.get("branch") or "n/a"
    dirty = env.get("git_dirty")
    dirty_s = "dirty" if dirty is True else ("clean" if dirty is False else "n/a")

    top_regs = _top_regressions(nova, native, 3)
    top_jitters = _top_jitter(nova, native, 1)

    lines: list[str] = []
    lines.append("<!-- nova-bench -->")
    lines.append("## 🚀 Unified Benchmark Report")
    lines.append("")
    lines.append(f"- **Platform:** `{platform_s}`  •  **CPU:** `{cpu_s}`")
    lines.append(f"- **Git:** `{sha}` (`{branch}`, {dirty_s})")
    
    if top_regs:
        lines.append("- **Top Regressions:**")
        for pct, ctx in top_regs:
            lines.append(f"  - {pct:+.2f}% ⚠️ ({ctx})")
    else:
        lines.append("- **Regressions:** ✅ No regressions detected.")

    if top_jitters:
        cv, ctx = top_jitters[0]
        badge = "⚠️" if cv > 3.0 else "✅"
        lines.append(f"- **Highest Jitter:** {cv:.2f}% {badge} ({ctx})")

    run_url = _run_url()
    if run_url:
        lines.append(f"- **Run:** {run_url}")
    lines.append("")

    if nova:
        nova_rows = _parse_nova_rows(nova)
        lines.append("### Nova (high-level)")
        lines.append("")
        if not nova_rows:
            lines.append("_No Nova benchmark rows found._")
        else:
            lines.append("| benchmark | real median (s) | CV% | 95% CI | regression |")
            lines.append("|---|---:|---:|---:|---:|")
            for r in nova_rows:
                lines.append(
                    f"| `{r['name']}` | {_fmt_float(r['median'], 6)} | {_fmt_cv(r['cv'])} | {_fmt_ci(r['ci'], 6)} | {_fmt_pct_badge(r['reg'])} |"
                )
        lines.append("")

    if native:
        native_rows = _parse_native_rows(native)
        lines.append("### Native kernels (HGEMV/HGEMM)")
        lines.append("")
        if not native_rows:
            lines.append("_No native kernel rows found._")
        else:
            lines.append("| kernel | thr | shape | GFLOPS median | CV% | 95% CI | reg | ms median | CV% | 95% CI | reg |")
            lines.append("|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|")
            for r in native_rows:
                lines.append(
                    f"| `{r['bench']}` | {r['threads']} | `{r['shape']}` | "
                    f"{_fmt_float(r['g_median'], 2)} | {_fmt_cv(r['g_cv'])} | {_fmt_ci(r['g_ci'], 2)} | {_fmt_pct_badge(r['g_reg'])} | "
                    f"{_fmt_float(r['ms_median'], 3)} | {_fmt_cv(r['ms_cv'])} | {_fmt_ci(r['ms_ci'], 3)} | {_fmt_pct_badge(r['ms_reg'])} |"
                )
        lines.append("")

    lines.append("> Not: Pozitif regression% = kötü (daha yavaş / daha düşük throughput).")
    return "\n".join(lines).rstrip() + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--nova", default=None)
    ap.add_argument("--native", default=None)
    ap.add_argument("--out", default="bench_comment.md")
    args = ap.parse_args()

    nova = json.loads(Path(args.nova).read_text(encoding="utf-8")) if args.nova else None
    native = json.loads(Path(args.native).read_text(encoding="utf-8")) if args.native else None
    
    if nova:
        _require_keys(nova, "Nova", ["env", "benchmarks"])
    if native:
        _require_keys(native, "Native", ["env", "benches"])

    if not nova and not native:
        raise SystemExit("Provide at least one of --nova or --native")

    Path(args.out).write_text(render(nova, native), encoding="utf-8")
    print(f"✅ wrote {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
