/****************************************************************************

                       (C) copyright Fraunhofer-IIS (2004)
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
  \brief  Remaining SBR Bit Writing Routines
*/

#include "env_bit.h"
#include "cmondata.h"


#ifndef min
#define min(a,b) ( a < b ? a:b)
#endif

#ifndef max
#define max(a,b) ( a > b ? a:b)
#endif

/* ***************************** crcAdvance **********************************/
/**
 * @fn
 * @brief    updates crc data register
 * @return   none
 *
 * This function updates the crc register
 *
 */
static void crcAdvance(USHORT crcPoly,
                       USHORT crcMask,
                       USHORT *crc,
                       ULONG   bValue,
                       INT     bBits
                      )
{
  INT i;
  USHORT flag;

  for (i=bBits-1; i>=0; i--) {
    flag = ((*crc) & crcMask) ? (1) : (0) ;
    flag ^= (bValue & (1<<i)) ? (1) : (0) ;

    (*crc)<<=1;
    if(flag) (*crc) ^= crcPoly;
  }
}


/* ***************************** FDKsbrEnc_InitSbrBitstream **********************************/
/**
 * @fn
 * @brief    Inittialisation of sbr bitstream, write of dummy header and CRC
 * @return   none
 *
 *
 *
 */

INT  FDKsbrEnc_InitSbrBitstream(HANDLE_COMMON_DATA  hCmonData,
                                UCHAR              *memoryBase,      /*!< Pointer to bitstream buffer */
                                INT                 memorySize,      /*!< Length of bitstream buffer in bytes */
                                HANDLE_FDK_CRCINFO  hCrcInfo,
                                UINT                sbrSyntaxFlags)  /*!< SBR syntax flags */
{
  INT crcRegion = 0;

  /* reset bit buffer */
  FDKresetBitbuffer(&hCmonData->sbrBitbuf, BS_WRITER);

  FDKinitBitStream(&hCmonData->tmpWriteBitbuf, memoryBase,
                   memorySize, 0, BS_WRITER);

  if (sbrSyntaxFlags & SBR_SYNTAX_CRC) {
    if (sbrSyntaxFlags & SBR_SYNTAX_DRM_CRC)
    { /* Init and start CRC region */
      FDKwriteBits (&hCmonData->sbrBitbuf, 0x0, SI_SBR_DRM_CRC_BITS);
      FDKcrcInit( hCrcInfo, 0x001d, 0xFFFF, SI_SBR_DRM_CRC_BITS );
      crcRegion = FDKcrcStartReg( hCrcInfo, &hCmonData->sbrBitbuf, 0 );
    } else {
      FDKwriteBits (&hCmonData->sbrBitbuf, 0x0, SI_SBR_CRC_BITS);
    }
  }

  return (crcRegion);
}


/* ************************** FDKsbrEnc_AssembleSbrBitstream *******************************/
/**
 * @fn
 * @brief    Formats the SBR payload
 * @return   nothing
 *
 * Also the CRC will be calculated here.
 *
 */

void
FDKsbrEnc_AssembleSbrBitstream( HANDLE_COMMON_DATA  hCmonData,
                                HANDLE_FDK_CRCINFO  hCrcInfo,
                                INT                 crcRegion,
                                UINT                sbrSyntaxFlags)
{
  USHORT crcReg =  SBR_CRCINIT;
  INT numCrcBits,i;

  /* check if SBR is present */
  if ( hCmonData==NULL )
    return;

  hCmonData->sbrFillBits = 0; /* Fill bits are written only for GA streams */

  if ( sbrSyntaxFlags & SBR_SYNTAX_DRM_CRC )
  {
    /*
     * Calculate and write DRM CRC
     */
    FDKcrcEndReg( hCrcInfo, &hCmonData->sbrBitbuf, crcRegion );
    FDKwriteBits( &hCmonData->tmpWriteBitbuf, FDKcrcGetCRC(hCrcInfo)^0xFF, SI_SBR_DRM_CRC_BITS );
  }
  else
  {
    if ( !(sbrSyntaxFlags & SBR_SYNTAX_LOW_DELAY) )
    {
      /* Do alignment here, because its defined as part of the sbr_extension_data */
      int sbrLoad = hCmonData->sbrHdrBits + hCmonData->sbrDataBits;

      if ( sbrSyntaxFlags & SBR_SYNTAX_CRC ) {
        sbrLoad += SI_SBR_CRC_BITS;
      }

      sbrLoad += 4;         /* Do byte Align with 4 bit offset. ISO/IEC 14496-3:2005(E) page 39. */

      hCmonData->sbrFillBits = (8 - (sbrLoad % 8)) % 8;

      /*
        append fill bits
      */
      FDKwriteBits(&hCmonData->sbrBitbuf, 0,  hCmonData->sbrFillBits );

      FDK_ASSERT(FDKgetValidBits(&hCmonData->sbrBitbuf) % 8 == 4);
    }

    /*
      calculate crc
    */
    if ( sbrSyntaxFlags & SBR_SYNTAX_CRC ) {
      FDK_BITSTREAM  tmpCRCBuf = hCmonData->sbrBitbuf;
      FDKresetBitbuffer( &tmpCRCBuf, BS_READER );

      numCrcBits = hCmonData->sbrHdrBits + hCmonData->sbrDataBits + hCmonData->sbrFillBits;

      for(i=0;i<numCrcBits;i++){
        INT bit;
        bit = FDKreadBits(&tmpCRCBuf,1);
        crcAdvance(SBR_CRC_POLY,SBR_CRC_MASK,&crcReg,bit,1);
      }
      crcReg &= (SBR_CRC_RANGE);

      /*
       * Write CRC data.
       */
      FDKwriteBits (&hCmonData->tmpWriteBitbuf, crcReg, SI_SBR_CRC_BITS);
    }
  }

  FDKsyncCache(&hCmonData->tmpWriteBitbuf);
}

