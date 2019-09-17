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
	int output_size, ret;
	uint8_t *output_buf;
	INT_PCM *decode_buf;
	HANDLE_AACDECODER handle;
	AAC_DECODER_ERROR err;
	unsigned frame_size = 0, i;
	UINT input_length;
	int status = 0;
	if (argc < 3) {
		fprintf(stderr, "%s in.m4a out.wav\n", argv[0]);
		return 1;
	}
	infile = argv[1];
	outfile = argv[2];

#if LIBAVFORMAT_VERSION_MICRO < 100 || LIBAVFORMAT_VERSION_MAJOR < 58 || LIBAVFORMAT_VERSION_MINOR < 9
	av_register_all();
	avformat_network_init();
#endif
	ret = avformat_open_input(&in, infile, NULL, NULL);
#ifdef AVFMT_FLAG_KEEP_SIDE_DATA
	in->flags |= AVFMT_FLAG_KEEP_SIDE_DATA;
#endif
	if (ret < 0) {
		char buf[100];
		av_strerror(ret, buf, sizeof(buf));
		fprintf(stderr, "%s: %s\n", infile, buf);
		return 1;
	}
	for (i = 0; i < in->nb_streams && !st; i++) {
		if (in->streams[i]->codecpar->codec_id == AV_CODEC_ID_AAC)
			st = in->streams[i];
	}
	if (!st) {
		fprintf(stderr, "No AAC stream found\n");
		return 1;
	}
	if (!st->codecpar->extradata_size) {
		fprintf(stderr, "No AAC ASC found\n");
		return 1;
	}
	handle = aacDecoder_Open(TT_MP4_RAW, 1);
	input_length = st->codecpar->extradata_size;
	err = aacDecoder_ConfigRaw(handle, &st->codecpar->extradata, &input_length);
	if (err != AAC_DEC_OK) {
		fprintf(stderr, "Unable to decode the ASC\n");
		return 1;
	}

	output_size = 8*sizeof(INT_PCM)*2048;
	output_buf = (uint8_t*) malloc(output_size);
	decode_buf = (INT_PCM*) malloc(output_size);

	while (1) {
		UINT valid;
		AVPacket pkt = { 0 };
		int ret = av_read_frame(in, &pkt);
		if (ret < 0) {
			if (ret == AVERROR(EAGAIN))
				continue;
			break;
		}
		if (pkt.stream_index != st->index) {
			av_packet_unref(&pkt);
			continue;
		}

		valid = pkt.size;
		input_length = pkt.size;
		err = aacDecoder_Fill(handle, &pkt.data, &input_length, &valid);
		if (err != AAC_DEC_OK) {
			fprintf(stderr, "Fill failed: %x\n", err);
			status = 1;
			break;
		}
		err = aacDecoder_DecodeFrame(handle, decode_buf, output_size / sizeof(INT_PCM), 0);
		av_packet_unref(&pkt);
		if (err == AAC_DEC_NOT_ENOUGH_BITS)
			continue;
		if (err != AAC_DEC_OK) {
			fprintf(stderr, "Decode failed: %x\n", err);
			status = 1;
			break;
		}
		if (!wav) {
			CStreamInfo *info = aacDecoder_GetStreamInfo(handle);
			if (!info || info->sampleRate <= 0) {
				fprintf(stderr, "No stream info\n");
				status = 1;
				break;
			}
			frame_size = info->frameSize * info->numChannels;
			// Note, this probably doesn't return channels > 2 in the right order for wav
			wav = wav_write_open(outfile, info->sampleRate, sizeof(INT_PCM)*8, info->numChannels);
			if (!wav) {
				perror(outfile);
				status = 1;
				break;
			}
		}
		for (i = 0; i < frame_size; i++) {
			uint8_t* out = &output_buf[sizeof(INT_PCM)*i];
			unsigned j;
			for (j = 0; j < sizeof(INT_PCM); j++)
				out[j] = (uint8_t)(decode_buf[i] >> (8*j));
		}
		wav_write_data(wav, output_buf, sizeof(INT_PCM)*frame_size);
	}
	free(output_buf);
	free(decode_buf);
	avformat_close_input(&in);
	if (wav)
		wav_write_close(wav);
	aacDecoder_Close(handle);
	return status;
}

