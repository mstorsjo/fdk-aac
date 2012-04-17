LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
        aacenc.cpp \
        aacEnc_ram.cpp \
        band_nrg.cpp \
        block_switch.cpp \
        grp_data.cpp \
        metadata_main.cpp \
        pre_echo_control.cpp \
        quantize.cpp \
        tonality.cpp \
        aacenc_hcr.cpp \
        aacEnc_rom.cpp \
        bandwidth.cpp \
        channel_map.cpp \
        intensity.cpp \
        ms_stereo.cpp \
        psy_configuration.cpp \
        sf_estim.cpp \
        transform.cpp \
        aacenc_lib.cpp \
        aacenc_tns.cpp \
        bit_cnt.cpp \
        chaosmeasure.cpp \
        line_pe.cpp \
        noisedet.cpp \
        psy_main.cpp \
        spreading.cpp \
        aacenc_pns.cpp \
        adj_thr.cpp \
        bitenc.cpp \
        dyn_bits.cpp \
        metadata_compressor.cpp \
        pnsparam.cpp \
        qc_main.cpp \
        tns_param.cpp

LOCAL_CFLAGS := -DANDROID

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \
        $(LOCAL_PATH)/../include \
        $(LOCAL_PATH)/../../libPCMutils/include \
        $(LOCAL_PATH)/../../libFDK/include \
        $(LOCAL_PATH)/../../libSYS/include \
        $(LOCAL_PATH)/../../libMpegTPEnc/include \
        $(LOCAL_PATH)/../../libSBRenc/include

LOCAL_MODULE:= libAACenc

include $(BUILD_STATIC_LIBRARY)
