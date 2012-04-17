LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
        cmdl_parser.cpp \
        conv_string.cpp \
        genericStds.cpp \
        wav_file.cpp

LOCAL_CFLAGS := -DANDROID

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \
        $(LOCAL_PATH)/../include \
        $(LOCAL_PATH)/../../libFDK/include

LOCAL_MODULE:= libSYS

include $(BUILD_STATIC_LIBRARY)
