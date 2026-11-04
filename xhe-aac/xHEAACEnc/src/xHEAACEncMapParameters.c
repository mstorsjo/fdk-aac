
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

#include "iisutillib.h"
#include "iisParamList.h"
#include "xHEAACEnc.h"
#include "xHEAACEncCommon.h"
#include "xHEAACEncMapParameters.h"

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapAotToIntern(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case IIS_XHEAACENC_AOT_USAC:
        hParam->paramValue._int = PARAMLIST_AOT_42;
        break;
      default:
        hParam->paramValue._int = PARAMLIST_AOT_INVALID;
        break;
    }

  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapBitrateModeToIntern(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case IIS_XHEAACENC_BITRATEMODE_CBR:
        hParam->paramValue._int = PARAMLIST_BITRATEMODE_CBR;
        break;
      case IIS_XHEAACENC_BITRATEMODE_AAC_VBR0:
        hParam->paramValue._int = PARAMLIST_BITRATEMODE_VBR0;
        break;
      case IIS_XHEAACENC_BITRATEMODE_AAC_VBR1:
        hParam->paramValue._int = PARAMLIST_BITRATEMODE_VBR1;
        break;
      case IIS_XHEAACENC_BITRATEMODE_AAC_VBR2:
        hParam->paramValue._int = PARAMLIST_BITRATEMODE_VBR2;
        break;
      case IIS_XHEAACENC_BITRATEMODE_AAC_VBR3:
        hParam->paramValue._int = PARAMLIST_BITRATEMODE_VBR3;
        break;
      case IIS_XHEAACENC_BITRATEMODE_AAC_VBR4:
        hParam->paramValue._int = PARAMLIST_BITRATEMODE_VBR4;
        break;
      case IIS_XHEAACENC_BITRATEMODE_AAC_VBR5:
        hParam->paramValue._int = PARAMLIST_BITRATEMODE_VBR5;
        break;
      case IIS_XHEAACENC_BITRATEMODE_AAC_VBR6:
        hParam->paramValue._int = PARAMLIST_BITRATEMODE_VBR6;
        break;
      default:
        hParam->paramValue._int = PARAMLIST_BITRATEMODE_INVALID;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapBitrateToIntern(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    if (hParam->paramValue._int < IIS_XHEAACENC_BITRATE_MIN) {
      hParam->paramValue._int = PARAMLIST_BITRATE_INVALID;
    }
    if (hParam->paramValue._int > IIS_XHEAACENC_BITRATE_MAX) {
      hParam->paramValue._int = PARAMLIST_BITRATE_INVALID;
    }

  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapChannelConfigToIntern(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case IIS_XHEAACENC_CHANNELCONFIG_MONO:
        hParam->paramValue._int = PARAMLIST_CHANNELCONFIG_MONO;
        break;
      case IIS_XHEAACENC_CHANNELCONFIG_STEREO:
        hParam->paramValue._int = PARAMLIST_CHANNELCONFIG_STEREO;
        break;
      case IIS_XHEAACENC_CHANNELCONFIG_INVALID:
      default:
        hParam->paramValue._int = PARAMLIST_CHANNELCONFIG_INVALID;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapSampleRateToIntern(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case IIS_XHEAACENC_OUTSAMPLERATE_96000:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_96000;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_88200:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_88200;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_64000:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_64000;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_48000:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_48000;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_44100:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_44100;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_38400:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_38400;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_35280:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_35280;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_32000:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_32000;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_29400:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_29400;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_24000:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_24000;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_22050:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_22050;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_19200:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_19200;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_16000:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_16000;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_12000:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_12000;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_11025:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_11025;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_9600:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_9600;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_8000:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_8000;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_7350:
        hParam->paramValue._int = PARAMLIST_SAMPLERATE_7350;
        break;
      case IIS_XHEAACENC_OUTSAMPLERATE_INVALID:
      default:

        hParam->paramValue._int = PARAMLIST_SAMPLERATE_INVALID;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapLowDelaySwitchingToIntern(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    if (hParam->paramValue._int < 0 || hParam->paramValue._int > 1) {
      hParam->paramValue._int = PARAMLIST_LOWDELAYSWITCHING_INVALID;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }
  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapFrameSamplesToIntern(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case IIS_XHEAACENC_FRAMESAMPLES_768:
        hParam->paramValue._int = PARAMLIST_FRAMESAMPLES_768;
        break;
      case IIS_XHEAACENC_FRAMESAMPLES_960:
        hParam->paramValue._int = PARAMLIST_FRAMESAMPLES_960;
        break;
      case IIS_XHEAACENC_FRAMESAMPLES_1024:
        hParam->paramValue._int = PARAMLIST_FRAMESAMPLES_1024;
        break;
      case IIS_XHEAACENC_FRAMESAMPLES_1920:
        hParam->paramValue._int = PARAMLIST_FRAMESAMPLES_1920;
        break;
      case IIS_XHEAACENC_FRAMESAMPLES_2048:
        hParam->paramValue._int = PARAMLIST_FRAMESAMPLES_2048;
        break;
      case IIS_XHEAACENC_FRAMESAMPLES_4096:
        hParam->paramValue._int = PARAMLIST_FRAMESAMPLES_4096;
        break;
      case IIS_XHEAACENC_FRAMESAMPLES_INVALID:
      default:
        hParam->paramValue._int = PARAMLIST_FRAMESAMPLES_INVALID;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapTransportFormatToIntern(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case IIS_XHEAACENC_TRANSPORTFORMAT_RAW:
        hParam->paramValue._int = PARAMLIST_TRANSPORTFORMAT_RAW;
        break;
      case IIS_XHEAACENC_TRANSPORTFORMAT_ADTS:
        hParam->paramValue._int = PARAMLIST_TRANSPORTFORMAT_ADTS;
        break;
      case IIS_XHEAACENC_TRANSPORTFORMAT_LATMLOAS:
        hParam->paramValue._int = PARAMLIST_TRANSPORTFORMAT_LATMLOAS;
        break;
      case IIS_XHEAACENC_TRANSPORTFORMAT_INVALID:
      default:
        hParam->paramValue._int = PARAMLIST_TRANSPORTFORMAT_INVALID;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapRapOccurrenceToIntern(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case IIS_XHEAACENC_RAP_OCCURRENCE_CONSTANT_INTERVAL:
        hParam->paramValue._int = PARAMLIST_RAP_OCCURRENCE_CONSTANT_INTERVAL;
        break;
      case IIS_XHEAACENC_RAP_OCCURRENCE_ON_DEMAND:
        hParam->paramValue._int = PARAMLIST_RAP_OCCURRENCE_ON_DEMAND;
        break;
      case IIS_XHEAACENC_RAP_OCCURRENCE_INVALID:
      default:
        hParam->paramValue._int = PARAMLIST_RAP_OCCURRENCE_INVALID;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapMpeg2AACToIntern(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case 0:
        hParam->paramValue._int = PARAMLIST_MPEG2AAC_OFF;
        break;
      case 1:
        hParam->paramValue._int = PARAMLIST_MPEG2AAC_ON;
        break;
      default:
        hParam->paramValue._int = PARAMLIST_MPEG2AAC_INVALID;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapFlushingModeToIntern(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case IIS_XHEAACENC_FLUSHINGMODE_DEFAULT:
        hParam->paramValue._int = PARAMLIST_FLUSHINGMODE_DEFAULT;
        break;
      case IIS_XHEAACENC_FLUSHINGMODE_SYNC:
        hParam->paramValue._int = PARAMLIST_FLUSHINGMODE_SYNC;
        break;
      case IIS_XHEAACENC_FLUSHINGMODE_INVALID:
      default:
        hParam->paramValue._int = PARAMLIST_FLUSHINGMODE_INVALID;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapAotToAPI(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case PARAMLIST_AOT_42:
        hParam->paramValue._int = IIS_XHEAACENC_AOT_USAC;
        break;
      default:
        hParam->paramValue._int = IIS_XHEAACENC_AOT_INVALID;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapBitrateModeToAPI(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case PARAMLIST_BITRATEMODE_CBR:
        hParam->paramValue._int = IIS_XHEAACENC_BITRATEMODE_CBR;
        break;
      case PARAMLIST_BITRATEMODE_VBR0:
        hParam->paramValue._int = IIS_XHEAACENC_BITRATEMODE_AAC_VBR0;
        break;
      case PARAMLIST_BITRATEMODE_VBR1:
        hParam->paramValue._int = IIS_XHEAACENC_BITRATEMODE_AAC_VBR1;
        break;
      case PARAMLIST_BITRATEMODE_VBR2:
        hParam->paramValue._int = IIS_XHEAACENC_BITRATEMODE_AAC_VBR2;
        break;
      case PARAMLIST_BITRATEMODE_VBR3:
        hParam->paramValue._int = IIS_XHEAACENC_BITRATEMODE_AAC_VBR3;
        break;
      case PARAMLIST_BITRATEMODE_VBR4:
        hParam->paramValue._int = IIS_XHEAACENC_BITRATEMODE_AAC_VBR4;
        break;
      case PARAMLIST_BITRATEMODE_VBR5:
        hParam->paramValue._int = IIS_XHEAACENC_BITRATEMODE_AAC_VBR5;
        break;
      case PARAMLIST_BITRATEMODE_VBR6:
        hParam->paramValue._int = IIS_XHEAACENC_BITRATEMODE_AAC_VBR6;
        break;
      default:
        hParam->paramValue._int = IIS_XHEAACENC_BITRATEMODE_INVALID;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapChannelConfigToAPI(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case PARAMLIST_CHANNELCONFIG_MONO:
        hParam->paramValue._int = IIS_XHEAACENC_CHANNELCONFIG_MONO;
        break;
      case PARAMLIST_CHANNELCONFIG_STEREO:
        hParam->paramValue._int = IIS_XHEAACENC_CHANNELCONFIG_STEREO;
        break;
      case PARAMLIST_CHANNELCONFIG_INVALID:
      default:
        hParam->paramValue._int = IIS_XHEAACENC_CHANNELCONFIG_INVALID;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapTransportFormatToAPI(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case PARAMLIST_TRANSPORTFORMAT_RAW:
        hParam->paramValue._int = IIS_XHEAACENC_TRANSPORTFORMAT_RAW;
        break;
      case PARAMLIST_TRANSPORTFORMAT_ADTS:
        hParam->paramValue._int = IIS_XHEAACENC_TRANSPORTFORMAT_ADTS;
        break;
      case PARAMLIST_TRANSPORTFORMAT_LATMLOAS:
        hParam->paramValue._int = IIS_XHEAACENC_TRANSPORTFORMAT_LATMLOAS;
        break;
      case PARAMLIST_TRANSPORTFORMAT_INVALID:
      default:
        hParam->paramValue._int = IIS_XHEAACENC_TRANSPORTFORMAT_INVALID;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapRapOccurrenceToAPI(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case PARAMLIST_RAP_OCCURRENCE_CONSTANT_INTERVAL:
        hParam->paramValue._int = IIS_XHEAACENC_RAP_OCCURRENCE_CONSTANT_INTERVAL;
        break;
      case PARAMLIST_RAP_OCCURRENCE_ON_DEMAND:
        hParam->paramValue._int = IIS_XHEAACENC_RAP_OCCURRENCE_ON_DEMAND;
        break;
      case PARAMLIST_RAP_OCCURRENCE_INVALID:
      default:
        hParam->paramValue._int = IIS_XHEAACENC_RAP_OCCURRENCE_INVALID;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapMpeg2AACToAPI(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case PARAMLIST_MPEG2AAC_OFF:
        hParam->paramValue._int = 0;
        break;
      case PARAMLIST_MPEG2AAC_ON:
        hParam->paramValue._int = 1;
        break;
      case PARAMLIST_MPEG2AAC_INVALID:
      default:
        hParam->paramValue._int = -1;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapDisableLoudnessToAPI(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case PARAMLIST_DISABLELOUDNESS_OFF:
        hParam->paramValue._int = 0;
        break;
      case PARAMLIST_DISABLELOUDNESS_ON:
        hParam->paramValue._int = 1;
        break;
      case PARAMLIST_DISABLELOUDNESS_INVALID:
      default:
        hParam->paramValue._int = -1;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapFlushingModeToAPI(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case PARAMLIST_FLUSHINGMODE_DEFAULT:
        hParam->paramValue._int = IIS_XHEAACENC_FLUSHINGMODE_DEFAULT;
        break;
      case PARAMLIST_FLUSHINGMODE_SYNC:
        hParam->paramValue._int = IIS_XHEAACENC_FLUSHINGMODE_SYNC;
        break;
      case PARAMLIST_FLUSHINGMODE_INVALID:
      default:
        hParam->paramValue._int = IIS_XHEAACENC_FLUSHINGMODE_INVALID;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapSampleRateToAPI(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case PARAMLIST_SAMPLERATE_96000:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_96000;
        break;
      case PARAMLIST_SAMPLERATE_88200:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_88200;
        break;
      case PARAMLIST_SAMPLERATE_64000:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_64000;
        break;
      case PARAMLIST_SAMPLERATE_48000:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_48000;
        break;
      case PARAMLIST_SAMPLERATE_44100:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_44100;
        break;
      case PARAMLIST_SAMPLERATE_38400:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_38400;
        break;
      case PARAMLIST_SAMPLERATE_35280:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_35280;
        break;
      case PARAMLIST_SAMPLERATE_32000:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_32000;
        break;
      case PARAMLIST_SAMPLERATE_29400:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_29400;
        break;
      case PARAMLIST_SAMPLERATE_24000:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_24000;
        break;
      case PARAMLIST_SAMPLERATE_22050:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_22050;
        break;
      case PARAMLIST_SAMPLERATE_19200:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_19200;
        break;
      case PARAMLIST_SAMPLERATE_16000:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_16000;
        break;
      case PARAMLIST_SAMPLERATE_12000:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_12000;
        break;
      case PARAMLIST_SAMPLERATE_11025:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_11025;
        break;
      case PARAMLIST_SAMPLERATE_9600:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_9600;
        break;
      case PARAMLIST_SAMPLERATE_8000:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_8000;
        break;
      case PARAMLIST_SAMPLERATE_7350:
        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_7350;
        break;
      case PARAMLIST_SAMPLERATE_192000:
      case PARAMLIST_SAMPLERATE_176400:
      case PARAMLIST_SAMPLERATE_6000:
      case PARAMLIST_SAMPLERATE_INVALID:
      default:

        hParam->paramValue._int = IIS_XHEAACENC_OUTSAMPLERATE_INVALID;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}

HANDLE_ERROR_INFO IIS_xHEAACEnc_MapFrameSamplesToAPI(
    PARAM_INSTANCE_HANDLE hParam) {
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hParam) {
    switch (hParam->paramValue._int) {
      case PARAMLIST_FRAMESAMPLES_768:
        hParam->paramValue._int = IIS_XHEAACENC_FRAMESAMPLES_768;
        break;
      case PARAMLIST_FRAMESAMPLES_960:
        hParam->paramValue._int = IIS_XHEAACENC_FRAMESAMPLES_960;
        break;
      case PARAMLIST_FRAMESAMPLES_1024:
        hParam->paramValue._int = IIS_XHEAACENC_FRAMESAMPLES_1024;
        break;
      case PARAMLIST_FRAMESAMPLES_1920:
        hParam->paramValue._int = IIS_XHEAACENC_FRAMESAMPLES_1920;
        break;
      case PARAMLIST_FRAMESAMPLES_2048:
        hParam->paramValue._int = IIS_XHEAACENC_FRAMESAMPLES_2048;
        break;
      case PARAMLIST_FRAMESAMPLES_4096:
        hParam->paramValue._int = IIS_XHEAACENC_FRAMESAMPLES_4096;
        break;
      case PARAMLIST_FRAMESAMPLES_INVALID:
      default:
        hParam->paramValue._int = IIS_XHEAACENC_FRAMESAMPLES_INVALID;
        break;
    }
  } else {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  return errorInfo;
}
