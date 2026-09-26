
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
#include <math.h>

#include "iisDRCGainEnc_nodeSetting.h"
#include "iisDRCGainEnc_internal.h"

static void
fillNodeBuffer(const float *drcGainInputBuffer,
               const int frameSize,
               NodeBuffer *nodeBuffer) {
  int n, i;

  assert((frameSize % NODE_BUFFER_SIZE) == 0);
  nodeBuffer->nSamplesPerNode = frameSize / NODE_BUFFER_SIZE;

  for (n = 0; n < NODE_BUFFER_SIZE; n++) {
    float accumulatedGainDb = 0.0f;

    for (i = 0; i < nodeBuffer->nSamplesPerNode; i++) {
      accumulatedGainDb += drcGainInputBuffer[n * nodeBuffer->nSamplesPerNode + i];
    }
    nodeBuffer->gainDb[n] = accumulatedGainDb / nodeBuffer->nSamplesPerNode;
    nodeBuffer->timeIndex[n] = n;
  }
  nodeBuffer->nNodes = NODE_BUFFER_SIZE;
}

static IISDRCGAINENC_RETURN
setAllNodes(DrcGainSequence *pDrcGainSequence,
            const float *drcGainInputBuffer,
            const int bIsPrerollFrame,
            const int frameSize) {
  int n, hopsize;
  NodeBuffer nodeBuffer;

  if (pDrcGainSequence->deltaTmin * pDrcGainSequence->nNodesMax != frameSize)
    return IISDRCGAINENC_RETURN_ERROR_UNKNOWN;

  (void)bIsPrerollFrame;

  fillNodeBuffer(drcGainInputBuffer, frameSize, &nodeBuffer);

  pDrcGainSequence->nNodes = 16;
  hopsize = nodeBuffer.nNodes / pDrcGainSequence->nNodes;

  for (n = 0; n < pDrcGainSequence->nNodes; n++) {
    pDrcGainSequence->pGainDb[n] = nodeBuffer.gainDb[(n + 1) * hopsize - 1];
    pDrcGainSequence->pTimeSamples[n] = (nodeBuffer.timeIndex[(n + 1) * hopsize - 1] + 1) * nodeBuffer.nSamplesPerNode - 1;
    pDrcGainSequence->pSlope[n] = 0.0f;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static void
nodeBuffer_addNode(NodeBuffer *nodeBuffer,
                   float const newNode_gainDb,
                   int const newNode_timeIndex) {
  int i, insertIndex = -1;
  for (i = 0; i < nodeBuffer->nNodes; i++) {
    if (nodeBuffer->timeIndex[i] < newNode_timeIndex) {
      continue;
    } else if (nodeBuffer->timeIndex[i] == newNode_timeIndex) {
      nodeBuffer->gainDb[i] = newNode_gainDb;
      return;
    } else {
      insertIndex = i;
      break;
    }
  }

  if (insertIndex < 0) return;
  if ((nodeBuffer->nNodes + 1) > NODE_BUFFER_SIZE_WITH_PREV_NODE) return;

  nodeBuffer->nNodes++;
  for (i = nodeBuffer->nNodes - 1; i > insertIndex; i--) {
    nodeBuffer->gainDb[i] = nodeBuffer->gainDb[i - 1];
    nodeBuffer->timeIndex[i] = nodeBuffer->timeIndex[i - 1];
  }

  nodeBuffer->gainDb[insertIndex] = newNode_gainDb;
  nodeBuffer->timeIndex[insertIndex] = newNode_timeIndex;
}

static int
nodeBuffer_getMinValueIndex(NodeBuffer const *nodeBuffer_in) {
  int i, minValueIndex = 0;
  float tmpMinimum = nodeBuffer_in->gainDb[0];

  for (i = 1; i < nodeBuffer_in->nNodes; i++) {
    if (nodeBuffer_in->gainDb[i] <= tmpMinimum) {
      tmpMinimum = nodeBuffer_in->gainDb[i];
      minValueIndex = i;
    }
  }
  return minValueIndex;
}

static int
nodeBuffer_getMaxValueIndex(NodeBuffer const *nodeBuffer_in) {
  int i, maxValueIndex = 0;
  float tmpMaximum = nodeBuffer_in->gainDb[0];

  for (i = 1; i < nodeBuffer_in->nNodes; i++) {
    if (nodeBuffer_in->gainDb[i] >= tmpMaximum) {
      tmpMaximum = nodeBuffer_in->gainDb[i];
      maxValueIndex = i;
    }
  }
  return maxValueIndex;
}

static IISDRCGAINENC_RETURN
setSalientGainNode(DrcGainSequence *pDrcGainSequence,
                   const float *drcGainInputBuffer,
                   const int bIsPrerollFrame,
                   const int frameSize) {
  NodeBuffer fullNodeBuffer;
  int k;
  int minGainIndex, maxGainIndex, lastGainIndex, selectedIndex;
  float start, stop, min, max, lowBorder, highBorder;
  const float additionalMargin = 0.5f;

  fillNodeBuffer(drcGainInputBuffer, frameSize, &fullNodeBuffer);
  nodeBuffer_addNode(&fullNodeBuffer, pDrcGainSequence->prevGainDb, -1);

  minGainIndex = nodeBuffer_getMinValueIndex(&fullNodeBuffer);
  maxGainIndex = nodeBuffer_getMaxValueIndex(&fullNodeBuffer);
  lastGainIndex = fullNodeBuffer.nNodes - 1;

  start = fullNodeBuffer.gainDb[0];
  stop = fullNodeBuffer.gainDb[lastGainIndex];
  min = fullNodeBuffer.gainDb[minGainIndex];
  max = fullNodeBuffer.gainDb[maxGainIndex];

  if (start >= stop) {
    highBorder = start + additionalMargin;
    lowBorder = stop - additionalMargin;
  } else {
    highBorder = stop + additionalMargin;
    lowBorder = start - additionalMargin;
  }

  if (min < lowBorder) {
    selectedIndex = minGainIndex;
  } else if (max > highBorder) {
    selectedIndex = maxGainIndex;
  } else {
    selectedIndex = lastGainIndex;
  }

  pDrcGainSequence->prevGainDb = stop;

  k = 0;
  if (bIsPrerollFrame == 2) {
    if (selectedIndex == lastGainIndex) {
      selectedIndex--;
    }
    pDrcGainSequence->nNodes = 1;
    pDrcGainSequence->pGainDb[k] = fullNodeBuffer.gainDb[selectedIndex];
    pDrcGainSequence->pTimeSamples[k] = (fullNodeBuffer.timeIndex[selectedIndex] + 1) * fullNodeBuffer.nSamplesPerNode - 1;
    pDrcGainSequence->pSlope[k] = 0.0f;
  } else if (bIsPrerollFrame == 1) {
    int nNodesNodeReservoir = 1, kT;
    float gainDbDifference;

    assert(pDrcGainSequence->fullFrame == 0);

    pDrcGainSequence->nNodes = 2;

    pDrcGainSequence->pGainDb[k] = fullNodeBuffer.gainDb[0];
    kT = pDrcGainSequence->nNodes - nNodesNodeReservoir + k;
    pDrcGainSequence->pTimeSamples[kT] = 2 * frameSize - 1;
    pDrcGainSequence->pSlope[k] = 0.0f;

    k = 1;
    pDrcGainSequence->pGainDb[k] = fullNodeBuffer.gainDb[selectedIndex];
    kT = k - nNodesNodeReservoir;
    pDrcGainSequence->pTimeSamples[kT] = (fullNodeBuffer.timeIndex[selectedIndex] + 1) * fullNodeBuffer.nSamplesPerNode - 1;
    pDrcGainSequence->pSlope[k] = 0.0f;

    gainDbDifference = pDrcGainSequence->pGainDb[k] - pDrcGainSequence->pGainDb[k - 1];
    if ((gainDbDifference > 1.125f) || (gainDbDifference < -2.125f)) {
      for (; selectedIndex > 1; selectedIndex--) {
        pDrcGainSequence->pGainDb[k] = fullNodeBuffer.gainDb[selectedIndex];
        gainDbDifference = pDrcGainSequence->pGainDb[k] - pDrcGainSequence->pGainDb[k - 1];
        if ((gainDbDifference <= 1.125f) && (gainDbDifference >= -2.125f)) {
          break;
        }
      }
      pDrcGainSequence->pTimeSamples[kT] = (fullNodeBuffer.timeIndex[selectedIndex] + 1) * fullNodeBuffer.nSamplesPerNode - 1;
    }
  } else {
    pDrcGainSequence->nNodes = 1;
    pDrcGainSequence->pGainDb[k] = fullNodeBuffer.gainDb[selectedIndex];
    pDrcGainSequence->pTimeSamples[k] = (fullNodeBuffer.timeIndex[selectedIndex] + 1) * fullNodeBuffer.nSamplesPerNode - 1;
    pDrcGainSequence->pSlope[k] = 0.0f;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
setSingleNode(DrcGainSequence *pDrcGainSequence,
              const float *drcGainInputBuffer,
              const int bIsPrerollFrame,
              const int frameSize) {
  int k;

  k = 0;
  if (bIsPrerollFrame == 2) {
    pDrcGainSequence->nNodes = 1;
    pDrcGainSequence->pGainDb[k] = drcGainInputBuffer[frameSize / 2 - 1];
    pDrcGainSequence->pTimeSamples[k] = frameSize / 2 - 1;
    pDrcGainSequence->pSlope[k] = 0;
  } else if (bIsPrerollFrame == 1) {
    int nNodesNodeReservoir = 1, kT;
    float gainDbDifference;

    assert(pDrcGainSequence->fullFrame == 0);

    pDrcGainSequence->nNodes = 2;

    pDrcGainSequence->pGainDb[k] = drcGainInputBuffer[0];
    kT = pDrcGainSequence->nNodes - nNodesNodeReservoir + k;
    pDrcGainSequence->pTimeSamples[kT] = 2 * frameSize - 1;
    pDrcGainSequence->pSlope[k] = 0;

    k = 1;
    pDrcGainSequence->pGainDb[k] = drcGainInputBuffer[frameSize - 1];
    kT = k - nNodesNodeReservoir;
    pDrcGainSequence->pTimeSamples[kT] = frameSize - 1;
    pDrcGainSequence->pSlope[k] = 0;

    gainDbDifference = pDrcGainSequence->pGainDb[k] - pDrcGainSequence->pGainDb[k - 1];
    if ((gainDbDifference > 1.125f) || (gainDbDifference < -2.125f)) {
      int nSamplesPerNode = frameSize / NODE_BUFFER_SIZE;
      int timeSamples = frameSize - 1;
      int n;
      assert((frameSize % NODE_BUFFER_SIZE) == 0);

      for (n = 0; n < NODE_BUFFER_SIZE - 1; n++) {
        timeSamples -= nSamplesPerNode;
        pDrcGainSequence->pGainDb[k] = drcGainInputBuffer[timeSamples];
        gainDbDifference = pDrcGainSequence->pGainDb[k] - pDrcGainSequence->pGainDb[k - 1];
        if ((gainDbDifference <= 1.125f) && (gainDbDifference >= -2.125f)) {
          break;
        }
      }
      pDrcGainSequence->pTimeSamples[kT] = timeSamples;
    }
  } else {
    pDrcGainSequence->nNodes = 1;

    pDrcGainSequence->pGainDb[k] = drcGainInputBuffer[frameSize - 1];
    pDrcGainSequence->pTimeSamples[k] = frameSize - 1;
    pDrcGainSequence->pSlope[k] = 0;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

IISDRCGAINENC_RETURN
iisDRCGainEnc_getNodesFromInputBuffer(HANDLE_IISDRCGAINENC_PARAMS hIisDrcGainEnc_params,
                                      const float *const *drcGainInputBuffer,
                                      const int bIsPrerollFrame) {
  int k;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  IISDRCGAINENC_RETURN(*pFuncSetNodesGeneric)
  (DrcGainSequence *, const float *, const int, int);

  switch (hIisDrcGainEnc_params->nodeSettingMode) {
    case IISDRCGAINENC_NODESETTINGMODE_SINGLENODE:
      pFuncSetNodesGeneric = &setSingleNode;
      break;
    case IISDRCGAINENC_NODESETTINGMODE_ALLNODES:
      pFuncSetNodesGeneric = &setAllNodes;
      break;
    case IISDRCGAINENC_NODESETTINGMODE_SALIENTNODE:
      pFuncSetNodesGeneric = &setSalientGainNode;
      break;
    default:
      return IISDRCGAINENC_RETURN_ERROR_UNSUPPORTEDNODESETTINGMODE;
      break;
  }

  for (k = 0; k < hIisDrcGainEnc_params->gainSequenceCount; k++) {
    DrcGainSequence *pDrcGainSequence = &hIisDrcGainEnc_params->pUniDrcGain->pDrcGainSequence[k];
    if (pDrcGainSequence->gainCodingProfile != IISDRCGAINENC_GAINCODINGPROFILE_CONSTANT) {
      int bIsPrerollFrame_used = bIsPrerollFrame;
      retVal = pFuncSetNodesGeneric(&hIisDrcGainEnc_params->pUniDrcGain->pDrcGainSequence[k], drcGainInputBuffer[k], bIsPrerollFrame_used, hIisDrcGainEnc_params->frameSize);
      if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

      if (pDrcGainSequence->fullFrame == 1) {
        if (pDrcGainSequence->pTimeSamples[pDrcGainSequence->nNodes - 1] != (hIisDrcGainEnc_params->frameSize - 1)) {
          pDrcGainSequence->pTimeSamples[pDrcGainSequence->nNodes] = hIisDrcGainEnc_params->frameSize - 1;
          pDrcGainSequence->pGainDb[pDrcGainSequence->nNodes] = drcGainInputBuffer[k][hIisDrcGainEnc_params->frameSize - 1];
          pDrcGainSequence->pSlope[pDrcGainSequence->nNodes] = 0.f;
          pDrcGainSequence->nNodes++;
        }
      }

      if (pDrcGainSequence->pTimeSamples[pDrcGainSequence->nNodes - 1] == (hIisDrcGainEnc_params->frameSize - 1)) {
        pDrcGainSequence->frameEndFlag = 1;
      } else {
        pDrcGainSequence->frameEndFlag = 0;
      }

      assert(!((pDrcGainSequence->fullFrame == 1) && (pDrcGainSequence->frameEndFlag == 0)));

      if ((pDrcGainSequence->frameEndFlag == 1) && (pDrcGainSequence->nNodes == 1)) {
        pDrcGainSequence->drcGainCodingMode = 0;
      } else {
        pDrcGainSequence->drcGainCodingMode = 1;
      }
    }
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}
