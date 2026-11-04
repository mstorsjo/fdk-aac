
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

#ifndef H_IIS_SIGMAP_H
#define H_IIS_SIGMAP_H

#include "glob_con.h"

#define SIGMAP_MAX_SIGNALS_PER_ELEMENT 2
#define SIGMAP_MAX_CHANNELS 56
#define SIGMAP_MAX_OBJECTS (SIGMAP_MAX_OBJECTS)
#define SIGMAP_MAX_SIGNALS (SIGMAP_MAX_CHANNELS)
#define SIGMAP_MAX_ELEMENTS (SIGMAP_MAX_SIGNALS + 3)
#define SIGMAP_INDEX_MAX_DEF 128
#define SIGMAP_MAX_SIGNAL_GROUPS 32

#define SIGMAP_ERROR_FIRST 1000
typedef enum {
  SIGMAP_NO_ERROR = 0,
  SIGMAP_WARNING_NO_SETUP_IN_OBJECTS,
  SIGMAP_WARNING_NO_MORE_ELEMENT_TYPES,
  SIGMAP_ERROR_INVALID_HANDLE = SIGMAP_ERROR_FIRST,
  SIGMAP_ERROR_MEMORY,
  SIGMAP_ERROR_INVALID_SETUP,
  SIGMAP_ERROR_INVALID_INPUT,
  SIGMAP_ERROR_INVALID_AZIMUTH,
  SIGMAP_ERROR_INVALID_ELEVATION,
  SIGMAP_ERROR_INVALID_LFE_FLAG,
  SIGMAP_ERROR_INVALID_LOUDSPEAKER_INDEX,
  SIGMAP_ERROR_INVALID_CICP_INDEX,
  SIGMAP_ERROR_UNKNOWN
} SIGMAP_RETURN;

typedef enum {
  SIGMAP_ELEMENT_INVALID = -1,
  SIGMAP_ELEMENT_NOT_DEFINED = 0,
  SIGMAP_ELEMENT_SCE = 1,
  SIGMAP_ELEMENT_CPE = 2,
  SIGMAP_ELEMENT_CPE_PS = 3,
  SIGMAP_ELEMENT_CPE_1 = 4,
  SIGMAP_ELEMENT_CPE_2 = 5,
  SIGMAP_ELEMENT_CPE_3 = 6,
  SIGMAP_ELEMENT_LFE = 7,
  SIGMAP_ELEMENT_CCE = 8,
  SIGMAP_ELEMENT_QCE_D = 9,
  SIGMAP_ELEMENT_QCE_0 = 10,
  SIGMAP_ELEMENT_QCE_R = 11,
  SIGMAP_ELEMENT_DSE = 50,
  SIGMAP_ELEMENT_DSE_DRC = 51,
  SIGMAP_ELEMENT_PCE = 52,
  SIGMAP_ELEMENT_OBJ_DISC = 100,
  SIGMAP_ELEMENT_OBJ_HOA = 101,
  SIGMAP_ELEMENT_OBJ_SAOC = 102,

  SIGMAP_ELEMENT_END = 999
} SIGMAP_ELEMENT_TYPE;

typedef enum {
  SIGMAP_SIGNAL_INVALID = -1,
  SIGMAP_SIGNAL_MONO = 1,
  SIGMAP_SIGNAL_STEREO = 2
} SIGMAP_SIGNAL_TYPE;

typedef enum {
  SIGMAP_FROM_FILE = -1,
  SIGMAP_CICP_NOT_DEFINED = 0,
  SIGMAP_CICP_1 = 1,
  SIGMAP_CICP_2 = 2,
  SIGMAP_CICP_3 = 3,
  SIGMAP_CICP_4 = 4,
  SIGMAP_CICP_5 = 5,
  SIGMAP_CICP_6 = 6,
  SIGMAP_CICP_7 = 7,
  SIGMAP_CICP_8 = 8,
  SIGMAP_CICP_9 = 9,
  SIGMAP_CICP_10 = 10,
  SIGMAP_CICP_11 = 11,
  SIGMAP_CICP_12 = 12,
  SIGMAP_CICP_13 = 13,
  SIGMAP_CICP_14 = 14,
  SIGMAP_CICP_15 = 15,
  SIGMAP_CICP_16 = 16,
  SIGMAP_CICP_17 = 17,
  SIGMAP_CICP_18 = 18,
  SIGMAP_CICP_19 = 19,
  SIGMAP_CICP_20 = 20,
  SIGMAP_CICP_RESERVED_FOR_ISO = 63,
  SIGMAP_CUSTOMIZED_3_OVER_5_1 = 64,
  SIGMAP_CUSTOMIZED_4_OVER_5_1 = 65,
  SIGMAP_CUSTOMIZED_7_1_SIDE = 66,
  SIGMAP_INDEX_MAX = SIGMAP_INDEX_MAX_DEF,
  SIGMAP_INVALID = -1000
} SIGMAP_INDEX;

typedef enum {
  SIGMAP_LOUDSPEAKER_POSITION_INVALID = -1,

  SIGMAP_LOUDSPEAKER_POSITION_M_START = 100,
  SIGMAP_LOUDSPEAKER_POSITION_M0,
  SIGMAP_LOUDSPEAKER_POSITION_M22R,
  SIGMAP_LOUDSPEAKER_POSITION_M22L,
  SIGMAP_LOUDSPEAKER_POSITION_M30R,
  SIGMAP_LOUDSPEAKER_POSITION_M30L,
  SIGMAP_LOUDSPEAKER_POSITION_M45R,
  SIGMAP_LOUDSPEAKER_POSITION_M45L,
  SIGMAP_LOUDSPEAKER_POSITION_M60R,
  SIGMAP_LOUDSPEAKER_POSITION_M60L,
  SIGMAP_LOUDSPEAKER_POSITION_M90R,
  SIGMAP_LOUDSPEAKER_POSITION_M90L,
  SIGMAP_LOUDSPEAKER_POSITION_M110R,
  SIGMAP_LOUDSPEAKER_POSITION_M110L,
  SIGMAP_LOUDSPEAKER_POSITION_M135R,
  SIGMAP_LOUDSPEAKER_POSITION_M135L,
  SIGMAP_LOUDSPEAKER_POSITION_M180,

  SIGMAP_LOUDSPEAKER_POSITION_U_START = 200,
  SIGMAP_LOUDSPEAKER_POSITION_U0,
  SIGMAP_LOUDSPEAKER_POSITION_U30R,
  SIGMAP_LOUDSPEAKER_POSITION_U30L,
  SIGMAP_LOUDSPEAKER_POSITION_U90R,
  SIGMAP_LOUDSPEAKER_POSITION_U90L,
  SIGMAP_LOUDSPEAKER_POSITION_U110R,
  SIGMAP_LOUDSPEAKER_POSITION_U110L,
  SIGMAP_LOUDSPEAKER_POSITION_U135R,
  SIGMAP_LOUDSPEAKER_POSITION_U135L,
  SIGMAP_LOUDSPEAKER_POSITION_U180,

  SIGMAP_LOUDSPEAKER_POSITION_L_START = 300,
  SIGMAP_LOUDSPEAKER_POSITION_L0,
  SIGMAP_LOUDSPEAKER_POSITION_L45R,
  SIGMAP_LOUDSPEAKER_POSITION_L45L,

  SIGMAP_LOUDSPEAKER_POSITION_T_START = 400,
  SIGMAP_LOUDSPEAKER_POSITION_T90,

  SIGMAP_LOUDSPEAKER_POSITION_E_START = 500,
  SIGMAP_LOUDSPEAKER_POSITION_E_LEFT_EDGE,
  SIGMAP_LOUDSPEAKER_POSITION_E_RIGHT_EDGE,
  SIGMAP_LOUDSPEAKER_POSITION_E_HALF_LEFT_EDGE,
  SIGMAP_LOUDSPEAKER_POSITION_E_HALF_RIGHT_EDGE,

  SIGMAP_LOUDSPEAKER_POSITION_END = 1000
} SIGMAP_LOUDSPEAKER_POSITION;

typedef enum SIGMAP_CICP_SPEAKER {
  SIGMAP_CICP_SPEAKER_INVALID = -1,
  SIGMAP_CICP_SPEAKER_L = 0,
  SIGMAP_CICP_SPEAKER_R = 1,
  SIGMAP_CICP_SPEAKER_C = 2,
  SIGMAP_CICP_SPEAKER_LFE = 3,
  SIGMAP_CICP_SPEAKER_LS = 4,
  SIGMAP_CICP_SPEAKER_RS = 5,
  SIGMAP_CICP_SPEAKER_LC = 6,
  SIGMAP_CICP_SPEAKER_RC = 7,
  SIGMAP_CICP_SPEAKER_LSR = 8,
  SIGMAP_CICP_SPEAKER_RSR = 9,
  SIGMAP_CICP_SPEAKER_CS = 10,
  SIGMAP_CICP_SPEAKER_LSD = 11,
  SIGMAP_CICP_SPEAKER_RSD = 12,
  SIGMAP_CICP_SPEAKER_LSS = 13,
  SIGMAP_CICP_SPEAKER_RSS = 14,
  SIGMAP_CICP_SPEAKER_LW = 15,
  SIGMAP_CICP_SPEAKER_RW = 16,
  SIGMAP_CICP_SPEAKER_LV = 17,
  SIGMAP_CICP_SPEAKER_RV = 18,
  SIGMAP_CICP_SPEAKER_CV = 19,
  SIGMAP_CICP_SPEAKER_LVR = 20,
  SIGMAP_CICP_SPEAKER_RVR = 21,
  SIGMAP_CICP_SPEAKER_CVR = 22,
  SIGMAP_CICP_SPEAKER_LVSS = 23,
  SIGMAP_CICP_SPEAKER_RVSS = 24,
  SIGMAP_CICP_SPEAKER_TS = 25,
  SIGMAP_CICP_SPEAKER_LFE2 = 26,
  SIGMAP_CICP_SPEAKER_LB = 27,
  SIGMAP_CICP_SPEAKER_RB = 28,
  SIGMAP_CICP_SPEAKER_CB = 29,
  SIGMAP_CICP_SPEAKER_LVS = 30,
  SIGMAP_CICP_SPEAKER_RVS = 31,

  SIGMAP_CICP_SPEAKER_RESERVED_32 = 32,
  SIGMAP_CICP_SPEAKER_RESERVED_33 = 33,
  SIGMAP_CICP_SPEAKER_RESERVED_34 = 34,
  SIGMAP_CICP_SPEAKER_RESERVED_35 = 35,
  SIGMAP_CICP_SPEAKER_LFE3 = 36,
  SIGMAP_CICP_SPEAKER_LEOS = 37,
  SIGMAP_CICP_SPEAKER_REOS = 38,
  SIGMAP_CICP_SPEAKER_HWBCAL = 39,
  SIGMAP_CICP_SPEAKER_HWBCAR = 40,
  SIGMAP_CICP_SPEAKER_LBS = 41,
  SIGMAP_CICP_SPEAKER_RBS = 42

} SIGMAP_CICP_SPEAKER;

typedef enum SIGMAP_CICP_SPEAKER_LAYOUT {
  SIGMAP_CICP_SPEAKER_LAYOUT_INVALID = -1,
  SIGMAP_CICP_SPEAKER_LAYOUT_ANY = 0,
  SIGMAP_CICP_SPEAKER_LAYOUT_1_0_0 = 1,
  SIGMAP_CICP_SPEAKER_LAYOUT_1_DOT_0 = SIGMAP_CICP_SPEAKER_LAYOUT_1_0_0,
  SIGMAP_CICP_SPEAKER_LAYOUT_2_0_0 = 2,
  SIGMAP_CICP_SPEAKER_LAYOUT_3_0_0 = 3,
  SIGMAP_CICP_SPEAKER_LAYOUT_3_1_0 = 4,
  SIGMAP_CICP_SPEAKER_LAYOUT_3_2_0 = 5,
  SIGMAP_CICP_SPEAKER_LAYOUT_3_2_1 = 6,
  SIGMAP_CICP_SPEAKER_LAYOUT_5_DOT_1 = SIGMAP_CICP_SPEAKER_LAYOUT_3_2_1,
  SIGMAP_CICP_SPEAKER_LAYOUT_5_2_1 = 7,
  SIGMAP_CICP_SPEAKER_LAYOUT_1_PLUS_1 = 8,
  SIGMAP_CICP_SPEAKER_LAYOUT_2_1_0 = 9,
  SIGMAP_CICP_SPEAKER_LAYOUT_2_2_0 = 10,
  SIGMAP_CICP_SPEAKER_LAYOUT_3_3_1 = 11,
  SIGMAP_CICP_SPEAKER_LAYOUT_3_4_1 = 12,
  SIGMAP_CICP_SPEAKER_LAYOUT_7_DOT_1 = SIGMAP_CICP_SPEAKER_LAYOUT_3_4_1,
  SIGMAP_CICP_SPEAKER_LAYOUT_11_11_2 = 13,
  SIGMAP_CICP_SPEAKER_LAYOUT_22_DOT_2 = SIGMAP_CICP_SPEAKER_LAYOUT_11_11_2,
  SIGMAP_CICP_SPEAKER_LAYOUT_5_2_1_ELEV = 14,
  SIGMAP_CICP_SPEAKER_LAYOUT_5_5_2 = 15,
  SIGMAP_CICP_SPEAKER_LAYOUT_10_DOT_2 = SIGMAP_CICP_SPEAKER_LAYOUT_5_5_2,
  SIGMAP_CICP_SPEAKER_LAYOUT_5_4_1 = 16,
  SIGMAP_CICP_SPEAKER_LAYOUT_6_5_1 = 17,
  SIGMAP_CICP_SPEAKER_LAYOUT_6_7_1 = 18,
  SIGMAP_CICP_SPEAKER_LAYOUT_7_4_1 = 19,
  SIGMAP_CICP_SPEAKER_LAYOUT_7_DOT_1_PLUS_4 = SIGMAP_CICP_SPEAKER_LAYOUT_7_4_1,
  SIGMAP_CICP_SPEAKER_LAYOUT_9_4_1 = 20
} SIGMAP_CICP_SPEAKER_LAYOUT;

typedef enum {
  SIGMAP_SPEAKER_LAYOUT_UNKNOWN = -1,
  SIGMAP_SPEAKER_LAYOUT_CICP_SETUP = 0,
  SIGMAP_SPEAKER_LAYOUT_CICP_SPEAKER = 1,
  SIGMAP_SPEAKER_LAYOUT_GEOMETRY = 2,
  SIGMAP_SPEAKER_LAYOUT_CONTRIBUTION = 3
} SIGMAP_SPEAKER_LAYOUT_TYPE;

typedef enum {
  SIGMAP_SIGNAL_GROUP_TYPE_UNKNOWN = -1,
  SIGMAP_SIGNAL_SIGNAL_GROUP_TYPE_CHANNELS = 0,
  SIGMAP_SIGNAL_SIGNAL_GROUP_TYPE_OBJECT = 1,
  SIGMAP_SIGNAL_SIGNAL_GROUP_TYPE_SAOC = 2,
  SIGMAP_SIGNAL_SIGNAL_GROUP_TYPE_HOA = 3
} SIGMAP_SIGNAL_GROUP_TYPE;

typedef struct {
  SIGMAP_SPEAKER_LAYOUT_TYPE speakerLayoutType;
  unsigned int numSpeakers;
  SIGMAP_CICP_SPEAKER_LAYOUT cicpSpeakerLayoutIndex;
  SIGMAP_CICP_SPEAKER cicpSpeakerIdxList[SIGMAP_MAX_CHANNELS];
} CHANNEL_GROUP_INFO;

typedef struct {
  SIGMAP_SIGNAL_GROUP_TYPE signalGroupType;
  CHANNEL_GROUP_INFO channelGroupInfo;

} SIGNAL_GROUP_INFO;

typedef enum {
  SIGMAP_CICP_STATUS_KNOWN = 1,
  SIGMAP_CICP_STATUS_UNKNOWN = -1
} SIGMAP_CICP_STATUS;

typedef enum {
  SIGMAP_TYPE_INVALID = -1,
  SIGMAP_TYPE_NOT_DEFINED = 0,
  SIGMAP_TYPE_MPEG4 = 1,
  SIGMAP_TYPE_USAC,
  SIGMAP_TYPE_MPEGH
} SIGMAP_TYPE;

typedef struct element_info_struct {
  ELEMENT_TYPE elType;
  int instanceTag;
  int nChannelsInEl;
  int bMpegs212;
  int ChannelIndex[SIGMAP_MAX_SIGNALS_PER_ELEMENT];
  int ChannelShift[SIGMAP_MAX_SIGNALS_PER_ELEMENT];
  int ChannelShiftBits[SIGMAP_MAX_SIGNALS_PER_ELEMENT];
  int coreMode[SIGMAP_MAX_SIGNALS_PER_ELEMENT];
  float relativeBits;
  int isDRC;
  int isPS;
  int isHOA;
  int isQCEResidual;
  SIGMAP_ELEMENT_TYPE sigMapType;
} ELEMENT_INFO, *ELEMENT_INFO_HANDLE;

typedef struct channel_mapping_struct {
  SIGMAP_INDEX cicpLayoutIndex;
  int nElements;
  int nChannels;
  int nEffectiveChannels;
  int monoStereoMode;
  int nSigGroups;
  int sigGroupID[SIGMAP_MAX_ELEMENTS];
  ELEMENT_INFO elInfo[SIGMAP_MAX_ELEMENTS];
  SIGNAL_GROUP_INFO sigGroupInfo[SIGMAP_MAX_SIGNAL_GROUPS];
} CHANNEL_MAPPING, *CHANNEL_MAPPING_HANDLE;

typedef struct sigmap_object_list_struct *SIGMAP_OBJECT_LIST_HANDLE;

typedef struct sigmap_setup_struct {
  SIGMAP_TYPE type;
  SIGMAP_INDEX cicpLayoutIndex;
  SIGMAP_OBJECT_LIST_HANDLE hObjects;
  SIGMAP_ELEMENT_TYPE elementType[SIGMAP_MAX_ELEMENTS];
  int sigGroupID[SIGMAP_MAX_ELEMENTS];
} SIGMAP_SETUP, *SIGMAP_SETUP_HANDLE;

typedef struct sigmap_instance_struct {
  char infoModuleVersion[32];
} SIGMAP_INSTANCE, *SIGMAP_INSTANCE_HANDLE;

SIGMAP_RETURN iisSigMapNew(
    SIGMAP_INSTANCE_HANDLE *phInstance);

SIGMAP_RETURN iisSigMapConfigure(
    SIGMAP_INSTANCE_HANDLE hInstance,
    SIGMAP_SETUP_HANDLE hSetup,
    CHANNEL_MAPPING_HANDLE hChMap);

SIGMAP_RETURN iisSigMapGetChannelMap(
    SIGMAP_INSTANCE_HANDLE hInstance,
    CHANNEL_MAPPING_HANDLE hChMap);

SIGMAP_RETURN iisSigMapDelete(
    SIGMAP_INSTANCE_HANDLE hInstance);

SIGMAP_RETURN iisSigMapAddElement(
    SIGMAP_SETUP_HANDLE hSetup,
    SIGMAP_ELEMENT_TYPE elementType,
    float relativeBits,
    int sigGroupID);

SIGMAP_RETURN iisSigMapAddSignal(
    SIGMAP_SETUP_HANDLE hSetup,
    SIGMAP_SIGNAL_TYPE signalType,
    float relativeBits,
    int sigGroupID);

SIGMAP_RETURN iisSigMapAddLoudspeaker(
    SIGMAP_SETUP_HANDLE hSetup,
    SIGMAP_LOUDSPEAKER_POSITION position,
    float relativeBits,
    int sigGroupID);

void iisSigMapDeleteObjectsFromSetup(
    SIGMAP_SETUP_HANDLE hSetup);

SIGMAP_RETURN iisSigMapInitDefaultSignalGroupInfo(
    SIGNAL_GROUP_INFO *const pSignGroupInfo);

SIGMAP_RETURN iisSigMapAddSignalGroupInfo(
    CHANNEL_MAPPING_HANDLE hChMap,
    SIGNAL_GROUP_INFO const *const pSignGroupInfo,
    int sigGroupID);

#endif
