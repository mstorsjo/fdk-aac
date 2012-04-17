/***************************  Fraunhofer IIS FDK Tools  ***********************

                        (C) Copyright Fraunhofer IIS (2005)
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
   Author(s):   M. Lohwasser
   Description: fixed point abs definitions

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#if !defined(__ABS_H__)
#define __ABS_H__


#if defined(__mips__)	/* cppp replaced: elif */
#include "mips/abs_mips.h"

#elif defined(__x86__)	/* cppp replaced: elif */
#include "x86/abs_x86.h"

#endif /* all cores */

/*************************************************************************
 *************************************************************************
    Software fallbacks for missing functions
**************************************************************************
**************************************************************************/

#if !defined(FUNCTION_fixabs_D)
inline FIXP_DBL fixabs_D(FIXP_DBL x) { return ((x) > (FIXP_DBL)(0)) ? (x) : -(x) ; }
#endif

#if !defined(FUNCTION_fixabs_I)
inline INT fixabs_I(INT x)           { return ((x) > (INT)(0))      ? (x) : -(x) ; }
#endif

#if !defined(FUNCTION_fixabs_S)
inline FIXP_SGL fixabs_S(FIXP_SGL x) { return ((x) > (FIXP_SGL)(0)) ? (x) : -(x) ; }
#endif

#endif /* __ABS_H__ */
