LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
        env_calc.cpp \
        env_dec.cpp \
        env_extr.cpp \
        huff_dec.cpp \
        lpp_tran.cpp \
        psbitdec.cpp \
        psdec.cpp \
        psdec_hybrid.cpp \
        sbr_crc.cpp \
        sbr_deb.cpp \
        sbr_dec.cpp \
        sbrdec_drc.cpp \
        sbrdec_freq_sca.cpp \
        sbrdecoder.cpp \
        sbr_ram.cpp \
        sbr_rom.cpp

LOCAL_CFLAGS := -DANDROID

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \
        $(LOCAL_PATH)/../include \
        $(LOCAL_PATH)/../../libFDK/include \
        $(LOCAL_PATH)/../../libSYS/include

LOCAL_MODULE:= libSBRdec

include $(BUILD_STATIC_LIBRARY)
