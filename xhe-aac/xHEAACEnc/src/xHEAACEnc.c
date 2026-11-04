
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

#define IIS_XHEAACENC_MODULE_NAME "xHEAACEnc"
#include "xHEAACEncVersion.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "iisutillib.h"

#include "iisxHEAACEncLib.h"
#include "xHEAACEncConfig.h"
#include "xHEAACEnc.h"
#include "xHEAACEncWarningHandling.h"

#include "xHEAACEncLoudness.h"

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define LOUDNESS_DEVIATION_TOLERANCE (0.1f)
#define LOUD_MIN_THRESHOLD (-60.0f)
#define IIS_VERSION_STRING IIS_XHEAACENC_VERSION_NUMBER IIS_XHEAACENC_LIBRARY_VERSION

/**********************************************************************/ /**
 Gets the version information for this library.
 \return returns version information string
 **************************************************************************/
char const* IIS_XHEAACAPI IIS_xHEAACEnc_GetVersionInfoStr() {
  return IIS_VERSION_STRING;
}

/**********************************************************************/ /**
 private data needed for this module (unique for every instance of this module)
 **************************************************************************/

/** DOXYGEN [IIS_XHEAACENC_PRIVATE_DATA] **/ /** DOXYGEN [IIS_XHEAACENC_PRIVATE_DATA_HANDLE] **/
typedef struct xheaacenc_instance_handle {
  int nSize;                                      /**< overall size */
  IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback; /**< Message callback function */

  PARAMLIST_INSTANCE_HANDLE hCodecParamList; /**< Codec parameter list */

  XHEAACENCLIB_INSTANCE_HANDLE hxHEAACEncLib; /**< xHE-AAC Encoder Lib */

  ENCCONFIG_INSTANCE_HANDLE hEncoderConfig; /**< encoder config handle */

  IIS_XHEAACENC_WARNING_LIST warningList;

  IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE xHEAACLoudness_internal;

  float* importedInstantaneousLoudness;

  int nLoudnessVerifiedBlocks;

} IIS_XHEAACENC_PRIVATE_DATA, *IIS_XHEAACENC_PRIVATE_DATA_HANDLE;

/** DOXYGEN [IIS_XHEAACENC_PRIVATE_DATA] **/ /** DOXYGEN [IIS_XHEAACENC_PRIVATE_DATA_HANDLE] **/

/**********************************************************************/ /**
 Allocates memory for a new instance
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_New(
    IIS_XHEAACENC_INSTANCE_HANDLE* phInstance, /**< inout: ptr to instance handle */
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback);

/**********************************************************************/ /**
 Update parameter list in IIS_XHEAACENC_INSTANCE_HANDLE
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_UpdateParamList(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc /**< inout: encoder handle */
);

/**********************************************************************/ /**
 retrieves the private data handle
 \return a handle to a private data structure, wich is dedicated to the instance of this module
 **************************************************************************/
static IIS_XHEAACENC_PRIVATE_DATA_HANDLE IIS_xHEAACEnc_GetPrivateDataHandle(
    IIS_XHEAACENC_INSTANCE_HANDLE const hInstance /**< in: instance handle */
);

/**********************************************************************/ /**
 Maps the internal syncframe type to API syncframe type
 **************************************************************************/
static IIS_XHEAACENC_SYNCFRAME_TYPES IIS_xHEAACEnc_MapSyncFrameType(XHEAACENCLIB_SYNCFRAME_TYPES syncframeType);

/**********************************************************************/ /**
 Maps the internal encoder state to API encoder state
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE mapInternalEncoderState(
    XHEAACENCLIB_ENCODER_STATE encoderStateLib, /**< in:  Internal encoder state*/
    IIS_XHEAACENC_ENCODER_STATE* encoderState   /**< out: Mapped encoder state */
);

/**********************************************************************/ /**
 Get Access Unit information
 \return {An error code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other value indicates an error. See the 'Error Handling' section
 for details.}
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_GetAuInfo(
    XHEAACENCLIB_INSTANCE_HANDLE hxHEAACEncLib, /**< in: A valid, pre-configured library handle. */
    IIS_XHEAACENC_AUINFO* pAuInfo,              /**< out: information about the access units */
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback);

/**********************************************************************/ /**
 Checks if the errorValue is an error
 \return returns 1 if the errorValue is an error and 0, if it is no error
 **************************************************************************/
static int isError(
    IIS_XHEAACENC_RETURN_CODE errorValue /**< in: errorCode */
);

/**********************************************************************/ /**
 Checks if the errorValue is an configuration error
 \return returns 1 if the errorValue is an error and 0, if it is no error
 **************************************************************************/
static int isConfigError(
    IIS_XHEAACENC_RETURN_CODE errorValue /**< in: errorCode */
);

/**********************************************************************/ /**
 Maps the error code of the iisxHEAACEnc to the internal error Code
 \return returns the mapped error code of the iisEncoderConfig
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_MappingIISxHEAACEncErrorToInternal(
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback,
    XHEAACENCLIB_RETURN iisxHEAACEncError);

static IIS_XHEAACENC_RETURN_CODE mapInternalxHEAACEncErrorToString(
    XHEAACENCLIB_RETURN iisxHEAACEncError,
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback);

/**********************************************************************/ /**
 Validates parameter config lists (user and already set internal params)
 \return returns error code
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI validateParamLists(
    PARAMLIST_INSTANCE_HANDLE hMappedUserParamList, /**< in: mapped user parameter list */
    PARAMLIST_INSTANCE_HANDLE hCodecParamList,      /**< in: internal parameter list */
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback);

/**********************************************************************/ /**
 Maps the user parameter config list to the internal namespace
 \return IIS_XHEAACENC_RETURN_CODE
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI mapUserParamListToInternal(
    PARAMLIST_INSTANCE_HANDLE hMappedUserParamList, /**< in: mapped user parameter list */
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback);

/**********************************************************************/ /**
 Initializes the paramInfo used by the user parameter config setup
 \return returns error code
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE initParamInfo(
    PARAMLIST_INSTANCE_HANDLE const hMappedUserParamList, /**< in: mapped user parameter list */
    IIS_XHEAACENC_MESSAGE_CALLBACK const messageCallback,
    IIS_XHEAACENC_PARAM_INFO* const paramInfo);

/**********************************************************************/ /**
 Initializes the advanced paramInfo used by the user parameter config setup
 \return returns error code
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE initAdvancedParamInfo(
    PARAMLIST_INSTANCE_HANDLE const hMappedUserParamList, /**< in: mapped user parameter list */
    IIS_XHEAACENC_PARAM_INFO* const paramInfo);

/**********************************************************************/ /**
 Perform loudness processing and pipe data to the xHE-AAC encoder.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_LoudnessProcessing(
    IIS_XHEAACENC_INSTANCE_HANDLE const hxHEAACEnc,    /**< in: encoder handle */
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig /**< in: encoder configuration */
);

/**********************************************************************/ /**
 Opens an internal loudness instance for verifying imported loudness.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_internalLoudnessOpen(
    IIS_XHEAACENC_INSTANCE_HANDLE const hxHEAACEnc,    /**< in: encoder handle */
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig /**< in: encoder configuration */
);

/**********************************************************************/ /**
 Get loudness data handle from param list.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_GetLoudnessDataHandle(
    PARAMLIST_INSTANCE_HANDLE const hCodecParamList,
    IIS_XHEAACENC_LOUDNESS_DATA** const phLoudnessData);

/**********************************************************************/ /**
 Get loudness data handle from param list.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_fillLoudnessSetup(
    PARAMLIST_INSTANCE_HANDLE const hCodecParamList,
    IIS_XHEAACENC_LOUDNESS_SETUP* const phLoudnessSetup,
    int const instantaneousLoudnessLength);

/**********************************************************************/ /**
 Create new instance of LRA Control DRC gain curve params
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_LoudnessProcessing_Open(
    IIS_XHEAACENC_LOUDNESS_DATA* const pLoudnessData,                       /**< in: param list to be updated by encoder lib */
    IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA** const pLraControlDrcGainsData /**< out: loudness data handle */
);

/**********************************************************************/ /**
 Start loudness processing
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_LoudnessProcessing_Process(
    PARAMLIST_INSTANCE_HANDLE const hCodecParamList,                       /**< in: param list to be updated by encoder lib */
    const unsigned char* const vaBuffer,                                   /**< in: buffer with voice activity information. Set to NULL if not available. */
    IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA* const pLraControlDrcGainsData /**< out: loudness data handle */
);

/**********************************************************************/ /**
 Get target loudness range from param list.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_GetTargetLra(
    PARAMLIST_INSTANCE_HANDLE const hCodecParamList,
    float* const drcTargetLra);

/**********************************************************************/ /**
 Delete loudness processing related memory allocation.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_LoudnessProcessing_Delete(
    IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA* pLraControlDrcGainsData /**< in: loudness data handle */
);

/**********************************************************************/ /**
 Transport / pipe DRC related data such as DRC Gains and external node data
 to the encoder instance's internal memory.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_PipeDrcDataToInternal(
    IIS_XHEAACENC_INSTANCE_HANDLE const hxHEAACEnc,                              /**< inout: A valid, pre-configured encoder handle. */
    IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA const* const pLraControlDrcGainsData /**< in: DRC related metadata */
);

/**********************************************************************/ /**
 In case of imported loudness, this function copies the imported loudness data.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE copyImportedLoudnessDataToHandle(
    IIS_XHEAACENC_INSTANCE_HANDLE const hxHEAACEnc, /**< in: encoder handle */
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig    /**< inout: pointer to config handle */
);

/**********************************************************************/ /**
 Validates loudness data in the second pass
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE validateImportedLoudness(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc,
    const float* const pSamples,
    const unsigned int nSamples);

/**********************************************************************/ /**
 returns true if LRA Control is on
 **************************************************************************/
static int isLRAC(
    PARAMLIST_INSTANCE_HANDLE const hCodecParamList);

/**********************************************************************/ /**
 Submits all runtime warnings, that were previously raised in the iisxHEAACEncLib module.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE submitEncodingWarningsLib(
    IIS_XHEAACENC_WARNING_LIST* const pWarningList,
    XHEAACENCLIB_INSTANCE_HANDLE const hxHEAACEncLib);

/**********************************************************************/ /**
 Maps warnings raised by the library to API warning return code
 **************************************************************************/
static IIS_XHEAACENC_WARNING mapLibWarningsToAPIWarning(XHEAACENCLIB_WARNING const libWarning);

/**********************************************************************/ /**
 Maps the dataFormat to the internal lib's param formats. Used in
 IIS_xHEAACEnc_SubmitAdvancedParam and IIS_xHEAACEnc_SubmitAdvancedData.
 **************************************************************************/

static IIS_XHEAACENC_RETURN_CODE mapDataFormat(
    IIS_XHEAACENC_PARAM_FORMAT const dataFormat,
    PARAM_FORMAT* const paramFormat);

/**********************************************************************/ /**
 Set a user defined message callback function.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_SetMessageCallback(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig,        /**< inout: pointer to config handle */
    IIS_XHEAACENC_MESSAGE_CALLBACK const messageCallback /**< in: pointer to message function */
) {
  IIS_XHEAACENC_RETURN_CODE retVal = IIS_XHEAACENC_NO_ERROR;

  if (hConfig) {
    hConfig->messageCallback = messageCallback;
  }

  return retVal;
}

/**********************************************************************/ /**
 Open and initialize one instance of the encoder.
 The encoder configuration is dependent on the parameter list provided.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Open(
    IIS_XHEAACENC_INSTANCE_HANDLE* phxHEAACEnc,        /**< out: pointer to an encoder handle. */
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig /**< in:  configuration */
) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  IIS_XHEAACENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc = NULL;

  if ((hConfig == NULL) || (phxHEAACEnc == NULL)) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  } else if (!hConfig->isFinalized) {
    retValue = IIS_XHEAACENC_ERROR_CONFIG_NOT_FINALIZED;
  }

  if (!isError(retValue)) {
    if ((*phxHEAACEnc) == NULL) {
      retValue = IIS_xHEAACEnc_New(&hxHEAACEnc, hConfig->messageCallback);
    } else {
      hxHEAACEnc = *phxHEAACEnc;
    }
  }

  if (!isError(retValue) && IIS_xHEAACEnc_ParamExists(hConfig->hCodecParamList, PARAMLIST_PARAMETER_INTERNAL_LOUDNESS_VERIFICATION)) {
    retValue = copyImportedLoudnessDataToHandle(hxHEAACEnc, hConfig);

    if (!isError(retValue)) {
      retValue = IIS_xHEAACEnc_internalLoudnessOpen(hxHEAACEnc, hConfig);
    }
  }

  if (!isError(retValue) && isLRAC(hConfig->hCodecParamList)) {
    retValue = IIS_xHEAACEnc_LoudnessProcessing(hxHEAACEnc, hConfig);
  }

  if (!isError(retValue)) {
    hPrivateData = IIS_xHEAACEnc_GetPrivateDataHandle(hxHEAACEnc);
    if (hPrivateData == NULL) {
      retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
    }
  }

  if (!isError(retValue)) {
    if (hConfig->messageCallback) {
      hPrivateData->messageCallback = hConfig->messageCallback;
    }
  }

  if (!isError(retValue)) {
    hPrivateData->hCodecParamList = iisParamListCopy(hConfig->hCodecParamList);
  }

  if (!isError(retValue)) {
    XHEAACENCLIB_RETURN retValueLib = XHEAACENCLIB_RETURN_NO_ERROR;
    retValueLib = IIS_xHEAACEncLib_Update(hPrivateData->hxHEAACEncLib, hPrivateData->hCodecParamList);
    retValue = IIS_xHEAACEnc_MappingIISxHEAACEncErrorToInternal(hPrivateData->messageCallback, retValueLib);
  }

  if (!isError(retValue)) {
    retValue = IIS_xHEAACEnc_UpdateParamList(hxHEAACEnc);
  }

  if (!isError(retValue)) {
    *phxHEAACEnc = hxHEAACEnc;
  } else {
    IIS_xHEAACEnc_Delete(hxHEAACEnc);
    if (phxHEAACEnc != NULL) {
      *phxHEAACEnc = NULL;
    }
    if (hPrivateData != NULL) {
      hPrivateData = NULL;
    }
  }
  return retValue;
}

/**********************************************************************/ /**
 Main encoding function. This function converts one frame of uncompressed
 audio data into an ISO/MPEG Extended HE-AAC bit stream payload.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_EncodeFrame(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc, /**< inout: A valid, pre-configured encoder handle. */
    float const* const pSamples,              /**< in: Pointer to the uncompressed interleaved input audio samples. The samples
                                                       must be of type float, the sample values must be within the range [-1.0 ... 1.0]. */
    const int nSamples,                       /**< in: The number of valid input samples. This value must to be a multiple of number of input channels */
    unsigned char* const pOutput,             /**< inout: A pointer to a user supplied buffer to hold the output. Only complete frames are returned. */
    int* const pOutputBytes,                  /**< out: Upon return, this value will hold the number of valid bytes in the output buffer. */
    const unsigned int outputBufSizeBytes,    /**< in: size of output buffer in bytes */
    IIS_XHEAACENC_AUINFO* const pAuInfo       /**< out: Pointer to structure which contains information about access unit. */
) {
  IIS_XHEAACENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (hxHEAACEnc == NULL || pOutput == NULL || pOutputBytes == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    memset(pAuInfo, 0, sizeof(IIS_XHEAACENC_AUINFO));
  }
  if (!isError(retValue)) {
    hPrivateData = IIS_xHEAACEnc_GetPrivateDataHandle(hxHEAACEnc);

    if (hPrivateData->xHEAACLoudness_internal != NULL) {
      retValue = validateImportedLoudness(hxHEAACEnc, pSamples, nSamples);
    }
  }

  if (!isError(retValue)) {
    XHEAACENCLIB_RETURN retError = XHEAACENCLIB_RETURN_NO_ERROR;
    retError = IIS_xHEAACEncLib_Encode(hPrivateData->hxHEAACEncLib, pSamples, nSamples, pOutput, pOutputBytes, outputBufSizeBytes);
    retValue = IIS_xHEAACEnc_MappingIISxHEAACEncErrorToInternal(hPrivateData->messageCallback, retError);
  }

  if (!isError(retValue)) {
    retValue = IIS_xHEAACEnc_GetAuInfo(hPrivateData->hxHEAACEncLib, pAuInfo, hPrivateData->messageCallback);
  }

  return retValue;
}

/**********************************************************************/ /**
 Submit frame by frame data of the type IIS_XHEAACENC_DATA prior to a call
 of IIS_xHEAACEnc_EncodeFrame(). A return code of IIS_XHEAACENC_NO_ERROR indicates
 correct execution of the function. Any other return code indicates an error or a warning.
 See the 'Error Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_SubmitData(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc,    /**< input: A valid, pre-configured encoder poiter. */
    IIS_XHEAACENC_DATA const type,               /**< in: type of submitted data */
    IIS_XHEAACENC_PARAM_FORMAT const dataFormat, /**< in: parameter format i.e. char, int */
    void const* const pData,                     /**< in: Pointer to buffer from which data will be copied */
    int const dataSize                           /**< in: Size of data to submit in chars */
) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  XHEAACENCLIB_DATA dataType = XHEAACENCLIB_DATA_INVALID;
  PARAM_FORMAT paramFormat = -1;

  if (hxHEAACEnc == NULL || pData == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  } else if (((int)type < IIS_XHEAACENC_DATA_FIRST) || ((int)type > IIS_XHEAACENC_DATA_LAST)) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_PARAM;
  }

  if (!isError(retValue)) {
    switch (type) {
      case IIS_XHEAACENC_DATA_RAP_IN_X_SAMPLES:
        dataType = XHEAACENCLIB_DATA_RAP_IN_X_SAMPLES;
        break;
      case IIS_XHEAACENC_DATA_INVALID:
      default:
        retValue = IIS_XHEAACENC_ERROR_INVALID_PARAM;
        break;
    }
  }

  if (!isError(retValue)) {
    retValue = mapDataFormat(dataFormat, &paramFormat);
  }

  if (!isError(retValue)) {
    XHEAACENCLIB_RETURN retValueLib = XHEAACENCLIB_RETURN_NO_ERROR;

    IIS_XHEAACENC_PRIVATE_DATA_HANDLE hPrivateData = IIS_xHEAACEnc_GetPrivateDataHandle(hxHEAACEnc);

    retValueLib = IIS_xHEAACEncLib_Submit(hPrivateData->hxHEAACEncLib,
                                          dataType, paramFormat,
                                          pData, dataSize);
    retValue = IIS_xHEAACEnc_MappingIISxHEAACEncErrorToInternal(hPrivateData->messageCallback, retValueLib);
  }

  return retValue;
}

/**********************************************************************/ /**
 Validate the configuration. This function can be used to validate the configuration
 without a loudness level, e.g. before the loudness measurement.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function and that the config is valid. A return code of IIS_XHEAACENC_WARNING_INVALID_CONFIG
 indicates that the config in the current state is not valid. Any other return code indicates
 an error. If the config is invalid, the parameter validateErrorInfo will provide more information
 on the reason. See the 'Error Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_Validate(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig, /**< in:  config instance handle */
    IIS_XHEAACENC_RETURN_CODE* const validateErrorInfo  /**< out: error information if config is invalid */
) {
  HANDLE_ERROR_INFO errorInfo = noError;
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  PARAMLIST_INSTANCE_HANDLE hTempCodecParamList = NULL;
  PARAMLIST_INSTANCE_HANDLE hMappedUserParamList = NULL;

  if (hConfig == NULL || validateErrorInfo == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    *validateErrorInfo = IIS_XHEAACENC_NO_ERROR;
  }

  if (!isError(retValue) && !isConfigError(*validateErrorInfo)) {
    if (hTempCodecParamList == NULL) {
      errorInfo = iisParamListNew(&hTempCodecParamList);
      if (errorInfo != noError) {
        IIS_xHEAACEnc_mapInternalParamListErrorToString(PARAMLIST_ERROR_NEW_PARAM, hConfig->messageCallback);
        retValue = IIS_XHEAACENC_ERROR_INTERNAL;
      }
    }
  }

  if (!isError(retValue) && !isConfigError(*validateErrorInfo)) {
    hMappedUserParamList = iisParamListCopy(hConfig->hUserParamList);
  }

  if (!isError(retValue) && !isConfigError(*validateErrorInfo)) {
    *validateErrorInfo = validateParamLists(hMappedUserParamList, hConfig->hCodecParamList, hConfig->messageCallback);
    if (isError(*validateErrorInfo) && !isConfigError(*validateErrorInfo)) {
      retValue = *validateErrorInfo;
    }
  }

  if (!isError(retValue) && !isConfigError(*validateErrorInfo)) {
    retValue = mapUserParamListToInternal(hMappedUserParamList, hConfig->messageCallback);
  }

  if (!isError(retValue) && !isConfigError(*validateErrorInfo)) {
    int loudnessLevelExists = iisParamListParamExists(hMappedUserParamList, PARAMLIST_PARAMETER_LOUDNESS_LEVEL);
    int mpeg4ProgRefLevelExists = iisParamListParamExists(hMappedUserParamList, PARAMLIST_PARAMETER_MPEG4_PROG_REF_LEVEL);
    int anchorLoudessLevelExists = iisParamListParamExists(hMappedUserParamList, PARAMLIST_PARAMETER_ANCHOR_LOUDNESS_LEVEL);
    int liveLoudnessLevelExists = iisParamListParamExists(hMappedUserParamList, PARAMLIST_PARAMETER_LIVE_LOUDNESS_LEVEL);

    if (!loudnessLevelExists && !mpeg4ProgRefLevelExists && !anchorLoudessLevelExists && !liveLoudnessLevelExists) {
      errorInfo = iisParamListAddParamValueInt(hMappedUserParamList, PARAMLIST_PARAMETER_VALIDATE_CONFIGURATION, PARAMLIST_VALIDATE_CONFIGURATION_ON, PARAMLIST_MODE_APPENDIFMISSING);
      if (errorInfo != noError) {
        retValue = IIS_XHEAACENC_ERROR_INTERNAL;
      }
    }
  }

  if (!isError(retValue) && !isConfigError(*validateErrorInfo)) {
    *validateErrorInfo = IIS_xHEAACEnc_Config_refineAndVerifyEncoderConfig(hConfig->messageCallback,
                                                                           hConfig->hUserParamList,
                                                                           hTempCodecParamList,
                                                                           hMappedUserParamList);
  }

  if (!isError(retValue)) {
    if (!isError(*validateErrorInfo)) {
      IIS_xHEAACEnc_Message(hConfig->messageCallback, IIS_XHEAACENC_MESSAGE_LEVEL_CONFIG, "\nUser config is valid!\n");
    } else if (isConfigError(*validateErrorInfo)) {
      IIS_xHEAACEnc_Message(hConfig->messageCallback, IIS_XHEAACENC_MESSAGE_LEVEL_CONFIG, "\nUser config is not valid!\n");
      retValue = IIS_XHEAACENC_WARNING_INVALID_CONFIG;
    } else {
      retValue = *validateErrorInfo;
    }
  }

  if (hTempCodecParamList) {
    iisParamListDelete(hTempCodecParamList);
  }
  if (hMappedUserParamList) {
    iisParamListDelete(hMappedUserParamList);
  }
  if (errorInfo) {
    freeErrorTraceback(errorInfo);
  }

  return retValue;
}

/**********************************************************************/ /**
 Finalize and validate the configuration.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_Finalize(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig /**< in:  config instance handle */
) {
  HANDLE_ERROR_INFO errorInfo = noError;
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  PARAMLIST_INSTANCE_HANDLE hMappedUserParamList = NULL;

  if (hConfig == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = IIS_xHEAACEnc_Config_validateAndAddParamDrcMetadata(hConfig);
  }

  if (!isError(retValue)) {
    hMappedUserParamList = iisParamListCopy(hConfig->hUserParamList);
  }

  if (!isError(retValue)) {
    retValue = validateParamLists(hMappedUserParamList, hConfig->hCodecParamList, hConfig->messageCallback);
  }

  if (!isError(retValue)) {
    retValue = mapUserParamListToInternal(hMappedUserParamList, hConfig->messageCallback);
  }

  if (!isError(retValue)) {
    retValue = IIS_xHEAACEnc_Config_refineAndVerifyEncoderConfig(hConfig->messageCallback,
                                                                 hConfig->hUserParamList,
                                                                 hConfig->hCodecParamList,
                                                                 hMappedUserParamList);
  }

  if (!isError(retValue)) {
    retValue = IIS_xHEAACEnc_Config_validateMeasuredLoudness(hConfig);
  }

  if (!isError(retValue)) {
    hConfig->isFinalized = 1;
  }

  if (!isError(retValue)) {
    PARAMLIST_INSTANCE_HANDLE apiConfigParamList = NULL;

    apiConfigParamList = iisParamListCopy(hConfig->hCodecParamList);

    errorInfo = iisParamListIterate(apiConfigParamList, IIS_xHEAACEnc_MapParamListToAPICB, NULL);

    if (errorInfo == noError && apiConfigParamList) {
      IIS_xHEAACEnc_Message(hConfig->messageCallback, IIS_XHEAACENC_MESSAGE_LEVEL_CONFIG, "\n\nxHEAACEnc Configuration\n");
      errorInfo = iisParamListIterate(apiConfigParamList, IIS_xHEAACEnc_PrintAPIConfigParamsCB, hConfig);
    }

    if (apiConfigParamList) {
      errorInfo = iisParamListDelete(apiConfigParamList);
    }

    if (errorInfo != noError) {
      IIS_xHEAACEnc_mapInternalParamListErrorToString(PARAMLIST_ERROR_DELETE, hConfig->messageCallback);
      retValue = IIS_XHEAACENC_ERROR_INTERNAL;
    }
  }

  if (hMappedUserParamList) {
    iisParamListDelete(hMappedUserParamList);
  }
  if (errorInfo) {
    freeErrorTraceback(errorInfo);
  }

  return retValue;
}

/**********************************************************************/ /**
 Get information about specific parameters from the encoder.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_GetParam(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc,     /**< in: A valid, pre-configured encoder handle. */
    IIS_XHEAACENC_PARAMETER const paramTag,       /**< in: Parameter name/tag */
    IIS_XHEAACENC_PARAM_FORMAT const paramFormat, /**< in: Parameter format i.e. int, float, double, automatic */
    void* const pData,                            /**< out: Pointer to buffer into which data will be copied */
    int const size                                /**< in: Size of data to get in chars */
) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  HANDLE_ERROR_INFO errorInfo = noError;
  char pDataChar[64];
  int staticData = 1;

  if (pData == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  } else if (size < 0) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_VALUE;
  }

  if (!isError(retValue)) {
    switch (paramTag) {
      case IIS_XHEAACENC_PARAMETER_LIB_NAME:
        sprintf(pDataChar, "%s", IIS_XHEAACENC_MODULE_NAME);
        memcpy(pData, pDataChar, min(sizeof(pDataChar), (unsigned int)size));
        break;
      default:
        staticData = 0;
        break;
    }
  }

  if ((hxHEAACEnc != NULL) && (!staticData)) {
    IIS_XHEAACENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;
    PARAM_INSTANCE param = {0};
    PARAM_INSTANCE_HANDLE pParam = NULL;
    PARAM_INSTANCE paramCopy = {0};

    if (!isError(retValue)) {
      hPrivateData = IIS_xHEAACEnc_GetPrivateDataHandle(hxHEAACEnc);
    }

    if (!isError(retValue)) {
      param.paramTag = paramTag;
      IIS_xHEAACEnc_MapParamListToInternCB(NULL, &param, NULL);
    }

    if (!isError(retValue)) {
      errorInfo = iisParamListGetParam(hPrivateData->hCodecParamList, param.paramTag, &pParam);
      if (errorInfo != noError) {
        retValue = IIS_XHEAACENC_ERROR_INVALID_PARAM;
      } else {
        if (pParam) {
          paramCopy.paramFormat = pParam->paramFormat;
          paramCopy.paramLength = pParam->paramLength;
          paramCopy.paramSize = pParam->paramSize;
          paramCopy.paramTag = pParam->paramTag;
          if (pParam->paramFormat == PARAM_CHAR_ARRAY || pParam->paramFormat == PARAM_SHORT_ARRAY || pParam->paramFormat == PARAM_INT_ARRAY || pParam->paramFormat == PARAM_FLOAT_ARRAY || pParam->paramFormat == PARAM_DOUBLE_ARRAY) {
            if (size >= paramCopy.paramSize) {
              if (paramCopy.paramLength > 0) {
                memcpy(pData, pParam->paramValue._pvoid, pParam->paramSize);
                paramCopy.paramValue._pvoid = (void*)pData;
              }
              paramCopy.paramValue._pvoid = (void*)pData;
            } else {
              IIS_xHEAACEnc_Message(hPrivateData->messageCallback, IIS_XHEAACENC_MESSAGE_LEVEL_ERROR, "Given size is too small to store the parameter");
              retValue = IIS_XHEAACENC_ERROR_MEMORY;
            }
          } else {
            paramCopy.paramValue = pParam->paramValue;
          }
        }
      }
    }

    if (!isError(retValue)) {
      IIS_xHEAACEnc_MapParamListToAPICB(NULL, &paramCopy, NULL);
    }

    if (!isError(retValue)) {
      switch (paramFormat) {
        case IIS_XHEAACENC_PARAM_INVALID:
          retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
          break;
        case IIS_XHEAACENC_PARAM_CHAR:
          if (paramCopy.paramFormat != PARAM_CHAR) {
            retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
          }
          break;
        case IIS_XHEAACENC_PARAM_CHAR_ARRAY:
          if (paramCopy.paramFormat != PARAM_CHAR_ARRAY) {
            retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
          }
          break;
        case IIS_XHEAACENC_PARAM_SHORT:
          if (paramCopy.paramFormat != PARAM_SHORT) {
            retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
          }
          break;
        case IIS_XHEAACENC_PARAM_SHORT_ARRAY:
          if (paramCopy.paramFormat != PARAM_SHORT_ARRAY) {
            retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
          }
          break;
        case IIS_XHEAACENC_PARAM_INT:
          if (paramCopy.paramFormat != PARAM_INT) {
            retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
          }
          break;
        case IIS_XHEAACENC_PARAM_INT_ARRAY:
          if (paramCopy.paramFormat != PARAM_INT_ARRAY) {
            retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
          }
          break;
        case IIS_XHEAACENC_PARAM_FLOAT:
          if (paramCopy.paramFormat != PARAM_FLOAT) {
            retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
          }
          break;
        case IIS_XHEAACENC_PARAM_FLOAT_ARRAY:
          if (paramCopy.paramFormat != PARAM_FLOAT_ARRAY) {
            retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
          }
          break;
        case IIS_XHEAACENC_PARAM_DOUBLE:
          if (paramCopy.paramFormat != PARAM_DOUBLE) {
            retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
          }
          break;
        case IIS_XHEAACENC_PARAM_DOUBLE_ARRAY:
          if (paramCopy.paramFormat != PARAM_DOUBLE_ARRAY) {
            retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
          }
          break;
        case IIS_XHEAACENC_PARAM_VOID_POINTER:
          if (paramCopy.paramFormat != PARAM_VOID_POINTER) {
            retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
          }
          break;
        case IIS_XHEAACENC_PARAM_AUTO:
        default:
          break;
      }
    }

    if (!isError(retValue)) {
      if (size >= paramCopy.paramSize) {
      } else {
        IIS_xHEAACEnc_Message(hPrivateData->messageCallback, IIS_XHEAACENC_MESSAGE_LEVEL_ERROR, "Given size is too small to store the parameter");
        retValue = IIS_XHEAACENC_ERROR_MEMORY;
      }
    }

    if ((!isError(retValue)) && (pParam)) {
      if (paramCopy.paramFormat == PARAM_CHAR_ARRAY || paramCopy.paramFormat == PARAM_SHORT_ARRAY || paramCopy.paramFormat == PARAM_INT_ARRAY || paramCopy.paramFormat == PARAM_FLOAT_ARRAY || paramCopy.paramFormat == PARAM_DOUBLE_ARRAY) {
      } else {
        memcpy(pData, &(paramCopy.paramValue._pvoid), paramCopy.paramSize);
      }
    }
  }
  if (errorInfo) {
    freeErrorTraceback(errorInfo);
  }
  return retValue;
}

/**********************************************************************/ /**
 Get Audio specific config information.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_GetAscInfo(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc, /**< in: A valid, pre-configured encoder handle. */
    IIS_XHEAACENC_ASCINFO* const pAscInfo     /**< out: buffer containing ASC information */
) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  XHEAACENCLIB_ASCINFO encLibAscInfo = {0};

  if (hxHEAACEnc == NULL || pAscInfo == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    XHEAACENCLIB_RETURN retError = XHEAACENCLIB_RETURN_NO_ERROR;
    IIS_XHEAACENC_PRIVATE_DATA_HANDLE hPrivateData = IIS_xHEAACEnc_GetPrivateDataHandle(hxHEAACEnc);
    retError = IIS_xHEAACEncLib_GetAscInfo(hPrivateData->hxHEAACEncLib, &encLibAscInfo);
    retValue = IIS_xHEAACEnc_MappingIISxHEAACEncErrorToInternal(hPrivateData->messageCallback, retError);
  }

  if (!isError(retValue)) {
    pAscInfo->ascSizeBits = encLibAscInfo.ascSizeBits;
    memcpy(pAscInfo->ascBuffer, encLibAscInfo.ascBuffer, sizeof(encLibAscInfo.ascBuffer));
  }

  return retValue;
}

/**********************************************************************/ /**
 Delete the encoder instance.
 Call this function to de-initialize and close the encoder instance. All memory
 and other allocated resources will be freed.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Delete(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc /**< inout: pointer to encoder instance handle. */
) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  HANDLE_ERROR_INFO errorInfo = noError;
  IIS_XHEAACENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  ENCODERCONFIG_RETURN_CODE retConfig = ENCODERCONFIG_NO_ERROR;

  if (hxHEAACEnc != NULL) {
    hPrivateData = IIS_xHEAACEnc_GetPrivateDataHandle(hxHEAACEnc);

    if (hPrivateData->hCodecParamList != NULL) {
      errorInfo = iisParamListDelete(hPrivateData->hCodecParamList);
    }

    if (hPrivateData->hEncoderConfig != NULL) {
      retConfig = iisEncoderConfigDelete(hPrivateData->hEncoderConfig);
      if (!isError(retValue)) {
        retValue = IIS_xHEAACEnc_Config_mappingConfigErrorToInternal(hPrivateData->messageCallback, retConfig);
      }
    }
    if (hPrivateData->xHEAACLoudness_internal != NULL && !isError(retValue)) {
      retValue = IIS_xHEAACEnc_Loudness_Delete(hPrivateData->xHEAACLoudness_internal);
    }

    if (hPrivateData->importedInstantaneousLoudness != NULL && !isError(retValue)) {
      free(hPrivateData->importedInstantaneousLoudness);
      hPrivateData->importedInstantaneousLoudness = NULL;
    }

    IIS_xHEAACEncLib_Delete(&hPrivateData->hxHEAACEncLib);

    iisFree(hxHEAACEnc);

  } else {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }
  if (errorInfo) {
    freeErrorTraceback(errorInfo);
  }
  return retValue;
}

/**********************************************************************/ /**
 initialization of an instance of this module, pass a ptr to a hInstance
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_New(
    IIS_XHEAACENC_INSTANCE_HANDLE* phInstance, /**< inout: ptr to instance handle */
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  ENCODERCONFIG_RETURN_CODE retConfig = ENCODERCONFIG_NO_ERROR;
  HANDLE_ERROR_INFO errorInfo = noError;

  int nSize = 0;
  IIS_XHEAACENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if (!phInstance) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    nSize += sizeof(struct xheaacenc_instance_handle);

    *phInstance = (IIS_XHEAACENC_INSTANCE_HANDLE)iisMalloc(nSize);
  }

  if (*phInstance != NULL) {
    if (!isError(retValue)) {
      memset(*phInstance, 0, nSize);
      hPrivateData = (IIS_XHEAACENC_PRIVATE_DATA_HANDLE)((char*)(*phInstance));
      hPrivateData->nSize = nSize;
    }

    if (!isError(retValue)) {
      if (messageCallback) {
        hPrivateData->messageCallback = messageCallback;
      }
    }

    if (!isError(retValue)) {
      retConfig = iisEncoderConfigNew(&hPrivateData->hEncoderConfig);
      retValue = IIS_xHEAACEnc_Config_mappingConfigErrorToInternal(hPrivateData->messageCallback, retConfig);
    }

    if (!isError(retValue)) {
      XHEAACENCLIB_RETURN retIISxHEAACEnc;
      retIISxHEAACEnc = IIS_xHEAACEncLib_New(&hPrivateData->hxHEAACEncLib);
      retValue = IIS_xHEAACEnc_MappingIISxHEAACEncErrorToInternal(hPrivateData->messageCallback, retIISxHEAACEnc);
    }

  } else {
    retValue = IIS_XHEAACENC_ERROR_MEMORY;
  }

  if (errorInfo) {
    freeErrorTraceback(errorInfo);
  }

  return retValue;
}

/**********************************************************************/ /**
 Update parameter list in IIS_XHEAACENC_INSTANCE_HANDLE
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_UpdateParamList(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc /**< inout: encoder handle */
) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  IIS_XHEAACENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if (hxHEAACEnc == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    XHEAACENCLIB_RETURN retError;
    hPrivateData = IIS_xHEAACEnc_GetPrivateDataHandle(hxHEAACEnc);
    retError = IIS_xHEAACEncLib_UpdateParamList(hPrivateData->hxHEAACEncLib);
    retValue = IIS_xHEAACEnc_MappingIISxHEAACEncErrorToInternal(hPrivateData->messageCallback, retError);
  }
  return retValue;
}

/**********************************************************************/ /**
 retrieves the private data handle
 \return a handle to a private data structure, which is dedicated to the instance of this module
 **************************************************************************/
static IIS_XHEAACENC_PRIVATE_DATA_HANDLE IIS_xHEAACEnc_GetPrivateDataHandle(
    IIS_XHEAACENC_INSTANCE_HANDLE const hInstance /**< in: instance handle */
) {
  IIS_XHEAACENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  hPrivateData = (IIS_XHEAACENC_PRIVATE_DATA_HANDLE)((char*)hInstance);
  return hPrivateData;
}

/**********************************************************************/ /**
 Maps the internal syncframe type to API syncframe type
 **************************************************************************/
static IIS_XHEAACENC_SYNCFRAME_TYPES IIS_xHEAACEnc_MapSyncFrameType(XHEAACENCLIB_SYNCFRAME_TYPES syncframeType) {
  IIS_XHEAACENC_SYNCFRAME_TYPES retVal = IIS_XHEAACENC_SYNCFRAME_INVALID;
  switch (syncframeType) {
    case XHEAACENCLIB_SYNCFRAME_NO:
      retVal = IIS_XHEAACENC_SYNCFRAME_NO;
      break;
    case XHEAACENCLIB_SYNCFRAME_INDEPENDENT_FRAME:
      retVal = IIS_XHEAACENC_SYNCFRAME_INDEPENDENT_FRAME;
      break;
    case XHEAACENCLIB_SYNCFRAME_IMMEDIATE_PLAY_OUT_FRAME:
      retVal = IIS_XHEAACENC_SYNCFRAME_IMMEDIATE_PLAY_OUT_FRAME;
      break;
    case XHEAACENCLIB_SYNCFRAME_STREAM_MUX_CONFIG_IF:
      retVal = IIS_XHEAACENC_SYNCFRAME_STREAM_MUX_CONFIG_IF;
      break;
    case XHEAACENCLIB_SYNCFRAME_STREAM_MUX_CONFIG_IPF:
      retVal = IIS_XHEAACENC_SYNCFRAME_STREAM_MUX_CONFIG_IPF;
      break;
    case XHEAACENCLIB_SYNCFRAME_HE_AAC_RAP_SWITCHABLE:
      retVal = IIS_XHEAACENC_SYNCFRAME_HE_AAC_RAP_SWITCHABLE;
      break;
    case XHEAACENC_SYNCFRAME_STREAM_MUX_CONFIG_HE_AAC_SWITCHABLE:
      retVal = IIS_XHEAACENC_SYNCFRAME_STREAM_MUX_CONFIG_HE_AAC_SWITCHABLE;
      break;
    case XHEAACENC_SYNCFRAME_STREAM_MUX_CONFIG_HE_AAC_ACCESS:
      retVal = IIS_XHEAACENC_SYNCFRAME_STREAM_MUX_CONFIG_HE_AAC_ACCESS;
      break;
    case XHEAACENCLIB_SYNCFRAME_INVALID:
    default:
      retVal = IIS_XHEAACENC_SYNCFRAME_INVALID;
      break;
  }
  return retVal;
}

/**********************************************************************/ /**
 Get Access Unit information
 \return {An error code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other value indicates an error. See the 'Error Handling' section
 for details.}
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_GetAuInfo(
    XHEAACENCLIB_INSTANCE_HANDLE hxHEAACEncLib, /**< in: A valid, pre-configured encoder handle. */
    IIS_XHEAACENC_AUINFO* pAuInfo,              /**< out: information about the access units */
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  XHEAACENCLIB_RETURN retError = XHEAACENCLIB_RETURN_NO_ERROR;
  XHEAACENCLIB_AUINFOLIST_HANDLE hxHEAACEncLibAuInfoList = NULL;

  if (hxHEAACEncLib == NULL || pAuInfo == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  memset(pAuInfo, 0, sizeof(IIS_XHEAACENC_AUINFO));

  if (!isError(retValue)) {
    retError = IIS_xHEAACEncLib_GetAuInfo(hxHEAACEncLib, &hxHEAACEncLibAuInfoList);
    retValue = IIS_xHEAACEnc_MappingIISxHEAACEncErrorToInternal(messageCallback, retError);
  }

  if (!isError(retValue)) {
    XHEAACENCLIB_AUINFO_HANDLE hxHEAACEncLibAuInfo = hxHEAACEncLibAuInfoList->hAccessUnits;

    pAuInfo->auSize = hxHEAACEncLibAuInfo->auSize;
    pAuInfo->auSamplesValid = hxHEAACEncLibAuInfo->auSamplesValid;
    pAuInfo->auOffset = hxHEAACEncLibAuInfo->auOffset;
    pAuInfo->isSyncFrame = IIS_xHEAACEnc_MapSyncFrameType(hxHEAACEncLibAuInfo->isSyncFrame);
  }

  return retValue;
}

/**********************************************************************/ /**
 Get Encoder State.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_GetEncoderState(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc,       /**<  in: Encoder handle */
    IIS_XHEAACENC_ENCODER_STATE* const encoderState /**< out: Encoder state*/
) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  XHEAACENCLIB_RETURN retError = XHEAACENCLIB_RETURN_NO_ERROR;

  if (!hxHEAACEnc || !encoderState) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    XHEAACENCLIB_ENCODER_STATE encoderStateInternal;
    retError = IIS_xHEAACEncLib_GetEncoderState(hxHEAACEnc->hxHEAACEncLib, &encoderStateInternal);
    retValue = IIS_xHEAACEnc_MappingIISxHEAACEncErrorToInternal(hxHEAACEnc->messageCallback, retError);

    if (!isError(retValue)) {
      retValue = mapInternalEncoderState(encoderStateInternal, encoderState);
    }
  }
  return retValue;
}

/**********************************************************************/ /**
 Initializes the paramInfo used by the user parameter config setup
 \return returns error code
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE initParamInfo(
    PARAMLIST_INSTANCE_HANDLE const hMappedUserParamList, /**< in: mapped user parameter list */
    IIS_XHEAACENC_MESSAGE_CALLBACK const messageCallback,
    IIS_XHEAACENC_PARAM_INFO* const paramInfo) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if ((hMappedUserParamList == NULL) || (paramInfo == NULL)) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    paramInfo->developerActive = 0;

    retValue = initAdvancedParamInfo(hMappedUserParamList, paramInfo);

    paramInfo->returnError = IIS_XHEAACENC_NO_ERROR;
    paramInfo->messageCallback = messageCallback;
    paramInfo->listIgnoreParam = NULL;
    paramInfo->numberIgnoreParam = 0;
  }
  return retValue;
}

/**********************************************************************/ /**
 Initializes the advanced paramInfo used by the user parameter config setup
 \return returns error code
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE initAdvancedParamInfo(
    PARAMLIST_INSTANCE_HANDLE const hMappedUserParamList, /**< in: mapped user parameter list */
    IIS_XHEAACENC_PARAM_INFO* const paramInfo) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (!isError(retValue)) {
    if ((hMappedUserParamList == NULL) || (paramInfo == NULL)) {
      retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
    }
  }

  if (!isError(retValue)) {
    paramInfo->advancedLiveEncoding = 0;
    paramInfo->advancedDrc = 0;
    paramInfo->advancedLoudness1 = 0;
    paramInfo->advancedLoudness2 = 0;
    paramInfo->advancedLegacyAac = 0;
    paramInfo->advancedRapControl = 0;
    paramInfo->advancedRateControl = 0;
    paramInfo->advancedContBitrate = 0;
    paramInfo->advancedBackwardsCompatibility = 0;
    paramInfo->advancedIfControl = 0;
  }
  return retValue;
}

/**********************************************************************/ /**
 Validates parameter config lists (user and already set internal params)
 \return returns error code
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI validateParamLists(
    PARAMLIST_INSTANCE_HANDLE hMappedUserParamList, /**< in: mapped user parameter list */
    PARAMLIST_INSTANCE_HANDLE hCodecParamList,      /**< in: internal parameter list */
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  HANDLE_ERROR_INFO errorInfo = noError;
  IIS_XHEAACENC_PARAM_INFO paramInfo;

  if (hMappedUserParamList == NULL || hCodecParamList == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = initParamInfo(hMappedUserParamList, messageCallback, &paramInfo);
  }

  if (!isError(retValue)) {
    errorInfo = iisParamListIterate(hMappedUserParamList, IIS_xHEAACEnc_Config_FinalizeCB, &paramInfo);
    if (errorInfo != noError) {
      retValue = IIS_XHEAACENC_ERROR_CONFIG_NOT_FINALIZED;
    } else if (paramInfo.returnError != IIS_XHEAACENC_NO_ERROR) {
      retValue = paramInfo.returnError;
    }
  }

  if (!isError(retValue)) {
    errorInfo = iisParamListIterate(hCodecParamList, IIS_xHEAACEnc_Config_FinalizeCB, &paramInfo);
    if (errorInfo != noError) {
      retValue = IIS_XHEAACENC_ERROR_CONFIG_NOT_FINALIZED;
    } else if (paramInfo.returnError != IIS_XHEAACENC_NO_ERROR) {
      retValue = paramInfo.returnError;
    }
  }

  if (!isError(retValue)) {
    errorInfo = iisParamListIterate(hMappedUserParamList, IIS_xHEAACEnc_RemoveNotAllowedParamsCB, &paramInfo);
    if (errorInfo != noError) {
      IIS_xHEAACEnc_mapInternalParamListErrorToString(PARAMLIST_ERROR_DELETE, messageCallback);
      retValue = IIS_XHEAACENC_ERROR_INTERNAL;
    } else if (paramInfo.returnError != IIS_XHEAACENC_NO_ERROR) {
      retValue = paramInfo.returnError;
    }
  }

  return retValue;
}
/**********************************************************************/ /**
 Maps the user parameter config list to the internal namespace
 \return IIS_XHEAACENC_RETURN_CODE
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI mapUserParamListToInternal(
    PARAMLIST_INSTANCE_HANDLE hMappedUserParamList, /**< in: mapped user parameter list */
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  HANDLE_ERROR_INFO errorInfo = noError;

  if (hMappedUserParamList == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    errorInfo = iisParamListIterate(hMappedUserParamList, IIS_xHEAACEnc_MapParamListToInternCB, NULL);
    if (errorInfo != noError) {
      IIS_xHEAACEnc_mapInternalParamListErrorToString(PARAMLIST_ERROR_ITERATE, messageCallback);
      retValue = IIS_XHEAACENC_ERROR_INTERNAL;
    }
  }

  return retValue;
}

/**********************************************************************/ /**
 In case of imported loudness, this function copies the imported loudness data.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE copyImportedLoudnessDataToHandle(
    IIS_XHEAACENC_INSTANCE_HANDLE const hxHEAACEnc, /**< in: encoder handle */
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig    /**< inout: pointer to config handle */
) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  IIS_XHEAACENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  IIS_XHEAACENC_LOUDNESS_DATA* pLoudnessData = NULL;

  unsigned int instantaneousLoudnessLength = 0;

  if (!isError(retValue)) {
    hPrivateData = IIS_xHEAACEnc_GetPrivateDataHandle(hxHEAACEnc);
    retValue = IIS_xHEAACEnc_GetLoudnessDataHandle(hConfig->hCodecParamList, &pLoudnessData);
  }

  if (!isError(retValue)) {
    instantaneousLoudnessLength = pLoudnessData->instantaneousLoudnessLength;

    hPrivateData->importedInstantaneousLoudness = (float*)calloc(instantaneousLoudnessLength, sizeof(float));

    memcpy(hPrivateData->importedInstantaneousLoudness, pLoudnessData->instantaneousLoudness, instantaneousLoudnessLength * sizeof(float));
  }

  return retValue;
}
/**********************************************************************/ /**
 Maps the internal encoder state to API encoder state
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE mapInternalEncoderState(
    XHEAACENCLIB_ENCODER_STATE encoderStateLib, /**< in:  Internal encoder state*/
    IIS_XHEAACENC_ENCODER_STATE* encoderState   /**< out: Mapped encoder state */
) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (!encoderState) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    switch (encoderStateLib) {
      case XHEAACENCLIB_ENCODER_STATE_STARTUP:
        *encoderState = IIS_XHEAACENC_ENCODER_STATE_STARTUP;
        break;
      case XHEAACENCLIB_ENCODER_STATE_ENCODING:
        *encoderState = IIS_XHEAACENC_ENCODER_STATE_ENCODING;
        break;
      case XHEAACENCLIB_ENCODER_STATE_WAITING:
        *encoderState = IIS_XHEAACENC_ENCODER_STATE_WAITING;
        break;
      case XHEAACENCLIB_ENCODER_STATE_FLUSHING:
        *encoderState = IIS_XHEAACENC_ENCODER_STATE_FLUSHING;
        break;
      case XHEAACENCLIB_ENCODER_STATE_READY_TO_CLOSE:
        *encoderState = IIS_XHEAACENC_ENCODER_STATE_READY_TO_CLOSE;
        break;
      default:
        retValue = IIS_XHEAACENC_ERROR_INVALID_VALUE;
        break;
    }
  }

  return retValue;
}

/**********************************************************************/ /**
 Checks if the errorValue is an error
 \return returns 1 if the errorValue is an error and 0, if it is no error
 **************************************************************************/
static int isError(
    IIS_XHEAACENC_RETURN_CODE errorValue) {
  int retValue = 0;
  if (errorValue >= IIS_XHEAACENC_ERROR_FIRST) {
    retValue = 1;
  }
  return retValue;
}

/**********************************************************************/ /**
 Checks if the errorValue is an configuration error
 \return returns 1 if the errorValue is an error and 0, if it is no error
 **************************************************************************/
static int isConfigError(
    IIS_XHEAACENC_RETURN_CODE errorValue) {
  int retValue = 0;
  if (errorValue >= IIS_XHEAACENC_ERROR_PARAM_FIRST) {
    retValue = 1;
  }
  return retValue;
}

/**********************************************************************/ /**
 Maps the error code of the iisxHEAACEncLib to the internal error Code
 \return returns the mapped error code of the iisEncoderConfig
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_MappingIISxHEAACEncErrorToInternal(
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback,
    XHEAACENCLIB_RETURN iisxHEAACEncError) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  retValue = mapInternalxHEAACEncErrorToString(iisxHEAACEncError, messageCallback);
  if (retValue != IIS_XHEAACENC_ERROR_INTERNAL) {
    switch (iisxHEAACEncError) {
      case XHEAACENCLIB_RETURN_NO_ERROR:
        retValue = IIS_XHEAACENC_NO_ERROR;
        break;
      case XHEAACENCLIB_RETURN_WARNING_RUNTIME:
        retValue = IIS_XHEAACENC_WARNING_ENCODING;
        break;
      case XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE:
        retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
        break;
      case XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION:
      case XHEAACENCLIB_RETURN_ERROR_SYNC_FRAME_MEMORY:
      case XHEAACENCLIB_RETURN_ERROR_SBR_MEMORY_ALLOCATION:
      case XHEAACENCLIB_RETURN_ERROR_CALLOC_FAIL:
        retValue = IIS_XHEAACENC_ERROR_MEMORY_ALLOCATION;
        break;
      case XHEAACENCLIB_RETURN_ERROR_CONFIGURATION:
        retValue = IIS_XHEAACENC_ERROR_INVALID_CONFIG;
        break;
      case XHEAACENCLIB_RETURN_ERROR_PARAM_LIST:
        retValue = IIS_XHEAACENC_ERROR_INTERNAL;
        break;
      case XHEAACENCLIB_RETURN_ERROR_BUFFER_SIZE:
        retValue = IIS_XHEAACENC_ERROR_BUFFER_SIZE;
        break;
      case XHEAACENCLIB_RETURN_ERROR_RAP_TOO_CLOSE:
        retValue = IIS_XHEAACENC_WARNING_RAP_TOO_CLOSE;
        break;
      case XHEAACENCLIB_RETURN_ERROR_RAP_TOO_SOON:
        retValue = IIS_XHEAACENC_WARNING_RAP_TOO_SOON;
        break;
      case XHEAACENCLIB_RETURN_ERROR_INVALID_RAP_POSITION:
        retValue = IIS_XHEAACENC_ERROR_INVALID_RAP_POSITION;
        break;
      case XHEAACENCLIB_RETURN_ERROR_WRONG_RAP_OCCURRENCE:
        retValue = IIS_XHEAACENC_ERROR_WRONG_RAP_OCCURRENCE;
        break;
      case XHEAACENCLIB_RETURN_ERROR_SYNC_FRAME:
        retValue = IIS_XHEAACENC_ERROR_SYNC_FRAME;
        break;

      case XHEAACENCLIB_RETURN_ERROR_BD_CONFIG:
      case XHEAACENCLIB_RETURN_ERROR_BD_WRONG_CALLING_SEQUENCE:
      case XHEAACENCLIB_RETURN_ERROR_BD_WRONG_ELEMENTID:
      case XHEAACENCLIB_RETURN_ERROR_BD_WRONG_PARAMETER:
      case XHEAACENCLIB_RETURN_ERROR_BD_WRONG_IMPLEMENTATION:
        retValue = IIS_XHEAACENC_ERROR_BITDISTRIBUTION;
        break;
      case XHEAACENCLIB_RETURN_ERROR_TOO_MANY_INPUT_SAMPLES:
        retValue = IIS_XHEAACENC_ERROR_TOO_MANY_INPUT_SAMPLES;
        break;
      case XHEAACENCLIB_RETURN_ERROR_UNEXPECTED_INPUT_SAMPLES:
        retValue = IIS_XHEAACENC_ERROR_UNEXPECTED_INPUT_SAMPLES_FLUSHING;
        break;
      case XHEAACENCLIB_RETURN_ERROR_SETTING_DEFAULT_LOUDNESS:
        retValue = IIS_XHEAACENC_ERROR_PARAM_NO_LOUDNESS_PROVIDED;
        break;
      case XHEAACENCLIB_RETURN_ERROR_WRONG_MAX_SIZE_OF_BD:
      case XHEAACENCLIB_RETURN_ERROR_UNKNOWN:
      default:
        retValue = IIS_XHEAACENC_ERROR_UNKNOWN;
        break;
    }
  }
  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE mapInternalxHEAACEncErrorToString(
    XHEAACENCLIB_RETURN iisxHEAACEncError,
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_ERROR_INTERNAL;
  char* pErrorString = NULL;

  switch (iisxHEAACEncError) {
    case XHEAACENCLIB_RETURN_ERROR_RAP_PROPERTY:
      pErrorString = "Internal error :  L1001";
      break;
    case XHEAACENCLIB_RETURN_ERROR_WRONG_MAX_SIZE_OF_BD:
      pErrorString = "Internal error :  L1002";
      break;
    case XHEAACENCLIB_RETURN_ERROR_LRACONTROL_DRC_GAIN:
    case XHEAACENCLIB_RETURN_ERROR_DRC:
    case XHEAACENCLIB_RETURN_ERROR_DRC_TOO_MANY_NODES:
    case XHEAACENCLIB_RETURN_ERROR_DRC_TOO_MANY_SEQUENCES:
    case XHEAACENCLIB_RETURN_ERROR_DRC_INVALID_INSTRUCTIONS:
    case XHEAACENCLIB_RETURN_ERROR_DRC_UNSUPPORTED_CHARACTERISTICS:
    case XHEAACENCLIB_RETURN_ERROR_DRC_IFC_INVALID_SETUP:
      pErrorString = "Internal error :  L1003";
      break;
    case XHEAACENCLIB_RETURN_ERROR_AAC_ENC_UPDATE:
    case XHEAACENCLIB_RETURN_ERROR_AAC_HEADER_BITS:
      pErrorString = "Internal error :  L1004";
      break;
    case XHEAACENCLIB_RETURN_ERROR_LIVE_LOUDNESS_INIT_FAIL:
    case XHEAACENCLIB_RETURN_ERROR_LIVE_LOUDNESS_PROCESS:
      pErrorString = "Internal error :  L1005";
      break;
    case XHEAACENCLIB_RETURN_ERROR_LOUDNESS_MEASUREMENT:
      pErrorString = "Internal error :  L1006";
      break;
    case XHEAACENCLIB_RETURN_ERROR_BUFFER_SWITCHING_DECISION_TOO_SMALL:
    case XHEAACENCLIB_RETURN_ERROR_IIS_SWITCHING_DECISION:
    case XHEAACENCLIB_RETURN_ERROR_IIS_SWITCHING_DECISION_FEED:
      pErrorString = "Internal error :  L1007";
      break;
    case XHEAACENCLIB_RETURN_ERROR_ENC_NOT_FLUSHING_TOO_LESS_SAMPLES:
      pErrorString = "Internal error :  L1008";
      break;
    case XHEAACENCLIB_RETURN_ERROR_nSAMPLES_NOT_MULTIPLE_nIP_CHANNELS:
      pErrorString = "Internal error :  L1009";
      break;
    case XHEAACENCLIB_RETURN_ERROR_DELAY_AND_BUFFER:
      pErrorString = "Internal error :  L1010";
      break;
    case XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER:
    case XHEAACENCLIB_RETURN_ERROR_MP4_SAVE_TIMEBUFFER:
      pErrorString = "Internal error :  L1011";
      break;
    case XHEAACENCLIB_RETURN_ERROR_MP4_SPACE_ENC:
      pErrorString = "Internal error :  L1012";
      break;
    case XHEAACENCLIB_RETURN_ERROR_MP4_ASC:
      pErrorString = "Internal error :  L1013";
      break;
    case XHEAACENCLIB_RETURN_ERROR_INVALID_PARAMETER:
      pErrorString = "Internal error :  L1014";
      break;
    case XHEAACENCLIB_RETURN_ERROR_INVALID_FRAME_SIZE:
      pErrorString = "Internal error :  L1015";
      break;
    case XHEAACENCLIB_RETURN_ERROR_INVALID_CICP_INDEX:
      pErrorString = "Internal error :  L1016";
      break;
    case XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL:
    case XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_NO_DATA:
    case XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_CONFIG:
    case XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_INSTANCE:
    case XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_INSTANCE_WR:
    case XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_INVALID:
    case XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_DELAY:
    case XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_CROSSOVER_FLAG_CHANGE:
      pErrorString = "Internal error :  L1017";
      break;
    case XHEAACENCLIB_RETURN_ERROR_IPF_NONINDEPENDENT_FRAME:
    case XHEAACENCLIB_RETURN_ERROR_IPF_INVALID:
      pErrorString = "Internal error :  L1018";
      break;
    case XHEAACENCLIB_RETURN_ERROR_INVALID_BYTE_ALIGNMENT:
      pErrorString = "Internal error :  L1019";
      break;
    case XHEAACENCLIB_RETURN_ERROR_AOT_CONFIGURATION:
      pErrorString = "Internal error :  L1020";
      break;
    case XHEAACENCLIB_RETURN_ERROR_TOO_MANY_EXTENSION_CONFIGS:
    case XHEAACENCLIB_RETURN_ERROR_EXTENSION_CONFIG_EXISTS:
      pErrorString = "Internal error :  L1021";
      break;
    case XHEAACENCLIB_RETURN_ERROR_SPREADING:
      pErrorString = "Internal error :  L1022";
      break;
    case XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD:
    case XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD_CONTAINER:
    case XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD_AUDIO_PREROLL:
      pErrorString = "Internal error :  L1023";
      break;
    case XHEAACENCLIB_RETURN_ERROR_NO_EXT_ELEMENT:
    case XHEAACENCLIB_RETURN_ERROR_INVALID_EXT_ELEMENT:
      pErrorString = "Internal error :  L1024";
      break;
    case XHEAACENCLIB_RETURN_ERROR_INVALID_RAP_INTERVAL:
      pErrorString = "Internal error :  L1025";
      break;
    case XHEAACENCLIB_RETURN_ERROR_SYNC_INVALID_PARAMETER:
    case XHEAACENCLIB_RETURN_ERROR_SYNC_STARTUP_LEN_CHNG:
    case XHEAACENCLIB_RETURN_ERROR_SYNC_START_FRAME_LEN_INVALID:
    case XHEAACENCLIB_RETURN_ERROR_SYNC_MORE_INTERVALS_SPECIFIED:
    case XHEAACENCLIB_RETURN_ERROR_SYNC_EARLIER_SYNC_DELAY:
    case XHEAACENCLIB_RETURN_ERROR_SYNC_NO_FORCED_FRAMES:
      pErrorString = "Internal error :  L1026";
      break;
    case XHEAACENCLIB_RETURN_ERROR_SBR:
    case XHEAACENCLIB_RETURN_ERROR_SBR_INIT_DELAY_COMP:
    case XHEAACENCLIB_RETURN_ERROR_SBR_ENC_OPEN:
      pErrorString = "Internal error :  L1027";
      break;
    case XHEAACENCLIB_RETURN_ERROR_BIT_DISTRIBUTION:
      pErrorString = "Internal error :  L1028";
      break;
    case XHEAACENCLIB_RETURN_ERROR_BIT_RESERVOIR:
      pErrorString = "Internal error :  L1029";
      break;
    case XHEAACENCLIB_RETURN_ERROR_MPEGS_INIT_FAIL:
      pErrorString = "Internal error :  L1030";
      break;
    case XHEAACENCLIB_RETURN_ERROR_SAP_SYNC:
      pErrorString = "Internal error :  L1031";
      break;
    case XHEAACENCLIB_RETURN_ERROR_CORE_RESAMPLER:
      pErrorString = "Internal error :  L1032";
      break;
    case XHEAACENCLIB_RETURN_ERROR_SIGMAP:
      pErrorString = "Internal error :  L1033";
      break;
    case XHEAACENCLIB_RETURN_ERROR_LOAS_WRAPPER:
      pErrorString = "Internal error :  L1034";
      break;
    default:
      retValue = IIS_XHEAACENC_NO_ERROR;
      break;
  }
  if (isError(retValue)) {
    IIS_xHEAACEnc_Message(messageCallback, IIS_XHEAACENC_MESSAGE_LEVEL_ERROR, pErrorString);
  }

  return retValue;
}

/**********************************************************************/ /**
 Perform loudness processing and pipe data to the xHE-AAC encoder.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_LoudnessProcessing(
    IIS_XHEAACENC_INSTANCE_HANDLE const hxHEAACEnc,    /**< in: encoder handle */
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig /**< in: encoder configuration */
) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  IIS_XHEAACENC_LOUDNESS_DATA* pLoudnessData = NULL;

  if (NULL == hxHEAACEnc || NULL == hConfig) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = IIS_xHEAACEnc_GetLoudnessDataHandle(hConfig->hCodecParamList, &pLoudnessData);
  }

  if (!isError(retValue)) {
    if (pLoudnessData) {
      IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA* pLraControlDrcGainsData = NULL;

      retValue = IIS_xHEAACEnc_LoudnessProcessing_Open(pLoudnessData, &pLraControlDrcGainsData);

      if (!isError(retValue)) {
        retValue = IIS_xHEAACEnc_LoudnessProcessing_Process(hConfig->hCodecParamList, pLoudnessData->vaBuffer, pLraControlDrcGainsData);
      }

      if (!isError(retValue)) {
        retValue = IIS_xHEAACEnc_PipeDrcDataToInternal(hxHEAACEnc, pLraControlDrcGainsData);
      }

      if (!isError(retValue)) {
        retValue = IIS_xHEAACEnc_LoudnessProcessing_Delete(pLraControlDrcGainsData);
      }
    }
  }

  return retValue;
}

/**********************************************************************/ /**
 Opens an internal loudness instance for verifying imported loudness.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_internalLoudnessOpen(
    IIS_XHEAACENC_INSTANCE_HANDLE const hxHEAACEnc,    /**< in: encoder handle */
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig /**< in: encoder configuration */
) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  IIS_XHEAACENC_LOUDNESS_DATA* pLoudnessData = NULL;
  IIS_XHEAACENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  IIS_XHEAACENC_LOUDNESS_SETUP xHEAACLoudnessSetup;

  memset(&xHEAACLoudnessSetup, 0, sizeof(IIS_XHEAACENC_LOUDNESS_SETUP));

  if (NULL == hxHEAACEnc || NULL == hConfig) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    hPrivateData = IIS_xHEAACEnc_GetPrivateDataHandle(hxHEAACEnc);
    retValue = IIS_xHEAACEnc_GetLoudnessDataHandle(hConfig->hCodecParamList, &pLoudnessData);
  }

  if (!isError(retValue)) {
    retValue = IIS_xHEAACEnc_fillLoudnessSetup(hConfig->hCodecParamList, &xHEAACLoudnessSetup, pLoudnessData->instantaneousLoudnessLength);
  }

  if (!isError(retValue)) {
    retValue = IIS_xHEAACEnc_Loudness_Open(&(hPrivateData->xHEAACLoudness_internal),
                                           xHEAACLoudnessSetup);
  }

  return retValue;
}

/**********************************************************************/ /**
 Get loudness data handle from param list.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_GetLoudnessDataHandle(
    PARAMLIST_INSTANCE_HANDLE const hCodecParamList,
    IIS_XHEAACENC_LOUDNESS_DATA** const phLoudnessData) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (hCodecParamList == NULL || phLoudnessData == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (IIS_xHEAACEnc_ParamExists(hCodecParamList, PARAMLIST_PARAMETER_LOUDNESS_DATA)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    PARAM_INSTANCE_HANDLE hParam_loudnessInstance = NULL;

    errorInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_LOUDNESS_DATA, &hParam_loudnessInstance);
    if (errorInfo != noError || !hParam_loudnessInstance) {
      retValue = IIS_XHEAACENC_ERROR_PARAM_LOUDNESS_DATA;
    } else {
      *phLoudnessData = (IIS_XHEAACENC_LOUDNESS_DATA*)hParam_loudnessInstance->paramValue._pvoid;
    }
  }

  return retValue;
}

/**********************************************************************/ /**
 Get loudness data handle from param list.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_fillLoudnessSetup(
    PARAMLIST_INSTANCE_HANDLE const hCodecParamList,
    IIS_XHEAACENC_LOUDNESS_SETUP* const phLoudnessSetup,
    int const instantaneousLoudnessLength) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  unsigned int audioBlockLength = 0;

  if (hCodecParamList == NULL || phLoudnessSetup == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (IIS_xHEAACEnc_ParamExists(hCodecParamList, PARAMLIST_PARAMETER_INSAMPLERATE)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    PARAM_INSTANCE_HANDLE hParam_sampleRate = NULL;

    errorInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_INSAMPLERATE, &hParam_sampleRate);
    if (errorInfo != noError) {
      retValue = IIS_XHEAACENC_ERROR_PARAM_INSAMPLERATE;
    } else {
      (*phLoudnessSetup).sampleRate = (unsigned int)hParam_sampleRate->paramValue._int;
    }
  }

  if (IIS_xHEAACEnc_ParamExists(hCodecParamList, PARAMLIST_PARAMETER_CHANNELCONFIG)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    PARAM_INSTANCE_HANDLE hParam_channelConfig = NULL;

    errorInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_CHANNELCONFIG, &hParam_channelConfig);
    if (errorInfo != noError) {
      retValue = IIS_XHEAACENC_ERROR_PARAM_INSAMPLERATE;
    } else {
      PARAMLIST_CHANNELCONFIG channelConfig = hParam_channelConfig->paramValue._int;
      switch (channelConfig) {
        case PARAMLIST_CHANNELCONFIG_MONO:
          (*phLoudnessSetup).channelConfig = IIS_XHEAACENC_CHANNELCONFIG_MONO;
          break;
        case PARAMLIST_CHANNELCONFIG_STEREO:
          (*phLoudnessSetup).channelConfig = IIS_XHEAACENC_CHANNELCONFIG_STEREO;
          break;
        default:
          assert(0);
          retValue = IIS_XHEAACENC_ERROR_PARAM_CHANNELCONFIG;
      }
    }
  }

  if (!isError(retValue)) {
    if ((*phLoudnessSetup).sampleRate < 88200) {
      audioBlockLength = ((*phLoudnessSetup).sampleRate * XHEAACENC_LOUDNESS_ENV_BLOCK_LENGTH_MS) / 1000;
    } else {
      if ((*phLoudnessSetup).sampleRate == 96000) {
        audioBlockLength = 4800;
      }
      if ((*phLoudnessSetup).sampleRate == 88200) {
        audioBlockLength = 4410;
      }
    }
    (*phLoudnessSetup).audioInputLengthSamples = (uint64_t)instantaneousLoudnessLength * audioBlockLength;
    (*phLoudnessSetup).audioInputLengthAvailable = 1;
  }

  return retValue;
}
/**********************************************************************/ /**
 Get target loudness range from param list.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_GetTargetLra(
    PARAMLIST_INSTANCE_HANDLE const hCodecParamList,
    float* const drcTargetLra) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (hCodecParamList == NULL || drcTargetLra == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (IIS_xHEAACEnc_ParamExists(hCodecParamList, PARAMLIST_PARAMETER_MPEGD_DRC_TARGET_LOUDNESS_RANGE)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    PARAM_INSTANCE_HANDLE hParam_drcTagetLra = NULL;

    errorInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEGD_DRC_TARGET_LOUDNESS_RANGE, &hParam_drcTagetLra);
    if (errorInfo != noError || !hParam_drcTagetLra) {
      retValue = IIS_XHEAACENC_ERROR_PARAM_TARGET_LOUDNESS_RANGE;
    } else {
      *drcTargetLra = hParam_drcTagetLra->paramValue._float;
    }
  }

  return retValue;
}

/**********************************************************************/ /**
 Create new instance of LRA Control DRC gain curve params
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_LoudnessProcessing_Open(
    IIS_XHEAACENC_LOUDNESS_DATA* const pLoudnessData,                       /**< in: param list to be updated by encoder lib */
    IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA** const pLraControlDrcGainsData /**< out: loudness data handle */
) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (pLoudnessData == NULL || pLraControlDrcGainsData == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = IIS_xHEAACEnc_Loudness_LraControlDrcGainDataCreate(pLoudnessData, pLraControlDrcGainsData);
  }
  return retValue;
}

/**********************************************************************/ /**
 Start loudness processing
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_LoudnessProcessing_Process(
    PARAMLIST_INSTANCE_HANDLE const hCodecParamList,                       /**< in: param list to be updated by encoder lib */
    const unsigned char* const vaBuffer,                                   /**< in: buffer with voice activity information. Set to NULL if not available. */
    IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA* const pLraControlDrcGainsData /**< inout: loudness data handle */
) {
  HANDLE_ERROR_INFO errorInfo = noError;
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  PARAMLIST_DRCMODE drcMode = PARAMLIST_DRCMODE_INVALID;

  if (!hCodecParamList || !pLraControlDrcGainsData) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    PARAM_INSTANCE_HANDLE hParam_drcMode = NULL;

    errorInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_DRC_MODE, &hParam_drcMode);

    if ((errorInfo == noError) && (hParam_drcMode != NULL) && (hParam_drcMode->paramFormat == PARAM_INT)) {
      drcMode = hParam_drcMode->paramValue._int;
    } else {
      retValue = IIS_XHEAACENC_ERROR_PARAM_DRCMODE;
    }
  }

  if (!isError(retValue)) {
    float loudnessLevel = XHEAACENC_INVALID_LOUDNESS_LEVEL;
    float targetLRA = XHEAACENC_INVALID_LOUDNESS_LEVEL;
    IIS_XHEAACENC_LOUDNESS_LEVEL_TYPE loudnessLevelType = IIS_XHEAACENC_LOUDNESS_LEVEL_TYPE_INVALID;

    if (IIS_xHEAACEnc_ParamExists(hCodecParamList, PARAMLIST_PARAMETER_ANCHOR_LOUDNESS_LEVEL) || IIS_xHEAACEnc_ParamExists(hCodecParamList, PARAMLIST_PARAMETER_INTERNAL_MEASURED_ANCHOR_LOUDNESS)) {
      PARAM_INSTANCE_HANDLE hParam_anchor_loudness_measured = NULL;
      PARAM_INSTANCE_HANDLE hParam_anchor_loudness_provided = NULL;

      errorInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_ANCHOR_LOUDNESS_LEVEL, &hParam_anchor_loudness_provided);
      if (errorInfo == noError) {
        errorInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_INTERNAL_MEASURED_ANCHOR_LOUDNESS, &hParam_anchor_loudness_measured);
      }
      if (errorInfo != noError || (!hParam_anchor_loudness_measured && !hParam_anchor_loudness_provided)) {
        retValue = IIS_XHEAACENC_ERROR_PARAM_ANCHOR_LOUDNESS_LEVEL;
      } else {
        if (hParam_anchor_loudness_provided) {
          loudnessLevel = hParam_anchor_loudness_provided->paramValue._float;
          loudnessLevelType = IIS_XHEAACENC_LOUDNESS_LEVEL_TYPE_ANCHOR_EXTERNAL_SET;
        } else {
          loudnessLevel = hParam_anchor_loudness_measured->paramValue._float;
          loudnessLevelType = IIS_XHEAACENC_LOUDNESS_LEVEL_TYPE_ANCHOR_INTERNAL_MEASURED;
        }
      }

    } else {
      PARAM_INSTANCE_HANDLE hParam_loudness_provided = NULL;
      PARAM_INSTANCE_HANDLE hParam_loudness_measured = NULL;

      errorInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_LOUDNESS_LEVEL, &hParam_loudness_provided);
      if (errorInfo == noError) {
        errorInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_INTERNAL_MEASURED_LOUDNESS, &hParam_loudness_measured);
      }
      if (errorInfo != noError || (!hParam_loudness_measured && !hParam_loudness_provided)) {
        retValue = IIS_XHEAACENC_ERROR_PARAM_LOUDNESS_LEVEL;
      } else {
        loudnessLevelType = IIS_XHEAACENC_LOUDNESS_LEVEL_TYPE_PRL;
        if (hParam_loudness_provided) {
          loudnessLevel = hParam_loudness_provided->paramValue._float;
        } else {
          loudnessLevel = hParam_loudness_measured->paramValue._float;
        }
      }
    }

    if (!isError(retValue)) {
      retValue = IIS_xHEAACEnc_GetTargetLra(hCodecParamList, &targetLRA);
    }
    if (!isError(retValue)) {
      retValue = IIS_xHEAACEnc_Loudness_ConvertToDrcGains(pLraControlDrcGainsData, loudnessLevel, loudnessLevelType, vaBuffer, targetLRA, drcMode);
    }
  }
  return retValue;
}

/**********************************************************************/ /**
 Transport / pipe DRC related data such as DRC Gains and external node data
 to the encoder instance's internal memory.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_PipeDrcDataToInternal(
    IIS_XHEAACENC_INSTANCE_HANDLE const hxHEAACEnc,                              /**< inout: A valid, pre-configured encoder handle. */
    IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA const* const pLraControlDrcGainsData /**< in: DRC related metadata */
) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  XHEAACENCLIB_RETURN errorLibError = XHEAACENCLIB_RETURN_NO_ERROR;
  IIS_XHEAACENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if (NULL == hxHEAACEnc || NULL == pLraControlDrcGainsData) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    hPrivateData = IIS_xHEAACEnc_GetPrivateDataHandle(hxHEAACEnc);
    if (NULL == hPrivateData) {
      retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
    }
  }

  if (!isError(retValue)) {
    errorLibError = IIS_xHEAACEncLib_SubmitDrcMetadata(hPrivateData->hxHEAACEncLib,
                                                       pLraControlDrcGainsData->lraControlDrcGains,
                                                       pLraControlDrcGainsData->lraControlDrcGainsLength,
                                                       pLraControlDrcGainsData->drcExternalNodeNumNodes,
                                                       pLraControlDrcGainsData->drcExternalNodeLevels,
                                                       pLraControlDrcGainsData->drcExternalNodeGains,
                                                       pLraControlDrcGainsData->drcGainOffset,
                                                       XHEAACENC_MAX_DRC_SEQUENCES);

    retValue = IIS_xHEAACEnc_MappingIISxHEAACEncErrorToInternal(hPrivateData->messageCallback, errorLibError);
  }

  return retValue;
}

/**********************************************************************/ /**
 Delete loudness processing related memory instance.
 \return A return code of IIS_XHEAACENC_NO_ERROR indicates correct execution of the
 function. Any other return code indicates an error or a warning. See the 'Error
 Handling' section for details.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_LoudnessProcessing_Delete(
    IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA* pLraControlDrcGainsData /**< in: loudness data handle */
) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  retValue = IIS_xHEAACEnc_Loudness_LraControlDrcGainDataDelete(pLraControlDrcGainsData);
  return retValue;
}

/**********************************************************************/ /**
 validates the imported loudness data
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE validateImportedLoudness(IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc,
                                                          const float* const pSamples,
                                                          const unsigned int nSamples) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  IIS_XHEAACENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  int n = 0;

  hPrivateData = IIS_xHEAACEnc_GetPrivateDataHandle(hxHEAACEnc);
  n = hPrivateData->nLoudnessVerifiedBlocks;

  if (pSamples != NULL) {
    retValue = IIS_xHEAACEnc_Loudness_Measure(hPrivateData->xHEAACLoudness_internal,
                                              pSamples,
                                              nSamples);
  }

  if (nSamples == 0 &&

      (hPrivateData->xHEAACLoudness_internal->hLoudnessData->instantaneousLoudnessLength - hPrivateData->nLoudnessVerifiedBlocks) > 1) {
    retValue = submitEncodingWarning(&hxHEAACEnc->warningList, IIS_XHEAACENC_WARN_MORE_SAMPLES_EXPECTED);
  }

  if (hPrivateData->nLoudnessVerifiedBlocks <= hPrivateData->xHEAACLoudness_internal->hLoudnessData->instantaneousLoudnessLength) {
    while (n < hPrivateData->xHEAACLoudness_internal->hLoudnessData->audioBlocksProcessed) {
      if (fabs(hPrivateData->xHEAACLoudness_internal->hLoudnessData->instantaneousLoudness[n] -
               hPrivateData->importedInstantaneousLoudness[n]) > LOUDNESS_DEVIATION_TOLERANCE) {
        if (hPrivateData->xHEAACLoudness_internal->hLoudnessData->instantaneousLoudness[n] > LOUD_MIN_THRESHOLD &&
            hPrivateData->importedInstantaneousLoudness[n] > LOUD_MIN_THRESHOLD) {
          retValue = IIS_XHEAACENC_ERROR_LOUDNESS_IMPORT_MISMATCH;
        }
      }

      hPrivateData->nLoudnessVerifiedBlocks++;
      n++;
    }
  } else {
    retValue = IIS_XHEAACENC_ERROR_LOUDNESS_IMPORT_MISMATCH;
  }
  return retValue;
}

/**********************************************************************/ /**
 returns true if LRA Control is on
 **************************************************************************/
static int isLRAC(PARAMLIST_INSTANCE_HANDLE const hCodecParamList) {
  int retVal = 0;
  if (IIS_xHEAACEnc_ParamExists(hCodecParamList, PARAMLIST_PARAMETER_DRC_MODE)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    PARAM_INSTANCE_HANDLE hParam_channelConfig = NULL;

    errorInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_DRC_MODE, &hParam_channelConfig);
    if (errorInfo != noError) {
      retVal = 0;
    } else {
      PARAMLIST_DRCMODE drcMode = hParam_channelConfig->paramValue._int;
      switch (drcMode) {
        case PARAMLIST_DRCMODE_OFF:
          retVal = 0;
          break;
        case PARAMLIST_DRCMODE_LN_NE_LI_GE_LRACONTROL:
          retVal = 1;
          break;
        default:
          retVal = 0;
          assert(0);
          break;
      }
    }
  }
  return retVal;
}

/**********************************************************************/ /**
 Collects all the warnings that have been raised by the instance since the
 last call to this function
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_GetEncodingWarnings(
    IIS_XHEAACENC_INSTANCE_HANDLE hxHEAACEnc, /**< inout: ptr to instance handle */
    int* const pnWarning,
    IIS_XHEAACENC_WARNING const** const ppWarnings) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  IIS_XHEAACENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if (hxHEAACEnc == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  } else if (pnWarning == NULL || ppWarnings == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_PARAM;
  }

  if (retValue == IIS_XHEAACENC_NO_ERROR) {
    hPrivateData = IIS_xHEAACEnc_GetPrivateDataHandle(hxHEAACEnc);
    if (hPrivateData == NULL) {
      retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
    }
  }

  if (retValue == IIS_XHEAACENC_NO_ERROR) {
    retValue = submitEncodingWarningsLib(&hPrivateData->warningList, hPrivateData->hxHEAACEncLib);
  }

  if (retValue < IIS_XHEAACENC_ERROR_FIRST) {
    retValue = getWarnings(&hPrivateData->warningList, pnWarning, ppWarnings);
  }

  return retValue;
}

/**********************************************************************/ /**
 Collects all the warnings that have been raised during configuration since the
 last call to this function
 **************************************************************************/
IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_GetConfigWarnings(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig, /**< inout: ptr to instance handle */
    int* const pnWarning,
    IIS_XHEAACENC_WARNING const** const ppWarnings) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (hConfig == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  } else if (pnWarning == NULL || ppWarnings == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_PARAM;
  }

  if (retValue == IIS_XHEAACENC_NO_ERROR) {
    retValue = getWarnings(&hConfig->warningList, pnWarning, ppWarnings);
  }

  return retValue;
}

/**********************************************************************/ /**
 Submits all runtime warnings, that were previously raised in the iisxHEAACEncLib module.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE submitEncodingWarningsLib(
    IIS_XHEAACENC_WARNING_LIST* const pWarningList,
    XHEAACENCLIB_INSTANCE_HANDLE const hxHEAACEncLib) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  XHEAACENCLIB_WARNING warningInit = XHEAACENCLIB_WARN_INVALID;
  const XHEAACENCLIB_WARNING* warningsLib = &warningInit;
  int nWarningsLib = 0;

  if (!pWarningList || !hxHEAACEncLib) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (retValue == IIS_XHEAACENC_NO_ERROR) {
    XHEAACENCLIB_RETURN retError = IIS_xHEAACEncLib_GetEncodingWarnings(hxHEAACEncLib, &nWarningsLib, &warningsLib);
    if (retError != XHEAACENCLIB_RETURN_NO_ERROR) {
      retValue = IIS_XHEAACENC_ERROR_UNKNOWN;
    }
  }

  if (retValue == IIS_XHEAACENC_NO_ERROR) {
    if (nWarningsLib > 0) {
      int i = 0;
      for (i = 0; i < nWarningsLib && retValue == IIS_XHEAACENC_NO_ERROR; i++) {
        retValue = submitEncodingWarning(pWarningList, mapLibWarningsToAPIWarning(warningsLib[i]));
      }

      if (retValue == IIS_XHEAACENC_NO_ERROR) {
        retValue = IIS_XHEAACENC_WARNING_ENCODING;
      }
    }
  }

  return retValue;
}

/**********************************************************************/ /**
 Maps warnings raised by the library to API warning return code
 **************************************************************************/
static IIS_XHEAACENC_WARNING mapLibWarningsToAPIWarning(XHEAACENCLIB_WARNING const libWarning) {
  IIS_XHEAACENC_WARNING retWarning = IIS_XHEAACENC_WARNING_INVALID;

  switch (libWarning) {
    case XHEAACENCLIB_WARN_RAP_TOO_CLOSE:
      retWarning = IIS_XHEAACENC_WARN_RAP_TOO_CLOSE;
      break;
    case XHEAACENCLIB_WARN_RAP_TOO_SOON:
      retWarning = IIS_XHEAACENC_WARN_RAP_TOO_SOON;
      break;
    case XHEAACENCLIB_WARN_INVALID_CONFIG:
      retWarning = IIS_XHEAACENC_WARN_INVALID_CONFIG;
      break;
    case XHEAACENCLIB_WARN_LOUDNESS_DEVIATION:
      retWarning = IIS_XHEAACENC_WARN_LOUDNESS_DEVIATION;
      break;
    case XHEAACENCLIB_WARN_LARGE_LOUDNESS_DEVIATION:
      retWarning = IIS_XHEAACENC_WARN_LARGE_LOUDNESS_DEVIATION;
      break;
    case XHEAACENCLIB_WARN_VALUE_OUT_OF_RANGE:
      retWarning = IIS_XHEAACENC_WARN_MPEG4_MD_INVALID_VALUE;
      break;
    case XHEAACENCLIB_WARN_VALUE_UPDATE_NOT_ALLOWED:
      retWarning = IIS_XHEAACENC_WARN_MPEG4_MD_UPDATE_NOT_ALLOWED;
      break;
    default:
      assert(0);
      break;
  }

  return retWarning;
}

/**********************************************************************/ /**
 Maps the dataFormat to the internal lib's param formats. Used in
 IIS_xHEAACEnc_SubmitAdvancedParam and IIS_xHEAACEnc_SubmitAdvancedData.
 **************************************************************************/
static IIS_XHEAACENC_RETURN_CODE mapDataFormat(
    IIS_XHEAACENC_PARAM_FORMAT const dataFormat,
    PARAM_FORMAT* const paramFormat) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  switch (dataFormat) {
    case IIS_XHEAACENC_PARAM_CHAR:
      *paramFormat = PARAM_CHAR;
      break;
    case IIS_XHEAACENC_PARAM_CHAR_ARRAY:
      *paramFormat = PARAM_CHAR_ARRAY;
      break;
    case IIS_XHEAACENC_PARAM_SHORT:
      *paramFormat = PARAM_SHORT;
      break;
    case IIS_XHEAACENC_PARAM_SHORT_ARRAY:
      *paramFormat = PARAM_CHAR_ARRAY;
      break;
    case IIS_XHEAACENC_PARAM_INT:
      *paramFormat = PARAM_INT;
      break;
    case IIS_XHEAACENC_PARAM_INT_ARRAY:
      *paramFormat = PARAM_INT_ARRAY;
      break;
    case IIS_XHEAACENC_PARAM_FLOAT:
      *paramFormat = PARAM_FLOAT;
      break;
    case IIS_XHEAACENC_PARAM_FLOAT_ARRAY:
      *paramFormat = PARAM_FLOAT_ARRAY;
      break;
    case IIS_XHEAACENC_PARAM_DOUBLE:
      *paramFormat = PARAM_DOUBLE;
      break;
    case IIS_XHEAACENC_PARAM_DOUBLE_ARRAY:
      *paramFormat = PARAM_DOUBLE_ARRAY;
      break;
    case IIS_XHEAACENC_PARAM_VOID_POINTER:
      *paramFormat = PARAM_VOID_POINTER;
      break;
    case IIS_XHEAACENC_PARAM_AUTO:

    case IIS_XHEAACENC_PARAM_INVALID:
    default:
      retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
  }

  return retValue;
}
