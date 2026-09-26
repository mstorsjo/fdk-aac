
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

#include "iisxHEAACEncLib_loaswrapper.h"
#include "mp4audioheader.h"
#include "iisxHEAACEncLib_returnCodes.h"
#include "iisxHEAACEncLib_common.h"

XHEAACENCLIB_RETURN
iisxHEAACEncLibCreate_loaswriter(HANDLE_STREAM_FORMAT* hLoaswriter,
                                 HANDLE_BITBUFFER* hBb,
                                 HANDLE_CODER_CONFIG_INFO hcci,
                                 LOASWRITER_MODE mode) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int lay, prg;
  HANDLE_CODER_CONFIG_INFO tmp_hcci[LATM_MAX_PROGRAMS][MAX_LAYERS] = {{NULL}};

  if (hLoaswriter == NULL || hBb == NULL || hcci == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
#define SAFETY_MARGIN (512)
#define MAX_BB_SIZE (MAX_ENCODER_CHANNELS * 6144 + SAFETY_MARGIN * 8)

    *hBb = CreateBitBuffer(MAX_BB_SIZE);
    if (*hBb == NULL) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
    }
    if (!isError(retValue)) {
      for (lay = 0; lay < MAX_LAYERS; lay++) {
        for (prg = 0; prg < LATM_MAX_PROGRAMS; prg++) {
          tmp_hcci[prg][lay] = &hcci[lay];
          if (prg > 0)
            tmp_hcci[prg][lay]->ascFlag = 0;
        }
      }
      HANDLE_ERROR_INFO errorInfo = noError;
      errorInfo = IIS_LoasWriter_Open(hLoaswriter, mode, tmp_hcci);
      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_LOAS_WRAPPER;
      }
    }
  }
  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibDelete_loaswriter(HANDLE_STREAM_FORMAT hLoaswriter,
                                 HANDLE_BITBUFFER hBb) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  HANDLE_ERROR_INFO errorInfo = noError;
  DeleteBitBuffer(hBb);
  errorInfo = IIS_LoasWriter_Close(hLoaswriter);
  if (errorInfo != noError) {
    retValue = XHEAACENCLIB_RETURN_ERROR_LOAS_WRAPPER;
  }
  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibLoaswriter_setLatmSmcTimeInterval(HANDLE_STREAM_FORMAT hLoaswriter, const int sendSmcTimeInterval) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hLoaswriter == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = IIS_LoasWriter_SetSmcInterval(hLoaswriter, sendSmcTimeInterval);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_LOAS_WRAPPER;
    }
  }
  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibLoaswriter_setSmcOutOfBitres(HANDLE_STREAM_FORMAT hLoaswriter, const int smcOutOfBitres) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hLoaswriter == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = IIS_LoasWriter_SetSmcOutOfBitres(hLoaswriter, smcOutOfBitres);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_LOAS_WRAPPER;
    }
  }
  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibLoaswriter_setNrOfSubframes(HANDLE_STREAM_FORMAT hLoaswriter, const int nrOfSubframes) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hLoaswriter == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = IIS_LoasWriter_SetNrOfSubframes(hLoaswriter, nrOfSubframes);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_LOAS_WRAPPER;
    }
  }
  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibAdvance_loaswriter(
    HANDLE_STREAM_FORMAT hLoaswriter,
    HANDLE_BITBUFFER hBb,
    unsigned char* pIn,
    const int nInputBufSize,
    unsigned char* pOut,
    const int nOutputBufSize,

    int* const pOutputBytes,
    LOASWRITER_SMC_WRITTEN* smcWritten) {
  int i, j;
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int bufferFullness[LATM_MAX_PROGRAMS][MAX_LAYERS];
  HANDLE_BITBUFFER bb[LATM_MAX_PROGRAMS][MAX_LAYERS];
  LOASWRITER_FRAME_INFO loasWriterFrameInfo = {0};

  if (hLoaswriter == NULL || hBb == NULL || pIn == NULL || pOut == NULL || pOutputBytes == NULL || smcWritten == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    for (i = 0; i < LATM_MAX_PROGRAMS; i++) {
      for (j = 0; j < MAX_LAYERS; j++) {
        bufferFullness[i][j] = -1;
        bb[i][j] = NULL;
      }
    }

    for (i = 0; i < nInputBufSize; i++)
      WriteBits(hBb, pIn[i], 8);
    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = IncAuReady(hBb, nInputBufSize * 8);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_LOAS_WRAPPER;
    }
    bb[0][0] = hBb;

    *pOutputBytes = 0;
    if (!isError(retValue)) {
      errorInfo = IIS_LoasWriter_Advance(hLoaswriter,
                                         bb,
                                         bufferFullness,
                                         pOut,
                                         pOutputBytes,
                                         &loasWriterFrameInfo);
      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_LOAS_WRAPPER;
      }

      if (smcWritten != NULL) {
        *smcWritten = loasWriterFrameInfo.smcWritten;
      }
    }

    if (!isError(retValue) && nOutputBufSize < *pOutputBytes) {
      retValue = XHEAACENCLIB_RETURN_ERROR_LOAS_WRAPPER;
    }
  }

  return retValue;
}

unsigned int
iisxHEAACEncLibLoaswriter_countBitDemandHeader(HANDLE_STREAM_FORMAT hLoasStream,
                                               unsigned int streamDataLength, int mode) {
  return IIS_LoasWriter_CountBitDemandHeader(hLoasStream, streamDataLength, mode);
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibLoaswriter_getLatmStreamMuxConfig(HANDLE_STREAM_FORMAT hLoasStream,
                                                 unsigned char* buffer, int* nBits) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hLoasStream == NULL || buffer == NULL || nBits == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = IIS_LoasWriter_getLatmStreamMuxConfig(hLoasStream, buffer, nBits);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_LOAS_WRAPPER;
    }
  }
  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibLoaswriter_triggerSmc(HANDLE_STREAM_FORMAT hLoasStream) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hLoasStream == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = IIS_LoasWriter_triggerSmc(hLoasStream);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_LOAS_WRAPPER;
    }
  }
  return retValue;
}
