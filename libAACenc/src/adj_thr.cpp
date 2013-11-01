
/* -----------------------------------------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright  1995 - 2013 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V.
  All rights reserved.

 1.    INTRODUCTION
The Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software that implements
the MPEG Advanced Audio Coding ("AAC") encoding and decoding scheme for digital audio.
This FDK AAC Codec software is intended to be used on a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient general perceptual
audio codecs. AAC-ELD is considered the best-performing full-bandwidth communications codec by
independent studies and is widely deployed. AAC has been standardized by ISO and IEC as part
of the MPEG specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including those of Fraunhofer)
may be obtained through Via Licensing (www.vialicensing.com) or through the respective patent owners
individually for the purpose of encoding or decoding bit streams in products that are compliant with
the ISO/IEC MPEG audio standards. Please note that most manufacturers of Android devices already license
these patent claims through Via Licensing or directly from the patent owners, and therefore FDK AAC Codec
software may already be covered under those patent licenses when it is used for those licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions with enhanced sound quality,
are also available from Fraunhofer. Users are encouraged to check the Fraunhofer website for additional
applications information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification, are permitted without
payment of copyright license fees provided that you satisfy the following conditions:

You must retain the complete text of this software license in redistributions of the FDK AAC Codec or
your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation and/or other materials
provided with redistributions of the FDK AAC Codec or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived from this library without
prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute the FDK AAC Codec
software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating that you changed the software
and the date of any change. For modified versions of the FDK AAC Codec, the term
"Fraunhofer FDK AAC Codec Library for Android" must be replaced by the term
"Third-Party Modified Version of the Fraunhofer FDK AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without limitation the patents of Fraunhofer,
ARE GRANTED BY THIS SOFTWARE LICENSE. Fraunhofer provides no warranty of patent non-infringement with
respect to this software.

You may use this FDK AAC Codec software or modifications thereto only for purposes that are authorized
by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright holders and contributors
"AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied warranties
of merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary, or consequential damages,
including but not limited to procurement of substitute goods or services; loss of use, data, or profits,
or business interruption, however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------------------------------------- */

/******************************** MPEG Audio Encoder **************************

    Initial author:       M. Werner
    contents/description: Threshold compensation

******************************************************************************/

#include "common_fix.h"

#include "adj_thr_data.h"
#include "adj_thr.h"
#include "qc_data.h"
#include "sf_estim.h"
#include "aacEnc_ram.h"




#define INV_INT_TAB_SIZE  (8)
static const FIXP_DBL invInt[INV_INT_TAB_SIZE] =
{
  0x7fffffff, 0x7fffffff, 0x40000000, 0x2aaaaaaa, 0x20000000, 0x19999999, 0x15555555, 0x12492492
};


#define INV_SQRT4_TAB_SIZE  (8)
static const FIXP_DBL invSqrt4[INV_SQRT4_TAB_SIZE] =
{
  0x7fffffff, 0x7fffffff, 0x6ba27e65, 0x61424bb5, 0x5a827999, 0x55994845, 0x51c8e33c, 0x4eb160d1
};


/*static const INT      invRedExp = 4;*/
static const FIXP_DBL  SnrLdMin1 = (FIXP_DBL)0xfcad0ddf; /*FL2FXCONST_DBL(FDKlog(0.316)/FDKlog(2.0)/LD_DATA_SCALING);*/
static const FIXP_DBL  SnrLdMin2 = (FIXP_DBL)0x0351e1a2; /*FL2FXCONST_DBL(FDKlog(3.16) /FDKlog(2.0)/LD_DATA_SCALING);*/
static const FIXP_DBL  SnrLdFac  = (FIXP_DBL)0xff5b2c3e; /*FL2FXCONST_DBL(FDKlog(0.8)  /FDKlog(2.0)/LD_DATA_SCALING);*/

static const FIXP_DBL  SnrLdMin3 = (FIXP_DBL)0xfe000000; /*FL2FXCONST_DBL(FDKlog(0.5)  /FDKlog(2.0)/LD_DATA_SCALING);*/
static const FIXP_DBL  SnrLdMin4 = (FIXP_DBL)0x02000000; /*FL2FXCONST_DBL(FDKlog(2.0)  /FDKlog(2.0)/LD_DATA_SCALING);*/
static const FIXP_DBL  SnrLdMin5 = (FIXP_DBL)0xfc000000; /*FL2FXCONST_DBL(FDKlog(0.25) /FDKlog(2.0)/LD_DATA_SCALING);*/


/*
The bits2Pe factors are choosen for the case that some times
the crash recovery strategy will be activated once.
*/

typedef struct {
  INT                 bitrate;
  LONG                bits2PeFactor_mono;
  LONG                bits2PeFactor_mono_slope;
  LONG                bits2PeFactor_stereo;
  LONG                bits2PeFactor_stereo_slope;
  LONG                bits2PeFactor_mono_scfOpt;
  LONG                bits2PeFactor_mono_scfOpt_slope;
  LONG                bits2PeFactor_stereo_scfOpt;
  LONG                bits2PeFactor_stereo_scfOpt_slope;

} BIT_PE_SFAC;

typedef struct {
  const INT           sampleRate;
  const BIT_PE_SFAC * pPeTab;
  const INT           nEntries;

} BITS2PE_CFG_TAB;

static const BIT_PE_SFAC S_Bits2PeTab16000[] = {
  { 10000, 0x228F5C29, 0x02FEF55D, 0x1D70A3D7, 0x09BC9D6D, 0x228F5C29, 0x02FEF55D, 0x1C28F5C3, 0x0CBB92CA},
  { 24000, 0x23D70A3D, 0x029F16B1, 0x2199999A, 0x07DD4413, 0x23D70A3D, 0x029F16B1, 0x2199999A, 0x07DD4413},
  { 32000, 0x247AE148, 0x11B1D92B, 0x23851EB8, 0x01F75105, 0x247AE148, 0x110A137F, 0x23851EB8, 0x01F75105},
  { 48000, 0x2D1EB852, 0x6833C600, 0x247AE148, 0x014F8B59, 0x2CCCCCCD, 0x68DB8BAC, 0x247AE148, 0x01F75105},
  { 64000, 0x60000000, 0x00000000, 0x251EB852, 0x154C985F, 0x60000000, 0x00000000, 0x2570A3D7, 0x154C985F},
  { 96000, 0x60000000, 0x00000000, 0x39EB851F, 0x088509C0, 0x60000000, 0x00000000, 0x3A3D70A4, 0x088509C0},
  {128000, 0x60000000, 0x00000000, 0x423D70A4, 0x18A43BB4, 0x60000000, 0x00000000, 0x428F5C29, 0x181E03F7},
  {148000, 0x60000000, 0x00000000, 0x5147AE14, 0x00000000, 0x60000000, 0x00000000, 0x5147AE14, 0x00000000}
};

static const BIT_PE_SFAC S_Bits2PeTab22050[] = {
  { 16000, 0x1a8f5c29, 0x1797cc3a, 0x128f5c29, 0x18e75793, 0x175c28f6, 0x221426fe, 0x00000000, 0x5a708ede},
  { 24000, 0x2051eb85, 0x092ccf6c, 0x18a3d70a, 0x13a92a30, 0x1fae147b, 0xbcbe61d,  0x16147ae1, 0x18e75793},
  { 32000, 0x228f5c29, 0x029f16b1, 0x1d70a3d7, 0x088509c0, 0x228f5c29, 0x29f16b1,  0x1c28f5c3, 0x0b242071},
  { 48000, 0x23d70a3d, 0x014f8b59, 0x2199999a, 0x03eea20a, 0x23d70a3d, 0x14f8b59,  0x2199999a, 0x03eea20a},
  { 64000, 0x247ae148, 0x08d8ec96, 0x23851eb8, 0x00fba882, 0x247ae148, 0x88509c0,  0x23851eb8, 0x00fba882},
  { 96000, 0x2d1eb852, 0x3419e300, 0x247ae148, 0x00a7c5ac, 0x2ccccccd, 0x346dc5d6, 0x247ae148, 0x00fba882},
  {128000, 0x60000000, 0x00000000, 0x251eb852, 0x029f16b1, 0x60000000, 0x00000000, 0x2570a3d7, 0x009f16b1},
  {148000, 0x60000000, 0x00000000, 0x26b851ec, 0x00000000, 0x60000000, 0x00000000, 0x270a3d71, 0x00000000}
};

static const BIT_PE_SFAC S_Bits2PeTab24000[] = {
  { 16000, 0x19eb851f, 0x13a92a30, 0x1147ae14, 0x164840e1, 0x1999999a, 0x12599ed8, 0x00000000, 0x46c764ae},
  { 24000, 0x1eb851ec, 0x0d1b7176, 0x16b851ec, 0x18e75793, 0x1e147ae1, 0x0fba8827, 0x1147ae14, 0x2c9081c3},
  { 32000, 0x21eb851f, 0x049667b6, 0x1ccccccd, 0x07357e67, 0x21eb851f, 0x03eea20a, 0x1c28f5c3, 0x07357e67},
  { 48000, 0x2428f5c3, 0x014f8b59, 0x2051eb85, 0x053e2d62, 0x23d70a3d, 0x01f75105, 0x1fae147b, 0x07357e67},
  { 64000, 0x24cccccd, 0x05e5f30e, 0x22e147ae, 0x01a36e2f, 0x24cccccd, 0x05e5f30e, 0x23333333, 0x014f8b59},
  { 96000, 0x2a8f5c29, 0x24b33db0, 0x247ae148, 0x00fba882, 0x2a8f5c29, 0x26fe718b, 0x247ae148, 0x00fba882},
  {128000, 0x4e666666, 0x1cd5f99c, 0x2570a3d7, 0x010c6f7a, 0x50a3d70a, 0x192a7371, 0x2570a3d7, 0x010c6f7a},
  {148000, 0x60000000, 0x00000000, 0x26147ae1, 0x00000000, 0x60000000, 0x00000000, 0x26147ae1, 0x00000000}
};

static const BIT_PE_SFAC S_Bits2PeTab32000[] = {
  { 16000, 0x1199999a, 0x20c49ba6, 0x00000000, 0x4577d955, 0x00000000, 0x60fe4799, 0x00000000, 0x00000000},
  { 24000, 0x1999999a, 0x0fba8827, 0x10f5c28f, 0x1b866e44, 0x17ae147b, 0x0fba8827, 0x00000000, 0x4d551d69},
  { 32000, 0x1d70a3d7, 0x07357e67, 0x17ae147b, 0x09d49518, 0x1b851eb8, 0x0a7c5ac4, 0x12e147ae, 0x110a137f},
  { 48000, 0x20f5c28f, 0x049667b6, 0x1c7ae148, 0x053e2d62, 0x20a3d70a, 0x053e2d62, 0x1b333333, 0x05e5f30e},
  { 64000, 0x23333333, 0x029f16b1, 0x1f0a3d71, 0x02f2f987, 0x23333333, 0x029f16b1, 0x1e147ae1, 0x03eea20a},
  { 96000, 0x25c28f5c, 0x2c3c9eed, 0x21eb851f, 0x01f75105, 0x25c28f5c, 0x0a7c5ac4, 0x21eb851f, 0x01a36e2f},
  {128000, 0x50f5c28f, 0x18a43bb4, 0x23d70a3d, 0x010c6f7a, 0x30000000, 0x168b5cc0, 0x23851eb8, 0x0192a737},
  {148000, 0x60000000, 0x00000000, 0x247ae148, 0x00dfb23b, 0x3dc28f5c, 0x300f4aaf, 0x247ae148, 0x01bf6476},
  {160000, 0x60000000, 0xb15b5740, 0x24cccccd, 0x053e2d62, 0x4f5c28f6, 0xbefd0072, 0x251eb852, 0x04fb1184},
  {200000, 0x00000000, 0x00000000, 0x2b333333, 0x0836be91, 0x00000000, 0x00000000, 0x2b333333, 0x0890390f},
  {320000, 0x00000000, 0x00000000, 0x4947ae14, 0x00000000, 0x00000000, 0x00000000, 0x4a8f5c29, 0x00000000}
};

static const BIT_PE_SFAC S_Bits2PeTab44100[] = {
  { 16000, 0x10a3d70a, 0x1797cc3a, 0x00000000, 0x00000000, 0x00000000, 0x59210386, 0x00000000, 0x00000000},
  { 24000, 0x16666666, 0x1797cc3a, 0x00000000, 0x639d5e4a, 0x15c28f5c, 0x12599ed8, 0x00000000, 0x5bc01a37},
  { 32000, 0x1c28f5c3, 0x049667b6, 0x1851eb85, 0x049667b6, 0x1a3d70a4, 0x088509c0, 0x16666666, 0x053e2d62},
  { 48000, 0x1e666666, 0x05e5f30e, 0x1a8f5c29, 0x049667b6, 0x1e666666, 0x05e5f30e, 0x18f5c28f, 0x05e5f30e},
  { 64000, 0x2147ae14, 0x0346dc5d, 0x1ccccccd, 0x02f2f987, 0x2147ae14, 0x02f2f987, 0x1bd70a3d, 0x039abf34},
  { 96000, 0x247ae148, 0x068db8bb, 0x1fae147b, 0x029f16b1, 0x2428f5c3, 0x0639d5e5, 0x1f5c28f6, 0x029f16b1},
  {128000, 0x2ae147ae, 0x1b435265, 0x223d70a4, 0x0192a737, 0x2a3d70a4, 0x1040bfe4, 0x21eb851f, 0x0192a737},
  {148000, 0x3b851eb8, 0x2832069c, 0x23333333, 0x00dfb23b, 0x3428f5c3, 0x2054c288, 0x22e147ae, 0x00dfb23b},
  {160000, 0x4a3d70a4, 0xc32ebe5a, 0x23851eb8, 0x01d5c316, 0x40000000, 0xcb923a2b, 0x23333333, 0x01d5c316},
  {200000, 0x00000000, 0x00000000, 0x25c28f5c, 0x0713f078, 0x00000000, 0x00000000, 0x2570a3d7, 0x072a4f17},
  {320000, 0x00000000, 0x00000000, 0x3fae147b, 0x00000000, 0x00000000, 0x00000000, 0x3fae147b, 0x00000000}
};

static const BIT_PE_SFAC S_Bits2PeTab48000[] = {
  { 16000, 0x0f5c28f6, 0x31ceaf25, 0x00000000, 0x00000000, 0x00000000, 0x74a771c9, 0x00000000, 0x00000000},
  { 24000, 0x1b851eb8, 0x029f16b1, 0x00000000, 0x663c74fb, 0x1c7ae148, 0xe47991bd, 0x00000000, 0x49667b5f},
  { 32000, 0x1c28f5c3, 0x029f16b1, 0x18f5c28f, 0x07357e67, 0x15c28f5c, 0x0f12c27a, 0x11eb851f, 0x13016484},
  { 48000, 0x1d70a3d7, 0x053e2d62, 0x1c7ae148, 0xfe08aefc, 0x1d1eb852, 0x068db8bb, 0x1b333333, 0xfeb074a8},
  { 64000, 0x20000000, 0x03eea20a, 0x1b851eb8, 0x0346dc5d, 0x2051eb85, 0x0346dc5d, 0x1a8f5c29, 0x039abf34},
  { 96000, 0x23d70a3d, 0x053e2d62, 0x1eb851ec, 0x029f16b1, 0x23851eb8, 0x04ea4a8c, 0x1e147ae1, 0x02f2f987},
  {128000, 0x28f5c28f, 0x14727dcc, 0x2147ae14, 0x0218def4, 0x2851eb85, 0x0e27e0f0, 0x20f5c28f, 0x0218def4},
  {148000, 0x3570a3d7, 0x1cd5f99c, 0x228f5c29, 0x01bf6476, 0x30f5c28f, 0x18777e75, 0x223d70a4, 0x01bf6476},
  {160000, 0x40000000, 0xcb923a2b, 0x23333333, 0x0192a737, 0x39eb851f, 0xd08d4bae, 0x22e147ae, 0x0192a737},
  {200000, 0x00000000, 0x00000000, 0x251eb852, 0x06775a1b, 0x00000000, 0x00000000, 0x24cccccd, 0x06a4175a},
  {320000, 0x00000000, 0x00000000, 0x3ccccccd, 0x00000000, 0x00000000, 0x00000000, 0x3d1eb852, 0x00000000}
};

static const BITS2PE_CFG_TAB bits2PeConfigTab[] = {
  { 16000, S_Bits2PeTab16000, sizeof(S_Bits2PeTab16000)/sizeof(BIT_PE_SFAC) },
  { 22050, S_Bits2PeTab22050, sizeof(S_Bits2PeTab22050)/sizeof(BIT_PE_SFAC) },
  { 24000, S_Bits2PeTab24000, sizeof(S_Bits2PeTab24000)/sizeof(BIT_PE_SFAC) },
  { 32000, S_Bits2PeTab32000, sizeof(S_Bits2PeTab32000)/sizeof(BIT_PE_SFAC) },
  { 44100, S_Bits2PeTab44100, sizeof(S_Bits2PeTab44100)/sizeof(BIT_PE_SFAC) },
  { 48000, S_Bits2PeTab48000, sizeof(S_Bits2PeTab48000)/sizeof(BIT_PE_SFAC) }
};



/* values for avoid hole flag */
enum _avoid_hole_state {
    NO_AH              =0,
    AH_INACTIVE        =1,
    AH_ACTIVE          =2
};


/*  Q format definitions */
#define Q_BITFAC    (24)   /* Q scaling used in FDKaacEnc_bitresCalcBitFac() calculation */
#define Q_AVGBITS   (17)   /* scale bit values */


/*****************************************************************************
    functionname: FDKaacEnc_InitBits2PeFactor
    description:  retrieve bits2PeFactor from table
*****************************************************************************/
static void FDKaacEnc_InitBits2PeFactor(
        FIXP_DBL *bits2PeFactor_m,
        INT *bits2PeFactor_e,
        const INT bitRate,
        const INT nChannels,
        const INT sampleRate,
        const INT advancedBitsToPe,
        const INT invQuant
        )
{
  /* default bits2pe factor */
  FIXP_DBL bit2PE_m = FL2FXCONST_DBL(1.18f/(1<<(1)));
  INT      bit2PE_e = 1;

  /* make use of advanced bits to pe factor table */
  if (advancedBitsToPe) {

    int i;
    const BIT_PE_SFAC *peTab = NULL;
    INT size = 0;


    /* Get correct table entry */
    for (i=0; i<(INT)(sizeof(bits2PeConfigTab)/sizeof(BITS2PE_CFG_TAB)); i++) {
      if (sampleRate >= bits2PeConfigTab[i].sampleRate) {
        peTab = bits2PeConfigTab[i].pPeTab;
        size  = bits2PeConfigTab[i].nEntries;
      }
    }

    if ( (peTab!=NULL) && (size!=0) ) {

      INT startB      = -1;
      LONG startPF    = 0;
      LONG peSlope    = 0;

      /* stereo or mono mode and invQuant used or not */
      for (i=0; i<size-1; i++)
      {
        if ((peTab[i].bitrate<=bitRate) && ((peTab[i+1].bitrate>bitRate) || ((i==size-2)) ))
        {
          if (nChannels==1)
          {
            startPF = (!invQuant) ? peTab[i].bits2PeFactor_mono   : peTab[i].bits2PeFactor_mono_scfOpt;
            peSlope = (!invQuant) ? peTab[i].bits2PeFactor_mono_slope : peTab[i].bits2PeFactor_mono_scfOpt_slope;
            /*endPF   = (!invQuant) ? peTab[i+1].bits2PeFactor_mono : peTab[i+1].bits2PeFactor_mono_scfOpt;
            endB=peTab[i+1].bitrate;*/
            startB=peTab[i].bitrate;
            break;
          }
          else
          {
            startPF = (!invQuant) ? peTab[i].bits2PeFactor_stereo   : peTab[i].bits2PeFactor_stereo_scfOpt;
            peSlope = (!invQuant) ? peTab[i].bits2PeFactor_stereo_slope : peTab[i].bits2PeFactor_stereo_scfOpt_slope;
            /*endPF   = (!invQuant) ? peTab[i+1].bits2PeFactor_stereo : peTab[i+1].bits2PeFactor_stereo_scfOpt;
            endB=peTab[i+1].bitrate;*/
            startB=peTab[i].bitrate;
            break;
          }
        }
      } /* for i */

      /* if a configuration is available */
      if (startB!=-1) {
        /* linear interpolate to actual PEfactor */
        FIXP_DBL peFac = fMult((FIXP_DBL)(bitRate-startB)<<14, (FIXP_DBL)peSlope) << 2;
        FIXP_DBL bit2PE = peFac + (FIXP_DBL)startPF; /* startPF_float = startPF << 2 */

        /* sanity check if bits2pe value is high enough */
        if ( bit2PE >= (FL2FXCONST_DBL(0.35f) >> 2) ) {
          bit2PE_m = bit2PE;
          bit2PE_e = 2; /*  table is fixed scaled */
        }
      } /* br */
    } /* sr */
  } /* advancedBitsToPe */


  /* return bits2pe factor */
  *bits2PeFactor_m = bit2PE_m;
  *bits2PeFactor_e = bit2PE_e;
}


/*****************************************************************************
functionname: FDKaacEnc_bits2pe2
description:  convert from bits to pe
*****************************************************************************/
static INT FDKaacEnc_bits2pe2(
        const INT                 bits,
        const FIXP_DBL            factor_m,
        const INT                 factor_e
        )
{
   return (INT)(fMult(factor_m, (FIXP_DBL)(bits<<Q_AVGBITS)) >> (Q_AVGBITS-factor_e));
}

/*****************************************************************************
functionname: FDKaacEnc_calcThreshExp
description:  loudness calculation (threshold to the power of redExp)
*****************************************************************************/
static void FDKaacEnc_calcThreshExp(FIXP_DBL thrExp[(2)][MAX_GROUPED_SFB],
                          QC_OUT_CHANNEL*  qcOutChannel[(2)],
                          PSY_OUT_CHANNEL*  psyOutChannel[(2)],
                          const INT nChannels)
{
   INT ch, sfb, sfbGrp;
   FIXP_DBL thrExpLdData;

   for (ch=0; ch<nChannels; ch++) {
     for(sfbGrp = 0;sfbGrp < psyOutChannel[ch]->sfbCnt;sfbGrp+= psyOutChannel[ch]->sfbPerGroup) {
       for (sfb=0; sfb<psyOutChannel[ch]->maxSfbPerGroup; sfb++) {
         thrExpLdData = psyOutChannel[ch]->sfbThresholdLdData[sfbGrp+sfb]>>2 ;
         thrExp[ch][sfbGrp+sfb] = CalcInvLdData(thrExpLdData);
       }
     }
   }
}


/*****************************************************************************
    functionname: FDKaacEnc_adaptMinSnr
    description:  reduce minSnr requirements for bands with relative low energies
*****************************************************************************/
static void FDKaacEnc_adaptMinSnr(QC_OUT_CHANNEL     *qcOutChannel[(2)],
                                  PSY_OUT_CHANNEL    *psyOutChannel[(2)],
                                  MINSNR_ADAPT_PARAM *msaParam,
                                  const INT           nChannels)
{
  INT ch, sfb, sfbGrp, nSfb;
  FIXP_DBL avgEnLD64, dbRatio, minSnrRed;
  FIXP_DBL minSnrLimitLD64 = FL2FXCONST_DBL(-0.00503012648262f); /* ld64(0.8f) */
  FIXP_DBL nSfbLD64;
  FIXP_DBL accu;

  for (ch=0; ch<nChannels; ch++) {
    /* calc average energy per scalefactor band */
    nSfb = 0;
    accu = FL2FXCONST_DBL(0.0f);

    for (sfbGrp=0; sfbGrp < psyOutChannel[ch]->sfbCnt; sfbGrp+=psyOutChannel[ch]->sfbPerGroup) {
      for (sfb=0; sfb<psyOutChannel[ch]->maxSfbPerGroup; sfb++) {
        accu += psyOutChannel[ch]->sfbEnergy[sfbGrp+sfb]>>6;
				 nSfb++;
			 }
    }

    if ((accu == FL2FXCONST_DBL(0.0f)) || (nSfb == 0)) {
      avgEnLD64 = FL2FXCONST_DBL(-1.0f);
    }
    else {
      nSfbLD64  = CalcLdInt(nSfb);
      avgEnLD64 = CalcLdData(accu);
      avgEnLD64 = avgEnLD64 + FL2FXCONST_DBL(0.09375f) - nSfbLD64;  /* 0.09375f: compensate shift with 6 */
    }

    /* reduce minSnr requirement by minSnr^minSnrRed dependent on avgEn/sfbEn */
    for (sfbGrp=0; sfbGrp < psyOutChannel[ch]->sfbCnt; sfbGrp+=psyOutChannel[ch]->sfbPerGroup) {
      for (sfb=0; sfb<psyOutChannel[ch]->maxSfbPerGroup; sfb++) {
        if ( (msaParam->startRatio + qcOutChannel[ch]->sfbEnergyLdData[sfbGrp+sfb]) < avgEnLD64 ) {
          dbRatio = fMult((avgEnLD64 - qcOutChannel[ch]->sfbEnergyLdData[sfbGrp+sfb]),FL2FXCONST_DBL(0.3010299956f)); /* scaled by (1.0f/(10.0f*64.0f)) */
          minSnrRed = msaParam->redOffs + fMult(msaParam->redRatioFac,dbRatio); /* scaled by 1.0f/64.0f*/
          minSnrRed = fixMax(minSnrRed, msaParam->maxRed); /* scaled by 1.0f/64.0f*/
          qcOutChannel[ch]->sfbMinSnrLdData[sfbGrp+sfb] = (fMult(qcOutChannel[ch]->sfbMinSnrLdData[sfbGrp+sfb],minSnrRed)) << 6;
          qcOutChannel[ch]->sfbMinSnrLdData[sfbGrp+sfb] = fixMin(minSnrLimitLD64, qcOutChannel[ch]->sfbMinSnrLdData[sfbGrp+sfb]);
        }
      }
    }
  }
}


/*****************************************************************************
functionname: FDKaacEnc_initAvoidHoleFlag
description:  determine bands where avoid hole is not necessary resp. possible
*****************************************************************************/
static void FDKaacEnc_initAvoidHoleFlag(QC_OUT_CHANNEL  *qcOutChannel[(2)],
                              PSY_OUT_CHANNEL *psyOutChannel[(2)],
                              UCHAR ahFlag[(2)][MAX_GROUPED_SFB],
                              struct TOOLSINFO *toolsInfo,
                              const INT nChannels,
                              const PE_DATA *peData,
                              AH_PARAM *ahParam)
{
   INT ch, sfb, sfbGrp;
   FIXP_DBL sfbEn, sfbEnm1;
   FIXP_DBL sfbEnLdData;
   FIXP_DBL avgEnLdData;

   /* decrease spread energy by 3dB for long blocks, resp. 2dB for shorts
      (avoid more holes in long blocks) */
   for (ch=0; ch<nChannels; ch++) {
      INT sfbGrp, sfb;
      QC_OUT_CHANNEL*  qcOutChan  = qcOutChannel[ch];

      if (psyOutChannel[ch]->lastWindowSequence != SHORT_WINDOW) {
         for (sfbGrp = 0;sfbGrp < psyOutChannel[ch]->sfbCnt;sfbGrp+= psyOutChannel[ch]->sfbPerGroup)
           for (sfb=0; sfb<psyOutChannel[ch]->maxSfbPerGroup; sfb++)
              qcOutChan->sfbSpreadEnergy[sfbGrp+sfb] >>= 1 ;
      }
      else {
         for (sfbGrp = 0;sfbGrp < psyOutChannel[ch]->sfbCnt;sfbGrp+= psyOutChannel[ch]->sfbPerGroup)
           for (sfb=0; sfb<psyOutChannel[ch]->maxSfbPerGroup; sfb++)
              qcOutChan->sfbSpreadEnergy[sfbGrp+sfb] =
                   fMult(FL2FXCONST_DBL(0.63f),
                         qcOutChan->sfbSpreadEnergy[sfbGrp+sfb]) ;
      }
   }

   /* increase minSnr for local peaks, decrease it for valleys */
   if (ahParam->modifyMinSnr) {
      for(ch=0; ch<nChannels; ch++) {
         QC_OUT_CHANNEL*  qcOutChan  = qcOutChannel[ch];
         for(sfbGrp = 0;sfbGrp < psyOutChannel[ch]->sfbCnt;sfbGrp+= psyOutChannel[ch]->sfbPerGroup){
            for (sfb=0; sfb<psyOutChannel[ch]->maxSfbPerGroup; sfb++) {
               FIXP_DBL sfbEnp1, avgEn;
               if (sfb > 0)
                  sfbEnm1 = qcOutChan->sfbEnergy[sfbGrp+sfb-1];
               else
                  sfbEnm1 = qcOutChan->sfbEnergy[sfbGrp+sfb];

               if (sfb < psyOutChannel[ch]->maxSfbPerGroup-1)
                  sfbEnp1 = qcOutChan->sfbEnergy[sfbGrp+sfb+1];
               else
                  sfbEnp1 = qcOutChan->sfbEnergy[sfbGrp+sfb];

               avgEn = (sfbEnm1>>1) + (sfbEnp1>>1);
               avgEnLdData = CalcLdData(avgEn);
               sfbEn = qcOutChan->sfbEnergy[sfbGrp+sfb];
               sfbEnLdData = qcOutChan->sfbEnergyLdData[sfbGrp+sfb];
               /* peak ? */
               if (sfbEn > avgEn) {
                  FIXP_DBL tmpMinSnrLdData;
                  if (psyOutChannel[ch]->lastWindowSequence==LONG_WINDOW)
                     tmpMinSnrLdData = fixMax( SnrLdFac + (FIXP_DBL)(avgEnLdData - sfbEnLdData), (FIXP_DBL)SnrLdMin1 ) ;
                  else
                     tmpMinSnrLdData = fixMax( SnrLdFac + (FIXP_DBL)(avgEnLdData - sfbEnLdData), (FIXP_DBL)SnrLdMin3 ) ;

                  qcOutChan->sfbMinSnrLdData[sfbGrp+sfb] =
                     fixMin(qcOutChan->sfbMinSnrLdData[sfbGrp+sfb], tmpMinSnrLdData);
               }
               /* valley ? */
               if ( ((sfbEnLdData+(FIXP_DBL)SnrLdMin4) < (FIXP_DBL)avgEnLdData) && (sfbEn > FL2FXCONST_DBL(0.0)) ) {
                  FIXP_DBL tmpMinSnrLdData = avgEnLdData - sfbEnLdData -(FIXP_DBL)SnrLdMin4 + qcOutChan->sfbMinSnrLdData[sfbGrp+sfb];
                  tmpMinSnrLdData = fixMin((FIXP_DBL)SnrLdFac, tmpMinSnrLdData);
                  qcOutChan->sfbMinSnrLdData[sfbGrp+sfb] = fixMin(tmpMinSnrLdData,
                        (FIXP_DBL)(qcOutChan->sfbMinSnrLdData[sfbGrp+sfb] + SnrLdMin2 ));
               }
            }
         }
      }
   }

   /* stereo: adapt the minimum requirements sfbMinSnr of mid and
      side channels to avoid spending unnoticable bits */
   if (nChannels == 2) {
      QC_OUT_CHANNEL*  qcOutChanM  = qcOutChannel[0];
      QC_OUT_CHANNEL*  qcOutChanS  = qcOutChannel[1];
      PSY_OUT_CHANNEL*  psyOutChanM  = psyOutChannel[0];
      for(sfbGrp = 0;sfbGrp < psyOutChanM->sfbCnt;sfbGrp+= psyOutChanM->sfbPerGroup){
        for (sfb=0; sfb<psyOutChanM->maxSfbPerGroup; sfb++) {
          if (toolsInfo->msMask[sfbGrp+sfb]) {
             FIXP_DBL maxSfbEnLd = fixMax(qcOutChanM->sfbEnergyLdData[sfbGrp+sfb],qcOutChanS->sfbEnergyLdData[sfbGrp+sfb]);
             FIXP_DBL maxThrLd, sfbMinSnrTmpLd;

             if ( ((SnrLdMin5>>1) + (maxSfbEnLd>>1) + (qcOutChanM->sfbMinSnrLdData[sfbGrp+sfb]>>1)) <= FL2FXCONST_DBL(-0.5f))
               maxThrLd = FL2FXCONST_DBL(-1.0f) ;
             else
               maxThrLd = SnrLdMin5 + maxSfbEnLd + qcOutChanM->sfbMinSnrLdData[sfbGrp+sfb];

             if (qcOutChanM->sfbEnergy[sfbGrp+sfb] > FL2FXCONST_DBL(0.0f))
               sfbMinSnrTmpLd = maxThrLd - qcOutChanM->sfbEnergyLdData[sfbGrp+sfb];
             else
               sfbMinSnrTmpLd = FL2FXCONST_DBL(0.0f);

             qcOutChanM->sfbMinSnrLdData[sfbGrp+sfb] = fixMax(qcOutChanM->sfbMinSnrLdData[sfbGrp+sfb],sfbMinSnrTmpLd);

             if (qcOutChanM->sfbMinSnrLdData[sfbGrp+sfb] <= FL2FXCONST_DBL(0.0f))
                qcOutChanM->sfbMinSnrLdData[sfbGrp+sfb] = fixMin(qcOutChanM->sfbMinSnrLdData[sfbGrp+sfb], (FIXP_DBL)SnrLdFac);

             if (qcOutChanS->sfbEnergy[sfbGrp+sfb] > FL2FXCONST_DBL(0.0f))
               sfbMinSnrTmpLd = maxThrLd - qcOutChanS->sfbEnergyLdData[sfbGrp+sfb];
             else
               sfbMinSnrTmpLd = FL2FXCONST_DBL(0.0f);

             qcOutChanS->sfbMinSnrLdData[sfbGrp+sfb] = fixMax(qcOutChanS->sfbMinSnrLdData[sfbGrp+sfb],sfbMinSnrTmpLd);

             if (qcOutChanS->sfbMinSnrLdData[sfbGrp+sfb] <= FL2FXCONST_DBL(0.0f))
                qcOutChanS->sfbMinSnrLdData[sfbGrp+sfb] = fixMin(qcOutChanS->sfbMinSnrLdData[sfbGrp+sfb],(FIXP_DBL)SnrLdFac);

             if (qcOutChanM->sfbEnergy[sfbGrp+sfb]>qcOutChanM->sfbSpreadEnergy[sfbGrp+sfb])
                qcOutChanS->sfbSpreadEnergy[sfbGrp+sfb] =
                   fMult(qcOutChanS->sfbEnergy[sfbGrp+sfb], FL2FXCONST_DBL(0.9f));

             if (qcOutChanS->sfbEnergy[sfbGrp+sfb]>qcOutChanS->sfbSpreadEnergy[sfbGrp+sfb])
                qcOutChanM->sfbSpreadEnergy[sfbGrp+sfb] =
                   fMult(qcOutChanM->sfbEnergy[sfbGrp+sfb], FL2FXCONST_DBL(0.9f));
          }
        }
      }
   }

   /* init ahFlag (0: no ah necessary, 1: ah possible, 2: ah active */
   for(ch=0; ch<nChannels; ch++) {
      QC_OUT_CHANNEL  *qcOutChan  = qcOutChannel[ch];
      PSY_OUT_CHANNEL  *psyOutChan  = psyOutChannel[ch];
      for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup){
        for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {
          if ((qcOutChan->sfbSpreadEnergy[sfbGrp+sfb] > qcOutChan->sfbEnergy[sfbGrp+sfb])
              || (qcOutChan->sfbMinSnrLdData[sfbGrp+sfb] > FL2FXCONST_DBL(0.0f))) {
             ahFlag[ch][sfbGrp+sfb] = NO_AH;
          }
          else {
             ahFlag[ch][sfbGrp+sfb] = AH_INACTIVE;
          }
        }
      }
   }
}



/**
 * \brief  Calculate constants that do not change during successive pe calculations.
 *
 * \param peData                Pointer to structure containing PE data of current element.
 * \param psyOutChannel         Pointer to PSY_OUT_CHANNEL struct holding nChannels elements.
 * \param qcOutChannel          Pointer to QC_OUT_CHANNEL struct holding nChannels elements.
 * \param nChannels             Number of channels in element.
 * \param peOffset              Fixed PE offset defined while FDKaacEnc_AdjThrInit() depending on bitrate.
 *
 * \return  void
 */
static
void FDKaacEnc_preparePe(PE_DATA *peData,
               PSY_OUT_CHANNEL* psyOutChannel[(2)],
               QC_OUT_CHANNEL* qcOutChannel[(2)],
               const INT nChannels,
               const INT peOffset)
{
    INT ch;

    for(ch=0; ch<nChannels; ch++) {
        PSY_OUT_CHANNEL *psyOutChan = psyOutChannel[ch];
        FDKaacEnc_prepareSfbPe(&peData->peChannelData[ch],
            psyOutChan->sfbEnergyLdData,
            psyOutChan->sfbThresholdLdData,
            qcOutChannel[ch]->sfbFormFactorLdData,
            psyOutChan->sfbOffsets,
            psyOutChan->sfbCnt,
            psyOutChan->sfbPerGroup,
            psyOutChan->maxSfbPerGroup);
    }
    peData->offset = peOffset;
}

/**
 * \brief  Calculate weighting factor for threshold adjustment.
 *
 * Calculate weighting factor to be applied at energies and thresholds in ld64 format.
 *
 * \param peData,               Pointer to PE data in current element.
 * \param psyOutChannel         Pointer to PSY_OUT_CHANNEL struct holding nChannels elements.
 * \param qcOutChannel          Pointer to QC_OUT_CHANNEL struct holding nChannels elements.
 * \param toolsInfo             Pointer to tools info struct of current element.
 * \param adjThrStateElement    Pointer to ATS_ELEMENT holding enFacPatch states.
 * \param nChannels             Number of channels in element.
 * \param usePatchTool          Apply the weighting tool 0 (no) else (yes).
 *
 * \return  void
 */
static
void FDKaacEnc_calcWeighting(PE_DATA *peData,
               PSY_OUT_CHANNEL* psyOutChannel[(2)],
               QC_OUT_CHANNEL* qcOutChannel[(2)],
               struct TOOLSINFO *toolsInfo,
               ATS_ELEMENT* adjThrStateElement,
               const INT nChannels,
               const INT usePatchTool)
{
    int ch, noShortWindowInFrame = TRUE;
    INT exePatchM = 0;

    for (ch=0; ch<nChannels; ch++) {
        if (psyOutChannel[ch]->lastWindowSequence == SHORT_WINDOW) {
            noShortWindowInFrame = FALSE;
        }
        FDKmemclear(qcOutChannel[ch]->sfbEnFacLd, MAX_GROUPED_SFB*sizeof(FIXP_DBL));
    }

    if (usePatchTool==0) {
        return; /* tool is disabled */
    }

    for (ch=0; ch<nChannels; ch++) {

        PSY_OUT_CHANNEL *psyOutChan = psyOutChannel[ch];

        if (noShortWindowInFrame) { /* retain energy ratio between blocks of different length */

            FIXP_DBL nrgSum14, nrgSum12, nrgSum34, nrgTotal;
            FIXP_DBL nrgFacLd_14, nrgFacLd_12, nrgFacLd_34;
            INT usePatch, exePatch;
            int sfb, sfbGrp, nLinesSum = 0;

            nrgSum14 = nrgSum12 = nrgSum34 = nrgTotal = FL2FXCONST_DBL(0.f);

            /* calculate flatness of audible spectrum, i.e. spectrum above masking threshold. */
            for (sfbGrp = 0;sfbGrp < psyOutChannel[ch]->sfbCnt; sfbGrp+=psyOutChannel[ch]->sfbPerGroup) {
              for (sfb=0; sfb<psyOutChannel[ch]->maxSfbPerGroup; sfb++) {
                FIXP_DBL nrgFac12 = CalcInvLdData(psyOutChan->sfbEnergyLdData[sfbGrp+sfb]>>1); /* nrg^(1/2) */
                FIXP_DBL nrgFac14 = CalcInvLdData(psyOutChan->sfbEnergyLdData[sfbGrp+sfb]>>2); /* nrg^(1/4) */

                /* maximal number of bands is 64, results scaling factor 6 */
                nLinesSum += peData->peChannelData[ch].sfbNLines[sfbGrp+sfb];             /* relevant lines */
                nrgTotal  += ( psyOutChan->sfbEnergy[sfbGrp+sfb] >> 6 );                  /* sum up nrg */
                nrgSum12  += ( nrgFac12 >> 6 );                                           /* sum up nrg^(2/4) */
                nrgSum14  += ( nrgFac14 >> 6 );                                           /* sum up nrg^(1/4) */
                nrgSum34  += ( fMult(nrgFac14, nrgFac12) >> 6 );                          /* sum up nrg^(3/4) */
              }
            }

            nrgTotal = CalcLdData(nrgTotal);                                              /* get ld64 of total nrg */

            nrgFacLd_14 = CalcLdData(nrgSum14) - nrgTotal;                                /* ld64(nrgSum14/nrgTotal) */
            nrgFacLd_12 = CalcLdData(nrgSum12) - nrgTotal;                                /* ld64(nrgSum12/nrgTotal) */
            nrgFacLd_34 = CalcLdData(nrgSum34) - nrgTotal;                                /* ld64(nrgSum34/nrgTotal) */

            adjThrStateElement->chaosMeasureEnFac[ch] = FDKmax( FL2FXCONST_DBL(0.1875f), fDivNorm(nLinesSum,psyOutChan->sfbOffsets[psyOutChan->sfbCnt]) );

            usePatch = (adjThrStateElement->chaosMeasureEnFac[ch] > FL2FXCONST_DBL(0.78125f));
            exePatch = ((usePatch) && (adjThrStateElement->lastEnFacPatch[ch]));

            for (sfbGrp = 0;sfbGrp < psyOutChannel[ch]->sfbCnt; sfbGrp+=psyOutChannel[ch]->sfbPerGroup) {
              for (sfb=0; sfb<psyOutChannel[ch]->maxSfbPerGroup; sfb++) {

                INT sfbExePatch;

                /* for MS coupled SFBs, also execute patch in side channel if done in mid channel */
                if ((ch == 1) && (toolsInfo->msMask[sfbGrp+sfb])) {
                    sfbExePatch = exePatchM;
                }
                else {
                    sfbExePatch = exePatch;
                }

                if ( (sfbExePatch) && (psyOutChan->sfbEnergy[sfbGrp+sfb]>FL2FXCONST_DBL(0.f)) )
                {
                    /* execute patch based on spectral flatness calculated above */
                    if (adjThrStateElement->chaosMeasureEnFac[ch] > FL2FXCONST_DBL(0.8125f)) {
                        qcOutChannel[ch]->sfbEnFacLd[sfbGrp+sfb] = ( (nrgFacLd_14 + (psyOutChan->sfbEnergyLdData[sfbGrp+sfb]+(psyOutChan->sfbEnergyLdData[sfbGrp+sfb]>>1)))>>1 ); /* sfbEnergy^(3/4) */
                    }
                    else if (adjThrStateElement->chaosMeasureEnFac[ch] > FL2FXCONST_DBL(0.796875f)) {
                        qcOutChannel[ch]->sfbEnFacLd[sfbGrp+sfb] = ( (nrgFacLd_12 + psyOutChan->sfbEnergyLdData[sfbGrp+sfb])>>1 );          /* sfbEnergy^(2/4) */
                    }
                    else {
                        qcOutChannel[ch]->sfbEnFacLd[sfbGrp+sfb] = ( (nrgFacLd_34 + (psyOutChan->sfbEnergyLdData[sfbGrp+sfb]>>1))>>1 );     /* sfbEnergy^(1/4) */
                    }
                    qcOutChannel[ch]->sfbEnFacLd[sfbGrp+sfb] = fixMin(qcOutChannel[ch]->sfbEnFacLd[sfbGrp+sfb],(FIXP_DBL)0);

                }
              }
            } /* sfb loop */

            adjThrStateElement->lastEnFacPatch[ch] = usePatch;
            exePatchM = exePatch;
        }
        else {
            /* !noShortWindowInFrame */
            adjThrStateElement->chaosMeasureEnFac[ch] = FL2FXCONST_DBL(0.75f);
            adjThrStateElement->lastEnFacPatch[ch] = TRUE; /* allow use of sfbEnFac patch in upcoming frame */
        }

    } /* ch loop */

}




/*****************************************************************************
functionname: FDKaacEnc_calcPe
description:  calculate pe for both channels
*****************************************************************************/
static
void FDKaacEnc_calcPe(PSY_OUT_CHANNEL* psyOutChannel[(2)],
            QC_OUT_CHANNEL* qcOutChannel[(2)],
            PE_DATA *peData,
            const INT nChannels)
{
   INT ch;

   peData->pe = peData->offset;
   peData->constPart = 0;
   peData->nActiveLines = 0;
   for(ch=0; ch<nChannels; ch++) {
      PE_CHANNEL_DATA *peChanData = &peData->peChannelData[ch];
      FDKaacEnc_calcSfbPe(&peData->peChannelData[ch],
                qcOutChannel[ch]->sfbWeightedEnergyLdData,
                qcOutChannel[ch]->sfbThresholdLdData,
                psyOutChannel[ch]->sfbCnt,
                psyOutChannel[ch]->sfbPerGroup,
                psyOutChannel[ch]->maxSfbPerGroup,
                psyOutChannel[ch]->isBook,
                psyOutChannel[ch]->isScale);

      peData->pe += peChanData->pe;
      peData->constPart += peChanData->constPart;
      peData->nActiveLines += peChanData->nActiveLines;
   }
}

void FDKaacEnc_peCalculation(PE_DATA *peData,
                             PSY_OUT_CHANNEL* psyOutChannel[(2)],
                             QC_OUT_CHANNEL* qcOutChannel[(2)],
                             struct TOOLSINFO *toolsInfo,
                             ATS_ELEMENT* adjThrStateElement,
                             const INT nChannels)
{
  /* constants that will not change during successive pe calculations */
  FDKaacEnc_preparePe(peData, psyOutChannel, qcOutChannel, nChannels, adjThrStateElement->peOffset);

  /* calculate weighting factor for threshold adjustment */
  FDKaacEnc_calcWeighting(peData, psyOutChannel, qcOutChannel, toolsInfo, adjThrStateElement, nChannels, 1);
{
    /* no weighting of threholds and energies for mlout */
    /* weight energies and thresholds */
    int ch;
    for (ch=0; ch<nChannels; ch++) {

        int sfb, sfbGrp;
        QC_OUT_CHANNEL* pQcOutCh = qcOutChannel[ch];

        for (sfbGrp = 0;sfbGrp < psyOutChannel[ch]->sfbCnt; sfbGrp+=psyOutChannel[ch]->sfbPerGroup) {
          for (sfb=0; sfb<psyOutChannel[ch]->maxSfbPerGroup; sfb++) {
            pQcOutCh->sfbWeightedEnergyLdData[sfb+sfbGrp] = pQcOutCh->sfbEnergyLdData[sfb+sfbGrp] - pQcOutCh->sfbEnFacLd[sfb+sfbGrp];
            pQcOutCh->sfbThresholdLdData[sfb+sfbGrp]     -= pQcOutCh->sfbEnFacLd[sfb+sfbGrp];
          }
        }
    }
}

  /* pe without reduction */
  FDKaacEnc_calcPe(psyOutChannel, qcOutChannel, peData, nChannels);
}



/*****************************************************************************
functionname: FDKaacEnc_FDKaacEnc_calcPeNoAH
description:  sum the pe data only for bands where avoid hole is inactive
*****************************************************************************/
static void FDKaacEnc_FDKaacEnc_calcPeNoAH(INT *pe,
                       INT *constPart,
                       INT    *nActiveLines,
                       PE_DATA *peData,
                       UCHAR ahFlag[(2)][MAX_GROUPED_SFB],
                       PSY_OUT_CHANNEL* psyOutChannel[(2)],
                       const INT nChannels)
{
    INT ch, sfb,sfbGrp;

    INT pe_tmp = peData->offset;
    INT constPart_tmp = 0;
    INT nActiveLines_tmp = 0;
    for(ch=0; ch<nChannels; ch++) {
        PE_CHANNEL_DATA *peChanData = &peData->peChannelData[ch];
        for(sfbGrp = 0;sfbGrp < psyOutChannel[ch]->sfbCnt;sfbGrp+= psyOutChannel[ch]->sfbPerGroup){
            for (sfb=0; sfb<psyOutChannel[ch]->maxSfbPerGroup; sfb++) {
                if(ahFlag[ch][sfbGrp+sfb] < AH_ACTIVE) {
                    pe_tmp += peChanData->sfbPe[sfbGrp+sfb];
                    constPart_tmp += peChanData->sfbConstPart[sfbGrp+sfb];
                    nActiveLines_tmp += peChanData->sfbNActiveLines[sfbGrp+sfb];
                }
            }
        }
    }
    /* correct scaled pe and constPart values */
    *pe = pe_tmp >> PE_CONSTPART_SHIFT;
    *constPart = constPart_tmp >> PE_CONSTPART_SHIFT;

	*nActiveLines = nActiveLines_tmp;
}


/*****************************************************************************
functionname: FDKaacEnc_reduceThresholdsCBR
description:  apply reduction formula
*****************************************************************************/
static const FIXP_DBL limitThrReducedLdData = (FIXP_DBL)0x00008000; /*FL2FXCONST_DBL(FDKpow(2.0,-LD_DATA_SCALING/4.0));*/

static void FDKaacEnc_reduceThresholdsCBR(QC_OUT_CHANNEL*  qcOutChannel[(2)],
                                PSY_OUT_CHANNEL* psyOutChannel[(2)],
                                UCHAR ahFlag[(2)][MAX_GROUPED_SFB],
                                FIXP_DBL thrExp[(2)][MAX_GROUPED_SFB],
                                const INT nChannels,
                                const FIXP_DBL redVal,
                                const SCHAR redValScaling)
{
   INT ch, sfb, sfbGrp;
   FIXP_DBL sfbEnLdData, sfbThrLdData, sfbThrReducedLdData;
   FIXP_DBL sfbThrExp;

    for(ch=0; ch<nChannels; ch++) {
      QC_OUT_CHANNEL *qcOutChan = qcOutChannel[ch];
      for(sfbGrp = 0; sfbGrp < psyOutChannel[ch]->sfbCnt; sfbGrp+= psyOutChannel[ch]->sfbPerGroup){
        for (sfb=0; sfb<psyOutChannel[ch]->maxSfbPerGroup; sfb++) {
         sfbEnLdData  = qcOutChan->sfbWeightedEnergyLdData[sfbGrp+sfb];
         sfbThrLdData = qcOutChan->sfbThresholdLdData[sfbGrp+sfb];
         sfbThrExp    = thrExp[ch][sfbGrp+sfb];
         if ((sfbEnLdData > sfbThrLdData) && (ahFlag[ch][sfbGrp+sfb] != AH_ACTIVE)) {

            /* threshold reduction formula:
             float tmp = thrExp[ch][sfb]+redVal;
             tmp *= tmp;
             sfbThrReduced = tmp*tmp;
            */
            int minScale = fixMin(CountLeadingBits(sfbThrExp), CountLeadingBits(redVal) - (DFRACT_BITS-1-redValScaling) )-1;

            /* 4*log( sfbThrExp + redVal ) */
            sfbThrReducedLdData = CalcLdData(fAbs(scaleValue(sfbThrExp, minScale) + scaleValue(redVal,(DFRACT_BITS-1-redValScaling)+minScale)))
                                             - (FIXP_DBL)(minScale<<(DFRACT_BITS-1-LD_DATA_SHIFT));
            sfbThrReducedLdData <<= 2;

            /* avoid holes */
            if ( ((sfbThrReducedLdData - sfbEnLdData) > qcOutChan->sfbMinSnrLdData[sfbGrp+sfb] )
                    && (ahFlag[ch][sfbGrp+sfb] != NO_AH) )
            {
              if (qcOutChan->sfbMinSnrLdData[sfbGrp+sfb]  > (FL2FXCONST_DBL(-1.0f) - sfbEnLdData) ){
                   sfbThrReducedLdData = fixMax((qcOutChan->sfbMinSnrLdData[sfbGrp+sfb] + sfbEnLdData), sfbThrLdData);
              }
              else sfbThrReducedLdData = sfbThrLdData;
              ahFlag[ch][sfbGrp+sfb] = AH_ACTIVE;
            }

            /* minimum of 29 dB Ratio for Thresholds */
            if ((sfbEnLdData+(FIXP_DBL)MAXVAL_DBL) > FL2FXCONST_DBL(9.6336206/LD_DATA_SCALING)){
                sfbThrReducedLdData = fixMax(sfbThrReducedLdData, (sfbEnLdData - FL2FXCONST_DBL(9.6336206/LD_DATA_SCALING)));
            }

            qcOutChan->sfbThresholdLdData[sfbGrp+sfb] = sfbThrReducedLdData;
         }
        }
      }
   }
}

/* similar to prepareSfbPe1() */
static FIXP_DBL FDKaacEnc_calcChaosMeasure(PSY_OUT_CHANNEL *psyOutChannel,
                                 const FIXP_DBL  *sfbFormFactorLdData)
{
  #define SCALE_FORM_FAC     (4)    /* (SCALE_FORM_FAC+FORM_FAC_SHIFT) >= ld(FRAME_LENGTH)*/
  #define SCALE_NRGS         (8)
  #define SCALE_NLINES      (16)
  #define SCALE_NRGS_SQRT4   (2)    /* 0.25 * SCALE_NRGS */
  #define SCALE_NLINES_P34  (12)    /* 0.75 * SCALE_NLINES */

  INT   sfbGrp, sfb;
  FIXP_DBL chaosMeasure;
  INT frameNLines = 0;
  FIXP_DBL frameFormFactor = FL2FXCONST_DBL(0.f);
  FIXP_DBL frameEnergy = FL2FXCONST_DBL(0.f);

  for (sfbGrp=0; sfbGrp<psyOutChannel->sfbCnt; sfbGrp+=psyOutChannel->sfbPerGroup) {
    for (sfb=0; sfb<psyOutChannel->maxSfbPerGroup; sfb++){
      if (psyOutChannel->sfbEnergyLdData[sfbGrp+sfb] > psyOutChannel->sfbThresholdLdData[sfbGrp+sfb]) {
        frameFormFactor += (CalcInvLdData(sfbFormFactorLdData[sfbGrp+sfb])>>SCALE_FORM_FAC);
        frameNLines     += (psyOutChannel->sfbOffsets[sfbGrp+sfb+1] - psyOutChannel->sfbOffsets[sfbGrp+sfb]);
        frameEnergy     += (psyOutChannel->sfbEnergy[sfbGrp+sfb]>>SCALE_NRGS);
      }
    }
  }

  if(frameNLines > 0){

    /*  frameNActiveLines = frameFormFactor*2^FORM_FAC_SHIFT * ((frameEnergy *2^SCALE_NRGS)/frameNLines)^-0.25
        chaosMeasure      = frameNActiveLines / frameNLines */
    chaosMeasure =
           CalcInvLdData( (((CalcLdData(frameFormFactor)>>1) -
                            (CalcLdData(frameEnergy)>>(2+1))) -
                           (fMultDiv2(FL2FXCONST_DBL(0.75f),CalcLdData((FIXP_DBL)frameNLines<<(DFRACT_BITS-1-SCALE_NLINES))) -
                            (((FIXP_DBL)(SCALE_FORM_FAC-SCALE_NRGS_SQRT4+FORM_FAC_SHIFT-(SCALE_NLINES_P34))<<(DFRACT_BITS-1-LD_DATA_SHIFT))>>1))
                          )<<1 );
  } else {

    /* assuming total chaos, if no sfb is above thresholds */
    chaosMeasure = FL2FXCONST_DBL(1.f);
  }

  return chaosMeasure;
}

/* apply reduction formula for VBR-mode */
static void FDKaacEnc_reduceThresholdsVBR(QC_OUT_CHANNEL* qcOutChannel[(2)],
                                PSY_OUT_CHANNEL* psyOutChannel[(2)],
                                UCHAR ahFlag[(2)][MAX_GROUPED_SFB],
                                FIXP_DBL thrExp[(2)][MAX_GROUPED_SFB],
                                const INT nChannels,
                                const FIXP_DBL  vbrQualFactor,
                                FIXP_DBL* chaosMeasureOld)
{
  INT ch, sfbGrp, sfb;
  FIXP_DBL chGroupEnergy[TRANS_FAC][2];/*energy for each group and channel*/
  FIXP_DBL chChaosMeasure[2];
  FIXP_DBL frameEnergy = FL2FXCONST_DBL(1e-10f);
  FIXP_DBL chaosMeasure = FL2FXCONST_DBL(0.f);
  FIXP_DBL sfbEnLdData, sfbThrLdData, sfbThrExp;
  FIXP_DBL sfbThrReducedLdData;
  FIXP_DBL chaosMeasureAvg;
  INT groupCnt; /* loop counter */
  FIXP_DBL redVal[TRANS_FAC]; /* reduction values; in short-block case one redVal for each group */
  QC_OUT_CHANNEL  *qcOutChan  = NULL;
  PSY_OUT_CHANNEL  *psyOutChan  = NULL;

#define SCALE_GROUP_ENERGY   (8)

#define CONST_CHAOS_MEAS_AVG_FAC_0  (FL2FXCONST_DBL(0.25f))
#define CONST_CHAOS_MEAS_AVG_FAC_1  (FL2FXCONST_DBL(1.f-0.25f))

#define MIN_LDTHRESH                (FL2FXCONST_DBL(-0.515625f))


  for(ch=0; ch<nChannels; ch++){
    qcOutChan  = qcOutChannel[ch];
    psyOutChan  = psyOutChannel[ch];

    /* adding up energy for each channel and each group separately */
    FIXP_DBL chEnergy = FL2FXCONST_DBL(0.f);
    groupCnt=0;

    for (sfbGrp=0; sfbGrp<psyOutChan->sfbCnt; sfbGrp+=psyOutChan->sfbPerGroup, groupCnt++) {
      chGroupEnergy[groupCnt][ch] = FL2FXCONST_DBL(0.f);
      for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++){
        chGroupEnergy[groupCnt][ch] += (psyOutChan->sfbEnergy[sfbGrp+sfb]>>SCALE_GROUP_ENERGY);
      }
      chEnergy += chGroupEnergy[groupCnt][ch];
    }
    frameEnergy += chEnergy;

    /* chaosMeasure */
    if (psyOutChannel[0]->lastWindowSequence == SHORT_WINDOW) {
      chChaosMeasure[ch] = FL2FXCONST_DBL(0.5f); /* assume a constant chaos measure of 0.5f for short blocks */
    } else {
      chChaosMeasure[ch] = FDKaacEnc_calcChaosMeasure(psyOutChannel[ch], qcOutChannel[ch]->sfbFormFactorLdData);
    }
    chaosMeasure += fMult(chChaosMeasure[ch], chEnergy);
  }

  if(frameEnergy > chaosMeasure) {
    INT scale = CntLeadingZeros(frameEnergy) - 1;
    FIXP_DBL num   = chaosMeasure<<scale;
    FIXP_DBL denum = frameEnergy<<scale;
    chaosMeasure   = schur_div(num,denum,16);
  }
  else {
    chaosMeasure = FL2FXCONST_DBL(1.f);
  }

  chaosMeasureAvg = fMult(CONST_CHAOS_MEAS_AVG_FAC_0, chaosMeasure) +
                    fMult(CONST_CHAOS_MEAS_AVG_FAC_1, *chaosMeasureOld);      /* averaging chaos measure */
  *chaosMeasureOld = chaosMeasure = (fixMin(chaosMeasure, chaosMeasureAvg));  /* use min-value, safe for next frame */

  /* characteristic curve
     chaosMeasure = 0.2f + 0.7f/0.3f * (chaosMeasure - 0.2f);
     chaosMeasure = fixMin(1.0f, fixMax(0.1f, chaosMeasure));
     constants scaled by 4.f
  */
  chaosMeasure = ((FL2FXCONST_DBL(0.2f)>>2) + fMult(FL2FXCONST_DBL(0.7f/(4.f*0.3f)), (chaosMeasure - FL2FXCONST_DBL(0.2f))));
  chaosMeasure = (fixMin((FIXP_DBL)(FL2FXCONST_DBL(1.0f)>>2), fixMax((FIXP_DBL)(FL2FXCONST_DBL(0.1f)>>2), chaosMeasure)))<<2;

  /* calculation of reduction value */
  if (psyOutChannel[0]->lastWindowSequence == SHORT_WINDOW){ /* short-blocks */
    FDK_ASSERT(TRANS_FAC==8);
    #define   WIN_TYPE_SCALE   (3)

    INT sfbGrp, groupCnt=0;
    for (sfbGrp=0; sfbGrp<psyOutChan->sfbCnt; sfbGrp+=psyOutChan->sfbPerGroup,groupCnt++) {

      FIXP_DBL groupEnergy = FL2FXCONST_DBL(0.f);

      for(ch=0;ch<nChannels;ch++){
        groupEnergy += chGroupEnergy[groupCnt][ch];   /* adding up the channels groupEnergy */
      }

      FDK_ASSERT(psyOutChannel[0]->groupLen[groupCnt]<=INV_INT_TAB_SIZE);
      groupEnergy = fMult(groupEnergy,invInt[psyOutChannel[0]->groupLen[groupCnt]]);  /* correction of group energy */
      groupEnergy = fixMin(groupEnergy, frameEnergy>>WIN_TYPE_SCALE);                 /* do not allow an higher redVal as calculated framewise */

      groupEnergy>>=2; /* 2*WIN_TYPE_SCALE = 6 => 6+2 = 8 ==> 8/4 = int number */

      redVal[groupCnt] = fMult(fMult(vbrQualFactor,chaosMeasure),
                               CalcInvLdData(CalcLdData(groupEnergy)>>2) )
                         << (int)( ( 2 + (2*WIN_TYPE_SCALE) + SCALE_GROUP_ENERGY )>>2 ) ;

    }
  } else { /* long-block */

    redVal[0] = fMult( fMult(vbrQualFactor,chaosMeasure),
                       CalcInvLdData(CalcLdData(frameEnergy)>>2) )
                << (int)( SCALE_GROUP_ENERGY>>2 ) ;
  }

  for(ch=0; ch<nChannels; ch++) {
    qcOutChan  = qcOutChannel[ch];
    psyOutChan  = psyOutChannel[ch];

    for (sfbGrp=0; sfbGrp<psyOutChan->sfbCnt; sfbGrp+=psyOutChan->sfbPerGroup) {
      for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++){

        sfbEnLdData  = (qcOutChan->sfbWeightedEnergyLdData[sfbGrp+sfb]);
        sfbThrLdData = (qcOutChan->sfbThresholdLdData[sfbGrp+sfb]);
        sfbThrExp    = thrExp[ch][sfbGrp+sfb];

        if ( (sfbThrLdData>=MIN_LDTHRESH) && (sfbEnLdData > sfbThrLdData) && (ahFlag[ch][sfbGrp+sfb] != AH_ACTIVE)) {

          /* Short-Window */
          if (psyOutChannel[ch]->lastWindowSequence == SHORT_WINDOW) {
            const int groupNumber = (int) sfb/psyOutChan->sfbPerGroup;

            FDK_ASSERT(INV_SQRT4_TAB_SIZE>psyOutChan->groupLen[groupNumber]);

            sfbThrExp = fMult(sfbThrExp, fMult( FL2FXCONST_DBL(2.82f/4.f), invSqrt4[psyOutChan->groupLen[groupNumber]]))<<2 ;

            if ( sfbThrExp <= (limitThrReducedLdData-redVal[groupNumber]) ) {
                sfbThrReducedLdData = FL2FXCONST_DBL(-1.0f);
            }
            else {
                if ((FIXP_DBL)redVal[groupNumber] >= FL2FXCONST_DBL(1.0f)-sfbThrExp)
                    sfbThrReducedLdData = FL2FXCONST_DBL(0.0f);
                else {
                    /* threshold reduction formula */
                    sfbThrReducedLdData = CalcLdData(sfbThrExp + redVal[groupNumber]);
                    sfbThrReducedLdData <<= 2;
                }
            }
            sfbThrReducedLdData += ( CalcLdInt(psyOutChan->groupLen[groupNumber]) -
                                     ((FIXP_DBL)6<<(DFRACT_BITS-1-LD_DATA_SHIFT)) );
          }

          /* Long-Window */
          else {
            if ((FIXP_DBL)redVal[0] >= FL2FXCONST_DBL(1.0f)-sfbThrExp) {
              sfbThrReducedLdData = FL2FXCONST_DBL(0.0f);
            }
            else {
              /* threshold reduction formula */
              sfbThrReducedLdData = CalcLdData(sfbThrExp + redVal[0]);
              sfbThrReducedLdData <<= 2;
            }
          }

          /* avoid holes */
          if ( ((sfbThrReducedLdData - sfbEnLdData) > qcOutChan->sfbMinSnrLdData[sfbGrp+sfb] )
                  && (ahFlag[ch][sfbGrp+sfb] != NO_AH) )
          {
            if (qcOutChan->sfbMinSnrLdData[sfbGrp+sfb]  > (FL2FXCONST_DBL(-1.0f) - sfbEnLdData) ){
                 sfbThrReducedLdData = fixMax((qcOutChan->sfbMinSnrLdData[sfbGrp+sfb] + sfbEnLdData), sfbThrLdData);
            }
            else sfbThrReducedLdData = sfbThrLdData;
            ahFlag[ch][sfbGrp+sfb] = AH_ACTIVE;
          }

          if (sfbThrReducedLdData<FL2FXCONST_DBL(-0.5f))
               sfbThrReducedLdData = FL2FXCONST_DBL(-1.f);

          /* minimum of 29 dB Ratio for Thresholds */
          if ((sfbEnLdData+FL2FXCONST_DBL(1.0f)) > FL2FXCONST_DBL(9.6336206/LD_DATA_SCALING)){
            sfbThrReducedLdData = fixMax(sfbThrReducedLdData, sfbEnLdData - FL2FXCONST_DBL(9.6336206/LD_DATA_SCALING));
          }

          sfbThrReducedLdData = fixMax(MIN_LDTHRESH,sfbThrReducedLdData);

          qcOutChan->sfbThresholdLdData[sfbGrp+sfb] = sfbThrReducedLdData;
        }
      }
    }
  }
}

/*****************************************************************************
functionname: FDKaacEnc_correctThresh
description:  if pe difference deltaPe between desired pe and real pe is small enough,
the difference can be distributed among the scale factor bands.
New thresholds can be derived from this pe-difference
*****************************************************************************/
static void FDKaacEnc_correctThresh(CHANNEL_MAPPING* cm,
                          QC_OUT_ELEMENT*  qcElement[(8)],
                          PSY_OUT_ELEMENT* psyOutElement[(8)],
                          UCHAR            ahFlag[(8)][(2)][MAX_GROUPED_SFB],
                          FIXP_DBL         thrExp[(8)][(2)][MAX_GROUPED_SFB],
                          const            FIXP_DBL redVal[(8)],
                          const            SCHAR redValScaling[(8)],
                          const            INT deltaPe,
                          const            INT processElements,
                          const            INT elementOffset)
{
   INT ch, sfb, sfbGrp;
   QC_OUT_CHANNEL *qcOutChan;
   PSY_OUT_CHANNEL *psyOutChan;
   PE_CHANNEL_DATA *peChanData;
   FIXP_DBL thrFactorLdData;
   FIXP_DBL sfbEnLdData, sfbThrLdData, sfbThrReducedLdData;
   FIXP_DBL *sfbPeFactorsLdData[(8)][(2)];
   FIXP_DBL sfbNActiveLinesLdData[(8)][(2)][MAX_GROUPED_SFB];
   INT      normFactorInt;
   FIXP_DBL normFactorLdData;

   INT nElements = elementOffset+processElements;
   INT elementId;

   /* scratch is empty; use temporal memory from quantSpec in QC_OUT_CHANNEL */
   for(elementId=elementOffset;elementId<nElements;elementId++) {
     for(ch=0; ch<cm->elInfo[elementId].nChannelsInEl; ch++) {
       SHORT* ptr = qcElement[elementId]->qcOutChannel[ch]->quantSpec;
       sfbPeFactorsLdData[elementId][ch] = (FIXP_DBL*)ptr;
     }
   }

   /* for each sfb calc relative factors for pe changes */
   normFactorInt = 0;

   for(elementId=elementOffset;elementId<nElements;elementId++) {
     if (cm->elInfo[elementId].elType != ID_DSE) {

       for(ch=0; ch<cm->elInfo[elementId].nChannelsInEl; ch++) {

          qcOutChan = qcElement[elementId]->qcOutChannel[ch];
          psyOutChan = psyOutElement[elementId]->psyOutChannel[ch];
          peChanData = &qcElement[elementId]->peData.peChannelData[ch];

          for(sfbGrp = 0; sfbGrp < psyOutChan->sfbCnt; sfbGrp+= psyOutChan->sfbPerGroup){
            for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {

             if ( peChanData->sfbNActiveLines[sfbGrp+sfb] == 0 ) {
                sfbNActiveLinesLdData[elementId][ch][sfbGrp+sfb] = FL2FXCONST_DBL(-1.0f);
             }
             else {
                /* Both CalcLdInt and CalcLdData can be used!
                 * No offset has to be subtracted, because sfbNActiveLinesLdData
                 * is shorted while thrFactor calculation */
                sfbNActiveLinesLdData[elementId][ch][sfbGrp+sfb] = CalcLdInt(peChanData->sfbNActiveLines[sfbGrp+sfb]);
             }
             if ( ((ahFlag[elementId][ch][sfbGrp+sfb] < AH_ACTIVE) || (deltaPe > 0)) &&
                   peChanData->sfbNActiveLines[sfbGrp+sfb] != 0 )
             {
                if (thrExp[elementId][ch][sfbGrp+sfb] > -redVal[elementId]) {

                   /* sfbPeFactors[ch][sfbGrp+sfb] = peChanData->sfbNActiveLines[sfbGrp+sfb] /
                                     (thrExp[elementId][ch][sfbGrp+sfb] + redVal[elementId]); */

                   int minScale = fixMin(CountLeadingBits(thrExp[elementId][ch][sfbGrp+sfb]), CountLeadingBits(redVal[elementId]) - (DFRACT_BITS-1-redValScaling[elementId]) ) - 1;

                   /* sumld = ld64( sfbThrExp + redVal ) */
                   FIXP_DBL sumLd = CalcLdData(scaleValue(thrExp[elementId][ch][sfbGrp+sfb], minScale) + scaleValue(redVal[elementId], (DFRACT_BITS-1-redValScaling[elementId])+minScale))
                                               - (FIXP_DBL)(minScale<<(DFRACT_BITS-1-LD_DATA_SHIFT));

                   if (sumLd < FL2FXCONST_DBL(0.f)) {
                      sfbPeFactorsLdData[elementId][ch][sfbGrp+sfb] = sfbNActiveLinesLdData[elementId][ch][sfbGrp+sfb] - sumLd;
                   }
                   else {
                     if ( sfbNActiveLinesLdData[elementId][ch][sfbGrp+sfb] > (FL2FXCONST_DBL(-1.f) + sumLd) ) {
                       sfbPeFactorsLdData[elementId][ch][sfbGrp+sfb] = sfbNActiveLinesLdData[elementId][ch][sfbGrp+sfb] - sumLd;
                     }
                     else {
                      sfbPeFactorsLdData[elementId][ch][sfbGrp+sfb] = sfbNActiveLinesLdData[elementId][ch][sfbGrp+sfb];
                     }
                   }

                   normFactorInt += (INT)CalcInvLdData(sfbPeFactorsLdData[elementId][ch][sfbGrp+sfb]);
                }
                else sfbPeFactorsLdData[elementId][ch][sfbGrp+sfb] = FL2FXCONST_DBL(1.0f);
             }
             else sfbPeFactorsLdData[elementId][ch][sfbGrp+sfb] = FL2FXCONST_DBL(-1.0f);
            }
          }
       }
     }
   }

   /* normFactorLdData = ld64(deltaPe/normFactorInt) */
   normFactorLdData = CalcLdData((FIXP_DBL)((deltaPe<0) ? (-deltaPe) : (deltaPe))) - CalcLdData((FIXP_DBL)normFactorInt);

   /* distribute the pe difference to the scalefactors
      and calculate the according thresholds */
   for(elementId=elementOffset;elementId<nElements;elementId++) {
     if (cm->elInfo[elementId].elType != ID_DSE) {

       for(ch=0; ch<cm->elInfo[elementId].nChannelsInEl; ch++) {
          qcOutChan = qcElement[elementId]->qcOutChannel[ch];
          psyOutChan = psyOutElement[elementId]->psyOutChannel[ch];
          peChanData = &qcElement[elementId]->peData.peChannelData[ch];

          for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup){
            for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {

              if (peChanData->sfbNActiveLines[sfbGrp+sfb] > 0) {

                 /* pe difference for this sfb */
                 if ( (sfbPeFactorsLdData[elementId][ch][sfbGrp+sfb]==FL2FXCONST_DBL(-1.0f)) ||
                      (deltaPe==0) )
                 {
                   thrFactorLdData = FL2FXCONST_DBL(0.f);
                 }
                 else {
                   /* new threshold */
                   FIXP_DBL tmp = CalcInvLdData(sfbPeFactorsLdData[elementId][ch][sfbGrp+sfb] + normFactorLdData - sfbNActiveLinesLdData[elementId][ch][sfbGrp+sfb] - FL2FXCONST_DBL((float)LD_DATA_SHIFT/LD_DATA_SCALING));

                   /* limit thrFactor to 60dB */
                   tmp = (deltaPe<0) ? tmp : (-tmp);
                   thrFactorLdData = FDKmin(tmp, FL2FXCONST_DBL(20.f/LD_DATA_SCALING));
                 }

                 /* new threshold */
                 sfbThrLdData = qcOutChan->sfbThresholdLdData[sfbGrp+sfb];
                 sfbEnLdData  = qcOutChan->sfbWeightedEnergyLdData[sfbGrp+sfb];

                 if (thrFactorLdData < FL2FXCONST_DBL(0.f)) {
                   if( sfbThrLdData > (FL2FXCONST_DBL(-1.f)-thrFactorLdData) ) {
                     sfbThrReducedLdData = sfbThrLdData + thrFactorLdData;
                   }
                   else {
                     sfbThrReducedLdData = FL2FXCONST_DBL(-1.f);
                   }
                 }
                 else{
                    sfbThrReducedLdData = sfbThrLdData + thrFactorLdData;
                 }

                 /* avoid hole */
                 if ( (sfbThrReducedLdData - sfbEnLdData > qcOutChan->sfbMinSnrLdData[sfbGrp+sfb]) &&
                      (ahFlag[elementId][ch][sfbGrp+sfb] == AH_INACTIVE) )
                 {
                    /* sfbThrReduced = max(psyOutChan[ch]->sfbMinSnr[i] * sfbEn, sfbThr); */
                    if ( sfbEnLdData > (sfbThrLdData-qcOutChan->sfbMinSnrLdData[sfbGrp+sfb]) ) {
                        sfbThrReducedLdData = qcOutChan->sfbMinSnrLdData[sfbGrp+sfb] + sfbEnLdData;
                    }
                    else {
                        sfbThrReducedLdData = sfbThrLdData;
                    }
                    ahFlag[elementId][ch][sfbGrp+sfb] = AH_ACTIVE;
                 }

                 qcOutChan->sfbThresholdLdData[sfbGrp+sfb] = sfbThrReducedLdData;
              }
            }
          }
       }
     }
   }
}

/*****************************************************************************
    functionname: FDKaacEnc_reduceMinSnr
    description:  if the desired pe can not be reached, reduce pe by
                  reducing minSnr
*****************************************************************************/
void FDKaacEnc_reduceMinSnr(CHANNEL_MAPPING* cm,
                            QC_OUT_ELEMENT*  qcElement[(8)],
                            PSY_OUT_ELEMENT* psyOutElement[(8)],
                            UCHAR            ahFlag[(8)][(2)][MAX_GROUPED_SFB],
                            const            INT desiredPe,
                            INT*             redPeGlobal,
                            const            INT processElements,
                            const            INT elementOffset)

{
   INT elementId;
   INT nElements = elementOffset+processElements;

   INT newGlobalPe = *redPeGlobal;

   for(elementId=elementOffset;elementId<nElements;elementId++) {
     if (cm->elInfo[elementId].elType != ID_DSE) {
       INT ch;
       INT maxSfbPerGroup[2];
       INT sfbCnt[2];
       INT sfbPerGroup[2];

       for(ch=0; ch<cm->elInfo[elementId].nChannelsInEl; ch++) {
         maxSfbPerGroup[ch] = psyOutElement[elementId]->psyOutChannel[ch]->maxSfbPerGroup-1;
         sfbCnt[ch]         = psyOutElement[elementId]->psyOutChannel[ch]->sfbCnt;
         sfbPerGroup[ch]    = psyOutElement[elementId]->psyOutChannel[ch]->sfbPerGroup;
       }

       PE_DATA *peData = &qcElement[elementId]->peData;

       do
       {
         for(ch=0; ch<cm->elInfo[elementId].nChannelsInEl; ch++) {

            INT sfb, sfbGrp;
            QC_OUT_CHANNEL  *qcOutChan = qcElement[elementId]->qcOutChannel[ch];
            INT noReduction = 1;

            if (maxSfbPerGroup[ch]>=0) {  /* sfb in next channel */
              INT deltaPe = 0;
              sfb = maxSfbPerGroup[ch]--;
              noReduction = 0;

              for (sfbGrp = 0; sfbGrp < sfbCnt[ch]; sfbGrp += sfbPerGroup[ch]) {

                if (ahFlag[elementId][ch][sfbGrp+sfb] != NO_AH &&
                    qcOutChan->sfbMinSnrLdData[sfbGrp+sfb] < SnrLdFac)
                {
                  /* increase threshold to new minSnr of 1dB */
                  qcOutChan->sfbMinSnrLdData[sfbGrp+sfb] = SnrLdFac;

                  /* sfbThrReduced = max(psyOutChan[ch]->sfbMinSnr[i] * sfbEn, sfbThr); */
                  if ( qcOutChan->sfbWeightedEnergyLdData[sfbGrp+sfb] >= qcOutChan->sfbThresholdLdData[sfbGrp+sfb] - qcOutChan->sfbMinSnrLdData[sfbGrp+sfb] ) {

                     qcOutChan->sfbThresholdLdData[sfbGrp+sfb] = qcOutChan->sfbWeightedEnergyLdData[sfbGrp+sfb] + qcOutChan->sfbMinSnrLdData[sfbGrp+sfb];

                     /* calc new pe */
                     /* C2 + C3*ld(1/0.8) = 1.5 */
                     deltaPe -= (peData->peChannelData[ch].sfbPe[sfbGrp+sfb]>>PE_CONSTPART_SHIFT);

                     /* sfbPe = 1.5 * sfbNLines */
                     peData->peChannelData[ch].sfbPe[sfbGrp+sfb] = (3*peData->peChannelData[ch].sfbNLines[sfbGrp+sfb]) << (PE_CONSTPART_SHIFT-1);
                     deltaPe += (peData->peChannelData[ch].sfbPe[sfbGrp+sfb]>>PE_CONSTPART_SHIFT);
                   }
                }

              } /* sfbGrp loop */

              peData->pe += deltaPe;
              peData->peChannelData[ch].pe += deltaPe;
              newGlobalPe += deltaPe;

              /* stop if enough has been saved */
              if (peData->pe <= desiredPe) {
                goto bail;
              }

            } /* sfb > 0 */

            if ( (ch==(cm->elInfo[elementId].nChannelsInEl-1)) && noReduction ) {
              goto bail;
            }

         } /* ch loop */

       } while ( peData->pe > desiredPe);

     } /* != ID_DSE */
   } /* element loop */


bail:
   /* update global PE */
   *redPeGlobal = newGlobalPe;
}


/*****************************************************************************
    functionname: FDKaacEnc_allowMoreHoles
    description:  if the desired pe can not be reached, some more scalefactor
                  bands have to be quantized to zero
*****************************************************************************/
static void FDKaacEnc_allowMoreHoles(CHANNEL_MAPPING* cm,
                           QC_OUT_ELEMENT*  qcElement[(8)],
                           PSY_OUT_ELEMENT* psyOutElement[(8)],
                           ATS_ELEMENT*     AdjThrStateElement[(8)],
                           UCHAR            ahFlag[(8)][(2)][MAX_GROUPED_SFB],
                           const INT        desiredPe,
                           const INT        currentPe,
                           const int        processElements,
                           const int        elementOffset)
{
  INT elementId;
  INT nElements = elementOffset+processElements;
  INT actPe = currentPe;

  if (actPe <= desiredPe) {
    return; /* nothing to do */
  }

  for (elementId = elementOffset;elementId<nElements;elementId++) {
    if (cm->elInfo[elementId].elType != ID_DSE) {

      INT ch, sfb, sfbGrp;

      PE_DATA *peData = &qcElement[elementId]->peData;
      const INT nChannels = cm->elInfo[elementId].nChannelsInEl;

      QC_OUT_CHANNEL*  qcOutChannel[(2)] = {NULL};
      PSY_OUT_CHANNEL* psyOutChannel[(2)] = {NULL};

      for (ch=0; ch<nChannels; ch++) {

        /* init pointers */
        qcOutChannel[ch] = qcElement[elementId]->qcOutChannel[ch];
        psyOutChannel[ch] = psyOutElement[elementId]->psyOutChannel[ch];

        for(sfbGrp=0; sfbGrp < psyOutChannel[ch]->sfbCnt; sfbGrp+= psyOutChannel[ch]->sfbPerGroup) {
          for (sfb=psyOutChannel[ch]->maxSfbPerGroup; sfb<psyOutChannel[ch]->sfbPerGroup; sfb++) {
            peData->peChannelData[ch].sfbPe[sfbGrp+sfb] = 0;
          }
        }
      }

      /* for MS allow hole in the channel with less energy */
      if ( nChannels==2 && psyOutChannel[0]->lastWindowSequence==psyOutChannel[1]->lastWindowSequence ) {

        for (sfb=0; sfb<psyOutChannel[0]->maxSfbPerGroup; sfb++) {
          for(sfbGrp=0; sfbGrp < psyOutChannel[0]->sfbCnt; sfbGrp+=psyOutChannel[0]->sfbPerGroup) {
            if (psyOutElement[elementId]->toolsInfo.msMask[sfbGrp+sfb]) {
              FIXP_DBL EnergyLd_L = qcOutChannel[0]->sfbWeightedEnergyLdData[sfbGrp+sfb];
              FIXP_DBL EnergyLd_R = qcOutChannel[1]->sfbWeightedEnergyLdData[sfbGrp+sfb];

              /* allow hole in side channel ? */
              if ( (ahFlag[elementId][1][sfbGrp+sfb] != NO_AH) &&
                   (((FL2FXCONST_DBL(-0.02065512648f)>>1) + (qcOutChannel[0]->sfbMinSnrLdData[sfbGrp+sfb]>>1))
                        > ((EnergyLd_R>>1) - (EnergyLd_L>>1))) )
              {
                  ahFlag[elementId][1][sfbGrp+sfb] = NO_AH;
                  qcOutChannel[1]->sfbThresholdLdData[sfbGrp+sfb] = FL2FXCONST_DBL(0.015625f) + EnergyLd_R;
                  actPe -= peData->peChannelData[1].sfbPe[sfbGrp+sfb]>>PE_CONSTPART_SHIFT;
              }
              /* allow hole in mid channel ? */
              else if ( (ahFlag[elementId][0][sfbGrp+sfb] != NO_AH) &&
                        (((FL2FXCONST_DBL(-0.02065512648f)>>1) + (qcOutChannel[1]->sfbMinSnrLdData[sfbGrp+sfb]>>1))
                             > ((EnergyLd_L>>1) - (EnergyLd_R>>1))) )
              {
                  ahFlag[elementId][0][sfbGrp+sfb] = NO_AH;
                  qcOutChannel[0]->sfbThresholdLdData[sfbGrp+sfb] = FL2FXCONST_DBL(0.015625f) + EnergyLd_L;
                  actPe -= peData->peChannelData[0].sfbPe[sfbGrp+sfb]>>PE_CONSTPART_SHIFT;
              } /* if (ahFlag) */
            } /* if MS */
          } /* sfbGrp */
          if (actPe <= desiredPe) {
            return; /* stop if enough has been saved */
          }
        } /* sfb */
      } /* MS possible ? */

      /* more holes necessary? subsequently erase bands
         starting with low energies */
      INT startSfb[2];
      FIXP_DBL avgEnLD64,minEnLD64;
      INT ahCnt;
      FIXP_DBL ahCntLD64;
      INT enIdx;
      FIXP_DBL enLD64[4];
      FIXP_DBL avgEn;

      /* do not go below startSfb */
      for (ch=0; ch<nChannels; ch++) {
        if (psyOutChannel[ch]->lastWindowSequence != SHORT_WINDOW)
          startSfb[ch] = AdjThrStateElement[elementId]->ahParam.startSfbL;
        else
          startSfb[ch] = AdjThrStateElement[elementId]->ahParam.startSfbS;
      }

      /* calc avg and min energies of bands that avoid holes */
      avgEn = FL2FXCONST_DBL(0.0f);
      minEnLD64 = FL2FXCONST_DBL(0.0f);
      ahCnt = 0;

      for (ch=0; ch<nChannels; ch++) {

        sfbGrp=0;
        sfb=startSfb[ch];

        do {
          for (; sfb<psyOutChannel[ch]->maxSfbPerGroup; sfb++) {
            if ((ahFlag[elementId][ch][sfbGrp+sfb]!=NO_AH) &&
                (qcOutChannel[ch]->sfbWeightedEnergyLdData[sfbGrp+sfb] > qcOutChannel[ch]->sfbThresholdLdData[sfbGrp+sfb])){
              minEnLD64 = fixMin(minEnLD64,qcOutChannel[ch]->sfbEnergyLdData[sfbGrp+sfb]);
              avgEn += qcOutChannel[ch]->sfbEnergy[sfbGrp+sfb] >> 6;
              ahCnt++;
            }
          }

          sfbGrp += psyOutChannel[ch]->sfbPerGroup;
          sfb=0;

        } while (sfbGrp < psyOutChannel[ch]->sfbCnt);
      }

      if ( (avgEn == FL2FXCONST_DBL(0.0f)) || (ahCnt == 0) ) {
        avgEnLD64 = FL2FXCONST_DBL(0.0f);
      }
      else {
        avgEnLD64 = CalcLdData(avgEn);
        ahCntLD64 = CalcLdInt(ahCnt);
        avgEnLD64 = avgEnLD64 + FL2FXCONST_DBL(0.09375f) - ahCntLD64; /* compensate shift with 6 */
      }

      /* calc some energy borders between minEn and avgEn */
      /* for (enIdx=0; enIdx<4; enIdx++) */
        /* en[enIdx] = minEn * (float)FDKpow(avgEn/(minEn+FLT_MIN), (2*enIdx+1)/7.0f); */
      enLD64[0] = minEnLD64 + fMult((avgEnLD64-minEnLD64),FL2FXCONST_DBL(0.14285714285f));
      enLD64[1] = minEnLD64 + fMult((avgEnLD64-minEnLD64),FL2FXCONST_DBL(0.42857142857f));
      enLD64[2] = minEnLD64 + fMult((avgEnLD64-minEnLD64),FL2FXCONST_DBL(0.71428571428f));
      enLD64[3] = minEnLD64 + (avgEnLD64-minEnLD64);

      for (enIdx=0; enIdx<4; enIdx++) {
        INT noReduction = 1;

        INT maxSfbPerGroup[2];
        INT sfbCnt[2];
        INT sfbPerGroup[2];

        for(ch=0; ch<cm->elInfo[elementId].nChannelsInEl; ch++) {
          maxSfbPerGroup[ch] = psyOutElement[elementId]->psyOutChannel[ch]->maxSfbPerGroup-1;
          sfbCnt[ch]         = psyOutElement[elementId]->psyOutChannel[ch]->sfbCnt;
          sfbPerGroup[ch]    = psyOutElement[elementId]->psyOutChannel[ch]->sfbPerGroup;
        }

        do {

          noReduction = 1;

          for(ch=0; ch<cm->elInfo[elementId].nChannelsInEl; ch++) {

            INT sfb, sfbGrp;

            /* start with lowest energy border at highest sfb */
            if (maxSfbPerGroup[ch]>=startSfb[ch]) {  /* sfb in next channel */
              sfb = maxSfbPerGroup[ch]--;
              noReduction = 0;

              for (sfbGrp = 0; sfbGrp < sfbCnt[ch]; sfbGrp += sfbPerGroup[ch]) {
                /* sfb energy below border ? */
                if (ahFlag[elementId][ch][sfbGrp+sfb] != NO_AH && qcOutChannel[ch]->sfbEnergyLdData[sfbGrp+sfb] < enLD64[enIdx]) {
                  /* allow hole */
                  ahFlag[elementId][ch][sfbGrp+sfb] = NO_AH;
                  qcOutChannel[ch]->sfbThresholdLdData[sfbGrp+sfb] = FL2FXCONST_DBL(0.015625f) + qcOutChannel[ch]->sfbWeightedEnergyLdData[sfbGrp+sfb];
                  actPe -= peData->peChannelData[ch].sfbPe[sfbGrp+sfb]>>PE_CONSTPART_SHIFT;
                }
              } /* sfbGrp  */

              if (actPe <= desiredPe) {
                return; /* stop if enough has been saved */
              }
            } /* sfb > 0 */
          } /* ch loop */

        } while( (noReduction == 0) && (actPe > desiredPe) );

        if (actPe <= desiredPe) {
          return; /* stop if enough has been saved */
        }

      } /* enIdx loop */

    } /* EOF DSE-suppression */
  } /* EOF for all elements... */

}

/* reset avoid hole flags from AH_ACTIVE to AH_INACTIVE  */
static void FDKaacEnc_resetAHFlags( UCHAR ahFlag[(2)][MAX_GROUPED_SFB],
                          const int nChannels,
                          PSY_OUT_CHANNEL  *psyOutChannel[(2)])
{
  int ch, sfb, sfbGrp;

  for(ch=0; ch<nChannels; ch++) {
    for (sfbGrp=0; sfbGrp < psyOutChannel[ch]->sfbCnt; sfbGrp+=psyOutChannel[ch]->sfbPerGroup) {
      for (sfb=0; sfb<psyOutChannel[ch]->maxSfbPerGroup; sfb++) {
        if ( ahFlag[ch][sfbGrp+sfb] == AH_ACTIVE) {
          ahFlag[ch][sfbGrp+sfb] = AH_INACTIVE;
        }
      }
    }
  }
}


static FIXP_DBL CalcRedValPower(FIXP_DBL num,
                                FIXP_DBL denum,
                                INT*     scaling )
{
    FIXP_DBL value = FL2FXCONST_DBL(0.f);

    if (num>=FL2FXCONST_DBL(0.f)) {
      value = fDivNorm( num, denum, scaling);
    }
    else {
      value = -fDivNorm( -num, denum, scaling);
    }
    value = f2Pow(value, *scaling, scaling);
    *scaling = DFRACT_BITS-1-*scaling;

    return value;
}


/*****************************************************************************
functionname: FDKaacEnc_adaptThresholdsToPe
description:  two guesses for the reduction value and one final correction of the thresholds
*****************************************************************************/
static void FDKaacEnc_adaptThresholdsToPe(CHANNEL_MAPPING*  cm,
                                ATS_ELEMENT*      AdjThrStateElement[(8)],
                                QC_OUT_ELEMENT*   qcElement[(8)],
                                PSY_OUT_ELEMENT*  psyOutElement[(8)],
                                const INT         desiredPe,
                                const INT         processElements,
                                const INT         elementOffset)
{
   FIXP_DBL redValue[(8)];
   SCHAR    redValScaling[(8)];
   UCHAR    pAhFlag[(8)][(2)][MAX_GROUPED_SFB];
   FIXP_DBL pThrExp[(8)][(2)][MAX_GROUPED_SFB];
   int iter;

   INT constPartGlobal, noRedPeGlobal, nActiveLinesGlobal, redPeGlobal;
   constPartGlobal = noRedPeGlobal = nActiveLinesGlobal = redPeGlobal = 0;

   int elementId;

   int nElements = elementOffset+processElements;
   if(nElements > cm->nElements) {
     nElements = cm->nElements;
   }

   /* ------------------------------------------------------- */
   /* Part I: Initialize data structures and variables... */
   /* ------------------------------------------------------- */
   for (elementId = elementOffset;elementId<nElements;elementId++) {
     if (cm->elInfo[elementId].elType != ID_DSE) {

       INT nChannels = cm->elInfo[elementId].nChannelsInEl;
       PE_DATA *peData    = &qcElement[elementId]->peData;

       /* thresholds to the power of redExp */
       FDKaacEnc_calcThreshExp(pThrExp[elementId], qcElement[elementId]->qcOutChannel, psyOutElement[elementId]->psyOutChannel, nChannels);

       /* lower the minSnr requirements for low energies compared to the average
          energy in this frame */
       FDKaacEnc_adaptMinSnr(qcElement[elementId]->qcOutChannel, psyOutElement[elementId]->psyOutChannel, &AdjThrStateElement[elementId]->minSnrAdaptParam, nChannels);

       /* init ahFlag (0: no ah necessary, 1: ah possible, 2: ah active */
       FDKaacEnc_initAvoidHoleFlag(qcElement[elementId]->qcOutChannel, psyOutElement[elementId]->psyOutChannel, pAhFlag[elementId], &psyOutElement[elementId]->toolsInfo, nChannels, peData, &AdjThrStateElement[elementId]->ahParam);

       /* sum up */
       constPartGlobal    += peData->constPart;
       noRedPeGlobal      += peData->pe;
       nActiveLinesGlobal += fixMax((INT)peData->nActiveLines, 1);

     } /* EOF DSE-suppression */
   } /* EOF for all elements... */

   /* ----------------------------------------------------------------------- */
   /* Part II: Calculate bit consumption of initial bit constraints setup */
   /* ----------------------------------------------------------------------- */
   for (elementId = elementOffset;elementId<nElements;elementId++) {
     if (cm->elInfo[elementId].elType != ID_DSE) {
       /*
       redVal = ( 2 ^ ( (constPartGlobal-desiredPe) / (invRedExp*nActiveLinesGlobal) )
                - 2 ^ ( (constPartGlobal-noRedPeGlobal) / (invRedExp*nActiveLinesGlobal) ) )
       */


       INT nChannels = cm->elInfo[elementId].nChannelsInEl;
       PE_DATA *peData    = &qcElement[elementId]->peData;

       /* first guess of reduction value */
       int scale0=0, scale1=0;
       FIXP_DBL tmp0 = CalcRedValPower( constPartGlobal-desiredPe, 4*nActiveLinesGlobal, &scale0 );
       FIXP_DBL tmp1 = CalcRedValPower( constPartGlobal-noRedPeGlobal, 4*nActiveLinesGlobal, &scale1 );

       int scalMin = FDKmin(scale0, scale1)-1;

       redValue[elementId]  = scaleValue(tmp0,(scalMin-scale0)) - scaleValue(tmp1,(scalMin-scale1));
       redValScaling[elementId] = scalMin;

       /* reduce thresholds */
       FDKaacEnc_reduceThresholdsCBR(qcElement[elementId]->qcOutChannel, psyOutElement[elementId]->psyOutChannel, pAhFlag[elementId], pThrExp[elementId], nChannels, redValue[elementId], redValScaling[elementId]);

       /* pe after first guess */
       FDKaacEnc_calcPe(psyOutElement[elementId]->psyOutChannel, qcElement[elementId]->qcOutChannel, peData, nChannels);

       redPeGlobal += peData->pe;
     } /* EOF DSE-suppression */
   } /* EOF for all elements... */

   /* -------------------------------------------------- */
   /* Part III: Iterate until bit constraints are met */
   /* -------------------------------------------------- */
   iter = 0;
   while ((fixp_abs(redPeGlobal - desiredPe) > fMultI(FL2FXCONST_DBL(0.05f),desiredPe)) && (iter < 1)) {

     INT desiredPeNoAHGlobal;
     INT redPeNoAHGlobal = 0;
     INT constPartNoAHGlobal = 0;
     INT nActiveLinesNoAHGlobal = 0;

     for (elementId = elementOffset;elementId<nElements;elementId++) {
       if (cm->elInfo[elementId].elType != ID_DSE) {

         INT redPeNoAH, constPartNoAH, nActiveLinesNoAH;
         INT nChannels = cm->elInfo[elementId].nChannelsInEl;
         PE_DATA *peData    = &qcElement[elementId]->peData;

         /* pe for bands where avoid hole is inactive */
         FDKaacEnc_FDKaacEnc_calcPeNoAH(&redPeNoAH, &constPartNoAH, &nActiveLinesNoAH,
                    peData, pAhFlag[elementId], psyOutElement[elementId]->psyOutChannel, nChannels);

         redPeNoAHGlobal += redPeNoAH;
         constPartNoAHGlobal += constPartNoAH;
         nActiveLinesNoAHGlobal += nActiveLinesNoAH;
       } /* EOF DSE-suppression */
     } /* EOF for all elements... */

     /* Calculate new redVal ... */
     if(desiredPe < redPeGlobal) {

       /* new desired pe without bands where avoid hole is active */
       desiredPeNoAHGlobal = desiredPe - (redPeGlobal - redPeNoAHGlobal);

       /* limit desiredPeNoAH to positive values, as the PE can not become negative */
       desiredPeNoAHGlobal = FDKmax(0,desiredPeNoAHGlobal);

       /* second guess (only if there are bands left where avoid hole is inactive)*/
       if (nActiveLinesNoAHGlobal > 0) {
         for (elementId = elementOffset;elementId<nElements;elementId++) {
           if (cm->elInfo[elementId].elType != ID_DSE) {
             /*
             redVal += ( 2 ^ ( (constPartNoAHGlobal-desiredPeNoAHGlobal) / (invRedExp*nActiveLinesNoAHGlobal) )
                       - 2 ^ ( (constPartNoAHGlobal-redPeNoAHGlobal) / (invRedExp*nActiveLinesNoAHGlobal) ) )
             */
             int scale0 = 0;
             int scale1 = 0;

             FIXP_DBL tmp0 = CalcRedValPower( constPartNoAHGlobal-desiredPeNoAHGlobal, 4*nActiveLinesNoAHGlobal, &scale0 );
             FIXP_DBL tmp1 = CalcRedValPower( constPartNoAHGlobal-redPeNoAHGlobal, 4*nActiveLinesNoAHGlobal, &scale1 );

             int scalMin = FDKmin(scale0, scale1)-1;

             tmp0 = scaleValue(tmp0,(scalMin-scale0)) - scaleValue(tmp1,(scalMin-scale1));
             scale0 = scalMin;

             /* old reduction value */
             tmp1 = redValue[elementId];
             scale1 = redValScaling[elementId];

             scalMin = fixMin(scale0,scale1)-1;

             /* sum up old and new reduction value */
             redValue[elementId] = scaleValue(tmp0,(scalMin-scale0)) + scaleValue(tmp1,(scalMin-scale1));
             redValScaling[elementId] = scalMin;

           } /* EOF DSE-suppression */
         } /* EOF for all elements... */
       } /* nActiveLinesNoAHGlobal > 0 */
     }
     else {
        /* desiredPe >= redPeGlobal */
        for (elementId = elementOffset;elementId<nElements;elementId++) {
          if (cm->elInfo[elementId].elType != ID_DSE) {

            INT redVal_scale = 0;
            FIXP_DBL tmp = fDivNorm((FIXP_DBL)redPeGlobal, (FIXP_DBL)desiredPe, &redVal_scale);

            /* redVal *= redPeGlobal/desiredPe; */
            redValue[elementId] = fMult(redValue[elementId], tmp);
            redValScaling[elementId] -= redVal_scale;

            FDKaacEnc_resetAHFlags(pAhFlag[elementId], cm->elInfo[elementId].nChannelsInEl, psyOutElement[elementId]->psyOutChannel);
          } /* EOF DSE-suppression */
        } /* EOF for all elements... */
     }

     redPeGlobal = 0;
     /* Calculate new redVal's PE... */
     for (elementId = elementOffset;elementId<nElements;elementId++) {
       if (cm->elInfo[elementId].elType != ID_DSE) {

         INT nChannels = cm->elInfo[elementId].nChannelsInEl;
         PE_DATA *peData    = &qcElement[elementId]->peData;

         /* reduce thresholds */
         FDKaacEnc_reduceThresholdsCBR(qcElement[elementId]->qcOutChannel, psyOutElement[elementId]->psyOutChannel, pAhFlag[elementId], pThrExp[elementId], nChannels, redValue[elementId], redValScaling[elementId]);

         /* pe after second guess */
         FDKaacEnc_calcPe(psyOutElement[elementId]->psyOutChannel, qcElement[elementId]->qcOutChannel, peData, nChannels);
         redPeGlobal += peData->pe;

       } /* EOF DSE-suppression */
     } /* EOF for all elements... */

     iter++;
   } /* EOF while */


   /* ------------------------------------------------------- */
   /* Part IV: if still required, further reduce constraints  */
   /* ------------------------------------------------------- */
   /*                  1.0*        1.15*       1.20*
    *               desiredPe   desiredPe   desiredPe
    *                   |           |           |
    * ...XXXXXXXXXXXXXXXXXXXXXXXXXXX|           |
    *                   |           |           |XXXXXXXXXXX...
    *                   |           |XXXXXXXXXXX|
    *            --- A ---          | --- B --- | --- C ---
    *
    * (X): redPeGlobal
    * (A): FDKaacEnc_correctThresh()
    * (B): FDKaacEnc_allowMoreHoles()
    * (C): FDKaacEnc_reduceMinSnr()
   */

   /* correct thresholds to get closer to the desired pe */
   if ( redPeGlobal > desiredPe ) {
     FDKaacEnc_correctThresh(cm, qcElement, psyOutElement, pAhFlag, pThrExp, redValue, redValScaling,
                   desiredPe - redPeGlobal, processElements, elementOffset);

     /* update PE */
     redPeGlobal = 0;
     for(elementId=elementOffset;elementId<nElements;elementId++) {
       if (cm->elInfo[elementId].elType != ID_DSE) {

         INT nChannels = cm->elInfo[elementId].nChannelsInEl;
         PE_DATA *peData    = &qcElement[elementId]->peData;

         /* pe after correctThresh */
         FDKaacEnc_calcPe(psyOutElement[elementId]->psyOutChannel, qcElement[elementId]->qcOutChannel, peData, nChannels);
         redPeGlobal += peData->pe;

       } /* EOF DSE-suppression */
     } /* EOF for all elements... */
   }

   if ( redPeGlobal > desiredPe ) {
     /* reduce pe by reducing minSnr requirements */
     FDKaacEnc_reduceMinSnr(cm, qcElement, psyOutElement, pAhFlag,
                            (fMultI(FL2FXCONST_DBL(0.15f),desiredPe) + desiredPe),
                            &redPeGlobal, processElements, elementOffset);

     /* reduce pe by allowing additional spectral holes */
     FDKaacEnc_allowMoreHoles(cm, qcElement, psyOutElement, AdjThrStateElement, pAhFlag,
                    desiredPe, redPeGlobal, processElements, elementOffset);
   }

}

/* similar to FDKaacEnc_adaptThresholdsToPe(), for  VBR-mode */
void FDKaacEnc_AdaptThresholdsVBR(QC_OUT_CHANNEL* qcOutChannel[(2)],
                               PSY_OUT_CHANNEL* psyOutChannel[(2)],
                               ATS_ELEMENT* AdjThrStateElement,
                               struct TOOLSINFO *toolsInfo,
                               PE_DATA *peData,
                               const INT nChannels)
{
   UCHAR    (*pAhFlag)[MAX_GROUPED_SFB];
   FIXP_DBL (*pThrExp)[MAX_GROUPED_SFB];

   /* allocate scratch memory */
   C_ALLOC_SCRATCH_START(_pAhFlag, UCHAR, (2)*MAX_GROUPED_SFB)
   C_ALLOC_SCRATCH_START(_pThrExp, FIXP_DBL, (2)*MAX_GROUPED_SFB)
   pAhFlag = (UCHAR(*)[MAX_GROUPED_SFB])_pAhFlag;
   pThrExp = (FIXP_DBL(*)[MAX_GROUPED_SFB])_pThrExp;

   /* thresholds to the power of redExp */
   FDKaacEnc_calcThreshExp(pThrExp, qcOutChannel, psyOutChannel, nChannels);

   /* lower the minSnr requirements for low energies compared to the average
      energy in this frame */
   FDKaacEnc_adaptMinSnr(qcOutChannel, psyOutChannel, &AdjThrStateElement->minSnrAdaptParam, nChannels);

   /* init ahFlag (0: no ah necessary, 1: ah possible, 2: ah active */
   FDKaacEnc_initAvoidHoleFlag(qcOutChannel, psyOutChannel, pAhFlag, toolsInfo,
                     nChannels, peData, &AdjThrStateElement->ahParam);

   /* reduce thresholds */
   FDKaacEnc_reduceThresholdsVBR(qcOutChannel, psyOutChannel, pAhFlag, pThrExp, nChannels,
                       AdjThrStateElement->vbrQualFactor,
                       &AdjThrStateElement->chaosMeasureOld);

   /* free scratch memory */
   C_ALLOC_SCRATCH_END(_pThrExp, FIXP_DBL, (2)*MAX_GROUPED_SFB)
   C_ALLOC_SCRATCH_END(_pAhFlag, UCHAR, (2)*MAX_GROUPED_SFB)
}


/*****************************************************************************

  functionname: FDKaacEnc_calcBitSave
  description:  Calculates percentage of bit save, see figure below
  returns:
  input:        parameters and bitres-fullness
  output:       percentage of bit save

*****************************************************************************/
/*
        bitsave
                    maxBitSave(%)|   clipLow
                                 |---\
                                 |    \
                                 |     \
                                 |      \
                                 |       \
                                 |--------\--------------> bitres
                                 |         \
                    minBitSave(%)|          \------------
                                          clipHigh      maxBitres
*/
static FIXP_DBL FDKaacEnc_calcBitSave(FIXP_DBL fillLevel,
    const FIXP_DBL clipLow,
    const FIXP_DBL clipHigh,
    const FIXP_DBL minBitSave,
    const FIXP_DBL maxBitSave,
    const FIXP_DBL bitsave_slope)
{
    FIXP_DBL bitsave;

    fillLevel = fixMax(fillLevel, clipLow);
    fillLevel = fixMin(fillLevel, clipHigh);

    bitsave = maxBitSave - fMult((fillLevel-clipLow), bitsave_slope);

    return (bitsave);
}

/*****************************************************************************

  functionname: FDKaacEnc_calcBitSpend
  description:  Calculates percentage of bit spend, see figure below
  returns:
  input:        parameters and bitres-fullness
  output:       percentage of bit spend

*****************************************************************************/
/*
                              bitspend      clipHigh
                   maxBitSpend(%)|          /-----------maxBitres
                                 |         /
                                 |        /
                                 |       /
                                 |      /
                                 |     /
                                 |----/-----------------> bitres
                                 |   /
                   minBitSpend(%)|--/
                                   clipLow
*/
static FIXP_DBL FDKaacEnc_calcBitSpend(FIXP_DBL fillLevel,
    const FIXP_DBL clipLow,
    const FIXP_DBL clipHigh,
    const FIXP_DBL minBitSpend,
    const FIXP_DBL maxBitSpend,
    const FIXP_DBL bitspend_slope)
{
    FIXP_DBL bitspend;

    fillLevel = fixMax(fillLevel, clipLow);
    fillLevel = fixMin(fillLevel, clipHigh);

    bitspend = minBitSpend + fMult(fillLevel-clipLow, bitspend_slope);

    return (bitspend);
}


/*****************************************************************************

  functionname: FDKaacEnc_adjustPeMinMax()
  description:  adjusts peMin and peMax parameters over time
  returns:
  input:        current pe, peMin, peMax, bitres size
  output:       adjusted peMin/peMax

*****************************************************************************/
static void FDKaacEnc_adjustPeMinMax(const INT currPe,
    INT      *peMin,
    INT      *peMax)
{
  FIXP_DBL minFacHi = FL2FXCONST_DBL(0.3f), maxFacHi = (FIXP_DBL)MAXVAL_DBL, minFacLo = FL2FXCONST_DBL(0.14f), maxFacLo = FL2FXCONST_DBL(0.07f);
    INT diff;

    INT minDiff_fix = fMultI(FL2FXCONST_DBL(0.1666666667f), currPe);

    if (currPe > *peMax)
    {
        diff = (currPe-*peMax) ;
        *peMin += fMultI(minFacHi,diff);
        *peMax += fMultI(maxFacHi,diff);
    }
    else if (currPe < *peMin)
    {
        diff = (*peMin-currPe) ;
        *peMin -= fMultI(minFacLo,diff);
        *peMax -= fMultI(maxFacLo,diff);
    }
    else
    {
        *peMin += fMultI(minFacHi, (currPe - *peMin));
        *peMax -= fMultI(maxFacLo, (*peMax - currPe));
    }

    if ((*peMax - *peMin) < minDiff_fix)
    {
        INT peMax_fix = *peMax, peMin_fix = *peMin;
        FIXP_DBL partLo_fix, partHi_fix;

        partLo_fix = (FIXP_DBL)fixMax(0, currPe - peMin_fix);
        partHi_fix = (FIXP_DBL)fixMax(0, peMax_fix - currPe);

        peMax_fix = (INT)(currPe + fMultI(fDivNorm(partHi_fix, (partLo_fix+partHi_fix)), minDiff_fix));
        peMin_fix = (INT)(currPe - fMultI(fDivNorm(partLo_fix, (partLo_fix+partHi_fix)), minDiff_fix));
        peMin_fix = fixMax(0, peMin_fix);

        *peMax = peMax_fix;
        *peMin = peMin_fix;
    }
}



/*****************************************************************************

  functionname: BitresCalcBitFac
  description:  calculates factor of spending bits for one frame
  1.0 : take all frame dynpart bits
  >1.0 : take all frame dynpart bits + bitres
  <1.0 : put bits in bitreservoir
  returns:      BitFac
  input:        bitres-fullness, pe, blockType, parameter-settings
  output:

*****************************************************************************/
/*
                     bitfac(%)            pemax
                   bitspend(%)   |          /-----------maxBitres
                                 |         /
                                 |        /
                                 |       /
                                 |      /
                                 |     /
                                 |----/-----------------> pe
                                 |   /
                   bitsave(%)    |--/
                                    pemin
*/

static FIXP_DBL FDKaacEnc_bitresCalcBitFac(const INT       bitresBits,
    const INT        maxBitresBits,
    const INT        pe,
    const INT        lastWindowSequence,
    const INT        avgBits,
    const FIXP_DBL   maxBitFac,
    ADJ_THR_STATE   *AdjThr,
    ATS_ELEMENT     *adjThrChan)
{
    BRES_PARAM *bresParam;
    INT pex;

    INT qmin, qbr, qbres, qmbr;
    FIXP_DBL bitSave, bitSpend;

    FIXP_DBL bitresFac_fix, tmp_cst, tmp_fix;
    FIXP_DBL pe_pers, bits_ratio, maxBrVal;
    FIXP_DBL bitsave_slope, bitspend_slope, maxBitFac_tmp;
    FIXP_DBL fillLevel_fix = (FIXP_DBL)0x7fffffff;
    FIXP_DBL UNITY = (FIXP_DBL)0x7fffffff;
    FIXP_DBL POINT7 = (FIXP_DBL)0x5999999A;

    if (maxBitresBits > bitresBits) {
      fillLevel_fix = fDivNorm(bitresBits, maxBitresBits);
    }

    if (lastWindowSequence != SHORT_WINDOW)
    {
        bresParam = &(AdjThr->bresParamLong);
        bitsave_slope = (FIXP_DBL)0x3BBBBBBC;
        bitspend_slope = (FIXP_DBL)0x55555555;
    }
    else
    {
        bresParam = &(AdjThr->bresParamShort);
        bitsave_slope = (FIXP_DBL)0x2E8BA2E9;
        bitspend_slope = (FIXP_DBL)0x7fffffff;
    }

    pex = fixMax(pe, adjThrChan->peMin);
    pex = fixMin(pex, adjThrChan->peMax);

    bitSave = FDKaacEnc_calcBitSave(fillLevel_fix,
        bresParam->clipSaveLow, bresParam->clipSaveHigh,
        bresParam->minBitSave, bresParam->maxBitSave, bitsave_slope);

    bitSpend = FDKaacEnc_calcBitSpend(fillLevel_fix,
        bresParam->clipSpendLow, bresParam->clipSpendHigh,
        bresParam->minBitSpend, bresParam->maxBitSpend, bitspend_slope);

    pe_pers = fDivNorm(pex - adjThrChan->peMin, adjThrChan->peMax - adjThrChan->peMin);
    tmp_fix = fMult(((FIXP_DBL)bitSpend + (FIXP_DBL)bitSave), pe_pers);
    bitresFac_fix = (UNITY>>1) - ((FIXP_DBL)bitSave>>1) + (tmp_fix>>1); qbres = (DFRACT_BITS-2);

    /* (float)bitresBits/(float)avgBits */
    bits_ratio = fDivNorm(bitresBits, avgBits, &qbr);
    qbr = DFRACT_BITS-1-qbr;

    /* Add 0.7 in q31 to bits_ratio in qbr */
    /* 0.7f + (float)bitresBits/(float)avgBits */
    qmin = fixMin(qbr, (DFRACT_BITS-1));
    bits_ratio = bits_ratio >> (qbr - qmin);
    tmp_cst = POINT7 >> ((DFRACT_BITS-1) - qmin);
    maxBrVal = (bits_ratio>>1) + (tmp_cst>>1); qmbr = qmin - 1;

    /* bitresFac_fix = fixMin(bitresFac_fix, 0.7 + bitresBits/avgBits); */
    bitresFac_fix = bitresFac_fix >> (qbres - qmbr); qbres = qmbr;
    bitresFac_fix = fixMin(bitresFac_fix, maxBrVal);

    /* Compare with maxBitFac */
    qmin = fixMin(Q_BITFAC, qbres);
    bitresFac_fix = bitresFac_fix >> (qbres - qmin);
    maxBitFac_tmp = maxBitFac >> (Q_BITFAC - qmin);
    if(maxBitFac_tmp < bitresFac_fix)
    {
        bitresFac_fix = maxBitFac;
    }
    else
    {
        if(qmin < Q_BITFAC)
        {
            bitresFac_fix = bitresFac_fix << (Q_BITFAC-qmin);
        }
        else
        {
            bitresFac_fix = bitresFac_fix >> (qmin-Q_BITFAC);
        }
    }

    FDKaacEnc_adjustPeMinMax(pe, &adjThrChan->peMin, &adjThrChan->peMax);

    return bitresFac_fix;
}


/*****************************************************************************
functionname: FDKaacEnc_AdjThrNew
description:  allocate ADJ_THR_STATE
*****************************************************************************/
INT FDKaacEnc_AdjThrNew(ADJ_THR_STATE** phAdjThr,
                        INT             nElements)
{
    INT err = 0;
    INT i;
    ADJ_THR_STATE* hAdjThr = GetRam_aacEnc_AdjustThreshold();
    if (hAdjThr==NULL) {
        err = 1;
        goto bail;
    }

    for (i=0; i<nElements; i++) {
        hAdjThr->adjThrStateElem[i] =  GetRam_aacEnc_AdjThrStateElement(i);
        if (hAdjThr->adjThrStateElem[i]==NULL) {
          err = 1;
          goto bail;
        }
    }

bail:
    *phAdjThr = hAdjThr;
    return err;
}


/*****************************************************************************
functionname: FDKaacEnc_AdjThrInit
description:  initialize ADJ_THR_STATE
*****************************************************************************/
void FDKaacEnc_AdjThrInit(
        ADJ_THR_STATE   *hAdjThr,
        const INT       meanPe,
        ELEMENT_BITS    *elBits[(8)],
        INT             invQuant,
        INT             nElements,
        INT             nChannelsEff,
        INT             sampleRate,
        INT             advancedBitsToPe,
        FIXP_DBL        vbrQualFactor
        )
{
  INT i;

  FIXP_DBL POINT8 = FL2FXCONST_DBL(0.8f);
  FIXP_DBL POINT6 = FL2FXCONST_DBL(0.6f);

  /* common for all elements: */
  /* parameters for bitres control */
  hAdjThr->bresParamLong.clipSaveLow   = (FIXP_DBL)0x1999999a; /* FL2FXCONST_DBL(0.2f); */
  hAdjThr->bresParamLong.clipSaveHigh  = (FIXP_DBL)0x7999999a; /* FL2FXCONST_DBL(0.95f); */
  hAdjThr->bresParamLong.minBitSave    = (FIXP_DBL)0xf999999a; /* FL2FXCONST_DBL(-0.05f); */
  hAdjThr->bresParamLong.maxBitSave    = (FIXP_DBL)0x26666666; /* FL2FXCONST_DBL(0.3f); */
  hAdjThr->bresParamLong.clipSpendLow  = (FIXP_DBL)0x1999999a; /* FL2FXCONST_DBL(0.2f); */
  hAdjThr->bresParamLong.clipSpendHigh = (FIXP_DBL)0x7999999a; /* FL2FXCONST_DBL(0.95f); */
  hAdjThr->bresParamLong.minBitSpend   = (FIXP_DBL)0xf3333333; /* FL2FXCONST_DBL(-0.10f); */
  hAdjThr->bresParamLong.maxBitSpend   = (FIXP_DBL)0x33333333; /* FL2FXCONST_DBL(0.4f); */

  hAdjThr->bresParamShort.clipSaveLow   = (FIXP_DBL)0x199999a0; /* FL2FXCONST_DBL(0.2f); */
  hAdjThr->bresParamShort.clipSaveHigh  = (FIXP_DBL)0x5fffffff; /* FL2FXCONST_DBL(0.75f); */
  hAdjThr->bresParamShort.minBitSave    = (FIXP_DBL)0x00000000; /* FL2FXCONST_DBL(0.0f); */
  hAdjThr->bresParamShort.maxBitSave    = (FIXP_DBL)0x199999a0; /* FL2FXCONST_DBL(0.2f); */
  hAdjThr->bresParamShort.clipSpendLow  = (FIXP_DBL)0x199999a0; /* FL2FXCONST_DBL(0.2f); */
  hAdjThr->bresParamShort.clipSpendHigh = (FIXP_DBL)0x5fffffff; /* FL2FXCONST_DBL(0.75f); */
  hAdjThr->bresParamShort.minBitSpend   = (FIXP_DBL)0xf9999998; /* FL2FXCONST_DBL(-0.05f); */
  hAdjThr->bresParamShort.maxBitSpend   = (FIXP_DBL)0x40000000; /* FL2FXCONST_DBL(0.5f); */

  /* specific for each element: */
  for (i=0; i<nElements; i++) {
    ATS_ELEMENT* atsElem = hAdjThr->adjThrStateElem[i];
    MINSNR_ADAPT_PARAM *msaParam = &atsElem->minSnrAdaptParam;
    INT chBitrate = elBits[i]->chBitrateEl;

    /* parameters for bitres control */
    atsElem->peMin = fMultI(POINT8, meanPe) >> 1;
    atsElem->peMax = fMultI(POINT6, meanPe);

    /* for use in FDKaacEnc_reduceThresholdsVBR */
    atsElem->chaosMeasureOld = FL2FXCONST_DBL(0.3f);

    /* additional pe offset to correct pe2bits for low bitrates */
    atsElem->peOffset = 0;

    /* vbr initialisation */
    atsElem->vbrQualFactor = vbrQualFactor;
    if (chBitrate < 32000)
    {
      atsElem->peOffset = fixMax(50, 100-fMultI((FIXP_DBL)0x666667, chBitrate));
    }

    /* avoid hole parameters */
    if (chBitrate > 20000) {
      atsElem->ahParam.modifyMinSnr = TRUE;
      atsElem->ahParam.startSfbL = 15;
      atsElem->ahParam.startSfbS = 3;
    }
    else {
      atsElem->ahParam.modifyMinSnr = FALSE;
      atsElem->ahParam.startSfbL = 0;
      atsElem->ahParam.startSfbS = 0;
    }

    /* minSnr adaptation */
    msaParam->maxRed = FL2FXCONST_DBL(0.00390625f); /* 0.25f/64.0f */
    /* start adaptation of minSnr for avgEn/sfbEn > startRatio */
    msaParam->startRatio = FL2FXCONST_DBL(0.05190512648f); /* ld64(10.0f) */
    /* maximum minSnr reduction to minSnr^maxRed is reached for
       avgEn/sfbEn >= maxRatio */
    /* msaParam->maxRatio = 1000.0f; */
    /*msaParam->redRatioFac = ((float)1.0f - msaParam->maxRed) / ((float)10.0f*log10(msaParam->startRatio/msaParam->maxRatio)/log10(2.0f)*(float)0.3010299956f);*/
    msaParam->redRatioFac = FL2FXCONST_DBL(-0.375f); /* -0.0375f * 10.0f */
    /*msaParam->redOffs = (float)1.0f - msaParam->redRatioFac * (float)10.0f * log10(msaParam->startRatio)/log10(2.0f) * (float)0.3010299956f;*/
    msaParam->redOffs = FL2FXCONST_DBL(0.021484375); /* 1.375f/64.0f */

    /* init pe correction */
    atsElem->peCorrectionFactor_m = FL2FXCONST_DBL(0.5f); /* 1.0 */
    atsElem->peCorrectionFactor_e = 1;

    atsElem->dynBitsLast = -1;
    atsElem->peLast = 0;

    /* init bits to pe factor */

    /* init bits2PeFactor */
    FDKaacEnc_InitBits2PeFactor(
              &atsElem->bits2PeFactor_m,
              &atsElem->bits2PeFactor_e,
              chBitrate,       /* bitrate/channel*/
              nChannelsEff,    /* number of channels */
              sampleRate,
              advancedBitsToPe,
              invQuant
              );

  } /* for nElements */

}


/*****************************************************************************
    functionname: FDKaacEnc_FDKaacEnc_calcPeCorrection
    description:  calc desired pe
*****************************************************************************/
static void FDKaacEnc_FDKaacEnc_calcPeCorrection(
        FIXP_DBL *const           correctionFac_m,
        INT *const                correctionFac_e,
        const INT                 peAct,
        const INT                 peLast,
        const INT                 bitsLast,
        const FIXP_DBL            bits2PeFactor_m,
        const INT                 bits2PeFactor_e
        )
{
  if ( (bitsLast > 0) && (peAct < 1.5f*peLast) && (peAct > 0.7f*peLast) &&
       (FDKaacEnc_bits2pe2(bitsLast, fMult(FL2FXCONST_DBL(1.2f/2.f), bits2PeFactor_m), bits2PeFactor_e+1) > peLast) &&
       (FDKaacEnc_bits2pe2(bitsLast, fMult(FL2FXCONST_DBL(0.65f),    bits2PeFactor_m), bits2PeFactor_e  ) < peLast) )
  {
    FIXP_DBL corrFac = *correctionFac_m;

    int scaling = 0;
    FIXP_DBL denum = (FIXP_DBL)FDKaacEnc_bits2pe2(bitsLast, bits2PeFactor_m, bits2PeFactor_e);
    FIXP_DBL newFac = fDivNorm((FIXP_DBL)peLast, denum, &scaling);

    /* dead zone, newFac and corrFac are scaled by 0.5 */
    if ((FIXP_DBL)peLast <= denum) { /* ratio <= 1.f */
      newFac = fixMax(scaleValue(fixMin( fMult(FL2FXCONST_DBL(1.1f/2.f), newFac), scaleValue(FL2FXCONST_DBL(  1.f/2.f), -scaling)), scaling), FL2FXCONST_DBL(0.85f/2.f) );
    }
    else { /* ratio < 1.f */
     newFac = fixMax( fixMin( scaleValue(fMult(FL2FXCONST_DBL(0.9f/2.f), newFac), scaling), FL2FXCONST_DBL(1.15f/2.f) ), FL2FXCONST_DBL(   1.f/2.f) );
    }

    if (   ((newFac > FL2FXCONST_DBL(1.f/2.f)) && (corrFac < FL2FXCONST_DBL(1.f/2.f)))
        || ((newFac < FL2FXCONST_DBL(1.f/2.f)) && (corrFac > FL2FXCONST_DBL(1.f/2.f))))
    {
      corrFac = FL2FXCONST_DBL(1.f/2.f);
    }

    /* faster adaptation towards 1.0, slower in the other direction */
    if ( (corrFac < FL2FXCONST_DBL(1.f/2.f) && newFac < corrFac)
      || (corrFac > FL2FXCONST_DBL(1.f/2.f) && newFac > corrFac) )
    {
      corrFac = fMult(FL2FXCONST_DBL(0.85f), corrFac) + fMult(FL2FXCONST_DBL(0.15f), newFac);
    }
    else {
      corrFac = fMult(FL2FXCONST_DBL(0.7f), corrFac) + fMult(FL2FXCONST_DBL(0.3f), newFac);
    }

    corrFac = fixMax( fixMin( corrFac, FL2FXCONST_DBL(1.15f/2.f) ), FL2FXCONST_DBL(0.85/2.f) );

    *correctionFac_m = corrFac;
    *correctionFac_e = 1;
  }
  else {
    *correctionFac_m = FL2FXCONST_DBL(1.f/2.f);
    *correctionFac_e = 1;
  }
}


static void FDKaacEnc_calcPeCorrectionLowBitRes(
        FIXP_DBL *const           correctionFac_m,
        INT *const                correctionFac_e,
        const INT                 peLast,
        const INT                 bitsLast,
        const INT                 bitresLevel,
        const INT                 nChannels,
        const FIXP_DBL            bits2PeFactor_m,
        const INT                 bits2PeFactor_e
        )
{
  /* tuning params */
  const FIXP_DBL amp     = FL2FXCONST_DBL(0.005);
  const FIXP_DBL maxDiff = FL2FXCONST_DBL(0.25f);

  if (bitsLast > 0) {

    /* Estimate deviation of granted and used dynamic bits in previous frame, in PE units */
    const int bitsBalLast = peLast - FDKaacEnc_bits2pe2(
          bitsLast,
          bits2PeFactor_m,
          bits2PeFactor_e);

    /* reserve n bits per channel */
    int headroom = (bitresLevel>=50*nChannels) ? 0 : (100*nChannels);

    /* in PE units */
    headroom = FDKaacEnc_bits2pe2(
          headroom,
          bits2PeFactor_m,
          bits2PeFactor_e);

    /*
     * diff = amp * ((bitsBalLast - headroom) / (bitresLevel + headroom)
     * diff = max ( min ( diff, maxDiff, -maxDiff)) / 2
     */
    FIXP_DBL denominator = (FIXP_DBL)FDKaacEnc_bits2pe2(bitresLevel, bits2PeFactor_m, bits2PeFactor_e) + (FIXP_DBL)headroom;

    int scaling = 0;
    FIXP_DBL diff = (bitsBalLast>=headroom)
         ?  fMult(amp, fDivNorm( (FIXP_DBL)(bitsBalLast - headroom), denominator, &scaling))
         : -fMult(amp, fDivNorm(-(FIXP_DBL)(bitsBalLast - headroom), denominator, &scaling)) ;

    scaling -= 1; /* divide by 2 */

    diff = (scaling<=0) ? FDKmax( FDKmin (diff>>(-scaling), maxDiff>>1), -maxDiff>>1)
                        : FDKmax( FDKmin (diff, maxDiff>>(1+scaling)), -maxDiff>>(1+scaling)) << scaling;

    /*
     * corrFac += diff
     * corrFac = max ( min ( corrFac/2.f, 1.f/2.f, 0.75f/2.f ) )
     */
    *correctionFac_m = FDKmax(FDKmin((*correctionFac_m)+diff, FL2FXCONST_DBL(1.0f/2.f)), FL2FXCONST_DBL(0.75f/2.f)) ;
    *correctionFac_e = 1;
  }
  else {
    *correctionFac_m = FL2FXCONST_DBL(0.75/2.f);
    *correctionFac_e = 1;
  }
}

void FDKaacEnc_DistributeBits(ADJ_THR_STATE *adjThrState,
    ATS_ELEMENT       *AdjThrStateElement,
    PSY_OUT_CHANNEL   *psyOutChannel[(2)],
    PE_DATA           *peData,
    INT               *grantedPe,
    INT               *grantedPeCorr,
    const INT         nChannels,
    const INT         commonWindow,
    const INT         grantedDynBits,
    const INT         bitresBits,
    const INT         maxBitresBits,
    const FIXP_DBL    maxBitFac,
    const INT         bitDistributionMode)
{
  FIXP_DBL bitFactor;
  INT noRedPe = peData->pe;

  /* prefer short windows for calculation of bitFactor */
  INT curWindowSequence = LONG_WINDOW;
  if (nChannels==2) {
    if ((psyOutChannel[0]->lastWindowSequence == SHORT_WINDOW) ||
        (psyOutChannel[1]->lastWindowSequence == SHORT_WINDOW)) {
        curWindowSequence = SHORT_WINDOW;
    }
  }
  else {
    curWindowSequence = psyOutChannel[0]->lastWindowSequence;
  }

  if (grantedDynBits >= 1) {
    if (bitDistributionMode!=0) {
      *grantedPe = FDKaacEnc_bits2pe2(grantedDynBits, AdjThrStateElement->bits2PeFactor_m, AdjThrStateElement->bits2PeFactor_e);
    }
    else
    {
    /* factor dependend on current fill level and pe */
    bitFactor = FDKaacEnc_bitresCalcBitFac(bitresBits, maxBitresBits, noRedPe,
                                 curWindowSequence, grantedDynBits, maxBitFac,
                                 adjThrState,
                                 AdjThrStateElement
                                 );

    /* desired pe for actual frame */
    /* Worst case max of grantedDynBits is = 1024 * 5.27 * 2 */
    *grantedPe = FDKaacEnc_bits2pe2(grantedDynBits,
                     fMult(bitFactor, AdjThrStateElement->bits2PeFactor_m), AdjThrStateElement->bits2PeFactor_e+(DFRACT_BITS-1-Q_BITFAC)
                     );
    }
  }
  else {
    *grantedPe = 0; /* prevent divsion by 0 */
  }

  /* correction of pe value */
  switch (bitDistributionMode) {
  case 2:
  case 1:
    FDKaacEnc_calcPeCorrectionLowBitRes(
           &AdjThrStateElement->peCorrectionFactor_m,
           &AdjThrStateElement->peCorrectionFactor_e,
            AdjThrStateElement->peLast,
            AdjThrStateElement->dynBitsLast,
            bitresBits,
            nChannels,
            AdjThrStateElement->bits2PeFactor_m,
            AdjThrStateElement->bits2PeFactor_e
        );
    break;
  case 0:
  default:
      FDKaacEnc_FDKaacEnc_calcPeCorrection(
           &AdjThrStateElement->peCorrectionFactor_m,
           &AdjThrStateElement->peCorrectionFactor_e,
            fixMin(*grantedPe, noRedPe),
            AdjThrStateElement->peLast,
            AdjThrStateElement->dynBitsLast,
            AdjThrStateElement->bits2PeFactor_m,
            AdjThrStateElement->bits2PeFactor_e
            );
    break;
  }

  *grantedPeCorr = (INT)(fMult((FIXP_DBL)(*grantedPe<<Q_AVGBITS), AdjThrStateElement->peCorrectionFactor_m) >> (Q_AVGBITS-AdjThrStateElement->peCorrectionFactor_e));

  /* update last pe */
  AdjThrStateElement->peLast = *grantedPe;
  AdjThrStateElement->dynBitsLast = -1;

}

/*****************************************************************************
functionname: FDKaacEnc_AdjustThresholds
description:  adjust thresholds
*****************************************************************************/
void FDKaacEnc_AdjustThresholds(ATS_ELEMENT*        AdjThrStateElement[(8)],
                                QC_OUT_ELEMENT*     qcElement[(8)],
                                QC_OUT*             qcOut,
                                PSY_OUT_ELEMENT*    psyOutElement[(8)],
                                INT                 CBRbitrateMode,
                                CHANNEL_MAPPING*    cm)
{
    int i;
    if (CBRbitrateMode)
    {
        /* In case, no bits must be shifted between different elements, */
        /* an element-wise execution of the pe-dependent threshold- */
        /* adaption becomes necessary... */
            for (i=0; i<cm->nElements; i++)
            {
                ELEMENT_INFO elInfo = cm->elInfo[i];

                if ((elInfo.elType == ID_SCE) || (elInfo.elType == ID_CPE) ||
                    (elInfo.elType == ID_LFE))
                {
                    /* qcElement[i]->grantedPe = 2000; */  /* Use this only for debugging */
                    //if (totalGrantedPeCorr < totalNoRedPe) {
                    if (qcElement[i]->grantedPe < qcElement[i]->peData.pe)
                    {
                        /* calc threshold necessary for desired pe */
                        FDKaacEnc_adaptThresholdsToPe(cm,
                                            AdjThrStateElement,
                                            qcElement,
                                            psyOutElement,
                                            qcElement[i]->grantedPeCorr,
                                            1,         /* Process only 1 element */
                                            i);        /* Process exactly THIS element */

                    }

                }  /*  -end- if(ID_SCE || ID_CPE || ID_LFE) */

            }  /* -end- element loop */
    }
    else {
        for (i=0; i<cm->nElements; i++)
        {
            ELEMENT_INFO elInfo = cm->elInfo[i];

            if ((elInfo.elType == ID_SCE) || (elInfo.elType == ID_CPE) ||
                (elInfo.elType == ID_LFE))
            {
                  /* for VBR-mode */
                  FDKaacEnc_AdaptThresholdsVBR(qcElement[i]->qcOutChannel,
                                            psyOutElement[i]->psyOutChannel,
                                            AdjThrStateElement[i],
                                            &psyOutElement[i]->toolsInfo,
                                            &qcElement[i]->peData,
                                            cm->elInfo[i].nChannelsInEl);
            }  /*  -end- if(ID_SCE || ID_CPE || ID_LFE) */

        }  /* -end- element loop */

    }
    for (i=0; i<cm->nElements; i++) {
        int ch,sfb,sfbGrp;
        /* no weighting of threholds and energies for mlout */
        /* weight energies and thresholds */
        for (ch=0; ch<cm->elInfo[i].nChannelsInEl; ch++) {
            QC_OUT_CHANNEL* pQcOutCh = qcElement[i]->qcOutChannel[ch];
            for (sfbGrp = 0;sfbGrp < psyOutElement[i]->psyOutChannel[ch]->sfbCnt; sfbGrp+=psyOutElement[i]->psyOutChannel[ch]->sfbPerGroup) {
                for (sfb=0; sfb<psyOutElement[i]->psyOutChannel[ch]->maxSfbPerGroup; sfb++) {
                    pQcOutCh->sfbThresholdLdData[sfb+sfbGrp] += pQcOutCh->sfbEnFacLd[sfb+sfbGrp];
                }
            }
        }
    }
}

void FDKaacEnc_AdjThrClose(ADJ_THR_STATE** phAdjThr)
{
    INT i;
    ADJ_THR_STATE* hAdjThr = *phAdjThr;

    if (hAdjThr!=NULL) {
      for (i=0; i<(8); i++) {
        if (hAdjThr->adjThrStateElem[i]!=NULL) {
          FreeRam_aacEnc_AdjThrStateElement(&hAdjThr->adjThrStateElem[i]);
        }
      }
      FreeRam_aacEnc_AdjustThreshold(phAdjThr);
    }
}

