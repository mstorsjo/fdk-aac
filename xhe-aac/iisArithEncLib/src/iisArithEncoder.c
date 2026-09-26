
/* -----------------------------------------------------------------------------
Software Copyright License for The Fraunhofer FDK Extended High Efficiency AAC
Encoder Software for Android

© Copyright 1995 - 2025 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. and Contributors
All rights reserved.

1.    INTRODUCTION

The Fraunhofer FDK Extended High Efficiency AAC Encoder Software for Android
("FDK Extended High Efficiency AAC Encoder") is software that implements the
encoding of digital audio according to the MPEG-D Unified Speech and Audio
Coding (USAC) standard and MPEG-D Dynamic Range Control (DRC) standard. This FDK
Extended High Efficiency AAC Encoder Software is intended to be used on a wide
variety of Android devices. It is technically not suited to encode content for
digital radio broadcasting services, including DRM and similar standards.

Patent licenses for necessary patent claims for the FDK Extended High Efficiency
AAC Encoder Software (including those of Fraunhofer), for the use in commercial
products and services, may be obtained from the respective patent owners
individually and/or from Via Licensing Alliance (www.via-la.com).

Fraunhofer supports the development of Extended High Efficiency AAC products and
services by offering additional software, documentation, and technical advice.
In addition, it operates the xHE-AAC Trademark Program to ease interoperability
testing of end products. Please visit http://www.xhe-aac.com for more
information.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees, provided that you
satisfy the following conditions:

You must retain the complete text of this software license in redistributions of
the FDK Extended High Efficiency AAC Encoder Software or your modifications
thereto in source code form.

You must retain the complete text of this software license in the documentation
and/or other materials provided with redistributions of the FDK Extended High
Efficiency AAC Encoder Software or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the
FDK Extended High Efficiency AAC Encoder Software and your modifications thereto
to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived
from this software without prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute
the FDK Extended High Efficiency AAC Encoder Software or your modifications
thereto.

Your modified versions of the FDK Extended High Efficiency AAC Encoder Software
must carry prominent notices stating that you changed the software and the date
of any change. For modified versions of the FDK Extended High Efficiency AAC
Encoder Software, the term "Fraunhofer FDK Extended High Efficiency AAC Encoder
Software for Android" must be replaced by the term "Third-Party Modified Version
of the Fraunhofer FDK Extended High Efficiency AAC Encoder Software for
Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty for patent non-infringement with respect to this
software. You may use this FDK Extended High Efficiency AAC Encoder Software or
modifications thereto only for purposes that are authorized by appropriate
patent licenses.

4.    DISCLAIMER

This FDK Extended High Efficiency AAC Encoder Software is provided by Fraunhofer
on behalf of the copyright holders and contributors "AS IS" and WITHOUT ANY
EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied
warranties of merchantability and fitness for a particular purpose. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE for any direct, indirect,
incidental, special, exemplary, or consequential damages, including but not
limited to procurement of substitute goods or services; loss of use, data, or
profits, or business interruption, however caused and on any theory of
liability, whether in contract, strict liability, or tort (including
negligence), arising in any way out of the use of this software, even if advised
of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Division Audio and Media Technologies - FDK Extended High Efficiency
AAC Encoder
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------- */

#define ARITH_ENC_VERSION_NUMBER "02.02.05"

#define MAX_BIT_BUFFER_BITS 122880
#define MAX_BIT_BUFFER_BYTES 15360
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "iisArithEncoder.h"

#define PRIVATE_DATA_OFFSET_BYTES (sizeof(void *) - sizeof(struct arienc_public_data_struct) % sizeof(void *))

#ifndef min
#define min(a, b) ((a) <= (b) ? (a) : (b))
#endif

typedef struct arienc_private_data_struct {
  int size;
  int previous_N;
  unsigned int *q0;
  unsigned int *q1;
  unsigned int *save_q0;
  unsigned int *save_q1;
  unsigned int *save_aac_enc_q0;
  unsigned int *save_aac_enc_q1;
  int save_aac_enc_previous_N;
  int save_previous_N;
  unsigned int *reset_q0;
  unsigned int *reset_q1;
  int *x_ac_enc;
  int bitCount;
  int bitCountAtStart;
  unsigned char *pBitBuffer;
  char saveBitBuffer[MAX_BIT_BUFFER_BYTES + 1];
  unsigned char internalBitBuffer[MAX_BIT_BUFFER_BYTES + 1];
  unsigned int qBuffer[2048 * 8];
  unsigned int high;
  unsigned int low;
  unsigned int bits_to_follow;
  unsigned int arith_shift_reg_cnt;
  unsigned int arith_shift_reg;

  unsigned int save_high;
  unsigned int save_low;
  int save_bitCount;
  unsigned int save_bits_to_follow;
  unsigned int save_arith_shift_reg_cnt;
  unsigned int save_arith_shift_reg;
  int save_pki;
  int save_c;

  const unsigned short *arith_lookup_m;
  int arith_lookup_m_length;
  const unsigned char *arith_hash_m_direct_return;

  const unsigned int *arith_hash_m;
  int arith_hash_m_length;

  const unsigned short *arith_cf_m[128];
  int arith_cf_m_no_prob_models;
  int arith_cf_m_no_elements;

  int context_saved;
} ARIENC_PRIVATE_DATA;

static const unsigned short arith_lookup_m_USAC[742] = {
    0x01, 0x34, 0x0D, 0x13, 0x12, 0x25, 0x00, 0x3A, 0x05, 0x00, 0x21, 0x13, 0x1F, 0x1A, 0x1D, 0x36,
    0x24, 0x2B, 0x1B, 0x33, 0x37, 0x29, 0x1D, 0x33, 0x37, 0x33, 0x37, 0x33, 0x37, 0x33, 0x2C, 0x00,
    0x21, 0x13, 0x25, 0x2A, 0x00, 0x21, 0x24, 0x12, 0x2C, 0x1E, 0x37, 0x24, 0x1F, 0x35, 0x37, 0x24,
    0x35, 0x37, 0x35, 0x37, 0x38, 0x2D, 0x21, 0x29, 0x1E, 0x21, 0x13, 0x2D, 0x36, 0x38, 0x29, 0x36,
    0x37, 0x24, 0x36, 0x38, 0x37, 0x38, 0x00, 0x20, 0x23, 0x20, 0x23, 0x36, 0x38, 0x24, 0x3B, 0x24,
    0x26, 0x29, 0x1F, 0x30, 0x2D, 0x0D, 0x12, 0x3F, 0x2D, 0x21, 0x1C, 0x2A, 0x00, 0x21, 0x12, 0x1E,
    0x36, 0x38, 0x36, 0x37, 0x3F, 0x1E, 0x0D, 0x1F, 0x2A, 0x1E, 0x21, 0x24, 0x12, 0x2A, 0x3C, 0x21,
    0x24, 0x1F, 0x3C, 0x21, 0x29, 0x36, 0x38, 0x36, 0x37, 0x38, 0x21, 0x1E, 0x00, 0x3B, 0x25, 0x1E,
    0x20, 0x10, 0x1F, 0x3C, 0x20, 0x23, 0x29, 0x08, 0x23, 0x12, 0x08, 0x23, 0x21, 0x38, 0x00, 0x20,
    0x13, 0x20, 0x3B, 0x1C, 0x20, 0x3B, 0x29, 0x20, 0x23, 0x24, 0x21, 0x24, 0x21, 0x24, 0x3B, 0x13,
    0x23, 0x26, 0x23, 0x13, 0x21, 0x24, 0x26, 0x29, 0x12, 0x22, 0x2B, 0x02, 0x1E, 0x0D, 0x1F, 0x2D,
    0x00, 0x0D, 0x12, 0x00, 0x3C, 0x21, 0x29, 0x3C, 0x21, 0x2A, 0x3C, 0x3B, 0x22, 0x1E, 0x20, 0x10,
    0x1F, 0x3C, 0x0D, 0x29, 0x3C, 0x21, 0x24, 0x08, 0x23, 0x20, 0x38, 0x39, 0x3C, 0x20, 0x13, 0x3C,
    0x00, 0x0D, 0x13, 0x1F, 0x3C, 0x09, 0x26, 0x1F, 0x08, 0x09, 0x26, 0x12, 0x08, 0x23, 0x29, 0x20,
    0x23, 0x21, 0x24, 0x20, 0x13, 0x20, 0x3B, 0x16, 0x20, 0x3B, 0x29, 0x20, 0x3B, 0x29, 0x20, 0x3B,
    0x13, 0x21, 0x24, 0x29, 0x0B, 0x13, 0x09, 0x3B, 0x13, 0x09, 0x3B, 0x13, 0x21, 0x3B, 0x13, 0x0D,
    0x26, 0x29, 0x26, 0x29, 0x3D, 0x12, 0x22, 0x28, 0x2E, 0x04, 0x08, 0x13, 0x3C, 0x3B, 0x3C, 0x20,
    0x10, 0x3C, 0x21, 0x07, 0x08, 0x10, 0x00, 0x08, 0x0D, 0x29, 0x08, 0x0D, 0x29, 0x08, 0x09, 0x13,
    0x20, 0x23, 0x39, 0x08, 0x09, 0x13, 0x08, 0x09, 0x16, 0x08, 0x09, 0x10, 0x12, 0x20, 0x3B, 0x3D,
    0x09, 0x26, 0x20, 0x3B, 0x24, 0x39, 0x09, 0x26, 0x20, 0x0D, 0x13, 0x00, 0x09, 0x13, 0x20, 0x0D,
    0x26, 0x12, 0x20, 0x3B, 0x13, 0x21, 0x26, 0x0B, 0x12, 0x09, 0x3B, 0x16, 0x09, 0x3B, 0x3D, 0x09,
    0x26, 0x0D, 0x13, 0x26, 0x3D, 0x1C, 0x12, 0x1F, 0x28, 0x2E, 0x07, 0x0B, 0x08, 0x09, 0x00, 0x39,
    0x0B, 0x08, 0x26, 0x08, 0x09, 0x13, 0x20, 0x0B, 0x39, 0x10, 0x39, 0x0D, 0x13, 0x20, 0x10, 0x12,
    0x09, 0x13, 0x20, 0x3B, 0x13, 0x09, 0x26, 0x0B, 0x09, 0x3B, 0x1C, 0x09, 0x3B, 0x13, 0x20, 0x3B,
    0x13, 0x09, 0x26, 0x0B, 0x16, 0x0D, 0x13, 0x09, 0x13, 0x09, 0x13, 0x26, 0x3D, 0x1C, 0x1F, 0x28,
    0x2E, 0x07, 0x10, 0x39, 0x0B, 0x39, 0x39, 0x13, 0x39, 0x0B, 0x39, 0x0B, 0x39, 0x26, 0x39, 0x10,
    0x20, 0x3B, 0x16, 0x20, 0x10, 0x09, 0x26, 0x0B, 0x13, 0x09, 0x13, 0x26, 0x1C, 0x0B, 0x3D, 0x1C,
    0x1F, 0x28, 0x2B, 0x07, 0x0C, 0x39, 0x0B, 0x39, 0x0B, 0x0C, 0x0B, 0x26, 0x0B, 0x26, 0x3D, 0x0D,
    0x1C, 0x14, 0x28, 0x2B, 0x39, 0x0B, 0x0C, 0x0E, 0x3D, 0x1C, 0x0D, 0x12, 0x22, 0x2B, 0x07, 0x0C,
    0x0E, 0x3D, 0x1C, 0x10, 0x1F, 0x2B, 0x0C, 0x0E, 0x19, 0x14, 0x10, 0x1F, 0x28, 0x0C, 0x0E, 0x19,
    0x14, 0x26, 0x22, 0x2B, 0x0C, 0x0E, 0x19, 0x14, 0x26, 0x28, 0x0E, 0x19, 0x14, 0x26, 0x28, 0x0E,
    0x19, 0x14, 0x28, 0x0E, 0x19, 0x14, 0x22, 0x28, 0x2B, 0x0E, 0x14, 0x2B, 0x31, 0x00, 0x3A, 0x3A,
    0x05, 0x05, 0x1B, 0x1D, 0x33, 0x06, 0x35, 0x35, 0x20, 0x21, 0x37, 0x21, 0x24, 0x05, 0x1B, 0x2C,
    0x2C, 0x2C, 0x06, 0x34, 0x1E, 0x34, 0x00, 0x08, 0x36, 0x09, 0x21, 0x26, 0x1C, 0x2C, 0x00, 0x02,
    0x02, 0x02, 0x3F, 0x04, 0x04, 0x04, 0x34, 0x39, 0x20, 0x0A, 0x0C, 0x39, 0x0B, 0x0F, 0x07, 0x07,
    0x07, 0x07, 0x34, 0x39, 0x39, 0x0A, 0x0C, 0x39, 0x0C, 0x0F, 0x07, 0x07, 0x07, 0x00, 0x39, 0x39,
    0x0C, 0x0F, 0x07, 0x07, 0x39, 0x0C, 0x0F, 0x07, 0x39, 0x0C, 0x0F, 0x39, 0x39, 0x0C, 0x0F, 0x39,
    0x0C, 0x39, 0x0C, 0x0F, 0x00, 0x11, 0x27, 0x17, 0x2F, 0x27, 0x00, 0x27, 0x17, 0x00, 0x11, 0x17,
    0x00, 0x11, 0x17, 0x11, 0x00, 0x27, 0x15, 0x11, 0x17, 0x01, 0x15, 0x11, 0x15, 0x11, 0x15, 0x15,
    0x17, 0x00, 0x27, 0x01, 0x27, 0x27, 0x15, 0x00, 0x27, 0x11, 0x27, 0x15, 0x15, 0x15, 0x27, 0x15,
    0x15, 0x15, 0x15, 0x17, 0x2F, 0x11, 0x17, 0x27, 0x27, 0x27, 0x11, 0x27, 0x15, 0x27, 0x27, 0x15,
    0x15, 0x27, 0x17, 0x2F, 0x27, 0x17, 0x2F, 0x27, 0x17, 0x2F, 0x27, 0x17, 0x2F, 0x27, 0x17, 0x2F,
    0x27, 0x17, 0x2F, 0x27, 0x17, 0x2F, 0x27, 0x17, 0x2F, 0x27, 0x17, 0x2F, 0x27, 0x17, 0x2F, 0x27,
    0x17, 0x2F, 0x27, 0x17, 0x2F, 0x27, 0x17, 0x2F, 0x17, 0x2F, 0x2B, 0x00, 0x27, 0x00, 0x00, 0x11,
    0x15, 0x00, 0x11, 0x11, 0x27, 0x27, 0x15, 0x17, 0x15, 0x17, 0x15, 0x17, 0x27, 0x17, 0x27, 0x17,
    0x27, 0x17, 0x27, 0x17, 0x27, 0x17, 0x27, 0x17, 0x27, 0x17, 0x27, 0x17, 0x27, 0x17, 0x27, 0x17,
    0x27, 0x15, 0x27, 0x27, 0x15, 0x27};

static const unsigned int arith_hash_m_USAC[742] = {
    0x0A340401UL, 0x1613100DUL, 0x251F1212UL, 0x2E252525UL, 0x0000A49FUL, 0x0000F96EUL, 0x00111000UL, 0x01111E83UL,
    0x01113146UL, 0x01114036UL, 0x01116863UL, 0x011194E9UL, 0x0111F7EEUL, 0x0112269BUL, 0x01124775UL, 0x01126DA1UL,
    0x0112D912UL, 0x01131AF0UL, 0x011336DDUL, 0x01135CF5UL, 0x01139DF8UL, 0x01141A5BUL, 0x01144773UL, 0x01146CF5UL,
    0x0114FDE9UL, 0x01166CF5UL, 0x0116FDE4UL, 0x01174CF3UL, 0x011FFDCFUL, 0x01211CC2UL, 0x01213B2DUL, 0x01214036UL,
    0x01216863UL, 0x012194D2UL, 0x0122197FUL, 0x01223AADUL, 0x01224036UL, 0x01226878UL, 0x0122A929UL, 0x0122F4ABUL,
    0x01232B2DUL, 0x012347B6UL, 0x01237DF8UL, 0x0123B929UL, 0x012417DDUL, 0x01245D76UL, 0x01249DF8UL, 0x0124F912UL,
    0x01255D75UL, 0x0125FDE9UL, 0x01265D75UL, 0x012B8DF7UL, 0x01311E2AUL, 0x01313B5EUL, 0x0131687BUL, 0x01321A6DUL,
    0x013237BCUL, 0x01326863UL, 0x0132F4EEUL, 0x01332B5EUL, 0x01335DA1UL, 0x01338E24UL, 0x01341A5EUL, 0x01343DB6UL,
    0x01348DF8UL, 0x01351935UL, 0x01355DB7UL, 0x0135FE12UL, 0x01376DF7UL, 0x013FFE29UL, 0x01400024UL, 0x01423821UL,
    0x014318F6UL, 0x01433821UL, 0x0143F8E5UL, 0x01443DA1UL, 0x01486E38UL, 0x014FF929UL, 0x01543EE3UL, 0x015FF912UL,
    0x016F298CUL, 0x018A5A69UL, 0x021007F1UL, 0x02112C2CUL, 0x02114B48UL, 0x02117353UL, 0x0211F4AEUL, 0x02122FEAUL,
    0x02124B48UL, 0x02127850UL, 0x0212F72EUL, 0x02133A9EUL, 0x02134036UL, 0x02138864UL, 0x021414ADUL, 0x0214379EUL,
    0x02145DB6UL, 0x0214FE1FUL, 0x02166DB7UL, 0x02200DC4UL, 0x02212FEAUL, 0x022147A0UL, 0x02218369UL, 0x0221F7EEUL,
    0x02222AADUL, 0x02224788UL, 0x02226863UL, 0x02229929UL, 0x0222F4ABUL, 0x02232A9EUL, 0x02234F08UL, 0x02237864UL,
    0x0223A929UL, 0x022417DEUL, 0x02244F36UL, 0x02248863UL, 0x02251A74UL, 0x02256DA1UL, 0x0225FE12UL, 0x02263DB6UL,
    0x02276DF7UL, 0x022FFE29UL, 0x0231186DUL, 0x023137BCUL, 0x02314020UL, 0x02319ED6UL, 0x0232196DUL, 0x023237BCUL,
    0x02325809UL, 0x02329429UL, 0x023317EDUL, 0x02333F08UL, 0x02335809UL, 0x023378E4UL, 0x02341A7CUL, 0x02344221UL,
    0x0234A8D3UL, 0x023514BCUL, 0x02354221UL, 0x0235F8DFUL, 0x02364861UL, 0x023FFE29UL, 0x02400024UL, 0x0241583BUL,
    0x024214C8UL, 0x02424809UL, 0x02427EE6UL, 0x02431708UL, 0x02434809UL, 0x02436ED0UL, 0x02441A76UL, 0x02443821UL,
    0x024458E3UL, 0x0244F91FUL, 0x02454863UL, 0x0246190AUL, 0x02464863UL, 0x024FF929UL, 0x02525ED0UL, 0x025314E1UL,
    0x025348FBUL, 0x025419A1UL, 0x025458D0UL, 0x0254F4E5UL, 0x02552861UL, 0x025FF912UL, 0x02665993UL, 0x027F5A69UL,
    0x029F1481UL, 0x02CF28A4UL, 0x03100AF0UL, 0x031120AAUL, 0x031147A0UL, 0x03118356UL, 0x031217ECUL, 0x03123B5EUL,
    0x03124008UL, 0x03127350UL, 0x031314AAUL, 0x0313201EUL, 0x03134F08UL, 0x03136863UL, 0x03141A5EUL, 0x03143F3CUL,
    0x03200847UL, 0x03212AADUL, 0x03214F20UL, 0x03218ED6UL, 0x032218ADUL, 0x032237BCUL, 0x03225809UL, 0x03229416UL,
    0x032317EDUL, 0x03234F20UL, 0x03237350UL, 0x0323FA6BUL, 0x03243F08UL, 0x03246863UL, 0x0324F925UL, 0x03254221UL,
    0x0325F8DFUL, 0x03264821UL, 0x032FFE29UL, 0x03311E47UL, 0x03313F08UL, 0x0331580DUL, 0x033214DEUL, 0x03323F08UL,
    0x03324020UL, 0x03326350UL, 0x033294E9UL, 0x033317DEUL, 0x03333F08UL, 0x0333627BUL, 0x0333A9A9UL, 0x033417FCUL,
    0x03343220UL, 0x0334627BUL, 0x0334A9A9UL, 0x0335148AUL, 0x03353220UL, 0x033588E4UL, 0x03361A4AUL, 0x03363821UL,
    0x0336F8D2UL, 0x03376863UL, 0x03411939UL, 0x0341583BUL, 0x034214C8UL, 0x03424809UL, 0x03426ED0UL, 0x03431588UL,
    0x03434809UL, 0x03436ED0UL, 0x03441A48UL, 0x0344480DUL, 0x03446ED0UL, 0x03451A4AUL, 0x03453809UL, 0x03455EFBUL,
    0x034614CAUL, 0x03463849UL, 0x034F8924UL, 0x03500A69UL, 0x035252D0UL, 0x035314E0UL, 0x0353324DUL, 0x03535ED0UL,
    0x035414E0UL, 0x0354324DUL, 0x03545ED0UL, 0x0354F4E8UL, 0x0355384DUL, 0x03555ED0UL, 0x0355F4DFUL, 0x03564350UL,
    0x035969A6UL, 0x035FFA52UL, 0x036649A6UL, 0x036FFA52UL, 0x037F4F66UL, 0x039D7492UL, 0x03BF6892UL, 0x03DF8A1FUL,
    0x04100B84UL, 0x04112107UL, 0x0411520DUL, 0x041214EAUL, 0x04124F20UL, 0x04131EDEUL, 0x04133F08UL, 0x04135809UL,
    0x0413F42BUL, 0x04142F08UL, 0x04200847UL, 0x042121FCUL, 0x04214209UL, 0x04221407UL, 0x0422203CUL, 0x04224209UL,
    0x04226350UL, 0x04231A7CUL, 0x04234209UL, 0x0423637BUL, 0x04241A7CUL, 0x04243220UL, 0x0424627BUL, 0x042514C8UL,
    0x04254809UL, 0x042FF8E9UL, 0x04311E47UL, 0x04313220UL, 0x0431527BUL, 0x043214FCUL, 0x04323220UL, 0x04326250UL,
    0x043315BCUL, 0x04333220UL, 0x0433527BUL, 0x04338413UL, 0x04341488UL, 0x04344809UL, 0x04346ED0UL, 0x04351F48UL,
    0x0435527BUL, 0x0435F9A5UL, 0x04363809UL, 0x04375EFBUL, 0x043FF929UL, 0x04412E79UL, 0x0441427BUL, 0x044219B9UL,
    0x04423809UL, 0x0442537BUL, 0x044314C8UL, 0x04432020UL, 0x0443527BUL, 0x044414CAUL, 0x04443809UL, 0x0444537BUL,
    0x04448993UL, 0x0445148AUL, 0x04453809UL, 0x04455ED0UL, 0x0445F4E5UL, 0x0446384DUL, 0x045009A6UL, 0x045272D3UL,
    0x045314A0UL, 0x0453324DUL, 0x04535ED0UL, 0x045415A0UL, 0x0454324DUL, 0x04545ED0UL, 0x04551F60UL, 0x0455324DUL,
    0x04562989UL, 0x04564350UL, 0x045FF4D2UL, 0x04665993UL, 0x047FFF62UL, 0x048FF725UL, 0x049F44BDUL, 0x04BFB7E5UL,
    0x04EF8A25UL, 0x04FFFB98UL, 0x051131F9UL, 0x051212C7UL, 0x05134209UL, 0x05200247UL, 0x05211007UL, 0x05213E60UL,
    0x052212C7UL, 0x05224209UL, 0x052319BCUL, 0x05233220UL, 0x0523527BUL, 0x052414C8UL, 0x05243820UL, 0x053112F9UL,
    0x05313E49UL, 0x05321439UL, 0x05323E49UL, 0x0532537BUL, 0x053314C8UL, 0x0533480DUL, 0x05337413UL, 0x05341488UL,
    0x0534527BUL, 0x0534F4EBUL, 0x05353809UL, 0x05356ED0UL, 0x0535F4E5UL, 0x0536427BUL, 0x054119B9UL, 0x054212F9UL,
    0x05423249UL, 0x05426ED3UL, 0x05431739UL, 0x05433249UL, 0x05435ED0UL, 0x0543F4EBUL, 0x05443809UL, 0x05445ED0UL,
    0x0544F4E8UL, 0x0545324DUL, 0x054FF992UL, 0x055362D3UL, 0x0553F5ABUL, 0x05544350UL, 0x055514CAUL, 0x0555427BUL,
    0x0555F4E5UL, 0x0556327BUL, 0x055FF4D2UL, 0x05665993UL, 0x05774F53UL, 0x059FF728UL, 0x05CC37FDUL, 0x05EFBA28UL,
    0x05FFFB98UL, 0x061131F9UL, 0x06121407UL, 0x06133E60UL, 0x061A72E4UL, 0x06211E47UL, 0x06214E4BUL, 0x062214C7UL,
    0x06223E60UL, 0x062312F9UL, 0x06233E60UL, 0x063112F9UL, 0x06313E4CUL, 0x063219B9UL, 0x06323E49UL, 0x06331439UL,
    0x06333809UL, 0x06336EE6UL, 0x0633F5ABUL, 0x06343809UL, 0x0634F42BUL, 0x0635427BUL, 0x063FF992UL, 0x064342FBUL,
    0x0643F4EBUL, 0x0644427BUL, 0x064524C9UL, 0x06655993UL, 0x0666170AUL, 0x066652E6UL, 0x067A6F56UL, 0x0698473DUL,
    0x06CF67D2UL, 0x06EF3A26UL, 0x06FFFAD8UL, 0x071131CCUL, 0x07211307UL, 0x07222E79UL, 0x072292DCUL, 0x07234E4BUL,
    0x073112F9UL, 0x07322339UL, 0x073632CBUL, 0x073FF992UL, 0x074432CBUL, 0x075549A6UL, 0x0776FF68UL, 0x07774350UL,
    0x0788473DUL, 0x07CF4516UL, 0x07EF3A26UL, 0x07FFFAD8UL, 0x08222E79UL, 0x083112F9UL, 0x0834330BUL, 0x0845338BUL,
    0x08756F5CUL, 0x0887F725UL, 0x08884366UL, 0x08AF649CUL, 0x08F00898UL, 0x08FFFAD8UL, 0x091111C7UL, 0x0932330BUL,
    0x0945338BUL, 0x09774F7DUL, 0x0998C725UL, 0x09996416UL, 0x09EF87E5UL, 0x09FFFAD8UL, 0x0A34330BUL, 0x0A45338BUL,
    0x0A77467DUL, 0x0AA9F52BUL, 0x0AAA6416UL, 0x0ABD67DFUL, 0x0AFFFA18UL, 0x0B33330BUL, 0x0B4443A6UL, 0x0B76467DUL,
    0x0BB9751FUL, 0x0BBB59BDUL, 0x0BEF5892UL, 0x0BFFFAD8UL, 0x0C221339UL, 0x0C53338EUL, 0x0C76367DUL, 0x0CCAF52EUL,
    0x0CCC6996UL, 0x0CFFFA18UL, 0x0D44438EUL, 0x0D64264EUL, 0x0DDCF52EUL, 0x0DDD5996UL, 0x0DFFFA18UL, 0x0E43338EUL,
    0x0E68465CUL, 0x0EEE651CUL, 0x0EFFFA18UL, 0x0F33238EUL, 0x0F553659UL, 0x0F8F451CUL, 0x0FAFF8AEUL, 0x0FF00A2EUL,
    0x0FFF1ACCUL, 0x0FFF33BDUL, 0x0FFF7522UL, 0x0FFFFAD8UL, 0x10002C72UL, 0x1111103EUL, 0x11121E83UL, 0x11131E9AUL,
    0x1121115AUL, 0x11221170UL, 0x112316F0UL, 0x1124175DUL, 0x11311CC2UL, 0x11321182UL, 0x11331D42UL, 0x11411D48UL,
    0x11421836UL, 0x11431876UL, 0x11441DF5UL, 0x1152287BUL, 0x12111903UL, 0x1212115AUL, 0x121316F0UL, 0x12211B30UL,
    0x12221B30UL, 0x12231B02UL, 0x12311184UL, 0x12321D04UL, 0x12331784UL, 0x12411D39UL, 0x12412020UL, 0x12422220UL,
    0x12511D89UL, 0x1252227BUL, 0x1258184AUL, 0x12832992UL, 0x1311171AUL, 0x13121B30UL, 0x1312202CUL, 0x131320AAUL,
    0x132120AAUL, 0x132220ADUL, 0x13232FEDUL, 0x13312107UL, 0x13322134UL, 0x13332134UL, 0x13411D39UL, 0x13431E74UL,
    0x13441834UL, 0x134812B4UL, 0x1352230BUL, 0x13611E4BUL, 0x136522E4UL, 0x141113C2UL, 0x141211C4UL, 0x143121F9UL,
    0x143221F9UL, 0x143321CAUL, 0x14351D34UL, 0x14431E47UL, 0x14441E74UL, 0x144612B4UL, 0x1452230EUL, 0x14551E74UL,
    0x1471130EUL, 0x151113C2UL, 0x152121F9UL, 0x153121F9UL, 0x153221F9UL, 0x15331007UL, 0x15522E4EUL, 0x15551E74UL,
    0x1571130EUL, 0x161113C7UL, 0x162121F9UL, 0x163121F9UL, 0x16611E79UL, 0x16661334UL, 0x171113C7UL, 0x172121F9UL,
    0x17451E47UL, 0x1771130CUL, 0x181113C7UL, 0x18211E47UL, 0x18511E4CUL, 0x1882130CUL, 0x191113C7UL, 0x19331E79UL,
    0x1A111307UL, 0x1A311E79UL, 0x1F52230EUL, 0x200003C1UL, 0x20001027UL, 0x20004467UL, 0x200079E7UL, 0x2000E5EFUL,
    0x21100BC0UL, 0x211129C0UL, 0x21114011UL, 0x211189E7UL, 0x2111F5EFUL, 0x21124011UL, 0x21127455UL, 0x211325C0UL,
    0x21134011UL, 0x21137455UL, 0x211425C0UL, 0x21212440UL, 0x21213001UL, 0x2121F9EFUL, 0x21222540UL, 0x21226455UL,
    0x2122F5EFUL, 0x21233051UL, 0x2123F56FUL, 0x21244451UL, 0x21312551UL, 0x21323451UL, 0x21332551UL, 0x21844555UL,
    0x221125C0UL, 0x22113011UL, 0x2211F9EFUL, 0x22123051UL, 0x2212F9EFUL, 0x221329D1UL, 0x22212541UL, 0x22213011UL,
    0x2221F9EFUL, 0x22223451UL, 0x2222F9EFUL, 0x22232551UL, 0x2223F56FUL, 0x22312551UL, 0x223229D1UL, 0x2232F56FUL,
    0x22332551UL, 0x2233F56FUL, 0x22875555UL, 0x22DAB5D7UL, 0x23112BD1UL, 0x23115467UL, 0x231225D1UL, 0x232129D1UL,
    0x232229D1UL, 0x2322F9EFUL, 0x23233451UL, 0x2323F9EFUL, 0x23312551UL, 0x233229D1UL, 0x2332F9EFUL, 0x2333F56FUL,
    0x237FF557UL, 0x238569D5UL, 0x23D955D7UL, 0x24100BE7UL, 0x248789E7UL, 0x24E315D7UL, 0x24FFFBEFUL, 0x259869E7UL,
    0x25DFF5EFUL, 0x25FFFBEFUL, 0x268789E7UL, 0x26DFA5D7UL, 0x26FFFBEFUL, 0x279649E7UL, 0x27E425D7UL, 0x27FFFBEFUL,
    0x288879E7UL, 0x28EFF5EFUL, 0x28FFFBEFUL, 0x298439E7UL, 0x29F115EFUL, 0x29FFFBEFUL, 0x2A7659E7UL, 0x2AEF75D7UL,
    0x2AFFFBEFUL, 0x2B7C89E7UL, 0x2BEF95D7UL, 0x2BFFFBEFUL, 0x2C6659E7UL, 0x2CD555D7UL, 0x2CFFFBEFUL, 0x2D6329E7UL,
    0x2DDD55E7UL, 0x2DFFFBEBUL, 0x2E8479D7UL, 0x2EEE35E7UL, 0x2EFFFBEFUL, 0x2F5459E7UL, 0x2FCF85D7UL, 0x2FFEFBEBUL,
    0x2FFFA5EFUL, 0x2FFFEBEFUL, 0x30001AE7UL, 0x30002001UL, 0x311129C0UL, 0x31221015UL, 0x31232000UL, 0x31332451UL,
    0x32112540UL, 0x32131027UL, 0x32212440UL, 0x33452455UL, 0x4000F9D7UL, 0x4122F9D7UL, 0x43F65555UL, 0x43FFF5D7UL,
    0x44F55567UL, 0x44FFF5D7UL, 0x45F00557UL, 0x45FFF5D7UL, 0x46F659D7UL, 0x471005E7UL, 0x47F449E7UL, 0x481005E7UL,
    0x48EFA9D5UL, 0x48FFF5EFUL, 0x49F449E7UL, 0x49FFF5EFUL, 0x4AEA79E7UL, 0x4AFFF5EFUL, 0x4BE9C9D5UL, 0x4BFFF5EFUL,
    0x4CE549E7UL, 0x4CFFF5EFUL, 0x4DE359E7UL, 0x4DFFF5D7UL, 0x4EE469E7UL, 0x4EFFF5D7UL, 0x4FEF39E7UL, 0x4FFFF5EFUL,
    0x6000F9E7UL, 0x69FFF557UL, 0x6FFFF9D7UL, 0x811009D7UL, 0x8EFFF555UL, 0xFFFFF9E7UL};
static const unsigned char arith_lookup_m_USAC_direct_return[16] = {
    0x01,
    0x04,
    0x34,
    0x0A,
    0x0D,
    0x10,
    0x13,
    0x16,
    0x12,
    0x12,
    0x1F,
    0x25,
    0x25,
    0x25,
    0x25,
    0x2E,
};

static const unsigned short arith_cf_m_USAC[64][17] = {
    {708, 706, 579, 569, 568, 567, 479, 469, 297, 138, 97, 91, 72, 52, 38, 34, 0},
    {7619, 6917, 6519, 6412, 5514, 5003, 4683, 4563, 3907, 3297, 3125, 3060, 2904, 2718, 2631, 2590, 0},
    {7263, 4888, 4810, 4803, 1889, 415, 335, 327, 195, 72, 52, 49, 36, 20, 15, 14, 0},
    {3626, 2197, 2188, 2187, 582, 57, 47, 46, 30, 12, 9, 8, 6, 4, 3, 2, 0},
    {7806, 5541, 5451, 5441, 2720, 834, 691, 674, 487, 243, 179, 167, 139, 98, 77, 70, 0},
    {6684, 4101, 4058, 4055, 1748, 426, 368, 364, 322, 257, 235, 232, 228, 222, 217, 215, 0},
    {9162, 5964, 5831, 5819, 3269, 866, 658, 638, 535, 348, 258, 244, 234, 214, 195, 186, 0},
    {10638, 8491, 8365, 8351, 4418, 2067, 1859, 1834, 1190, 601, 495, 478, 356, 217, 174, 164, 0},
    {13389, 10514, 10032, 9961, 7166, 3488, 2655, 2524, 2015, 1140, 760, 672, 585, 426, 325, 283, 0},
    {14861, 12788, 12115, 11952, 9987, 6657, 5323, 4984, 4324, 3001, 2205, 1943, 1764, 1394, 1115, 978, 0},
    {12876, 10004, 9661, 9610, 7107, 3435, 2711, 2595, 2257, 1508, 1059, 952, 893, 753, 609, 538, 0},
    {15125, 13591, 13049, 12874, 11192, 8543, 7406, 7023, 6291, 4922, 4104, 3769, 3465, 2890, 2486, 2275, 0},
    {14574, 13106, 12731, 12638, 10453, 7947, 7233, 7037, 6031, 4618, 4081, 3906, 3465, 2802, 2476, 2349, 0},
    {15070, 13179, 12517, 12351, 10742, 7657, 6200, 5825, 5264, 3998, 3014, 2662, 2510, 2153, 1799, 1564, 0},
    {15542, 14466, 14007, 13844, 12489, 10409, 9481, 9132, 8305, 6940, 6193, 5867, 5458, 4743, 4291, 4047, 0},
    {15165, 14384, 14084, 13934, 12911, 11485, 10844, 10513, 10002, 8993, 8380, 8051, 7711, 7036, 6514, 6233, 0},
    {15642, 14279, 13625, 13393, 12348, 9971, 8405, 7858, 7335, 6119, 4918, 4376, 4185, 3719, 3231, 2860, 0},
    {13408, 13407, 11471, 11218, 11217, 11216, 9473, 9216, 6480, 3689, 2857, 2690, 2256, 1732, 1405, 1302, 0},
    {16098, 15584, 15191, 14931, 14514, 13578, 12703, 12103, 11830, 11172, 10475, 9867, 9695, 9281, 8825, 8389, 0},
    {15844, 14873, 14277, 13996, 13230, 11535, 10205, 9543, 9107, 8086, 7085, 6419, 6214, 5713, 5195, 4731, 0},
    {16131, 15720, 15443, 15276, 14848, 13971, 13314, 12910, 12591, 11874, 11225, 10788, 10573, 10077, 9585, 9209, 0},
    {16331, 16330, 12283, 11435, 11434, 11433, 8725, 8049, 6065, 4138, 3187, 2842, 2529, 2171, 1907, 1745, 0},
    {16011, 15292, 14782, 14528, 14008, 12767, 11556, 10921, 10591, 9759, 8813, 8043, 7855, 7383, 6863, 6282, 0},
    {16380, 16379, 15159, 14610, 14609, 14608, 12859, 12111, 11046, 9536, 8348, 7713, 7216, 6533, 5964, 5546, 0},
    {16367, 16333, 16294, 16253, 16222, 16143, 16048, 15947, 15915, 15832, 15731, 15619, 15589, 15512, 15416, 15310, 0},
    {15967, 15319, 14937, 14753, 14010, 12638, 11787, 11360, 10805, 9706, 8934, 8515, 8166, 7456, 6911, 6575, 0},
    {4906, 3005, 2985, 2984, 875, 102, 83, 81, 47, 17, 12, 11, 8, 5, 4, 3, 0},
    {7217, 4346, 4269, 4264, 1924, 428, 340, 332, 280, 203, 179, 175, 171, 164, 159, 157, 0},
    {16010, 15415, 15032, 14805, 14228, 13043, 12168, 11634, 11265, 10419, 9645, 9110, 8892, 8378, 7850, 7437, 0},
    {8573, 5218, 5046, 5032, 2787, 771, 555, 533, 443, 286, 218, 205, 197, 181, 168, 162, 0},
    {11474, 8095, 7822, 7796, 4632, 1443, 1046, 1004, 748, 351, 218, 194, 167, 121, 93, 83, 0},
    {16152, 15764, 15463, 15264, 14925, 14189, 13536, 13070, 12846, 12314, 11763, 11277, 11131, 10777, 10383, 10011, 0},
    {14187, 11654, 11043, 10919, 8498, 4885, 3778, 3552, 2947, 1835, 1283, 1134, 998, 749, 585, 514, 0},
    {14162, 11527, 10759, 10557, 8601, 5417, 4105, 3753, 3286, 2353, 1708, 1473, 1370, 1148, 959, 840, 0},
    {16205, 15902, 15669, 15498, 15213, 14601, 14068, 13674, 13463, 12970, 12471, 12061, 11916, 11564, 11183, 10841, 0},
    {15043, 12972, 12092, 11792, 10265, 7446, 5934, 5379, 4883, 3825, 3036, 2647, 2507, 2185, 1901, 1699, 0},
    {15320, 13694, 12782, 12352, 11191, 8936, 7433, 6671, 6255, 5366, 4622, 4158, 4020, 3712, 3420, 3198, 0},
    {16255, 16020, 15768, 15600, 15416, 14963, 14440, 14006, 13875, 13534, 13137, 12697, 12602, 12364, 12084, 11781, 0},
    {15627, 14503, 13906, 13622, 12557, 10527, 9269, 8661, 8117, 6933, 5994, 5474, 5222, 4664, 4166, 3841, 0},
    {16366, 16365, 14547, 14160, 14159, 14158, 11969, 11473, 8735, 6147, 4911, 4530, 3865, 3180, 2710, 2473, 0},
    {16257, 16038, 15871, 15754, 15536, 15071, 14673, 14390, 14230, 13842, 13452, 13136, 13021, 12745, 12434, 12154, 0},
    {15855, 14971, 14338, 13939, 13239, 11782, 10585, 9805, 9444, 8623, 7846, 7254, 7079, 6673, 6262, 5923, 0},
    {9492, 6318, 6197, 6189, 3004, 652, 489, 477, 333, 143, 96, 90, 78, 60, 50, 47, 0},
    {16313, 16191, 16063, 15968, 15851, 15590, 15303, 15082, 14968, 14704, 14427, 14177, 14095, 13899, 13674, 13457, 0},
    {8485, 5473, 5389, 5383, 2411, 494, 386, 377, 278, 150, 117, 112, 103, 89, 81, 78, 0},
    {10497, 7154, 6959, 6943, 3788, 1004, 734, 709, 517, 238, 152, 138, 120, 90, 72, 66, 0},
    {16317, 16226, 16127, 16040, 15955, 15762, 15547, 15345, 15277, 15111, 14922, 14723, 14671, 14546, 14396, 14239, 0},
    {16382, 16381, 15858, 15540, 15539, 15538, 14704, 14168, 13768, 13092, 12452, 11925, 11683, 11268, 10841, 10460, 0},
    {5974, 3798, 3758, 3755, 1275, 205, 166, 162, 95, 35, 26, 24, 18, 11, 8, 7, 0},
    {3532, 2258, 2246, 2244, 731, 135, 118, 115, 87, 45, 36, 34, 29, 21, 17, 16, 0},
    {7466, 4882, 4821, 4811, 2476, 886, 788, 771, 688, 531, 469, 457, 437, 400, 369, 361, 0},
    {9580, 5772, 5291, 5216, 3444, 1496, 1025, 928, 806, 578, 433, 384, 366, 331, 296, 273, 0},
    {10692, 7730, 7543, 7521, 4679, 1746, 1391, 1346, 1128, 692, 495, 458, 424, 353, 291, 268, 0},
    {11040, 7132, 6549, 6452, 4377, 1875, 1253, 1130, 958, 631, 431, 370, 346, 296, 253, 227, 0},
    {12687, 9332, 8701, 8585, 6266, 3093, 2182, 2004, 1683, 1072, 712, 608, 559, 458, 373, 323, 0},
    {13429, 9853, 8860, 8584, 6806, 4039, 2862, 2478, 2239, 1764, 1409, 1224, 1178, 1077, 979, 903, 0},
    {14685, 12163, 11061, 10668, 9101, 6345, 4871, 4263, 3908, 3200, 2668, 2368, 2285, 2106, 1942, 1819, 0},
    {13295, 11302, 10999, 10945, 7947, 5036, 4490, 4385, 3391, 2185, 1836, 1757, 1424, 998, 833, 785, 0},
    {4992, 2993, 2972, 2970, 1269, 575, 552, 549, 530, 505, 497, 495, 493, 489, 486, 485, 0},
    {15419, 13862, 13104, 12819, 11429, 8753, 7220, 6651, 6020, 4667, 3663, 3220, 2995, 2511, 2107, 1871, 0},
    {12468, 9263, 8912, 8873, 5758, 2193, 1625, 1556, 1187, 589, 371, 330, 283, 200, 149, 131, 0},
    {15870, 15076, 14615, 14369, 13586, 12034, 10990, 10423, 9953, 8908, 8031, 7488, 7233, 6648, 6101, 5712, 0},
    {1693, 978, 976, 975, 194, 18, 16, 15, 11, 7, 6, 5, 4, 3, 2, 1, 0},
    {7992, 5218, 5147, 5143, 2152, 366, 282, 276, 173, 59, 38, 35, 27, 16, 11, 10, 0}};

static const unsigned short arith_cf_r[3][4] = {
    {12571, 10569, 3696, 0},
    {12661, 5700, 3751, 0},
    {10827, 6884, 2929, 0}};

static uint32_t uint32_lShift(uint32_t value, uint32_t shiftBy) {
  if (shiftBy >= 32) {
    value = 0;
  } else {
    value = value << shiftBy;
  }
  return value;
}

static uint32_t uint32_rShift(uint32_t value, uint32_t shiftBy) {
  if (shiftBy >= 32) {
    value = 0;
  } else {
    value = value >> shiftBy;
  }
  return value;
}

static uint32_t uint32_ones(uint32_t nOnes) {
  uint32_t onesMask = uint32_rShift(0xFFFFFFFF, (8 * sizeof(uint32_t) - nOnes));
  return onesMask;
}

static int arith_get_pk(
    ARIENC_PRIVATE_DATA_HANDLE hPrivateData,
    int c);

static ARIENC_PRIVATE_DATA_HANDLE iisArithEncoderGetPrivateDataHandle(
    ARIENC_PUBLIC_DATA_HANDLE const hPublicData);

static ARIENC_PRIVATE_DATA_HANDLE iisArithEncoderGetPrivateDataHandle(
    ARIENC_PUBLIC_DATA_HANDLE const hPublicData) {
  ARIENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  hPrivateData = hPublicData->hPrivateData;
  return hPrivateData;
}

HANDLE_ERROR_INFO iisArithEncoderOpen(
    ARIENC_PUBLIC_DATA_HANDLE *hPublicData) {
  HANDLE_ERROR_INFO retError = noError;
  int size = 0;
  int i;
  ARIENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  size += sizeof(struct arienc_public_data_struct);
  size += sizeof(struct arienc_private_data_struct);

  *hPublicData = (ARIENC_PUBLIC_DATA_HANDLE)iisCalloc(1, sizeof(ARIENC_PUBLIC_DATA));
  if (*hPublicData == NULL) {
    return iisUtil_ERROR(CDI, "Memory allocation in function iisArithEncoderOpen failed");
  }

  hPrivateData = (ARIENC_PRIVATE_DATA_HANDLE)iisCalloc(1, sizeof(ARIENC_PRIVATE_DATA));
  if (hPrivateData == NULL) {
    iisFree(*hPublicData);
    *hPublicData = NULL;
    return iisUtil_ERROR(CDI, "Memory allocation in function iisArithEncoderOpen failed");
  }
  (*hPublicData)->hPrivateData = hPrivateData;
  hPrivateData->size = size;

  hPrivateData->q0 = &hPrivateData->qBuffer[0];
  hPrivateData->q1 = hPrivateData->q0 + 2048;
  hPrivateData->save_q0 = hPrivateData->q1 + 2048;
  hPrivateData->save_q1 = hPrivateData->save_q0 + 2048;
  hPrivateData->reset_q0 = hPrivateData->save_q1 + 2048;
  hPrivateData->reset_q1 = hPrivateData->reset_q0 + 2048;
  hPrivateData->save_aac_enc_q0 = hPrivateData->reset_q1 + 2048;
  hPrivateData->save_aac_enc_q1 = hPrivateData->save_aac_enc_q0 + 2048;
  hPrivateData->previous_N = 2048;

  hPrivateData->context_saved = 0;
  hPrivateData->arith_lookup_m = arith_lookup_m_USAC;
  hPrivateData->arith_lookup_m_length = sizeof(arith_lookup_m_USAC) / sizeof(arith_lookup_m_USAC[0]);
  hPrivateData->arith_hash_m_direct_return = arith_lookup_m_USAC_direct_return;

  hPrivateData->arith_hash_m = arith_hash_m_USAC;
  hPrivateData->arith_hash_m_length = sizeof(arith_hash_m_USAC) / sizeof(arith_hash_m_USAC[0]);

  hPrivateData->arith_cf_m_no_prob_models = 64;
  hPrivateData->arith_cf_m_no_elements = 17;
  for (i = 0; i < hPrivateData->arith_cf_m_no_prob_models; i++) {
    hPrivateData->arith_cf_m[i] = arith_cf_m_USAC[i];
  }
  return retError;
}

ARIENC_PUBLIC_DATA_HANDLE iisArithEncoderClose(
    ARIENC_PUBLIC_DATA_HANDLE const hPublicData) {
  if (hPublicData != NULL) {
    ARIENC_PRIVATE_DATA_HANDLE hPrivateData = iisArithEncoderGetPrivateDataHandle(hPublicData);
    if (hPrivateData != NULL) {
      iisFree(hPrivateData);
    }
    iisFree(hPublicData);
  }
  return NULL;
}

static int arith_map_context(
    int N,
    int arith_reset_flag,
    unsigned int *q0,
    unsigned int *q1,
    int *previous_N) {
  int k, j;
  float ratio;

  if (arith_reset_flag) {
    memset(q0, 0, sizeof(int) * (N >> 2));
  } else {
    ratio = ((float)(*previous_N)) / ((float)N);
    for (j = 0; j < N / 4; j++) {
      k = (int)((float)j * ratio);
      q0[j] = q1[k];
    }
  }

  (*previous_N) = N;

  return (q0[0] << 12);
}

static int arith_get_context(
    int c,
    int i,
    int N,
    unsigned int *q0,
    unsigned int *q1) {
  c = (c & 0xffff) >> 4;
  if (i < N / 4 - 1)
    c = c + (q0[i + 1] << 12);
  c = (c & 0xFFF0);
  if (i > 0)
    c = c + (q1[i - 1]);
  if (i > 3) {
    if ((q1[i - 3] + q1[i - 2] + q1[i - 1]) < 5)
      return (c + 0x10000);
  }
  return (c);
}

static int arith_get_pk(
    ARIENC_PRIVATE_DATA_HANDLE hPrivateData,
    int c) {
  if (c <= (int)0xF) {
    return hPrivateData->arith_hash_m_direct_return[c];
  } else {
    const unsigned int *p = hPrivateData->arith_hash_m;
    unsigned int j;
    unsigned int c12 = ((unsigned int)c << 12) - 1;

    if (c12 > p[370]) {
      p += 371;
    }
    if (c12 > p[185]) {
      p += 186;
    }
    if (c12 > p[92]) {
      p += 93;
    }
    if (c12 > p[46]) {
      p += 47;
    }
    if (c12 > p[23]) {
      p += 24;
    }
    if (c12 > p[11]) {
      p += 12;
    }
    if (c12 > p[5]) {
      p += 6;
    }
    if (c12 > p[2]) {
      p += 3;
    }
    j = p[1];
    if (c12 > j) {
      j = p[2];
      if ((unsigned int)c != (j >> 12)) {
        j = j >> 6;
      }
    } else if ((unsigned int)c != (j >> 12)) {
      if (c12 > p[0]) {
        j = j >> 6;
      } else if ((unsigned int)c != (p[0] >> 12)) {
        j = p[0] >> 6;
      } else {
        j = p[0];
      }
    }
    return (j & 0x3F);
  }
}

static ARIENC_ERROR write_bit(unsigned char *pBitBuffer, int *bitCount, int bit) {
  if (*bitCount < (MAX_BIT_BUFFER_BITS)) {
    if (bit) {
      pBitBuffer[(*bitCount) >> 3] |= 128 >> ((*bitCount) & 7);
    }
    (*bitCount)++;
    return ARIENC_OK;
  }
  return ARIENC_WARNING_BUFFER_FULL;
}

static ARIENC_ERROR write_bits(unsigned char *pBitBuffer, int *bitCount, unsigned int count, unsigned int bits) {
  if (*bitCount + count <= (MAX_BIT_BUFFER_BITS)) {
    if (count) {
      unsigned int bitOffset = *bitCount & 7;
      unsigned char *bitPtr = pBitBuffer + *bitCount / 8;
      unsigned int bitsToWrite = count;

      if (bitOffset) {
        int firstBits = min(bitsToWrite, 8 - bitOffset);

        unsigned char mask = ((1 << firstBits) - 1) << (8 - bitOffset - firstBits);
        unsigned char byte = *bitPtr;
        *bitPtr++ = (byte & ~(mask)) | ((bits << (32 - count)) >> (24 + bitOffset));
        bitsToWrite -= firstBits;
      }

      while (bitsToWrite >= 8) {
        *bitPtr++ = (unsigned char)(bits >> (bitsToWrite - 8));
        bitsToWrite -= 8;
      }

      if (bitsToWrite > 0) {
        unsigned char mask = (unsigned char)(0xff << (8 - bitsToWrite));
        unsigned char byte = *bitPtr;
        *bitPtr = (byte & ~mask) | (unsigned char)(bits << (8 - bitsToWrite));
        bitsToWrite = 0;
      }

      (*bitCount) += count;
    }
    return ARIENC_OK;
  }
  return ARIENC_WARNING_BUFFER_FULL;
}

static ARIENC_ERROR arith_encode(
    ARIENC_PRIVATE_DATA_HANDLE hPrivateData,
    int symbol,
    const unsigned short *cum_freq) {
  int range;
  unsigned int high_low;
  unsigned int bits_to_follow = hPrivateData->bits_to_follow;
  unsigned int arith_bits = hPrivateData->arith_shift_reg;
  unsigned int arith_bit_cnt = hPrivateData->arith_shift_reg_cnt;
  ARIENC_ERROR retError = ARIENC_OK;

  range = hPrivateData->high - hPrivateData->low + 1;

  if (symbol > 0) {
    hPrivateData->high = (hPrivateData->low + ((range * cum_freq[symbol - 1]) >> 14) - 1);
  }
  hPrivateData->low += (range * cum_freq[symbol]) >> 14;

  high_low = (hPrivateData->high << 16) | hPrivateData->low;

  for (;;) {
    if (high_low < 0x80000000) {
      arith_bits <<= 1;
      arith_bit_cnt++;
      if (arith_bit_cnt == 8 * sizeof(arith_bits)) {
        write_bits(hPrivateData->pBitBuffer, &hPrivateData->bitCount, arith_bit_cnt, arith_bits);
        arith_bit_cnt = arith_bits = 0;
      }

      while (bits_to_follow) {
        unsigned int one_bits_to_write = min(bits_to_follow, 8 * sizeof(arith_bits) - arith_bit_cnt);
        arith_bits = uint32_lShift(arith_bits, one_bits_to_write);
        arith_bits |= uint32_ones(one_bits_to_write);
        arith_bit_cnt += one_bits_to_write;
        bits_to_follow -= one_bits_to_write;
        if (arith_bit_cnt == 8 * sizeof(arith_bits)) {
          write_bits(hPrivateData->pBitBuffer, &hPrivateData->bitCount, arith_bit_cnt, arith_bits);
          arith_bit_cnt = arith_bits = 0;
        }
      }
    } else if (high_low & 0x00008000) {
      arith_bits <<= 1;
      arith_bits |= 1;
      arith_bit_cnt++;
      if (arith_bit_cnt == 8 * sizeof(arith_bits)) {
        write_bits(hPrivateData->pBitBuffer, &hPrivateData->bitCount, arith_bit_cnt, arith_bits);
        arith_bit_cnt = arith_bits = 0;
      }

      while (bits_to_follow) {
        unsigned int zero_bits_to_write = min(bits_to_follow, 8 * sizeof(arith_bits) - arith_bit_cnt);
        arith_bits = uint32_lShift(arith_bits, zero_bits_to_write);
        arith_bit_cnt += zero_bits_to_write;
        bits_to_follow -= zero_bits_to_write;
        if (arith_bit_cnt == 8 * sizeof(arith_bits)) {
          write_bits(hPrivateData->pBitBuffer, &hPrivateData->bitCount, arith_bit_cnt, arith_bits);
          arith_bit_cnt = arith_bits = 0;
        }
      }
    } else if ((high_low & 0x40004000) == 0x00004000) {
      bits_to_follow++;
      high_low ^= 0x40004000;
    } else
      break;

    high_low = (high_low << 1) | 0x10000;
  }

  hPrivateData->arith_shift_reg = arith_bits;
  hPrivateData->arith_shift_reg_cnt = arith_bit_cnt;

  hPrivateData->high = high_low >> 16;
  hPrivateData->low = high_low & 0xffff;
  hPrivateData->bits_to_follow = bits_to_follow;

  return retError;
}

static ARIENC_ERROR arith_encode_flush(
    ARIENC_PRIVATE_DATA_HANDLE hPrivateData) {
  unsigned int low = hPrivateData->low;
  unsigned int bits_to_follow = hPrivateData->bits_to_follow;
  ARIENC_ERROR retError = ARIENC_OK;

  bits_to_follow++;
  if (!(low & 0xc000)) {
    hPrivateData->bitCount++;
    while (bits_to_follow > 0 && !retError) {
      uint32_t numOnes = min(bits_to_follow, (8 * sizeof(uint32_t)));
      retError = write_bits(hPrivateData->pBitBuffer, &hPrivateData->bitCount, numOnes, uint32_ones(numOnes));
      bits_to_follow -= numOnes;
    }
  } else {
    if (!retError) retError = write_bit(hPrivateData->pBitBuffer, &hPrivateData->bitCount, 1);

    while (bits_to_follow > 0 && !retError) {
      hPrivateData->bitCount += bits_to_follow;
      bits_to_follow = 0;
    }
  }

  hPrivateData->bits_to_follow = bits_to_follow;

  return retError;
}

#ifndef MACRO_ABS
#define MACRO_ABS(x) (((x) > 0) ? (x) : -(x))
#endif
#ifndef ARITH_ESCAPE
#define ARITH_ESCAPE 16
#endif
#ifndef ARITH_STOP
#define ARITH_STOP 0
#endif

static void reset_bitbuffer(ARIENC_PRIVATE_DATA_HANDLE hPrivateData, int startbitCnt, int stopbitCnt) {
  if (stopbitCnt <= MAX_BIT_BUFFER_BITS) {
    memset(hPrivateData->pBitBuffer + ((startbitCnt + 7) >> 3), 0, ((stopbitCnt - startbitCnt) >> 3) + 1);
  } else if (startbitCnt < MAX_BIT_BUFFER_BITS) {
    memset(hPrivateData->pBitBuffer + ((startbitCnt + 7) >> 3), 0, ((MAX_BIT_BUFFER_BITS - startbitCnt) >> 3) + 1);
  }
  hPrivateData->pBitBuffer[startbitCnt >> 3] &= ~((unsigned char)255 >> (startbitCnt & 7));
}

static void arith_state_save(ARIENC_PRIVATE_DATA_HANDLE hPrivateData, int pki, int c) {
  hPrivateData->save_high = hPrivateData->high;
  hPrivateData->save_low = hPrivateData->low;
  hPrivateData->save_bits_to_follow = hPrivateData->bits_to_follow;

  hPrivateData->save_bitCount = hPrivateData->bitCount;
  hPrivateData->save_pki = pki;
  hPrivateData->save_c = c;
  hPrivateData->save_arith_shift_reg = hPrivateData->arith_shift_reg;
  hPrivateData->save_arith_shift_reg_cnt = hPrivateData->arith_shift_reg_cnt;
}

static void arith_state_resume(ARIENC_PRIVATE_DATA_HANDLE hPrivateData, int *pki, int *c) {
  reset_bitbuffer(hPrivateData, hPrivateData->save_bitCount, hPrivateData->bitCount);
  hPrivateData->high = hPrivateData->save_high;
  hPrivateData->low = hPrivateData->save_low;

  hPrivateData->bits_to_follow = hPrivateData->save_bits_to_follow;
  hPrivateData->bitCount = hPrivateData->save_bitCount;
  *pki = hPrivateData->save_pki;
  *c = hPrivateData->save_c;
  hPrivateData->arith_shift_reg = hPrivateData->save_arith_shift_reg;
  hPrivateData->arith_shift_reg_cnt = hPrivateData->save_arith_shift_reg_cnt;
}

static ARIENC_ERROR arith_encode_spec(
    ARIENC_PRIVATE_DATA_HANDLE hPrivateData,
    int lg,
    int arith_reset_flag,
    int N,
    int *bitCount) {
  int c, i;
  int pki;
  int lev, esc_nb;

  int m;
  unsigned int a1, b1;
  int *a, *b;
  unsigned int *q1;
  int lsbidx;
  int r[32];
  int stop_pos = 0;
  int compression_with_stop = 0;
  int bitCount_with_Stop = 0;
  ARIENC_ERROR retError = ARIENC_OK;

  c = arith_map_context(N, arith_reset_flag, hPrivateData->q0, hPrivateData->q1, &hPrivateData->previous_N);

  for (i = 0; i < lg; i++) {
    if (hPrivateData->x_ac_enc[lg - i - 1] != 0) {
      stop_pos = (lg - i + 1) / 2;
      break;
    }
  }

  a = hPrivateData->x_ac_enc;
  b = hPrivateData->x_ac_enc + 1;
  q1 = hPrivateData->q1;

  hPrivateData->high = 65535;
  hPrivateData->low = 0;
  hPrivateData->bits_to_follow = 0;

  for (i = 0; i < lg / 2; i++) {
    esc_nb = 0;
    lev = 0;

    a1 = (unsigned int)MACRO_ABS(*a);
    b1 = (unsigned int)MACRO_ABS(*b);
    a += 2;
    b += 2;

    c = arith_get_context(c, i, N, hPrivateData->q0, hPrivateData->q1);

    *q1 = a1 + b1 + 1;
    if ((*q1) > 0xF) *q1 = 0xF;
    q1++;

    while (a1 > 3 || b1 > 3) {
      pki = arith_get_pk(hPrivateData, c + (esc_nb << 17));

      if (!retError) retError = arith_encode(hPrivateData, ARITH_ESCAPE, hPrivateData->arith_cf_m[pki]);

      r[lev++] = (a1 & 1) | ((b1 & 1) << 1);

      a1 >>= 1;
      b1 >>= 1;
      esc_nb += 1;

      if (esc_nb > 7)
        esc_nb = 7;
    }

    m = a1 + (b1 << 2);

    pki = arith_get_pk(hPrivateData, c + (esc_nb << 17));

    if (i == stop_pos) {
      compression_with_stop = 1;

      arith_state_save(hPrivateData, pki, c);

      if (!retError) retError = arith_encode(hPrivateData, ARITH_ESCAPE, hPrivateData->arith_cf_m[pki]);

      pki = arith_get_pk(hPrivateData, c + (1 << 17));

      if (!retError) retError = arith_encode(hPrivateData, ARITH_STOP, hPrivateData->arith_cf_m[pki]);

      if (hPrivateData->arith_shift_reg_cnt) {
        write_bits(hPrivateData->pBitBuffer, &hPrivateData->bitCount, hPrivateData->arith_shift_reg_cnt, hPrivateData->arith_shift_reg);
        hPrivateData->arith_shift_reg_cnt = hPrivateData->arith_shift_reg = 0;
      }

      if (!retError) retError = arith_encode_flush(hPrivateData);

      bitCount_with_Stop = hPrivateData->bitCount;

      arith_state_resume(hPrivateData, &pki, &c);
    }
    if (!retError) retError = arith_encode(hPrivateData, m, hPrivateData->arith_cf_m[pki]);

    while (lev--) {
      lsbidx = (a1 == 0) ? 1 : ((b1 == 0) ? 0 : 2);

      if (!retError) retError = arith_encode(hPrivateData, r[lev], arith_cf_r[lsbidx]);

      a1 = (a1 << 1) | (r[lev] & 1);
      b1 = (b1 << 1) | ((r[lev] >> 1) & 1);
    }
  }

  if (hPrivateData->arith_shift_reg_cnt) {
    write_bits(hPrivateData->pBitBuffer, &hPrivateData->bitCount, hPrivateData->arith_shift_reg_cnt, hPrivateData->arith_shift_reg);
    hPrivateData->arith_shift_reg_cnt = hPrivateData->arith_shift_reg = 0;
  }

  if (lg && !retError) retError = arith_encode_flush(hPrivateData);

  for (; i < N / 4; i++) {
    *q1++ = 1;
  }

  if (compression_with_stop) {
    if (hPrivateData->bitCount > bitCount_with_Stop) {
      arith_state_resume(hPrivateData, &pki, &c);

      if (!retError) retError = arith_encode(hPrivateData, ARITH_ESCAPE, hPrivateData->arith_cf_m[pki]);

      pki = arith_get_pk(hPrivateData, c + (1 << 17));

      if (!retError) retError = arith_encode(hPrivateData, ARITH_STOP, hPrivateData->arith_cf_m[pki]);

      if (hPrivateData->arith_shift_reg_cnt) {
        write_bits(hPrivateData->pBitBuffer, &hPrivateData->bitCount, hPrivateData->arith_shift_reg_cnt, hPrivateData->arith_shift_reg);
        hPrivateData->arith_shift_reg_cnt = hPrivateData->arith_shift_reg = 0;
      }

      if (!retError) retError = arith_encode_flush(hPrivateData);
    }
  }

  hPrivateData->arith_shift_reg_cnt = hPrivateData->arith_shift_reg = 0;

  for (i = 0; i < stop_pos * 2; i++) {
    if (hPrivateData->x_ac_enc[i] != 0) {
      hPrivateData->arith_shift_reg <<= 1;

      hPrivateData->arith_shift_reg |= (hPrivateData->x_ac_enc[i] >> 31) + 1;
      hPrivateData->arith_shift_reg_cnt++;
    }

    if (hPrivateData->arith_shift_reg_cnt == 8 * sizeof(unsigned int)) {
      write_bits(hPrivateData->pBitBuffer, &hPrivateData->bitCount, hPrivateData->arith_shift_reg_cnt, hPrivateData->arith_shift_reg);
      hPrivateData->arith_shift_reg_cnt = hPrivateData->arith_shift_reg = 0;
    }
  }

  if (hPrivateData->arith_shift_reg_cnt) {
    write_bits(hPrivateData->pBitBuffer, &hPrivateData->bitCount, hPrivateData->arith_shift_reg_cnt, hPrivateData->arith_shift_reg);
    hPrivateData->arith_shift_reg_cnt = hPrivateData->arith_shift_reg = 0;
  }

  if (bitCount) {
    *bitCount = hPrivateData->bitCount;
  }
  return retError;
}

static void save_encoder_state(ARIENC_PRIVATE_DATA_HANDLE hPrivateData, int N) {
  memcpy(hPrivateData->save_q0, hPrivateData->q0, sizeof(int) * (N >> 2));
  memcpy(hPrivateData->save_q1, hPrivateData->q1, sizeof(int) * (N >> 2));
  hPrivateData->save_previous_N = hPrivateData->previous_N;
}

static void resume_encoder_state(ARIENC_PRIVATE_DATA_HANDLE hPrivateData, int N) {
  reset_bitbuffer(hPrivateData, hPrivateData->bitCountAtStart, hPrivateData->bitCount);
  memcpy(hPrivateData->reset_q0, hPrivateData->q0, sizeof(int) * (N >> 2));
  memcpy(hPrivateData->reset_q1, hPrivateData->q1, sizeof(int) * (N >> 2));
  memcpy(hPrivateData->q0, hPrivateData->save_q0, sizeof(int) * (N >> 2));
  memcpy(hPrivateData->q1, hPrivateData->save_q1, sizeof(int) * (N >> 2));
  hPrivateData->previous_N = hPrivateData->save_previous_N;
}
static void get_reset_encoder_state(ARIENC_PRIVATE_DATA_HANDLE hPrivateData, int N) {
  memcpy(hPrivateData->q0, hPrivateData->reset_q0, sizeof(int) * (N >> 2));
  memcpy(hPrivateData->q1, hPrivateData->reset_q1, sizeof(int) * (N >> 2));
}

unsigned int iisArithEncoderGetData(ARIENC_PUBLIC_DATA_HANDLE hPublicData,
                                    unsigned char const **ppData,
                                    int *pnBits) {
  ARIENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  unsigned int nBits = 0;

  if (hPublicData != NULL) {
    hPrivateData = iisArithEncoderGetPrivateDataHandle(hPublicData);
    nBits = hPrivateData->bitCount;
    hPrivateData->context_saved = 0;

    if (ppData != NULL) {
      *ppData = hPrivateData->internalBitBuffer;
    }

    if (pnBits != NULL) {
      *pnBits = nBits;
    }
  }

  return nBits;
}

ARIENC_ERROR iisArithEncoderEncode(
    ARIENC_PUBLIC_DATA_HANDLE hPublicData,
    unsigned char *pBitBuffer,
    int *bitCount,
    int *p_x_ac_enc,
    int lg,
    int *arith_reset_flag,
    int N

) {
  int reset_0 = 0;
  int reset_1 = 0;
  int bStart = 0;
  int bLength = 0;
  ARIENC_ERROR retError = ARIENC_OK;

  ARIENC_PRIVATE_DATA_HANDLE hPrivateData;

  hPrivateData = iisArithEncoderGetPrivateDataHandle(hPublicData);

  if (pBitBuffer != NULL) {
    memcpy(hPrivateData->internalBitBuffer, pBitBuffer, (*bitCount >> 3) + 1);
  }

  hPrivateData->pBitBuffer = hPrivateData->internalBitBuffer;

  hPrivateData->bitCount = *bitCount;
  hPrivateData->bitCountAtStart = *bitCount;
  hPrivateData->arith_shift_reg_cnt = 0;
  hPrivateData->arith_shift_reg = 0;
  hPrivateData->x_ac_enc = p_x_ac_enc;

  reset_bitbuffer(hPrivateData, hPrivateData->bitCount, MAX_BIT_BUFFER_BYTES * 8);

  hPublicData->info_bitsWritten = 0;

  if (*arith_reset_flag > 0) {
    if (!retError) retError = arith_encode_spec(hPrivateData, lg, 1, N, NULL);

  } else if (*arith_reset_flag == 0) {
    if (!retError) retError = arith_encode_spec(hPrivateData, lg, 0, N, NULL);

  } else if (*arith_reset_flag < 0) {
    save_encoder_state(hPrivateData, N);
    if (!retError) retError = arith_encode_spec(hPrivateData, lg, 1, N, &reset_1);

    bStart = hPrivateData->bitCountAtStart >> 3;
    bLength = (hPrivateData->bitCount >> 3) - bStart + 1;

    memcpy(hPrivateData->saveBitBuffer, hPrivateData->pBitBuffer + bStart, bLength);

    resume_encoder_state(hPrivateData, N);

    hPrivateData->bitCount = hPrivateData->bitCountAtStart;
    hPrivateData->arith_shift_reg = 0;
    hPrivateData->arith_shift_reg_cnt = 0;

    if (!retError) retError = arith_encode_spec(hPrivateData, lg, 0, N, &reset_0);

    if (reset_1 <= reset_0) {
      *arith_reset_flag = 1;
      reset_bitbuffer(hPrivateData, hPrivateData->bitCountAtStart, reset_0);
      get_reset_encoder_state(hPrivateData, N);
      memcpy(hPrivateData->pBitBuffer + bStart, hPrivateData->saveBitBuffer, bLength);
      hPrivateData->bitCount = reset_1;

    } else {
      *arith_reset_flag = 0;
    }
  }

  hPublicData->info_bitsWritten = hPrivateData->bitCount - hPrivateData->bitCountAtStart;
  hPublicData->info_arith_reset_flag = *arith_reset_flag;

  *bitCount = hPrivateData->bitCount;

  if (pBitBuffer != NULL) {
    memcpy(pBitBuffer, hPrivateData->internalBitBuffer, (*bitCount >> 3) + 1);
  }
  if (*bitCount > MAX_BIT_BUFFER_BITS) {
    return ARIENC_WARNING_BUFFER_FULL;
  }
  return retError;
}

void iisArithEncoderSaveContextState(
    ARIENC_PUBLIC_DATA_HANDLE hPublicData) {
  ARIENC_PRIVATE_DATA_HANDLE hPrivateData;
  hPrivateData = iisArithEncoderGetPrivateDataHandle(hPublicData);
  if (!hPrivateData->context_saved) {
    memcpy(hPrivateData->save_aac_enc_q0, hPrivateData->q0, sizeof(int) * (hPrivateData->previous_N >> 2));
    memcpy(hPrivateData->save_aac_enc_q1, hPrivateData->q1, sizeof(int) * (hPrivateData->previous_N >> 2));
    hPrivateData->save_aac_enc_previous_N = hPrivateData->previous_N;
    hPrivateData->context_saved = 1;
  }
}

void iisArithEncoderRestoreContextState(
    ARIENC_PUBLIC_DATA_HANDLE hPublicData) {
  ARIENC_PRIVATE_DATA_HANDLE hPrivateData;

  hPrivateData = iisArithEncoderGetPrivateDataHandle(hPublicData);
  if (hPrivateData->context_saved) {
    hPrivateData->previous_N = hPrivateData->save_aac_enc_previous_N;

    memcpy(hPrivateData->q0, hPrivateData->save_aac_enc_q0, sizeof(int) * (hPrivateData->previous_N >> 2));
    memcpy(hPrivateData->q1, hPrivateData->save_aac_enc_q1, sizeof(int) * (hPrivateData->previous_N >> 2));
  }
}

void iisArithEncoderSetLastContextState(ARIENC_PUBLIC_DATA_HANDLE hPublicData) {
  (void)hPublicData;
  return;
}

void iisArithEncoderSetTables(
    ARIENC_PUBLIC_DATA_HANDLE hPublicData,
    unsigned short *ari_merged_ps,
    int ari_merged_ps_length,
    const unsigned int *ari_merged_hash,
    int ari_merged_hash_length,
    unsigned short *ari_pk[],
    int ari_pk_no_prob_models,
    int ari_pk_no_elements) {
  ARIENC_PRIVATE_DATA_HANDLE hPrivateData;
  int i;

  hPrivateData = iisArithEncoderGetPrivateDataHandle(hPublicData);

  hPrivateData->arith_lookup_m = ari_merged_ps;
  hPrivateData->arith_lookup_m_length = ari_merged_ps_length;

  hPrivateData->arith_hash_m = ari_merged_hash;
  hPrivateData->arith_hash_m_length = ari_merged_hash_length;

  for (i = 0; i < ari_pk_no_prob_models; i++) {
    hPrivateData->arith_cf_m[i] = ari_pk[i];
  }
  hPrivateData->arith_cf_m_no_prob_models = ari_pk_no_prob_models;
  hPrivateData->arith_cf_m_no_elements = ari_pk_no_elements;
}

HANDLE_ERROR_INFO iisArithEncoderCopy(
    ARIENC_PUBLIC_DATA_HANDLE hPublicData_src,
    ARIENC_PUBLIC_DATA_HANDLE *hPublicData_dest) {
  HANDLE_ERROR_INFO retError = noError;
  ARIENC_PRIVATE_DATA_HANDLE hPrivateData_dest = NULL;
  if (hPublicData_src == NULL) {
    return iisUtil_ERROR(CDI, "Invalid Pointer");
  }

  if (*hPublicData_dest == NULL) {
    SAFECALL(retError, iisArithEncoderOpen(hPublicData_dest));
    if (retError != noError) {
      return retError;
    }
  }

  if (*hPublicData_dest != NULL) {
    const ARIENC_PRIVATE_DATA_HANDLE hPrivateData_src = iisArithEncoderGetPrivateDataHandle(hPublicData_src);
    hPrivateData_dest = iisArithEncoderGetPrivateDataHandle(*hPublicData_dest);
    memcpy(*hPublicData_dest, hPublicData_src, sizeof(ARIENC_PUBLIC_DATA));
    memcpy(hPrivateData_dest, hPrivateData_src, sizeof(ARIENC_PRIVATE_DATA));
    (*hPublicData_dest)->hPrivateData = hPrivateData_dest;
  }

  if (*hPublicData_dest != NULL) {
    hPrivateData_dest->q0 = &hPrivateData_dest->qBuffer[0];
    hPrivateData_dest->q1 = hPrivateData_dest->q0 + 2048;
    hPrivateData_dest->save_q0 = hPrivateData_dest->q1 + 2048;
    hPrivateData_dest->save_q1 = hPrivateData_dest->save_q0 + 2048;
    hPrivateData_dest->reset_q0 = hPrivateData_dest->save_q1 + 2048;
    hPrivateData_dest->reset_q1 = hPrivateData_dest->reset_q0 + 2048;
    hPrivateData_dest->save_aac_enc_q0 = hPrivateData_dest->reset_q1 + 2048;
    hPrivateData_dest->save_aac_enc_q1 = hPrivateData_dest->save_aac_enc_q0 + 2048;
  }

  return retError;
}
