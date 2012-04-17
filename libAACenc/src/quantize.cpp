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
   contents/description: Quantization

******************************************************************************/

#include "quantize.h"

#include "aacEnc_rom.h"

/*****************************************************************************

    functionname: FDKaacEnc_quantizeLines
    description: quantizes spectrum lines
    returns:
    input: global gain, number of lines to process, spectral data
    output: quantized spectrum

*****************************************************************************/
static void FDKaacEnc_quantizeLines(INT      gain,
                          INT      noOfLines,
                          FIXP_DBL *mdctSpectrum,
                          SHORT      *quaSpectrum)
{
  int   line;
  FIXP_DBL k = FL2FXCONST_DBL(-0.0946f + 0.5f)>>16;
  FIXP_QTD quantizer = FDKaacEnc_quantTableQ[(-gain)&3];
  INT      quantizershift = ((-gain)>>2)+1;


  for (line = 0; line < noOfLines; line++)
  {
    FIXP_DBL accu = fMultDiv2(mdctSpectrum[line],quantizer);

    if (accu < FL2FXCONST_DBL(0.0f))
    {
      accu=-accu;
      /* normalize */
      INT   accuShift = CntLeadingZeros(accu) - 1;  /* CountLeadingBits() is not necessary here since test value is always > 0 */
      accu <<= accuShift;
      INT tabIndex = (INT)(accu>>(DFRACT_BITS-2-MANT_DIGITS))&(~MANT_SIZE);
      INT totalShift = quantizershift-accuShift+1;
      accu = fMultDiv2(FDKaacEnc_mTab_3_4[tabIndex],FDKaacEnc_quantTableE[totalShift&3]);
      totalShift = (16-4)-(3*(totalShift>>2));
      FDK_ASSERT(totalShift >=0); /* MAX_QUANT_VIOLATION */
      accu>>=totalShift;
      quaSpectrum[line] = (SHORT)(-((LONG)(k + accu) >> (DFRACT_BITS-1-16)));
    }
    else if(accu > FL2FXCONST_DBL(0.0f))
    {
      /* normalize */
      INT   accuShift = CntLeadingZeros(accu) - 1;  /* CountLeadingBits() is not necessary here since test value is always > 0 */
      accu <<= accuShift;
      INT tabIndex = (INT)(accu>>(DFRACT_BITS-2-MANT_DIGITS))&(~MANT_SIZE);
      INT totalShift = quantizershift-accuShift+1;
      accu = fMultDiv2(FDKaacEnc_mTab_3_4[tabIndex],FDKaacEnc_quantTableE[totalShift&3]);
      totalShift = (16-4)-(3*(totalShift>>2));
      FDK_ASSERT(totalShift >=0); /* MAX_QUANT_VIOLATION */
      accu>>=totalShift;
      quaSpectrum[line] = (SHORT)((LONG)(k + accu) >> (DFRACT_BITS-1-16));
    }
    else
      quaSpectrum[line]=0;
  }
}


/*****************************************************************************

    functionname:iFDKaacEnc_quantizeLines
    description: iquantizes spectrum lines
                 mdctSpectrum = iquaSpectrum^4/3 *2^(0.25*gain)
    input: global gain, number of lines to process,quantized spectrum
    output: spectral data

*****************************************************************************/
static void FDKaacEnc_invQuantizeLines(INT  gain,
                             INT  noOfLines,
                             SHORT *quantSpectrum,
                             FIXP_DBL *mdctSpectrum)

{
  INT iquantizermod;
  INT iquantizershift;
  INT line;

  iquantizermod = gain&3;
  iquantizershift = gain>>2;

  for (line = 0; line < noOfLines; line++) {

    if(quantSpectrum[line] < 0) {
      FIXP_DBL accu;
      INT ex,specExp,tabIndex;
      FIXP_DBL s,t;

      accu = (FIXP_DBL) -quantSpectrum[line];

      ex = CountLeadingBits(accu);
      accu <<= ex;
      specExp = (DFRACT_BITS-1) - ex;

      FDK_ASSERT(specExp < 14);       /* this fails if abs(value) > 8191 */

      tabIndex = (INT)(accu>>(DFRACT_BITS-2-MANT_DIGITS))&(~MANT_SIZE);

      /* calculate "mantissa" ^4/3 */
      s = FDKaacEnc_mTab_4_3Elc[tabIndex];

      /* get approperiate exponent multiplier for specExp^3/4 combined with scfMod */
      t = FDKaacEnc_specExpMantTableCombElc[iquantizermod][specExp];

      /* multiply "mantissa" ^4/3 with exponent multiplier */
      accu = fMult(s,t);

      /* get approperiate exponent shifter */
      specExp = FDKaacEnc_specExpTableComb[iquantizermod][specExp]-1; /* -1 to avoid overflows in accu */

      if ((-iquantizershift-specExp) < 0)
        accu <<= -(-iquantizershift-specExp);
      else
        accu >>= -iquantizershift-specExp;

      mdctSpectrum[line] = -accu;
    }
    else if (quantSpectrum[line] > 0) {
      FIXP_DBL accu;
      INT ex,specExp,tabIndex;
      FIXP_DBL s,t;

      accu = (FIXP_DBL)(INT)quantSpectrum[line];

      ex = CountLeadingBits(accu);
      accu <<= ex;
      specExp = (DFRACT_BITS-1) - ex;

      FDK_ASSERT(specExp < 14);       /* this fails if abs(value) > 8191 */

      tabIndex = (INT)(accu>>(DFRACT_BITS-2-MANT_DIGITS))&(~MANT_SIZE);

      /* calculate "mantissa" ^4/3 */
      s = FDKaacEnc_mTab_4_3Elc[tabIndex];

      /* get approperiate exponent multiplier for specExp^3/4 combined with scfMod */
      t = FDKaacEnc_specExpMantTableCombElc[iquantizermod][specExp];

      /* multiply "mantissa" ^4/3 with exponent multiplier */
      accu = fMult(s,t);

      /* get approperiate exponent shifter */
      specExp = FDKaacEnc_specExpTableComb[iquantizermod][specExp]-1; /* -1 to avoid overflows in accu */

      if (( -iquantizershift-specExp) < 0)
        accu <<= -(-iquantizershift-specExp);
      else
        accu >>= -iquantizershift-specExp;

      mdctSpectrum[line] = accu;
    }
    else {
      mdctSpectrum[line] = FL2FXCONST_DBL(0.0f);
    }
  }
}

/*****************************************************************************

    functionname: FDKaacEnc_QuantizeSpectrum
    description: quantizes the entire spectrum
    returns:
    input: number of scalefactor bands to be quantized, ...
    output: quantized spectrum

*****************************************************************************/
void FDKaacEnc_QuantizeSpectrum(INT sfbCnt,
                      INT maxSfbPerGroup,
                      INT sfbPerGroup,
                      INT *sfbOffset,
                      FIXP_DBL *mdctSpectrum,
                      INT globalGain,
                      INT *scalefactors,
                      SHORT *quantizedSpectrum)
{
  INT sfbOffs,sfb;

  /* in FDKaacEnc_quantizeLines quaSpectrum is calculated with:
        spec^(3/4) * 2^(-3/16*QSS) * 2^(3/4*scale) + k
     simplify scaling calculation and reduce QSS before:
        spec^(3/4) * 2^(-3/16*(QSS - 4*scale)) */

  for(sfbOffs=0;sfbOffs<sfbCnt;sfbOffs+=sfbPerGroup)
  for (sfb = 0; sfb < maxSfbPerGroup; sfb++)
  {
    INT scalefactor = scalefactors[sfbOffs+sfb] ;

    FDKaacEnc_quantizeLines(globalGain - scalefactor, /* QSS */
                  sfbOffset[sfbOffs+sfb+1] - sfbOffset[sfbOffs+sfb],
                  mdctSpectrum + sfbOffset[sfbOffs+sfb],
                  quantizedSpectrum + sfbOffset[sfbOffs+sfb]);
  }
}

/*****************************************************************************

    functionname: FDKaacEnc_calcSfbDist
    description: calculates distortion of quantized values
    returns: distortion
    input: gain, number of lines to process, spectral data
    output:

*****************************************************************************/
FIXP_DBL FDKaacEnc_calcSfbDist(FIXP_DBL *mdctSpectrum,
                     SHORT *quantSpectrum,
                     INT noOfLines,
                     INT gain
                     )
{
  INT i,scale;
  FIXP_DBL xfsf;
  FIXP_DBL diff;
  FIXP_DBL invQuantSpec;

  xfsf = FL2FXCONST_DBL(0.0f);

  for (i=0; i<noOfLines; i++) {
    /* quantization */
    FDKaacEnc_quantizeLines(gain,
                  1,
                 &mdctSpectrum[i],
                 &quantSpectrum[i]);

    /* inverse quantization */
    FDKaacEnc_invQuantizeLines(gain,1,&quantSpectrum[i],&invQuantSpec);

    /* dist */
    diff = fixp_abs(fixp_abs(invQuantSpec) - fixp_abs(mdctSpectrum[i]>>1));

    scale = CountLeadingBits(diff);
    diff = scaleValue(diff, scale);
    diff = fPow2(diff);
    scale = fixMin(2*(scale-1), DFRACT_BITS-1);

    diff = scaleValue(diff, -scale);

    xfsf = xfsf + diff;
  }

  xfsf = CalcLdData(xfsf);

  return xfsf;
}

/*****************************************************************************

    functionname: FDKaacEnc_calcSfbQuantEnergyAndDist
    description: calculates energy and distortion of quantized values
    returns:
    input: gain, number of lines to process, quantized spectral data,
           spectral data
    output: energy, distortion

*****************************************************************************/
void FDKaacEnc_calcSfbQuantEnergyAndDist(FIXP_DBL *mdctSpectrum,
                               SHORT *quantSpectrum,
                               INT noOfLines,
                               INT gain,
                               FIXP_DBL *en,
                               FIXP_DBL *dist)
{
  INT i,scale;
  FIXP_DBL invQuantSpec;
  FIXP_DBL diff;

  *en   = FL2FXCONST_DBL(0.0f);
  *dist = FL2FXCONST_DBL(0.0f);

  for (i=0; i<noOfLines; i++) {
    /* inverse quantization */
    FDKaacEnc_invQuantizeLines(gain,1,&quantSpectrum[i],&invQuantSpec);

    /* energy */
    *en += fPow2(invQuantSpec);

    /* dist */
    diff = fixp_abs(fixp_abs(invQuantSpec) - fixp_abs(mdctSpectrum[i]>>1));

    scale = CountLeadingBits(diff);
    diff = scaleValue(diff, scale);
    diff = fPow2(diff);

    scale = fixMin(2*(scale-1), DFRACT_BITS-1);

    diff = scaleValue(diff, -scale);

    *dist += diff;
  }

  *en   = CalcLdData(*en)+FL2FXCONST_DBL(0.03125f);
  *dist = CalcLdData(*dist);
}

