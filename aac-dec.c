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

int main(int argc, char *argv[]) {
	const char *infile, *outfile;
	FILE *in;
	void *wav = NULL;
	int output_size;
	uint8_t *output_buf;
	int16_t *decode_buf;
	HANDLE_AACDECODER handle;
	int frame_size = 0;
	if (argc < 3) {
		fprintf(stderr, "%s in.aac out.wav\n", argv[0]);
		return 1;
	}
	infile = argv[1];
	outfile = argv[2];

	handle = aacDecoder_Open(TT_MP4_ADTS, 1);
	in = fopen(infile, "rb");
	if (!in) {
		perror(infile);
		return 1;
	}

	output_size = 8*2*2048;
	output_buf = (uint8_t*) malloc(output_size);
	decode_buf = (int16_t*) malloc(output_size);

	while (1) {
		uint8_t packet[10240], *ptr = packet;
		int n, i;
		UINT valid, packet_size;
		AAC_DECODER_ERROR err;
		n = fread(packet, 1, 150, in);
		if (n <= 0)
			break;
		packet_size = n;
		valid = packet_size;
		err = aacDecoder_Fill(handle, &ptr, &packet_size, &valid);
		if (err != AAC_DEC_OK) {
			fprintf(stderr, "Fill failed: %x\n", err);
			break;
		}
		if (valid != 0) {
			fprintf(stderr, "Unable to feed all %d input bytes, %d bytes left\n", n, valid);
		}
		while (1) {
			err = aacDecoder_DecodeFrame(handle, decode_buf, output_size / sizeof(INT_PCM), 0);
			if (err == AAC_DEC_NOT_ENOUGH_BITS)
				break;
			if (err != AAC_DEC_OK) {
				fprintf(stderr, "Decode failed: %x\n", err);
				break;
			}
			if (!wav) {
				CStreamInfo *info = aacDecoder_GetStreamInfo(handle);
				if (!info || info->sampleRate <= 0) {
					fprintf(stderr, "No stream info\n");
					goto end;
				}
				frame_size = info->frameSize * info->numChannels;
				// Note, this probably doesn't return channels > 2 in the right order for wav
				wav = wav_write_open(outfile, info->sampleRate, 16, info->numChannels);
				if (!wav) {
					perror(outfile);
					goto end;
				}
			}
			for (i = 0; i < frame_size; i++) {
				uint8_t* out = &output_buf[2*i];
				out[0] = decode_buf[i] & 0xff;
				out[1] = decode_buf[i] >> 8;
			}
			wav_write_data(wav, output_buf, 2*frame_size);
		}
	}
end:
	free(output_buf);
	free(decode_buf);
	fclose(in);
	if (wav)
		wav_write_close(wav);
	aacDecoder_Close(handle);
	return 0;
}

