
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
#include <math.h>
#include "iisDRCGainEnc_api.h"
#include "iisDRCGainEnc_bitstreamWriter.h"
#include "iisDRCGainEnc_nodeSetting.h"
#include "iisutillib.h"

static IISDRCGAINENC_RETURN
getDeltaTMin(int *deltaTmin,
             int sampleRate) {
  if (sampleRate >= 1000 && sampleRate < 2000) {
    *deltaTmin = 1;
  } else if (sampleRate >= 2000 && sampleRate < 4000) {
    *deltaTmin = 2;
  } else if (sampleRate >= 4000 && sampleRate < 8000) {
    *deltaTmin = 4;
  } else if (sampleRate >= 8000 && sampleRate < 16000) {
    *deltaTmin = 8;
  } else if (sampleRate >= 16000 && sampleRate < 32000) {
    *deltaTmin = 16;
  } else if (sampleRate >= 32000 && sampleRate < 64000) {
    *deltaTmin = 32;
  } else if (sampleRate >= 64000 && sampleRate < 128000) {
    *deltaTmin = 64;
  } else {
    return IISDRCGAINENC_RETURN_ERROR_UNSUPPORTEDSAMPLERATE;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
checkDrcInstructionsUniDrc(DrcInstructionsUniDrc *pDrcInstructionsUniDrc,
                           int drcInstructionsUniDrcCount) {
  int k, m, n, grpCnt;
  int gainSetIndexForChannelGroup[CHANNEL_COUNT_MAX];

  if (pDrcInstructionsUniDrc == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_NULLPOINTER;
  }

  for (k = 0; k < drcInstructionsUniDrcCount; k++) {
    if ((pDrcInstructionsUniDrc[k].additionalDownmixIdPresent != 0 || pDrcInstructionsUniDrc[k].downmixId == 0x7F) && pDrcInstructionsUniDrc[k].drcChannelCount != 1) {
      return IISDRCGAINENC_RETURN_ERROR_WRONGCHANNELCOUNT;
    }

    if (pDrcInstructionsUniDrc[k].drcSetEffect != IISDRCGAINENC_DRCSETEFFECT_BIT_DUCK_OTHER &&
        pDrcInstructionsUniDrc[k].drcSetEffect != IISDRCGAINENC_DRCSETEFFECT_BIT_DUCK_SELF) {
      grpCnt = 0;
      for (m = 0; m < pDrcInstructionsUniDrc[k].drcChannelCount; m++) {
        gainSetIndexForChannelGroup[grpCnt] = -1;
        if (pDrcInstructionsUniDrc[k].pGainSetIndex[m] >= 0) {
          for (n = 0; n < grpCnt; n++) {
            if (gainSetIndexForChannelGroup[n] == pDrcInstructionsUniDrc[k].pGainSetIndex[m]) {
              break;
            }
          }
          if (n == grpCnt) {
            gainSetIndexForChannelGroup[grpCnt] = pDrcInstructionsUniDrc[k].pGainSetIndex[m];
            grpCnt++;
          }
        }
      }

      if (grpCnt != pDrcInstructionsUniDrc[k].drcChannelGroupCount) {
        return IISDRCGAINENC_RETURN_ERROR_WRONGGROUPCOUNT;
      }
    }
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
checkUniDrcConfig(UniDrcConfig *pUniDrcConfig) {
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;
  int gainSetParamsPresent = 0;

  if (pUniDrcConfig->drcCoefficientsUniDrcCount > 0) {
    if (pUniDrcConfig->pDrcCoefficientsUniDrc[0].gainSetCount > 0) {
      gainSetParamsPresent = 1;
    }
  }
  if (pUniDrcConfig->uniDrcConfigExtension.drcCoefficientsUniDrcV1Count > 0) {
    if (pUniDrcConfig->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1[0].gainSetCount == 0 || gainSetParamsPresent) {
      return IISDRCGAINENC_RETURN_ERROR_WRONGCONFIG;
    }
  }

  if (pUniDrcConfig->uniDrcConfigExtPresent == 1) {
    if (pUniDrcConfig->uniDrcConfigExtension.drcCoeffsAndInstructionsUniDrcV1Present == 1) {
      if (pUniDrcConfig->uniDrcConfigExtension.drcInstructionsUniDrcV1Count > 0) {
        retVal = checkDrcInstructionsUniDrc(pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1, pUniDrcConfig->uniDrcConfigExtension.drcInstructionsUniDrcV1Count);
        if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
      }
    }
  }

  if (pUniDrcConfig->drcInstructionsUniDrcCount > 0) {
    retVal = checkDrcInstructionsUniDrc(pUniDrcConfig->pDrcInstructionsUniDrc, pUniDrcConfig->drcInstructionsUniDrcCount);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
allocAndCopyDrcCoefficientsUniDrc(DrcCoefficientsUniDrc **pDrcCoefficientsUniDrcDst,
                                  DrcCoefficientsUniDrc *pDrcCoefficientsUniDrcSrc,
                                  int drcCoefficientsUniDrcCount) {
  int k, m;

  if (pDrcCoefficientsUniDrcSrc == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_NULLPOINTER;
  }

  *pDrcCoefficientsUniDrcDst = (DrcCoefficientsUniDrc *)iisCalloc(drcCoefficientsUniDrcCount, sizeof(DrcCoefficientsUniDrc));
  if (*pDrcCoefficientsUniDrcDst == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_MEMORY;
  }

  memcpy(*pDrcCoefficientsUniDrcDst, pDrcCoefficientsUniDrcSrc, drcCoefficientsUniDrcCount * sizeof(DrcCoefficientsUniDrc));

  for (k = 0; k < drcCoefficientsUniDrcCount; k++) {
    if (pDrcCoefficientsUniDrcSrc[k].drcCharacteristicLeftPresent == 1) {
      (*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicLeft = (CustomDrcCharacteristic *)iisCalloc(pDrcCoefficientsUniDrcSrc[k].drcCharacteristicLeftCount, sizeof(CustomDrcCharacteristic));
      if ((*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicLeft == NULL) {
        return IISDRCGAINENC_RETURN_ERROR_MEMORY;
      }

      memcpy((*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicLeft, pDrcCoefficientsUniDrcSrc[k].pCustomDrcCharacteristicLeft, pDrcCoefficientsUniDrcSrc[k].drcCharacteristicLeftCount * sizeof(CustomDrcCharacteristic));

      for (m = 0; m < pDrcCoefficientsUniDrcSrc[k].drcCharacteristicLeftCount; m++) {
        if (pDrcCoefficientsUniDrcSrc[k].pCustomDrcCharacteristicLeft[m].charNodeCount > 0) {
          (*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicLeft[m].pNodeGain = (float *)iisCalloc(pDrcCoefficientsUniDrcSrc[k].pCustomDrcCharacteristicLeft[m].charNodeCount, sizeof(float));
          if ((*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicLeft[m].pNodeGain == NULL) {
            return IISDRCGAINENC_RETURN_ERROR_MEMORY;
          }

          memcpy((*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicLeft[m].pNodeGain, pDrcCoefficientsUniDrcSrc[k].pCustomDrcCharacteristicLeft[m].pNodeGain, pDrcCoefficientsUniDrcSrc[k].pCustomDrcCharacteristicLeft[m].charNodeCount * sizeof(float));

          (*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicLeft[m].pNodeLevel = (float *)iisCalloc(pDrcCoefficientsUniDrcSrc[k].pCustomDrcCharacteristicLeft[m].charNodeCount, sizeof(float));
          if ((*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicLeft[m].pNodeLevel == NULL) {
            return IISDRCGAINENC_RETURN_ERROR_MEMORY;
          }

          memcpy((*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicLeft[m].pNodeLevel, pDrcCoefficientsUniDrcSrc[k].pCustomDrcCharacteristicLeft[m].pNodeLevel, pDrcCoefficientsUniDrcSrc[k].pCustomDrcCharacteristicLeft[m].charNodeCount * sizeof(float));
        }
      }
    } else {
      (*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicLeft = NULL;
    }

    if (pDrcCoefficientsUniDrcSrc[k].drcCharacteristicRightPresent == 1) {
      (*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicRight = (CustomDrcCharacteristic *)iisCalloc(pDrcCoefficientsUniDrcSrc[k].drcCharacteristicRightCount, sizeof(CustomDrcCharacteristic));
      if ((*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicRight == NULL) {
        return IISDRCGAINENC_RETURN_ERROR_MEMORY;
      }

      memcpy((*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicRight, pDrcCoefficientsUniDrcSrc[k].pCustomDrcCharacteristicRight, pDrcCoefficientsUniDrcSrc[k].drcCharacteristicRightCount * sizeof(CustomDrcCharacteristic));

      for (m = 0; m < pDrcCoefficientsUniDrcSrc[k].drcCharacteristicRightCount; m++) {
        if (pDrcCoefficientsUniDrcSrc[k].pCustomDrcCharacteristicRight[m].charNodeCount > 0) {
          (*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicRight[m].pNodeGain = (float *)iisCalloc(pDrcCoefficientsUniDrcSrc[k].pCustomDrcCharacteristicRight[m].charNodeCount, sizeof(float));
          if ((*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicRight[m].pNodeGain == NULL) {
            return IISDRCGAINENC_RETURN_ERROR_MEMORY;
          }

          memcpy((*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicRight[m].pNodeGain, pDrcCoefficientsUniDrcSrc[k].pCustomDrcCharacteristicRight[m].pNodeGain, pDrcCoefficientsUniDrcSrc[k].pCustomDrcCharacteristicRight[m].charNodeCount * sizeof(float));

          (*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicRight[m].pNodeLevel = (float *)iisCalloc(pDrcCoefficientsUniDrcSrc[k].pCustomDrcCharacteristicRight[m].charNodeCount, sizeof(float));
          if ((*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicRight[m].pNodeLevel == NULL) {
            return IISDRCGAINENC_RETURN_ERROR_MEMORY;
          }

          memcpy((*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicRight[m].pNodeLevel, pDrcCoefficientsUniDrcSrc[k].pCustomDrcCharacteristicRight[m].pNodeLevel, pDrcCoefficientsUniDrcSrc[k].pCustomDrcCharacteristicRight[m].charNodeCount * sizeof(float));
        }
      }
    } else {
      (*pDrcCoefficientsUniDrcDst)[k].pCustomDrcCharacteristicRight = NULL;
    }

    if (pDrcCoefficientsUniDrcSrc[k].shapeFiltersPresent == 1) {
      (*pDrcCoefficientsUniDrcDst)[k].pShapeFilterParams = (ShapeFilterParams *)iisCalloc(pDrcCoefficientsUniDrcSrc[k].shapeFilterCount, sizeof(ShapeFilterParams));
      if ((*pDrcCoefficientsUniDrcDst)[k].pShapeFilterParams == NULL) {
        return IISDRCGAINENC_RETURN_ERROR_MEMORY;
      }

      memcpy((*pDrcCoefficientsUniDrcDst)[k].pShapeFilterParams, pDrcCoefficientsUniDrcSrc[k].pShapeFilterParams, pDrcCoefficientsUniDrcSrc[k].shapeFilterCount * sizeof(ShapeFilterParams));
    } else {
      (*pDrcCoefficientsUniDrcDst)[k].pShapeFilterParams = NULL;
    }

    if (pDrcCoefficientsUniDrcSrc[k].gainSetCount > 0) {
      (*pDrcCoefficientsUniDrcDst)[k].pGainSetParams = (GainSetParams *)iisCalloc(pDrcCoefficientsUniDrcSrc[k].gainSetCount, sizeof(GainSetParams));
      if ((*pDrcCoefficientsUniDrcDst)[k].pGainSetParams == NULL) {
        return IISDRCGAINENC_RETURN_ERROR_MEMORY;
      }

      memcpy((*pDrcCoefficientsUniDrcDst)[k].pGainSetParams, pDrcCoefficientsUniDrcSrc[k].pGainSetParams, pDrcCoefficientsUniDrcSrc[k].gainSetCount * sizeof(GainSetParams));

      for (m = 0; m < pDrcCoefficientsUniDrcSrc[k].gainSetCount; m++) {
        if (pDrcCoefficientsUniDrcSrc[k].pGainSetParams[m].gainCodingProfile != IISDRCGAINENC_GAINCODINGPROFILE_CONSTANT) {
          (*pDrcCoefficientsUniDrcDst)[k].pGainSetParams[m].pGainSequenceParams = (GainSequenceParams *)iisCalloc(pDrcCoefficientsUniDrcSrc[k].pGainSetParams[m].bandCount, sizeof(GainSequenceParams));
          if ((*pDrcCoefficientsUniDrcDst)[k].pGainSetParams[m].pGainSequenceParams == NULL) {
            return IISDRCGAINENC_RETURN_ERROR_MEMORY;
          }

          memcpy((*pDrcCoefficientsUniDrcDst)[k].pGainSetParams[m].pGainSequenceParams, pDrcCoefficientsUniDrcSrc[k].pGainSetParams[m].pGainSequenceParams, pDrcCoefficientsUniDrcSrc[k].pGainSetParams[m].bandCount * sizeof(GainSequenceParams));
        } else {
          (*pDrcCoefficientsUniDrcDst)[k].pGainSetParams[m].pGainSequenceParams = NULL;
        }
      }
    }
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
allocAndCopyDrcInstructionsUniDrc(DrcInstructionsUniDrc **pDrcInstructionsUniDrcDst,
                                  DrcInstructionsUniDrc *pDrcInstructionsUniDrcSrc,
                                  UniDrcConfig *pUniDrcConfig,
                                  int drcInstructionsUniDrcCount) {
  int k, m, n, grpCnt;
  int gainSetIndexForChannelGroup[CHANNEL_COUNT_MAX];

  if (pDrcInstructionsUniDrcSrc == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_NULLPOINTER;
  }

  *pDrcInstructionsUniDrcDst = (DrcInstructionsUniDrc *)iisCalloc(drcInstructionsUniDrcCount, sizeof(DrcInstructionsUniDrc));
  if (*pDrcInstructionsUniDrcDst == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_MEMORY;
  }

  memcpy(*pDrcInstructionsUniDrcDst, pDrcInstructionsUniDrcSrc, drcInstructionsUniDrcCount * sizeof(DrcInstructionsUniDrc));

  for (k = 0; k < drcInstructionsUniDrcCount; k++) {
    if (pDrcInstructionsUniDrcSrc[k].additionalDownmixIdPresent == 1) {
      (*pDrcInstructionsUniDrcDst)[k].pAdditionalDownmixId = (int *)iisCalloc(pDrcInstructionsUniDrcSrc[k].additionalDownmixIdCount, sizeof(int));
      if ((*pDrcInstructionsUniDrcDst)[k].pAdditionalDownmixId == NULL) {
        return IISDRCGAINENC_RETURN_ERROR_MEMORY;
      }

      memcpy((*pDrcInstructionsUniDrcDst)[k].pAdditionalDownmixId, pDrcInstructionsUniDrcSrc[k].pAdditionalDownmixId, pDrcInstructionsUniDrcSrc[k].additionalDownmixIdCount * sizeof(int));
    } else {
      (*pDrcInstructionsUniDrcDst)[k].pAdditionalDownmixId = NULL;
    }

    (*pDrcInstructionsUniDrcDst)[k].pGainSetIndex = (int *)iisCalloc(pDrcInstructionsUniDrcSrc[k].drcChannelCount, sizeof(int));
    if ((*pDrcInstructionsUniDrcDst)[k].pGainSetIndex == NULL) {
      return IISDRCGAINENC_RETURN_ERROR_MEMORY;
    }

    memcpy((*pDrcInstructionsUniDrcDst)[k].pGainSetIndex, pDrcInstructionsUniDrcSrc[k].pGainSetIndex, pDrcInstructionsUniDrcSrc[k].drcChannelCount * sizeof(int));

    if (pDrcInstructionsUniDrcSrc[k].drcSetEffect == IISDRCGAINENC_DRCSETEFFECT_BIT_DUCK_OTHER ||
        pDrcInstructionsUniDrcSrc[k].drcSetEffect == IISDRCGAINENC_DRCSETEFFECT_BIT_DUCK_SELF) {
      if (pDrcInstructionsUniDrcSrc[k].drcChannelCount > 0) {
        (*pDrcInstructionsUniDrcDst)[k].pDuckingModifications = (DuckingModifications *)iisCalloc(pDrcInstructionsUniDrcSrc[k].drcChannelCount, sizeof(DuckingModifications));
        if ((*pDrcInstructionsUniDrcDst)[k].pDuckingModifications == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }
        memcpy((*pDrcInstructionsUniDrcDst)[k].pDuckingModifications, pDrcInstructionsUniDrcSrc[k].pDuckingModifications, pDrcInstructionsUniDrcSrc[k].drcChannelCount * sizeof(DuckingModifications));
      }

      (*pDrcInstructionsUniDrcDst)[k].pGainModifications = NULL;
    } else {
      if (pDrcInstructionsUniDrcSrc[k].drcChannelGroupCount > 0) {
        (*pDrcInstructionsUniDrcDst)[k].pGainModifications = (GainModifications *)iisCalloc(pDrcInstructionsUniDrcSrc[k].drcChannelGroupCount, sizeof(GainModifications));
        if ((*pDrcInstructionsUniDrcDst)[k].pGainModifications == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }

        memcpy((*pDrcInstructionsUniDrcDst)[k].pGainModifications, pDrcInstructionsUniDrcSrc[k].pGainModifications, pDrcInstructionsUniDrcSrc[k].drcChannelGroupCount * sizeof(GainModifications));

        (*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup = (int *)iisCalloc(pDrcInstructionsUniDrcSrc[k].drcChannelGroupCount, sizeof(int));
        if ((*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }
      }

      grpCnt = 0;
      for (m = 0; m < pDrcInstructionsUniDrcSrc[k].drcChannelCount; m++) {
        gainSetIndexForChannelGroup[grpCnt] = -1;
        if (pDrcInstructionsUniDrcSrc[k].pGainSetIndex[m] >= 0) {
          for (n = 0; n < grpCnt; n++) {
            if (gainSetIndexForChannelGroup[n] == pDrcInstructionsUniDrcSrc[k].pGainSetIndex[m]) {
              break;
            }
          }
          if (n == grpCnt) {
            gainSetIndexForChannelGroup[grpCnt] = pDrcInstructionsUniDrcSrc[k].pGainSetIndex[m];
            grpCnt++;
          }
        }
      }

      for (m = 0; m < grpCnt; m++) {
        if (pUniDrcConfig->drcCoefficientsUniDrcCount > 0) {
          if (pUniDrcConfig->pDrcCoefficientsUniDrc[0].gainSetCount > 0) {
            (*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m] = pUniDrcConfig->pDrcCoefficientsUniDrc[0].pGainSetParams[gainSetIndexForChannelGroup[m]].bandCount;
          }
        }
        if (pUniDrcConfig->uniDrcConfigExtension.drcCoefficientsUniDrcV1Count > 0) {
          if (pUniDrcConfig->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1[0].gainSetCount > 0) {
            (*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m] = pUniDrcConfig->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1[0].pGainSetParams[gainSetIndexForChannelGroup[m]].bandCount;
          }
        }
      }

      for (m = 0; m < pDrcInstructionsUniDrcSrc[k].drcChannelGroupCount; m++) {
        (*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pTargetCharacteristicLeftPresent = (int *)iisCalloc((*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m], sizeof(int));
        if ((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pTargetCharacteristicLeftPresent == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }

        (*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pTargetCharacteristicLeftIndex = (int *)iisCalloc((*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m], sizeof(int));
        if ((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pTargetCharacteristicLeftIndex == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }

        if (pDrcInstructionsUniDrcSrc[k].pGainModifications[m].pTargetCharacteristicLeftPresent != NULL) {
          memcpy((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pTargetCharacteristicLeftPresent, pDrcInstructionsUniDrcSrc[k].pGainModifications[m].pTargetCharacteristicLeftPresent, (*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m] * sizeof(int));
          memcpy((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pTargetCharacteristicLeftIndex, pDrcInstructionsUniDrcSrc[k].pGainModifications[m].pTargetCharacteristicLeftIndex, (*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m] * sizeof(int));
        }

        (*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pTargetCharacteristicRightPresent = (int *)iisCalloc((*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m], sizeof(int));
        if ((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pTargetCharacteristicRightPresent == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }
        (*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pTargetCharacteristicRightIndex = (int *)iisCalloc((*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m], sizeof(int));
        if ((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pTargetCharacteristicRightIndex == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }

        if (pDrcInstructionsUniDrcSrc[k].pGainModifications[m].pTargetCharacteristicRightPresent != NULL) {
          memcpy((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pTargetCharacteristicRightPresent, pDrcInstructionsUniDrcSrc[k].pGainModifications[m].pTargetCharacteristicRightPresent, (*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m] * sizeof(int));
          memcpy((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pTargetCharacteristicRightIndex, pDrcInstructionsUniDrcSrc[k].pGainModifications[m].pTargetCharacteristicRightIndex, (*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m] * sizeof(int));
        }

        (*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pGainScalingPresent = (int *)iisCalloc((*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m], sizeof(int));
        if ((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pGainScalingPresent == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }
        memcpy((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pGainScalingPresent, pDrcInstructionsUniDrcSrc[k].pGainModifications[m].pGainScalingPresent, (*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m] * sizeof(int));

        (*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pAttenuationScaling = (float *)iisCalloc((*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m], sizeof(float));
        if ((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pAttenuationScaling == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }
        memcpy((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pAttenuationScaling, pDrcInstructionsUniDrcSrc[k].pGainModifications[m].pAttenuationScaling, (*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m] * sizeof(float));

        (*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pAmplificationScaling = (float *)iisCalloc((*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m], sizeof(float));
        if ((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pAmplificationScaling == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }
        memcpy((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pAmplificationScaling, pDrcInstructionsUniDrcSrc[k].pGainModifications[m].pAmplificationScaling, (*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m] * sizeof(float));

        (*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pGainOffsetPresent = (int *)iisCalloc((*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m], sizeof(int));
        if ((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pGainOffsetPresent == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }
        memcpy((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pGainOffsetPresent, pDrcInstructionsUniDrcSrc[k].pGainModifications[m].pGainOffsetPresent, (*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m] * sizeof(int));

        (*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pGainOffset = (float *)iisCalloc((*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m], sizeof(float));
        if ((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pGainOffset == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }
        memcpy((*pDrcInstructionsUniDrcDst)[k].pGainModifications[m].pGainOffset, pDrcInstructionsUniDrcSrc[k].pGainModifications[m].pGainOffset, (*pDrcInstructionsUniDrcDst)[k].pBandCountForChannelGroup[m] * sizeof(float));
      }
      (*pDrcInstructionsUniDrcDst)[k].pDuckingModifications = NULL;
    }
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
allocAndCopyDownmixInstructions(DownmixInstructions **pDownmixInstructionsDst,
                                DownmixInstructions *pDownmixInstructionsSrc,
                                int downmixInstructionsCount,
                                int baseChannelCount) {
  int k;

  if (pDownmixInstructionsSrc == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_NULLPOINTER;
  }

  *pDownmixInstructionsDst = (DownmixInstructions *)iisCalloc(downmixInstructionsCount, sizeof(DownmixInstructions));
  if (*pDownmixInstructionsDst == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_MEMORY;
  }

  memcpy(*pDownmixInstructionsDst, pDownmixInstructionsSrc, downmixInstructionsCount * sizeof(DownmixInstructions));

  for (k = 0; k < downmixInstructionsCount; k++) {
    if ((*pDownmixInstructionsDst)[k].downmixCoefficientsPresent == 1) {
      if (pDownmixInstructionsSrc[k].pDownmixCoefficient != NULL) {
        (*pDownmixInstructionsDst)[k].pDownmixCoefficient = (float *)iisCalloc((*pDownmixInstructionsDst)[k].targetChannelCount * baseChannelCount, sizeof(float));
        if ((*pDownmixInstructionsDst)[k].pDownmixCoefficient == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }

        memcpy((*pDownmixInstructionsDst)[k].pDownmixCoefficient, pDownmixInstructionsSrc[k].pDownmixCoefficient, (*pDownmixInstructionsDst)[k].targetChannelCount * baseChannelCount * sizeof(float));

      } else {
        (*pDownmixInstructionsDst)[k].pDownmixCoefficient = NULL;
      }

      if (pDownmixInstructionsSrc[k].pLfeChannel != NULL) {
        (*pDownmixInstructionsDst)[k].pLfeChannel = (int *)iisCalloc((*pDownmixInstructionsDst)[k].targetChannelCount * baseChannelCount, sizeof(int));
        if ((*pDownmixInstructionsDst)[k].pLfeChannel == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }

        memcpy((*pDownmixInstructionsDst)[k].pLfeChannel, pDownmixInstructionsSrc[k].pLfeChannel, (*pDownmixInstructionsDst)[k].targetChannelCount * baseChannelCount * sizeof(float));

      } else {
        (*pDownmixInstructionsDst)[k].pLfeChannel = NULL;
      }
    } else {
      (*pDownmixInstructionsDst)[k].pDownmixCoefficient = NULL;
      (*pDownmixInstructionsDst)[k].pLfeChannel = NULL;
    }
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
allocAndCopyUniDrcConfig(UniDrcConfig **pUniDrcConfigDst,
                         UniDrcConfig *pUniDrcConfigSrc) {
  int k;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  if (pUniDrcConfigSrc == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_NULLPOINTER;
  }

  retVal = checkUniDrcConfig(pUniDrcConfigSrc);
  if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

  *pUniDrcConfigDst = (UniDrcConfig *)iisCalloc(1, sizeof(UniDrcConfig));
  if (*pUniDrcConfigDst == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_MEMORY;
  }

  memcpy(*pUniDrcConfigDst, pUniDrcConfigSrc, sizeof(UniDrcConfig));

  if (pUniDrcConfigSrc->downmixInstructionsCount > 0) {
    retVal = allocAndCopyDownmixInstructions(&(*pUniDrcConfigDst)->pDownmixInstructions, pUniDrcConfigSrc->pDownmixInstructions, pUniDrcConfigSrc->downmixInstructionsCount, pUniDrcConfigSrc->channelLayout.baseChannelCount);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  if (pUniDrcConfigSrc->uniDrcConfigExtPresent == 1) {
    (*pUniDrcConfigDst)->uniDrcConfigExtension.pUniDrcConfigExtType = (IISDRCGAINENC_UNIDRCCONFEXT_TYPE *)iisCalloc(pUniDrcConfigSrc->uniDrcConfigExtension.numUniDrcConfigExtensions, sizeof(IISDRCGAINENC_UNIDRCCONFEXT_TYPE));
    if ((*pUniDrcConfigDst)->uniDrcConfigExtension.pUniDrcConfigExtType == NULL) {
      return IISDRCGAINENC_RETURN_ERROR_MEMORY;
    }

    (*pUniDrcConfigDst)->uniDrcConfigExtension.pExtBitSize = (int *)iisCalloc(pUniDrcConfigSrc->uniDrcConfigExtension.numUniDrcConfigExtensions, sizeof(int));
    if ((*pUniDrcConfigDst)->uniDrcConfigExtension.pExtBitSize == NULL) {
      return IISDRCGAINENC_RETURN_ERROR_MEMORY;
    }

    memcpy((*pUniDrcConfigDst)->uniDrcConfigExtension.pUniDrcConfigExtType, pUniDrcConfigSrc->uniDrcConfigExtension.pUniDrcConfigExtType, pUniDrcConfigSrc->uniDrcConfigExtension.numUniDrcConfigExtensions * sizeof(IISDRCGAINENC_UNIDRCCONFEXT_TYPE));
    memcpy((*pUniDrcConfigDst)->uniDrcConfigExtension.pExtBitSize, pUniDrcConfigSrc->uniDrcConfigExtension.pExtBitSize, pUniDrcConfigSrc->uniDrcConfigExtension.numUniDrcConfigExtensions * sizeof(int));

    if (pUniDrcConfigSrc->uniDrcConfigExtension.downmixInstructionsV1Present == 1) {
      retVal = allocAndCopyDownmixInstructions(&(*pUniDrcConfigDst)->uniDrcConfigExtension.pDownmixInstructionsV1, pUniDrcConfigSrc->uniDrcConfigExtension.pDownmixInstructionsV1, pUniDrcConfigSrc->uniDrcConfigExtension.downmixInstructionsV1Count, pUniDrcConfigSrc->channelLayout.baseChannelCount);
      if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
    }

    if (pUniDrcConfigSrc->uniDrcConfigExtension.drcCoeffsAndInstructionsUniDrcV1Present == 1) {
      if (pUniDrcConfigSrc->uniDrcConfigExtension.drcCoefficientsUniDrcV1Count > 0) {
        retVal = allocAndCopyDrcCoefficientsUniDrc(&(*pUniDrcConfigDst)->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1, pUniDrcConfigSrc->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1, pUniDrcConfigSrc->uniDrcConfigExtension.drcCoefficientsUniDrcV1Count);
        if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
      }

      if (pUniDrcConfigSrc->uniDrcConfigExtension.drcInstructionsUniDrcV1Count > 0) {
        retVal = allocAndCopyDrcInstructionsUniDrc(&(*pUniDrcConfigDst)->uniDrcConfigExtension.pDrcInstructionsUniDrcV1, pUniDrcConfigSrc->uniDrcConfigExtension.pDrcInstructionsUniDrcV1, pUniDrcConfigSrc, pUniDrcConfigSrc->uniDrcConfigExtension.drcInstructionsUniDrcV1Count);
        if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
      }
    }
  } else {
    (*pUniDrcConfigDst)->uniDrcConfigExtension.pUniDrcConfigExtType = NULL;
  }

  if (pUniDrcConfigSrc->drcDescriptionBasicPresent == 1) {
    if (pUniDrcConfigSrc->drcCoefficientsBasicCount > 0) {
      (*pUniDrcConfigDst)->pDrcCoefficientsBasic = (DrcCoefficientsBasic *)iisCalloc(pUniDrcConfigSrc->drcCoefficientsBasicCount, sizeof(DrcCoefficientsBasic));
      if ((*pUniDrcConfigDst)->pDrcCoefficientsBasic == NULL) {
        return IISDRCGAINENC_RETURN_ERROR_MEMORY;
      }

      memcpy((*pUniDrcConfigDst)->pDrcCoefficientsBasic, pUniDrcConfigSrc->pDrcCoefficientsBasic, pUniDrcConfigSrc->drcCoefficientsBasicCount * sizeof(DrcCoefficientsBasic));
    } else {
      (*pUniDrcConfigDst)->pDrcCoefficientsBasic = NULL;
    }

    if (pUniDrcConfigSrc->drcInstructionsBasicCount > 0) {
      (*pUniDrcConfigDst)->pDrcInstructionsBasic = (DrcInstructionsBasic *)iisCalloc(pUniDrcConfigSrc->drcInstructionsBasicCount, sizeof(DrcInstructionsBasic));
      if ((*pUniDrcConfigDst)->pDrcInstructionsBasic == NULL) {
        return IISDRCGAINENC_RETURN_ERROR_MEMORY;
      }

      memcpy((*pUniDrcConfigDst)->pDrcInstructionsBasic, pUniDrcConfigSrc->pDrcInstructionsBasic, pUniDrcConfigSrc->drcInstructionsBasicCount * sizeof(DrcInstructionsBasic));

      for (k = 0; k < pUniDrcConfigSrc->drcInstructionsBasicCount; k++) {
        if (pUniDrcConfigSrc->pDrcInstructionsBasic[k].additionalDownmixIdPresent == 1) {
          (*pUniDrcConfigDst)->pDrcInstructionsBasic[k].pAdditionalDownmixId = (int *)iisCalloc(pUniDrcConfigSrc->pDrcInstructionsBasic[k].additionalDownmixIdCount, sizeof(int));
          if ((*pUniDrcConfigDst)->pDrcInstructionsBasic[k].pAdditionalDownmixId == NULL) {
            return IISDRCGAINENC_RETURN_ERROR_MEMORY;
          }

          memcpy((*pUniDrcConfigDst)->pDrcInstructionsBasic[k].pAdditionalDownmixId, pUniDrcConfigSrc->pDrcInstructionsBasic[k].pAdditionalDownmixId, pUniDrcConfigSrc->pDrcInstructionsBasic[k].additionalDownmixIdCount * sizeof(int));
        } else {
          (*pUniDrcConfigDst)->pDrcInstructionsBasic[k].pAdditionalDownmixId = NULL;
        }
      }
    } else {
      (*pUniDrcConfigDst)->pDrcInstructionsBasic = NULL;
    }
  }

  if (pUniDrcConfigSrc->drcCoefficientsUniDrcCount > 0) {
    retVal = allocAndCopyDrcCoefficientsUniDrc(&(*pUniDrcConfigDst)->pDrcCoefficientsUniDrc, pUniDrcConfigSrc->pDrcCoefficientsUniDrc, pUniDrcConfigSrc->drcCoefficientsUniDrcCount);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  if (pUniDrcConfigSrc->drcInstructionsUniDrcCount > 0) {
    retVal = allocAndCopyDrcInstructionsUniDrc(&(*pUniDrcConfigDst)->pDrcInstructionsUniDrc, pUniDrcConfigSrc->pDrcInstructionsUniDrc, pUniDrcConfigSrc, pUniDrcConfigSrc->drcInstructionsUniDrcCount);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  if (pUniDrcConfigSrc->channelLayout.layoutSignalingPresent == 1 && pUniDrcConfigSrc->channelLayout.definedLayout == 0) {
    (*pUniDrcConfigDst)->channelLayout.pSpeakerPosition = (int *)iisCalloc(pUniDrcConfigSrc->channelLayout.baseChannelCount, sizeof(int));
    if ((*pUniDrcConfigDst)->channelLayout.pSpeakerPosition == NULL) {
      return IISDRCGAINENC_RETURN_ERROR_MEMORY;
    }
  } else {
    (*pUniDrcConfigDst)->channelLayout.pSpeakerPosition = NULL;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
freeDrcInstructionsUniDrc(DrcInstructionsUniDrc **pDrcInstructionsUniDrc,
                          int drcInstructionsUniDrcCount) {
  int k, m;

  for (k = 0; k < drcInstructionsUniDrcCount; k++) {
    if ((*pDrcInstructionsUniDrc)[k].drcSetEffect == IISDRCGAINENC_DRCSETEFFECT_BIT_DUCK_OTHER ||
        (*pDrcInstructionsUniDrc)[k].drcSetEffect == IISDRCGAINENC_DRCSETEFFECT_BIT_DUCK_SELF) {
      iisFree((*pDrcInstructionsUniDrc)[k].pDuckingModifications);
      (*pDrcInstructionsUniDrc)[k].pDuckingModifications = NULL;
    } else {
      for (m = 0; m < (*pDrcInstructionsUniDrc)[k].drcChannelGroupCount; m++) {
        iisFree((*pDrcInstructionsUniDrc)[k].pGainModifications[m].pGainOffset);
        (*pDrcInstructionsUniDrc)[k].pGainModifications[m].pGainOffset = NULL;
        iisFree((*pDrcInstructionsUniDrc)[k].pGainModifications[m].pGainOffsetPresent);
        (*pDrcInstructionsUniDrc)[k].pGainModifications[m].pGainOffsetPresent = NULL;
        iisFree((*pDrcInstructionsUniDrc)[k].pGainModifications[m].pAmplificationScaling);
        (*pDrcInstructionsUniDrc)[k].pGainModifications[m].pAmplificationScaling = NULL;
        iisFree((*pDrcInstructionsUniDrc)[k].pGainModifications[m].pAttenuationScaling);
        (*pDrcInstructionsUniDrc)[k].pGainModifications[m].pAttenuationScaling = NULL;
        iisFree((*pDrcInstructionsUniDrc)[k].pGainModifications[m].pGainScalingPresent);
        (*pDrcInstructionsUniDrc)[k].pGainModifications[m].pGainScalingPresent = NULL;
        iisFree((*pDrcInstructionsUniDrc)[k].pGainModifications[m].pTargetCharacteristicRightIndex);
        (*pDrcInstructionsUniDrc)[k].pGainModifications[m].pTargetCharacteristicRightIndex = NULL;
        iisFree((*pDrcInstructionsUniDrc)[k].pGainModifications[m].pTargetCharacteristicRightPresent);
        (*pDrcInstructionsUniDrc)[k].pGainModifications[m].pTargetCharacteristicRightPresent = NULL;
        iisFree((*pDrcInstructionsUniDrc)[k].pGainModifications[m].pTargetCharacteristicLeftIndex);
        (*pDrcInstructionsUniDrc)[k].pGainModifications[m].pTargetCharacteristicLeftIndex = NULL;
        iisFree((*pDrcInstructionsUniDrc)[k].pGainModifications[m].pTargetCharacteristicLeftPresent);
        (*pDrcInstructionsUniDrc)[k].pGainModifications[m].pTargetCharacteristicLeftPresent = NULL;
      }

      iisFree((*pDrcInstructionsUniDrc)[k].pBandCountForChannelGroup);
      (*pDrcInstructionsUniDrc)[k].pBandCountForChannelGroup = NULL;
      iisFree((*pDrcInstructionsUniDrc)[k].pGainModifications);
      (*pDrcInstructionsUniDrc)[k].pGainModifications = NULL;
    }
    iisFree((*pDrcInstructionsUniDrc)[k].pGainSetIndex);
    (*pDrcInstructionsUniDrc)[k].pGainSetIndex = NULL;
    iisFree((*pDrcInstructionsUniDrc)[k].pAdditionalDownmixId);
    (*pDrcInstructionsUniDrc)[k].pAdditionalDownmixId = NULL;
  }

  iisFree((*pDrcInstructionsUniDrc));
  (*pDrcInstructionsUniDrc) = NULL;

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
freeDrcCoefficientsUniDrc(DrcCoefficientsUniDrc **pDrcCoefficientsUniDrc,
                          int drcCoefficientsUniDrcCount) {
  int k, m;

  for (k = 0; k < drcCoefficientsUniDrcCount; k++) {
    if ((*pDrcCoefficientsUniDrc)[k].gainSetCount > 0) {
      for (m = 0; m < (*pDrcCoefficientsUniDrc)[k].gainSetCount; m++) {
        iisFree((*pDrcCoefficientsUniDrc)[k].pGainSetParams[m].pGainSequenceParams);
        (*pDrcCoefficientsUniDrc)[k].pGainSetParams[m].pGainSequenceParams = NULL;
      }
      iisFree((*pDrcCoefficientsUniDrc)[k].pGainSetParams);
      (*pDrcCoefficientsUniDrc)[k].pGainSetParams = NULL;
    }

    if ((*pDrcCoefficientsUniDrc)[k].shapeFiltersPresent == 1) {
      iisFree((*pDrcCoefficientsUniDrc)[k].pShapeFilterParams);
      (*pDrcCoefficientsUniDrc)[k].pShapeFilterParams = NULL;
    }

    if ((*pDrcCoefficientsUniDrc)[k].drcCharacteristicRightPresent == 1) {
      for (m = 0; m < (*pDrcCoefficientsUniDrc)[k].drcCharacteristicRightCount; m++) {
        iisFree((*pDrcCoefficientsUniDrc)[k].pCustomDrcCharacteristicRight[m].pNodeLevel);
        (*pDrcCoefficientsUniDrc)[k].pCustomDrcCharacteristicRight[m].pNodeLevel = NULL;
        iisFree((*pDrcCoefficientsUniDrc)[k].pCustomDrcCharacteristicRight[m].pNodeGain);
        (*pDrcCoefficientsUniDrc)[k].pCustomDrcCharacteristicRight[m].pNodeGain = NULL;
      }
      iisFree((*pDrcCoefficientsUniDrc)[k].pCustomDrcCharacteristicRight);
      (*pDrcCoefficientsUniDrc)[k].pCustomDrcCharacteristicRight = NULL;
    }

    if ((*pDrcCoefficientsUniDrc)[k].drcCharacteristicLeftPresent == 1) {
      for (m = 0; m < (*pDrcCoefficientsUniDrc)[k].drcCharacteristicLeftCount; m++) {
        iisFree((*pDrcCoefficientsUniDrc)[k].pCustomDrcCharacteristicLeft[m].pNodeLevel);
        (*pDrcCoefficientsUniDrc)[k].pCustomDrcCharacteristicLeft[m].pNodeLevel = NULL;
        iisFree((*pDrcCoefficientsUniDrc)[k].pCustomDrcCharacteristicLeft[m].pNodeGain);
        (*pDrcCoefficientsUniDrc)[k].pCustomDrcCharacteristicLeft[m].pNodeGain = NULL;
      }
      iisFree((*pDrcCoefficientsUniDrc)[k].pCustomDrcCharacteristicLeft);
      (*pDrcCoefficientsUniDrc)[k].pCustomDrcCharacteristicLeft = NULL;
    }
  }

  iisFree((*pDrcCoefficientsUniDrc));
  (*pDrcCoefficientsUniDrc) = NULL;

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
freeDownmixInstructions(DownmixInstructions **pDownmixInstructions,
                        int downmixInstructionsCount) {
  int k;

  for (k = 0; k < downmixInstructionsCount; k++) {
    if ((*pDownmixInstructions)[k].downmixCoefficientsPresent == 1) {
      iisFree((*pDownmixInstructions)[k].pDownmixCoefficient);
      (*pDownmixInstructions)[k].pDownmixCoefficient = NULL;

      iisFree((*pDownmixInstructions)[k].pLfeChannel);
      (*pDownmixInstructions)[k].pLfeChannel = NULL;
    }
  }
  iisFree(*pDownmixInstructions);
  *pDownmixInstructions = NULL;

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
freeUniDrcConfig(UniDrcConfig **pUniDrcConfig) {
  int k;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  if ((*pUniDrcConfig)->channelLayout.layoutSignalingPresent == 1 && (*pUniDrcConfig)->channelLayout.definedLayout == 0) {
    iisFree((*pUniDrcConfig)->channelLayout.pSpeakerPosition);
    (*pUniDrcConfig)->channelLayout.pSpeakerPosition = NULL;
  }

  if ((*pUniDrcConfig)->drcInstructionsUniDrcCount > 0) {
    retVal = freeDrcInstructionsUniDrc(&(*pUniDrcConfig)->pDrcInstructionsUniDrc, (*pUniDrcConfig)->drcInstructionsUniDrcCount);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  if ((*pUniDrcConfig)->drcCoefficientsUniDrcCount > 0) {
    retVal = freeDrcCoefficientsUniDrc(&(*pUniDrcConfig)->pDrcCoefficientsUniDrc, (*pUniDrcConfig)->drcCoefficientsUniDrcCount);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  if ((*pUniDrcConfig)->drcDescriptionBasicPresent == 1) {
    if ((*pUniDrcConfig)->drcInstructionsBasicCount > 0) {
      for (k = 0; k < (*pUniDrcConfig)->drcInstructionsBasicCount; k++) {
        if ((*pUniDrcConfig)->pDrcInstructionsBasic[k].additionalDownmixIdPresent == 1) {
          iisFree((*pUniDrcConfig)->pDrcInstructionsBasic[k].pAdditionalDownmixId);
          (*pUniDrcConfig)->pDrcInstructionsBasic[k].pAdditionalDownmixId = NULL;
        }
      }
      iisFree((*pUniDrcConfig)->pDrcInstructionsBasic);
      (*pUniDrcConfig)->pDrcInstructionsBasic = NULL;
    }

    if ((*pUniDrcConfig)->drcCoefficientsBasicCount > 0) {
      iisFree((*pUniDrcConfig)->pDrcCoefficientsBasic);
      (*pUniDrcConfig)->pDrcCoefficientsBasic = NULL;
    }
  }

  if ((*pUniDrcConfig)->uniDrcConfigExtPresent == 1) {
    if ((*pUniDrcConfig)->uniDrcConfigExtension.drcCoeffsAndInstructionsUniDrcV1Present == 1) {
      if ((*pUniDrcConfig)->uniDrcConfigExtension.drcInstructionsUniDrcV1Count > 0) {
        retVal = freeDrcInstructionsUniDrc(&(*pUniDrcConfig)->uniDrcConfigExtension.pDrcInstructionsUniDrcV1, (*pUniDrcConfig)->uniDrcConfigExtension.drcInstructionsUniDrcV1Count);
        if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
      }

      if ((*pUniDrcConfig)->uniDrcConfigExtension.drcCoefficientsUniDrcV1Count > 0) {
        retVal = freeDrcCoefficientsUniDrc(&(*pUniDrcConfig)->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1, (*pUniDrcConfig)->uniDrcConfigExtension.drcCoefficientsUniDrcV1Count);
        if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
      }
    }

    if ((*pUniDrcConfig)->uniDrcConfigExtension.downmixInstructionsV1Present == 1) {
      retVal = freeDownmixInstructions(&(*pUniDrcConfig)->uniDrcConfigExtension.pDownmixInstructionsV1, (*pUniDrcConfig)->uniDrcConfigExtension.downmixInstructionsV1Count);
      if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
    }

    iisFree((*pUniDrcConfig)->uniDrcConfigExtension.pExtBitSize);
    (*pUniDrcConfig)->uniDrcConfigExtension.pExtBitSize = NULL;
    iisFree((*pUniDrcConfig)->uniDrcConfigExtension.pUniDrcConfigExtType);
    (*pUniDrcConfig)->uniDrcConfigExtension.pUniDrcConfigExtType = NULL;
  }

  if ((*pUniDrcConfig)->downmixInstructionsCount > 0) {
    retVal = freeDownmixInstructions(&(*pUniDrcConfig)->pDownmixInstructions, (*pUniDrcConfig)->downmixInstructionsCount);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  iisFree(*pUniDrcConfig);
  *pUniDrcConfig = NULL;

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
allocAndCopyLoudnessInfoSet(LoudnessInfoSet **pLoudnessInfoSetDst,
                            LoudnessInfoSet *pLoudnessInfoSetSrc) {
  int k;

  if (pLoudnessInfoSetSrc == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_NULLPOINTER;
  }

  *pLoudnessInfoSetDst = (LoudnessInfoSet *)iisCalloc(1, sizeof(LoudnessInfoSet));
  if (*pLoudnessInfoSetDst == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_MEMORY;
  }

  memcpy(*pLoudnessInfoSetDst, pLoudnessInfoSetSrc, sizeof(LoudnessInfoSet));

  if (pLoudnessInfoSetSrc->loudnessInfoCount > 0) {
    (*pLoudnessInfoSetDst)->pLoudnessInfo = (LoudnessInfo *)iisCalloc(pLoudnessInfoSetSrc->loudnessInfoCount, sizeof(LoudnessInfo));
    if ((*pLoudnessInfoSetDst)->pLoudnessInfo == NULL) {
      return IISDRCGAINENC_RETURN_ERROR_MEMORY;
    }

    memcpy((*pLoudnessInfoSetDst)->pLoudnessInfo, pLoudnessInfoSetSrc->pLoudnessInfo, pLoudnessInfoSetSrc->loudnessInfoCount * sizeof(LoudnessInfo));

    for (k = 0; k < pLoudnessInfoSetSrc->loudnessInfoCount; k++) {
      if (pLoudnessInfoSetSrc->pLoudnessInfo[k].measurementCount > 0) {
        (*pLoudnessInfoSetDst)->pLoudnessInfo[k].pLoudnessMeasure = (LoudnessMeasure *)iisCalloc(pLoudnessInfoSetSrc->pLoudnessInfo[k].measurementCount, sizeof(LoudnessMeasure));
        if ((*pLoudnessInfoSetDst)->pLoudnessInfo[k].pLoudnessMeasure == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }

        memcpy((*pLoudnessInfoSetDst)->pLoudnessInfo[k].pLoudnessMeasure, pLoudnessInfoSetSrc->pLoudnessInfo[k].pLoudnessMeasure, pLoudnessInfoSetSrc->pLoudnessInfo[k].measurementCount * sizeof(LoudnessMeasure));
      } else {
        (*pLoudnessInfoSetDst)->pLoudnessInfo[k].pLoudnessMeasure = NULL;
      }
    }
  } else {
    (*pLoudnessInfoSetDst)->pLoudnessInfo = NULL;
  }

  if (pLoudnessInfoSetSrc->loudnessInfoAlbumCount > 0) {
    (*pLoudnessInfoSetDst)->pLoudnessInfoAlbum = (LoudnessInfo *)iisCalloc(pLoudnessInfoSetSrc->loudnessInfoAlbumCount, sizeof(LoudnessInfo));
    if ((*pLoudnessInfoSetDst)->pLoudnessInfoAlbum == NULL) {
      return IISDRCGAINENC_RETURN_ERROR_MEMORY;
    }

    memcpy((*pLoudnessInfoSetDst)->pLoudnessInfoAlbum, pLoudnessInfoSetSrc->pLoudnessInfoAlbum, pLoudnessInfoSetSrc->loudnessInfoAlbumCount * sizeof(LoudnessInfo));

    for (k = 0; k < pLoudnessInfoSetSrc->loudnessInfoAlbumCount; k++) {
      if (pLoudnessInfoSetSrc->pLoudnessInfoAlbum[k].measurementCount > 0) {
        (*pLoudnessInfoSetDst)->pLoudnessInfoAlbum[k].pLoudnessMeasure = (LoudnessMeasure *)iisCalloc(pLoudnessInfoSetSrc->pLoudnessInfoAlbum[k].measurementCount, sizeof(LoudnessMeasure));
        if ((*pLoudnessInfoSetDst)->pLoudnessInfoAlbum[k].pLoudnessMeasure == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }

        memcpy((*pLoudnessInfoSetDst)->pLoudnessInfoAlbum[k].pLoudnessMeasure, pLoudnessInfoSetSrc->pLoudnessInfoAlbum[k].pLoudnessMeasure, pLoudnessInfoSetSrc->pLoudnessInfoAlbum[k].measurementCount * sizeof(LoudnessMeasure));
      } else {
        (*pLoudnessInfoSetDst)->pLoudnessInfoAlbum[k].pLoudnessMeasure = NULL;
      }
    }
  } else {
    (*pLoudnessInfoSetDst)->pLoudnessInfoAlbum = NULL;
  }

  if (pLoudnessInfoSetSrc->loudnessInfoSetExtPresent) {
    (*pLoudnessInfoSetDst)->loudnessInfoSetExtension.pLoudnessInfoSetExtType = (IISDRCGAINENC_UNIDRCLOUDEXT_TYPE *)iisCalloc(pLoudnessInfoSetSrc->loudnessInfoSetExtension.numLoudnessExtensions, sizeof(IISDRCGAINENC_UNIDRCLOUDEXT_TYPE));
    if ((*pLoudnessInfoSetDst)->loudnessInfoSetExtension.pLoudnessInfoSetExtType == NULL) {
      return IISDRCGAINENC_RETURN_ERROR_MEMORY;
    }

    (*pLoudnessInfoSetDst)->loudnessInfoSetExtension.pExtBitSize = (int *)iisCalloc(pLoudnessInfoSetSrc->loudnessInfoSetExtension.numLoudnessExtensions, sizeof(int));
    if ((*pLoudnessInfoSetDst)->loudnessInfoSetExtension.pExtBitSize == NULL) {
      return IISDRCGAINENC_RETURN_ERROR_MEMORY;
    }

    memcpy((*pLoudnessInfoSetDst)->loudnessInfoSetExtension.pLoudnessInfoSetExtType, pLoudnessInfoSetSrc->loudnessInfoSetExtension.pLoudnessInfoSetExtType, pLoudnessInfoSetSrc->loudnessInfoSetExtension.numLoudnessExtensions * sizeof(IISDRCGAINENC_UNIDRCLOUDEXT_TYPE));
    memcpy((*pLoudnessInfoSetDst)->loudnessInfoSetExtension.pExtBitSize, pLoudnessInfoSetSrc->loudnessInfoSetExtension.pExtBitSize, pLoudnessInfoSetSrc->loudnessInfoSetExtension.numLoudnessExtensions * sizeof(int));

    if (pLoudnessInfoSetSrc->loudnessInfoSetExtension.loudnessInfoSetExtEq.loudnessInfoV1Count > 0) {
      (*pLoudnessInfoSetDst)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1 = (LoudnessInfo *)iisCalloc(pLoudnessInfoSetSrc->loudnessInfoSetExtension.loudnessInfoSetExtEq.loudnessInfoV1Count, sizeof(LoudnessInfo));
      if ((*pLoudnessInfoSetDst)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1 == NULL) {
        return IISDRCGAINENC_RETURN_ERROR_MEMORY;
      }

      memcpy((*pLoudnessInfoSetDst)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1, pLoudnessInfoSetSrc->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1, pLoudnessInfoSetSrc->loudnessInfoSetExtension.loudnessInfoSetExtEq.loudnessInfoV1Count * sizeof(LoudnessInfo));
      for (k = 0; k < pLoudnessInfoSetSrc->loudnessInfoSetExtension.loudnessInfoSetExtEq.loudnessInfoV1Count; k++) {
        (*pLoudnessInfoSetDst)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1[k].pLoudnessMeasure = (LoudnessMeasure *)iisCalloc(pLoudnessInfoSetSrc->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1[k].measurementCount, sizeof(LoudnessMeasure));
        if ((*pLoudnessInfoSetDst)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1[k].pLoudnessMeasure == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }

        memcpy((*pLoudnessInfoSetDst)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1[k].pLoudnessMeasure, pLoudnessInfoSetSrc->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1[k].pLoudnessMeasure, pLoudnessInfoSetSrc->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1[k].measurementCount * sizeof(LoudnessMeasure));
      }
    } else {
      (*pLoudnessInfoSetDst)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1 = NULL;
    }

    if (pLoudnessInfoSetSrc->loudnessInfoSetExtension.loudnessInfoSetExtEq.loudnessInfoV1AlbumCount > 0) {
      (*pLoudnessInfoSetDst)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album = (LoudnessInfo *)iisCalloc(pLoudnessInfoSetSrc->loudnessInfoSetExtension.loudnessInfoSetExtEq.loudnessInfoV1AlbumCount, sizeof(LoudnessInfo));
      if ((*pLoudnessInfoSetDst)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album == NULL) {
        return IISDRCGAINENC_RETURN_ERROR_MEMORY;
      }

      memcpy((*pLoudnessInfoSetDst)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album, pLoudnessInfoSetSrc->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album, pLoudnessInfoSetSrc->loudnessInfoSetExtension.loudnessInfoSetExtEq.loudnessInfoV1AlbumCount * sizeof(LoudnessInfo));
      for (k = 0; k < pLoudnessInfoSetSrc->loudnessInfoSetExtension.loudnessInfoSetExtEq.loudnessInfoV1AlbumCount; k++) {
        (*pLoudnessInfoSetDst)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album[k].pLoudnessMeasure = (LoudnessMeasure *)iisCalloc(pLoudnessInfoSetSrc->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album[k].measurementCount, sizeof(LoudnessMeasure));
        if ((*pLoudnessInfoSetDst)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album[k].pLoudnessMeasure == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }

        memcpy((*pLoudnessInfoSetDst)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album[k].pLoudnessMeasure, pLoudnessInfoSetSrc->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album[k].pLoudnessMeasure, pLoudnessInfoSetSrc->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album[k].measurementCount * sizeof(LoudnessMeasure));
      }
    } else {
      (*pLoudnessInfoSetDst)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album = NULL;
    }
  } else {
    (*pLoudnessInfoSetDst)->loudnessInfoSetExtension.pLoudnessInfoSetExtType = NULL;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
freeLoudnessInfoSet(LoudnessInfoSet **pLoudnessInfoSet) {
  int k;

  if ((*pLoudnessInfoSet)->loudnessInfoSetExtPresent) {
    iisFree((*pLoudnessInfoSet)->loudnessInfoSetExtension.pExtBitSize);
    (*pLoudnessInfoSet)->loudnessInfoSetExtension.pExtBitSize = NULL;
    iisFree((*pLoudnessInfoSet)->loudnessInfoSetExtension.pLoudnessInfoSetExtType);
    (*pLoudnessInfoSet)->loudnessInfoSetExtension.pLoudnessInfoSetExtType = NULL;

    for (k = 0; k < (*pLoudnessInfoSet)->loudnessInfoSetExtension.loudnessInfoSetExtEq.loudnessInfoV1Count; k++) {
      iisFree((*pLoudnessInfoSet)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1[k].pLoudnessMeasure);
      (*pLoudnessInfoSet)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1[k].pLoudnessMeasure = NULL;
    }
    iisFree((*pLoudnessInfoSet)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1);
    (*pLoudnessInfoSet)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1 = NULL;

    for (k = 0; k < (*pLoudnessInfoSet)->loudnessInfoSetExtension.loudnessInfoSetExtEq.loudnessInfoV1AlbumCount; k++) {
      iisFree((*pLoudnessInfoSet)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album[k].pLoudnessMeasure);
      (*pLoudnessInfoSet)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album[k].pLoudnessMeasure = NULL;
    }
    iisFree((*pLoudnessInfoSet)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album);
    (*pLoudnessInfoSet)->loudnessInfoSetExtension.loudnessInfoSetExtEq.pLoudnessInfoV1Album = NULL;
  }

  if ((*pLoudnessInfoSet)->loudnessInfoAlbumCount > 0) {
    for (k = 0; k < (*pLoudnessInfoSet)->loudnessInfoAlbumCount; k++) {
      iisFree((*pLoudnessInfoSet)->pLoudnessInfoAlbum[k].pLoudnessMeasure);
      (*pLoudnessInfoSet)->pLoudnessInfoAlbum[k].pLoudnessMeasure = NULL;
    }
    iisFree((*pLoudnessInfoSet)->pLoudnessInfoAlbum);
    (*pLoudnessInfoSet)->pLoudnessInfoAlbum = NULL;
  }

  if ((*pLoudnessInfoSet)->loudnessInfoCount > 0) {
    for (k = 0; k < (*pLoudnessInfoSet)->loudnessInfoCount; k++) {
      iisFree((*pLoudnessInfoSet)->pLoudnessInfo[k].pLoudnessMeasure);
      (*pLoudnessInfoSet)->pLoudnessInfo[k].pLoudnessMeasure = NULL;
    }
    iisFree((*pLoudnessInfoSet)->pLoudnessInfo);
    (*pLoudnessInfoSet)->pLoudnessInfo = NULL;
  }

  iisFree(*pLoudnessInfoSet);
  *pLoudnessInfoSet = NULL;

  return IISDRCGAINENC_RETURN_NOERROR;
}

IISDRCGAINENC_RETURN
iisDRCGainEnc_open(HANDLE_IISDRCGAINENC_PARAMS *phIisDrcGainEnc_params,
                   IISDRCGAINENC_CONFIG *pIisDRCGainEnc_config,
                   int *pGainSequenceCount) {
  int k, m, allBandGainCount, gainSetCount = 0, gainSeqCounter;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;
  int deltaTminTmp;

  if (*phIisDrcGainEnc_params != NULL) {
    return IISDRCGAINENC_RETURN_ERROR_INPUTHANDLE;
  }

  *phIisDrcGainEnc_params = (HANDLE_IISDRCGAINENC_PARAMS)iisCalloc(1, sizeof(IISDRCGAINENC_PARAMS));
  if (phIisDrcGainEnc_params == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_MEMORY;
  }

  (*phIisDrcGainEnc_params)->nodeSettingMode = pIisDRCGainEnc_config->nodeSettingMode;
  (*phIisDrcGainEnc_params)->frameSize = pIisDRCGainEnc_config->frameSize;

  if (pIisDRCGainEnc_config->pUniDrcConfig != NULL) {
    allBandGainCount = 0;
    if (pIisDRCGainEnc_config->pUniDrcConfig->uniDrcConfigExtension.drcCoeffsAndInstructionsUniDrcV1Present == 1 && pIisDRCGainEnc_config->pUniDrcConfig->uniDrcConfigExtension.drcCoefficientsUniDrcV1Count > 0) {
      gainSetCount = pIisDRCGainEnc_config->pUniDrcConfig->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1[0].gainSetCount;
      for (k = 0; k < gainSetCount; k++) {
        allBandGainCount += pIisDRCGainEnc_config->pUniDrcConfig->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1[0].pGainSetParams[k].bandCount;
      }
    } else {
      gainSetCount = pIisDRCGainEnc_config->pUniDrcConfig->pDrcCoefficientsUniDrc[0].gainSetCount;
      for (k = 0; k < gainSetCount; k++) {
        allBandGainCount += pIisDRCGainEnc_config->pUniDrcConfig->pDrcCoefficientsUniDrc[0].pGainSetParams[k].bandCount;
      }
    }

    (*phIisDrcGainEnc_params)->gainSequenceCount = allBandGainCount;

    if (pIisDRCGainEnc_config->pUniDrcConfig->uniDrcConfigExtension.drcCoeffsAndInstructionsUniDrcV1Present == 1 && pIisDRCGainEnc_config->pUniDrcConfig->uniDrcConfigExtension.drcCoefficientsUniDrcV1Count > 0) {
      pIisDRCGainEnc_config->pUniDrcConfig->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1[0].gainSequenceCount = allBandGainCount;
    }

    if (pGainSequenceCount != NULL) {
      *pGainSequenceCount = (*phIisDrcGainEnc_params)->gainSequenceCount;
    }

    retVal = allocAndCopyUniDrcConfig(&(*phIisDrcGainEnc_params)->pUniDrcConfig, pIisDRCGainEnc_config->pUniDrcConfig);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  } else {
    if (pGainSequenceCount != NULL) {
      gainSetCount = *pGainSequenceCount;
      (*phIisDrcGainEnc_params)->gainSequenceCount = *pGainSequenceCount;
    }
  }

  if (pIisDRCGainEnc_config->pUniDrcConfig != NULL || pGainSequenceCount != NULL) {
    GainSetParams gainSetParamsTmp;

    (*phIisDrcGainEnc_params)->pUniDrcGain = (UniDrcGain *)iisCalloc(1, sizeof(UniDrcGain));
    if ((*phIisDrcGainEnc_params)->pUniDrcGain == NULL) {
      return IISDRCGAINENC_RETURN_ERROR_MEMORY;
    }

    (*phIisDrcGainEnc_params)->pUniDrcGain->uniDrcGainExtPresent = 0;

    (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence = (DrcGainSequence *)iisCalloc((*phIisDrcGainEnc_params)->gainSequenceCount, sizeof(DrcGainSequence));
    if ((*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence == NULL) {
      return IISDRCGAINENC_RETURN_ERROR_MEMORY;
    }

    gainSeqCounter = 0;
    retVal = getDeltaTMin(&deltaTminTmp, pIisDRCGainEnc_config->sampleRate);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

    for (k = 0; k < gainSetCount; k++) {
      if (pIisDRCGainEnc_config->pUniDrcConfig != NULL) {
        if (pIisDRCGainEnc_config->pUniDrcConfig->uniDrcConfigExtension.drcCoeffsAndInstructionsUniDrcV1Present == 1 && pIisDRCGainEnc_config->pUniDrcConfig->uniDrcConfigExtension.drcCoefficientsUniDrcV1Count > 0) {
          gainSetParamsTmp = pIisDRCGainEnc_config->pUniDrcConfig->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1[0].pGainSetParams[k];
        } else {
          gainSetParamsTmp = pIisDRCGainEnc_config->pUniDrcConfig->pDrcCoefficientsUniDrc[0].pGainSetParams[k];
        }
      } else {
        gainSetParamsTmp.gainCodingProfile = IISDRCGAINENC_GAINCODINGPROFILE_REGULAR;
        gainSetParamsTmp.gainInterpolationType = IISDRCGAINENC_GAININTERPOLATIONTYPE_LINEAR;
        gainSetParamsTmp.fullFrame = 0;
        gainSetParamsTmp.timeAlignment = 0;
        gainSetParamsTmp.timeDeltaMinPresent = 0;
        gainSetParamsTmp.timeDeltaMin = 0;
        gainSetParamsTmp.bandCount = 1;
      }
      for (m = 0; m < gainSetParamsTmp.bandCount; m++) {
        int maxNNodes = 0;
        (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].gainInterpolationType = gainSetParamsTmp.gainInterpolationType;
        (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].gainCodingProfile = gainSetParamsTmp.gainCodingProfile;
        (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].fullFrame = gainSetParamsTmp.fullFrame;

        if (gainSetParamsTmp.timeDeltaMinPresent == 1) {
          (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].deltaTmin = gainSetParamsTmp.timeDeltaMin;
        } else {
          (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].deltaTmin = deltaTminTmp;
        }

        (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].nNodesMax = pIisDRCGainEnc_config->frameSize / (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].deltaTmin;

        if (pIisDRCGainEnc_config->nodeSettingMode == IISDRCGAINENC_NODESETTINGMODE_SINGLENODE) {
          maxNNodes = 2;
        } else if (pIisDRCGainEnc_config->nodeSettingMode == IISDRCGAINENC_NODESETTINGMODE_ALLNODES) {
          maxNNodes = 16;
        } else if (pIisDRCGainEnc_config->nodeSettingMode == IISDRCGAINENC_NODESETTINGMODE_SALIENTNODE) {
          maxNNodes = 2;
        } else {
          return IISDRCGAINENC_RETURN_ERROR_UNSUPPORTEDNODESETTINGMODE;
        }
        (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].pGainDb = (float *)iisCalloc(maxNNodes, sizeof(float));
        if ((*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].pGainDb == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }
        (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].pTimeSamples = (int *)iisCalloc(maxNNodes, sizeof(int));
        if ((*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].pTimeSamples == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }
        (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].pSlope = (float *)iisCalloc(maxNNodes, sizeof(float));
        if ((*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].pSlope == NULL) {
          return IISDRCGAINENC_RETURN_ERROR_MEMORY;
        }

        (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].nBitsMaxTime = 0;
        while ((1 << (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].nBitsMaxTime) < (2 * (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].nNodesMax)) {
          (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[gainSeqCounter].nBitsMaxTime++;
        }

        gainSeqCounter++;
      }
    }
  }

  if (pIisDRCGainEnc_config->pLoudnessInfoSet != NULL) {
    retVal = allocAndCopyLoudnessInfoSet(&(*phIisDrcGainEnc_params)->pLoudnessInfoSet, pIisDRCGainEnc_config->pLoudnessInfoSet);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

IISDRCGAINENC_RETURN
iisDRCGainEnc_close(HANDLE_IISDRCGAINENC_PARAMS *phIisDrcGainEnc_params) {
  int k;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  if ((*phIisDrcGainEnc_params) == NULL) {
    return IISDRCGAINENC_RETURN_NOERROR;
  }

  if ((*phIisDrcGainEnc_params)->pUniDrcConfig != NULL) {
    retVal = freeUniDrcConfig(&(*phIisDrcGainEnc_params)->pUniDrcConfig);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  if ((*phIisDrcGainEnc_params)->pLoudnessInfoSet != NULL) {
    retVal = freeLoudnessInfoSet(&(*phIisDrcGainEnc_params)->pLoudnessInfoSet);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  if ((*phIisDrcGainEnc_params)->pUniDrcGain != NULL) {
    if ((*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence != NULL) {
      for (k = 0; k < (*phIisDrcGainEnc_params)->gainSequenceCount; k++) {
        iisFree((*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[k].pGainDb);
        (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[k].pGainDb = NULL;
        iisFree((*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[k].pSlope);
        (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[k].pSlope = NULL;
        iisFree((*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[k].pTimeSamples);
        (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence[k].pTimeSamples = NULL;
      }
      iisFree((*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence);
      (*phIisDrcGainEnc_params)->pUniDrcGain->pDrcGainSequence = NULL;
    }
    iisFree((*phIisDrcGainEnc_params)->pUniDrcGain);
    (*phIisDrcGainEnc_params)->pUniDrcGain = NULL;
  }

  iisFree(*phIisDrcGainEnc_params);
  *phIisDrcGainEnc_params = NULL;

  return IISDRCGAINENC_RETURN_NOERROR;
}

IISDRCGAINENC_RETURN
iisDRCGainEnc_writeUniDrcConfig(HANDLE_IISDRCGAINENC_PARAMS hIisDrcGainEnc_params,
                                IISDRCGAINENC_SYNTAXMODE syntaxModeFlag,
                                unsigned char *pBitstreamBuffer,
                                int *pBitCount) {
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  if (hIisDrcGainEnc_params == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_NULLPOINTER;
  }

  if (syntaxModeFlag == IISDRCGAINENC_SYNTAXMODE_DEFAULT) {
    retVal = iisDRCGainEnc_writeUniDrcConfigBits(hIisDrcGainEnc_params->pUniDrcConfig, pBitstreamBuffer, pBitCount);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  } else {
    return IISDRCGAINENC_RETURN_ERROR_UNSUPPORTEDSYNTAXMODE;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

IISDRCGAINENC_RETURN
iisDRCGainEnc_writeUniDrcGain(HANDLE_IISDRCGAINENC_PARAMS hIisDrcGainEnc_params,
                              const float *const *drcGainInputBuffer,
                              const int bIsPrerollFrame,
                              unsigned char *pBitstreamBuffer,
                              int *pBitCount) {
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  if (hIisDrcGainEnc_params == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_NULLPOINTER;
  }
  if (hIisDrcGainEnc_params->pUniDrcGain == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_NULLPOINTER;
  }

  retVal = iisDRCGainEnc_getNodesFromInputBuffer(hIisDrcGainEnc_params, drcGainInputBuffer, bIsPrerollFrame);
  if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

  retVal = iisDRCGainEnc_writeUniDrcGainBits(hIisDrcGainEnc_params->pUniDrcGain, hIisDrcGainEnc_params->gainSequenceCount, pBitstreamBuffer, pBitCount);
  if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

  return IISDRCGAINENC_RETURN_NOERROR;
}

IISDRCGAINENC_RETURN
iisDRCGainEnc_writeLoudnessInfoSet(HANDLE_IISDRCGAINENC_PARAMS hIisDrcGainEnc_params,
                                   IISDRCGAINENC_SYNTAXMODE syntaxModeFlag,
                                   unsigned char *pBitstreamBuffer,
                                   int *pBitCount) {
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  if (hIisDrcGainEnc_params == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_NULLPOINTER;
  }

  if (syntaxModeFlag == IISDRCGAINENC_SYNTAXMODE_DEFAULT) {
    retVal = iisDRCGainEnc_writeLoudnessInfoSetBits(hIisDrcGainEnc_params->pLoudnessInfoSet, pBitstreamBuffer, pBitCount);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  } else {
    return IISDRCGAINENC_RETURN_ERROR_UNSUPPORTEDSYNTAXMODE;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

IISDRCGAINENC_RETURN
iisDRCGainEnc_replaceLoudnessInfoSet(HANDLE_IISDRCGAINENC_PARAMS hIisDrcGainEnc_params,
                                     LoudnessInfoSet *pLoudnessInfoSet) {
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  if (hIisDrcGainEnc_params == NULL || pLoudnessInfoSet == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_NULLPOINTER;
  }

  if (hIisDrcGainEnc_params->pLoudnessInfoSet != NULL) {
    retVal = freeLoudnessInfoSet(&hIisDrcGainEnc_params->pLoudnessInfoSet);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  retVal = allocAndCopyLoudnessInfoSet(&hIisDrcGainEnc_params->pLoudnessInfoSet, pLoudnessInfoSet);
  if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

  return IISDRCGAINENC_RETURN_NOERROR;
}

IISDRCGAINENC_RETURN
iisDRCGainEnc_writeUniDrc(HANDLE_IISDRCGAINENC_PARAMS hIisDrcGainEnc_params,
                          const float *const *drcGainInputBuffer,
                          const int bIsPrerollFrame,
                          const int bLoudnessInfoSetPresent,
                          const int bUniDrcConfigPresent,
                          unsigned char *pBitstreamBuffer,
                          int *pBitCount) {
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  retVal = iisDRCGainEnc_getNodesFromInputBuffer(hIisDrcGainEnc_params, drcGainInputBuffer, bIsPrerollFrame);
  if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

  retVal = iisDRCGainEnc_writeUniDrcBits(hIisDrcGainEnc_params, bLoudnessInfoSetPresent, bUniDrcConfigPresent, pBitstreamBuffer, pBitCount);
  if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

  return IISDRCGAINENC_RETURN_NOERROR;
}

IISDRCGAINENC_RETURN
iisDRCGainEnc_getDrcInstructionsUniDrcMaxCount(int *drcInstructionsUniDrcMaxCount) {
  if (drcInstructionsUniDrcMaxCount == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_NULLPOINTER;
  }

  *drcInstructionsUniDrcMaxCount = DRC_INSTRUCTIONSUNIDRC_COUNT_MAX;

  return IISDRCGAINENC_RETURN_NOERROR;
}

IISDRCGAINENC_RETURN
iisDRCGainEnc_getGainSequenceMaxCount(int *gainSequenceMaxCount) {
  if (gainSequenceMaxCount == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_NULLPOINTER;
  }

  *gainSequenceMaxCount = DRC_GAINSEQUENCE_COUNT_MAX;

  return IISDRCGAINENC_RETURN_NOERROR;
}

IISDRCGAINENC_RETURN
iisDRCGainEnc_getDrcGainEncoderDelay(HANDLE_IISDRCGAINENC_PARAMS hIisDrcGainEnc_params,
                                     int *drcGainEncoderDelay) {
  if (drcGainEncoderDelay == NULL || hIisDrcGainEnc_params == NULL) {
    return IISDRCGAINENC_RETURN_ERROR_NULLPOINTER;
  }

  *drcGainEncoderDelay = 0;

  return IISDRCGAINENC_RETURN_NOERROR;
}
