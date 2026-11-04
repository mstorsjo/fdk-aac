
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
#include <float.h>

#include "cFeatSFM2.h"

#include "mathlib.h"
#include "iisutillib.h"

#define m_NBANDS 4

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

static const unsigned int lfreq[] = {302, 603, 1508, 4221};
static const unsigned int ufreq[] = {603, 1508, 4221, 9604};

int cFeatSFM2_close(cFeatSFM2 **phcFeatSFM2) {
  iisFree((*phcFeatSFM2)->m_sfm2);
  iisFree((*phcFeatSFM2)->m_lbin);
  iisFree((*phcFeatSFM2)->m_ubin);
  iisFree((*phcFeatSFM2)->m_oneOverNbins);
  iisFree((*phcFeatSFM2)->m_Nbins);
  iisFree(*phcFeatSFM2);
  *phcFeatSFM2 = NULL;
  return (0);
}

int cFeatSFM2_open(cFeatSFM2 **phcFeatSFM2, unsigned int fs, unsigned int psize) {
  cFeatSFM2 *hcFeatSFM2;
  unsigned int n;

  *phcFeatSFM2 = (cFeatSFM2 *)iisCalloc(1, sizeof(cFeatSFM2));
  if (*phcFeatSFM2 == NULL)
    return (1);

  hcFeatSFM2 = *phcFeatSFM2;

  hcFeatSFM2->m_PSIZE = psize;

  hcFeatSFM2->m_sfm2 = (float *)iisCalloc(m_NBANDS, sizeof(float));

  hcFeatSFM2->m_lbin = (unsigned int *)iisCalloc(m_NBANDS, sizeof(unsigned int));
  for (n = 0; n < m_NBANDS; n++)
    hcFeatSFM2->m_lbin[n] = min((unsigned int)((float)(psize - 1) * 2 * lfreq[n] / fs), psize);

  hcFeatSFM2->m_ubin = (unsigned int *)iisCalloc(m_NBANDS, sizeof(unsigned int));
  for (n = 0; n < m_NBANDS; n++)
    hcFeatSFM2->m_ubin[n] = min((unsigned int)((float)(psize - 1) * 2 * ufreq[n] / fs), psize - 1);

  hcFeatSFM2->m_Nbins = (unsigned int *)iisCalloc(m_NBANDS, sizeof(unsigned int));
  hcFeatSFM2->m_oneOverNbins = (float *)iisCalloc(m_NBANDS, sizeof(float));

  for (n = 0; n < m_NBANDS; n++) {
    hcFeatSFM2->m_Nbins[n] = (hcFeatSFM2->m_ubin[n] - hcFeatSFM2->m_lbin[n] + 1);
    hcFeatSFM2->m_oneOverNbins[n] = 1.0f / hcFeatSFM2->m_Nbins[n];
  }
  return (0);
}

int cFeatSFM2_process(cFeatSFM2 *hcFeatSFM2, float procSpec[]) {
  unsigned int n;

  for (n = 0; n < m_NBANDS; n++) {
    unsigned int k;
    float f_mean_arit = 0.f;
    float f_mean_geom = 0.f;

    hcFeatSFM2->m_sfm2[n] = 0.f;

    for (k = hcFeatSFM2->m_lbin[n] - 1; k < hcFeatSFM2->m_ubin[n]; k++) {
      f_mean_geom += (float)log(procSpec[k] + FLT_MIN);
    }

    f_mean_arit = sumFLOAT(&procSpec[hcFeatSFM2->m_lbin[n] - 1], hcFeatSFM2->m_Nbins[n]);

    if (f_mean_arit != 0.0) {
      f_mean_arit = f_mean_arit * hcFeatSFM2->m_oneOverNbins[n];
      f_mean_geom *= hcFeatSFM2->m_oneOverNbins[n];
      f_mean_geom = (float)exp(f_mean_geom);

      hcFeatSFM2->m_sfm2[n] = f_mean_geom / f_mean_arit;
    }
  }

  return (0);
}
