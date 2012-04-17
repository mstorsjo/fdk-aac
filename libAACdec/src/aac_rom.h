/*****************************  MPEG-4 AAC Decoder  **************************

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


   $Id$
   Author(s):   Josef Hoepfl
   Description: Definition of constant tables

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef AAC_ROM_H
#define AAC_ROM_H

#include "common_fix.h"
#include "FDK_audio.h"
#include "aacdec_hcr_types.h"
#include "aacdec_hcrs.h"

#define AAC_NF_NO_RANDOM_VAL  512   /*!< Size of random number array for noise floor */

#define INV_QUANT_TABLESIZE 256

extern const FIXP_DBL InverseQuantTable [INV_QUANT_TABLESIZE + 1] ;
extern const FIXP_DBL MantissaTable [4][14] ;
extern const SCHAR ExponentTable [4][14] ;

#define NUM_LD_COEF_512 1536
#define NUM_LD_COEF_480 1440
/* Window table partition exponents. */
#define WTS0 (1)
#define WTS1 (0)
#define WTS2 (-2)
extern const FIXP_WTB LowDelaySynthesis512[1536];
extern const FIXP_WTB LowDelaySynthesis480[1440];

typedef struct {
  const SHORT *sfbOffsetLong;
  const SHORT *sfbOffsetShort;
  UCHAR numberOfSfbLong;
  UCHAR numberOfSfbShort;
} SFB_INFO;

extern const SFB_INFO sfbOffsetTables[5][16];

/* Huffman tables */
enum {
  HuffmanBits = 2,
  HuffmanEntries = (1 << HuffmanBits)
};

typedef struct
{
  const USHORT (*CodeBook)[HuffmanEntries];
  UCHAR Dimension;
  UCHAR numBits;
  UCHAR Offset;
} CodeBookDescription;

extern const CodeBookDescription AACcodeBookDescriptionTable[13];
extern const CodeBookDescription AACcodeBookDescriptionSCL;


extern const STATEFUNC  aStateConstant2State[];

extern const SCHAR      aCodebook2StartInt[];

extern const UCHAR      aMinOfCbPair[];
extern const UCHAR      aMaxOfCbPair[];

extern const UCHAR      aMaxCwLen[];
extern const UCHAR      aDimCb[];
extern const UCHAR      aDimCbShift[];
extern const UCHAR      aSignCb[];
extern const UCHAR      aCbPriority[];

extern const UINT      *aHuffTable[];
extern const SCHAR     *aQuantTable[];

extern const USHORT     aLargestAbsoluteValue[];

extern const UINT   aHuffTreeRvlcEscape[];
extern const UINT   aHuffTreeRvlCodewds[];


extern const UCHAR tns_max_bands_tbl[13][2];

extern const UCHAR tns_max_bands_tbl_480[5];
extern const UCHAR tns_max_bands_tbl_512[5];

#define FIXP_TCC FIXP_DBL

extern const FIXP_TCC FDKaacDec_tnsCoeff3[8];
extern const FIXP_TCC FDKaacDec_tnsCoeff4[16];

extern const USHORT randomSign[AAC_NF_NO_RANDOM_VAL/16];

extern const FIXP_DBL pow2_div24minus1[47];
extern const int offsetTab[2][16];

/* Channel mapping indices for time domain I/O. First dimension is channel count-1. */
extern const UCHAR channelMappingTablePassthrough[8][8];
extern const UCHAR channelMappingTableWAV[8][8];

/* Lookup tables for elements in ER bitstream */
extern const MP4_ELEMENT_ID elementsTab[8][7];

#endif /* #ifndef AAC_ROM_H */
