/***************************  Fraunhofer IIS FDK Tools  **********************

                        (C) Copyright Fraunhofer IIS (2006)
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
   Author(s):   M. Lohwasser, M. Gayer
   Description: min/max inline functions and defines

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef FIXMINMAX__H
#define FIXMINMAX__H

#include "FDK_archdef.h"
#include "machine_type.h"

/* Inline Function to determine the smaller/bigger value of two values with same type. */


template <class T> inline T fixmin (T a, T b)
{
	return (a < b ? a : b);
}

template <class T> inline T fixmax (T a, T b)
{
	return (a > b ? a : b);
}

#define fixmax_D(a,b) fixmax(a,b)
#define fixmin_D(a,b) fixmin(a,b)
#define fixmax_S(a,b) fixmax(a,b)
#define fixmin_S(a,b) fixmin(a,b)
#define fixmax_I(a,b) fixmax(a,b)
#define fixmin_I(a,b) fixmin(a,b)
#define fixmax_UI(a,b) fixmax(a,b)
#define fixmin_UI(a,b) fixmin(a,b)


#endif
