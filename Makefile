ifeq ($(OS),Windows_NT)
DIST_BUILD_SCRIPT := ports\dist.bat
CLEAN_SCRIPT := ports\clean.bat
else
DIST_BUILD_SCRIPT := ./ports/dist.sh
CLEAN_SCRIPT := ./ports/clean.sh
PACKAGE_SCRIPT := ./ports/package.sh
endif

EXECUTABLES = cmake
build_reqs := $(foreach exec,$(EXECUTABLES),\
        $(if $(shell which $(exec)),some string,$(error "No $(exec) in PATH")))

# Pull all submodules
update:
	git submodule update --init
	git submodule status

# Patch submodules (issue update first)
patch:
	-git -C ext/lwip apply ../lwip.patch
	-git -C ext/lwip-contrib apply ../lwip-contrib.patch
	-git -C ext/ZeroTierOne apply ../ZeroTierOne.patch

# Target-specific clean
clean_ios:
	-rm -rf ports/xcode_ios-arm64
clean_macos:
	-rm -rf ports/xcode_macos
clean_android:
	-rm -rf ports/android/app/build
	-find ports -name ".externalNativeBuild" -exec rm -r "{}" \;
clean_products:
	-rm -rf products
.PHONY: clean
clean: clean_ios clean_macos clean_android
	-rm -rf tmp lib bin products
	-rm -f *.o *.s *.exp *.lib *.core core
	find . -type f \( -name '*.dylib' -o -name '*.so' -o -name \
		'*.a' -o -name '*.o' -o -name '*.o.d' -o -name \
        '*.out' -o -name '*.log' -o -name '*.dSYM' -o -name '*.class' \) -delete

# Use CMake generators to build projects from CMakeLists.txt
projects:
	$(DIST_BUILD_SCRIPT) generate_projects

# Android
android_debug:
	$(DIST_BUILD_SCRIPT) android "debug"
android_release:
	$(DIST_BUILD_SCRIPT) android "release"
android_clean:
	$(DIST_BUILD_SCRIPT) android "clean"
android: android_debug android_release

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

# [For distribution process only] Marge and package everything into a tarball
dist:
	$(DIST_BUILD_SCRIPT) merge
	$(DIST_BUILD_SCRIPT) dist
