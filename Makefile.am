ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

libfdk_aac_la_CPPFLAGS = \
    -I$(top_srcdir)/cpp/libAACdec/include \
    -I$(top_srcdir)/cpp/libAACenc/include \
    -I$(top_srcdir)/cpp/libArithCoding/include \
    -I$(top_srcdir)/cpp/libDRCdec/include \
    -I$(top_srcdir)/cpp/libSACdec/include \
    -I$(top_srcdir)/cpp/libSACenc/include \
    -I$(top_srcdir)/cpp/libSBRdec/include \
    -I$(top_srcdir)/cpp/libSBRenc/include \
    -I$(top_srcdir)/cpp/libMpegTPDec/include \
    -I$(top_srcdir)/cpp/libMpegTPEnc/include \
    -I$(top_srcdir)/cpp/libSYS/include \
    -I$(top_srcdir)/cpp/libFDK/include \
    -I$(top_srcdir)/cpp/libPCMutils/include

libfdk_xhe_aac_la_CPPFLAGS = \
    -I$(top_srcdir)/xhe-aac/aacfastenc/include \
    -I$(top_srcdir)/xhe-aac/applauseDetectionlib/include \
    -I$(top_srcdir)/xhe-aac/iisArithEncLib/include \
    -I$(top_srcdir)/xhe-aac/iisAudioPrerollLib/include \
    -I$(top_srcdir)/xhe-aac/iisBitFrame/include \
    -I$(top_srcdir)/xhe-aac/iisDRCGainEncLib/include \
    -I$(top_srcdir)/xhe-aac/iisDRCGainGenLib/include \
    -I$(top_srcdir)/xhe-aac/iisEncoderConfig/include \
    -I$(top_srcdir)/xhe-aac/iisEncoderConfigCTable/include \
    -I$(top_srcdir)/xhe-aac/iisLevelerLib/include \
    -I$(top_srcdir)/xhe-aac/iisLinkedList/include \
    -I$(top_srcdir)/xhe-aac/iisLoudnessLib/include \
    -I$(top_srcdir)/xhe-aac/iisLPDComLib/include \
    -I$(top_srcdir)/xhe-aac/iisLPDEncLib/include \
    -I$(top_srcdir)/xhe-aac/iisParamList/include \
    -I$(top_srcdir)/xhe-aac/iisSigMap/include \
    -I$(top_srcdir)/xhe-aac/iisSwitchingDecision/include \
    -I$(top_srcdir)/xhe-aac/iisxHEAACEncLib/include \
    -I$(top_srcdir)/xhe-aac/limiterlib/include \
    -I$(top_srcdir)/xhe-aac/mathlib/include \
    -I$(top_srcdir)/xhe-aac/mp4ASC/IIS_MP4ASC_Interface \
    -I$(top_srcdir)/xhe-aac/mp4ASC/include \
    -I$(top_srcdir)/xhe-aac/mp4General \
    -I$(top_srcdir)/xhe-aac/mp4bitbuf/include \
    -I$(top_srcdir)/xhe-aac/mp4bitmux/src/aac \
    -I$(top_srcdir)/xhe-aac/mp4loaswrite/include \
    -I$(top_srcdir)/xhe-aac/mp4timebuf/include \
    -I$(top_srcdir)/xhe-aac/qmflib/include \
    -I$(top_srcdir)/xhe-aac/readonlybitbuf/include \
    -I$(top_srcdir)/xhe-aac/resamplelib/include \
    -I$(top_srcdir)/xhe-aac/sbrEnclib/include \
    -I$(top_srcdir)/xhe-aac/spaceEnclib/include \
    -I$(top_srcdir)/xhe-aac/statClassLib_C/include \
    -I$(top_srcdir)/xhe-aac/utillib/include \
    -I$(top_srcdir)/xhe-aac/utillib/include/IISutillib \
    -I$(top_srcdir)/xhe-aac/writeonlybitbuf/include \
    -I$(top_srcdir)/xhe-aac/xHEAACEnc/include

AM_CXXFLAGS = -fno-exceptions -fno-rtti
libfdk_aac_la_LINK = $(LINK) $(libfdk_aac_la_LDFLAGS)
# Mention a dummy pure C file to trigger generation of the $(LINK) variable
nodist_EXTRA_libfdk_aac_la_SOURCES = dummy.c

fdk_aacincludedir = $(includedir)/fdk-aac
fdk_aacinclude_HEADERS = \
    $(top_srcdir)/cpp/libSYS/include/machine_type.h \
    $(top_srcdir)/cpp/libSYS/include/genericStds.h \
    $(top_srcdir)/cpp/libSYS/include/FDK_audio.h \
    $(top_srcdir)/cpp/libSYS/include/syslib_channelMapDescr.h \
    $(top_srcdir)/cpp/libAACenc/include/aacenc_lib.h \
    $(top_srcdir)/cpp/libAACdec/include/aacdecoder_lib.h

fdk_xhe_aacincludedir = $(includedir)/fdk-xhe-aac
fdk_xhe_aacinclude_HEADERS = \
    $(top_srcdir)/xhe-aac/xHEAACEnc/include/xHEAACEnc.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = fdk-aac.pc fdk-xhe-aac.pc

lib_LTLIBRARIES = libfdk-aac.la libfdk-xhe-aac.la

libfdk_aac_la_LDFLAGS = -version-info @FDK_AAC_VERSION@ -no-undefined \
    -export-symbols $(top_srcdir)/fdk-aac.sym

libfdk_xhe_aac_la_LDFLAGS = -version-info @FDK_XHE_AAC_VERSION@ -no-undefined \
    -export-symbols $(top_srcdir)/fdk-xhe-aac.sym

if EXAMPLE
bin_PROGRAMS = aac-enc$(EXEEXT)

aac_enc_LDADD = libfdk-aac.la
aac_enc_CPPFLAGS = $(libfdk_aac_la_CPPFLAGS) -I$(top_srcdir)/cpp
aac_enc_SOURCES = aac-enc.c wavreader.c

noinst_HEADERS = wavreader.h
endif

AACDEC_SRC = \
    cpp/libAACdec/src/FDK_delay.cpp \
    cpp/libAACdec/src/aac_ram.cpp \
    cpp/libAACdec/src/aac_rom.cpp \
    cpp/libAACdec/src/aacdec_drc.cpp \
    cpp/libAACdec/src/aacdec_hcr.cpp \
    cpp/libAACdec/src/aacdec_hcr_bit.cpp \
    cpp/libAACdec/src/aacdec_hcrs.cpp \
    cpp/libAACdec/src/aacdec_pns.cpp \
    cpp/libAACdec/src/aacdec_tns.cpp \
    cpp/libAACdec/src/aacdecoder.cpp \
    cpp/libAACdec/src/aacdecoder_lib.cpp \
    cpp/libAACdec/src/block.cpp \
    cpp/libAACdec/src/channel.cpp \
    cpp/libAACdec/src/channelinfo.cpp \
    cpp/libAACdec/src/conceal.cpp \
    cpp/libAACdec/src/ldfiltbank.cpp \
    cpp/libAACdec/src/pulsedata.cpp \
    cpp/libAACdec/src/rvlc.cpp \
    cpp/libAACdec/src/rvlcbit.cpp \
    cpp/libAACdec/src/rvlcconceal.cpp \
    cpp/libAACdec/src/stereo.cpp \
    cpp/libAACdec/src/usacdec_ace_d4t64.cpp \
    cpp/libAACdec/src/usacdec_ace_ltp.cpp \
    cpp/libAACdec/src/usacdec_acelp.cpp \
    cpp/libAACdec/src/usacdec_fac.cpp \
    cpp/libAACdec/src/usacdec_lpc.cpp \
    cpp/libAACdec/src/usacdec_lpd.cpp \
    cpp/libAACdec/src/usacdec_rom.cpp

AACENC_SRC = \
    cpp/libAACenc/src/aacEnc_ram.cpp \
    cpp/libAACenc/src/aacEnc_rom.cpp \
    cpp/libAACenc/src/aacenc.cpp \
    cpp/libAACenc/src/aacenc_lib.cpp \
    cpp/libAACenc/src/aacenc_pns.cpp \
    cpp/libAACenc/src/aacenc_tns.cpp \
    cpp/libAACenc/src/adj_thr.cpp \
    cpp/libAACenc/src/band_nrg.cpp \
    cpp/libAACenc/src/bandwidth.cpp \
    cpp/libAACenc/src/bit_cnt.cpp \
    cpp/libAACenc/src/bitenc.cpp \
    cpp/libAACenc/src/block_switch.cpp \
    cpp/libAACenc/src/channel_map.cpp \
    cpp/libAACenc/src/chaosmeasure.cpp \
    cpp/libAACenc/src/dyn_bits.cpp \
    cpp/libAACenc/src/grp_data.cpp \
    cpp/libAACenc/src/intensity.cpp \
    cpp/libAACenc/src/line_pe.cpp \
    cpp/libAACenc/src/metadata_compressor.cpp \
    cpp/libAACenc/src/metadata_main.cpp \
    cpp/libAACenc/src/mps_main.cpp \
    cpp/libAACenc/src/ms_stereo.cpp \
    cpp/libAACenc/src/noisedet.cpp \
    cpp/libAACenc/src/pnsparam.cpp \
    cpp/libAACenc/src/pre_echo_control.cpp \
    cpp/libAACenc/src/psy_configuration.cpp \
    cpp/libAACenc/src/psy_main.cpp \
    cpp/libAACenc/src/qc_main.cpp \
    cpp/libAACenc/src/quantize.cpp \
    cpp/libAACenc/src/sf_estim.cpp \
    cpp/libAACenc/src/spreading.cpp \
    cpp/libAACenc/src/tonality.cpp \
    cpp/libAACenc/src/transform.cpp

ARITHCODING_SRC = \
    cpp/libArithCoding/src/ac_arith_coder.cpp

DRCDEC_SRC = \
    cpp/libDRCdec/src/FDK_drcDecLib.cpp \
    cpp/libDRCdec/src/drcDec_gainDecoder.cpp \
    cpp/libDRCdec/src/drcDec_reader.cpp \
    cpp/libDRCdec/src/drcDec_rom.cpp \
    cpp/libDRCdec/src/drcDec_selectionProcess.cpp \
    cpp/libDRCdec/src/drcDec_tools.cpp \
    cpp/libDRCdec/src/drcGainDec_init.cpp \
    cpp/libDRCdec/src/drcGainDec_preprocess.cpp \
    cpp/libDRCdec/src/drcGainDec_process.cpp

FDK_SRC = \
    cpp/libFDK/src/FDK_bitbuffer.cpp \
    cpp/libFDK/src/FDK_core.cpp \
    cpp/libFDK/src/FDK_crc.cpp \
    cpp/libFDK/src/FDK_decorrelate.cpp \
    cpp/libFDK/src/FDK_hybrid.cpp \
    cpp/libFDK/src/FDK_lpc.cpp \
    cpp/libFDK/src/FDK_matrixCalloc.cpp \
    cpp/libFDK/src/FDK_qmf_domain.cpp \
    cpp/libFDK/src/FDK_tools_rom.cpp \
    cpp/libFDK/src/FDK_trigFcts.cpp \
    cpp/libFDK/src/autocorr2nd.cpp \
    cpp/libFDK/src/dct.cpp \
    cpp/libFDK/src/fft.cpp \
    cpp/libFDK/src/fft_rad2.cpp \
    cpp/libFDK/src/fixpoint_math.cpp \
    cpp/libFDK/src/huff_nodes.cpp \
    cpp/libFDK/src/mdct.cpp \
    cpp/libFDK/src/nlc_dec.cpp \
    cpp/libFDK/src/qmf.cpp \
    cpp/libFDK/src/scale.cpp

MPEGTPDEC_SRC = \
    cpp/libMpegTPDec/src/tpdec_adif.cpp \
    cpp/libMpegTPDec/src/tpdec_adts.cpp \
    cpp/libMpegTPDec/src/tpdec_asc.cpp \
    cpp/libMpegTPDec/src/tpdec_drm.cpp \
    cpp/libMpegTPDec/src/tpdec_latm.cpp \
    cpp/libMpegTPDec/src/tpdec_lib.cpp

MPEGTPENC_SRC = \
    cpp/libMpegTPEnc/src/tpenc_adif.cpp \
    cpp/libMpegTPEnc/src/tpenc_adts.cpp \
    cpp/libMpegTPEnc/src/tpenc_asc.cpp \
    cpp/libMpegTPEnc/src/tpenc_latm.cpp \
    cpp/libMpegTPEnc/src/tpenc_lib.cpp

PCMUTILS_SRC = \
    cpp/libPCMutils/src/limiter.cpp \
    cpp/libPCMutils/src/pcm_utils.cpp \
    cpp/libPCMutils/src/pcmdmx_lib.cpp

SACDEC_SRC = \
    cpp/libSACdec/src/sac_bitdec.cpp \
    cpp/libSACdec/src/sac_calcM1andM2.cpp \
    cpp/libSACdec/src/sac_dec.cpp \
    cpp/libSACdec/src/sac_dec_conceal.cpp \
    cpp/libSACdec/src/sac_dec_lib.cpp \
    cpp/libSACdec/src/sac_process.cpp \
    cpp/libSACdec/src/sac_qmf.cpp \
    cpp/libSACdec/src/sac_reshapeBBEnv.cpp \
    cpp/libSACdec/src/sac_rom.cpp \
    cpp/libSACdec/src/sac_smoothing.cpp \
    cpp/libSACdec/src/sac_stp.cpp \
    cpp/libSACdec/src/sac_tsd.cpp

SACENC_SRC = \
    cpp/libSACenc/src/sacenc_bitstream.cpp \
    cpp/libSACenc/src/sacenc_delay.cpp \
    cpp/libSACenc/src/sacenc_dmx_tdom_enh.cpp \
    cpp/libSACenc/src/sacenc_filter.cpp \
    cpp/libSACenc/src/sacenc_framewindowing.cpp \
    cpp/libSACenc/src/sacenc_huff_tab.cpp \
    cpp/libSACenc/src/sacenc_lib.cpp \
    cpp/libSACenc/src/sacenc_nlc_enc.cpp \
    cpp/libSACenc/src/sacenc_onsetdetect.cpp \
    cpp/libSACenc/src/sacenc_paramextract.cpp \
    cpp/libSACenc/src/sacenc_staticgain.cpp \
    cpp/libSACenc/src/sacenc_tree.cpp \
    cpp/libSACenc/src/sacenc_vectorfunctions.cpp

SBRDEC_SRC = \
    cpp/libSBRdec/src/HFgen_preFlat.cpp \
    cpp/libSBRdec/src/env_calc.cpp \
    cpp/libSBRdec/src/env_dec.cpp \
    cpp/libSBRdec/src/env_extr.cpp \
    cpp/libSBRdec/src/hbe.cpp \
    cpp/libSBRdec/src/huff_dec.cpp \
    cpp/libSBRdec/src/lpp_tran.cpp \
    cpp/libSBRdec/src/psbitdec.cpp \
    cpp/libSBRdec/src/psdec.cpp \
    cpp/libSBRdec/src/psdec_drm.cpp \
    cpp/libSBRdec/src/psdecrom_drm.cpp \
    cpp/libSBRdec/src/pvc_dec.cpp \
    cpp/libSBRdec/src/sbr_deb.cpp \
    cpp/libSBRdec/src/sbr_dec.cpp \
    cpp/libSBRdec/src/sbr_ram.cpp \
    cpp/libSBRdec/src/sbr_rom.cpp \
    cpp/libSBRdec/src/sbrdec_drc.cpp \
    cpp/libSBRdec/src/sbrdec_freq_sca.cpp \
    cpp/libSBRdec/src/sbrdecoder.cpp

SBRENC_SRC = \
    cpp/libSBRenc/src/bit_sbr.cpp \
    cpp/libSBRenc/src/code_env.cpp \
    cpp/libSBRenc/src/env_bit.cpp \
    cpp/libSBRenc/src/env_est.cpp \
    cpp/libSBRenc/src/fram_gen.cpp \
    cpp/libSBRenc/src/invf_est.cpp \
    cpp/libSBRenc/src/mh_det.cpp \
    cpp/libSBRenc/src/nf_est.cpp \
    cpp/libSBRenc/src/ps_bitenc.cpp \
    cpp/libSBRenc/src/ps_encode.cpp \
    cpp/libSBRenc/src/ps_main.cpp \
    cpp/libSBRenc/src/resampler.cpp \
    cpp/libSBRenc/src/sbr_encoder.cpp \
    cpp/libSBRenc/src/sbr_misc.cpp \
    cpp/libSBRenc/src/sbrenc_freq_sca.cpp \
    cpp/libSBRenc/src/sbrenc_ram.cpp \
    cpp/libSBRenc/src/sbrenc_rom.cpp \
    cpp/libSBRenc/src/ton_corr.cpp \
    cpp/libSBRenc/src/tran_det.cpp

SYS_SRC = \
    cpp/libSYS/src/genericStds.cpp \
    cpp/libSYS/src/syslib_channelMapDescr.cpp

libfdk_aac_la_SOURCES = \
    $(AACDEC_SRC) $(AACENC_SRC) \
    $(ARITHCODING_SRC) \
    $(DRCDEC_SRC) \
    $(MPEGTPDEC_SRC) $(MPEGTPENC_SRC) \
    $(SACDEC_SRC) $(SACENC_SRC) \
    $(SBRDEC_SRC) $(SBRENC_SRC) \
    $(PCMUTILS_SRC) $(FDK_SRC) $(SYS_SRC)

AACFASTENC_SRC = \
    xhe-aac/aacfastenc/src/aacenc.c \
    xhe-aac/aacfastenc/src/adj_thr.c \
    xhe-aac/aacfastenc/src/band_nrg.c \
    xhe-aac/aacfastenc/src/bandwidth.c \
    xhe-aac/aacfastenc/src/bit_cnt.c \
    xhe-aac/aacfastenc/src/block_switch.c \
    xhe-aac/aacfastenc/src/cplxpred_stereo.c \
    xhe-aac/aacfastenc/src/dyn_bits.c \
    xhe-aac/aacfastenc/src/grp_data.c \
    xhe-aac/aacfastenc/src/imdct.c \
    xhe-aac/aacfastenc/src/int_dec.c \
    xhe-aac/aacfastenc/src/interface.c \
    xhe-aac/aacfastenc/src/line_pe.c \
    xhe-aac/aacfastenc/src/lpd_wrapper.c \
    xhe-aac/aacfastenc/src/ms_stereo.c \
    xhe-aac/aacfastenc/src/pre_echo_control.c \
    xhe-aac/aacfastenc/src/psy_configuration.c \
    xhe-aac/aacfastenc/src/psy_main.c \
    xhe-aac/aacfastenc/src/qc_main.c \
    xhe-aac/aacfastenc/src/quantize.c \
    xhe-aac/aacfastenc/src/realpred_stereo.c \
    xhe-aac/aacfastenc/src/sf_estim.c \
    xhe-aac/aacfastenc/src/spreading.c \
    xhe-aac/aacfastenc/src/stack_alignment.c \
    xhe-aac/aacfastenc/src/tns.c \
    xhe-aac/aacfastenc/src/transform.c \
    xhe-aac/aacfastenc/src/unified_stereo_psych.c

APPLAUSE_DETECTION_SRC = \
    xhe-aac/applauseDetectionlib/src/ApplDet_C_ifc.c \
    xhe-aac/applauseDetectionlib/src/cFeatMFCC.c \
    xhe-aac/applauseDetectionlib/src/cFeatNormalize.c \
    xhe-aac/applauseDetectionlib/src/cFeatSFM2.c \
    xhe-aac/applauseDetectionlib/src/cFeatSpecCentroid.c \
    xhe-aac/applauseDetectionlib/src/cFeatSpecFlux.c \
    xhe-aac/applauseDetectionlib/src/cFeatSpecSpread.c \
    xhe-aac/applauseDetectionlib/src/cFEX.c \
    xhe-aac/applauseDetectionlib/src/cSdNeuralNet.c \
    xhe-aac/applauseDetectionlib/src/cSpDtct.c \
    xhe-aac/applauseDetectionlib/src/paramdata.c

IIS_ARITH_ENC_SRC = \
    xhe-aac/iisArithEncLib/src/iisArithEncoder.c

IIS_AUDIO_PREROLL_SRC = \
    xhe-aac/iisAudioPrerollLib/src/iisAudioPrerollLib.c

IIS_BIT_FRAME_SRC = \
    xhe-aac/iisBitFrame/src/iisBitFrame.c

IIS_DRC_GAIN_ENC_SRC = \
    xhe-aac/iisDRCGainEncLib/src/iisDRCGainEnc_bitstreamWriter.c \
    xhe-aac/iisDRCGainEncLib/src/iisDRCGainEnc_nodeSetting.c \
    xhe-aac/iisDRCGainEncLib/src/iisDRCGainEnc_process.c \
    xhe-aac/iisDRCGainGenLib/src/iisDRCCompressor.c \
    xhe-aac/iisDRCGainGenLib/src/iisDRCGainGenerator.c \
    xhe-aac/iisDRCGainGenLib/src/iisDRCGainGeneratorProcess.c \
    xhe-aac/iisDRCGainGenLib/src/iisDRCLevelComputationBS1770.c \
    xhe-aac/iisDRCGainGenLib/src/iisDRCOfflineGain.c \
    xhe-aac/iisDRCGainGenLib/src/iisDRCRealtimeLRAControl.c

IIS_ENCODER_CONFIG_SRC = \
    xhe-aac/iisEncoderConfig/src/iisEncoderConfig.c

IIS_ENCODER_CONFIG_CT_TABLE_SRC = \
    xhe-aac/iisEncoderConfigCTable/src/iisEncoderConfigCTable.c \
    xhe-aac/iisEncoderConfigCTable/src/main_aot_42_dash_oss.c \
    xhe-aac/iisEncoderConfigCTable/src/tab_coremode.c \
    xhe-aac/iisEncoderConfigCTable/src/tab_hbe.c \
    xhe-aac/iisEncoderConfigCTable/src/tab_insamplerate.c \
    xhe-aac/iisEncoderConfigCTable/src/tab_outsamplerate_oss.c \
    xhe-aac/iisEncoderConfigCTable/src/tab_quality.c \
    xhe-aac/iisEncoderConfigCTable/src/tab_sbrpvc.c \
    xhe-aac/iisEncoderConfigCTable/src/tab_sbrratio.c \
    xhe-aac/iisEncoderConfigCTable/src/tab_sbrsignaling.c \
    xhe-aac/iisEncoderConfigCTable/src/tab_stereoconfigidx.c \
    xhe-aac/iisEncoderConfigCTable/src/tab_tsd.c

IIS_LEVELER_SRC = \
    xhe-aac/iisLevelerLib/src/iisLevelerLib.c

IIS_LINKED_LIST_SRC = \
    xhe-aac/iisLinkedList/src/iisLinkedList.c

IIS_LOUDNESS_SRC = \
    xhe-aac/iisLoudnessLib/src/loudnessLib.c

IIS_LPD_COM_SRC = \
    xhe-aac/iisLPDComLib/src/iisLPDComLib_acelp_Util.c \
    xhe-aac/iisLPDComLib/src/iisLPDComLib_lpc.c \
    xhe-aac/iisLPDComLib/src/iisLPDComLib_tcx_Tools.c \
    xhe-aac/iisLPDComLib/src/iisLPDComLib_tcx_Transform.c \
    xhe-aac/iisLPDComLib/src/iisLPDComLib_tools.c \
    xhe-aac/iisLPDComLib/src/iisLPDComLib_vq_RE8.c

IIS_LPD_ENC_SRC = \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_acelp_AdaptiveCB.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_acelp_ImprovedInnovativeCB.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_acelp_InnovativeCB.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_acelp_Main.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_arithEncWrapper.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_bpf.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_bs.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_closedLoop.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_cmConfig.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_lpc_LPCAnalysis.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_lpc_Quantize.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_main.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_tcx_Main.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_tcx_Tools.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_transitions.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_vq_Main.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib_vq_RE8.c \
    xhe-aac/iisLPDEncLib/src/iisLPDEncLib.c

IIS_PARAM_LIST_SRC = \
    xhe-aac/iisParamList/src/iisParam.c \
    xhe-aac/iisParamList/src/iisParamList.c

IIS_SIG_MAP_SRC = \
    xhe-aac/iisSigMap/src/iisSigMap.c \
    xhe-aac/iisSigMap/src/iisSigMapCommon.c \
    xhe-aac/iisSigMap/src/iisSigMapConfig.c \
    xhe-aac/iisSigMap/src/iisSigMapUsac.c

IIS_SWITCHING_DECISION_SRC = \
    xhe-aac/iisSwitchingDecision/src/iisSwitchingDecision.c

IIS_XHEAAC_ENC_SRC = \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_aac_ifc.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_asc_ifc.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_audioPreRoll.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_bitDistribution.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_common.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_delayAndBuffer.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_drc_ifc.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_drc_loudness_ifc.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_encodeFrame_ifc.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_encoderStateMachine.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_extentsionData.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_live_loudness_leveler_ifc.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_loaswrapper.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_loudness_ifc.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_lraControl.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_mpegs_bitrate_cfg.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_mpegs_cfg_expert.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_mpegs_ifc.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_rapOnDemand.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_resampler_ifc.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_sbr_ifc.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_SigMap_ifc.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_submitDrcMetadata.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_syncframe.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_updateParamList.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib_warnings.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLib.c \
    xhe-aac/iisxHEAACEncLib/src/iisxHEAACEncLibConfig.c

LIMITER_SRC = \
    xhe-aac/limiterlib/src/limiter.c

MATH_SRC = \
    xhe-aac/mathlib/src/aux_func.c \
    xhe-aac/mathlib/src/cfft.c \
    xhe-aac/mathlib/src/cpuinfo.c \
    xhe-aac/mathlib/src/fpucontrol.c \
    xhe-aac/mathlib/src/iis_dxt.c \
    xhe-aac/mathlib/src/iis_fft.c \
    xhe-aac/mathlib/src/iis_mdxt.c \
    xhe-aac/mathlib/src/iisfft.c \
    xhe-aac/mathlib/src/math_inf.c \
    xhe-aac/mathlib/src/mathlib.c

MP4_ASC_SRC = \
    xhe-aac/mp4ASC/IIS_MP4ASC_Interface/IIS_MP4ASC_Interface.c \
    xhe-aac/mp4ASC/src/audiospecificconfig.c \
    xhe-aac/mp4ASC/src/usacconfig.c

MP4_BITBUF_SRC = \
    xhe-aac/mp4bitbuf/src/bit_buf.c

MP4_BITMUX_SRC = \
    xhe-aac/mp4bitmux/src/aac/bit_aac.c \
    xhe-aac/mp4bitmux/src/aac/bit_enc.c \
    xhe-aac/mp4bitmux/src/aac/codebook.c \
    xhe-aac/mp4bitmux/src/aac/extpayload.c

MP4_LOAS_WRITE_SRC = \
    xhe-aac/mp4loaswrite/src/IIS_LATM_Interface.c \
    xhe-aac/mp4loaswrite/src/latm_interface.c

MP4_TIMEBUF_SRC = \
    xhe-aac/mp4timebuf/src/time_buffer.c

QMF_SRC = \
    xhe-aac/qmflib/src/qmflib_lowfreqfilter.c \
    xhe-aac/qmflib/src/qmflib_tables.c \
    xhe-aac/qmflib/src/qmflib.c

READONLY_BITBUF_SRC = \
    xhe-aac/readonlybitbuf/src/readonlybitbuf.c

RESAMPLE_SRC = \
    xhe-aac/resamplelib/src/polyphase/convolve.c \
    xhe-aac/resamplelib/src/polyphase/filtgen.c \
    xhe-aac/resamplelib/src/polyphase/helpfunc.c \
    xhe-aac/resamplelib/src/polyphase/kernfunc.c \
    xhe-aac/resamplelib/src/polyphase/polyfilt.c \
    xhe-aac/resamplelib/src/polyphase/polyphase.c \
    xhe-aac/resamplelib/src/polyphase/resample.c \
    xhe-aac/resamplelib/src/resamplelib.c \
    xhe-aac/resamplelib/src/smpl_resampler.c

SBR_ENC_SRC = \
    xhe-aac/sbrEnclib/src/bit_codec.c \
    xhe-aac/sbrEnclib/src/bit_sbr.c \
    xhe-aac/sbrEnclib/src/cmondata.c \
    xhe-aac/sbrEnclib/src/code_env.c \
    xhe-aac/sbrEnclib/src/env_est.c \
    xhe-aac/sbrEnclib/src/fram_gen.c \
    xhe-aac/sbrEnclib/src/freq_sca.c \
    xhe-aac/sbrEnclib/src/invf_est.c \
    xhe-aac/sbrEnclib/src/ivaltime.c \
    xhe-aac/sbrEnclib/src/mh_det.c \
    xhe-aac/sbrEnclib/src/nf_est.c \
    xhe-aac/sbrEnclib/src/sbr_main.c \
    xhe-aac/sbrEnclib/src/sbr_misc.c \
    xhe-aac/sbrEnclib/src/sib_det.c \
    xhe-aac/sbrEnclib/src/ton_corr.c \
    xhe-aac/sbrEnclib/src/tran_det.c

SPACE_ENC_SRC = \
    xhe-aac/spaceEnclib/src/sac_huff_cld.c \
    xhe-aac/spaceEnclib/src/sac_huff_icc.c \
    xhe-aac/spaceEnclib/src/sac_huff_ipd.c \
    xhe-aac/spaceEnclib/src/sac_huff_pt0.c \
    xhe-aac/spaceEnclib/src/sac_huff_reshape.c \
    xhe-aac/spaceEnclib/src/sac_nlc_enc.c \
    xhe-aac/spaceEnclib/src/sac_tonality.c \
    xhe-aac/spaceEnclib/src/sac_tsd.c \
    xhe-aac/spaceEnclib/src/space_bitstream.c \
    xhe-aac/spaceEnclib/src/space_delay.c \
    xhe-aac/spaceEnclib/src/space_filter.c \
    xhe-aac/spaceEnclib/src/space_framewindowing.c \
    xhe-aac/spaceEnclib/src/space_hybrid.c \
    xhe-aac/spaceEnclib/src/space_onsetdetect.c \
    xhe-aac/spaceEnclib/src/space_paramextract.c \
    xhe-aac/spaceEnclib/src/space_staticgain.c \
    xhe-aac/spaceEnclib/src/space_tree.c \
    xhe-aac/spaceEnclib/src/spaceEnclib.c

STAT_CLASS_SRC = \
    xhe-aac/statClassLib_C/src/statClass_decision.c \
    xhe-aac/statClassLib_C/src/statClass_deltaCep.c \
    xhe-aac/statClassLib_C/src/statClass_deltaLsf.c \
    xhe-aac/statClassLib_C/src/statClass_features.c \
    xhe-aac/statClassLib_C/src/statClass_gmm_14_7k.c \
    xhe-aac/statClassLib_C/src/statClass_gmm_16k.c \
    xhe-aac/statClassLib_C/src/statClass_gmm.c \
    xhe-aac/statClassLib_C/src/statClass_lpc.c \
    xhe-aac/statClassLib_C/src/statClass_ltpDelayVar.c \
    xhe-aac/statClassLib_C/src/statClass_mel2plp.c \
    xhe-aac/statClassLib_C/src/statClass_pauseDetection.c \
    xhe-aac/statClassLib_C/src/statClass_peakednessFreqVsTime.c \
    xhe-aac/statClassLib_C/src/statClass_pitch_ol.c \
    xhe-aac/statClassLib_C/src/statClass_preemph.c \
    xhe-aac/statClassLib_C/src/statClass_signal2mel.c \
    xhe-aac/statClassLib_C/src/statClass_speechClassification.c \
    xhe-aac/statClassLib_C/src/statClass_timeTransient.c \
    xhe-aac/statClassLib_C/src/statClass_utils.c \
    xhe-aac/statClassLib_C/src/statClasslib.c

UTIL_SRC = \
    xhe-aac/utillib/src/errorhnd.c \
    xhe-aac/utillib/src/matrixCalloc.c \
    xhe-aac/utillib/src/ngsalloc.c

WRITEONLY_BITBUF_SRC = \
    xhe-aac/writeonlybitbuf/src/writeonlybitbuf.c

XHEAAC_ENC_SRC = \
    xhe-aac/xHEAACEnc/src/xHEAACEnc.c \
    xhe-aac/xHEAACEnc/src/xHEAACEncCommon.c \
    xhe-aac/xHEAACEnc/src/xHEAACEncConfig.c \
    xhe-aac/xHEAACEnc/src/xHEAACEncExportImport.c \
    xhe-aac/xHEAACEnc/src/xHEAACEncLoudness.c \
    xhe-aac/xHEAACEnc/src/xHEAACEncMapParameters.c \
    xhe-aac/xHEAACEnc/src/xHEAACEncParameter.c \
    xhe-aac/xHEAACEnc/src/xHEAACEncWarningHandling.c

libfdk_xhe_aac_la_SOURCES = \
    $(AACFASTENC_SRC) $(APPLAUSE_DETECTION_SRC) \
    $(IIS_ARITH_ENC_SRC) $(IIS_AUDIO_PREROLL_SRC) $(IIS_BIT_FRAME_SRC) \
    $(IIS_DRC_GAIN_ENC_SRC) $(IIS_ENCODER_CONFIG_SRC) \
    $(IIS_ENCODER_CONFIG_CT_TABLE_SRC) $(IIS_LEVELER_SRC) \
    $(IIS_LINKED_LIST_SRC) $(IIS_LOUDNESS_SRC) $(IIS_LPD_COM_SRC) \
    $(IIS_LPD_ENC_SRC) $(IIS_PARAM_LIST_SRC) $(IIS_SIG_MAP_SRC) \
    $(IIS_SWITCHING_DECISION_SRC) $(IIS_XHEAAC_ENC_SRC) \
    $(LIMITER_SRC) $(MATH_SRC) \
    $(MP4_ASC_SRC) $(MP4_BITBUF_SRC) $(MP4_BITMUX_SRC) $(MP4_LOAS_WRITE_SRC) \
    $(MP4_TIMEBUF_SRC) \
    $(QMF_SRC) $(READONLY_BITBUF_SRC) $(RESAMPLE_SRC) $(SBR_ENC_SRC) \
    $(SPACE_ENC_SRC) $(STAT_CLASS_SRC) $(UTIL_SRC) $(WRITEONLY_BITBUF_SRC) \
    $(XHEAAC_ENC_SRC)

EXTRA_DIST = \
    $(top_srcdir)/.clang-format \
    $(top_srcdir)/autogen.sh \
    $(top_srcdir)/MODULE_LICENSE_FRAUNHOFER \
    $(top_srcdir)/NOTICE \
    $(top_srcdir)/OWNERS \
    $(top_srcdir)/Android.bp \
    $(top_srcdir)/fdk-aac.sym \
    $(top_srcdir)/fdk-xhe-aac.def \
    $(top_srcdir)/fdk-xhe-aac.sym \
    $(top_srcdir)/cpp/Android.bp \
    $(top_srcdir)/cpp/documentation/*.pdf \
    $(top_srcdir)/cpp/fuzzer/* \
    $(top_srcdir)/cpp/libAACdec/src/*.h \
    $(top_srcdir)/cpp/libAACdec/src/arm/*.cpp \
    $(top_srcdir)/cpp/libAACenc/src/*.h \
    $(top_srcdir)/cpp/libArithCoding/include/*.h \
    $(top_srcdir)/cpp/libDRCdec/include/*.h \
    $(top_srcdir)/cpp/libDRCdec/src/*.h \
    $(top_srcdir)/cpp/libSACdec/include/*.h \
    $(top_srcdir)/cpp/libSACdec/src/*.h \
    $(top_srcdir)/cpp/libSACenc/include/*.h \
    $(top_srcdir)/cpp/libSACenc/src/*.h \
    $(top_srcdir)/cpp/libSBRenc/src/*.h \
    $(top_srcdir)/cpp/libSBRenc/include/*.h \
    $(top_srcdir)/cpp/libSBRdec/src/*.h \
    $(top_srcdir)/cpp/libSBRdec/include/*.h \
    $(top_srcdir)/cpp/libSYS/include/*.h \
    $(top_srcdir)/cpp/libPCMutils/include/*.h \
    $(top_srcdir)/cpp/libPCMutils/src/*.h \
    $(top_srcdir)/cpp/libMpegTPEnc/include/*.h \
    $(top_srcdir)/cpp/libMpegTPEnc/src/*.h \
    $(top_srcdir)/cpp/libMpegTPDec/include/*.h \
    $(top_srcdir)/cpp/libMpegTPDec/src/*.h \
    $(top_srcdir)/cpp/libFDK/include/*.h \
    $(top_srcdir)/cpp/libFDK/include/arm/*.h \
    $(top_srcdir)/cpp/libFDK/include/mips/*.h \
    $(top_srcdir)/cpp/libFDK/include/ppc/*.h \
    $(top_srcdir)/cpp/libFDK/include/x86/*.h \
    $(top_srcdir)/cpp/libFDK/src/arm/*.cpp \
    $(top_srcdir)/cpp/libFDK/src/mips/*.cpp \
    $(top_srcdir)/rust/.cargo \
    $(top_srcdir)/rust/* \
    $(top_srcdir)/METADATA \
    $(top_srcdir)/tests/* \
    $(top_srcdir)/xhe-aac/.clang-format \
    $(top_srcdir)/xhe-aac/Android.bp \
    $(top_srcdir)/xhe-aac/NOTICE \
    $(top_srcdir)/xhe-aac/README.md \
    $(top_srcdir)/xhe-aac/*/*.h \
    $(top_srcdir)/xhe-aac/*/*/*.c \
    $(top_srcdir)/xhe-aac/*/*/*.h \
    $(top_srcdir)/xhe-aac/*/src/*/*.c \
    $(top_srcdir)/xhe-aac/*/src/*/*.h \
    $(top_srcdir)/xhe-aac/*/include/*/*.h

