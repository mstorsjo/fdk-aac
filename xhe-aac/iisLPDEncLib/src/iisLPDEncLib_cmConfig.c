
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

#include "iisLPDEncLib_cmConfig.h"
#include "iisLPDEncLib.h"
#include "mathlib.h"

static void StartEmergencyMode(
    Coder_State_Plus *st);

void LPDEnc_cmConfig_RestrictAcmAndTcxLevel(
    Coder_State_Plus *st,
    int bitRate) {
  int core_mode;

  core_mode = st->acelp_core_mode_nominal;

  if (!(st->isVbr)) {
    if (st->bitResFillLevel < 0.1f) {
      core_mode = max(core_mode - 1, MIN_ACELP_COREMODE);
    }

    st->lpd_channel_stream.acelp_core_mode = core_mode;

    if (st->bitResFillLevel < 0.1f) {
      if (st->bitResFillLevel < 0.03f) {
        st->restrictedMode = 0xfe;
        st->calcMode.emergencyRestrictedMode |= 0x10;
      }
    } else {
      st->calcMode.emergencyRestrictedMode &= 0x00EF;
      st->restrictedMode = 0xff;
    }
  } else {
    st->lpd_channel_stream.acelp_core_mode = core_mode;
    st->calcMode.emergencyRestrictedMode &= 0x00EF;
    st->restrictedMode = 0xff;
  }

  if (!(st->isVbr)) {
    if (bitRate <= 8000) {
      if (bitRate < 6000) {
        if (st->bitResFillLevel < 0.25f) {
          st->TCXLevel = 0.8f;
          st->restrictedMode &= 0xfe;
        } else {
          st->TCXLevel = 1.0f;
          st->restrictedMode |= 1;
        }
      } else if (bitRate < 8000) {
        if (st->restrictedMode == 1) {
          st->TCXLevel = 0.6f + 0.3f * st->bitResFillLevel;
        } else {
          st->TCXLevel = 0.8f + 0.6f * st->bitResFillLevel;
        }
      } else {
        st->TCXLevel = 0.6f + 0.3f * st->bitResFillLevel;
      }
    } else {
      st->TCXLevel = 0.8f + 0.4f * st->bitResFillLevel;
    }
  } else {
    if (bitRate <= 40000) {
      st->TCXLevel = 1.16f;
    } else {
      st->TCXLevel = 1.08f;
    }
  }

  if ((st->calcMode.emergencyRestrictedMode & 0x10) == 0x10) {
    st->TCXLevel = 0.6f;
  }

  if ((st->calcMode.emergencyRestrictedMode & 0x1) == 0x1 && st->calcMode.numberMoreBitsUsedThanBitRes > 0) {
    StartEmergencyMode(st);
  }
}

void LPDEnc_cmConfig_AdaptAcm(
    Coder_State_Plus *st,
    int bitRate,
    float *Tnc) {
  float TncEnergy = 1.0f;
  int i;
  int core_mode = st->lpd_channel_stream.acelp_core_mode;
  for (i = 0; i < (2 * st->nbDiv); i++) TncEnergy *= Tnc[i];

  if (TncEnergy > (st->bitResFillLevel + 0.25f)) {
    core_mode--;
    if (core_mode < MIN_ACELP_COREMODE) {
      core_mode = MIN_ACELP_COREMODE;
    }
  }
  if (TncEnergy < (st->bitResFillLevel - 0.25f)) {
    if (bitRate < 6000) {
    } else {
      core_mode++;
    }
    if (core_mode > MAX_ACELP_COREMODE) {
      core_mode = MAX_ACELP_COREMODE;
    }
  }
  st->lpd_channel_stream.acelp_core_mode = core_mode;
}

static void StartEmergencyMode(
    Coder_State_Plus *st) {
  int core_mode = st->lpd_channel_stream.acelp_core_mode;

  if (st->calcMode.acelpCoreMode >= MIN_ACELP_COREMODE) {
    st->calcMode.acelpCoreMode = min(st->calcMode.acelpCoreMode, core_mode) - 1;
  }

  core_mode = st->calcMode.acelpCoreMode;
  if (core_mode < MIN_ACELP_COREMODE) {
    core_mode = MIN_ACELP_COREMODE;
    if ((st->calcMode.emergencyRestrictedMode & 0x8) == 0x8 || (st->calcMode.emergencyRestrictedMode & 0x6) == 0x0)

      st->calcMode.EmergencyForceTCXLevel *= 0.9f;

    st->calcMode.emergencyRestrictedMode |= 0x8;
    st->restrictedMode &= 0xFE;
    if ((st->calcMode.emergencyRestrictedMode & 0x2) == 0x2)
      st->restrictedMode &= 0xFD;
    if ((st->calcMode.emergencyRestrictedMode & 0x4) == 0x4)
      st->restrictedMode &= 0xFB;

    if ((st->restrictedMode & 0xF) == 0x0) {
      st->restrictedMode = 0xFE;
    }
    st->TCXLevel = st->calcMode.EmergencyForceTCXLevel;
  }
  st->lpd_channel_stream.acelp_core_mode = core_mode;
}

void LPDEnc_cmConfig_SetEmergencyMode(
    ENC_MODE_CALC_LPD_DATA_HANDLE calc_mode_data,
    int numberMoreBitsUsedThanBitRes,
    int maxBitsToUse,
    int const lpc_bits_tcx20,
    int const lpc_bits_tcx40,
    int const disableFac) {
  calc_mode_data->emergencyRestrictedMode |= 0x1;
  calc_mode_data->numberMoreBitsUsedThanBitRes = numberMoreBitsUsedThanBitRes;

  if (maxBitsToUse < 11 + 66 + lpc_bits_tcx20) {
    calc_mode_data->emergencyRestrictedMode |= 0x2;
  }

  if (maxBitsToUse < 11 + 32 + lpc_bits_tcx40) {
    calc_mode_data->emergencyRestrictedMode |= 0x4;
  }

  if (disableFac) {
    calc_mode_data->emergencyRestrictedMode |= 0x20;
  }
}

void LPDEnc_cmConfig_Reset(
    ENC_MODE_CALC_LPD_DATA_HANDLE calc_mode_data) {
  calc_mode_data->emergencyRestrictedMode &= 0x0000;
  calc_mode_data->EmergencyForceTCXLevel = 0.8f / 0.9f;
  calc_mode_data->acelpCoreMode = MAX_ACELP_COREMODE;
  calc_mode_data->numberMoreBitsUsedThanBitRes = 0;
}

int LPDEnc_cmConfig_RestrictSubFrameAcm(
    const ENC_MODE_CALC_LPD_DATA_HANDLE calc_mode_data,
    const int subFrame,
    const int *mod,
    const int restrictedMode,
    const int IPFrestrictedMode) {
  int restrictedMode_ret = restrictedMode;

  if (calc_mode_data->acelpCoreMode < MIN_ACELP_COREMODE && (calc_mode_data->emergencyRestrictedMode & 0x1)) {
    restrictedMode_ret &= 0xE;
    if ((restrictedMode_ret & 0xF) == 0x0) {
      restrictedMode_ret = 0xE;
    }
  }

  if ((subFrame == 3 || subFrame == 1) && mod[subFrame - 1] < 0) {
    restrictedMode_ret &= 0xC;
    if (restrictedMode_ret == 0) {
      restrictedMode_ret = 0xC;
    }
  }

  if ((subFrame == 3 || subFrame == 2) && (mod[0] < 0 || mod[1] < 0)) {
    restrictedMode_ret = 0x8;
  }

  if (0 != IPFrestrictedMode) {
    restrictedMode_ret = 0x2;
  }

  return restrictedMode_ret;
}
