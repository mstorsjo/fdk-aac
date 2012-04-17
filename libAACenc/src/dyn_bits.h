/******************************** MPEG Audio Encoder **************************

                     (C) Copyright Fraunhofer IIS (1999)
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
   Initial author:       M.Werner
   contents/description: Noiseless coder module

******************************************************************************/
#ifndef __DYN_BITS_H
#define __DYN_BITS_H

#include "common_fix.h"

#include "psy_const.h"
#include "aacenc_tns.h"

#define MAX_SECTIONS          MAX_GROUPED_SFB
#define SECT_ESC_VAL_LONG    31
#define SECT_ESC_VAL_SHORT    7
#define CODE_BOOK_BITS        4
#define SECT_BITS_LONG        5
#define SECT_BITS_SHORT       3
#define PNS_PCM_BITS          9

typedef struct
{
  INT codeBook;
  INT sfbStart;
  INT sfbCnt;
  INT sectionBits;              /* huff + si ! */
} SECTION_INFO;


typedef struct
{
  INT blockType;
  INT noOfGroups;
  INT sfbCnt;
  INT maxSfbPerGroup;
  INT sfbPerGroup;
  INT noOfSections;
  SECTION_INFO huffsection[MAX_SECTIONS];
  INT sideInfoBits;             /* sectioning bits       */
  INT huffmanBits;              /* huffman    coded bits */
  INT scalefacBits;             /* scalefac   coded bits */
  INT noiseNrgBits;             /* noiseEnergy coded bits */
  INT firstScf;                 /* first scf to be coded */
} SECTION_DATA;


struct BITCNTR_STATE
{
  INT *bitLookUp;
  INT *mergeGainLookUp;
};


INT  FDKaacEnc_BCNew(BITCNTR_STATE **phBC
          ,UCHAR* dynamic_RAM
          );

void FDKaacEnc_BCClose(BITCNTR_STATE **phBC);

#if defined(PNS_PRECOUNT_ENABLE)
INT  noisePreCount(const INT *noiseNrg, INT maxSfb);
#endif

INT FDKaacEnc_dynBitCount(
        BITCNTR_STATE* const         hBC,
        const SHORT* const           quantSpectrum,
        const UINT* const            maxValueInSfb,
        const INT* const             scalefac,
        const INT                    blockType,
        const INT                    sfbCnt,
        const INT                    maxSfbPerGroup,
        const INT                    sfbPerGroup,
        const INT* const             sfbOffset,
        SECTION_DATA* const RESTRICT sectionData,
        const INT* const             noiseNrg,
        const INT* const             isBook,
        const INT* const             isScale,
        const UINT                   syntaxFlags
        );

#endif
