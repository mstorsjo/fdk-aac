
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

#include "IISutillib/ngsalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if (!defined _MSC_VER) || (defined _MSC_VER && _MSC_VER > 1500)

#include <stdint.h>
#else

#include <vadefs.h>
#endif

typedef struct meminfo {
  void *pRawMemBlock;
  size_t userMemorySize;
} MEMINFO;

#define GET_MEMINFO_PTR(pUserMemory) (&(((MEMINFO *)pUserMemory)[-1]))

void *iisCalloc_mem(size_t s, size_t t, const char *file, int line) {
  void *p;

  p = iisMalloc_mem(s * t, file, line);
  if (p) memset(p, 0, s * t);
  return p;
}

void *iisMalloc_mem(size_t s, const char *file, int line) {
  void *pUserMemory = NULL;

  if (s != 0) {
    size_t userMemorySize = s;
    void *pRawMemBlock = NULL;
    s += sizeof(MEMINFO) + MEM_ALIGNMENT_SIZE;
    pRawMemBlock = malloc(s);

    if (pRawMemBlock) {
      MEMINFO *pMemInfo = NULL;

      size_t offsetFromMemalignedGrid = ((uintptr_t)((unsigned char *)pRawMemBlock + sizeof(MEMINFO))) % MEM_ALIGNMENT_SIZE;
      size_t meminfoOffset = 0;
      if (offsetFromMemalignedGrid > 0) {
        meminfoOffset = MEM_ALIGNMENT_SIZE - offsetFromMemalignedGrid;
      }

      pUserMemory = (void *)((unsigned char *)pRawMemBlock + meminfoOffset + sizeof(MEMINFO));
      pMemInfo = GET_MEMINFO_PTR(pUserMemory);

      pMemInfo->pRawMemBlock = pRawMemBlock;
      pMemInfo->userMemorySize = userMemorySize;
    }
  }

  (void)file;
  (void)line;

  return pUserMemory;
}

void *iisRealloc_mem(void *p, size_t s, const char *file, int line) {
  void *pNewUserMemory = NULL;
  if (s != 0) {
    if (p != NULL) {
      MEMINFO *pMemInfo = GET_MEMINFO_PTR(p);
      size_t old_userMemorySize = pMemInfo->userMemorySize;
      pNewUserMemory = iisMalloc_mem(s, file, line);
      if (pNewUserMemory) {
        memcpy(pNewUserMemory, p, s < old_userMemorySize ? s : old_userMemorySize);
      }
      iisFree_mem(p, file, line);
    } else {
      pNewUserMemory = iisMalloc_mem(s, file, line);
    }
  } else {
    iisFree_mem(p, file, line);
  }

  return pNewUserMemory;
}

void iisFree_mem(void *p, const char *file, int line) {
  (void)file;
  (void)line;

  if (p != NULL) {
    MEMINFO *pMemInfo = GET_MEMINFO_PTR(p);
    p = pMemInfo->pRawMemBlock;
  }
  free(p);
}

void ngsInitAllocationCheck(void) {
}

void iisInitAllocationCheck(int n) {
  (void)n;
}

void iisAllocationCheck(void) {
}
