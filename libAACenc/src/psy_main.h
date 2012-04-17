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
   contents/description: Psychoaccoustic major function block

******************************************************************************/
#ifndef _PSYMAIN_H
#define _PSYMAIN_H


#include "psy_configuration.h"
#include "qc_data.h"
#include "aacenc_pns.h"

/*
  psych internal
*/
typedef struct  {

  PSY_STATIC*        psyStatic[(2)];

}PSY_ELEMENT;

typedef struct  {

  PSY_DATA           psyData[(2)];
  TNS_DATA           tnsData[(2)];
  PNS_DATA           pnsData[(2)];

}PSY_DYNAMIC;


typedef struct  {

  PSY_CONFIGURATION  psyConf[2]; /* LONG / SHORT */
  PSY_ELEMENT*       psyElement[(6)];
  PSY_STATIC*        pStaticChannels[(6)];
  PSY_DYNAMIC*       psyDynamic;
  INT                granuleLength;

}PSY_INTERNAL;


AAC_ENCODER_ERROR FDKaacEnc_PsyNew(PSY_INTERNAL  **phpsy,
                                   const INT       nElements,
                                   const INT       nChannels
                                  ,UCHAR          *dynamic_RAM
                                  );

AAC_ENCODER_ERROR FDKaacEnc_PsyOutNew(PSY_OUT    **phpsyOut,
                                      const INT    nElements,
                                      const INT    nChannels,
                                      const INT    nSubFrames
                                     ,UCHAR       *dynamic_RAM
                                     );

AAC_ENCODER_ERROR FDKaacEnc_psyInit(PSY_INTERNAL    *hPsy,
                                    PSY_OUT        **phpsyOut,
                                    const INT        nSubFrames,
                                    const INT        nMaxChannels,
                                    const AUDIO_OBJECT_TYPE audioObjectType,
                                    CHANNEL_MAPPING *cm);

AAC_ENCODER_ERROR FDKaacEnc_psyMainInit(PSY_INTERNAL *hPsy,
                                        AUDIO_OBJECT_TYPE audioObjectType,
                                        CHANNEL_MAPPING *cm,
                                        INT sampleRate,
                                        INT granuleLength,
                                        INT bitRate,
                                        INT tnsMask,
                                        INT bandwidth,
                                        INT usePns,
                                        INT useIS,
                                        UINT syntaxFlags,
                                        ULONG initFlags);

AAC_ENCODER_ERROR FDKaacEnc_psyMain(INT                 channels,
                                    PSY_ELEMENT         *psyElement,
                                    PSY_DYNAMIC         *psyDynamic,
                                    PSY_CONFIGURATION   *psyConf,
                                    PSY_OUT_ELEMENT     *psyOutElement,
                                    INT_PCM             *pInput,
                                    INT                 *chIdx,
                                    INT                  totalChannels
                                   );

void FDKaacEnc_PsyClose(PSY_INTERNAL   **phPsyInternal,
                        PSY_OUT        **phPsyOut);

#endif /* _PSYMAIN_H */
