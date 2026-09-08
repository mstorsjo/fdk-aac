/* ------------------------------------------------------------------
 * Copyright (C) 2026 Martin Storsjo
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

#if defined(_MSC_VER)
#include <getopt.h>
#else
#include <unistd.h>
#endif

#include <stdlib.h>
#include "xHEAACEnc/include/xHEAACEnc.h"
#include "wavreader.h"

static int export_loudness(void *wav, int sample_rate, int channels, IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE loudness_handle, const char *export_loudness_file) {
	int frame_size;
	uint8_t *input_buf;
	float *convert_buf;
	IIS_XHEAACENC_RETURN_CODE ret;
	const unsigned char *loudness_data_buf = NULL;
	unsigned int loudness_data_size = 0;
	FILE *out;

	// Feed audio in 100 ms chunks; this chunk size is arbitrary.
	frame_size = channels * sample_rate * 100 / 1000;
	input_buf = malloc(sizeof(int16_t) * frame_size);
	convert_buf = malloc(sizeof(float) * frame_size);

	while (1) {
		int i, read, num_samples;

		read = wav_read_data(wav, input_buf, sizeof(int16_t) * frame_size);
		if (read <= 0)
			break;

		num_samples = read <= 0 ? 0 : read / sizeof(int16_t);
		for (i = 0; i < num_samples; i++) {
			const uint8_t* in = &input_buf[2*i];
			int16_t sample = in[0] | (in[1] << 8);
			convert_buf[i] = sample * (1 / 32768.0f);
		}

		if ((ret = IIS_xHEAACEnc_Loudness_Measure(loudness_handle, convert_buf, num_samples)) != IIS_XHEAACENC_NO_ERROR) {
			fprintf(stderr, "Measure loudness failed, ret %d\n", ret);
			return 1;
		}
	}

	if ((ret = IIS_xHEAACEnc_Loudness_Export(loudness_handle, &loudness_data_buf, &loudness_data_size)) != IIS_XHEAACENC_NO_ERROR) {
		fprintf(stderr, "Export loudness failed, ret %d\n", ret);
		return 1;
	}
	out = fopen(export_loudness_file, "wb");
	if (!out) {
		perror(export_loudness_file);
		return 1;
	}
	fwrite(loudness_data_buf, 1, loudness_data_size, out);
	fclose(out);
	if ((ret = IIS_xHEAACEnc_Loudness_Delete(loudness_handle)) != IIS_XHEAACENC_NO_ERROR) {
		fprintf(stderr, "Unable to delete loudness, ret %d\n", ret);
		return 1;
	}
	wav_read_close(wav);
	free(input_buf);
	free(convert_buf);
	return 0;
}

static void usage(const char* name) {
	fprintf(stderr, "%s [-r bitrate] [-v vbr] [-l live_loudness] [-e export_loudness] [-i import_loudness] in.wav out.loas\n", name);
}

int main(int argc, char *argv[]) {
	int bitrate = 64000;
	int ch;
	const char *infile, *outfile;
	FILE *out;
	void *wav;
	int format, sample_rate, channels, bits_per_sample;
	int alloc_size;
	uint8_t *input_buf;
	float *convert_buf;
	int vbr = -1;
	float live_loudness = -24;
	const char *export_loudness_file = NULL, *import_loudness_file = NULL;
	IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE config_handle = NULL;
	IIS_XHEAACENC_RETURN_CODE ret;
	IIS_XHEAACENC_CHANNELCONFIG channel_config;
	IIS_XHEAACENC_BITRATEMODE bitrate_mode;
	IIS_XHEAACENC_INSTANCE_HANDLE handle = NULL;
	IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE loudness_handle = NULL;
	IIS_XHEAACENC_LOUDNESS_SETUP loudness_setup;
	int frame_size;

	while ((ch = getopt(argc, argv, "r:v:l:e:i:")) != -1) {
		switch (ch) {
		case 'r':
			bitrate = atoi(optarg);
			break;
		case 'v':
			vbr = atoi(optarg);
			break;
		case 'l':
			live_loudness = atof(optarg);
			break;
		case 'e':
			export_loudness_file = optarg;
			break;
		case 'i':
			import_loudness_file = optarg;
			break;
		case '?':
		default:
			usage(argv[0]);
			return 1;
		}
	}
	if (export_loudness_file) {
		if (argc - optind < 1) {
			usage(argv[0]);
			return 1;
		}
		infile = argv[optind];
	} else {
		if (argc - optind < 2) {
			usage(argv[0]);
			return 1;
		}
		infile = argv[optind];
		outfile = argv[optind + 1];
	}

	wav = wav_read_open(infile);
	if (!wav) {
		fprintf(stderr, "Unable to open wav file %s\n", infile);
		return 1;
	}
	if (!wav_get_header(wav, &format, &channels, &sample_rate, &bits_per_sample, NULL)) {
		fprintf(stderr, "Bad wav file %s\n", infile);
		return 1;
	}
	if (format != 1) {
		fprintf(stderr, "Unsupported WAV format %d\n", format);
		return 1;
	}
	if (bits_per_sample != 16) {
		fprintf(stderr, "Unsupported WAV sample depth %d\n", bits_per_sample);
		return 1;
	}

	switch (channels) {
	case 1: channel_config = IIS_XHEAACENC_CHANNELCONFIG_MONO; break;
	case 2: channel_config = IIS_XHEAACENC_CHANNELCONFIG_STEREO; break;
	default:
		fprintf(stderr, "Unsupported WAV channels %d\n", channels);
		return 1;
	}

	if (export_loudness_file || import_loudness_file) {
		FILE *loudness_in;
		size_t loudness_size, n;
		unsigned char *loudness_buf;

		loudness_setup.sampleRate = sample_rate;
		loudness_setup.channelConfig = channel_config;
		loudness_setup.audioInputLengthSamples = 0;
		loudness_setup.audioInputLengthAvailable = 0;
		if ((ret = IIS_xHEAACEnc_Loudness_Open(&loudness_handle, loudness_setup)) != IIS_XHEAACENC_NO_ERROR) {
			fprintf(stderr, "Unable to create loudness handle, ret %d\n", ret);
			return 1;
		}

		if (export_loudness_file)
			return export_loudness(wav, sample_rate, channels, loudness_handle, export_loudness_file);

		loudness_in = fopen(import_loudness_file, "rb");
		if (!loudness_in) {
			perror(import_loudness_file);
			return 1;
		}
		fseek(loudness_in, 0, SEEK_END);
		loudness_size = ftell(loudness_in);
		fseek(loudness_in, 0, SEEK_SET);

		loudness_buf = malloc(loudness_size);
		n = fread(loudness_buf, 1, loudness_size, loudness_in);
		fclose(loudness_in);

		if (n != loudness_size) {
			fprintf(stderr, "Unable to read whole loudness file\n");
			return 1;
		}

		if ((ret = IIS_xHEAACEnc_Loudness_Import(loudness_handle, loudness_buf, loudness_size)) != IIS_XHEAACENC_NO_ERROR) {
			fprintf(stderr, "Unable to import loudness data, ret %d\n", ret);
			return 1;
		}
		free(loudness_buf);
	} else {
		if (live_loudness < -31.0f || live_loudness > -10.0f) {
			fprintf(stderr, "Loudness out of range, must be between -31.0 and -10.0\n");
			return -1;
		}
	}

	if ((ret = IIS_xHEAACEnc_Config_Open(&config_handle, sample_rate, channel_config)) != IIS_XHEAACENC_NO_ERROR) {
		fprintf(stderr, "Unable to open config, ret %d\n", ret);
		return 1;
	}
	if (vbr >= 0) {
		switch (vbr) {
		case 0: bitrate_mode = IIS_XHEAACENC_BITRATEMODE_AAC_VBR0; break;
		case 1: bitrate_mode = IIS_XHEAACENC_BITRATEMODE_AAC_VBR1; break;
		case 2: bitrate_mode = IIS_XHEAACENC_BITRATEMODE_AAC_VBR2; break;
		case 3: bitrate_mode = IIS_XHEAACENC_BITRATEMODE_AAC_VBR3; break;
		case 4: bitrate_mode = IIS_XHEAACENC_BITRATEMODE_AAC_VBR4; break;
		case 5: bitrate_mode = IIS_XHEAACENC_BITRATEMODE_AAC_VBR5; break;
		case 6: bitrate_mode = IIS_XHEAACENC_BITRATEMODE_AAC_VBR6; break;
		default:
			fprintf(stderr, "Unsupported VBR mode %d (supporting modes 0-6)\n", vbr);
			return 1;
		}
	} else {
		bitrate_mode = IIS_XHEAACENC_BITRATEMODE_CBR;

		if ((ret = IIS_xHEAACEnc_Config_AddParamValueInt(config_handle, IIS_XHEAACENC_PARAMETER_BITRATE, bitrate)) != IIS_XHEAACENC_NO_ERROR) {
			fprintf(stderr, "Unable to set bitrate, ret %d\n", ret);
			return 1;
		}
	}
	if ((ret = IIS_xHEAACEnc_Config_AddParamValueInt(config_handle, IIS_XHEAACENC_PARAMETER_BITRATEMODE, bitrate_mode)) != IIS_XHEAACENC_NO_ERROR) {
		fprintf(stderr, "Unable to set bitrate mode, ret %d\n", ret);
		return 1;
	}
	if ((ret = IIS_xHEAACEnc_Config_AddParamValueInt(config_handle, IIS_XHEAACENC_PARAMETER_AOT, IIS_XHEAACENC_AOT_USAC)) != IIS_XHEAACENC_NO_ERROR) {
		fprintf(stderr, "Unable to set AOT, ret %d\n", ret);
		return 1;
	}
	if ((ret = IIS_xHEAACEnc_Config_AddParamValueInt(config_handle, IIS_XHEAACENC_PARAMETER_TRANSPORTFORMAT, IIS_XHEAACENC_TRANSPORTFORMAT_LATMLOAS)) != IIS_XHEAACENC_NO_ERROR) {
		fprintf(stderr, "Unable to set transport format, ret %d\n", ret);
		return 1;
	}
	if ((ret = IIS_xHEAACEnc_Config_AddParamValueInt(config_handle, IIS_XHEAACENC_PARAMETER_STREAMID, 0)) != IIS_XHEAACENC_NO_ERROR) {
		fprintf(stderr, "Unable to set stream ID, ret %d\n", ret);
		return 1;
	}
	if ((ret = IIS_xHEAACEnc_Config_AddParamValueInt(config_handle, IIS_XHEAACENC_PARAMETER_RAP_OCCURRENCE, IIS_XHEAACENC_RAP_OCCURRENCE_CONSTANT_INTERVAL)) != IIS_XHEAACENC_NO_ERROR) {
		fprintf(stderr, "Unable to set RAP occurrence, ret %d\n", ret);
		return 1;
	}
	if ((ret = IIS_xHEAACEnc_Config_AddParamValueInt(config_handle, IIS_XHEAACENC_PARAMETER_RAP_INTERVAL_MS, 1000)) != IIS_XHEAACENC_NO_ERROR) {
		fprintf(stderr, "Unable to set RAP interval, ret %d\n", ret);
		return 1;
	}
	if (import_loudness_file) {
		if ((ret = IIS_xHEAACEnc_Config_AddParamValuePointer(config_handle, IIS_XHEAACENC_PARAMETER_LOUDNESS_DATA, loudness_handle)) != IIS_XHEAACENC_NO_ERROR) {
			fprintf(stderr, "Unable to set loudness handle, ret %d\n", ret);
			return 1;
		}
	} else {
		if ((ret = IIS_xHEAACEnc_Config_AddParamValueFloat(config_handle, IIS_XHEAACENC_PARAMETER_LIVE_LOUDNESS_LEVEL, live_loudness)) != IIS_XHEAACENC_NO_ERROR) {
			fprintf(stderr, "Unable to set live loudness, ret %d\n", ret);
			return 1;
		}
	}
	if ((ret = IIS_xHEAACEnc_Config_Finalize(config_handle)) >= IIS_XHEAACENC_ERROR_FIRST) {
		fprintf(stderr, "Unable to finalize config, ret %d\n", ret);
		return 1;
	}

	if ((ret = IIS_xHEAACEnc_Open(&handle, config_handle)) != IIS_XHEAACENC_NO_ERROR) {
		fprintf(stderr, "Unable to initialize the encoder, ret %d\n", ret);
		return 1;
	}
	if ((ret = IIS_xHEAACEnc_GetParam(handle, IIS_XHEAACENC_PARAMETER_FRAMESAMPLES, IIS_XHEAACENC_PARAM_INT, &frame_size, sizeof(frame_size))) != IIS_XHEAACENC_NO_ERROR) {
		fprintf(stderr, "Unable to get the frame size, ret %d\n", ret);
		return 1;
	}

	if ((ret = IIS_xHEAACEnc_Config_Delete(config_handle)) != IIS_XHEAACENC_NO_ERROR) {
		fprintf(stderr, "Unable to delete config handle, ret %d\n", ret);
		return 1;
	}

	out = fopen(outfile, "wb");
	if (!out) {
		perror(outfile);
		return 1;
	}

	alloc_size = channels * frame_size;
	input_buf = malloc(sizeof(int16_t) * alloc_size);
	convert_buf = malloc(sizeof(float) * alloc_size);

	while (1) {
		int i, read, num_samples;
		uint8_t outbuf[20480];
		int out_size;
		int samples_next;
		IIS_XHEAACENC_AUINFO au_info;
		IIS_XHEAACENC_ENCODER_STATE state;

		if ((ret = IIS_xHEAACEnc_GetParam(handle, IIS_XHEAACENC_PARAMETER_SAMPLES_NEXT, IIS_XHEAACENC_PARAM_INT, &samples_next, sizeof(samples_next))) != IIS_XHEAACENC_NO_ERROR) {
			fprintf(stderr, "Unable to get the number of required samples, ret %d\n", ret);
			return 1;
		}
		if (samples_next > alloc_size) {
			alloc_size = samples_next;
			input_buf = realloc(input_buf, sizeof(int16_t) * alloc_size);
			convert_buf = realloc(convert_buf, sizeof(float) * alloc_size);
		}

		read = wav_read_data(wav, input_buf, sizeof(int16_t) * samples_next);
		num_samples = read <= 0 ? 0 : read / sizeof(int16_t);
		for (i = 0; i < num_samples; i++) {
			const uint8_t* in = &input_buf[2*i];
			int16_t sample = in[0] | (in[1] << 8);
			convert_buf[i] = sample * (1 / 32768.0f);
		}

		if ((ret = IIS_xHEAACEnc_EncodeFrame(handle, convert_buf, num_samples, outbuf, &out_size, sizeof(outbuf), &au_info)) != IIS_XHEAACENC_NO_ERROR) {
			fprintf(stderr, "Encode frame failed, ret %d\n", ret);
			return 1;
		}

		if ((ret = IIS_xHEAACEnc_GetEncoderState(handle, &state)) != IIS_XHEAACENC_NO_ERROR) {
			fprintf(stderr, "Unable to get encoder state, ret %d\n", ret);
			return 1;
		}

		// If no output was generated, and the encoder isn't yet ready to close,
		// get the encoder into flushing mode
		while (out_size <= 0 && num_samples <= 0 && state != IIS_XHEAACENC_ENCODER_STATE_READY_TO_CLOSE) {
			while (state != IIS_XHEAACENC_ENCODER_STATE_FLUSHING) {
				if ((ret = IIS_xHEAACEnc_EncodeFrame(handle, NULL, 0, outbuf, &out_size, sizeof(outbuf), &au_info)) != IIS_XHEAACENC_NO_ERROR) {
					fprintf(stderr, "Flush encoder failed, ret %d\n", ret);
					return 1;
				}

				if ((ret = IIS_xHEAACEnc_GetEncoderState(handle, &state)) != IIS_XHEAACENC_NO_ERROR) {
					fprintf(stderr, "Unable to get encoder state, ret %d\n", ret);
					return 1;
				}
			}
		}

		if (out_size <= 0) {
			if (num_samples <= 0)
				break;
			continue;
		}
		fwrite(outbuf, 1, out_size, out);
	}
	free(input_buf);
	free(convert_buf);
	fclose(out);
	wav_read_close(wav);
	if (loudness_handle) {
		if ((ret = IIS_xHEAACEnc_Loudness_Delete(loudness_handle)) != IIS_XHEAACENC_NO_ERROR) {
			fprintf(stderr, "Unable to delete loudness, ret %d\n", ret);
			return 1;
		}
	}
	if ((ret = IIS_xHEAACEnc_Delete(handle)) != IIS_XHEAACENC_NO_ERROR) {
		fprintf(stderr, "Unable to delete encoder, ret %d\n", ret);
		return 1;
	}

	return 0;
}

