/***************************  Fraunhofer IIS FDK Tools  **********************

                        (C) Copyright Fraunhofer IIS (2011)
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
   Author(s): Markus Lohwasser
   Description: FDK Tools Hybrid Filterbank

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef __FDK_HYBRID_H
#define __FDK_HYBRID_H



#include "common_fix.h"

/*--------------- enums -------------------------------*/

/**
 * Hybrid Filterband modes.
 */
typedef enum {
    THREE_TO_TEN,
    THREE_TO_TWELVE,
    THREE_TO_SIXTEEN

} FDK_HYBRID_MODE;


/*--------------- structure definitions ---------------*/
typedef struct FDK_HYBRID_SETUP *HANDLE_FDK_HYBRID_SETUP;

typedef struct
{
  FIXP_DBL             *bufferLFReal[3];     /*!< LF real filter states. */
  FIXP_DBL             *bufferLFImag[3];     /*!< LF imag filter states. */
  FIXP_DBL             *bufferHFReal[13];    /*!< HF real delay lines. */
  FIXP_DBL             *bufferHFImag[13];    /*!< HF imag delay lines. */

  INT                   bufferLFpos;         /*!< Position to write incoming data into ringbuffer. */
  INT                   bufferHFpos;         /*!< Delay line positioning. */
  INT                   nrBands;             /*!< Number of QMF bands. */
  INT                   cplxBands;           /*!< Number of complex QMF bands.*/
  UCHAR                 hfMode;              /*!< Flag signalizes treatment of HF bands. */

  FIXP_DBL             *pLFmemory;           /*!< Pointer to LF states buffer. */
  FIXP_DBL             *pHFmemory;           /*!< Pointer to HF states buffer. */

  UINT                  LFmemorySize;        /*!< Size of LF states buffer. */
  UINT                  HFmemorySize;        /*!< Size of HF states buffer. */

  HANDLE_FDK_HYBRID_SETUP pSetup;            /*!< Pointer to filter setup. */

} FDK_ANA_HYB_FILTER;


typedef struct
{
  INT                   nrBands;             /*!< Number of QMF bands. */
  INT                   cplxBands;           /*!< Number of complex QMF bands.*/

  HANDLE_FDK_HYBRID_SETUP pSetup;            /*!< Pointer to filter setup. */

} FDK_SYN_HYB_FILTER;

typedef FDK_ANA_HYB_FILTER *HANDLE_FDK_ANA_HYB_FILTER;
typedef FDK_SYN_HYB_FILTER *HANDLE_FDK_SYN_HYB_FILTER;


/**
 * \brief  Create one instance of Hybrid Analyis Filterbank.
 *
 * \param hAnalysisHybFilter  Pointer to an outlying allocated Hybrid Analysis Filterbank structure.
 * \param pLFmemory           Pointer to outlying buffer used LF filtering.
 * \param LFmemorySize        Size of pLFmemory in bytes.
 * \param pHFmemory           Pointer to outlying buffer used HF delay line.
 * \param HFmemorySize        Size of pLFmemory in bytes.
 *
 * \return  0 on success.
 */
INT FDKhybridAnalysisOpen(
        HANDLE_FDK_ANA_HYB_FILTER  hAnalysisHybFilter,
        FIXP_DBL *const            pLFmemory,
        const UINT                 LFmemorySize,
        FIXP_DBL *const            pHFmemory,
        const UINT                 HFmemorySize
        );


/**
 * \brief  Initialize and configure Hybrdid Analysis Filterbank instance.
 *
 * \param hAnalysisHybFilter  A Hybrid Analysis Filterbank handle.
 * \param mode                Select hybrid filter configuration.
 * \param qmfBands            Number of qmf bands to be processed.
 * \param cplxBands           Number of complex qmf bands to be processed.
 * \param initStatesFlag      Indicates whether the states buffer has to be cleared.
 *
 * \return  0 on success.
 */
INT FDKhybridAnalysisInit(
        HANDLE_FDK_ANA_HYB_FILTER  hAnalysisHybFilter,
        const FDK_HYBRID_MODE      mode,
        const INT                  qmfBands,
        const INT                  cplxBands,
        const INT                  initStatesFlag
        );


/**
 * \brief  Adjust Hybrdid Analysis Filterbank states.
 *
 * \param hAnalysisHybFilter  A Hybrid Analysis Filterbank handle.
 * \param scalingValue        Scaling value to be applied on filter states.
 *
 * \return  0 on success.
 */
INT FDKhybridAnalysisScaleStates(
        HANDLE_FDK_ANA_HYB_FILTER  hAnalysisHybFilter,
        const INT                  scalingValue
        );


/**
 * \brief  Apply Hybrid Analysis Filterbank on Qmf input data.
 *
 * \param hAnalysisHybFilter  A Hybrid Analysis Filterbank handle.
 * \param pQmfReal            Qmf input data.
 * \param pQmfImag            Qmf input data.
 * \param pHybridReal         Hybrid output data.
 * \param pHybridImag         Hybrid output data.
 *
 * \return  0 on success.
 */
INT FDKhybridAnalysisApply(
        HANDLE_FDK_ANA_HYB_FILTER  hAnalysisHybFilter,
        const FIXP_DBL *const      pQmfReal,
        const FIXP_DBL *const      pQmfImag,
        FIXP_DBL *const            pHybridReal,
        FIXP_DBL *const            pHybridImag
        );


/**
 * \brief  Close a Hybrid Analysis Filterbank instance.
 *
 * \param hAnalysisHybFilter  Pointer to a Hybrid Analysis Filterbank instance.
 *
 * \return  0 on success.
 */
INT FDKhybridAnalysisClose(
        HANDLE_FDK_ANA_HYB_FILTER  hAnalysisHybFilter
        );

/**
 * \brief  Initialize and configure Hybrdid Synthesis Filterbank instance.
 *
 * \param hSynthesisHybFilter A Hybrid Synthesis Filterbank handle.
 * \param mode                Select hybrid filter configuration.
 * \param qmfBands            Number of qmf bands to be processed.
 * \param cplxBands           Number of complex qmf bands to be processed.
 *
 * \return  0 on success.
 */
INT FDKhybridSynthesisInit(
        HANDLE_FDK_SYN_HYB_FILTER  hSynthesisHybFilter,
        const FDK_HYBRID_MODE      mode,
        const INT                  qmfBands,
        const INT                  cplxBands
        );

/**
 * \brief  Apply Hybrid Analysis Filterbank on Hybrid data.
 *
 * \param hSynthesisHybFilter  A Hybrid Analysis Filterbandk handle.
 * \param pHybridReal          Hybrid input data.
 * \param pHybridImag          Hybrid input data.
 * \param pQmfReal             Qmf output data.
 * \param pQmfImag             Qmf output data.
 *
 * \return  0 on success.
 */
INT FDKhybridSynthesisApply(
        HANDLE_FDK_SYN_HYB_FILTER  hSynthesisHybFilter,
        const FIXP_DBL *const      pHybridReal,
        const FIXP_DBL *const      pHybridImag,
        FIXP_DBL *const            pQmfReal,
        FIXP_DBL *const            pQmfImag
        );


#endif /* __FDK_HYBRID_H */
