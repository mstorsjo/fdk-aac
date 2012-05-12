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
  \file   qmf.h
  \brief  Complex qmf analysis/synthesis  $Revision: 37444 $
  \author Markus Werner

*/
#ifndef __QMF_H
#define __QMF_H



#include "common_fix.h"
#include "FDK_tools_rom.h"
#include "dct.h"

/*
 * Filter coefficient type definition
 */
#ifdef QMF_DATA_16BIT
#define FIXP_QMF FIXP_SGL
#define FX_DBL2FX_QMF FX_DBL2FX_SGL
#define FX_QMF2FX_DBL FX_SGL2FX_DBL
#define QFRACT_BITS FRACT_BITS
#else
#define FIXP_QMF FIXP_DBL
#define FX_DBL2FX_QMF
#define FX_QMF2FX_DBL
#define QFRACT_BITS DFRACT_BITS
#endif

/* ARM neon optimized QMF analysis filter requires 32 bit input.
   Implemented for RVCT only, currently disabled. See src/arm/qmf_arm.cpp:45 */
#define FIXP_QAS FIXP_PCM
#define QAS_BITS SAMPLE_BITS

#ifdef QMFSYN_STATES_16BIT
#define FIXP_QSS FIXP_SGL
#define QSS_BITS FRACT_BITS
#else
#define FIXP_QSS FIXP_DBL
#define QSS_BITS DFRACT_BITS
#endif

/* Flags for QMF intialization */
/* Low Power mode flag */
#define QMF_FLAG_LP           1
/* Filter is not symetric. This flag is set internally in the QMF initialization as required. */
#define QMF_FLAG_NONSYMMETRIC 2
/* Complex Low Delay Filter Bank (or std symmetric filter bank) */
#define QMF_FLAG_CLDFB        4
/* Flag indicating that the states should be kept. */
#define QMF_FLAG_KEEP_STATES  8
/* Complex Low Delay Filter Bank used in MPEG Surround Encoder */
#define QMF_FLAG_MPSLDFB     16
/* Complex Low Delay Filter Bank used in MPEG Surround Encoder allows a optimized calculation of the modulation in qmfForwardModulationHQ() */
#define QMF_FLAG_MPSLDFB_OPTIMIZE_MODULATION  32


typedef struct
{
  int lb_scale;        /*!< Scale of low band area                   */
  int ov_lb_scale;     /*!< Scale of adjusted overlap low band area  */
  int hb_scale;        /*!< Scale of high band area                  */
  int ov_hb_scale;     /*!< Scale of adjusted overlap high band area */
} QMF_SCALE_FACTOR;

struct QMF_FILTER_BANK
{
  const FIXP_PFT *p_filter;     /*!< Pointer to filter coefficients */

  void *FilterStates;           /*!< Pointer to buffer of filter states
                                     FIXP_PCM in analyse and
                                     FIXP_DBL in synthesis filter */
  int FilterSize;               /*!< Size of prototype filter. */
  const FIXP_QTW *t_cos;        /*!< Modulation tables. */
  const FIXP_QTW *t_sin;
  int filterScale;              /*!< filter scale */

  int no_channels;              /*!< Total number of channels (subbands) */
  int no_col;                   /*!< Number of time slots       */
  int lsb;                      /*!< Top of low subbands */
  int usb;                      /*!< Top of high subbands */

  int outScalefactor;           /*!< Scale factor of output data (syn only) */
  FIXP_DBL outGain;             /*!< Gain output data (syn only) (init with 0x80000000 to ignore) */

  UINT flags;                   /*!< flags */
  UCHAR p_stride;               /*!< Stride Factor of polyphase filters */

};

typedef struct QMF_FILTER_BANK *HANDLE_QMF_FILTER_BANK;

void
qmfAnalysisFiltering( HANDLE_QMF_FILTER_BANK anaQmf,  /*!< Handle of Qmf Analysis Bank   */
                      FIXP_QMF **qmfReal,             /*!< Pointer to real subband slots */
                      FIXP_QMF **qmfImag,             /*!< Pointer to imag subband slots */
                      QMF_SCALE_FACTOR *scaleFactor,  /*!< Scale factors of QMF data     */
                      const INT_PCM *timeIn,          /*!< Time signal */
                      const int  stride,              /*!< Stride factor of audio data   */
                      FIXP_QMF  *pWorkBuffer          /*!< pointer to temporal working buffer */
                      );

void
qmfSynthesisFiltering( HANDLE_QMF_FILTER_BANK synQmf,       /*!< Handle of Qmf Synthesis Bank  */
                       FIXP_QMF  **QmfBufferReal,           /*!< Pointer to real subband slots */
                       FIXP_QMF  **QmfBufferImag,           /*!< Pointer to imag subband slots */
                       const QMF_SCALE_FACTOR *scaleFactor, /*!< Scale factors of QMF data     */
                       const int   ov_len,                  /*!< Length of band overlap        */
                       INT_PCM    *timeOut,                 /*!< Time signal */
                       const int   stride,                  /*!< Stride factor of audio data   */
                       FIXP_QMF   *pWorkBuffer              /*!< pointer to temporal working buffer */
                       );

int
qmfInitAnalysisFilterBank( HANDLE_QMF_FILTER_BANK h_Qmf, /*!< QMF Handle */
                           FIXP_QAS *pFilterStates,      /*!< Pointer to filter state buffer */
                           int noCols,                   /*!< Number of time slots  */
                           int lsb,                      /*!< Number of lower bands */
                           int usb,                      /*!< Number of upper bands */
                           int no_channels,              /*!< Number of critically sampled bands */
                           int flags);                   /*!< Flags */

void
qmfAnalysisFilteringSlot( HANDLE_QMF_FILTER_BANK anaQmf,  /*!< Handle of Qmf Synthesis Bank  */
                          FIXP_QMF      *qmfReal,         /*!< Low and High band, real */
                          FIXP_QMF      *qmfImag,         /*!< Low and High band, imag */
                          const INT_PCM *timeIn,          /*!< Pointer to input */
                          const int      stride,          /*!< stride factor of input */
                          FIXP_QMF      *pWorkBuffer      /*!< pointer to temporal working buffer */
                         );

int
qmfInitSynthesisFilterBank( HANDLE_QMF_FILTER_BANK h_Qmf, /*!< QMF Handle */
                            FIXP_QSS *pFilterStates,      /*!< Pointer to filter state buffer */
                            int noCols,                   /*!< Number of time slots  */
                            int lsb,                      /*!< Number of lower bands */
                            int usb,                      /*!< Number of upper bands */
                            int no_channels,              /*!< Number of critically sampled bands */
                            int flags);                   /*!< Flags */

void qmfSynthesisFilteringSlot( HANDLE_QMF_FILTER_BANK  synQmf,
                                const FIXP_QMF *realSlot,
                                const FIXP_QMF *imagSlot,
                                const int       scaleFactorLowBand,
                                const int       scaleFactorHighBand,
                                INT_PCM        *timeOut,
                                const int       stride,
                                FIXP_QMF       *pWorkBuffer);

void
qmfChangeOutScalefactor (HANDLE_QMF_FILTER_BANK synQmf,     /*!< Handle of Qmf Synthesis Bank */
                         int outScalefactor                 /*!< New scaling factor for output data */
                        );

void
qmfChangeOutGain (HANDLE_QMF_FILTER_BANK synQmf,     /*!< Handle of Qmf Synthesis Bank */
                  FIXP_DBL outputGain                /*!< New gain for output data */
                 );



#endif /* __QMF_H */
