
/* -----------------------------------------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright  1995 - 2012 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V.
  All rights reserved.

 1.    INTRODUCTION
The Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software that implements
the MPEG Advanced Audio Coding ("AAC") encoding and decoding scheme for digital audio.
This FDK AAC Codec software is intended to be used on a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient general perceptual
audio codecs. AAC-ELD is considered the best-performing full-bandwidth communications codec by
independent studies and is widely deployed. AAC has been standardized by ISO and IEC as part
of the MPEG specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including those of Fraunhofer)
may be obtained through Via Licensing (www.vialicensing.com) or through the respective patent owners
individually for the purpose of encoding or decoding bit streams in products that are compliant with
the ISO/IEC MPEG audio standards. Please note that most manufacturers of Android devices already license
these patent claims through Via Licensing or directly from the patent owners, and therefore FDK AAC Codec
software may already be covered under those patent licenses when it is used for those licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions with enhanced sound quality,
are also available from Fraunhofer. Users are encouraged to check the Fraunhofer website for additional
applications information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification, are permitted without
payment of copyright license fees provided that you satisfy the following conditions:

You must retain the complete text of this software license in redistributions of the FDK AAC Codec or
your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation and/or other materials
provided with redistributions of the FDK AAC Codec or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived from this library without
prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute the FDK AAC Codec
software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating that you changed the software
and the date of any change. For modified versions of the FDK AAC Codec, the term
"Fraunhofer FDK AAC Codec Library for Android" must be replaced by the term
"Third-Party Modified Version of the Fraunhofer FDK AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without limitation the patents of Fraunhofer,
ARE GRANTED BY THIS SOFTWARE LICENSE. Fraunhofer provides no warranty of patent non-infringement with
respect to this software.

You may use this FDK AAC Codec software or modifications thereto only for purposes that are authorized
by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright holders and contributors
"AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied warranties
of merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary, or consequential damages,
including but not limited to procurement of substitute goods or services; loss of use, data, or profits,
or business interruption, however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------------------------------------- */

/*****************************  MPEG Audio Encoder  ***************************

   Initial Authors:      M. Multrus
   Contents/Description: PS Wrapper, Downmix

******************************************************************************/

#include "ps_main.h"


/* Includes ******************************************************************/

#include "ps_const.h"
#include "ps_bitenc.h"

#include "sbr_ram.h"

/*--------------- function declarations --------------------*/
static void psFindBestScaling(
        HANDLE_PARAMETRIC_STEREO  hParametricStereo,
        FIXP_DBL                 *hybridData[HYBRID_FRAMESIZE][MAX_PS_CHANNELS][2],
        UCHAR                    *dynBandScale,
        FIXP_QMF                 *maxBandValue,
        SCHAR                    *dmxScale
        );

/*------------- function definitions ----------------*/
FDK_PSENC_ERROR PSEnc_Create(
        HANDLE_PARAMETRIC_STEREO *phParametricStereo
        )
{
  FDK_PSENC_ERROR error = PSENC_OK;

  if (phParametricStereo==NULL) {
    error = PSENC_INVALID_HANDLE;
  }
  else {
    int i;
    HANDLE_PARAMETRIC_STEREO hParametricStereo = NULL;

    if (NULL==(hParametricStereo = GetRam_ParamStereo())) {
      error = PSENC_MEMORY_ERROR;
      goto bail;
    }
    FDKmemclear(hParametricStereo, sizeof(PARAMETRIC_STEREO));

    if (PSENC_OK != (error = FDKsbrEnc_CreatePSEncode(&hParametricStereo->hPsEncode))) {
      goto bail;
    }

    for (i=0; i<MAX_PS_CHANNELS; i++) {
      if (FDKhybridAnalysisOpen(
            &hParametricStereo->fdkHybAnaFilter[i],
             hParametricStereo->__staticHybAnaStatesLF[i],
             sizeof(hParametricStereo->__staticHybAnaStatesLF[i]),
             hParametricStereo->__staticHybAnaStatesHF[i],
             sizeof(hParametricStereo->__staticHybAnaStatesHF[i])
             ) !=0 )
      {
        error = PSENC_MEMORY_ERROR;
        goto bail;
      }
    }

    *phParametricStereo = hParametricStereo; /* return allocated handle */
  }
bail:
  return error;
}

FDK_PSENC_ERROR PSEnc_Init(
        HANDLE_PARAMETRIC_STEREO  hParametricStereo,
        const HANDLE_PSENC_CONFIG hPsEncConfig,
        INT                       noQmfSlots,
        INT                       noQmfBands
       ,UCHAR                    *dynamic_RAM
        )
{
  FDK_PSENC_ERROR error = PSENC_OK;

  if ( (NULL==hParametricStereo) || (NULL==hPsEncConfig) ) {
    error = PSENC_INVALID_HANDLE;
  }
  else {
    int ch, i;

    hParametricStereo->initPS = 1;
    hParametricStereo->noQmfSlots = noQmfSlots;
    hParametricStereo->noQmfBands = noQmfBands;

    /* clear delay lines */
    FDKmemclear(hParametricStereo->qmfDelayLines, sizeof(hParametricStereo->qmfDelayLines));

    hParametricStereo->qmfDelayScale = FRACT_BITS-1;

    /* create configuration for hybrid filter bank */
    for (ch=0; ch<MAX_PS_CHANNELS; ch++) {
      FDKhybridAnalysisInit(
            &hParametricStereo->fdkHybAnaFilter[ch],
             THREE_TO_TEN,
             QMF_CHANNELS,
             QMF_CHANNELS,
             1
             );
    } /* ch */

    FDKhybridSynthesisInit(
          &hParametricStereo->fdkHybSynFilter,
           THREE_TO_TEN,
           QMF_CHANNELS,
           QMF_CHANNELS
           );

    /* determine average delay */
    hParametricStereo->psDelay = (HYBRID_FILTER_DELAY*hParametricStereo->noQmfBands);

    if ( (hPsEncConfig->maxEnvelopes < PSENC_NENV_1) || (hPsEncConfig->maxEnvelopes > PSENC_NENV_MAX) ) {
      hPsEncConfig->maxEnvelopes = PSENC_NENV_DEFAULT;
    }
    hParametricStereo->maxEnvelopes = hPsEncConfig->maxEnvelopes;

    if (PSENC_OK != (error = FDKsbrEnc_InitPSEncode(hParametricStereo->hPsEncode, (PS_BANDS) hPsEncConfig->nStereoBands, hPsEncConfig->iidQuantErrorThreshold))){
      goto bail;
    }

    for (ch = 0; ch<MAX_PS_CHANNELS; ch ++) {
      FIXP_DBL *pDynReal = GetRam_Sbr_envRBuffer (ch, dynamic_RAM);
      FIXP_DBL *pDynImag = GetRam_Sbr_envIBuffer (ch, dynamic_RAM);

      for (i=0; i<HYBRID_FRAMESIZE; i++) {
        hParametricStereo->pHybridData[i+HYBRID_READ_OFFSET][ch][0] = &pDynReal[i*MAX_HYBRID_BANDS];
        hParametricStereo->pHybridData[i+HYBRID_READ_OFFSET][ch][1] = &pDynImag[i*MAX_HYBRID_BANDS];;
      }

      for (i=0; i<HYBRID_READ_OFFSET; i++) {
        hParametricStereo->pHybridData[i][ch][0] = hParametricStereo->__staticHybridData[i][ch][0];
        hParametricStereo->pHybridData[i][ch][1] = hParametricStereo->__staticHybridData[i][ch][1];
      }
    } /* ch */

    /* clear static hybrid buffer */
    FDKmemclear(hParametricStereo->__staticHybridData, sizeof(hParametricStereo->__staticHybridData));

    /* clear bs buffer */
    FDKmemclear(hParametricStereo->psOut, sizeof(hParametricStereo->psOut));

    /* clear scaling buffer */
    FDKmemclear(hParametricStereo->dynBandScale, sizeof(UCHAR)*PS_MAX_BANDS);
    FDKmemclear(hParametricStereo->maxBandValue, sizeof(FIXP_QMF)*PS_MAX_BANDS);

  } /* valid handle */
bail:
  return error;
}


FDK_PSENC_ERROR PSEnc_Destroy(
        HANDLE_PARAMETRIC_STEREO *phParametricStereo
        )
{
  FDK_PSENC_ERROR error = PSENC_OK;

  if (NULL!=phParametricStereo) {
    HANDLE_PARAMETRIC_STEREO hParametricStereo = *phParametricStereo;
    if(hParametricStereo != NULL){
      FDKsbrEnc_DestroyPSEncode(&hParametricStereo->hPsEncode);
      FreeRam_ParamStereo(phParametricStereo);
    }
  }

  return error;
}

static FDK_PSENC_ERROR ExtractPSParameters(
        HANDLE_PARAMETRIC_STEREO  hParametricStereo,
        const int                 sendHeader,
        FIXP_DBL                 *hybridData[HYBRID_FRAMESIZE][MAX_PS_CHANNELS][2]
        )
{
  FDK_PSENC_ERROR error = PSENC_OK;

  if (hParametricStereo == NULL) {
    error = PSENC_INVALID_HANDLE;
  }
  else {
    /* call ps encode function */
    if (hParametricStereo->initPS){
      hParametricStereo->psOut[1] = hParametricStereo->psOut[0];
    }
    hParametricStereo->psOut[0] = hParametricStereo->psOut[1];

    if (PSENC_OK != (error = FDKsbrEnc_PSEncode(
            hParametricStereo->hPsEncode,
           &hParametricStereo->psOut[1],
            hParametricStereo->dynBandScale,
            hParametricStereo->maxEnvelopes,
            hybridData,
            hParametricStereo->noQmfSlots,
            sendHeader)))
    {
      goto bail;
    }

    if (hParametricStereo->initPS) {
      hParametricStereo->psOut[0] = hParametricStereo->psOut[1];
      hParametricStereo->initPS = 0;
    }
  }
bail:
  return error;
}


static FDK_PSENC_ERROR DownmixPSQmfData(
       HANDLE_PARAMETRIC_STEREO  hParametricStereo,
       HANDLE_QMF_FILTER_BANK    sbrSynthQmf,
       FIXP_QMF       **RESTRICT mixRealQmfData,
       FIXP_QMF       **RESTRICT mixImagQmfData,
       INT_PCM                  *downsampledOutSignal,
       FIXP_DBL                 *hybridData[HYBRID_FRAMESIZE][MAX_PS_CHANNELS][2],
       const INT                 noQmfSlots,
       const INT                 psQmfScale[MAX_PS_CHANNELS],
       SCHAR                    *qmfScale
       )
{
  FDK_PSENC_ERROR error = PSENC_OK;

  if(hParametricStereo == NULL){
    error = PSENC_INVALID_HANDLE;
  }
  else {
    int n, k;
    C_ALLOC_SCRATCH_START(pWorkBuffer, FIXP_QMF, QMF_CHANNELS*2);    

    /* define scalings */
    int dynQmfScale = fixMax(0, hParametricStereo->dmxScale-1); /* scale one bit more for addition of left and right */
    int downmixScale = psQmfScale[0] - dynQmfScale;
    const FIXP_DBL maxStereoScaleFactor = MAXVAL_DBL; /* 2.f/2.f */

    for (n = 0; n<noQmfSlots; n++) {

      FIXP_DBL tmpHybrid[2][MAX_HYBRID_BANDS];

      for(k = 0; k<71; k++){
          int dynScale, sc; /* scaling */
          FIXP_QMF tmpLeftReal, tmpRightReal, tmpLeftImag, tmpRightImag;
          FIXP_DBL tmpScaleFactor, stereoScaleFactor;

          tmpLeftReal  = hybridData[n][0][0][k];
          tmpLeftImag  = hybridData[n][0][1][k];
          tmpRightReal = hybridData[n][1][0][k];
          tmpRightImag = hybridData[n][1][1][k];

          sc = fixMax(0,CntLeadingZeros( fixMax(fixMax(fixp_abs(tmpLeftReal),fixp_abs(tmpLeftImag)),fixMax(fixp_abs(tmpRightReal),fixp_abs(tmpRightImag))) )-2);

          tmpLeftReal  <<= sc; tmpLeftImag  <<= sc;
          tmpRightReal <<= sc; tmpRightImag <<= sc;
          dynScale = fixMin(sc-dynQmfScale,DFRACT_BITS-1);

          /* calc stereo scale factor to avoid loss of energy in bands                                                 */
          /* stereo scale factor = min(2.0f, sqrt( (abs(l(k, n)^2 + abs(r(k, n)^2 )))/(0.5f*abs(l(k, n) + r(k, n))) )) */
          stereoScaleFactor = fPow2Div2(tmpLeftReal)  + fPow2Div2(tmpLeftImag)
                            + fPow2Div2(tmpRightReal) + fPow2Div2(tmpRightImag) ;

          /* might be that tmpScaleFactor becomes negative, so fabs(.) */
          tmpScaleFactor    = fixp_abs(stereoScaleFactor + fMult(tmpLeftReal,tmpRightReal) + fMult(tmpLeftImag,tmpRightImag));

          /* min(2.0f, sqrt(stereoScaleFactor/(0.5f*tmpScaleFactor)))  */
          if ( (stereoScaleFactor>>1) < fMult(maxStereoScaleFactor,tmpScaleFactor) ) {

              int sc_num   = CountLeadingBits(stereoScaleFactor) ;
              int sc_denum = CountLeadingBits(tmpScaleFactor) ;
              sc       = -(sc_num-sc_denum);

              tmpScaleFactor = schur_div((stereoScaleFactor<<(sc_num))>>1,
                                          tmpScaleFactor<<sc_denum,
                                          16) ;

              /* prevent odd scaling for next sqrt calculation */
              if (sc&0x1) {
                sc++;
                tmpScaleFactor>>=1;
              }
              stereoScaleFactor = sqrtFixp(tmpScaleFactor);
              stereoScaleFactor <<= (sc>>1);
          }
          else {
              stereoScaleFactor = maxStereoScaleFactor;
          }

          /* write data to hybrid output */
          tmpHybrid[0][k] = fMultDiv2(stereoScaleFactor, (FIXP_QMF)(tmpLeftReal + tmpRightReal))>>dynScale;
          tmpHybrid[1][k] = fMultDiv2(stereoScaleFactor, (FIXP_QMF)(tmpLeftImag + tmpRightImag))>>dynScale;

      } /* hybrid bands - k */

      FDKhybridSynthesisApply(
            &hParametricStereo->fdkHybSynFilter,
             tmpHybrid[0],
             tmpHybrid[1],
             mixRealQmfData[n],
             mixImagQmfData[n]);

      qmfSynthesisFilteringSlot(
            sbrSynthQmf,
            mixRealQmfData[n],
            mixImagQmfData[n],
            downmixScale-7,
            downmixScale-7,
            downsampledOutSignal+(n*sbrSynthQmf->no_channels),
            1,
            pWorkBuffer);

    } /* slots */

    *qmfScale = -downmixScale + 7;

    C_ALLOC_SCRATCH_END(pWorkBuffer, FIXP_QMF, QMF_CHANNELS*2);


  {
    const INT noQmfSlots2 = hParametricStereo->noQmfSlots>>1;
    const int noQmfBands  = hParametricStereo->noQmfBands;

    INT scale, i, j, slotOffset;

    FIXP_QMF tmp[2][QMF_CHANNELS];

    for (i=0; i<noQmfSlots2; i++) {
      FDKmemcpy(tmp[0], hParametricStereo->qmfDelayLines[0][i], noQmfBands*sizeof(FIXP_QMF));
      FDKmemcpy(tmp[1], hParametricStereo->qmfDelayLines[1][i], noQmfBands*sizeof(FIXP_QMF));

      FDKmemcpy(hParametricStereo->qmfDelayLines[0][i], mixRealQmfData[i+noQmfSlots2], noQmfBands*sizeof(FIXP_QMF));
      FDKmemcpy(hParametricStereo->qmfDelayLines[1][i], mixImagQmfData[i+noQmfSlots2], noQmfBands*sizeof(FIXP_QMF));

      FDKmemcpy(mixRealQmfData[i+noQmfSlots2], mixRealQmfData[i], noQmfBands*sizeof(FIXP_QMF));
      FDKmemcpy(mixImagQmfData[i+noQmfSlots2], mixImagQmfData[i], noQmfBands*sizeof(FIXP_QMF));

      FDKmemcpy(mixRealQmfData[i], tmp[0], noQmfBands*sizeof(FIXP_QMF));
      FDKmemcpy(mixImagQmfData[i], tmp[1], noQmfBands*sizeof(FIXP_QMF));
    }

    if (hParametricStereo->qmfDelayScale > *qmfScale) {
      scale = hParametricStereo->qmfDelayScale - *qmfScale;
      slotOffset = 0;
    }
    else {
      scale = *qmfScale - hParametricStereo->qmfDelayScale;
      slotOffset = noQmfSlots2;
    }

    for (i=0; i<noQmfSlots2; i++) {
      for (j=0; j<noQmfBands; j++) {
        mixRealQmfData[i+slotOffset][j] >>= scale;
        mixImagQmfData[i+slotOffset][j] >>= scale;
      }
    }

    scale = *qmfScale;
    *qmfScale = FDKmin(*qmfScale, hParametricStereo->qmfDelayScale);
    hParametricStereo->qmfDelayScale = scale;
  }

  } /* valid handle */

  return error;
}


INT FDKsbrEnc_PSEnc_WritePSData(
        HANDLE_PARAMETRIC_STEREO  hParametricStereo,
        HANDLE_FDK_BITSTREAM      hBitstream
        )
{
  return ( (hParametricStereo!=NULL) ? FDKsbrEnc_WritePSBitstream(&hParametricStereo->psOut[0], hBitstream) : 0 );
}


FDK_PSENC_ERROR FDKsbrEnc_PSEnc_ParametricStereoProcessing(
        HANDLE_PARAMETRIC_STEREO  hParametricStereo,
        INT_PCM                  *samples[2],
        UINT                      timeInStride,
        QMF_FILTER_BANK         **hQmfAnalysis,
        FIXP_QMF **RESTRICT       downmixedRealQmfData,
        FIXP_QMF **RESTRICT       downmixedImagQmfData,
        INT_PCM                  *downsampledOutSignal,
        HANDLE_QMF_FILTER_BANK    sbrSynthQmf,
        SCHAR                    *qmfScale,
        const int                 sendHeader
        )
{
  FDK_PSENC_ERROR error = PSENC_OK;
  INT noQmfBands  = hParametricStereo->noQmfBands;
  INT psQmfScale[MAX_PS_CHANNELS] = {0};
  int psCh, i;
  C_ALLOC_SCRATCH_START(pWorkBuffer, FIXP_DBL, QMF_CHANNELS*4);    

  for (psCh = 0; psCh<MAX_PS_CHANNELS; psCh ++) {

    for (i = 0; i < hQmfAnalysis[psCh]->no_col; i++) {

      qmfAnalysisFilteringSlot(
          hQmfAnalysis[psCh],
         &pWorkBuffer[2*QMF_CHANNELS], /* qmfReal[QMF_CHANNELS] */
         &pWorkBuffer[3*QMF_CHANNELS], /* qmfImag[QMF_CHANNELS] */
          samples[psCh]+i*(hQmfAnalysis[psCh]->no_channels*timeInStride),
          timeInStride,
         &pWorkBuffer[0*QMF_CHANNELS]  /* qmf workbuffer 2*QMF_CHANNELS */
          );

      FDKhybridAnalysisApply(
         &hParametricStereo->fdkHybAnaFilter[psCh],
         &pWorkBuffer[2*QMF_CHANNELS],  /* qmfReal[QMF_CHANNELS] */
         &pWorkBuffer[3*QMF_CHANNELS],  /* qmfImag[QMF_CHANNELS] */
          hParametricStereo->pHybridData[i+HYBRID_READ_OFFSET][psCh][0],
          hParametricStereo->pHybridData[i+HYBRID_READ_OFFSET][psCh][1]
          );

    } /* no_col loop  i  */

    psQmfScale[psCh] = hQmfAnalysis[psCh]->outScalefactor;

  } /* for psCh */

  C_ALLOC_SCRATCH_END(pWorkBuffer, FIXP_DBL, QMF_CHANNELS*4);

  /* find best scaling in new QMF and Hybrid data */
  psFindBestScaling( hParametricStereo,
                    &hParametricStereo->pHybridData[HYBRID_READ_OFFSET],
                     hParametricStereo->dynBandScale,
                     hParametricStereo->maxBandValue,
                    &hParametricStereo->dmxScale ) ;


  /* extract the ps parameters */
  if(PSENC_OK != (error = ExtractPSParameters(hParametricStereo, sendHeader, &hParametricStereo->pHybridData[0]))){
    goto bail;
  }

  /* save hybrid date for next frame */
  for (i=0; i<HYBRID_READ_OFFSET; i++) {
    FDKmemcpy(hParametricStereo->pHybridData[i][0][0], hParametricStereo->pHybridData[HYBRID_FRAMESIZE+i][0][0], MAX_HYBRID_BANDS*sizeof(FIXP_DBL)); /* left, real */
    FDKmemcpy(hParametricStereo->pHybridData[i][0][1], hParametricStereo->pHybridData[HYBRID_FRAMESIZE+i][0][1], MAX_HYBRID_BANDS*sizeof(FIXP_DBL)); /* left, imag */
    FDKmemcpy(hParametricStereo->pHybridData[i][1][0], hParametricStereo->pHybridData[HYBRID_FRAMESIZE+i][1][0], MAX_HYBRID_BANDS*sizeof(FIXP_DBL)); /* right, real */
    FDKmemcpy(hParametricStereo->pHybridData[i][1][1], hParametricStereo->pHybridData[HYBRID_FRAMESIZE+i][1][1], MAX_HYBRID_BANDS*sizeof(FIXP_DBL)); /* right, imag */
  }

  /* downmix and hybrid synthesis */
  if (PSENC_OK != (error = DownmixPSQmfData(hParametricStereo, sbrSynthQmf, downmixedRealQmfData, downmixedImagQmfData, downsampledOutSignal, &hParametricStereo->pHybridData[HYBRID_READ_OFFSET], hParametricStereo->noQmfSlots, psQmfScale, qmfScale))) {
    goto bail;
  }

bail:

  return error;
}

static void psFindBestScaling(
        HANDLE_PARAMETRIC_STEREO  hParametricStereo,
        FIXP_DBL                 *hybridData[HYBRID_FRAMESIZE][MAX_PS_CHANNELS][2],
        UCHAR                    *dynBandScale,
        FIXP_QMF                 *maxBandValue,
        SCHAR                    *dmxScale
        )
{
  HANDLE_PS_ENCODE hPsEncode      =  hParametricStereo->hPsEncode;

  INT group, bin, col, band;
  const INT frameSize  = hParametricStereo->noQmfSlots;
  const INT psBands    = (INT) hPsEncode->psEncMode;
  const INT nIidGroups = hPsEncode->nQmfIidGroups + hPsEncode->nSubQmfIidGroups;

  /* group wise scaling */
  FIXP_QMF maxVal [2][PS_MAX_BANDS];
  FIXP_QMF maxValue = FL2FXCONST_DBL(0.f);

  FDKmemclear(maxVal, sizeof(maxVal));

  /* start with hybrid data */
  for (group=0; group < nIidGroups; group++) {
    /* Translate group to bin */
    bin = hPsEncode->subband2parameterIndex[group];

    /* Translate from 20 bins to 10 bins */
    if (hPsEncode->psEncMode == PS_BANDS_COARSE) {
      bin >>= 1;
    }

    /* QMF downmix scaling */
    {
      FIXP_QMF tmp = maxVal[0][bin];
      int i;
      for (col=0; col<frameSize-HYBRID_READ_OFFSET; col++) {
        for (i = hPsEncode->iidGroupBorders[group]; i < hPsEncode->iidGroupBorders[group+1]; i++) {
          tmp = fixMax(tmp, (FIXP_QMF)fixp_abs(hybridData[col][0][0][i]));
          tmp = fixMax(tmp, (FIXP_QMF)fixp_abs(hybridData[col][0][1][i]));
          tmp = fixMax(tmp, (FIXP_QMF)fixp_abs(hybridData[col][1][0][i]));
          tmp = fixMax(tmp, (FIXP_QMF)fixp_abs(hybridData[col][1][1][i]));
        }
      }
      maxVal[0][bin] = tmp;

      tmp = maxVal[1][bin];
      for (col=frameSize-HYBRID_READ_OFFSET; col<frameSize; col++) {
        for (i = hPsEncode->iidGroupBorders[group]; i < hPsEncode->iidGroupBorders[group+1]; i++) {
          tmp = fixMax(tmp, (FIXP_QMF)fixp_abs(hybridData[col][0][0][i]));
          tmp = fixMax(tmp, (FIXP_QMF)fixp_abs(hybridData[col][0][1][i]));
          tmp = fixMax(tmp, (FIXP_QMF)fixp_abs(hybridData[col][1][0][i]));
          tmp = fixMax(tmp, (FIXP_QMF)fixp_abs(hybridData[col][1][1][i]));
        }
      }
      maxVal[1][bin] = tmp;
    }
  } /* nIidGroups */

  /* convert maxSpec to maxScaling, find scaling space */
  for (band=0; band<psBands; band++) {
#ifndef MULT_16x16
    dynBandScale[band] = CountLeadingBits(fixMax(maxVal[0][band],maxBandValue[band]));
#else
    dynBandScale[band] = fixMax(0,CountLeadingBits(fixMax(maxVal[0][band],maxBandValue[band]))-FRACT_BITS);
#endif
    maxValue = fixMax(maxValue,fixMax(maxVal[0][band],maxVal[1][band]));
    maxBandValue[band] = fixMax(maxVal[0][band], maxVal[1][band]);
  }

  /* calculate maximal scaling for QMF downmix */
#ifndef MULT_16x16
  *dmxScale = fixMin(DFRACT_BITS, CountLeadingBits(maxValue));
#else
  *dmxScale = fixMax(0,fixMin(FRACT_BITS, CountLeadingBits(FX_QMF2FX_DBL(maxValue))));
#endif

}

