
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

#include "iisxHEAACEncLib_SigMap_ifc.h"
#include "iisxHEAACEncLib_common.h"

static IIS_XHEAACENCLIB_SIGMAP_ERROR GetSigMapLayout(XHEAACENCLIB_CONFIG_HANDLE hConfig,
                                                     SIGMAP_SETUP *sigMapSetup) {
  IIS_XHEAACENCLIB_SIGMAP_ERROR retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_NO_ERROR;

  if (hConfig == NULL || sigMapSetup == NULL) {
    retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_ERROR_INVALID_HANDLE;
  }

  if (retValueSigMapIfc == IIS_XHEAACENCLIB_SIGMAP_NO_ERROR) {
    if (hConfig->aot == AUD_OBJ_TYP_PS) {
      sigMapSetup->cicpLayoutIndex = SIGMAP_CICP_2;
    } else {
      switch (hConfig->CicpIndex) {
        case XHEAACENCLIB_SIGMAP_CICP_1:
          sigMapSetup->cicpLayoutIndex = SIGMAP_CICP_1;
          break;

        case XHEAACENCLIB_SIGMAP_CICP_2:
          sigMapSetup->cicpLayoutIndex = SIGMAP_CICP_2;
          break;

        case XHEAACENCLIB_SIGMAP_CICP_5:
          sigMapSetup->cicpLayoutIndex = SIGMAP_CICP_5;
          break;

        case XHEAACENCLIB_SIGMAP_CICP_6:
          sigMapSetup->cicpLayoutIndex = SIGMAP_CICP_6;
          break;

        case XHEAACENCLIB_SIGMAP_CICP_7:
          sigMapSetup->cicpLayoutIndex = SIGMAP_CICP_7;
          break;

        case XHEAACENCLIB_SIGMAP_CICP_12:
          sigMapSetup->cicpLayoutIndex = SIGMAP_CICP_12;
          break;

        case XHEAACENCLIB_SIGMAP_CICP_13:
          sigMapSetup->cicpLayoutIndex = SIGMAP_CICP_13;
          break;

        case XHEAACENCLIB_SIGMAP_CICP_14:
          sigMapSetup->cicpLayoutIndex = SIGMAP_CICP_14;
          break;

        default:
          sigMapSetup->cicpLayoutIndex = SIGMAP_INVALID;
          break;
      }
    }

    if (hConfig->aot != AUD_OBJ_TYP_USAC) {
      if (hConfig->nMetadataBitRate) {
        iisSigMapAddElement(sigMapSetup, SIGMAP_ELEMENT_DSE_DRC, 0.0f, 0);
      }

      if (hConfig->nAncDataBitRate) {
        iisSigMapAddElement(sigMapSetup, SIGMAP_ELEMENT_DSE, 0.0f, 0);
      }
    }
  }

  return retValueSigMapIfc;
}

static IIS_XHEAACENCLIB_SIGMAP_ERROR iisxHEAACEncLib_SigMap_ModifyElementType(XHEAACENCLIB_CONFIG_HANDLE hConfig,
                                                                              SIGMAP_SETUP *sigMapSetup) {
  IIS_XHEAACENCLIB_SIGMAP_ERROR retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_NO_ERROR;

  if (hConfig == NULL || sigMapSetup == NULL) {
    retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_ERROR_INVALID_HANDLE;
  }

  if (retValueSigMapIfc == IIS_XHEAACENCLIB_SIGMAP_NO_ERROR) {
    switch (hConfig->CicpIndex) {
      case XHEAACENCLIB_SIGMAP_CICP_1:

        break;

      case XHEAACENCLIB_SIGMAP_CICP_2:
        if (hConfig->aot == AUD_OBJ_TYP_PS) {
          sigMapSetup->elementType[0] = SIGMAP_ELEMENT_CPE_PS;
        }
        if (hConfig->aot == AUD_OBJ_TYP_USAC) {
          switch (hConfig->stereoConfigIndex) {
            case 0:

              break;
            case 1:
              sigMapSetup->elementType[0] = SIGMAP_ELEMENT_CPE_1;
              break;
            case 2:
              sigMapSetup->elementType[0] = SIGMAP_ELEMENT_CPE_2;
              break;
            case 3:
              sigMapSetup->elementType[0] = SIGMAP_ELEMENT_CPE_3;
              break;
          }
        }
        break;

      case XHEAACENCLIB_SIGMAP_CICP_5:
      case XHEAACENCLIB_SIGMAP_CICP_6:
        if (hConfig->aot == AUD_OBJ_TYP_USAC) {
          switch (hConfig->stereoConfigIndex) {
            case 0:

              break;
            case 1:
              sigMapSetup->elementType[1] = SIGMAP_ELEMENT_CPE_1;
              sigMapSetup->elementType[2] = SIGMAP_ELEMENT_CPE_1;
              break;
            case 2:
              sigMapSetup->elementType[1] = SIGMAP_ELEMENT_CPE_2;
              sigMapSetup->elementType[2] = SIGMAP_ELEMENT_CPE_2;
              break;
            case 3:
              sigMapSetup->elementType[1] = SIGMAP_ELEMENT_CPE_3;
              sigMapSetup->elementType[2] = SIGMAP_ELEMENT_CPE_3;
          }
        }
        break;

      case XHEAACENCLIB_SIGMAP_CICP_7:
      case XHEAACENCLIB_SIGMAP_CICP_12:
        if (hConfig->aot == AUD_OBJ_TYP_USAC) {
          switch (hConfig->stereoConfigIndex) {
            case 0:

              break;
            case 1:
              sigMapSetup->elementType[1] = SIGMAP_ELEMENT_CPE_1;
              sigMapSetup->elementType[2] = SIGMAP_ELEMENT_CPE_1;
              sigMapSetup->elementType[3] = SIGMAP_ELEMENT_CPE_1;
              break;
            case 2:
              sigMapSetup->elementType[1] = SIGMAP_ELEMENT_CPE_2;
              sigMapSetup->elementType[2] = SIGMAP_ELEMENT_CPE_2;
              sigMapSetup->elementType[3] = SIGMAP_ELEMENT_CPE_2;
              break;
            case 3:
              sigMapSetup->elementType[1] = SIGMAP_ELEMENT_CPE_3;
              sigMapSetup->elementType[2] = SIGMAP_ELEMENT_CPE_3;
              sigMapSetup->elementType[3] = SIGMAP_ELEMENT_CPE_3;
          }
        }
        break;
      default:
        break;
    }
  }

  return retValueSigMapIfc;
}

IIS_XHEAACENCLIB_SIGMAP_ERROR iisxHEAACEncLib_SigMap_New(XHEAACENCLIB_SIGMAP_HANDLE *p_hSigMap) {
  IIS_XHEAACENCLIB_SIGMAP_ERROR retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_NO_ERROR;
  XHEAACENCLIB_SIGMAP_HANDLE hXheSigMap = NULL;
  SIGMAP_RETURN retValueSigMap = SIGMAP_NO_ERROR;

  if (p_hSigMap == NULL) {
    retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_ERROR_INVALID_HANDLE;
  }

  if (retValueSigMapIfc == IIS_XHEAACENCLIB_SIGMAP_NO_ERROR) {
    if (*p_hSigMap == NULL) {
      hXheSigMap = iisCalloc(1, sizeof(XHEAACENCLIB_SIGMAP));
    }

    hXheSigMap->cm = (CHANNEL_MAPPING *)iisCalloc(1, sizeof(CHANNEL_MAPPING));
    if (hXheSigMap->cm == NULL) {
      retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_ERROR_MEMORY;
    }
  }

  if (retValueSigMapIfc == IIS_XHEAACENCLIB_SIGMAP_NO_ERROR) {
    retValueSigMap = iisSigMapNew(&hXheSigMap->hSigMap);
    if (retValueSigMap != SIGMAP_NO_ERROR) {
      retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_ERROR_UNKOWN;
    }
  }

  if (retValueSigMapIfc == IIS_XHEAACENCLIB_SIGMAP_NO_ERROR) {
    (*p_hSigMap) = hXheSigMap;
  }

  return retValueSigMapIfc;
}

IIS_XHEAACENCLIB_SIGMAP_ERROR iisxHEAACEncLib_SigMap_Configure(XHEAACENCLIB_SIGMAP_HANDLE hXheSigMap,
                                                               XHEAACENCLIB_CONFIG_HANDLE hConfig) {
  IIS_XHEAACENCLIB_SIGMAP_ERROR retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_NO_ERROR;
  SIGMAP_RETURN retValueSigMap = SIGMAP_NO_ERROR;
  SIGMAP_SETUP sigMapSetup = {0};

  if (hXheSigMap == NULL || hConfig == NULL) {
    retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_ERROR_INVALID_HANDLE;
  }

  if (retValueSigMapIfc == IIS_XHEAACENCLIB_SIGMAP_NO_ERROR) {
    if (hConfig->aot == AUD_OBJ_TYP_USAC) {
      sigMapSetup.type = SIGMAP_TYPE_USAC;
    } else {
      sigMapSetup.type = SIGMAP_TYPE_MPEG4;
    }

    retValueSigMapIfc = GetSigMapLayout(hConfig, &sigMapSetup);
  }

  if (retValueSigMapIfc == IIS_XHEAACENCLIB_SIGMAP_NO_ERROR) {
    retValueSigMapIfc = iisxHEAACEncLib_SigMap_ModifyElementType(hConfig, &sigMapSetup);
  }

  if (retValueSigMapIfc == IIS_XHEAACENCLIB_SIGMAP_NO_ERROR) {
    retValueSigMap = iisSigMapConfigure(hXheSigMap->hSigMap, &sigMapSetup, hXheSigMap->cm);
    if (retValueSigMap != SIGMAP_NO_ERROR) {
      retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_ERROR_UNKOWN;
    }
  }

  if (retValueSigMapIfc == IIS_XHEAACENCLIB_SIGMAP_NO_ERROR) {
    hXheSigMap->nChannels = hXheSigMap->cm->nChannels;
    hXheSigMap->nEffectiveChannels = hXheSigMap->cm->nEffectiveChannels;
  }

  iisSigMapDeleteObjectsFromSetup(&sigMapSetup);

  return retValueSigMapIfc;
}

IIS_XHEAACENCLIB_SIGMAP_ERROR iisxHEAACEncLib_SigMap_GetChannelMap(XHEAACENCLIB_SIGMAP_HANDLE hXheSigMap,
                                                                   CHANNEL_MAPPING **cm) {
  IIS_XHEAACENCLIB_SIGMAP_ERROR retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_NO_ERROR;

  if (hXheSigMap == NULL || cm == NULL) {
    retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_ERROR_INVALID_HANDLE;
  }

  if (retValueSigMapIfc == IIS_XHEAACENCLIB_SIGMAP_NO_ERROR) {
    (*cm) = hXheSigMap->cm;
  }

  return retValueSigMapIfc;
}

IIS_XHEAACENCLIB_SIGMAP_ERROR iisxHEAACEncLib_SigMap_Delete(XHEAACENCLIB_SIGMAP_HANDLE hXheSigMap) {
  IIS_XHEAACENCLIB_SIGMAP_ERROR retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_NO_ERROR;

  if (hXheSigMap != NULL) {
    iisSigMapDelete(hXheSigMap->hSigMap);
    if (hXheSigMap->cm) iisFree(hXheSigMap->cm);
    iisFree(hXheSigMap);
  }

  return retValueSigMapIfc;
}

IIS_XHEAACENCLIB_SIGMAP_ERROR iisxHEAACEncLib_SigMap_GetCoreElements(XHEAACENCLIB_SIGMAP_INDEX cicpIndex,
                                                                     USAC_ELEMENT_TYPE *eleType,
                                                                     int *numCoreElement) {
  IIS_XHEAACENCLIB_SIGMAP_ERROR retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_NO_ERROR;

  if (eleType == NULL || numCoreElement == NULL) {
    retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_ERROR_INVALID_HANDLE;
  }

  if (retValueSigMapIfc == IIS_XHEAACENCLIB_SIGMAP_NO_ERROR) {
    switch (cicpIndex) {
      case XHEAACENCLIB_SIGMAP_CICP_1:
        eleType[0] = USAC_ELEMENT_TYPE_SCE;
        *numCoreElement = 1;
        break;

      case XHEAACENCLIB_SIGMAP_CICP_2:
        eleType[0] = USAC_ELEMENT_TYPE_CPE;
        *numCoreElement = 1;
        break;

      case XHEAACENCLIB_SIGMAP_CICP_5:
        eleType[0] = USAC_ELEMENT_TYPE_SCE;
        eleType[1] = USAC_ELEMENT_TYPE_CPE;
        eleType[2] = USAC_ELEMENT_TYPE_CPE;
        *numCoreElement = 3;
        break;

      case XHEAACENCLIB_SIGMAP_CICP_6:
        eleType[0] = USAC_ELEMENT_TYPE_SCE;
        eleType[1] = USAC_ELEMENT_TYPE_CPE;
        eleType[2] = USAC_ELEMENT_TYPE_CPE;
        eleType[3] = USAC_ELEMENT_TYPE_LFE;
        *numCoreElement = 4;
        break;

      case XHEAACENCLIB_SIGMAP_CICP_7:
      case XHEAACENCLIB_SIGMAP_CICP_12:
        eleType[0] = USAC_ELEMENT_TYPE_SCE;
        eleType[1] = USAC_ELEMENT_TYPE_CPE;
        eleType[2] = USAC_ELEMENT_TYPE_CPE;
        eleType[3] = USAC_ELEMENT_TYPE_CPE;
        eleType[4] = USAC_ELEMENT_TYPE_LFE;
        *numCoreElement = 5;
        break;

      case XHEAACENCLIB_SIGMAP_CICP_14:
        eleType[0] = USAC_ELEMENT_TYPE_SCE;
        eleType[1] = USAC_ELEMENT_TYPE_CPE;
        eleType[2] = USAC_ELEMENT_TYPE_CPE;
        eleType[3] = USAC_ELEMENT_TYPE_LFE;
        eleType[4] = USAC_ELEMENT_TYPE_CPE;
        *numCoreElement = 5;
        break;

      default:

        assert(0);
        retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_ERROR_INVALID_PARAM;
        break;
    }
  }

  return retValueSigMapIfc;
}
