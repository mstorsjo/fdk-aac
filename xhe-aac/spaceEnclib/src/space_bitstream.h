
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

#ifndef INCLUDED_SPACE_BITSTREAM_H
#define INCLUDED_SPACE_BITSTREAM_H

#include "iisutillib.h"
#include "bit_buf.h"

#include "spaceEnclib_const.h"

#define MAX_NUM_BOXES 8
#define MAX_NUM_BINS 28
#define MAX_NUM_PARAMS 8
#define MAX_NUM_ARBDMX_CH 2
#define MAX_NUM_OUTPUTCHANNELS 16
#define MAX_AAC_SHORTWINDOWGROUPS 4
#define MAX_AAC_MDCT 1024
#define MAX_AAC_FRAMES 2
#define MAX_RESIDUAL_FRAMES 2
#define MAX_NUM_CH_PER_RES_ELEM 2
#define BITBUFSIZE 768
#define EIGHT_SHORT_SEQUENCE 2
#define NUM_DATATYPES 3
#define MAX_TIME_SLOTS 128

typedef enum {
  TREE_USAC_212 = 16
} TREECONFIG;

typedef enum {
  MPS_MODE_INVALID = -1,
  MPS_MODE_DEFAULT = 0,
  MPS_MODE_LD = 1,

  MPS_MODE_USAC = 2,
  MPS_MODE_USAC_UNISTE = 3

} MPS_MODE;

typedef enum {
  FREQ_RES_40 = 0,
  FREQ_RES_20 = 1,
  FREQ_RES_10 = 2,
  FREQ_RES_5 = 3
} FREQ;

typedef enum {
  QUANTMODE_INVALID = -1,
  QUANTMODE_FINE = 0,
  QUANTMODE_EBQ1 = 1,
  QUANTMODE_EBQ2 = 2
} QUANTMODE;

typedef enum {
  TEMPSHAPE_INVALID = -1,
  TEMPSHAPE_OFF = 0,
  TEMPSHAPE_STP = 1,
  TEMPSHAPE_GES = 2,
  TEMPSHAPE_TSD = 3
} TEMPSHAPECONFIG;

typedef enum {
  FIXEDGAINDMX_INVALID = -1,
  FIXEDGAINDMX_0 = 0,
  FIXEDGAINDMX_1 = 1,
  FIXEDGAINDMX_2 = 2,
  FIXEDGAINDMX_3 = 3,
  FIXEDGAINDMX_4 = 4,
  FIXEDGAINDMX_5 = 5,
  FIXEDGAINDMX_6 = 6,
  FIXEDGAINDMX_7 = 7
} FIXEDGAINDMXCONFIG;

typedef enum {
  DECORR_INVALID = -1,
  DECORR_QMFSPLIT0 = 0,
  DECORR_QMFSPLIT1 = 1,
  DECORR_QMFSPLIT2 = 2
} DECORRCONFIG;

typedef enum {
  DOWNMIXTYPE_INVALID = -1,
  DOWNMIXTYPE_CLASSICMPS = 0,
  DOWNMIXTYPE_SIMPLIFIED = 3,
  DOWNMIXTYPE_SIMPLIFIED_ABOVE_RESIDUAL = 4
} DOWNMIXTYPE;

typedef enum {
  IPDMODE_INVALID = -1,
  IPDMODE_NONE = 0,
  IPDMODE_NO_RESIDUAL = 1,
  IPDMODE_RESIDUAL = 2,
  IPDMODE_LAST = IPDMODE_RESIDUAL
} IPDMODE;

typedef enum {
  DEFAULT = 0,
  KEEP = 1,
  INTERPOLATE = 2,
  FINECOARSE = 3
} DATA_MODE;

typedef struct {
  int numOttBoxes;
  int ottModeLfe[MAX_NUM_BOXES];
  int numTttBoxes;
  int numInChan;
  int numOutChan;
} TREEDESCRIPTION;

typedef struct {
  int bsOttBands;
} OTTCONFIG;

typedef struct {
  int bsResidualPresent;
  int bsResidualBands;
} RESIDUALCONFIG;

typedef struct {
  int bsSamplingFrequency;
  int bsFrameLength;
  int numBands;
  TREECONFIG bsTreeConfig;
  QUANTMODE bsQuantMode;
  int bsOneIcc;
  int bsResidualCoding;
  FIXEDGAINDMXCONFIG bsFixedGainDMX;
  int bsMatrixMode;
  TEMPSHAPECONFIG bsTempShapeConfig;
  int bsEnvQuantMode;
  DECORRCONFIG bsDecorrConfig;
  int bs3DaudioMode;
  TREEDESCRIPTION treeDescription;
  OTTCONFIG ottConfig[MAX_NUM_BOXES];
  int aacResidualFramesPerSpatialFrame;
  int aacResidualUseTNS;
  float aacResidualBWHz[MAX_NUM_BOXES];
  int aacResidualBitRate[MAX_NUM_BOXES];
  int bsResidualSamplingFrequency;
  int bsResidualFramesPerSpatialFrame;
  RESIDUALCONFIG residualConfig[MAX_NUM_BOXES];

  int bLowDelay;

  int bsHighRateMode;
  IPDMODE bsIpdMode;
  int bsOpdSmoothing;
  int bsPseudoLr;

  int bsOttBandsPhase;
  int bsOttBandsPhasePresent;

} SPATIALSPECIFICCONFIG;

typedef struct {
  int bsFramingType;
  int numParamSets;
  int bsParamSlots[MAX_NUM_PARAMS];
} FRAMINGINFO;

typedef struct {
  int cld[MAX_NUM_BOXES][MAX_NUM_PARAMS][MAX_NUM_BINS];
  int icc[MAX_NUM_BOXES][MAX_NUM_PARAMS][MAX_NUM_BINS];
  int cld_old[MAX_NUM_BOXES][MAX_NUM_BINS];
  int icc_old[MAX_NUM_BOXES][MAX_NUM_BINS];

  int ipd[MAX_NUM_BOXES][MAX_NUM_PARAMS][MAX_NUM_BINS];
  int ipd_old[MAX_NUM_BOXES][MAX_NUM_BINS];

  int quantCoarseCldPrev[MAX_NUM_BOXES][MAX_NUM_PARAMS];
  int quantCoarseIccPrev[MAX_NUM_BOXES][MAX_NUM_PARAMS];
  int quantCoarseIpdPrev[MAX_NUM_BOXES][MAX_NUM_PARAMS];
} OTTDATA;

typedef struct {
  int bsSmoothControl;
  int bsSmoothMode[MAX_NUM_PARAMS];
  int bsSmoothTime[MAX_NUM_PARAMS];
  int bsFreqResStride[MAX_NUM_PARAMS];
  int bsSmgData[MAX_NUM_PARAMS][MAX_NUM_BINS];
} SMGDATA;

typedef struct {
  int cld[MAX_NUM_ARBDMX_CH][MAX_NUM_PARAMS][MAX_NUM_BINS];
  int cld_old[MAX_NUM_ARBDMX_CH][MAX_NUM_BINS];
  int quantCoarseCldPrev[MAX_NUM_ARBDMX_CH][MAX_NUM_PARAMS];
  int bsTsdEnable;
  int tsdSepData[MAX_TIME_SLOTS];
  int bsTsdTrPhaseData[MAX_TIME_SLOTS];
  int bsTsdNumTrSlots;
  unsigned short bsTsdCodedPos[4];
  int TsdCodewordLength;
} TSDDATA;

typedef struct {
  int bsIccDiffPresent;
  int bsIccDiff[MAX_NUM_BINS];
} RESIDUALICCDIFFDATA;

typedef struct {
  int windowSequence;
  int windowGrouping[MAX_AAC_SHORTWINDOWGROUPS];
  float mdctSample[MAX_AAC_MDCT];
} RESIDUALAACFRAMEDATA;

typedef struct {
  RESIDUALICCDIFFDATA iccDiffData[MAX_NUM_PARAMS];
  RESIDUALAACFRAMEDATA aacFrameData[MAX_RESIDUAL_FRAMES][MAX_AAC_FRAMES];
} RESIDUALDATA;

typedef struct {
  int bsXXXDataMode[MAX_NUM_BOXES][MAX_NUM_PARAMS];
  int bsDataPair[MAX_NUM_BOXES][MAX_NUM_PARAMS];
  int bsQuantCoarseXXX[MAX_NUM_BOXES][MAX_NUM_PARAMS];
  int bsFreqResStrideXXX[MAX_NUM_BOXES][MAX_NUM_PARAMS];
} LOSSLESSDATA;

typedef struct {
  FRAMINGINFO framingInfo;
  int bsIndependencyFlag;
  OTTDATA ottData;
  SMGDATA smgData;

  TSDDATA tsdData;

  RESIDUALDATA residualData[MAX_NUM_BOXES];
  RESIDUALDATA residualDataArbDmx[MAX_NUM_CH_PER_RES_ELEM * MAX_NUM_ARBDMX_RES_ELEMS];
  LOSSLESSDATA CLDLosslessData;
  LOSSLESSDATA ICCLosslessData;

  LOSSLESSDATA IPDLosslessData;
  int bsPhaseMode;
  int numBinsIPD;

  LOSSLESSDATA CPCLosslessData;
  LOSSLESSDATA ADGLosslessData;
  int aacWindowGrouping[MAX_AAC_FRAMES][MAX_AAC_SHORTWINDOWGROUPS];
  int aacWindowSequence;
  int bUseBBCues;
  int numArbDmxChannels;
  float alpha;
} SPATIALFRAME;

typedef struct {
  unsigned int sampleRate;
  MP4SPACEENC_RES_CODEC codec;
  int enabled[MAX_NUM_BOXES];
  float bandWidth[MAX_NUM_BOXES];
  unsigned int bitRate[MAX_NUM_BOXES];
  int restrictResidualFramesize[MAX_NUM_BOXES];
} BSF_RESIDUAL_CONFIG;

typedef struct {
  unsigned int sampleRate;
  MP4SPACEENC_RES_CODEC codec;
  float bandWidth;
  unsigned int bitRate[MAX_NUM_ARBDMX_RES_ELEMS];
  int numArbDmxChannels;
} BSF_RESIDUAL_CONFIG_ARB_DMX;

typedef struct BSF_INSTANCE *HANDLE_BSF_INSTANCE;

HANDLE_ERROR_INFO DestroySpatialBitstreamEncoder(HANDLE_BSF_INSTANCE *selfPtr);

HANDLE_ERROR_INFO CreateSpatialBitstreamEncoder(HANDLE_BSF_INSTANCE *selfPtr, BSF_RESIDUAL_CONFIG *pConfig, BSF_RESIDUAL_CONFIG_ARB_DMX *pConfigArbDmx);

SPATIALSPECIFICCONFIG *GetSpatialSpecificConfig(HANDLE_BSF_INSTANCE selfPtr);

HANDLE_ERROR_INFO WriteSpatialSpecificConfig(HANDLE_BIT_BUF bitstream, SPATIALSPECIFICCONFIG *spatialSpecificConfig, int *pnBitsWritten);

SPATIALFRAME *GetSpatialFrame(HANDLE_BSF_INSTANCE selfPtr, int nOffset);

HANDLE_ERROR_INFO WriteSpatialFrame(HANDLE_BIT_BUF bitstream, HANDLE_BSF_INSTANCE selfPtr, int bUsacIndependencyFlag);

HANDLE_ERROR_INFO DuplicateParameterSet(SPATIALFRAME *hFrom,
                                        int setFrom,
                                        SPATIALFRAME *hTo,
                                        int setTo);

int GetResidualSampleRate(int spatialSampleRate,
                          int frameTimeSlots,
                          int resFramesPerSpatial,
                          MP4SPACEENC_MODE encMode);

HANDLE_ERROR_INFO
GetUsacMps212Config(SPATIALSPECIFICCONFIG *hSsc,
                    MP4SPACEENC_USAC_MPS212_CONFIG *pUsacMps212Config);

#endif
