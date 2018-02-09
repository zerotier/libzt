#!/usr/bin/env python

from setuptools import setup, Extension, Command, Distribution
import glob
import os

class BinaryDistribution(Distribution):
    def is_pure(self):
        return False

# WINDOWS
if os.name == 'nt':
	projDir='..\\..'
	source_list = ['libzt_wrap.cxx']
	source_list.extend(list(glob.glob(projDir+'\\src\\*.cpp')))
	source_list.extend(list(glob.glob(projDir+'\\zto\\node\\*.cpp')))
	source_list.extend(list(glob.glob(projDir+'\\zto\\osdep\\*.cpp')))
	source_list.extend(list(glob.glob(projDir+'\\zto\\service\\*.cpp')))
	source_list.extend(list(glob.glob(projDir+'\\zto\\controller\\*.cpp')))

	source_list = list(set(source_list)-set(
	[
		projDir+'\\zto\\osdep\\LinuxEthernetTap.cpp',
		projDir+'\\zto\\osdep\\BSDEthernetTap.cpp',
		projDir+'\\zto\\osdep\\OSXEthernetTap.cpp',
		projDir+'\\zto\\osdep\\WindowsEthernetTap.cpp'
	]))

	libzt_module = Extension('libzt',
		extra_compile_args=['/std:c++14', '-DNOMINMAX=1', '-DZT_SDK', '-DSDK', '-DZT_SOFTWARE_UPDATE_DEFAULT=\"disable\"'],
		extra_link_args=['/LIBPATH:.', 'WS2_32.Lib', 'ShLwApi.Lib', 'iphlpapi.Lib','lwip.lib','http.lib'],
		sources=source_list,
		include_dirs=[projDir+'\\include',
			projDir+'\\ext\\lwip\\src\\include',
			projDir+'\\ext\\lwip-contrib\\ports\\win32\\include',
			projDir+'\\zto\\include',
			projDir+'\\zto\\node',
			projDir+'\\zto\\service',
			projDir+'\\zto\\osdep',
			projDir+'\\zto\\controller']                           
		)
# EVERYTHING ELSE
else:
	projDir='../..'
	source_list = ['libzt_wrap.cxx']
	source_list.extend(list(glob.glob(projDir+'/src/*.cpp')))
	source_list.extend(list(glob.glob(projDir+'/zto/node/*.cpp')))
	source_list.extend(list(glob.glob(projDir+'/zto/osdep/*.cpp')))
	source_list.extend(list(glob.glob(projDir+'/zto/service/*.cpp')))
	source_list.extend(list(glob.glob(projDir+'/zto/controller/*.cpp')))

	source_list = list(set(source_list)-set(
	[
		projDir+'/zto/osdep/LinuxEthernetTap.cpp',
		projDir+'/zto/osdep/BSDEthernetTap.cpp',
		projDir+'/zto/osdep/OSXEthernetTap.cpp',
		projDir+'/zto/osdep/WindowsEthernetTap.cpp'
	]))

	libzt_module = Extension('libzt',
		extra_compile_args=['-std=c++11', '-DZT_SDK', '-DZT_SOFTWARE_UPDATE_DEFAULT=\"disable\"'],
		extra_link_args=['-L.','-llwip','-lhttp'],
		sources=source_list,
		include_dirs=[projDir+'/include',
			projDir+'/ext/lwip/src/include',
			projDir+'/ext/lwip-contrib/ports/unix/include',
			projDir+'/zto/include',
			projDir+'/zto/node',
			projDir+'/zto/service',
			projDir+'/zto/osdep',
			projDir+'/zto/controller']                           
		)

setup(    
	include_package_data=True,
    distclass=BinaryDistribution,
	ext_modules = [libzt_module],
	py_modules = ['libzt'],
	name = 'libzt',
	packages = ['libzt'],
	version = '1.1.6a0',
	description = 'ZeroTier',
	long_description = 'Encrypted P2P communication between apps and services',
	author = 'ZeroTier, Inc.',
	author_email = 'josep@zerotier.com',
	url = 'https://github.com/zerotier/libzt',
	license='GPLv3',
	download_url = 'https://github.com/zerotier/libzt/archive/1.1.6.tar.gz',
	keywords = 'zerotier sdwan sdn virtual network socket p2p peer-to-peer',
	classifiers = ['Development Status :: 3 - Alpha',
		'Environment :: MacOS X',
		'Environment :: Win32 (MS Windows)',
		'Intended Audience :: Information Technology',
		'Intended Audience :: Science/Research',
		'Intended Audience :: System Administrators',
		'Intended Audience :: Telecommunications Industry',
		'Intended Audience :: End Users/Desktop',
		'Intended Audience :: Developers',
		'License :: Free for non-commercial use',
		'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
		'Operating System :: iOS',
		'Operating System :: MacOS :: MacOS X',
		'Operating System :: Microsoft :: Windows',
		'Operating System :: POSIX :: BSD',
		'Operating System :: Unix',
		'Programming Language :: C++',
		'Programming Language :: C',
		'Programming Language :: Python',
		'Topic :: Internet',
		'Topic :: Security :: Cryptography',
		'Topic :: System :: Networking'
	],
)