
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

#include "time_buffer.h"
#include "mathlib.h"
#include "iisutillib.h"

typedef struct T_MP4TIMEBUF {
  float *timeSignal;
  int readOffset;
  int nValidSamples;
  int bufferSize;
  int simSpace;
  int nChannels;
} MP4TIMEBUF;

HANDLE_ERROR_INFO
MP4TIMEBUF_Create(HANDLE_MP4TIMEBUF *phInputBuffer,
                  int bufferSize,
                  int simSpaceSize,
                  int nChannels,
                  int writeOffset) {
  HANDLE_ERROR_INFO error = noError;

  if (error == noError) {
    if (bufferSize < 0) {
      error = iisUtil_ERROR(CDI, "Invalid bufferSize.");
    }
  }

  if (error == noError) {
    if (simSpaceSize < 0) {
      error = iisUtil_ERROR(CDI, "Invalid simSpaceSize.");
    }
  }

  if (error == noError) {
    if ((nChannels < 1) || (nChannels > 64)) {
      error = iisUtil_ERROR(CDI, "Invalid nChannels.");
    }
  }

  if (error == noError) {
    if (writeOffset < 0 || writeOffset > bufferSize) {
      error = iisUtil_ERROR(CDI, "Invalid writeOffset.");
    }
  }

  if (error == noError) {
    if (NULL == ((*phInputBuffer) = (HANDLE_MP4TIMEBUF)iisCalloc(1, sizeof(MP4TIMEBUF)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc memory.");
    }
  }

  if (error == noError) {
    (*phInputBuffer)->timeSignal = NULL;
    (*phInputBuffer)->readOffset = 0;
    (*phInputBuffer)->nValidSamples = writeOffset;
    (*phInputBuffer)->bufferSize = bufferSize;
    (*phInputBuffer)->simSpace = simSpaceSize;
    (*phInputBuffer)->nChannels = nChannels;
  }

  if (error == noError) {
    if (NULL == ((*phInputBuffer)->timeSignal = (float *)iisCalloc(nChannels * (bufferSize + simSpaceSize), sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc memory.");
    } else {
      setFLOAT(0.0f, (*phInputBuffer)->timeSignal, nChannels * (bufferSize + simSpaceSize));
    }
  }

  return error;
}

HANDLE_ERROR_INFO
MP4TIMEBUF_Delete(HANDLE_MP4TIMEBUF *phInputBuffer) {
  HANDLE_ERROR_INFO error = noError;

  if (*phInputBuffer) {
    if ((*phInputBuffer)->timeSignal) {
      iisFree((*phInputBuffer)->timeSignal);
      (*phInputBuffer)->timeSignal = NULL;
    }
    iisFree(*phInputBuffer);
    *phInputBuffer = NULL;
  }

  return error;
}

static void transfer(float a, const float X[], int incX,
                     float Z[], int incZ, int n) {
  if (a != 1.0f) {
    if ((incZ == 1) && (incX == 1))
      smulFLOAT(a, X, Z, n);
    else
      smulFLOATflex(a, X, incX, Z, incZ, n);
  } else {
    if ((incZ == 1) && (incX == 1))
      copyFLOAT(X, Z, n);
    else
      copyFLOATflex(X, incX, Z, incZ, n);
  }
}

static int __GetWriteOffset(HANDLE_MP4TIMEBUF const hInputBuffer) {
  if (hInputBuffer != NULL) {
    return (hInputBuffer->nValidSamples + hInputBuffer->readOffset) % hInputBuffer->bufferSize;
  }
  return -1;
}

HANDLE_ERROR_INFO
MP4TIMEBUF_FeedBufferMulti(HANDLE_MP4TIMEBUF hInputBuffer,
                           float const *timeSignal,
                           int *channelSamples) {
  HANDLE_ERROR_INFO error = noError;

  if (hInputBuffer != NULL) {
    if (error == noError) {
      if (channelSamples == NULL) {
        error = iisUtil_ERROR(CDI, "Invalid pointer.");
      }
    }

    if (error == noError) {
      if (*channelSamples < 0) {
        error = iisUtil_ERROR(CDI, "Invalid channelSamples.");
      }
    }

    if (error == noError) {
      int firstPart = 0;
      float *pDest = 0;
      int nChan = hInputBuffer->nChannels;
      int nSamples = *channelSamples;
      int nSamplesAvail = hInputBuffer->bufferSize - hInputBuffer->nValidSamples;
      const int writeOffset = __GetWriteOffset(hInputBuffer);

      nSamples = min(nSamples, nSamplesAvail);
      firstPart = min(nSamples, hInputBuffer->bufferSize - writeOffset);
      pDest = &(hInputBuffer->timeSignal[nChan * writeOffset]);

      if (timeSignal) {
        transfer(1.f, timeSignal, 1, pDest, 1, nChan * firstPart);
      } else {
        setFLOATflex(0, pDest, 1, nChan * firstPart);
      }

      if (firstPart < nSamples) {
        if (timeSignal) {
          transfer(1.f, &timeSignal[nChan * firstPart], 1, hInputBuffer->timeSignal, 1, (nSamples - firstPart) * nChan);
        } else {
          setFLOATflex(0, hInputBuffer->timeSignal, 1, nChan * (nSamples - firstPart));
        }
      }

      *channelSamples -= nSamples;

      if (firstPart < nSamples) {
        int mirrorLength = min(hInputBuffer->simSpace, nSamples - firstPart);
        copyFLOAT(hInputBuffer->timeSignal,
                  &(hInputBuffer->timeSignal[nChan * hInputBuffer->bufferSize]),
                  nChan * (mirrorLength));
      }

      if (writeOffset < hInputBuffer->simSpace) {
        copyFLOAT(&(hInputBuffer->timeSignal[nChan * writeOffset]),
                  &(hInputBuffer->timeSignal[nChan * (hInputBuffer->bufferSize + writeOffset)]),
                  nChan * (hInputBuffer->simSpace - writeOffset));
      }

      hInputBuffer->nValidSamples += nSamples;
      if (hInputBuffer->nValidSamples > hInputBuffer->bufferSize || hInputBuffer->nValidSamples < 0) {
        error = iisUtil_ERROR(CDI, "Overrun of the MP4TIMEBUF");
      }
    }

  } else {
    error = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return error;
}

HANDLE_ERROR_INFO
MP4TIMEBUF_FeedBufferStereo(HANDLE_MP4TIMEBUF hInputBuffer,
                            const float *timeSigDataL,
                            const float *timeSigDataR,
                            unsigned int stride,
                            float scalefac,
                            int channelSamples) {
  HANDLE_ERROR_INFO error = noError;

  if (hInputBuffer != NULL) {
    if (error == noError) {
      if (channelSamples > hInputBuffer->bufferSize - hInputBuffer->nValidSamples || channelSamples < 0) {
        error = iisUtil_ERROR(CDI, "Too many channelSamples tried to feed in into MP4TIMEBUF");
      }
    }

    if (error == noError) {
      int firstPart = 0;
      float *pDest = 0;
      int nChan = hInputBuffer->nChannels;
      const int writeOffset = __GetWriteOffset(hInputBuffer);

      firstPart = min(channelSamples, hInputBuffer->bufferSize - writeOffset);
      pDest = &(hInputBuffer->timeSignal[nChan * writeOffset]);

      if (timeSigDataL) {
        transfer(scalefac, timeSigDataL, stride, pDest, nChan, firstPart);
      } else {
        setFLOATflex(0, pDest, nChan, firstPart);
      }

      if (nChan == 2) {
        if (timeSigDataR) {
          transfer(scalefac, timeSigDataR, stride, pDest + 1, nChan, firstPart);
        } else {
          setFLOATflex(0, pDest + 1, nChan, firstPart);
        }
      }

      if (firstPart < channelSamples) {
        if (timeSigDataL) {
          transfer(scalefac, &timeSigDataL[stride * firstPart], stride, hInputBuffer->timeSignal, nChan, (channelSamples - firstPart));
        } else {
          setFLOATflex(0, hInputBuffer->timeSignal, nChan, (channelSamples - firstPart));
        }

        if (nChan == 2) {
          if (timeSigDataR) {
            transfer(scalefac, &timeSigDataR[stride * firstPart], stride, hInputBuffer->timeSignal + 1, nChan, (channelSamples - firstPart));
          } else {
            setFLOATflex(0, hInputBuffer->timeSignal + 1, nChan, (channelSamples - firstPart));
          }
        }
      }

      if (firstPart < channelSamples) {
        int mirrorLength = min(hInputBuffer->simSpace, channelSamples - firstPart);
        copyFLOAT(hInputBuffer->timeSignal,
                  &(hInputBuffer->timeSignal[nChan * hInputBuffer->bufferSize]),
                  nChan * (mirrorLength));
      }

      if (writeOffset < hInputBuffer->simSpace) {
        copyFLOAT(&(hInputBuffer->timeSignal[nChan * writeOffset]),
                  &(hInputBuffer->timeSignal[nChan * (hInputBuffer->bufferSize + writeOffset)]),
                  nChan * (hInputBuffer->simSpace - writeOffset));
      }

      hInputBuffer->nValidSamples += channelSamples;
      if (hInputBuffer->nValidSamples > hInputBuffer->bufferSize || hInputBuffer->nValidSamples < 0) {
        error = iisUtil_ERROR(CDI, "Overrun of the MP4TIMEBUF");
      }
    }

  } else {
    error = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return error;
}

HANDLE_ERROR_INFO
MP4TIMEBUF_FeedBufferMonoFlex(HANDLE_MP4TIMEBUF hInputBuffer,
                              const float *timeSigData,
                              unsigned int stride,
                              float scalefac,
                              int channelSamples) {
  HANDLE_ERROR_INFO error = noError;

  if (hInputBuffer != NULL) {
    int firstPart = 0;
    float *pDest = 0;
    const int writeOffset = __GetWriteOffset(hInputBuffer);

    if (error == noError) {
      if (channelSamples > hInputBuffer->bufferSize - hInputBuffer->nValidSamples || channelSamples < 0) {
        error = iisUtil_ERROR(CDI, "Too many channelSamples tried to feed in into MP4TIMEBUF");
      }
    }

    if (error == noError) {
      if (hInputBuffer->nChannels != 1) {
        error = iisUtil_ERROR(CDI, "Invalid nChannels used for initialisation.");
      }
    }

    if (error == noError) {
      firstPart = min(channelSamples, hInputBuffer->bufferSize - writeOffset);
      pDest = &(hInputBuffer->timeSignal[writeOffset]);

      if (timeSigData) {
        transfer(scalefac, timeSigData, stride, pDest, 1, firstPart);
      } else
        setFLOAT(0, pDest, firstPart);

      if (firstPart < channelSamples) {
        if (timeSigData) {
          transfer(scalefac, &timeSigData[stride * firstPart], stride, hInputBuffer->timeSignal, 1, (channelSamples - firstPart));

        } else
          setFLOAT(0, hInputBuffer->timeSignal, channelSamples - firstPart);
      }

      if (firstPart < channelSamples) {
        int mirrorLength = min(hInputBuffer->simSpace, channelSamples - firstPart);
        copyFLOAT(hInputBuffer->timeSignal,
                  &hInputBuffer->timeSignal[hInputBuffer->bufferSize],
                  mirrorLength);
      }

      if (writeOffset < hInputBuffer->simSpace) {
        copyFLOAT(&hInputBuffer->timeSignal[writeOffset],
                  &hInputBuffer->timeSignal[hInputBuffer->bufferSize + writeOffset],
                  hInputBuffer->simSpace - writeOffset);
      }

      hInputBuffer->nValidSamples += channelSamples;
      if (hInputBuffer->nValidSamples > hInputBuffer->bufferSize || hInputBuffer->nValidSamples < 0) {
        error = iisUtil_ERROR(CDI, "Overrun of the MP4TIMEBUF");
      }
    }

  } else {
    error = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return error;
}

HANDLE_ERROR_INFO
MP4TIMEBUF_FeedBufferMono(HANDLE_MP4TIMEBUF hInputBuffer,
                          const float *timeSigData,
                          int channelSamples) {
  HANDLE_ERROR_INFO error = noError;

  if (hInputBuffer != NULL) {
    int firstPart = 0;
    float *pDest = 0;
    const int writeOffset = __GetWriteOffset(hInputBuffer);

    if (error == noError) {
      if (channelSamples > hInputBuffer->bufferSize - hInputBuffer->nValidSamples || channelSamples < 0) {
        error = iisUtil_ERROR(CDI, "Invalid channelSamples.");
      }
    }

    if (error == noError) {
      if (hInputBuffer->nChannels != 1) {
        error = iisUtil_ERROR(CDI, "Invalid nChannels used for initialisation.");
      }
    }

    if (error == noError) {
      firstPart = min(channelSamples, hInputBuffer->bufferSize - writeOffset);
      pDest = &(hInputBuffer->timeSignal[writeOffset]);

      if (timeSigData)
        copyFLOAT(timeSigData, pDest, firstPart);
      else
        setFLOAT(0, pDest, firstPart);

      if (firstPart < channelSamples) {
        if (timeSigData)
          copyFLOAT(&timeSigData[firstPart], hInputBuffer->timeSignal, channelSamples - firstPart);
        else
          setFLOAT(0, hInputBuffer->timeSignal, channelSamples - firstPart);
      }

      if (firstPart < channelSamples) {
        int mirrorLength = min(hInputBuffer->simSpace, channelSamples - firstPart);
        copyFLOAT(hInputBuffer->timeSignal,
                  &hInputBuffer->timeSignal[hInputBuffer->bufferSize],
                  mirrorLength);
      }

      if (writeOffset < hInputBuffer->simSpace) {
        copyFLOAT(&hInputBuffer->timeSignal[writeOffset],
                  &hInputBuffer->timeSignal[hInputBuffer->bufferSize + writeOffset],
                  hInputBuffer->simSpace - writeOffset);
      }

      hInputBuffer->nValidSamples += channelSamples;
      if (hInputBuffer->nValidSamples > hInputBuffer->bufferSize || hInputBuffer->nValidSamples < 0) {
        error = iisUtil_ERROR(CDI, "Overrun of the MP4TIMEBUF");
      }
    }

  } else {
    error = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return error;
}

HANDLE_ERROR_INFO MP4TIMEBUF_CopyBuffer(HANDLE_MP4TIMEBUF hMp4TimeBufDst, HANDLE_MP4TIMEBUF hMp4TimeBufSrc) {
  HANDLE_ERROR_INFO error = noError;

  float *timeSignalBufferSrc = NULL;
  float *timeSignalBufferDst = NULL;

  timeSignalBufferSrc = hMp4TimeBufSrc->timeSignal;
  timeSignalBufferDst = hMp4TimeBufDst->timeSignal;

  copyFLOAT(timeSignalBufferSrc, timeSignalBufferDst, hMp4TimeBufSrc->bufferSize);

  hMp4TimeBufDst->readOffset = hMp4TimeBufSrc->readOffset;
  hMp4TimeBufDst->nValidSamples = hMp4TimeBufSrc->nValidSamples;

  return error;
}

float *MP4TIMEBUF_AccessBuffer(HANDLE_MP4TIMEBUF hMp4TimeBuf, int offset, int ch)

{
  float *timeSignalBuffer = NULL;

  if (hMp4TimeBuf) {
    timeSignalBuffer = hMp4TimeBuf->timeSignal + ((hMp4TimeBuf->readOffset + offset) % hMp4TimeBuf->bufferSize) * hMp4TimeBuf->nChannels + ch;
  }

  return (timeSignalBuffer);
}

HANDLE_ERROR_INFO MP4TIMEBUF_SaveAccessBuffer(HANDLE_MP4TIMEBUF hMp4TimeBuf, int offset, int minValidSamplesPerChannel, int ch, float **accessPointer) {
  HANDLE_ERROR_INFO error = noError;
  int tmp_buffer_offset = 0;

  if (error == noError) {
    if (hMp4TimeBuf == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid pointer");
    }
  }

  if (error == noError) {
    tmp_buffer_offset = (hMp4TimeBuf->readOffset + offset) % hMp4TimeBuf->bufferSize;
    if ((minValidSamplesPerChannel > hMp4TimeBuf->nValidSamples - offset) || (minValidSamplesPerChannel > hMp4TimeBuf->bufferSize - tmp_buffer_offset + hMp4TimeBuf->simSpace)) {
      error = iisUtil_ERROR(CDI, "not enough valid samples in Timebuffer");
    }
  }

  if (error == noError) {
    if (ch >= hMp4TimeBuf->nChannels) {
      error = iisUtil_ERROR(CDI, "channel index is higher or equal to number of channels");
    }
  }

  if (error == noError) {
    if (accessPointer != NULL) {
      *accessPointer = hMp4TimeBuf->timeSignal + tmp_buffer_offset * hMp4TimeBuf->nChannels + ch;
    }
  }

  return error;
}

HANDLE_ERROR_INFO
MP4TIMEBUF_UpdateBuffer(HANDLE_MP4TIMEBUF const hInputBuffer,
                        float const *const timeSignal,
                        int const offset,
                        int numChannelSamples) {
  HANDLE_ERROR_INFO error = noError;
  int initialNumValidSamples = 0;

  if (hInputBuffer == NULL || timeSignal == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid pointer");
  } else if (offset < 0) {
    error = iisUtil_ERROR(CDI, "Invalid offset");
  } else if (numChannelSamples + offset > hInputBuffer->nValidSamples) {
    error = iisUtil_ERROR(CDI, "Not enough valid samples in timebuffer");
  }

  if (error == noError) {
    initialNumValidSamples = hInputBuffer->nValidSamples;
    hInputBuffer->nValidSamples = offset;

    error = MP4TIMEBUF_FeedBufferMulti(hInputBuffer, timeSignal, &numChannelSamples);
  }

  if (error == noError) {
    if (numChannelSamples != 0) {
      error = iisUtil_ERROR(CDI, "Not enough samples updated");
    } else if (hInputBuffer->nValidSamples > initialNumValidSamples) {
      error = iisUtil_ERROR(CDI, "Too many samples updated");
    }
  }

  if (error == noError) {
    hInputBuffer->nValidSamples = initialNumValidSamples;
  }

  return error;
}

HANDLE_ERROR_INFO
MP4TIMEBUF_InvalidateBuffer(HANDLE_MP4TIMEBUF hMp4TimeBuf, int size) {
  HANDLE_ERROR_INFO error = noError;

  if (error == noError) {
    if (hMp4TimeBuf != NULL) {
      hMp4TimeBuf->readOffset = (hMp4TimeBuf->readOffset + size) % hMp4TimeBuf->bufferSize;
      hMp4TimeBuf->nValidSamples -= size;
      if (hMp4TimeBuf->nValidSamples < 0 || hMp4TimeBuf->nValidSamples > hMp4TimeBuf->bufferSize) {
        hMp4TimeBuf->nValidSamples = 0;
        error = iisUtil_ERROR(CDI, "Invalidate too many Data");
      }
    } else {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    }
  }

  return error;
}

HANDLE_ERROR_INFO
MP4TIMEBUF_getFreeSamples(HANDLE_MP4TIMEBUF hMp4TimeBuf, int *freeSize) {
  HANDLE_ERROR_INFO error = noError;

  if (error == noError) {
    if (hMp4TimeBuf == NULL || freeSize == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    }
  }
  if (error == noError) {
    *freeSize = hMp4TimeBuf->bufferSize - hMp4TimeBuf->nValidSamples;
    if (*freeSize < 0 || *freeSize > hMp4TimeBuf->bufferSize) {
      error = iisUtil_ERROR(CDI, "Error in free samples calculation");
    }
  }

  return error;
}

HANDLE_ERROR_INFO
MP4TIMEBUF_getValidSamples(HANDLE_MP4TIMEBUF hMp4TimeBuf, int *validSamples) {
  HANDLE_ERROR_INFO error = noError;

  if (error == noError) {
    if (hMp4TimeBuf == NULL || validSamples == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    }
  }
  if (error == noError) {
    *validSamples = hMp4TimeBuf->nValidSamples;
    if (*validSamples < 0 || *validSamples > hMp4TimeBuf->bufferSize) {
      error = iisUtil_ERROR(CDI, "Error in valid samples calculation");
    }
  }

  return error;
}
