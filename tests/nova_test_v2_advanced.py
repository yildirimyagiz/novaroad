#!/usr/bin/env python3
"""
nova_test.py — Single-file test runner for the Nova compiler.

Quick start
  python3 nova_test.py --nova ./nova --tests ./tests
  python3 nova_test.py -k borrow
  python3 nova_test.py --list

Supported test file extensions
  - .nova
  - .zn

Directives (top of each test file)
  // RUN: run|aot            Run program (AOT)                 (default: run)
  // RUN: jit                Run program via JIT               (nova run <file> --jit)
  // RUN: check              Type-check only                  (nova check <file>)
  // RUN: parse              Parse+check (per CLI)            (nova <file>)
  // RUN: emit-ir|ir         Emit LLVM IR (captured to file)   (nova <file> --emit-ir)
  // RUN: emit-ast|ast       Emit AST dump (captured to file)  (nova <file> --emit-ast)
  // RUN: build|exe          Build executable (artifact)       (nova <file> -o <out>)
  // ARGS: ...               Extra CLI args for nova
  // TIMEOUT: 10             Seconds (default: 20)
  // ENV: KEY=VALUE          Repeatable; per-test environment

Expectations
  // EXPECT-EXIT: 0          Exact exit code
  // EXPECT-STDOUT: "..."    Exact stdout match
  // EXPECT-STDERR: "..."    Exact stderr match
  // EXPECT-ERROR: "..."     Substring in (stderr + stdout) (repeatable)
  // EXPECT: stdout "..."    Substring in stdout (repeatable)
  // EXPECT: stderr "..."    Substring in stderr (repeatable)
  // EXPECT: exit 42         Exit code (alias)
  // EXPECT: error "..."     Substring in (stderr + stdout) (alias; repeatable)
  // EXPECT: fail            Exit code must be non-zero (alias)
  // DETERMINISM: ir|ast|exe|stdout   Run twice; compare sha256 of artifact or stdout
  // SKIP: reason
  // XFAIL: reason           Expected failure; won't fail the suite

Defaults
- If RUN is omitted, defaults to RUN=run.
- If RUN is omitted BUT an error expectation exists (EXPECT-ERROR / EXPECT: error / EXPECT: fail),
  it defaults to RUN=check (so negative tests don't accidentally execute).

Matching behavior
- Substring expectations are case-insensitive by default.
- Use --case-sensitive to require exact casing for substring expectations.

Template overrides (if your CLI changes)
  --template-run      "nova run {file}"
  --template-jit      "nova run {file} --jit"
  --template-check    "nova check {file}"
  --template-parse    "nova {file}"
  --template-emit-ir  "nova {file} --emit-ir"
  --template-emit-ast "nova {file} --emit-ast"
  --template-build    "nova {file} -o {out}"

Placeholders:
  {file} : test file path
  {out}  : output artifact path (for build/emit)
"""

from __future__ import annotations

import argparse
import dataclasses
import hashlib
import os
import re
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple


DIRECTIVE_RE = re.compile(r"^\s*(?://|#)\s*([A-Z0-9_-]+)\s*:\s*(.*?)\s*$", re.IGNORECASE)
DEFAULT_TIMEOUT_S = 20
TEST_EXTS = {".nova", ".zn"}


@dataclasses.dataclass
class Directives:
    run: str = "run"
    run_explicit: bool = False

    args: List[str] = dataclasses.field(default_factory=list)
    timeout_s: int = DEFAULT_TIMEOUT_S
    env: Dict[str, str] = dataclasses.field(default_factory=dict)

    expect_exit: Optional[int] = None
    expect_exit_nonzero: bool = False

    expect_stdout: Optional[str] = None  # exact
    expect_stderr: Optional[str] = None  # exact

    expect_stdout_contains: List[str] = dataclasses.field(default_factory=list)
    expect_stderr_contains: List[str] = dataclasses.field(default_factory=list)
    expect_error_contains: List[str] = dataclasses.field(default_factory=list)  # stderr+stdout

    determinism: Optional[str] = None  # ir|ast|exe|stdout

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
    if not s:
        return s
    if (s.startswith('"') and s.endswith('"')) or (s.startswith("'") and s.endswith("'")):
        inner = s[1:-1]
        return bytes(inner, "utf-8").decode("unicode_escape")
    return s


def parse_directives(text: str) -> Directives:
    d = Directives()
    for line in text.splitlines():
        if not line.strip():
            continue
        if not (line.lstrip().startswith("//") or line.lstrip().startswith("#")):
            break
        m = DIRECTIVE_RE.match(line)
        if not m:
            continue
        key = m.group(1).strip().upper()
        val = m.group(2).strip()

        if key == "RUN":
            d.run = val.strip().lower()
            d.run_explicit = True
        elif key == "ARGS":
            d.args.extend(shlex.split(val))
        elif key == "TIMEOUT":
            try:
                d.timeout_s = int(val.strip())
            except ValueError:
                pass
        elif key == "ENV":
            kv = val.strip()
            if "=" in kv:
                k, v = kv.split("=", 1)
                d.env[k.strip()] = _unescape_quoted(v.strip())
        elif key == "EXPECT-EXIT":
            try:
                d.expect_exit = int(val.strip())
            except ValueError:
                pass
        elif key == "EXPECT-STDOUT":
            d.expect_stdout = _unescape_quoted(val)
        elif key == "EXPECT-STDERR":
            d.expect_stderr = _unescape_quoted(val)
        elif key == "EXPECT-ERROR":
            d.expect_error_contains.append(_unescape_quoted(val))
        elif key == "EXPECT":
            parts = val.split(None, 1)
            if not parts:
                continue
            kind = parts[0].strip().lower()
            rest = parts[1].strip() if len(parts) > 1 else ""

            if kind in {"stdout", "out"}:
                d.expect_stdout_contains.append(_unescape_quoted(rest))
            elif kind in {"stderr", "err"}:
                d.expect_stderr_contains.append(_unescape_quoted(rest))
            elif kind in {"exit", "code"}:
                try:
                    d.expect_exit = int(rest.strip())
                except ValueError:
                    pass
            elif kind in {"error", "fail"}:
                if kind == "fail" and not rest:
                    d.expect_exit_nonzero = True
                else:
                    d.expect_error_contains.append(_unescape_quoted(rest))
            elif kind in {"nonzero", "nz"}:
                d.expect_exit_nonzero = True
        elif key == "DETERMINISM":
            d.determinism = val.strip().lower()
        elif key == "SKIP":
            d.skip = val.strip() or "skipped"
        elif key == "XFAIL":
            d.xfail = val.strip() or "expected failure"

    if not d.run_explicit:
        if d.expect_exit_nonzero or d.expect_error_contains:
            d.run = "check"

    return d


def discover_tests(root: Path) -> List[TestCase]:
    tests: List[TestCase] = []
    for p in sorted(root.rglob("*")):
        if not p.is_file():
            continue
        if p.suffix.lower() not in TEST_EXTS:
            continue
        try:
            text = p.read_text(encoding="utf-8", errors="replace")
        except Exception:
            continue
        d = parse_directives(text)
        rel = str(p.relative_to(root))
        tests.append(TestCase(path=p, rel=rel, directives=d))
    return tests


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def sha256_text(s: str) -> str:
    return hashlib.sha256(s.encode("utf-8", errors="replace")).hexdigest()


def run_subprocess(
    cmd: Sequence[str],
    *,
    cwd: Optional[Path] = None,
    env: Optional[Dict[str, str]] = None,
    timeout_s: int = DEFAULT_TIMEOUT_S,
) -> RunResult:
    import time

    t0 = time.time()
    p = subprocess.run(
        list(cmd),
        cwd=str(cwd) if cwd else None,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        timeout=timeout_s,
    )
    elapsed_ms = int((time.time() - t0) * 1000)
    return RunResult(cmd=list(cmd), exit_code=p.returncode, stdout=p.stdout, stderr=p.stderr, elapsed_ms=elapsed_ms)


def _help_text(nova: Path) -> str:
    try:
        r = run_subprocess([str(nova), "--help"], timeout_s=8)
        return (r.stdout + "\n" + r.stderr).strip()
    except Exception:
        return ""


@dataclasses.dataclass
class Templates:
    run: str
    jit: str
    check: str
    parse: str
    emit_ir: str
    emit_ast: str
    build: str


def detect_templates(nova: Path, overrides: Dict[str, Optional[str]]) -> Templates:
    def ov(key: str, default: str) -> str:
        v = overrides.get(key)
        return v if v else default

    help_text = _help_text(nova).lower()

    run = f"{nova} run {{file}}" if " run " in help_text else f"{nova} {{file}}"
    jit = f"{nova} run {{file}} --jit" if "--jit" in help_text else run
    check = f"{nova} check {{file}}" if "check" in help_text else f"{nova} {{file}}"
    parse = f"{nova} {{file}}"
    emit_ir = f"{nova} {{file}} --emit-ir" if "--emit-ir" in help_text else f"{nova} {{file}}"
    emit_ast = f"{nova} {{file}} --emit-ast" if "--emit-ast" in help_text else f"{nova} {{file}}"
    build = f"{nova} {{file}} -o {{out}}" if " -o " in help_text else f"{nova} build {{file}}"

    return Templates(
        run=ov("run", run),
        jit=ov("jit", jit),
        check=ov("check", check),
        parse=ov("parse", parse),
        emit_ir=ov("emit_ir", emit_ir),
        emit_ast=ov("emit_ast", emit_ast),
        build=ov("build", build),
    )


def _format_cmd(template: str, *, file: Path, out: Optional[Path] = None) -> List[str]:
    s = template.format(file=str(file), out=str(out) if out else "")
    return shlex.split(s)


def _base_env(nova: Path, extra_env: Dict[str, str]) -> Dict[str, str]:
    env = dict(os.environ)
    env.setdefault("ASAN_OPTIONS", "detect_leaks=0:abort_on_error=1")
    env.setdefault("UBSAN_OPTIONS", "halt_on_error=1")
    env["PATH"] = f"{nova.parent}{os.pathsep}{env.get('PATH','')}"
    env.update(extra_env)
    return env


def _contains(hay: str, needle: str, *, case_sensitive: bool) -> bool:
    if case_sensitive:
        return needle in hay
    return needle.casefold() in hay.casefold()


def assert_expectations(tc: TestCase, res: RunResult, *, case_sensitive: bool) -> Tuple[bool, str]:
    d = tc.directives

    if d.expect_exit is not None and res.exit_code != d.expect_exit:
        return False, f"exit code mismatch: expected {d.expect_exit}, got {res.exit_code}"

    if d.expect_exit_nonzero and res.exit_code == 0:
        return False, "expected non-zero exit code"

    if d.expect_stdout is not None and res.stdout != d.expect_stdout:
        return False, "stdout mismatch (exact)"

    if d.expect_stderr is not None and res.stderr != d.expect_stderr:
        return False, "stderr mismatch (exact)"

    for s in d.expect_stdout_contains:
        if not _contains(res.stdout, s, case_sensitive=case_sensitive):
            return False, f"stdout missing substring: {s!r}"

    for s in d.expect_stderr_contains:
        if not _contains(res.stderr, s, case_sensitive=case_sensitive):
            return False, f"stderr missing substring: {s!r}"

    if d.expect_error_contains:
        hay = (res.stderr + "\n" + res.stdout)
        for s in d.expect_error_contains:
            if not _contains(hay, s, case_sensitive=case_sensitive):
                return False, f"error substring not found: {s}"

    has_any_expect = any(
        [
            d.expect_exit is not None,
            d.expect_exit_nonzero,
            d.expect_stdout is not None,
            d.expect_stderr is not None,
            d.expect_stdout_contains,
            d.expect_stderr_contains,
            d.expect_error_contains,
        ]
    )
    if not has_any_expect and res.exit_code != 0:
        return False, f"non-zero exit code: {res.exit_code}"

    return True, "ok"


def _format_output_tail(res: RunResult, max_chars: int = 4000) -> str:
    out = (res.stdout or "").strip()
    err = (res.stderr or "").strip()
    blob = f"\n--- cmd ---\n{' '.join(shlex.quote(x) for x in res.cmd)}"
    if out:
        blob += "\n--- stdout ---\n" + out
    if err:
        blob += "\n--- stderr ---\n" + err
    if len(blob) > max_chars:
        blob = blob[: max_chars - 20] + "\n...(truncated)..."
    return blob


def _write_capture(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8", errors="replace")


def run_one(
    tc: TestCase,
    *,
    nova: Path,
    templates: Templates,
    workdir: Path,
    extra_env: Dict[str, str],
    case_sensitive: bool,
    verbose: bool = False,
) -> Tuple[str, bool, str]:
    d = tc.directives
    if d.skip:
        return "SKIP", True, d.skip

    mode = (d.run or "run").strip().lower()
    if mode == "aot":
        mode = "run"
    if mode == "ir":
        mode = "emit-ir"
    if mode == "ast":
        mode = "emit-ast"
    if mode == "exe":
        mode = "build"

    env = _base_env(nova, {**extra_env, **d.env})

    out_art: Optional[Path] = None
    cmd_template: str
    capture_to_artifact = False

    if mode == "run":
        cmd_template = templates.run
    elif mode == "jit":
        cmd_template = templates.jit
    elif mode == "check":
        cmd_template = templates.check
    elif mode == "parse":
        cmd_template = templates.parse
    elif mode == "emit-ir":
        cmd_template = templates.emit_ir
        out_art = workdir / (tc.path.stem + ".ll")
        capture_to_artifact = "{out}" not in cmd_template
    elif mode == "emit-ast":
        cmd_template = templates.emit_ast
        out_art = workdir / (tc.path.stem + ".ast")
        capture_to_artifact = "{out}" not in cmd_template
    elif mode == "build":
        cmd_template = templates.build
        out_art = workdir / (tc.path.stem + ".exe")
    else:
        return "ERROR", False, f"unknown RUN mode: {mode}"

    cmd = _format_cmd(cmd_template, file=tc.path, out=out_art)
    cmd.extend(d.args)

    if verbose:
        print(f"[cmd] {tc.rel}: {' '.join(shlex.quote(x) for x in cmd)}")

    def run_once(cmdx: List[str], out_capture: Optional[Path]) -> Tuple[RunResult, Optional[Path]]:
        resx = run_subprocess(cmdx, cwd=workdir, env=env, timeout_s=d.timeout_s)
        if out_capture is not None and capture_to_artifact:
            _write_capture(out_capture, resx.stdout)
        return resx, out_capture

    if d.determinism:
        det = d.determinism.lower()

        r1, _ = run_once(cmd, out_art if det in {"ir", "ast"} else None)
        ok1, msg1 = assert_expectations(tc, r1, case_sensitive=case_sensitive)

        if det == "stdout":
            r2, _ = run_once(cmd, None)
            ok2, msg2 = assert_expectations(tc, r2, case_sensitive=case_sensitive)
            if not ok1:
                return ("XFAIL" if d.xfail else "FAIL"), bool(d.xfail), msg1 + _format_output_tail(r1)
            if not ok2:
                return ("XFAIL" if d.xfail else "FAIL"), bool(d.xfail), msg2 + _format_output_tail(r2)
            if sha256_text(r1.stdout) != sha256_text(r2.stdout):
                return ("XFAIL" if d.xfail else "FAIL"), bool(d.xfail), "stdout determinism hash mismatch"
            return ("XPASS" if d.xfail else "PASS"), True, "deterministic (stdout)"

        if det not in {"ir", "ast", "exe"}:
            return "ERROR", False, f"unknown DETERMINISM mode: {det}"

        if out_art is None:
            return "FAIL", False, "DETERMINISM requires an artifact-producing RUN (emit-ir/emit-ast/build)"

        art2 = workdir / (out_art.stem + ".det2" + out_art.suffix)
        cmd2 = _format_cmd(cmd_template, file=tc.path, out=art2)
        cmd2.extend(d.args)

        r2, _ = run_once(cmd2, art2 if det in {"ir", "ast"} else None)
        ok2, msg2 = assert_expectations(tc, r2, case_sensitive=case_sensitive)

        if not ok1:
            return ("XFAIL" if d.xfail else "FAIL"), bool(d.xfail), msg1 + _format_output_tail(r1)
        if not ok2:
            return ("XFAIL" if d.xfail else "FAIL"), bool(d.xfail), msg2 + _format_output_tail(r2)

        if not out_art.exists() or not art2.exists():
            return ("XFAIL" if d.xfail else "FAIL"), bool(d.xfail), "determinism artifact missing"

        if sha256_file(out_art) != sha256_file(art2):
            return ("XFAIL" if d.xfail else "FAIL"), bool(d.xfail), f"determinism hash mismatch ({det})"

        return ("XPASS" if d.xfail else "PASS"), True, f"deterministic ({det})"

    try:
        res = run_subprocess(cmd, cwd=workdir, env=env, timeout_s=d.timeout_s)
    except subprocess.TimeoutExpired:
        return ("XFAIL" if d.xfail else "FAIL"), bool(d.xfail), f"timeout after {d.timeout_s}s"
    except FileNotFoundError:
        return "ERROR", False, f"command not found: {cmd[0]}"
    except Exception as e:
        return "ERROR", False, f"runner error: {e}"

    if capture_to_artifact and out_art is not None:
        _write_capture(out_art, res.stdout)

    ok, msg = assert_expectations(tc, res, case_sensitive=case_sensitive)
    if ok:
        return ("XPASS" if d.xfail else "PASS"), True, msg
    if d.xfail:
        return "XFAIL", True, msg + _format_output_tail(res)
    return "FAIL", False, msg + _format_output_tail(res)


def main(argv: Optional[Sequence[str]] = None) -> int:
    ap = argparse.ArgumentParser(prog="nova_test.py")
    ap.add_argument("--nova", type=Path, default=None, help="Path to Nova compiler executable")
    ap.add_argument("--tests", type=Path, default=Path("tests"), help="Tests root directory")
    ap.add_argument("-k", "--filter", default=None, help="Substring filter on test path")
    ap.add_argument("--list", action="store_true", help="List tests and exit")
    ap.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    ap.add_argument("--workdir", type=Path, default=None, help="Work directory (default: temp dir)")
    ap.add_argument("--env", action="append", default=[], help="Global env KEY=VALUE (repeatable)")
    ap.add_argument("--case-sensitive", action="store_true", help="Case-sensitive substring expectations")

    ap.add_argument("--template-run", default=None)
    ap.add_argument("--template-jit", default=None)
    ap.add_argument("--template-check", default=None)
    ap.add_argument("--template-parse", default=None)
    ap.add_argument("--template-emit-ir", default=None)
    ap.add_argument("--template-emit-ast", default=None)
    ap.add_argument("--template-build", default=None)

    args = ap.parse_args(list(argv) if argv is not None else None)

    nova = args.nova
    if nova is None:
        for cand in [Path("./nova"), Path("/mnt/data/nova")]:
            if cand.exists():
                nova = cand
                break
    if nova is None or not nova.exists():
        print("error: --nova not provided and no ./nova or /mnt/data/nova found", file=sys.stderr)
        return 2
    nova = nova.resolve()

    tests_root = args.tests.resolve()
    if not tests_root.exists():
        print(f"error: tests dir not found: {tests_root}", file=sys.stderr)
        return 2

    extra_env: Dict[str, str] = {}
    for kv in args.env:
        if "=" in kv:
            k, v = kv.split("=", 1)
            extra_env[k.strip()] = v.strip()

    overrides = {
        "run": args.template_run,
        "jit": args.template_jit,
        "check": args.template_check,
        "parse": args.template_parse,
        "emit_ir": args.template_emit_ir,
        "emit_ast": args.template_emit_ast,
        "build": args.template_build,
    }
    templates = detect_templates(nova, overrides)

    tests = discover_tests(tests_root)
    if args.filter:
        tests = [t for t in tests if args.filter in t.rel]

    if args.list:
        for t in tests:
            d = t.directives
            meta = f"RUN={d.run}"
            if d.skip:
                meta += f" SKIP={d.skip}"
            if d.xfail:
                meta += f" XFAIL={d.xfail}"
            if d.determinism:
                meta += f" DETERMINISM={d.determinism}"
            print(f"{t.rel}  ({meta})")
        return 0

    if not tests:
        print("no tests found", file=sys.stderr)
        return 2

    if args.workdir:
        workdir = args.workdir.resolve()
        workdir.mkdir(parents=True, exist_ok=True)
        temp_ctx = None
    else:
        temp_ctx = tempfile.TemporaryDirectory(prefix="nova-tests-")
        workdir = Path(temp_ctx.name)

    total = passed = failed = xfailed = skipped = xpassed = errored = 0

    try:
        for tc in tests:
            total += 1
            status, ok, msg = run_one(
                tc,
                nova=nova,
                templates=templates,
                workdir=workdir,
                extra_env=extra_env,
                case_sensitive=args.case_sensitive,
                verbose=args.verbose,
            )

            if status == "PASS":
                passed += 1
                print(f"✅ {tc.rel}")
            elif status == "FAIL":
                failed += 1
                print(f"❌ {tc.rel}: {msg}")
            elif status == "XFAIL":
                xfailed += 1
                print(f"🟡 {tc.rel} (xfail): {msg}")
            elif status == "XPASS":
                xpassed += 1
                print(f"🟠 {tc.rel} (xpass): {msg}")
            elif status == "SKIP":
                skipped += 1
                print(f"⏭️  {tc.rel} (skip): {msg}")
            else:
                errored += 1
                print(f"💥 {tc.rel}: {msg}")

        print()
        print(
            f"Summary: total={total} pass={passed} fail={failed} "
            f"xfail={xfailed} xpass={xpassed} skip={skipped} error={errored}"
        )

        if failed > 0 or errored > 0 or xpassed > 0:
            return 1
        return 0
    finally:
        if temp_ctx is not None:
            temp_ctx.cleanup()


if __name__ == "__main__":
    raise SystemExit(main())
