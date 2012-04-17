/******************************************************************************

                     (C) Copyright Fraunhofer IIS (2005)
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

   $Id:
   Initial authors:      M. Lohwasser, M. Gayer
   Contents/description:

******************************************************************************/
/*!
  \file
  \brief  Memory layout  $Revision: 36838 $
  \author Markus Lohwasser
*/

#ifndef AAC_ENC_ROM_H
#define AAC_ENC_ROM_H

#include "common_fix.h"

#include "psy_const.h"
#include "psy_configuration.h"
#include "FDK_tools_rom.h"

/*
  Huffman Tables
*/
extern const INT    FDKaacEnc_huff_ltab1_2[3][3][3][3];
extern const INT    FDKaacEnc_huff_ltab3_4[3][3][3][3];
extern const INT    FDKaacEnc_huff_ltab5_6[9][9];
extern const INT    FDKaacEnc_huff_ltab7_8[8][8];
extern const INT    FDKaacEnc_huff_ltab9_10[13][13];
extern const UCHAR  FDKaacEnc_huff_ltab11[17][17];
extern const UCHAR  FDKaacEnc_huff_ltabscf[121];
extern const USHORT FDKaacEnc_huff_ctab1[3][3][3][3];
extern const USHORT FDKaacEnc_huff_ctab2[3][3][3][3];
extern const USHORT FDKaacEnc_huff_ctab3[3][3][3][3];
extern const USHORT FDKaacEnc_huff_ctab4[3][3][3][3];
extern const USHORT FDKaacEnc_huff_ctab5[9][9];
extern const USHORT FDKaacEnc_huff_ctab6[9][9];
extern const USHORT FDKaacEnc_huff_ctab7[8][8];
extern const USHORT FDKaacEnc_huff_ctab8[8][8];
extern const USHORT FDKaacEnc_huff_ctab9[13][13];
extern const USHORT FDKaacEnc_huff_ctab10[13][13];
extern const USHORT FDKaacEnc_huff_ctab11[21][17];
extern const INT    FDKaacEnc_huff_ctabscf[121];

/*
  quantizer
*/
#define MANT_DIGITS 9
#define MANT_SIZE   (1<<MANT_DIGITS)

#if defined(ARCH_PREFER_MULT_32x16)
#define FIXP_QTD FIXP_SGL
#define QTC FX_DBL2FXCONST_SGL
#else
#define FIXP_QTD FIXP_DBL
#define QTC
#endif

extern const FIXP_QTD FDKaacEnc_mTab_3_4[MANT_SIZE];
extern const FIXP_QTD FDKaacEnc_quantTableQ[4];
extern const FIXP_QTD FDKaacEnc_quantTableE[4];

extern const FIXP_DBL FDKaacEnc_mTab_4_3Elc[512];
extern const FIXP_DBL FDKaacEnc_specExpMantTableCombElc[4][14];
extern const UCHAR FDKaacEnc_specExpTableComb[4][14];


/*
  table to count used number of bits
*/
extern const SHORT FDKaacEnc_sideInfoTabLong[MAX_SFB_LONG + 1];
extern const SHORT FDKaacEnc_sideInfoTabShort[MAX_SFB_SHORT + 1];


/*
  Psy Configuration constants
*/
extern const SFB_PARAM_LONG  p_FDKaacEnc_8000_long_1024;
extern const SFB_PARAM_SHORT p_FDKaacEnc_8000_short_128;
extern const SFB_PARAM_LONG  p_FDKaacEnc_11025_long_1024;
extern const SFB_PARAM_SHORT p_FDKaacEnc_11025_short_128;
extern const SFB_PARAM_LONG  p_FDKaacEnc_12000_long_1024;
extern const SFB_PARAM_SHORT p_FDKaacEnc_12000_short_128;
extern const SFB_PARAM_LONG  p_FDKaacEnc_16000_long_1024;
extern const SFB_PARAM_SHORT p_FDKaacEnc_16000_short_128;
extern const SFB_PARAM_LONG  p_FDKaacEnc_22050_long_1024;
extern const SFB_PARAM_SHORT p_FDKaacEnc_22050_short_128;
extern const SFB_PARAM_LONG  p_FDKaacEnc_24000_long_1024;
extern const SFB_PARAM_SHORT p_FDKaacEnc_24000_short_128;
extern const SFB_PARAM_LONG  p_FDKaacEnc_32000_long_1024;
extern const SFB_PARAM_SHORT p_FDKaacEnc_32000_short_128;
extern const SFB_PARAM_LONG  p_FDKaacEnc_44100_long_1024;
extern const SFB_PARAM_SHORT p_FDKaacEnc_44100_short_128;
extern const SFB_PARAM_LONG  p_FDKaacEnc_48000_long_1024;
extern const SFB_PARAM_SHORT p_FDKaacEnc_48000_short_128;
extern const SFB_PARAM_LONG  p_FDKaacEnc_64000_long_1024;
extern const SFB_PARAM_SHORT p_FDKaacEnc_64000_short_128;
extern const SFB_PARAM_LONG  p_FDKaacEnc_88200_long_1024;
extern const SFB_PARAM_SHORT p_FDKaacEnc_88200_short_128;
extern const SFB_PARAM_LONG  p_FDKaacEnc_96000_long_1024;
extern const SFB_PARAM_SHORT p_FDKaacEnc_96000_short_128;


/*
  TNS filter coefficients
*/
extern const FIXP_DBL FDKaacEnc_tnsEncCoeff3[8];
extern const FIXP_DBL FDKaacEnc_tnsCoeff3Borders[8];
extern const FIXP_DBL FDKaacEnc_tnsEncCoeff4[16];
extern const FIXP_DBL FDKaacEnc_tnsCoeff4Borders[16];

#define WTC0 WTC
#define WTC1 WTC
#define WTC2 WTC

extern const FIXP_WTB ELDAnalysis512[1536];
extern const FIXP_WTB ELDAnalysis480[1440];


#endif /* #ifndef AAC_ENC_ROM_H */
