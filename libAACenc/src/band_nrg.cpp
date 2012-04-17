/*****************************  MPEG-4 AAC Encoder  **************************

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


   $Id$
   Initial author:       M. Werner
   contents/description: Band/Line energy calculations

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "band_nrg.h"


/*****************************************************************************
  functionname: FDKaacEnc_CalcSfbMaxScaleSpec
  description:
  input:
  output:
*****************************************************************************/
void
FDKaacEnc_CalcSfbMaxScaleSpec(const FIXP_DBL *RESTRICT mdctSpectrum,
                              const INT      *RESTRICT bandOffset,
                              INT            *RESTRICT sfbMaxScaleSpec,
                              const INT       numBands)
{
  INT i,j;
  FIXP_DBL maxSpc, tmp;

  for(i=0; i<numBands; i++) {
    maxSpc = (FIXP_DBL)0;
    for (j=bandOffset[i]; j<bandOffset[i+1]; j++) {
      tmp = fixp_abs(mdctSpectrum[j]);
      maxSpc = fixMax(maxSpc, tmp);
    }
    sfbMaxScaleSpec[i] = (maxSpc==(FIXP_DBL)0) ? (DFRACT_BITS-2) : CntLeadingZeros(maxSpc)-1;
    /* CountLeadingBits() is not necessary here since test value is always > 0 */
  }
}

/*****************************************************************************
  functionname: FDKaacEnc_CheckBandEnergyOptim
  description:
  input:
  output:
*****************************************************************************/
FIXP_DBL
FDKaacEnc_CheckBandEnergyOptim(const FIXP_DBL *RESTRICT mdctSpectrum,
                               INT            *RESTRICT sfbMaxScaleSpec,
                               const INT      *RESTRICT bandOffset,
                               const INT       numBands,
                               FIXP_DBL       *RESTRICT bandEnergy,
                               FIXP_DBL       *RESTRICT bandEnergyLdData,
                               INT             minSpecShift)
{
  INT i, j, scale, nr = 0;
  FIXP_DBL maxNrgLd = FL2FXCONST_DBL(-1.0f);
  FIXP_DBL maxNrg = 0;
  FIXP_DBL spec;

  for(i=0; i<numBands; i++) {
    scale = fixMax(0, sfbMaxScaleSpec[i]-4);
    FIXP_DBL tmp = 0;
    for (j=bandOffset[i]; j<bandOffset[i+1]; j++){
       spec = mdctSpectrum[j]<<scale;
       tmp = fPow2AddDiv2(tmp, spec);
    }
    bandEnergy[i] = tmp<<1;

    /* calculate ld of bandNrg, subtract scaling */
    bandEnergyLdData[i] = CalcLdData(bandEnergy[i]);
    if (bandEnergyLdData[i] != FL2FXCONST_DBL(-1.0f)) {
      bandEnergyLdData[i] -= scale*FL2FXCONST_DBL(2.0/64);
    }
    /* find index of maxNrg */
    if (bandEnergyLdData[i] > maxNrgLd) {
        maxNrgLd = bandEnergyLdData[i];
        nr = i;
    }
  }

  /* return unscaled maxNrg*/
  scale = fixMax(0,sfbMaxScaleSpec[nr]-4);
  scale = fixMax(2*(minSpecShift-scale),-(DFRACT_BITS-1));

  maxNrg = scaleValue(bandEnergy[nr], scale);

  return maxNrg;
}

/*****************************************************************************
  functionname: FDKaacEnc_CalcBandEnergyOptimLong
  description:
  input:
  output:
*****************************************************************************/
INT
FDKaacEnc_CalcBandEnergyOptimLong(const FIXP_DBL *RESTRICT mdctSpectrum,
                                  INT            *RESTRICT sfbMaxScaleSpec,
                                  const INT      *RESTRICT bandOffset,
                                  const INT       numBands,
                                  FIXP_DBL       *RESTRICT bandEnergy,
                                  FIXP_DBL       *RESTRICT bandEnergyLdData)
{
  INT i, j, shiftBits = 0;
  FIXP_DBL maxNrgLd = FL2FXCONST_DBL(0.0f);

  FIXP_DBL spec;

  for(i=0; i<numBands; i++) {
     INT leadingBits = sfbMaxScaleSpec[i]-4;            /* max sfbWidth = 96 ; 2^7=128 => 7/2 = 4 (spc*spc) */
     FIXP_DBL tmp = FL2FXCONST_DBL(0.0);
     /* don't use scaleValue() here, it increases workload quite sufficiently... */
     if (leadingBits>=0) {
        for (j=bandOffset[i];j<bandOffset[i+1];j++) {
           spec = mdctSpectrum[j]<<leadingBits;
           tmp = fPow2AddDiv2(tmp, spec);
        }
     } else {
        INT shift = -leadingBits;
        for (j=bandOffset[i];j<bandOffset[i+1];j++){
           spec = mdctSpectrum[j]>>shift;
           tmp = fPow2AddDiv2(tmp, spec);
        }
     }
     bandEnergy[i] = tmp<<1;
  }

  /* calculate ld of bandNrg, subtract scaling */
  LdDataVector(bandEnergy, bandEnergyLdData, numBands);
  for(i=numBands; i--!=0; ) {
      FIXP_DBL scaleDiff = (sfbMaxScaleSpec[i]-4)*FL2FXCONST_DBL(2.0/64);

      bandEnergyLdData[i] = (bandEnergyLdData[i] >= ((FL2FXCONST_DBL(-1.f)>>1) + (scaleDiff>>1)))
                          ? bandEnergyLdData[i]-scaleDiff : FL2FXCONST_DBL(-1.f);
      /* find maxNrgLd */
      maxNrgLd = fixMax(maxNrgLd, bandEnergyLdData[i]);
  }

  if (maxNrgLd<=(FIXP_DBL)0)
  {
     for(i=numBands; i--!=0; )
     {
         INT scale = fixMin((sfbMaxScaleSpec[i]-4)<<1,(DFRACT_BITS-1));
         bandEnergy[i] = scaleValue(bandEnergy[i], -scale);
     }
     return 0;
  }
  else
  {  /* scale down NRGs */
     while (maxNrgLd>FL2FXCONST_DBL(0.0f))
     {
        maxNrgLd -=  FL2FXCONST_DBL(2.0/64);
        shiftBits++;
     }
     for(i=numBands; i--!=0; )
     {
         INT scale = fixMin( ((sfbMaxScaleSpec[i]-4)+shiftBits)<<1, (DFRACT_BITS-1));
         bandEnergyLdData[i] -= shiftBits*FL2FXCONST_DBL(2.0/64);
         bandEnergy[i] = scaleValue(bandEnergy[i], -scale);
     }
     return shiftBits;
  }
}


/*****************************************************************************
  functionname: FDKaacEnc_CalcBandEnergyOptimShort
  description:
  input:
  output:
*****************************************************************************/
void
FDKaacEnc_CalcBandEnergyOptimShort(const FIXP_DBL *RESTRICT mdctSpectrum,
                                   INT            *RESTRICT sfbMaxScaleSpec,
                                   const INT      *RESTRICT bandOffset,
                                   const INT       numBands,
                                   FIXP_DBL       *RESTRICT bandEnergy)
{
  INT i, j;

  for(i=0; i<numBands; i++)
  {
    int leadingBits = fixMax(0,sfbMaxScaleSpec[i]-4);            /* max sfbWidth = 96 ; 2^7=128 => 7/2 = 4 (spc*spc) */
    FIXP_DBL tmp = FL2FXCONST_DBL(0.0);
    for (j=bandOffset[i];j<bandOffset[i+1];j++)
    {
       FIXP_DBL spec = mdctSpectrum[j]<<leadingBits;
       tmp = fPow2AddDiv2(tmp, spec);
    }
    bandEnergy[i] = tmp<<1;
  }

  for(i=0; i<numBands; i++)
  {
      INT scale = 2*fixMax(0,sfbMaxScaleSpec[i]-4);            /* max sfbWidth = 96 ; 2^7=128 => 7/2 = 4 (spc*spc) */
      scale = fixMin(scale,(DFRACT_BITS-1));
      bandEnergy[i] >>= scale;
  }
}


/*****************************************************************************
  functionname: FDKaacEnc_CalcBandNrgMSOpt
  description:
  input:
  output:
*****************************************************************************/
void FDKaacEnc_CalcBandNrgMSOpt(const FIXP_DBL   *RESTRICT mdctSpectrumLeft,
                                const FIXP_DBL   *RESTRICT mdctSpectrumRight,
                                INT              *RESTRICT sfbMaxScaleSpecLeft,
                                INT              *RESTRICT sfbMaxScaleSpecRight,
                                const INT        *RESTRICT bandOffset,
                                const INT         numBands,
                                FIXP_DBL         *RESTRICT bandEnergyMid,
                                FIXP_DBL         *RESTRICT bandEnergySide,
                                INT               calcLdData,
                                FIXP_DBL         *RESTRICT bandEnergyMidLdData,
                                FIXP_DBL         *RESTRICT bandEnergySideLdData)
{
  INT i, j, minScale;
  FIXP_DBL NrgMid, NrgSide, specm, specs;

  for (i=0; i<numBands; i++) {

    NrgMid = NrgSide = FL2FXCONST_DBL(0.0);
    minScale = fixMin(sfbMaxScaleSpecLeft[i], sfbMaxScaleSpecRight[i])-4;
    minScale = fixMax(0, minScale);

    if (minScale > 0) {
      for (j=bandOffset[i];j<bandOffset[i+1];j++) {
          FIXP_DBL specL = mdctSpectrumLeft[j]<<(minScale-1);
          FIXP_DBL specR = mdctSpectrumRight[j]<<(minScale-1);
          specm = specL + specR;
          specs = specL - specR;
          NrgMid = fPow2AddDiv2(NrgMid, specm);
          NrgSide = fPow2AddDiv2(NrgSide, specs);
      }
    } else {
      for (j=bandOffset[i];j<bandOffset[i+1];j++) {
          FIXP_DBL specL = mdctSpectrumLeft[j]>>1;
          FIXP_DBL specR = mdctSpectrumRight[j]>>1;
          specm = specL + specR;
          specs = specL - specR;
          NrgMid = fPow2AddDiv2(NrgMid, specm);
          NrgSide = fPow2AddDiv2(NrgSide, specs);
      }
    }
    bandEnergyMid[i] = NrgMid<<1;
    bandEnergySide[i] = NrgSide<<1;
  }

  if(calcLdData) {
    LdDataVector(bandEnergyMid, bandEnergyMidLdData, numBands);
    LdDataVector(bandEnergySide, bandEnergySideLdData, numBands);
  }

  for (i=0; i<numBands; i++)
  {
    INT minScale = fixMin(sfbMaxScaleSpecLeft[i], sfbMaxScaleSpecRight[i]);
    INT scale = fixMax(0, 2*(minScale-4));

    if (calcLdData)
    {
       /* using the minimal scaling of left and right channel can cause very small energies;
       check ldNrg before subtract scaling multiplication: fract*INT we don't need fMult */
       
       int minus = scale*FL2FXCONST_DBL(1.0/64);

       if (bandEnergyMidLdData[i] != FL2FXCONST_DBL(-1.0f))
         bandEnergyMidLdData[i] -= minus;

       if (bandEnergySideLdData[i] != FL2FXCONST_DBL(-1.0f))
         bandEnergySideLdData[i] -= minus;
    }
    scale = fixMin(scale, (DFRACT_BITS-1));
    bandEnergyMid[i]  >>= scale;
    bandEnergySide[i] >>= scale;
  }
}
