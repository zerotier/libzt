"""
Poetry build.py

See: https://stackoverflow.com/questions/60073711/how-to-build-c-extensions-via-poetry
"""
import os
import shutil
import subprocess
from distutils.command.build_ext import build_ext
from distutils.errors import DistutilsPlatformError, CCompilerError, DistutilsExecError
from distutils.extension import Extension

from glob import glob


DIR = os.path.dirname(os.path.realpath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(DIR, "..", ".."))

INCLUDE_DIRS = [
    os.path.join(ROOT_DIR, "include"),
    os.path.join(ROOT_DIR, "src"),
    os.path.join(ROOT_DIR, "src/bindings/python"),
    os.path.join(ROOT_DIR, "ext/concurrentqueue"),
    os.path.join(ROOT_DIR, "ext/lwip/src/include"),
    os.path.join(ROOT_DIR, "ext/lwip-contrib/ports/unix/port/include"),
    os.path.join(ROOT_DIR, "ext/ZeroTierOne/include"),
    os.path.join(ROOT_DIR, "ext/ZeroTierOne"),
    os.path.join(ROOT_DIR, "ext/ZeroTierOne/node"),
    os.path.join(ROOT_DIR, "ext/ZeroTierOne/service"),
    os.path.join(ROOT_DIR, "ext/ZeroTierOne/osdep"),
    os.path.join(ROOT_DIR, "ext/ZeroTierOne/controller"),
]


class LibztModule(Extension):
    """Libzt extension module"""
    def __init__(self):
        sources = [
            *glob(os.path.join(ROOT_DIR, "src/bindings/python/*.cxx")),
            *glob(os.path.join(ROOT_DIR, "src/bindings/python/zt.i")),
            *glob(os.path.join(ROOT_DIR, "src/*.cpp")),
            *glob(os.path.join(ROOT_DIR, "ext/ZeroTierOne/node/*.cpp")),
            *glob(os.path.join(ROOT_DIR, "ext/ZeroTierOne/osdep/OSUtils.cpp")),
            *glob(os.path.join(ROOT_DIR, "ext/ZeroTierOne/osdep/PortMapper.cpp")),
            *glob(os.path.join(ROOT_DIR, "ext/ZeroTierOne/osdep/ManagedRoute.cpp")),
        ]
        # noinspection PyUnresolvedReferences
        assert len(sources) > 0, "no sources found"

        # noinspection PyTypeChecker
        super().__init__(
            "libzt._libzt",
            sources=sources,
            include_dirs=INCLUDE_DIRS,
            extra_compile_args=[
                "-std=c++11",
                "-DZTS_ENABLE_PYTHON=1",
                "-DZT_SDK",
                "-Wno-parentheses-equality",
                "-Wno-macro-redefined",
                "-Wno-tautological-overlap-compare",
                "-Wno-tautological-constant-out-of-range-compare",
            ],
            swig_opts=[
                "-c++",
                f"-I{os.path.join(ROOT_DIR, 'include')}",
            ],
        )


def cstuff():
    """C library"""
    sources = [
        # libnatpmp
        *glob(os.path.join(ROOT_DIR, "ext/ZeroTierOne/ext/libnatpmp/natpmp.c")),
        *glob(os.path.join(ROOT_DIR, "ext/ZeroTierOne/ext/libnatpmp/wingettimeofday.c")),
        *glob(os.path.join(ROOT_DIR, "ext/ZeroTierOne/ext/libnatpmp/getgateway.c")),
        # miniupnpc
        *glob(os.path.join(ROOT_DIR, "ext/miniupnpc/*.c")),
        # lwip
        *glob(os.path.join(ROOT_DIR, "ext/lwip/src/netif/*.c")),
        *glob(os.path.join(ROOT_DIR, "ext/lwip/src/api/*.c")),
        *glob(os.path.join(ROOT_DIR, "ext/lwip/src/core/*.c")),
        *glob(os.path.join(ROOT_DIR, "ext/lwip/src/core/ipv4/*.c")),
        *glob(os.path.join(ROOT_DIR, "ext/lwip/src/core/ipv6/*.c")),
        *glob(os.path.join(ROOT_DIR, "ext/lwip/src/netif/*.c")),
        *glob(os.path.join(ROOT_DIR, "ext/lwip-contrib/ports/unix/port/sys_arch.c")),
    ]
    # noinspection PyUnresolvedReferences
    assert len(sources) > 0, "no sources"

    return ("cstuff", {
        "sources": sources,
        "include_dirs": INCLUDE_DIRS,
    })


class BuildFailed(Exception):
    pass


class ExtBuilder(build_ext):

    def run(self):
        try:
            build_ext.run(self)
        except (DistutilsPlatformError, FileNotFoundError):
            raise BuildFailed('File not found. Could not compile C extension.')

    def build_extension(self, ext):
        try:
            build_ext.build_extension(self, ext)
        except (CCompilerError, DistutilsExecError, DistutilsPlatformError, ValueError):
            raise BuildFailed('Could not compile C extension.')


def build(setup_kwargs):
    """Build """

    assert os.getcwd() == DIR, f"must be invoked from {DIR}"

    # Ensure git submodules are loaded
    subprocess.run(["git", "submodule", "update", "--init"])

    # Copy Python files into packaging directory
    src_dir = os.path.join(ROOT_DIR, "src", "bindings", "python")
    dst_dir = os.path.join(DIR, "libzt")
    for filename in os.listdir(src_dir):
        if os.path.isfile(os.path.join(src_dir, filename)) and filename.endswith(".py"):
            shutil.copy(os.path.join(src_dir, filename), os.path.join(dst_dir, filename))

    # LICENSE file
    shutil.copy(os.path.join(ROOT_DIR, "LICENSE.txt"), os.path.join(DIR, "LICENSE"))

    # Pass up extensions information
    setup_kwargs["libraries"] = [cstuff()]
    setup_kwargs["ext_modules"] = [LibztModule()]
    setup_kwargs["py_modules"] = ["libzt"]
    setup_kwargs["cmdclass"] = {"build_ext": ExtBuilder}
