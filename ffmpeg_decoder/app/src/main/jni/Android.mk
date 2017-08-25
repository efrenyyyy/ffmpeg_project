LOCAL_PATH := $(call my-dir)
TARGET_ARCH_ABI := arm64-v8a

include $(CLEAR_VARS)
LOCAL_MODULE := ffmpeg
LOCAL_SRC_FILES := libffmpeg.so
include $(PREBUILT_SHARED_LIBRARY)

# Program
include $(CLEAR_VARS)
LOCAL_MODULE := decoder-lib
LOCAL_SRC_FILES := com_example_efrenyang_ffmpeg_decoder_MainActivity.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_LDLIBS := -llog -lz
LOCAL_SHARED_LIBRARIES :=  ffmpeg
include $(BUILD_SHARED_LIBRARY)