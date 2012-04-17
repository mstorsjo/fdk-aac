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
  \file
  \brief  Envelope decoding $Revision: 36841 $
*/
#ifndef __ENV_DEC_H
#define __ENV_DEC_H

#include "sbrdecoder.h"
#include "env_extr.h"

void decodeSbrData (HANDLE_SBR_HEADER_DATA hHeaderData,
                    HANDLE_SBR_FRAME_DATA h_data_left,
                    HANDLE_SBR_PREV_FRAME_DATA h_prev_data_left,
                    HANDLE_SBR_FRAME_DATA h_data_right,
                    HANDLE_SBR_PREV_FRAME_DATA h_prev_data_right);


#endif
