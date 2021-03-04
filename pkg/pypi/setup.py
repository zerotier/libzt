#!/usr/bin/env python

from setuptools import setup, Extension, Command, Distribution
import glob
import os

class BinaryDistribution(Distribution):
	def is_pure(self):
		return False

cpp_glob = []
c_glob = []

# Windows
if os.name == 'nt':
	print('TODO')
	#extra_compile_args=['/std:c++14', '-DNOMINMAX=1', '-DZT_SDK', '-DSDK'],
	#extra_link_args=['/LIBPATH:.', 'WS2_32.Lib', 'ShLwApi.Lib', 'iphlpapi.Lib','lwip.lib'],

# Everything else
else:
	cpp_glob.extend(list(glob.glob('native/src/bindings/python/*.cpp')))
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
			'native/ext/ZeroTierOne/node',
			'native/ext/ZeroTierOne/service',
			'native/ext/ZeroTierOne/osdep',
			'native/ext/ZeroTierOne/controller']

	libzt_module = Extension('libzt._libzt',
		extra_compile_args=['-std=c++11', '-DZTS_ENABLE_PYTHON=1', '-DZT_SDK'],
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
	version = '1.3.3',
	description = 'ZeroTier',
	long_description = 'Encrypted P2P communication between apps and services',
	author = 'ZeroTier, Inc.',
	author_email = 'joseph.henry@zerotier.com',
	url = 'https://github.com/zerotier/libzt',
	license='BUSL 1.1',
	download_url = 'https://github.com/zerotier/libzt/archive/1.3.3.tar.gz',
	keywords = 'zerotier sdwan sdn virtual network socket p2p peer-to-peer',
	py_modules = ['libzt'],
	packages = ['libzt'],
	classifiers = ['Development Status :: 3 - Alpha',
		'Topic :: Internet',
		'Topic :: System :: Networking',
		'Topic :: Security :: Cryptography',
		'Operating System :: OS Independent',
		'Intended Audience :: Developers',
		'Intended Audience :: Information Technology',
		'Intended Audience :: Science/Research',
		'Intended Audience :: System Administrators',
		'Intended Audience :: Telecommunications Industry',
		'Intended Audience :: End Users/Desktop',
		'License :: Free for non-commercial use',
		'Operating System :: MacOS :: MacOS X',
		'Operating System :: Microsoft :: Windows',
		'Operating System :: POSIX :: BSD',
		'Operating System :: Unix',
		'Programming Language :: C++',
		'Programming Language :: C',
		'Programming Language :: Python'
	],
	distclass=BinaryDistribution,
	libraries=[cstuff],
	ext_modules = [libzt_module],
	python_requires='>=3.0',
)