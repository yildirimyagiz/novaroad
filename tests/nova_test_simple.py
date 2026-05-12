#!/usr/bin/env python3
"""nova_test.py — Single-file test runner for Nova compiler"""

from __future__ import annotations
import argparse, dataclasses, hashlib, os, re, shlex, subprocess, sys, tempfile
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

DIRECTIVE_RE = re.compile(r"^\s*(?://|#)\s*([A-Z0-9_-]+)\s*:\s*(.*?)\s*$", re.IGNORECASE)
DEFAULT_TIMEOUT_S = 20
TEST_EXTS = {".nova", ".zn"}

@dataclasses.dataclass
class Directives:
    run: str = "run"
    args: List[str] = dataclasses.field(default_factory=list)
    timeout_s: int = DEFAULT_TIMEOUT_S
    env: Dict[str, str] = dataclasses.field(default_factory=dict)
    expect_exit: Optional[int] = None
    expect_stdout: Optional[str] = None
    expect_stderr: Optional[str] = None
    expect_error_substr: Optional[str] = None
    determinism: Optional[str] = None
    skip: Optional[str] = None
    xfail: Optional[str] = None

@dataclasses.dataclass
class RunResult:
    cmd: List[str]
    exit_code: int
    stdout: str
    stderr: str
    elapsed_ms: int

@dataclasses.dataclass
class TestCase:
    path: Path
    rel: str
    directives: Directives

def _unescape_quoted(s: str) -> str:
    s = s.strip()
    if not s: return s
    if (s.startswith('"') and s.endswith('"')) or (s.startswith("'") and s.endswith("'")):
        return bytes(s[1:-1], "utf-8").decode("unicode_escape")
    return s

def parse_directives(text: str) -> Directives:
    d = Directives()
    for line in text.splitlines():
        if not line.strip(): continue
        if not (line.lstrip().startswith("//") or line.lstrip().startswith("#")): break
        m = DIRECTIVE_RE.match(line)
        if not m: continue
        key, val = m.group(1).strip().upper(), m.group(2).strip()
        
        if key == "RUN": d.run = val.lower()
        elif key == "ARGS": d.args.extend(shlex.split(val))
        elif key == "TIMEOUT": 
            try: d.timeout_s = int(val)
            except: pass
        elif key == "ENV":
            if "=" in val:
                k, v = val.split("=", 1)
                d.env[k.strip()] = _unescape_quoted(v.strip())
        elif key == "EXPECT-EXIT":
            try: d.expect_exit = int(val)
            except: pass
        elif key == "EXPECT-STDOUT": d.expect_stdout = _unescape_quoted(val)
        elif key == "EXPECT-STDERR": d.expect_stderr = _unescape_quoted(val)
        elif key == "EXPECT-ERROR": d.expect_error_substr = _unescape_quoted(val)
        elif key == "DETERMINISM": d.determinism = val.lower()
        elif key == "SKIP": d.skip = val or "skipped"
        elif key == "XFAIL": d.xfail = val or "expected failure"
    return d

def discover_tests(root: Path) -> List[TestCase]:
    tests = []
    for p in sorted(root.rglob("*")):
        if not p.is_file() or p.suffix.lower() not in TEST_EXTS: continue
        try:
            text = p.read_text(encoding="utf-8", errors="replace")
            d = parse_directives(text)
            tests.append(TestCase(path=p, rel=str(p.relative_to(root)), directives=d))
        except: pass
    return tests

def run_subprocess(cmd: Sequence[str], *, cwd=None, env=None, timeout_s=DEFAULT_TIMEOUT_S) -> RunResult:
    import time
    t0 = time.time()
    p = subprocess.run(list(cmd), cwd=str(cwd) if cwd else None, env=env,
                      stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=timeout_s)
    return RunResult(cmd=list(cmd), exit_code=p.returncode, stdout=p.stdout, 
                    stderr=p.stderr, elapsed_ms=int((time.time()-t0)*1000))

def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--nova", type=Path, default=Path("./nova"))
    ap.add_argument("--tests", type=Path, default=Path("tests"))
    ap.add_argument("-k", "--filter", default=None)
    ap.add_argument("--list", action="store_true")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args(argv)
    
    nova = args.nova.resolve()
    if not nova.exists():
        print(f"error: nova not found: {nova}", file=sys.stderr)
        return 2
    
    tests = discover_tests(args.tests.resolve())
    if args.filter:
        tests = [t for t in tests if args.filter in t.rel]
    
    if args.list:
        for t in tests:
            print(f"{t.rel} (RUN={t.directives.run})")
        return 0
    
    total = passed = failed = 0
    for tc in tests:
        total += 1
        d = tc.directives
        if d.skip:
            print(f"⏭️  {tc.rel} (skip)")
            continue
        
        # Simple run test
        cmd = [str(nova), str(tc.path)]
        cmd.extend(d.args)
        
        try:
            res = run_subprocess(cmd, timeout_s=d.timeout_s)
            ok = True
            msg = "ok"
            
            if d.expect_exit is not None and res.exit_code != d.expect_exit:
                ok, msg = False, f"exit: expected {d.expect_exit}, got {res.exit_code}"
            elif d.expect_error_substr and d.expect_error_substr not in (res.stdout + res.stderr):
                ok, msg = False, f"error substring not found: {d.expect_error_substr}"
            
            if ok:
                passed += 1
                print(f"✅ {tc.rel}")
            else:
                failed += 1
                print(f"❌ {tc.rel}: {msg}")
        except Exception as e:
            failed += 1
            print(f"💥 {tc.rel}: {e}")
    
    print(f"\nSummary: total={total} pass={passed} fail={failed}")
    return 0 if failed == 0 else 1

if __name__ == "__main__":
    raise SystemExit(main())
