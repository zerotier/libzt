# NOTE: This file only exists as a convenience for cleaning and building
# products for release. To build, use CMake. Instructions in README.md

.PHONY: clean
clean:
	rm -rf bin build products tmp
	rm -f *.o *.s *.exp *.lib .depend* *.core core
	rm -rf .depend
	find . -type f \( -name '*.a' -o -name '*.o' -o -name '*.so' -o -name \
		'*.o.d' -o -name '*.out' -o -name '*.log' -o -name '*.dSYM' -o -name '*.dylib' -o -name '*.class' \) -delete

# Build and package everything
# This command shall be run twice:
# (1) Generates projects
#    <perform any required modifications>
# (2) Build products and package everything
.PHONY: dist
dist: patch
	./packages/dist.sh

# Initialize submodules and apply patches
.PHONY: all
all: update patch
	cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
	cmake --build build

# Clean build paths
clean_win:
	-"rd /S /Q bin"
	-"rd /S /Q build"

# Remove any CMake-generated library-building projects
clean_packages:
	rm -rf packages/xcode_ios
	rm -rf packages/xcode_macos

.PHONY: update
update:
	git submodule update --init --recursive

# Patch submodules
patch:
	-git -C ext/lwip apply ../lwip.patch
	-git -C ext/lwip-contrib apply ../lwip-contrib.patch