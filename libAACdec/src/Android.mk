LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
        aacdec_drc.cpp \
        aacdec_hcr.cpp \
        aacdecoder.cpp \
        aacdec_pns.cpp \
        aac_ram.cpp \
        block.cpp \
        channelinfo.cpp \
        ldfiltbank.cpp \
        rvlcbit.cpp \
        rvlc.cpp \
        aacdec_hcr_bit.cpp \
        aacdec_hcrs.cpp \
        aacdecoder_lib.cpp \
        aacdec_tns.cpp \
        aac_rom.cpp \
        channel.cpp \
        conceal.cpp \
        pulsedata.cpp \
        rvlcconceal.cpp \
        stereo.cpp

LOCAL_CFLAGS := -DANDROID

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \
        $(LOCAL_PATH)/../include \
        $(LOCAL_PATH)/../../libPCMutils/include \
        $(LOCAL_PATH)/../../libFDK/include \
        $(LOCAL_PATH)/../../libSYS/include \
        $(LOCAL_PATH)/../../libMpegTPDec/include \
        $(LOCAL_PATH)/../../libSBRdec/include

LOCAL_MODULE:= libAACdec

include $(BUILD_STATIC_LIBRARY)
