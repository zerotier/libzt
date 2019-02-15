#
# ZeroTier SDK - Network Virtualization Everywhere
# Copyright (C) 2011-2019  ZeroTier, Inc.  https://www.zerotier.com/
# 
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program. If not, see <http://www.gnu.org/licenses/>.
# 
#  --
# 
#  You can be released from the requirements of the license by purchasing
#  a commercial license. Buying such a license is mandatory as soon as you
#  develop commercial closed-source software that incorporates or links
#  directly against ZeroTier software without disclosing the source code
#  of your own application.
# 

ifeq ($(OS),Windows_NT)
DIST_BUILD_SCRIPT := ports\dist.bat
CLEAN_SCRIPT := ports\clean.bat
else
DIST_BUILD_SCRIPT := ./ports/dist.sh
CLEAN_SCRIPT := ./ports/clean.sh
PACKAGE_SCRIPT := ./ports/package.sh
endif

CONCURRENT_BUILD_JOBS=2

# Patch submodules
patch:
	-git -C ext/lwip apply ../lwip.patch
	-git -C ext/lwip-contrib apply ../lwip-contrib.patch
	-git -C ext/ZeroTierOne apply ../ZeroTierOne.patch

.PHONY: clean
clean:
	rm -rf bin staging generated dist

all: debug release

release:
	-mkdir generated
	cmake -H. -Bgenerated/release -DCMAKE_BUILD_TYPE=Release
	cmake --build generated/release -j $(CONCURRENT_BUILD_JOBS)

debug:
	-mkdir generated
	cmake -H. -Bgenerated/debug -DCMAKE_BUILD_TYPE=Debug
	cmake --build generated/debug -j $(CONCURRENT_BUILD_JOBS)

# dist:
# Build and package everything
# This command shall be run twice:
# (1) Generates projects
#    <perform any required modifications>
# (2) Build products and package everything
.PHONY: dist
dist: patch
	$(DIST_BUILD_SCRIPT)
