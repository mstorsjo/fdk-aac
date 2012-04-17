LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
        bit_sbr.cpp \
        env_bit.cpp \
        fram_gen.cpp \
        mh_det.cpp \
        ps_bitenc.cpp \
        ps_encode.cpp \
        resampler.cpp \
        sbr_encoder.cpp \
        sbr_ram.cpp \
        ton_corr.cpp \
        code_env.cpp \
        env_est.cpp \
        invf_est.cpp \
        nf_est.cpp \
        psenc_hybrid.cpp \
        ps_main.cpp \
        sbrenc_freq_sca.cpp \
        sbr_misc.cpp \
        sbr_rom.cpp \
        tran_det.cpp

LOCAL_CFLAGS := -DANDROID

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \
        $(LOCAL_PATH)/../include \
        $(LOCAL_PATH)/../../libFDK/include \
        $(LOCAL_PATH)/../../libSYS/include

LOCAL_MODULE:= libSBRenc

include $(BUILD_STATIC_LIBRARY)
