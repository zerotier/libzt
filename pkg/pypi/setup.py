# -*- coding: utf-8 -*-
from setuptools import setup

packages = \
['libzt']

package_data = \
{'': ['*']}

setup_kwargs = {
    'name': 'libzt',
    'version': '1.8.10',
    'description': 'ZeroTier',
    'long_description': '<div align="center">\n\n<img width=120px src=https://github.com/zerotier/ZeroTierOne/raw/master/artwork/ZeroTierIcon512x512.png></img>\n\n<h1>libzt (ZeroTier)</h1>\nPeer-to-peer and cross-platform encrypted connections built right into your app or service. No drivers, no root, and no host configuration.\n\n<a href="https://github.com/zerotier/libzt/"><img alt="latest libzt version" src="https://img.shields.io/github/v/tag/zerotier/libzt?label=latest version"/></a>\n<a href="https://github.com/zerotier/libzt/commits/master"><img alt="Last Commit" src="https://img.shields.io/github/last-commit/zerotier/libzt"/></a>\n<a href="https://github.com/zerotier/libzt/actions"><img alt="Build Status (master branch)" src="https://img.shields.io/github/workflow/status/zerotier/libzt/CMake/master"/></a>\n</div>\n\n<div align="center"> \n\nExamples, tutorials and API docs for Python and other languages: [github.com/zerotier/libzt](https://www.github.com/zerotier/libzt)\n\n*NOTE: The implementation of this language binding attempts to be as Pythonic as possible. If something is not as it should be, please make a pull request or issue. Thanks.*\n',
    'author': 'ZeroTier, Inc.',
    'author_email': 'contact@zerotier.com',
    'maintainer': 'Joseph Henry',
    'maintainer_email': 'contact@zerotier.com',
    'url': 'https://www.zerotier.com/',
    'packages': packages,
    'package_data': package_data,
    'python_requires': '>=3.5,<4.0',
}
from build import *
build(setup_kwargs)

setup(**setup_kwargs)
