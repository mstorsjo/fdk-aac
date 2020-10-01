/******************************************************************************
 *
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************
 * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
 */

#include <string>
#include "aacenc_lib.h"

using namespace std;

// IN_AUDIO_DATA, IN_ANCILLRY_DATA and IN_METADATA_SETUP
constexpr size_t kMaxBuffers = 3;

constexpr size_t kMaxOutputBufferSize = 8192;

constexpr uint32_t kMinBitRate = 8000;
constexpr uint32_t kMaxBitRate = 960000;

constexpr uint32_t kSampleRates[] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000};
constexpr size_t kSampleRatesSize = size(kSampleRates);

constexpr CHANNEL_MODE kChannelModes[] = {MODE_1,     MODE_2,       MODE_1_2,       MODE_1_2_1,
                                          MODE_1_2_2, MODE_1_2_2_1, MODE_1_2_2_2_1, MODE_7_1_BACK};
constexpr size_t kChannelModesSize = size(kChannelModes);

constexpr TRANSPORT_TYPE kIdentifiers[] = {
    TT_MP4_RAW, TT_MP4_ADIF, TT_MP4_ADTS, TT_MP4_LATM_MCP1, TT_MP4_LATM_MCP0, TT_MP4_LOAS, TT_DRM};
constexpr size_t kIdentifiersSize = size(kIdentifiers);

constexpr AUDIO_OBJECT_TYPE kAudioObjectTypes[] = {AOT_AAC_LC, AOT_ER_AAC_ELD, AOT_SBR, AOT_PS,
                                                   AOT_ER_AAC_LD};
constexpr size_t kAudioObjectTypesSize = size(kAudioObjectTypes);

constexpr int32_t kSbrRatios[] = {0, 1, 2};
constexpr size_t kSbrRatiosSize = size(kSbrRatios);

constexpr AACENC_METADATA_DRC_PROFILE kMetaDataDrcProfiles[] = {
    AACENC_METADATA_DRC_NONE,       AACENC_METADATA_DRC_FILMSTANDARD,
    AACENC_METADATA_DRC_FILMLIGHT,  AACENC_METADATA_DRC_MUSICSTANDARD,
    AACENC_METADATA_DRC_MUSICLIGHT, AACENC_METADATA_DRC_SPEECH,
    AACENC_METADATA_DRC_NOT_PRESENT};
constexpr size_t kMetaDataDrcProfilesSize = size(kMetaDataDrcProfiles);

enum {
    IDX_SBR_MODE = 0,
    IDX_AAC_AOT,
    IDX_SAMPLE_RATE,
    IDX_BIT_RATE_1,
    IDX_BIT_RATE_2,
    IDX_BIT_RATE_3,
    IDX_CHANNEL,
    IDX_IDENTIFIER,
    IDX_SBR_RATIO,

    IDX_METADATA_DRC_PROFILE,
    IDX_METADATA_COMP_PROFILE,
    IDX_METADATA_DRC_TARGET_REF_LEVEL,
    IDX_METADATA_COMP_TARGET_REF_LEVEL,
    IDX_METADATA_PROG_LEVEL_PRESENT,
    IDX_METADATA_PROG_LEVEL,
    IDX_METADATA_PCE_MIXDOWN_IDX_PRESENT,
    IDX_METADATA_ETSI_DMXLVL_PRESENT,
    IDX_METADATA_CENTER_MIX_LEVEL,
    IDX_METADATA_SURROUND_MIX_LEVEL,
    IDX_METADATA_DOLBY_SURROUND_MODE,
    IDX_METADATA_DRC_PRESENTATION_MODE,
    IDX_METADATA_EXT_ANC_DATA_ENABLE,
    IDX_METADATA_EXT_DOWNMIX_LEVEL_ENABLE,
    IDX_METADATA_EXT_DOWNMIX_LEVEL_A,
    IDX_METADATA_EXT_DOWNMIX_LEVEL_B,
    IDX_METADATA_DMX_GAIN_ENABLE,
    IDX_METADATA_DMX_GAIN_5,
    IDX_METADATA_DMX_GAIN_2,
    IDX_METADATA_LFE_DMX_ENABLE,
    IDX_METADATA_LFE_DMX_LEVEL,

    IDX_IN_BUFFER_INDEX_1,
    IDX_IN_BUFFER_INDEX_2,
    IDX_IN_BUFFER_INDEX_3,

    IDX_LAST
};

enum aac_sbr_mode_t : uint32_t {
    AAC_SBR_OFF,
    AAC_SBR_SINGLE_RATE,
    AAC_SBR_DUAL_RATE,
    AAC_SBR_AUTO
};

template <typename type1, typename type2, typename type3>
auto generateNumberInRangeFromData(type1 data, type2 min, type3 max) -> decltype(max) {
    return (data % (1 + max - min)) + min;
}

class Codec {
   public:
    ~Codec() { deInitEncoder(); }
    bool initEncoder(uint8_t **dataPtr, size_t *sizePtr);
    void encodeFrames(const uint8_t *data, size_t size);
    void deInitEncoder();

   private:
    void setupMetaData(uint8_t *data);

    HANDLE_AACENCODER mEncoder = nullptr;
    AACENC_MetaData mMetaData = {};
    uint32_t mInBufferIdx_1 = 0;
    uint32_t mInBufferIdx_2 = 0;
    uint32_t mInBufferIdx_3 = 0;
};

void Codec::setupMetaData(uint8_t *data) {
    uint32_t drcProfileIndex = generateNumberInRangeFromData(data[IDX_METADATA_DRC_PROFILE], 0,
                                                             kMetaDataDrcProfilesSize - 1);
    AACENC_METADATA_DRC_PROFILE drcProfile = kMetaDataDrcProfiles[drcProfileIndex];
    mMetaData.drc_profile = drcProfile;

    uint32_t compProfileIndex = generateNumberInRangeFromData(data[IDX_METADATA_COMP_PROFILE], 0,
                                                              kMetaDataDrcProfilesSize - 1);
    AACENC_METADATA_DRC_PROFILE compProfile = kMetaDataDrcProfiles[compProfileIndex];
    mMetaData.comp_profile = compProfile;

    INT drcTargetRefLevel =
        generateNumberInRangeFromData(data[IDX_METADATA_DRC_TARGET_REF_LEVEL], 0, UINT8_MAX);
    mMetaData.drc_TargetRefLevel = drcTargetRefLevel;

    INT compTargetRefLevel =
        generateNumberInRangeFromData(data[IDX_METADATA_COMP_TARGET_REF_LEVEL], 0, UINT8_MAX);
    mMetaData.comp_TargetRefLevel = compTargetRefLevel;

    INT isProgRefLevelPresent =
        generateNumberInRangeFromData(data[IDX_METADATA_PROG_LEVEL_PRESENT], 0, 1);
    mMetaData.prog_ref_level_present = isProgRefLevelPresent;

    INT progRefLevel = generateNumberInRangeFromData(data[IDX_METADATA_PROG_LEVEL], 0, UINT8_MAX);
    mMetaData.prog_ref_level = progRefLevel;

    UCHAR isPCEMixdownIdxPresent =
        generateNumberInRangeFromData(data[IDX_METADATA_PCE_MIXDOWN_IDX_PRESENT], 0, 1);
    mMetaData.PCE_mixdown_idx_present = isPCEMixdownIdxPresent;

    UCHAR isETSIDmxLvlPresent =
        generateNumberInRangeFromData(data[IDX_METADATA_ETSI_DMXLVL_PRESENT], 0, 1);
    mMetaData.ETSI_DmxLvl_present = isETSIDmxLvlPresent;

    SCHAR centerMixLevel = generateNumberInRangeFromData(data[IDX_METADATA_CENTER_MIX_LEVEL], 0, 7);
    mMetaData.centerMixLevel = centerMixLevel;

    SCHAR surroundMixLevel =
        generateNumberInRangeFromData(data[IDX_METADATA_SURROUND_MIX_LEVEL], 0, 7);
    mMetaData.surroundMixLevel = surroundMixLevel;

    UCHAR dolbySurroundMode =
        generateNumberInRangeFromData(data[IDX_METADATA_DOLBY_SURROUND_MODE], 0, 2);
    mMetaData.dolbySurroundMode = dolbySurroundMode;

    UCHAR drcPresentationMode =
        generateNumberInRangeFromData(data[IDX_METADATA_DRC_PRESENTATION_MODE], 0, 2);
    mMetaData.drcPresentationMode = drcPresentationMode;

    UCHAR extAncDataEnable =
        generateNumberInRangeFromData(data[IDX_METADATA_EXT_ANC_DATA_ENABLE], 0, 1);
    mMetaData.ExtMetaData.extAncDataEnable = extAncDataEnable;

    UCHAR extDownmixLevelEnable =
        generateNumberInRangeFromData(data[IDX_METADATA_EXT_DOWNMIX_LEVEL_ENABLE], 0, 1);
    mMetaData.ExtMetaData.extDownmixLevelEnable = extDownmixLevelEnable;

    UCHAR extDownmixLevel_A =
        generateNumberInRangeFromData(data[IDX_METADATA_EXT_DOWNMIX_LEVEL_A], 0, 7);
    mMetaData.ExtMetaData.extDownmixLevel_A = extDownmixLevel_A;

    UCHAR extDownmixLevel_B =
        generateNumberInRangeFromData(data[IDX_METADATA_EXT_DOWNMIX_LEVEL_B], 0, 7);
    mMetaData.ExtMetaData.extDownmixLevel_B = extDownmixLevel_B;

    UCHAR dmxGainEnable = generateNumberInRangeFromData(data[IDX_METADATA_DMX_GAIN_ENABLE], 0, 1);
    mMetaData.ExtMetaData.dmxGainEnable = dmxGainEnable;

    INT dmxGain5 = generateNumberInRangeFromData(data[IDX_METADATA_DMX_GAIN_5], 0, UINT8_MAX);
    mMetaData.ExtMetaData.dmxGain5 = dmxGain5;

    INT dmxGain2 = generateNumberInRangeFromData(data[IDX_METADATA_DMX_GAIN_2], 0, UINT8_MAX);
    mMetaData.ExtMetaData.dmxGain2 = dmxGain2;

    UCHAR lfeDmxEnable = generateNumberInRangeFromData(data[IDX_METADATA_LFE_DMX_ENABLE], 0, 1);
    mMetaData.ExtMetaData.lfeDmxEnable = lfeDmxEnable;

    UCHAR lfeDmxLevel = generateNumberInRangeFromData(data[IDX_METADATA_LFE_DMX_LEVEL], 0, 15);
    mMetaData.ExtMetaData.lfeDmxLevel = lfeDmxLevel;
}

bool Codec::initEncoder(uint8_t **dataPtr, size_t *sizePtr) {
    uint8_t *data = *dataPtr;

    if (AACENC_OK != aacEncOpen(&mEncoder, 0, 0)) {
        return false;
    }

    uint32_t sbrMode = generateNumberInRangeFromData(data[IDX_SBR_MODE], (uint32_t)AAC_SBR_OFF,
                                                     (uint32_t)AAC_SBR_AUTO);
    aacEncoder_SetParam(mEncoder, AACENC_SBR_MODE, sbrMode);

    uint32_t sbrRatioIndex =
        generateNumberInRangeFromData(data[IDX_SBR_RATIO], 0, kSbrRatiosSize - 1);
    int32_t sbrRatio = kSbrRatios[sbrRatioIndex];
    aacEncoder_SetParam(mEncoder, AACENC_SBR_RATIO, sbrRatio);

    uint32_t aacAOTIndex =
        generateNumberInRangeFromData(data[IDX_AAC_AOT], 0, kAudioObjectTypesSize - 1);
    uint32_t aacAOT = kAudioObjectTypes[aacAOTIndex];
    aacEncoder_SetParam(mEncoder, AACENC_AOT, aacAOT);

    uint32_t sampleRateIndex =
        generateNumberInRangeFromData(data[IDX_SAMPLE_RATE], 0, kSampleRatesSize - 1);
    uint32_t sampleRate = kSampleRates[sampleRateIndex];
    aacEncoder_SetParam(mEncoder, AACENC_SAMPLERATE, sampleRate);

    uint32_t tempValue =
        (data[IDX_BIT_RATE_1] << 16) | (data[IDX_BIT_RATE_2] << 8) | data[IDX_BIT_RATE_3];
    uint32_t bitRate = generateNumberInRangeFromData(tempValue, kMinBitRate, kMaxBitRate);
    aacEncoder_SetParam(mEncoder, AACENC_BITRATE, bitRate);

    uint32_t channelModeIndex =
        generateNumberInRangeFromData(data[IDX_CHANNEL], 0, kChannelModesSize - 1);
    CHANNEL_MODE channelMode = kChannelModes[channelModeIndex];
    aacEncoder_SetParam(mEncoder, AACENC_CHANNELMODE, channelMode);

    uint32_t identifierIndex =
        generateNumberInRangeFromData(data[IDX_IDENTIFIER], 0, kIdentifiersSize - 1);
    uint32_t identifier = kIdentifiers[identifierIndex];
    aacEncoder_SetParam(mEncoder, AACENC_TRANSMUX, identifier);

    mInBufferIdx_1 = generateNumberInRangeFromData(data[IDX_IN_BUFFER_INDEX_1], 0, kMaxBuffers - 1);
    mInBufferIdx_2 = generateNumberInRangeFromData(data[IDX_IN_BUFFER_INDEX_2], 0, kMaxBuffers - 1);
    mInBufferIdx_3 = generateNumberInRangeFromData(data[IDX_IN_BUFFER_INDEX_3], 0, kMaxBuffers - 1);

    setupMetaData(data);

    // Not re-using the data which was used for configuration for encoding
    *dataPtr += IDX_LAST;
    *sizePtr -= IDX_LAST;

    return true;
}

static void deleteBuffers(uint8_t **buffers, size_t size) {
    for (size_t n = 0; n < size; ++n) {
        delete[] buffers[n];
    }
    delete[] buffers;
}

void Codec::encodeFrames(const uint8_t *data, size_t size) {
    uint8_t *audioData = (uint8_t *)data;
    uint8_t *ancData = (uint8_t *)data;
    size_t audioSize = size;
    size_t ancSize = size;

    while ((audioSize > 0) && (ancSize > 0)) {
        AACENC_InArgs inargs;
        memset(&inargs, 0, sizeof(inargs));
        inargs.numInSamples = audioSize / sizeof(int16_t);
        inargs.numAncBytes = ancSize;

        void *buffers[] = {(void *)audioData, (void *)ancData, &mMetaData};
        INT bufferIds[] = {IN_AUDIO_DATA, IN_ANCILLRY_DATA, IN_METADATA_SETUP};
        INT bufferSizes[] = {static_cast<INT>(audioSize), static_cast<INT>(ancSize),
                             static_cast<INT>(sizeof(mMetaData))};
        INT bufferElSizes[] = {sizeof(int16_t), sizeof(UCHAR), sizeof(AACENC_MetaData)};

        void *inBuffer[kMaxBuffers] = {};
        INT inBufferIds[kMaxBuffers] = {};
        INT inBufferSize[kMaxBuffers] = {};
        INT inBufferElSize[kMaxBuffers] = {};
        for (int32_t buffer = 0; buffer < kMaxBuffers; ++buffer) {
            uint32_t Idxs[] = {mInBufferIdx_1, mInBufferIdx_2, mInBufferIdx_3};
            inBuffer[buffer] = buffers[Idxs[buffer]];
            inBufferIds[buffer] = bufferIds[Idxs[buffer]];
            inBufferSize[buffer] = bufferSizes[Idxs[buffer]];
            inBufferElSize[buffer] = bufferElSizes[Idxs[buffer]];
        }

        AACENC_BufDesc inBufDesc;
        inBufDesc.numBufs = kMaxBuffers;
        inBufDesc.bufs = (void **)&inBuffer;
        inBufDesc.bufferIdentifiers = inBufferIds;
        inBufDesc.bufSizes = inBufferSize;
        inBufDesc.bufElSizes = inBufferElSize;

        uint8_t **outPtrRef = new uint8_t *[kMaxBuffers];
        for (int32_t buffer = 0; buffer < kMaxBuffers; ++buffer) {
            outPtrRef[buffer] = new uint8_t[kMaxOutputBufferSize];
        }

        void *outBuffer[kMaxBuffers];
        INT outBufferIds[kMaxBuffers];
        INT outBufferSize[kMaxBuffers];
        INT outBufferElSize[kMaxBuffers];

        for (int32_t buffer = 0; buffer < kMaxBuffers; ++buffer) {
            outBuffer[buffer] = outPtrRef[buffer];
            outBufferIds[buffer] = OUT_BITSTREAM_DATA;
            outBufferSize[buffer] = (INT)kMaxOutputBufferSize;
            outBufferElSize[buffer] = sizeof(UCHAR);
        }

        AACENC_BufDesc outBufDesc;
        outBufDesc.numBufs = kMaxBuffers;
        outBufDesc.bufs = (void **)&outBuffer;
        outBufDesc.bufferIdentifiers = outBufferIds;
        outBufDesc.bufSizes = outBufferSize;
        outBufDesc.bufElSizes = outBufferElSize;

        AACENC_OutArgs outargs = {};
        aacEncEncode(mEncoder, &inBufDesc, &outBufDesc, &inargs, &outargs);

        if (outargs.numOutBytes == 0) {
            if (audioSize > 0) {
                ++audioData;
                --audioSize;
            }
            if (ancSize > 0) {
                ++ancData;
                --ancSize;
            }
        } else {
            size_t audioConsumed = outargs.numInSamples * sizeof(int16_t);
            audioData += audioConsumed;
            audioSize -= audioConsumed;

            size_t ancConsumed = outargs.numAncBytes;
            ancData += ancConsumed;
            ancSize -= ancConsumed;
        }
        deleteBuffers(outPtrRef, kMaxBuffers);

        // break out of loop if only metadata was sent in all the input buffers
        // as sending it multiple times in a loop is redundant.
        if ((mInBufferIdx_1 == kMaxBuffers - 1) && (mInBufferIdx_2 == kMaxBuffers - 1) &&
            (mInBufferIdx_3 == kMaxBuffers - 1)) {
            break;
        }
    }
}

void Codec::deInitEncoder() { aacEncClose(&mEncoder); }

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < IDX_LAST) {
        return 0;
    }
    Codec encoder;
    if (encoder.initEncoder(const_cast<uint8_t **>(&data), &size)) {
        encoder.encodeFrames(data, size);
    }
    return 0;
}
