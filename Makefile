ifeq ($(OS),Windows_NT)
DIST_BUILD_SCRIPT := dist.bat
CLEAN_SCRIPT := clean.bat
else
DIST_BUILD_SCRIPT := ./dist.sh
endif

EXECUTABLES = cmake
build_reqs := $(foreach exec,$(EXECUTABLES),\
        $(if $(shell which $(exec)),some string,$(error "No $(exec) in PATH")))

.PHONY: list
list:
	@$(MAKE) -pRrq -f $(lastword $(MAKEFILE_LIST)) : 2>/dev/null | awk -v RS= \
	-F: '/^# File/,/^# Finished Make data base/ {if ($$1 !~ "^[#.]") \
	{print $$1}}' | sort | egrep -v -e '^[^[:alnum:]]' -e '^$@$$' | xargs

# Pull all submodules
update:
	@git submodule update --init
	@git submodule status

# Patch submodules (issue update first)
patch:
	#-cd ext/lwip; git apply ../lwip.patch;
	#-cd ext/lwip-contrib; git apply ../lwip-contrib.patch;
	#-cd ext/ZeroTierOne; git apply ../ZeroTierOne.patch;

# Target-specific clean
clean_ios:
	-rm -rf ports/xcode_ios
	-rm -rf ports/xcode_ios_simulator
clean_macos:
	-rm -rf ports/xcode_macos
clean_android:
	-rm -rf ports/android/app/build
	-find ports -name ".externalNativeBuild" -exec rm -r "{}" \;
clean_products:
	-rm -rf products
.PHONY: clean
clean: clean_ios clean_macos clean_android
	$(DIST_BUILD_SCRIPT) clean

# Use CMake generators to build projects from CMakeLists.txt
projects:
	$(DIST_BUILD_SCRIPT) generate_projects

# Android
android_debug:
	$(DIST_BUILD_SCRIPT) android "debug"
	$(DIST_BUILD_SCRIPT) clean_android_project
	$(DIST_BUILD_SCRIPT) prep_android_example "debug"
android_release:
	$(DIST_BUILD_SCRIPT) android "release"
	$(DIST_BUILD_SCRIPT) clean_android_project
	$(DIST_BUILD_SCRIPT) prep_android_example "release"
android_clean:
	$(DIST_BUILD_SCRIPT) android "clean"
android: android_debug android_release
prep_android_debug_example:
	$(DIST_BUILD_SCRIPT) prep_android_example "debug"
prep_android_release_example:
	$(DIST_BUILD_SCRIPT) prep_android_example "release"

# macOS
macos_debug:
	$(DIST_BUILD_SCRIPT) macos "debug"
macos_release:
	$(DIST_BUILD_SCRIPT) macos "release"
macos: macos_debug macos_release

# iOS
ios_debug:
	$(DIST_BUILD_SCRIPT) ios "debug"
ios_release:
	$(DIST_BUILD_SCRIPT) ios "release"
ios: ios_debug ios_release

# Host
host_release:
	$(DIST_BUILD_SCRIPT) host "release"
host_debug:
	$(DIST_BUILD_SCRIPT) host "debug"
host_clean:
	$(DIST_BUILD_SCRIPT) host "clean"
host_jar_debug:
	$(DIST_BUILD_SCRIPT) host_jar "debug"
host_jar_release:
	$(DIST_BUILD_SCRIPT) host_jar "release"
host_jar: host_jar_debug host_jar_release
host: host_debug host_release

# Build every target available on this host
all: host host_jar macos ios android
	$(DIST_BUILD_SCRIPT) display

# [For distribution process only] Prepare remote builds
wrap:
	$(DIST_BUILD_SCRIPT) wrap

# Binary distribution
bdist:
	$(DIST_BUILD_SCRIPT) merge
	$(DIST_BUILD_SCRIPT) bdist

# Source distribution
sdist: update patch
	$(DIST_BUILD_SCRIPT) sdist

dist: bdist sdist
