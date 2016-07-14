NDK_TOOLCHAIN_VERSION := clang
APP_STL := c++_static
APP_CPPFLAGS := -O3 -DZT_DEBUG -DSDK_DEBUG -DLWIP_DEBUG -DUSE_SOCKS_PROXY -DSDK -fPIC -fPIE -fvectorize -Wall -fstack-protector -fexceptions -fno-strict-aliasing -Wno-deprecated-register -DZT_NO_TYPE_PUNNING=1
APP_PLATFORM := android-14

# Architectures
# APP_ABI := all
#APP_ABI += arm64-v8a
APP_ABI += armeabi
#APP_ABI += armeabi-v7a
#APP_ABI += mips
#APP_ABI += mips64
#APP_ABI += x86
#APP_ABI += x86_64
