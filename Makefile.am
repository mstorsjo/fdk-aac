ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = \
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

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = fdk-aac.pc

lib_LTLIBRARIES = libfdk-aac.la

libfdk_aac_la_LDFLAGS = -version-info @FDK_AAC_VERSION@ -no-undefined \
    -export-symbols $(top_srcdir)/fdk-aac.sym

if EXAMPLE
bin_PROGRAMS = aac-enc$(EXEEXT)
noinst_PROGRAMS = test-encode-decode$(EXEEXT)

aac_enc_LDADD = libfdk-aac.la
aac_enc_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/cpp
aac_enc_SOURCES = aac-enc.c wavreader.c

test_encode_decode_LDADD = libfdk-aac.la
test_encode_decode_SOURCES = test-encode-decode.c wavreader.c sha1.c

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

EXTRA_DIST = \
    $(top_srcdir)/CMakeLists.txt \
    $(top_srcdir)/.clang-format \
    $(top_srcdir)/autogen.sh \
    $(top_srcdir)/MODULE_LICENSE_FRAUNHOFER \
    $(top_srcdir)/NOTICE \
    $(top_srcdir)/OWNERS \
    $(top_srcdir)/Android.bp \
    $(top_srcdir)/fdk-aac.def \
    $(top_srcdir)/fdk-aac.sym \
    $(top_srcdir)/fdk-aac-config.cmake.in \
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
    $(top_srcdir)/sha1.h \
    $(top_srcdir)/test/* \
    $(top_srcdir)/tests/* \
    $(top_srcdir)/win32/*.h

