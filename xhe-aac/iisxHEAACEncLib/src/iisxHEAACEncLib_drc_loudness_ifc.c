
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mathlib.h"
#include "iisxHEAACEncLib_drc_loudness_ifc.h"
#include "iisxHEAACEncLib_common.h"
#include "time_buffer.h"
#include "iisDRCGainEnc_api.h"

#define MAX_LOUDNESS_ENC_PAYLOAD_BYTES 512
#define MAX_LOUDNESS_ENC_PAYLOAD_BITS (MAX_LOUDNESS_ENC_PAYLOAD_BYTES << 3)
#define PRIVATE_DATA_OFFSET_BYTES (sizeof(void *) - sizeof(struct loudnessencoder_struct) % sizeof(void *))

typedef struct drc_loudness_ifc_private_data_struct {
  int nSize;
  unsigned char pLoudnessInfoSetBs[MAX_LOUDNESS_ENC_PAYLOAD_BYTES];
  int iLoudnessInfoSetLength;
  HANDLE_IISDRCGAINENC_PARAMS hDRCLoudnessEnc;
  LoudnessInfoSet *pLoudnessInfoSet;
  IISDRCGAINENC_CONFIG *config_drcGainEnc;

} DRC_LOUDNESS_IFC_PRIVATE_DATA, *DRC_LOUDNESS_IFC_PRIVATE_DATA_HANDLE;

static DRC_LOUDNESS_IFC_PRIVATE_DATA_HANDLE iisxHEAACEncLib_drc_loudness_ifc_GetPrivateDataHandle(
    XHEAACENCLIB_HANDLE_DRC_LOUDNESS const hInstance);

DRC_LOUDNESS_IFC_RETURN iisxHEAACEncLib_drc_loudness_ifc_new(
    XHEAACENCLIB_HANDLE_DRC_LOUDNESS *phInstance) {
  DRC_LOUDNESS_IFC_RETURN retValueDrcLoudness = DRC_LOUDNESS_IFC_NO_ERROR;
  int nSize = 0;

  DRC_LOUDNESS_IFC_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  nSize += sizeof(struct loudnessencoder_struct);
  nSize += PRIVATE_DATA_OFFSET_BYTES;
  nSize += sizeof(struct drc_loudness_ifc_private_data_struct);

  *phInstance = (XHEAACENCLIB_HANDLE_DRC_LOUDNESS)iisMalloc(nSize);
  if (*phInstance != NULL) {
    memset(*phInstance, 0, nSize);
    hPrivateData = (DRC_LOUDNESS_IFC_PRIVATE_DATA_HANDLE)((char *)(*phInstance) + sizeof(struct loudnessencoder_struct) + PRIVATE_DATA_OFFSET_BYTES);
    hPrivateData->nSize = nSize;
    hPrivateData->pLoudnessInfoSet = iisCalloc(1, sizeof(LoudnessInfoSet));
    hPrivateData->config_drcGainEnc = iisCalloc(1, sizeof(IISDRCGAINENC_CONFIG));
  } else {
    retValueDrcLoudness = DRC_LOUDNESS_IFC_ERROR_MEMORY;
  }
  return retValueDrcLoudness;
}

DRC_LOUDNESS_IFC_RETURN iisxHEAACEncLib_drc_loudness_ifc_delete(
    XHEAACENCLIB_HANDLE_DRC_LOUDNESS hInstance) {
  DRC_LOUDNESS_IFC_RETURN retValueDrcLoudness = DRC_LOUDNESS_IFC_NO_ERROR;

  DRC_LOUDNESS_IFC_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  int k;

  if (hInstance == NULL) {
    retValueDrcLoudness = DRC_LOUDNESS_IFC_ERROR_INVALID_HANDLE;
  }

  if (retValueDrcLoudness == DRC_LOUDNESS_IFC_NO_ERROR) {
    hPrivateData = iisxHEAACEncLib_drc_loudness_ifc_GetPrivateDataHandle(hInstance);

    for (k = 0; k < hPrivateData->pLoudnessInfoSet->loudnessInfoAlbumCount; k++) {
      iisFree(hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[k].pLoudnessMeasure);
      hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[k].pLoudnessMeasure = NULL;
    }
    iisFree(hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum);
    hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum = NULL;

    for (k = 0; k < hPrivateData->pLoudnessInfoSet->loudnessInfoCount; k++) {
      iisFree(hPrivateData->pLoudnessInfoSet->pLoudnessInfo[k].pLoudnessMeasure);
      hPrivateData->pLoudnessInfoSet->pLoudnessInfo[k].pLoudnessMeasure = NULL;
    }
    iisFree(hPrivateData->pLoudnessInfoSet->pLoudnessInfo);
    hPrivateData->pLoudnessInfoSet->pLoudnessInfo = NULL;

    if (hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtPresent == 1) {
      for (k = 0; k < hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtEq.loudnessInfoV1AlbumCount; k++) {
        iisFree(hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album[k].pLoudnessMeasure);
        hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album[k].pLoudnessMeasure = NULL;
      }
      iisFree(hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album);
      hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album = NULL;

      for (k = 0; k < hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtEq.loudnessInfoV1Count; k++) {
        iisFree(hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1[k].pLoudnessMeasure);
        hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1[k].pLoudnessMeasure = NULL;
      }
      iisFree(hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1);
      hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1 = NULL;

      iisFree(hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtension.pLoudnessInfoSetExtType);
      hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtension.pLoudnessInfoSetExtType = NULL;
      iisFree(hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtension.pExtBitSize);
      hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtension.pExtBitSize = NULL;
    }

    iisFree(hPrivateData->pLoudnessInfoSet);
    hPrivateData->pLoudnessInfoSet = NULL;
    iisFree(hPrivateData->config_drcGainEnc);
    hPrivateData->config_drcGainEnc = NULL;

    IISDRCGAINENC_RETURN retValueDrcEnc = IISDRCGAINENC_RETURN_NOERROR;
    retValueDrcEnc = iisDRCGainEnc_close(&(hPrivateData->hDRCLoudnessEnc));
    if (retValueDrcEnc != IISDRCGAINENC_RETURN_NOERROR) {
      retValueDrcLoudness = DRC_LOUDNESS_IFC_ERROR_UNKNOWN;
    }

    if (retValueDrcLoudness == DRC_LOUDNESS_IFC_NO_ERROR) {
      iisFree(hInstance);
      hInstance = NULL;
    }
  }
  return retValueDrcLoudness;
}

DRC_LOUDNESS_IFC_RETURN iisxHEAACEncLib_drc_loudness_ifc_init(
    XHEAACENCLIB_HANDLE_DRC_LOUDNESS hInstance,
    XHEAACENCLIB_DRC_LOUDNESS_SETUP setup, LoudnessInfoSet **pLoudnessInfoSet) {
  DRC_LOUDNESS_IFC_RETURN retValueDrcLoudness = DRC_LOUDNESS_IFC_NO_ERROR;

  DRC_LOUDNESS_IFC_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  IISDRCGAINENC_RETURN retValueDrcEnc = IISDRCGAINENC_RETURN_NOERROR;

  int n, m;

  if (hInstance == NULL) {
    retValueDrcLoudness = DRC_LOUDNESS_IFC_ERROR_INVALID_HANDLE;
  }

  if (retValueDrcLoudness == DRC_LOUDNESS_IFC_NO_ERROR) {
    hPrivateData = iisxHEAACEncLib_drc_loudness_ifc_GetPrivateDataHandle(hInstance);

    hPrivateData->pLoudnessInfoSet->loudnessInfoCount = 1;
    n = 0;
    {
      hPrivateData->pLoudnessInfoSet->pLoudnessInfo = (LoudnessInfo *)iisCalloc(hPrivateData->pLoudnessInfoSet->loudnessInfoCount, sizeof(LoudnessInfo));
      if (hPrivateData->pLoudnessInfoSet->pLoudnessInfo == NULL) {
        return DRC_LOUDNESS_IFC_ERROR_MEMORY;
      }
      hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].drcSetId = 0;
      hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].eqSetId = 0;
      hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].downmixId = 0;
      hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].samplePeakLevelPresent = setup.samplePeakPresent;
      hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].samplePeakLevel = setup.samplePeak;
      hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].truePeakLevelPresent = 0;
      hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].truePeakMeasure.truePeakLevel = 0.0f;
      hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].truePeakMeasure.truePeakLevelMeasurementSystem = IISDRCGAINENC_LOUDNESS_MEASUREMENT_SYSTEM_BS1770_4;
      hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].truePeakMeasure.truePeakLevelReliability = IISDRCGAINENC_LOUDNESS_RELIABILITY_MEASURED_AND_ACCURATE;
      hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].measurementCount = 0;
      if (setup.bLoudnessLevelPresent) {
        hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].measurementCount++;
      }
      if (setup.bAnchorLoudnessLevelPresent) {
        hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].measurementCount++;
      }
      if (hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].measurementCount == 0) {
        return DRC_LOUDNESS_IFC_ERROR_INVALID_SETUP;
      }
      hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].pLoudnessMeasure = (LoudnessMeasure *)iisCalloc(hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].measurementCount, sizeof(LoudnessMeasure));
      if (hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].pLoudnessMeasure == NULL) {
        return DRC_LOUDNESS_IFC_ERROR_MEMORY;
      }

      m = 0;
      if (setup.bLoudnessLevelPresent) {
        hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].pLoudnessMeasure[m].methodDefinition = IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_PROGRAM_LOUDNESS;
        hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].pLoudnessMeasure[m].methodValue = setup.loudnessLevel;
        hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].pLoudnessMeasure[m].measurementSystem = IISDRCGAINENC_LOUDNESS_MEASUREMENT_SYSTEM_BS1770_4;
        hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].pLoudnessMeasure[m].reliability = IISDRCGAINENC_LOUDNESS_RELIABILITY_MEASURED_AND_ACCURATE;
        m++;
      }
      if (setup.bAnchorLoudnessLevelPresent) {
        hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].pLoudnessMeasure[m].methodDefinition = IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_ANCHOR_LOUDNESS;
        hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].pLoudnessMeasure[m].methodValue = setup.anchorLoudnessLevel;
        hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].pLoudnessMeasure[m].measurementSystem = IISDRCGAINENC_LOUDNESS_MEASUREMENT_SYSTEM_BS1770_4;
        hPrivateData->pLoudnessInfoSet->pLoudnessInfo[n].pLoudnessMeasure[m].reliability = IISDRCGAINENC_LOUDNESS_RELIABILITY_MEASURED_AND_ACCURATE;
        m++;
      }
    }

    if (setup.bAlbumLoudnessLevelPresent == 1) {
      hPrivateData->pLoudnessInfoSet->loudnessInfoAlbumCount = 1;
      hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum = (LoudnessInfo *)iisCalloc(hPrivateData->pLoudnessInfoSet->loudnessInfoAlbumCount, sizeof(LoudnessInfo));
      if (hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum == NULL) {
        return DRC_LOUDNESS_IFC_ERROR_MEMORY;
      }

      n = 0;
      {
        hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].drcSetId = 0;
        hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].eqSetId = 0;
        hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].downmixId = 0;
        hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].samplePeakLevelPresent = 0;
        hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].samplePeakLevel = 0.0f;
        hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].truePeakLevelPresent = 0;
        hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].truePeakMeasure.truePeakLevel = 0.0f;
        hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].truePeakMeasure.truePeakLevelMeasurementSystem = IISDRCGAINENC_LOUDNESS_MEASUREMENT_SYSTEM_BS1770_4;
        hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].truePeakMeasure.truePeakLevelReliability = IISDRCGAINENC_LOUDNESS_RELIABILITY_MEASURED_AND_ACCURATE;
        hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].measurementCount = 1;
        hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].pLoudnessMeasure = (LoudnessMeasure *)iisCalloc(hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].measurementCount, sizeof(LoudnessMeasure));
        if (hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].pLoudnessMeasure == NULL) {
          return DRC_LOUDNESS_IFC_ERROR_MEMORY;
        }

        m = 0;
        {
          hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].pLoudnessMeasure[m].methodDefinition = IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_PROGRAM_LOUDNESS;
          hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].pLoudnessMeasure[m].methodValue = setup.albumLoudnessLevel;
          hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].pLoudnessMeasure[m].measurementSystem = IISDRCGAINENC_LOUDNESS_MEASUREMENT_SYSTEM_BS1770_4;
          hPrivateData->pLoudnessInfoSet->pLoudnessInfoAlbum[n].pLoudnessMeasure[m].reliability = IISDRCGAINENC_LOUDNESS_RELIABILITY_MEASURED_AND_ACCURATE;
        }
      }
    } else {
      hPrivateData->pLoudnessInfoSet->loudnessInfoAlbumCount = 0;
    }

    hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtPresent = 0;
    hPrivateData->pLoudnessInfoSet->loudnessInfoSetExtension.numLoudnessExtensions = 0;

    *pLoudnessInfoSet = hPrivateData->pLoudnessInfoSet;

    hPrivateData->config_drcGainEnc->pLoudnessInfoSet = hPrivateData->pLoudnessInfoSet;

    retValueDrcEnc = iisDRCGainEnc_open(&hPrivateData->hDRCLoudnessEnc, hPrivateData->config_drcGainEnc, NULL);
    if (retValueDrcEnc != IISDRCGAINENC_RETURN_NOERROR) {
      retValueDrcLoudness = DRC_LOUDNESS_IFC_ERROR_INVALID_SETUP;
    }
  }
  return retValueDrcLoudness;
}

DRC_LOUDNESS_IFC_RETURN iisxHEAACEncLib_GetLoudnessInfoSet(
    XHEAACENCLIB_HANDLE_DRC_LOUDNESS hInstance,
    unsigned char **pLoudnessInfoSetBs,
    int *iLoudnessInfoSetLength) {
  DRC_LOUDNESS_IFC_RETURN retValueDrcLoudness = DRC_LOUDNESS_IFC_NO_ERROR;

  DRC_LOUDNESS_IFC_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  IISDRCGAINENC_RETURN retValueDrcEnc = IISDRCGAINENC_RETURN_NOERROR;
  int bitCount = 0;

  if (hInstance == NULL) {
    retValueDrcLoudness = DRC_LOUDNESS_IFC_ERROR_INVALID_HANDLE;
  }

  if (retValueDrcLoudness == DRC_LOUDNESS_IFC_NO_ERROR) {
    hPrivateData = iisxHEAACEncLib_drc_loudness_ifc_GetPrivateDataHandle(hInstance);

    retValueDrcEnc = iisDRCGainEnc_writeLoudnessInfoSet(hPrivateData->hDRCLoudnessEnc, IISDRCGAINENC_SYNTAXMODE_DEFAULT, hPrivateData->pLoudnessInfoSetBs, &bitCount);
    if (retValueDrcEnc != IISDRCGAINENC_RETURN_NOERROR) {
      retValueDrcLoudness = DRC_LOUDNESS_IFC_ERROR_WRITELOUDNESS;
    }

    hPrivateData->iLoudnessInfoSetLength = (bitCount + 7) / 8;

    assert(hPrivateData->iLoudnessInfoSetLength < MAX_LOUDNESS_ENC_PAYLOAD_BYTES);

    *pLoudnessInfoSetBs = hPrivateData->pLoudnessInfoSetBs;
    *iLoudnessInfoSetLength = hPrivateData->iLoudnessInfoSetLength;
  }
  return retValueDrcLoudness;
}

static DRC_LOUDNESS_IFC_PRIVATE_DATA_HANDLE iisxHEAACEncLib_drc_loudness_ifc_GetPrivateDataHandle(
    XHEAACENCLIB_HANDLE_DRC_LOUDNESS const hInstance) {
  DRC_LOUDNESS_IFC_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  hPrivateData = (DRC_LOUDNESS_IFC_PRIVATE_DATA_HANDLE)((char *)hInstance + sizeof(struct loudnessencoder_struct) + PRIVATE_DATA_OFFSET_BYTES);
  return hPrivateData;
}
