# Extended High Efficiency AAC encoder in AOSP

This is the Extended High Efficiency AAC encoder integrated into AOSP framework based on the Android version android-16.0.0_r2.
The new MediaCodec C2 component (c2.android.xheaac.encoder) and its mime type MEDIA_MIMETYPE_AUDIO_AAC_XHE link to the new Extended High Efficiency AAC encoder code from Fraunhofer; registered as libcodec2_soft_xheaacenc.

The Extended High Efficiency AAC codec is part of the fourth codec generation in the AAC codec family. It provides improved quality at stereo bit rates of 16 to 320+ kbit/s and adaptive streaming.
It also introduces mandatory loudness and dynamic range control with MPEG-D DRC for consistent playback loudness of live and file-based content.

The addition of the new codec introduces the following AOSP media API keys for controlling its features:
* `KEY_AAC_LIVE_AUTOMATIC_LOUDNESS_MODE`
* `KEY_AAC_LOUDNESS_DATA`
* `KEY_AAC_STREAM_ID`
* `KEY_AAC_RAP_INTERVALS`
* `KEY_AAC_CONSTANT_RAP_INTERVAL`

And the following MediaCodec configuration flag:
 * `CONFIGURE_FLAG_MEASURE_LOUDNESS` (for the first pass of xHE encoding)
 
Detailed documentation can be found below. 


## KEY_AAC_LIVE_AUTOMATIC_LOUDNESS_MODE

An optional key for selection of a live automatic loudness mode for one-pass encoding with Extended High Efficiency AAC (MediaCodecInfo.CodecProfileLevel.AACObjectXHE).
The associated value is an integer and can be set to following values:


| Live automatic loudness mode | Value |
| ---------------------------- |:-----:|
| general                      | 0     |
| aggressive                   | 1     | 

For user-generated content, the aggressive live automatic loudness mode should be used. 
For professionally mixed content with known properties, the general live automatic loudness mode should be used. This mode preserves the content dynamics to the maximum possible extent.
The default mode is general.
This is an encoder-only key.

```
public static final String KEY_AAC_LIVE_AUTOMATIC_LOUDNESS_MODE = "aac-live-automatic-loudness-mode";
```

## KEY_AAC_LOUDNESS_DATA

An optional key used for two-pass encoding with Extended High Efficiency AAC (`MediaCodecInfo.CodecProfileLevel.AACObjectXHE`). 

This key is used to hand over the loudness related metadata generated in the first Extended High Efficiency AAC encoding pass, to the second Extended High Efficiency AAC encoding pass. 

This data buffer varies in size and scales with the duration of the audio input to the first encoding pass according to this equation: 44 bytes + 40 bytes per second of audio input.

The associated value is a ByteBuffer in native byte order of the underlying platform according to following format:
| Parameter                                 | Description |
| -------------                             | ------------|
| `measureAnchorLoudness` <br/> `int(32)`       | Shall be set to a value of 0. |
| `loudness` <br/> `float(32)`                  | The long-term loudness level in LKFS according to ITU-R BS.1770. |
| `samplePeak` <br/>`float(32)`                  | The maximum sample amplitude in dBFS. |
| `loudnessRange` <br/>`float(32)`               | The Loudness Range in LU according to EBU Tech 3342. |
| `nChannels` <br/>`int(32)`                     | The number of channels. |
| `audioBlockLength` <br/>`int(32)`              | The audio block length in samples corresponding to one entry in the `instantaneousLoudness` array. |
| `sampleRate` <br/>`int(32)`                    | The sample rate in samples per second. |
| `nSamplesInLoudnessMeter` <br/>`int(32)`       | The number of samples not represented in the `instantaneousLoudness` array. <br/>`item length (in samples per channel) = audioBlocksProcessed * audioBlockLength + (nSamplesInLoudnessMeter / nChannels)`
| `audioBlocksProcessed` <br/>`int(32)`          | The number of audio blocks represented in the `instantaneousLoudness` array. |
| `minRequiredSamplesProcessed` <br/>`int(32)`   | A flag which indicates that the minimum number of samples has been processed. <br/>If this flag is set to 0, the ByteBuffer is invalid. |
| `instantaneousLoudness` <br/>`audioBlocksProcessed` elements of type `float(32)`           | An array of instantaneous loudness values (in LKFS) of length `audioBlocksProcessed`.|
| `checkSum` <br/>4 elements of type `unsigned char(8)`           | Hash value for verification of the ByteBuffer integrity.| 

```
public static final String KEY_AAC_LOUDNESS_DATA = "aac-loudness-data";
```

## KEY_AAC_STREAM_ID

An optional key for setting a stream ID for encoding with Extended High Efficiency AAC (`MediaCodecInfo.CodecProfileLevel.ACObjectXHE`). 

The associated value is an unsigned integer as the stream ID. 

When it is not specified, a random number generator produces an integer and writes it to the bitstream.

```
public static final String KEY_AAC_STREAM_ID = "aac-stream-id";
```
 
 ## KEY_AAC_RAP_INTERVALS

An optional key for specifying a list of Random Access Point (RAP) positions in samples in the Extended High Efficiency AAC bitstream (`MediaCodecInfo.CodecProfileLevel.AACObjectXHE`). 

The associated value is a buffer of unsigned 32-bit integers containing the intervals in milliseconds. Each RAP position value specified by the user will be readjusted internally to the next rounded multiple of 4096 samples. 

The first encoded frame will always be a RAP, thus the first interval in the associated list specifies the interval between the first and second RAP. 

If the list of RAP intervals specified does not cover the full audio stream, it will wrap to the start and thus be repeated until the end of the encoded audio stream. 

For example, specifying a RAP interval list of [896, 2432] milliseconds at 48kHz and with 1024-sample frames will result in the following (1-based) encoded frames to be RAPs: 1, 43, 157, 199, 313,… 

 **Note:** The associated value is a ByteBuffer in native order.

 **Note:** Only one of `KEY_AAC_RAP_INTERVALS` and `KEY_AAC_CONSTANT_RAP_INTERVAL` may be set for a single encode. 

 This key is only used during encoding. 
 
```
public static final String KEY_AAC_RAP_INTERVALS = "aac-rap-intervals";
```
 
 ## KEY_AAC_CONSTANT_RAP_INTERVAL

 An optional key for selection of a constant Random Access Point (RAP) interval in milliseconds for writing RAPs in the Extended High Efficiency AAC bitstream (`MediaCodecInfo.CodecProfileLevel.AACObjectXHE`). 

 The associated value is an unsigned integer containing the RAP interval in milliseconds and will be mapped internally to the next rounded multiple of 4096 samples. The first encoded frame will always be a RAP. 

 For example, specifying a constant RAP interval of 896ms at 48kHz and with 1024-sample frames will result in the following (1-based) encoded frames to be RAPs: 1, 43, 85, 127, 169, ...
 
 **Note:** Only one of `KEY_AAC_RAP_INTERVALS` and `KEY_AAC_CONSTANT_RAP_INTERVAL` may be set for a single encode. 
 
 ```
public static final String KEY_AAC_CONSTANT_RAP_INTERVAL = "aac-constant-rap-interval";
```
