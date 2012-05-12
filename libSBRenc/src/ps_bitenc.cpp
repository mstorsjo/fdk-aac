/*****************************  MPEG Audio Encoder  ***************************

                     (C) Copyright Fraunhofer IIS (2004-2005)
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
   Initial author:       N. Rettelbach
   contents/description: Parametric Stereo bitstream encoder

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "ps_main.h"


#include "ps_const.h"
#include "ps_bitenc.h"

static
inline UCHAR FDKsbrEnc_WriteBits_ps(HANDLE_FDK_BITSTREAM hBitStream, UINT value,
                                    const UINT numberOfBits)
{
  /* hBitStream == NULL happens here intentionally */
  if(hBitStream!=NULL){
    FDKwriteBits(hBitStream, value, numberOfBits);
  }
  return numberOfBits;
}

#define SI_SBR_EXTENSION_SIZE_BITS              4
#define SI_SBR_EXTENSION_ESC_COUNT_BITS         8
#define SI_SBR_EXTENSION_ID_BITS                2
#define EXTENSION_ID_PS_CODING                  2
#define PS_EXT_ID_V0                            0

static const INT iidDeltaCoarse_Offset = 14;
static const INT iidDeltaCoarse_MaxVal = 28;
static const INT iidDeltaFine_Offset = 30;
static const INT iidDeltaFine_MaxVal = 60;

/* PS Stereo Huffmantable: iidDeltaFreqCoarse */
static const UINT iidDeltaFreqCoarse_Length[] =
{
  17,    17,    17,    17,    16,  15,    13,    10,     9,     7,
   6,     5,     4,     3,     1,   3,     4,     5,     6,     6,
   8,    11,    13,    14,    14,  15,    17,    18,    18
};
static const UINT iidDeltaFreqCoarse_Code[]  =
{
  0x0001fffb,  0x0001fffc,  0x0001fffd,  0x0001fffa,  0x0000fffc,  0x00007ffc,  0x00001ffd,  0x000003fe,  0x000001fe,  0x0000007e,
  0x0000003c,  0x0000001d,  0x0000000d,  0x00000005,  0000000000,  0x00000004,  0x0000000c,  0x0000001c,  0x0000003d,  0x0000003e,
  0x000000fe,  0x000007fe,  0x00001ffc,  0x00003ffc,  0x00003ffd,  0x00007ffd,  0x0001fffe,  0x0003fffe,  0x0003ffff
};

/* PS Stereo Huffmantable: iidDeltaFreqFine */
static const UINT iidDeltaFreqFine_Length[] =
{
  18,    18,    18,    18,    18,  18,    18,    18,    18,    17,
  18,    17,    17,    16,    16,  15,    14,    14,    13,    12,
  12,    11,    10,    10,     8,   7,     6,     5,     4,     3,
   1,     3,     4,     5,     6,   7,     8,     9,    10,    11,
  11,    12,    13,    14,    14,  15,    16,    16,    17,    17,
  18,    17,    18,    18,    18,  18,    18,    18,    18,    18,
  18
};
static const UINT iidDeltaFreqFine_Code[] =
{
  0x0001feb4,  0x0001feb5,  0x0001fd76,  0x0001fd77,  0x0001fd74,  0x0001fd75,  0x0001fe8a,  0x0001fe8b,  0x0001fe88,  0x0000fe80,
  0x0001feb6,  0x0000fe82,  0x0000feb8,  0x00007f42,  0x00007fae,  0x00003faf,  0x00001fd1,  0x00001fe9,  0x00000fe9,  0x000007ea,
  0x000007fb,  0x000003fb,  0x000001fb,  0x000001ff,  0x0000007c,  0x0000003c,  0x0000001c,  0x0000000c,  0000000000,  0x00000001,
  0x00000001,  0x00000002,  0x00000001,  0x0000000d,  0x0000001d,  0x0000003d,  0x0000007d,  0x000000fc,  0x000001fc,  0x000003fc,
  0x000003f4,  0x000007eb,  0x00000fea,  0x00001fea,  0x00001fd6,  0x00003fd0,  0x00007faf,  0x00007f43,  0x0000feb9,  0x0000fe83,
  0x0001feb7,  0x0000fe81,  0x0001fe89,  0x0001fe8e,  0x0001fe8f,  0x0001fe8c,  0x0001fe8d,  0x0001feb2,  0x0001feb3,  0x0001feb0,
  0x0001feb1
};

/* PS Stereo Huffmantable: iidDeltaTimeCoarse */
static const UINT iidDeltaTimeCoarse_Length[] =
{
  19,    19,    19,    20,    20,  20,    17,    15,    12,    10,
   8,     6,     4,     2,     1,   3,     5,     7,     9,    11,
  13,    14,    17,    19,    20,  20,    20,    20,    20
};
static const UINT iidDeltaTimeCoarse_Code[] =
{
  0x0007fff9,  0x0007fffa,  0x0007fffb,  0x000ffff8,  0x000ffff9,  0x000ffffa,  0x0001fffd,  0x00007ffe,  0x00000ffe,  0x000003fe,
  0x000000fe,  0x0000003e,  0x0000000e,  0x00000002,  0000000000,  0x00000006,  0x0000001e,  0x0000007e,  0x000001fe,  0x000007fe,
  0x00001ffe,  0x00003ffe,  0x0001fffc,  0x0007fff8,  0x000ffffb,  0x000ffffc,  0x000ffffd,  0x000ffffe,  0x000fffff
};

/* PS Stereo Huffmantable: iidDeltaTimeFine */
static const UINT iidDeltaTimeFine_Length[] =
{
  16,    16,    16,    16,    16,  16,    16,    16,    16,    15,
  15,    15,    15,    15,    15,  14,    14,    13,    13,    13,
  12,    12,    11,    10,     9,   9,     7,     6,     5,     3,
   1,     2,     5,     6,     7,   8,     9,    10,    11,    11,
  12,    12,    13,    13,    14,  14,    15,    15,    15,    15,
  16,    16,    16,    16,    16,  16,    16,    16,    16,    16,
  16
};
static const UINT iidDeltaTimeFine_Code[] =
{
  0x00004ed4,  0x00004ed5,  0x00004ece,  0x00004ecf,  0x00004ecc,  0x00004ed6,  0x00004ed8,  0x00004f46,  0x00004f60,  0x00002718,
  0x00002719,  0x00002764,  0x00002765,  0x0000276d,  0x000027b1,  0x000013b7,  0x000013d6,  0x000009c7,  0x000009e9,  0x000009ed,
  0x000004ee,  0x000004f7,  0x00000278,  0x00000139,  0x0000009a,  0x0000009f,  0x00000020,  0x00000011,  0x0000000a,  0x00000003,
  0x00000001,  0000000000,  0x0000000b,  0x00000012,  0x00000021,  0x0000004c,  0x0000009b,  0x0000013a,  0x00000279,  0x00000270,
  0x000004ef,  0x000004e2,  0x000009ea,  0x000009d8,  0x000013d7,  0x000013d0,  0x000027b2,  0x000027a2,  0x0000271a,  0x0000271b,
  0x00004f66,  0x00004f67,  0x00004f61,  0x00004f47,  0x00004ed9,  0x00004ed7,  0x00004ecd,  0x00004ed2,  0x00004ed3,  0x00004ed0,
  0x00004ed1
};

static const INT iccDelta_Offset =  7;
static const INT iccDelta_MaxVal = 14;
/* PS Stereo Huffmantable: iccDeltaFreq */
static const UINT iccDeltaFreq_Length[] =
{
  14,    14,    12,    10,     7,   5,     3,     1,     2,     4,
   6,     8,     9,    11,    13
};
static const UINT iccDeltaFreq_Code[] =
{
  0x00003fff,  0x00003ffe,  0x00000ffe,  0x000003fe,  0x0000007e,  0x0000001e,  0x00000006,  0000000000,  0x00000002,  0x0000000e,
  0x0000003e,  0x000000fe,  0x000001fe,  0x000007fe,  0x00001ffe
};

/* PS Stereo Huffmantable: iccDeltaTime */
static const UINT iccDeltaTime_Length[] =
{
  14,    13,    11,     9,     7,   5,     3,     1,     2,     4,
   6,     8,    10,    12,    14
};
static const UINT iccDeltaTime_Code[] =
{
  0x00003ffe,  0x00001ffe,  0x000007fe,  0x000001fe,  0x0000007e,  0x0000001e,  0x00000006,  0000000000,  0x00000002,  0x0000000e,
  0x0000003e,  0x000000fe,  0x000003fe,  0x00000ffe,  0x00003fff
};



static const INT ipdDelta_Offset = 0;
static const INT ipdDelta_MaxVal = 7;
/* PS Stereo Huffmantable: ipdDeltaFreq */
static const UINT ipdDeltaFreq_Length[] =
{
   1,     3,     4,     4,     4,   4,     4,     4
};
static const UINT ipdDeltaFreq_Code[] =
{
  0x00000001,  0000000000,  0x00000006,  0x00000004,  0x00000002,  0x00000003,  0x00000005,  0x00000007
};

/* PS Stereo Huffmantable: ipdDeltaTime */
static const UINT ipdDeltaTime_Length[] =
{
   1,     3,     4,     5,     5,   4,     4,     3
};
static const UINT ipdDeltaTime_Code[] =
{
  0x00000001,  0x00000002,  0x00000002,  0x00000003,  0x00000002,  0000000000,  0x00000003,  0x00000003
};


static const INT opdDelta_Offset = 0;
static const INT opdDelta_MaxVal = 7;
/* PS Stereo Huffmantable: opdDeltaFreq */
static const UINT opdDeltaFreq_Length[] =
{
   1,     3,     4,     4,     5,   5,     4,     3
};
static const UINT opdDeltaFreq_Code[] =
{
  0x00000001,  0x00000001,  0x00000006,  0x00000004,  0x0000000f,  0x0000000e,  0x00000005,  0000000000,
};

/* PS Stereo Huffmantable: opdDeltaTime */
static const UINT opdDeltaTime_Length[] =
{
   1,     3,     4,     5,     5,   4,     4,     3
};
static const UINT opdDeltaTime_Code[] =
{
  0x00000001,  0x00000002,  0x00000001,  0x00000007,  0x00000006,  0000000000,  0x00000002,  0x00000003
};

static const INT psBands[] =
{
  PS_BANDS_COARSE,
  PS_BANDS_MID
};

static INT getNoBands(PS_RESOLUTION mode)
{
  if(mode>=6)
    return 0;

  if(mode>=3)
    mode = (PS_RESOLUTION)(mode-3);

  return psBands[mode];
}

static INT getIIDRes(INT iidMode)
{
  if(iidMode<3)
    return PS_IID_RES_COARSE;
  else
    return PS_IID_RES_FINE;
}

static INT
encodeDeltaFreq(HANDLE_FDK_BITSTREAM hBitBuf,
                const INT          *val,
                const INT           nBands,
                const UINT *codeTable,
                const UINT *lengthTable,
                const INT           tableOffset,
                const INT           maxVal,
                INT                *error)
{
  INT bitCnt = 0;
  INT lastVal = 0;
  INT band;

  for(band=0;band<nBands;band++) {
    INT delta = (val[band] - lastVal) + tableOffset;
    lastVal = val[band];
    if( (delta>maxVal) || (delta<0) ) {
      *error = 1;
      delta = delta>0?maxVal:0;
    }
    bitCnt += FDKsbrEnc_WriteBits_ps(hBitBuf, codeTable[delta], lengthTable[delta]);
  }

  return bitCnt;
}

static INT
encodeDeltaTime(HANDLE_FDK_BITSTREAM hBitBuf,
                const INT          *val,
                const INT          *valLast,
                const INT           nBands,
                const UINT *codeTable,
                const UINT *lengthTable,
                const INT           tableOffset,
                const INT           maxVal,
                INT                *error)
{
  INT bitCnt = 0;
  INT band;

  for(band=0;band<nBands;band++) {
    INT delta = (val[band] - valLast[band]) + tableOffset;
    if( (delta>maxVal) || (delta<0) ) {
      *error = 1;
      delta = delta>0?maxVal:0;
    }
    bitCnt += FDKsbrEnc_WriteBits_ps(hBitBuf, codeTable[delta], lengthTable[delta]);
  }

  return bitCnt;
}

INT FDKsbrEnc_EncodeIid(HANDLE_FDK_BITSTREAM     hBitBuf,
              const INT               *iidVal,
              const INT               *iidValLast,
              const INT                nBands,
              const PS_IID_RESOLUTION  res,
              const PS_DELTA           mode,
              INT                     *error)
{
  const UINT *codeTable;
  const UINT *lengthTable;
  INT bitCnt = 0;

  bitCnt = 0;

  switch(mode) {
  case PS_DELTA_FREQ:
    switch(res) {
    case PS_IID_RES_COARSE:
      codeTable   = iidDeltaFreqCoarse_Code;
      lengthTable = iidDeltaFreqCoarse_Length;
      bitCnt += encodeDeltaFreq(hBitBuf, iidVal, nBands, codeTable,
                                lengthTable, iidDeltaCoarse_Offset,
                                iidDeltaCoarse_MaxVal, error);
    break;
    case PS_IID_RES_FINE:
      codeTable   = iidDeltaFreqFine_Code;
      lengthTable = iidDeltaFreqFine_Length;
      bitCnt += encodeDeltaFreq(hBitBuf, iidVal, nBands, codeTable,
                                lengthTable, iidDeltaFine_Offset,
                                iidDeltaFine_MaxVal, error);
    break;
    default:
      *error = 1;
    }
    break;

  case PS_DELTA_TIME:
    switch(res) {
    case PS_IID_RES_COARSE:
      codeTable   = iidDeltaTimeCoarse_Code;
      lengthTable = iidDeltaTimeCoarse_Length;
      bitCnt += encodeDeltaTime(hBitBuf, iidVal, iidValLast, nBands, codeTable,
                                lengthTable, iidDeltaCoarse_Offset,
                                iidDeltaCoarse_MaxVal, error);
    break;
    case PS_IID_RES_FINE:
      codeTable   = iidDeltaTimeFine_Code;
      lengthTable = iidDeltaTimeFine_Length;
      bitCnt += encodeDeltaTime(hBitBuf, iidVal, iidValLast, nBands, codeTable,
                                lengthTable, iidDeltaFine_Offset,
                                iidDeltaFine_MaxVal, error);
    break;
    default:
      *error = 1;
    }
    break;

  default:
    *error = 1;
  }

  return bitCnt;
}


INT FDKsbrEnc_EncodeIcc(HANDLE_FDK_BITSTREAM hBitBuf,
              const INT      *iccVal,
              const INT      *iccValLast,
              const INT       nBands,
              const PS_DELTA  mode,
              INT            *error)
{
  const UINT *codeTable;
  const UINT *lengthTable;
  INT bitCnt = 0;

  switch(mode) {
  case PS_DELTA_FREQ:
    codeTable   = iccDeltaFreq_Code;
    lengthTable = iccDeltaFreq_Length;
    bitCnt += encodeDeltaFreq(hBitBuf, iccVal, nBands, codeTable,
                              lengthTable, iccDelta_Offset, iccDelta_MaxVal, error);
    break;

  case PS_DELTA_TIME:
    codeTable   = iccDeltaTime_Code;
    lengthTable = iccDeltaTime_Length;

    bitCnt += encodeDeltaTime(hBitBuf, iccVal, iccValLast, nBands, codeTable,
                              lengthTable, iccDelta_Offset, iccDelta_MaxVal, error);
    break;

  default:
    *error = 1;
  }

  return bitCnt;
}

INT FDKsbrEnc_EncodeIpd(HANDLE_FDK_BITSTREAM hBitBuf,
              const INT      *ipdVal,
              const INT      *ipdValLast,
              const INT       nBands,
              const PS_DELTA  mode,
              INT            *error)
{
  const UINT *codeTable;
  const UINT *lengthTable;
  INT bitCnt = 0;

  switch(mode) {
  case PS_DELTA_FREQ:
    codeTable   = ipdDeltaFreq_Code;
    lengthTable = ipdDeltaFreq_Length;
    bitCnt += encodeDeltaFreq(hBitBuf, ipdVal, nBands, codeTable,
                              lengthTable, ipdDelta_Offset, ipdDelta_MaxVal, error);
    break;

  case PS_DELTA_TIME:
    codeTable   = ipdDeltaTime_Code;
    lengthTable = ipdDeltaTime_Length;

    bitCnt += encodeDeltaTime(hBitBuf, ipdVal, ipdValLast, nBands, codeTable,
                              lengthTable, ipdDelta_Offset, ipdDelta_MaxVal, error);
    break;

  default:
    *error = 1;
  }

  return bitCnt;
}

INT FDKsbrEnc_EncodeOpd(HANDLE_FDK_BITSTREAM hBitBuf,
              const INT      *opdVal,
              const INT      *opdValLast,
              const INT       nBands,
              const PS_DELTA  mode,
              INT            *error)
{
  const UINT *codeTable;
  const UINT *lengthTable;
  INT bitCnt = 0;

  switch(mode) {
  case PS_DELTA_FREQ:
    codeTable   = opdDeltaFreq_Code;
    lengthTable = opdDeltaFreq_Length;
    bitCnt += encodeDeltaFreq(hBitBuf, opdVal, nBands, codeTable,
                              lengthTable, opdDelta_Offset, opdDelta_MaxVal, error);
    break;

  case PS_DELTA_TIME:
    codeTable   = opdDeltaTime_Code;
    lengthTable = opdDeltaTime_Length;

    bitCnt += encodeDeltaTime(hBitBuf, opdVal, opdValLast, nBands, codeTable,
                              lengthTable, opdDelta_Offset, opdDelta_MaxVal, error);
    break;

  default:
    *error = 1;
  }

  return bitCnt;
}

static INT encodeIpdOpd(HANDLE_PS_OUT        psOut,
                        HANDLE_FDK_BITSTREAM hBitBuf )
{
  INT bitCnt = 0;
  INT error  = 0;
  INT env;

  FDKsbrEnc_WriteBits_ps(hBitBuf, psOut->enableIpdOpd, 1);

  if(psOut->enableIpdOpd==1) {
    INT *ipdLast = psOut->ipdLast;
    INT *opdLast = psOut->opdLast;

    for(env=0; env<psOut->nEnvelopes; env++) {
      bitCnt += FDKsbrEnc_WriteBits_ps( hBitBuf, psOut->deltaIPD[env], 1);
      bitCnt += FDKsbrEnc_EncodeIpd( hBitBuf,
                           psOut->ipd[env],
                           ipdLast,
                           getNoBands((PS_RESOLUTION)psOut->iidMode),
                           psOut->deltaIPD[env],
                           &error);

      bitCnt += FDKsbrEnc_WriteBits_ps( hBitBuf, psOut->deltaOPD[env], 1);
      bitCnt += FDKsbrEnc_EncodeOpd( hBitBuf,
                           psOut->opd[env],
                           opdLast,
                           getNoBands((PS_RESOLUTION)psOut->iidMode),
                           psOut->deltaOPD[env],
                           &error );
    }
    /* reserved bit */
    bitCnt += FDKsbrEnc_WriteBits_ps( hBitBuf, 0, 1);
  }


  return bitCnt;
}

static INT getEnvIdx(const INT nEnvelopes, const INT frameClass)
{
  INT envIdx = 0;

  switch(nEnvelopes) {
  case 0:
    envIdx = 0;
    break;

  case 1:
    if (frameClass==0)
      envIdx = 1;
    else
      envIdx = 0;
    break;

  case 2:
    if (frameClass==0)
      envIdx = 2;
    else
      envIdx = 1;
    break;

  case 3:
    envIdx = 2;
    break;

  case 4:
    envIdx = 3;
    break;

  default:
    /* unsupported number of envelopes */
    envIdx = 0;
  }

  return envIdx;
}


static INT encodePSExtension(const HANDLE_PS_OUT   psOut,
                             HANDLE_FDK_BITSTREAM  hBitBuf )
{
  INT bitCnt = 0;

  if(psOut->enableIpdOpd==1) {
    INT ipdOpdBits = 0;
    INT extSize = (2 + encodeIpdOpd(psOut,NULL)+7)>>3;

    if(extSize<15) {
      bitCnt += FDKsbrEnc_WriteBits_ps(hBitBuf,  extSize, 4);
    }
    else {
      bitCnt += FDKsbrEnc_WriteBits_ps(hBitBuf,          15 , 4);
      bitCnt += FDKsbrEnc_WriteBits_ps(hBitBuf, (extSize-15), 8);
    }

    /* write ipd opd data */
    ipdOpdBits += FDKsbrEnc_WriteBits_ps(hBitBuf, PS_EXT_ID_V0, 2);
    ipdOpdBits += encodeIpdOpd(psOut, hBitBuf );

    /* byte align the ipd opd data  */
    if(ipdOpdBits%8)
      ipdOpdBits += FDKsbrEnc_WriteBits_ps(hBitBuf, 0, (8-(ipdOpdBits%8)) );

    bitCnt += ipdOpdBits;
  }

  return (bitCnt);
}

INT FDKsbrEnc_WritePSBitstream(const HANDLE_PS_OUT   psOut,
                               HANDLE_FDK_BITSTREAM  hBitBuf )
{
  INT psExtEnable = 0;
  INT bitCnt = 0;
  INT error = 0;
  INT env;

  if(psOut != NULL){

    /* PS HEADER */
    bitCnt += FDKsbrEnc_WriteBits_ps( hBitBuf, psOut->enablePSHeader, 1);

    if(psOut->enablePSHeader) {

      bitCnt += FDKsbrEnc_WriteBits_ps( hBitBuf, psOut->enableIID, 1);
      if(psOut->enableIID) {
        bitCnt += FDKsbrEnc_WriteBits_ps( hBitBuf, psOut->iidMode, 3);
      }
      bitCnt += FDKsbrEnc_WriteBits_ps( hBitBuf, psOut->enableICC, 1);
      if(psOut->enableICC) {
        bitCnt += FDKsbrEnc_WriteBits_ps( hBitBuf, psOut->iccMode, 3);
      }
      if(psOut->enableIpdOpd) {
        psExtEnable = 1;
      }
      bitCnt += FDKsbrEnc_WriteBits_ps( hBitBuf, psExtEnable, 1);
    }

    /* Frame class, number of envelopes */
    bitCnt += FDKsbrEnc_WriteBits_ps( hBitBuf, psOut->frameClass, 1);
    bitCnt += FDKsbrEnc_WriteBits_ps( hBitBuf, getEnvIdx(psOut->nEnvelopes, psOut->frameClass), 2);

    if(psOut->frameClass==1) {
      for(env=0; env<psOut->nEnvelopes; env++) {
        bitCnt += FDKsbrEnc_WriteBits_ps( hBitBuf, psOut->frameBorder[env], 5);
      }
    }

    if(psOut->enableIID==1) {
      INT *iidLast = psOut->iidLast;
      for(env=0; env<psOut->nEnvelopes; env++) {
        bitCnt += FDKsbrEnc_WriteBits_ps( hBitBuf, psOut->deltaIID[env], 1);
        bitCnt += FDKsbrEnc_EncodeIid( hBitBuf,
                             psOut->iid[env],
                             iidLast,
                             getNoBands((PS_RESOLUTION)psOut->iidMode),
                             (PS_IID_RESOLUTION)getIIDRes(psOut->iidMode),
                             psOut->deltaIID[env],
                             &error );

        iidLast = psOut->iid[env];
      }
    }

    if(psOut->enableICC==1) {
      INT *iccLast = psOut->iccLast;
      for(env=0; env<psOut->nEnvelopes; env++) {
        bitCnt += FDKsbrEnc_WriteBits_ps( hBitBuf, psOut->deltaICC[env], 1);
        bitCnt += FDKsbrEnc_EncodeIcc( hBitBuf,
                             psOut->icc[env],
                             iccLast,
                             getNoBands((PS_RESOLUTION)psOut->iccMode),
                             psOut->deltaICC[env],
                             &error);

        iccLast = psOut->icc[env];
      }
    }

    if(psExtEnable!=0) {
      bitCnt += encodePSExtension(psOut, hBitBuf);
    }

  } /* if(psOut != NULL) */

  return bitCnt;
}

