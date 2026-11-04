
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

#ifndef IISXHEAACENCLIB_SYNCFRAME_H
#define IISXHEAACENCLIB_SYNCFRAME_H

#include "iisxHEAACEncLib_common.h"
#include "iisxHEAACEncLib_sbr_ifc.h"
#include "iisAudioPrerollLib.h"
#include "iisutillib.h"
#include "iisxHEAACEncLib_returnCodes.h"
#include "IIS_loaswrite_interface.h"
#include "iisxHEAACEncLibConfig.h"

#define XHEAACENCLIB_MAX_SYNCFRAME_INFO 2

typedef enum {
  XHEAACENCLIB_SYNCFRAME_TYPE_USAC_INDEP = 0,
  XHEAACENCLIB_SYNCFRAME_TYPE_SBR_HEADER,
  XHEAACENCLIB_SYNCFRAME_TYPE_SBR_SET_FIX_BORDER,
  XHEAACENCLIB_SYNCFRAME_TYPE_CORE_LOW_OVERLAP,
  XHEAACENCLIB_SYNCFRAME_TYPE_CORE_HIGH_BW,
  XHEAACENCLIB_SYNCFRAME_TYPE_LOAS_SMC,
  XHEAACENCLIB_SYNCFRAME_TYPE_IPF,
  XHEAACENCLIB_SYNCFRAME_TYPE_IS_RAP,

  XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS

} XHEAACENCLIB_SYNCFRAME_TYPE,
    *XHEAACENCLIB_SYNCFRAME_TYPE_HANDLE;

typedef struct xheaacenclib_syncframe_struct *XHEAACENCLIB_SYNCFRAME_HANDLE;

typedef struct xHEAACenclib_rap_sync_frame_type {
  int syncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS];
  int diffToSyncFrame;
} XHEAACENCLIB_RAP_SYNC_FRAME_TYPE;

typedef struct xHEAACenclib_rap_sync_info {
  XHEAACENCLIB_RAP_SYNC_FRAME_TYPE frame_type[XHEAACENCLIB_MAX_SYNCFRAME_INFO];
  int nSyncFrameType;
} XHEAACENCLIB_RAP_SYNC_INFO, *XHEAACENCLIB_RAP_SYNC_INFO_HANDLE;

XHEAACENCLIB_RETURN iisxHEAACEncLib_syncFrame_New(XHEAACENCLIB_SYNCFRAME_HANDLE *const phSyncFrame);

XHEAACENCLIB_RETURN iisxHEAACEncLib_syncFrame_Delete(XHEAACENCLIB_SYNCFRAME_HANDLE *const hSyncFrame);

XHEAACENCLIB_RETURN iisxHEAACEncLib_syncFrame_startNextFrame(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame);

XHEAACENCLIB_RETURN iisxHEAACEncLib_syncFrame_ForceSyncFrame(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, int const offset, int const bSyncType[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS]);

XHEAACENCLIB_RETURN iisxHEAACEncLib_syncFrame_GetDelay(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, int *const getDelay, int const bSyncType[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS]);

XHEAACENCLIB_RETURN iisxHEAACEncLib_syncFrame_GetNextSyncFrame(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, int *const nextSyncFrame, XHEAACENCLIB_SYNCFRAME_TYPE const syncType);

XHEAACENCLIB_RETURN iisxHEAACEncLib_syncFrame_IsSyncFrame(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, int const offset, int *const isSyncFrame, XHEAACENCLIB_SYNCFRAME_TYPE const syncType);

XHEAACENCLIB_RETURN iisxHEAACEncLib_syncframe_setup(
    XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame,
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_HANDLE_SBRENCODER const hSbrEnc,
    AUDIOPREROLLLIB_INSTANCE_HANDLE const hAudioPreRoll,
    MPEG4_DELAY const *const mpeg4DelayParameter,
    HANDLE_STREAM_FORMAT const hLoaswriter,
    int const nTrashAUs,
    int *const rapFrameInAdvance,
    int const usacIndepDelay);
#endif
