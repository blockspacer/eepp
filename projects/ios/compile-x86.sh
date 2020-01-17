#!/bin/sh
cd $(dirname "$0")

CLANGPATH=`xcrun -find -sdk iphoneos clang`
export TOOLCHAINPATH=`dirname $CLANGPATH`/

if [ -z "$IOSVERSION" ]; then
export IOSVERSION=10.0
fi

if [ -z "$SYSROOTPATH" ]; then
export SYSROOTPATH=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator$IOSVERSION.sdk/
fi

premake4 --file=../../premake4.lua --platform=ios-x86 --with-static-eepp --with-gles1 --with-gles2 --with-static-backend --use-frameworks gmake 

cd ../../make/ios-x86/
make -j`nproc` $@ eepp-static
