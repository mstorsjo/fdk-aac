
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

#include "spaceEnclib_const.h"
#include "space_delay.h"

#define DEFAULT_DMX_ALIGN 0
#define DEFAULT_ARBITRARY_DMX 0
#define DEFAULT_TIME_DOM_DMX 0
#define DEFAULT_LIMITER_ENABLE 0
#define DEFAULT_MPS_RESIDUAL_CODING 0
#define DEFAULT_ARBITRARY_DMX_RESIDUAL 0
#define DEFAULT_MINIMIZE_DELAY 0
#define DEFAULT_SAC_TIME_ALIGNMENT_DYNAMIC_OUT 0
#define DEFAULT_SURROUND_DELAY 0
#define DEFAULT_ARB_DMX_DELAY 0
#define DEFAULT_LIMITER_DELAY 0
#define DEFAULT_SAC_TIME_ALIGNMENT 0

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

struct DELAY_CONFIG {
  int bDmxAlign;
  int bArbitraryDmx;
  int bTimeDomDmx;
  int bLowDelay;
  int bLimiterEnable;
  int bMpsResidualCoding;
  int bArbitraryDmxResidual;
  int bMinimizeDelay;
  int bSacTimeAlignmentDynamicOut;

  int bUsac212;

  int nQmfLen;
  int nFrameLen;
  int nResidualCodersLookahead;
  int nSurroundDelay;
  int nArbDmxDelay;
  int nLimiterDelay;
  int nCoreCoderDelay;
  int nSacStreamMuxDelay;
  int nSacTimeAlignment;
};

struct DELAY {
  int bDmxAlign;
  int bArbitraryDmx;
  int bTimeDomDmx;
  int bLowDelay;
  int bLimiterEnable;
  int bMpsResidualCoding;
  int bArbitraryDmxResidual;
  int bMinimizeDelay;
  int bSacTimeAlignmentDynamicOut;

  int bUsac212;

  int nQmfLen;
  int nFrameLen;
  int nResidualCodersLookahead;
  int nSurroundDelay;
  int nArbDmxDelay;
  int nLimiterDelay;
  int nCoreCoderDelay;
  int nSacStreamMuxDelay;
  int nSacTimeAlignment;

  int nEncoderAnDelay;
  int nEncoderSynDelay;
  int nEncoderWinDelay;
  int nDecoderAnDelay;
  int nDecoderSynDelay;
  int nResidualCoderFrameDelay;
  int nArbDmxResidualCoderFrameDelay;

  int nDmxAlignBuffer;
  int nSurroundAnalysisBuffer;
  int nArbDmxAnalysisBuffer;
  int nOutputAudioBuffer;
  int nBitstreamFrameBuffer;
  int nOutputAudioQmfFrameBuffer;
  int nDiscardOutFrames;

  int nBitstreamFrameBufferSize;

  int nInfoDmxDelay;
  int nInfoCodecDelay;
};

static HANDLE_ERROR_INFO Delay_SubCalulateBufferDelays(HANDLE_DELAY hDel);

static HANDLE_ERROR_INFO Delay_calculateDelayComponents(HANDLE_DELAY hDel);

static HANDLE_ERROR_INFO Delay_defaultUsacDelayCalculation(HANDLE_DELAY hDel);

HANDLE_ERROR_INFO
Delay_OpenConfig(HANDLE_DELAY_CONFIG* phDelayConfig,
                 int nQmfLen,
                 int nFrameLen,
                 int nResidualCodersLookahead,
                 int nCoreCoderDelay,
                 int nSacStreamMuxDelay,
                 int bLowDelay,
                 int bUsac212) {
  HANDLE_ERROR_INFO error = noError;
  HANDLE_DELAY_CONFIG hDelCon = NULL;
  if (NULL == phDelayConfig) {
    error = iisUtil_ERROR(CDI, "Invalid Handlepointer");
  }

  if (noError == error) {
    if (NULL != *phDelayConfig) {
      iisFree(*phDelayConfig);
      *phDelayConfig = NULL;
    }
    if (noError == error) {
      *phDelayConfig = (HANDLE_DELAY_CONFIG)iisCalloc(1, sizeof(struct DELAY_CONFIG));
    }
    if (*phDelayConfig == NULL) {
      error = iisUtil_ERROR(CDI, "Memory Allocation Failed");
    } else {
      hDelCon = *phDelayConfig;
    }
  }

  if (noError == error) {
    hDelCon->bDmxAlign = DEFAULT_DMX_ALIGN;
    hDelCon->bArbitraryDmx = DEFAULT_ARBITRARY_DMX;
    hDelCon->bTimeDomDmx = DEFAULT_TIME_DOM_DMX;
    hDelCon->bLowDelay = bLowDelay;
    hDelCon->bUsac212 = bUsac212;
    hDelCon->bLimiterEnable = DEFAULT_LIMITER_ENABLE;
    hDelCon->bMpsResidualCoding = DEFAULT_MPS_RESIDUAL_CODING;
    hDelCon->bArbitraryDmxResidual = DEFAULT_ARBITRARY_DMX_RESIDUAL;
    hDelCon->bMinimizeDelay = DEFAULT_MINIMIZE_DELAY;
    hDelCon->bSacTimeAlignmentDynamicOut = DEFAULT_SAC_TIME_ALIGNMENT_DYNAMIC_OUT;
    hDelCon->nQmfLen = nQmfLen;
    hDelCon->nFrameLen = nFrameLen;
    hDelCon->nResidualCodersLookahead = nResidualCodersLookahead;
    hDelCon->nSurroundDelay = DEFAULT_SURROUND_DELAY;
    hDelCon->nArbDmxDelay = DEFAULT_ARB_DMX_DELAY;
    hDelCon->nLimiterDelay = DEFAULT_LIMITER_DELAY;
    hDelCon->nCoreCoderDelay = nCoreCoderDelay;
    hDelCon->nSacStreamMuxDelay = nSacStreamMuxDelay;
    hDelCon->nSacTimeAlignment = DEFAULT_SAC_TIME_ALIGNMENT;
  }

  return error;
}

HANDLE_ERROR_INFO Delay_CloseConfig(HANDLE_DELAY_CONFIG* phDelayConfig) {
  HANDLE_ERROR_INFO error = noError;
  if (NULL != phDelayConfig) {
    if (NULL != *phDelayConfig) {
      iisFree(*phDelayConfig);
      *phDelayConfig = NULL;
    }
  }

  return error;
}

HANDLE_ERROR_INFO
Delay_Open(HANDLE_DELAY* phDelay, HANDLE_DELAY_CONFIG hDelayConfig) {
  HANDLE_ERROR_INFO error = noError;
  HANDLE_DELAY hDel = NULL;

  if (NULL == phDelay || NULL == hDelayConfig) {
    error = iisUtil_ERROR(CDI, "Invalid Handlepointer");
  }

  if (noError == error) {
    if (NULL != *phDelay) {
      iisFree(*phDelay);
      *phDelay = NULL;
    }
    if (noError == error) {
      *phDelay = (HANDLE_DELAY)iisCalloc(1, sizeof(struct DELAY));
    }
    if (*phDelay == NULL) {
      error = iisUtil_ERROR(CDI, "Memory Allocation Failed");
    } else {
      hDel = *phDelay;
    }
  }

  if (noError == error) {
    hDel->bDmxAlign = hDelayConfig->bDmxAlign;
    hDel->bArbitraryDmx = hDelayConfig->bArbitraryDmx;
    hDel->bTimeDomDmx = hDelayConfig->bTimeDomDmx;
    hDel->bLowDelay = hDelayConfig->bLowDelay;
    hDel->bUsac212 = hDelayConfig->bUsac212;
    hDel->bLimiterEnable = hDelayConfig->bLimiterEnable;
    hDel->bMpsResidualCoding = hDelayConfig->bMpsResidualCoding;
    hDel->bArbitraryDmxResidual = hDelayConfig->bArbitraryDmxResidual;
    hDel->bMinimizeDelay = hDelayConfig->bMinimizeDelay;
    hDel->bSacTimeAlignmentDynamicOut = hDelayConfig->bSacTimeAlignmentDynamicOut;
    hDel->nQmfLen = hDelayConfig->nQmfLen;
    hDel->nFrameLen = hDelayConfig->nFrameLen;
    hDel->nResidualCodersLookahead = hDelayConfig->nResidualCodersLookahead;
    hDel->nSurroundDelay = hDelayConfig->nSurroundDelay;
    hDel->nArbDmxDelay = hDelayConfig->nArbDmxDelay;
    hDel->nLimiterDelay = hDelayConfig->nLimiterDelay;
    hDel->nCoreCoderDelay = hDelayConfig->nCoreCoderDelay;
    hDel->nSacStreamMuxDelay = hDelayConfig->nSacStreamMuxDelay;
    hDel->nSacTimeAlignment = hDelayConfig->nSacTimeAlignment;

    if (hDel->bSacTimeAlignmentDynamicOut > 0) {
      hDel->nSacTimeAlignment = 0;
    }

    error = Delay_SubCalulateBufferDelays(hDel);
  }

  return error;
}

HANDLE_ERROR_INFO Delay_Close(HANDLE_DELAY* phDelay) {
  HANDLE_ERROR_INFO error = noError;
  if (NULL != phDelay) {
    if (NULL != *phDelay) {
      iisFree(*phDelay);
      *phDelay = NULL;
    }
  }

  return error;
}

HANDLE_ERROR_INFO
Delay_SetDmxAlign(HANDLE_DELAY_CONFIG hDelayConfig, int bDmxAlignIn) {
  HANDLE_ERROR_INFO error = noError;
  if (NULL == hDelayConfig) {
    error = iisUtil_ERROR(CDI, "Invalid Handlepointer");
  }
  if (noError == error) {
    if (0 <= bDmxAlignIn && bDmxAlignIn <= 1) {
      hDelayConfig->bDmxAlign = bDmxAlignIn;
    } else {
      error = iisUtil_ERROR(CDI, "Invalid Input Value");
    }
  }
  return error;
}

HANDLE_ERROR_INFO
Delay_SetTimeDomDmx(HANDLE_DELAY_CONFIG hDelayConfig, int bTimeDomDmxIn) {
  HANDLE_ERROR_INFO error = noError;
  if (NULL == hDelayConfig) {
    error = iisUtil_ERROR(CDI, "Invalid Handlepointer");
  }
  if (noError == error) {
    if (0 <= bTimeDomDmxIn && bTimeDomDmxIn <= 1) {
      hDelayConfig->bTimeDomDmx = bTimeDomDmxIn;
    } else {
      error = iisUtil_ERROR(CDI, "Invalid Input Value");
    }
  }
  return error;
}

HANDLE_ERROR_INFO
Delay_SetMpsResidualCoding(HANDLE_DELAY_CONFIG hDelayConfig, int bMpsResidualCodingIn) {
  HANDLE_ERROR_INFO error = noError;
  if (NULL == hDelayConfig) {
    error = iisUtil_ERROR(CDI, "Invalid Handlepointer");
  }
  if (noError == error) {
    if (0 <= bMpsResidualCodingIn && bMpsResidualCodingIn <= 1) {
      hDelayConfig->bMpsResidualCoding = bMpsResidualCodingIn;
    } else {
      error = iisUtil_ERROR(CDI, "Invalid Input Value");
    }
  }
  return error;
}

const int* Delay_GetOutputAudioBufferDelayPtr(HANDLE_DELAY hDelay) {
  return &(hDelay->nOutputAudioBuffer);
}

const int* Delay_GetSurroundAnalysisBufferDelayPtr(HANDLE_DELAY hDelay) {
  return &(hDelay->nSurroundAnalysisBuffer);
}

const int* Delay_GetBitstreamFrameBufferSizePtr(HANDLE_DELAY hDelay) {
  return &(hDelay->nBitstreamFrameBufferSize);
}

const int* Delay_GetDmxAlignBufferDelayPtr(HANDLE_DELAY hDelay) {
  return &(hDelay->nDmxAlignBuffer);
}

const int* Delay_GetDiscardOutFramesPtr(HANDLE_DELAY hDelay) {
  return &(hDelay->nDiscardOutFrames);
}

const int* Delay_GetInfoDmxDelayPtr(HANDLE_DELAY hDelay) {
  return &(hDelay->nInfoDmxDelay);
}

const int* Delay_GetInfoCodecDelayPtr(HANDLE_DELAY hDelay) {
  return &(hDelay->nInfoCodecDelay);
}

static HANDLE_ERROR_INFO Delay_SubCalulateBufferDelays(HANDLE_DELAY hDel) {
  HANDLE_ERROR_INFO error = noError;

  error = Delay_calculateDelayComponents(hDel);

  if (noError == error && hDel->bUsac212 == 1) {
    error = Delay_defaultUsacDelayCalculation(hDel);
  }

  hDel->nBitstreamFrameBufferSize = hDel->nBitstreamFrameBuffer + 1;

  return error;
}

static HANDLE_ERROR_INFO Delay_calculateDelayComponents(HANDLE_DELAY hDel) {
  HANDLE_ERROR_INFO error = noError;

  if (NULL == hDel) {
    error = iisUtil_ERROR(CDI, "Invalid Handlepointer");
    return error;
  }

  if (hDel->bLowDelay == 0) {
    hDel->nEncoderAnDelay = 5 * hDel->nQmfLen + 6 * hDel->nQmfLen;

    hDel->nEncoderSynDelay = 4 * hDel->nQmfLen + 1;

    hDel->nDecoderAnDelay = 5 * hDel->nQmfLen + 5 * hDel->nQmfLen + 6 * hDel->nQmfLen;

    hDel->nDecoderSynDelay = 4 * hDel->nQmfLen + 1;

    hDel->nEncoderWinDelay = hDel->nFrameLen;
  }

  if (hDel->bMpsResidualCoding == 0) {
    hDel->nResidualCoderFrameDelay = 0;
  } else {
    hDel->nResidualCoderFrameDelay = hDel->nResidualCodersLookahead;
  }

  if (hDel->bArbitraryDmxResidual == 0) {
    hDel->nArbDmxResidualCoderFrameDelay = 0;
  } else {
    hDel->nArbDmxResidualCoderFrameDelay = hDel->nResidualCodersLookahead;
  }

  return error;
}

static HANDLE_ERROR_INFO Delay_defaultUsacDelayCalculation(HANDLE_DELAY hDel) {
  HANDLE_ERROR_INFO error = noError;
  int nDecoderDelay = 0;

  if (NULL == hDel) {
    error = iisUtil_ERROR(CDI, "Invalid Handlepointer");
    return error;
  }

  hDel->nDecoderAnDelay = 5 * hDel->nQmfLen +
                          0 +
                          6 * hDel->nQmfLen;

  hDel->nResidualCoderFrameDelay = 0;

  nDecoderDelay = hDel->nDecoderSynDelay +
                  hDel->nDecoderAnDelay +
                  hDel->nCoreCoderDelay +
                  hDel->nLimiterDelay;

  hDel->nBitstreamFrameBuffer = (nDecoderDelay + hDel->nFrameLen - 1) /
                                hDel->nFrameLen;

  hDel->nOutputAudioBuffer = (hDel->nBitstreamFrameBuffer * hDel->nFrameLen) - nDecoderDelay;

  hDel->nInfoDmxDelay = hDel->bDmxAlign ? 0 : (hDel->nEncoderAnDelay + (hDel->nResidualCoderFrameDelay + 1) * hDel->nEncoderWinDelay + hDel->nEncoderSynDelay + hDel->nOutputAudioBuffer);

  hDel->nInfoCodecDelay = (hDel->bDmxAlign ? 0 : hDel->nInfoDmxDelay) + nDecoderDelay;

  return error;
}
