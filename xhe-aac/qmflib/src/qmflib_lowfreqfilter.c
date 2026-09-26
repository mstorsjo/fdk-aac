
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
#include <string.h>
#include "qmflib_lowfreqfilter.h"
#include "iis_fft.h"

#define FFT_IDX_R(a) (2 * a)
#define FFT_IDX_I(a) (2 * a + 1)

#define MAX_NUMER_OF_QMF_BANDS_TO_HYBRID (5)
#define LOW_FREQUENCY_FILTER_LENGTH (13)

typedef enum {
  LOW_FREQ_2,
  LOW_FREQ_2I,
  LOW_FREQ_4,
  LOW_FREQ_6,
  LOW_FREQ_8,
  LOW_FREQ_PS_2,
  LOW_FREQ_PS_2I,
  LOW_FREQ_PS_6,
  LOW_FREQ_PS_8,
  LOW_FREQ_PS_FINE_4,
  LOW_FREQ_PS_FINE_8,
  LOW_FREQ_PS_FINE_12
} LOW_FREQ_FILTER_CFG;

typedef struct T_LOW_FREQUENCY_FILTER_SETUP {
  int nQmfBandsToHybrid;
  int nHybBands[MAX_NUMER_OF_QMF_BANDS_TO_HYBRID];
  int kHybrid[MAX_NUMER_OF_QMF_BANDS_TO_HYBRID];
  int filterLength;
  int filterDelay;

} LOW_FREQUENCY_FILTER_SETUP;

typedef struct T_LOW_FREQUENCY_FILTER_ANALYSIS {
  float *bufferLFReal[MAX_NUMER_OF_QMF_BANDS_TO_HYBRID];
  float *bufferLFImag[MAX_NUMER_OF_QMF_BANDS_TO_HYBRID];
  float *bufferHFReal[LOW_FREQUENCY_FILTER_LENGTH];
  float *bufferHFImag[LOW_FREQUENCY_FILTER_LENGTH];

  int bufferHFpos;
  int nQmfBands;
  int nCplxQmfBands;
  int alignDelay;

  LOW_FREQUENCY_FILTER_SETUP const *pFilterSetup;

} LOW_FREQUENCY_FILTER_ANALYSIS;

typedef struct T_LOW_FREQUENCY_FILTER_SYNTHESIS {
  int nQmfBands;
  int nCplxQmfBands;

  LOW_FREQUENCY_FILTER_SETUP const *pSetup;

} LOW_FREQUENCY_FILTER_SYNTHESIS;

static const LOW_FREQUENCY_FILTER_SETUP setup_3_16 = {3, {8, 4, 4}, {LOW_FREQ_8, LOW_FREQ_4, LOW_FREQ_4}, LOW_FREQUENCY_FILTER_LENGTH, (LOW_FREQUENCY_FILTER_LENGTH - 1) / 2};
static const LOW_FREQUENCY_FILTER_SETUP setup_3_12 = {3, {8, 2, 2}, {LOW_FREQ_8, LOW_FREQ_2, LOW_FREQ_2}, LOW_FREQUENCY_FILTER_LENGTH, (LOW_FREQUENCY_FILTER_LENGTH - 1) / 2};
static const LOW_FREQUENCY_FILTER_SETUP setup_3_10 = {3, {6, 2, 2}, {LOW_FREQ_6, LOW_FREQ_2I, LOW_FREQ_2}, LOW_FREQUENCY_FILTER_LENGTH, (LOW_FREQUENCY_FILTER_LENGTH - 1) / 2};
static const LOW_FREQUENCY_FILTER_SETUP setup_ps_coarse = {3, {6, 2, 2}, {LOW_FREQ_PS_6, LOW_FREQ_PS_2I, LOW_FREQ_PS_2}, LOW_FREQUENCY_FILTER_LENGTH, (LOW_FREQUENCY_FILTER_LENGTH - 1) / 2};
static const LOW_FREQUENCY_FILTER_SETUP setup_ps_fine = {5, {12, 8, 4, 4, 4}, {LOW_FREQ_PS_FINE_12, LOW_FREQ_PS_FINE_8, LOW_FREQ_PS_FINE_4, LOW_FREQ_PS_FINE_4, LOW_FREQ_PS_FINE_4}, LOW_FREQUENCY_FILTER_LENGTH, (LOW_FREQUENCY_FILTER_LENGTH - 1) / 2};

static const float LowFrequencyFilterCoeffs2[LOW_FREQUENCY_FILTER_LENGTH] =
    {
        0.0f, 0.01899487526049f, 0.0f, -0.07293139167538f,
        0.0f, 0.30596630545168f, 0.50000000000000f, 0.30596630545168f,
        0.0f, -0.07293139167538f, 0.0f, 0.01899487526049f,
        0.0f};

static const float LowFrequencyFilterCoeffs4[LOW_FREQUENCY_FILTER_LENGTH] =
    {
        -0.00305151927305f, -0.00794862316203f, 0.0f, 0.04318924038756f,
        0.12542448210445f, 0.21227807049160f, 0.25f, 0.21227807049160f,
        0.12542448210445f, 0.04318924038756f, 0.0f, -0.00794862316203f,
        -0.00305151927305f};

static const float LowFrequencyFilterCoeffs8[LOW_FREQUENCY_FILTER_LENGTH] =
    {
        0.00746082949812f, 0.02270420949825f, 0.04546865930473f, 0.07266113929591f,
        0.09885108575264f, 0.11793710567217f, 0.125f, 0.11793710567217f,
        0.09885108575264f, 0.07266113929591f, 0.04546865930473f, 0.02270420949825f,
        0.00746082949812f};

static const float LowFrequencyFilterCoeffsFine4[LOW_FREQUENCY_FILTER_LENGTH] =
    {
        -0.05908211155639f, -0.04871498374946f, 0.00000000000000f, 0.07778723915851f,
        0.16486303567403f, 0.23279856662996f, 0.25000000000000f, 0.23279856662996f,
        0.16486303567403f, 0.07778723915851f, 0.00000000000000f, -0.04871498374946f,
        -0.05908211155639f};

static const float LowFrequencyFilterCoeffsFine8[LOW_FREQUENCY_FILTER_LENGTH] =
    {
        0.01565675600122f, 0.03752716391991f, 0.05417891378782f, 0.08417044116767f,
        0.10307344158036f, 0.12222452249753f, 0.12500000000000f, 0.12222452249753f,
        0.10307344158036f, 0.08417044116767f, 0.05417891378782f, 0.03752716391991f,
        0.01565675600122f};

static const float LowFrequencyFilterCoeffsFine12[LOW_FREQUENCY_FILTER_LENGTH] =
    {
        0.04081179924692f, 0.03812810994926f, 0.05144908135699f, 0.06399831151592f,
        0.07428313801106f, 0.08100347892914f, 0.08333333333333f, 0.08100347892914f,
        0.07428313801106f, 0.06399831151592f, 0.05144908135699f, 0.03812810994926f,
        0.04081179924692f};

static void kChannelFiltering(const float *const pQmfDataReal,
                              const float *const pQmfDataImag,
                              float *const pHybridDataReal,
                              float *const pHybridDataImag,
                              const signed int lowFrequencyFilterConfig);

HANDLE_ERROR_INFO QMFlib_LowFrequencyFilterCreateAnalysis(HANDLE_LOW_FREQUENCY_FILTER_ANALYSIS *hLowFreqFilterAnalysis) {
  HANDLE_ERROR_INFO error = noError;

  *hLowFreqFilterAnalysis = (HANDLE_LOW_FREQUENCY_FILTER_ANALYSIS)iisCalloc(1, sizeof(LOW_FREQUENCY_FILTER_ANALYSIS));
  if (*hLowFreqFilterAnalysis == NULL) {
    error = iisUtil_ERROR(CDI, "Unable to calloc for hLowFreqFilterAnalysis.");
  }
  return error;
}

HANDLE_ERROR_INFO QMFlib_LowFrequencyFilterInitAnalysis(
    HANDLE_LOW_FREQUENCY_FILTER_ANALYSIS hLowFreqFilterAnalysis,
    LOW_FREQUENCY_FILTER_MODE mode,
    int nQmfBands,
    int nCplxQmfBands,
    int alignDelay) {
  HANDLE_ERROR_INFO error = noError;
  int k;

  LOW_FREQUENCY_FILTER_SETUP const *setup = NULL;

  switch (mode) {
    case THREE_TO_TEN:
      setup = &setup_3_10;
      break;
    case THREE_TO_TWELVE:
      setup = &setup_3_12;
      break;
    case THREE_TO_SIXTEEN:
      setup = &setup_3_16;
      break;
    case PS_THREE_TO_TEN:
      setup = &setup_ps_coarse;
      break;
    case PS_FIVE_TO_THIRTYTWO:
      setup = &setup_ps_fine;
      break;
    default:
      error = iisUtil_ERROR(CDI, "Invalid hybrid filter bank configuration.");
      break;
  }

  if (error == noError) {
    hLowFreqFilterAnalysis->bufferHFpos = 0;
    hLowFreqFilterAnalysis->nQmfBands = nQmfBands;
    hLowFreqFilterAnalysis->nCplxQmfBands = nCplxQmfBands;
    hLowFreqFilterAnalysis->alignDelay = alignDelay;
    hLowFreqFilterAnalysis->pFilterSetup = setup;
  }

  if (error == noError) {
    if (nCplxQmfBands <= setup->nQmfBandsToHybrid) {
      error = iisUtil_ERROR(CDI, "Number of complex qmf bands must be greater than number of qmf bands to hybrid qmf bands.");
    }
  }

  if (error == noError) {
    for (k = 0; k < setup->nQmfBandsToHybrid; k++) {
      hLowFreqFilterAnalysis->bufferLFReal[k] = (float *)iisCalloc(setup->filterLength, sizeof(float));
      hLowFreqFilterAnalysis->bufferLFImag[k] = (float *)iisCalloc(setup->filterLength, sizeof(float));
    }

    for (k = 0; k < setup->filterDelay; k++) {
      hLowFreqFilterAnalysis->bufferHFReal[k] = (float *)iisCalloc(nQmfBands - setup->nQmfBandsToHybrid, sizeof(float));
      hLowFreqFilterAnalysis->bufferHFImag[k] = (float *)iisCalloc(nCplxQmfBands - setup->nQmfBandsToHybrid, sizeof(float));
    }
  }

  return error;
}

HANDLE_ERROR_INFO
QMFlib_LowFrequencyFilterCalculateAnalysis(HANDLE_LOW_FREQUENCY_FILTER_ANALYSIS hLowFreqFilterAnalysis,
                                           const float *const pQmfDataReal,
                                           const float *const pQmfDataImag,
                                           float *const pHybridDataReal,
                                           float *const pHybridDataImag) {
  HANDLE_ERROR_INFO error = noError;
  int k, hybOffset = 0;
  const int nrQmfBandsLF = hLowFreqFilterAnalysis->pFilterSetup->nQmfBandsToHybrid;

  for (k = 0; k < nrQmfBandsLF; k++) {
    memmove(&hLowFreqFilterAnalysis->bufferLFReal[k][0], &hLowFreqFilterAnalysis->bufferLFReal[k][1], (hLowFreqFilterAnalysis->pFilterSetup->filterLength - 1) * sizeof(float));
    memmove(&hLowFreqFilterAnalysis->bufferLFImag[k][0], &hLowFreqFilterAnalysis->bufferLFImag[k][1], (hLowFreqFilterAnalysis->pFilterSetup->filterLength - 1) * sizeof(float));

    hLowFreqFilterAnalysis->bufferLFReal[k][hLowFreqFilterAnalysis->pFilterSetup->filterLength - 1] = pQmfDataReal[k];
    hLowFreqFilterAnalysis->bufferLFImag[k][hLowFreqFilterAnalysis->pFilterSetup->filterLength - 1] = pQmfDataImag[k];
  }

  for (k = 0; k < nrQmfBandsLF; k++) {
    kChannelFiltering(
        hLowFreqFilterAnalysis->bufferLFReal[k],
        hLowFreqFilterAnalysis->bufferLFImag[k],
        pHybridDataReal + hybOffset,
        pHybridDataImag + hybOffset,
        hLowFreqFilterAnalysis->pFilterSetup->kHybrid[k]);

    hybOffset += hLowFreqFilterAnalysis->pFilterSetup->nHybBands[k];
  }

  if (hLowFreqFilterAnalysis->alignDelay != 0) {
    memcpy(pHybridDataReal + hybOffset, &pQmfDataReal[nrQmfBandsLF], (hLowFreqFilterAnalysis->nQmfBands - nrQmfBandsLF) * sizeof(float));
    memcpy(pHybridDataImag + hybOffset, &pQmfDataImag[nrQmfBandsLF], (hLowFreqFilterAnalysis->nCplxQmfBands - nrQmfBandsLF) * sizeof(float));
  } else {
    memcpy(pHybridDataReal + hybOffset, hLowFreqFilterAnalysis->bufferHFReal[hLowFreqFilterAnalysis->bufferHFpos], (hLowFreqFilterAnalysis->nQmfBands - nrQmfBandsLF) * sizeof(float));
    memcpy(pHybridDataImag + hybOffset, hLowFreqFilterAnalysis->bufferHFImag[hLowFreqFilterAnalysis->bufferHFpos], (hLowFreqFilterAnalysis->nCplxQmfBands - nrQmfBandsLF) * sizeof(float));

    memcpy(hLowFreqFilterAnalysis->bufferHFReal[hLowFreqFilterAnalysis->bufferHFpos], &pQmfDataReal[nrQmfBandsLF], (hLowFreqFilterAnalysis->nQmfBands - nrQmfBandsLF) * sizeof(float));
    memcpy(hLowFreqFilterAnalysis->bufferHFImag[hLowFreqFilterAnalysis->bufferHFpos], &pQmfDataImag[nrQmfBandsLF], (hLowFreqFilterAnalysis->nCplxQmfBands - nrQmfBandsLF) * sizeof(float));

    if (++hLowFreqFilterAnalysis->bufferHFpos >= hLowFreqFilterAnalysis->pFilterSetup->filterDelay)
      hLowFreqFilterAnalysis->bufferHFpos = 0;
  }
  return error;
}

HANDLE_ERROR_INFO
QMFlib_LowFrequencyFilterDestroyAnalysis(HANDLE_LOW_FREQUENCY_FILTER_ANALYSIS *hLowFreqFilterAnalysis) {
  HANDLE_ERROR_INFO error = noError;
  int k;
  if (*hLowFreqFilterAnalysis != NULL) {
    for (k = 0; k < (*hLowFreqFilterAnalysis)->pFilterSetup->nQmfBandsToHybrid; k++) {
      if ((*hLowFreqFilterAnalysis)->bufferLFReal[k] != NULL) {
        iisFree((*hLowFreqFilterAnalysis)->bufferLFReal[k]);
        (*hLowFreqFilterAnalysis)->bufferLFReal[k] = NULL;
      }
      if ((*hLowFreqFilterAnalysis)->bufferLFImag[k] != NULL) {
        iisFree((*hLowFreqFilterAnalysis)->bufferLFImag[k]);
        (*hLowFreqFilterAnalysis)->bufferLFImag[k] = NULL;
      }
    }

    for (k = 0; k < (*hLowFreqFilterAnalysis)->pFilterSetup->filterDelay; k++) {
      iisFree((*hLowFreqFilterAnalysis)->bufferHFReal[k]);
      (*hLowFreqFilterAnalysis)->bufferHFReal[k] = NULL;
      iisFree((*hLowFreqFilterAnalysis)->bufferHFImag[k]);
      (*hLowFreqFilterAnalysis)->bufferHFImag[k] = NULL;
    }
    iisFree(*hLowFreqFilterAnalysis);
  }
  *hLowFreqFilterAnalysis = NULL;
  return error;
}

HANDLE_ERROR_INFO
QMFlib_LowFrequencyFilterCreateSynthesis(HANDLE_LOW_FREQUENCY_FILTER_SYNTHESIS *hLowFreqFilterSynthesis) {
  HANDLE_ERROR_INFO error = noError;

  *hLowFreqFilterSynthesis = (HANDLE_LOW_FREQUENCY_FILTER_SYNTHESIS)iisCalloc(1, sizeof(LOW_FREQUENCY_FILTER_SYNTHESIS));
  return error;
}

HANDLE_ERROR_INFO
QMFlib_LowFrequencyFilterInitSynthesis(
    HANDLE_LOW_FREQUENCY_FILTER_SYNTHESIS hLowFreqFilterSynthesis,
    const LOW_FREQUENCY_FILTER_MODE mode,
    const int nQmfBands,
    const int nCplxQmfBands) {
  HANDLE_ERROR_INFO error = noError;
  LOW_FREQUENCY_FILTER_SETUP const *setup = NULL;

  switch (mode) {
    case THREE_TO_TEN:
      setup = &setup_3_10;
      break;
    case THREE_TO_TWELVE:
      setup = &setup_3_12;
      break;
    case THREE_TO_SIXTEEN:
      setup = &setup_3_16;
      break;
    case PS_THREE_TO_TEN:
      setup = &setup_ps_coarse;
      break;
    case PS_FIVE_TO_THIRTYTWO:
      setup = &setup_ps_fine;
      break;
    default:
      error = iisUtil_ERROR(CDI, "Invalid low frequency filter mode.");
      break;
  }

  hLowFreqFilterSynthesis->pSetup = setup;
  hLowFreqFilterSynthesis->nQmfBands = nQmfBands;
  hLowFreqFilterSynthesis->nCplxQmfBands = nCplxQmfBands;

  if (nCplxQmfBands <= setup->nQmfBandsToHybrid) {
    error = iisUtil_ERROR(CDI, "Number of complex qmf bands must be greater than number of qmf bands to hybrid qmf bands.");
  }

  return error;
}

HANDLE_ERROR_INFO
QMFlib_LowFrequencyFilterCalculateSynthesis(
    HANDLE_LOW_FREQUENCY_FILTER_SYNTHESIS hLowFreqFilterSynthesis,
    const float *const pHybridReal,
    const float *const pHybridImag,
    float *const pQmfReal,
    float *const pQmfImag) {
  HANDLE_ERROR_INFO error = noError;
  int k, n, hybOffset = 0;
  int nrQmfBandsLF = hLowFreqFilterSynthesis->pSetup->nQmfBandsToHybrid;
  float accu1 = 0.f;
  float accu2 = 0.f;

  for (k = 0; k < nrQmfBandsLF; k++) {
    accu1 = 0.f;
    accu2 = 0.f;

    for (n = 0; n < hLowFreqFilterSynthesis->pSetup->nHybBands[k]; n++) {
      accu1 += pHybridReal[hybOffset + n];
      accu2 += pHybridImag[hybOffset + n];
    }
    hybOffset += hLowFreqFilterSynthesis->pSetup->nHybBands[k];

    pQmfReal[k] = accu1;
    pQmfImag[k] = accu2;
  }

  memcpy(&pQmfReal[nrQmfBandsLF], &pHybridReal[hybOffset], (hLowFreqFilterSynthesis->nQmfBands - nrQmfBandsLF) * sizeof(float));
  memcpy(&pQmfImag[nrQmfBandsLF], &pHybridImag[hybOffset], (hLowFreqFilterSynthesis->nCplxQmfBands - nrQmfBandsLF) * sizeof(float));
  return error;
}

HANDLE_ERROR_INFO
QMFlib_LowFrequencyFilterDestroySynthesis(
    HANDLE_LOW_FREQUENCY_FILTER_SYNTHESIS *hLowFreqFilterSynthesis) {
  HANDLE_ERROR_INFO error = noError;

  if (*hLowFreqFilterSynthesis != NULL)
    iisFree(*hLowFreqFilterSynthesis);
  *hLowFreqFilterSynthesis = NULL;

  return error;
}

HANDLE_ERROR_INFO
QMFlib_LowFrequencyFilterGetDelay(
    HANDLE_LOW_FREQUENCY_FILTER_ANALYSIS hLowFreqFilterAnalysis,
    int *delay) {
  HANDLE_ERROR_INFO error = noError;

  if ((hLowFreqFilterAnalysis == NULL) || (delay == NULL)) {
    error = iisUtil_ERROR(CDI, "Invalid input param");
  }

  if (error == noError) {
    *delay = hLowFreqFilterAnalysis->pFilterSetup->filterDelay;
  }
  return error;
}

HANDLE_ERROR_INFO
QMFlib_LowFrequencyFilterGetFilterLength(
    HANDLE_LOW_FREQUENCY_FILTER_ANALYSIS hLowFreqFilterAnalysis,
    int *length) {
  HANDLE_ERROR_INFO error = noError;

  if ((hLowFreqFilterAnalysis == NULL) || (length == NULL)) {
    error = iisUtil_ERROR(CDI, "Invalid input param");
  }

  if (error == noError) {
    *length = hLowFreqFilterAnalysis->pFilterSetup->filterLength;
    ;
  }
  return error;
}

HANDLE_ERROR_INFO QMFlib_LowFrequencyFilterGetBandResolution(
    HANDLE_LOW_FREQUENCY_FILTER_ANALYSIS hLowFreqFilterAnalysis,
    int const qmfBand,
    int *nHybridSubBands) {
  HANDLE_ERROR_INFO error = noError;

  if ((hLowFreqFilterAnalysis == NULL) || (nHybridSubBands == NULL)) {
    error = iisUtil_ERROR(CDI, "Invalid input param");
  }

  if (error == noError) {
    *nHybridSubBands = ((hLowFreqFilterAnalysis)->pFilterSetup)->nHybBands[qmfBand];
  }
  return error;
}

HANDLE_ERROR_INFO QMFlib_LowFrequencyFilterGetNumQmfBands(
    HANDLE_LOW_FREQUENCY_FILTER_ANALYSIS hLowFreqFilterAnalysis,
    int *nHybridQmfBands) {
  HANDLE_ERROR_INFO error = noError;
  if ((hLowFreqFilterAnalysis == NULL) || (nHybridQmfBands == NULL)) {
    error = iisUtil_ERROR(CDI, "Invalid input param");
  }

  if (error == noError) {
    *nHybridQmfBands = ((hLowFreqFilterAnalysis)->pFilterSetup)->nQmfBandsToHybrid;
  }

  return error;
}

HANDLE_ERROR_INFO QMFlib_LowFrequencyFilterGetNumLFBands(
    HANDLE_LOW_FREQUENCY_FILTER_ANALYSIS hLowFreqFilterAnalysis,
    int *nHybridBands) {
  HANDLE_ERROR_INFO error = noError;
  int nHybridBandsLocal = 0;
  int i;

  if (hLowFreqFilterAnalysis == NULL || nHybridBands == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid pointer for hybrid filter");
  }

  if (error == noError) {
    for (i = 0; i < ((hLowFreqFilterAnalysis)->pFilterSetup)->nQmfBandsToHybrid; i++) {
      nHybridBandsLocal += ((hLowFreqFilterAnalysis)->pFilterSetup)->nHybBands[i];
    }

    *nHybridBands = nHybridBandsLocal;
  }

  return error;
}

static void dualChannelFiltering(
    const float *const pQmfReal,
    const float *const pQmfImag,
    float *const pHybridReal,
    float *const pHybridImag,
    const int invert) {
  const float *p2_13_20 = LowFrequencyFilterCoeffs2;
  float r1, r3, r5, r6;
  float i1, i3, i5, i6;

  r1 = p2_13_20[1] * (pQmfReal[1] + pQmfReal[11]);
  r3 = p2_13_20[3] * (pQmfReal[3] + pQmfReal[9]);
  r5 = p2_13_20[5] * (pQmfReal[5] + pQmfReal[7]);
  r6 = p2_13_20[6] * pQmfReal[6];

  i1 = p2_13_20[1] * (pQmfImag[1] + pQmfImag[11]);
  i3 = p2_13_20[3] * (pQmfImag[3] + pQmfImag[9]);
  i5 = p2_13_20[5] * (pQmfImag[5] + pQmfImag[7]);
  i6 = p2_13_20[6] * pQmfImag[6];

  if (invert) {
    pHybridReal[1] = r1 + r3 + r5 + r6;
    pHybridImag[1] = i1 + i3 + i5 + i6;

    pHybridReal[0] = r6 - r1 - r3 - r5;
    pHybridImag[0] = i6 - i1 - i3 - i5;
  } else {
    pHybridReal[0] = r1 + r3 + r5 + r6;
    pHybridImag[0] = i1 + i3 + i5 + i6;

    pHybridReal[1] = r6 - r1 - r3 - r5;
    pHybridImag[1] = i6 - i1 - i3 - i5;
  }
}

static void fourChannelFiltering(
    const float *const pQmfReal,
    const float *const pQmfImag,
    float *const pHybridReal,
    float *const pHybridImag,
    const float *const p) {
  float pfft[8];

  static const float cr[13] = {
      0.f,
      -0.70710678118655f,
      -1.f,
      -0.70710678118655f,
      0.f,
      0.70710678118655f,
      1.f,
      0.70710678118655f,
      0.f,
      -0.70710678118655f,
      -1.f,
      -0.70710678118655f,
      0.f};

  static const float ci[13] = {
      -1.f,
      -0.70710678118655f,
      0.f,
      0.70710678118655f,
      1.f,
      0.70710678118655f,
      0.f,
      -0.70710678118655f,
      -1.f,
      -0.70710678118655f,
      0.f,
      0.70710678118655f,
      1.f};

  pfft[FFT_IDX_R(0)] = (p[10] * (cr[2] * pQmfReal[2] - ci[2] * pQmfImag[2]) +
                        p[6] * (cr[6] * pQmfReal[6] - ci[6] * pQmfImag[6]) +
                        p[2] * (cr[10] * pQmfReal[10] - ci[10] * pQmfImag[10]));
  pfft[FFT_IDX_I(0)] = (p[10] * (ci[2] * pQmfReal[2] + cr[2] * pQmfImag[2]) +
                        p[6] * (ci[6] * pQmfReal[6] + cr[6] * pQmfImag[6]) +
                        p[2] * (ci[10] * pQmfReal[10] + cr[10] * pQmfImag[10]));

  pfft[FFT_IDX_R(1)] = (p[9] * (cr[3] * pQmfReal[3] - ci[3] * pQmfImag[3]) +
                        p[5] * (cr[7] * pQmfReal[7] - ci[7] * pQmfImag[7]) +
                        p[1] * (cr[11] * pQmfReal[11] - ci[11] * pQmfImag[11]));
  pfft[FFT_IDX_I(1)] = (p[9] * (ci[3] * pQmfReal[3] + cr[3] * pQmfImag[3]) +
                        p[5] * (ci[7] * pQmfReal[7] + cr[7] * pQmfImag[7]) +
                        p[1] * (ci[11] * pQmfReal[11] + cr[11] * pQmfImag[11]));

  pfft[FFT_IDX_R(2)] = (p[12] * (cr[0] * pQmfReal[0] - ci[0] * pQmfImag[0]) +
                        p[8] * (cr[4] * pQmfReal[4] - ci[4] * pQmfImag[4]) +
                        p[4] * (cr[8] * pQmfReal[8] - ci[8] * pQmfImag[8]) +
                        p[0] * (cr[12] * pQmfReal[12] - ci[12] * pQmfImag[12]));
  pfft[FFT_IDX_I(2)] = (p[12] * (ci[0] * pQmfReal[0] + cr[0] * pQmfImag[0]) +
                        p[8] * (ci[4] * pQmfReal[4] + cr[4] * pQmfImag[4]) +
                        p[4] * (ci[8] * pQmfReal[8] + cr[8] * pQmfImag[8]) +
                        p[0] * (ci[12] * pQmfReal[12] + cr[12] * pQmfImag[12]));

  pfft[FFT_IDX_R(3)] = (p[11] * (cr[1] * pQmfReal[1] - ci[1] * pQmfImag[1]) +
                        p[7] * (cr[5] * pQmfReal[5] - ci[5] * pQmfImag[5]) +
                        p[3] * (cr[9] * pQmfReal[9] - ci[9] * pQmfImag[9]));
  pfft[FFT_IDX_I(3)] = (p[11] * (ci[1] * pQmfReal[1] + cr[1] * pQmfImag[1]) +
                        p[7] * (ci[5] * pQmfReal[5] + cr[5] * pQmfImag[5]) +
                        p[3] * (ci[9] * pQmfReal[9] + cr[9] * pQmfImag[9]));

  pHybridReal[0] = pfft[FFT_IDX_R(0)] + pfft[FFT_IDX_R(1)] + pfft[FFT_IDX_R(2)] + pfft[FFT_IDX_R(3)];
  pHybridImag[0] = pfft[FFT_IDX_I(0)] + pfft[FFT_IDX_I(1)] + pfft[FFT_IDX_I(2)] + pfft[FFT_IDX_I(3)];

  pHybridReal[1] = pfft[FFT_IDX_R(0)] + pfft[FFT_IDX_I(1)] - pfft[FFT_IDX_R(2)] - pfft[FFT_IDX_I(3)];
  pHybridImag[1] = pfft[FFT_IDX_I(0)] - pfft[FFT_IDX_R(1)] - pfft[FFT_IDX_I(2)] + pfft[FFT_IDX_R(3)];

  pHybridReal[2] = pfft[FFT_IDX_R(0)] - pfft[FFT_IDX_R(1)] + pfft[FFT_IDX_R(2)] - pfft[FFT_IDX_R(3)];
  pHybridImag[2] = pfft[FFT_IDX_I(0)] - pfft[FFT_IDX_I(1)] + pfft[FFT_IDX_I(2)] - pfft[FFT_IDX_I(3)];

  pHybridReal[3] = pfft[FFT_IDX_R(0)] - pfft[FFT_IDX_I(1)] - pfft[FFT_IDX_R(2)] + pfft[FFT_IDX_I(3)];
  pHybridImag[3] = pfft[FFT_IDX_I(0)] + pfft[FFT_IDX_R(1)] - pfft[FFT_IDX_I(2)] - pfft[FFT_IDX_R(3)];
}

static void eightChannelFiltering(
    const float *const pQmfReal,
    const float *const pQmfImag,
    float *const pHybridReal,
    float *const pHybridImag,
    const int outputMode,
    const float *const p) {
  int k;

  ALIGN_16_BYTE float pfft[16];

  ALIGN_16_BYTE static const float cr[13] = {
      -0.70710678118655f,
      -0.38268343236509f,
      0.f,
      0.38268343236509f,
      0.70710678118655f,
      0.92387953251129f,
      1.f,
      0.92387953251129f,
      0.70710678118655f,
      0.38268343236509f,
      0.f,
      -0.38268343236509f,
      -0.70710678118655f};

  ALIGN_16_BYTE static const float ci[13] = {
      0.70710678118655f,
      0.92387953251129f,
      1.f,
      0.92387953251129f,
      0.70710678118655f,
      0.38268343236509f,
      0.f,
      -0.38268343236509f,
      -0.70710678118655f,
      -0.92387953251129f,
      -1.f,
      -0.92387953251129f,
      -0.70710678118655f};

  pfft[FFT_IDX_R(0)] = p[6] * (cr[6] * pQmfReal[6] - ci[6] * pQmfImag[6]);
  pfft[FFT_IDX_I(0)] = p[6] * (ci[6] * pQmfReal[6] + cr[6] * pQmfImag[6]);

  pfft[FFT_IDX_R(1)] = p[7] * (cr[7] * pQmfReal[7] - ci[7] * pQmfImag[7]);
  pfft[FFT_IDX_I(1)] = p[7] * (ci[7] * pQmfReal[7] + cr[7] * pQmfImag[7]);

  pfft[FFT_IDX_R(2)] = (p[0] * (cr[0] * pQmfReal[0] - ci[0] * pQmfImag[0]) +
                        p[8] * (cr[8] * pQmfReal[8] - ci[8] * pQmfImag[8]));
  pfft[FFT_IDX_I(2)] = (p[0] * (ci[0] * pQmfReal[0] + cr[0] * pQmfImag[0]) +
                        p[8] * (ci[8] * pQmfReal[8] + cr[8] * pQmfImag[8]));

  pfft[FFT_IDX_R(3)] = (p[1] * (cr[1] * pQmfReal[1] - ci[1] * pQmfImag[1]) +
                        p[9] * (cr[9] * pQmfReal[9] - ci[9] * pQmfImag[9]));
  pfft[FFT_IDX_I(3)] = (p[1] * (ci[1] * pQmfReal[1] + cr[1] * pQmfImag[1]) +
                        p[9] * (ci[9] * pQmfReal[9] + cr[9] * pQmfImag[9]));

  pfft[FFT_IDX_R(4)] = (p[2] * (cr[2] * pQmfReal[2] - ci[2] * pQmfImag[2]) +
                        p[10] * (cr[10] * pQmfReal[10] - ci[10] * pQmfImag[10]));
  pfft[FFT_IDX_I(4)] = (p[2] * (ci[2] * pQmfReal[2] + cr[2] * pQmfImag[2]) +
                        p[10] * (ci[10] * pQmfReal[10] + cr[10] * pQmfImag[10]));

  pfft[FFT_IDX_R(5)] = (p[3] * (cr[3] * pQmfReal[3] - ci[3] * pQmfImag[3]) +
                        p[11] * (cr[11] * pQmfReal[11] - ci[11] * pQmfImag[11]));
  pfft[FFT_IDX_I(5)] = (p[3] * (ci[3] * pQmfReal[3] + cr[3] * pQmfImag[3]) +
                        p[11] * (ci[11] * pQmfReal[11] + cr[11] * pQmfImag[11]));

  pfft[FFT_IDX_R(6)] = (p[4] * (cr[4] * pQmfReal[4] - ci[4] * pQmfImag[4]) +
                        p[12] * (cr[12] * pQmfReal[12] - ci[12] * pQmfImag[12]));
  pfft[FFT_IDX_I(6)] = (p[4] * (ci[4] * pQmfReal[4] + cr[4] * pQmfImag[4]) +
                        p[12] * (ci[12] * pQmfReal[12] + cr[12] * pQmfImag[12]));

  pfft[FFT_IDX_R(7)] = p[5] * (cr[5] * pQmfReal[5] - ci[5] * pQmfImag[5]);
  pfft[FFT_IDX_I(7)] = p[5] * (ci[5] * pQmfReal[5] + cr[5] * pQmfImag[5]);

  iis_fftf(pfft, 8);

  switch (outputMode) {
    case 0:

      for (k = 0; k < 8; k++) {
        pHybridReal[k] = pfft[FFT_IDX_R(k)];
        pHybridImag[k] = pfft[FFT_IDX_I(k)];
      }
      break;
    case 1:
      pHybridReal[0] = pfft[FFT_IDX_R(7)];
      pHybridImag[0] = pfft[FFT_IDX_I(7)];
      pHybridReal[1] = pfft[FFT_IDX_R(0)];
      pHybridImag[1] = pfft[FFT_IDX_I(0)];

      pHybridReal[2] = pfft[FFT_IDX_R(6)];
      pHybridImag[2] = pfft[FFT_IDX_I(6)];
      pHybridReal[3] = pfft[FFT_IDX_R(1)];
      pHybridImag[3] = pfft[FFT_IDX_I(1)];

      pHybridReal[4] = pfft[FFT_IDX_R(2)];
      pHybridReal[4] += pfft[FFT_IDX_R(5)];
      pHybridImag[4] = pfft[FFT_IDX_I(2)];
      pHybridImag[4] += pfft[FFT_IDX_I(5)];

      pHybridReal[5] = pfft[FFT_IDX_R(3)];
      pHybridReal[5] += pfft[FFT_IDX_R(4)];
      pHybridImag[5] = pfft[FFT_IDX_I(3)];
      pHybridImag[5] += pfft[FFT_IDX_I(4)];
      break;

    case 2:
      pHybridReal[0] = pfft[FFT_IDX_R(6)];
      pHybridImag[0] = pfft[FFT_IDX_I(6)];

      pHybridReal[1] = pfft[FFT_IDX_R(7)];
      pHybridImag[1] = pfft[FFT_IDX_I(7)];

      pHybridReal[2] = pfft[FFT_IDX_R(0)];
      pHybridImag[2] = pfft[FFT_IDX_I(0)];

      pHybridReal[3] = pfft[FFT_IDX_R(1)];
      pHybridImag[3] = pfft[FFT_IDX_I(1)];

      pHybridReal[4] = pfft[FFT_IDX_R(2)];
      pHybridReal[4] += pfft[FFT_IDX_R(5)];
      pHybridImag[4] = pfft[FFT_IDX_I(2)];
      pHybridImag[4] += pfft[FFT_IDX_I(5)];

      pHybridReal[5] = pfft[FFT_IDX_R(3)];
      pHybridReal[5] += pfft[FFT_IDX_R(4)];
      pHybridImag[5] = pfft[FFT_IDX_I(3)];
      pHybridImag[5] += pfft[FFT_IDX_I(4)];
      break;
    default:
      assert(0);
      break;
  }
}

static void twelveChannelFiltering(const float *const pQmfReal,
                                   const float *const pQmfImag,
                                   float *const pHybridReal,
                                   float *const pHybridImag,
                                   const float *const p) {
  ALIGN_16_BYTE float fft[24];

  ALIGN_16_BYTE static const float cr[13] =
      {
          0.f, 0.25881904510252f, 0.5f,
          0.70710678118655f, 0.86602540378444f, 0.96592582628906f,
          1.f,
          0.96592582628906f, 0.86602540378444f, 0.70710678118655f,
          0.5f, 0.25881904510252f, 0.f};

  ALIGN_16_BYTE static const float ci[13] =
      {
          1.f, 0.96592582628906f, 0.86602540378444f,
          0.70710678118655f, 0.5f, 0.25881904510252f,
          0.f,
          -0.25881904510252f, -0.5f, -0.70710678118655f,
          -0.86602540378444f, -0.96592582628906f, -1.f};

  int bin;

  fft[FFT_IDX_R(0)] = p[6] * (cr[6] * pQmfReal[6] - ci[6] * pQmfImag[6]);
  fft[FFT_IDX_I(0)] = p[6] * (ci[6] * pQmfReal[6] + cr[6] * pQmfImag[6]);

  fft[FFT_IDX_R(1)] = p[5] * (cr[7] * pQmfReal[7] - ci[7] * pQmfImag[7]);
  fft[FFT_IDX_I(1)] = p[5] * (ci[7] * pQmfReal[7] + cr[7] * pQmfImag[7]);

  fft[FFT_IDX_R(2)] = p[4] * (cr[8] * pQmfReal[8] - ci[8] * pQmfImag[8]);
  fft[FFT_IDX_I(2)] = p[4] * (ci[8] * pQmfReal[8] + cr[8] * pQmfImag[8]);

  fft[FFT_IDX_R(3)] = p[3] * (cr[9] * pQmfReal[9] - ci[9] * pQmfImag[9]);
  fft[FFT_IDX_I(3)] = p[3] * (ci[9] * pQmfReal[9] + cr[9] * pQmfImag[9]);

  fft[FFT_IDX_R(4)] = p[2] * (cr[10] * pQmfReal[10] - ci[10] * pQmfImag[10]);
  fft[FFT_IDX_I(4)] = p[2] * (ci[10] * pQmfReal[10] + cr[10] * pQmfImag[10]);

  fft[FFT_IDX_R(5)] = p[1] * (cr[11] * pQmfReal[11] - ci[11] * pQmfImag[11]);
  fft[FFT_IDX_I(5)] = p[1] * (ci[11] * pQmfReal[11] + cr[11] * pQmfImag[11]);

  fft[FFT_IDX_R(6)] = (p[12] * (cr[0] * pQmfReal[0] - ci[0] * pQmfImag[0]) +
                       p[0] * (cr[12] * pQmfReal[12] - ci[12] * pQmfImag[12]));
  fft[FFT_IDX_I(6)] = (p[12] * (ci[0] * pQmfReal[0] + cr[0] * pQmfImag[0]) +
                       p[0] * (ci[12] * pQmfReal[12] + cr[12] * pQmfImag[12]));

  fft[FFT_IDX_R(7)] = p[11] * (cr[1] * pQmfReal[1] - ci[1] * pQmfImag[1]);
  fft[FFT_IDX_I(7)] = p[11] * (ci[1] * pQmfReal[1] + cr[1] * pQmfImag[1]);

  fft[FFT_IDX_R(8)] = p[10] * (cr[2] * pQmfReal[2] - ci[2] * pQmfImag[2]);
  fft[FFT_IDX_I(8)] = p[10] * (ci[2] * pQmfReal[2] + cr[2] * pQmfImag[2]);

  fft[FFT_IDX_R(9)] = p[9] * (cr[3] * pQmfReal[3] - ci[3] * pQmfImag[3]);
  fft[FFT_IDX_I(9)] = p[9] * (ci[3] * pQmfReal[3] + cr[3] * pQmfImag[3]);

  fft[FFT_IDX_R(10)] = p[8] * (cr[4] * pQmfReal[4] - ci[4] * pQmfImag[4]);
  fft[FFT_IDX_I(10)] = p[8] * (ci[4] * pQmfReal[4] + cr[4] * pQmfImag[4]);

  fft[FFT_IDX_R(11)] = p[7] * (cr[5] * pQmfReal[5] - ci[5] * pQmfImag[5]);
  fft[FFT_IDX_I(11)] = p[7] * (ci[5] * pQmfReal[5] + cr[5] * pQmfImag[5]);

  iis_fftf(fft, 12);

  for (bin = 0; bin < 12; bin++) {
    pHybridReal[bin] = fft[FFT_IDX_R(bin)];
    pHybridImag[bin] = fft[FFT_IDX_I(bin)];
  }
}

static void kChannelFiltering(const float *const pQmfDataReal,
                              const float *const pQmfDataImag,
                              float *const pHybridQmfDataReal,
                              float *const pHybridQmfDataImag,
                              const signed int lfFilterConfig) {
  switch (lfFilterConfig) {
    case LOW_FREQ_2:
    case LOW_FREQ_PS_2:
      dualChannelFiltering(pQmfDataReal, pQmfDataImag, pHybridQmfDataReal, pHybridQmfDataImag, 0);
      break;
    case LOW_FREQ_2I:
    case LOW_FREQ_PS_2I:
      dualChannelFiltering(pQmfDataReal, pQmfDataImag, pHybridQmfDataReal, pHybridQmfDataImag, 1);
      break;
    case LOW_FREQ_4:
      fourChannelFiltering(pQmfDataReal, pQmfDataImag, pHybridQmfDataReal, pHybridQmfDataImag, LowFrequencyFilterCoeffs4);
      break;
    case LOW_FREQ_6:

      eightChannelFiltering(pQmfDataReal, pQmfDataImag, pHybridQmfDataReal, pHybridQmfDataImag, 1, LowFrequencyFilterCoeffs8);
      break;
    case LOW_FREQ_PS_6:

      eightChannelFiltering(pQmfDataReal, pQmfDataImag, pHybridQmfDataReal, pHybridQmfDataImag, 2, LowFrequencyFilterCoeffs8);
      break;
    case LOW_FREQ_8:
    case LOW_FREQ_PS_8:
      eightChannelFiltering(pQmfDataReal, pQmfDataImag, pHybridQmfDataReal, pHybridQmfDataImag, 0, LowFrequencyFilterCoeffs8);
      break;
    case LOW_FREQ_PS_FINE_4:
      fourChannelFiltering(pQmfDataReal, pQmfDataImag, pHybridQmfDataReal, pHybridQmfDataImag, LowFrequencyFilterCoeffsFine4);
      break;
    case LOW_FREQ_PS_FINE_8:
      eightChannelFiltering(pQmfDataReal, pQmfDataImag, pHybridQmfDataReal, pHybridQmfDataImag, 0, LowFrequencyFilterCoeffsFine8);
      break;
    case LOW_FREQ_PS_FINE_12:
      twelveChannelFiltering(pQmfDataReal, pQmfDataImag, pHybridQmfDataReal, pHybridQmfDataImag, LowFrequencyFilterCoeffsFine12);
      break;
  }
}
