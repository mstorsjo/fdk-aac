/***************************  Fraunhofer IIS FDK Tools  ***********************

                        (C) Copyright Fraunhofer IIS (2009)
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
   Description: bitstream interface to bitbuffer routines

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef __FDK_BITSTREAM_H__
#define __FDK_BITSTREAM_H__



#include "FDK_bitbuffer.h"
#include "machine_type.h"

#include "genericStds.h"

#define CACHE_BITS 32

typedef enum {
    BS_READER,
    BS_WRITER
} FDK_BS_CFG;


typedef struct
{
  UINT       CacheWord ;
  UINT       BitsInCache ;
  FDK_BITBUF hBitBuf;
  UINT       ConfigCache ;
} FDK_BITSTREAM;

typedef FDK_BITSTREAM* HANDLE_FDK_BITSTREAM;

/**
 * \brief CreateBitStream Function.
 *
 * Create and initialize bitstream with extern allocated buffer.
 *
 * \param pBuffer  Pointer to BitBuffer array.
 * \param bufSize  Length of BitBuffer array. (awaits size 2^n)
 * \param config   Initialize BitStream as Reader or Writer.
 */
FDK_INLINE
HANDLE_FDK_BITSTREAM FDKcreateBitStream (UCHAR *pBuffer,
                                      UINT bufSize,
                                      FDK_BS_CFG config = BS_READER)
{
  HANDLE_FDK_BITSTREAM hBitStream = (HANDLE_FDK_BITSTREAM) FDKcalloc(1, sizeof(FDK_BITSTREAM));
  FDK_InitBitBuffer(&hBitStream->hBitBuf, pBuffer, bufSize, 0) ;

  /* init cache */
  hBitStream->CacheWord = hBitStream->BitsInCache = 0 ;
  hBitStream->ConfigCache = config ;

  return hBitStream ;
}


/**
 * \brief Initialize BistreamBuffer. BitBuffer can point to filled BitBuffer array .
 *
 * \param hBitStream HANDLE_FDK_BITSTREAM handle
 * \param pBuffer    Pointer to BitBuffer array.
 * \param bufSize    Length of BitBuffer array. (awaits size 2^n)
 * \param validBits  Number of valid BitBuffer filled Bits.
 * \param config     Initialize BitStream as Reader or Writer.
 * \return void
 */
FDK_INLINE
void FDKinitBitStream (HANDLE_FDK_BITSTREAM hBitStream,
                       UCHAR *pBuffer,
                       UINT bufSize,
                       UINT validBits,
                       FDK_BS_CFG config = BS_READER)
{
  FDK_InitBitBuffer(&hBitStream->hBitBuf, pBuffer, bufSize, validBits) ;

  /* init cache */
  hBitStream->CacheWord = hBitStream->BitsInCache = 0 ;
  hBitStream->ConfigCache = config ;
}


/**
 * \brief ResetBitbuffer Function. Reset states in BitBuffer and Cache.
 *
 * \param hBitStream HANDLE_FDK_BITSTREAM handle
 * \param config     Initialize BitStream as Reader or Writer.
 * \return void
 */
FDK_INLINE void FDKresetBitbuffer( HANDLE_FDK_BITSTREAM hBitStream, FDK_BS_CFG config = BS_READER)
{
  FDK_ResetBitBuffer( &hBitStream->hBitBuf ) ;

  /* init cache */
  hBitStream->CacheWord = hBitStream->BitsInCache = 0 ;
  hBitStream->ConfigCache = config ;
}


/** DeleteBitStream.

    Deletes the in Create Bitstream allocated BitStream and BitBuffer.
*/
FDK_INLINE void FDKdeleteBitStream (HANDLE_FDK_BITSTREAM hBitStream)
{
  FDK_DeleteBitBuffer(&hBitStream->hBitBuf) ;
  FDKfree(hBitStream) ;
}


/**
 * \brief ReadBits Function (forward). This function returns a number of sequential
 *        bits from the input bitstream.
 *
 * \param hBitStream HANDLE_FDK_BITSTREAM handle
 * \param numberOfBits  The number of bits to be retrieved.
 * \return the requested bits, right aligned
 * \return
 */
#define OPTIMIZE_FDKREADBITS

FDK_INLINE UINT FDKreadBits(HANDLE_FDK_BITSTREAM hBitStream,
                        const UINT numberOfBits)
{
#ifdef noOPTIMIZE_FDKREADBITS
  INT missingBits = numberOfBits - hBitStream->BitsInCache;
  if (missingBits > 0)
  {
    UINT bits = hBitStream->CacheWord << missingBits;
    hBitStream->CacheWord = FDK_get32 (&hBitStream->hBitBuf) ;
    hBitStream->BitsInCache = CACHE_BITS - missingBits;
    return ( bits | (hBitStream->CacheWord >> hBitStream->BitsInCache)) & BitMask[numberOfBits];
  }

  hBitStream->BitsInCache -= numberOfBits;
  return ( hBitStream->CacheWord >> hBitStream->BitsInCache) & BitMask[numberOfBits];

#else
  const UINT validMask = BitMask [numberOfBits] ;

  if (hBitStream->BitsInCache <= numberOfBits)
  {
    const INT freeBits = (CACHE_BITS-1) - hBitStream->BitsInCache ;

    hBitStream->CacheWord = (hBitStream->CacheWord << freeBits) | FDK_get (&hBitStream->hBitBuf,freeBits) ;
    hBitStream->BitsInCache += freeBits ;
  }

  hBitStream->BitsInCache -= numberOfBits ;

  return (hBitStream->CacheWord >> hBitStream->BitsInCache) & validMask ;
#endif
}

FDK_INLINE UINT FDKreadBit(HANDLE_FDK_BITSTREAM hBitStream)
{
#ifdef OPTIMIZE_FDKREADBITS
  if (!hBitStream->BitsInCache)
  {
    hBitStream->CacheWord = FDK_get32 (&hBitStream->hBitBuf);
    hBitStream->BitsInCache = CACHE_BITS;
  }
  hBitStream->BitsInCache--;

  return (hBitStream->CacheWord >> hBitStream->BitsInCache) & 1;
#else
 return FDKreadBits(hBitStream,1);
#endif
}

/**
 * \brief Read2Bits Function (forward). This function 2 sequential
 *        bits from the input bitstream. It is the optimized version
          of FDKreadBits() for readign 2 bits.
 *
 * \param hBitStream HANDLE_FDK_BITSTREAM handle
 * \return the requested bits, right aligned
 * \return
 */
inline UINT FDKread2Bits(HANDLE_FDK_BITSTREAM hBitStream)
{
  UINT BitsInCache = hBitStream->BitsInCache;
  if (BitsInCache < 2)  /* Comparison changed from 'less-equal' to 'less' */
  {
    const INT freeBits = (CACHE_BITS-1) - BitsInCache ;

    hBitStream->CacheWord = (hBitStream->CacheWord << freeBits) | FDK_get (&hBitStream->hBitBuf,freeBits) ;
    BitsInCache += freeBits;
  }
  hBitStream->BitsInCache = BitsInCache - 2;
  return (hBitStream->CacheWord >> hBitStream->BitsInCache) & 0x3;
}

/**
 * \brief ReadBits Function (backward). This function returns a number of sequential bits
 *        from the input bitstream.
 *
 * \param hBitStream HANDLE_FDK_BITSTREAM handle
 * \param numberOfBits  The number of bits to be retrieved.
 * \return the requested bits, right aligned
 */
FDK_INLINE UINT FDKreadBitsBwd(HANDLE_FDK_BITSTREAM hBitStream,
                           const UINT numberOfBits)
{
  const UINT validMask = BitMask [numberOfBits] ;

  if (hBitStream->BitsInCache <= numberOfBits)
  {
    const INT freeBits = (CACHE_BITS-1) - hBitStream->BitsInCache ;

    hBitStream->CacheWord = (hBitStream->CacheWord << freeBits) | FDK_getBwd (&hBitStream->hBitBuf,freeBits) ;
    hBitStream->BitsInCache += freeBits ;
  }

  hBitStream->BitsInCache -= numberOfBits ;

  return (hBitStream->CacheWord >> hBitStream->BitsInCache) & validMask ;
}


/**
 * \brief return a number of bits from the bitBuffer.
 *        You have to know what you do! Cache has to be synchronized before using this
 *        function.
 *
 * \param hBitStream HANDLE_FDK_BITSTREAM handle
 * \param numBits The number of bits to be retrieved.
 * \return the requested bits, right aligned
 */
FDK_INLINE UINT FDKgetBits (HANDLE_FDK_BITSTREAM hBitStream, UINT numBits)
{
    return FDK_get (&hBitStream->hBitBuf, numBits) ;
}


/**
 * \brief WriteBits Function. This function writes numberOfBits of value into bitstream.
 *
 * \param hBitStream HANDLE_FDK_BITSTREAM handle
 * \param value         Variable holds data to be written.
 * \param numberOfBits  The number of bits to be written.
 * \return number of bits written
 */
FDK_INLINE UCHAR FDKwriteBits(HANDLE_FDK_BITSTREAM hBitStream, UINT value,
                     const UINT numberOfBits)
{
  const UINT validMask = BitMask [numberOfBits] ;

  if ((hBitStream->BitsInCache+numberOfBits) < CACHE_BITS)
  {
    hBitStream->BitsInCache += numberOfBits ;
    hBitStream->CacheWord   =  (hBitStream->CacheWord << numberOfBits) | (value & validMask);
  }
  else
  {
    FDK_put(&hBitStream->hBitBuf, hBitStream->CacheWord, hBitStream->BitsInCache) ;
    hBitStream->BitsInCache = numberOfBits ;
    hBitStream->CacheWord   = (value & validMask) ;
  }

 return numberOfBits;
}


/**
 * \brief WriteBits Function (backward). This function writes numberOfBits of value into bitstream.
 *
 * \param hBitStream HANDLE_FDK_BITSTREAM handle
 * \param value         Variable holds data to be written.
 * \param numberOfBits  The number of bits to be written.
 * \return number of bits written
 */
FDK_INLINE UCHAR FDKwriteBitsBwd(HANDLE_FDK_BITSTREAM hBitStream, UINT value,
                                 const UINT numberOfBits)
{
  const UINT validMask = BitMask [numberOfBits] ;

  if ((hBitStream->BitsInCache+numberOfBits) <= CACHE_BITS)
  {
    hBitStream->BitsInCache += numberOfBits ;
    hBitStream->CacheWord   =  (hBitStream->CacheWord << numberOfBits) | (value & validMask);
  }
  else
  {
    FDK_putBwd(&hBitStream->hBitBuf, hBitStream->CacheWord, hBitStream->BitsInCache) ;
    hBitStream->BitsInCache = numberOfBits ;
    hBitStream->CacheWord   = (value & validMask) ;
  }

 return numberOfBits;
}


/**
 * \brief SyncCache Function. Clear cache after read forward.
 *
 * \param hBitStream HANDLE_FDK_BITSTREAM handle
 * \return void
 */
FDK_INLINE void FDKsyncCache (HANDLE_FDK_BITSTREAM hBitStream)
{
  if (hBitStream->ConfigCache == BS_READER)
    FDK_pushBack (&hBitStream->hBitBuf,hBitStream->BitsInCache,hBitStream->ConfigCache) ;
  else                        /* BS_WRITER */
    FDK_put(&hBitStream->hBitBuf, hBitStream->CacheWord, hBitStream->BitsInCache) ;

  hBitStream->BitsInCache = 0 ;
  hBitStream->CacheWord   = 0 ;
}


/**
 * \brief SyncCache Function. Clear cache after read backwards.
 *
 * \param  hBitStream HANDLE_FDK_BITSTREAM handle
 * \return void
 */
FDK_INLINE void FDKsyncCacheBwd (HANDLE_FDK_BITSTREAM hBitStream)
{
  if (hBitStream->ConfigCache == BS_READER) {
    FDK_pushForward (&hBitStream->hBitBuf,hBitStream->BitsInCache,hBitStream->ConfigCache) ;
  } else {                      /* BS_WRITER */
    FDK_putBwd (&hBitStream->hBitBuf, hBitStream->CacheWord, hBitStream->BitsInCache) ;
  }

  hBitStream->BitsInCache = 0 ;
  hBitStream->CacheWord   = 0 ;
}


/**
 * \brief Byte Alignment Function.
 *        This function performs the byte_alignment() syntactic function on the input stream,
 *        i.e. some bits will be discarded/padded so that the next bits to be read/written will
 *        be aligned on a byte boundary with respect to the bit position 0.
 *
 * \param  hBitStream HANDLE_FDK_BITSTREAM handle
 * \return void
 */
FDK_INLINE void FDKbyteAlign (HANDLE_FDK_BITSTREAM hBitStream)
{
  FDKsyncCache (hBitStream) ;
  FDK_byteAlign (&hBitStream->hBitBuf, (UCHAR)hBitStream->ConfigCache) ;
}


/**
 * \brief Byte Alignment Function with anchor
 *        This function performs the byte_alignment() syntactic function on the input stream,
 *        i.e. some bits will be discarded so that the next bits to be read/written would be aligned
 *        on a byte boundary with respect to the given alignment anchor.
 *
 * \param hBitStream HANDLE_FDK_BITSTREAM handle
 * \param alignmentAnchor bit position to be considered as origin for byte alignment
 * \return void
 */
FDK_INLINE void FDKbyteAlign (HANDLE_FDK_BITSTREAM hBitStream, UINT alignmentAnchor)
{
  FDKsyncCache (hBitStream) ;
  if (hBitStream->ConfigCache == BS_READER) {
    FDK_pushForward (&hBitStream->hBitBuf,
                     (8 - ((alignmentAnchor - FDK_getValidBits(&hBitStream->hBitBuf)) & 0x07)) & 0x07,
                     hBitStream->ConfigCache) ;
  }
  else {
    FDK_put (&hBitStream->hBitBuf,
             0,
             (8 - ((FDK_getValidBits(&hBitStream->hBitBuf)-alignmentAnchor) & 0x07)) & 0x07 );
  }
}


/**
 * \brief Push Back(Cache) / For / BiDirectional Function.
 *        PushBackCache function ungets a number of bits erroneously read/written by the last Get() call.
 *       NB: The number of bits to be stuffed back into the stream may never exceed the
 *       number of bits returned by the immediately preceding Get() call.
 *
 *       PushBack function ungets a number of bits (combines cache and bitbuffer indices)
 *       PushFor  function gets a number of bits (combines cache and bitbuffer indices)
 *       PushBiDirectional gets/ungets number of bits as defined in PusBack/For function
 *       NB: The sign of bits is not known, so the function checks direction and calls
 *        appropriate function. (positive sign pushFor, negative sign pushBack )
 *
 * \param hBitStream HANDLE_FDK_BITSTREAM handle
 * \param numberOfBits  The number of bits to be pushed back/for.
 * \return void
 */
FDK_INLINE void FDKpushBackCache (HANDLE_FDK_BITSTREAM hBitStream, const UINT numberOfBits)
{
  FDK_ASSERT ((hBitStream->BitsInCache+numberOfBits)<=CACHE_BITS);
  hBitStream->BitsInCache += numberOfBits ;
}

FDK_INLINE void FDKpushBack (HANDLE_FDK_BITSTREAM hBitStream, const UINT numberOfBits)
{
  if ((hBitStream->BitsInCache+numberOfBits)<CACHE_BITS && (hBitStream->ConfigCache == BS_READER) ) {
    hBitStream->BitsInCache += numberOfBits ;
    FDKsyncCache(hBitStream) ; /* sync cache to avoid invalid cache */
  }
  else {
    FDKsyncCache(hBitStream) ;
    FDK_pushBack(&hBitStream->hBitBuf,numberOfBits,hBitStream->ConfigCache);
  }
}

FDK_INLINE void FDKpushFor (HANDLE_FDK_BITSTREAM hBitStream, const UINT numberOfBits)
{
  if ( (hBitStream->BitsInCache>numberOfBits) && (hBitStream->ConfigCache == BS_READER) ) {
    hBitStream->BitsInCache -= numberOfBits;
  }
  else {
    FDKsyncCache(hBitStream) ;
    FDK_pushForward(&hBitStream->hBitBuf,numberOfBits,hBitStream->ConfigCache);
  }
}

FDK_INLINE void FDKpushBiDirectional (HANDLE_FDK_BITSTREAM hBitStream, const INT numberOfBits)
{
  if(numberOfBits>=0)  FDKpushFor(hBitStream, numberOfBits) ;
  else                 FDKpushBack(hBitStream, -numberOfBits) ;
}


/**
 * \brief GetValidBits Function.  Clear cache and return valid Bits from Bitbuffer.
 * \param hBitStream HANDLE_FDK_BITSTREAM handle
 * \return amount of valid bits that still can be read or were already written.
 *
 */
FDK_INLINE UINT FDKgetValidBits (HANDLE_FDK_BITSTREAM hBitStream)
{
  FDKsyncCache(hBitStream) ;
  return FDK_getValidBits(&hBitStream->hBitBuf) ;
}


/**
 * \brief return amount of unused Bits from Bitbuffer.
 * \param hBitStream HANDLE_FDK_BITSTREAM handle
 * \return amount of free bits that still can be written into the bitstream
 */
FDK_INLINE INT FDKgetFreeBits (HANDLE_FDK_BITSTREAM hBitStream)
{
   return FDK_getFreeBits (&hBitStream->hBitBuf) ;
}

/**
 * \brief reset bitcounter in bitBuffer to zero.
 * \param hBitStream HANDLE_FDK_BITSTREAM handle
 * \return void
 */
FDK_INLINE void FDKresetBitCnt (HANDLE_FDK_BITSTREAM hBitStream)
{
    FDKsyncCache (hBitStream) ;
    FDK_setBitCnt (&hBitStream->hBitBuf, 0) ;
}

/**
 * \brief set bitcoutner in bitBuffer to given value.
 * \param hBitStream HANDLE_FDK_BITSTREAM handle
 * \param value new value to be assigned to the bit counter
 * \return void
 */
FDK_INLINE void FDKsetBitCnt (HANDLE_FDK_BITSTREAM hBitStream, UINT value)
{
    FDKsyncCache (hBitStream) ;
    FDK_setBitCnt (&hBitStream->hBitBuf, value) ;
}

/**
 * \brief get bitcounter state from bitBuffer.
 * \param hBitStream HANDLE_FDK_BITSTREAM handle
 * \return current bit counter value
 */
FDK_INLINE INT  FDKgetBitCnt (HANDLE_FDK_BITSTREAM hBitStream)
{
    FDKsyncCache(hBitStream) ;
    return FDK_getBitCnt(&hBitStream->hBitBuf) ;
}


/**
 * \brief Fill the BitBuffer with a number of input bytes from  external source.
 *        The bytesValid variable returns the number of ramaining valid bytes in extern inputBuffer.
 *
 * \param hBitStream  HANDLE_FDK_BITSTREAM handle
 * \param inputBuffer Pointer to input buffer with bitstream data.
 * \param bufferSize  Total size of inputBuffer array.
 * \param bytesValid  Input: number of valid bytes in inputBuffer. Output: bytes still left unread in inputBuffer.
 * \return void
 */
FDK_INLINE void FDKfeedBuffer (HANDLE_FDK_BITSTREAM hBitStream, const UCHAR inputBuffer [], const UINT bufferSize, UINT *bytesValid)
{
  FDKsyncCache (hBitStream) ;
  FDK_Feed(&hBitStream->hBitBuf, (UCHAR*)inputBuffer, bufferSize, bytesValid  ) ;
}


/**
 * \brief fill destination BitBuffer with a number of bytes from source BitBuffer. The
 *        bytesValid variable returns the number of ramaining valid bytes in source BitBuffer.
 *
 * \param hBSDst            HANDLE_FDK_BITSTREAM handle to write data into
 * \param hBSSrc            HANDLE_FDK_BITSTREAM handle to read data from
 * \param bytesValid        Input: number of valid bytes in inputBuffer. Output: bytes still left unread in inputBuffer.
 * \return void
 */
FDK_INLINE void FDKcopyBuffer (HANDLE_FDK_BITSTREAM hBSDst, HANDLE_FDK_BITSTREAM hBSSrc, UINT *bytesValid)
{
  FDKsyncCache (hBSSrc) ;
  FDK_Copy (&hBSDst->hBitBuf, &hBSSrc->hBitBuf, bytesValid) ;
}


/**
 * \brief fill the outputBuffer with all valid bytes hold in BitBuffer. The WriteBytes
 *        variable returns the number of written Bytes.
 *
 * \param hBitStream    HANDLE_FDK_BITSTREAM handle
 * \param outputBuffer  Pointer to output buffer.
 * \param writeBytes    Number of bytes write to output buffer.
 * \return void
 */
FDK_INLINE void FDKfetchBuffer(HANDLE_FDK_BITSTREAM hBitStream, UCHAR *outputBuffer, UINT *writeBytes)
{
  FDKsyncCache (hBitStream) ;
  FDK_Fetch(&hBitStream->hBitBuf, outputBuffer, writeBytes);
}


#endif
