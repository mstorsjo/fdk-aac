
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
#include <assert.h>
#include <math.h>

#include "cSpDtct.h"
#include "paramdata.h"
#include "iisutillib.h"

int cSpDtct_open(cSpDtct **phcSpDtct, int frameSize, int fs) {
  int n;
  cSpDtct *hcSpDtct;

  *phcSpDtct = (cSpDtct *)iisCalloc(1, sizeof(cSpDtct));
  if (*phcSpDtct == NULL)
    return (1);

  hcSpDtct = *phcSpDtct;

  hcSpDtct->m_FS = fs;

  hcSpDtct->m_FSIZE = frameSize;

  hcSpDtct->m_FFTSIZE = frameSize;

  hcSpDtct->m_PSIZE = frameSize / 2 + 1;

  hcSpDtct->m_FEAT_DIM = 25;

  hcSpDtct->m_FEAT_SET_DIM = 75;

  InitMathOpt();

  hcSpDtct->m_hIIS_RFFT_f = NULL;

  hcSpDtct->m_iis_fft_inR_f = (float *)iisMalloc(sizeof(float) * hcSpDtct->m_FFTSIZE);
  hcSpDtct->m_iis_fft_out_f = (float *)iisMalloc(sizeof(float) * hcSpDtct->m_FFTSIZE);

  hcSpDtct->m_IIS_FFT_status = IIS_RFFT_Create(&hcSpDtct->m_hIIS_RFFT_f, hcSpDtct->m_FFTSIZE, IIS_FFT_FWD);

  hcSpDtct->m_winFun = (float *)iisCalloc(hcSpDtct->m_FSIZE, sizeof(float));

  for (n = 0; n < hcSpDtct->m_FSIZE; n++) {
    hcSpDtct->m_winFun[n] = 0.5f * (1.f - (float)cos(2.f * 3.14159265358979f * n / (hcSpDtct->m_FSIZE - 1)));
  }

  hcSpDtct->m_procSpec = (float *)iisCalloc(hcSpDtct->m_PSIZE, sizeof(float));
  hcSpDtct->m_classLabel1 = 1;
  hcSpDtct->m_conf1 = 0.f;

  cFEX_open(&hcSpDtct->m_fEX, hcSpDtct->m_FS, hcSpDtct->m_FFTSIZE, hcSpDtct->m_PSIZE, hcSpDtct->m_FEAT_DIM, hcSpDtct->m_FEAT_SET_DIM);

  cFeatNormalize_open(&hcSpDtct->m_featNorm, &Xmean_1, &Xstd_1);

  cSdNeuralNet_open(&hcSpDtct->m_classifier1, &nnModel_w1_1, &nnModel_w2_1, &nnModel_b1_1, &nnModel_b2_1);

  hcSpDtct->m_features1 = (float *)iisCalloc(hcSpDtct->m_FEAT_SET_DIM, sizeof(float));

  return (0);
}

int cSpDtct_close(cSpDtct **phcSpDtct) {
  cFeatNormalize_close(&(*phcSpDtct)->m_featNorm);
  cFEX_close(&(*phcSpDtct)->m_fEX);
  cSdNeuralNet_close(&(*phcSpDtct)->m_classifier1);

  IIS_RFFT_Destroy(&(*phcSpDtct)->m_hIIS_RFFT_f);
  iisFree((*phcSpDtct)->m_iis_fft_inR_f);
  iisFree((*phcSpDtct)->m_iis_fft_out_f);

  iisFree((*phcSpDtct)->m_winFun);
  iisFree((*phcSpDtct)->m_features1);
  iisFree((*phcSpDtct)->m_procSpec);

  iisFree(*phcSpDtct);
  *phcSpDtct = NULL;

  return (0);
}

int cSpDtct_main(cSpDtct *hcSpDtct, const float input[], unsigned short int *classLabel, float *classConf) {
  unsigned int m = 1;
  int n;
  float a;

  multFLOAT(&input[0], &(hcSpDtct->m_winFun[0]), &(hcSpDtct->m_iis_fft_inR_f[0]), hcSpDtct->m_FSIZE);

  hcSpDtct->m_IIS_FFT_status = IIS_FFT_Apply_RFFT(hcSpDtct->m_hIIS_RFFT_f, &(hcSpDtct->m_iis_fft_inR_f[0]), &(hcSpDtct->m_iis_fft_out_f[0]));

  assert(!(hcSpDtct->m_FFTSIZE % 2));

  hcSpDtct->m_procSpec[0] = (float)fabs(hcSpDtct->m_iis_fft_out_f[0]);

  hcSpDtct->m_procSpec[hcSpDtct->m_FFTSIZE / 2] = (float)fabs(hcSpDtct->m_iis_fft_out_f[1]);

  for (n = 2; n < hcSpDtct->m_FFTSIZE; n += 2) {
    hcSpDtct->m_procSpec[m] = (float)sqrt(hcSpDtct->m_iis_fft_out_f[n] * hcSpDtct->m_iis_fft_out_f[n] +
                                          hcSpDtct->m_iis_fft_out_f[n + 1] * hcSpDtct->m_iis_fft_out_f[n + 1]);
    m++;
  }

  a = 1.f / (float)sqrt((float)hcSpDtct->m_FFTSIZE);
  smulFLOAT(a, &(hcSpDtct->m_procSpec[0]), &(hcSpDtct->m_procSpec[0]), hcSpDtct->m_PSIZE);

  cFEX_process(hcSpDtct->m_fEX, hcSpDtct->m_procSpec);

  cFeatNormalize_process(hcSpDtct->m_featNorm, hcSpDtct->m_features1, hcSpDtct->m_fEX->m_featSet);

  cSdNeuralNet_classify(hcSpDtct->m_classifier1, &hcSpDtct->m_classLabel1, &hcSpDtct->m_conf1, hcSpDtct->m_features1);

  *classLabel = hcSpDtct->m_classLabel1;
  *classConf = hcSpDtct->m_conf1;

  return (0);
}
