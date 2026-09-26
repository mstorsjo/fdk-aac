/* -----------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright 2025 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung
e.V. All rights reserved.

 1.    INTRODUCTION
The Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software
that implements the MPEG Advanced Audio Coding ("AAC") encoding and decoding
scheme for digital audio. This FDK AAC Codec software is intended to be used on
a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient
general perceptual audio codecs. AAC-ELD is considered the best-performing
full-bandwidth communications codec by independent studies and is widely
deployed. AAC has been standardized by ISO and IEC as part of the MPEG
specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including
those of Fraunhofer) may be obtained through Via Licensing
(www.vialicensing.com) or through the respective patent owners individually for
the purpose of encoding or decoding bit streams in products that are compliant
with the ISO/IEC MPEG audio standards. Please note that most manufacturers of
Android devices already license these patent claims through Via Licensing or
directly from the patent owners, and therefore FDK AAC Codec software may
already be covered under those patent licenses when it is used for those
licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions
with enhanced sound quality, are also available from Fraunhofer. Users are
encouraged to check the Fraunhofer website for additional applications
information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees provided that you
satisfy the following conditions:

You must retain the complete text of this software license in redistributions of
the FDK AAC Codec or your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation
and/or other materials provided with redistributions of the FDK AAC Codec or
your modifications thereto in binary form. You must make available free of
charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived
from this library without prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute
the FDK AAC Codec software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating
that you changed the software and the date of any change. For modified versions
of the FDK AAC Codec, the term "Fraunhofer FDK AAC Codec Library for Android"
must be replaced by the term "Third-Party Modified Version of the Fraunhofer FDK
AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty of patent non-infringement with respect to this
software.

You may use this FDK AAC Codec software or modifications thereto only for
purposes that are authorized by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright
holders and contributors "AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
including but not limited to the implied warranties of merchantability and
fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary,
or consequential damages, including but not limited to procurement of substitute
goods or services; loss of use, data, or profits, or business interruption,
however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of
this software, even if advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------- */
#ifndef AAC_API_TYPES_H
#define AAC_API_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Transport type identifiers.
 */
typedef enum {
  TT_UNKNOWN = -1, /**< Unknown format.            */
  TT_MP4_RAW = 0,  /**< "as is" access units (packet based since there is
                      obviously no sync layer) */
  TT_MP4_ADIF = 1, /**< ADIF bitstream format.     */
  TT_MP4_ADTS = 2, /**< ADTS bitstream format.     */

  TT_MP4_LATM_MCP1 = 6, /**< Audio Mux Elements with muxConfigPresent = 1 */
  TT_MP4_LATM_MCP0 = 7, /**< Audio Mux Elements with muxConfigPresent = 0, out
                           of band StreamMuxConfig */

  TT_MP4_LOAS = 10 /**< Audio Sync Stream.         */

} TRANSPORT_TYPE;

#define TT_IS_PACKET(x)                                \
  (((x) == TT_MP4_RAW) || ((x) == TT_MP4_LATM_MCP0) || \
   ((x) == TT_MP4_LATM_MCP1))

#define TT_CFG_IS_INBAND(x)                                              \
  (((x) == TT_MP4_ADTS) || ((x) == TT_MP4_ADIF) || ((x) == TT_MP1_L1) || \
   ((x) == TT_MP1_L2) || ((x) == TT_MP1_L3) || ((x) == TT_RSVD50) ||     \
   ((x) == TT_MP4_LATM_MCP1) || ((x) == TT_MHAS) ||                      \
   ((x) == TT_MHAS_PACKETIZED))

#define TT_NEEDS_BS_COPY(x) \
  (((x) == TT_MHA_RAW) || ((x) == TT_MHAS) || ((x) == TT_MHAS_PACKETIZED))

/**
 * Audio Object Type definitions.
 */
typedef enum {
  AOT_NONE = -1,
  AOT_NULL_OBJECT = 0,
  AOT_AAC_LC = 2, /**< Low Complexity object                     */
  AOT_SBR = 5,
  AOT_ER_AAC_LC = 17,   /**< Error Resilient(ER) AAC Low Complexity    */
  AOT_ER_AAC_SCAL = 20, /**< Error Resilient(ER) AAC Scalable object   */
  AOT_ER_AAC_LD = 23,   /**< Error Resilient(ER) AAC LowDelay object   */
  AOT_PS = 29,          /**< PS, Parametric Stereo (includes SBR)      */
  AOT_ESCAPE = 31,      /**< Signal AOT uses more than 5 bits          */
  AOT_ER_AAC_ELD = 39,  /**< AAC Enhanced Low Delay                    */
  AOT_USAC = 42,        /**< USAC                                      */
  AOT_MP2_AAC_LC = 129, /**< Virtual AOT MP2 Low Complexity profile */
  AOT_MP2_SBR = 132, /**< Virtual AOT MP2 Low Complexity Profile with SBR    */
  AOT_DRM_AAC = 143, /**< Virtual AOT for DRM (ER-AAC-SCAL without SBR)    */
  AOT_DRM_SURROUND =
      146, /**< Virtual AOT for DRM Surround (ER-AAC-SCAL (+SBR) +MPS) */
  AOT_DRM_USAC = 147 /**< Virtual AOT for DRM with USAC */

} AUDIO_OBJECT_TYPE;

/** Represents the order of the channels. */
typedef enum {
  CH_ORDER_MPEG = 0, /* ISO/IEC 14496-3 */
  CH_ORDER_WAV = 1,  /* RIFF WAV fmt */
  CH_ORDER_CICP = 2  /* ISO/IEC 23001-8 */

} CHANNEL_ORDER;

/**
 * Audio Codec flags.
 */
#define AC_ER_VCB11                                                           \
  0x000001 /*!< aacSectionDataResilienceFlag     flag (from ASC): 1 means use \
              virtual codebooks  */
#define AC_ER_RVLC                                                             \
  0x000002 /*!< aacSpectralDataResilienceFlag     flag (from ASC): 1 means use \
              huffman codeword reordering */
#define AC_ER_HCR                                                             \
  0x000004 /*!< aacSectionDataResilienceFlag     flag (from ASC): 1 means use \
              virtual codebooks  */
#define AC_SCALABLE 0x000008         /*!< AAC Scalable*/
#define AC_ELD 0x000010              /*!< AAC-ELD */
#define AC_LD 0x000020               /*!< AAC-LD */
#define AC_ER 0x000040               /*!< ER syntax */
#define AC_USAC 0x000100             /*!< USAC */
#define AC_USAC_HAS_PREROLL 0x000200 /*!< USAC may have preroll */
#define AC_HBE_PRESENT 0x000400      /*!< Harmonic SBR present flag */
#define AC_SBR_PRESENT                                  \
  0x008000 /*!< SBR present flag (from ASC or implicit) \
            */
#define AC_SBRCRC \
  0x010000 /*!< SBR CRC present flag. Only relevant for AAC-ELD for now. */
#define AC_PS_PRESENT 0x020000 /*!< PS present flag (from ASC or implicit)  */
#define AC_MPS_PRESENT                                                 \
  0x040000                /*!< MPS present flag (from ASC or implicit) \
                           */
#define AC_DRM 0x080000   /*!< DRM bit stream syntax */
#define AC_INDEP 0x100000 /*!< Independency flag */
#define AC_PCE 0x200000   /*!< PCE present in current raw_data_block() */
#define AC_ELD_DOWNSCALE 0x1000000 /*!< ELD Downscaled playout */
#define AC_LD_MPS 0x2000000        /*!< Low Delay MPS. */
#define AC_DRC_PRESENT                                   \
  0x4000000 /*!< Dynamic Range Control (DRC) data found. \
             */
#define AC_FRAME_LENGTH 0x08000000 /*!< Frame length flag */
#define AC_MPEG4_ESBR 0x10000000   /*!< MPEG-4 SBR Enhancements */
#define AC_USAC_SCFGI1 \
  0x20000000 /*!< USAC flag: If stereoConfigIndex is 1 the flag is set. */
#define AC_USAC_SCFGI2 \
  0x40000000 /*!< USAC flag: If stereoConfigIndex is 2 the flag is set. */
#define AC_USAC_SCFGI3 \
  0x80000000 /*!< USAC flag: If stereoConfigIndex is 3 the flag is set. */

/**
 * Speaker description tags.
 * Do not change the enumeration values unless it keeps the following
 * segmentation:
 * - Bit 0-3: Horizontal postion (0: none, 1: front, 2: side, 3: back, 4: lfe)
 * - Bit 4-7: Vertical position (0: normal, 1: top, 2: bottom)
 */
typedef enum {
  ACT_NONE = 0x00,
  ACT_FRONT = 0x01, /*!< Front speaker position (at normal height) */
  ACT_SIDE = 0x02,  /*!< Side speaker position (at normal height) */
  ACT_BACK = 0x03,  /*!< Back speaker position (at normal height) */
  ACT_LFE = 0x04,   /*!< Low frequency effect speaker postion (front) */

  ACT_TOP =
      0x10, /*!< Top speaker area (for combination with speaker positions) */
  ACT_FRONT_TOP = 0x11, /*!< Top front speaker = (ACT_FRONT|ACT_TOP) */
  ACT_SIDE_TOP = 0x12,  /*!< Top side speaker  = (ACT_SIDE |ACT_TOP) */
  ACT_BACK_TOP = 0x13,  /*!< Top back speaker  = (ACT_BACK |ACT_TOP) */

  ACT_BOTTOM =
      0x20, /*!< Bottom speaker area (for combination with speaker positions) */
  ACT_FRONT_BOTTOM = 0x21, /*!< Bottom front speaker = (ACT_FRONT|ACT_BOTTOM) */
  ACT_SIDE_BOTTOM = 0x22,  /*!< Bottom side speaker  = (ACT_SIDE |ACT_BOTTOM) */
  ACT_BACK_BOTTOM = 0x23   /*!< Bottom back speaker  = (ACT_BACK |ACT_BOTTOM) */

} AUDIO_CHANNEL_TYPE;

#define USAC_ID_BIT 16 /** USAC element IDs start at USAC_ID_BIT */

/** MP4 Element IDs. */
typedef enum {
  /* mp4 element IDs */
  ID_NONE = -1, /**< Invalid Element helper ID.             */
  ID_SCE = 0,   /**< Single Channel Element.                */
  ID_CPE = 1,   /**< Channel Pair Element.                  */
  ID_CCE = 2,   /**< Coupling Channel Element.              */
  ID_LFE = 3,   /**< LFE Channel Element.                   */
  ID_DSE = 4,   /**< Currently one Data Stream Element for ancillary data is
                   supported. */
  ID_PCE = 5,   /**< Program Config Element.                */
  ID_FIL = 6,   /**< Fill Element.                          */
  ID_END = 7,   /**< Arnie (End Element = Terminator).      */
  ID_EXT = 8,   /**< Extension Payload (ER only).           */
  /* USAC element IDs */
  ID_USAC_SCE = 0 + USAC_ID_BIT, /**< Single Channel Element.                */
  ID_USAC_CPE = 1 + USAC_ID_BIT, /**< Channel Pair Element.                  */
  ID_USAC_LFE = 2 + USAC_ID_BIT, /**< LFE Channel Element.                   */
  ID_USAC_EXT = 3 + USAC_ID_BIT, /**< Extension Element.                     */
  ID_LAST
} MP4_ELEMENT_ID;

#ifdef __cplusplus
}
#endif

#endif /* AAC_API_TYPES_H */
