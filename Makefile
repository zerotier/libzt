# NOTE: This file only exists as a convenience for cleaning and building
# products for release. To build, use CMake. Instructions in README.md

ifeq ($(OS),Windows_NT)
DIST_BUILD_SCRIPT := packages\dist.bat
CLEAN_SCRIPT := packages\clean.bat
else
DIST_BUILD_SCRIPT := ./packages/dist.sh
CLEAN_SCRIPT := ./packages/clean.sh
PACKAGE_SCRIPT := ./packages/package.sh
endif

.PHONY: clean
clean:
	$(CLEAN_SCRIPT)

# Build and package everything
# This command shall be run twice:
# (1) Generates projects
#    <perform any required modifications>
# (2) Build products and package everything

.PHONY: dist
dist: patch
	$(DIST_BUILD_SCRIPT)

.PHONY: package
package:
	$(PACKAGE_SCRIPT)

# Initialize submodules and apply patches
.PHONY: all
all: update patch
	cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
	cmake --build build

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
	-git -C ext/ZeroTierOne apply ../ZeroTierOne.patch
