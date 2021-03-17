#!/bin/bash

PYBIN=python3

# Build the extension module
ext()
{
	# Symbolic link to source tree so that sdist structure makes sense
	ln -s ../../ native
	# Copy language bindings into module directory
	cp -f native/src/bindings/python/*.py libzt/
	cp -f native/LICENSE.txt LICENSE
	# Build C libraries (and then) C++ extension
	$PYBIN setup.py build_clib --verbose build_ext -i --verbose
}

# Build a wheel
wheel()
{
	ext
	$PYBIN setup.py bdist_wheel
}

clean()
{
	find . -name '*.so' -type f -delete
	find . -name '*.pyc' -type f -delete
	find . -name '__pycache__' -type d -delete
	rm -rf libzt/sockets.py
	rm -rf libzt/libzt.py
	rm -rf src ext build dist native
	rm -rf libzt.egg-info
	rm -rf LICENSE
}

"$@"
