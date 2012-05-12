LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
        autocorr2nd.cpp \
        dct.cpp \
        FDK_bitbuffer.cpp \
        FDK_core.cpp \
        FDK_crc.cpp \
        FDK_hybrid.cpp \
        FDK_tools_rom.cpp \
        FDK_trigFcts.cpp \
        fft.cpp \
        fft_rad2.cpp \
        fixpoint_math.cpp \
        mdct.cpp \
        qmf.cpp \
        scale.cpp

LOCAL_CFLAGS := -DANDROID

LOCAL_CFLAGS += -Wno-sequence-point -Wno-extra

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \
        $(LOCAL_PATH)/../include \
        $(LOCAL_PATH)/../../libSYS/include \

LOCAL_MODULE:= libFDK

include $(BUILD_STATIC_LIBRARY)
