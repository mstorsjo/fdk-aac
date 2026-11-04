/* Huffman codeword reordering (HCR) for the DRM AAC encoder, ISO/IEC 14496-3
   4.5.2.3 and ETSI ES 201 980 clause 5.4.1.

   Written for the Dream DRM software (https://github.com/Drm-tools/dream) as the
   encoder side of libAACdec/src/aacdec_hcr*.cpp, and distributed under the terms
   of the Fraunhofer FDK AAC Codec licence of this library (see NOTICE). */

#include "aacenc_hcr.h"
#include "bit_cnt.h"
#include "FDK_bitstream.h"
#include "aacEnc_rom.h"

#define HCR_MAX_CB 32
#define HCR_HI_LTAB(a) ((a) >> 16)
#define HCR_LO_LTAB(a) ((a) & 0xffff)

/* FDKaacEnc_codeValues() writes codebooks 5 and 6 two codewords at a time; HCR
   needs every pair on its own */
static void hcrCodePair56(const SHORT *v, INT cb, FDK_BITSTREAM *bs) {
  const INT t0 = v[0] + 4, t1 = v[1] + 4;
  if (cb == 5)
    FDKwriteBits(bs, FDKaacEnc_huff_ctab5[t0][t1],
                 HCR_HI_LTAB(FDKaacEnc_huff_ltab5_6[t0][t1]));
  else
    FDKwriteBits(bs, FDKaacEnc_huff_ctab6[t0][t1],
                 HCR_LO_LTAB(FDKaacEnc_huff_ltab5_6[t0][t1]));
}

/* the same tables as in the decoder (libAACdec/src/aac_rom.cpp) */
static const UCHAR hcrMaxCwLen[HCR_MAX_CB] = {
    0,  11, 9,  20, 16, 13, 11, 14, 12, 17, 14, 49, 0,  0,  0,  0,
    14, 17, 21, 21, 25, 25, 29, 29, 29, 29, 33, 33, 33, 37, 37, 41};
static const UCHAR hcrDimShift[HCR_MAX_CB] = {1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1,
                                              1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                              1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
static const UCHAR hcrPriority[HCR_MAX_CB] = {
    0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 22, 0, 0, 0, 0,
    6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21};

typedef struct {
  UCHAR cb;
  UCHAR len;
  UCHAR done;
  UCHAR pos;           /* bits already placed */
  UCHAR bit[64];       /* the codeword: body, sign bits, escape sequences */
} HCR_CW;

typedef struct {
  INT left, right;     /* next free bit from the left / from the right */
  INT start;           /* first bit of the segment in the reordered data */
  INT width;
  INT remaining;
} HCR_SEG;

/* The codeword arrays are large: keep them off the stack */
static HCR_CW hcrCw[HCR_MAX_CODEWORDS];
static HCR_SEG hcrSeg[HCR_MAX_CODEWORDS];

INT FDKaacEnc_HcrEncode(HCR_ENC_DATA *hcr, const INT *sfbOffset,
                        const SECTION_DATA *sd, SHORT *quantSpectrum) {
  INT nCw = 0;
  INT total = 0;
  INT longest = 0;
  INT prio, i, k, s, t;

  if (sd->blockType == SHORT_WINDOW) return 1; /* not supported */

  /* codewords of all sections, ordered by priority (stable inside a priority) */
  for (prio = 22; prio >= 1; prio--) {
    for (i = 0; i < sd->noOfSections; i++) {
      const INT cb = sd->huffsection[i].codeBook;
      if (cb < 0 || cb >= HCR_MAX_CB || hcrPriority[cb] != prio) continue;
      {
        const INT sfbStart = sd->huffsection[i].sfbStart;
        const INT sfbEnd = sfbStart + sd->huffsection[i].sfbCnt;
        const INT line0 = sfbOffset[sfbStart];
        const INT lines = sfbOffset[sfbEnd] - line0;
        const INT dim = 1 << hcrDimShift[cb];
        for (k = 0; k + dim <= lines; k += dim) {
          UCHAR buf[64];
          FDK_BITSTREAM bs;
          INT len, b;
          if (nCw >= HCR_MAX_CODEWORDS) return 2;
          FDKinitBitStream(&bs, buf, sizeof(buf), 0, BS_WRITER);
          if (cb == 5 || cb == 6)
            hcrCodePair56(quantSpectrum + line0 + k, cb, &bs);
          else
            FDKaacEnc_codeValues(quantSpectrum + line0 + k, dim, cb, &bs);
          len = (INT)FDKgetValidBits(&bs);
          FDKsyncCache(&bs);
          if (len <= 0 || len > 49) return 3;
          hcrCw[nCw].cb = (UCHAR)cb;
          hcrCw[nCw].len = (UCHAR)len;
          hcrCw[nCw].done = 0;
          hcrCw[nCw].pos = 0;
          for (b = 0; b < len; b++)
            hcrCw[nCw].bit[b] = (buf[b >> 3] >> (7 - (b & 7))) & 1;
          total += len;
          if (len > longest) longest = len;
          nCw++;
        }
      }
    }
  }

  hcr->lengthReordered = total;
  hcr->longestCodeword = longest;
  if (nCw == 0) return 0;
  if (total > HCR_MAX_BITS || longest > 49) return 4;

  /* segments: one per codeword while they fit, the last one takes the rest */
  {
    INT nSeg = 0, start = 0;
    for (k = 0; k < nCw; k++) {
      INT w = hcrMaxCwLen[hcrCw[k].cb];
      if (w > longest) w = longest;
      if (start + w <= total) {
        hcrSeg[nSeg].start = start;
        hcrSeg[nSeg].width = w;
        start += w;
        nSeg++;
      } else {
        if (nSeg == 0) return 5;
        hcrSeg[nSeg - 1].width = total - hcrSeg[nSeg - 1].start;
        break;
      }
    }
    for (s = 0; s < nSeg; s++) {
      hcrSeg[s].left = 0;
      hcrSeg[s].right = hcrSeg[s].width - 1;
      hcrSeg[s].remaining = hcrSeg[s].width;
    }

    /* priority codewords: codeword s in segment s, from the left */
    for (s = 0; s < nSeg; s++) {
      HCR_CW *c = &hcrCw[s];
      if (c->len > hcrSeg[s].width) return 6;
      for (i = 0; i < c->len; i++)
        hcr->bits[hcrSeg[s].start + hcrSeg[s].left++] = c->bit[i];
      hcrSeg[s].remaining -= c->len;
      c->done = 1;
    }

    /* the other codewords, set by set: first set from the right, then
       alternating. Trial t gives segment s the codeword (s - t) mod nSeg. */
    {
      INT first = nSeg;
      INT set = 1;
      while (first < nCw) {
        const INT cnt = (nCw - first < nSeg) ? (nCw - first) : nSeg;
        const INT fromRight = (set & 1);
        if (set >= 14) return 9; /* the decoder handles at most 14 sets */
        for (t = 0; t < nSeg; t++) {
          for (s = 0; s < nSeg; s++) {
            INT c = (s - t) % nSeg;
            HCR_CW *cw;
            if (c < 0) c += nSeg;
            if (c >= cnt || hcrSeg[s].remaining <= 0) continue;
            cw = &hcrCw[first + c];
            while (!cw->done && hcrSeg[s].remaining > 0) {
              const INT p = fromRight ? hcrSeg[s].right-- : hcrSeg[s].left++;
              hcr->bits[hcrSeg[s].start + p] = cw->bit[cw->pos++];
              hcrSeg[s].remaining--;
              if (cw->pos == cw->len) cw->done = 1;
            }
          }
        }
        first += cnt;
        set++;
      }
    }

    /* everything has to be in place */
    for (k = 0; k < nCw; k++)
      if (!hcrCw[k].done) return 7;
    for (s = 0; s < nSeg; s++)
      if (hcrSeg[s].remaining != 0) return 8;
  }
  return 0;
}
