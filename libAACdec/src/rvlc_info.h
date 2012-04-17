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

****************************************************************************/
/*!
  \file
  \brief  Defines structures for RVLC
  \author Robert Weidner
*/
#ifndef RVLC_INFO_H
#define RVLC_INFO_H



#define FWD                           0     /* bitstream decoding direction forward (RVL coded part) */
#define BWD                           1     /* bitstream decoding direction backward (RVL coded part) */

#define MAX_RVL                       7     /* positive RVLC escape */
#define MIN_RVL                      -7     /* negative RVLC escape */
#define MAX_ALLOWED_DPCM_INDEX       14     /* the maximum allowed index of a decoded dpcm value (offset 'TABLE_OFFSET' incl --> must be subtracted) */
#define TABLE_OFFSET                  7     /* dpcm offset of valid output values of rvl table decoding, the rvl table ouly returns positive values, therefore the offset */
#define MAX_LEN_RVLC_CODE_WORD        9     /* max length of a RVL codeword in bits */
#define MAX_LEN_RVLC_ESCAPE_WORD     20     /* max length of huffman coded RVLC escape word in bits */

#define DPCM_NOISE_NRG_BITS           9
#define SF_OFFSET                   100     /* offset for correcting scf value */

#define CONCEAL_MAX_INIT           1311     /* arbitrary value */
#define CONCEAL_MIN_INIT          -1311     /* arbitrary value */

#define RVLC_MAX_SFB              ((8) * (16))

/* sideinfo of RVLC */
typedef struct
{
  /* ------- ESC 1 Data: --------- */     /* order of RVLC-bitstream components in bitstream (RVLC-initialization), every component appears only once in bitstream */
  INT        sf_concealment;              /* 1          */
  INT        rev_global_gain;             /* 2          */
  SHORT      length_of_rvlc_sf;           /* 3          */  /* original value, gets modified (subtract 9) in case of noise (PNS); is kept for later use */
  INT        dpcm_noise_nrg;              /* 4 optional */
  INT        sf_escapes_present;          /* 5          */
  SHORT      length_of_rvlc_escapes;      /* 6 optional */
  INT        dpcm_noise_last_position;    /* 7 optional */

  INT        dpcm_is_last_position;

  SHORT      length_of_rvlc_sf_fwd;       /* length_of_rvlc_sf used for forward decoding */
  SHORT      length_of_rvlc_sf_bwd;       /* length_of_rvlc_sf used for backward decoding */

  /* for RVL-Codeword decoder to distinguish between fwd and bwd decoding */
  SHORT     *pRvlBitCnt_RVL;
  USHORT    *pBitstrIndxRvl_RVL;

  UCHAR      numWindowGroups;
  UCHAR      maxSfbTransmitted;
  UCHAR      first_noise_group;
  UCHAR      first_noise_band;
  UCHAR      direction;

  /* bitstream indices */
  USHORT     bitstreamIndexRvlFwd;        /* base address of RVL-coded-scalefactor data (ESC 2) for forward  decoding */
  USHORT     bitstreamIndexRvlBwd;        /* base address of RVL-coded-scalefactor data (ESC 2) for backward decoding */
  USHORT     bitstreamIndexEsc;           /* base address where RVLC-escapes start (ESC 2) */

  /* decoding trees */
  const UINT *pHuffTreeRvlCodewds;
  const UINT *pHuffTreeRvlcEscape;

  /* escape counters */
  UCHAR      numDecodedEscapeWordsFwd;    /* when decoding RVL-codes forward  */
  UCHAR      numDecodedEscapeWordsBwd;    /* when decoding RVL-codes backward */
  UCHAR      numDecodedEscapeWordsEsc;    /* when decoding the escape-Words */

  SCHAR      noise_used;
  SCHAR      intensity_used;
  SCHAR      sf_used;

  SHORT      firstScf;
  SHORT      lastScf;
  SHORT      firstNrg;
  SHORT      lastNrg;
  SHORT      firstIs;
  SHORT      lastIs;

  /* ------ RVLC error detection ------ */
  UINT       errorLogRvlc;                /* store RVLC errors  */
  SHORT      conceal_min;                 /* is set at backward decoding  */
  SHORT      conceal_max;                 /* is set at forward  decoding  */
  SHORT      conceal_min_esc;             /* is set at backward decoding  */
  SHORT      conceal_max_esc;             /* is set at forward  decoding  */
} CErRvlcInfo;

typedef CErRvlcInfo RVLC_INFO; /* temp */



#endif /* RVLC_INFO_H */
