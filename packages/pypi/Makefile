# generate a new wrapper *.cxx from the *.i interface file
wrapper:
	swig -c++ -python libzt.i

# build and package a binary distribution
bdist:
	cd ../../; cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=DEBUG
	cd ../../; cmake --build build
	cp ../../bin/lib/*.a .
	python3 setup.py bdist_wheel	

# upload to PyPI
upload:
	twine upload dist/*

clean:
	rm -rf *.a
	rm -rf build
	rm -rf dist
	rm -rf libzt.egg-info
