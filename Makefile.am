ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = \
    -I$(top_srcdir)/libAACdec/include \
    -I$(top_srcdir)/libAACenc/include \
    -I$(top_srcdir)/libSBRdec/include \
    -I$(top_srcdir)/libSBRenc/include \
    -I$(top_srcdir)/libMpegTPDec/include \
    -I$(top_srcdir)/libMpegTPEnc/include \
    -I$(top_srcdir)/libSYS/include \
    -I$(top_srcdir)/libFDK/include \
    -I$(top_srcdir)/libPCMutils/include

AM_CXXFLAGS = -fno-exceptions -fno-rtti
libfdk_aac_la_LINK = $(LINK) $(libfdk_aac_la_LDFLAGS)
# Mention a dummy pure C file to trigger generation of the $(LINK) variable
nodist_EXTRA_libfdk_aac_la_SOURCES = dummy.c

fdk_aacincludedir = $(includedir)/fdk-aac
fdk_aacinclude_HEADERS = \
    $(top_srcdir)/libSYS/include/machine_type.h \
    $(top_srcdir)/libSYS/include/genericStds.h \
    $(top_srcdir)/libSYS/include/FDK_audio.h \
    $(top_srcdir)/libAACenc/include/aacenc_lib.h \
    $(top_srcdir)/libAACdec/include/aacdecoder_lib.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = fdk-aac.pc

lib_LTLIBRARIES = libfdk-aac.la

libfdk_aac_la_LDFLAGS = -version-info @FDK_AAC_VERSION@ -no-undefined \
    -export-symbols $(top_srcdir)/fdk-aac.sym

AACDEC_DIR = $(top_srcdir)/libAACdec/src
AACDEC_SRC = \
    $(AACDEC_DIR)/aacdec_drc.cpp \
    $(AACDEC_DIR)/aacdec_hcr.cpp \
    $(AACDEC_DIR)/aacdecoder.cpp \
    $(AACDEC_DIR)/aacdec_pns.cpp \
    $(AACDEC_DIR)/aac_ram.cpp \
    $(AACDEC_DIR)/block.cpp \
    $(AACDEC_DIR)/channelinfo.cpp \
    $(AACDEC_DIR)/ldfiltbank.cpp \
    $(AACDEC_DIR)/rvlcbit.cpp \
    $(AACDEC_DIR)/rvlc.cpp \
    $(AACDEC_DIR)/aacdec_hcr_bit.cpp \
    $(AACDEC_DIR)/aacdec_hcrs.cpp \
    $(AACDEC_DIR)/aacdecoder_lib.cpp \
    $(AACDEC_DIR)/aacdec_tns.cpp \
    $(AACDEC_DIR)/aac_rom.cpp \
    $(AACDEC_DIR)/channel.cpp \
    $(AACDEC_DIR)/conceal.cpp \
    $(AACDEC_DIR)/pulsedata.cpp \
    $(AACDEC_DIR)/rvlcconceal.cpp \
    $(AACDEC_DIR)/stereo.cpp

AACENC_DIR = $(top_srcdir)/libAACenc/src
AACENC_SRC = \
    $(AACENC_DIR)/aacenc.cpp \
    $(AACENC_DIR)/aacEnc_ram.cpp \
    $(AACENC_DIR)/band_nrg.cpp \
    $(AACENC_DIR)/block_switch.cpp \
    $(AACENC_DIR)/grp_data.cpp \
    $(AACENC_DIR)/metadata_main.cpp \
    $(AACENC_DIR)/pre_echo_control.cpp \
    $(AACENC_DIR)/quantize.cpp \
    $(AACENC_DIR)/tonality.cpp \
    $(AACENC_DIR)/aacenc_hcr.cpp \
    $(AACENC_DIR)/aacEnc_rom.cpp \
    $(AACENC_DIR)/bandwidth.cpp \
    $(AACENC_DIR)/channel_map.cpp \
    $(AACENC_DIR)/intensity.cpp \
    $(AACENC_DIR)/ms_stereo.cpp \
    $(AACENC_DIR)/psy_configuration.cpp \
    $(AACENC_DIR)/sf_estim.cpp \
    $(AACENC_DIR)/transform.cpp \
    $(AACENC_DIR)/aacenc_lib.cpp \
    $(AACENC_DIR)/aacenc_tns.cpp \
    $(AACENC_DIR)/bit_cnt.cpp \
    $(AACENC_DIR)/chaosmeasure.cpp \
    $(AACENC_DIR)/line_pe.cpp \
    $(AACENC_DIR)/noisedet.cpp \
    $(AACENC_DIR)/psy_main.cpp \
    $(AACENC_DIR)/spreading.cpp \
    $(AACENC_DIR)/aacenc_pns.cpp \
    $(AACENC_DIR)/adj_thr.cpp \
    $(AACENC_DIR)/bitenc.cpp \
    $(AACENC_DIR)/dyn_bits.cpp \
    $(AACENC_DIR)/metadata_compressor.cpp \
    $(AACENC_DIR)/pnsparam.cpp \
    $(AACENC_DIR)/qc_main.cpp \
    $(AACENC_DIR)/tns_param.cpp

FDK_DIR = $(top_srcdir)/libFDK/src
FDK_SRC = \
    $(FDK_DIR)/autocorr2nd.cpp \
    $(FDK_DIR)/dct.cpp \
    $(FDK_DIR)/FDK_bitbuffer.cpp \
    $(FDK_DIR)/FDK_core.cpp \
    $(FDK_DIR)/FDK_crc.cpp \
    $(FDK_DIR)/FDK_hybrid.cpp \
    $(FDK_DIR)/FDK_tools_rom.cpp \
    $(FDK_DIR)/FDK_trigFcts.cpp \
    $(FDK_DIR)/fft.cpp \
    $(FDK_DIR)/fft_rad2.cpp \
    $(FDK_DIR)/fixpoint_math.cpp \
    $(FDK_DIR)/mdct.cpp \
    $(FDK_DIR)/qmf.cpp \
    $(FDK_DIR)/scale.cpp

MPEGTPDEC_DIR = $(top_srcdir)/libMpegTPDec/src
MPEGTPDEC_SRC = \
    $(MPEGTPDEC_DIR)/tpdec_adif.cpp \
    $(MPEGTPDEC_DIR)/tpdec_adts.cpp \
    $(MPEGTPDEC_DIR)/tpdec_asc.cpp \
    $(MPEGTPDEC_DIR)/tpdec_latm.cpp \
    $(MPEGTPDEC_DIR)/tpdec_lib.cpp

MPEGTPENC_DIR = $(top_srcdir)/libMpegTPEnc/src
MPEGTPENC_SRC = \
    $(MPEGTPENC_DIR)/tpenc_adif.cpp \
    $(MPEGTPENC_DIR)/tpenc_adts.cpp \
    $(MPEGTPENC_DIR)/tpenc_asc.cpp \
    $(MPEGTPENC_DIR)/tpenc_latm.cpp \
    $(MPEGTPENC_DIR)/tpenc_lib.cpp

PCMUTILS_DIR = $(top_srcdir)/libPCMutils/src
PCMUTILS_SRC = \
    $(PCMUTILS_DIR)/pcmutils_lib.cpp

SBRDEC_DIR = $(top_srcdir)/libSBRdec/src
SBRDEC_SRC = \
    $(SBRDEC_DIR)/env_calc.cpp \
    $(SBRDEC_DIR)/env_dec.cpp \
    $(SBRDEC_DIR)/env_extr.cpp \
    $(SBRDEC_DIR)/huff_dec.cpp \
    $(SBRDEC_DIR)/lpp_tran.cpp \
    $(SBRDEC_DIR)/psbitdec.cpp \
    $(SBRDEC_DIR)/psdec.cpp \
    $(SBRDEC_DIR)/psdec_hybrid.cpp \
    $(SBRDEC_DIR)/sbr_crc.cpp \
    $(SBRDEC_DIR)/sbr_deb.cpp \
    $(SBRDEC_DIR)/sbr_dec.cpp \
    $(SBRDEC_DIR)/sbrdec_drc.cpp \
    $(SBRDEC_DIR)/sbrdec_freq_sca.cpp \
    $(SBRDEC_DIR)/sbrdecoder.cpp \
    $(SBRDEC_DIR)/sbr_ram.cpp \
    $(SBRDEC_DIR)/sbr_rom.cpp

SBRENC_DIR = $(top_srcdir)/libSBRenc/src
SBRENC_SRC = \
    $(SBRENC_DIR)/bit_sbr.cpp \
    $(SBRENC_DIR)/env_bit.cpp \
    $(SBRENC_DIR)/fram_gen.cpp \
    $(SBRENC_DIR)/mh_det.cpp \
    $(SBRENC_DIR)/ps_bitenc.cpp \
    $(SBRENC_DIR)/ps_encode.cpp \
    $(SBRENC_DIR)/resampler.cpp \
    $(SBRENC_DIR)/sbr_encoder.cpp \
    $(SBRENC_DIR)/sbr_ram_enc.cpp \
    $(SBRENC_DIR)/ton_corr.cpp \
    $(SBRENC_DIR)/code_env.cpp \
    $(SBRENC_DIR)/env_est.cpp \
    $(SBRENC_DIR)/invf_est.cpp \
    $(SBRENC_DIR)/nf_est.cpp \
    $(SBRENC_DIR)/ps_main.cpp \
    $(SBRENC_DIR)/sbrenc_freq_sca.cpp \
    $(SBRENC_DIR)/sbr_misc.cpp \
    $(SBRENC_DIR)/sbr_rom_enc.cpp \
    $(SBRENC_DIR)/tran_det.cpp

SYS_DIR = $(top_srcdir)/libSYS/src
SYS_SRC = \
    $(SYS_DIR)/cmdl_parser.cpp \
    $(SYS_DIR)/conv_string.cpp \
    $(SYS_DIR)/genericStds.cpp \
    $(SYS_DIR)/wav_file.cpp

libfdk_aac_la_SOURCES = \
    $(AACDEC_SRC) $(AACENC_SRC) \
    $(MPEGTPDEC_SRC) $(MPEGTPENC_SRC) \
    $(SBRDEC_SRC) $(SBRENC_SRC) \
    $(PCMUTILS_SRC) $(FDK_SRC) $(SYS_SRC)

EXTRA_DIST = \
    $(top_srcdir)/NOTICE \
    $(top_srcdir)/Android.mk \
    $(top_srcdir)/fdk-aac.sym \
    $(top_srcdir)/documentation/*.pdf \
    $(top_srcdir)/libAACdec/src/*.h \
    $(top_srcdir)/libAACdec/src/arm/*.cpp \
    $(top_srcdir)/libAACenc/src/*.h \
    $(top_srcdir)/libSBRenc/src/*.h \
    $(top_srcdir)/libSBRenc/include/*.h \
    $(top_srcdir)/libSBRdec/src/*.h \
    $(top_srcdir)/libSBRdec/src/arm/*.cpp \
    $(top_srcdir)/libSBRdec/include/*.h \
    $(top_srcdir)/libSYS/include/*.h \
    $(top_srcdir)/libSYS/src/linux/*.cpp \
    $(top_srcdir)/libSYS/src/mips/*.cpp \
    $(top_srcdir)/libPCMutils/include/*.h \
    $(top_srcdir)/libMpegTPEnc/include/*.h \
    $(top_srcdir)/libMpegTPEnc/src/*.h \
    $(top_srcdir)/libMpegTPEnc/src/version \
    $(top_srcdir)/libMpegTPDec/include/*.h \
    $(top_srcdir)/libMpegTPDec/src/*.h \
    $(top_srcdir)/libMpegTPDec/src/version \
    $(top_srcdir)/libFDK/include/*.h \
    $(top_srcdir)/libFDK/include/arm/*.h \
    $(top_srcdir)/libFDK/include/mips/*.h \
    $(top_srcdir)/libFDK/include/x86/*.h \
    $(top_srcdir)/libFDK/src/arm/*.cpp \
    $(top_srcdir)/libFDK/src/mips/*.cpp

