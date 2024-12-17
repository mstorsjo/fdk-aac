/******************************************************************************
 *
 * Copyright (C) 2024 The Android Open Source Project
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
 */

#include <benchmark/benchmark.h>
#include <log/log.h>

#include <iostream>
#include <sys/stat.h>
#include <vector>

#include "aacenc_lib.h"

class AACEncoder {
   private:
    HANDLE_AACENCODER mAACEncoder;
    AACENC_InfoStruct mEncInfo;
    const AUDIO_OBJECT_TYPE mProfile;
    const CHANNEL_MODE mChannelCount;
    const int mSampleRate;
    const int mBitRate;

   public:
    AACEncoder(int sampleRate, int bitRate, AUDIO_OBJECT_TYPE profile, CHANNEL_MODE channelCount)
        : mAACEncoder(nullptr), mProfile(profile), mChannelCount(channelCount),
        mSampleRate(sampleRate), mBitRate(bitRate) {}

    bool initialize() {
        if (aacEncOpen(&mAACEncoder, 0, 0) != AACENC_OK) {
            ALOGE("Failed to initialize AAC encoder");
            return false;
        }

        if (aacEncoder_SetParam(mAACEncoder, AACENC_AOT, mProfile) != AACENC_OK
            || aacEncoder_SetParam(mAACEncoder, AACENC_SAMPLERATE, mSampleRate) != AACENC_OK
            || aacEncoder_SetParam(mAACEncoder, AACENC_CHANNELMODE, mChannelCount) != AACENC_OK
            || aacEncoder_SetParam(mAACEncoder, AACENC_BITRATE, mBitRate) != AACENC_OK
            || aacEncoder_SetParam(mAACEncoder, AACENC_TRANSMUX, TT_MP4_RAW) != AACENC_OK) {
            ALOGE("Failed to set AAC encoder parameters");
            return false;
        }

        if (aacEncEncode(mAACEncoder, nullptr, nullptr, nullptr, nullptr) != AACENC_OK) {
            ALOGE("Unable to initialize encoder for profile:%d, sample-rate: %d, bit-rate: %d, "
                "channels: %d", mProfile, mSampleRate, mBitRate, mChannelCount);
            return false;
        }

        if (aacEncInfo(mAACEncoder, &mEncInfo) != AACENC_OK) {
            ALOGE("Failed to get AAC encoder info");
            return false;
        }
        return true;
    }

    ~AACEncoder() {
        if (mAACEncoder) {
            aacEncClose(&mAACEncoder);
        }
    }

    int getChannels() const { return aacEncoder_GetParam(mAACEncoder, AACENC_CHANNELMODE); }
    int getSampleRate() const { return aacEncoder_GetParam(mAACEncoder, AACENC_SAMPLERATE); }
    int getBitRate() const { return aacEncoder_GetParam(mAACEncoder, AACENC_BITRATE); }
    int getProfile() const { return aacEncoder_GetParam(mAACEncoder, AACENC_AOT); }

    bool encode(const std::vector<uint8_t>& pcmFrames) {
        size_t frameSize = mEncInfo.frameLength * mChannelCount * sizeof(uint16_t);
        std::vector<uint8_t> encodedBuffer(frameSize);
        AACENC_BufDesc inBufDesc, outBufDesc;
        AACENC_InArgs inArgs;
        AACENC_OutArgs outArgs;

        void* outBuffer[] = {encodedBuffer.data()};
        int outBufferIds[] = {OUT_BITSTREAM_DATA};
        int outBufferSize[] = {static_cast<int>(encodedBuffer.size())};
        int outBufferElSize[] = {sizeof(UCHAR)};

        outBufDesc.numBufs = sizeof(outBuffer) / sizeof(void*);
        outBufDesc.bufs = (void**)&outBuffer;
        outBufDesc.bufferIdentifiers = outBufferIds;
        outBufDesc.bufSizes = outBufferSize;
        outBufDesc.bufElSizes = outBufferElSize;

        size_t numFrames = pcmFrames.size() / frameSize;

        for (size_t frameIdx = 0; ; ++frameIdx) {

            const uint8_t* frameData = nullptr;
            void* inBuffer[1];
            int inBufferSize[1];
            if (frameIdx < numFrames) {
                frameData = pcmFrames.data() + frameIdx * frameSize;
            }

            if (frameData != nullptr) {
                inBuffer[0] = const_cast<uint8_t*>(frameData);
                inBufferSize[0] = static_cast<int>(frameSize);
                inArgs.numInSamples = frameSize / sizeof(uint16_t);
            } else {
                inBuffer[0] = nullptr;
                inBufferSize[0] = 0;
                inArgs.numInSamples = -1;
            }

            int inBufferIds[] = {IN_AUDIO_DATA};
            int inBufferElSize[] = {sizeof(int16_t)};

            inBufDesc.numBufs = sizeof(inBuffer) / sizeof(void*);
            inBufDesc.bufs = (void**)&inBuffer;
            inBufDesc.bufferIdentifiers = inBufferIds;
            inBufDesc.bufSizes = inBufferSize;
            inBufDesc.bufElSizes = inBufferElSize;
            AACENC_ERROR err =
                aacEncEncode(mAACEncoder, &inBufDesc, &outBufDesc, &inArgs, &outArgs);
            if (err != AACENC_OK) {
                if (err == AACENC_ENCODE_EOF) {
                    break;
                }
                ALOGE("Failed to encode AAC frame");
                return false;
            }
        }
        return true;
    }
};

std::vector<uint8_t> readInputFile(const std::string& folderPath, const std::string& pcmFile) {
    std::string fullPcmPath = folderPath + "/" + pcmFile;
    std::vector<uint8_t> inputBuffer;

    FILE* pcmFilePtr = fopen(fullPcmPath.c_str(), "rb");
    if (!pcmFilePtr) {
        ALOGE("Failed to open pcm file %s", fullPcmPath.c_str());
        return inputBuffer;
    }

    struct stat fileStat;
    int fd = fileno(pcmFilePtr);

    if (fstat(fd, &fileStat) == -1) {
        ALOGE("Error occured while accessing the pcm file");
        return inputBuffer;
    }
    size_t fileSize = fileStat.st_size;
    inputBuffer.resize(fileSize);
    size_t bytesRead = fread(inputBuffer.data(), sizeof(uint8_t), inputBuffer.size(), pcmFilePtr);
    if (bytesRead != fileSize) {
        ALOGE("Failed to read the complete pcm data");
        return std::vector<uint8_t>();
    }

    fclose(pcmFilePtr);
    return inputBuffer;
}

static void BM_EncodeAAC(benchmark::State& state, const std::string& inpFolderPath,
                         const std::string& pcmFile, const int sampleRate, const int bitRate,
                         const AUDIO_OBJECT_TYPE profile, const CHANNEL_MODE channelCount) {
    auto inputBuffer = readInputFile(inpFolderPath, pcmFile);
    if (inputBuffer.empty()) {
        state.SkipWithError("Failed to read input from pcm file");
        return;
    }
    AACEncoder encoder(sampleRate, bitRate, profile, channelCount);

    if (!encoder.initialize()) {
        state.SkipWithError("Unable to initialize encoder");
        return;
    }

    for (auto _ : state) {
        if (!encoder.encode(inputBuffer)) {
            state.SkipWithError("Unable to encode the Stream");
            return;
        }
    }

    state.SetLabel(pcmFile + ", " + std::to_string(encoder.getChannels()) + ", "
                   + std::to_string(encoder.getSampleRate()) + ", "
                   + std::to_string(encoder.getBitRate()) + ", "
                   + std::to_string(encoder.getProfile()));
}

void RegisterBenchmarks(const std::string& folderPath) {
    // testlabel, BM function, folderpath, pcm file, sampleRate, bitRate, profile, ChannelCount
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_1ch_8kHz_48kbps_lc", BM_EncodeAAC,
                                folderPath, "bbb_1ch_8kHz.pcm", 8000, 48000, AOT_AAC_LC,
                                MODE_1);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_1ch_48kHz_128kbps_lc", BM_EncodeAAC,
                                folderPath, "bbb_1ch_48kHz.pcm", 48000, 128000, AOT_AAC_LC,
                                MODE_1);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_2ch_48kHz_128kbps_lc", BM_EncodeAAC,
                                folderPath, "bbb_2ch_48kHz.pcm", 48000, 128000, AOT_AAC_LC,
                                MODE_2);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_6ch_48kHz_128kbps_lc", BM_EncodeAAC,
                                folderPath, "bbb_6ch_48kHz.pcm", 48000, 128000, AOT_AAC_LC,
                                MODE_1_2_2_1);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_1ch_16kHz_48kbps_he", BM_EncodeAAC,
                                folderPath, "bbb_1ch_16kHz.pcm", 16000, 48000, AOT_SBR,
                                MODE_1);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_1ch_48kHz_128kbps_he", BM_EncodeAAC,
                                folderPath, "bbb_1ch_48kHz.pcm", 48000, 128000, AOT_SBR,
                                MODE_1);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_2ch_48kHz_128kbps_he", BM_EncodeAAC,
                                folderPath, "bbb_2ch_48kHz.pcm", 48000, 128000, AOT_SBR,
                                MODE_2);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_2ch_16kHz_48kbps_hev2", BM_EncodeAAC,
                                folderPath, "bbb_2ch_16kHz.pcm", 16000, 48000, AOT_PS,
                                MODE_2);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_2ch_48kHz_128kbps_hev2", BM_EncodeAAC,
                                folderPath, "bbb_2ch_48kHz.pcm", 48000, 128000, AOT_PS,
                                MODE_2);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_1ch_48kHz_128kbps_ld", BM_EncodeAAC,
                                folderPath, "bbb_1ch_48kHz.pcm", 48000, 128000, AOT_ER_AAC_LD,
                                MODE_1);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_2ch_48kHz_128kbps_ld", BM_EncodeAAC,
                                folderPath, "bbb_2ch_48kHz.pcm", 48000, 128000, AOT_ER_AAC_LD,
                                MODE_2);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_6ch_48kHz_128kbps_ld", BM_EncodeAAC,
                                folderPath, "bbb_6ch_48kHz.pcm", 48000, 128000, AOT_ER_AAC_LD,
                                MODE_1_2_2_1);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_1ch_16kHz_64kbps_eld", BM_EncodeAAC,
                                folderPath, "bbb_1ch_16kHz.pcm", 16000, 64000, AOT_ER_AAC_ELD,
                                MODE_1);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_1ch_48kHz_128kbps_eld", BM_EncodeAAC,
                                folderPath, "bbb_1ch_48kHz.pcm", 48000, 128000, AOT_ER_AAC_ELD,
                                MODE_1);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_2ch_16kHz_64kbps_eld", BM_EncodeAAC,
                                folderPath, "bbb_2ch_16kHz.pcm", 16000, 64000, AOT_ER_AAC_ELD,
                                MODE_2);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_2ch_48kHz_128kbps_eld", BM_EncodeAAC,
                                folderPath, "bbb_2ch_48kHz.pcm", 48000, 128000, AOT_ER_AAC_ELD,
                                MODE_2);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_6ch_16kHz_64kbps_eld", BM_EncodeAAC,
                                folderPath, "bbb_6ch_16kHz.pcm", 16000, 64000, AOT_ER_AAC_ELD,
                                MODE_1_2_2_1);
    benchmark::RegisterBenchmark("BM_EncodeAAC/bbb_6ch_48kHz_128kbps_eld", BM_EncodeAAC,
                                folderPath, "bbb_6ch_48kHz.pcm", 48000, 128000, AOT_ER_AAC_ELD,
                                MODE_1_2_2_1);
}

class CustomCsvReporter : public benchmark::BenchmarkReporter {
   public:
    CustomCsvReporter() : mPrintedHeader(false) {};
    virtual bool ReportContext(const Context& context);
    virtual void ReportRuns(const std::vector<Run>& reports);

   private:
    void PrintRunData(const Run& report);
    bool mPrintedHeader;
    std::vector<std::string> mHeaders = {"File",    "Channels",      "SampleRate",  "BitRate",
                                         "profile", "real_time(ns)", "cpu_time(ns)"};
};

bool CustomCsvReporter::ReportContext(const Context& context /* __unused */) { return true; }

void CustomCsvReporter::ReportRuns(const std::vector<Run>& reports) {
    std::ostream& Out = GetOutputStream();

    if (!mPrintedHeader) {
        // print the header
        for (auto header = mHeaders.begin(); header != mHeaders.end();) {
            Out << *header++;
            if (header != mHeaders.end()) Out << ",";
        }
        Out << "\n";
        mPrintedHeader = true;
    }

    // print results for each run
    for (const auto& run : reports) {
        PrintRunData(run);
    }
}

void CustomCsvReporter::PrintRunData(const Run& run) {
    if (run.skipped) {
        return;
    }
    std::ostream& Out = GetOutputStream();
    Out << run.report_label << ",";
    Out << run.GetAdjustedRealTime() << ",";
    Out << run.GetAdjustedCPUTime() << ",";
    Out << '\n';
}

int main(int argc, char** argv) {
    std::unique_ptr<benchmark::BenchmarkReporter> csvReporter;
    std::string pathArg, inpFolderPath;

    for (int i = 1; i < argc; ++i) {
        // pass --path=/path/to/resourcefolder in command line while running without atest
        // to specify where resources are present
        if (std::string(argv[i]).find("--path") != std::string ::npos) {
            pathArg = argv[i];
            auto separator = pathArg.find('=');
            if (separator != std::string::npos) {
                inpFolderPath = pathArg.substr(separator + 1);
            }
        }
        // pass --benchmark_out=/path/to/.csv in command line to generate csv report
        if (std::string(argv[i]).find("--benchmark_out") != std::string::npos) {
            csvReporter.reset(new CustomCsvReporter);
            break;
        }
    }

    if (inpFolderPath.empty()) {
        inpFolderPath = "/sdcard/test/AacEncBenchmark-1.0";
    }

    FILE* pcmFilePath = fopen(inpFolderPath.c_str(), "r");
    if (!pcmFilePath) {
        std::cerr << "Error: Invalid path provided: " << inpFolderPath << std::endl;
        return -1;
    }
    fclose(pcmFilePath);

    RegisterBenchmarks(inpFolderPath);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks(nullptr, csvReporter.get());
    benchmark::Shutdown();
    return 0;
}
