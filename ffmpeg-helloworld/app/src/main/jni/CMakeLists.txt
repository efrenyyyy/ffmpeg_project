# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.
cmake_minimum_required(VERSION 3.4.1)
include_directories(include/)
# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add.library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.
add_library( # Specifies the name of the library.
             hello-jni
            # Sets the library as a shared library.
             SHARED
            # Provides a relative path to your source file(s).
            com_example_efrenyang_helloworld_Hellojni.c
            com_example_efrenyang_helloworld_MainActivity.c )

find_library( # Sets the name of the path variable.
                          log-lib

                          # Specifies the name of the NDK library that
                          # you want CMake to locate.
                          log )

target_link_libraries( # Specifies the target library.
                                   hello-jni
                                   ${CMAKE_SOURCE_DIR}/libavcodec-57.so
                                   ${CMAKE_SOURCE_DIR}/libavutil-55.so
                                   ${CMAKE_SOURCE_DIR}/libavfilter-6.so
                                   ${CMAKE_SOURCE_DIR}/libavformat-57.so
                                   ${CMAKE_SOURCE_DIR}/libswresample-2.so
                                   ${CMAKE_SOURCE_DIR}/libswscale-4.so
                                   -lm -lz
                                   # Links the target library to the log library
                                   # included in the NDK.
                                   ${log-lib} )
