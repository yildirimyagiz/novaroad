"""
Pack Nova + Native baseline JSONs into a single multi-module baseline bundle.

Why:
- Single artifact for baseline distribution/auditing.
- Keeps original per-module baselines intact (stored verbatim).

Output schema:
{
  "schema": "nova-baseline-pack/v1",
  "generated_at": "...",
  "platform": "darwin-arm64",
  "components": {
     "nova":   <original nova baseline json>,
     "native": <original native baseline json>
  }
}

Usage:
  python3 scripts/pack_baseline.py \
    --nova baselines/darwin-arm64.json \
    --native baselines/native_darwin-arm64.json \
    --out baselines/pack_darwin-arm64.json
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
from pathlib import Path
from typing import Any, Optional


def _load_json(path: str) -> dict[str, Any]:
    p = Path(path)
    if not p.exists():
        raise FileNotFoundError(f"File not found: {path}")
    try:
        return json.loads(p.read_text(encoding="utf-8"))
    except json.JSONDecodeError as e:
        raise ValueError(f"Invalid JSON: {path}: {e}") from e


def _infer_platform(doc: dict[str, Any]) -> Optional[str]:
    env = doc.get("env")
    if isinstance(env, dict):
        # Handle both bench_report_v4 and bench_native_gemm_report platform keys
        plat = env.get("platform") or env.get("platform_id")
        if isinstance(plat, str) and plat.strip():
            return plat.strip()
    return None


def _basic_validate_nova(doc: dict[str, Any]) -> None:
    if not isinstance(doc.get("env"), dict):
        raise ValueError("Nova baseline missing 'env' object.")
    if "benchmarks" not in doc:
        raise ValueError("Nova baseline missing 'benchmarks' field.")
    if not isinstance(doc.get("benchmarks"), list):
        raise ValueError("Nova baseline 'benchmarks' must be a list.")


def _basic_validate_native(doc: dict[str, Any]) -> None:
    if not isinstance(doc.get("env"), dict):
        raise ValueError("Native baseline missing 'env' object.")
    if "benches" not in doc:
        raise ValueError("Native baseline missing 'benches' field.")
    if not isinstance(doc.get("benches"), list):
        raise ValueError("Native baseline 'benches' must be a list.")


def _now_iso() -> str:
    return dt.datetime.utcnow().replace(microsecond=0).isoformat() + "Z"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--nova", required=True, help="Nova baseline JSON (bench_report_v4.py output).")
    ap.add_argument("--native", required=True, help="Native baseline JSON (bench_native_gemm_report.py output).")
    ap.add_argument("--out", required=True, help="Output packed baseline JSON.")
    ap.add_argument("--platform", default=None, help="Override platform string.")
    ap.add_argument("--force", action="store_true", help="Allow platform mismatch.")
    args = ap.parse_args()

    nova = _load_json(args.nova)
    native = _load_json(args.native)

    _basic_validate_nova(nova)
    _basic_validate_native(native)

    nova_plat = _infer_platform(nova)
    native_plat = _infer_platform(native)

    platform_out = args.platform or nova_plat or native_plat
    if not platform_out:
        raise ValueError("Could not infer platform from inputs; pass --platform.")

    if not args.force:
        mismatches = []
        if nova_plat and nova_plat != platform_out:
            mismatches.append(f"nova.env.platform={nova_plat}")
        if native_plat and native_plat != platform_out:
            mismatches.append(f"native.env.platform={native_plat}")
        if mismatches:
            raise ValueError(
                "Platform mismatch. Use --platform to override or --force to ignore.\n"
                + "\n".join(mismatches)
            )

    packed = {
        "schema": "nova-baseline-pack/v1",
        "generated_at": _now_iso(),
        "platform": platform_out,
        "components": {
            "nova": nova,
            "native": native,
        },
    }

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(packed, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"✅ wrote packed baseline: {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
