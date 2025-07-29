#!/usr/bin/env python3

import os
import sys
import shutil
from glob import glob
import subprocess as sp
from pathlib import Path

def run(cmd):
    p = sp.Popen(cmd, stdin=sp.DEVNULL, stdout=sp.PIPE, stderr=sp.PIPE, text=True)
    o, e = p.communicate()
    if p.returncode != 0:
        raise Exception(f"command {cmd} failed with non-zero return code {p.returncode}. stderr:\n", e)

    return o.strip()

def pkg_config_cflags(pkg: str):
    return run(["pkg-config", "--cflags", pkg]).split()

def pkg_config_libs(pkg: str):
    return run(["pkg-config", "--libs", pkg]).split()

src_dir = Path(__file__).parent
build_dir = src_dir / "build"

if "--clean" in sys.argv or "-c" in sys.argv:
    shutil.rmtree(build_dir)

if not build_dir.exists():
    build_dir.mkdir()

ninja = open(build_dir / "build.ninja", "w")

deps = ["glfw3", "vulkan"]
cxxflags = ["-std=c++17", "-Wall"]
ldflags = []
for dep in deps:
    cxxflags += pkg_config_cflags(dep)
    ldflags += pkg_config_libs(dep)

ninja.write(f"srcdir = {src_dir.absolute()}\n")
ninja.write(f"cxxflags = {' '.join(cxxflags)}\n")
ninja.write(f"ldflags = {' '.join(ldflags)}\n")
ninja.write("rule cxx\n")
ninja.write("    command = clang++ $cxxflags -c $in -o $out\n")
ninja.write("rule link\n")
ninja.write("    command = clang++ $in -o $out $ldflags\n")
+ninja.write("rule glsl\n")
+ninja.write("    command = glslc $in -o $out\n")

srcs = glob("src/*.cc")
objs = []
for src in srcs:
    src = Path(src)
    obj = src.with_suffix('.o')
    ninja.write(f"build {obj}: cxx $srcdir/{src}\n")
    objs.append(str(obj))

shaders = glob("shaders/*.*")
spvs = []
for sh in shaders:
    sh = sh
    spv = sh + ".spv"
    ninja.write(f"build {spv}: glsl $srcdir/{sh}\n")
    spvs.append(spv)

executable = "learn-vulkan"

ninja.write(f"build {executable}: link {' '.join(objs)} | {' '.join(spvs)}\n")
ninja.write(f"default {executable}\n")
ninja.close()

os.chdir(build_dir)
os.execvp("ninja", ["ninja"])
