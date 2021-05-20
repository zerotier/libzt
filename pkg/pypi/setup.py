#!/usr/bin/env python

from setuptools import setup, Extension, Command, Distribution
from distutils.util import convert_path
import glob
import os

main_ns = {}
ver_path = convert_path('libzt/version.py')
with open(ver_path) as ver_file:
    exec(ver_file.read(), main_ns)

from os import path
this_directory = path.abspath(path.dirname(__file__))
with open(path.join(this_directory, 'README.md'), encoding='utf-8') as f:
    long_description = f.read()

class BinaryDistribution(Distribution):
	def is_pure(self):
		return False

# monkey-patch for parallel compilation
# Copied from: https://stackoverflow.com/a/13176803
def parallelCCompile(self, sources, output_dir=None, macros=None, include_dirs=None, debug=0, extra_preargs=None, extra_postargs=None, depends=None):
    # those lines are copied from distutils.ccompiler.CCompiler directly
    macros, objects, extra_postargs, pp_opts, build = self._setup_compile(output_dir, macros, include_dirs, sources, depends, extra_postargs)
    cc_args = self._get_cc_args(pp_opts, debug, extra_preargs)
    # parallel code
    N=16 # number of parallel compilations
    import multiprocessing.pool
    def _single_compile(obj):
        try: src, ext = build[obj]
        except KeyError: return
        self._compile(obj, src, ext, cc_args, extra_postargs, pp_opts)
    # convert to list, imap is evaluated on-demand
    list(multiprocessing.pool.ThreadPool(N).imap(_single_compile,objects))
    return objects
import distutils.ccompiler
distutils.ccompiler.CCompiler.compile=parallelCCompile

# Build

cpp_glob = []
c_glob = []

# Windows
if os.name == 'nt':
	print('TODO')
	#extra_compile_args=['/std:c++14', '-DNOMINMAX=1', '-DZT_SDK', '-DSDK'],
	#extra_link_args=['/LIBPATH:.', 'WS2_32.Lib', 'ShLwApi.Lib', 'iphlpapi.Lib','lwip.lib'],

# Everything else
else:
	cpp_glob.extend(list(glob.glob('native/src/bindings/python/*.cxx')))
	cpp_glob.extend(list(glob.glob('native/src/*.cpp')))
	cpp_glob.extend(list(glob.glob('native/ext/ZeroTierOne/node/*.cpp')))
	cpp_glob.extend(list(glob.glob('native/ext/ZeroTierOne/osdep/OSUtils.cpp')))
	cpp_glob.extend(list(glob.glob('native/ext/ZeroTierOne/osdep/PortMapper.cpp')))
	cpp_glob.extend(list(glob.glob('native/ext/ZeroTierOne/osdep/ManagedRoute.cpp')))

	my_include_dirs=['native/include',
			'native/src',
			'native/src/bindings/python',
			'native/ext/concurrentqueue',
			'native/ext/lwip/src/include',
			'native/ext/lwip-contrib/ports/unix/port/include',
			'native/ext/ZeroTierOne/include',
			'native/ext/ZeroTierOne',
			'native/ext/ZeroTierOne/node',
			'native/ext/ZeroTierOne/service',
			'native/ext/ZeroTierOne/osdep',
			'native/ext/ZeroTierOne/controller']

	libzt_module = Extension('libzt._libzt',
		extra_compile_args=['-std=c++11', '-DZTS_ENABLE_PYTHON=1', '-DZT_SDK', '-Wno-parentheses-equality', '-Wno-macro-redefined', '-Wno-tautological-overlap-compare', '-Wno-tautological-constant-out-of-range-compare'],
		sources=cpp_glob, include_dirs=my_include_dirs)

	# Separate C library, this is needed since C++ compiler flags are applied
	# to everything in the extension module regardless of type.

	# libnatpmp
	c_glob.extend(list(glob.glob('native/ext/ZeroTierOne/ext/libnatpmp/natpmp.c')))
	c_glob.extend(list(glob.glob('native/ext/ZeroTierOne/ext/libnatpmp/wingettimeofday.c')))
	c_glob.extend(list(glob.glob('native/ext/ZeroTierOne/ext/libnatpmp/getgateway.c')))
	# miniupnpc
	c_glob.extend(list(glob.glob('native/ext/miniupnpc/*.c')))
	# lwip
	c_glob.extend(list(glob.glob('native/ext/lwip/src/netif/*.c')))
	c_glob.extend(list(glob.glob('native/ext/lwip/src/api/*.c')))
	c_glob.extend(list(glob.glob('native/ext/lwip/src/core/*.c')))
	c_glob.extend(list(glob.glob('native/ext/lwip/src/core/ipv4/*.c')))
	c_glob.extend(list(glob.glob('native/ext/lwip/src/core/ipv6/*.c')))
	c_glob.extend(list(glob.glob('native/ext/lwip/src/netif/*.c')))
	c_glob.extend(list(glob.glob('native/ext/lwip-contrib/ports/unix/port/sys_arch.c')))

cstuff = ('cstuff', {'sources':
	c_glob, 'include_dirs': my_include_dirs})

setup(
	name = 'libzt',
	version = main_ns['__version__'],
	description = 'ZeroTier',
	long_description=long_description,
    long_description_content_type='text/markdown',
	author = 'ZeroTier, Inc.',
	author_email = 'joseph@zerotier.com',
	url = 'https://github.com/zerotier/libzt',
	license='BUSL 1.1',
	download_url = 'https://github.com/zerotier/libzt/releases',
	keywords = 'zerotier p2p peer-to-peer sdwan sdn virtual network socket tcp udp zt encryption encrypted',
	py_modules = ['libzt'],
	packages = ['libzt'],
	classifiers = ['Topic :: Internet',
		'Topic :: System :: Networking',
		'Topic :: Security :: Cryptography',
		'Intended Audience :: Developers',
		'Intended Audience :: Information Technology',
		'Intended Audience :: System Administrators',
		'Intended Audience :: Telecommunications Industry',
		'Intended Audience :: End Users/Desktop',
		'License :: Free for non-commercial use',
		'Operating System :: MacOS',
		'Operating System :: POSIX :: BSD',
		'Operating System :: POSIX :: Linux',
		'Operating System :: Unix',
		'Programming Language :: C',
		'Programming Language :: C++',
		'Programming Language :: Python'
	],
	distclass=BinaryDistribution,
	libraries=[cstuff],
	ext_modules = [libzt_module],
)
