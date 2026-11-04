
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

#ifndef MATHLIB_H
#define MATHLIB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  char versionNo[20];
} MathLibInfo;

#define NO_OPTIMIZATION

void MathGetLibInfo(MathLibInfo *libInfo);
void InitMathOpt(void);

void InitMathOpt(void);

float sumFLOAT(const float *X, int n);

void addFLOAT(const float X[], const float Y[], float Z[], int n);
void addFLOATflex(const float X[], int incX, const float Y[], int incY, float Z[], int incZ, int n);
void saddFLOAT(float b, const float *X, float *Z, int n);

void subFLOAT(const float X[], const float Y[], float Z[], int n);
void subFLOATflex(const float X[], int incX, const float Y[], int incY, float Z[], int incZ, int n);

void multFLOAT(const float X[], const float Y[], float Z[], int n);
void multFLOATflex(const float X[], int incX, const float Y[], int incY, float Z[], int incZ, int n);

void divFLOAT(const float X[], const float Y[], float Z[], int n);
void divFLOATflex(const float X[], int incX, const float Y[], int incY, float Z[], int incZ, int n);

void divFLOAT_Approx(const float X[], const float Y[], float Z[], int n);

void copyFLOAT(const float X[], float Z[], int n);
void copyFLOATflex(const float X[], int incX, float Z[], int incZ, int n);
void moveFLOAT(const float X[], float Z[], int n);

void smulFLOAT(float a, const float X[], float Z[], int n);
void smulFLOATflex(float a, const float X[], int incX, float Z[], int incZ, int n);

void setFLOAT(float a, float X[], int n);
void setFLOATflex(float a, float X[], int incX, int n);

float dotFLOAT(const float X[], const float Y[], int n);
float dotFLOATflex(const float X[], int incX, const float Y[], int incY, int n);

float dist2FLOAT(const float X[], const float Y[], int n);
float dist2FLOATflex(const float X[], int incX, const float Y[], int incY, int n);

float norm2FLOAT(const float X[], int n);
void norm2FCOMPLEX(const float *X, float *Z, int n);

void rad2FCOMPLEX(const float X[], float Z[], int n);

void addINT(const int X[], int const Y[], int Z[], int n);
void addINTflex(const int X[], int incX, int const Y[], int incY, int Z[], int incZ, int n);

void subINT(const int X[], int const Y[], int Z[], int n);
void subINTflex(const int X[], int incX, int const Y[], int incY, int Z[], int incZ, int n);

void multINT(const int X[], int const Y[], int Z[], int n);
void multINTflex(const int X[], int incX, int const Y[], int incY, int Z[], int incZ, int n);

void divINT(const int X[], int const Y[], int Z[], int n);
void divINTflex(const int X[], int incX, int const Y[], int incY, int Z[], int incZ, int n);

void smulINT(int a, const int X[], int Z[], int n);
void smulINTflex(int a, const int X[], int incX, int Z[], int incZ, int n);

void smultFLOATip(float a, float *X, int n);

void smultINTip(int a, int *X, int n);

void copyINT(const int X[], int Z[], int n);
void copyINTflex(const int X[], int incX, int Z[], int incZ, int n);
void moveINT(const int X[], int Z[], int n);

void setINT(int a, int X[], int n);
void setINTflex(int a, int X[], int incX, int n);

void copyCHAR(const char X[], char Z[], int n);

void minFLOAT(const float X[], const float Y[], float Z[], int n);
void minFLOATflex(const float X[], int incX, const float Y[], int incY, float Z[], int incZ, int n);
float findminFLOAT(const float *X, int n);

void maxFLOAT(const float X[], const float Y[], float Z[], int n);
void maxFLOATflex(const float X[], int incX, const float Y[], int incY, float Z[], int incZ, int n);
float findmaxFLOAT(const float *X, int n);

void absFLOAT(const float X[], float Z[], int n);
void absFLOATflex(const float X[], int incX, float Z[], int incZ, int n);

void limitFLOAT(float a, float b, const float X[], float Z[], int n);
void limitFLOATflex(float a, float b, const float X[], int incX, float Z[], int incZ, int n);

void signFLOAT(const float X[], float Z[], int n);
void signFLOATflex(const float X[], int incX, float Z[], int incZ, int n);

void minINT(const int X[], const int Y[], int Z[], int n);
void minINTflex(const int X[], int incX, const int Y[], int incY, int Z[], int incZ, int n);

void maxINT(const int X[], const int Y[], int Z[], int n);
void maxINTflex(const int X[], int incX, const int Y[], int incY, int Z[], int incZ, int n);

void absINT(const int X[], int Z[], int n);
void absINTflex(const int X[], int incX, int Z[], int incZ, int n);

void limitINT(int a, int b, const int X[], int Z[], int n);
void limitINTflex(int a, int b, const int X[], int incX, int Z[], int incZ, int n);

void signINT(const int X[], int Z[], int n);
void signINTflex(const int X[], int incX, int Z[], int incZ, int n);

void floorFLOAT(const float X[], float Z[], int n);
void floorFLOATflex(const float X[], int incX, float Z[], int incZ, int n);

void ceilFLOAT(const float X[], float Z[], int n);
void ceilFLOATflex(const float X[], int incX, float Z[], int incZ, int n);
int ceillog2(int a);

void nintFLOAT(const float X[], float Z[], int n);
void nintFLOATflex(const float X[], int incX, float Z[], int incZ, int n);

void truncFLOAT(const float X[], float Z[], int n);
void truncFLOATflex(const float X[], int incX, float Z[], int incZ, int n);

void quantFLOATtoUINT(float a, float b, const float X[], unsigned int Z[], int n);

void roundFLOAT2FLOAT16(const float A[], float B[], int n);
void roundFLOAT2INT(const float A[], int B[], int n);
void roundFLOAT2SHORT(const float A[], signed short B[], int n);

void sinFLOAT(const float X[], float Z[], int n);
void sinFLOATflex(const float X[], int incX, float Z[], int incZ, int n);

void cosFLOAT(const float X[], float Z[], int n);
void cosFLOATflex(const float X[], int incX, float Z[], int incZ, int n);

void expFLOAT(const float X[], float Z[], int n);
void expFLOATflex(const float X[], int incX, float Z[], int incZ, int n);

void spowFLOAT(float a, const float X[], float Z[], int n);
void spowFLOATflex(float a, const float X[], int incX, float Z[], int incZ, int n);

void logFLOAT(const float X[], float Z[], int n);
void logFLOATflex(const float X[], int incX, float Z[], int incZ, int n);

void log2FLOAT(const float X[], float Z[], int n);
void log2FLOATflex(const float X[], int incX, float Z[], int incZ, int n);

void log10FLOAT(const float X[], float Z[], int n);
void log10FLOATflex(const float X[], int incX, float Z[], int incZ, int n);

void alogbFLOAT(float a, float b, const float X[], float Z[], int n);
void alogbFLOATflex(float a, float b, const float X[], int incX, float Z[], int incZ, int n);

void sqrtFLOAT(const float X[], float Z[], int n);
void sqrtFLOATflex(const float X[], int incX, float Z[], int incZ, int n);

float iisml_randomSign(unsigned int *seed);

void shellsortFLOAT(float *in, int n);
void shellsortINT(int *in, int n);
void shellsortFLOAT_Opt(float *in, int n);
void shellsortINT_Opt(int *in, int n);

void enableFPEs(void);

void disableFPEs(void);
#if (defined _MSC_VER && (_MSC_VER < 1800))

double rint(double);
#endif

#if (defined _MSC_VER && (_MSC_VER >= 1400))
#define ML_DEPRECATED(f) __declspec(deprecated) f
#elif defined __GNUC__
#define ML_DEPRECATED(f) f __attribute__((deprecated))
#else
#define ML_DEPRECATED(f) f
#endif

enum { CFFTN_OK = 0,
       CFFTN_ERROR };
ML_DEPRECATED(int FreqToBandWithRounding(float freq, float fs, int numOfBands,
                                         const int *bandStartOffset));

ML_DEPRECATED(int BandToFreqWithRounding(int band, float fs, int numOfLines,
                                         const int *bandOffsetTable));

ML_DEPRECATED(int FreqToBandCut(float freq, float fs, int numOfBands,
                                const int *bandStartOffset));

ML_DEPRECATED(void BuildIdxUp(float *inData, int *idx, int elements));

#define CreateSineTable(x) ((float *)1)
#define DestroySineTable(x)

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#endif

#ifndef ABS
#define ABS(x) ((x) > 0 ? (x) : -(x))
#endif

#if defined(macintosh) && defined(__MRC__)
#include <float.h>
#undef FLT_MIN
#undef FLT_MAX
#define FLT_MIN 1.175494351e-38F
#define FLT_MAX 3.402823466e+38F
#endif

#endif
