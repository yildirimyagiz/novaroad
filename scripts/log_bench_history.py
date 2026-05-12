"""
Append benchmark results to a historical log (JSONL) for trend analysis.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
from pathlib import Path
from typing import Any


def _extract_nova(nova: dict[str, Any]) -> list[dict[str, Any]]:
    out = []
    env = nova.get("env", {})
    ts = env.get("timestamp_istanbul") or dt.datetime.now().isoformat()
    sha = env.get("git_sha") or "unknown"
    plat = env.get("platform") or env.get("platform_id") or "unknown"

    for b in nova.get("benchmarks", []) or []:
        name = b.get("bench_path") or b.get("name")
        median = b.get("timing", {}).get("real", {}).get("median")
        if name and median is not None:
            out.append({
                "ts": ts,
                "sha": sha,
                "platform": plat,
                "module": "nova",
                "name": name,
                "val": float(median),
                "unit": "s"
            })
    return out


def _extract_native(native: dict[str, Any]) -> list[dict[str, Any]]:
    out = []
    env = native.get("env", {})
    ts = env.get("timestamp_istanbul") or dt.datetime.now().isoformat()
    sha = env.get("git_sha") or env.get("sha") or "unknown"
    plat = env.get("platform") or "unknown"

    for bench in native.get("benches", []) or []:
        name = bench.get("name")
        for th in bench.get("threads", []) or []:
            t = th.get("threads")
            for row in th.get("rows", []) or []:
                shape = row.get("shape")
                # We log both GFLOPS and MS
                g = row.get("gflops", {}).get("median")
                ms = row.get("ms", {}).get("median")
                if name and t and shape:
                    if g is not None:
                        out.append({
                            "ts": ts,
                            "sha": sha,
                            "platform": plat,
                            "module": "native",
                            "name": f"{name}_{t}th_{shape}_gflops",
                            "val": float(g),
                            "unit": "gflops"
                        })
                    if ms is not None:
                        out.append({
                            "ts": ts,
                            "sha": sha,
                            "platform": plat,
                            "module": "native",
                            "name": f"{name}_{t}th_{shape}_ms",
                            "val": float(ms),
                            "unit": "ms"
                        })
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--nova", help="Nova snapshot JSON")
    ap.add_argument("--native", help="Native snapshot JSON")
    ap.add_argument("--log", default="bench_history.jsonl", help="Historical log file (append-only)")
    args = ap.parse_args()

    records = []
    if args.nova:
        records.extend(_extract_nova(json.loads(Path(args.nova).read_text(encoding="utf-8"))))
    if args.native:
        records.extend(_extract_native(json.loads(Path(args.native).read_text(encoding="utf-8"))))

    if not records:
        print("No records found to log.")
        return 0

    log_path = Path(args.log)
    with log_path.open("a", encoding="utf-8") as f:
        for r in records:
            f.write(json.dumps(r, ensure_ascii=False) + "\n")

    print(f"✅ Appended {len(records)} records to {args.log}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
