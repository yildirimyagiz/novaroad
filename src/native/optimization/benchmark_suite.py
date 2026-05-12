#!/usr/bin/env python3
"""
Nova Performance Benchmark Suite
Measures current performance and tracks improvements
"""

import time
import statistics
import json
from pathlib import Path
from typing import Dict, List, Callable
import sys

# Add Nova to path
NOVA_PATH = Path("/Users/yldyagz/nova")
sys.path.insert(0, str(NOVA_PATH))

try:
    from src.lexer import Lexer
    from src.parser import Parser
    from src.bytecode import BytecodeGenerator
    from src.vm import VirtualMachine

    NOVA_AVAILABLE = True
except ImportError:
    NOVA_AVAILABLE = False
    print("⚠️  Nova not available at expected path")


class BenchmarkResult:
    """Stores benchmark results"""

    def __init__(self, name: str, iterations: int = 10):
        self.name = name
        self.iterations = iterations
        self.times: List[float] = []

    def add_time(self, time_ms: float):
        self.times.append(time_ms)

    @property
    def avg_time(self) -> float:
        return statistics.mean(self.times) if self.times else 0.0

    @property
    def min_time(self) -> float:
        return min(self.times) if self.times else 0.0

    @property
    def max_time(self) -> float:
        return max(self.times) if self.times else 0.0

    @property
    def std_dev(self) -> float:
        return statistics.stdev(self.times) if len(self.times) > 1 else 0.0

    def to_dict(self) -> dict:
        return {
            "name": self.name,
            "iterations": self.iterations,
            "avg_time_ms": self.avg_time,
            "min_time_ms": self.min_time,
            "max_time_ms": self.max_time,
            "std_dev_ms": self.std_dev,
            "times": self.times,
        }


class NovaBenchmark:
    """Main benchmark suite"""

    def __init__(self):
        self.results: Dict[str, BenchmarkResult] = {}
        self.baseline_results: Dict[str, float] = {}

    def run_nova_code(self, source_code: str) -> float:
        """Run Nova code and return execution time in ms"""
        if not NOVA_AVAILABLE:
            return 0.0

        start = time.perf_counter()

        try:
            # Lexer
            lexer = Lexer(source_code)
            tokens = lexer.tokenize()

            # Parser
            parser = Parser(tokens)
            ast = parser.parse()

            # Bytecode generation
            generator = BytecodeGenerator()
            bytecode = generator.generate(ast)

            # Execution
            vm = VirtualMachine(
                generator.instructions,
                generator.constants,
                generator.functions,
                generator.function_params,
            )
            vm.run()

        except Exception as e:
            print(f"Error running Nova code: {e}")
            return 0.0

        end = time.perf_counter()
        return (end - start) * 1000  # Convert to ms

    def run_python_equivalent(self, func: Callable) -> float:
        """Run Python equivalent and return execution time"""
        start = time.perf_counter()
        func()
        end = time.perf_counter()
        return (end - start) * 1000

    def benchmark_integer_arithmetic(self, iterations: int = 10):
        """Benchmark: Integer arithmetic"""
        print("🔢 Benchmark: Integer Arithmetic")

        nova_code = """
fn main():
    let sum = 0
    for i in 0..1000000:
        sum = sum + i
    return sum
"""

        def python_equiv():
            sum_val = 0
            for i in range(1000000):
                sum_val += i
            return sum_val

        result = BenchmarkResult("integer_arithmetic", iterations)

        for i in range(iterations):
            nova_time = self.run_nova_code(nova_code)
            result.add_time(nova_time)
            print(f"  Iteration {i+1}/{iterations}: {nova_time:.2f}ms")

        # Python baseline
        python_time = self.run_python_equivalent(python_equiv)
        self.baseline_results["integer_arithmetic_python"] = python_time

        self.results["integer_arithmetic"] = result

        print(f"  ✅ Nova avg: {result.avg_time:.2f}ms")
        print(f"  📊 Python: {python_time:.2f}ms")
        print(f"  📈 Slowdown: {result.avg_time/python_time:.2f}x\n")

    def benchmark_function_calls(self, iterations: int = 10):
        """Benchmark: Function call overhead"""
        print("📞 Benchmark: Function Calls")

        nova_code = """
fn add_one(x: int) -> int:
    return x + 1

fn main():
    let result = 0
    for i in 0..100000:
        result = add_one(result)
    return result
"""

        def python_equiv():
            def add_one(x):
                return x + 1

            result = 0
            for i in range(100000):
                result = add_one(result)
            return result

        result = BenchmarkResult("function_calls", iterations)

        for i in range(iterations):
            nova_time = self.run_nova_code(nova_code)
            result.add_time(nova_time)
            print(f"  Iteration {i+1}/{iterations}: {nova_time:.2f}ms")

        python_time = self.run_python_equivalent(python_equiv)
        self.baseline_results["function_calls_python"] = python_time

        self.results["function_calls"] = result

        print(f"  ✅ Nova avg: {result.avg_time:.2f}ms")
        print(f"  📊 Python: {python_time:.2f}ms")
        print(f"  📈 Slowdown: {result.avg_time/python_time:.2f}x\n")

    def benchmark_list_operations(self, iterations: int = 10):
        """Benchmark: List/array operations"""
        print("📋 Benchmark: List Operations")

        nova_code = """
fn main():
    let sum = 0
    for i in 0..10000:
        let arr = [1, 2, 3, 4, 5]
        for val in arr:
            sum = sum + val
    return sum
"""

        def python_equiv():
            sum_val = 0
            for i in range(10000):
                arr = [1, 2, 3, 4, 5]
                for val in arr:
                    sum_val += val
            return sum_val

        result = BenchmarkResult("list_operations", iterations)

        for i in range(iterations):
            nova_time = self.run_nova_code(nova_code)
            result.add_time(nova_time)
            print(f"  Iteration {i+1}/{iterations}: {nova_time:.2f}ms")

        python_time = self.run_python_equivalent(python_equiv)
        self.baseline_results["list_operations_python"] = python_time

        self.results["list_operations"] = result

        print(f"  ✅ Nova avg: {result.avg_time:.2f}ms")
        print(f"  📊 Python: {python_time:.2f}ms")
        print(f"  📈 Slowdown: {result.avg_time/python_time:.2f}x\n")

    def benchmark_recursive_fibonacci(self, iterations: int = 5):
        """Benchmark: Recursive function calls"""
        print("🔄 Benchmark: Recursive Fibonacci")

        nova_code = """
fn fib(n: int) -> int:
    if n <= 1:
        return n
    return fib(n - 1) + fib(n - 2)

fn main():
    return fib(20)
"""

        def python_equiv():
            def fib(n):
                if n <= 1:
                    return n
                return fib(n - 1) + fib(n - 2)

            return fib(20)

        result = BenchmarkResult("recursive_fib", iterations)

        for i in range(iterations):
            nova_time = self.run_nova_code(nova_code)
            result.add_time(nova_time)
            print(f"  Iteration {i+1}/{iterations}: {nova_time:.2f}ms")

        python_time = self.run_python_equivalent(python_equiv)
        self.baseline_results["recursive_fib_python"] = python_time

        self.results["recursive_fib"] = result

        print(f"  ✅ Nova avg: {result.avg_time:.2f}ms")
        print(f"  📊 Python: {python_time:.2f}ms")
        print(f"  📈 Slowdown: {result.avg_time/python_time:.2f}x\n")

    def run_all_benchmarks(self):
        """Run all benchmarks"""
        print("╔" + "=" * 78 + "╗")
        print("║" + " " * 22 + "NOVA BENCHMARK SUITE" + " " * 33 + "║")
        print("╚" + "=" * 78 + "╝\n")

        if not NOVA_AVAILABLE:
            print("❌ Nova not available. Cannot run benchmarks.")
            return

        self.benchmark_integer_arithmetic(iterations=5)
        self.benchmark_function_calls(iterations=5)
        self.benchmark_list_operations(iterations=5)
        self.benchmark_recursive_fibonacci(iterations=3)

        self.print_summary()
        self.save_results()

    def print_summary(self):
        """Print benchmark summary"""
        print("\n" + "=" * 80)
        print("📊 BENCHMARK SUMMARY")
        print("=" * 80)

        print(
            f"\n{'Benchmark':<30} {'Nova (ms)':<15} {'Python (ms)':<15} {'Slowdown':<10}"
        )
        print("-" * 80)

        for name, result in self.results.items():
            python_baseline = self.baseline_results.get(f"{name}_python", 0)
            slowdown = result.avg_time / python_baseline if python_baseline > 0 else 0

            print(
                f"{name:<30} {result.avg_time:>10.2f}     "
                f"{python_baseline:>10.2f}     {slowdown:>8.2f}x"
            )

        print("=" * 80)

        # Overall statistics
        if self.results:
            avg_slowdown = statistics.mean(
                [
                    self.results[name].avg_time
                    / self.baseline_results.get(f"{name}_python", 1)
                    for name in self.results.keys()
                    if self.baseline_results.get(f"{name}_python", 0) > 0
                ]
            )

            print(f"\n📈 Average slowdown vs Python: {avg_slowdown:.2f}x")
            print(f"🎯 Target after optimization: 0.5x (2x faster than Python)\n")

    def save_results(self):
        """Save results to JSON"""
        results_data = {
            "timestamp": time.time(),
            "benchmarks": {
                name: result.to_dict() for name, result in self.results.items()
            },
            "baselines": self.baseline_results,
        }

        output_file = Path("benchmark_results.json")
        with open(output_file, "w") as f:
            json.dump(results_data, f, indent=2)

        print(f"💾 Results saved to: {output_file}")


def main():
    """Main entry point"""
    benchmark = NovaBenchmark()
    benchmark.run_all_benchmarks()


if __name__ == "__main__":
    main()
