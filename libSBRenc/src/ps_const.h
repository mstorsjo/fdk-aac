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
   Initial author:       N. Rettelbach
   contents/description: Parametric Stereo constants

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/
#ifndef PS_CONST_H
#define PS_CONST_H

typedef enum {
  PS_RES_COARSE  = 0,
  PS_RES_MID     = 1,
  PS_RES_FINE    = 2
} PS_RESOLUTION;

typedef enum {
  PS_BANDS_COARSE  = 10,
  PS_BANDS_MID     = 20,
  PS_BANDS_FINE    = 34,
  PS_MAX_BANDS     = PS_BANDS_FINE
} PS_BANDS;

typedef enum {
  PS_IID_RES_COARSE=0,
  PS_IID_RES_FINE
} PS_IID_RESOLUTION;

typedef enum {
  PS_ICC_ROT_A=0,
  PS_ICC_ROT_B
} PS_ICC_ROTATION_MODE;

typedef enum {
  PS_DELTA_FREQ,
  PS_DELTA_TIME
} PS_DELTA;


typedef enum {
  PS_MAX_ENVELOPES = 4

} PS_CONSTS;

#endif
