"""
Read historical benchmark logs (JSONL) and generate a markdown trend summary.
"""

from __future__ import annotations

import argparse
import collections
import json
from pathlib import Path
from typing import Any


def _fmt_trend_spark(vals: list[float]) -> str:
    """Simple ASCII-ish trend sparkline."""
    if not vals:
        return ""
    if len(vals) < 2:
        return "→"
    v_last = vals[-1]
    v_prev = vals[-2]
    if v_last < v_prev * 0.98:
        return "📉"  # improve (if time) or degrade (if gflops)
    if v_last > v_prev * 1.02:
        return "📈"
    return "→"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--log", default="bench_history.jsonl", help="Historical log file")
    ap.add_argument("--last", type=int, default=10, help="Last N entries per metric")
    ap.add_argument("--out", default="bench_trend.md", help="Output Markdown summary")
    args = ap.parse_args()

    log_path = Path(args.log)
    if not log_path.exists():
        print(f"No log file found: {args.log}")
        return 0

    history = collections.defaultdict(list)
    try:
        with log_path.open("r", encoding="utf-8") as f:
            for line in f:
                if not line.strip():
                    continue
                r = json.loads(line)
                key = (r["module"], r["name"], r["unit"])
                history[key].append(r["val"])
    except Exception as e:
        print(f"Error reading log: {e}")
        return 1

    lines = ["### 📈 Performance Trends (Last 10 Runs)"]
    lines.append("")
    lines.append("| module | metric | latest | trend | avg (last 10) |")
    lines.append("|---|---|---|---|---|")

    # Sort keys for consistent output
    sorted_keys = sorted(history.keys())
    for key in sorted_keys:
        module, name, unit = key
        vals = history[key][-args.last:]
        latest = vals[-1]
        avg = sum(vals) / len(vals)
        trend = _fmt_trend_spark(vals)
        
        # Determine trend direction (some metrics like 's' want lower, some like 'gflops' want higher)
        # For simplicity, we just show up/down here.
        lines.append(f"| `{module}` | `{name}` | {latest:.6f} {unit} | {trend} | {avg:.6f} |")

    Path(args.out).write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"✅ wrote {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
