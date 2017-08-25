#!/bin/bash 
NDK=E:/iqiyi/android/ndk/android-ndk-r14b 
SYSROOT=$NDK/platforms/android-21/arch-arm64/
TOOLCHAIN=$NDK/toolchains/aarch64-linux-android-4.9/prebuilt/windows-x86_64
function build_one { 
./configure \
     --prefix=$PREFIX \
     --enable-shared \
     --disable-static \
     --disable-doc \
     --disable-ffmpeg \
     --disable-ffplay \
     --disable-ffprobe \
     --disable-ffserver \
     --disable-avdevice \
     --disable-doc \
     --disable-symver \
     --cross-prefix=$TOOLCHAIN/bin/aarch64-linux-android- \
     --target-os=linux \
     --arch=aarch64 \
     --enable-cross-compile \
     --sysroot=$SYSROOT \
     --extra-cflags="-Os -fpic $ADDI_CFLAGS" \
     --extra-ldflags="$ADDI_LDFLAGS" \
     $ADDITIONAL_CONFIGURE_FLAG 
     make clean 
     make -j 4
     make install
 }
 CPU=arm
 PREFIX=$(pwd)/android/$CPU
 ADDI_CFLAGS=" " 
 build_one