
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

#ifndef IISXHEAACENCLIB_RETURNCODES_H
#define IISXHEAACENCLIB_RETURNCODES_H

#define XHEAACENCLIB_WARNING_FIRST (100)
#define XHEAACENCLIB_ERROR_FIRST (1000)
#define XHEAACENCLIB_MAX_WARNINGS (20)

typedef enum {
  XHEAACENCLIB_RETURN_NO_ERROR = 0,

  XHEAACENCLIB_RETURN_WARNING_RUNTIME = XHEAACENCLIB_WARNING_FIRST,

  XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE = XHEAACENCLIB_ERROR_FIRST,
  XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION,
  XHEAACENCLIB_RETURN_ERROR_BUFFER_SIZE,
  XHEAACENCLIB_RETURN_ERROR_BD_CONFIG,
  XHEAACENCLIB_RETURN_ERROR_BD_WRONG_CALLING_SEQUENCE,
  XHEAACENCLIB_RETURN_ERROR_BD_WRONG_ELEMENTID,
  XHEAACENCLIB_RETURN_ERROR_BD_WRONG_PARAMETER,
  XHEAACENCLIB_RETURN_ERROR_BD_WRONG_IMPLEMENTATION,
  XHEAACENCLIB_RETURN_ERROR_CONFIGURATION,
  XHEAACENCLIB_RETURN_ERROR_PARAM_LIST,
  XHEAACENCLIB_RETURN_ERROR_RAP_TOO_CLOSE,
  XHEAACENCLIB_RETURN_ERROR_RAP_TOO_SOON,
  XHEAACENCLIB_RETURN_ERROR_INVALID_RAP_POSITION,
  XHEAACENCLIB_RETURN_ERROR_WRONG_RAP_OCCURRENCE,
  XHEAACENCLIB_RETURN_ERROR_SYNC_FRAME,
  XHEAACENCLIB_RETURN_ERROR_RAP_PROPERTY,
  XHEAACENCLIB_RETURN_ERROR_WRONG_MAX_SIZE_OF_BD,
  XHEAACENCLIB_RETURN_ERROR_TOO_MANY_INPUT_SAMPLES,
  XHEAACENCLIB_RETURN_ERROR_UNEXPECTED_INPUT_SAMPLES,
  XHEAACENCLIB_RETURN_ERROR_LRACONTROL_DRC_GAIN,
  XHEAACENCLIB_RETURN_ERROR_DRC_TOO_MANY_NODES,

  XHEAACENCLIB_RETURN_ERROR_DRC_TOO_MANY_SEQUENCES,
  XHEAACENCLIB_RETURN_ERROR_LIVE_LOUDNESS_INIT_FAIL,
  XHEAACENCLIB_RETURN_ERROR_LIVE_LOUDNESS_PROCESS,
  XHEAACENCLIB_RETURN_ERROR_LOUDNESS_MEASUREMENT,
  XHEAACENCLIB_RETURN_ERROR_SETTING_DEFAULT_LOUDNESS,
  XHEAACENCLIB_RETURN_ERROR_UNKNOWN,
  XHEAACENCLIB_RETURN_ERROR_BUFFER_SWITCHING_DECISION_TOO_SMALL,
  XHEAACENCLIB_RETURN_ERROR_ENC_NOT_FLUSHING_TOO_LESS_SAMPLES,
  XHEAACENCLIB_RETURN_ERROR_nSAMPLES_NOT_MULTIPLE_nIP_CHANNELS,
  XHEAACENCLIB_RETURN_ERROR_DELAY_AND_BUFFER,
  XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER,
  XHEAACENCLIB_RETURN_ERROR_MP4_SAVE_TIMEBUFFER,
  XHEAACENCLIB_RETURN_ERROR_MP4_SPACE_ENC,
  XHEAACENCLIB_RETURN_ERROR_MP4_ASC,
  XHEAACENCLIB_RETURN_ERROR_IIS_SWITCHING_DECISION_FEED,
  XHEAACENCLIB_RETURN_ERROR_INVALID_PARAMETER,
  XHEAACENCLIB_RETURN_ERROR_INVALID_FRAME_SIZE,
  XHEAACENCLIB_RETURN_ERROR_INVALID_CICP_INDEX,
  XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL,
  XHEAACENCLIB_RETURN_ERROR_IPF_NONINDEPENDENT_FRAME,
  XHEAACENCLIB_RETURN_ERROR_IPF_INVALID,
  XHEAACENCLIB_RETURN_ERROR_INVALID_BYTE_ALIGNMENT,
  XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_NO_DATA,
  XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_CONFIG,
  XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_INSTANCE,
  XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_INSTANCE_WR,
  XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_INVALID,
  XHEAACENCLIB_RETURN_ERROR_AOT_CONFIGURATION,
  XHEAACENCLIB_RETURN_ERROR_TOO_MANY_EXTENSION_CONFIGS,
  XHEAACENCLIB_RETURN_ERROR_SPREADING,
  XHEAACENCLIB_RETURN_ERROR_EXTENSION_CONFIG_EXISTS,
  XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD_CONTAINER,
  XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD,
  XHEAACENCLIB_RETURN_ERROR_NO_EXT_ELEMENT,
  XHEAACENCLIB_RETURN_ERROR_INVALID_EXT_ELEMENT,
  XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_DELAY,
  XHEAACENCLIB_RETURN_ERROR_INVALID_RAP_INTERVAL,
  XHEAACENCLIB_RETURN_ERROR_SYNC_FRAME_MEMORY,
  XHEAACENCLIB_RETURN_ERROR_SYNC_INVALID_PARAMETER,
  XHEAACENCLIB_RETURN_ERROR_SYNC_STARTUP_LEN_CHNG,
  XHEAACENCLIB_RETURN_ERROR_SYNC_START_FRAME_LEN_INVALID,
  XHEAACENCLIB_RETURN_ERROR_SYNC_MORE_INTERVALS_SPECIFIED,
  XHEAACENCLIB_RETURN_ERROR_SYNC_EARLIER_SYNC_DELAY,
  XHEAACENCLIB_RETURN_ERROR_SYNC_NO_FORCED_FRAMES,
  XHEAACENCLIB_RETURN_ERROR_SBR,
  XHEAACENCLIB_RETURN_ERROR_SBR_INIT_DELAY_COMP,
  XHEAACENCLIB_RETURN_ERROR_SBR_ENC_OPEN,
  XHEAACENCLIB_RETURN_ERROR_SBR_MEMORY_ALLOCATION,
  XHEAACENCLIB_RETURN_ERROR_BIT_DISTRIBUTION,
  XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD_AUDIO_PREROLL,
  XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_CROSSOVER_FLAG_CHANGE,
  XHEAACENCLIB_RETURN_ERROR_IIS_SWITCHING_DECISION,
  XHEAACENCLIB_RETURN_ERROR_DRC_INVALID_INSTRUCTIONS,
  XHEAACENCLIB_RETURN_ERROR_DRC_UNSUPPORTED_CHARACTERISTICS,
  XHEAACENCLIB_RETURN_ERROR_DRC,
  XHEAACENCLIB_RETURN_ERROR_DRC_IFC_INVALID_SETUP,
  XHEAACENCLIB_RETURN_ERROR_MPEGS_INIT_FAIL,
  XHEAACENCLIB_RETURN_ERROR_CALLOC_FAIL,
  XHEAACENCLIB_RETURN_ERROR_AAC_ENC_UPDATE,
  XHEAACENCLIB_RETURN_ERROR_AAC_HEADER_BITS,
  XHEAACENCLIB_RETURN_ERROR_SAP_SYNC,
  XHEAACENCLIB_RETURN_ERROR_CORE_RESAMPLER,
  XHEAACENCLIB_RETURN_ERROR_BIT_RESERVOIR,
  XHEAACENCLIB_RETURN_ERROR_SIGMAP,
  XHEAACENCLIB_RETURN_ERROR_LOAS_WRAPPER,
} XHEAACENCLIB_RETURN;

typedef enum {
  XHEAACENCLIB_WARN_INVALID = 0,
  XHEAACENCLIB_WARN_RAP_TOO_CLOSE,
  XHEAACENCLIB_WARN_RAP_TOO_SOON,
  XHEAACENCLIB_WARN_INVALID_CONFIG,
  XHEAACENCLIB_WARN_LOUDNESS_DEVIATION,
  XHEAACENCLIB_WARN_LARGE_LOUDNESS_DEVIATION,
  XHEAACENCLIB_WARN_VALUE_OUT_OF_RANGE,
  XHEAACENCLIB_WARN_VALUE_UPDATE_NOT_ALLOWED,

  XHEAACENCLIB_WARN_LAST = XHEAACENCLIB_MAX_WARNINGS
} XHEAACENCLIB_WARNING;

#endif
