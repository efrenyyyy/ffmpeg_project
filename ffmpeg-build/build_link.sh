#!/bin/bash 
NDK=E:/iqiyi/android/ndk/android-ndk-r14b 
PREBUILT=$NDK/toolchains/aarch64-linux-android-4.9/prebuilt
PLATFORM=$NDK/platforms/android-21/arch-arm64
PREFIX=./android/arm

$PREBUILT/windows-x86_64/bin/aarch64-linux-android-ld -rpath-link=$PLATFORM/usr/lib -L$PLATFORM/usr/lib -L$PREFIX/lib -soname libffmpeg.so \
-shared -nostdlib -Bsymbolic --whole-archive --no-undefined -o $PREFIX/libffmpeg.so $PREFIX/lib/libavcodec.a $PREFIX/lib/libavfilter.a $PREFIX/lib/libswresample.a \
$PREFIX/lib/libavformat.a $PREFIX/lib/libavutil.a $PREFIX/lib/libswscale.a $PREFIX/lib/libavdevice.a -lc -lm -lz -ldl -llog --dynamic-linker=/system/bin/linker \
$PREBUILT/windows-x86_64/lib/gcc/aarch64-linux-android/4.9.x/libgcc.a 