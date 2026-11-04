
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

#ifndef ngsalloc_h
#define ngsalloc_h

#include <stdlib.h>

#if defined __ICL
#define ALIGN_16_BYTE __declspec(align(16))

#elif defined _MSC_VER
#define ALIGN_16_BYTE __declspec(align(16))

#elif defined __GNUC__ && !defined __sparc__ && !defined __sparc_v9__
#define ALIGN_16_BYTE __attribute__((aligned(16)))

#else
#define ALIGN_16_BYTE
#endif

#define MEM_ALIGNMENT_SIZE 16

#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 201112L
#include <assert.h>
static_assert(MEM_ALIGNMENT_SIZE % sizeof(double) == 0, "MEM_ALIGNMENT_SIZE must be a multiple of sizeof(double)");
static_assert(MEM_ALIGNMENT_SIZE % sizeof(void **) == 0, "MEM_ALIGNMENT_SIZE must be a multiple of sizeof(void**)");
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern void ngsInitAllocationCheck(void);
extern void iisInitAllocationCheck(int n);
extern void iisAllocationCheck(void);
#define ngsAllocationCheck iisAllocationCheck

#ifdef __cplusplus
}
#endif

#define ngsMalloc(size) iisMalloc_mem(size, 0, 0)
#define ngsRealloc(ptr, size) iisRealloc_mem(ptr, size, 0, 0)
#define ngsCalloc(num, size) iisCalloc_mem(num, size, 0, 0)
#define ngsFree(p) iisFree_mem(p, 0, 0)

#define iisMalloc(size) iisMalloc_mem(size, 0, 0)
#define iisRealloc(ptr, size) iisRealloc_mem(ptr, size, 0, 0)
#define iisCalloc(num, size) iisCalloc_mem(num, size, 0, 0)
#define iisFree(p) iisFree_mem(p, 0, 0)

#ifdef __cplusplus
extern "C" {
#endif
extern void *iisMalloc_mem(size_t s, const char *file, int line);
extern void *iisRealloc_mem(void *p, size_t s, const char *file, int line);
extern void *iisCalloc_mem(size_t s, size_t t, const char *file, int line);
extern void iisFree_mem(void *p, const char *file, int line);
#ifdef __cplusplus
}
#endif

#endif
