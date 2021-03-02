LIB_OUTPUT_DIR = $(shell cd ../../ && ./build.sh gethosttype)

copy_wrapper_sources:
	cp -f ../../src/bindings/python/*.py .

debug: copy_wrapper_sources
	cd ../../ && ./build.sh host-python "debug"
	cp -f ../../dist/$(LIB_OUTPUT_DIR)-python-debug/lib/*.so .
	
release: copy_wrapper_sources
	cd ../../ && ./build.sh host-python "release"
	cp -f ../../dist/$(LIB_OUTPUT_DIR)-python-release/lib/*.so .

clean:
	rm -rf *.so libzt.py prototype.py