/****************************************************************************

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


   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

 $Id$

*******************************************************************************/
/*!
\file
\brief Sbr scaling factors, $Revision: 36841 $
To deal with the dynamic range in the different processing stages, a
fixed point specific code has to rely on scaling factors. A floating
point code carries a scaling factor -- the exponent -- for each value,
so scaling is not necessary there.

The output of the core decoder (low band) is scaled up to cover as much
as possible bits for each value. As high band and low band are processed
in different algorithm sections, they require their own scaling
factors. In addition, any static buffers, e.g. filter states, require a
separate scaling factor as well. The code takes care to do the proper
adjustment, if scaling factors of a filter state and the time signal differ.

\sa #QMF_SCALE_FACTOR, \ref documentationOverview
*/

#ifndef __SBR_SCALE_H
#define __SBR_SCALE_H

/*!
\verbatim
  scale:
    0             left aligned e.g. |max| >=0.5
    FRACT_BITS-1  zero e.g |max| = 0
\endverbatim

  Dynamic scaling is used to achieve sufficient accuracy even when the signal
  energy is low.  The dynamic framing of SBR produces a variable overlap area
  where samples from the previous QMF-Analysis are stored. Depending on the
  start position and stop position of the current SBR envelopes, the processing
  buffer consists of differently scaled regions like illustrated in the below
  figure.

  \image html scales.png Scale
*/


#endif
