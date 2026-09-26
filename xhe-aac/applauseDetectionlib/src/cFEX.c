
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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "cFEX.h"

#include "mathlib.h"
#include "iisutillib.h"

#define m_SF_NBANDS 4
#define m_DELTA_FILT_ORDER 1

static const float delta_iir_coeff_a[2] = {1.f, -0.67959929822453f};
static const float delta_iir_coeff_b[2] = {0.83979964911226f, -0.83979964911226f};

int cFEX_close(cFEX **phcFEX) {
  cFeatMFCC_close(&(*phcFEX)->m_featMFCC);
  cFeatSFM2_close(&(*phcFEX)->m_featSFM2);
  cFeatSpecCentroid_close(&(*phcFEX)->m_featSC);
  cFeatSpecFlux_close(&(*phcFEX)->m_featSF);
  cFeatSpecSpread_close(&(*phcFEX)->m_featSSP);

  iisFree((*phcFEX)->m_feat);
  iisFree((*phcFEX)->m_dfeat);
  iisFree((*phcFEX)->m_sfeat);
  iisFree((*phcFEX)->m_featSet);

  iisFree((*phcFEX)->m_dbuf_x);
  iisFree((*phcFEX)->m_dbuf_y);

  iisFree((*phcFEX)->m_sbuf_y);

  iisFree((*phcFEX)->m_delta_iir_coeff_a);
  iisFree((*phcFEX)->m_delta_iir_coeff_b);

  iisFree(*phcFEX);
  *phcFEX = NULL;

  return (0);
}

int cFEX_open(cFEX **phcFEX, unsigned int fs, unsigned int fftsize, unsigned int psize,
              unsigned int featdim, unsigned int featsetdim) {
  cFEX *hcFEX;

  *phcFEX = (cFEX *)iisCalloc(1, sizeof(cFEX));
  if (*phcFEX == NULL)
    return (1);

  hcFEX = *phcFEX;

  hcFEX->m_FS = fs;
  hcFEX->m_PSIZE = fftsize;
  hcFEX->m_PSIZE = psize;
  hcFEX->m_FEAT_DIM = featdim;
  hcFEX->m_FEAT_SET_DIM = featsetdim;

  cFeatMFCC_open(&hcFEX->m_featMFCC, fs, fftsize, psize);
  cFeatSFM2_open(&hcFEX->m_featSFM2, fs, psize);
  cFeatSpecCentroid_open(&hcFEX->m_featSC, fs, psize);
  cFeatSpecSpread_open(&hcFEX->m_featSSP, fs, psize);
  cFeatSpecFlux_open(&hcFEX->m_featSF, fs, psize);

  hcFEX->m_dtctOn = 0;

  hcFEX->m_feat = (float *)iisCalloc(hcFEX->m_FEAT_DIM, sizeof(float));
  hcFEX->m_dfeat = (float *)iisCalloc(hcFEX->m_FEAT_DIM, sizeof(float));
  hcFEX->m_sfeat = (float *)iisCalloc(hcFEX->m_FEAT_DIM, sizeof(float));

  hcFEX->m_featSet = (float *)iisCalloc(hcFEX->m_FEAT_SET_DIM, sizeof(float));

  hcFEX->m_delta_iir_coeff_a = (float *)iisCalloc(m_DELTA_FILT_ORDER + 1, sizeof(float));
  memcpy(hcFEX->m_delta_iir_coeff_a, delta_iir_coeff_a, sizeof(float) * (m_DELTA_FILT_ORDER + 1));

  hcFEX->m_delta_iir_coeff_b = (float *)iisCalloc(m_DELTA_FILT_ORDER + 1, sizeof(float));
  memcpy(hcFEX->m_delta_iir_coeff_b, delta_iir_coeff_b, sizeof(float) * (m_DELTA_FILT_ORDER + 1));

  hcFEX->m_dbuf_x = (float *)iisCalloc(hcFEX->m_FEAT_DIM, sizeof(float));
  hcFEX->m_dbuf_y = (float *)iisCalloc(hcFEX->m_FEAT_DIM, sizeof(float));

  hcFEX->m_sbuf_y = (float *)iisCalloc(hcFEX->m_FEAT_DIM, sizeof(float));

  return (0);
}

int cFEX_process(cFEX *hcFEX, float procSpec[]) {
  unsigned int n = 0;

  cFeatSpecCentroid_process(hcFEX->m_featSC, procSpec);
  cFeatSFM2_process(hcFEX->m_featSFM2, procSpec);
  cFeatSpecSpread_process(hcFEX->m_featSSP, procSpec, hcFEX->m_featSC->m_sc);
  cFeatSpecFlux_process(hcFEX->m_featSF, procSpec);

  for (n = 0; n < hcFEX->m_PSIZE; n++)
    procSpec[n] = procSpec[n] * procSpec[n];

  hcFEX->m_rms = sumFLOAT(&procSpec[0], hcFEX->m_PSIZE);

  if (hcFEX->m_rms == 0.f) {
    memset(hcFEX->m_featSet, 0, sizeof(float) * hcFEX->m_FEAT_SET_DIM);
    return (0);
  }

  cFeatMFCC_process(hcFEX->m_featMFCC, procSpec);

  copyFLOAT(&(hcFEX->m_featSC->m_sc[0]), &(hcFEX->m_feat[0]), m_SF_NBANDS);

  copyFLOAT(&(hcFEX->m_featSFM2->m_sfm2[0]), &(hcFEX->m_feat[m_SF_NBANDS]), m_SF_NBANDS);

  copyFLOAT(&(hcFEX->m_featSSP->m_ssp[0]), &(hcFEX->m_feat[2 * m_SF_NBANDS]), m_SF_NBANDS);

  copyFLOAT(&(hcFEX->m_featSF->m_sf[0]), &(hcFEX->m_feat[3 * m_SF_NBANDS]), m_SF_NBANDS);

  copyFLOAT(&(hcFEX->m_featMFCC->m_mfcc[0]), &(hcFEX->m_feat[4 * m_SF_NBANDS]), m_MFCC_MODELORDER - 3);

  for (n = 0; n < hcFEX->m_FEAT_DIM; n++)
    hcFEX->m_sfeat[n] = 0.1f * hcFEX->m_feat[n] + 0.9f * hcFEX->m_sbuf_y[n];

  memcpy(hcFEX->m_sbuf_y, hcFEX->m_sfeat, sizeof(float) * (hcFEX->m_FEAT_DIM));

  for (n = 0; n < hcFEX->m_FEAT_DIM; n++)
    hcFEX->m_dfeat[n] = hcFEX->m_delta_iir_coeff_b[0] * hcFEX->m_feat[n] + hcFEX->m_delta_iir_coeff_b[1] * hcFEX->m_dbuf_x[n] - hcFEX->m_delta_iir_coeff_a[1] * hcFEX->m_dbuf_y[n];

  memcpy(hcFEX->m_dbuf_x, hcFEX->m_feat, sizeof(float) * (hcFEX->m_FEAT_DIM));
  memcpy(hcFEX->m_dbuf_y, hcFEX->m_dfeat, sizeof(float) * (hcFEX->m_FEAT_DIM));

  memcpy(hcFEX->m_featSet, hcFEX->m_feat, sizeof(float) * (hcFEX->m_FEAT_DIM));
  memcpy(hcFEX->m_featSet + hcFEX->m_FEAT_DIM, hcFEX->m_dfeat, sizeof(float) * (hcFEX->m_FEAT_DIM));
  memcpy(hcFEX->m_featSet + 2 * hcFEX->m_FEAT_DIM, hcFEX->m_sfeat, sizeof(float) * (hcFEX->m_FEAT_DIM));

  return (0);
}
