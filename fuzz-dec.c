#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "aacdecoder_lib.h"

static const uint8_t tag[] = "FUZZ-TAG";

static void test(const uint8_t *ptr, int size) {
	const uint8_t *base = ptr;
	const uint8_t *end = ptr + size;
	int tagsize = sizeof(tag) - 1;

	int decoder_buffer_size = 2048 * 2 * 8;
	uint8_t *decoder_buffer = malloc(decoder_buffer_size);

	HANDLE_AACDECODER decoder = aacDecoder_Open(TT_MP4_ADTS, 1);
	aacDecoder_SetParam(decoder, AAC_CONCEAL_METHOD, 1);
	aacDecoder_SetParam(decoder, AAC_PCM_LIMITER_ENABLE, 0);

	while (1) {
		const uint8_t* start = ptr;
		UINT valid, buffer_size;
		AAC_DECODER_ERROR err;

		while (ptr + tagsize < end) {
			if (!memcmp(ptr, tag, tagsize))
				break;
			ptr++;
		}
		if (ptr + tagsize > end)
			ptr = end;

		do {
			valid = buffer_size = ptr - start;
			err = aacDecoder_Fill(decoder, (UCHAR**) &start, &buffer_size, &valid);
			start += buffer_size - valid;
			if (err == AAC_DEC_NOT_ENOUGH_BITS)
				continue;
			if (err == AAC_DEC_OK)
				err = aacDecoder_DecodeFrame(decoder, (INT_PCM *) decoder_buffer, decoder_buffer_size / sizeof(INT_PCM), 0);
			if (err != AAC_DEC_NOT_ENOUGH_BITS && err != AAC_DEC_OK)
				break;
		} while (start < ptr);

		aacDecoder_GetStreamInfo(decoder);

		if (ptr + tagsize <= end) {
			ptr += tagsize;
		} else {
			break;
		}
	}
	while (1) {
		const uint8_t* start = NULL;
		UINT buffer_size = 0, valid = 0;
		AAC_DECODER_ERROR err;
		err = aacDecoder_Fill(decoder, (UCHAR**) &start, &buffer_size, &valid);
		if (err == AAC_DEC_OK)
			err = aacDecoder_DecodeFrame(decoder, (INT_PCM *) decoder_buffer, decoder_buffer_size / sizeof(INT_PCM), 0);
		if (err != AAC_DEC_OK)
			break;
	}
	free(decoder_buffer);
	aacDecoder_Close(decoder);
}

int main(int argc, char *argv[]) {
	FILE *f;
	int size;
	uint8_t *buf;

	if (argc < 2) {
		printf("%s file\n", argv[0]);
		return 0;
	}
	f = fopen(argv[1], "rb");
	if (!f) {
		perror(argv[1]);
		return 1;
	}

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);

	buf = malloc(size);
	if (fread(buf, 1, size, f) == size)
		test(buf, size);
	free(buf);

	fclose(f);
	return 0;
}
