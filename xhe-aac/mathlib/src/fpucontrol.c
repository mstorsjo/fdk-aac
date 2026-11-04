
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

#include <stdint.h>

#include "cpuinfo.h"
#include "fpucontrol.h"
#include "iisutillib.h"

#if (defined _M_IX86 || defined _M_X64 || defined __i386__ || defined __x86_64__)
#include "opt/fpucontrol_opt.h"
#endif

struct IIS_FPU_CONTROL {
#if defined(__arm64__) || defined(__aarch64__) || defined(__ARM_ARCH_7A__)
  uintptr_t fpuCntrl;
  uintptr_t vecCntrl;
#else
  unsigned int fpuCntrl;
  unsigned int vecCntrl;
#endif
};

#if (defined _MSC_VER && (_MSC_VER < 1400)) || (defined __x86_64__ || defined __i386__)

#if (defined _MSC_VER && !(defined _M_IX86 || defined _M_X64))
unsigned int fpuctrl_getSSE_mxcsr(void) {
  return 0;
}

unsigned int fpuctrl_setSSE_mxcsr(unsigned int ctrlWord) {
  return 0;
}
#endif

static int setDenormalModeVec(HANDLE_IIS_FPU_CONTROL hFpu) {
  unsigned int newCntrlWord = 0;

  if (hFpu) {
    if (GetCPUInfo(HAS_CPU_SSE)) {
      hFpu->vecCntrl = fpuctrl_getSSE_mxcsr();
      newCntrlWord = hFpu->vecCntrl | 0x8040U;
      if (newCntrlWord != hFpu->vecCntrl) {
        fpuctrl_setSSE_mxcsr(newCntrlWord);
      }
    }
  }

  return 0;
}

static int restoreDenormalModeVec(HANDLE_IIS_FPU_CONTROL hFpu) {
  int err = 0;
  unsigned int newCntrlWord = 0;

  if (hFpu) {
    if (GetCPUInfo(HAS_CPU_SSE)) {
      unsigned int state = fpuctrl_getSSE_mxcsr();
      newCntrlWord = state & (hFpu->vecCntrl | ~0x8040U);
      if (newCntrlWord != state) {
        fpuctrl_setSSE_mxcsr(newCntrlWord);
      }
    }
  }

  return err;
}
#else
static int setDenormalModeVec(HANDLE_IIS_FPU_CONTROL hFpu) {
  (void)hFpu;
  return 0;
}

static int restoreDenormalModeVec(HANDLE_IIS_FPU_CONTROL hFpu) {
  (void)hFpu;
  int err = 0;

  return err;
}

#endif

#ifdef _WIN32
#include <float.h>
static int setDenormalModeFPU(HANDLE_IIS_FPU_CONTROL hFpu) {
  unsigned int x86;

  if (hFpu) {
    hFpu->fpuCntrl = _controlfp(0, 0);
    x86 = _controlfp(_DN_FLUSH, _MCW_DN);
  }

  return 0;
}

static int restoreDenormalModeFPU(HANDLE_IIS_FPU_CONTROL hFpu) {
  unsigned int x86;

  if (hFpu) {
    x86 = _controlfp(hFpu->fpuCntrl, _MCW_DN);
  }

  return 0;
}
#elif defined(__arm64__) || defined(__aarch64__) || defined(__ARM_ARCH_7A__)
#if defined __ARM_FP
static void readFPCR(uintptr_t *fpuCntrl) {
  uintptr_t fpcr = 0;
#ifdef __arm__

  __asm volatile("vmrs %0, fpscr"
                 : "=r"(fpcr));
#else

  __asm volatile("mrs %0, fpcr"
                 : "=r"(fpcr));
#endif
  *fpuCntrl = fpcr;
}

static void writeFPCR(uintptr_t newCntrlWord) {
#ifdef __arm__

  __asm volatile("vmsr fpscr, %0"
                 :
                 : "r"(newCntrlWord));
#else

  __asm volatile("msr fpcr, %0"
                 :
                 : "r"(newCntrlWord));
#endif
}
#else
#pragma message("no support for reading writing FPU Status/Control Register")
static void readFPCR(uintptr_t* fpuCntrl) {
  (void)fpuCntrl;
}
static void writeFPCR(uintptr_t newCntrlWord) {
  (void)newCntrlWord;
}

#endif

static const uintptr_t fzBit = (1UL << 24);

static int setDenormalModeFPU(HANDLE_IIS_FPU_CONTROL hFpu) {
  int err = 0;
  uintptr_t newCntrlWord = 0;
  if (hFpu) {
    readFPCR(&hFpu->fpuCntrl);
    newCntrlWord = hFpu->fpuCntrl | fzBit;
    if (newCntrlWord != hFpu->fpuCntrl) {
      writeFPCR(newCntrlWord);
    }
  } else {
    err = 1;
  }

  return err;
}

static int restoreDenormalModeFPU(HANDLE_IIS_FPU_CONTROL hFpu) {
  int err = 0;
  uintptr_t newCntrlWord = 0;
  if (hFpu) {
    uintptr_t state = 0;
    readFPCR(&state);
    newCntrlWord = state & (hFpu->fpuCntrl | ~fzBit);
    if (newCntrlWord != state) {
      writeFPCR(newCntrlWord);
    }
  } else {
    err = 1;
  }
  return err;
}

#else

static int setDenormalModeFPU(HANDLE_IIS_FPU_CONTROL hFpu) {
  (void)hFpu;
  return 0;
}

static int restoreDenormalModeFPU(HANDLE_IIS_FPU_CONTROL hFpu) {
  (void)hFpu;
  return 0;
}
#endif

int IIS_FPUControl_Create(HANDLE_IIS_FPU_CONTROL *phFpu) {
  int err = 0;
  (*phFpu) = (HANDLE_IIS_FPU_CONTROL)iisCalloc(1, sizeof(struct IIS_FPU_CONTROL));
  if (*phFpu == NULL) {
    err = -1;
  }
  return err;
}

int IIS_FPUControl_Delete(HANDLE_IIS_FPU_CONTROL hFpu) {
  int err = 0;
  if (hFpu)
    iisFree(hFpu);

  return err;
}

int IIS_FPUcontrol_SetDenormal_FTZ_AZ(HANDLE_IIS_FPU_CONTROL hFpu) {
  int err = 0;
  setDenormalModeVec(hFpu);
  setDenormalModeFPU(hFpu);

  return err;
}

int IIS_FPUcontrol_Restore(HANDLE_IIS_FPU_CONTROL hFpu) {
  int err = 0;
  restoreDenormalModeVec(hFpu);
  restoreDenormalModeFPU(hFpu);

  return err;
}
