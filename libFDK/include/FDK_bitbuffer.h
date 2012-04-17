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
   Description: common bitbuffer read/write routines

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef __FDK_BITBUFFER_H__
#define __FDK_BITBUFFER_H__



#include "machine_type.h"

typedef struct
{
  UINT ValidBits;
  UINT ReadOffset;
  UINT WriteOffset;
  UINT BitCnt;
  UINT BitNdx;

  UCHAR *Buffer;
  UINT   bufSize;
  UINT   bufBits;
} FDK_BITBUF;

typedef FDK_BITBUF *HANDLE_FDK_BITBUF;

#ifdef __cplusplus
extern "C"
{
#endif

extern const UINT BitMask [32+1];

/**  The BitBuffer Functions are called straight from FDK_bitstream Interface.
     For Functions functional survey look there.
*/

void FDK_CreateBitBuffer (HANDLE_FDK_BITBUF *hBitBuffer, UCHAR *pBuffer, UINT bufSize) ;

void FDK_InitBitBuffer   (HANDLE_FDK_BITBUF hBitBuffer, UCHAR *pBuffer,
                          UINT bufSize, UINT validBits) ;

void FDK_ResetBitBuffer  (HANDLE_FDK_BITBUF hBitBuffer) ;

void FDK_DeleteBitBuffer (HANDLE_FDK_BITBUF hBitBuffer) ;

INT  FDK_get             (HANDLE_FDK_BITBUF hBitBuffer, const UINT numberOfBits) ;

INT  FDK_get32           (HANDLE_FDK_BITBUF hBitBuffer) ;

void FDK_put             (HANDLE_FDK_BITBUF hBitBuffer, UINT value, const UINT numberOfBits) ;

INT  FDK_getBwd          (HANDLE_FDK_BITBUF hBitBuffer, const UINT numberOfBits);
void FDK_putBwd          (HANDLE_FDK_BITBUF hBitBuffer, UINT value, const UINT numberOfBits) ;

void FDK_pushBack        (HANDLE_FDK_BITBUF hBitBuffer, const UINT numberOfBits, UCHAR config) ;
void FDK_pushForward     (HANDLE_FDK_BITBUF hBitBuffer, const UINT numberOfBits, UCHAR config) ;

void FDK_byteAlign       (HANDLE_FDK_BITBUF hBitBuffer, UCHAR config) ;

UINT FDK_getValidBits    (HANDLE_FDK_BITBUF hBitBuffer) ;
INT  FDK_getFreeBits     (HANDLE_FDK_BITBUF hBitBuffer) ;

void FDK_setBitCnt       (HANDLE_FDK_BITBUF hBitBuffer, const UINT value) ;
INT  FDK_getBitCnt       (HANDLE_FDK_BITBUF hBitBuffer) ;

void FDK_Feed            (HANDLE_FDK_BITBUF hBitBuffer, UCHAR inputBuffer [],
                          const UINT bufferSize, UINT *bytesValid) ;

void FDK_Copy            (HANDLE_FDK_BITBUF hBitBufDst, HANDLE_FDK_BITBUF hBitBufSrc, UINT *bytesValid) ;

void FDK_Fetch           (HANDLE_FDK_BITBUF hBitBuffer, UCHAR outBuf[], UINT *writeBytes);


#ifdef __cplusplus
}
#endif


#endif
