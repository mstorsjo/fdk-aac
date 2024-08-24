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

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "aacdecoder_lib.h"

class AACDecoder {
   private:
    HANDLE_AACDECODER mAACDecoder;
    CStreamInfo* mStreamInfo;

   public:
    AACDecoder() : mAACDecoder(nullptr), mStreamInfo(nullptr) {}

    bool initialize() {
        mAACDecoder = aacDecoder_Open(TT_MP4_RAW, 1);
        if (!mAACDecoder) {
            ALOGE("Failed to initialize AAC decoder");
            return false;
        }

        mStreamInfo = aacDecoder_GetStreamInfo(mAACDecoder);
        if (!mStreamInfo) {
            ALOGE("Failed to get stream info after initialization");
            return false;
        }
        return true;
    }

    ~AACDecoder() {
        if (mAACDecoder) {
            aacDecoder_Close(mAACDecoder);
        }
    }

    int getChannels() const { return mStreamInfo ? mStreamInfo->numChannels : 0; }
    int getFrameSize() const { return mStreamInfo ? mStreamInfo->frameSize : 0; }
    int getSampleRate() const { return mStreamInfo ? mStreamInfo->sampleRate : 0; }

    bool decode(const std::vector<std::pair<std::vector<uint8_t>, int>>& inputBuffers) {
        for (const auto& [buffer, flag] : inputBuffers) {
            std::vector<INT_PCM> frameOutput;
            if (flag == 2) {
                if (!configureDecoder(buffer)) {
                    return false;
                }
            } else {
                if (!decodeFrame(buffer, frameOutput)) {
                    return false;
                }
            }
        }
        return true;
    }

   private:
    bool configureDecoder(const std::vector<uint8_t>& configBuffer) {
        UINT bytesRead = configBuffer.size();
        UCHAR* configData = const_cast<UCHAR*>(configBuffer.data());
        UCHAR* configArray[1] = {configData};

        AAC_DECODER_ERROR err = aacDecoder_ConfigRaw(mAACDecoder, configArray, &bytesRead);
        if (err != AAC_DEC_OK) {
            ALOGE("Failed to configure decoder: error %d", err);
            return false;
        }
        return true;
    }

    bool decodeFrame(const std::vector<uint8_t>& inputBuffer, std::vector<INT_PCM>& outputBuffer) {
        constexpr size_t kOutputBufferSize = 10240;
        UINT bytesRead = inputBuffer.size();
        UINT validBytes = bytesRead;
        UCHAR* inputPtr = const_cast<UCHAR*>(inputBuffer.data());
        UCHAR* bufferArray[1] = {inputPtr};

        AAC_DECODER_ERROR err = aacDecoder_Fill(mAACDecoder, bufferArray, &bytesRead, &validBytes);
        if (err != AAC_DEC_OK) {
            ALOGE("Failed to fill decoder buffer: error %d", err);
            return false;
        }

        outputBuffer.resize(kOutputBufferSize);  // Ensure buffer is large enough
        err = aacDecoder_DecodeFrame(mAACDecoder, outputBuffer.data(), outputBuffer.size(), 0);
        if (err != AAC_DEC_OK) {
            ALOGE("Failed to decode frame: error %d", err);
            return false;
        }

        outputBuffer.resize(mStreamInfo->numChannels * mStreamInfo->frameSize);
        return true;
    }
};

std::vector<std::pair<std::vector<uint8_t>, int>> readInputFiles(const std::string& folderPath,
                                                                 const std::string& bitstreamFile,
                                                                 const std::string& infoFile) {
    std::string fullBitstreamPath = folderPath + "/" + bitstreamFile;
    std::string fullInfoPath = folderPath + "/" + infoFile;
    std::vector<std::pair<std::vector<uint8_t>, int>> inputBuffers;

    FILE* bitStreamFilePtr = fopen(fullBitstreamPath.c_str(), "rb");
    if (!bitStreamFilePtr) {
        ALOGE("Failed to open bitstream file %s", fullBitstreamPath.c_str());
        return inputBuffers;
    }

    FILE* infoFilePtr = fopen(fullInfoPath.c_str(), "r");
    if (!infoFilePtr) {
        ALOGE("Failed to open info file %s", fullInfoPath.c_str());
        return inputBuffers;
    }

    int bufferSize, flag;
    long pts;

    while (fscanf(infoFilePtr, "%d %d %ld", &bufferSize, &flag, &pts) == 3) {
        std::vector<uint8_t> buffer(bufferSize);
        size_t bytesRead = fread(buffer.data(), 1, bufferSize, bitStreamFilePtr);
        if (bytesRead != bufferSize) {
            ALOGE("Failed to read input data");
            return std::vector<std::pair<std::vector<uint8_t>, int>>();
        }
        inputBuffers.emplace_back(std::move(buffer), flag);
    }

    fclose(bitStreamFilePtr);
    fclose(infoFilePtr);
    return inputBuffers;
}

static void BM_DecodeAAC(benchmark::State& state, const std::string& inpFolderPath,
                         const std::string& bitstreamFile, const std::string& infoFile) {
    auto inputBuffers = readInputFiles(inpFolderPath, bitstreamFile, infoFile);
    if(inputBuffers.empty()) {
        state.SkipWithError("Failed to read input data completely");
    }
    AACDecoder decoder;

    if (!decoder.initialize()) {
        state.SkipWithError("Unable to initialize decoder");
    }

    for (auto _ : state) {
        if(!decoder.decode(inputBuffers)) {
            state.SkipWithError("Unable to decode the Stream");
        }
    }

    state.SetLabel(bitstreamFile + ", " + std::to_string(decoder.getChannels()) + ", "
                   + std::to_string(decoder.getSampleRate()) + ", "
                   + std::to_string(decoder.getFrameSize()));
}

// Function to register benchmarks
void RegisterBenchmarks(const std::string& folderPath) {
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_1ch_8kHz_64kbps_lc", BM_DecodeAAC,
                                 folderPath, "bbb_1ch_8kHz_64kbps_lc.bin",
                                 "bbb_1ch_8kHz_64kbps_lc.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_1ch_48kHz_128kbps_lc", BM_DecodeAAC,
                                 folderPath, "bbb_1ch_48kHz_128kbps_lc.bin",
                                 "bbb_1ch_48kHz_128kbps_lc.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_2ch_8kHz_64kbps_lc", BM_DecodeAAC,
                                 folderPath, "bbb_2ch_8kHz_64kbps_lc.bin",
                                 "bbb_2ch_8kHz_64kbps_lc.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_2ch_48kHz_128kbps_lc", BM_DecodeAAC,
                                 folderPath, "bbb_2ch_48kHz_128kbps_lc.bin",
                                 "bbb_2ch_48kHz_128kbps_lc.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_6ch_8kHz_64kbps_lc", BM_DecodeAAC,
                                 folderPath, "bbb_6ch_8kHz_64kbps_lc.bin",
                                 "bbb_6ch_8kHz_64kbps_lc.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_6ch_48kHz_128kbps_lc", BM_DecodeAAC,
                                 folderPath, "bbb_6ch_48kHz_128kbps_lc.bin",
                                 "bbb_6ch_48kHz_128kbps_lc.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_1ch_16kHz_64kbps_he", BM_DecodeAAC,
                                 folderPath, "bbb_1ch_16kHz_64kbps_he.bin",
                                 "bbb_1ch_16kHz_64kbps_he.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_1ch_48kHz_128kbps_he", BM_DecodeAAC,
                                 folderPath, "bbb_1ch_48kHz_128kbps_he.bin",
                                 "bbb_1ch_48kHz_128kbps_he.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_2ch_16kHz_64kbps_he", BM_DecodeAAC,
                                 folderPath, "bbb_2ch_16kHz_64kbps_he.bin",
                                 "bbb_2ch_16kHz_64kbps_he.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_2ch_48kHz_128kbps_he", BM_DecodeAAC,
                                 folderPath, "bbb_2ch_48kHz_128kbps_he.bin",
                                 "bbb_2ch_48kHz_128kbps_he.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_2ch_16kHz_64kbps_hev2", BM_DecodeAAC,
                                 folderPath, "bbb_2ch_16kHz_64kbps_hev2.bin",
                                 "bbb_2ch_16kHz_64kbps_hev2.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_2ch_48kHz_128kbps_hev2", BM_DecodeAAC,
                                 folderPath, "bbb_2ch_48kHz_128kbps_hev2.bin",
                                 "bbb_2ch_48kHz_128kbps_hev2.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_1ch_48kHz_128kbps_ld", BM_DecodeAAC,
                                 folderPath, "bbb_1ch_48kHz_128kbps_ld.bin",
                                 "bbb_1ch_48kHz_128kbps_ld.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_2ch_48kHz_128kbps_ld", BM_DecodeAAC,
                                 folderPath, "bbb_2ch_48kHz_128kbps_ld.bin",
                                 "bbb_2ch_48kHz_128kbps_ld.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_6ch_48kHz_128kbps_ld", BM_DecodeAAC,
                                 folderPath, "bbb_6ch_48kHz_128kbps_ld.bin",
                                 "bbb_6ch_48kHz_128kbps_ld.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_1ch_16kHz_64kbps_eld", BM_DecodeAAC,
                                 folderPath, "bbb_1ch_16kHz_64kbps_eld.bin",
                                 "bbb_1ch_16kHz_64kbps_eld.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_1ch_48kHz_128kbps_eld", BM_DecodeAAC,
                                 folderPath, "bbb_1ch_48kHz_128kbps_eld.bin",
                                 "bbb_1ch_48kHz_128kbps_eld.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_2ch_16kHz_64kbps_eld", BM_DecodeAAC,
                                 folderPath, "bbb_2ch_16kHz_64kbps_eld.bin",
                                 "bbb_2ch_16kHz_64kbps_eld.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_2ch_48kHz_128kbps_eld", BM_DecodeAAC,
                                 folderPath, "bbb_2ch_48kHz_128kbps_eld.bin",
                                 "bbb_2ch_48kHz_128kbps_eld.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_6ch_16kHz_64kbps_eld", BM_DecodeAAC,
                                 folderPath, "bbb_6ch_16kHz_64kbps_eld.bin",
                                 "bbb_6ch_16kHz_64kbps_eld.info");
    benchmark::RegisterBenchmark("BM_DecodeAAC/bbb_6ch_48kHz_128kbps_eld", BM_DecodeAAC,
                                 folderPath, "bbb_6ch_48kHz_128kbps_eld.bin",
                                 "bbb_6ch_48kHz_128kbps_eld.info");
}

class CustomCsvReporter : public benchmark::BenchmarkReporter {
   public:
    CustomCsvReporter() : mPrintedHeader(false) {}
    virtual bool ReportContext(const Context& context);
    virtual void ReportRuns(const std::vector<Run>& reports);

   private:
    void PrintRunData(const Run& report);
    bool mPrintedHeader;
    std::vector<std::string> mHeaders = {"File",      "Channels",      "SampleRate",
                                         "FrameSize", "real_time(ns)", "cpu_time(ns)"};
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
        inpFolderPath = "/sdcard/test/AacDecBenchmark-1.0";
    }
    RegisterBenchmarks(inpFolderPath);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks(nullptr, csvReporter.get());
    benchmark::Shutdown();
    return 0;
}
