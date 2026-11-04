
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

#include <string.h>
#include "iisxHEAACEncLib_submitDrcMetadata.h"
#include "iisxHEAACEncLib_common.h"

static XHEAACENCLIB_RETURN pipeDrcGains(
    XHEAACENCLIB_LRACONTROL_DRC_GAIN_DATA* const lraControlDrcGainData,
    float const* const drcGains,
    unsigned int const nValues) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (NULL == lraControlDrcGainData || NULL == drcGains) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    lraControlDrcGainData->lraControlDrcGainsLength = 0;
    lraControlDrcGainData->lraControlDrcProcessingIsActive = 0;

    if ((nValues > 0) && (NULL == lraControlDrcGainData->lraControlDrcGains)) {
      lraControlDrcGainData->lraControlDrcGains = (float*)iisCalloc(nValues, sizeof(float));

      if (NULL == lraControlDrcGainData->lraControlDrcGains) {
        retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
      }
    } else if (nValues != lraControlDrcGainData->lraControlDrcGainsLength) {
      retValue = XHEAACENCLIB_RETURN_ERROR_LRACONTROL_DRC_GAIN;
    }
  }

  if (!isError(retValue)) {
    memcpy(lraControlDrcGainData->lraControlDrcGains, drcGains, sizeof(float) * nValues);
    lraControlDrcGainData->lraControlDrcGainsLength = nValues;
    lraControlDrcGainData->lraControlDrcProcessingIsActive = 1;
  }

  return retValue;
}

static XHEAACENCLIB_RETURN pipeDrcExternalNodeNumNodes(
    XHEAACENCLIB_DRC_EXTERNAL_NODES_DATA* const drcExternalNodes,
    unsigned int const drcExternalNodeNumNodes) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (NULL == drcExternalNodes) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (drcExternalNodeNumNodes > XHEAACENCLIB_MAX_DRC_CHAR_NODES) {
    retValue = XHEAACENCLIB_RETURN_ERROR_DRC_TOO_MANY_NODES;
  }

  if (!isError(retValue)) {
    drcExternalNodes->drcExternalNodeNumNodes = drcExternalNodeNumNodes;
  }

  return retValue;
}

static XHEAACENCLIB_RETURN pipeDrcExternalNodeLevels(
    XHEAACENCLIB_DRC_EXTERNAL_NODES_DATA* const drcExternalNodes,
    int const* const drcExternalNodeLevels,
    unsigned int const nValues) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if ((NULL == drcExternalNodes) || (NULL == drcExternalNodeLevels)) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (nValues > XHEAACENCLIB_MAX_DRC_CHAR_NODES) {
    retValue = XHEAACENCLIB_RETURN_ERROR_DRC_TOO_MANY_NODES;
  }

  if (!isError(retValue)) {
    memcpy(drcExternalNodes->drcExternalNodeLevels, drcExternalNodeLevels, sizeof(float) * nValues);
  }

  return retValue;
}

static XHEAACENCLIB_RETURN pipeDrcExternalNodeGains(
    XHEAACENCLIB_DRC_EXTERNAL_NODES_DATA* const drcExternalNodes,
    int const* const drcExternalNodeGains,
    unsigned int const nValues) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (NULL == drcExternalNodes || NULL == drcExternalNodeGains) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (nValues > XHEAACENCLIB_MAX_DRC_CHAR_NODES) {
    retValue = XHEAACENCLIB_RETURN_ERROR_DRC_TOO_MANY_NODES;
  }

  if (!isError(retValue)) {
    memcpy(drcExternalNodes->drcExternalNodeGains, drcExternalNodeGains, sizeof(float) * nValues);
  }

  return retValue;
}

static XHEAACENCLIB_RETURN pipeDrcGainOffset(
    XHEAACENCLIB_DRC_EXTERNAL_NODES_DATA* const drcExternalNodes,
    float const* const drcGainOffset,
    unsigned int const nValues) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (NULL == drcExternalNodes || NULL == drcGainOffset) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (nValues > XHEAACENCLIB_MAX_DRC_SEQUENCES) {
    retValue = XHEAACENCLIB_RETURN_ERROR_DRC_TOO_MANY_SEQUENCES;
  }

  if (!isError(retValue)) {
    memcpy(drcExternalNodes->drcGainOffset, drcGainOffset, sizeof(float) * nValues);
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_submitDrcMetadata(
    XHEAACENCLIB_LRACONTROL_DRC_GAIN_DATA* const lraControlDrcGainData,
    XHEAACENCLIB_DRC_EXTERNAL_NODES_DATA* const drcExternalNodes,
    float const* const drcGains,
    unsigned int const drcGainsLength,
    unsigned int const drcExternalNodeNumNodes,
    int const* const drcExternalNodeLevels,
    int const* const drcExternalNodeGains,
    float const* const drcGainOffset,
    unsigned int const drcGainOffsetLength) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if ((NULL == lraControlDrcGainData) || (NULL == drcExternalNodes) || (NULL == drcGains) || (NULL == drcExternalNodeLevels) || (NULL == drcExternalNodeGains) || (NULL == drcGainOffset)) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = pipeDrcGains(lraControlDrcGainData, drcGains, drcGainsLength);
  }

  if (!isError(retValue)) {
    retValue = pipeDrcExternalNodeNumNodes(drcExternalNodes, drcExternalNodeNumNodes);
  }

  if (!isError(retValue)) {
    retValue = pipeDrcExternalNodeLevels(drcExternalNodes, drcExternalNodeLevels, drcExternalNodeNumNodes);
  }

  if (!isError(retValue)) {
    retValue = pipeDrcExternalNodeGains(drcExternalNodes, drcExternalNodeGains, drcExternalNodeNumNodes);
  }

  if (!isError(retValue)) {
    retValue = pipeDrcGainOffset(drcExternalNodes, drcGainOffset, drcGainOffsetLength);
  }

  return retValue;
}
