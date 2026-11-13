/* Huffman codeword reordering (HCR) for the DRM AAC encoder, ISO/IEC 14496-3
   4.5.2.3 and ETSI ES 201 980 clause 5.4.1.

   Written for the Dream DRM software (https://github.com/Drm-tools/dream) as the
   encoder side of libAACdec/src/aacdec_hcr*.cpp, and distributed under the terms
   of the Fraunhofer FDK AAC Codec licence of this library (see NOTICE). */

/* The codewords of one channel are sorted by priority, placed one per segment
   (priority codewords) and the others in the space left in the segments, set by
   set, trial by trial, exactly as the decoder takes them out again. */
#ifndef AACENC_HCR_H
#define AACENC_HCR_H

#include "common_fix.h"
#include "dyn_bits.h"

#define HCR_MAX_CODEWORDS 512
#define HCR_MAX_BITS 16383 /* 14 bit length field */

typedef struct {
  INT lengthReordered;  /* length_of_reordered_spectral_data */
  INT longestCodeword;  /* length_of_longest_codeword */
  UCHAR bits[HCR_MAX_BITS + 1]; /* the reordered spectral data, one bit per entry */
} HCR_ENC_DATA;

/* Returns 0 on success. Long blocks only. */
INT FDKaacEnc_HcrEncode(HCR_ENC_DATA *hcr, const INT *sfbOffset,
                        const SECTION_DATA *sectionData, SHORT *quantSpectrum);

#endif
