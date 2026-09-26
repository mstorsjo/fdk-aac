
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
#include <assert.h>
#include "options.h"
#include "iisLPDComLib_tcx_Transform.h"
#include <math.h>
#include <string.h>
#include "mathlib.h"
#include "iisutillib.h"
#include "iis_fft.h"

#ifndef PI
#define PI 3.14159265358979323846264338327950288
#endif

struct T_TCX_DCT4 {
  int nLines;
  float *pPreTwiddleReal;
  float *pPreTwiddleImag;
  float *pBufferOdd;
  float *pBufferEven;
  float *pBufferReal;
  float *pBufferImag;
  float *pBufferTmp1;
  float *pBufferTmp2;
  float *pPostTwiddleReal;
  float *pPostTwiddleImag;
  HANDLE_IIS_FFT hIisFft;
};

typedef struct T_TCX_MDCT {
  HANDLE_TCX_DCT4 hDct4_1152;
  HANDLE_TCX_DCT4 hDct4_1088;
  HANDLE_TCX_DCT4 hDct4_1024;
  HANDLE_TCX_DCT4 hDct4_0768;
  HANDLE_TCX_DCT4 hDct4_0576;
  HANDLE_TCX_DCT4 hDct4_0512;
  HANDLE_TCX_DCT4 hDct4_0384;
  HANDLE_TCX_DCT4 hDct4_0320;
  HANDLE_TCX_DCT4 hDct4_0256;
  HANDLE_TCX_DCT4 hDct4_0192;
  HANDLE_TCX_DCT4 hDct4_0128;
  HANDLE_TCX_DCT4 hDct4_0096;
  HANDLE_TCX_DCT4 hDct4_0064;
  HANDLE_TCX_DCT4 hDct4_0048;

} TCX_MDCT;

const float lpdcom_sineWindow96[96] =
    {
        0.008181F, 0.024541F, 0.040895F, 0.057237F, 0.073565F, 0.089872F, 0.106156F, 0.122411F,
        0.138633F, 0.154818F, 0.170962F, 0.187060F, 0.203108F, 0.219101F, 0.235036F, 0.250908F,
        0.266713F, 0.282446F, 0.298104F, 0.313682F, 0.329176F, 0.344581F, 0.359895F, 0.375112F,
        0.390229F, 0.405241F, 0.420145F, 0.434936F, 0.449611F, 0.464166F, 0.478596F, 0.492898F,
        0.507068F, 0.521103F, 0.534998F, 0.548749F, 0.562354F, 0.575808F, 0.589108F, 0.602251F,
        0.615232F, 0.628048F, 0.640696F, 0.653173F, 0.665475F, 0.677598F, 0.689541F, 0.701298F,
        0.712868F, 0.724247F, 0.735432F, 0.746420F, 0.757209F, 0.767795F, 0.778175F, 0.788346F,
        0.798307F, 0.808054F, 0.817585F, 0.826897F, 0.835987F, 0.844854F, 0.853494F, 0.861906F,
        0.870087F, 0.878035F, 0.885748F, 0.893224F, 0.900461F, 0.907457F, 0.914210F, 0.920718F,
        0.926979F, 0.932993F, 0.938756F, 0.944269F, 0.949528F, 0.954533F, 0.959283F, 0.963776F,
        0.968011F, 0.971987F, 0.975702F, 0.979156F, 0.982349F, 0.985278F, 0.987943F, 0.990344F,
        0.992480F, 0.994350F, 0.995953F, 0.997290F, 0.998361F, 0.999163F, 0.999699F, 0.999967F};

const float lpdcom_sineWindow128[128] =
    {
        0.006136F, 0.018407F, 0.030675F, 0.042938F, 0.055195F, 0.067444F, 0.079682F, 0.091909F,
        0.104122F, 0.116319F, 0.128498F, 0.140658F, 0.152797F, 0.164913F, 0.177004F, 0.189069F,
        0.201105F, 0.213110F, 0.225084F, 0.237024F, 0.248928F, 0.260794F, 0.272621F, 0.284408F,
        0.296151F, 0.307850F, 0.319502F, 0.331106F, 0.342661F, 0.354164F, 0.365613F, 0.377007F,
        0.388345F, 0.399624F, 0.410843F, 0.422000F, 0.433094F, 0.444122F, 0.455084F, 0.465977F,
        0.476799F, 0.487550F, 0.498228F, 0.508830F, 0.519356F, 0.529804F, 0.540172F, 0.550458F,
        0.560662F, 0.570781F, 0.580814F, 0.590760F, 0.600617F, 0.610383F, 0.620057F, 0.629638F,
        0.639124F, 0.648514F, 0.657807F, 0.667000F, 0.676093F, 0.685084F, 0.693971F, 0.702755F,
        0.711432F, 0.720003F, 0.728464F, 0.736817F, 0.745058F, 0.753187F, 0.761202F, 0.769103F,
        0.776888F, 0.784557F, 0.792107F, 0.799537F, 0.806848F, 0.814036F, 0.821103F, 0.828045F,
        0.834863F, 0.841555F, 0.848120F, 0.854558F, 0.860867F, 0.867046F, 0.873095F, 0.879012F,
        0.884797F, 0.890449F, 0.895966F, 0.901349F, 0.906596F, 0.911706F, 0.916679F, 0.921514F,
        0.926210F, 0.930767F, 0.935184F, 0.939459F, 0.943593F, 0.947586F, 0.951435F, 0.955141F,
        0.958703F, 0.962121F, 0.965394F, 0.968522F, 0.971504F, 0.974339F, 0.977028F, 0.979570F,
        0.981964F, 0.984210F, 0.986308F, 0.988258F, 0.990058F, 0.991710F, 0.993212F, 0.994565F,
        0.995767F, 0.996820F, 0.997723F, 0.998476F, 0.999078F, 0.999529F, 0.999831F, 0.999981F};

const float lpdcom_sineWindow192[192] =
    {
        0.004091F, 0.012272F, 0.020452F, 0.028630F, 0.036807F, 0.044982F, 0.053153F, 0.061321F,
        0.069484F, 0.077643F, 0.085797F, 0.093945F, 0.102087F, 0.110222F, 0.118350F, 0.126469F,
        0.134581F, 0.142683F, 0.150776F, 0.158858F, 0.166930F, 0.174991F, 0.183040F, 0.191077F,
        0.199101F, 0.207111F, 0.215108F, 0.223091F, 0.231058F, 0.239010F, 0.246946F, 0.254866F,
        0.262768F, 0.270653F, 0.278520F, 0.286368F, 0.294197F, 0.302006F, 0.309795F, 0.317563F,
        0.325310F, 0.333036F, 0.340739F, 0.348419F, 0.356076F, 0.363709F, 0.371317F, 0.378901F,
        0.386459F, 0.393992F, 0.401498F, 0.408978F, 0.416430F, 0.423854F, 0.431249F, 0.438616F,
        0.445954F, 0.453261F, 0.460539F, 0.467785F, 0.475000F, 0.482184F, 0.489335F, 0.496453F,
        0.503538F, 0.510590F, 0.517607F, 0.524590F, 0.531537F, 0.538449F, 0.545325F, 0.552164F,
        0.558967F, 0.565732F, 0.572459F, 0.579148F, 0.585798F, 0.592409F, 0.598980F, 0.605511F,
        0.612002F, 0.618451F, 0.624859F, 0.631226F, 0.637550F, 0.643832F, 0.650070F, 0.656265F,
        0.662416F, 0.668522F, 0.674584F, 0.680601F, 0.686572F, 0.692497F, 0.698376F, 0.704208F,
        0.709993F, 0.715731F, 0.721420F, 0.727062F, 0.732654F, 0.738198F, 0.743692F, 0.749136F,
        0.754531F, 0.759874F, 0.765167F, 0.770409F, 0.775599F, 0.780737F, 0.785823F, 0.790857F,
        0.795837F, 0.800764F, 0.805638F, 0.810457F, 0.815223F, 0.819933F, 0.824589F, 0.829190F,
        0.833735F, 0.838225F, 0.842658F, 0.847035F, 0.851355F, 0.855618F, 0.859824F, 0.863973F,
        0.868063F, 0.872096F, 0.876070F, 0.879986F, 0.883842F, 0.887640F, 0.891378F, 0.895056F,
        0.898674F, 0.902233F, 0.905731F, 0.909168F, 0.912544F, 0.915860F, 0.919114F, 0.922306F,
        0.925437F, 0.928506F, 0.931513F, 0.934457F, 0.937339F, 0.940158F, 0.942914F, 0.945607F,
        0.948237F, 0.950803F, 0.953306F, 0.955745F, 0.958120F, 0.960431F, 0.962677F, 0.964859F,
        0.966976F, 0.969029F, 0.971017F, 0.972940F, 0.974798F, 0.976590F, 0.978317F, 0.979979F,
        0.981575F, 0.983105F, 0.984570F, 0.985969F, 0.987301F, 0.988568F, 0.989768F, 0.990903F,
        0.991970F, 0.992972F, 0.993907F, 0.994775F, 0.995577F, 0.996313F, 0.996981F, 0.997583F,
        0.998118F, 0.998586F, 0.998988F, 0.999322F, 0.999590F, 0.999791F, 0.999925F, 0.999992F};

const float lpdcom_sineWindow256[256] =
    {
        0.00306796F, 0.00920375F, 0.01533921F, 0.02147408F, 0.02760815F, 0.03374117F, 0.03987293F, 0.04600318F,
        0.05213170F, 0.05825826F, 0.06438263F, 0.07050457F, 0.07662386F, 0.08274026F, 0.08885355F, 0.09496350F,
        0.10106986F, 0.10717242F, 0.11327095F, 0.11936521F, 0.12545498F, 0.13154003F, 0.13762012F, 0.14369503F,
        0.14976453F, 0.15582840F, 0.16188639F, 0.16793829F, 0.17398387F, 0.18002290F, 0.18605515F, 0.19208040F,
        0.19809841F, 0.20410897F, 0.21011184F, 0.21610680F, 0.22209362F, 0.22807208F, 0.23404196F, 0.24000302F,
        0.24595505F, 0.25189782F, 0.25783110F, 0.26375468F, 0.26966833F, 0.27557182F, 0.28146494F, 0.28734746F,
        0.29321916F, 0.29907983F, 0.30492923F, 0.31076715F, 0.31659338F, 0.32240768F, 0.32820984F, 0.33399965F,
        0.33977688F, 0.34554132F, 0.35129276F, 0.35703096F, 0.36275572F, 0.36846683F, 0.37416406F, 0.37984721F,
        0.38551605F, 0.39117038F, 0.39680999F, 0.40243465F, 0.40804416F, 0.41363831F, 0.41921689F, 0.42477968F,
        0.43032648F, 0.43585708F, 0.44137127F, 0.44686884F, 0.45234959F, 0.45781330F, 0.46325978F, 0.46868882F,
        0.47410021F, 0.47949376F, 0.48486925F, 0.49022648F, 0.49556526F, 0.50088538F, 0.50618665F, 0.51146885F,
        0.51673180F, 0.52197529F, 0.52719913F, 0.53240313F, 0.53758708F, 0.54275078F, 0.54789406F, 0.55301671F,
        0.55811853F, 0.56319934F, 0.56825895F, 0.57329717F, 0.57831380F, 0.58330865F, 0.58828155F, 0.59323230F,
        0.59816071F, 0.60306660F, 0.60794978F, 0.61281008F, 0.61764731F, 0.62246128F, 0.62725182F, 0.63201874F,
        0.63676186F, 0.64148101F, 0.64617601F, 0.65084668F, 0.65549285F, 0.66011434F, 0.66471098F, 0.66928259F,
        0.67382900F, 0.67835004F, 0.68284555F, 0.68731534F, 0.69175926F, 0.69617713F, 0.70056879F, 0.70493408F,
        0.70927283F, 0.71358487F, 0.71787005F, 0.72212819F, 0.72635916F, 0.73056277F, 0.73473888F, 0.73888732F,
        0.74300795F, 0.74710061F, 0.75116513F, 0.75520138F, 0.75920919F, 0.76318842F, 0.76713891F, 0.77106052F,
        0.77495311F, 0.77881651F, 0.78265060F, 0.78645521F, 0.79023022F, 0.79397548F, 0.79769084F, 0.80137617F,
        0.80503133F, 0.80865618F, 0.81225059F, 0.81581441F, 0.81934752F, 0.82284978F, 0.82632106F, 0.82976123F,
        0.83317016F, 0.83654773F, 0.83989379F, 0.84320824F, 0.84649094F, 0.84974177F, 0.85296060F, 0.85614733F,
        0.85930182F, 0.86242396F, 0.86551362F, 0.86857071F, 0.87159509F, 0.87458665F, 0.87754529F, 0.88047089F,
        0.88336334F, 0.88622253F, 0.88904836F, 0.89184071F, 0.89459949F, 0.89732458F, 0.90001589F, 0.90267332F,
        0.90529676F, 0.90788612F, 0.91044129F, 0.91296219F, 0.91544872F, 0.91790078F, 0.92031828F, 0.92270113F,
        0.92504924F, 0.92736253F, 0.92964090F, 0.93188427F, 0.93409255F, 0.93626567F, 0.93840353F, 0.94050607F,
        0.94257320F, 0.94460484F, 0.94660091F, 0.94856135F, 0.95048607F, 0.95237501F, 0.95422810F, 0.95604525F,
        0.95782641F, 0.95957151F, 0.96128049F, 0.96295327F, 0.96458979F, 0.96619000F, 0.96775384F, 0.96928124F,
        0.97077214F, 0.97222650F, 0.97364425F, 0.97502535F, 0.97636973F, 0.97767736F, 0.97894818F, 0.98018214F,
        0.98137919F, 0.98253930F, 0.98366242F, 0.98474850F, 0.98579751F, 0.98680940F, 0.98778414F, 0.98872169F,
        0.98962202F, 0.99048508F, 0.99131086F, 0.99209931F, 0.99285041F, 0.99356414F, 0.99424045F, 0.99487933F,
        0.99548076F, 0.99604470F, 0.99657115F, 0.99706007F, 0.99751146F, 0.99792529F, 0.99830154F, 0.99864022F,
        0.99894129F, 0.99920476F, 0.99943060F, 0.99961882F, 0.99976941F, 0.99988235F, 0.99995764F, 0.99999529F};

int LPDCom_tcx_OpenDCT4(HANDLE_TCX_DCT4 *phDct4, int nLines) {
  TCX_DCT_ERROR error = TCX_DCT_NO_ERROR;

  if (error == TCX_DCT_NO_ERROR) {
    if (NULL == (*phDct4 = (HANDLE_TCX_DCT4)iisCalloc(1, sizeof(struct T_TCX_DCT4)))) {
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if (error == TCX_DCT_NO_ERROR) {
    if (nLines % 2 == 0) {
      (*phDct4)->nLines = nLines;
    } else {
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if (error == TCX_DCT_NO_ERROR) {
    if (NULL == ((*phDct4)->pPreTwiddleReal = (float *)iisCalloc((*phDct4)->nLines / 2, sizeof(float)))) {
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if (error == TCX_DCT_NO_ERROR) {
    if (NULL == ((*phDct4)->pPreTwiddleImag = (float *)iisCalloc((*phDct4)->nLines / 2, sizeof(float)))) {
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if (error == TCX_DCT_NO_ERROR) {
    int i;

    for (i = 0; i < (*phDct4)->nLines / 2; i++) {
      (*phDct4)->pPreTwiddleReal[i] = (float)cos((-1.0f * i - 0.25f) * PI / ((float)(*phDct4)->nLines));
      (*phDct4)->pPreTwiddleImag[i] = (float)sin((-1.0f * i - 0.25f) * PI / ((float)(*phDct4)->nLines));
    }
  }

  if (error == TCX_DCT_NO_ERROR) {
    if (NULL == ((*phDct4)->pBufferOdd = (float *)iisCalloc((*phDct4)->nLines / 2, sizeof(float)))) {
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if (error == TCX_DCT_NO_ERROR) {
    if (NULL == ((*phDct4)->pBufferEven = (float *)iisCalloc((*phDct4)->nLines / 2, sizeof(float)))) {
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if (error == TCX_DCT_NO_ERROR) {
    if (NULL == ((*phDct4)->pBufferReal = (float *)iisCalloc((*phDct4)->nLines / 2, sizeof(float)))) {
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if (error == TCX_DCT_NO_ERROR) {
    if (NULL == ((*phDct4)->pBufferImag = (float *)iisCalloc((*phDct4)->nLines / 2, sizeof(float)))) {
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if (error == TCX_DCT_NO_ERROR) {
    if (NULL == ((*phDct4)->pBufferTmp1 = (float *)iisCalloc((*phDct4)->nLines / 2, sizeof(float)))) {
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if (error == TCX_DCT_NO_ERROR) {
    if (NULL == ((*phDct4)->pBufferTmp2 = (float *)iisCalloc((*phDct4)->nLines / 2, sizeof(float)))) {
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if (error == TCX_DCT_NO_ERROR) {
    if (NULL == ((*phDct4)->pPostTwiddleReal = (float *)iisCalloc((*phDct4)->nLines / 2, sizeof(float)))) {
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if (error == TCX_DCT_NO_ERROR) {
    if (NULL == ((*phDct4)->pPostTwiddleImag = (float *)iisCalloc((*phDct4)->nLines / 2, sizeof(float)))) {
      error = TCX_DCT_FATAL_ERROR;
    }
  }

  if (error == TCX_DCT_NO_ERROR) {
    int i;

    for (i = 0; i < (*phDct4)->nLines / 2; i++) {
      (*phDct4)->pPostTwiddleReal[i] = (float)cos((-1.0f * i) * PI / ((float)(*phDct4)->nLines));
      (*phDct4)->pPostTwiddleImag[i] = (float)sin((-1.0f * i) * PI / ((float)(*phDct4)->nLines));
    }
  }

  if (error == TCX_DCT_NO_ERROR) {
    IIS_FFT_ERROR err;
    err = IIS_CFFT_Create(&((*phDct4)->hIisFft), (*phDct4)->nLines / 2, -1);
    if (err != IIS_FFT_NO_ERROR)
      error = TCX_DCT_FATAL_ERROR;
  }

  return error;
}

int LPDCom_tcx_CloseDCT4(HANDLE_TCX_DCT4 *phDct4) {
  if (phDct4) {
    if (*phDct4) {
      if ((*phDct4)->pPreTwiddleReal) {
        iisFree((*phDct4)->pPreTwiddleReal);
      }
      (*phDct4)->pPreTwiddleReal = NULL;

      if ((*phDct4)->pPreTwiddleImag) {
        iisFree((*phDct4)->pPreTwiddleImag);
      }
      (*phDct4)->pPreTwiddleImag = NULL;

      if ((*phDct4)->pBufferOdd) {
        iisFree((*phDct4)->pBufferOdd);
      }
      (*phDct4)->pBufferOdd = NULL;

      if ((*phDct4)->pBufferEven) {
        iisFree((*phDct4)->pBufferEven);
      }
      (*phDct4)->pBufferEven = NULL;

      if ((*phDct4)->pBufferReal) {
        iisFree((*phDct4)->pBufferReal);
      }
      (*phDct4)->pBufferReal = NULL;

      if ((*phDct4)->pBufferImag) {
        iisFree((*phDct4)->pBufferImag);
      }
      (*phDct4)->pBufferImag = NULL;

      if ((*phDct4)->pBufferTmp1) {
        iisFree((*phDct4)->pBufferTmp1);
      }
      (*phDct4)->pBufferTmp1 = NULL;

      if ((*phDct4)->pBufferTmp2) {
        iisFree((*phDct4)->pBufferTmp2);
      }
      (*phDct4)->pBufferTmp2 = NULL;

      if ((*phDct4)->pPostTwiddleReal) {
        iisFree((*phDct4)->pPostTwiddleReal);
      }
      (*phDct4)->pPostTwiddleReal = NULL;

      if ((*phDct4)->pPostTwiddleImag) {
        iisFree((*phDct4)->pPostTwiddleImag);
      }
      (*phDct4)->pPostTwiddleImag = NULL;

      if ((*phDct4)->hIisFft)
        IIS_CFFT_Destroy(&((*phDct4)->hIisFft));

      iisFree(*phDct4);
    }
    *phDct4 = NULL;
  }

  return TCX_DCT_NO_ERROR;
}

int LPDCom_tcx_ApplyDCT4(HANDLE_TCX_DCT4 hDct4, float *pIn, float *pOut) {
  TCX_DCT_ERROR error = TCX_DCT_NO_ERROR;

  if (hDct4) {
    if (error == TCX_DCT_NO_ERROR) {
      if (pIn == NULL) {
        error = TCX_DCT_FATAL_ERROR;
      }
    }

    if (error == TCX_DCT_NO_ERROR) {
      int i;

      for (i = 0; i < hDct4->nLines / 2; i++) {
        hDct4->pBufferEven[i] = pIn[2 * i];
        hDct4->pBufferOdd[i] = pIn[hDct4->nLines - 1 - 2 * i];
      }
    }

    if (error == TCX_DCT_NO_ERROR) {
      multFLOAT(hDct4->pBufferEven, hDct4->pPreTwiddleReal, hDct4->pBufferTmp1, hDct4->nLines / 2);
      multFLOAT(hDct4->pBufferOdd, hDct4->pPreTwiddleImag, hDct4->pBufferTmp2, hDct4->nLines / 2);
      subFLOAT(hDct4->pBufferTmp1, hDct4->pBufferTmp2, hDct4->pBufferReal, hDct4->nLines / 2);

      multFLOAT(hDct4->pBufferEven, hDct4->pPreTwiddleImag, hDct4->pBufferTmp1, hDct4->nLines / 2);
      multFLOAT(hDct4->pBufferOdd, hDct4->pPreTwiddleReal, hDct4->pBufferTmp2, hDct4->nLines / 2);
      addFLOAT(hDct4->pBufferTmp1, hDct4->pBufferTmp2, hDct4->pBufferImag, hDct4->nLines / 2);
    }

    if (error == TCX_DCT_NO_ERROR) {
      IIS_FFT_Apply_CFFT(hDct4->hIisFft, hDct4->pBufferReal, hDct4->pBufferImag, hDct4->pBufferReal, hDct4->pBufferImag);
    }

    if (error == TCX_DCT_NO_ERROR) {
      multFLOAT(hDct4->pBufferReal, hDct4->pPostTwiddleReal, hDct4->pBufferTmp1, hDct4->nLines / 2);
      multFLOAT(hDct4->pBufferImag, hDct4->pPostTwiddleImag, hDct4->pBufferTmp2, hDct4->nLines / 2);
      subFLOAT(hDct4->pBufferTmp1, hDct4->pBufferTmp2, hDct4->pBufferEven, hDct4->nLines / 2);

      multFLOAT(hDct4->pBufferReal, hDct4->pPostTwiddleImag, hDct4->pBufferTmp1, hDct4->nLines / 2);
      multFLOAT(hDct4->pBufferImag, hDct4->pPostTwiddleReal, hDct4->pBufferTmp2, hDct4->nLines / 2);
      addFLOAT(hDct4->pBufferTmp1, hDct4->pBufferTmp2, hDct4->pBufferOdd, hDct4->nLines / 2);
      smulFLOAT(-1.0f, hDct4->pBufferOdd, hDct4->pBufferOdd, hDct4->nLines / 2);
    }

    if (error == TCX_DCT_NO_ERROR) {
      if (pOut == NULL) {
        error = TCX_DCT_FATAL_ERROR;
      }
    }

    if (error == TCX_DCT_NO_ERROR) {
      int i;

      for (i = 0; i < hDct4->nLines / 2; i++) {
        pOut[2 * i] = hDct4->pBufferEven[i];
        pOut[hDct4->nLines - 1 - 2 * i] = hDct4->pBufferOdd[i];
      }
    }
  } else {
    error = TCX_DCT_FATAL_ERROR;
  }

  return error;
}

int LPDCom_tcx_OpenMDCT(HANDLE_TCX_MDCT *phTcxMdct) {
  TCX_MDCT_ERROR error = TCX_MDCT_NO_ERROR;

  if (error == TCX_MDCT_NO_ERROR) {
    if (NULL == ((*phTcxMdct) = iisCalloc(1, sizeof(struct T_TCX_MDCT)))) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (TCX_DCT_NO_ERROR != LPDCom_tcx_OpenDCT4(&((*phTcxMdct)->hDct4_1152), 1152)) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (TCX_DCT_NO_ERROR != LPDCom_tcx_OpenDCT4(&((*phTcxMdct)->hDct4_1088), 1088)) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (TCX_DCT_NO_ERROR != LPDCom_tcx_OpenDCT4(&((*phTcxMdct)->hDct4_1024), 1024)) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (TCX_DCT_NO_ERROR != LPDCom_tcx_OpenDCT4(&((*phTcxMdct)->hDct4_0768), 768)) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (TCX_DCT_NO_ERROR != LPDCom_tcx_OpenDCT4(&((*phTcxMdct)->hDct4_0576), 576)) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (TCX_DCT_NO_ERROR != LPDCom_tcx_OpenDCT4(&((*phTcxMdct)->hDct4_0512), 512)) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (TCX_DCT_NO_ERROR != LPDCom_tcx_OpenDCT4(&((*phTcxMdct)->hDct4_0384), 384)) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (TCX_DCT_NO_ERROR != LPDCom_tcx_OpenDCT4(&((*phTcxMdct)->hDct4_0320), 320)) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (TCX_DCT_NO_ERROR != LPDCom_tcx_OpenDCT4(&((*phTcxMdct)->hDct4_0256), 256)) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (TCX_DCT_NO_ERROR != LPDCom_tcx_OpenDCT4(&((*phTcxMdct)->hDct4_0192), 192)) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (TCX_DCT_NO_ERROR != LPDCom_tcx_OpenDCT4(&((*phTcxMdct)->hDct4_0128), 128)) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (TCX_DCT_NO_ERROR != LPDCom_tcx_OpenDCT4(&((*phTcxMdct)->hDct4_0096), 96)) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (TCX_DCT_NO_ERROR != LPDCom_tcx_OpenDCT4(&((*phTcxMdct)->hDct4_0064), 64)) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (TCX_DCT_NO_ERROR != LPDCom_tcx_OpenDCT4(&((*phTcxMdct)->hDct4_0048), 48)) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  return error;
}

int LPDCom_tcx_CloseMDCT(HANDLE_TCX_MDCT *phTcxMdct) {
  if (phTcxMdct) {
    if (*phTcxMdct) {
      if ((*phTcxMdct)->hDct4_1152) {
        LPDCom_tcx_CloseDCT4(&(*phTcxMdct)->hDct4_1152);
      }
      (*phTcxMdct)->hDct4_1152 = NULL;

      if ((*phTcxMdct)->hDct4_1088) {
        LPDCom_tcx_CloseDCT4(&(*phTcxMdct)->hDct4_1088);
      }
      (*phTcxMdct)->hDct4_1088 = NULL;

      if ((*phTcxMdct)->hDct4_1024) {
        LPDCom_tcx_CloseDCT4(&(*phTcxMdct)->hDct4_1024);
      }
      (*phTcxMdct)->hDct4_1024 = NULL;
      if ((*phTcxMdct)->hDct4_0768) {
        LPDCom_tcx_CloseDCT4(&(*phTcxMdct)->hDct4_0768);
      }
      (*phTcxMdct)->hDct4_0768 = NULL;

      if ((*phTcxMdct)->hDct4_0576) {
        LPDCom_tcx_CloseDCT4(&(*phTcxMdct)->hDct4_0576);
      }
      (*phTcxMdct)->hDct4_0576 = NULL;

      if ((*phTcxMdct)->hDct4_0512) {
        LPDCom_tcx_CloseDCT4(&(*phTcxMdct)->hDct4_0512);
      }
      (*phTcxMdct)->hDct4_0512 = NULL;
      if ((*phTcxMdct)->hDct4_0384) {
        LPDCom_tcx_CloseDCT4(&(*phTcxMdct)->hDct4_0384);
      }
      (*phTcxMdct)->hDct4_0384 = NULL;
      if ((*phTcxMdct)->hDct4_0320) {
        LPDCom_tcx_CloseDCT4(&(*phTcxMdct)->hDct4_0320);
      }
      (*phTcxMdct)->hDct4_0320 = NULL;

      if ((*phTcxMdct)->hDct4_0256) {
        LPDCom_tcx_CloseDCT4(&(*phTcxMdct)->hDct4_0256);
      }
      (*phTcxMdct)->hDct4_0256 = NULL;
      if ((*phTcxMdct)->hDct4_0192) {
        LPDCom_tcx_CloseDCT4(&(*phTcxMdct)->hDct4_0192);
      }
      (*phTcxMdct)->hDct4_0192 = NULL;
      if ((*phTcxMdct)->hDct4_0128) {
        LPDCom_tcx_CloseDCT4(&(*phTcxMdct)->hDct4_0128);
      }
      (*phTcxMdct)->hDct4_0128 = NULL;
      if ((*phTcxMdct)->hDct4_0096) {
        LPDCom_tcx_CloseDCT4(&(*phTcxMdct)->hDct4_0096);
      }
      (*phTcxMdct)->hDct4_0096 = NULL;
      if ((*phTcxMdct)->hDct4_0064) {
        LPDCom_tcx_CloseDCT4(&(*phTcxMdct)->hDct4_0064);
      }
      (*phTcxMdct)->hDct4_0064 = NULL;
      if ((*phTcxMdct)->hDct4_0048) {
        LPDCom_tcx_CloseDCT4(&(*phTcxMdct)->hDct4_0048);
      }
      (*phTcxMdct)->hDct4_0048 = NULL;

      iisFree(*phTcxMdct);
    }

    *phTcxMdct = NULL;
  }

  return TCX_MDCT_NO_ERROR;
}

int LPDCom_tcx_ApplyMDCT(HANDLE_TCX_MDCT hTcxMdct, float *x, float *y, int l, int m, int r) {
  TCX_MDCT_ERROR error = TCX_MDCT_NO_ERROR;
  int dctLength = l / 2 + m + r / 2;
  float dctInBuffer[1152] = {0.f};
  HANDLE_TCX_DCT4 hDct = NULL;

  if (error == TCX_MDCT_NO_ERROR) {
    if (hTcxMdct == NULL) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (NULL == (hDct = LPDCom_tcx_GetDCT4Handle(hTcxMdct, dctLength))) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (x == NULL) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    int i;

    for (i = 0; i < m / 2; i++) {
      dctInBuffer[m / 2 + r / 2 + i] = -1.0f * x[l + m / 2 - 1 - i];
    }
    for (i = 0; i < l / 2; i++) {
      dctInBuffer[m / 2 + r / 2 + m / 2 + i] = x[i] - x[l - 1 - i];
    }
    for (i = 0; i < m / 2; i++) {
      dctInBuffer[m / 2 + r / 2 - 1 - i] = -1.0f * x[l + m / 2 + i];
    }
    for (i = 0; i < r / 2; i++) {
      dctInBuffer[m / 2 + r / 2 - 1 - m / 2 - i] = -1.0f * x[l + m + i] - x[l + m + r - 1 - i];
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (TCX_DCT_NO_ERROR != LPDCom_tcx_ApplyDCT4(hDct, dctInBuffer, y)) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  return error;
}

int LPDCom_tcx_ApplyInvMDCT(HANDLE_TCX_MDCT hTcxMdct, float *x, float *y, int l, int m, int r) {
  TCX_MDCT_ERROR error = TCX_MDCT_NO_ERROR;
  int dctLength = l / 2 + m + r / 2;
  float dctOutBuffer[1152] = {0.f};
  HANDLE_TCX_DCT4 hDct = NULL;

  if (error == TCX_MDCT_NO_ERROR) {
    if (hTcxMdct == NULL) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (NULL == (hDct = LPDCom_tcx_GetDCT4Handle(hTcxMdct, dctLength))) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (TCX_DCT_NO_ERROR != LPDCom_tcx_ApplyDCT4(hDct, x, dctOutBuffer)) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    if (y == NULL) {
      error = TCX_MDCT_FATAL_ERROR;
    }
  }

  if (error == TCX_MDCT_NO_ERROR) {
    int i;

    for (i = 0; i < m / 2; i++) {
      y[l + m / 2 - 1 - i] = -1.0f * dctOutBuffer[m / 2 + r / 2 + i];
    }
    for (i = 0; i < l / 2; i++) {
      y[i] = dctOutBuffer[m / 2 + r / 2 + m / 2 + i];
      y[l - 1 - i] = -1.0f * dctOutBuffer[m / 2 + r / 2 + m / 2 + i];
    }
    for (i = 0; i < m / 2; i++) {
      y[l + m / 2 + i] = -1.0f * dctOutBuffer[m / 2 + r / 2 - 1 - i];
    }
    for (i = 0; i < r / 2; i++) {
      y[l + m + i] = -1.0f * dctOutBuffer[m / 2 + r / 2 - 1 - m / 2 - i];
      y[l + m + r - 1 - i] = -1.0f * dctOutBuffer[m / 2 + r / 2 - 1 - m / 2 - i];
    }
  }

  return error;
}

HANDLE_TCX_DCT4 LPDCom_tcx_GetDCT4Handle(HANDLE_TCX_MDCT hTcxMdct, int dctLength) {
  HANDLE_TCX_DCT4 hDct = NULL;
  if (hTcxMdct) {
    switch (dctLength) {
      case 1152:
        hDct = hTcxMdct->hDct4_1152;
        break;
      case 1088:
        hDct = hTcxMdct->hDct4_1088;
        break;
      case 1024:
        hDct = hTcxMdct->hDct4_1024;
        break;
      case 768:
        hDct = hTcxMdct->hDct4_0768;
        break;
      case 576:
        hDct = hTcxMdct->hDct4_0576;
        break;
      case 512:
        hDct = hTcxMdct->hDct4_0512;
        break;
      case 384:
        hDct = hTcxMdct->hDct4_0384;
        break;
      case 320:
        hDct = hTcxMdct->hDct4_0320;
        break;
      case 256:
        hDct = hTcxMdct->hDct4_0256;
        break;
      case 192:
        hDct = hTcxMdct->hDct4_0192;
        break;
      case 128:
        hDct = hTcxMdct->hDct4_0128;
        break;
      case 96:
        hDct = hTcxMdct->hDct4_0096;
        break;
      case 64:
        hDct = hTcxMdct->hDct4_0064;
        break;
      case 48:
        hDct = hTcxMdct->hDct4_0048;
        break;
      default:
        assert(0);
        break;
    }
  }

  return hDct;
}
