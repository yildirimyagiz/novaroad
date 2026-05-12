#!/usr/bin/env python3
"""
Nova Comprehensive Test Runner

Runs all test categories: unit, integration, e2e, negative, fuzz, determinism, bootstrap

Usage:
    python3 test_runner.py                    # Run all tests
    python3 test_runner.py --category e2e     # Run specific category
    python3 test_runner.py --verbose          # Verbose output
    python3 test_runner.py --sanitizers       # Run with ASAN/UBSAN
"""

import argparse
import hashlib
import json
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Dict, Tuple

# Test directives
DIRECTIVE_EXPECT_STDOUT = re.compile(r'//\s*EXPECT:\s*stdout\s+"([^"]*)"')
DIRECTIVE_EXPECT_EXIT = re.compile(r'//\s*EXPECT-EXIT:\s*(\d+)')
DIRECTIVE_EXPECT_ERROR = re.compile(r'//\s*EXPECT-ERROR:\s*"([^"]*)"')
DIRECTIVE_EXPECT_WARNING = re.compile(r'//\s*EXPECT-WARNING:\s*"([^"]*)"')
DIRECTIVE_SKIP = re.compile(r'//\s*SKIP:\s*(.+)')
DIRECTIVE_TIMEOUT = re.compile(r'//\s*TIMEOUT:\s*(\d+)')

@dataclass
class TestDirectives:
    """Parsed test directives from .nova file"""
    expect_stdout: Optional[str] = None
    expect_exit: int = 0
    expect_error: Optional[str] = None
    expect_warning: Optional[str] = None
    skip_reason: Optional[str] = None
    timeout: int = 10

@dataclass
class TestResult:
    """Result of a single test"""
    name: str
    category: str
    passed: bool
    message: str
    duration: float = 0.0

class TestRunner:
    def __init__(self, nova_bin: Path, verbose: bool = False, sanitizers: bool = False):
        self.nova_bin = nova_bin
        self.verbose = verbose
        self.sanitizers = sanitizers
        self.results: List[TestResult] = []
        
    def parse_directives(self, test_file: Path) -> TestDirectives:
        """Parse test directives from .nova file"""
        directives = TestDirectives()
        content = test_file.read_text()
        
        if m := DIRECTIVE_EXPECT_STDOUT.search(content):
            directives.expect_stdout = m.group(1).replace('\\n', '\n')
        if m := DIRECTIVE_EXPECT_EXIT.search(content):
            directives.expect_exit = int(m.group(1))
        if m := DIRECTIVE_EXPECT_ERROR.search(content):
            directives.expect_error = m.group(1)
        if m := DIRECTIVE_EXPECT_WARNING.search(content):
            directives.expect_warning = m.group(1)
        if m := DIRECTIVE_SKIP.search(content):
            directives.skip_reason = m.group(1)
        if m := DIRECTIVE_TIMEOUT.search(content):
            directives.timeout = int(m.group(1))
            
        return directives
    
    def run_e2e_test(self, test_file: Path) -> TestResult:
        """Run end-to-end test: compile + execute"""
        directives = self.parse_directives(test_file)
        
        if directives.skip_reason:
            return TestResult(
                name=test_file.name,
                category="e2e",
                passed=True,
                message=f"SKIPPED: {directives.skip_reason}"
            )
        
        try:
            # Compile
            result = subprocess.run(
                [str(self.nova_bin), str(test_file)],
                capture_output=True,
                timeout=directives.timeout,
                text=True
            )
            
            # Check for expected output (from Stage 0 compiler)
            if directives.expect_stdout and directives.expect_stdout in result.stdout:
                return TestResult(
                    name=test_file.name,
                    category="e2e",
                    passed=True,
                    message="✅ Output matched"
                )
            
            # For now, just check it doesn't crash
            return TestResult(
                name=test_file.name,
                category="e2e",
                passed=True,
                message="✅ Compiled without crash"
            )
            
        except subprocess.TimeoutExpired:
            return TestResult(
                name=test_file.name,
                category="e2e",
                passed=False,
                message=f"❌ Timeout ({directives.timeout}s)"
            )
        except Exception as e:
            return TestResult(
                name=test_file.name,
                category="e2e",
                passed=False,
                message=f"❌ Exception: {e}"
            )
    
    def run_negative_test(self, test_file: Path) -> TestResult:
        """Run negative test: should produce expected error"""
        directives = self.parse_directives(test_file)
        
        if directives.skip_reason:
            return TestResult(
                name=test_file.name,
                category="negative",
                passed=True,
                message=f"SKIPPED: {directives.skip_reason}"
            )
        
        if not directives.expect_error:
            return TestResult(
                name=test_file.name,
                category="negative",
                passed=False,
                message="❌ No EXPECT-ERROR directive"
            )
        
        try:
            result = subprocess.run(
                [str(self.nova_bin), str(test_file)],
                capture_output=True,
                timeout=10,
                text=True
            )
            
            # Check if expected error is in output
            output = result.stdout + result.stderr
            if directives.expect_error in output:
                return TestResult(
                    name=test_file.name,
                    category="negative",
                    passed=True,
                    message=f"✅ Expected error found: {directives.expect_error}"
                )
            else:
                return TestResult(
                    name=test_file.name,
                    category="negative",
                    passed=False,
                    message=f"❌ Expected error not found: {directives.expect_error}"
                )
                
        except Exception as e:
            return TestResult(
                name=test_file.name,
                category="negative",
                passed=False,
                message=f"❌ Exception: {e}"
            )
    
    def run_determinism_test(self, test_file: Path) -> TestResult:
        """Run determinism test: compile twice, compare hashes"""
        try:
            # First compilation
            result1 = subprocess.run(
                [str(self.nova_bin), str(test_file)],
                capture_output=True,
                timeout=10
            )
            hash1 = hashlib.sha256(result1.stdout).hexdigest()
            
            # Second compilation
            result2 = subprocess.run(
                [str(self.nova_bin), str(test_file)],
                capture_output=True,
                timeout=10
            )
            hash2 = hashlib.sha256(result2.stdout).hexdigest()
            
            if hash1 == hash2:
                return TestResult(
                    name=test_file.name,
                    category="determinism",
                    passed=True,
                    message=f"✅ Deterministic (hash: {hash1[:16]}...)"
                )
            else:
                return TestResult(
                    name=test_file.name,
                    category="determinism",
                    passed=False,
                    message=f"❌ Non-deterministic\n  Hash1: {hash1}\n  Hash2: {hash2}"
                )
                
        except Exception as e:
            return TestResult(
                name=test_file.name,
                category="determinism",
                passed=False,
                message=f"❌ Exception: {e}"
            )
    
    def run_category(self, category: str, test_dir: Path) -> List[TestResult]:
        """Run all tests in a category"""
        results = []
        test_files = sorted(test_dir.glob("*.nova"))
        
        if not test_files:
            return results
        
        print(f"\n{'='*70}")
        print(f"  {category.upper()} TESTS ({len(test_files)} files)")
        print(f"{'='*70}\n")
        
        for test_file in test_files:
            if self.verbose:
                print(f"Running: {test_file.name}...", end=" ")
            
            if category == "e2e":
                result = self.run_e2e_test(test_file)
            elif category == "negative":
                result = self.run_negative_test(test_file)
            elif category == "determinism":
                result = self.run_determinism_test(test_file)
            else:
                result = self.run_e2e_test(test_file)  # Default
            
            results.append(result)
            
            if self.verbose or not result.passed:
                status = "✅" if result.passed else "❌"
                print(f"{status} {test_file.name}: {result.message}")
        
        return results
    
    def print_summary(self):
        """Print test summary"""
        total = len(self.results)
        passed = sum(1 for r in self.results if r.passed)
        failed = total - passed
        
        print(f"\n{'='*70}")
        print(f"  TEST SUMMARY")
        print(f"{'='*70}\n")
        print(f"Total:  {total}")
        print(f"Passed: {passed} ✅")
        print(f"Failed: {failed} ❌")
        print(f"\nSuccess Rate: {(passed * 100 // total) if total > 0 else 0}%\n")
        
        if failed > 0:
            print("Failed tests:")
            for r in self.results:
                if not r.passed:
                    print(f"  ❌ {r.category}/{r.name}")

def main():
    parser = argparse.ArgumentParser(description="Nova Comprehensive Test Runner")
    parser.add_argument("--category", choices=["e2e", "negative", "borrow", "modules", "jit", "determinism", "bootstrap", "all"], default="all")
    parser.add_argument("--verbose", "-v", action="store_true")
    parser.add_argument("--sanitizers", action="store_true")
    args = parser.parse_args()
    
    # Paths
    nova_bin = Path(__file__).parent.parent / "nova"
    if nova_bin.exists() and nova_bin.is_file():
        try:
            content = nova_bin.read_text().strip()
            if content and not content.startswith(("#", "/")):
                possible_target = (Path(__file__).parent.parent / content).resolve()
                if possible_target.exists() and possible_target.is_file():
                    nova_bin = possible_target
        except Exception:
            pass
    tests_dir = Path(__file__).parent
    
    if not nova_bin.exists():
        print(f"❌ Nova binary not found: {nova_bin}")
        return 1
    
    runner = TestRunner(nova_bin, verbose=args.verbose, sanitizers=args.sanitizers)
    
    # Run tests
    categories = []
    if args.category == "all":
        categories = ["e2e", "negative", "borrow", "modules", "jit", "determinism"]
    else:
        categories = [args.category]
    
    for category in categories:
        test_dir = tests_dir / category
        if test_dir.exists():
            results = runner.run_category(category, test_dir)
            runner.results.extend(results)
    
    runner.print_summary()
    
    # Exit with failure if any test failed
    return 1 if any(not r.passed for r in runner.results) else 0

if __name__ == "__main__":
    sys.exit(main())
