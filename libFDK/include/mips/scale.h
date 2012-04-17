/***************************  Fraunhofer IIS FDK Tools  **********************

                        (C) Copyright Fraunhofer IIS (2007)
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
   Author(s):
   Description:

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef MIPS_SCALE_H
#define MIPS_SCALE_H

#if defined(__mips_dsp)

/*!
*
*  \brief  Scale input value by 2^{scale} and saturate output to 2^{dBits-1}
*  \return scaled and saturated value
*
*  This macro scales src value right or left and applies saturation to (2^dBits)-1
*  maxima output.
*/
#define SATURATE_RIGHT_SHIFT(src, scale, dBits)       \
          (__builtin_mips_shll_s_w((src)>>scale,(DFRACT_BITS-(dBits)))>>(DFRACT_BITS-(dBits)))


#endif /*__mips_dsp */

#endif /* MIPS_SCALE_H */
