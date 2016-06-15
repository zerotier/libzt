NDK_TOOLCHAIN_VERSION := clang
APP_STL := c++_static
APP_CPPFLAGS := -O3 -DUSE_SOCKS_PROXY -DZT_SDK -fPIC -fPIE -fvectorize -Wall -fstack-protector -fexceptions -fno-strict-aliasing -Wno-deprecated-register -DZT_NO_TYPE_PUNNING=1
APP_PLATFORM := android-14

# Architectures
#APP_ABI := all
APP_ABI := armeabi-v7a
