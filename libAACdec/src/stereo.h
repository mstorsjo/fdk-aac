/*****************************  MPEG-4 AAC Decoder  **************************

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


   $Id$
   Author(s):   Josef Hoepfl
   Description: joint stereo processing

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef STEREO_H
#define STEREO_H



#include "machine_type.h"
#include "FDK_bitstream.h"

enum
{
  JointStereoMaximumGroups = 8,
  JointStereoMaximumBands = 64
};

typedef struct
{
  UCHAR MsMaskPresent;
  UCHAR MsUsed[JointStereoMaximumBands]; /*!< every arry element contains flags for up to 8 groups */
} CJointStereoData;


/*!
  \brief Read joint stereo data from bitstream

  The function reads joint stereo data from bitstream.

  \param bs bit stream handle data source.
  \param pJointStereoData pointer to stereo data structure to receive decoded data.
  \param windowGroups number of window groups.
  \param scaleFactorBandsTransmitted number of transmitted scalefactor bands.
  \param flags decoder flags

  \return  0 on success, -1 on error.
*/
int CJointStereo_Read(
        HANDLE_FDK_BITSTREAM bs,
        CJointStereoData *pJointStereoData,
        int windowGroups,
        int scaleFactorBandsTransmitted,
        UINT flags
        );


#endif /* #ifndef STEREO_H */
