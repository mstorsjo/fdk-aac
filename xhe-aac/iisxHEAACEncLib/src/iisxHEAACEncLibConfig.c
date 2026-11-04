
/* -----------------------------------------------------------------------------
Software Copyright License for The Fraunhofer FDK Extended High Efficiency AAC
Encoder Software for Android

© Copyright 1995 - 2025 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. and Contributors
All rights reserved.

1.    INTRODUCTION

The Fraunhofer FDK Extended High Efficiency AAC Encoder Software for Android
("FDK Extended High Efficiency AAC Encoder") is software that implements the
encoding of digital audio according to the MPEG-D Unified Speech and Audio
Coding (USAC) standard and MPEG-D Dynamic Range Control (DRC) standard. This FDK
Extended High Efficiency AAC Encoder Software is intended to be used on a wide
variety of Android devices. It is technically not suited to encode content for
digital radio broadcasting services, including DRM and similar standards.

Patent licenses for necessary patent claims for the FDK Extended High Efficiency
AAC Encoder Software (including those of Fraunhofer), for the use in commercial
products and services, may be obtained from the respective patent owners
individually and/or from Via Licensing Alliance (www.via-la.com).

Fraunhofer supports the development of Extended High Efficiency AAC products and
services by offering additional software, documentation, and technical advice.
In addition, it operates the xHE-AAC Trademark Program to ease interoperability
testing of end products. Please visit http://www.xhe-aac.com for more
information.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees, provided that you
satisfy the following conditions:

You must retain the complete text of this software license in redistributions of
the FDK Extended High Efficiency AAC Encoder Software or your modifications
thereto in source code form.

You must retain the complete text of this software license in the documentation
and/or other materials provided with redistributions of the FDK Extended High
Efficiency AAC Encoder Software or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the
FDK Extended High Efficiency AAC Encoder Software and your modifications thereto
to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived
from this software without prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute
the FDK Extended High Efficiency AAC Encoder Software or your modifications
thereto.

Your modified versions of the FDK Extended High Efficiency AAC Encoder Software
must carry prominent notices stating that you changed the software and the date
of any change. For modified versions of the FDK Extended High Efficiency AAC
Encoder Software, the term "Fraunhofer FDK Extended High Efficiency AAC Encoder
Software for Android" must be replaced by the term "Third-Party Modified Version
of the Fraunhofer FDK Extended High Efficiency AAC Encoder Software for
Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty for patent non-infringement with respect to this
software. You may use this FDK Extended High Efficiency AAC Encoder Software or
modifications thereto only for purposes that are authorized by appropriate
patent licenses.

4.    DISCLAIMER

This FDK Extended High Efficiency AAC Encoder Software is provided by Fraunhofer
on behalf of the copyright holders and contributors "AS IS" and WITHOUT ANY
EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied
warranties of merchantability and fitness for a particular purpose. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE for any direct, indirect,
incidental, special, exemplary, or consequential damages, including but not
limited to procurement of substitute goods or services; loss of use, data, or
profits, or business interruption, however caused and on any theory of
liability, whether in contract, strict liability, or tort (including
negligence), arising in any way out of the use of this software, even if advised
of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Division Audio and Media Technologies - FDK Extended High Efficiency
AAC Encoder
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>

#include "iisxHEAACEncLibConfig.h"
#include "iisxHEAACEncLib_common.h"

#include "aacenc.h"
#include "iisutillib.h"
#include "iisParam.h"

#define IISXHEAACENCLIB_SMC_INTERVAL_DEFAULT 24488
#define IISXHEAACENCLIB_SYNC_ADDITIONAL_FLUSHING 962

int iisxHEAACEncLibGetParamList(
    PARAMLIST_INSTANCE_HANDLE const hParamList,
    PARAM_INSTANCE_HANDLE const hParam,
    void *ptr) {
  XHEAACENCLIB_CONFIG_HANDLE hConfig = (XHEAACENCLIB_CONFIG_HANDLE)ptr;
  int error = 0;
  (void)hParamList;

  if (hParam && hConfig) {
    switch (hParam->paramTag) {
      case PARAMLIST_PARAMETER_AOT:
        switch (hParam->paramValue._int) {
          case PARAMLIST_AOT_42:
            hConfig->aot = AUD_OBJ_TYP_USAC;
            break;
          case PARAMLIST_AOT_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_BITRATE:
        hConfig->bitRate = hParam->paramValue._int;
        break;

      case PARAMLIST_PARAMETER_LOUDNESS_LEVEL:
        hConfig->loudnessLevel = hParam->paramValue._float;
        hConfig->bLoudnessLevelSet = 1;
        break;

      case PARAMLIST_PARAMETER_LIVE_LRAC:
        switch (hParam->paramValue._int) {
          case PARAMLIST_LIVE_LRAC_ON:
            hConfig->bRealtimeLRAC = 1;
            break;
          case PARAMLIST_LIVE_LRAC_OFF:
            hConfig->bRealtimeLRAC = 0;
            break;
          case PARAMLIST_LIVE_LRAC_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_ANCHOR_LOUDNESS_LEVEL:
        hConfig->anchorLoudnessLevel = hParam->paramValue._float;
        hConfig->bAnchorLoudnessLevelSet = 1;
        break;

      case PARAMLIST_PARAMETER_INTERNAL_MEASURED_ANCHOR_LOUDNESS:
        hConfig->anchorLoudnessLevel = hParam->paramValue._float;
        hConfig->bAnchorLoudnessLevelSet = 1;
        break;

      case PARAMLIST_PARAMETER_LOUDNESS_DATA:
        hConfig->loudnessEnvelope = hParam->paramValue._pvoid;
        hConfig->bLoudnessEnvelopeSet = 1;
        break;

      case PARAMLIST_PARAMETER_SAMPLE_PEAK:
        hConfig->samplePeak = hParam->paramValue._float;
        hConfig->samplePeakPresent = 1;
        break;

      case PARAMLIST_PARAMETER_DISABLE_LOUDNESS:
        switch (hParam->paramValue._int) {
          case PARAMLIST_DISABLELOUDNESS_ON:
            hConfig->bDisableLoudness = 1;
            break;
          case PARAMLIST_DISABLELOUDNESS_OFF:
            hConfig->bDisableLoudness = 0;
            break;
          case PARAMLIST_DISABLELOUDNESS_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_QUIET_LOUDNESS_THRESHOLD:
        hConfig->quietLoudnessThreshold = hParam->paramValue._float;
        hConfig->bQuietLoudnessThresholdSet = 1;
        break;

      case PARAMLIST_PARAMETER_GRANULELENGTH:
        switch (hParam->paramValue._int) {
          case PARAMLIST_GRANULELENGTH_768:
            hConfig->granuleLength = XHEAACENCLIB_GRANULELENGTH_768;
            break;
          case PARAMLIST_GRANULELENGTH_960:
            hConfig->granuleLength = XHEAACENCLIB_GRANULELENGTH_960;
            break;
          case PARAMLIST_GRANULELENGTH_1024:
            hConfig->granuleLength = XHEAACENCLIB_GRANULELENGTH_1024;
            break;
          case PARAMLIST_GRANULELENGTH_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_INSAMPLERATE:
        hConfig->sampleRateIn = hParam->paramValue._int;
        break;

      case PARAMLIST_PARAMETER_ENHANCED_PULSESEARCH:
        switch (hParam->paramValue._int) {
          case PARAMLIST_PULSESEARCH_ENHANCED_OFF:
            hConfig->optimizedSpeedPulseSearch = 0;
            break;
          case PARAMLIST_PULSESEARCH_ENHANCED_ON:
            hConfig->optimizedSpeedPulseSearch = 1;
            break;
          case PARAMLIST_PULSESEARCH_ENHANCED_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_RAP_ON_DEMAND_ADVANCED_MODE:
        switch (hParam->paramValue._int) {
          case PARAMLIST_RAP_ON_DEMAND_ADVANCED_MODE_OFF:
            hConfig->rapOnDemandAdvancedMode = 0;
            break;
          case PARAMLIST_RAP_ON_DEMAND_ADVANCED_MODE_ON:
            hConfig->rapOnDemandAdvancedMode = 1;
            break;
          case PARAMLIST_RAP_ON_DEMAND_ADVANCED_MODE_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_COREMODE:
        switch (hParam->paramValue._int) {
          case PARAMLIST_COREMODE_FD:
            hConfig->coreMode = XHEAACENCLIB_CODING_MODE_FD;
            break;
          case PARAMLIST_COREMODE_LPD:
            hConfig->coreMode = XHEAACENCLIB_CODING_MODE_LPD;
            break;
          case PARAMLIST_COREMODE_SWITCHED:
            hConfig->coreMode = XHEAACENCLIB_CODING_MODE_SWITCHED;
            break;
          case PARAMLIST_COREMODE_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_ACELPMODEINDEX:
        hConfig->acelpModeIndex = hParam->paramValue._int;
        break;

      case PARAMLIST_PARAMETER_SBR:
        switch (hParam->paramValue._int) {
          case PARAMLIST_SBR_ON:
            hConfig->bUseSBR = 1;
            break;
          case PARAMLIST_SBR_OFF:
            hConfig->bUseSBR = 0;
            break;
          case PARAMLIST_SBR_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_FRAMESAMPLES:
        switch (hParam->paramValue._int) {
          case PARAMLIST_FRAMESAMPLES_768:
            hConfig->nFrameSamples = FRAMELENGTH_768;
            break;
          case PARAMLIST_FRAMESAMPLES_960:
            hConfig->nFrameSamples = FRAMELENGTH_960;
            break;
          case PARAMLIST_FRAMESAMPLES_1024:
            hConfig->nFrameSamples = FRAMELENGTH_1024;
            break;
          case PARAMLIST_FRAMESAMPLES_1920:
            hConfig->nFrameSamples = FRAMELENGTH_1920;
            break;
          case PARAMLIST_FRAMESAMPLES_2048:
            hConfig->nFrameSamples = FRAMELENGTH_2048;
            break;
          case PARAMLIST_FRAMESAMPLES_4096:
            hConfig->nFrameSamples = FRAMELENGTH_4096;
            break;
          case PARAMLIST_FRAMESAMPLES_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_SBRRATIO:
        switch (hParam->paramValue._int) {
          case PARAMLIST_SBRRATIO_NONE:
            hConfig->sbrRatio.upFac = 1;
            hConfig->sbrRatio.downFac = 1;
            break;
          case PARAMLIST_SBRRATIO_2_1:
            hConfig->sbrRatio.upFac = 2;
            hConfig->sbrRatio.downFac = 1;
            break;
          case PARAMLIST_SBRRATIO_4_1:
            hConfig->sbrRatio.upFac = 4;
            hConfig->sbrRatio.downFac = 1;
            hConfig->bUseSBR41 = 1;
            break;
          case PARAMLIST_SBRRATIO_8_3:
            hConfig->sbrRatio.upFac = 8;
            hConfig->sbrRatio.downFac = 3;
            break;
          case PARAMLIST_SBR_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_SBRRAISEDXOVERFREQ:
        hConfig->bRaisedXOverFreq = hParam->paramValue._int;
        break;

      case PARAMLIST_PARAMETER_NOISEFILLING:
        switch (hParam->paramValue._int) {
          case PARAMLIST_NOISEFILLING_ON:
            hConfig->useNoiseFilling = 1;
            break;
          case PARAMLIST_NOISEFILLING_OFF:
            hConfig->useNoiseFilling = 0;
            break;
          case PARAMLIST_NOISEFILLING_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_TSD:
        switch (hParam->paramValue._int) {
          case PARAMLIST_TSD_ON:
            hConfig->bUseTSD = 1;
            break;
          case PARAMLIST_TSD_OFF:
            hConfig->bUseTSD = 0;
            break;
          case PARAMLIST_TSD_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_SBRFIXBORDERFORINDEPFLAG:
        switch (hParam->paramValue._int) {
          case PARAMLIST_SBRFIXBORDERFORINDEPFLAG_ON:
            hConfig->useSbrFixBorderForIndepFlag = TOOL_MODE_ON;
            break;
          case PARAMLIST_SBRFIXBORDERFORINDEPFLAG_OFF:
            hConfig->useSbrFixBorderForIndepFlag = TOOL_MODE_OFF;
            break;
          case PARAMLIST_SBRFIXBORDERFORINDEPFLAG_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_MPEGS_HIGHRATE_MODE:
        switch (hParam->paramValue._int) {
          case PARAMLIST_MPEGSHIGHRATEMODE_ON:
            hConfig->useMpegsHighRateMode = TOOL_MODE_ON;
            break;
          case PARAMLIST_MPEGSHIGHRATEMODE_OFF:
            hConfig->useMpegsHighRateMode = TOOL_MODE_OFF;
            break;
          case PARAMLIST_MPEGSHIGHRATEMODE_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_MPEGSNUMPARAMBANDS:
        hConfig->mpegsSetParamBands = hParam->paramValue._int;
        break;

      case PARAMLIST_PARAMETER_UNIFIEDSTEREO:
        break;

      case PARAMLIST_PARAMETER_HBE:
        switch (hParam->paramValue._int) {
          case PARAMLIST_HBE_ON:
            hConfig->bUseHBE = 1;
            break;
          case PARAMLIST_HBE_OFF:
            hConfig->bUseHBE = 0;
            break;
          case PARAMLIST_HBE_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_SBRPVC:
        switch (hParam->paramValue._int) {
          case PARAMLIST_SBRPVC_ON:
            hConfig->bUseSBRPVC = 1;
            break;
          case PARAMLIST_SBRPVC_OFF:
            hConfig->bUseSBRPVC = 0;
            break;
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_SBRSIGNALING:
        switch (hParam->paramValue._int) {
          case PARAMLIST_SBRSIGNALING_OFF:
            hConfig->sbrSignaling = SBR_SIGNALING_DISABLE;
            break;
          case PARAMLIST_SBRSIGNALING_EXPL_BC:
            hConfig->sbrSignaling = SBR_SIGNALING_EXPL_BC;
            break;
          case PARAMLIST_SBRSIGNALING_EXPL_HIER:
            hConfig->sbrSignaling = SBR_SIGNALING_EXPL_HIER;
            break;
          case PARAMLIST_SBRSIGNALING_IMPLICIT:
            hConfig->sbrSignaling = SBR_SIGNALING_IMPLICIT;
            break;
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_QUALITY:
        switch (hParam->paramValue._int) {
          case PARAMLIST_QUALITY_FAST:
            hConfig->quality = XHEAACENCLIB_QUAL_FAST;
            break;
          case PARAMLIST_QUALITY_MEDIUM:
            hConfig->quality = XHEAACENCLIB_QUAL_MEDIUM;
            break;
          case PARAMLIST_QUALITY_HIGH:
            hConfig->quality = XHEAACENCLIB_QUAL_HIGH;
            break;
          case PARAMLIST_QUALITY_HIGHEST:
            hConfig->quality = XHEAACENCLIB_QUAL_HIGH;
            break;
          case PARAMLIST_QUALITY_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_CHANNELCONFIG:
        switch (hParam->paramValue._int) {
          case PARAMLIST_CHANNELCONFIG_MONO:
            hConfig->CicpIndex = XHEAACENCLIB_SIGMAP_CICP_1;
            break;
          case PARAMLIST_CHANNELCONFIG_STEREO:
            hConfig->CicpIndex = XHEAACENCLIB_SIGMAP_CICP_2;
            break;
          case PARAMLIST_CHANNELCONFIG_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_BITRATEMODE:
        switch (hParam->paramValue._int) {
          case PARAMLIST_BITRATEMODE_CBR:
            hConfig->bitrateMode = XHEAACENCLIB_BR_MODE_CBR;
            break;
          case PARAMLIST_BITRATEMODE_VBR0:
            hConfig->bitrateMode = XHEAACENCLIB_BR_MODE_VBR_0;
            break;
          case PARAMLIST_BITRATEMODE_VBR1:
            hConfig->bitrateMode = XHEAACENCLIB_BR_MODE_VBR_1;
            break;
          case PARAMLIST_BITRATEMODE_VBR2:
            hConfig->bitrateMode = XHEAACENCLIB_BR_MODE_VBR_2;
            break;
          case PARAMLIST_BITRATEMODE_VBR3:
            hConfig->bitrateMode = XHEAACENCLIB_BR_MODE_VBR_3;
            break;
          case PARAMLIST_BITRATEMODE_VBR4:
            hConfig->bitrateMode = XHEAACENCLIB_BR_MODE_VBR_4;
            break;
          case PARAMLIST_BITRATEMODE_VBR5:
            hConfig->bitrateMode = XHEAACENCLIB_BR_MODE_VBR_5;
            break;
          case PARAMLIST_BITRATEMODE_VBR6:
            hConfig->bitrateMode = XHEAACENCLIB_BR_MODE_VBR_6;
            break;
          case PARAMLIST_BITRATEMODE_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_TRANSPORTFORMAT:
        switch (hParam->paramValue._int) {
          case PARAMLIST_TRANSPORTFORMAT_RAW:
            hConfig->transportFormat = TT_RAW;
            break;
          case PARAMLIST_TRANSPORTFORMAT_ADTS:
            hConfig->transportFormat = TT_ADTS;
            break;
          case PARAMLIST_TRANSPORTFORMAT_LATM:
            hConfig->transportFormat = TT_LATM;
            break;
          case PARAMLIST_TRANSPORTFORMAT_LATMLOAS:
            hConfig->transportFormat = TT_LOAS;
            break;
          case PARAMLIST_TRANSPORTFORMAT_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_CONFIGSET:
        switch (hParam->paramValue._int) {
          case PARAMLIST_CONFIGSET_DASH:
            hConfig->configSet = CONFIG_SET_DASH;
            break;
          case PARAMLIST_CONFIGSET_DRM30:
            hConfig->configSet = CONFIG_SET_DRM_30;
            break;
          case PARAMLIST_CONFIGSET_DRMPLUS:
            hConfig->configSet = CONFIG_SET_DRM_PLUS;
            break;
          case PARAMLIST_CONFIGSET_SEEKABLE:
            hConfig->configSet = CONFIG_SET_SEEKABLE;
            break;
          case PARAMLIST_CONFIGSET_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_PRESET:
        switch (hParam->paramValue._int) {
          case PARAMLIST_PRESET_AUDIO_ARCHIVE:
            hConfig->preset = XHEAACENCLIB_PRESET_AUDIO_ARCHIVE;
            break;
          case PARAMLIST_PRESET_AUDIO_STREAMING:
            hConfig->preset = XHEAACENCLIB_PRESET_AUDIO_STREAMING;
            break;
          case PARAMLIST_PRESET_AV_STREAMING:
            hConfig->preset = XHEAACENCLIB_PRESET_AV_STREAMING;
            break;
          case PARAMLIST_PRESET_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_ANC_BITRATE:
        hConfig->nAncDataBitRate = hParam->paramValue._int;
        break;

      case PARAMLIST_PARAMETER_RAP_INTERVAL:
        hConfig->randomAccessIntervalMs = hParam->paramValue._int;
        break;

      case PARAMLIST_PARAMETER_RAP_INTERVAL_SAMPLES:
        hConfig->randomAccessIntervalSamples = hParam->paramValue._int;
        break;

      case PARAMLIST_PARAMETER_RAP_MIN_INTERVAL_SAMPLES:
        hConfig->randomAccessIntervalMin = hParam->paramValue._int;
        break;

      case PARAMLIST_PARAMETER_TRANSPORTFORMAT_SUBFRAMES:
        hConfig->loasNrSubFrames = hParam->paramValue._int;
        break;

      case PARAMLIST_PARAMETER_USAC_MAX_IF_DIST:
        hConfig->usacIndepFlagIntervalMs = hParam->paramValue._int;
        break;

      case PARAMLIST_PARAMETER_USAC_MAX_IF_DIST_SAMPLES:
        hConfig->usacIndepFlagIntervalSamples = hParam->paramValue._int;
        break;

      case PARAMLIST_PARAMETER_TNS:
        hConfig->bUseTNS = hParam->paramValue._int;
        break;

      case PARAMLIST_PARAMETER_STEREOCONFIGIDX:
        switch (hParam->paramValue._int) {
          case PARAMLIST_STEREOCONFIGIDX_NONE:
            hConfig->stereoConfigIndex = -1;
            break;
          case PARAMLIST_STEREOCONFIGIDX_0:
            hConfig->stereoConfigIndex = 0;
            break;
          case PARAMLIST_STEREOCONFIGIDX_1:
            hConfig->stereoConfigIndex = 1;
            break;
          case PARAMLIST_STEREOCONFIGIDX_2:
            hConfig->stereoConfigIndex = 2;
            break;
          case PARAMLIST_STEREOCONFIGIDX_3:
            hConfig->stereoConfigIndex = 3;
            break;
          case PARAMLIST_STEREOCONFIGIDX_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_LOWDELAYSWITCHING:
        hConfig->lowDelaySwitching = hParam->paramValue._int;
        break;

      case PARAMLIST_PARAMETER_PREROLLFRAMES:
        switch (hParam->paramValue._int) {
          case PARAMLIST_PREROLLFRAMES_ON:

            hConfig->audioPreRollNumAU = -1;
            break;
          case PARAMLIST_PREROLLFRAMES_OFF:
            hConfig->audioPreRollNumAU = 0;
            break;
          case PARAMLIST_PREROLLFRAMES_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_AUDIOPREROLLOUTOFBITRES:
        if (hParam->paramValue._int == 0) {
          hConfig->audioPreRollBitResMode = XHEAACENCLIB_APR_BITRESMODE_IN_FAILSAVE_DUMP_PREROLL;
        } else if (hParam->paramValue._int == 1) {
          hConfig->audioPreRollBitResMode = XHEAACENCLIB_APR_BITRESMODE_OUT;
        }

        else {
          assert(0);
          hConfig->audioPreRollBitResMode = XHEAACENCLIB_APR_BITRESMODE_INVALID;
        }
        break;

      case PARAMLIST_PARAMETER_FLUSHINGMODE:
        switch (hParam->paramValue._int) {
          case PARAMLIST_FLUSHINGMODE_DEFAULT:
            hConfig->additionalSyncFlushing = 0;
            break;
          case PARAMLIST_FLUSHINGMODE_SYNC:
            hConfig->additionalSyncFlushing = IISXHEAACENCLIB_SYNC_ADDITIONAL_FLUSHING;
            break;
          case PARAMLIST_FLUSHINGMODE_INVALID:
          default:
            assert(0);
            hConfig->additionalSyncFlushing = 0;
            break;
        }
        break;

      case PARAMLIST_PARAMETER_STREAMID:
        hConfig->streamID = hParam->paramValue._int;
        break;

      case PARAMLIST_PARAMETER_RAP_OCCURRENCE:
        switch (hParam->paramValue._int) {
          case PARAMLIST_RAP_OCCURRENCE_CONSTANT_INTERVAL:
            hConfig->rapOccurrence = XHEAACENCLIB_RAP_OCCURRENCE_CONSTANT_INTERVAL;
            break;
          case PARAMLIST_RAP_OCCURRENCE_ON_DEMAND:
            hConfig->rapOccurrence = XHEAACENCLIB_RAP_OCCURRENCE_ON_DEMAND;
            break;
          case PARAMLIST_RAP_OCCURRENCE_INVALID:
          default:
            assert(0);
            hConfig->rapOccurrence = XHEAACENCLIB_RAP_OCCURRENCE_INVALID;
            break;
        }
        break;

      case PARAMLIST_PARAMETER_RAP_PROPERTY:
        switch (hParam->paramValue._int) {
          case PARAMLIST_RAP_PROPERTY_OFF:
            hConfig->rapProperty = XHEAACENCLIB_RAP_PROPERTY_OFF;
            break;
          case PARAMLIST_RAP_PROPERTY_ACCESS:
            hConfig->rapProperty = XHEAACENCLIB_RAP_PROPERTY_ACCESS;
            break;
          case PARAMLIST_RAP_PROPERTY_SWITCHABLE:
            hConfig->rapProperty = XHEAACENCLIB_RAP_PROPERTY_SWITCHABLE;
            break;
          case PARAMLIST_RAP_PROPERTY_SEEKABLE:
            hConfig->rapProperty = XHEAACENCLIB_RAP_PROPERTY_SEEKABLE;
            break;
          case PARAMLIST_RAP_PROPERTY_INVALID:
          default:
            assert(0);
            hConfig->rapProperty = XHEAACENCLIB_RAP_PROPERTY_INVALID;
            break;
        }
        break;

      case PARAMLIST_PARAMETER_DRC_MODE:
        switch (hParam->paramValue._int) {
          case PARAMLIST_DRCMODE_OFF:
            hConfig->drcMode = XHEAACENCLIB_DRCMODE_OFF;
            break;
          case PARAMLIST_DRCMODE_LN_NE_LI_GE_LRACONTROL:
            hConfig->drcMode = XHEAACENCLIB_DRCMODE_LN_NE_LI_GE_LRACONTROL;
            break;
          case PARAMLIST_DRCMODE_INVALID:
            hConfig->drcMode = XHEAACENCLIB_DRCMODE_INVALID;
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_MPEGD_DRC_TARGET_LOUDNESS_RANGE:
        hConfig->drcTargetLra = hParam->paramValue._float;
        hConfig->drcTargetLraPresent = 1;
        break;

      case PARAMLIST_PARAMETER_LIVE_LOUDNESS_LEVEL:
        hConfig->liveLoudnessLevel = hParam->paramValue._float;
        hConfig->bLiveLoudnessLevelSet = 1;
        break;

      case PARAMLIST_PARAMETER_LIVE_SAMPLE_PEAK:
        hConfig->liveSamplePeak = hParam->paramValue._float;
        hConfig->bLiveSamplePeakSet = 1;
        break;

      case PARAMLIST_PARAMETER_LIVE_MODE:
        hConfig->liveMode = hParam->paramValue._int;
        hConfig->bLiveModeSet = 1;
        break;

      case PARAMLIST_PARAMETER_LIVE_LOUDNESS_REL_MAX_GAIN:
        hConfig->liveRelMaxGain = hParam->paramValue._float;
        hConfig->bLiveRelMaxGainSet = 1;
        break;

      case PARAMLIST_PARAMETER_ALBUM_LOUDNESS_LEVEL:
        hConfig->albumLoudnessLevel = hParam->paramValue._float;
        hConfig->bAlbumLoudnessLevelSet = 1;
        break;

      case PARAMLIST_PARAMETER_PRIMING_MODE:
        switch (hParam->paramValue._int) {
          case PARAMLIST_PRIMING_FULL:
            hConfig->primingMode = XHEAACENC_PRIMINGMODE_FULL;
            break;
          case PARAMLIST_PRIMING_NONE:
            hConfig->primingMode = XHEAACENC_PRIMINGMODE_NONE;
            break;
          case PARAMLIST_PRIMING_STDDELAY:
            hConfig->primingMode = XHEAACENC_PRIMINGMODE_STDDELAY;
            break;
          case PARAMLIST_PRIMING_INVALID:
          default:
            assert(0);
            hConfig->primingMode = XHEAACENC_PRIMINGMODE_INVALID;
            break;
        }
        break;

      case PARAMLIST_PARAMETER_MPEG2AAC:
        switch (hParam->paramValue._int) {
          case PARAMLIST_MPEG2AAC_ON:
            hConfig->mpeg2AAC = XHEAACENCLIB_MPEG2AAC_ON;
            break;
          case PARAMLIST_MPEG2AAC_OFF:
            hConfig->mpeg2AAC = XHEAACENCLIB_MPEG2AAC_OFF;
            break;
          case PARAMLIST_MPEG2AAC_INVALID:
          default:
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_MPEG4_DRC_LIGHT_PROF:
        switch (hParam->paramValue._int) {
          case PARAMLIST_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT:
            hConfig->mpeg4DrcLightProf = XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT;
            break;
          case PARAMLIST_MPEG4_DRC_LIGHT_PROF_NONE:
            hConfig->mpeg4DrcLightProf = XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_NONE;
            break;
          case PARAMLIST_MPEG4_DRC_LIGHT_PROF_FILMSTANDARD:
            hConfig->mpeg4DrcLightProf = XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_FILMSTANDARD;
            break;
          case PARAMLIST_MPEG4_DRC_LIGHT_PROF_FILMLIGHT:
            hConfig->mpeg4DrcLightProf = XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_FILMLIGHT;
            break;
          case PARAMLIST_MPEG4_DRC_LIGHT_PROF_MUSICSTANDARD:
            hConfig->mpeg4DrcLightProf = XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_MUSICSTANDARD;
            break;
          case PARAMLIST_MPEG4_DRC_LIGHT_PROF_MUSICLIGHT:
            hConfig->mpeg4DrcLightProf = XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_MUSICLIGHT;
            break;
          case PARAMLIST_MPEG4_DRC_LIGHT_PROF_SPEECH:
            hConfig->mpeg4DrcLightProf = XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_SPEECH;
            break;
          case PARAMLIST_MPEG4_DRC_LIGHT_PROF_INVALID:
            hConfig->mpeg4DrcLightProf = XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_INVALID;
            break;
          case PARAMLIST_MPEG4_DRC_LIGHT_EXTERNAL_GAIN:
            hConfig->mpeg4DrcLightProf = XHEAACENCLIB_MPEG4_DRC_LIGHT_EXT_GAIN;
            break;
          default:
            hConfig->mpeg4DrcLightProf = XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_INVALID;
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_MPEG4_DRC_LIGHT_TARGET_LEVEL:
        hConfig->mpeg4DRCLightTrgtLvl = hParam->paramValue._float;
        hConfig->mpeg4DRCLightTrgtLvl_present = 1;
        break;

      case PARAMLIST_PARAMETER_MPEG4_DRC_HEAVY_PROF:
        switch (hParam->paramValue._int) {
          case PARAMLIST_MPEG4_DRC_HEAVY_PROF_NOT_PRESENT:
            hConfig->mpeg4DrcHeavyProf = XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_NOT_PRESENT;
            break;
          case PARAMLIST_MPEG4_DRC_HEAVY_PROF_NONE:
            hConfig->mpeg4DrcHeavyProf = XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_NONE;
            break;
          case PARAMLIST_MPEG4_DRC_HEAVY_PROF_FILMSTANDARD:
            hConfig->mpeg4DrcHeavyProf = XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_FILMSTANDARD;
            break;
          case PARAMLIST_MPEG4_DRC_HEAVY_PROF_FILMLIGHT:
            hConfig->mpeg4DrcHeavyProf = XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_FILMLIGHT;
            break;
          case PARAMLIST_MPEG4_DRC_HEAVY_PROF_MUSICSTANDARD:
            hConfig->mpeg4DrcHeavyProf = XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_MUSICSTANDARD;
            break;
          case PARAMLIST_MPEG4_DRC_HEAVY_PROF_MUSICLIGHT:
            hConfig->mpeg4DrcHeavyProf = XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_MUSICLIGHT;
            break;
          case PARAMLIST_MPEG4_DRC_HEAVY_PROF_SPEECH:
            hConfig->mpeg4DrcHeavyProf = XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_SPEECH;
            break;
          case PARAMLIST_MPEG4_DRC_HEAVY_PROF_INVALID:
            hConfig->mpeg4DrcHeavyProf = XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_INVALID;
            break;
          case PARAMLIST_MPEG4_DRC_HEAVY_EXTERNAL_GAIN:
            hConfig->mpeg4DrcHeavyProf = XHEAACENCLIB_MPEG4_DRC_HEAVY_EXT_GAIN;
            break;
          default:
            hConfig->mpeg4DrcHeavyProf = XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_INVALID;
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_MPEG4_DRC_HEAVY_TARGET_LEVEL:
        hConfig->mpeg4DRCHeavyTrgtLvl = hParam->paramValue._float;
        hConfig->mpeg4DRCHeavyTrgtLvl_present = 1;
        break;

      case PARAMLIST_PARAMETER_MPEG4_PROG_REF_LEVEL:
        hConfig->mpeg4ProgRefLevel = hParam->paramValue._float;
        hConfig->bMpeg4ProgRefLevelSet = 1;
        break;

      case PARAMLIST_PARAMETER_MPEG4_NO_START_STOP_SEQUENCE:
        switch (hParam->paramValue._int) {
          case PARAMLIST_NO_START_STOP_OFF:
            hConfig->bMpeg4NoStartStopSequence = 0;
            break;
          case PARAMLIST_NO_START_STOP_ON:
            hConfig->bMpeg4NoStartStopSequence = 1;
            break;
          default:
            assert(0);
            hConfig->bMpeg4NoStartStopSequence = 0;
            break;
        }
        break;

      case PARAMLIST_PARAMETER_MPEG4_CENTER_MIX_LEVEL:
        hConfig->bMpeg4CenterMixLvlSet = 1;
        switch (hParam->paramValue._int) {
          case PARAMLIST_MPEG4_DMX_GAIN_0_dB:
            hConfig->mpeg4CenterMixLvl = 0;
            break;
          case PARAMLIST_MPEG4_DMX_GAIN_1_5_dB:
            hConfig->mpeg4CenterMixLvl = 1;
            break;
          case PARAMLIST_MPEG4_DMX_GAIN_3_dB:
            hConfig->mpeg4CenterMixLvl = 2;
            break;
          case PARAMLIST_MPEG4_DMX_GAIN_4_5_dB:
            hConfig->mpeg4CenterMixLvl = 3;
            break;
          case PARAMLIST_MPEG4_DMX_GAIN_6_dB:
            hConfig->mpeg4CenterMixLvl = 4;
            break;
          case PARAMLIST_MPEG4_DMX_GAIN_7_5_dB:
            hConfig->mpeg4CenterMixLvl = 5;
            break;
          case PARAMLIST_MPEG4_DMX_GAIN_9_dB:
            hConfig->mpeg4CenterMixLvl = 6;
            break;
          case PARAMLIST_MPEG4_DMX_GAIN_INF:
            hConfig->mpeg4CenterMixLvl = 7;
            break;
          default:
            assert(0);
            hConfig->bMpeg4CenterMixLvlSet = 0;
            break;
        }
        break;

      case PARAMLIST_PARAMETER_MPEG4_SURROUND_MIX_LEVEL:
        hConfig->bMpeg4SurroundMixLvlSet = 1;
        switch (hParam->paramValue._int) {
          case PARAMLIST_MPEG4_DMX_GAIN_0_dB:
            hConfig->mpeg4SurroundMixLvl = 0;
            break;
          case PARAMLIST_MPEG4_DMX_GAIN_1_5_dB:
            hConfig->mpeg4SurroundMixLvl = 1;
            break;
          case PARAMLIST_MPEG4_DMX_GAIN_3_dB:
            hConfig->mpeg4SurroundMixLvl = 2;
            break;
          case PARAMLIST_MPEG4_DMX_GAIN_4_5_dB:
            hConfig->mpeg4SurroundMixLvl = 3;
            break;
          case PARAMLIST_MPEG4_DMX_GAIN_6_dB:
            hConfig->mpeg4SurroundMixLvl = 4;
            break;
          case PARAMLIST_MPEG4_DMX_GAIN_7_5_dB:
            hConfig->mpeg4SurroundMixLvl = 5;
            break;
          case PARAMLIST_MPEG4_DMX_GAIN_9_dB:
            hConfig->mpeg4SurroundMixLvl = 6;
            break;
          case PARAMLIST_MPEG4_DMX_GAIN_INF:
            hConfig->mpeg4SurroundMixLvl = 7;
            break;
          default:
            assert(0);
            hConfig->bMpeg4SurroundMixLvlSet = 0;
            break;
        }
        break;

      case PARAMLIST_PARAMETER_MPEG4_WRITE_PCE_MIXDOWN_IDX:
        hConfig->bMpeg4WritePceMixDnLvlSet = 1;
        switch (hParam->paramValue._int) {
          case PARAMLIST_MPEG4_WRITE_PCE_MIXDN_OFF:
            hConfig->mpeg4WritePceMixDnLvl = 0;
            break;
          case PARAMLIST_MPEG4_WRITE_PCE_MIXDN_ON:
            hConfig->mpeg4WritePceMixDnLvl = 1;
            break;
          default:
            assert(0);
            hConfig->bMpeg4WritePceMixDnLvlSet = 0;
            break;
        }
        break;

      case PARAMLIST_PARAMETER_MPEG4_ETSI_DWNMIX_PRESENT:
        hConfig->bMpeg4EtsiDmxPresentSet = 1;
        switch (hParam->paramValue._int) {
          case PARAMLIST_MPEG4_ETSIDMXPRESENT_FALSE:
            hConfig->mpeg4EtsiDmxPresent = 0;
            break;
          case PARAMLIST_MPEG4_ETSIDMXPRESENT_TRUE:
            hConfig->mpeg4EtsiDmxPresent = 1;
            break;
          default:
            assert(0);
            hConfig->bMpeg4EtsiDmxPresentSet = 0;
            break;
        }
        break;

      case PARAMLIST_PARAMETER_MPEG4_DOLBY_SURROUND_MODE:
        hConfig->bMpeg4DolbySurroundModeSet = 1;
        switch (hParam->paramValue._int) {
          case PARAMLIST_MPEG4_DSUR_NOT_INDICATED:
            hConfig->mpeg4DolbySurroundMode = 0;
            break;
          case PARAMLIST_MPEG4_DSUR_NOT_USED:
            hConfig->mpeg4DolbySurroundMode = 1;
            break;
          case PARAMLIST_MPEG4_DSUR_IS_USED:
            hConfig->mpeg4DolbySurroundMode = 2;
            break;
          default:
            assert(0);
            hConfig->bMpeg4DolbySurroundModeSet = 0;
            break;
        }
        break;

      case PARAMLIST_PARAMETER_MPEG4_PSEUDO_SUR_DMX_ENABLE:
        hConfig->bMpeg4DolbyPseudoSurDmxEnaSet = 1;
        switch (hParam->paramValue._int) {
          case PARAMLIST_MPEG4_PSEUDO_SUR_OFF:
            hConfig->mpeg4DolbyPseudoSurDmxEna = 0;
            break;
          case PARAMLIST_MPEG4_PSEUDO_SUR_ON:
            hConfig->mpeg4DolbyPseudoSurDmxEna = 1;
            break;
          default:
            assert(0);
            hConfig->bMpeg4DolbyPseudoSurDmxEnaSet = 0;
            break;
        }
        break;

      case PARAMLIST_PARAMETER_MPEG4_DRC_PRES_MODE:
        hConfig->bMpeg4DrcPresModeSet = 1;
        switch (hParam->paramValue._int) {
          case PARAMLIST_MPEG4_DRCPRESENTATION_NOT_INDICATED:
            hConfig->mpeg4DrcPresMode = 0;
            break;
          case PARAMLIST_MPEG4_DRCPRESENTATION_MODE_1:
            hConfig->mpeg4DrcPresMode = 1;
            break;
          case PARAMLIST_MPEG4_DRCPRESENTATION_MODE_2:
            hConfig->mpeg4DrcPresMode = 2;
            break;
          default:
            assert(0);
            hConfig->bMpeg4DrcPresModeSet = 0;
            break;
        }
        break;

      case PARAMLIST_PARAMETER_MPEG4_METADATA_MODE:
        hConfig->metadataModeSet = 1;
        switch (hParam->paramValue._int) {
          case PARAMLIST_MPEG4_METADATA_MODE_NONE:
            hConfig->metadataMode = METADATA_NONE;
            break;
          case PARAMLIST_MPEG4_METADATA_MODE_MPEG:
            hConfig->metadataMode = METADATA_MPEG;
            break;
          case PARAMLIST_MPEG4_METADATA_MODE_MPEG_ETSI:
            hConfig->metadataMode = METADATA_MPEG_ETSI;
            break;
          default:
            assert(0);
            hConfig->metadataModeSet = 0;
            break;
        }
        break;

      case PARAMLIST_PARAMETER_MPEG4_DRC_EXT_DRC_GAIN:
        hConfig->mpeg4DrcGain = hParam->paramValue._float;
        break;

      case PARAMLIST_PARAMETER_MPEG4_DRC_EXT_COMP_GAIN:
        hConfig->mpeg4CompGain = hParam->paramValue._float;
        break;

      case PARAMLIST_PARAMETER_OUTSAMPLERATE:
        switch (hParam->paramValue._int) {
          case PARAMLIST_SAMPLERATE_192000:
            hConfig->sampleRateOut = 192000;
            break;
          case PARAMLIST_SAMPLERATE_176400:
            hConfig->sampleRateOut = 176400;
            break;
          case PARAMLIST_SAMPLERATE_96000:
            hConfig->sampleRateOut = 96000;
            break;
          case PARAMLIST_SAMPLERATE_88200:
            hConfig->sampleRateOut = 88200;
            break;
          case PARAMLIST_SAMPLERATE_64000:
            hConfig->sampleRateOut = 64000;
            break;
          case PARAMLIST_SAMPLERATE_48000:
            hConfig->sampleRateOut = 48000;
            break;
          case PARAMLIST_SAMPLERATE_44100:
            hConfig->sampleRateOut = 44100;
            break;
          case PARAMLIST_SAMPLERATE_40000:
            hConfig->sampleRateOut = 40000;
            break;
          case PARAMLIST_SAMPLERATE_38400:
            hConfig->sampleRateOut = 38400;
            break;
          case PARAMLIST_SAMPLERATE_35280:
            hConfig->sampleRateOut = 35280;
            break;
          case PARAMLIST_SAMPLERATE_32000:
            hConfig->sampleRateOut = 32000;
            break;
          case PARAMLIST_SAMPLERATE_29400:
            hConfig->sampleRateOut = 29400;
            break;
          case PARAMLIST_SAMPLERATE_24000:
            hConfig->sampleRateOut = 24000;
            break;
          case PARAMLIST_SAMPLERATE_22050:
            hConfig->sampleRateOut = 22050;
            break;
          case PARAMLIST_SAMPLERATE_19200:
            hConfig->sampleRateOut = 19200;
            break;
          case PARAMLIST_SAMPLERATE_16000:
            hConfig->sampleRateOut = 16000;
            break;
          case PARAMLIST_SAMPLERATE_12000:
            hConfig->sampleRateOut = 12000;
            break;
          case PARAMLIST_SAMPLERATE_11025:
            hConfig->sampleRateOut = 11025;
            break;
          case PARAMLIST_SAMPLERATE_9600:
            hConfig->sampleRateOut = 9600;
            break;
          case PARAMLIST_SAMPLERATE_8000:
            hConfig->sampleRateOut = 8000;
            break;
          case PARAMLIST_SAMPLERATE_6000:
            hConfig->sampleRateOut = 6000;
            break;
          case PARAMLIST_SAMPLERATE_INVALID:
          default:
            hConfig->sampleRateOut = -1;
            assert(0);
            break;
        }
        break;

      case PARAMLIST_PARAMETER_INTERNAL_LOUDNESS_VERIFICATION:
        break;

      case PARAMLIST_PARAMETER_INTERNAL_MEASURED_LOUDNESS:
        hConfig->loudnessLevel = hParam->paramValue._float;
        hConfig->bLoudnessLevelSet = 1;
        break;

      case PARAMLIST_PARAMETER_INTERNAL_MEASURED_SAMPLE_PEAK:
        hConfig->samplePeak = hParam->paramValue._float;
        hConfig->samplePeakPresent = 1;
        break;

      case PARAMLIST_PARAMETER_DEVELOPER:
      case PARAMLIST_PARAMETER_ADVANCED:
      case PARAMLIST_PARAMETER_ADVANCED_LIVE_ENCODING:
      case PARAMLIST_PARAMETER_ADVANCED_DRC_CONTROL:
      case PARAMLIST_PARAMETER_ADVANCED_LOUDNESS_CONTROL_1:
      case PARAMLIST_PARAMETER_ADVANCED_LOUDNESS_CONTROL_2:
      case PARAMLIST_PARAMETER_ADVANCED_LEGACY_AAC_SUPPORT:
      case PARAMLIST_PARAMETER_ADVANCED_RAP_CONTROL:
      case PARAMLIST_PARAMETER_ADVANCED_RATE_CONTROL:
      case PARAMLIST_PARAMETER_ADVANCED_CONT_BITRATE:
      case PARAMLIST_PARAMETER_ADVANCED_BACKWARDS_COMPATIBILITY:
      case PARAMLIST_PARAMETER_ADVANCED_IF_CONTROL:
      case PARAMLIST_PARAMETER_ADVANCED_MESSAGE_CALLBACK:
      case PARAMLIST_PARAMETER_CONFIGSTATUS:
      case PARAMLIST_PARAMETER_ALLOUTSAMPLERATE:
      case PARAMLIST_PARAMETER_DISABLE_LOUDNESS_MEASUREMENT:

        break;

      default:

        assert(0);
        break;
    }
  }

  return error;
}

static unsigned int iisxHEAACEncLib_SigMap_CicpIndexToChannels(XHEAACENCLIB_SIGMAP_INDEX CicpIndex) {
  unsigned int nChannels;

  switch (CicpIndex) {
    case XHEAACENCLIB_SIGMAP_CICP_1:
      nChannels = 1;
      break;

    case XHEAACENCLIB_SIGMAP_CICP_2:
      nChannels = 2;
      break;

    case XHEAACENCLIB_SIGMAP_CICP_5:
      nChannels = 5;
      break;

    case XHEAACENCLIB_SIGMAP_CICP_6:
      nChannels = 6;
      break;

    case XHEAACENCLIB_SIGMAP_CICP_7:
    case XHEAACENCLIB_SIGMAP_CICP_12:
    case XHEAACENCLIB_SIGMAP_CICP_14:
      nChannels = 8;
      break;

    case XHEAACENCLIB_SIGMAP_CICP_13:
      nChannels = 24;
      break;

    default:
      nChannels = 0;
  }

  return nChannels;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_ConfigNew(
    XHEAACENCLIB_CONFIG_HANDLE *phConfig) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  XHEAACENCLIB_CONFIG_HANDLE hConfig = NULL;

  if (phConfig == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if ((*phConfig) == NULL) {
      (*phConfig) = (XHEAACENCLIB_CONFIG_HANDLE)iisMalloc(sizeof(XHEAACENCLIB_CONFIG));
    }
    if (*phConfig != NULL) {
      hConfig = (*phConfig);
    } else {
      retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
    }
  }

  if (!isError(retValue)) {
    memset(hConfig, 0, sizeof(XHEAACENCLIB_CONFIG));
  }

  if (!isError(retValue)) {
    hConfig->configSet = CONFIG_SET_MP4;
    hConfig->loasNrSubFrames = -1;
    hConfig->randomAccessIntervalSamples = -1;
    hConfig->randomAccessIntervalMs = -1;
    hConfig->randomAccessIntervalMin = -1;
    hConfig->randomAccessIntervalInFrames = -1;
    hConfig->coreMode = XHEAACENCLIB_CODING_MODE_FD;
    hConfig->bUseSBR = 0;
    hConfig->sbrRatio.upFac = 1;
    hConfig->sbrRatio.downFac = 1;
    hConfig->useNoiseFilling = 0;
    hConfig->bUseTNS = -1;
    hConfig->lowDelaySwitching = 0;
    hConfig->optimizedSpeedPulseSearch = 1;
    hConfig->bitRateFractRemainder = 0;
    hConfig->bitRateFractTimeBase = 1;

    hConfig->primingMode = XHEAACENC_PRIMINGMODE_INVALID;
    hConfig->rapProperty = XHEAACENCLIB_RAP_PROPERTY_INVALID;
    hConfig->rapOccurrence = XHEAACENCLIB_RAP_OCCURRENCE_INVALID;
    hConfig->additionalSyncFlushing = 0;

    hConfig->audioPreRollNumAU = -1;
    hConfig->audioPreRollBitResMode = XHEAACENCLIB_APR_BITRESMODE_INVALID;
    hConfig->streamID = -1;

    hConfig->mpegsSetParamBands = 0;
    hConfig->useMpegsHighRateMode = TOOL_MODE_DEFAULT;
    hConfig->useSbrFixBorderForIndepFlag = TOOL_MODE_DEFAULT;
    hConfig->bUseTSD = 0;
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_ConfigDelete(
    XHEAACENCLIB_CONFIG_HANDLE *phConfig) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (phConfig != NULL) {
    if ((*phConfig) != NULL) {
      iisFree(*phConfig);
    }
  }

  return retValue;
}

static AACENC_BITRATE_MODE BitrateMode2AacEncBitrateMode(XHEAACENCLIB_BITRATE_MODE bitrateMode) {
  AACENC_BITRATE_MODE aacBitrateMode;

  switch (bitrateMode) {
    case XHEAACENCLIB_BR_MODE_CBR:
      aacBitrateMode = AACENC_BR_MODE_CBR;
      break;
    case XHEAACENCLIB_BR_MODE_VBR_0:
      aacBitrateMode = AACENC_BR_MODE_VBR_0;
      break;
    case XHEAACENCLIB_BR_MODE_VBR_1:
      aacBitrateMode = AACENC_BR_MODE_VBR_1;
      break;
    case XHEAACENCLIB_BR_MODE_VBR_2:
      aacBitrateMode = AACENC_BR_MODE_VBR_2;
      break;
    case XHEAACENCLIB_BR_MODE_VBR_3:
      aacBitrateMode = AACENC_BR_MODE_VBR_3;
      break;
    case XHEAACENCLIB_BR_MODE_VBR_4:
      aacBitrateMode = AACENC_BR_MODE_VBR_4;
      break;
    case XHEAACENCLIB_BR_MODE_VBR_5:
      aacBitrateMode = AACENC_BR_MODE_VBR_5;
      break;
    case XHEAACENCLIB_BR_MODE_VBR_6:
      aacBitrateMode = AACENC_BR_MODE_VBR_6;
      break;
    case XHEAACENCLIB_BR_MODE_INVALID:
    default:
      assert(0);
      aacBitrateMode = AACENC_BR_MODE_INVALID;
      break;
  }

  return aacBitrateMode;
}

static AACENC_SETUP_CODEC_TYPE CodecType2AacEncCodecType(XHEAACENCLIB_CODEC_TYPE codecType) {
  AACENC_SETUP_CODEC_TYPE aacCodecType = AACENC_SETUP_CODEC_UNKNOWN;

  switch (codecType) {
    case XHEAACENCLIB_CODEC_AAC:
      aacCodecType = AACENC_SETUP_CODEC_AAC;
      break;
    case XHEAACENCLIB_CODEC_USAC:
      aacCodecType = AACENC_SETUP_CODEC_XHEAAC;
      break;
    default:
      assert(0);
  }

  return aacCodecType;
}

XHEAACENCLIB_RETURN applyConfigurationSettings(XHEAACENCLIB_CONFIG_HANDLE hConfig) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hConfig == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    AACENC_BITRATE_MODE aacBitrateMode = BitrateMode2AacEncBitrateMode(hConfig->bitrateMode);

    hConfig->mpegsDownmixCfg = MPEGS_DOWNMIX_DEFAULT;
    hConfig->mpegsIndependencyTimeInterval = -1.0;
    hConfig->mpegsPayloadMode = MPEGS_PAYLOAD_EMBED;
    hConfig->nInChannels = iisxHEAACEncLib_SigMap_CicpIndexToChannels(hConfig->CicpIndex);

    switch (hConfig->aot) {
      case AUD_OBJ_TYP_PS:
        hConfig->nChannelsCoreCoder = 1;
        hConfig->CicpIndexCoreCoder = XHEAACENCLIB_SIGMAP_CICP_1;
        hConfig->codecType = XHEAACENCLIB_CODEC_AAC;
        break;
      case AUD_OBJ_TYP_USAC:
        hConfig->nChannelsCoreCoder = hConfig->nInChannels;
        hConfig->CicpIndexCoreCoder = hConfig->CicpIndex;
        hConfig->codecType = XHEAACENCLIB_CODEC_USAC;
        break;
      default:

        hConfig->nChannelsCoreCoder = hConfig->nInChannels;
        hConfig->CicpIndexCoreCoder = hConfig->CicpIndex;
        hConfig->codecType = XHEAACENCLIB_CODEC_AAC;
        break;
    }

    AACENC_SETUP_CODEC_TYPE aacCodecType = CodecType2AacEncCodecType(hConfig->codecType);

    switch (hConfig->bitrateMode) {
      case XHEAACENCLIB_BR_MODE_VBR_0:
      case XHEAACENCLIB_BR_MODE_VBR_1:
      case XHEAACENCLIB_BR_MODE_VBR_2:
      case XHEAACENCLIB_BR_MODE_VBR_3:
      case XHEAACENCLIB_BR_MODE_VBR_4:
      case XHEAACENCLIB_BR_MODE_VBR_5:
      case XHEAACENCLIB_BR_MODE_VBR_6:
        hConfig->bitRate = IISAACFENC_GetVBRBitrate(aacBitrateMode, hConfig->cm, aacCodecType);
        break;
      default:
        break;
    }

    if (hConfig->bUseSBR) {
      if (hConfig->bUseSBR41) {
        hConfig->sampleRateAAC = hConfig->sampleRateOut / 4;
      } else if (hConfig->granuleLength == XHEAACENCLIB_GRANULELENGTH_768) {
        hConfig->sampleRateAAC = 3 * hConfig->sampleRateOut / 8;
      } else if (hConfig->bUseDownsampledSbr) {
        hConfig->sampleRateAAC = hConfig->sampleRateOut;
      } else {
        hConfig->sampleRateAAC = hConfig->sampleRateOut / 2;
      }
    } else {
      hConfig->sampleRateAAC = hConfig->sampleRateOut;
    }

    hConfig->bUseSbrQmfInput = 0;
  }

  if (!isError(retValue)) {
    if ((hConfig->transportFormat == TT_LOAS) || (hConfig->transportFormat == TT_LATM) ||
        (hConfig->transportFormat == TT_LOAS_NOSMC) || (hConfig->transportFormat == TT_LATM_NOSMC)) {
      if (((hConfig->aot == AUD_OBJ_TYP_HEAAC) || (hConfig->aot == AUD_OBJ_TYP_PS)) &&
          (hConfig->sbrSignaling == SBR_SIGNALING_EXPL_BC)) {
        retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_PARAMETER;
      }
    }
  }

  if (!isError(retValue)) {
    if (hConfig->aot == AUD_OBJ_TYP_USAC) {
      int usacIndepFlagintervalFrames = hConfig->usacIndepFlagIntervalSamples / hConfig->nFrameSamples;
      if (usacIndepFlagintervalFrames < 1) {
        retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_PARAMETER;
      }
    }
  }

  if (!isError(retValue)) {
    switch (hConfig->configSet) {
      case CONFIG_SET_MP4:
      case CONFIG_SET_DASH:
      case CONFIG_SET_SEEKABLE:
        break;
      case CONFIG_SET_INVALID:
      default:
        retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
        break;
    }
  }

  if (!isError(retValue)) {
    if (hConfig->primingMode != XHEAACENC_PRIMINGMODE_FULL &&
        hConfig->primingMode != XHEAACENC_PRIMINGMODE_NONE && hConfig->primingMode != XHEAACENC_PRIMINGMODE_STDDELAY) {
      retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
    }
  }

  if (!isError(retValue)) {
    switch (hConfig->rapOccurrence) {
      case XHEAACENCLIB_RAP_OCCURRENCE_CONSTANT_INTERVAL:
      case XHEAACENCLIB_RAP_OCCURRENCE_ON_DEMAND:

        if (hConfig->audioPreRollBitResMode != XHEAACENCLIB_APR_BITRESMODE_IN_FAILSAVE_DUMP_PREROLL &&
            hConfig->audioPreRollBitResMode != XHEAACENCLIB_APR_BITRESMODE_OUT) {
          retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
        }
        break;
      case XHEAACENCLIB_RAP_OCCURRENCE_OFF:

        break;
      case XHEAACENCLIB_RAP_OCCURRENCE_INVALID:
      default:
        retValue = XHEAACENCLIB_RETURN_ERROR_WRONG_RAP_OCCURRENCE;
        break;
    }
  }

  if (!isError(retValue)) {
    if (hConfig->randomAccessIntervalSamples >= 0) {
      hConfig->randomAccessIntervalInFrames = hConfig->randomAccessIntervalSamples / hConfig->nFrameSamples;
      assert(hConfig->randomAccessIntervalInFrames >= 0);
      switch (hConfig->nFrameSamples) {
        case FRAMELENGTH_768:
        case FRAMELENGTH_960:
        case FRAMELENGTH_1024:
        case FRAMELENGTH_1920:
        case FRAMELENGTH_2048:
        case FRAMELENGTH_4096:
          if (hConfig->randomAccessIntervalSamples % hConfig->nFrameSamples) {
            retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
          }
          break;
        default:
          retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_FRAME_SIZE;
          break;
      }
    } else {
      hConfig->randomAccessIntervalInFrames = -1;

      if (hConfig->rapOccurrence == XHEAACENCLIB_RAP_OCCURRENCE_CONSTANT_INTERVAL) {
        retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
      }
    }
  }

  return retValue;
}
