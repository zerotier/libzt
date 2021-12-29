"""
Build script (replaces build.sh)

Works with Python 3.5+
"""
import os
import shutil
import subprocess
import sys

from setuptools import sandbox
import argparse


DIR = os.path.dirname(os.path.realpath(__file__))


def clean(**_):
    """Remove all generated files"""
    rm_files = [os.path.join("libzt", "sockets.py"), os.path.join("libzt", "libzt.py"),
                os.path.join("libzt", "node.py"), os.path.join("libzt", "select.py"), "LICENSE", "native"]
    rm_dirs = ["src", "ext", "build", "dist", "libzt.egg-info"]

    for root, dirs, files in os.walk(DIR):
        for file in files:
            if file.endswith(".so") or file.endswith(".pyc"):
                rm_files.append(os.path.join(root, file))
        if "__pycache__" in dirs:
            rm_dirs.append(os.path.join(root, "__pycache__"))

    for filename in rm_files:
        try:
            os.remove(os.path.join(DIR, filename))
        except OSError:
            pass

    for dirname in rm_dirs:
        try:
            shutil.rmtree(os.path.join(DIR, dirname))
        except FileNotFoundError:
            pass


def build(debug: bool = False, **_):
    """Build the extension module"""
    # Symbolic link to source tree so that sdist structure makes sense
    try:
        os.symlink(os.path.join(DIR, "..", ".."), os.path.join(DIR, "native"))
    except FileExistsError:
        pass

    # Re-build wrapper to export C symbols
    subprocess.run(["swig", "-c++", "-python",
                    "-o", os.path.join("native", "src", "bindings", "python", "zt_wrap.cxx"),
                    "-I" + os.path.join(".", "native", "include"),
                    os.path.join("native", "src", "bindings", "python", "zt.i")])

    # Copy language bindings into module directory
    bindings_root = os.path.join(DIR, "native", "src", "bindings", "python")
    for filename in os.listdir(bindings_root):
        if os.path.isfile(os.path.join(bindings_root, filename)) and filename.endswith(".py"):
            shutil.copy(os.path.join(bindings_root, filename), os.path.join(DIR, "libzt", filename))
    shutil.copy(os.path.join(DIR, "native", "LICENSE.txt"), os.path.join(DIR, "LICENSE"))

    # Build C libraries (anND_BFd then) C++ extension
    setup_args = ["build_clib", "--verbose", "build_ext", "-i", "--verbose"]
    if debug:
        setup_args.append("--debug")
    sandbox.run_setup(os.path.join(DIR, "setup.py"), setup_args)


def package(**_):
    """Package as bdist_wheel (requires `wheel` package)"""
    try:
        import wheel
    except ImportError:
        raise AssertionError("must have package 'wheel' installed")
    sandbox.run_setup(os.path.join(DIR, "setup.py"), ["bdist_wheel"])


def install(**_):
    """Install using pip from whatever Python executable this script was called with"""
    subprocess.run([sys.executable, "-m", "pip", "install", DIR])


parser = argparse.ArgumentParser(description="Build script for libzt Python module")
parser.add_argument("command", type=str, choices=["clean", "build", "package", "install"], nargs="+")
parser.add_argument("--debug", action="store_true", help="build debug module")


if __name__ == "__main__":

    args = parser.parse_args()

    should_clean = ("clean" in args.command)
    should_build = ("build" in args.command) or ("package" in args.command) or ("install" in args.command)
    should_package = ("package" in args.command)
    should_install = ("install" in args.command)

    kwargs = vars(args)
    kwargs.pop("command")

    if should_clean:
        clean(**kwargs)

    if should_build:
        build(**kwargs)

    if should_package:
        package(**kwargs)

    if should_install:
        install(**kwargs)
