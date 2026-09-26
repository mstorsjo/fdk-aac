
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
#include "cFeatMFCC.h"

#include "iisutillib.h"
#include "mathlib.h"

static const float melLowerFreq[] =
    {
        133.33330000000f,
        199.99996666000f,
        266.66663332000f,
        333.33329998000f,
        399.99996664000f,
        466.66663330000f,
        533.33329996000f,
        599.99996662000f,
        666.66663328000f,
        733.33329994000f,
        799.99996660000f,
        866.66663326000f,
        933.33329992000f,
        999.75891087530f,
        1070.91205248996f,
        1147.12918453929f,
        1228.77071274171f,
        1316.22269299875f,
        1409.89865692628f,
        1510.24156730932f,
        1617.72591272719f,
        1732.85995125376f,
        1856.18811384247f,
        1988.29357876108f,
        2129.80102924958f,
        2281.37960744158f,
        2443.74607851708f,
        2617.66822004896f,
        2803.96845257031f,
        3003.52772853027f,
        3217.28969802809f,
        3446.26517102366f,
        3691.53689712497f,
        3954.26468555442f,
        4235.69088950473f,
        4537.14628081805f,
        4860.05634276776f,
        5205.94801069944f,
        5576.45689240532f,
        5973.33500237487f,
};

static const float melUpperFreq[] =
    {
        266.66663332000f,
        333.33329998000f,
        399.99996664000f,
        466.66663330000f,
        533.33329996000f,
        599.99996662000f,
        666.66663328000f,
        733.33329994000f,
        799.99996660000f,
        866.66663326000f,
        933.33329992000f,
        999.75891087530f,
        1070.91205248996f,
        1147.12918453929f,
        1228.77071274171f,
        1316.22269299875f,
        1409.89865692628f,
        1510.24156730932f,
        1617.72591272719f,
        1732.85995125376f,
        1856.18811384247f,
        1988.29357876108f,
        2129.80102924958f,
        2281.37960744158f,
        2443.74607851708f,
        2617.66822004896f,
        2803.96845257031f,
        3003.52772853027f,
        3217.28969802809f,
        3446.26517102366f,
        3691.53689712497f,
        3954.26468555442f,
        4235.69088950473f,
        4537.14628081805f,
        4860.05634276776f,
        5205.94801069944f,
        5576.45689240532f,
        5973.33500237487f,
        6398.45904649439f,
        6853.83929637111f,
};

static const float mfccDCTMatrix[m_MEL_NBANDS][m_MFCC_MODELORDER + 1] = {
    {
        0.15811388f,
        0.22343441f,
        0.22291749f,
        0.22205686f,
        0.22085383f,
        0.21931026f,
        0.21742852f,
        0.21521153f,
        0.21266270f,
        0.20978596f,
        0.20658574f,
        0.20306699f,
        0.19923512f,
    },
    {
        0.15811388f,
        0.22205686f,
        0.21742852f,
        0.20978596f,
        0.19923512f,
        0.18592226f,
        0.17003194f,
        0.15178446f,
        0.13143278f,
        0.10925903f,
        0.08557062f,
        0.06069593f,
        0.03497981f,
    },
    {
        0.15811388f,
        0.21931026f,
        0.20658574f,
        0.18592226f,
        0.15811388f,
        0.12422928f,
        0.08557062f,
        0.04362352f,
        0.00000000f,
        -0.04362352f,
        -0.08557062f,
        -0.12422928f,
        -0.15811388f,
    },
    {
        0.15811388f,
        0.21521153f,
        0.19065614f,
        0.15178446f,
        0.10151536f,
        0.04362352f,
        -0.01754399f,
        -0.07739413f,
        -0.13143278f,
        -0.17560220f,
        -0.20658574f,
        -0.22205686f,
        -0.22085383f,
    },
    {
        0.15811388f,
        0.20978596f,
        0.17003194f,
        0.10925903f,
        0.03497981f,
        -0.04362352f,
        -0.11683423f,
        -0.17560220f,
        -0.21266270f,
        -0.22343441f,
        -0.20658574f,
        -0.16419950f,
        -0.10151536f,
    },
    {
        0.15811388f,
        0.20306699f,
        0.14522100f,
        0.06069593f,
        -0.03497981f,
        -0.12422928f,
        -0.19065614f,
        -0.22205686f,
        -0.21266270f,
        -0.16419950f,
        -0.08557062f,
        0.00877876f,
        0.10151536f,
    },
    {
        0.15811388f,
        0.19509604f,
        0.11683423f,
        0.00877876f,
        -0.10151536f,
        -0.18592226f,
        -0.22291749f,
        -0.20306699f,
        -0.13143278f,
        -0.02628216f,
        0.08557062f,
        0.17560220f,
        0.22085383f,
    },
    {
        0.15811388f,
        0.18592226f,
        0.08557062f,
        -0.04362352f,
        -0.15811388f,
        -0.21931026f,
        -0.20658574f,
        -0.12422928f,
        -0.00000000f,
        0.12422928f,
        0.20658574f,
        0.21931026f,
        0.15811388f,
    },
    {
        0.15811388f,
        0.17560220f,
        0.05219997f,
        -0.09361516f,
        -0.19923512f,
        -0.21931026f,
        -0.14522100f,
        -0.00877876f,
        0.13143278f,
        0.21521153f,
        0.20658574f,
        0.10925903f,
        -0.03497981f,
    },
    {
        0.15811388f,
        0.16419950f,
        0.01754399f,
        -0.13843362f,
        -0.22085383f,
        -0.18592226f,
        -0.05219997f,
        0.10925903f,
        0.21266270f,
        0.20306699f,
        0.08557062f,
        -0.07739413f,
        -0.19923512f,
    },
    {
        0.15811388f,
        0.15178446f,
        -0.01754399f,
        -0.17560220f,
        -0.22085383f,
        -0.12422928f,
        0.05219997f,
        0.19509604f,
        0.21266270f,
        0.09361516f,
        -0.08557062f,
        -0.20978596f,
        -0.19923512f,
    },
    {
        0.15811388f,
        0.13843362f,
        -0.05219997f,
        -0.20306699f,
        -0.19923512f,
        -0.04362352f,
        0.14522100f,
        0.22343441f,
        0.13143278f,
        -0.06069593f,
        -0.20658574f,
        -0.19509604f,
        -0.03497981f,
    },
    {
        0.15811388f,
        0.12422928f,
        -0.08557062f,
        -0.21931026f,
        -0.15811388f,
        0.04362352f,
        0.20658574f,
        0.18592226f,
        0.00000000f,
        -0.18592226f,
        -0.20658574f,
        -0.04362352f,
        0.15811388f,
    },
    {
        0.15811388f,
        0.10925903f,
        -0.11683423f,
        -0.22343441f,
        -0.10151536f,
        0.12422928f,
        0.22291749f,
        0.09361516f,
        -0.13143278f,
        -0.22205686f,
        -0.08557062f,
        0.13843362f,
        0.22085383f,
    },
    {
        0.15811388f,
        0.09361516f,
        -0.14522100f,
        -0.21521153f,
        -0.03497981f,
        0.18592226f,
        0.19065614f,
        -0.02628216f,
        -0.21266270f,
        -0.15178446f,
        0.08557062f,
        0.22343441f,
        0.10151536f,
    },
    {
        0.15811388f,
        0.07739413f,
        -0.17003194f,
        -0.19509604f,
        0.03497981f,
        0.21931026f,
        0.11683423f,
        -0.13843362f,
        -0.21266270f,
        -0.00877876f,
        0.20658574f,
        0.15178446f,
        -0.10151536f,
    },
    {
        0.15811388f,
        0.06069593f,
        -0.19065614f,
        -0.16419950f,
        0.10151536f,
        0.21931026f,
        0.01754399f,
        -0.20978596f,
        -0.13143278f,
        0.13843362f,
        0.20658574f,
        -0.02628216f,
        -0.22085383f,
    },
    {
        0.15811388f,
        0.04362352f,
        -0.20658574f,
        -0.12422928f,
        0.15811388f,
        0.18592226f,
        -0.08557062f,
        -0.21931026f,
        -0.00000000f,
        0.21931026f,
        0.08557062f,
        -0.18592226f,
        -0.15811388f,
    },
    {
        0.15811388f,
        0.02628216f,
        -0.21742852f,
        -0.07739413f,
        0.19923512f,
        0.12422928f,
        -0.17003194f,
        -0.16419950f,
        0.13143278f,
        0.19509604f,
        -0.08557062f,
        -0.21521153f,
        0.03497981f,
    },
    {
        0.15811388f,
        0.00877876f,
        -0.22291749f,
        -0.02628216f,
        0.22085383f,
        0.04362352f,
        -0.21742852f,
        -0.06069593f,
        0.21266270f,
        0.07739413f,
        -0.20658574f,
        -0.09361516f,
        0.19923512f,
    },
    {
        0.15811388f,
        -0.00877876f,
        -0.22291749f,
        0.02628216f,
        0.22085383f,
        -0.04362352f,
        -0.21742852f,
        0.06069593f,
        0.21266270f,
        -0.07739413f,
        -0.20658574f,
        0.09361516f,
        0.19923512f,
    },
    {
        0.15811388f,
        -0.02628216f,
        -0.21742852f,
        0.07739413f,
        0.19923512f,
        -0.12422928f,
        -0.17003194f,
        0.16419950f,
        0.13143278f,
        -0.19509604f,
        -0.08557062f,
        0.21521153f,
        0.03497981f,
    },
    {
        0.15811388f,
        -0.04362352f,
        -0.20658574f,
        0.12422928f,
        0.15811388f,
        -0.18592226f,
        -0.08557062f,
        0.21931026f,
        0.00000000f,
        -0.21931026f,
        0.08557062f,
        0.18592226f,
        -0.15811388f,
    },
    {
        0.15811388f,
        -0.06069593f,
        -0.19065614f,
        0.16419950f,
        0.10151536f,
        -0.21931026f,
        0.01754399f,
        0.20978596f,
        -0.13143278f,
        -0.13843362f,
        0.20658574f,
        0.02628216f,
        -0.22085383f,
    },
    {
        0.15811388f,
        -0.07739413f,
        -0.17003194f,
        0.19509604f,
        0.03497981f,
        -0.21931026f,
        0.11683423f,
        0.13843362f,
        -0.21266270f,
        0.00877876f,
        0.20658574f,
        -0.15178446f,
        -0.10151536f,
    },
    {
        0.15811388f,
        -0.09361516f,
        -0.14522100f,
        0.21521153f,
        -0.03497981f,
        -0.18592226f,
        0.19065614f,
        0.02628216f,
        -0.21266270f,
        0.15178446f,
        0.08557062f,
        -0.22343441f,
        0.10151536f,
    },
    {
        0.15811388f,
        -0.10925903f,
        -0.11683423f,
        0.22343441f,
        -0.10151536f,
        -0.12422928f,
        0.22291749f,
        -0.09361516f,
        -0.13143278f,
        0.22205686f,
        -0.08557062f,
        -0.13843362f,
        0.22085383f,
    },
    {
        0.15811388f,
        -0.12422928f,
        -0.08557062f,
        0.21931026f,
        -0.15811388f,
        -0.04362352f,
        0.20658574f,
        -0.18592226f,
        0.00000000f,
        0.18592226f,
        -0.20658574f,
        0.04362352f,
        0.15811388f,
    },
    {
        0.15811388f,
        -0.13843362f,
        -0.05219997f,
        0.20306699f,
        -0.19923512f,
        0.04362352f,
        0.14522100f,
        -0.22343441f,
        0.13143278f,
        0.06069593f,
        -0.20658574f,
        0.19509604f,
        -0.03497981f,
    },
    {
        0.15811388f,
        -0.15178446f,
        -0.01754399f,
        0.17560220f,
        -0.22085383f,
        0.12422928f,
        0.05219997f,
        -0.19509604f,
        0.21266270f,
        -0.09361516f,
        -0.08557062f,
        0.20978596f,
        -0.19923512f,
    },
    {
        0.15811388f,
        -0.16419950f,
        0.01754399f,
        0.13843362f,
        -0.22085383f,
        0.18592226f,
        -0.05219997f,
        -0.10925903f,
        0.21266270f,
        -0.20306699f,
        0.08557062f,
        0.07739413f,
        -0.19923512f,
    },
    {
        0.15811388f,
        -0.17560220f,
        0.05219997f,
        0.09361516f,
        -0.19923512f,
        0.21931026f,
        -0.14522100f,
        0.00877876f,
        0.13143278f,
        -0.21521153f,
        0.20658574f,
        -0.10925903f,
        -0.03497981f,
    },
    {
        0.15811388f,
        -0.18592226f,
        0.08557062f,
        0.04362352f,
        -0.15811388f,
        0.21931026f,
        -0.20658574f,
        0.12422928f,
        0.00000000f,
        -0.12422928f,
        0.20658574f,
        -0.21931026f,
        0.15811388f,
    },
    {
        0.15811388f,
        -0.19509604f,
        0.11683423f,
        -0.00877876f,
        -0.10151536f,
        0.18592226f,
        -0.22291749f,
        0.20306699f,
        -0.13143278f,
        0.02628216f,
        0.08557062f,
        -0.17560220f,
        0.22085383f,
    },
    {
        0.15811388f,
        -0.20306699f,
        0.14522100f,
        -0.06069593f,
        -0.03497981f,
        0.12422928f,
        -0.19065614f,
        0.22205686f,
        -0.21266270f,
        0.16419950f,
        -0.08557062f,
        -0.00877876f,
        0.10151536f,
    },
    {
        0.15811388f,
        -0.20978596f,
        0.17003194f,
        -0.10925903f,
        0.03497981f,
        0.04362352f,
        -0.11683423f,
        0.17560220f,
        -0.21266270f,
        0.22343441f,
        -0.20658574f,
        0.16419950f,
        -0.10151536f,
    },
    {
        0.15811388f,
        -0.21521153f,
        0.19065614f,
        -0.15178446f,
        0.10151536f,
        -0.04362352f,
        -0.01754399f,
        0.07739413f,
        -0.13143278f,
        0.17560220f,
        -0.20658574f,
        0.22205686f,
        -0.22085383f,
    },
    {
        0.15811388f,
        -0.21931026f,
        0.20658574f,
        -0.18592226f,
        0.15811388f,
        -0.12422928f,
        0.08557062f,
        -0.04362352f,
        -0.00000000f,
        0.04362352f,
        -0.08557062f,
        0.12422928f,
        -0.15811388f,
    },
    {
        0.15811388f,
        -0.22205686f,
        0.21742852f,
        -0.20978596f,
        0.19923512f,
        -0.18592226f,
        0.17003194f,
        -0.15178446f,
        0.13143278f,
        -0.10925903f,
        0.08557062f,
        -0.06069593f,
        0.03497981f,
    },
    {
        0.15811388f,
        -0.22343441f,
        0.22291749f,
        -0.22205686f,
        0.22085383f,
        -0.21931026f,
        0.21742852f,
        -0.21521153f,
        0.21266270f,
        -0.20978596f,
        0.20658574f,
        -0.20306699f,
        0.19923512f,
    },
};

int cFeatMFCC_close(cFeatMFCC **phcFeatMFCC) {
  iisFree((*phcFeatMFCC)->m_pMelBand);
  iisFree((*phcFeatMFCC)->m_melLowerFreq);
  iisFree((*phcFeatMFCC)->m_melUpperFreq);
  iisFree((*phcFeatMFCC)->m_melLowerBin);
  iisFree((*phcFeatMFCC)->m_melUpperBin);
  iisFree((*phcFeatMFCC)->m_mfccDCTMat);
  iisFree((*phcFeatMFCC)->m_mfcc);

  iisFree(*phcFeatMFCC);
  *phcFeatMFCC = NULL;

  return (0);
}
int cFeatMFCC_open(cFeatMFCC **phcFeatMFCC, unsigned int fs, unsigned int fftsize, unsigned int psize) {
  unsigned int n;
  float factor;

  cFeatMFCC *hcFeatMFCC;

  *phcFeatMFCC = (cFeatMFCC *)iisCalloc(1, sizeof(cFeatMFCC));
  if (*phcFeatMFCC == NULL)
    return (1);

  hcFeatMFCC = *phcFeatMFCC;

  hcFeatMFCC->m_FS = fs;
  hcFeatMFCC->m_FFTSIZE = fftsize;
  hcFeatMFCC->m_PSIZE = psize;

  hcFeatMFCC->m_pMelBand = (float *)iisCalloc(m_MEL_NBANDS, sizeof(float));

  hcFeatMFCC->m_melLowerFreq = (float *)iisCalloc(m_MEL_NBANDS, sizeof(float));
  memcpy(hcFeatMFCC->m_melLowerFreq, melLowerFreq, sizeof(float) * m_MEL_NBANDS);

  hcFeatMFCC->m_melUpperFreq = (float *)iisCalloc(m_MEL_NBANDS, sizeof(float));
  memcpy(hcFeatMFCC->m_melUpperFreq, melUpperFreq, sizeof(float) * m_MEL_NBANDS);

  hcFeatMFCC->m_melLowerBin = (unsigned int *)iisCalloc(m_MEL_NBANDS, sizeof(unsigned int));
  hcFeatMFCC->m_melUpperBin = (unsigned int *)iisCalloc(m_MEL_NBANDS, sizeof(unsigned int));

  factor = (float)hcFeatMFCC->m_FFTSIZE / hcFeatMFCC->m_FS;
  for (n = 0; n < m_MEL_NBANDS; n++) {
    hcFeatMFCC->m_melLowerBin[n] = (unsigned int)(floor(hcFeatMFCC->m_melLowerFreq[n] * factor));
    hcFeatMFCC->m_melUpperBin[n] = (unsigned int)(ceil(hcFeatMFCC->m_melUpperFreq[n] * factor));
  }

  for (n = 0; n < m_MEL_NBANDS; n++) {
    assert(hcFeatMFCC->m_melLowerBin[n] > 0);
    assert(hcFeatMFCC->m_melUpperBin[n] > 3);
    assert(hcFeatMFCC->m_melUpperBin[n] > hcFeatMFCC->m_melLowerBin[n]);
  }

  hcFeatMFCC->m_mfcc = (float *)iisCalloc(m_MFCC_MODELORDER + 1, sizeof(float));

  hcFeatMFCC->m_mfccDCTMat = (float *)iisCalloc((m_MFCC_MODELORDER + 1) * m_MEL_NBANDS, sizeof(float));

  memcpy(hcFeatMFCC->m_mfccDCTMat, mfccDCTMatrix, sizeof(float) * 13 * 40);

  return (0);
}

int cFeatMFCC_process(cFeatMFCC *hcFeatMFCC, float powerSpec[]) {
  unsigned int n = 0, m = 0;
  memset(hcFeatMFCC->m_pMelBand, 0, sizeof(float) * m_MEL_NBANDS);

  for (n = 0; n < m_MEL_NBANDS; n++) {
    float triangleHeight = 2.f / (hcFeatMFCC->m_melUpperBin[n] - hcFeatMFCC->m_melLowerBin[n]);

    hcFeatMFCC->m_pMelBand[n] = 0.f;

    hcFeatMFCC->m_pMelBand[n] += 0.125f * powerSpec[hcFeatMFCC->m_melLowerBin[n] - 1] + 0.125f * powerSpec[hcFeatMFCC->m_melUpperBin[n] - 1];

    if (hcFeatMFCC->m_melUpperBin[n] - hcFeatMFCC->m_melLowerBin[n] == 3) {
      hcFeatMFCC->m_pMelBand[n] += 0.5f * powerSpec[hcFeatMFCC->m_melLowerBin[n] + 1] + 0.5f * powerSpec[hcFeatMFCC->m_melUpperBin[n] - 3];
    }

    if (hcFeatMFCC->m_melUpperBin[n] - hcFeatMFCC->m_melLowerBin[n] == 4) {
      hcFeatMFCC->m_pMelBand[n] += 0.25f * powerSpec[hcFeatMFCC->m_melLowerBin[n]] + 0.25f * powerSpec[hcFeatMFCC->m_melUpperBin[n] - 2];
      hcFeatMFCC->m_pMelBand[n] += 0.5f * powerSpec[hcFeatMFCC->m_melLowerBin[n] + 1];
    }

    if (hcFeatMFCC->m_melUpperBin[n] - hcFeatMFCC->m_melLowerBin[n] > 4) {
      hcFeatMFCC->m_pMelBand[n] += 0.25f * powerSpec[hcFeatMFCC->m_melLowerBin[n]] + 0.25f * powerSpec[hcFeatMFCC->m_melUpperBin[n] - 2];
      hcFeatMFCC->m_pMelBand[n] += 0.5f * powerSpec[hcFeatMFCC->m_melLowerBin[n] + 1] + 0.5f * powerSpec[hcFeatMFCC->m_melUpperBin[n] - 3];
    }

    for (m = hcFeatMFCC->m_melLowerBin[n] + 2; m < hcFeatMFCC->m_melUpperBin[n] - 3; m++)
      hcFeatMFCC->m_pMelBand[n] += powerSpec[m];

    hcFeatMFCC->m_pMelBand[n] *= triangleHeight;
  }

  for (m = 1; m < m_MFCC_MODELORDER + 1; m++) {
    hcFeatMFCC->m_mfcc[m - 1] = 0.f;

    for (n = 0; n < m_MEL_NBANDS; n++)
      hcFeatMFCC->m_mfcc[m - 1] = hcFeatMFCC->m_mfcc[m - 1] + hcFeatMFCC->m_mfccDCTMat[n * (m_MFCC_MODELORDER + 1) + m] * (float)log10(hcFeatMFCC->m_pMelBand[n]);

    hcFeatMFCC->m_mfcc[m - 1] = 0.1f * hcFeatMFCC->m_mfcc[m - 1];
  }

  return (0);
}
