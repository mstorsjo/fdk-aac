/*****************************  MPEG Audio Encoder  ***************************

                     (C) Copyright Fraunhofer IIS (2004-2005)
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
   Initial Authors:      M. Multrus
   Contents/Description: PS Wrapper, Downmix

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "ps_main.h"


/* Includes ******************************************************************/

#include "ps_const.h"
#include "ps_bitenc.h"

#include "sbr_ram.h"



/* Function declarations ****************************************************/
static void psFindBestScaling(HANDLE_PARAMETRIC_STEREO hParametricStereo,
                              UCHAR  *dynBandScale,
                              FIXP_QMF *maxBandValue,
                              SCHAR  *dmxScale);

/*
  name:        static HANDLE_ERROR_INFO CreatePSQmf()
  description: Creates struct (buffer) to store qmf data
  returns:     error code of type HANDLE_ERROR_INFO
  input:       - INT nCols: number of qmf samples stored in regular qmf buffer
               - INT nRows: number qmf channels
               - INT hybridFilterDelay: delay in qmf samples of hybrid filter
  output:      - HANDLE_PS_QMF_DATA *hPsQmfData: according handle
*/
static HANDLE_ERROR_INFO CreatePSQmf(HANDLE_PS_QMF_DATA *phPsQmfData, INT ch)
{
  HANDLE_ERROR_INFO error = noError;
  HANDLE_PS_QMF_DATA hPsQmfData = GetRam_PsQmfData(ch);
  if (hPsQmfData==NULL) {
    error = 1;
    goto bail;
  }
  FDKmemclear(hPsQmfData, sizeof(PS_QMF_DATA));

  hPsQmfData->rQmfData[0] = GetRam_PsRqmf(ch);
  hPsQmfData->iQmfData[0] = GetRam_PsIqmf(ch);

  if ( (hPsQmfData->rQmfData[0]==NULL) || (hPsQmfData->iQmfData[0]==NULL) ) {
    error = 1;
    goto bail;
  }


bail:
  *phPsQmfData = hPsQmfData;
  return error;
}

static HANDLE_ERROR_INFO InitPSQmf(HANDLE_PS_QMF_DATA hPsQmfData, INT nCols, INT nRows, INT hybridFilterDelay, INT ch, UCHAR *dynamic_RAM)
{
  INT i, bufferLength = 0;

  hPsQmfData->nCols = nCols;
  hPsQmfData->nRows = nRows;
  hPsQmfData->bufferReadOffset       = QMF_READ_OFFSET;
  hPsQmfData->bufferReadOffsetHybrid = HYBRID_READ_OFFSET;  /* calc read offset for hybrid analysis in qmf samples */
  hPsQmfData->bufferWriteOffset = hPsQmfData->bufferReadOffsetHybrid + hybridFilterDelay;
  hPsQmfData->bufferLength = bufferLength = hPsQmfData->bufferWriteOffset + nCols;

  FDK_ASSERT(PSENC_QMF_BUFFER_LENGTH>=bufferLength);

  for(i=0; i<bufferLength; i++) {
    hPsQmfData->rQmfData[i] = FDKsbrEnc_SliceRam_PsRqmf(hPsQmfData->rQmfData[0], dynamic_RAM, ch, i, nCols);
    hPsQmfData->iQmfData[i] = FDKsbrEnc_SliceRam_PsIqmf(hPsQmfData->iQmfData[0], dynamic_RAM, ch, i, nCols);
  }

  for(i=0; i<bufferLength; i++){
    FDKmemclear(hPsQmfData->rQmfData[i], (sizeof(FIXP_QMF)*QMF_CHANNELS));
    FDKmemclear(hPsQmfData->iQmfData[i], (sizeof(FIXP_QMF)*QMF_CHANNELS));
  }

  return noError;
}


/*
  name:        static HANDLE_ERROR_INFO CreatePSChannel()
  description: Creates PS channel struct
  returns:     error code of type HANDLE_ERROR_INFO
  input:       - HANDLE_PS_HYBRID_CONFIG hHybConfig: config structure for hybrid filter bank
  output:      - HANDLE_PS_CHANNEL_DATA *hPsChannelData
*/
static HANDLE_ERROR_INFO CreatePSChannel(HANDLE_PS_CHANNEL_DATA *hPsChannelData,
                                         INT                     ch
                                        )
{
  HANDLE_ERROR_INFO error = noError;

  (*hPsChannelData) = GetRam_PsChData(ch);

  if (*hPsChannelData==NULL) {
    error = 1;
    goto bail;
  }
  FDKmemclear(*hPsChannelData, sizeof(PS_CHANNEL_DATA));


  if (error == noError) {
    if (noError != (error = FDKsbrEnc_CreateHybridFilterBank(&(*hPsChannelData)->hHybAna,
                                                              ch )))
    {
      goto bail;
    }
  }
  if (error == noError) {
    if (noError != (error = FDKsbrEnc_CreateHybridData( &((*hPsChannelData)->hHybData),
                                              ch))) {
      goto bail;
    }
  }
  if(error == noError){
    if(noError != (error = CreatePSQmf(&((*hPsChannelData)->hPsQmfData), ch)))
    {
      goto bail;
    }
  }
bail:
  return error;
}

static HANDLE_ERROR_INFO InitPSChannel(HANDLE_PS_CHANNEL_DATA    hPsChannelData,
                                         HANDLE_PS_HYBRID_CONFIG hHybConfig,
                                         INT                     noQmfSlots,
                                         INT                     noQmfBands
                                        ,INT                     ch,
                                         UCHAR                  *dynamic_RAM
                                         )
{
  HANDLE_ERROR_INFO error = noError;
  INT hybridFilterDelay = 0;

  if (error == noError) {
    if (noError != (error = FDKsbrEnc_InitHybridFilterBank(hPsChannelData->hHybAna,
                                                              hHybConfig,
                                                              noQmfSlots )))
    {
      error = handBack(error);
    }
  }

  if(error == noError){
    hybridFilterDelay = FDKsbrEnc_GetHybridFilterDelay(hPsChannelData->hHybAna);
    hPsChannelData->psChannelDelay = hybridFilterDelay * noQmfBands;
  }

  if (error == noError) {
    if (noError != (error = FDKsbrEnc_InitHybridData( hPsChannelData->hHybData,
                                              hHybConfig,
                                              noQmfSlots)))
    {
      error = handBack(error);
    }
  }

  if(error == noError){
    if(noError != (error = InitPSQmf(hPsChannelData->hPsQmfData,
                                        noQmfSlots,
                                        noQmfBands,
                                        hybridFilterDelay
                                       ,ch,
                                        dynamic_RAM
                                        )))
    {
      error = handBack(error);
    }
  }

  return error;
}


/*
  name:        static HANDLE_ERROR_INFO PSEnc_Create()
  description: Creates PS struct
  returns:     error code of type HANDLE_ERROR_INFO
  input:       HANDLE_PSENC_CONFIG hPsEncConfig: configuration
  output:      HANDLE_PARAMETRIC_STEREO *hParametricStereo

*/
HANDLE_ERROR_INFO
PSEnc_Create(HANDLE_PARAMETRIC_STEREO *phParametricStereo)
{
  HANDLE_ERROR_INFO error = noError;
  INT i;
  HANDLE_PARAMETRIC_STEREO hParametricStereo = GetRam_ParamStereo();

  if (hParametricStereo==NULL) {
    error = 1;
    goto bail;
  }

  FDKmemclear(hParametricStereo,sizeof(PARAMETRIC_STEREO));

  hParametricStereo->qmfDelayRealRef = GetRam_PsEnvRBuffer(0);
  hParametricStereo->qmfDelayImagRef = GetRam_PsEnvIBuffer(0);

  if ( (hParametricStereo->qmfDelayRealRef==NULL) || (hParametricStereo->qmfDelayImagRef==NULL) ) {
    error = 1;
    goto bail;
  }

  for (i = 0; i < (QMF_MAX_TIME_SLOTS>>1); i++) {
    hParametricStereo->qmfDelayReal[i] = hParametricStereo->qmfDelayRealRef + (i*QMF_CHANNELS);
    hParametricStereo->qmfDelayImag[i] = hParametricStereo->qmfDelayImagRef + (i*QMF_CHANNELS);
  }

  for(i=0; i<MAX_PS_CHANNELS; i++){
    if(noError != (error = CreatePSChannel(&hParametricStereo->hPsChannelData[i],
                                         i
                                        )))
    {
      goto bail;
    }
  }


  if(noError != (error = FDKsbrEnc_CreatePSEncode(&hParametricStereo->hPsEncode))) {
    error = 1;
    goto bail;
  }

  hParametricStereo->hHybridConfig = GetRam_PsHybConfig(); /* allocate memory */

  /* calc PS_OUT values and delay one frame ! */
  hParametricStereo->hPsOut[0] = GetRam_PsOut(0);
  hParametricStereo->hPsOut[1] = GetRam_PsOut(1);
  if ( (hParametricStereo->hHybridConfig==NULL) || (hParametricStereo->hPsOut[0]==NULL) || (hParametricStereo->hPsOut[1]==NULL) ) {
    error = 1;
    goto bail;
  }

bail:
  *phParametricStereo = hParametricStereo;
  return error;
}

HANDLE_ERROR_INFO
PSEnc_Init(HANDLE_PARAMETRIC_STEREO    hParametricStereo,
             HANDLE_PSENC_CONFIG       hPsEncConfig,
             INT                       noQmfSlots,
             INT                       noQmfBands
            ,UCHAR                    *dynamic_RAM
            )
{
  HANDLE_ERROR_INFO error = noError;
  INT i;
  INT tmpDelay = 0;

  if(error == noError){
    if(hPsEncConfig == NULL){
      error = ERROR(CDI, "Invalid configuration handle.");
    }
  }

  hParametricStereo->initPS = 1;
  hParametricStereo->noQmfSlots = noQmfSlots;
  hParametricStereo->noQmfBands = noQmfBands;

  for (i = 0; i < hParametricStereo->noQmfSlots>>1; i++) {
    FDKmemclear( hParametricStereo->qmfDelayReal[i],QMF_CHANNELS*sizeof(FIXP_DBL));
    FDKmemclear( hParametricStereo->qmfDelayImag[i],QMF_CHANNELS*sizeof(FIXP_DBL));
  }
  hParametricStereo->qmfDelayScale = FRACT_BITS-1;

  if(error == noError) {
    PS_BANDS nHybridSubbands = (PS_BANDS)0;

    switch(hPsEncConfig->nStereoBands){
    case PSENC_STEREO_BANDS_10:
      nHybridSubbands = PS_BANDS_COARSE;
      break;
    case PSENC_STEREO_BANDS_20:
      nHybridSubbands = PS_BANDS_MID;
      break;
    case PSENC_STEREO_BANDS_34:
      /* nHybridSubbands = PS_BANDS_FINE; */
      FDK_ASSERT(0); /* we don't support this mode! */
      break;
    default:
      nHybridSubbands = (PS_BANDS)0;
      break;
    }
    /* create configuration for hybrid filter bank */
    FDKmemclear(hParametricStereo->hHybridConfig,sizeof(PS_HYBRID_CONFIG));
    if(noError != (error = FDKsbrEnc_CreateHybridConfig(&hParametricStereo->hHybridConfig, nHybridSubbands))) {
      error = handBack(error);
    }
  }


  tmpDelay = 0;
  for(i=0; i<MAX_PS_CHANNELS; i++) {

    if(error == noError){
      if(noError != (error = InitPSChannel( hParametricStereo->hPsChannelData[i],
                                            hParametricStereo->hHybridConfig,
                                            hParametricStereo->noQmfSlots,
                                            hParametricStereo->noQmfBands
                                           ,i,
                                            dynamic_RAM
                                             )))
      {
        error = handBack(error);
      }
    }

    if(error == noError){
      /* sum up delay in samples for all channels (should be the same for all channels) */
      tmpDelay += hParametricStereo->hPsChannelData[i]->psChannelDelay;
    }
  }

  if(error == noError){
    /* determine average delay */
    hParametricStereo->psDelay = tmpDelay/MAX_PS_CHANNELS;
  }

  if(error == noError){
    if ( (hPsEncConfig->maxEnvelopes < PSENC_NENV_1)
         || (hPsEncConfig->maxEnvelopes > PSENC_NENV_MAX) ) {
      hPsEncConfig->maxEnvelopes = PSENC_NENV_DEFAULT;
    }
    hParametricStereo->maxEnvelopes = hPsEncConfig->maxEnvelopes;
  }

  if(error == noError){
    if(noError != (error = FDKsbrEnc_InitPSEncode(hParametricStereo->hPsEncode, (PS_BANDS) hPsEncConfig->nStereoBands, hPsEncConfig->iidQuantErrorThreshold))){
      error = handBack(error);
    }
  }

  /* clear buffer */
  FDKmemclear(hParametricStereo->hPsOut[0], sizeof(PS_OUT));
  FDKmemclear(hParametricStereo->hPsOut[1], sizeof(PS_OUT));

  /* clear scaling buffer */
  FDKmemclear(hParametricStereo->dynBandScale, sizeof(UCHAR)*PS_MAX_BANDS);
  FDKmemclear(hParametricStereo->maxBandValue, sizeof(FIXP_QMF)*PS_MAX_BANDS);

  return error;
}



/*
  name:        static HANDLE_ERROR_INFO DestroyPSQmf
  description: destroy PS qmf buffers
  returns:     error code of type HANDLE_ERROR_INFO
  input:       - HANDLE_PS_QMF_DATA *hPsQmfData
  output:      none
*/

static HANDLE_ERROR_INFO DestroyPSQmf(HANDLE_PS_QMF_DATA* phPsQmfData)
{
  HANDLE_PS_QMF_DATA hPsQmfData = *phPsQmfData;

  if(hPsQmfData) {
    FreeRam_PsRqmf(hPsQmfData->rQmfData);
    FreeRam_PsIqmf(hPsQmfData->iQmfData);
    FreeRam_PsQmfData(phPsQmfData);
  }

  return noError;
}



/*
  name:        static HANDLE_ERROR_INFO DestroyPSChannel
  description: destroy PS channel data
  returns:     error code of type HANDLE_ERROR_INFO
  input:       - HANDLE_PS_CHANNEL_DATA *hPsChannelDAta
  output:      none
*/


static HANDLE_ERROR_INFO DestroyPSChannel(HANDLE_PS_CHANNEL_DATA *phPsChannelData){

  HANDLE_ERROR_INFO error = noError;
  HANDLE_PS_CHANNEL_DATA hPsChannelData = *phPsChannelData;

  if(hPsChannelData != NULL){

    DestroyPSQmf(&hPsChannelData->hPsQmfData);

    FDKsbrEnc_DeleteHybridFilterBank(&hPsChannelData->hHybAna);

    FDKsbrEnc_DestroyHybridData(&hPsChannelData->hHybData);

    FreeRam_PsChData(phPsChannelData);
  }

  return error;
}


/*
  name:        static HANDLE_ERROR_INFO PSEnc_Destroy
  description: destroy PS encoder handle
  returns:     error code of type HANDLE_ERROR_INFO
  input:       - HANDLE_PARAMETRIC_STEREO *hParametricStereo
  output:      none
*/

HANDLE_ERROR_INFO
PSEnc_Destroy(HANDLE_PARAMETRIC_STEREO *phParametricStereo){

  HANDLE_ERROR_INFO error = noError;
  HANDLE_PARAMETRIC_STEREO hParametricStereo = *phParametricStereo;
  INT i;

  if(hParametricStereo != NULL){
    for(i=0; i<MAX_PS_CHANNELS; i++){
      DestroyPSChannel(&(hParametricStereo->hPsChannelData[i]));
    }
    FreeRam_PsEnvRBuffer(&hParametricStereo->qmfDelayRealRef);
    FreeRam_PsEnvIBuffer(&hParametricStereo->qmfDelayImagRef);

    FDKsbrEnc_DestroyPSEncode(&hParametricStereo->hPsEncode);

    FreeRam_PsOut(&hParametricStereo->hPsOut[0]);
    FreeRam_PsOut(&hParametricStereo->hPsOut[1]);

    FreeRam_PsHybConfig(&hParametricStereo->hHybridConfig);
    FreeRam_ParamStereo(phParametricStereo);
  }

  return error;
}

/*
  name:        static HANDLE_ERROR_INFO UpdatePSQmfData
  description: updates buffer containing qmf data first/second halve
  returns:     error code of type HANDLE_ERROR_INFO
  input:       - HANDLE_PARAMETRIC_STEREO hParametricStereo
  output:      - HANDLE_PARAMETRIC_STEREO hParametricStereo with updated qmf data
*/

static HANDLE_ERROR_INFO
UpdatePSQmfData_first(HANDLE_PARAMETRIC_STEREO hParametricStereo)
{
  HANDLE_ERROR_INFO error = noError;
  int i, ch;
  for (ch=0; ch<MAX_PS_CHANNELS; ch++) {
    /* get qmf buffers       */
    FIXP_QMF **RESTRICT realQmfData   = hParametricStereo->hPsChannelData[ch]->hPsQmfData->rQmfData + QMF_READ_OFFSET;
    FIXP_QMF **RESTRICT imagQmfData   = hParametricStereo->hPsChannelData[ch]->hPsQmfData->iQmfData + QMF_READ_OFFSET;

    /* get needed parameters */
    INT nCols = hParametricStereo->hPsChannelData[ch]->hPsQmfData->nCols;
    INT nRows = hParametricStereo->hPsChannelData[ch]->hPsQmfData->nRows;

    /* move processed buffer data nCols qmf samples forward */
    for(i=0; i<HYBRID_READ_OFFSET; i++){
      FDKmemcpy (realQmfData[i], realQmfData[i + nCols], sizeof(FIXP_QMF)*nRows );
      FDKmemcpy (imagQmfData[i], imagQmfData[i + nCols], sizeof(FIXP_QMF)*nRows );
    }
  }

  return error;
}

HANDLE_ERROR_INFO
UpdatePSQmfData_second(HANDLE_PARAMETRIC_STEREO hParametricStereo)
{
  HANDLE_ERROR_INFO error = noError;
  int i, ch;
  for (ch=0; ch<MAX_PS_CHANNELS; ch++) {
    /* get qmf buffers       */
    FIXP_QMF **RESTRICT realQmfData = hParametricStereo->hPsChannelData[ch]->hPsQmfData->rQmfData + QMF_READ_OFFSET;
    FIXP_QMF **RESTRICT imagQmfData = hParametricStereo->hPsChannelData[ch]->hPsQmfData->iQmfData + QMF_READ_OFFSET;

    /* get needed parameters */
    INT writeOffset = hParametricStereo->hPsChannelData[ch]->hPsQmfData->bufferWriteOffset;
    INT nCols       = hParametricStereo->hPsChannelData[ch]->hPsQmfData->nCols;
    INT nRows       = hParametricStereo->hPsChannelData[ch]->hPsQmfData->nRows;

    /* move processed buffer data nCols qmf samples forward */
    for(i=HYBRID_READ_OFFSET; i<writeOffset; i++){
      FDKmemcpy (realQmfData[i], realQmfData[i + nCols], sizeof(FIXP_QMF)*nRows );
      FDKmemcpy (imagQmfData[i], imagQmfData[i + nCols], sizeof(FIXP_QMF)*nRows );
    }
  }

  return error;
}



/*
  name:        static HANDLE_ERROR_INFO UpdatePSHybridData
  description: updates buffer containg PS hybrid data
  returns:     error code of type HANDLE_ERROR_INFO
  input:       - HANDLE_PARAMETRIC_STEREO hParametricStereo
  output:      - HANDLE_PARAMETRIC_STEREO hParametricStereo with updated hybrid data
*/

static HANDLE_ERROR_INFO UpdatePSHybridData(HANDLE_PARAMETRIC_STEREO hParametricStereo)
{
  INT i, ch;

  for (ch=0; ch<MAX_PS_CHANNELS; ch++) {
    HANDLE_PS_HYBRID_DATA  hHybData = hParametricStereo->hPsChannelData[ch]->hHybData;
    FIXP_QMF       **realHybridData = hHybData->rHybData + HYBRID_DATA_READ_OFFSET;
    FIXP_QMF       **imagHybridData = hHybData->iHybData + HYBRID_DATA_READ_OFFSET;
    INT              writeOffset    = hHybData->hybDataWriteOffset;
    INT              frameSize      = hHybData->frameSize;

    for(i=0; i<writeOffset; i++){
      FDKmemcpy (realHybridData[i], realHybridData[i + frameSize], sizeof(FIXP_QMF)*HYBRID_NUM_BANDS );
      FDKmemcpy (imagHybridData[i], imagHybridData[i + frameSize], sizeof(FIXP_QMF)*HYBRID_NUM_BANDS );
    }
  }

  return noError;
}


/*
  name:        static HANDLE_ERROR_INFO ExtractPSParameters
  description: PS parameter extraction
  returns:     error code of type HANDLE_ERROR_INFO
  input:       - HANDLE_PARAMETRIC_STEREO hParametricStereo
  output:      - HANDLE_PARAMETRIC_STEREO hParametricStereo PS parameter
*/

static HANDLE_ERROR_INFO
ExtractPSParameters(HANDLE_PARAMETRIC_STEREO hParametricStereo, const int sendHeader){

  HANDLE_ERROR_INFO error = noError;

  if(error == noError){
    if(hParametricStereo == NULL){
      error = ERROR(CDI, "Invalid handle hParametricStereo.");
    }
  }

  /* call ps encode function */
  if(error == noError){
     if (hParametricStereo->initPS){
      *hParametricStereo->hPsOut[1] = *hParametricStereo->hPsOut[0];
     }
    *hParametricStereo->hPsOut[0] = *hParametricStereo->hPsOut[1];

    if(noError != (error = FDKsbrEnc_PSEncode(hParametricStereo->hPsEncode,
                                              hParametricStereo->hPsOut[1],
                                              hParametricStereo->hPsChannelData[0],
                                              hParametricStereo->hPsChannelData[1],
                                              hParametricStereo->dynBandScale,
                                              hParametricStereo->maxEnvelopes,
                                              sendHeader))){
      error = handBack(error);
    }
   if (hParametricStereo->initPS){
    *hParametricStereo->hPsOut[0] = *hParametricStereo->hPsOut[1];
    hParametricStereo->initPS = 0;
   }
  }

  return error;
}


/*
  name:        static HANDLE_ERROR_INFO DownmixPSQmfData
  description: energy weighted downmix and hybrid synthesis
  returns:     error code of type HANDLE_ERROR_INFO
  input:       - HANDLE_PARAMETRIC_STEREO hParametricStereo containing left and right channel qmf data
  output:      - HANDLE_PARAMETRIC_STEREO with updated qmf data buffer, hybrid data buffer
               - FIXP_QMF **mixRealQmfData: pointer to buffer containing downmixed (real) qmf data
               - FIXP_QMF **mixImagQmfData: pointer to buffer containing downmixed (imag) qmf data
*/

static HANDLE_ERROR_INFO
DownmixPSQmfData(HANDLE_PARAMETRIC_STEREO hParametricStereo, FIXP_QMF **RESTRICT mixRealQmfData,
                 FIXP_QMF **RESTRICT mixImagQmfData, SCHAR *downmixScale)
{
  HANDLE_ERROR_INFO error = noError;
  int n, k;
  int dynQmfScale, adjQmfScale;
  int nQmfSamples=0, nQmfBands=0, nHybridQmfBands=0;
  FIXP_QMF **RESTRICT leftRealQmfData        = NULL;
  FIXP_QMF **RESTRICT leftImagQmfData        = NULL;
  FIXP_QMF **RESTRICT rightRealQmfData       = NULL;
  FIXP_QMF **RESTRICT rightImagQmfData       = NULL;
  FIXP_QMF **RESTRICT leftRealHybridQmfData  = NULL;
  FIXP_QMF **RESTRICT leftImagHybridQmfData  = NULL;
  FIXP_QMF **RESTRICT rightRealHybridQmfData = NULL;
  FIXP_QMF **RESTRICT rightImagHybridQmfData = NULL;

  if(hParametricStereo == NULL){
    error = ERROR(CDI, "Invalid handle hParametricStereo.");
  }

  if(error == noError){
    /* Update first part of qmf buffers...
       no whole buffer update possible; downmix is inplace */
    if(noError != (error = UpdatePSQmfData_first(hParametricStereo))){
      error = handBack(error);
    }
  }

  if(error == noError){
    /* get buffers: synchronize QMF buffers and hybrid buffers to compensate hybrid filter delay */
    /* hybrid filter bank looks nHybridFilterDelay qmf samples forward                           */
    leftRealQmfData        = hParametricStereo->hPsChannelData[0]->hPsQmfData->rQmfData + HYBRID_READ_OFFSET;
    leftImagQmfData        = hParametricStereo->hPsChannelData[0]->hPsQmfData->iQmfData + HYBRID_READ_OFFSET;
    rightRealQmfData       = hParametricStereo->hPsChannelData[1]->hPsQmfData->rQmfData + HYBRID_READ_OFFSET;
    rightImagQmfData       = hParametricStereo->hPsChannelData[1]->hPsQmfData->iQmfData + HYBRID_READ_OFFSET;

    leftRealHybridQmfData  = hParametricStereo->hPsChannelData[0]->hHybData->rHybData + HYBRID_WRITE_OFFSET;
    leftImagHybridQmfData  = hParametricStereo->hPsChannelData[0]->hHybData->iHybData + HYBRID_WRITE_OFFSET;
    rightRealHybridQmfData = hParametricStereo->hPsChannelData[1]->hHybData->rHybData + HYBRID_WRITE_OFFSET;
    rightImagHybridQmfData = hParametricStereo->hPsChannelData[1]->hHybData->iHybData + HYBRID_WRITE_OFFSET;

    /* get number of needed parameters */
    nQmfSamples            = hParametricStereo->hPsChannelData[0]->hPsQmfData->nCols;
    nQmfBands              = hParametricStereo->hPsChannelData[0]->hPsQmfData->nRows;
    nHybridQmfBands        = FDKsbrEnc_GetNumberHybridQmfBands(hParametricStereo->hPsChannelData[0]->hHybData);

    /* define scalings */
    adjQmfScale = hParametricStereo->hPsChannelData[0]->hHybData->sf_fixpHybrid
                - hParametricStereo->hPsChannelData[0]->psQmfScale;

    dynQmfScale = fixMax(0, hParametricStereo->dmxScale-1); /* scale one bit more for addition of left and right */

    *downmixScale = hParametricStereo->hPsChannelData[0]->hHybData->sf_fixpHybrid - dynQmfScale + 1;

    const FIXP_DBL maxStereoScaleFactor = FL2FXCONST_DBL(2.0f/2.f);

    for(n = 0; n<nQmfSamples; n++){
      INT hybridDataOffset = 0;

      for(k = 0; k<nQmfBands; k++){
        INT l, nHybridSubBands;
        FIXP_DBL tmpMixReal, tmpMixImag;

        if(k < nHybridQmfBands){
          /* process sub-subbands from hybrid qmf */
          nHybridSubBands = FDKsbrEnc_GetHybridResolution(hParametricStereo->hPsChannelData[0]->hHybData, k);
        } else {
          /* process qmf data */
          nHybridSubBands = 1;
        }

        tmpMixReal = FL2FXCONST_DBL(0.f);
        tmpMixImag = FL2FXCONST_DBL(0.f);

        for(l=0; l<nHybridSubBands; l++) {
          int dynScale, sc; /* scaling */
          FIXP_QMF tmpLeftReal, tmpRightReal, tmpLeftImag, tmpRightImag;
          FIXP_DBL tmpScaleFactor, stereoScaleFactor;

          if(k < nHybridQmfBands){
            /* process sub-subbands from hybrid qmf */
            tmpLeftReal  = (leftRealHybridQmfData[n][hybridDataOffset + l]);
            tmpLeftImag  = (leftImagHybridQmfData[n][hybridDataOffset + l]);
            tmpRightReal = (rightRealHybridQmfData[n][hybridDataOffset + l]);
            tmpRightImag = (rightImagHybridQmfData[n][hybridDataOffset + l]);
            dynScale     = dynQmfScale;
          } else {
            /* process qmf data */
            tmpLeftReal  = leftRealQmfData[n][k];
            tmpLeftImag  = leftImagQmfData[n][k];
            tmpRightReal = rightRealQmfData[n][k];
            tmpRightImag = rightImagQmfData[n][k];
            dynScale     = dynQmfScale-adjQmfScale;
          }

          sc = fixMax(0,CntLeadingZeros( fixMax(fixMax(fixp_abs(tmpLeftReal),fixp_abs(tmpLeftImag)),fixMax(fixp_abs(tmpRightReal),fixp_abs(tmpRightImag))) )-2);

          tmpLeftReal  <<= sc; tmpLeftImag  <<= sc;
          tmpRightReal <<= sc; tmpRightImag <<= sc;
          dynScale = fixMin(sc-dynScale,DFRACT_BITS-1);

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

          /* write data to output */
          tmpMixReal += fMultDiv2(stereoScaleFactor, (FIXP_QMF)(tmpLeftReal + tmpRightReal))>>dynScale;
          tmpMixImag += fMultDiv2(stereoScaleFactor, (FIXP_QMF)(tmpLeftImag + tmpRightImag))>>dynScale;
        }

        mixRealQmfData[n][k] = tmpMixReal;
        mixImagQmfData[n][k] = tmpMixImag;

        hybridDataOffset += nHybridSubBands;
      }
    }
  } /* if(error == noError) */


  if(error == noError){
    /* ... and update the hybrid data */
    if(noError != (error = UpdatePSHybridData(hParametricStereo))){
      error = handBack(error);
    }
  }

  return error;
}


/*
  name:        INT FDKsbrEnc_PSEnc_WritePSData()
  description: writes ps_data() element to bitstream (hBitstream), returns number of written bits;
               returns number of written bits only, if hBitstream == NULL
  returns:     number of bits in ps_data()
  input:       - HANDLE_PARAMETRIC_STEREO hParametricStereo containing extracted ps parameters
  output:      - HANDLE_FDK_BITSTREAM containing ps_data() element
*/

INT
FDKsbrEnc_PSEnc_WritePSData(HANDLE_PARAMETRIC_STEREO hParametricStereo, HANDLE_FDK_BITSTREAM hBitstream)
{

  INT nBitsWritten = 0;

  if(hParametricStereo != NULL){
    nBitsWritten = FDKsbrEnc_WritePSBitstream(hParametricStereo->hPsOut[0], hBitstream);
  }

  return nBitsWritten;
}


/*
  name:        static HANDLE_ERROR_INFO PSHybridAnalysis()
  description: hybrid analysis filter bank of lowest qmf banks
  returns:     error code of type HANDLE_ERROR_INFO
  input:       - HANDLE_PARAMETRIC_STEREO hParametricStereo containing qmf samples
  output:      - HANDLE_PARAMETRIC STEREO hParametricStereo also containing hybrid data
*/

static HANDLE_ERROR_INFO
PSHybridAnalysis(HANDLE_PARAMETRIC_STEREO hParametricStereo){

  HANDLE_ERROR_INFO error = noError;
  int ch;

  if(hParametricStereo == NULL){
    error = ERROR(CDI, "Invalid handle hParametricStereo.");
  }

  for (ch=0; ch<MAX_PS_CHANNELS; ch++) {
    if(error == noError){
      if(noError != (error  = HybridAnalysis(hParametricStereo->hPsChannelData[ch]->hHybAna,
                                             hParametricStereo->hPsChannelData[ch]->hPsQmfData->rQmfData + HYBRID_READ_OFFSET,
                                             hParametricStereo->hPsChannelData[ch]->hPsQmfData->iQmfData + HYBRID_READ_OFFSET,
                                             hParametricStereo->hPsChannelData[ch]->psQmfScale,
                                             hParametricStereo->hPsChannelData[ch]->hHybData->rHybData + HYBRID_WRITE_OFFSET,
                                             hParametricStereo->hPsChannelData[ch]->hHybData->iHybData + HYBRID_WRITE_OFFSET,
                                             &hParametricStereo->hPsChannelData[ch]->hHybData->sf_fixpHybrid))){
        error = handBack(error);
      }
    }
  }

  return error;
}

/*
  name:        HANDLE_ERROR_INFO FDKsbrEnc_PSEnc_ParametricStereoProcessing
  description: Complete PS Processing:
               qmf + hybrid analysis of time domain data (left and right channel),
               PS parameter extraction
               downmix of qmf data
  returns:     error code of type HANDLE_ERROR_INFO
  input:       - HANDLE_PARAMETRIC_STEREO hParametricStereo
  output:      - HANDLE_PARAMETRIC STEREO hParametricStereo containing extracted PS parameters
               - FIXP_DBL **qmfDataReal: Pointer to buffer containing downmixed, real qmf data
               - FIXP_DBL **qmfDataImag: Pointer to buffer containing downmixed, imag qmf data
               - INT_PCM  **downsampledOutSignal: Pointer to buffer containing downmixed time signal
               - SCHAR     *qmfScale: Updated scale value for the QMF downmix data

*/

HANDLE_ERROR_INFO
FDKsbrEnc_PSEnc_ParametricStereoProcessing(HANDLE_PARAMETRIC_STEREO hParametricStereo,
                                           FIXP_QMF **RESTRICT      qmfDataReal,
                                           FIXP_QMF **RESTRICT      qmfDataImag,
                                           INT                      qmfOffset,
                                           INT_PCM                 *downsampledOutSignal,
                                           HANDLE_QMF_FILTER_BANK   sbrSynthQmf,
                                           SCHAR                   *qmfScale,
                                           const int                sendHeader)
{
  HANDLE_ERROR_INFO error = noError;
  FIXP_QMF **downmixedRealQmfData = qmfDataReal+qmfOffset;
  FIXP_QMF **downmixedImagQmfData = qmfDataImag+qmfOffset;
  SCHAR dmScale   = 0;
  INT noQmfBands  = hParametricStereo->noQmfBands;


  if (error == noError) {
    /* do ps hybrid analysis */
        if(noError != (error = PSHybridAnalysis(hParametricStereo))){
      error = handBack(error);
    }
  }

  /* find best scaling in new QMF and Hybrid data */
  psFindBestScaling( hParametricStereo,
                     hParametricStereo->dynBandScale,
                     hParametricStereo->maxBandValue,
                    &hParametricStereo->dmxScale ) ;


  if(error == noError){
    /* extract the ps parameters */
    if(noError != (error = ExtractPSParameters(hParametricStereo, sendHeader))){
      error = handBack(error);
    }
  }

  if(error == noError){
    /* downmix and hybrid synthesis */
    if(noError != (error = DownmixPSQmfData(hParametricStereo, downmixedRealQmfData, downmixedImagQmfData, &dmScale))){
      error = handBack(error);
    }
  }


  if (error == noError)
  {
     C_ALLOC_SCRATCH_START(qmfWorkBuffer, FIXP_DBL, QMF_CHANNELS*2);
    /*

     QMF synthesis including downsampling

    */
    QMF_SCALE_FACTOR tmpScale;
    int scale = -dmScale;
    tmpScale.lb_scale = scale;
    tmpScale.ov_lb_scale = scale;
    tmpScale.hb_scale = scale;
    tmpScale.ov_hb_scale = 0;

    qmfSynthesisFiltering( sbrSynthQmf,
                           downmixedRealQmfData,
                           downmixedImagQmfData,
                          &tmpScale,
                           0,
                           downsampledOutSignal,
                           1,
                           qmfWorkBuffer );

    C_ALLOC_SCRATCH_END(qmfWorkBuffer, FIXP_DBL, QMF_CHANNELS*2);


  }

  /* scaling in sbr module differs -> scaling update */
  *qmfScale = -dmScale  + 7;


  /*
   * Do PS to SBR QMF data transfer/scaling buffer shifting, delay lines etc.
   */
  {
    INT noQmfSlots2 = hParametricStereo->noQmfSlots>>1;

    FIXP_QMF r_tmp1;
    FIXP_QMF i_tmp1;
    FIXP_QMF **delayQmfReal = hParametricStereo->qmfDelayReal;
    FIXP_QMF **delayQmfImag = hParametricStereo->qmfDelayImag;
    INT scale, i, j;

    if (hParametricStereo->qmfDelayScale > *qmfScale) {
      scale = hParametricStereo->qmfDelayScale - *qmfScale;

      for (i=0; i<noQmfSlots2; i++) {
        for (j=0; j<noQmfBands; j++) {
          r_tmp1 = qmfDataReal[i][j];
          i_tmp1 = qmfDataImag[i][j];

          qmfDataReal[i][j] = delayQmfReal[i][j] >> scale;
          qmfDataImag[i][j] = delayQmfImag[i][j] >> scale;
          delayQmfReal[i][j] = qmfDataReal[i+noQmfSlots2][j];
          delayQmfImag[i][j] = qmfDataImag[i+noQmfSlots2][j];
          qmfDataReal[i+noQmfSlots2][j] = r_tmp1;
          qmfDataImag[i+noQmfSlots2][j] = i_tmp1;
        }
      }
      hParametricStereo->qmfDelayScale = *qmfScale;
    }
    else {
      scale = *qmfScale - hParametricStereo->qmfDelayScale;
      for (i=0; i<noQmfSlots2; i++) {
        for (j=0; j<noQmfBands; j++) {
          r_tmp1 = qmfDataReal[i][j];
          i_tmp1 = qmfDataImag[i][j];

          qmfDataReal[i][j] = delayQmfReal[i][j];
          qmfDataImag[i][j] = delayQmfImag[i][j];
          delayQmfReal[i][j] = qmfDataReal[i+noQmfSlots2][j];
          delayQmfImag[i][j] = qmfDataImag[i+noQmfSlots2][j];
          qmfDataReal[i+noQmfSlots2][j] = r_tmp1 >> scale;
          qmfDataImag[i+noQmfSlots2][j] = i_tmp1 >> scale;
        }
      }
      scale = *qmfScale;
      *qmfScale = hParametricStereo->qmfDelayScale;
      hParametricStereo->qmfDelayScale = scale;
    }
  }

  return error;
}

static void psFindBestScaling(HANDLE_PARAMETRIC_STEREO hParametricStereo,
                              UCHAR  *RESTRICT dynBandScale,
                              FIXP_QMF *RESTRICT maxBandValue,
                              SCHAR  *RESTRICT dmxScale)
{
  HANDLE_PS_ENCODE hPsEncode      =  hParametricStereo->hPsEncode;
  HANDLE_PS_HYBRID_DATA hHybDatal =  hParametricStereo->hPsChannelData[0]->hHybData;

  INT group, bin, border, col, band;
  INT frameSize    = FDKsbrEnc_GetHybridFrameSize(hHybDatal); /* same as FDKsbrEnc_GetHybridFrameSize(hHybDatar) */
  INT psBands      = (INT) hPsEncode->psEncMode;
  INT nIidGroups   = hPsEncode->nQmfIidGroups + hPsEncode->nSubQmfIidGroups;

  FIXP_QMF **lr = hParametricStereo->hPsChannelData[0]->hHybData->rHybData;
  FIXP_QMF **li = hParametricStereo->hPsChannelData[0]->hHybData->iHybData;
  FIXP_QMF **rr = hParametricStereo->hPsChannelData[1]->hHybData->rHybData;
  FIXP_QMF **ri = hParametricStereo->hPsChannelData[1]->hHybData->iHybData;
  FIXP_QMF **lrBuffer = hParametricStereo->hPsChannelData[0]->hPsQmfData->rQmfData;
  FIXP_QMF **liBuffer = hParametricStereo->hPsChannelData[0]->hPsQmfData->iQmfData;
  FIXP_QMF **rrBuffer = hParametricStereo->hPsChannelData[1]->hPsQmfData->rQmfData;
  FIXP_QMF **riBuffer = hParametricStereo->hPsChannelData[1]->hPsQmfData->iQmfData;

  /* group wise scaling */
  FIXP_QMF maxVal [2][PS_MAX_BANDS];
  FIXP_QMF maxValue = FL2FXCONST_DBL(0.f);

  INT nHybridQmfOffset = 0;

  UCHAR switched = 0;

  FDKmemclear(maxVal, sizeof(maxVal));

  /* start with hybrid data */
  for (group=0; group < nIidGroups; group++) {
    /* Translate group to bin */
    bin = hPsEncode->subband2parameterIndex[group];

    if (!switched && group == hPsEncode->nSubQmfIidGroups) {
      /* switch to qmf data */
      lr = lrBuffer; li = liBuffer;
      rr = rrBuffer; ri = riBuffer;

      /* calc offset between hybrid subsubbands and qmf bands */
      nHybridQmfOffset = FDKsbrEnc_GetNumberHybridQmfBands(hHybDatal) - FDKsbrEnc_GetNumberHybridBands(hHybDatal);
      switched = 1;
    }

    /* Translate from 20 bins to 10 bins */
    if (hPsEncode->psEncMode == PS_BANDS_COARSE) {
      bin >>= 1;
    }

    /* determine group border */
    border = hPsEncode->iidGroupBorders[group+1];

    /* QMF downmix scaling */
    {
      FIXP_QMF tmp = maxVal[0][bin];
      int i;
      for (col=HYBRID_READ_OFFSET; col<frameSize; col++) {
        FIXP_QMF *pLR = &lr[col][hPsEncode->iidGroupBorders[group] + nHybridQmfOffset];
        FIXP_QMF *pLI = &li[col][hPsEncode->iidGroupBorders[group] + nHybridQmfOffset];
        FIXP_QMF *pRR = &rr[col][hPsEncode->iidGroupBorders[group] + nHybridQmfOffset];
        FIXP_QMF *pRI = &ri[col][hPsEncode->iidGroupBorders[group] + nHybridQmfOffset];
        for (i = 0; i<border-hPsEncode->iidGroupBorders[group]; i++) {
          tmp = fixMax(tmp, (FIXP_QMF)fixp_abs(*pLR++));
          tmp = fixMax(tmp, (FIXP_QMF)fixp_abs(*pLI++));
          tmp = fixMax(tmp, (FIXP_QMF)fixp_abs(*pRR++));
          tmp = fixMax(tmp, (FIXP_QMF)fixp_abs(*pRI++));
        }
      }
      maxVal[0][bin] = tmp;

      tmp = maxVal[1][bin];
      for (col=frameSize; col<HYBRID_READ_OFFSET+frameSize; col++) {
        FIXP_QMF *pLR = &lr[col][hPsEncode->iidGroupBorders[group] + nHybridQmfOffset];
        FIXP_QMF *pLI = &li[col][hPsEncode->iidGroupBorders[group] + nHybridQmfOffset];
        FIXP_QMF *pRR = &rr[col][hPsEncode->iidGroupBorders[group] + nHybridQmfOffset];
        FIXP_QMF *pRI = &ri[col][hPsEncode->iidGroupBorders[group] + nHybridQmfOffset];
        for (i = 0; i<border-hPsEncode->iidGroupBorders[group]; i++) {
          tmp = fixMax(tmp, (FIXP_QMF)fixp_abs(*pLR++));
          tmp = fixMax(tmp, (FIXP_QMF)fixp_abs(*pLI++));
          tmp = fixMax(tmp, (FIXP_QMF)fixp_abs(*pRR++));
          tmp = fixMax(tmp, (FIXP_QMF)fixp_abs(*pRI++));
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

