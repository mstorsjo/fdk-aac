#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "aacdecoder_lib.h"
#ifdef HAVE_LIBAVCODEC
#include <libavcodec/avcodec.h>
#include <libavutil/intreadwrite.h>
#endif

static const uint8_t tag[] = "FUZZ-TAG";

static void test(const uint8_t *ptr, int size) {
	const uint8_t *base = ptr;
	const uint8_t *end = ptr + size;
	int tagsize = sizeof(tag) - 1;

	int decoder_buffer_size = 2048 * 2 * 8;
	uint8_t *decoder_buffer = malloc(decoder_buffer_size);

	int extradata_size = 0;
	uint8_t *extradata = NULL;
#ifdef HAVE_LIBAVCODEC
	AVCodecParserContext *parser = NULL;
	AVCodecContext *avctx = NULL;
	if (size > 1024) {
		const uint8_t* params = ptr + size - 1024;
		size -= 1024;
		params += 20; // width, height, bit_rate, bits_per_coded_sample
		uint8_t flags = *params++;
		if (flags & 1) {
			parser = av_parser_init(AV_CODEC_ID_AAC);
			avctx = avcodec_alloc_context3(NULL);
		}
		extradata_size = AV_RL32(params);
		params += 4;
		if (extradata_size < size) {
			extradata = malloc(extradata_size);
			size -= extradata_size;
			memcpy(extradata, ptr + size, extradata_size);
		}
	}
#endif

	HANDLE_AACDECODER decoder = aacDecoder_Open(extradata_size ? TT_MP4_RAW : TT_MP4_ADTS, 1);
	aacDecoder_SetParam(decoder, AAC_CONCEAL_METHOD, 1);
	aacDecoder_SetParam(decoder, AAC_PCM_LIMITER_ENABLE, 0);

	if (extradata_size) {
		aacDecoder_ConfigRaw(decoder, &extradata, &extradata_size);
	}

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


		while (start < ptr) {
			const uint8_t *input = start;
			const uint8_t *inputend = ptr;
#ifdef HAVE_LIBAVCODEC
			if (parser) {
				int parse_size;
				int ret = av_parser_parse2(parser, avctx, (uint8_t**) &input, &parse_size, start, ptr - start, 0, 0, 0);
				start += ret;
				inputend = input + parse_size;
			} else
#endif
				start = ptr;

			while (input && input < inputend) {
				valid = buffer_size = inputend - input;
				err = aacDecoder_Fill(decoder, (UCHAR**) &input, &buffer_size, &valid);
				input += buffer_size - valid;
				if (err == AAC_DEC_NOT_ENOUGH_BITS)
					continue;
				if (err == AAC_DEC_OK)
					err = aacDecoder_DecodeFrame(decoder, (INT_PCM *) decoder_buffer, decoder_buffer_size / sizeof(INT_PCM), 0);
				if (err != AAC_DEC_NOT_ENOUGH_BITS && err != AAC_DEC_OK)
					break;
			}

			aacDecoder_GetStreamInfo(decoder);
		}

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
	free(extradata);
#ifdef HAVE_LIBAVCODEC
	if (parser) {
		av_parser_close(parser);
		avcodec_free_context(&avctx);
	}
#endif
}

int main(int argc, char *argv[]) {
	FILE *f;
	int size;
	uint8_t *buf;

#ifdef HAVE_LIBAVCODEC
	avcodec_register_all();
#endif

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
