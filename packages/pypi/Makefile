bdist:
	cd ../../; cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=DEBUG; cmake --build build
	cp ../../bin/lib/*.a .
	python3 setup.py bdist_wheel	

upload:
	twine upload dist/*

clean:
	rm -rf *.a
	rm -rf build
	rm -rf dist
	rm -rf libzt.egg-info
