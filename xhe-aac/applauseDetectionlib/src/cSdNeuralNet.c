
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
#include <stdlib.h>
#include <math.h>

#include "iisutillib.h"
#include "mathlib.h"

#include "cSdNeuralNet.h"
#include "paramdata.h"

int cSdNeuralNet_close(cSdNeuralNet **phcSdNeuralNet) {
  iisFree((*phcSdNeuralNet)->m_z);
  iisFree((*phcSdNeuralNet)->m_a);
  iisFree(*phcSdNeuralNet);
  *phcSdNeuralNet = NULL;
  return (0);
}

int cSdNeuralNet_classify(cSdNeuralNet *hcSdNeuralNet, unsigned short int *label, float *conf, float *in) {
  unsigned int n = 0;
  unsigned int maxIdx = 0;

  for (n = 0; n < hcSdNeuralNet->m_nhidden; n++) {
    hcSdNeuralNet->m_z[n] = dotFLOAT(&in[0], &(hcSdNeuralNet->m_w1[n * hcSdNeuralNet->m_nin]), hcSdNeuralNet->m_nin);

    hcSdNeuralNet->m_z[n] += hcSdNeuralNet->m_b1[n];

    hcSdNeuralNet->m_z[n] = (float)tanh(hcSdNeuralNet->m_z[n]);
  }

  for (n = 0; n < hcSdNeuralNet->m_nout; n++) {
    hcSdNeuralNet->m_a[n] = dotFLOAT(&(hcSdNeuralNet->m_z[0]), &(hcSdNeuralNet->m_w2[n * hcSdNeuralNet->m_nhidden]), hcSdNeuralNet->m_nhidden);

    hcSdNeuralNet->m_a[n] += hcSdNeuralNet->m_b2[n];

    if (hcSdNeuralNet->m_a[n] > 36.0437f)
      hcSdNeuralNet->m_a[n] = 36.0437f;

    if (hcSdNeuralNet->m_a[n] < -708.3964f)
      hcSdNeuralNet->m_a[n] = -708.3964f;

    hcSdNeuralNet->m_a[n] = 1.0f / (1.0f + (float)exp(-hcSdNeuralNet->m_a[n]));
  }

  *conf = 0;

  for (n = 1; n < hcSdNeuralNet->m_nout; n++) {
    if (hcSdNeuralNet->m_a[n] > hcSdNeuralNet->m_a[maxIdx])
      maxIdx = n;
  }

  *label = maxIdx;

  return (0);
}

int cSdNeuralNet_open(cSdNeuralNet **phcSdNeuralNet, t_paramdata *nnModel_w1, t_paramdata *nnModel_w2,
                      t_paramdata *nnModel_b1, t_paramdata *nnModel_b2) {
  cSdNeuralNet *hcSdNeuralNet;

  *phcSdNeuralNet = (cSdNeuralNet *)iisCalloc(1, sizeof(cSdNeuralNet));
  if (*phcSdNeuralNet == NULL)
    return (1);

  hcSdNeuralNet = *phcSdNeuralNet;

  hcSdNeuralNet->m_w1_dimOut = nnModel_w1->m_nrows;
  hcSdNeuralNet->m_w1_dimIn = nnModel_w1->m_ncols;
  hcSdNeuralNet->m_w1 = nnModel_w1->m_data;

  hcSdNeuralNet->m_b1_dimOut = nnModel_b1->m_nrows;
  hcSdNeuralNet->m_b1_dimIn = nnModel_b1->m_ncols;
  hcSdNeuralNet->m_b1 = nnModel_b1->m_data;

  hcSdNeuralNet->m_w2_dimOut = nnModel_w2->m_nrows;
  hcSdNeuralNet->m_w2_dimIn = nnModel_w2->m_ncols;
  hcSdNeuralNet->m_w2 = nnModel_w2->m_data;

  hcSdNeuralNet->m_b2_dimOut = nnModel_b2->m_nrows;
  hcSdNeuralNet->m_b2_dimIn = nnModel_b2->m_ncols;
  hcSdNeuralNet->m_b2 = nnModel_b2->m_data;

  hcSdNeuralNet->m_nin = hcSdNeuralNet->m_w1_dimOut;
  hcSdNeuralNet->m_nhidden = hcSdNeuralNet->m_w1_dimIn;
  hcSdNeuralNet->m_nout = hcSdNeuralNet->m_w2_dimIn;

  hcSdNeuralNet->m_z = (float *)iisCalloc(hcSdNeuralNet->m_nhidden, sizeof(float));

  hcSdNeuralNet->m_a = (float *)iisCalloc(hcSdNeuralNet->m_nout, sizeof(float));

  return (0);
}
