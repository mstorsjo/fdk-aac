# DRM AAC encoding

The encoder can produce the audio of the DRM broadcasting system (ETSI ES 201 980,
clause 5.4) through three virtual audio object types, the counterparts of what the
decoder reads with `TT_DRM`:

| `AACENC_AOT`            | value | content                                             |
|-------------------------|-------|-----------------------------------------------------|
| `AOT_DRM_AAC`           | 143   | AAC, mono or stereo                                 |
| `AOT_DRM_SBR`           | 144   | AAC with SBR (HE-AAC), mono or stereo               |
| `AOT_DRM_MPEG_PS`       | 145   | AAC with SBR and parametric stereo (HE-AAC v2)      |

Use them with `AACENC_TRANSMUX = TT_MP4_RAW` and an input sampling rate of 12 or 24 kHz
(the AAC rate of the DRM service), twice that with SBR. Stereo and parametric stereo
need 24 kHz. One call to `aacEncEncode()` returns one raw frame: a CRC byte followed by
the frame, which is what a DRM audio super frame carries per audio frame.

What the DRM syntax needs, and this adds to the encoder:

* the 960 sample transform (scalefactor band tables and TNS limits for it),
* the error robust AAC scalable element syntax with HCR (`aacenc_hcr.cpp`) and VCB11,
  long blocks only, no PNS,
* the 8 bit CRC over the side information,
* the SBR payload (with its own 8 bit CRC) at the end of the frame in reverse bit
  order.

## Origin

The 960 sample tables and TNS limits follow the patches of OpenDigitalRadio
([ODR-AudioEnc](https://github.com/Opendigitalradio/ODR-AudioEnc)) to fdk-aac 2.0.x; the
DRM encoding was written for the Dream DRM software
(<https://github.com/Drm-tools/dream>) and tested against the decoder of this library
and in a complete DRM transmitter / receiver chain. It has not been checked against
other DRM receivers.

## Licence

The Fraunhofer FDK AAC Codec licence in `NOTICE` applies to the whole library including
these additions; no patent licence is granted, see the licence text.
