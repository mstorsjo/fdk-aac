LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
        tpdec_adif.cpp \
        tpdec_adts.cpp \
        tpdec_asc.cpp \
        tpdec_latm.cpp \
        tpdec_lib.cpp

LOCAL_CFLAGS := -DANDROID

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \
        $(LOCAL_PATH)/../include \
        $(LOCAL_PATH)/../../libSYS/include \
        $(LOCAL_PATH)/../../libFDK/include

LOCAL_MODULE:= libMpegTPDec

include $(BUILD_STATIC_LIBRARY)
