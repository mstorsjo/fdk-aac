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
   Description: perceptual noise substitution tool

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "aacdec_pns.h"


#include "aac_ram.h"
#include "aac_rom.h"
#include "channelinfo.h"
#include "block.h"
#include "FDK_bitstream.h"

#include "genericStds.h"


#define NOISE_OFFSET 90           /* cf. ISO 14496-3 p. 175 */

/*!
  \brief Reset InterChannel and PNS data

  The function resets the InterChannel and PNS data
*/
void CPns_ResetData(
    CPnsData *pPnsData,
    CPnsInterChannelData *pPnsInterChannelData
    )
{
  /* Assign pointer always, since pPnsData is not persistent data */
  pPnsData->pPnsInterChannelData = pPnsInterChannelData;
  pPnsData->PnsActive = 0;
  pPnsData->CurrentEnergy = 0;

  FDKmemclear(pPnsData->pnsUsed,(8*16)*sizeof(UCHAR));
  FDKmemclear(pPnsInterChannelData->correlated,(8*16)*sizeof(UCHAR));
}

/*!
  \brief Initialize PNS data

  The function initializes the PNS data
*/
void CPns_InitPns(
    CPnsData *pPnsData,
    CPnsInterChannelData *pPnsInterChannelData,
    INT* currentSeed, INT* randomSeed)
{
  /* save pointer to inter channel data */
  pPnsData->pPnsInterChannelData = pPnsInterChannelData;

  /* use pointer because seed has to be
     same, left and right channel ! */
  pPnsData->currentSeed = currentSeed;
  pPnsData->randomSeed  = randomSeed;
}

/*!
  \brief Indicates if PNS is used

  The function returns a value indicating whether PNS is used or not
  acordding to the noise energy

  \return  PNS used
*/
int CPns_IsPnsUsed (const CPnsData *pPnsData,
                    const int group,
                    const int band)
{
  unsigned pns_band = group*16+band;

  return pPnsData->pnsUsed[pns_band] & (UCHAR)1;
}

/*!
  \brief Set correlation

  The function activates the noise correlation between the channel pair
*/
void CPns_SetCorrelation(CPnsData *pPnsData,
                         const int group,
                         const int band,
                         const int outofphase)
{
  CPnsInterChannelData *pInterChannelData = pPnsData->pPnsInterChannelData;
  unsigned pns_band = group*16+band;

  pInterChannelData->correlated[pns_band] = (outofphase) ? 3 : 1;
}

/*!
  \brief Indicates if correlation is used

  The function indicates if the noise correlation between the channel pair
  is activated

  \return  PNS is correlated
*/
static
int CPns_IsCorrelated(const CPnsData *pPnsData,
                      const int group,
                      const int band)
{
  CPnsInterChannelData *pInterChannelData = pPnsData->pPnsInterChannelData;
  unsigned pns_band = group*16+band;

  return (pInterChannelData->correlated[pns_band] & 0x01) ? 1 : 0;
}

/*!
  \brief Indicates if correlated out of phase mode is used.

  The function indicates if the noise correlation between the channel pair
  is activated in out-of-phase mode.

  \return  PNS is out-of-phase
*/
static
int CPns_IsOutOfPhase(const CPnsData *pPnsData,
                      const int group,
                      const int band)
{
  CPnsInterChannelData *pInterChannelData = pPnsData->pPnsInterChannelData;
  unsigned pns_band = group*16+band;

  return (pInterChannelData->correlated[pns_band] & 0x02) ? 1 : 0;
}

/*!
  \brief Read PNS information

  The function reads the PNS information from the bitstream
*/
void CPns_Read (CPnsData *pPnsData,
                HANDLE_FDK_BITSTREAM bs,
                const CodeBookDescription *hcb,
                SHORT *pScaleFactor,
                UCHAR global_gain,
                int band,
                int group /* = 0 */)
{
  int delta ;
  UINT pns_band = group*16+band;

  if (pPnsData->PnsActive) {
    /* Next PNS band case */
    delta = CBlock_DecodeHuffmanWord (bs, hcb) - 60;
  } else {
    /* First PNS band case */
    int noiseStartValue = FDKreadBits(bs,9);

    delta = noiseStartValue - 256 ;
    pPnsData->PnsActive = 1;
    pPnsData->CurrentEnergy = global_gain - NOISE_OFFSET;
  }

  pPnsData->CurrentEnergy += delta ;
  pScaleFactor[pns_band] = pPnsData->CurrentEnergy;

  pPnsData->pnsUsed[pns_band] = 1;
}


/**
 * \brief Generate a vector of noise of given length. The noise values are
 *        scaled in order to yield a noise energy of 1.0
 * \param spec pointer to were the noise values will be written to.
 * \param size amount of noise values to be generated.
 * \param pRandomState pointer to the state of the random generator being used.
 * \return exponent of generated noise vector.
 */
static int GenerateRandomVector (FIXP_DBL *RESTRICT spec,
                                  int size,
                                  int *pRandomState)
{
  int i, invNrg_e = 0, nrg_e = 0;
  FIXP_DBL invNrg_m, nrg_m = FL2FXCONST_DBL(0.0f) ;
  FIXP_DBL *RESTRICT ptr = spec;
  int randomState = *pRandomState;

#define GEN_NOISE_NRG_SCALE 7

  /* Generate noise and calculate energy. */
  for (i=0; i<size; i++)
  {
    randomState = (1664525L * randomState) + 1013904223L; // Numerical Recipes
    nrg_m = fPow2AddDiv2(nrg_m, (FIXP_DBL)randomState>>GEN_NOISE_NRG_SCALE);
    *ptr++ = (FIXP_DBL)randomState;
  }
  nrg_e = GEN_NOISE_NRG_SCALE*2 + 1;

  /* weight noise with = 1 / sqrt_nrg; */
  invNrg_m = invSqrtNorm2(nrg_m<<1, &invNrg_e);
  invNrg_e += -((nrg_e-1)>>1);

  for (i=size; i--; )
  {
    spec[i] = fMult(spec[i], invNrg_m);
  }

  /* Store random state */
  *pRandomState = randomState;

  return invNrg_e;
}

static void ScaleBand (FIXP_DBL *RESTRICT spec, int size, int scaleFactor, int specScale, int noise_e, int out_of_phase)
{
  int i, shift, sfExponent;
  FIXP_DBL sfMatissa;

  /* Get gain from scale factor value = 2^(scaleFactor * 0.25) */
  sfMatissa = MantissaTable[scaleFactor & 0x03][0];
  /* sfExponent = (scaleFactor >> 2) + ExponentTable[scaleFactor & 0x03][0]; */
  /* Note:  ExponentTable[scaleFactor & 0x03][0] is always 1. */
  sfExponent = (scaleFactor >> 2) + 1;

  if (out_of_phase != 0) {
    sfMatissa = -sfMatissa;
  }

  /* +1 because of fMultDiv2 below. */
  shift = sfExponent - specScale + 1 + noise_e;

  /* Apply gain to noise values */
  if (shift>=0) {
    shift = fixMin( shift, DFRACT_BITS-1 );
    for (i = size ; i-- != 0; ) {
      spec [i] = fMultDiv2 (spec [i], sfMatissa) << shift;
    }
  } else {
    shift = fixMin( -shift, DFRACT_BITS-1 );
    for (i = size ; i-- != 0; ) {
      spec [i] = fMultDiv2 (spec [i], sfMatissa) >> shift;
    }
  }
}


/*!
  \brief Apply PNS

  The function applies PNS (i.e. it generates noise) on the bands
  flagged as noisy bands

*/
void CPns_Apply (const CPnsData *pPnsData,
                 const CIcsInfo *pIcsInfo,
                 SPECTRAL_PTR pSpectrum,
                 const SHORT    *pSpecScale,
                 const SHORT    *pScaleFactor,
                 const SamplingRateInfo *pSamplingRateInfo,
                 const INT granuleLength,
                 const int channel)
{
  if (pPnsData->PnsActive) {
    const short *BandOffsets = GetScaleFactorBandOffsets(pIcsInfo, pSamplingRateInfo);

    int ScaleFactorBandsTransmitted = GetScaleFactorBandsTransmitted(pIcsInfo);

    for (int window = 0, group = 0; group < GetWindowGroups(pIcsInfo); group++) {
      for (int groupwin = 0; groupwin < GetWindowGroupLength(pIcsInfo, group); groupwin++, window++) {
        FIXP_DBL *spectrum = SPEC(pSpectrum, window, granuleLength);

        for (int band = 0 ; band < ScaleFactorBandsTransmitted; band++) {
          if (CPns_IsPnsUsed (pPnsData, group, band)) {
            UINT pns_band = group*16+band;

            int bandWidth = BandOffsets [band + 1] - BandOffsets [band] ;
            int noise_e;

            FDK_ASSERT(bandWidth >= 0);

            if (channel > 0 && CPns_IsCorrelated(pPnsData, group, band))
            {
              noise_e = GenerateRandomVector (spectrum + BandOffsets [band], bandWidth,
                                    &pPnsData->randomSeed [pns_band]) ;
            }
            else
            {
              pPnsData->randomSeed [pns_band] = *pPnsData->currentSeed ;

              noise_e = GenerateRandomVector (spectrum + BandOffsets [band], bandWidth,
                                    pPnsData->currentSeed) ;
            }

            int outOfPhase  = CPns_IsOutOfPhase (pPnsData, group, band);

            ScaleBand (spectrum + BandOffsets [band], bandWidth,
                       pScaleFactor[pns_band],
                       pSpecScale[window], noise_e, outOfPhase) ;
          }
        }
      }
    }
  }
}
