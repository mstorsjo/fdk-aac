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
  \brief  Sbr miscellaneous helper functions prototypes $Revision: 36847 $
  \author
*/

#ifndef _SBR_MISC_H
#define _SBR_MISC_H

#include "sbr_encoder.h"

/* Sorting routines */
void FDKsbrEnc_Shellsort_fract (FIXP_DBL *in, INT n);
void FDKsbrEnc_Shellsort_int   (INT *in, INT n);

void FDKsbrEnc_AddLeft (INT *vector, INT *length_vector, INT value);
void FDKsbrEnc_AddRight (INT *vector, INT *length_vector, INT value);
void FDKsbrEnc_AddVecLeft (INT *dst, INT *length_dst, INT *src, INT length_src);
void FDKsbrEnc_AddVecRight (INT *dst, INT *length_vector_dst, INT *src, INT length_src);

FIXP_DBL FDKsbrEnc_LSI_divide_scale_fract(FIXP_DBL num, FIXP_DBL denom, FIXP_DBL scale);

#endif
