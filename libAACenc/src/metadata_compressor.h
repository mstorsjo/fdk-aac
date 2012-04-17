/**********************  Fraunhofer IIS FDK AAC Encoder lib  ******************

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
   Author(s): M. Neusinger
   Description: Compressor for AAC Metadata Generator

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef _METADATA_COMPRESSOR_H
#define _METADATA_COMPRESSOR_H


#include "FDK_audio.h"
#include "common_fix.h"

#include "aacenc.h"


/**
 *  DRC compression profiles.
 */
typedef enum DRC_PROFILE {
  DRC_NONE          = 0,
  DRC_FILMSTANDARD  = 1,
  DRC_FILMLIGHT     = 2,
  DRC_MUSICSTANDARD = 3,
  DRC_MUSICLIGHT    = 4,
  DRC_SPEECH        = 5,
  DRC_DELAY_TEST    = 6

} DRC_PROFILE;


/**
 *  DRC Compressor handle.
 */
typedef struct DRC_COMP  DRC_COMP, *HDRC_COMP;

/**
 * \brief  Open a DRC Compressor instance.
 *
 * Allocate memory for a compressor instance.
 *
 * \param phDrcComp             A pointer to a compressor handle. Initialized on return.
 *
 * \return
 *          - 0, on succes.
 *          - unequal 0, on failure.
 */
INT FDK_DRC_Generator_Open(
        HDRC_COMP                      *phDrcComp
        );


/**
 * \brief  Close the DRC Compressor instance.
 *
 * Deallocate instance and free whole memory.
 *
 * \param phDrcComp             Pointer to the compressor handle to be deallocated.
 *
 * \return
 *          - 0, on succes.
 *          - unequal 0, on failure.
 */
INT FDK_DRC_Generator_Close(
        HDRC_COMP                      *phDrcComp
        );

/**
 * \brief  Configure DRC Compressor.
 *
 * \param drcComp               Compressor handle.
 * \param profileLine           DRC profile for line mode.
 * \param profileRF             DRC profile for RF mode.
 * \param blockLength           Length of processing block in samples per channel.
 * \param sampleRate            Sampling rate in Hz.
 * \param channelMode           Channel configuration.
 * \param channelOrder          Channel order, MPEG or WAV.
 * \param useWeighting          Use weighting filter for loudness calculation
 *
 * \return
 *          - 0, on success,
 *          - unequal 0, on failure
 */
INT FDK_DRC_Generator_Initialize(
        HDRC_COMP                       drcComp,
        const DRC_PROFILE               profileLine,
        const DRC_PROFILE               profileRF,
        const INT                       blockLength,
        const UINT                      sampleRate,
        const CHANNEL_MODE              channelMode,
        const CHANNEL_ORDER             channelOrder,
        const UCHAR                     useWeighting
        );

/**
 * \brief  Calculate DRC Compressor Gain.
 *
 * \param drcComp               Compressor handle.
 * \param inSamples             Pointer to interleaved input audio samples.
 * \param dialnorm              Dialog Level in dB (typically -31...-1).
 * \param drc_TargetRefLevel
 * \param comp_TargetRefLevel
 * \param clev                  Downmix center mix factor (typically 0.707, 0.595 or 0.5)
 * \param slev                  Downmix surround mix factor (typically 0.707, 0.5, or 0)
 * \param dynrng                Pointer to variable receiving line mode DRC gain in dB
 * \param compr                 Pointer to variable receiving RF mode DRC gain in dB
 *
 * \return
 *          - 0, on success,
 *          - unequal 0, on failure
 */
INT FDK_DRC_Generator_Calc(
        HDRC_COMP                       drcComp,
        const INT_PCM * const           inSamples,
        const INT                       dialnorm,
        const INT                       drc_TargetRefLevel,
        const INT                       comp_TargetRefLevel,
        FIXP_DBL                        clev,
        FIXP_DBL                        slev,
        INT * const                     dynrng,
        INT * const                     compr
        );


/**
 * \brief  Configure DRC Compressor Profile.
 *
 * \param drcComp               Compressor handle.
 * \param profileLine           DRC profile for line mode.
 * \param profileRF             DRC profile for RF mode.
 *
 * \return
 *          - 0, on success,
 *          - unequal 0, on failure
 */
INT FDK_DRC_Generator_setDrcProfile(
        HDRC_COMP                       drcComp,
        const DRC_PROFILE               profileLine,
        const DRC_PROFILE               profileRF
        );


/**
 * \brief  Get DRC profile for line mode.
 *
 * \param drcComp               Compressor handle.
 *
 * \return  Current Profile.
 */
DRC_PROFILE FDK_DRC_Generator_getDrcProfile(
        const HDRC_COMP                 drcComp
        );


/**
 * \brief  Get DRC profile for RF mode.
 *
 * \param drcComp               Compressor handle.
 *
 * \return  Current Profile.
 */
DRC_PROFILE FDK_DRC_Generator_getCompProfile(
        const HDRC_COMP                 drcComp
        );


#endif /* _METADATA_COMPRESSOR_H */

