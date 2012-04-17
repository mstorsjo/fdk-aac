/***************************  Fraunhofer IIS FDK Tools  **********************

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
   Author(s):   Marc Gayer
   Description: fixed point intrinsics

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#if !defined(__CLZ_H__)
#define __CLZ_H__

#include "FDK_archdef.h"
#include "machine_type.h"

#if defined(__arm__)
#include "arm/clz_arm.h"

#elif defined(__mips__)	/* cppp replaced: elif */
#include "mips/clz_mips.h"

#endif /* all cores */


/*************************************************************************
 *************************************************************************
    Software fallbacks for missing functions.
**************************************************************************
**************************************************************************/

#if !defined(FUNCTION_fixnormz_S)
#ifdef FUNCTION_fixnormz_D
inline INT fixnormz_S (SHORT a)
{
  return fixnormz_D((INT)(a));
}
#else
inline INT fixnormz_S (SHORT a)
{
    int leadingBits = 0;
    a = ~a;
    while(a & 0x8000) {
      leadingBits++;
      a <<= 1;
    }

    return (leadingBits);
}
#endif
#endif

#if !defined(FUNCTION_fixnormz_D)
inline INT fixnormz_D (LONG a)
{
    INT leadingBits = 0;
    a = ~a;
    while(a & 0x80000000) {
      leadingBits++;
      a <<= 1;
    }

    return (leadingBits);
}
#endif


/*****************************************************************************

    functionname: fixnorm_D
    description:  Count leading ones or zeros of operand val for dfract/LONG INT values.
                  Return this value minus 1. Return 0 if operand==0.
*****************************************************************************/
#if !defined(FUNCTION_fixnorm_S)
#ifdef FUNCTION_fixnorm_D
inline INT fixnorm_S(FIXP_SGL val)
{
  return fixnorm_D((INT)(val));
}
#else
inline INT fixnorm_S(FIXP_SGL val)
{
    INT leadingBits = 0;
    if ( val != (FIXP_SGL)0 ) {
        if ( val < (FIXP_SGL)0 ) {
            val = ~val;
        }
        leadingBits = fixnormz_S(val) - 1;
    }
    return (leadingBits);
}
#endif
#endif

#if !defined(FUNCTION_fixnorm_D)
inline INT fixnorm_D(FIXP_DBL val)
{
    INT leadingBits = 0;
    if ( val != (FIXP_DBL)0 ) {
        if ( val < (FIXP_DBL)0 ) {
            val = ~val;
        }
        leadingBits = fixnormz_D(val) - 1;
    }
    return (leadingBits);
}
#endif

#endif /* __CLZ_H__ */
