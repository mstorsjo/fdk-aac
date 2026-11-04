
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

#ifndef IIS_LEVELER_LIB_H
#define IIS_LEVELER_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  IIS_LEVELER_LIB_RETURN_NO_ERROR = 0,
  IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN,
  IIS_LEVELER_LIB_RETURN_ERROR_MEMORY,
  IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER,
  IIS_LEVELER_LIB_RETURN_ERROR_INVALID_ARGUMENT
} IIS_LEVELER_LIB_RETURN;

typedef enum {
  IIS_LEVELER_LIB_OUTPUT_FORMAT_GAINS_LINEAR = 0,
  IIS_LEVELER_LIB_OUTPUT_FORMAT_GAINS_DB = 1,
  IIS_LEVELER_LIB_OUTPUT_FORMAT_SAMPLES = 2
} IIS_LEVELER_LIB_OUTPUT_FORMAT;

#define IIS_LEVELER_LIB_MAX_CHANNELS (56)
#define IIS_LEVELER_LIB_DEFAULT_RELATIVE_MAX_GAIN (33)
#define IIS_LEVELER_LIB_MIN_RELATIVE_MAX_GAIN (0)
#define IIS_LEVELER_LIB_MAX_RELATIVE_MAX_GAIN (60)
#define IIS_LEVELER_LIB_MIN_TARGET_LOUDNESS (-31)
#define IIS_LEVELER_LIB_MAX_TARGET_LOUDNESS (-10)
#define IIS_LEVELER_LIB_MIN_SAMPLING_RATE (6000)
#define IIS_LEVELER_LIB_MAX_SAMPLING_RATE (192000)

typedef struct IIS_LEVELER_LIB_STRUCT* IIS_LEVELER_LIB_HANDLE;

typedef struct {
  int maxSamplesPerChannel;
  int numberOfChannels;
  int samplingRate;
  int delayMultiple;

  IIS_LEVELER_LIB_OUTPUT_FORMAT outputFormat;
} IIS_LEVELER_LIB_CONFIG;

typedef struct {
  int major;
  int minor;
  int patch;
} IIS_LEVELER_LIB_VERSION_INFO;

typedef enum {
  IIS_LEVELER_LIB_STATE_INITIALIZED,
  IIS_LEVELER_LIB_STATE_ON,
  IIS_LEVELER_LIB_STATE_OFF,
  IIS_LEVELER_LIB_STATE_RAMPING_UP,
  IIS_LEVELER_LIB_STATE_RAMPING_DOWN
} IIS_LEVELER_LIB_STATE;

IIS_LEVELER_LIB_VERSION_INFO
iisLevelerLib_GetVersionInfo(void);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Create(
    IIS_LEVELER_LIB_HANDLE* phLeveler,
    const IIS_LEVELER_LIB_CONFIG* pConfig);

int iisLevelerLib_Get_delayInSamples(
    IIS_LEVELER_LIB_HANDLE hLeveler);

int iisLevelerLib_CalculateDelayInSamples(
    int samplingRate,
    int delayMultiple);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Process(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    int samplesPerChannel,
    const float* audioIn,
    float* out,
    float currentLoudness);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Get_state(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    IIS_LEVELER_LIB_STATE* state);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Get_enabled(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    int* enabled);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_enabled(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    int enabled);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_targetLoudness(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    float targetLoudness);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_relativeMaxGain(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    float relativeMaxGain);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_loudnessComplianceStageEnabled(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    int loudnessComplianceStageEnabled);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_silenceDetectionEnabled(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    int silenceDetectionEnabled);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_adaptiveAttack(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    float adaptiveAttack);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_dynamicRangePreservation(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    float dynamicRangePreservation);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Get_relativeMaxGain(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    float* relativeMaxGain);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Get_loudnessComplianceStageEnabled(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    int* loudnessComplianceStageEnabled);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Get_silenceDetectionEnabled(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    int* silenceDetectionEnabled);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Get_adaptiveAttack(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    float* adaptiveAttack);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Get_dynamicRangePreservation(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    float* dynamicRangePreservation);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Get_targetLoudness(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    float* targetLoudness);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_channelWeights(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    const float channelWeights[IIS_LEVELER_LIB_MAX_CHANNELS]);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_ignoreChannel(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    const int ignoreChannel[IIS_LEVELER_LIB_MAX_CHANNELS]);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Reset(
    IIS_LEVELER_LIB_HANDLE hLeveler);

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Destroy(
    IIS_LEVELER_LIB_HANDLE* phLeveler);

#ifdef __cplusplus
}
#endif

#endif
