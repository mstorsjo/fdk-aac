/**************************  Fraunhofer IIS FDK SysLib  **********************

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
   Author(s):
   Description: string conversion functions

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/



#include "genericStds.h"
#include "conv_string.h"

INT charBuf2HexString(char *string, UCHAR *charBuf, INT charBufLength)
{
  INT i;
  UCHAR c1, c2;

  /* sanity checks */
  /* check array length */
  if (charBufLength == 0) {
    return -1;
  }

  /* define hex string Table */
  UCHAR hexSymb[16]={'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

  /* calculate corresponding hex string from charBuffer */
  for (i=0;i<charBufLength;i++) {
    c1 = ((charBuf[i])>>4)&0x0f; /* upper nibble */
    c2 = (charBuf[i])&0x0f;      /* lower nibble */

    string[i*2]   = hexSymb[c1];  /* convert to string */
    string[i*2+1] = hexSymb[c2];  /* convert to string */
  }

  /* terminate string */
  string[charBufLength<<1]='\0';

  return 0;

}

INT hexString2CharBuf(const char *string, UCHAR *charBuf, UINT charBufLength)
{
  UINT i, k = 0;
  UCHAR hNibble, lNibble;

  /* sanity checks */
  if (string[0] == '\0') {
    return -1; /* invalid string size */
  }

  if (charBufLength<=0){
    return -2; /* invalid buffer size */
  }

  /* convert to hex characters to corresponding 8bit value */
  for (i=0;(string[i]!='\0')&&((i>>1)<charBufLength);i+=2) {
    k = i>>1;
    hNibble = hexChar2Dec(string[i]);
    lNibble = hexChar2Dec(string[i+1]);
    if ((hNibble == 16) || (lNibble == 16)) {
      return -3; /* invalid character */
    }
    charBuf[k] = ((hNibble<<4)&0xf0) + lNibble;
  }

  /* check if last character was string terminator */
  if ((string[i-2]!=0) && (string[i]!=0)) {
    return -1; /* invalid string size */
  }

  /* fill charBuffer with zeros */
  for (i=k+1;i<charBufLength;i++) {
    charBuf[i] = 0;
  }

  return 0;

}

UCHAR hexChar2Dec(const char c)
{
  INT r = 0;
  if ((c >= '0') && (c <= '9'))
    r = c-'0';
  else if ((c >= 'a') && (c <= 'f'))
    r = c-'a'+10;
  else if ((c >= 'A') && (c <= 'F'))
    r = c-'A'+10;
  else
    r = 16; /* invalid hex character */

  return (UCHAR)r;
}

