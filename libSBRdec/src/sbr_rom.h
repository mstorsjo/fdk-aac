/****************************************************************************

                     (C) Copyright Fraunhofer IIS (2004)
                               All Rights Reserved

    Please be advised that this software and/or program delivery is
    Confidential Information of Fraunhofer and subject to and covered by the

    Fraunhofer IIS Software Evaluation Agreement
    between Google Inc. and  Fraunhofer
    effective and in full force since March 1, 2012.

    You may use this software and/or program only under the terms and
    conditions described in the above mentioned Fraunhofer IIS Software
    Evaluation Agreement. Any other and/or further use requires a separate agreement.


   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

 $Id$

*******************************************************************************/
/*!
\file
\brief Declaration of constant tables
$Revision: 36841 $
*/
#ifndef __rom_H
#define __rom_H

#include "sbrdecoder.h"
#include "env_extr.h"
#include "qmf.h"

#define INV_INT_TABLE_SIZE      49
#define SBR_NF_NO_RANDOM_VAL  512   /*!< Size of random number array for noise floor */

/*
  Frequency scales
*/
extern const UCHAR FDK_sbrDecoder_sbr_start_freq_16[16];
extern const UCHAR FDK_sbrDecoder_sbr_start_freq_22[16];
extern const UCHAR FDK_sbrDecoder_sbr_start_freq_24[16];
extern const UCHAR FDK_sbrDecoder_sbr_start_freq_32[16];
extern const UCHAR FDK_sbrDecoder_sbr_start_freq_40[16];
extern const UCHAR FDK_sbrDecoder_sbr_start_freq_44[16];
extern const UCHAR FDK_sbrDecoder_sbr_start_freq_48[16];

/*
  Low-Power-Profile Transposer
*/
#define NUM_WHFACTOR_TABLE_ENTRIES  9
extern const USHORT FDK_sbrDecoder_sbr_whFactorsIndex[NUM_WHFACTOR_TABLE_ENTRIES];
extern const FIXP_DBL FDK_sbrDecoder_sbr_whFactorsTable[NUM_WHFACTOR_TABLE_ENTRIES][6];



/*
  Envelope Adjustor
*/
extern const FIXP_SGL FDK_sbrDecoder_sbr_limGains_m[4];
extern const UCHAR    FDK_sbrDecoder_sbr_limGains_e[4];
extern const FIXP_SGL FDK_sbrDecoder_sbr_limiterBandsPerOctaveDiv4[4];
extern const FIXP_SGL FDK_sbrDecoder_sbr_smoothFilter[4];
extern const FIXP_SGL FDK_sbrDecoder_sbr_randomPhase[SBR_NF_NO_RANDOM_VAL][2];
extern const FIXP_SGL harmonicPhaseX [2][4];

/*
  Envelope Extractor
*/
extern const int FDK_sbrDecoder_envelopeTable_8  [8][5];
extern const int FDK_sbrDecoder_envelopeTable_15 [15][6];
extern const int FDK_sbrDecoder_envelopeTable_16 [16][6];

extern const FRAME_INFO FDK_sbrDecoder_sbr_frame_info1_15;
extern const FRAME_INFO FDK_sbrDecoder_sbr_frame_info2_15;
extern const FRAME_INFO FDK_sbrDecoder_sbr_frame_info4_15;
extern const FRAME_INFO FDK_sbrDecoder_sbr_frame_info8_15;

extern const FRAME_INFO FDK_sbrDecoder_sbr_frame_info1_16;
extern const FRAME_INFO FDK_sbrDecoder_sbr_frame_info2_16;
extern const FRAME_INFO FDK_sbrDecoder_sbr_frame_info4_16;
extern const FRAME_INFO FDK_sbrDecoder_sbr_frame_info8_16;

extern const SCHAR FDK_sbrDecoder_sbr_huffBook_EnvLevel10T[120][2];
extern const SCHAR FDK_sbrDecoder_sbr_huffBook_EnvLevel10F[120][2];
extern const SCHAR FDK_sbrDecoder_sbr_huffBook_EnvBalance10T[48][2];
extern const SCHAR FDK_sbrDecoder_sbr_huffBook_EnvBalance10F[48][2];
extern const SCHAR FDK_sbrDecoder_sbr_huffBook_EnvLevel11T[62][2];
extern const SCHAR FDK_sbrDecoder_sbr_huffBook_EnvLevel11F[62][2];
extern const SCHAR FDK_sbrDecoder_sbr_huffBook_EnvBalance11T[24][2];
extern const SCHAR FDK_sbrDecoder_sbr_huffBook_EnvBalance11F[24][2];
extern const SCHAR FDK_sbrDecoder_sbr_huffBook_NoiseLevel11T[62][2];
extern const SCHAR FDK_sbrDecoder_sbr_huffBook_NoiseBalance11T[24][2];


/*
 Parametric stereo
*/


extern const FIXP_DBL decayScaleFactTable[NO_QMF_CHANNELS];

/* FIX_BORDER can have 0, 1, 2, 4 envelops */
extern const UCHAR FDK_sbrDecoder_aFixNoEnvDecode[4];

/* IID & ICC Huffman codebooks */
extern const SCHAR aBookPsIidTimeDecode[28][2];
extern const SCHAR aBookPsIidFreqDecode[28][2];
extern const SCHAR aBookPsIccTimeDecode[14][2];
extern const SCHAR aBookPsIccFreqDecode[14][2];

/* IID-fine Huffman codebooks */

extern const SCHAR aBookPsIidFineTimeDecode[60][2];
extern const SCHAR aBookPsIidFineFreqDecode[60][2];

/* the values of the following 3 tables are shiftet right by 1 ! */
extern const FIXP_DBL ScaleFactors[NO_IID_LEVELS];
extern const FIXP_DBL ScaleFactorsFine[NO_IID_LEVELS_FINE];
extern const FIXP_DBL Alphas[NO_ICC_LEVELS];

#if defined(ARCH_PREFER_MULT_32x16)
extern const FIXP_SGL aAllpassLinkDecaySer[NO_SERIAL_ALLPASS_LINKS];
extern const FIXP_SGL aaFractDelayPhaseFactorReQmf[NO_QMF_CHANNELS];
extern const FIXP_SGL aaFractDelayPhaseFactorImQmf[NO_QMF_CHANNELS];
extern const FIXP_SGL aaFractDelayPhaseFactorReSubQmf20[NO_SUB_QMF_CHANNELS];
extern const FIXP_SGL aaFractDelayPhaseFactorImSubQmf20[NO_SUB_QMF_CHANNELS];

extern const FIXP_SGL aaFractDelayPhaseFactorSerReQmf[NO_QMF_CHANNELS][NO_SERIAL_ALLPASS_LINKS];
extern const FIXP_SGL aaFractDelayPhaseFactorSerImQmf[NO_QMF_CHANNELS][NO_SERIAL_ALLPASS_LINKS];
extern const FIXP_SGL aaFractDelayPhaseFactorSerReSubQmf20[NO_SUB_QMF_CHANNELS][NO_SERIAL_ALLPASS_LINKS];
extern const FIXP_SGL aaFractDelayPhaseFactorSerImSubQmf20[NO_SUB_QMF_CHANNELS][NO_SERIAL_ALLPASS_LINKS];

extern const FIXP_SGL p8_13_20[13];
extern const FIXP_SGL p2_13_20[13];

#else
extern const FIXP_DBL aAllpassLinkDecaySer[NO_SERIAL_ALLPASS_LINKS];
extern const FIXP_DBL aaFractDelayPhaseFactorReQmf[NO_QMF_CHANNELS];
extern const FIXP_DBL aaFractDelayPhaseFactorImQmf[NO_QMF_CHANNELS];
extern const FIXP_DBL aaFractDelayPhaseFactorReSubQmf20[NO_SUB_QMF_CHANNELS];
extern const FIXP_DBL aaFractDelayPhaseFactorImSubQmf20[NO_SUB_QMF_CHANNELS];

extern const FIXP_DBL aaFractDelayPhaseFactorSerReQmf[NO_QMF_CHANNELS][NO_SERIAL_ALLPASS_LINKS];
extern const FIXP_DBL aaFractDelayPhaseFactorSerImQmf[NO_QMF_CHANNELS][NO_SERIAL_ALLPASS_LINKS];
extern const FIXP_DBL aaFractDelayPhaseFactorSerReSubQmf20[NO_SUB_QMF_CHANNELS][NO_SERIAL_ALLPASS_LINKS];
extern const FIXP_DBL aaFractDelayPhaseFactorSerImSubQmf20[NO_SUB_QMF_CHANNELS][NO_SERIAL_ALLPASS_LINKS];

extern const FIXP_DBL p8_13_20[13];
extern const FIXP_DBL p2_13_20[13];
#endif

extern const UCHAR aAllpassLinkDelaySer[3];
extern const UCHAR delayIndexQmf[NO_QMF_CHANNELS];
extern const UCHAR groupBorders20[NO_IID_GROUPS + 1];
extern const UCHAR groupBorders34[NO_IID_GROUPS_HI_RES + 1];
extern const UCHAR bins2groupMap20[NO_IID_GROUPS];
extern const UCHAR quantizedIIDs[NO_IID_STEPS];
extern const UCHAR quantizedIIDsFine[NO_IID_STEPS_FINE];
extern const UCHAR FDK_sbrDecoder_aNoIidBins[3];
extern const UCHAR FDK_sbrDecoder_aNoIccBins[3];


/* Lookup tables for some arithmetic functions */

#define INV_TABLE_BITS 8
#define INV_TABLE_SIZE (1<<INV_TABLE_BITS)
extern const FIXP_SGL FDK_sbrDecoder_invTable[INV_TABLE_SIZE];

#endif // __rom_H
