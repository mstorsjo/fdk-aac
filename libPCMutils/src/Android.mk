LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := pcmutils_lib.cpp

LOCAL_CFLAGS := -DANDROID

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \
        $(LOCAL_PATH)/../include \
        $(LOCAL_PATH)/../../libSYS/include \
        $(LOCAL_PATH)/../../libFDK/include

LOCAL_MODULE:= libPCMutils

include $(BUILD_STATIC_LIBRARY)
