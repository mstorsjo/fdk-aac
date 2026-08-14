# # CMake project for fdk-aac
#
# ## Basic usage
#
# 1. Download and install CMake >= 3.10 (CMake >= 3.15 is recommended)
# 2. Clone fdk-aac repository
# 3. In fdk-aac directory create and go to `build` subdirectory:
#
#    mkdir build
#    cd build
#
# 4. Run cmake to configure project with desired build type (Release|Debug):
#
#    cmake .. -DCMAKE_BUILD_TYPE=Release
#
#    Note CMake configuration without CMAKE_BUILD_TYPE option will not set build
#    type to some default and it is most likely is not what you want.
#
#    If you want to build static (default is shared library), add BUILD_SHARED_LIBS=OFF option:
#
#    #    cmake .. -DBUILD_SHARED_LIBS=OFF
#
#    You can combine options as well:
#
#    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF
#
# 5. Run cmake to build project:
#
#    cmake --build . -c Release

cmake_minimum_required(VERSION 3.5.1)

# Policies

## Enables CMAKE_MSVC_RUNTIME_LIBRARY option support for CMake >= 3.15
## if you want to use a MSVC multi-threaded statically-linked runtime library
## If you enable it, CMake will build fdk-acc.dll without external dependencies.
##
## Example usage:
##
## cmake .. -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>
if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()

project(fdk-aac VERSION 2.0.3)

# Includes

include(CheckSymbolExists)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_VERSION VERSION_LESS 3.11)
  set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
endif()
include(CPack)

# Options

option(BUILD_SHARED_LIBS "Build shared library" ON)
option(BUILD_PROGRAMS "Build extra utilities" OFF)
option(FDK_AAC_INSTALL_CMAKE_CONFIG_MODULE "Install CMake package configuration file" ON)
option(FDK_AAC_INSTALL_PKGCONFIG_MODULE "Install pkg-config .pc file" ON)

# Checks

## Check if math functions are in separate library (Most of Linux distros, maybe some other OSes)
check_symbol_exists(sin "math.h" HAVE_DEFAULT_MATH)
if(NOT HAVE_DEFAULT_MATH)
  set(orig_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
  list(APPEND CMAKE_REQUIRED_LIBRARIES "m")
  check_symbol_exists(sin "math.h" HAVE_LIBM)
  set(CMAKE_REQUIRED_LIBRARIES ${orig_CMAKE_REQUIRED_LIBRARIES})
endif()

# Library

## Sources

set(AACDEC_SRC
  libAACdec/src/FDK_delay.cpp
  libAACdec/src/aac_ram.cpp
  libAACdec/src/aac_rom.cpp
  libAACdec/src/aacdec_drc.cpp
  libAACdec/src/aacdec_hcr.cpp
  libAACdec/src/aacdec_hcr_bit.cpp
  libAACdec/src/aacdec_hcrs.cpp
  libAACdec/src/aacdec_pns.cpp
  libAACdec/src/aacdec_tns.cpp
  libAACdec/src/aacdecoder.cpp
  libAACdec/src/aacdecoder_lib.cpp
  libAACdec/src/block.cpp
  libAACdec/src/channel.cpp
  libAACdec/src/channelinfo.cpp
  libAACdec/src/conceal.cpp
  libAACdec/src/ldfiltbank.cpp
  libAACdec/src/pulsedata.cpp
  libAACdec/src/rvlc.cpp
  libAACdec/src/rvlcbit.cpp
  libAACdec/src/rvlcconceal.cpp
  libAACdec/src/stereo.cpp
  libAACdec/src/usacdec_ace_d4t64.cpp
  libAACdec/src/usacdec_ace_ltp.cpp
  libAACdec/src/usacdec_acelp.cpp
  libAACdec/src/usacdec_fac.cpp
  libAACdec/src/usacdec_lpc.cpp
  libAACdec/src/usacdec_lpd.cpp
  libAACdec/src/usacdec_rom.cpp
  libAACdec/src/aac_ram.h
  libAACdec/src/aac_rom.h
  libAACdec/src/aacdec_drc.h
  libAACdec/src/aacdec_drc_types.h
  libAACdec/src/aacdec_hcr.h
  libAACdec/src/aacdec_hcr_bit.h
  libAACdec/src/aacdec_hcr_types.h
  libAACdec/src/aacdec_hcrs.h
  libAACdec/src/aacdec_pns.h
  libAACdec/src/aacdec_tns.h
  libAACdec/src/aacdecoder.h
  libAACdec/src/block.h
  libAACdec/src/channel.h
  libAACdec/src/channelinfo.h
  libAACdec/src/conceal.h
  libAACdec/src/conceal_types.h
  libAACdec/src/FDK_delay.h
  libAACdec/src/ldfiltbank.h
  libAACdec/src/overlapadd.h
  libAACdec/src/pulsedata.h
  libAACdec/src/rvlc.h
  libAACdec/src/rvlc_info.h
  libAACdec/src/rvlcbit.h
  libAACdec/src/rvlcconceal.h
  libAACdec/src/stereo.h
  libAACdec/src/usacdec_ace_d4t64.h
  libAACdec/src/usacdec_ace_ltp.h
  libAACdec/src/usacdec_acelp.h
  libAACdec/src/usacdec_const.h
  libAACdec/src/usacdec_fac.h
  libAACdec/src/usacdec_lpc.h
  libAACdec/src/usacdec_lpd.h
  libAACdec/src/usacdec_rom.h)

set(AACENC_SRC
  libAACenc/src/aacEnc_ram.cpp
  libAACenc/src/aacEnc_rom.cpp
  libAACenc/src/aacenc.cpp
  libAACenc/src/aacenc_lib.cpp
  libAACenc/src/aacenc_pns.cpp
  libAACenc/src/aacenc_tns.cpp
  libAACenc/src/adj_thr.cpp
  libAACenc/src/band_nrg.cpp
  libAACenc/src/bandwidth.cpp
  libAACenc/src/bit_cnt.cpp
  libAACenc/src/bitenc.cpp
  libAACenc/src/block_switch.cpp
  libAACenc/src/channel_map.cpp
  libAACenc/src/chaosmeasure.cpp
  libAACenc/src/dyn_bits.cpp
  libAACenc/src/grp_data.cpp
  libAACenc/src/intensity.cpp
  libAACenc/src/line_pe.cpp
  libAACenc/src/metadata_compressor.cpp
  libAACenc/src/metadata_main.cpp
  libAACenc/src/mps_main.cpp
  libAACenc/src/ms_stereo.cpp
  libAACenc/src/noisedet.cpp
  libAACenc/src/pnsparam.cpp
  libAACenc/src/pre_echo_control.cpp
  libAACenc/src/psy_configuration.cpp
  libAACenc/src/psy_main.cpp
  libAACenc/src/qc_main.cpp
  libAACenc/src/quantize.cpp
  libAACenc/src/sf_estim.cpp
  libAACenc/src/spreading.cpp
  libAACenc/src/tonality.cpp
  libAACenc/src/transform.cpp
  libAACenc/src/aacenc.h
  libAACenc/src/aacenc_pns.h
  libAACenc/src/aacEnc_ram.h
  libAACenc/src/aacEnc_rom.h
  libAACenc/src/aacenc_tns.h
  libAACenc/src/adj_thr.h
  libAACenc/src/adj_thr_data.h
  libAACenc/src/band_nrg.h
  libAACenc/src/bandwidth.h
  libAACenc/src/bit_cnt.h
  libAACenc/src/bitenc.h
  libAACenc/src/block_switch.h
  libAACenc/src/channel_map.h
  libAACenc/src/chaosmeasure.h
  libAACenc/src/dyn_bits.h
  libAACenc/src/grp_data.h
  libAACenc/src/intensity.h
  libAACenc/src/interface.h
  libAACenc/src/line_pe.h
  libAACenc/src/metadata_compressor.h
  libAACenc/src/metadata_main.h
  libAACenc/src/mps_main.h
  libAACenc/src/ms_stereo.h
  libAACenc/src/noisedet.h
  libAACenc/src/pns_func.h
  libAACenc/src/pnsparam.h
  libAACenc/src/pre_echo_control.h
  libAACenc/src/psy_configuration.h
  libAACenc/src/psy_const.h
  libAACenc/src/psy_data.h
  libAACenc/src/psy_main.h
  libAACenc/src/qc_data.h
  libAACenc/src/qc_main.h
  libAACenc/src/quantize.h
  libAACenc/src/sf_estim.h
  libAACenc/src/spreading.h
  libAACenc/src/tns_func.h
  libAACenc/src/tonality.h
  libAACenc/src/transform.h)

set(ARITHCODING_SRC
  libArithCoding/src/ac_arith_coder.cpp)

set(DRCDEC_SRC
  libDRCdec/src/FDK_drcDecLib.cpp
  libDRCdec/src/drcDec_gainDecoder.cpp
  libDRCdec/src/drcDec_reader.cpp
  libDRCdec/src/drcDec_rom.cpp
  libDRCdec/src/drcDec_selectionProcess.cpp
  libDRCdec/src/drcDec_tools.cpp
  libDRCdec/src/drcGainDec_init.cpp
  libDRCdec/src/drcGainDec_preprocess.cpp
  libDRCdec/src/drcGainDec_process.cpp
  libDRCdec/src/drcDec_gainDecoder.h
  libDRCdec/src/drcDec_reader.h
  libDRCdec/src/drcDec_rom.h
  libDRCdec/src/drcDec_selectionProcess.h
  libDRCdec/src/drcDec_tools.h
  libDRCdec/src/drcDec_types.h
  libDRCdec/src/drcDecoder.h
  libDRCdec/src/drcGainDec_init.h
  libDRCdec/src/drcGainDec_preprocess.h
  libDRCdec/src/drcGainDec_process.h)

set(FDK_SRC
  libFDK/src/FDK_bitbuffer.cpp
  libFDK/src/FDK_core.cpp
  libFDK/src/FDK_crc.cpp
  libFDK/src/FDK_decorrelate.cpp
  libFDK/src/FDK_hybrid.cpp
  libFDK/src/FDK_lpc.cpp
  libFDK/src/FDK_matrixCalloc.cpp
  libFDK/src/FDK_qmf_domain.cpp
  libFDK/src/FDK_tools_rom.cpp
  libFDK/src/FDK_trigFcts.cpp
  libFDK/src/autocorr2nd.cpp
  libFDK/src/dct.cpp
  libFDK/src/fft.cpp
  libFDK/src/fft_rad2.cpp
  libFDK/src/fixpoint_math.cpp
  libFDK/src/huff_nodes.cpp
  libFDK/src/mdct.cpp
  libFDK/src/nlc_dec.cpp
  libFDK/src/qmf.cpp
  libFDK/src/scale.cpp)

set(MPEGTPDEC_SRC
  libMpegTPDec/src/tpdec_adif.cpp
  libMpegTPDec/src/tpdec_adts.cpp
  libMpegTPDec/src/tpdec_asc.cpp
  libMpegTPDec/src/tpdec_drm.cpp
  libMpegTPDec/src/tpdec_latm.cpp
  libMpegTPDec/src/tpdec_lib.cpp
  libMpegTPDec/src/tp_version.h
  libMpegTPDec/src/tpdec_adif.h
  libMpegTPDec/src/tpdec_adts.h
  libMpegTPDec/src/tpdec_drm.h
  libMpegTPDec/src/tpdec_latm.h)

set(MPEGTPENC_SRC
  libMpegTPEnc/src/tpenc_adif.cpp
  libMpegTPEnc/src/tpenc_adts.cpp
  libMpegTPEnc/src/tpenc_asc.cpp
  libMpegTPEnc/src/tpenc_latm.cpp
  libMpegTPEnc/src/tpenc_lib.cpp
  libMpegTPEnc/src/tp_version.h
  libMpegTPEnc/src/tpenc_adif.h
  libMpegTPEnc/src/tpenc_adts.h
  libMpegTPEnc/src/tpenc_asc.h
  libMpegTPEnc/src/tpenc_latm.h)

set(PCMUTILS_SRC
  libPCMutils/src/limiter.cpp
  libPCMutils/src/pcm_utils.cpp
  libPCMutils/src/pcmdmx_lib.cpp
  libPCMutils/src/version.h)

set(SACDEC_SRC
  libSACdec/src/sac_bitdec.cpp
  libSACdec/src/sac_calcM1andM2.cpp
  libSACdec/src/sac_dec.cpp
  libSACdec/src/sac_dec_conceal.cpp
  libSACdec/src/sac_dec_lib.cpp
  libSACdec/src/sac_process.cpp
  libSACdec/src/sac_qmf.cpp
  libSACdec/src/sac_reshapeBBEnv.cpp
  libSACdec/src/sac_rom.cpp
  libSACdec/src/sac_smoothing.cpp
  libSACdec/src/sac_stp.cpp
  libSACdec/src/sac_tsd.cpp
  libSACdec/src/sac_bitdec.h
  libSACdec/src/sac_calcM1andM2.h
  libSACdec/src/sac_dec.h
  libSACdec/src/sac_dec_conceal.h
  libSACdec/src/sac_dec_interface.h
  libSACdec/src/sac_dec_ssc_struct.h
  libSACdec/src/sac_process.h
  libSACdec/src/sac_qmf.h
  libSACdec/src/sac_reshapeBBEnv.h
  libSACdec/src/sac_rom.h
  libSACdec/src/sac_smoothing.h
  libSACdec/src/sac_stp.h
  libSACdec/src/sac_tsd.h)

set(SACENC_SRC
  libSACenc/src/sacenc_bitstream.cpp
  libSACenc/src/sacenc_delay.cpp
  libSACenc/src/sacenc_dmx_tdom_enh.cpp
  libSACenc/src/sacenc_filter.cpp
  libSACenc/src/sacenc_framewindowing.cpp
  libSACenc/src/sacenc_huff_tab.cpp
  libSACenc/src/sacenc_lib.cpp
  libSACenc/src/sacenc_nlc_enc.cpp
  libSACenc/src/sacenc_onsetdetect.cpp
  libSACenc/src/sacenc_paramextract.cpp
  libSACenc/src/sacenc_staticgain.cpp
  libSACenc/src/sacenc_tree.cpp
  libSACenc/src/sacenc_vectorfunctions.cpp
  libSACenc/src/sacenc_bitstream.h
  libSACenc/src/sacenc_const.h
  libSACenc/src/sacenc_delay.h
  libSACenc/src/sacenc_dmx_tdom_enh.h
  libSACenc/src/sacenc_filter.h
  libSACenc/src/sacenc_framewindowing.h
  libSACenc/src/sacenc_huff_tab.h
  libSACenc/src/sacenc_nlc_enc.h
  libSACenc/src/sacenc_onsetdetect.h
  libSACenc/src/sacenc_paramextract.h
  libSACenc/src/sacenc_staticgain.h
  libSACenc/src/sacenc_tree.h
  libSACenc/src/sacenc_vectorfunctions.h)

set(SBRDEC_SRC
  libSBRdec/src/HFgen_preFlat.cpp
  libSBRdec/src/env_calc.cpp
  libSBRdec/src/env_dec.cpp
  libSBRdec/src/env_extr.cpp
  libSBRdec/src/hbe.cpp
  libSBRdec/src/huff_dec.cpp
  libSBRdec/src/lpp_tran.cpp
  libSBRdec/src/psbitdec.cpp
  libSBRdec/src/psdec.cpp
  libSBRdec/src/psdec_drm.cpp
  libSBRdec/src/psdecrom_drm.cpp
  libSBRdec/src/pvc_dec.cpp
  libSBRdec/src/sbr_deb.cpp
  libSBRdec/src/sbr_dec.cpp
  libSBRdec/src/sbr_ram.cpp
  libSBRdec/src/sbr_rom.cpp
  libSBRdec/src/sbrdec_drc.cpp
  libSBRdec/src/sbrdec_freq_sca.cpp
  libSBRdec/src/sbrdecoder.cpp
  libSBRdec/src/env_calc.h
  libSBRdec/src/env_dec.h
  libSBRdec/src/env_extr.h
  libSBRdec/src/hbe.h
  libSBRdec/src/HFgen_preFlat.h
  libSBRdec/src/huff_dec.h
  libSBRdec/src/lpp_tran.h
  libSBRdec/src/psbitdec.h
  libSBRdec/src/psdec.h
  libSBRdec/src/psdec_drm.h
  libSBRdec/src/pvc_dec.h
  libSBRdec/src/sbr_deb.h
  libSBRdec/src/sbr_dec.h
  libSBRdec/src/sbr_ram.h
  libSBRdec/src/sbr_rom.h
  libSBRdec/src/sbrdec_drc.h
  libSBRdec/src/sbrdec_freq_sca.h
  libSBRdec/src/transcendent.h)

set(SBRENC_SRC
  libSBRenc/src/bit_sbr.cpp
  libSBRenc/src/code_env.cpp
  libSBRenc/src/env_bit.cpp
  libSBRenc/src/env_est.cpp
  libSBRenc/src/fram_gen.cpp
  libSBRenc/src/invf_est.cpp
  libSBRenc/src/mh_det.cpp
  libSBRenc/src/nf_est.cpp
  libSBRenc/src/ps_bitenc.cpp
  libSBRenc/src/ps_encode.cpp
  libSBRenc/src/ps_main.cpp
  libSBRenc/src/resampler.cpp
  libSBRenc/src/sbr_encoder.cpp
  libSBRenc/src/sbr_misc.cpp
  libSBRenc/src/sbrenc_freq_sca.cpp
  libSBRenc/src/sbrenc_ram.cpp
  libSBRenc/src/sbrenc_rom.cpp
  libSBRenc/src/ton_corr.cpp
  libSBRenc/src/tran_det.cpp
  libSBRenc/src/bit_sbr.h
  libSBRenc/src/cmondata.h
  libSBRenc/src/code_env.h
  libSBRenc/src/env_bit.h
  libSBRenc/src/env_est.h
  libSBRenc/src/fram_gen.h
  libSBRenc/src/invf_est.h
  libSBRenc/src/mh_det.h
  libSBRenc/src/nf_est.h
  libSBRenc/src/ps_bitenc.h
  libSBRenc/src/ps_const.h
  libSBRenc/src/ps_encode.h
  libSBRenc/src/ps_main.h
  libSBRenc/src/resampler.h
  libSBRenc/src/sbr.h
  libSBRenc/src/sbr_def.h
  libSBRenc/src/sbr_misc.h
  libSBRenc/src/sbrenc_freq_sca.h
  libSBRenc/src/sbrenc_ram.h
  libSBRenc/src/sbrenc_rom.h
  libSBRenc/src/ton_corr.h
  libSBRenc/src/tran_det.h)

set(SYS_SRC
  libSYS/src/genericStds.cpp
  libSYS/src/syslib_channelMapDescr.cpp)

set(fdk_aacinclude_HEADERS
  libSYS/include/machine_type.h
  libSYS/include/genericStds.h
  libSYS/include/FDK_audio.h
  libSYS/include/syslib_channelMapDescr.h
  libAACenc/include/aacenc_lib.h
  libAACdec/include/aacdecoder_lib.h)

set(libfdk_aac_SOURCES
    ${AACDEC_SRC} ${AACENC_SRC}
    ${ARITHCODING_SRC}
    ${DRCDEC_SRC}
    ${MPEGTPDEC_SRC} ${MPEGTPENC_SRC}
    ${SACDEC_SRC} ${SACENC_SRC}
    ${SBRDEC_SRC} ${SBRENC_SRC}
    ${PCMUTILS_SRC} ${FDK_SRC} ${SYS_SRC})


set(AACFASTENC_SRC
  aacfastenc/src/aacenc.c
  aacfastenc/src/adj_thr.c
  aacfastenc/src/band_nrg.c
  aacfastenc/src/bandwidth.c
  aacfastenc/src/bit_cnt.c
  aacfastenc/src/block_switch.c
  aacfastenc/src/cplxpred_stereo.c
  aacfastenc/src/dyn_bits.c
  aacfastenc/src/grp_data.c
  aacfastenc/src/imdct.c
  aacfastenc/src/int_dec.c
  aacfastenc/src/interface.c
  aacfastenc/src/line_pe.c
  aacfastenc/src/lpd_wrapper.c
  aacfastenc/src/ms_stereo.c
  aacfastenc/src/pre_echo_control.c
  aacfastenc/src/psy_configuration.c
  aacfastenc/src/psy_main.c
  aacfastenc/src/qc_main.c
  aacfastenc/src/quantize.c
  aacfastenc/src/realpred_stereo.c
  aacfastenc/src/sf_estim.c
  aacfastenc/src/spreading.c
  aacfastenc/src/stack_alignment.c
  aacfastenc/src/tns.c
  aacfastenc/src/transform.c
  aacfastenc/src/unified_stereo_psych.c)

set(APPLAUSE_DETECTION_SRC
  applauseDetectionlib/src/ApplDet_C_ifc.c
  applauseDetectionlib/src/cFeatMFCC.c
  applauseDetectionlib/src/cFeatNormalize.c
  applauseDetectionlib/src/cFeatSFM2.c
  applauseDetectionlib/src/cFeatSpecCentroid.c
  applauseDetectionlib/src/cFeatSpecFlux.c
  applauseDetectionlib/src/cFeatSpecSpread.c
  applauseDetectionlib/src/cFEX.c
  applauseDetectionlib/src/cSdNeuralNet.c
  applauseDetectionlib/src/cSpDtct.c
  applauseDetectionlib/src/paramdata.c)

set(IIS_ARITH_ENC_SRC
  iisArithEncLib/src/iisArithEncoder.c)

set(IIS_AUDIO_PREROLL_SRC
  iisAudioPrerollLib/src/iisAudioPrerollLib.c)

set(IIS_BIT_FRAME_SRC
  iisBitFrame/src/iisBitFrame.c)

set(IIS_DRC_GAIN_ENC_SRC
  iisDRCGainEncLib/src/iisDRCGainEnc_bitstreamWriter.c
  iisDRCGainEncLib/src/iisDRCGainEnc_nodeSetting.c
  iisDRCGainEncLib/src/iisDRCGainEnc_process.c
  iisDRCGainGenLib/src/iisDRCCompressor.c
  iisDRCGainGenLib/src/iisDRCGainGenerator.c
  iisDRCGainGenLib/src/iisDRCGainGeneratorProcess.c
  iisDRCGainGenLib/src/iisDRCLevelComputationBS1770.c
  iisDRCGainGenLib/src/iisDRCOfflineGain.c
  iisDRCGainGenLib/src/iisDRCRealtimeLRAControl.c)

set(IIS_ENCODER_CONFIG_SRC
  iisEncoderConfig/src/iisEncoderConfig.c)

set(IIS_ENCODER_CONFIG_CT_TABLE_SRC
  iisEncoderConfigCTable/src/iisEncoderConfigCTable.c
  iisEncoderConfigCTable/src/main_aot_42_dash_oss.c
  iisEncoderConfigCTable/src/tab_coremode.c
  iisEncoderConfigCTable/src/tab_hbe.c
  iisEncoderConfigCTable/src/tab_insamplerate.c
  iisEncoderConfigCTable/src/tab_outsamplerate_oss.c
  iisEncoderConfigCTable/src/tab_quality.c
  iisEncoderConfigCTable/src/tab_sbrpvc.c
  iisEncoderConfigCTable/src/tab_sbrratio.c
  iisEncoderConfigCTable/src/tab_sbrsignaling.c
  iisEncoderConfigCTable/src/tab_stereoconfigidx.c
  iisEncoderConfigCTable/src/tab_tsd.c)

set(IIS_LEVELER_SRC
  iisLevelerLib/src/iisLevelerLib.c)

set(IIS_LINKED_LIST_SRC
  iisLinkedList/src/iisLinkedList.c)

set(IIS_LOUDNESS_SRC
  iisLoudnessLib/src/loudnessLib.c)

set(IIS_LPD_COM_SRC
  iisLPDComLib/src/iisLPDComLib_acelp_Util.c
  iisLPDComLib/src/iisLPDComLib_lpc.c
  iisLPDComLib/src/iisLPDComLib_tcx_Tools.c
  iisLPDComLib/src/iisLPDComLib_tcx_Transform.c
  iisLPDComLib/src/iisLPDComLib_tools.c
  iisLPDComLib/src/iisLPDComLib_vq_RE8.c)

set(IIS_LPD_ENC_SRC
  iisLPDEncLib/src/iisLPDEncLib_acelp_AdaptiveCB.c
  iisLPDEncLib/src/iisLPDEncLib_acelp_ImprovedInnovativeCB.c
  iisLPDEncLib/src/iisLPDEncLib_acelp_InnovativeCB.c
  iisLPDEncLib/src/iisLPDEncLib_acelp_Main.c
  iisLPDEncLib/src/iisLPDEncLib_arithEncWrapper.c
  iisLPDEncLib/src/iisLPDEncLib_bpf.c
  iisLPDEncLib/src/iisLPDEncLib_bs.c
  iisLPDEncLib/src/iisLPDEncLib_closedLoop.c
  iisLPDEncLib/src/iisLPDEncLib_cmConfig.c
  iisLPDEncLib/src/iisLPDEncLib_lpc_LPCAnalysis.c
  iisLPDEncLib/src/iisLPDEncLib_lpc_Quantize.c
  iisLPDEncLib/src/iisLPDEncLib_main.c
  iisLPDEncLib/src/iisLPDEncLib_tcx_Main.c
  iisLPDEncLib/src/iisLPDEncLib_tcx_Tools.c
  iisLPDEncLib/src/iisLPDEncLib_transitions.c
  iisLPDEncLib/src/iisLPDEncLib_vq_Main.c
  iisLPDEncLib/src/iisLPDEncLib_vq_RE8.c
  iisLPDEncLib/src/iisLPDEncLib.c)

set(IIS_PARAM_LIST_SRC
  iisParamList/src/iisParam.c
  iisParamList/src/iisParamList.c)

set(IIS_SIG_MAP_SRC
  iisSigMap/src/iisSigMap.c
  iisSigMap/src/iisSigMapCommon.c
  iisSigMap/src/iisSigMapConfig.c
  iisSigMap/src/iisSigMapUsac.c)

set(IIS_SWITCHING_DECISION_SRC
  iisSwitchingDecision/src/iisSwitchingDecision.c)

set(IIS_XHEAAC_ENC_SRC
  iisxHEAACEncLib/src/iisxHEAACEncLib_aac_ifc.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_asc_ifc.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_audioPreRoll.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_bitDistribution.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_common.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_delayAndBuffer.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_drc_ifc.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_drc_loudness_ifc.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_encodeFrame_ifc.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_encoderStateMachine.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_extentsionData.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_live_loudness_leveler_ifc.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_loaswrapper.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_loudness_ifc.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_lraControl.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_mpegs_bitrate_cfg.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_mpegs_cfg_expert.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_mpegs_ifc.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_rapOnDemand.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_resampler_ifc.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_sbr_ifc.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_SigMap_ifc.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_submitDrcMetadata.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_syncframe.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_updateParamList.c
  iisxHEAACEncLib/src/iisxHEAACEncLib_warnings.c
  iisxHEAACEncLib/src/iisxHEAACEncLib.c
  iisxHEAACEncLib/src/iisxHEAACEncLibConfig.c)

set(LIMITER_SRC
  limiterlib/src/limiter.c)

set(MATH_SRC
  mathlib/src/aux_func.c
  mathlib/src/cfft.c
  mathlib/src/cpuinfo.c
  mathlib/src/fpucontrol.c
  mathlib/src/iis_dxt.c
  mathlib/src/iis_fft.c
  mathlib/src/iis_mdxt.c
  mathlib/src/iisfft.c
  mathlib/src/math_inf.c
  mathlib/src/mathlib.c)

set(MP4_ASC_SRC
  mp4ASC/IIS_MP4ASC_Interface/IIS_MP4ASC_Interface.c
  mp4ASC/src/audiospecificconfig.c
  mp4ASC/src/usacconfig.c)

set(MP4_BITBUF_SRC
  mp4bitbuf/src/bit_buf.c)

set(MP4_BITMUX_SRC
  mp4bitmux/src/aac/bit_aac.c
  mp4bitmux/src/aac/bit_enc.c
  mp4bitmux/src/aac/codebook.c
  mp4bitmux/src/aac/extpayload.c)

set(MP4_LOAS_WRITE_SRC
  mp4loaswrite/src/IIS_LATM_Interface.c
  mp4loaswrite/src/latm_interface.c)

set(MP4_TIMEBUF_SRC
  mp4timebuf/src/time_buffer.c)

set(QMF_SRC
  qmflib/src/qmflib_lowfreqfilter.c
  qmflib/src/qmflib_tables.c
  qmflib/src/qmflib.c)

set(READONLY_BITBUF_SRC
  readonlybitbuf/src/readonlybitbuf.c)

set(RESAMPLE_SRC
  resamplelib/src/polyphase/convolve.c
  resamplelib/src/polyphase/filtgen.c
  resamplelib/src/polyphase/helpfunc.c
  resamplelib/src/polyphase/kernfunc.c
  resamplelib/src/polyphase/polyfilt.c
  resamplelib/src/polyphase/polyphase.c
  resamplelib/src/polyphase/resample.c
  resamplelib/src/resamplelib.c
  resamplelib/src/smpl_resampler.c)

set(SBR_ENC_SRC
  sbrEnclib/src/bit_codec.c
  sbrEnclib/src/bit_sbr.c
  sbrEnclib/src/cmondata.c
  sbrEnclib/src/code_env.c
  sbrEnclib/src/env_est.c
  sbrEnclib/src/fram_gen.c
  sbrEnclib/src/freq_sca.c
  sbrEnclib/src/invf_est.c
  sbrEnclib/src/ivaltime.c
  sbrEnclib/src/mh_det.c
  sbrEnclib/src/nf_est.c
  sbrEnclib/src/sbr_main.c
  sbrEnclib/src/sbr_misc.c
  sbrEnclib/src/sib_det.c
  sbrEnclib/src/ton_corr.c
  sbrEnclib/src/tran_det.c)

set(SPACE_ENC_SRC
  spaceEnclib/src/sac_huff_cld.c
  spaceEnclib/src/sac_huff_icc.c
  spaceEnclib/src/sac_huff_ipd.c
  spaceEnclib/src/sac_huff_pt0.c
  spaceEnclib/src/sac_huff_reshape.c
  spaceEnclib/src/sac_nlc_enc.c
  spaceEnclib/src/sac_tonality.c
  spaceEnclib/src/sac_tsd.c
  spaceEnclib/src/space_bitstream.c
  spaceEnclib/src/space_delay.c
  spaceEnclib/src/space_filter.c
  spaceEnclib/src/space_framewindowing.c
  spaceEnclib/src/space_hybrid.c
  spaceEnclib/src/space_onsetdetect.c
  spaceEnclib/src/space_paramextract.c
  spaceEnclib/src/space_staticgain.c
  spaceEnclib/src/space_tree.c
  spaceEnclib/src/spaceEnclib.c)

set(STAT_CLASS_SRC
  statClassLib_C/src/statClass_decision.c
  statClassLib_C/src/statClass_deltaCep.c
  statClassLib_C/src/statClass_deltaLsf.c
  statClassLib_C/src/statClass_features.c
  statClassLib_C/src/statClass_gmm_14_7k.c
  statClassLib_C/src/statClass_gmm_16k.c
  statClassLib_C/src/statClass_gmm.c
  statClassLib_C/src/statClass_lpc.c
  statClassLib_C/src/statClass_ltpDelayVar.c
  statClassLib_C/src/statClass_mel2plp.c
  statClassLib_C/src/statClass_pauseDetection.c
  statClassLib_C/src/statClass_peakednessFreqVsTime.c
  statClassLib_C/src/statClass_pitch_ol.c
  statClassLib_C/src/statClass_preemph.c
  statClassLib_C/src/statClass_signal2mel.c
  statClassLib_C/src/statClass_speechClassification.c
  statClassLib_C/src/statClass_timeTransient.c
  statClassLib_C/src/statClass_utils.c
  statClassLib_C/src/statClasslib.c)

set(UTIL_SRC
  utillib/src/errorhnd.c
  utillib/src/matrixCalloc.c
  utillib/src/ngsalloc.c)

set(WRITEONLY_BITBUF_SRC
  writeonlybitbuf/src/writeonlybitbuf.c)

set(XHEAAC_ENC_SRC
  xHEAACEnc/src/xHEAACEnc.c
  xHEAACEnc/src/xHEAACEncCommon.c
  xHEAACEnc/src/xHEAACEncConfig.c
  xHEAACEnc/src/xHEAACEncExportImport.c
  xHEAACEnc/src/xHEAACEncLoudness.c
  xHEAACEnc/src/xHEAACEncMapParameters.c
  xHEAACEnc/src/xHEAACEncParameter.c
  xHEAACEnc/src/xHEAACEncWarningHandling.c)

set(fdk_xhe_aacinclude_HEADERS
  xHEAACEnc/include/xHEAACEnc.h)

set(libfdk_xhe_aac_SOURCES
    ${AACFASTENC_SRC} ${APPLAUSE_DETECTION_SRC}
    ${IIS_ARITH_ENC_SRC} ${IIS_AUDIO_PREROLL_SRC} ${IIS_BIT_FRAME_SRC}
    ${IIS_DRC_GAIN_ENC_SRC} ${IIS_ENCODER_CONFIG_SRC}
    ${IIS_ENCODER_CONFIG_CT_TABLE_SRC} ${IIS_LEVELER_SRC}
    ${IIS_LINKED_LIST_SRC} ${IIS_LOUDNESS_SRC} ${IIS_LPD_COM_SRC}
    ${IIS_LPD_ENC_SRC} ${IIS_PARAM_LIST_SRC} ${IIS_SIG_MAP_SRC}
    ${IIS_SWITCHING_DECISION_SRC} ${IIS_XHEAAC_ENC_SRC}
    ${LIMITER_SRC} ${MATH_SRC}
    ${MP4_ASC_SRC} ${MP4_BITBUF_SRC} ${MP4_BITMUX_SRC} ${MP4_LOAS_WRITE_SRC}
    ${MP4_TIMEBUF_SRC}
    ${QMF_SRC} ${READONLY_BITBUF_SRC} ${RESAMPLE_SRC} ${SBR_ENC_SRC}
    ${SPACE_ENC_SRC} ${STAT_CLASS_SRC} ${UTIL_SRC} ${WRITEONLY_BITBUF_SRC}
    ${XHEAAC_ENC_SRC})


list(TRANSFORM libfdk_aac_SOURCES PREPEND cpp/)
list(TRANSFORM fdk_aacinclude_HEADERS PREPEND cpp/)

list(TRANSFORM libfdk_xhe_aac_SOURCES PREPEND xhe-aac/)
list(TRANSFORM fdk_xhe_aacinclude_HEADERS PREPEND xhe-aac/)

if(WIN32 AND BUILD_SHARED_LIBS)
  set(libfdk_aac_SOURCES ${libfdk_aac_SOURCES} fdk-aac.def)
  set(libfdk_xhe_aac_SOURCES ${libfdk_xhe_aac_SOURCES} fdk-xhe-aac.def)
endif()

## Create Library target. Actually fdk-aac is enough, but we also create
## FDK-AAC::fdk-aac for consistence with config-file package.

add_library(fdk-aac ${libfdk_aac_SOURCES})
add_library(FDK-AAC::fdk-aac ALIAS fdk-aac)

add_library(fdk-xhe-aac ${libfdk_xhe_aac_SOURCES})
add_library(FDK-XHE-AAC::fdk-xhe-aac ALIAS fdk-xhe-aac)

## Library target configuration

### Library target includes
target_include_directories(fdk-aac
  PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cpp/libAACdec/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cpp/libAACenc/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cpp/libSYS/include>
  PRIVATE
    cpp/libArithCoding/include
    cpp/libDRCdec/include
    cpp/libSACdec/include
    cpp/libSACenc/include
    cpp/libSBRdec/include
    cpp/libSBRenc/include
    cpp/libMpegTPDec/include
    cpp/libMpegTPEnc/include
    cpp/libFDK/include
    cpp/libPCMutils/include)

target_include_directories(fdk-xhe-aac
  PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/xhe-aac/xHEAACEnc/include>
  PRIVATE
    xhe-aac/aacfastenc/include
    xhe-aac/applauseDetectionlib/include
    xhe-aac/iisArithEncLib/include
    xhe-aac/iisAudioPrerollLib/include
    xhe-aac/iisBitFrame/include
    xhe-aac/iisDRCGainEncLib/include
    xhe-aac/iisDRCGainGenLib/include
    xhe-aac/iisEncoderConfig/include
    xhe-aac/iisEncoderConfigCTable/include
    xhe-aac/iisLevelerLib/include
    xhe-aac/iisLinkedList/include
    xhe-aac/iisLoudnessLib/include
    xhe-aac/iisLPDComLib/include
    xhe-aac/iisLPDEncLib/include
    xhe-aac/iisParamList/include
    xhe-aac/iisSigMap/include
    xhe-aac/iisSwitchingDecision/include
    xhe-aac/iisxHEAACEncLib/include
    xhe-aac/limiterlib/include
    xhe-aac/mathlib/include
    xhe-aac/mp4ASC/IIS_MP4ASC_Interface
    xhe-aac/mp4ASC/include
    xhe-aac/mp4General
    xhe-aac/mp4bitbuf/include
    xhe-aac/mp4bitmux/src/aac
    xhe-aac/mp4loaswrite/include
    xhe-aac/mp4timebuf/include
    xhe-aac/qmflib/include
    xhe-aac/readonlybitbuf/include
    xhe-aac/resamplelib/include
    xhe-aac/sbrEnclib/include
    xhe-aac/spaceEnclib/include
    xhe-aac/statClassLib_C/include
    xhe-aac/utillib/include
    xhe-aac/utillib/include/IISutillib
    xhe-aac/writeonlybitbuf/include
    xhe-aac/xHEAACEnc/include)

### Link math library if required
target_link_libraries(fdk-aac PRIVATE $<$<BOOL:${HAVE_LIBM}>:m>)
target_link_libraries(fdk-xhe-aac PRIVATE $<$<BOOL:${HAVE_LIBM}>:m>)

### Set public headers and shared library version. Version info is critical for Unix-like OSes.
set_target_properties(fdk-aac PROPERTIES
  PUBLIC_HEADER "${fdk_aacinclude_HEADERS}"
  VERSION 2.0.3
  SOVERSION 2
  MACHO_COMPATIBILITY_VERSION 3.0.0
  MACHO_CURRENT_VERSION 3.3.0
  LINKER_LANGUAGE C)
set_target_properties(fdk-xhe-aac PROPERTIES
  PUBLIC_HEADER "${fdk_xhe_aacinclude_HEADERS}"
  VERSION 0.0.0
  SOVERSION 0
  MACHO_COMPATIBILITY_VERSION 0.0.0
  MACHO_CURRENT_VERSION 0.0.0)

### Some compiler options from Makefile.am
if(MSVC)
  target_compile_options(fdk-aac PRIVATE /EHsc)
else()
  target_compile_options(fdk-aac PRIVATE -fno-exceptions -fno-rtti)
endif()

### Set proper name for MinGW or Cygwin DLL

if((MINGW OR CYGWIN) AND BUILD_SHARED_LIBS)
  set_property(TARGET fdk-aac PROPERTY RUNTIME_OUTPUT_NAME fdk-aac-2)
  set_property(TARGET fdk-xhe-aac PROPERTY RUNTIME_OUTPUT_NAME fdk-xhe-aac-0)
endif()

## Library installation

### Note we export `fdk-aac-targets` to use with config-file package.
install(TARGETS fdk-aac EXPORT fdk-aac-targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT BIN
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT DEV
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT DEV
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fdk-aac COMPONENT DEV)

install(TARGETS fdk-xhe-aac EXPORT fdk-xhe-aac-targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT BIN
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT DEV
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT DEV
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fdk-xhe-aac COMPONENT DEV)

### Configure and install pkg-config module.
if(FDK_AAC_INSTALL_PKGCONFIG_MODULE)
  set(prefix ${CMAKE_INSTALL_PREFIX})
  set(exec_prefix "\$\{prefix\}")
  set(libdir "\$\{exec_prefix\}/${CMAKE_INSTALL_LIBDIR}")
  set(includedir "\$\{prefix\}/${CMAKE_INSTALL_INCLUDEDIR}")
  set(PACKAGE_VERSION ${PROJECT_VERSION})
  if(HAVE_LIBM)
    if(BUILD_SHARED_LIBS)
      set(LIBS_PRIVATE "-lm")
    else()
      set(LIBS_PUBLIC "-lm")
    endif()
  endif()
  configure_file(fdk-aac.pc.in fdk-aac.pc @ONLY)
  configure_file(fdk-xhe-aac.pc.in fdk-xhe-aac.pc @ONLY)
  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/fdk-aac.pc
      ${CMAKE_CURRENT_BINARY_DIR}/fdk-xhe-aac.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

### Configure and install config-file package. Something like pkg-config module, but for Windows.
if(FDK_AAC_INSTALL_CMAKE_CONFIG_MODULE)

  set(FDK_AAC_INSTALL_PACKAGEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/fdk-aac)
  set(FDK_XHE_AAC_INSTALL_PACKAGEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/fdk-xhe-aac)

  configure_package_config_file(fdk-aac-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/fdk-aac-config.cmake
    INSTALL_DESTINATION ${FDK_AAC_INSTALL_PACKAGEDIR})
  configure_package_config_file(fdk-xhe-aac-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/fdk-xhe-aac-config.cmake
    INSTALL_DESTINATION ${FDK_XHE_AAC_INSTALL_PACKAGEDIR})

  if(CMAKE_VERSION VERSION_LESS 3.11)
    write_basic_package_version_file(fdk-aac-config-version.cmake COMPATIBILITY SameMajorVersion)
    write_basic_package_version_file(fdk-xhe-aac-config-version.cmake COMPATIBILITY SameMajorVersion)
  else()
    write_basic_package_version_file(fdk-aac-config-version.cmake COMPATIBILITY SameMinorVersion)
    write_basic_package_version_file(fdk-xhe-aac-config-version.cmake COMPATIBILITY SameMinorVersion)
  endif()

  install(EXPORT fdk-aac-targets
    NAMESPACE FDK-AAC::
    DESTINATION ${FDK_AAC_INSTALL_PACKAGEDIR})
  install(EXPORT fdk-xhe-aac-targets
    NAMESPACE FDK-XHE-AAC::
    DESTINATION ${FDK_XHE_AAC_INSTALL_PACKAGEDIR})
  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/fdk-aac-config.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/fdk-aac-config-version.cmake
    DESTINATION ${FDK_AAC_INSTALL_PACKAGEDIR})
  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/fdk-xhe-aac-config.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/fdk-xhe-aac-config-version.cmake
    DESTINATION ${FDK_XHE_AAC_INSTALL_PACKAGEDIR})

endif()

# Programs

if(BUILD_PROGRAMS)

  ## Program sources

  set(aac_enc_SOURCES
    aac-enc.c
    wavreader.c
    wavreader.h)

  set(aac_dec_SOURCES
    aac-dec.c
    wavwriter.c
    wavwriter.h)

  ## Program target
  add_executable(aac-enc ${aac_enc_SOURCES})
  add_executable(aac-dec ${aac_dec_SOURCES})

  ## Program target configuration
  target_link_libraries(aac-enc PRIVATE fdk-aac)
  target_link_libraries(aac-dec PRIVATE fdk-aac)
  target_compile_definitions(aac-enc PRIVATE $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>)
  target_compile_definitions(aac-dec PRIVATE $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>)
  target_include_directories(aac-enc PRIVATE cpp)
  target_include_directories(aac-dec PRIVATE cpp)
  if(WIN32)
    target_sources(aac-enc PRIVATE win32/getopt.h)
    target_sources(aac-dec PRIVATE win32/getopt.h)
    target_include_directories(aac-enc PRIVATE win32)
    target_include_directories(aac-dec PRIVATE win32)
  endif()

  ## Program target installation
  install(TARGETS aac-enc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(TARGETS aac-dec RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


  ## Tool for testing the library
  set(test_encode_decode_SOURCES
    test-encode-decode.c
    wavreader.c
    wavreader.h
    sha1.c
    sha1.h)

  add_executable(test-encode-decode ${test_encode_decode_SOURCES})
  target_link_libraries(test-encode-decode PRIVATE fdk-aac)
endif()
