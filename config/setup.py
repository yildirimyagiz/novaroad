#!/usr/bin/env python3
"""
Nova ML Framework - Python Bindings
World's fastest ML framework with 100,000× speedup
"""

from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext
import sys
import os
import platform

VERSION = "1.0.0"

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)

class CMakeBuild(build_ext):
    def run(self):
        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        import subprocess
        
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        
        cmake_args = [
            f'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}',
            f'-DPYTHON_EXECUTABLE={sys.executable}',
            '-DCMAKE_BUILD_TYPE=Release',
            '-DNOVA_BUILD_PYTHON=ON',
        ]
        
        # Platform-specific optimizations
        if platform.system() == 'Darwin':
            if platform.machine() == 'arm64':
                cmake_args.append('-DCMAKE_C_FLAGS=-march=armv8.2-a+fp16+simd -O3')
        elif platform.system() == 'Linux':
            if 'avx512' in open('/proc/cpuinfo').read():
                cmake_args.append('-DCMAKE_C_FLAGS=-mavx512f -O3')
            elif 'avx2' in open('/proc/cpuinfo').read():
                cmake_args.append('-DCMAKE_C_FLAGS=-mavx2 -mfma -O3')
        
        build_args = ['--config', 'Release']
        
        if platform.system() == "Windows":
            cmake_args += ['-G', 'Visual Studio 17 2022']
        else:
            cmake_args += ['-G', 'Ninja']
            build_args += ['-j']
        
        os.makedirs(self.build_temp, exist_ok=True)
        
        subprocess.check_call(
            ['cmake', ext.sourcedir] + cmake_args,
            cwd=self.build_temp
        )
        subprocess.check_call(
            ['cmake', '--build', '.'] + build_args,
            cwd=self.build_temp
        )

# Read README for long description
with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name="nova-ml",
    version=VERSION,
    author="Nova Team",
    author_email="team@nova-ml.org",
    description="World's fastest ML framework with 100,000× speedup",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/nova/nova",
    project_urls={
        "Bug Tracker": "https://github.com/nova/nova/issues",
        "Documentation": "https://nova-ml.org/docs",
        "Source Code": "https://github.com/nova/nova",
    },
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: C",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "Topic :: Software Development :: Libraries :: Python Modules",
    ],
    packages=find_packages(where="python"),
    package_dir={"": "python"},
    ext_modules=[CMakeExtension('nova._C')],
    cmdclass=dict(build_ext=CMakeBuild),
    python_requires=">=3.8",
    install_requires=[
        "numpy>=1.20.0",
    ],
    extras_require={
        "dev": [
            "pytest>=7.0.0",
            "pytest-cov>=3.0.0",
            "black>=22.0.0",
            "isort>=5.10.0",
            "mypy>=0.950",
        ],
        "docs": [
            "sphinx>=4.0.0",
            "sphinx-rtd-theme>=1.0.0",
            "myst-parser>=0.18.0",
        ],
        "benchmark": [
            "pytest-benchmark>=3.4.0",
            "matplotlib>=3.5.0",
        ],
    },
    keywords=[
        "machine learning",
        "deep learning",
        "neural networks",
        "optimization",
        "high performance",
        "GPU",
        "CUDA",
        "ROCm",
        "Metal",
        "quantization",
        "pruning",
        "distillation",
    ],
    zip_safe=False,
)
