/* ------------------------------------------------------------------
 * Copyright (C) 2013 Martin Storsjo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include "libAACdec/include/aacdecoder_lib.h"
#include "wavwriter.h"
#include <libavformat/avformat.h>

#if LIBAVCODEC_VERSION_MAJOR < 55
#define AV_CODEC_ID_AAC CODEC_ID_AAC
#endif

int main(int argc, char *argv[]) {
	const char *infile, *outfile;
	AVFormatContext *in = NULL;
	AVStream *st = NULL;
	void *wav = NULL;
	int output_size, ret, i;
	uint8_t *output_buf;
	int16_t *decode_buf;
	HANDLE_AACDECODER handle;
	AAC_DECODER_ERROR err;
	int frame_size = 0;
	if (argc < 3) {
		fprintf(stderr, "%s in.m4a out.wav\n", argv[0]);
		return 1;
	}
	infile = argv[1];
	outfile = argv[2];

	av_register_all();
	avformat_network_init();
	ret = avformat_open_input(&in, infile, NULL, NULL);
	if (ret < 0) {
		char buf[100];
		av_strerror(ret, buf, sizeof(buf));
		fprintf(stderr, "%s: %s\n", infile, buf);
		return 1;
	}
	for (i = 0; i < in->nb_streams && !st; i++) {
		if (in->streams[i]->codec->codec_id == AV_CODEC_ID_AAC)
			st = in->streams[i];
	}
	if (!st) {
		fprintf(stderr, "No AAC stream found\n");
		return 1;
	}
	if (!st->codec->extradata_size) {
		fprintf(stderr, "No AAC ASC found\n");
		return 1;
	}
	handle = aacDecoder_Open(TT_MP4_RAW, 1);
	err = aacDecoder_ConfigRaw(handle, &st->codec->extradata, &st->codec->extradata_size);
	if (err != AAC_DEC_OK) {
		fprintf(stderr, "Unable to decode the ASC\n");
		return 1;
	}

	output_size = 8*2*1024;
	output_buf = (uint8_t*) malloc(output_size);
	decode_buf = (int16_t*) malloc(output_size);

	while (1) {
		int i;
		UINT valid;
		AVPacket pkt = { 0 };
		int ret = av_read_frame(in, &pkt);
		if (ret < 0) {
			if (ret == AVERROR(EAGAIN))
				continue;
			break;
		}
		if (pkt.stream_index != st->index) {
			av_free_packet(&pkt);
			continue;
		}

		valid = pkt.size;
		err = aacDecoder_Fill(handle, &pkt.data, &pkt.size, &valid);
		if (err != AAC_DEC_OK) {
			fprintf(stderr, "Fill failed: %x\n", err);
			break;
		}
		err = aacDecoder_DecodeFrame(handle, decode_buf, output_size, 0);
		av_free_packet(&pkt);
		if (err == AAC_DEC_NOT_ENOUGH_BITS)
			continue;
		if (err != AAC_DEC_OK) {
			fprintf(stderr, "Decode failed: %x\n", err);
			continue;
		}
		if (!wav) {
			CStreamInfo *info = aacDecoder_GetStreamInfo(handle);
			if (!info || info->sampleRate <= 0) {
				fprintf(stderr, "No stream info\n");
				break;
			}
			frame_size = info->frameSize * info->numChannels;
			// Note, this probably doesn't return channels > 2 in the right order for wav
			wav = wav_write_open(outfile, info->sampleRate, 16, info->numChannels);
			if (!wav) {
				perror(outfile);
				break;
			}
		}
		for (i = 0; i < frame_size; i++) {
			uint8_t* out = &output_buf[2*i];
			out[0] = decode_buf[i] & 0xff;
			out[1] = decode_buf[i] >> 8;
		}
		wav_write_data(wav, output_buf, 2*frame_size);
	}
	free(output_buf);
	free(decode_buf);
	avformat_close_input(&in);
	if (wav)
		wav_write_close(wav);
	aacDecoder_Close(handle);
	return 0;
}

