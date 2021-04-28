#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "Origin_h264_encoder.h"
#include "Frame.h"
#include "MacroBlock.h"
#include "BitStream.h"
#include "Quantize.h"
#include "Cabac_H264.h"
#include "MB_Prediction.h"
#include "MotionInfo.h"
#include "NALU.h"
#include "Tables.h"
#include "Trace.h"
#include "Timing.h"

#ifdef SGIENG_DEBUG
extern FILE *sMVOutputFile;
#endif

#ifdef USE_SSE2
#include <emmintrin.h>
#endif

#define MV_BITCOST_LAMBDA    20
#define COEFF_BITCOST_LAMBDA 10	// coeff_cost is close to accurate if you factor it by 0.21f
#define IPRED_COST		     800

#include "MotionEstimation.h"

//#define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
#include "windows.h"
#include <stdio.h>
#endif

#if _DEBUG
#define DEBUG_MB_DECISION	\
{\
	if (gEncoderState.mb_debug_on)\
	{\
		ENC_TRACE("mb:%d intra: %5d  me: %5d  skip: %5d mode:%d ssd: %5d addcost:%5d  total:%5d\n", block->mb_index, block->ipred_ssd, block->me_ssd, block->pskip_ssd, mode, ssd, additional_cost, ssd+additional_cost);\
	}\
}
#else
#define DEBUG_MB_DECISION
#endif

#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant (for AVAILABLE macro)

//#define VERIFY_DCT_QUANT

#ifdef VERIFY_DCT_QUANT
static int sDiffCount[256];
static bool sDiffCountInit = false;
static int sDiffCount2[256];
static bool sDiffCountInit2 = false;
#endif

static MacroBlockType *sCurrentMacroBlockSet = NULL;

#define Q_BITS 15

static void (*FindBestPredictor) (MacroBlockType *block, YUVFrameType *frame);
static void (*CalcSKIP) (MacroBlockType *block, YUVFrameType *frame);
static int (*Quantize16x16_Luma_AC_16) (int16_t *dct16x16, int16_t scale_params[32], int offset_params[16], int shift_bits, int non_zero[16], int preserve_dc, int md_stepsize);
static int (*Quantize8x16_Chroma_AC_16) (int16_t *dct8x16, int16_t scale_params[32], int offset_param, int shift_bits, int non_zero[8]);

// dimensions in MB of frame

void InitMacroBlocks()
{
    for (int i = 0; i < gEncoderState.mb_max; i++)
    {
        ResetMacroBlock(&sCurrentMacroBlockSet[i]);
    }
}

void ResetMacroBlock(MacroBlockType *block)
{
#ifdef ENABLE_AVAILABLE_FLAG
    block->available = false;
#endif
	block->mb_skip_flag = false;
}

void FreeMacroBlockSet(MacroBlockType *set)
{
	if (set)
	{
		free (set);
	}
}

MacroBlockType *GetCurrentMacroBlockSet()
{
	return sCurrentMacroBlockSet;
}

#define BLOCK_SIZE 4
#define BLOCK_SIZE_16 16

#define SUB_DCT_8x1(result)		mm0 = _mm_sub_epi16(mm0,mm1);\
								mm2 = _mm_shufflelo_epi16(mm0, 0x1b);\
								mm2 = _mm_shufflehi_epi16(mm2, 0x1b);\
								mm1 = _mm_add_epi16(mm0, mm2);\
								mm3 = _mm_sub_epi16(mm2, mm0);\
								mm2 = _mm_shufflelo_epi16(mm3, 0x4c);\
								mm2 = _mm_shufflehi_epi16(mm2, 0x4c);\
								mm2 = _mm_add_epi16(mm2, mm2);\
								mm2 = _mm_and_si128(mm2, mask2);\
								mm0 = _mm_shufflelo_epi16(mm3, 0xc8);\
								mm0 = _mm_shufflehi_epi16(mm0, 0xc8);\
								mm0 = _mm_and_si128(mm0, mask2);\
								mm4 = _mm_shufflelo_epi16(mm1, 0x33);\
								mm4 = _mm_shufflehi_epi16(mm4, 0x33);\
								mm4 = _mm_and_si128(mm4, mask1);\
								mm4 = _mm_or_si128(mm2, mm4);\
								mm3 = _mm_sub_epi16(zero_mm, mm1);\
								mm3 = _mm_and_si128(mm3, _mm_slli_epi64(mask2,16));\
								mm1 = _mm_srli_epi64(mm1, 32);\
								mm1 = _mm_and_si128(mm1, mask1);\
								mm1 = _mm_or_si128(mm1, mm3);\
								mm1 = _mm_or_si128(mm1, mm0);\
								(result) = _mm_add_epi16(mm4, mm1);

#define LOAD_SRC_DST()	src = _mm_load_si128((__m128i*)src_block);\
						blk = _mm_load_si128((__m128i*)block);\
						src_block += MB_BLOCK_STRIDE;\
						block += 16;


static inline void DCT_SUB_16x16_SSE2_16(int16_t dst[16][16], uint8_t *block, uint8_t *src_block)//, int stride)
{
//	ENC_START_TIMER(20,"DCT new");

	__m128i zero_mm = _mm_setzero_si128();
	__m128i mask1 = _mm_set1_epi32(0x0000ffff);	// | 0 | 1 | 0 | 1 |
	__m128i mask2 = _mm_set1_epi32(0xffff0000);	// | 1 | 0 | 1 | 0 |
	__m128i mm0, mm1, mm2, mm3, mm4;
	__m128i mm_r0a, mm_r1a, mm_r2a, mm_r3a;
	__m128i mm_r0b, mm_r1b, mm_r2b, mm_r3b;
	__m128i src, blk;

//	__declspec(align(64)) int16_t tmp[256];
	int16_t *dst_ptr = (int16_t *) dst;

	for (int i = 0; i < 4; i++)
	{
		// ================== row 0 ==================
		// first 8 pixels
		LOAD_SRC_DST();
		mm0 = _mm_unpacklo_epi8(src, zero_mm);
		mm1 = _mm_unpacklo_epi8(blk, zero_mm);
		SUB_DCT_8x1(mm_r0a);

		// second 8 pixels
		mm0 = _mm_unpackhi_epi8(src, zero_mm);
		mm1 = _mm_unpackhi_epi8(blk, zero_mm);
		SUB_DCT_8x1(mm_r0b);

		// ================== row 1 ==================
		LOAD_SRC_DST();
		// first 8 pixels
		mm0 = _mm_unpacklo_epi8(src, zero_mm);
		mm1 = _mm_unpacklo_epi8(blk, zero_mm);
		SUB_DCT_8x1(mm_r1a);

		// second 8 pixels
		mm0 = _mm_unpackhi_epi8(src, zero_mm);
		mm1 = _mm_unpackhi_epi8(blk, zero_mm);
		SUB_DCT_8x1(mm_r1b);

		// ================== row 2 ==================
		LOAD_SRC_DST();
		// first 8 pixels
		mm0 = _mm_unpacklo_epi8(src, zero_mm);
		mm1 = _mm_unpacklo_epi8(blk, zero_mm);
		SUB_DCT_8x1(mm_r2a);

		// second 8 pixels
		mm0 = _mm_unpackhi_epi8(src, zero_mm);
		mm1 = _mm_unpackhi_epi8(blk, zero_mm);
		SUB_DCT_8x1(mm_r2b);

		// ================== row 3 ==================
		LOAD_SRC_DST();
		// first 8 pixels
		mm0 = _mm_unpacklo_epi8(src, zero_mm);
		mm1 = _mm_unpacklo_epi8(blk, zero_mm);
		SUB_DCT_8x1(mm_r3a);

		// second 8 pixels
		mm0 = _mm_unpackhi_epi8(src, zero_mm);
		mm1 = _mm_unpackhi_epi8(blk, zero_mm);
		SUB_DCT_8x1(mm_r3b);

		// now we have 4 4x4 blocks done with the first half of the transform
		mm0 = _mm_add_epi16(mm_r0a, mm_r3a);	// t0 = p0 + p3;
		mm1 = _mm_add_epi16(mm_r1a, mm_r2a);	// t1 = p1 + p2;
		mm2 = _mm_sub_epi16(mm_r1a, mm_r2a);	// t2 = p1 - p2;
		mm3 = _mm_sub_epi16(mm_r0a, mm_r3a);	// t3 = p0 - p3;

		mm4 = _mm_add_epi16(mm0, mm1);	// t0 + t1
		_mm_store_si128((__m128i *) dst_ptr, mm4);
		mm4 = _mm_add_epi16(mm2, _mm_add_epi16(mm3, mm3));	// t2 + (t3 << 1)
		_mm_store_si128((__m128i *) (dst_ptr + BLOCK_SIZE_16), mm4);
		mm4 = _mm_sub_epi16(mm0, mm1);	// t0 - t1
		_mm_store_si128((__m128i *) (dst_ptr + (BLOCK_SIZE_16 * 2)), mm4);
		mm4 = _mm_sub_epi16(mm3, _mm_add_epi16(mm2, mm2));	// t3 - (t2 << 1)
		_mm_store_si128((__m128i *) (dst_ptr + (BLOCK_SIZE_16 * 3)), mm4);

		// second half of 16x4
		dst_ptr += (BLOCK_SIZE_16 / 2);

		mm0 = _mm_add_epi16(mm_r0b, mm_r3b);	// t0 = p0 + p3;
		mm1 = _mm_add_epi16(mm_r1b, mm_r2b);	// t1 = p1 + p2;
		mm2 = _mm_sub_epi16(mm_r1b, mm_r2b);	// t2 = p1 - p2;
		mm3 = _mm_sub_epi16(mm_r0b, mm_r3b);	// t3 = p0 - p3;

		mm4 = _mm_add_epi16(mm0, mm1);	// t0 + t1
		_mm_store_si128((__m128i *) dst_ptr, mm4);
		mm4 = _mm_add_epi16(mm2, _mm_add_epi16(mm3, mm3));	// t2 + (t3 << 1)
		_mm_store_si128((__m128i *) (dst_ptr + BLOCK_SIZE_16), mm4);
		mm4 = _mm_sub_epi16(mm0, mm1);	// t0 - t1
		_mm_store_si128((__m128i *) (dst_ptr + (BLOCK_SIZE_16 * 2)), mm4);
		mm4 = _mm_sub_epi16(mm3, _mm_add_epi16(mm2, mm2));	// t3 - (t2 << 1)
		_mm_store_si128((__m128i *) (dst_ptr + (BLOCK_SIZE_16 * 3)), mm4);

		dst_ptr += (BLOCK_SIZE_16 / 2) + (BLOCK_SIZE_16 * 3);	// advance to next 4 rows
	}
//	ENC_PAUSE_TIMER(20, false);
#if 0
	// temp copy convert for testing
	int index = 0;
	for (int j = 0; j < 16; j++)
	{
		for (int i = 0; i < 16; i++, index++)
		{
			dst[j][i] = tmp[index];
		}
	}
#endif
}



// Hybrid to use SSE2 instructions for first half of DCT transform (fast) but because of memory arrangement issues
// the second half is much faster using the old way and let the compiler optimize.
static inline void DCT_SUB_16x16_SSE2(int dst[16][16], uint8_t *block, uint8_t *src_block, int stride)
{
	ENC_START_TIMER(21, "DCT old");
	__m128i zero_mm = _mm_setzero_si128();
	__m128i mask1 = _mm_set1_epi32(0x0000ffff);	// | 0 | 1 | 0 | 1 |
	__m128i mask2 = _mm_set1_epi32(0xffff0000);	// | 1 | 0 | 1 | 0 |
	__m128i mm0, mm1, mm2, mm3, mm4;

	__declspec(align(64)) int16_t tmp[256];
	uint32_t *dst_ptr = (uint32_t *) tmp;

	for (int i = 0; i < 16; i++, dst_ptr += 8, src_block += stride, block += 16)
	{
		__m128i src = _mm_load_si128((__m128i*)src_block);
		__m128i blk = _mm_load_si128((__m128i*)block);
		// first 8 pixels
		mm0 = _mm_unpacklo_epi8(src, zero_mm);
		mm1 = _mm_unpacklo_epi8(blk, zero_mm);
		mm0 = _mm_sub_epi16(mm0,mm1);			// | p0 | p1 | p2 | p3 |
		mm2 = _mm_shufflelo_epi16(mm0, 0x1b);	// | p3 | p2 | p1 | p0 |
		mm2 = _mm_shufflehi_epi16(mm2, 0x1b);	// 
		mm1 = _mm_add_epi16(mm0, mm2);			// | p0 + p3 | p1 + p2 | p2 + p1 | p3 + p0 |
		mm3 = _mm_sub_epi16(mm2, mm0);			// | p3 - p0 | p2 - p1 | p1 - p2 | p0 - p3 |

		mm2 = _mm_shufflelo_epi16(mm3, 0x4c);	// | x | p0 - p3 | x | p2 - p1 |
		mm2 = _mm_shufflehi_epi16(mm2, 0x4c);	// 
		mm2 = _mm_add_epi16(mm2, mm2);			// | x | (p0 - p3) << 1 | x | (p2 - p1) << 1 |
		mm2 = _mm_and_si128(mm2, mask2);		// | 0 | (p0 - p3) << 1 | 0 | (p2 - p1) << 1 |

		mm0 = _mm_shufflelo_epi16(mm3, 0xc8);	// | x | (p1 - p2) | x | (p0 - p3) |
		mm0 = _mm_shufflehi_epi16(mm0, 0xc8);	//
		mm0 = _mm_and_si128(mm0, mask2);		// | 0 | (p1 - p2) | 0 | (p0 - p3) |

		mm4 = _mm_shufflelo_epi16(mm1, 0x33);	// | (p0 + p3) | 0 | (p0 + p3) | 0 |
		mm4 = _mm_shufflehi_epi16(mm4, 0x33);	//
		mm4 = _mm_and_si128(mm4, mask1);		// | (p0 + p3) | 0 | (p0 + p3) | 0 |
		mm4 = _mm_or_si128(mm2, mm4);			// | (p0 + p3) | (p0 - p3) << 1 | (p0 + p3) | (p2 - p1) << 1 |

		mm3 = _mm_sub_epi16(zero_mm, mm1);		// | x | x | -(p2 + p1) | x |
		mm3 = _mm_and_si128(mm3, _mm_slli_epi64(mask2,16));	// | 0 | 0 | -(p2 + p1) | 0 |
		mm1 = _mm_srli_epi64(mm1, 32);
		mm1 = _mm_and_si128(mm1, mask1);		// | (p2 + p1) | 0 | 0 | 0 |
		mm1 = _mm_or_si128(mm1, mm3);			// | (p2 + p1) | 0 | -(p2 + p1) | 0 |
		mm1 = _mm_or_si128(mm1, mm0);			// | (p2 + p1) | (p1 - p2) | -(p2 + p1) | (p0 - p3) |

		mm4 = _mm_add_epi16(mm4, mm1);

		_mm_store_si128((__m128i *) dst_ptr, mm4);

		// second 8 pixels
		mm0 = _mm_unpackhi_epi8(src, zero_mm);
		mm1 = _mm_unpackhi_epi8(blk, zero_mm);
		mm0 = _mm_sub_epi16(mm0,mm1);			// | p0 | p1 | p2 | p3 |
		mm2 = _mm_shufflelo_epi16(mm0, 0x1b);	// | p3 | p2 | p1 | p0 |
		mm2 = _mm_shufflehi_epi16(mm2, 0x1b);	// 
		mm1 = _mm_add_epi16(mm0, mm2);			// | p0 + p3 | p1 + p2 | p2 + p1 | p3 + p0 |
		mm3 = _mm_sub_epi16(mm2, mm0);			// | p3 - p0 | p2 - p1 | p1 - p2 | p0 - p3 |

		mm2 = _mm_shufflelo_epi16(mm3, 0x4c);	// | x | p0 - p3 | x | p2 - p1 |
		mm2 = _mm_shufflehi_epi16(mm2, 0x4c);	// 
		mm2 = _mm_add_epi16(mm2, mm2);			// | x | (p0 - p3) << 1 | x | (p2 - p1) << 1 |
		mm2 = _mm_and_si128(mm2, mask2);		// | 0 | (p0 - p3) << 1 | 0 | (p2 - p1) << 1 |

		mm0 = _mm_shufflelo_epi16(mm3, 0xc8);	// | x | (p1 - p2) | x | (p0 - p3) |
		mm0 = _mm_shufflehi_epi16(mm0, 0xc8);	//
		mm0 = _mm_and_si128(mm0, mask2);		// | 0 | (p1 - p2) | 0 | (p0 - p3) |

		mm4 = _mm_shufflelo_epi16(mm1, 0x33);	// | (p0 + p3) | 0 | (p0 + p3) | 0 |
		mm4 = _mm_shufflehi_epi16(mm4, 0x33);	//
		mm4 = _mm_and_si128(mm4, mask1);		// | (p0 + p3) | 0 | (p0 + p3) | 0 |
		mm4 = _mm_or_si128(mm2, mm4);			// | (p0 + p3) | (p0 - p3) << 1 | (p0 + p3) | (p2 - p1) << 1 |

		mm3 = _mm_sub_epi16(zero_mm, mm1);		// | x | x | -(p2 + p1) | x |
		mm3 = _mm_and_si128(mm3, _mm_slli_epi64(mask2,16));	// | 0 | 0 | -(p2 + p1) | 0 |
		mm1 = _mm_srli_epi64(mm1, 32);
		mm1 = _mm_and_si128(mm1, mask1);		// | (p2 + p1) | 0 | 0 | 0 |
		mm1 = _mm_or_si128(mm1, mm3);			// | (p2 + p1) | 0 | -(p2 + p1) | 0 |
		mm1 = _mm_or_si128(mm1, mm0);			// | (p2 + p1) | (p1 - p2) | -(p2 + p1) | (p0 - p3) |

		mm4 = _mm_add_epi16(mm4, mm1);

		_mm_store_si128((__m128i *) &dst_ptr[4], mm4);
	}

	int p0,p1,p2,p3;
	int t0,t1,t2,t3;
	int16_t *pTmp;

	// Vertical 
	for (unsigned int i = 0; i < BLOCK_SIZE_16; i++)
	{
		pTmp = tmp + i;
		p0 = *pTmp;
		p1 = *(pTmp += BLOCK_SIZE_16);
		p2 = *(pTmp += BLOCK_SIZE_16);
		p3 = *(pTmp += BLOCK_SIZE_16);

		t0 = p0 + p3;
		t1 = p1 + p2;
		t2 = p1 - p2;
		t3 = p0 - p3;

		dst[0][i] = t0 +  t1;
		dst[1][i] = t2 + (t3 << 1);
		dst[2][i] = t0 -  t1;
		dst[3][i] = t3 - (t2 << 1);

		p0 = *(pTmp += BLOCK_SIZE_16);
		p1 = *(pTmp += BLOCK_SIZE_16);
		p2 = *(pTmp += BLOCK_SIZE_16);
		p3 = *(pTmp += BLOCK_SIZE_16);

		t0 = p0 + p3;
		t1 = p1 + p2;
		t2 = p1 - p2;
		t3 = p0 - p3;

		dst[4][i] = t0 +  t1;
		dst[5][i] = t2 + (t3 << 1);
		dst[6][i] = t0 -  t1;
		dst[7][i] = t3 - (t2 << 1);

		p0 = *(pTmp += BLOCK_SIZE_16);
		p1 = *(pTmp += BLOCK_SIZE_16);
		p2 = *(pTmp += BLOCK_SIZE_16);
		p3 = *(pTmp += BLOCK_SIZE_16);

		t0 = p0 + p3;
		t1 = p1 + p2;
		t2 = p1 - p2;
		t3 = p0 - p3;

		dst[8][i] = t0 +  t1;
		dst[9][i] = t2 + (t3 << 1);
		dst[10][i] = t0 -  t1;
		dst[11][i] = t3 - (t2 << 1);

		p0 = *(pTmp += BLOCK_SIZE_16);
		p1 = *(pTmp += BLOCK_SIZE_16);
		p2 = *(pTmp += BLOCK_SIZE_16);
		p3 = *(pTmp += BLOCK_SIZE_16);

		t0 = p0 + p3;
		t1 = p1 + p2;
		t2 = p1 - p2;
		t3 = p0 - p3;

		dst[12][i] = t0 +  t1;
		dst[13][i] = t2 + (t3 << 1);
		dst[14][i] = t0 -  t1;
		dst[15][i] = t3 - (t2 << 1);
	}

	ENC_PAUSE_TIMER(21, false);
}

void DCT_SUB_4x4(int dst[16][16], uint8_t block[16][16], uint8_t *src_block, int stride, int pos_y, int pos_x)
{
	int i, ii;  
	int p0,p1,p2,p3;
	int t0,t1,t2,t3;

	int16_t tmp[16];
	int16_t *pTmp = tmp;

	uint8_t *pblock, *sblock;
	// Horizontal
	for (i = pos_y; i < pos_y + BLOCK_SIZE; i++)
	{
		pblock = &block[i][pos_x];
		sblock = &src_block[(i*stride)+pos_x];
		p0 = *(sblock++) - *(pblock++);
		p1 = *(sblock++) - *(pblock++);
		p2 = *(sblock++) - *(pblock++);
		p3 = *(sblock)   - *(pblock  );

		t0 = p0 + p3;
		t1 = p1 + p2;
		t2 = p1 - p2;
		t3 = p0 - p3;

		*(pTmp++) = static_cast<int16_t>(t0 + t1);
		*(pTmp++) = static_cast<int16_t>((t3 << 1) + t2);
		*(pTmp++) = static_cast<int16_t>(t0 - t1);    
		*(pTmp++) = static_cast<int16_t>(t3 - (t2 << 1));
	}

	// Vertical 
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		pTmp = tmp + i;
		p0 = *pTmp;
		p1 = *(pTmp += BLOCK_SIZE);
		p2 = *(pTmp += BLOCK_SIZE);
		p3 = *(pTmp += BLOCK_SIZE);

		t0 = p0 + p3;
		t1 = p1 + p2;
		t2 = p1 - p2;
		t3 = p0 - p3;

		ii = pos_x + i;
		dst[pos_y    ][ii] = t0 +  t1;
		dst[pos_y + 1][ii] = t2 + (t3 << 1);
		dst[pos_y + 2][ii] = t0 -  t1;
		dst[pos_y + 3][ii] = t3 - (t2 << 1);
	}
}

// New 16-bit version
void DCT_SUB_8x8_UV_16_SSE2(int16_t dst[2][8][8], uint8_t *block, uint8_t *src_block)
{
	__m128i zero_mm = _mm_setzero_si128();
	__m128i maskb = _mm_set1_epi16(0x00ff);
	__m128i mask1 = _mm_set1_epi32(0x0000ffff);	// | 0 | 1 | 0 | 1 |
	__m128i mask2 = _mm_set1_epi32(0xffff0000);	// | 1 | 0 | 1 | 0 |
	__m128i mm0, mm1, mm2, mm3, mm4;
	__m128i mm_r0a, mm_r1a, mm_r2a, mm_r3a;
	__m128i mm_r0b, mm_r1b, mm_r2b, mm_r3b;
	__m128i src, blk;

	uint8_t *dst_ptr_U = (uint8_t *) dst[0][0];
	uint8_t *dst_ptr_V = (uint8_t *) dst[1][0];

	for (int i = 0; i < 2; i++)
	{
		// ================== row 0 ==================
		LOAD_SRC_DST();

		// first 8 pixels
		// interleaved so we want just the even first for U plane
		mm0 = _mm_and_si128(src, maskb);
		mm1 = _mm_and_si128(blk, maskb);

		SUB_DCT_8x1(mm_r0a);

		// second 8 pixels - V plane
		src = _mm_srli_si128(src,1);
		mm0 = _mm_and_si128(src, maskb);
		blk = _mm_srli_si128(blk,1);
		mm1 = _mm_and_si128(blk, maskb);

		SUB_DCT_8x1(mm_r0b);

		// ================== row 1 ==================
		LOAD_SRC_DST();

		// first 8 pixels
		// interleaved so we want just the even first for U plane
		mm0 = _mm_and_si128(src, maskb);
		mm1 = _mm_and_si128(blk, maskb);

		SUB_DCT_8x1(mm_r1a);

		// second 8 pixels - V plane
		src = _mm_srli_si128(src,1);
		mm0 = _mm_and_si128(src, maskb);
		blk = _mm_srli_si128(blk,1);
		mm1 = _mm_and_si128(blk, maskb);

		SUB_DCT_8x1(mm_r1b);

		// ================== row 2 ==================
		LOAD_SRC_DST();

		// first 8 pixels
		// interleaved so we want just the even first for U plane
		mm0 = _mm_and_si128(src, maskb);
		mm1 = _mm_and_si128(blk, maskb);

		SUB_DCT_8x1(mm_r2a);

		// second 8 pixels - V plane
		src = _mm_srli_si128(src,1);
		mm0 = _mm_and_si128(src, maskb);
		blk = _mm_srli_si128(blk,1);
		mm1 = _mm_and_si128(blk, maskb);

		SUB_DCT_8x1(mm_r2b);

		// ================== row 3 ==================
		LOAD_SRC_DST();

		// first 8 pixels
		// interleaved so we want just the even first for U plane
		mm0 = _mm_and_si128(src, maskb);
		mm1 = _mm_and_si128(blk, maskb);

		SUB_DCT_8x1(mm_r3a);

		// second 8 pixels - V plane
		src = _mm_srli_si128(src,1);
		mm0 = _mm_and_si128(src, maskb);
		blk = _mm_srli_si128(blk,1);
		mm1 = _mm_and_si128(blk, maskb);

		SUB_DCT_8x1(mm_r3b);

		// now we have 4 4x4 blocks done with the first half of the transform
		// U plane
		mm0 = _mm_add_epi16(mm_r0a, mm_r3a);	// t0 = p0 + p3;
		mm1 = _mm_add_epi16(mm_r1a, mm_r2a);	// t1 = p1 + p2;
		mm2 = _mm_sub_epi16(mm_r1a, mm_r2a);	// t2 = p1 - p2;
		mm3 = _mm_sub_epi16(mm_r0a, mm_r3a);	// t3 = p0 - p3;

		mm4 = _mm_add_epi16(mm0, mm1);	// t0 + t1
		_mm_store_si128((__m128i *) dst_ptr_U, mm4);
		mm4 = _mm_add_epi16(mm2, _mm_add_epi16(mm3, mm3));	// t2 + (t3 << 1)
		_mm_store_si128((__m128i *) (dst_ptr_U + BLOCK_SIZE_16), mm4);
		mm4 = _mm_sub_epi16(mm0, mm1);	// t0 - t1
		_mm_store_si128((__m128i *) (dst_ptr_U + (BLOCK_SIZE_16 * 2)), mm4);
		mm4 = _mm_sub_epi16(mm3, _mm_add_epi16(mm2, mm2));	// t3 - (t2 << 1)
		_mm_store_si128((__m128i *) (dst_ptr_U + (BLOCK_SIZE_16 * 3)), mm4);
		dst_ptr_U += BLOCK_SIZE_16 * 4;

		// V plane
		mm0 = _mm_add_epi16(mm_r0b, mm_r3b);	// t0 = p0 + p3;
		mm1 = _mm_add_epi16(mm_r1b, mm_r2b);	// t1 = p1 + p2;
		mm2 = _mm_sub_epi16(mm_r1b, mm_r2b);	// t2 = p1 - p2;
		mm3 = _mm_sub_epi16(mm_r0b, mm_r3b);	// t3 = p0 - p3;

		mm4 = _mm_add_epi16(mm0, mm1);	// t0 + t1
		_mm_store_si128((__m128i *) dst_ptr_V, mm4);
		mm4 = _mm_add_epi16(mm2, _mm_add_epi16(mm3, mm3));	// t2 + (t3 << 1)
		_mm_store_si128((__m128i *) (dst_ptr_V + BLOCK_SIZE_16), mm4);
		mm4 = _mm_sub_epi16(mm0, mm1);	// t0 - t1
		_mm_store_si128((__m128i *) (dst_ptr_V + (BLOCK_SIZE_16 * 2)), mm4);
		mm4 = _mm_sub_epi16(mm3, _mm_add_epi16(mm2, mm2));	// t3 - (t2 << 1)
		_mm_store_si128((__m128i *) (dst_ptr_V + (BLOCK_SIZE_16 * 3)), mm4);
		dst_ptr_V += BLOCK_SIZE_16 * 4;
	}
}

void DCT_SUB_8x8_UV_SSE2(int dst[2][8][8], ChromaPixelType block[8][8], ChromaPixelType *src_block, int stride)
{
	int p0,p1,p2,p3;
	int t0,t1,t2,t3;

	__m128i zero_mm = _mm_setzero_si128();
	__m128i maskb = _mm_set1_epi16(0x00ff);
	__m128i mask1 = _mm_set1_epi32(0x0000ffff);	// | 0 | 1 | 0 | 1 |
	__m128i mask2 = _mm_set1_epi32(0xffff0000);	// | 1 | 0 | 1 | 0 |
	__m128i mm0, mm1, mm2, mm3, mm4;

	__declspec(align(64)) int16_t tmp[256];
	uint32_t *dst_ptr = (uint32_t *) tmp;
	int16_t *pTmp = tmp;

	for (int i = 0; i < 8; i++, dst_ptr += 8, src_block += stride)
	{
		__m128i src = _mm_load_si128((__m128i*)src_block);
		__m128i blk = _mm_load_si128((__m128i*)block[i]);
		// first 8 pixels
		// interleaved so we want just the even first for U plane
		mm0 = _mm_and_si128(src, maskb);
		mm1 = _mm_and_si128(blk, maskb);
		mm0 = _mm_sub_epi16(mm0,mm1);			// | p0 | p1 | p2 | p3 |
		mm2 = _mm_shufflelo_epi16(mm0, 0x1b);	// | p3 | p2 | p1 | p0 |
		mm2 = _mm_shufflehi_epi16(mm2, 0x1b);	// 
		mm1 = _mm_add_epi16(mm0, mm2);			// | p0 + p3 | p1 + p2 | p2 + p1 | p3 + p0 |
		mm3 = _mm_sub_epi16(mm2, mm0);			// | p3 - p0 | p2 - p1 | p1 - p2 | p0 - p3 |

		mm2 = _mm_shufflelo_epi16(mm3, 0x4c);	// | x | p0 - p3 | x | p2 - p1 |
		mm2 = _mm_shufflehi_epi16(mm2, 0x4c);	// 
		mm2 = _mm_add_epi16(mm2, mm2);			// | x | (p0 - p3) << 1 | x | (p2 - p1) << 1 |
		mm2 = _mm_and_si128(mm2, mask2);		// | 0 | (p0 - p3) << 1 | 0 | (p2 - p1) << 1 |

		mm0 = _mm_shufflelo_epi16(mm3, 0xc8);	// | x | (p1 - p2) | x | (p0 - p3) |
		mm0 = _mm_shufflehi_epi16(mm0, 0xc8);	//
		mm0 = _mm_and_si128(mm0, mask2);		// | 0 | (p1 - p2) | 0 | (p0 - p3) |

		mm4 = _mm_shufflelo_epi16(mm1, 0x33);	// | (p0 + p3) | 0 | (p0 + p3) | 0 |
		mm4 = _mm_shufflehi_epi16(mm4, 0x33);	//
		mm4 = _mm_and_si128(mm4, mask1);		// | (p0 + p3) | 0 | (p0 + p3) | 0 |
		mm4 = _mm_or_si128(mm2, mm4);			// | (p0 + p3) | (p0 - p3) << 1 | (p0 + p3) | (p2 - p1) << 1 |

		mm3 = _mm_sub_epi16(zero_mm, mm1);		// | x | x | -(p2 + p1) | x |
		mm3 = _mm_and_si128(mm3, _mm_slli_epi64(mask2,16));	// | 0 | 0 | -(p2 + p1) | 0 |
		mm1 = _mm_srli_epi64(mm1, 32);
		mm1 = _mm_and_si128(mm1, mask1);		// | (p2 + p1) | 0 | 0 | 0 |
		mm1 = _mm_or_si128(mm1, mm3);			// | (p2 + p1) | 0 | -(p2 + p1) | 0 |
		mm1 = _mm_or_si128(mm1, mm0);			// | (p2 + p1) | (p1 - p2) | -(p2 + p1) | (p0 - p3) |

		mm4 = _mm_add_epi16(mm4, mm1);

		_mm_store_si128((__m128i *) dst_ptr, mm4);

		// second 8 pixels
		src = _mm_srli_si128(src,1);
		mm0 = _mm_and_si128(src, maskb);
		blk = _mm_srli_si128(blk,1);
		mm1 = _mm_and_si128(blk, maskb);

		mm0 = _mm_sub_epi16(mm0,mm1);			// | p0 | p1 | p2 | p3 |
		mm2 = _mm_shufflelo_epi16(mm0, 0x1b);	// | p3 | p2 | p1 | p0 |
		mm2 = _mm_shufflehi_epi16(mm2, 0x1b);	// 
		mm1 = _mm_add_epi16(mm0, mm2);			// | p0 + p3 | p1 + p2 | p2 + p1 | p3 + p0 |
		mm3 = _mm_sub_epi16(mm2, mm0);			// | p3 - p0 | p2 - p1 | p1 - p2 | p0 - p3 |

		mm2 = _mm_shufflelo_epi16(mm3, 0x4c);	// | x | p0 - p3 | x | p2 - p1 |
		mm2 = _mm_shufflehi_epi16(mm2, 0x4c);	// 
		mm2 = _mm_add_epi16(mm2, mm2);			// | x | (p0 - p3) << 1 | x | (p2 - p1) << 1 |
		mm2 = _mm_and_si128(mm2, mask2);		// | 0 | (p0 - p3) << 1 | 0 | (p2 - p1) << 1 |

		mm0 = _mm_shufflelo_epi16(mm3, 0xc8);	// | x | (p1 - p2) | x | (p0 - p3) |
		mm0 = _mm_shufflehi_epi16(mm0, 0xc8);	//
		mm0 = _mm_and_si128(mm0, mask2);		// | 0 | (p1 - p2) | 0 | (p0 - p3) |

		mm4 = _mm_shufflelo_epi16(mm1, 0x33);	// | (p0 + p3) | 0 | (p0 + p3) | 0 |
		mm4 = _mm_shufflehi_epi16(mm4, 0x33);	//
		mm4 = _mm_and_si128(mm4, mask1);		// | (p0 + p3) | 0 | (p0 + p3) | 0 |
		mm4 = _mm_or_si128(mm2, mm4);			// | (p0 + p3) | (p0 - p3) << 1 | (p0 + p3) | (p2 - p1) << 1 |

		mm3 = _mm_sub_epi16(zero_mm, mm1);		// | x | x | -(p2 + p1) | x |
		mm3 = _mm_and_si128(mm3, _mm_slli_epi64(mask2,16));	// | 0 | 0 | -(p2 + p1) | 0 |
		mm1 = _mm_srli_epi64(mm1, 32);
		mm1 = _mm_and_si128(mm1, mask1);		// | (p2 + p1) | 0 | 0 | 0 |
		mm1 = _mm_or_si128(mm1, mm3);			// | (p2 + p1) | 0 | -(p2 + p1) | 0 |
		mm1 = _mm_or_si128(mm1, mm0);			// | (p2 + p1) | (p1 - p2) | -(p2 + p1) | (p0 - p3) |

		mm4 = _mm_add_epi16(mm4, mm1);

		_mm_store_si128((__m128i *) &dst_ptr[4], mm4);
	}

	// Vertical - U
	for (int i = 0; i < 8; i++)
	{
		pTmp  = tmp  + i;
		p0  = *pTmp;
		p1  = *(pTmp += BLOCK_SIZE_16);
		p2  = *(pTmp += BLOCK_SIZE_16);
		p3  = *(pTmp += BLOCK_SIZE_16);

		t0 = p0 + p3;
		t1 = p1 + p2;
		t2 = p1 - p2;
		t3 = p0 - p3;

		dst[0][0    ][i] = t0 +  t1;
		dst[0][0 + 1][i] = t2 + (t3 << 1);
		dst[0][0 + 2][i] = t0 -  t1;
		dst[0][0 + 3][i] = t3 - (t2 << 1);

		p0  = *(pTmp += BLOCK_SIZE_16);
		p1  = *(pTmp += BLOCK_SIZE_16);
		p2  = *(pTmp += BLOCK_SIZE_16);
		p3  = *(pTmp += BLOCK_SIZE_16);

		t0 = p0 + p3;
		t1 = p1 + p2;
		t2 = p1 - p2;
		t3 = p0 - p3;

		dst[0][4    ][i] = t0 +  t1;
		dst[0][4 + 1][i] = t2 + (t3 << 1);
		dst[0][4 + 2][i] = t0 -  t1;
		dst[0][4 + 3][i] = t3 - (t2 << 1);
	}

	// Vertical - V
	for (int i = 0; i < 8; i++)
	{
		pTmp  = tmp  + i + 8;
		p0  = *pTmp;
		p1  = *(pTmp += BLOCK_SIZE_16);
		p2  = *(pTmp += BLOCK_SIZE_16);
		p3  = *(pTmp += BLOCK_SIZE_16);

		t0 = p0 + p3;
		t1 = p1 + p2;
		t2 = p1 - p2;
		t3 = p0 - p3;

		dst[1][0    ][i] = t0 +  t1;
		dst[1][0 + 1][i] = t2 + (t3 << 1);
		dst[1][0 + 2][i] = t0 -  t1;
		dst[1][0 + 3][i] = t3 - (t2 << 1);

		p0  = *(pTmp += BLOCK_SIZE_16);
		p1  = *(pTmp += BLOCK_SIZE_16);
		p2  = *(pTmp += BLOCK_SIZE_16);
		p3  = *(pTmp += BLOCK_SIZE_16);

		t0 = p0 + p3;
		t1 = p1 + p2;
		t2 = p1 - p2;
		t3 = p0 - p3;

		dst[1][4    ][i] = t0 +  t1;
		dst[1][4 + 1][i] = t2 + (t3 << 1);
		dst[1][4 + 2][i] = t0 -  t1;
		dst[1][4 + 3][i] = t3 - (t2 << 1);
	}
}


void DCT_SUB_4x4_UV(int dst[2][8][8], ChromaPixelType block[8][8], ChromaPixelType *src_block, int stride, int pos_y, int pos_x)
{
	int i, ii;  
	int tmp[16], tmpv[16];
	int *pTmp = tmp;
	int *pTmpv = tmpv;
	uint8_t *pblock, *sblock;
	int p0,p1,p2,p3;
	int pv0,pv1,pv2,pv3;
	int t0,t1,t2,t3;
	int tv0,tv1,tv2,tv3;

	// Horizontal
	for (i = pos_y; i < pos_y + BLOCK_SIZE; i++)
	{
		pblock = &block[i][pos_x].u;
		sblock = &(src_block[(i*stride)+pos_x].u);
		p0  = *(sblock++) - *(pblock++);
		pv0 = *(sblock++) - *(pblock++);
		p1  = *(sblock++) - *(pblock++);
		pv1 = *(sblock++) - *(pblock++);
		p2  = *(sblock++) - *(pblock++);
		pv2 = *(sblock++) - *(pblock++);
		p3  = *(sblock++) - *(pblock++);
		pv3 = *(sblock  ) - *(pblock);

		t0 = p0 + p3;
		t1 = p1 + p2;
		t2 = p1 - p2;
		t3 = p0 - p3;

		tv0 = pv0 + pv3;
		tv1 = pv1 + pv2;
		tv2 = pv1 - pv2;
		tv3 = pv0 - pv3;

		*(pTmp++) =  t0 + t1;
		*(pTmp++) = (t3 << 1) + t2;
		*(pTmp++) =  t0 - t1;    
		*(pTmp++) =  t3 - (t2 << 1);

		*(pTmpv++) =  tv0 + tv1;
		*(pTmpv++) = (tv3 << 1) + tv2;
		*(pTmpv++) =  tv0 - tv1;    
		*(pTmpv++) =  tv3 - (tv2 << 1);
	}

	// Vertical 
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		pTmp  = tmp  + i;
		pTmpv = tmpv + i;
		p0 = *pTmp;
		p1 = *(pTmp += BLOCK_SIZE);
		p2 = *(pTmp += BLOCK_SIZE);
		p3 = *(pTmp += BLOCK_SIZE);

		pv0 = *pTmpv;
		pv1 = *(pTmpv += BLOCK_SIZE);
		pv2 = *(pTmpv += BLOCK_SIZE);
		pv3 = *(pTmpv += BLOCK_SIZE);

		t0 = p0 + p3;
		t1 = p1 + p2;
		t2 = p1 - p2;
		t3 = p0 - p3;

		tv0 = pv0 + pv3;
		tv1 = pv1 + pv2;
		tv2 = pv1 - pv2;
		tv3 = pv0 - pv3;

		ii = pos_x + i;
		dst[0][pos_y    ][ii] = t0 +  t1;
		dst[0][pos_y + 1][ii] = t2 + (t3 << 1);
		dst[0][pos_y + 2][ii] = t0 -  t1;
		dst[0][pos_y + 3][ii] = t3 - (t2 << 1);

		dst[1][pos_y    ][ii] = tv0 +  tv1;
		dst[1][pos_y + 1][ii] = tv2 + (tv3 << 1);
		dst[1][pos_y + 2][ii] = tv0 -  tv1;
		dst[1][pos_y + 3][ii] = tv3 - (tv2 << 1);

	}
}

void Inv_DCT_4x4(int16_t block[16][16], int pos_y, int pos_x)
{
	int i, ii;  
	int16_t tmp[16];
	int16_t *pTmp = tmp, *pblock;
	int p0,p1,p2,p3;
	int t0,t1,t2,t3;

	// Horizontal
	for (i = pos_y; i < pos_y + BLOCK_SIZE; i++)
	{
		pblock = &block[i][pos_x];
		t0 = *(pblock++);
		t1 = *(pblock++);
		t2 = *(pblock++);
		t3 = *(pblock  );

		p0 =  t0 + t2;
		p1 =  t0 - t2;
		p2 = (t1 >> 1) - t3;
		p3 =  t1 + (t3 >> 1);

		*(pTmp++) = static_cast<int16_t>(p0 + p3);
		*(pTmp++) = static_cast<int16_t>(p1 + p2);
		*(pTmp++) = static_cast<int16_t>(p1 - p2);
		*(pTmp++) = static_cast<int16_t>(p0 - p3);
	}

	//  Vertical 
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		pTmp = tmp + i;
		t0 = *pTmp;
		t1 = *(pTmp += BLOCK_SIZE);
		t2 = *(pTmp += BLOCK_SIZE);
		t3 = *(pTmp += BLOCK_SIZE);

		p0 = t0 + t2;
		p1 = t0 - t2;
		p2 =(t1 >> 1) - t3;
		p3 = t1 + (t3 >> 1);

		ii = i + pos_x;
		block[pos_y    ][ii] = static_cast<int16_t>(p0 + p3);
		block[pos_y + 1][ii] = static_cast<int16_t>(p1 + p2);
		block[pos_y + 2][ii] = static_cast<int16_t>(p1 - p2);
		block[pos_y + 3][ii] = static_cast<int16_t>(p0 - p3);
	}
}

void Inv_DCT_4x4_32(int16_t block[16][16], int pos_y, int pos_x)
{
	int i, ii;  
	int tmp[16];
	int *pTmp = tmp;
	int16_t *pblock;
	int p0,p1,p2,p3;
	int t0,t1,t2,t3;

	// Horizontal
	for (i = pos_y; i < pos_y + BLOCK_SIZE; i++)
	{
		pblock = &block[i][pos_x];
		t0 = *(pblock++);
		t1 = *(pblock++);
		t2 = *(pblock++);
		t3 = *(pblock  );

		p0 =  t0 + t2;
		p1 =  t0 - t2;
		p2 = (t1 >> 1) - t3;
		p3 =  t1 + (t3 >> 1);

		*(pTmp++) = p0 + p3;
		*(pTmp++) = p1 + p2;
		*(pTmp++) = p1 - p2;
		*(pTmp++) = p0 - p3;
	}

	//  Vertical 
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		pTmp = tmp + i;
		t0 = *pTmp;
		t1 = *(pTmp += BLOCK_SIZE);
		t2 = *(pTmp += BLOCK_SIZE);
		t3 = *(pTmp += BLOCK_SIZE);

		p0 = t0 + t2;
		p1 = t0 - t2;
		p2 =(t1 >> 1) - t3;
		p3 = t1 + (t3 >> 1);

		ii = i + pos_x;
		block[pos_y    ][ii] = static_cast<int16_t>(p0 + p3);
		block[pos_y + 1][ii] = static_cast<int16_t>(p1 + p2);
		block[pos_y + 2][ii] = static_cast<int16_t>(p1 - p2);
		block[pos_y + 3][ii] = static_cast<int16_t>(p0 - p3);
	}
}

void Inv_DCT_4x4_UV_16(int16_t src_block[8][8], int pos_y, int pos_x)
{
	int i, ii;  
	int16_t tmp[16];
	int16_t *pTmp = tmp, *pblock;
	int p0,p1,p2,p3;
	int t0,t1,t2,t3;

	// Horizontal
	for (i = pos_y; i < pos_y + BLOCK_SIZE; i++)
	{
		pblock = &src_block[i][pos_x];
		t0 = *(pblock++);
		t1 = *(pblock++);
		t2 = *(pblock++);
		t3 = *(pblock  );

		p0 =  t0 + t2;
		p1 =  t0 - t2;
		p2 = (t1 >> 1) - t3;
		p3 =  t1 + (t3 >> 1);

		*(pTmp++) = static_cast<int16_t>(p0 + p3);
		*(pTmp++) = static_cast<int16_t>(p1 + p2);
		*(pTmp++) = static_cast<int16_t>(p1 - p2);
		*(pTmp++) = static_cast<int16_t>(p0 - p3);
	}

	//  Vertical 
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		pTmp = tmp + i;
		t0 = *pTmp;
		t1 = *(pTmp += BLOCK_SIZE);
		t2 = *(pTmp += BLOCK_SIZE);
		t3 = *(pTmp += BLOCK_SIZE);

		p0 = t0 + t2;
		p1 = t0 - t2;
		p2 =(t1 >> 1) - t3;
		p3 = t1 + (t3 >> 1);

		ii = i + pos_x;
		src_block[pos_y    ][ii] = static_cast<int16_t>(p0 + p3);
		src_block[pos_y + 1][ii] = static_cast<int16_t>(p1 + p2);
		src_block[pos_y + 2][ii] = static_cast<int16_t>(p1 - p2);
		src_block[pos_y + 3][ii] = static_cast<int16_t>(p0 - p3);
	}
}

void Inv_DCT_4x4_UV(int src_block[8][8], int pos_y, int pos_x)
{
	int i, ii;  
	int tmp[16];
	int *pTmp = tmp, *pblock;
	int p0,p1,p2,p3;
	int t0,t1,t2,t3;

	// Horizontal
	for (i = pos_y; i < pos_y + BLOCK_SIZE; i++)
	{
		pblock = &src_block[i][pos_x];
		t0 = *(pblock++);
		t1 = *(pblock++);
		t2 = *(pblock++);
		t3 = *(pblock  );

		p0 =  t0 + t2;
		p1 =  t0 - t2;
		p2 = (t1 >> 1) - t3;
		p3 =  t1 + (t3 >> 1);

		*(pTmp++) = p0 + p3;
		*(pTmp++) = p1 + p2;
		*(pTmp++) = p1 - p2;
		*(pTmp++) = p0 - p3;
	}

	//  Vertical 
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		pTmp = tmp + i;
		t0 = *pTmp;
		t1 = *(pTmp += BLOCK_SIZE);
		t2 = *(pTmp += BLOCK_SIZE);
		t3 = *(pTmp += BLOCK_SIZE);

		p0 = t0 + t2;
		p1 = t0 - t2;
		p2 =(t1 >> 1) - t3;
		p3 = t1 + (t3 >> 1);

		ii = i + pos_x;
		src_block[pos_y    ][ii] = p0 + p3;
		src_block[pos_y + 1][ii] = p1 + p2;
		src_block[pos_y + 2][ii] = p1 - p2;
		src_block[pos_y + 3][ii] = p0 - p3;
	}
}

void Hadamard4x4(int block[16], int tblock[16])
{
	int i;
	int tmp[16];
	int *pTmp = tmp, *pblock;
	int p0,p1,p2,p3;
	int t0,t1,t2,t3;

	// Horizontal
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		pblock = &block[i*BLOCK_SIZE];
		p0 = *(pblock++);
		p1 = *(pblock++);
		p2 = *(pblock++);
		p3 = *(pblock  );

		t0 = p0 + p3;
		t1 = p1 + p2;
		t2 = p1 - p2;
		t3 = p0 - p3;

		*(pTmp++) = t0 + t1;
		*(pTmp++) = t3 + t2;
		*(pTmp++) = t0 - t1;    
		*(pTmp++) = t3 - t2;
	}

	// Vertical 
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		pTmp = tmp + i;
		p0 = *pTmp;
		p1 = *(pTmp += BLOCK_SIZE);
		p2 = *(pTmp += BLOCK_SIZE);
		p3 = *(pTmp += BLOCK_SIZE);

		t0 = p0 + p3;
		t1 = p1 + p2;
		t2 = p1 - p2;
		t3 = p0 - p3;

		pblock = &tblock[i];
		*(pblock)             = (t0 + t1) >> 1;
		*(pblock+=BLOCK_SIZE) = (t2 + t3) >> 1;
		*(pblock+=BLOCK_SIZE) = (t0 - t1) >> 1;
		*(pblock+=BLOCK_SIZE) = (t3 - t2) >> 1;
	}
}

void Hadamard2x2(int block[4], int tblock[4])
{
	int p0,p1,p2,p3;

	p0 = block[0] + block[1];
	p1 = block[0] - block[1];
	p2 = block[2] + block[3];
	p3 = block[2] - block[3];

	tblock[0] = (p0 + p2);
	tblock[1] = (p1 + p3);
	tblock[2] = (p0 - p2);
	tblock[3] = (p1 - p3);
}

void Hadamard2x2_UV(int block[4][2], int tblock[4][2])
{
	int p0,p1,p2,p3;

	for (int i = 0; i < 2; i++)
	{
		p0 = block[0][i] + block[1][i];
		p1 = block[0][i] - block[1][i];
		p2 = block[2][i] + block[3][i];
		p3 = block[2][i] - block[3][i];

		tblock[0][i] = (p0 + p2);
		tblock[1][i] = (p1 + p3);
		tblock[2][i] = (p0 - p2);
		tblock[3][i] = (p1 - p3);
	}
}




void InvHadamard4x4(int16_t block[16], int16_t tblock[16])
{
	int i;
	int16_t tmp[16];
	int16_t *pTmp = tmp, *pblock;
	int p0,p1,p2,p3;
	int t0,t1,t2,t3;

	// Horizontal
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		pblock = &tblock[i*BLOCK_SIZE];
		t0 = *(pblock++);
		t1 = *(pblock++);
		t2 = *(pblock++);
		t3 = *(pblock  );

		p0 = t0 + t2;
		p1 = t0 - t2;
		p2 = t1 - t3;
		p3 = t1 + t3;

		*(pTmp++) = static_cast<int16_t>(p0 + p3);
		*(pTmp++) = static_cast<int16_t>(p1 + p2);
		*(pTmp++) = static_cast<int16_t>(p1 - p2);
		*(pTmp++) = static_cast<int16_t>(p0 - p3);
	}

	//  Vertical 
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		pTmp = tmp + i;
		t0 = *pTmp;
		t1 = *(pTmp += BLOCK_SIZE);
		t2 = *(pTmp += BLOCK_SIZE);
		t3 = *(pTmp += BLOCK_SIZE);

		p0 = t0 + t2;
		p1 = t0 - t2;
		p2 = t1 - t3;
		p3 = t1 + t3;

		pblock = &block[i];
		*(pblock)             = static_cast<int16_t>(p0 + p3);
		*(pblock+=BLOCK_SIZE) = static_cast<int16_t>(p1 + p2);
		*(pblock+=BLOCK_SIZE) = static_cast<int16_t>(p1 - p2);
		*(pblock+=BLOCK_SIZE) = static_cast<int16_t>(p0 - p3);
	}
}

void InvHadamard4x4_32(int block[16], int tblock[16])
{
	int i;
	int tmp[16];
	int *pTmp = tmp, *pblock;
	int p0,p1,p2,p3;
	int t0,t1,t2,t3;

	// Horizontal
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		pblock = &tblock[i*BLOCK_SIZE];
		t0 = *(pblock++);
		t1 = *(pblock++);
		t2 = *(pblock++);
		t3 = *(pblock  );

		p0 = t0 + t2;
		p1 = t0 - t2;
		p2 = t1 - t3;
		p3 = t1 + t3;

		*(pTmp++) = p0 + p3;
		*(pTmp++) = p1 + p2;
		*(pTmp++) = p1 - p2;
		*(pTmp++) = p0 - p3;
	}

	//  Vertical 
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		pTmp = tmp + i;
		t0 = *pTmp;
		t1 = *(pTmp += BLOCK_SIZE);
		t2 = *(pTmp += BLOCK_SIZE);
		t3 = *(pTmp += BLOCK_SIZE);

		p0 = t0 + t2;
		p1 = t0 - t2;
		p2 = t1 - t3;
		p3 = t1 + t3;

		pblock = &block[i];
		*(pblock)             = p0 + p3;
		*(pblock+=BLOCK_SIZE) = p1 + p2;
		*(pblock+=BLOCK_SIZE) = p1 - p2;
		*(pblock+=BLOCK_SIZE) = p0 - p3;
	}
}

void InvHadamard2x2(int block[4], int tblock[4])
{
	int t0,t1,t2,t3;

	t0 = tblock[0] + tblock[1];
	t1 = tblock[0] - tblock[1];
	t2 = tblock[2] + tblock[3];
	t3 = tblock[2] - tblock[3];

	block[0] = (t0 + t2);
	block[1] = (t1 + t3);
	block[2] = (t0 - t2);
	block[3] = (t1 - t3);
}

void InvHadamard2x2_UV(int tblock[4][2], int block[4][2])
{
	int p0,p1,p2,p3;

	for (int i = 0; i < 2; i++)
	{
		p0 = block[0][i] + block[1][i];
		p1 = block[0][i] - block[1][i];
		p2 = block[2][i] + block[3][i];
		p3 = block[2][i] - block[3][i];

		tblock[0][i] = (p0 + p2);
		tblock[1][i] = (p1 + p3);
		tblock[2][i] = (p0 - p2);
		tblock[3][i] = (p1 - p3);
	}
}

const uint8_t kLumaCBPTable[16] = { 0x1, 0x1, 0x2, 0x2, 
									0x1, 0x1, 0x2, 0x2,
									0x4, 0x4, 0x8, 0x8,
									0x4, 0x4, 0x8, 0x8};

bool SkipCheck(MacroBlockType *block)
{
	__declspec(align(16)) int16_t dct16x16[16][16];
	int qp_shift = block->qp / 6;
	int quant_offset = 3;

	DCT_SUB_16x16_SSE2_16(dct16x16, (uint8_t *) block->skip_dst_y_buffer, block->Y_frame_buffer);

	// AC processing for MB
	int16_t *qscaling = block->qparam->Quant4x4_Scale_2x[0][0][block->qp][0];
	//int *qinvscaling = block->qparam->Quant4x4_InvScale[0][0][block->qp][0];
	int *qoffsets = block->qparam->Quant4x4_Offset[block->qp][quant_offset];
	int nz[16];
	int non_zero = Quantize16x16_Luma_AC_16((int16_t *) dct16x16, qscaling, qoffsets, 15 + qp_shift, nz, false, 20);

	if (non_zero)
		return false;

	static const byte qp_chroma_scale[52]=
	{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		16,17,18,19,20,21,22,23,24,25, 26, 27, 28, 29, 29, 30,
		31,32,32,33,34,34,35,35,36,36, 37, 37, 37, 38, 38, 38,
		39,39,39,39
	};

	int c_qp = qp_chroma_scale[block->qp];
	qp_shift = c_qp / 6;
	__declspec(align(64)) int16_t dct8x8[2][8][8];
	__declspec(align(64)) int uv_dc2x2[2][4];

	DCT_SUB_8x8_UV_16_SSE2(dct8x8, (uint8_t *) block->skip_dst_uv_buffer, (uint8_t *) block->UV_frame_buffer);

	// pick out DC coeff
	for (int j = 0; j < 2; ++j)
		for (int i = 0; i < 2; ++i)
		{
			uv_dc2x2[0][(j*2)+i] = dct8x8[0][j << 2][i << 2];
			uv_dc2x2[1][(j*2)+i] = dct8x8[1][j << 2][i << 2];
		}

	Hadamard2x2(uv_dc2x2[0], uv_dc2x2[0]);
	Hadamard2x2(uv_dc2x2[1], uv_dc2x2[1]);

	non_zero = Quantize4x4_DC_UV_SSE2(uv_dc2x2, block->qparam->Quant4x4_Scale[0][0][c_qp][0][0],  block->qparam->Quant4x4_Offset[c_qp][quant_offset][0], 16 + qp_shift);

	if (non_zero)
	{
		return false;
	}
	else
	{
		dct8x8[0][0][0] = dct8x8[0][0][4] = dct8x8[0][4][0] = dct8x8[0][4][4] = 0;
		dct8x8[1][0][0] = dct8x8[1][0][4] = dct8x8[1][4][0] = dct8x8[1][4][4] = 0;
	}

	// AC processing for MB
	int16_t *qscaling_16 = block->qparam->Quant4x4_Scale_2x[0][0][c_qp][0];
	qoffsets = block->qparam->Quant4x4_Offset[c_qp][quant_offset];

	non_zero = Quantize8x16_Chroma_AC_16((int16_t *) dct8x8, qscaling_16, qoffsets[0], 15 + qp_shift, nz);

	if (non_zero)
		return false;

	return true;
}

int ProcessResidual_16x16_Luma(MacroBlockType *block)
{
	int cbp_luma = 0;	// Coded Block Pattern Luma (0 or 15)
	__declspec(align(16)) int16_t dct16x16[16][16];
	__declspec(align(16)) int dc4x4[16];
	int qp_shift = block->qp / 6;
	int quant_offset = 3;

#ifdef USE_SSE2
	DCT_SUB_16x16_SSE2_16(dct16x16, (uint8_t *) block->Y_pred_pixels->block, block->Y_frame_buffer);
#else
	// forward 4x4 integer transform - 
	for (int j = 0; j < 16; j+=4)
	{
		for (int i = 0; i < 16; i+=4)
		{	
			DCT_SUB_4x4(dct16x16, block->Y_pred_pixels->block, block->Y_frame_buffer, block->stride, j, i);
		}
	}
#endif
	if (block->isI16x16)
	{
		quant_offset = 0;
		// pick out DC coeff
		for (int j = 0; j < 4; ++j)
			for (int i = 0; i < 4; ++i)
				dc4x4[(j*4)+i] = dct16x16[j << 2][i << 2];
		Hadamard4x4(dc4x4, dc4x4);

#ifdef USE_SSE2
	int non_zero = Quantize4x4_DC_32_SSE2(dc4x4, block->qparam->Quant4x4_Scale[0][0][block->qp][0][0],  block->qparam->Quant4x4_Offset[block->qp][0][0], 16 + qp_shift);
#else
	int non_zero = Quantize4x4_DC_32(dc4x4, block->qparam->Quant4x4_Scale[0][0][block->qp][0][0],  block->qparam->Quant4x4_Offset[block->qp][0][0], 16 + qp_shift);
#endif

		// inverse DC transform
		if (non_zero)
		{
			block->luma_bit_flags |= LUMA_DC_BIT;
			block->cbf_bits[0] |= (int64_t) 0x01;
			block->coeff_cost += RL_ZigZagScan_4x4_32(dc4x4, block->data->level_run_dc);
			InvHadamard4x4_32(dc4x4, dc4x4);

			int inv_scale = block->qparam->Quant4x4_InvScale[0][0][block->qp][0][0];
			// inverse quantization for the DC coefficients
			for (int j = 0; j < 16; j += BLOCK_SIZE)
			{
				for (int i = 0; i < 16;i += BLOCK_SIZE)
				{
					dct16x16[j][i] = static_cast<int16_t>((((dc4x4[(j & 0xfffc) + (i>>2)] * inv_scale) << qp_shift) + (1 << 5)) >> 6);
				}
			}
		}
		else
		{
			for (int j = 0; j < 16; j += BLOCK_SIZE)
			{
				for (int i = 0; i < 16;i += BLOCK_SIZE)
				{
					dct16x16[j][i] = 0;
				}
			}
		}
	}

	// AC processing for MB
	int16_t *qscaling = block->qparam->Quant4x4_Scale_2x[0][0][block->qp][0];
	int *qinvscaling = block->qparam->Quant4x4_InvScale[0][0][block->qp][0];
	int *qoffsets = block->qparam->Quant4x4_Offset[block->qp][quant_offset];

	int cbp_index = 0;

#if 1	// new 16x16 Quantization

#if 0	// testing
	__declspec(align(16)) int16_t dct16x16_test[16][16];

	for (int jj = 0; jj < 16; jj+=1)
	{
		for (int ii = 0; ii < 16; ii += 1)
		{
			dct16x16_test[jj][ii] = dct16x16[jj][ii];
		}
	}


	int nz_test[16];
	int non_zero_test = Quantize16x16_Luma_AC_16_SSSE3((int16_t *) dct16x16_test, qscaling, qoffsets, 15 + qp_shift, nz_test, block->isI16x16, 20);

	int nz[16];
	int non_zero = Quantize16x16_Luma_AC_16_SSE2((int16_t *) dct16x16, qscaling, qoffsets, 15 + qp_shift, nz, block->isI16x16, 20);

	// verify
	ENC_ASSERT(non_zero == non_zero_test);

	for (int jj = 0; jj < 16; jj+=1)
	{
		for (int ii = 0; ii < 16; ii += 1)
		{
			ENC_ASSERT(dct16x16[jj][ii] == dct16x16_test[jj][ii]);
		}
	}

	for (int i = 0; i < 16; i++)
	{
		ENC_ASSERT(nz[i] == nz_test[i]);
	}
#else
	int nz[16];
	int non_zero = Quantize16x16_Luma_AC_16((int16_t *) dct16x16, qscaling, qoffsets, 15 + qp_shift, nz, block->isI16x16, 20);
#endif

	if (block->isI16x16 || non_zero)
	{
		for (int jj = 0; jj < 4; jj++)
		{
			if (block->isI16x16 || (non_zero & (1 << jj)))
			{
				int jpos = (jj << 2);
				for (int ii = 0; ii < 4; ii++)
				{
					int ipos = (ii << 2);

					if (nz[cbp_index])
					{
						cbp_luma |= kLumaCBPTable[cbp_index];
						block->luma_bit_flags |= (1L << (jpos + ii));
						block->cbf_bits[0] |= ((int64_t) 0x01 << (jpos + ii + 1));

						block->coeff_cost += RL_ZigZagScan_Sub_4x4_AC_16(&dct16x16[jpos][ipos], block->data->level_run_ac[jj][ii], qinvscaling, qp_shift, block->isI16x16);
					}

					//inverse transform
					if (dct16x16[jpos][ipos] != 0 || nz[cbp_index])
						Inv_DCT_4x4(dct16x16, jpos, ipos);

					cbp_index++;
				}
			}
			else
				cbp_index += 4;
		}
	}

#else
	for (int jj = 0; jj < 4; jj++)
	{
		int jpos = (jj << 2);
		for (int ii = 0; ii < 4; ii += 2)
		{
			int non_zero;
			int ipos = (ii << 2);

			// Quantization process
#ifdef USE_SSE2
			if (block->isI16x16)
			{
				int save = dct16x16[jpos][ipos];
				dct16x16[jpos][ipos] = 0;
#if 1
                non_zero = Quantize8x4_Luma_AC_16_SSE2(&dct16x16[jpos][ipos], qscaling, qoffsets, 15 + qp_shift, 16, block->isI16x16, 11);
#else
				non_zero = Quantize4x4_Luma_AC_32(&dct16x16[jpos][ipos], qscaling, qoffsets, 15 + qp_shift, 16, block->isI16x16, 11);
#endif
				dct16x16[jpos][ipos] = save;
			}
			else
            {
#if 1
                non_zero = Quantize8x4_Luma_AC_16_SSE2(&dct16x16[jpos][ipos], qscaling, qoffsets, 15 + qp_shift, 16, block->isI16x16, 11);
#else
				non_zero = Quantize4x4_Luma_AC_32(&dct16x16[jpos][ipos], qscaling, qoffsets, 15 + qp_shift, 16, block->isI16x16, 11);
#endif
            }

#else
			non_zero = Quantize4x4_AC_32(&dct16x16[jpos][ipos], qscaling, qoffsets, 15 + qp_shift, 16, block->isI16x16);
#endif

			if (non_zero)
			{
				cbp_luma |= kLumaCBPTable[cbp_index];
				block->luma_bit_flags |= (1L << (jpos + ii));
				block->cbf_bits[0] |= ((int64_t) 0x01 << (jpos + ii + 1));

				block->coeff_cost += RL_ZigZagScan_Sub_4x4_AC_16(&dct16x16[jpos][ipos], block->data->level_run_ac[jj][ii], qinvscaling, qp_shift, block->isI16x16);
			}

			cbp_index++;

			//inverse transform
			if (dct16x16[jpos][ipos] != 0 || non_zero)
				Inv_DCT_4x4(dct16x16, jpos, ipos);
//				Inv_DCT_4x4_32(dct16x16, jpos, ipos);
			else
			{	// zero out pixels 1-15
#if 1
				int16_t *p32 = (int16_t *) &dct16x16[jpos][ipos];
#else
				uint32_t *p32 = (uint32_t *) &dct16x16[jpos][ipos];
#endif
				p32[1] = p32[2] = p32[3] = 0;
				p32[16] = p32[17] = p32[18] = p32[19] = 0;
				p32[32] = p32[33] = p32[34] = p32[35] = 0;
				p32[48] = p32[49] = p32[50] = p32[51] = 0;
			}
		}
	}
#endif

	// add the predicted value to the residual values to get the final values the decoder will see for accurate predictions later during encoding
#ifdef USE_SSE2
	__m128i mm0, lo_pixels, hi_pixels;
	__m128i mrnd = _mm_set1_epi16(1 << 5);
	__m128i zero_mm = _mm_setzero_si128();
	__m128i clip_max = _mm_set1_epi16(255);
	__m128i src1, blk;

	for (int j = 0; j < 16; j++)
	{
		uint8_t *pred = block->Y_pred_pixels->block[j];

		int block_start_y = j * block->stride + block->Y_block_start_index;

		blk = _mm_load_si128((__m128i*)pred);
		src1 = _mm_load_si128((__m128i*)&dct16x16[j][0]);
		// first 8 pixels - pack into 16-bits
		src1 = _mm_add_epi16(src1, mrnd);
		src1 = _mm_srai_epi16(src1, 6);
		mm0  = _mm_unpacklo_epi8(blk, zero_mm);	// convert pred pixels to 16-bit
		lo_pixels = _mm_add_epi16(src1, mm0);	// add the pred pixels
		lo_pixels = _mm_min_epi16(clip_max, lo_pixels);	// clip hi
		lo_pixels = _mm_max_epi16(zero_mm, lo_pixels);	// clip low

		// now work on next 8 pixels
		src1 = _mm_load_si128((__m128i*)&dct16x16[j][8]);
		src1 = _mm_add_epi16(src1, mrnd);
		src1 = _mm_srai_epi16(src1, 6);
		mm0  = _mm_unpackhi_epi8(blk, zero_mm);	// convert pred pixels to 16-bit
		hi_pixels = _mm_add_epi16(src1, mm0);	// add the pred pixels
		hi_pixels = _mm_min_epi16(clip_max, hi_pixels);	// clip hi
		hi_pixels = _mm_max_epi16(zero_mm, hi_pixels);	// clip low

		mm0  = _mm_packus_epi16(lo_pixels,hi_pixels);	// pack pixels into 16 8-bit unsigned integers

		_mm_store_si128((__m128i *) &block->Y_frame_reconstructed[block_start_y], mm0);	// store 16 pixels
	}
#else
	for (int j = 0; j < 16; j++)
	{
        int block_start_y = j * block->stride + block->Y_block_start_index;
		for (int i = 0; i < 16; i++)
		{
			block->Y_frame_reconstructed[block_start_y + i] = CLIP3(0, 0xff, ((dct16x16[j][i] + (1 << 5)) >> 6) + block->Y_pred_pixels->block[j][i]);
		}
	}
#endif

#if 0 // debug
	OutputDebugString("Luma:\n");
	for (int j = 0; j < 16; j++)
	{
		int block_start_y = j * block->stride + block->Y_block_start_index;
		for (int i = 0; i < 16; i++)
		{
			char str[256];
			sprintf_s(str, "0x%2x ", block->Y_frame_reconstructed[block_start_y + i]);
			OutputDebugString(str);
		}
		OutputDebugString("\n");
	}
#endif

	if (block->isI16x16 && cbp_luma)
		cbp_luma = 15;
	
	return cbp_luma;
}
// 
int ProcessResidual_8x8_Chroma(MacroBlockType *block)
{
	static const byte qp_chroma_scale[52]=
	{
		 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		16,17,18,19,20,21,22,23,24,25, 26, 27, 28, 29, 29, 30,
		31,32,32,33,34,34,35,35,36,36, 37, 37, 37, 38, 38, 38,
		39,39,39,39
	};

	int cbp_chroma = 0;	// Coded Block Pattern Chroma (0, 1, 2)
	int c_qp = qp_chroma_scale[block->qp];
	int qp_shift = c_qp / 6;
	__declspec(align(64)) int16_t dct8x8[2][8][8];
	__declspec(align(64)) int uv_dc2x2[2][4];
	int quant_offset = (block->isI16x16) ? 0 : 3;

#ifdef USE_SSE2
	DCT_SUB_8x8_UV_16_SSE2(dct8x8, (uint8_t *) block->UV_pred_pixels->block, (uint8_t *) block->UV_frame_buffer);
#if 0	// for verification
	DCT_SUB_8x8_UV_SSE2(dct8x8, block->UV_pred_pixels->block, block->UV_frame_buffer, MB_BLOCK_STRIDE/2);
	for (int j = 0; j < 8; j += 1)
	{
		for (int i = 0; i < 8; i += 1)
		{
			ENC_ASSERT(dct8x8[0][j][i] == dct8x8_16[0][j][i]);
			ENC_ASSERT(dct8x8[1][j][i] == dct8x8_16[1][j][i]);
		}
	}
#endif
#else
	// forward 4x4 integer transform - 
	for (int j = 0; j < 8; j += 4)
	{
		for (int i = 0; i < 8; i += 4)
		{
			DCT_SUB_4x4_UV(dct8x8, block->UV_pred_pixels->block, block->UV_frame_buffer, MB_BLOCK_STRIDE / 2, j, i);
		}
	}
#endif
	// pick out DC coeff
	for (int j = 0; j < 2; ++j)
		for (int i = 0; i < 2; ++i)
		{
			uv_dc2x2[0][(j*2)+i] = dct8x8[0][j << 2][i << 2];
			uv_dc2x2[1][(j*2)+i] = dct8x8[1][j << 2][i << 2];
		}

	Hadamard2x2(uv_dc2x2[0], uv_dc2x2[0]);
	Hadamard2x2(uv_dc2x2[1], uv_dc2x2[1]);

#ifdef USE_SSE2
	int non_zero = Quantize4x4_DC_UV_SSE2(uv_dc2x2, block->qparam->Quant4x4_Scale[0][0][c_qp][0][0],  block->qparam->Quant4x4_Offset[c_qp][quant_offset][0], 16 + qp_shift);// don't take QP into acct now//Q_BITS + (block->qp / 6));
#else
	int non_zero = Quantize4x4_DC_UV(uv_dc2x2, block->qparam->Quant4x4_Scale[0][0][block->qp][0][0],  block->qparam->Quant4x4_Offset[block->qp][0][0], 16 + qp_shift);// don't take QP into acct now//Q_BITS + (block->qp / 6));
#endif

	// inverse DC transform
	if (non_zero)
	{
		int coeff_cost = 0;
		if (RL_ZigZagScan_2x2(uv_dc2x2[0], block->data->level_run_dc_uv[0], &coeff_cost))
		{
			block->cbf_bits[0] |= ((int64_t) 0x01 << 17);
			block->coeff_cost += coeff_cost;
		}
		if (RL_ZigZagScan_2x2(uv_dc2x2[1], block->data->level_run_dc_uv[1], &coeff_cost))
		{
			block->cbf_bits[0] |= ((int64_t) 0x01 << 18);
			block->coeff_cost += coeff_cost;
		}

		int inv_scale = block->qparam->Quant4x4_InvScale[0][0][c_qp][0][0];
		// inverse quantization for the DC coefficients
		for (int j = 0; j < 2; j++)
		{
			for (int i = 0; i < 2; i++)
			{
				uv_dc2x2[0][(j << 1) + i] = (((uv_dc2x2[0][(j << 1) + i]) * inv_scale) << qp_shift);
				uv_dc2x2[1][(j << 1) + i] = (((uv_dc2x2[1][(j << 1) + i]) * inv_scale) << qp_shift);
			}
		}
		InvHadamard2x2(uv_dc2x2[0], uv_dc2x2[0]);
		InvHadamard2x2(uv_dc2x2[1], uv_dc2x2[1]);

		// inverse quantization for the DC coefficients
		for (int j = 0; j < 8; j += BLOCK_SIZE)
		{
			for (int i = 0; i < 8;i += BLOCK_SIZE)
			{
				dct8x8[0][j][i] = static_cast<int16_t>(((uv_dc2x2[0][(j >> 1) + (i>>2)]) + (1 << 4)) >> 5);
				dct8x8[1][j][i] = static_cast<int16_t>(((uv_dc2x2[1][(j >> 1) + (i>>2)]) + (1 << 4)) >> 5);
			}
		}

		cbp_chroma = 1;	// mark as DC non-zero
		block->chroma_bit_flags |= CHROMA_DC_BIT;
	}
	else
	{
		dct8x8[0][0][0] = dct8x8[0][0][4] = dct8x8[0][4][0] = dct8x8[0][4][4] = 0;
		dct8x8[1][0][0] = dct8x8[1][0][4] = dct8x8[1][4][0] = dct8x8[1][4][4] = 0;
	}

	// AC processing for MB
	int16_t *qscaling_16 = block->qparam->Quant4x4_Scale_2x[0][0][c_qp][0];
	int *qinvscaling = block->qparam->Quant4x4_InvScale[0][0][c_qp][0];
	int *qoffsets = block->qparam->Quant4x4_Offset[c_qp][quant_offset];

#if 1
#if 0 // verify
	__declspec(align(64)) int16_t dct8x8_test[2][8][8];
	for (int jj = 0; jj < 8; jj+=1)
	{
		for (int ii = 0; ii < 8; ii += 1)
		{
			dct8x8_test[0][jj][ii] = dct8x8[0][jj][ii];
			dct8x8_test[1][jj][ii] = dct8x8[1][jj][ii];
		}
	}
	int nz_test[8];
	int non_zero_test = Quantize8x16_Chroma_AC_16_SSSE3((int16_t *) dct8x8_test, qscaling_16, qoffsets[0], 15 + qp_shift, nz_test);
	int nz[8];
	non_zero = Quantize8x16_Chroma_AC_16_SSE2((int16_t *) dct8x8, qscaling_16, qoffsets[0], 15 + qp_shift, nz);

	ENC_ASSERT(non_zero == non_zero_test);
	for (int jj = 0; jj < 8; jj+=1)
	{
		for (int ii = 0; ii < 8; ii += 1)
		{
			ENC_ASSERT(dct8x8_test[0][jj][ii] == dct8x8[0][jj][ii]);
			ENC_ASSERT(dct8x8_test[1][jj][ii] == dct8x8[1][jj][ii]);
		}

		ENC_ASSERT(nz[jj] == nz_test[jj]);
	}
#else
	int nz[8];
	non_zero = Quantize8x16_Chroma_AC_16((int16_t *) dct8x8, qscaling_16, qoffsets[0], 15 + qp_shift, nz);
#endif

	int ac_bit_shift = 19;
	int block_index = 0;

	for (int pl = 0; pl < 2; pl++, ac_bit_shift = 35)
	{
		for (int jj = 0; jj < 2; jj++)
		{
			int jpos = (jj << 2);
			for (int ii = 0; ii < 2; ii++, block_index++)
			{
				int ipos = (ii << 2);

				if (nz[block_index])
				{
					cbp_chroma |= 2;	// flag as AC non-zero
					block->chroma_bit_flags |= (1L << ((pl * 4) + (jj*2)+ii));
					block->cbf_bits[0] |= ((int64_t) 0x01 << (ac_bit_shift + (jpos+ii)));

					block->coeff_cost += RL_ZigZagScan_Sub_4x4_AC_UV_16(&dct8x8[pl][jpos][ipos], block->data->level_run_ac_uv[pl][jj][ii], qinvscaling, qp_shift);
				}

				//inverse transform
				if (dct8x8[pl][jpos][ipos] != 0 || nz[block_index])
					Inv_DCT_4x4_UV_16(dct8x8[pl], jpos, ipos);
			}
		}
	}

	__m128i mm0, lo_pixels, hi_pixels;
	__m128i mrnd = _mm_set1_epi16(1 << 5);
	__m128i zero_mm = _mm_setzero_si128();
	__m128i clip_max = _mm_set1_epi16(255);
	__m128i blk;

	for (int j = 0; j < 8; j++)
	{
		ChromaPixelType *pred = block->UV_pred_pixels->block[j];

		int block_start_y = j * block->uv_stride + block->UV_block_start_index;

		blk = _mm_load_si128((__m128i*)pred);
		__m128i src1 = _mm_load_si128((__m128i*)&dct8x8[0][j][0]);
		__m128i src2 = _mm_load_si128((__m128i*)&dct8x8[1][j][0]);

		// first 8 pixels -
		src1 = _mm_add_epi16(src1, mrnd);
		src1 = _mm_srai_epi16(src1, 6);
		src2 = _mm_add_epi16(src2, mrnd);
		src2 = _mm_srai_epi16(src2, 6);
		lo_pixels  = _mm_unpacklo_epi16(src1, src2);	// interleave u and v pixels
		mm0  = _mm_unpacklo_epi8(blk, zero_mm);			// convert low 8 pred pixels to 16-bit
		lo_pixels = _mm_add_epi16(lo_pixels, mm0);		// add the pred pixels
		lo_pixels = _mm_min_epi16(clip_max, lo_pixels);	// clip hi
		lo_pixels = _mm_max_epi16(zero_mm, lo_pixels);	// clip low

		// now work on next 8 pixels
		hi_pixels  = _mm_unpackhi_epi16(src1, src2);	// interleave u and v pixels
		mm0  = _mm_unpackhi_epi8(blk, zero_mm);			// convert high 8 pred pixels to 16-bit
		hi_pixels = _mm_add_epi16(hi_pixels, mm0);		// add the pred pixels
		hi_pixels = _mm_min_epi16(clip_max, hi_pixels);	// clip hi
		hi_pixels = _mm_max_epi16(zero_mm, hi_pixels);	// clip low

		mm0  = _mm_packus_epi16(lo_pixels,hi_pixels);	// pack pixels into 16 8-bit unsigned integers

		_mm_store_si128((__m128i *) &block->UV_frame_reconstructed[block_start_y], mm0);	// store 16 pixels
	}


#else
	int *qscaling = block->qparam->Quant4x4_Scale[0][0][c_qp][0];
	int ac_bit_shift = 19;
	for (int pl = 0; pl < 2; pl++, ac_bit_shift = 35)
	{
		for (int jj = 0; jj < 2; jj++)
		{
			int jpos = (jj << 2);
			for (int ii = 0; ii < 2; ii++)
			{
				int ipos = (ii << 2);

				// Quantization process
	#ifdef USE_SSE2
				int save = dct8x8[pl][jpos][ipos];
				dct8x8[pl][jpos][ipos] = 0;
#if 0 // use dead-zone quant for chroma
				non_zero = Quantize4x4_Luma_AC_32_SSE2(&dct8x8[pl][jpos][ipos], qscaling, qoffsets, 15 + qp_shift, 8, 1, 23);
#else
				non_zero = Quantize4x4_AC_32_SSE2(&dct8x8[pl][jpos][ipos], qscaling, qoffsets, 15 + qp_shift, 8);
#endif
				dct8x8[pl][jpos][ipos] = save;
	#else
				non_zero = Quantize4x4_AC_32(&dct8x8[pl][jpos][ipos], qscaling, qoffsets, 15 + qp_shift, 8);
	#endif

				if (non_zero)
				{
					static const int bit_shift[2] = {19 , 35};
					cbp_chroma |= 2;	// flag as AC non-zero
					block->chroma_bit_flags |= (1L << ((pl * 4) + (jj*2)+ii));
					block->cbf_bits[0] |= ((int64_t) 0x01 << (ac_bit_shift + (jpos+ii)));

					block->coeff_cost += RL_ZigZagScan_Sub_4x4_AC_UV(&dct8x8[pl][jpos][ipos], block->data->level_run_ac_uv[pl][jj][ii], qinvscaling, qp_shift);
				}

				//inverse transform
				if (dct8x8[pl][jpos][ipos] != 0 || non_zero)
					Inv_DCT_4x4_UV(dct8x8[pl], jpos, ipos);
#if 0
				else
				{	// zero out pixels 1-15
					uint32_t *p32 = (uint32_t *) &dct8x8[pl][jpos][ipos];
					p32[1] = p32[2] = p32[3] = 0;
					p32[8] = p32[9] = p32[10] = p32[11] = 0;
					p32[16] = p32[17] = p32[18] = p32[19] = 0;
					p32[24] = p32[25] = p32[26] = p32[27] = 0;
				}
#endif
			}
		}
	}
	// add the predicted value to the residual values to get the final values the decoder will see for accurate predictions later during encoding
#ifdef USE_SSE2
	__m128i mm0, lo_pixels, hi_pixels;
	__m128i mrnd = _mm_set1_epi16(1 << 5);
	__m128i zero_mm = _mm_setzero_si128();
	__m128i clip_max = _mm_set1_epi16(255);
	__m128i mask = _mm_set1_epi32(0x0000ffff);

	for (int j = 0; j < 8; j++)
	{
		ChromaPixelType *pred = block->UV_pred_pixels->block[j];

		int block_start_y = j * block->uv_stride + block->UV_block_start_index;

		__m128i src1 = _mm_load_si128((__m128i*)&dct8x8[0][j][0]);
		__m128i src2 = _mm_load_si128((__m128i*)&dct8x8[1][j][0]);
		__m128i blk = _mm_load_si128((__m128i*)pred);
		// first 8 pixels - pack into 16-bits
		src1 = _mm_and_si128(src1, mask);	// mask to interleave u words
		src2 = _mm_slli_epi32(src2, 16);	// shift logical left 16-bits to put v words in position
		src1 = _mm_or_si128(src1, src2);	// now or them to interleave
		src1 = _mm_add_epi16(src1, mrnd);
		src1 = _mm_srai_epi16(src1, 6);
		mm0  = _mm_unpacklo_epi8(blk, zero_mm);	// convert pred pixels to 16-bit
		lo_pixels = _mm_add_epi16(src1, mm0);	// add the pred pixels
		lo_pixels = _mm_min_epi16(clip_max, lo_pixels);	// clip hi
		lo_pixels = _mm_max_epi16(zero_mm, lo_pixels);	// clip low

		// now work on next 8 pixels
		src1 = _mm_load_si128((__m128i*)&dct8x8[0][j][4]);
		src2 = _mm_load_si128((__m128i*)&dct8x8[1][j][4]);
		src1 = _mm_and_si128(src1, mask);	// mask to interleave u words
		src2 = _mm_slli_epi32(src2, 16);	// shift logical left 16-bits to put v words in position
		src1 = _mm_or_si128(src1, src2);	// now or them to interleave
		src1 = _mm_add_epi16(src1, mrnd);
		src1 = _mm_srai_epi16(src1, 6);
		mm0  = _mm_unpackhi_epi8(blk, zero_mm);	// convert pred pixels to 16-bit
		hi_pixels = _mm_add_epi16(src1, mm0);	// add the pred pixels
		hi_pixels = _mm_min_epi16(clip_max, hi_pixels);	// clip hi
		hi_pixels = _mm_max_epi16(zero_mm, hi_pixels);	// clip low

		mm0  = _mm_packus_epi16(lo_pixels,hi_pixels);	// pack pixels into 16 8-bit unsigned integers

		_mm_store_si128((__m128i *) &block->UV_frame_reconstructed[block_start_y], mm0);	// store 16 pixels
	}
#else
	for (int j = 0; j < 8; j++)
	{
        int block_start_y = j * block->uv_stride + block->UV_block_start_index;
		for (int i = 0; i < 8; i++)
		{
#ifdef VERIFY_DCT_QUANT
			int test = ((dct8x8[j][i] + (1 << 5)) >> 6)  + block->UV_pred_pixels[u_or_v][j][i];
			ENC_ASSERT((test >= 0 ) && (test <= 255 ));
#endif

			block->UV_frame_reconstructed[block_start_y + i].u = CLIP3(0, 0xff, ((dct8x8[0][j][i] + (1 << 5)) >> 6) + block->UV_pred_pixels->block[j][i].u);
			block->UV_frame_reconstructed[block_start_y + i].v = CLIP3(0, 0xff, ((dct8x8[1][j][i] + (1 << 5)) >> 6) + block->UV_pred_pixels->block[j][i].v);
		}
	}
#endif
#endif

#if 0	// debug
	OutputDebugString("Chroma out U:\n");
	for (int j = 0; j < 8; j++)
	{
		int block_start_y = j * block->uv_stride + block->UV_block_start_index;
		for (int i = 0; i < 8; i++)
		{
			char str[256];
			sprintf_s(str, "0x%2x ", block->UV_frame_reconstructed[block_start_y + i].u);
			OutputDebugString(str);
		}
		OutputDebugString("\n");
	}
	OutputDebugString("Chroma out V:\n");
	for (int j = 0; j < 8; j++)
	{
		int block_start_y = j * block->uv_stride + block->UV_block_start_index;
		for (int i = 0; i < 8; i++)
		{
			char str[256];
			sprintf_s(str, "0x%2x ", block->UV_frame_reconstructed[block_start_y + i].v);
			OutputDebugString(str);
		}
		OutputDebugString("\n");
	}
#endif

	return cbp_chroma;
}


MacroBlockType *HasAdjacent4x4SubBlock(MacroBlockType *block, int direction, int *subx, int *suby, bool luma)
{
	int dim = (luma ? 4: 2);

	if (direction == ADJACENT_MB_LEFT)
	{
		if (*subx == 0)	// need to look at another mb?
		{
			if (block->mb_x > 0)
			{
				*subx = dim - 1;
				return block->mbAddrA;
			}
		}
		else
		{
			*subx -= 1;
			return block;
		}
	}
	else
	{	// ABOVE
		if (*suby == 0)	// need to look at another mb?
		{
			if (block->mb_y > 0)
			{
				*suby = dim - 1;
				return block->mbAddrB;
			}
		}
		else
		{
			*suby -= 1;
			return block;
		}
	}

	return NULL;
}

bool HasAdjacentMacroBlock(MacroBlockType *block, int direction, int *adj_mb_type)
{
	if (direction == ADJACENT_MB_LEFT)
	{
		if ((block->mb_x > 0) && block->mbAddrA && AVAILABLE(block->mbAddrA))
		{
			*adj_mb_type = block->mbAddrA->mb_type;
			return true;
		}
	}
	else
	{	// ABOVE
		if ((block->mb_y > 0) && block->mbAddrB && AVAILABLE(block->mbAddrB))
		{
			*adj_mb_type = block->mbAddrB->mb_type;
			return true;
		}
	}

	return false;
}

bool HasAdjacentMacroBlock_cpm(MacroBlockType *block, int direction, int *adj_chroma_pred_mode)
{
	if (direction == ADJACENT_MB_LEFT)
	{
		if ((block->mb_x > 0) && block->mbAddrA && AVAILABLE(block->mbAddrA))
		{
			*adj_chroma_pred_mode = block->mbAddrA->chroma_pred_mode;
			return true;
		}
	}
	else
	{	// ABOVE
		if ((block->mb_y > 0) && block->mbAddrB && AVAILABLE(block->mbAddrB))
		{
			*adj_chroma_pred_mode = block->mbAddrB->chroma_pred_mode;
			return true;
		}
	}

	return false;
}

void BuildMB_Type(MacroBlockType *mb)
{
	// build mb_type (from Table 9-36)
	if ((gEncoderState.slice_type == SLICE_TYPE_I) || (gEncoderState.slice_type == SLICE_TYPE_SI))
	{
		mb->mb_type = 1;	// not I_4x4 or I_8x8 yet

		if (mb->coded_block_pattern_luma)
			mb->mb_type += 12;
		mb->mb_type += mb->intra_pred_mode + (mb->coded_block_pattern_chroma * 4);
	}
	else
	if ((gEncoderState.slice_type == SLICE_TYPE_P) || (gEncoderState.slice_type == SLICE_TYPE_SP))
	{
		mb->mb_type = 6;	// 5..25
		if (mb->coded_block_pattern_luma)
			mb->mb_type += 12;
		mb->mb_type += mb->intra_pred_mode + (mb->coded_block_pattern_chroma * 4);
	}
	else
	if (gEncoderState.slice_type == SLICE_TYPE_B)
	{
		mb->mb_type = 24;	// 23..48 (23 is I_NxN so skip over)
		if (mb->coded_block_pattern_luma)
			mb->mb_type += 12;
		mb->mb_type += mb->intra_pred_mode + (mb->coded_block_pattern_chroma * 4);
	}
	else
	{
		ENC_ASSERT(0);	// need to support something
	}
}

// duplicate motion vectors x and y for all sub-blocks for now (change when we implement sub-blocks)
void SetMotionVectors16x16(uint32_t *ptr, int x, int y)
{
	uint32_t value_x = x << 16 | (0xffff & x);
	uint32_t value_y = y << 16 | (0xffff & y);

	*ptr++ = value_x;
	*ptr++ = value_x;
	*ptr++ = value_x;
	*ptr++ = value_x;
	*ptr++ = value_x;
	*ptr++ = value_x;
	*ptr++ = value_x;
	*ptr++ = value_x;

	*ptr++ = value_y;
	*ptr++ = value_y;
	*ptr++ = value_y;
	*ptr++ = value_y;
	*ptr++ = value_y;
	*ptr++ = value_y;
	*ptr++ = value_y;
	*ptr++ = value_y;
}

void SetMotionVectorDeltas16x16(uint32_t *ptr, int x, int y)
{
	uint32_t value_x = x << 16 | (0xffff & x);
	uint32_t value_y = y << 16 | (0xffff & y);

	*ptr++ = value_x;
	*ptr++ = value_x;
	*ptr++ = value_x;
	*ptr++ = value_x;
	*ptr++ = value_x;
	*ptr++ = value_x;
	*ptr++ = value_x;
	*ptr++ = value_x;

	*ptr++ = value_y;
	*ptr++ = value_y;
	*ptr++ = value_y;
	*ptr++ = value_y;
	*ptr++ = value_y;
	*ptr++ = value_y;
	*ptr++ = value_y;
	*ptr++ = value_y;
}

MacroBlockType *GetMacroBlock(int index)
{
	ENC_ASSERT((index >= 0) && (index < gEncoderState.mb_max));

	return &sCurrentMacroBlockSet[index];
}


//#define BUFFER_STR_OUTPUT
#ifdef BUFFER_STR_OUTPUT
static int sOutStrCount = 0;
static char out_str[300][256];
#define OUTPUT_STR(x) strcpy_s(out_str[sOutStrCount++], (x))
#else
#define OUTPUT_STR(x)// OutputDebugString((x))
#endif


void OutputResidual(uint8_t *prev, uint8_t *buffer, int stride, int plane)
{
	int diff = 0;
	char str[256];

	if (plane == 0)	// luma
	{
		int16_t *midABS_Table = &gABS_Table[1<<15];

		OUTPUT_STR("Luma:\n");

		for (int y = 0; y < 16; y++)
		{
			for (int x = 0; x < 16; x++)
			{
				diff = midABS_Table[(prev[(y*stride)+x] - buffer[(y*stride)+x])];
				sprintf_s(str, "%3d (%3d-%3d) ",diff, prev[(y*stride)+x], buffer[(y*stride)+x]);
				OUTPUT_STR(str);
			}
			OUTPUT_STR("\n");
		}
	}
	else	// chroma
	{
		int16_t *midABS_Table = &gABS_Table[1<<15];

		OUTPUT_STR("Chroma:\n");

		for (int y = 0; y < 8; y++)
		{
			for (int x = 0; x < 16; x++)
			{
				diff = midABS_Table[(prev[(y*stride)+x] - buffer[(y*stride)+x])];
				sprintf_s(str, "%3d (%3d-%3d) ",diff, prev[(y*stride)+x], buffer[(y*stride)+x]);
				OUTPUT_STR(str);
			}
			OUTPUT_STR("\n");
		}
	}
}

#define LUMA_TOLERANCE 1
#define CHROMA_TOLERANCE 3

int CompareBlockToFrame_Luma_SSE2(uint8_t *src, uint8_t *blk_src, int stride)	// blk_src must be 16-byte aligned
{
	int result;
	__m128i mm_src, mm_blk, mm0, mm1;
    int fullLuma = 0xff;
	__m128i mask = _mm_set1_epi8(static_cast<char>(fullLuma << LUMA_TOLERANCE));	// mask out lsb
	__m128i zero = _mm_setzero_si128();

	for (int i = 0; i < 16; i++)
	{
		mm_src = _mm_loadu_si128((__m128i *) src);
		mm_blk = _mm_load_si128((__m128i *) blk_src);
		mm_src = _mm_and_si128(mm_src, mask);
		mm_blk = _mm_and_si128(mm_blk, mask);
		mm_src = _mm_srli_epi32(mm_src, LUMA_TOLERANCE);	// shift right 1 bit 
		mm_blk = _mm_srli_epi32(mm_blk, LUMA_TOLERANCE);	// shift right 1 bit 
		mm0    = _mm_sub_epi8(mm_src, mm_blk);
		mm1    = _mm_cmpeq_epi8(mm0, zero);
		result = _mm_movemask_epi8(mm1);	// make sure all are set which means all are under 2 since we shifted the unsigned values down 1
		if (result != 0xffff)
			return (0x7fff);
		src += stride;
		blk_src += stride;
	}
	return (0);	// may eventually produce SAD
}

int CompareBlockToFrame_Chroma_SSE2(ChromaPixelType *src, ChromaPixelType *blk_src, int stride)	// blk_src must be 16-byte aligned
{
	int result;
	__m128i mm_src, mm_blk, mm0, mm1;
    int fullLuma = 0xff;
	__m128i mask = _mm_set1_epi8(static_cast<char>(fullLuma << CHROMA_TOLERANCE));	// mask out lower 2 lsb
	__m128i zero = _mm_setzero_si128();

	for (int i = 0; i < 8; i++)
	{
		mm_src = _mm_loadu_si128((__m128i *) src);
		mm_blk = _mm_load_si128((__m128i *) blk_src);
		mm_src = _mm_and_si128(mm_src, mask);
		mm_blk = _mm_and_si128(mm_blk, mask);
		mm_src = _mm_srli_epi32(mm_src, CHROMA_TOLERANCE);	// shift right 2 bits
		mm_blk = _mm_srli_epi32(mm_blk, CHROMA_TOLERANCE);	// shift right 2 bits 
		mm0    = _mm_sub_epi8(mm_src, mm_blk);
		mm1    = _mm_cmpeq_epi8(mm0, zero);
		result = _mm_movemask_epi8(mm1);	// make sure all are set which means all are under 4 since we shifted the unsigned values down 2
		if (result != 0xffff)
			return (0x7fff);
		src += stride;
		blk_src += stride;
	}
	return (0);	// may eventually produce SAD
}

int SumSquaredDifference_16x16(uint8_t *aligned_src1, int stride1, uint8_t *unaligned_src2, int stride2, int rows = 16 )
{
	uint32_t pixel_sum_sq = 0;
//	ENC_START_TIMER(21, "SSD");

#ifdef USE_SSE2
	__m128i mm_src, mm_src2, mm_diff, mm_sum_sq, mm0, mm1;
	__m128i mm_zero = _mm_setzero_si128();

	mm_src = _mm_load_si128((__m128i *) aligned_src1);
	mm_src2 = _mm_loadu_si128((__m128i *) unaligned_src2);
	mm_diff = _mm_unpacklo_epi8(mm_src, mm_zero);	// unpack low 8 values into 16 bit so we can subtract
	mm1 = _mm_unpacklo_epi8(mm_src2, mm_zero);			// unpack low 8 values into 16 bit so we can subtract
	mm1 = _mm_sub_epi16(mm_diff, mm1);				// take diff of low 8 values
	mm1 = _mm_mullo_epi16(mm1, mm1);				// square diff low 8 values
	mm_sum_sq = _mm_unpacklo_epi16(mm1, mm_zero);	// make low 4 values into 32-bits because adding to other 16-bit values can overflow 16-bits

	mm0 = _mm_unpackhi_epi16(mm1, mm_zero);			// make high 4 values into 32-bits
	mm_sum_sq = _mm_add_epi32(mm_sum_sq, mm0);		// add squared diffs 

	mm_diff = _mm_unpackhi_epi8(mm_src, mm_zero);	// unpack high 8 values into 16 bit so we can subtract
	mm1 = _mm_unpackhi_epi8(mm_src2, mm_zero);		// unpack high 8 values into 16 bit so we can subtract
	mm1 = _mm_sub_epi16(mm_diff, mm1);				// take diff of high 8 values

	mm1 = _mm_mullo_epi16(mm1, mm1);				// square diff high 8 values
	mm0 = _mm_unpacklo_epi16(mm1, mm_zero);			// make low 4 values into 32-bits because adding to other 16-bit values can overflow 16-bits
	mm_sum_sq = _mm_add_epi32(mm_sum_sq, mm0);		// accumulate squared differences
	mm0 = _mm_unpackhi_epi16(mm1, mm_zero);			// make high 4 values into 32-bits
	mm_sum_sq = _mm_add_epi32(mm_sum_sq, mm0);		// accumulate squared differences

	aligned_src1   += stride1;
	unaligned_src2 += stride2;

	for (int j = rows; j > 1; j--, aligned_src1 += stride1, unaligned_src2 += stride2)
	{
		mm_src = _mm_load_si128((__m128i *) aligned_src1);
		mm_src2 = _mm_loadu_si128((__m128i *) unaligned_src2);
		mm_diff = _mm_unpacklo_epi8(mm_src, mm_zero);	// unpack low 8 values into 16 bit so we can subtract
		mm1 = _mm_unpacklo_epi8(mm_src2, mm_zero);			// unpack low 8 values into 16 bit so we can subtract
		mm1 = _mm_sub_epi16(mm_diff, mm1);				// take diff of low 8 values
		mm1 = _mm_mullo_epi16(mm1, mm1);				// square diff low 8 values
		mm0 = _mm_unpacklo_epi16(mm1, mm_zero);			// make low 4 values into 32-bits because adding to other 16-bit values can overflow 16-bits
		mm_sum_sq = _mm_add_epi32(mm_sum_sq, mm0);		// add squared diffs 

		mm0 = _mm_unpackhi_epi16(mm1, mm_zero);			// make high 4 values into 32-bits
		mm_sum_sq = _mm_add_epi32(mm_sum_sq, mm0);		// add squared diffs 

		mm_diff = _mm_unpackhi_epi8(mm_src, mm_zero);	// unpack high 8 values into 16 bit so we can subtract
		mm1 = _mm_unpackhi_epi8(mm_src2, mm_zero);		// unpack high 8 values into 16 bit so we can subtract
		mm1 = _mm_sub_epi16(mm_diff, mm1);				// take diff of high 8 values

		mm1 = _mm_mullo_epi16(mm1, mm1);				// square diff high 8 values
		mm0 = _mm_unpacklo_epi16(mm1, mm_zero);			// make low 4 values into 32-bits because adding to other 16-bit values can overflow 16-bits
		mm_sum_sq = _mm_add_epi32(mm_sum_sq, mm0);		// accumulate squared differences
		mm0 = _mm_unpackhi_epi16(mm1, mm_zero);			// make high 4 values into 32-bits
		mm_sum_sq = _mm_add_epi32(mm_sum_sq, mm0);		// accumulate squared differences
	}

	pixel_sum_sq = mm_sum_sq.m128i_u32[0] + mm_sum_sq.m128i_u32[1] + mm_sum_sq.m128i_u32[2] + mm_sum_sq.m128i_u32[3];
#else
	int pix_sum_sq_c = 0;
	// for testing SSE2 version
//	aligned_src1   -= (stride1 * rows);
//	unaligned_src2 -= (stride2 * rows);
	for (int i = 0; i < rows; i++, aligned_src1   += stride1, unaligned_src2 += stride2)
	{
		for (int j = 0; j < 16; j++)
		{
			pix_sum_sq_c += (aligned_src1[j] - unaligned_src2[j]) * (aligned_src1[j] - unaligned_src2[j]);
		}
	}

	ENC_ASSERT(pix_sum_sq_c == pixel_sum_sq);
#endif
//	ENC_PAUSE_TIMER(21, false);

	return (pixel_sum_sq);
}

// perform Root-Mean-Square algorithm for contrast 
int CalcImageComplexity(MacroBlockType *mb)
{
	uint32_t pixel_sum = 0;
	uint32_t pixel_sum_sq = 0;
	uint8_t *ptr = mb->Y_frame_buffer;
	int val;

#ifdef USE_SSE2
	__m128i mm_src, mm_sum, mm_sum_sq, mm0, mm1;
	__m128i mm_zero = _mm_setzero_si128();

	mm_src = _mm_load_si128((__m128i *) ptr);
	mm_sum = _mm_sad_epu8(mm_src, mm_zero);
	mm1 = _mm_unpacklo_epi8(mm_src, mm_zero);
	mm1 = _mm_mullo_epi16(mm1, mm1);
	mm_sum_sq = _mm_unpacklo_epi16(mm1, mm_zero);	// make into 32-bits
	mm0 = _mm_unpackhi_epi16(mm1, mm_zero);
	mm_sum_sq = _mm_add_epi32(mm_sum_sq, mm0);
	mm1 = _mm_unpackhi_epi8(mm_src, mm_zero);
	mm1 = _mm_mullo_epi16(mm1, mm1);
	mm0 = _mm_unpacklo_epi16(mm1, mm_zero);	// make into 32-bits
	mm_sum_sq = _mm_add_epi32(mm_sum_sq, mm0);
	mm0 = _mm_unpackhi_epi16(mm1, mm_zero);	
	mm_sum_sq = _mm_add_epi32(mm_sum_sq, mm0);

	ptr += MB_BLOCK_STRIDE;//mb->stride;

	for (int j = 1; j < 16; j++, ptr += MB_BLOCK_STRIDE)
	{
		mm_src = _mm_load_si128((__m128i *) ptr);
		mm0    = _mm_sad_epu8(mm_src, mm_zero);
		mm_sum = _mm_add_epi16(mm_sum, mm0);
		mm1 = _mm_unpacklo_epi8(mm_src, mm_zero);
		mm1 = _mm_mullo_epi16(mm1, mm1);
		mm0 = _mm_unpacklo_epi16(mm1, mm_zero);	// make into 32-bits
		mm_sum_sq = _mm_add_epi32(mm_sum_sq, mm0);
		mm0 = _mm_unpackhi_epi16(mm1, mm_zero);
		mm_sum_sq = _mm_add_epi32(mm_sum_sq, mm0);
		mm1 = _mm_unpackhi_epi8(mm_src, mm_zero);
		mm1 = _mm_mullo_epi16(mm1, mm1);
		mm0 = _mm_unpacklo_epi16(mm1, mm_zero);	// make into 32-bits
		mm_sum_sq = _mm_add_epi32(mm_sum_sq, mm0);
		mm0 = _mm_unpackhi_epi16(mm1, mm_zero);	
		mm_sum_sq = _mm_add_epi32(mm_sum_sq, mm0);
	}

	pixel_sum = mm_sum.m128i_i16[0] + mm_sum.m128i_i16[4];
	pixel_sum_sq = mm_sum_sq.m128i_u32[0] + mm_sum_sq.m128i_u32[1] + mm_sum_sq.m128i_u32[2] + mm_sum_sq.m128i_u32[3];

	val = pixel_sum_sq - (pixel_sum * pixel_sum) / 256;
#else
	uint32_t pixel_sum = 0;
	uint32_t pixel_sum_sq = 0;
	uint8_t *ptr = block->Y_frame_buffer;//, frame->stride
	for (int j = 0; j < 16; j++, ptr += frame->stride)
	{
		for (int i = 0; i < 16; i++)
		{
			pixel_sum += ptr[i];
		}
	}

	pixel_sum /= 256;

	ptr = block->Y_frame_buffer;//, frame->stride
	for (int j = 0; j < 16; j++, ptr += frame->stride)
	{
		for (int i = 0; i < 16; i++)
		{
			pixel_sum_sq += (ptr[i] - pixel_sum) * (ptr[i] - pixel_sum);
		}
	}

	val = pixel_sum_sq;

#endif
	
	return (val);
}

static inline void FastCopyToBlock(void *dst, void *src, int src_stride)
{
#ifdef _x64	// no _asm{} in 64-bit - thanks VS
	uint8_t *src8 = (uint8_t *) src;
	uint8_t *dst8 = (uint8_t *) dst;
	__m128i mm0;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;

	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	_mm_store_si128((__m128i *) dst8, mm0);

#else
	_asm
	{
		mov esi,src
		mov edi,dst
		mov eax,src_stride
		mov ebx,eax
		imul ebx,3
		movdqu xmm0,0[esi]
		movdqa 0[edi],xmm0
		movdqu xmm0,0[esi+eax]
		movdqa 16[edi],xmm0
		movdqu xmm0,0[esi+eax*2]
		movdqa 32[edi],xmm0
		add esi,ebx
		movdqu xmm0,0[esi]
		movdqa 48[edi],xmm0
		movdqu xmm0,0[esi+eax]
		movdqa 64[edi],xmm0
		movdqu xmm0,0[esi+eax*2]
		movdqa 80[edi],xmm0
		add esi,ebx
		movdqu xmm0,0[esi]
		movdqa 96[edi],xmm0
		movdqu xmm0,0[esi+eax]
		movdqa 112[edi],xmm0

		movdqu xmm0,0[esi+eax*2]
		movdqa 128[edi],xmm0
		add esi,ebx
		movdqu xmm0,0[esi]
		movdqa 144[edi],xmm0
		movdqu xmm0,0[esi+eax]
		movdqa 160[edi],xmm0
		movdqu xmm0,0[esi+eax*2]
		movdqa 176[edi],xmm0
		add esi,ebx
		movdqu xmm0,0[esi]
		movdqa 192[edi],xmm0
		movdqu xmm0,0[esi+eax]
		movdqa 208[edi],xmm0
		movdqu xmm0,0[esi+eax*2]
		movdqa 224[edi],xmm0
		add esi,ebx
		movdqu xmm0,0[esi]
		movdqa 240[edi],xmm0
	}
#endif
}

static inline void FastCopyToBlock16x8(void *dst, void *src, int src_stride)
{
#ifdef _x64	// no _asm{} in 64-bit - thanks VS
	uint8_t *src8 = (uint8_t *) src;
	uint8_t *dst8 = (uint8_t *) dst;
	__m128i mm0;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	src8 += src_stride;
	_mm_store_si128((__m128i *) dst8, mm0);
	dst8 += MB_BLOCK_STRIDE;
	mm0 = _mm_loadu_si128((__m128i*)src8);
	_mm_store_si128((__m128i *) dst8, mm0);
#else
	_asm
	{
		mov esi,src
		mov edi,dst
		mov eax,src_stride
		movdqu xmm0,0[esi]
		movdqa 0[edi],xmm0
		add esi,eax
		movdqu xmm0,0[esi]
		movdqa 16[edi],xmm0
		add esi,eax
		movdqu xmm0,0[esi]
		movdqa 32[edi],xmm0
		add esi,eax
		movdqu xmm0,0[esi]
		movdqa 48[edi],xmm0
		add esi,eax
		movdqu xmm0,0[esi]
		movdqa 64[edi],xmm0
		add esi,eax
		movdqu xmm0,0[esi]
		movdqa 80[edi],xmm0
		add esi,eax
		movdqu xmm0,0[esi]
		movdqa 96[edi],xmm0
		add esi,eax
		movdqu xmm0,0[esi]
		movdqa 112[edi],xmm0
	}
#endif
}

void CalcPixelAverage_16x16(uint8_t *src1, uint8_t *src2, uint8_t *dst, int src_stride)
{
#ifdef USE_SSE2
	uint8_t *src_1 = (uint8_t *) src1;
	uint8_t *src_2 = (uint8_t *) src2;
	uint8_t *dst8 = (uint8_t *) dst;
	__m128i mm0, mm1, src16_1, src16_2, ave_lo, ave_hi;
	__m128i mm_zero = _mm_setzero_si128();
	__m128i mm_one = _mm_set1_epi16(1);

	for (int i = 0; i < 16; i++)
	{
		mm0 = _mm_loadu_si128((__m128i*)src_1);
		src_1 += src_stride;
		mm1 = _mm_loadu_si128((__m128i*)src_2);
		src_2 += src_stride;
		src16_1 = _mm_unpacklo_epi8(mm0, mm_zero);
		src16_2 = _mm_unpacklo_epi8(mm1, mm_zero);
		src16_1 = _mm_add_epi16(src16_1, src16_2);
		src16_1 = _mm_add_epi16(src16_1, mm_one);	// add one
		ave_lo  = _mm_srai_epi16(src16_1, 1);		// divide by two
		src16_1 = _mm_unpackhi_epi8(mm0, mm_zero);
		src16_2 = _mm_unpackhi_epi8(mm1, mm_zero);
		src16_1 = _mm_add_epi16(src16_1, src16_2);
		src16_1 = _mm_add_epi16(src16_1, mm_one);	// add one
		ave_hi  = _mm_srai_epi16(src16_1, 1);		// divide by two

		mm0 = _mm_packus_epi16(ave_lo, ave_hi);
		_mm_store_si128((__m128i *) dst8, mm0);
		dst8 += MB_BLOCK_STRIDE;
	}

#else
	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			dst[j]  = (src1[j]  + src2[j]  + 1) >> 1;
		}

		dst  += 16;
		src1  += src_stride;
		src2  += src_stride;
	}
#endif
}

void CalcPixelAverage_16x8(uint8_t *src1, uint8_t *src2, uint8_t *dst, int src_stride)
{
#ifdef USE_SSE2
	uint8_t *src_1 = (uint8_t *) src1;
	uint8_t *src_2 = (uint8_t *) src2;
	uint8_t *dst8 = (uint8_t *) dst;
	__m128i mm0, mm1, src16_1, src16_2, ave_lo, ave_hi;
	__m128i mm_zero = _mm_setzero_si128();
	__m128i mm_one = _mm_set1_epi16(1);

	for (int i = 0; i < 8; i++)
	{
		mm0 = _mm_loadu_si128((__m128i*)src_1);
		src_1 += src_stride;
		mm1 = _mm_loadu_si128((__m128i*)src_2);
		src_2 += src_stride;
		src16_1 = _mm_unpacklo_epi8(mm0, mm_zero);
		src16_2 = _mm_unpacklo_epi8(mm1, mm_zero);
		src16_1 = _mm_add_epi16(src16_1, src16_2);
		src16_1 = _mm_add_epi16(src16_1, mm_one);	// add one
		ave_lo  = _mm_srai_epi16(src16_1, 1);		// divide by two
		src16_1 = _mm_unpackhi_epi8(mm0, mm_zero);
		src16_2 = _mm_unpackhi_epi8(mm1, mm_zero);
		src16_1 = _mm_add_epi16(src16_1, src16_2);
		src16_1 = _mm_add_epi16(src16_1, mm_one);	// add one
		ave_hi  = _mm_srai_epi16(src16_1, 1);		// divide by two

		mm0 = _mm_packus_epi16(ave_lo, ave_hi);
		_mm_store_si128((__m128i *) dst8, mm0);
		dst8 += MB_BLOCK_STRIDE;
	}
#else
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			dst[j]  = (src1[j]  + src2[j]  + 1) >> 1;
		}

		dst  += 16;
		src1  += src_stride;
		src2  += src_stride;
	}
#endif
}

void CalcSKIP_B(MacroBlockType *block, YUVFrameType *frame)
{
	MotionVector mv_l0, mv_l1;
	int pmv[2][2];
	int direct_ref[2];
	int y_buffer_index[2];
	int uv_buffer_index[2];
	block->mb_skip_flag = true;	// there is a special case where if the mb is p_skip then zero out the vector
	block->mb_direct_flag = false;

#if _DEBUG	
	if ((gEncoderState.total_frame_num == 8) && (block->mb_index == 796))
		ENC_TRACE("");
#endif
	CalcMPredVector_Direct_Spatial(block, pmv[0], pmv[1], direct_ref, frame);

	// do frame clip test - reject if outside of frame
	for (int i = 0; i < 2; i++)
	{
		int full_x = block->mb_x * 16 + (pmv[i][0] >> 2);
		int full_y = block->mb_y * 16 + (pmv[i][1] >> 2);
		if (!((full_x < 0) || ((full_x + 16) >= frame->width) || (full_y < 0) || ((full_y + 16) >= frame->height)))
		{
			y_buffer_index[i]  = block->Y_block_start_index  + (frame->stride_padded   * (pmv[i][1] >> 2)) + (pmv[i][0] >> 2);
			uv_buffer_index[i] = block->UV_block_start_index + (frame->uv_width_padded * (pmv[i][1] >> 3)) + (pmv[i][0] >> 3);
		}
		else
		{
			block->pskip_ssd = INT_MAX;
			block->mb_skip_flag = false;
			return;
		}
	}

#if 1

	if ((direct_ref[0] != -1) && (direct_ref[1] != -1))
	{
		u8_16x16_Type *save_luma_pred = block->Y_pred_pixels;
		Chroma_8x8_Type *save_chroma_pred = block->UV_pred_pixels;
		__declspec(align(16)) u8_16x16_Type tmp_luma_pred[2];
		__declspec(align(16)) Chroma_8x8_Type tmp_chroma_pred[2];

		SetMotionVectors16x16((uint32_t *) block->data->mv_l0[0], pmv[0][0], pmv[0][1]);
		SetMotionVectors16x16((uint32_t *) block->data->mv_l1[0], pmv[1][0], pmv[1][1]);

		for (int dir = 0; dir < 2; dir++)
		{
			block->Y_pred_pixels = &tmp_luma_pred[dir];
			block->UV_pred_pixels = &tmp_chroma_pred[dir];

			ME_Prediction(block, dir);
		}

		// calc average for B_SKIP
		CalcPixelAverage_16x16((uint8_t *) &tmp_luma_pred[0],   (uint8_t *) &tmp_luma_pred[1],   block->skip_dst_y_buffer,  MB_BLOCK_STRIDE);
		CalcPixelAverage_16x8( (uint8_t *) &tmp_chroma_pred[0], (uint8_t *) &tmp_chroma_pred[1], block->skip_dst_uv_buffer, MB_BLOCK_STRIDE);

		// restore pointers
		block->Y_pred_pixels = save_luma_pred;
		block->UV_pred_pixels = save_chroma_pred;

		block->mb_direct_flag = true;	// means it is something we can try
	}
	else
	if (direct_ref[0] != -1)
	{
		SetMotionVectors16x16((uint32_t *) block->data->mv_l0[0], pmv[0][0], pmv[0][1]);
		ME_Prediction(block, ME_PRED_SKIP);
	}
	else
	{
		SetMotionVectors16x16((uint32_t *) block->data->mv_l1[0], pmv[1][0], pmv[1][1]);
		ME_Prediction(block, ME_PRED_SKIP | ME_PRED_DIR_FORWARD);
	}
#else
	// calc average (approximation as we are not using sub-pixel)
	if ((direct_ref[0] != -1) && (direct_ref[1] != -1))
	{
		uint8_t *skip_y_buffer[2], *skip_uv_buffer[2];
		uint8_t *prev_y_buffer, *prev_uv_buffer, *next_y_buffer, *next_uv_buffer;

		skip_y_buffer[0]  = prev_y_buffer  = &frame->prev_ref_frame->Y_frame_reconstructed[y_buffer_index[0]];
		skip_uv_buffer[0] = prev_uv_buffer = (uint8_t*) &frame->prev_ref_frame->UV_frame_reconstructed[uv_buffer_index[0]];
		skip_y_buffer[1]  = next_y_buffer  = &frame->next_ref_frame->Y_frame_reconstructed[y_buffer_index[1]];
		skip_uv_buffer[1] = next_uv_buffer = (uint8_t*) &frame->next_ref_frame->UV_frame_reconstructed[uv_buffer_index[1]];

		CalcPixelAverage_16x16(prev_y_buffer, next_y_buffer,  block->skip_dst_y_buffer,  frame->stride_padded);
		CalcPixelAverage_16x8(prev_uv_buffer, next_uv_buffer, block->skip_dst_uv_buffer, frame->uv_stride_padded);

		block->mb_direct_flag = true;	// means it is something we can try
	}
	else
	if (direct_ref[0] != -1)
	{
		FastCopyToBlock(block->skip_dst_y_buffer,  &frame->next_ref_frame->Y_frame_reconstructed[y_buffer_index[0]], frame->stride_padded);
		FastCopyToBlock16x8(block->skip_dst_uv_buffer, (uint8_t*) &frame->next_ref_frame->UV_frame_reconstructed[uv_buffer_index[0]], frame->uv_stride_padded);
	}
	else
	{
		FastCopyToBlock(block->skip_dst_y_buffer,  &frame->next_ref_frame->Y_frame_reconstructed[y_buffer_index[1]], frame->stride_padded);
		FastCopyToBlock16x8(block->skip_dst_uv_buffer, (uint8_t*) &frame->next_ref_frame->UV_frame_reconstructed[uv_buffer_index[1]], frame->uv_stride_padded);
	}
#endif

	// calc SSD
	int score_l  = SumSquaredDifference_16x16(block->Y_frame_buffer, MB_BLOCK_STRIDE, block->skip_dst_y_buffer, 16);
	int score_cr = SumSquaredDifference_16x16((uint8_t*) block->UV_frame_buffer, MB_BLOCK_STRIDE, block->skip_dst_uv_buffer, 16, 8);

	block->pskip_ssd = score_l + score_cr + 18; // bit cost factor (fake lambda)

	int pskip_mad    = ComputeSAD_SSE2(block->skip_dst_y_buffer, 16, block->Y_frame_buffer, MB_BLOCK_STRIDE, true);
#if 0
	int pskip_uv_mad = ComputeSAD_SSE2(block->skip_uv_buffer, frame->uv_stride, (uint8_t *) block->UV_frame_buffer, MB_BLOCK_STRIDE, false);
#endif

	block->pmv_skip[0][0] = pmv[0][0];
	block->pmv_skip[0][1] = pmv[0][1];
	block->pmv_skip[1][0] = pmv[1][0];
	block->pmv_skip[1][1] = pmv[1][1];
	block->direct_ref[0]  = static_cast<int8_t>(direct_ref[0]);
	block->direct_ref[1]  = static_cast<int8_t>(direct_ref[1]);

	block->mb_skip_flag = false;

//	if ((block->pskip_ssd < 2000) && (frame->prev_ref_frame->mb_set[block->mb_index].mv[0] == pmv[0][0]) && (frame->prev_ref_frame->mb_set[block->mb_index].mv[1] == pmv[0][1])
//		 && (pmv[0][0] == pmv[1][0]) && (pmv[0][1] == pmv[1][1]))
#if 0
	if ((score_l < 5000) && (score_cr < 25000)    )//block->pskip_ssd < 3000)
	{
		block->mb_skip_flag = true;
	}
	else
#endif
	{
		block->mb_skip_flag = SkipCheck(block);
	}

	if (block->mb_skip_flag)
	{
		// currently we just grab from the last frame - probably will need to be expanded
		SetMotionVectors16x16((uint32_t *) block->data->mv_l0[0], pmv[0][0], pmv[0][1]);
		SetMotionVectors16x16((uint32_t *) block->data->mv_l1[0], pmv[1][0], pmv[1][1]);	// need to do this for B slices - neighboring PMV calc

		uint8_t *y_buffer = block->skip_dst_y_buffer;
		for( int j = 0; j < 16; ++j, y_buffer += 16 )
		{
			int idx = j * block->stride + block->Y_block_start_index;
			memcpy(&block->Y_frame_reconstructed[idx], y_buffer, 16);
		}

		frame->frame_mad += pskip_mad;

		uint8_t *uv_buffer = block->skip_dst_uv_buffer;
		for( int k = 0; k < 8; ++k, uv_buffer += 16 )
		{
			int idx = k * block->uv_stride + block->UV_block_start_index;
			memcpy(&block->UV_frame_reconstructed[idx], uv_buffer, 16);
		}

		block->intra_pred_mode = INTRA_PRED_MODE_DC;
		block->chroma_pred_mode = CHROMA_PRED_MODE_DC;
		block->coded_block_pattern_luma = 0;
		block->coded_block_pattern_chroma = 0;
		block->isI16x16 = false;
		block->data->refIdxL0 = direct_ref[0];
		block->data->refIdxL1 = direct_ref[1];

		block->qp_delta = 0;
		if (block->mb_x > 0)
			block->qp = block->mbAddrA->qp;

		block->mb_direct_flag = false;
	}
	else
		block->pskip_sad = pskip_mad;
}

void CalcSKIP_P(MacroBlockType *block, YUVFrameType *frame)
{
	ENC_START_TIMER(12,"MB PSKIP");

	int pmv[2] = {0, 0};
    int pskip_mad = 0, pskip_uv_mad = 0;
	YUVFrameType *prev_frame = NULL;

	bool forcedPSkipZeroMV = false;

	prev_frame = frame->prev_ref_frame;
	// test for P_SKIP
	if ((gEncoderState.total_frame_num == 61) && (block->mb_index == 400))
		ENC_TRACE("");
	block->mb_skip_flag = true;	// not sure if this is right, but I need to get this pred vector to see if the pixels there match 
	// our current frame, but there is a special case where if the mb is p_skip then zero out the vector
	// if a neighbor to the left or above is (0 0). so funky
	block->mb_direct_flag = false;
	forcedPSkipZeroMV = CalcMPredVector(block, &pmv[0], &pmv[1]);

	// temp clip edge test
	int full_x = block->mb_x * 16 + (pmv[0] >> 2);
	int full_y = block->mb_y * 16 + (pmv[1] >> 2);
	if (!((full_x < 0) || ((full_x + 16) >= frame->width) || (full_y < 0) || ((full_y + 16) >= frame->height)))
	{
		block->data->mv_l0[0][0][0] = static_cast<int16_t>(pmv[0]);
		block->data->mv_l0[1][0][0] = static_cast<int16_t>(pmv[1]);
		ME_Prediction(block, ME_PRED_SKIP);

		// using SSD (sum of squared differences)
		int score_l  = SumSquaredDifference_16x16(block->Y_frame_buffer, MB_BLOCK_STRIDE, block->skip_dst_y_buffer, MB_BLOCK_STRIDE);
		int score_cr = SumSquaredDifference_16x16((uint8_t*) block->UV_frame_buffer, MB_BLOCK_STRIDE, block->skip_dst_uv_buffer, MB_BLOCK_STRIDE, 8);

		block->pskip_ssd = score_l + score_cr;// + 18; // bit cost factor (fake lambda)

		//			if ((score_l >= 500) || (score_cr >= 250))	// set this threshold for early PSKIP
		{
			block->mb_skip_flag = false;
		}

		pskip_mad    = ComputeSAD_SSE2(block->skip_dst_y_buffer, MB_BLOCK_STRIDE, block->Y_frame_buffer, MB_BLOCK_STRIDE, true);
		pskip_uv_mad = ComputeSAD_SSE2(block->skip_dst_uv_buffer, MB_BLOCK_STRIDE, (uint8_t *) block->UV_frame_buffer, MB_BLOCK_STRIDE, false);
#if 0
		if ((block->pskip_ssd < 2000) && (prev_frame->mb_set[block->mb_index].mv[0] == pmv[0]) && (prev_frame->mb_set[block->mb_index].mv[1] == pmv[1]))
		{
			block->mb_skip_flag = true;
		}
		else
#endif
		{
			block->mb_skip_flag = SkipCheck(block);
		}
		block->luma_bit_flags = 0;
		block->chroma_bit_flags = 0;
	}
	else
	{
		block->mb_skip_flag = false;
		block->pskip_ssd = INT_MAX;
	}

	if (block->mb_skip_flag)
	{
		// currently we just grab from the last frame - probably will need to be expanded
		SetMotionVectors16x16((uint32_t *) block->data->mv_l0[0], pmv[0], pmv[1]);
		SetMotionVectors16x16((uint32_t *) block->data->mv_l1[0], 0, 0);	// need to do this for B slices - neighboring PMV calc

		uint8_t *buf = block->skip_dst_y_buffer;
		for( int j = 0; j < 16; ++j, buf += MB_BLOCK_STRIDE )
		{
			int idx = j * block->stride + block->Y_block_start_index;
			memcpy(&block->Y_frame_reconstructed[idx], buf, 16);
		}

		frame->frame_mad += pskip_mad;

		buf = block->skip_dst_uv_buffer;
		for( int k = 0; k < 8; ++k, buf += MB_BLOCK_STRIDE )
		{
			int idx = k * block->uv_stride + block->UV_block_start_index;
			memcpy(&block->UV_frame_reconstructed[idx], buf, 16);
		}

		block->intra_pred_mode = INTRA_PRED_MODE_DC;
		block->chroma_pred_mode = CHROMA_PRED_MODE_DC;
		block->coded_block_pattern_luma = 0;
		block->coded_block_pattern_chroma = 0;
		block->isI16x16 = false;
		block->data->refIdxL0 = 0;
		block->qp_delta = 0;
		if (block->mb_x > 0)
			block->qp = block->mbAddrA->qp;
	}
	else
	{
		block->cbf_bits[0] = 0;
#if 0
		if( forcedPSkipZeroMV ) // since this is not P_Skip, compute median predictor
		{
			assert( block->mb_skip_flag == false );
			CalcMPredVector(block, &pmv[0], &pmv[1]);
		}
#endif
	}

	block->pskip_sad = pskip_mad;
	block->pmv_skip[0][0] = pmv[0];
	block->pmv_skip[0][1] = pmv[1];
	block->pmv_skip[1][0] = 0;
	block->pmv_skip[1][1] = 0;

	ENC_PAUSE_TIMER(12,false);
}

void CalcSKIP_I(MacroBlockType *block, YUVFrameType *frame)
{
	block->mb_skip_flag = false;
	block->mb_direct_flag = false;
}


void FindBestPredictor_B(MacroBlockType *block, YUVFrameType *frame)
{
	// use SSD for final prediction mode decision
	block->ipred_ssd = SumSquaredDifference_16x16(block->Y_frame_buffer, MB_BLOCK_STRIDE, block->Y_pred_pixels->block[0], 16) +
					   SumSquaredDifference_16x16((uint8_t*) block->UV_frame_buffer, MB_BLOCK_STRIDE, (uint8_t*) block->UV_pred_pixels->block[0], 16, 8);

	ENC_START_TIMER(10,"Motion Estimation");

	// 1. estimate cost for P16x16, C_p
	block->mb_partition_type = 1; // P16x16

#if _DEBUG	
	if ((gEncoderState.total_frame_num == 2) && (block->mb_index == 2162))
		ENC_TRACE("");
#endif
	MotionVector mv_l0, mv_l1;
	int pmv_l1[2];
	int me_sad[3], me_cost[3];
	int direction = ME_PRED_DIR_FORWARD;

	CalcMPredVector_B_L0(block, &block->pmv[0], &block->pmv[1]);
	CalcMPredVector_B_L1(block, &pmv_l1[0],     &pmv_l1[1]);
	// get best mv for previous ref frame
	mv_l0.mv_x = static_cast<short>(block->pmv[0]);
	mv_l0.mv_y = static_cast<short>(block->pmv[1]);
	me_cost[0] = ME_ComputeMotionVector(block, frame, frame->prev_ref_frame, &mv_l0, &me_sad[0]);
	block->mv[0] = mv_l0.mv_x;
	block->mv[1] = mv_l0.mv_y;

	// now get best mv for forward ref frame
	mv_l1.mv_x = static_cast<short>(pmv_l1[0]);
	mv_l1.mv_y = static_cast<short>(pmv_l1[1]);
	me_cost[1] = ME_ComputeMotionVector(block, frame, frame->next_ref_frame, &mv_l1, &me_sad[1]);

#if 0
	if (me_cost[0] < me_cost[1])
	{	// back
		block->data->refIdxL0 = 0;
		block->data->refIdxL1 = -1;
		direction = ME_PRED_DIR_BACK;
		SetMotionVectors16x16((uint32_t *) block->data->mv_l0[0], mv_l0.mv_x, mv_l0.mv_y);
		SetMotionVectorDeltas16x16((uint32_t *) block->data->mvd_l0[0], mv_l0.mv_x - block->pmv[0], mv_l0.mv_y - block->pmv[1]);
		block->me_cost = me_cost[0];
		block->me_sad  = me_sad[0];
	}
	else
	{	// forward
		block->data->refIdxL0 = -1;
		block->data->refIdxL1 = 0;
		direction = ME_PRED_DIR_FORWARD;
		SetMotionVectors16x16((uint32_t *) block->data->mv_l1[0], mv_l1.mv_x, mv_l1.mv_y);
		SetMotionVectors16x16((uint32_t *) block->data->mvd_l1[0], mv_l1.mv_x - pmv_l1[0], mv_l1.mv_y - pmv_l1[1]);
		block->me_cost = me_cost[1];
		block->me_sad  = me_sad[1];
	}

	// calc SSD for ME
	u8_16x16_Type *save_luma_pred = block->Y_pred_pixels;
	Chroma_8x8_Type *save_chroma_pred = block->UV_pred_pixels;
	__declspec(align(16)) u8_16x16_Type tmp_luma_pred;
	__declspec(align(16)) Chroma_8x8_Type tmp_chroma_pred;
	block->Y_pred_pixels = &tmp_luma_pred;
	block->UV_pred_pixels = &tmp_chroma_pred;

	ME_Prediction(block, direction);
	block->me_ssd = SumSquaredDifference_16x16(block->Y_frame_buffer, MB_BLOCK_STRIDE, block->Y_pred_pixels->block[0], 16) +
					SumSquaredDifference_16x16((uint8_t*) block->UV_frame_buffer, MB_BLOCK_STRIDE, (uint8_t*) block->UV_pred_pixels->block[0], 16, 8);
	block->me_ssd += (block->me_cost - block->me_sad);	// add mv bit cost est
#else
	// calc SSD for ME
	u8_16x16_Type *save_luma_pred = block->Y_pred_pixels;
	Chroma_8x8_Type *save_chroma_pred = block->UV_pred_pixels;
	__declspec(align(16)) u8_16x16_Type tmp_luma_pred[3];
	__declspec(align(16)) Chroma_8x8_Type tmp_chroma_pred[3];

	SetMotionVectors16x16((uint32_t *) block->data->mv_l0[0], mv_l0.mv_x, mv_l0.mv_y);
	SetMotionVectors16x16((uint32_t *) block->data->mv_l1[0], mv_l1.mv_x, mv_l1.mv_y);

	for (int dir = 0; dir < 2; dir++)
	{
		block->Y_pred_pixels = &tmp_luma_pred[dir];
		block->UV_pred_pixels = &tmp_chroma_pred[dir];

		ME_Prediction(block, dir);

		// adjust me_cost[x] for l0 vs l1 vs bi decision
		me_cost[dir] = ((me_cost[dir] - me_sad[dir]) * 15) + me_sad[dir];
	}

	// calc average for MB_TYPE_B_Bi_16x16
	CalcPixelAverage_16x16((uint8_t *) &tmp_luma_pred[0], (uint8_t *) &tmp_luma_pred[1], (uint8_t *)  &tmp_luma_pred[2], MB_BLOCK_STRIDE);
	me_sad[2] = ComputeSAD_SSE2_x((uint8_t *) &tmp_luma_pred[2], MB_BLOCK_STRIDE, block->Y_frame_buffer);
	me_cost[2] = (me_cost[0] - me_sad[0]) + (me_cost[1] - me_sad[1]) + me_sad[2];

	int me_type = 0;
	if (me_cost[0] > me_cost[1])
		me_type = 1;
	if (me_cost[me_type] > me_cost[2])
	{
		me_type = 2;
		CalcPixelAverage_16x8((uint8_t *) &tmp_chroma_pred[0], (uint8_t *) &tmp_chroma_pred[1], (uint8_t *)  &tmp_chroma_pred[2], MB_BLOCK_STRIDE);
		block->data->refIdxL0 = 0;
		block->data->refIdxL1 = 0;
		SetMotionVectors16x16((uint32_t *) block->data->mv_l0[0], mv_l0.mv_x, mv_l0.mv_y);
		SetMotionVectorDeltas16x16((uint32_t *) block->data->mvd_l0[0], mv_l0.mv_x - block->pmv[0], mv_l0.mv_y - block->pmv[1]);
		SetMotionVectors16x16((uint32_t *) block->data->mv_l1[0], mv_l1.mv_x, mv_l1.mv_y);
		SetMotionVectors16x16((uint32_t *) block->data->mvd_l1[0], mv_l1.mv_x - pmv_l1[0], mv_l1.mv_y - pmv_l1[1]);
	}
	else
	{
		if (me_type == 0)
		{	// back
			block->data->refIdxL0 = 0;
			block->data->refIdxL1 = -1;
			direction = ME_PRED_DIR_BACK;
			SetMotionVectors16x16((uint32_t *) block->data->mv_l0[0], mv_l0.mv_x, mv_l0.mv_y);
			SetMotionVectorDeltas16x16((uint32_t *) block->data->mvd_l0[0], mv_l0.mv_x - block->pmv[0], mv_l0.mv_y - block->pmv[1]);
		}
		else
		{	// forward
			block->data->refIdxL0 = -1;
			block->data->refIdxL1 = 0;
			direction = ME_PRED_DIR_FORWARD;
			SetMotionVectors16x16((uint32_t *) block->data->mv_l1[0], mv_l1.mv_x, mv_l1.mv_y);
			SetMotionVectors16x16((uint32_t *) block->data->mvd_l1[0], mv_l1.mv_x - pmv_l1[0], mv_l1.mv_y - pmv_l1[1]);
		}
	}
	block->me_cost = me_cost[me_type];
	block->me_sad  = me_sad[me_type];

	block->Y_pred_pixels = &tmp_luma_pred[me_type];
	block->UV_pred_pixels = &tmp_chroma_pred[me_type];
	block->me_ssd = SumSquaredDifference_16x16(block->Y_frame_buffer, MB_BLOCK_STRIDE, block->Y_pred_pixels->block[0], 16) +
					SumSquaredDifference_16x16((uint8_t*) block->UV_frame_buffer, MB_BLOCK_STRIDE, (uint8_t*) block->UV_pred_pixels->block[0], 16, 8);
	block->me_ssd += (block->me_cost - block->me_sad) / 15;	// add mv bit cost est

#endif
	ENC_PAUSE_TIMER(10,false);

	// 2. estimate cost for I16x16
	int C_i;
	if (block->luma_ipred_sad == INT_MAX)
		C_i = INT_MAX;
	else
		C_i = block->ipred_ssd;//block->luma_ipred_sad + block->chroma_ipred_sad;

	// 3. the lower cost decision wins
	int mode = 0;	// 0 - ipred, 1 - me, 2 - pskip
	int min_ssd = C_i;
	if (min_ssd > block->me_ssd)
	{
		mode = 1;
		min_ssd = block->me_ssd;

		// now test for b_direct16x16
		if (block->mb_direct_flag && (block->pskip_ssd > block->me_ssd))
			block->mb_direct_flag = false;
	}

	block->coeff_cost = 0;

	//if (min_ssd > block->pskip_ssd)
	{
		ENC_START_TIMER(3,"MB Transform");
		// now check residual transform
		if (mode == 1)	// ME
		{
			block->isI16x16 = false;

			if (block->mb_direct_flag)
			{
				block->Y_pred_pixels = (u8_16x16_Type *) block->skip_dst_y_buffer;
				block->UV_pred_pixels = (Chroma_8x8_Type *) block->skip_dst_uv_buffer;
			}
		}
		else
		{
			block->isI16x16 = true;
			block->mb_direct_flag = false;

			// restore saved pointers here because I pred was chosen over ME
			block->Y_pred_pixels = save_luma_pred;
			block->UV_pred_pixels = save_chroma_pred;
		}

		int cbp_luma   = ProcessResidual_16x16_Luma(block);
		ENC_START_TIMER(13,"MB T Crb");
		int cbp_chroma = ProcessResidual_8x8_Chroma(block);	// U & V

		int sad = ComputeSAD_SSE2(block->Y_frame_buffer, MB_BLOCK_STRIDE, &block->Y_frame_reconstructed[block->Y_block_start_index], frame->stride_padded, 1);
		int ssd = SumSquaredDifference_16x16(block->Y_frame_buffer, MB_BLOCK_STRIDE, &block->Y_frame_reconstructed[block->Y_block_start_index], frame->stride_padded) +
				  SumSquaredDifference_16x16((uint8_t*) block->UV_frame_buffer, MB_BLOCK_STRIDE, (uint8_t*) &block->UV_frame_reconstructed[block->UV_block_start_index], frame->uv_stride_padded, 8);

		//		int uv_sad = ComputeSAD_SSE2((uint8_t*) block->UV_frame_buffer, MB_BLOCK_STRIDE, (uint8_t*) &block->UV_frame_reconstructed[block->UV_block_start_index], frame->uv_stride_padded, 0);
		ENC_PAUSE_TIMER(13,false);
		ENC_PAUSE_TIMER(3,false);

		int save_sad = sad;
		int additional_cost = 0;

		if (cbp_luma || cbp_chroma)
			additional_cost += block->coeff_cost * COEFF_BITCOST_LAMBDA;

		if (mode == 1)
		{
			if (!block->mb_direct_flag)
				additional_cost += ((block->me_cost - block->me_sad) * MV_BITCOST_LAMBDA);
		}
		else
			additional_cost += IPRED_COST;

		DEBUG_MB_DECISION;
#if 1	// turn off because at this point we just care about distortion
		ssd += (additional_cost * block->qp) >> 5;	// scale by qp to allow for more distortion
#endif
		if (ssd >= block->pskip_ssd)
		{
			mode = 2;	// pskip

			block->mb_direct_flag = false;
		}
		else
		{
			block->coded_block_pattern_luma   = cbp_luma;
			block->coded_block_pattern_chroma = (cbp_chroma & 0x2) ? 2 : cbp_chroma;
			frame->frame_mad += save_sad;
		}
	}
	//else
	//{
	//	block->coded_block_pattern_luma   = -1;	// means need to run transforms
	//}

	// restore saved pointers
	block->Y_pred_pixels = save_luma_pred;
	block->UV_pred_pixels = save_chroma_pred;

	if (mode == 0)	// I Prediction
	{
		block->mv[0] = block->mv[1] = 0;
		block->isI16x16 = true;
		block->data->refIdxL0 = -1;
		block->data->refIdxL1 = -1;
	}
	else
	if ((mode == 1) && (!block->mb_direct_flag))	// ME
	{
		if (block->coded_block_pattern_luma == -1)	// means this wasn't set above
		{
			ME_Prediction(block, direction);	// can just copy from save_luma/chroma_pred buffers

			block->isI16x16 = false;
		}

		if (block->data->refIdxL0 == -1)
		{
			SetMotionVectors16x16((uint32_t *) block->data->mv_l0[0], 0, 0);
			SetMotionVectors16x16((uint32_t *) block->data->mvd_l0[0], 0, 0);
		}
		else
		{
			// add to temporal mv table
			frame->prev_ref_frame->temporal_mv[block->mb_index].mx = mv_l0.mv_x * frame->prev_ref_frame->temporal_mv_distscale;
			frame->prev_ref_frame->temporal_mv[block->mb_index].my = mv_l0.mv_y * frame->prev_ref_frame->temporal_mv_distscale;
		}

		if (block->data->refIdxL1 == -1)
		{
			SetMotionVectors16x16((uint32_t *) block->data->mv_l1[0], 0, 0);
			SetMotionVectors16x16((uint32_t *) block->data->mvd_l1[0], 0, 0);
		}
		else
		{
			// add to temporal mv table
			frame->next_ref_frame->temporal_mv[block->mb_index].mx = mv_l1.mv_x * frame->next_ref_frame->temporal_mv_distscale;
			frame->next_ref_frame->temporal_mv[block->mb_index].my = mv_l1.mv_y * frame->next_ref_frame->temporal_mv_distscale;
		}

		block->intra_pred_mode = INTRA_PRED_MODE_DC;
		block->chroma_pred_mode = CHROMA_PRED_MODE_DC;
	}
	else
	{	// BSKIP
		SetMotionVectors16x16((uint32_t *) block->data->mv_l0[0], block->pmv_skip[0][0], block->pmv_skip[0][1]);
		SetMotionVectors16x16((uint32_t *) block->data->mv_l1[0], block->pmv_skip[1][0], block->pmv_skip[1][1]);

		// add to temporal mv table
		frame->prev_ref_frame->temporal_mv[block->mb_index].mx = block->pmv_skip[0][0] * frame->prev_ref_frame->temporal_mv_distscale;
		frame->prev_ref_frame->temporal_mv[block->mb_index].my = block->pmv_skip[0][1] * frame->prev_ref_frame->temporal_mv_distscale;
		frame->next_ref_frame->temporal_mv[block->mb_index].mx = block->pmv_skip[1][0] * frame->next_ref_frame->temporal_mv_distscale;
		frame->next_ref_frame->temporal_mv[block->mb_index].my = block->pmv_skip[1][1] * frame->next_ref_frame->temporal_mv_distscale;

		if (!block->mb_direct_flag)
		{
			block->mb_skip_flag = true;

			uint8_t *y_buffer = block->skip_dst_y_buffer;
			for( int j = 0; j < 16; ++j, y_buffer += 16 )
			{
				int idx = j * block->stride + block->Y_block_start_index;
				memcpy(&block->Y_frame_reconstructed[idx], y_buffer, 16);
			}

			frame->frame_mad += block->pskip_sad;

			uint8_t *uv_buffer = block->skip_dst_uv_buffer;
			for( int k = 0; k < 8; ++k, uv_buffer += 16 )
			{
				int idx = k * block->uv_stride + block->UV_block_start_index;
				memcpy(&block->UV_frame_reconstructed[idx], uv_buffer, 16);
			}
			block->coded_block_pattern_luma = 0;
			block->coded_block_pattern_chroma = 0;
		}

		block->intra_pred_mode = INTRA_PRED_MODE_DC;
		block->chroma_pred_mode = CHROMA_PRED_MODE_DC;
		block->isI16x16 = false;
		block->data->refIdxL0 = block->direct_ref[0];
		block->data->refIdxL1 = block->direct_ref[1];
		block->qp_delta = 0;
		if (block->mb_x > 0)
			block->qp = block->mbAddrA->qp;
	}
}

void FindBestPredictor_P(MacroBlockType *block, YUVFrameType *frame)
{
	// use SSD for final prediction mode decision
	block->ipred_ssd = SumSquaredDifference_16x16(block->Y_frame_buffer, MB_BLOCK_STRIDE, block->Y_pred_pixels->block[0], 16) +
					   SumSquaredDifference_16x16((uint8_t*) block->UV_frame_buffer, MB_BLOCK_STRIDE, (uint8_t*) block->UV_pred_pixels->block[0], 16, 8);
	block->ipred_ssd +=	IPRED_COST;

	ENC_START_TIMER(10,"Motion Estimation");

	// 1. estimate cost for P16x16
	MotionVector mv;
	block->mb_partition_type = 1; // P16x16

#if _DEBUG	
	if ((gEncoderState.total_frame_num == 61) && (block->mb_index == 1881))
		ENC_TRACE("");
#endif
	CalcMPredVector(block, &block->pmv[0], &block->pmv[1]);
	mv.mv_x = static_cast<short>(block->pmv[0]);
	mv.mv_y = static_cast<short>(block->pmv[1]);
	block->me_cost = ME_ComputeMotionVector(block, frame, frame->prev_ref_frame, &mv, &block->me_sad);
	block->mv[0] = mv.mv_x;
	block->mv[1] = mv.mv_y;

	// calc SSD for ME
	SetMotionVectors16x16((uint32_t *) block->data->mv_l0[0], mv.mv_x, mv.mv_y);
	u8_16x16_Type *save_luma_pred = block->Y_pred_pixels;
	Chroma_8x8_Type *save_chroma_pred = block->UV_pred_pixels;
	__declspec(align(16)) u8_16x16_Type tmp_luma_pred;
	__declspec(align(16)) Chroma_8x8_Type tmp_chroma_pred;
	block->Y_pred_pixels = &tmp_luma_pred;
	block->UV_pred_pixels = &tmp_chroma_pred;

	ME_Prediction(block, ME_PRED_DIR_BACK);
	block->me_ssd = SumSquaredDifference_16x16(block->Y_frame_buffer, MB_BLOCK_STRIDE, block->Y_pred_pixels->block[0], 16) +
					SumSquaredDifference_16x16((uint8_t*) block->UV_frame_buffer, MB_BLOCK_STRIDE, (uint8_t*) block->UV_pred_pixels->block[0], 16, 8);
	block->me_ssd += (block->me_cost - block->me_sad);	// add mv bit cost est (multiply by factor because ssd is a different scale than SAD)

	ENC_PAUSE_TIMER(10,false);

	// 2. estimate cost for I16x16
	int C_i;
	if (block->luma_ipred_sad == INT_MAX)
		C_i = INT_MAX;
	else
	{
#if 0
		if (gEncoderState.mb_debug_on)
			ENC_TRACE("");

		block->isI16x16 = false;
		block->coeff_cost = 0;

		int cbp_luma   = ProcessResidual_16x16_Luma(block);
		int cbp_chroma = ProcessResidual_8x8_Chroma(block);	// U & V

		int ssd = SumSquaredDifference_16x16(block->Y_frame_buffer, MB_BLOCK_STRIDE, &block->Y_frame_reconstructed[block->Y_block_start_index], frame->stride_padded) +
			SumSquaredDifference_16x16((uint8_t*) block->UV_frame_buffer, MB_BLOCK_STRIDE, (uint8_t*) &block->UV_frame_reconstructed[block->UV_block_start_index], frame->uv_stride_padded, 8);

		int additional_cost = 0;

		if (cbp_luma || cbp_chroma)
			additional_cost += block->coeff_cost * COEFF_BITCOST_LAMBDA;
		additional_cost += ((block->me_cost - block->me_sad) * MV_BITCOST_LAMBDA);

		ssd += additional_cost;

		block->me_ssd = ssd;

		block->isI16x16 = true;
		block->coeff_cost = 0;
		block->cbf_bits[0] = 0;
		block->luma_bit_flags = block->chroma_bit_flags = 0;

		// restore saved pointers here because I pred was chosen over ME
		block->Y_pred_pixels = save_luma_pred;
		block->UV_pred_pixels = save_chroma_pred;

		cbp_luma   = ProcessResidual_16x16_Luma(block);
		cbp_chroma = ProcessResidual_8x8_Chroma(block);	// U & V

		ssd = SumSquaredDifference_16x16(block->Y_frame_buffer, MB_BLOCK_STRIDE, &block->Y_frame_reconstructed[block->Y_block_start_index], frame->stride_padded) +
			SumSquaredDifference_16x16((uint8_t*) block->UV_frame_buffer, MB_BLOCK_STRIDE, (uint8_t*) &block->UV_frame_reconstructed[block->UV_block_start_index], frame->uv_stride_padded, 8);

		additional_cost = 0;

		if (cbp_luma || cbp_chroma)
			additional_cost += block->coeff_cost * COEFF_BITCOST_LAMBDA;
		additional_cost += IPRED_COST;

		ssd += additional_cost;

		block->ipred_ssd = ssd;

		block->Y_pred_pixels = &tmp_luma_pred;
		block->UV_pred_pixels = &tmp_chroma_pred;
		block->cbf_bits[0] = 0;
		block->luma_bit_flags = block->chroma_bit_flags = 0;
#endif
		C_i = block->ipred_ssd;//block->luma_ipred_sad + block->chroma_ipred_sad;
	}

	// 3. the lower cost decision wins
	int mode = 0;	// 0 - ipred, 1 - me, 2 - pskip
	int min_ssd = C_i;
	if (min_ssd > block->me_ssd)
	{
		mode = 1;
		min_ssd = block->me_ssd;
	}

	block->coeff_cost = 0;

	//if (min_ssd > (block->pskip_ssd + (block->pskip_ssd >> 3)))	// if I or P SSD is 12% less than pskip SSD then accept without residual
	{
		ENC_START_TIMER(3,"MB Transform");
		// now check residual transform
		if (mode == 1)	// ME
		{
			block->isI16x16 = false;
		}
		else
		{
			block->isI16x16 = true;
			// restore saved pointers here because I pred was chosen over ME
			block->Y_pred_pixels = save_luma_pred;
			block->UV_pred_pixels = save_chroma_pred;
		}

		int cbp_luma   = ProcessResidual_16x16_Luma(block);
		ENC_START_TIMER(13,"MB T Crb");
		int cbp_chroma = ProcessResidual_8x8_Chroma(block);	// U & V

		int sad = ComputeSAD_SSE2(block->Y_frame_buffer, MB_BLOCK_STRIDE, &block->Y_frame_reconstructed[block->Y_block_start_index], frame->stride_padded, 1);
		int ssd = SumSquaredDifference_16x16(block->Y_frame_buffer, MB_BLOCK_STRIDE, &block->Y_frame_reconstructed[block->Y_block_start_index], frame->stride_padded) +
				  SumSquaredDifference_16x16((uint8_t*) block->UV_frame_buffer, MB_BLOCK_STRIDE, (uint8_t*) &block->UV_frame_reconstructed[block->UV_block_start_index], frame->uv_stride_padded, 8);

		//		int uv_sad = ComputeSAD_SSE2((uint8_t*) block->UV_frame_buffer, MB_BLOCK_STRIDE, (uint8_t*) &block->UV_frame_reconstructed[block->UV_block_start_index], frame->uv_stride_padded, 0);
		ENC_PAUSE_TIMER(13,false);
		ENC_PAUSE_TIMER(3,false);

		int save_sad = sad;
		int additional_cost = 0;

		if (cbp_luma || cbp_chroma)
			additional_cost += block->coeff_cost * COEFF_BITCOST_LAMBDA;

		if (mode == 1)
			additional_cost += ((block->me_cost - block->me_sad) * MV_BITCOST_LAMBDA);
		else
			additional_cost += IPRED_COST;

		DEBUG_MB_DECISION;

#if 1	// turn off because at this point we just care about distortion
		ssd += (additional_cost * block->qp) >> 5;	// scale by qp to allow for more distortion
#endif
		if (ssd >= block->pskip_ssd)
		{
			mode = 2;	// pskip

#if 0	// metric testing
			static uint64_t total_pskip_ssd = 0;
			static int pskip_count = 0;

			total_pskip_ssd += block->pskip_ssd;
			pskip_count++;
			if ((pskip_count % 80) == 0)
			{
				ENC_TRACE("%7.3f\n", (float) total_pskip_ssd / (float) pskip_count);
			}

#endif
		}
		else
		{
			block->coded_block_pattern_luma   = cbp_luma;
			block->coded_block_pattern_chroma = (cbp_chroma & 0x2) ? 2 : cbp_chroma;
			frame->frame_mad += save_sad;
		}
	}
	//else
	//{
	//	block->coded_block_pattern_luma   = -1;	// means need to run transforms
	//}

	// restore saved pointers
	block->Y_pred_pixels = save_luma_pred;
	block->UV_pred_pixels = save_chroma_pred;

	block->data->refIdxL1 = -1;	// always for P slice

	if (mode == 0)	// I Prediction
	{
		block->mv[0] = block->mv[1] = 0;
		block->isI16x16 = true;
		block->data->refIdxL0 = -1;
	}
	else
	if (mode == 1)	// ME
	{
		// add to temporal mv table
		frame->prev_ref_frame->temporal_mv[block->mb_index].mx = block->mv[0] * frame->prev_ref_frame->temporal_mv_distscale;
		frame->prev_ref_frame->temporal_mv[block->mb_index].my = block->mv[1] * frame->prev_ref_frame->temporal_mv_distscale;

		SetMotionVectorDeltas16x16((uint32_t *) block->data->mvd_l0[0], block->mv[0] - block->pmv[0], block->mv[1] - block->pmv[1]);
		if (block->coded_block_pattern_luma == -1)	// means this wasn't set above
		{
			SetMotionVectors16x16((uint32_t *) block->data->mv_l0[0], block->mv[0], block->mv[1]);

			ME_Prediction(block, ME_PRED_DIR_BACK);	// can optimize by just copying from local save_luma/chroma_pred buffers

			block->isI16x16 = false;
		}

		block->intra_pred_mode = INTRA_PRED_MODE_DC;
		block->chroma_pred_mode = CHROMA_PRED_MODE_DC;
		block->data->refIdxL0 = 0;
	}
	else
	{	// PSKIP
		block->mb_skip_flag = true;

		// add to temporal mv table
		frame->prev_ref_frame->temporal_mv[block->mb_index].mx = block->pmv_skip[0][0] * frame->prev_ref_frame->temporal_mv_distscale;
		frame->prev_ref_frame->temporal_mv[block->mb_index].my = block->pmv_skip[0][1] * frame->prev_ref_frame->temporal_mv_distscale;

		// currently we just grab from the last frame - probably will need to be expanded
		SetMotionVectors16x16((uint32_t *) block->data->mv_l0[0], block->pmv_skip[0][0], block->pmv_skip[0][1]);

		uint8_t *y_buffer = block->skip_dst_y_buffer;
		for( int j = 0; j < 16; ++j, y_buffer += MB_BLOCK_STRIDE )
		{
			int idx = j * block->stride + block->Y_block_start_index;
			memcpy(&block->Y_frame_reconstructed[idx], y_buffer, 16);
		}

		frame->frame_mad += block->pskip_sad;

		uint8_t *uv_buffer = block->skip_dst_uv_buffer;
		for( int k = 0; k < 8; ++k, uv_buffer += MB_BLOCK_STRIDE)//(frame->uv_stride * 2) )
		{
			int idx = k * block->uv_stride + block->UV_block_start_index;
			memcpy(&block->UV_frame_reconstructed[idx], uv_buffer, 16);
		}

		block->intra_pred_mode = INTRA_PRED_MODE_DC;
		block->chroma_pred_mode = CHROMA_PRED_MODE_DC;
		block->coded_block_pattern_luma = 0;
		block->coded_block_pattern_chroma = 0;
		block->isI16x16 = false;
		block->data->refIdxL0 = 0;
		block->qp_delta = 0;
		if (block->mb_x > 0)
			block->qp = block->mbAddrA->qp;
	}
}

void FindBestPredictor_I(MacroBlockType *block, YUVFrameType *frame)
{
	block->coded_block_pattern_luma   = -1;	// means need to run transforms

	block->isI16x16 = true;
	block->data->refIdxL0 = -1;
	block->data->refIdxL1 = -1;
	block->mv[0] = block->mv[1] = 0;
}


void SetFrameForMBProcessing(YUVFrameType *frame)
{
	sCurrentMacroBlockSet = frame->mb_set;

	if (frame->slice_type == SLICE_TYPE_B)
	{
		FindBestPredictor = FindBestPredictor_B;
		CalcSKIP = CalcSKIP_B;
	}
	else
	if (frame->slice_type == SLICE_TYPE_P)
	{
		FindBestPredictor = FindBestPredictor_P;
		CalcSKIP = CalcSKIP_P;
	}
	else
	{
		FindBestPredictor = FindBestPredictor_I;
		CalcSKIP = CalcSKIP_I;
	}

	if (gEncoderState.SSSE3_enabled)
	{
		Quantize16x16_Luma_AC_16 = Quantize16x16_Luma_AC_16_SSSE3;
		Quantize8x16_Chroma_AC_16 = Quantize8x16_Chroma_AC_16_SSSE3;
	}
	else
	{
		Quantize16x16_Luma_AC_16 = Quantize16x16_Luma_AC_16_SSE2;
		Quantize8x16_Chroma_AC_16 = Quantize8x16_Chroma_AC_16_SSE2;
	}
}

void ProcessMacroBlock(YUVFrameType *frame, int index, int qp)
{
	MacroBlockType *block;
	int mb_x, mb_y;
	bool force_qp_block = false;	// hack fix for multi-threading issue with MB qp delta
	bool skip_ok = true;	// hack fix for multi-threading issue with MB qp delta

#if _DEBUG	
	gEncoderState.mb_debug_on = (((gEncoderState.total_frame_num >= 0) && (gEncoderState.total_frame_num <= 2)) && ((index >= 3179) && (index <= 3180)));
	gEncoderState.mb_debug_on = (((gEncoderState.total_frame_num >= 1) && (gEncoderState.total_frame_num <= 1)) && (index == 0));
	
	if (gEncoderState.mb_debug_on)
	{
		ENC_TRACE("Frame:%d MB: %d ============================\n", frame->frame_number, index);
	}

#endif

	ENC_START_TIMER(11,"MB Setup");

	MacroBlockType *current_mb_set = sCurrentMacroBlockSet;
	int frame_mb_width = gEncoderState.mb_width;
	__declspec(align(64)) uint8_t y_frame_block_buffer[16*16];			// cached input frame data for better cpu cache usage (since it is referenced often esp in ME)
	__declspec(align(64)) ChromaPixelType uv_frame_block_buffer[8*8];	// cached input frame data for better cpu cache usage (since it is referenced often esp in ME)
	__declspec(align(64)) uint8_t skip_dst_y_frame_buffer[16*16];			// cached skip data
	__declspec(align(64)) ChromaPixelType skip_dst_uv_frame_buffer[8*8];	// cached skip data

	mb_x = index % frame_mb_width;
	mb_y = index / frame_mb_width;

	block = &current_mb_set[index];

	block->mb_x = mb_x;
	block->mb_y = mb_y;
	block->mb_index = index;

	block->mbAddrA = (mb_x > 0) ? block-1 : NULL;			// left
	block->mbAddrB =  (mb_y > 0) ? &current_mb_set[index-frame_mb_width] : NULL;	// above
	block->mbAddrC = ((mb_x < (gEncoderState.mb_width-1)) && (mb_y > 0)) ? &current_mb_set[index-frame_mb_width+1] : NULL;	// above-right
	block->mbAddrD = ((mb_x > 0) && (mb_y > 0)) ? &current_mb_set[index-frame_mb_width-1] : NULL ;				// above-left;
	block->mb3rdNeighbor = (block->mbAddrC == NULL) ? block->mbAddrD : block->mbAddrC;

	ENC_ASSERT((qp >= 0) && (qp <= 51));
	if (mb_x > 0)
	{
		if ((mb_x >= (frame_mb_width - 2)))//&& (gEncoderState.multi_threading_on))
		{
			force_qp_block = true;
			block->qp = gEncoderState.qp + gEncoderState.current_qp_delta;
			skip_ok = (block->mbAddrA->qp == block->qp);	// if previous block is already the frame qp, then skips are okay.
		}
		else
			block->qp = qp;
			
		block->qp_delta = block->qp - block->mbAddrA->qp;
		block->prev_qp_delta = block->mbAddrA->qp_delta;
	}
	else
	{
		block->qp = gEncoderState.qp + gEncoderState.current_qp_delta;
		block->prev_qp_delta = 0;
		block->qp_delta = 0;
	}

	block->cbf_bits[0] = 0;

	block->stride = frame->stride_padded;
	block->uv_stride = frame->uv_width_padded;	// use frame->uv_width_padded because block->uv_stride is used with ChromaPixelType
	block->Y_frame_buffer = y_frame_block_buffer;
	block->UV_frame_buffer = uv_frame_block_buffer;

#if _DEBUG	
	if ((gEncoderState.total_frame_num == 7) && (block->mb_index == 55))
		ENC_TRACE("");
#endif

	FastCopyToBlock(y_frame_block_buffer, frame->Y_frame_buffer + ((frame->stride * mb_y + mb_x) * 16), frame->stride);
	FastCopyToBlock16x8(uv_frame_block_buffer, frame->UV_frame_buffer + ((frame->uv_stride * mb_y + mb_x) * 8), frame->stride);

	block->qparam = GetQuantParams();

	block->Y_pred_pixels = (u8_16x16_Type *) (frame->mb_aligned_data_buffer + ((sizeof(u8_16x16_Type) + sizeof(Chroma_8x8_Type) + sizeof(MBDataType)) * index));
	block->UV_pred_pixels = (Chroma_8x8_Type *) (block->Y_pred_pixels + 1);
	block->data = (MBDataType *) (block->UV_pred_pixels + 1);
	block->luma_bit_flags = 0;
	block->chroma_bit_flags = 0;
	block->luma_ipred_sad = INT_MAX;
	block->chroma_ipred_sad = INT_MAX;

	// pointer to reconstructed frame buffer
	block->Y_frame_reconstructed = frame->Y_frame_reconstructed;
	block->UV_frame_reconstructed = frame->UV_frame_reconstructed;

	// compute index at which current block starts within reconstructed frame
	int block_pixel_x = block->mb_x << 4; // assume 16x16 block
	int block_pixel_y = block->mb_y << 4; // assume 16x16 block
	block->Y_block_start_index = block_pixel_y * frame->stride_padded + block_pixel_x;
	block->UV_block_start_index = (block_pixel_y >> 1) * frame->uv_width_padded + (block_pixel_x >> 1);

#ifdef USE_QUARTER_PIXEL_ME
	block->Y_quarter_buffer = frame->Y_quarter_buffer + ((frame->stride * mb_y + mb_x) * 16);
#endif

	block->skip_dst_y_buffer  = skip_dst_y_frame_buffer;
	block->skip_dst_uv_buffer = (uint8_t *) skip_dst_uv_frame_buffer;

	frame->lambda_factor = 1;//10;//frame->lambda_table[qp * 3];

	// adjustable qp
	if (gEncoderState.variable_qp && (block->mb_x > 0) && !force_qp_block)
	{
		static const int complexity_to_qp_mapping[32] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };

		int val = CalcImageComplexity(block);
		int saveval = val;
        H264_UNUSED(saveval);
		int base_qp, complexity = 0;
		while (val > 0)
		{
			val >>= 1;
			complexity++;
		}

		// narrow the range of possible qp and compress it when getting closer to the 13-39 limits
		// this will improve the image quality by not subjecting the high detail stuff to get
		// destroyed when the base qp is low.
		static const int range[27] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,	// 13-22
			10,11,12,10,12,11,10, 9, 8, 7,	// 23-32
			6, 5, 4, 3, 2, 1, 0 };			// 33-39

		//		base_qp = (complexity_to_qp_mapping[complexity] * range[block->qp - 13] * 2 / 24) + (block->qp - range[block->qp - 13]);
		base_qp = (complexity_to_qp_mapping[complexity] * 10 * 2 / 24) + (block->qp - 14);

		base_qp = CLIP3(0, 51, base_qp);
		//		ENC_TRACE("%4d: val:%7d cplx:%2d qp:%2d map:%2d range:%2d\n", index, saveval, complexity, base_qp, complexity_to_qp_mapping[complexity], range[block->qp-13] );
		block->qp = base_qp;
		block->qp_delta = base_qp - block->mbAddrA->qp;
	}
#if 0
	else
		if ((block->mb_x > 0) && !force_qp_block)
		{
			if (((gEncoderState.total_frame_num == 1) || (gEncoderState.total_frame_num == 4) || (gEncoderState.total_frame_num == 5)) && (index > 1127))
			{
				block->qp = 31;
				block->qp_delta = 31 - block->mbAddrA->qp;
			}
		}
#endif


	ENC_PAUSE_TIMER(11,false);

	if (skip_ok)
	{
		CalcSKIP(block, frame);

		if (block->mb_skip_flag)
			goto Encode_MB;
	}
	else
	{
		block->mb_skip_flag = false;
		block->mb_direct_flag = false;
		block->pskip_ssd = INT_MAX;
	}

	ENC_START_TIMER(2,"MB Pred");

	block->luma_ipred_sad   = LumaPrediction(block);
	block->chroma_ipred_sad = ChromaPrediction(block);

	ENC_PAUSE_TIMER(2,false);

	FindBestPredictor(block, frame);

	if (block->coded_block_pattern_luma == -1)	// not yet done
	{
		ENC_START_TIMER(3,"MB Transform");
		int cbp_luma  = ProcessResidual_16x16_Luma(block);
		ENC_START_TIMER(13,"MB T Crb");
		int cbp_chroma  = ProcessResidual_8x8_Chroma(block);	// U & V
		block->coded_block_pattern_luma   = cbp_luma;
		block->coded_block_pattern_chroma = (cbp_chroma & 0x2) ? 2 : cbp_chroma;

		YUVFrameType *frame = gEncoderState.current_frame;
		int sad = ComputeSAD_SSE2(block->Y_frame_buffer, MB_BLOCK_STRIDE, &block->Y_frame_reconstructed[block->Y_block_start_index], frame->stride_padded, 1);
		frame->frame_mad += sad;
		ENC_PAUSE_TIMER(13,false);
		ENC_PAUSE_TIMER(3,false);
	}

	BuildMB_Type(block);

	if( !block->isI16x16 ) // use 'isP16x16' flag?
	{
		if (frame->slice_type == SLICE_TYPE_P)
			block->mb_type = MB_TYPE_P16x16;
		else
		{
			if ((block->data->refIdxL0 != -1) && (block->data->refIdxL1 != -1))
			{
				if (block->mb_direct_flag)
					block->mb_type = MB_TYPE_B_DIRECT_16x16;
				else
					block->mb_type = MB_TYPE_B_Bi_16x16;
			}
			else
			if (block->data->refIdxL0 != -1)
				block->mb_type = MB_TYPE_B_L0_16x16;
			else
				block->mb_type = MB_TYPE_B_L1_16x16;
		}

		if (block->coded_block_pattern_luma == 0 && block->coded_block_pattern_chroma == 0)
		{
			// clause 7.4.5 for mb_qp_delta states that if mb_qp_delta is not present delta_qp is inferred to be 0
			if (block->mb_x > 0)
				block->qp = block->mbAddrA->qp;
			else
				block->qp = gEncoderState.qp + gEncoderState.current_qp_delta;

			if (force_qp_block && (block->qp != qp))
			{
				block->coded_block_pattern_luma = 1;	// send out one blank set of coefficients so we can set qp
				block->qp = qp;
			}
			else
				block->qp_delta = 0;
		}
	}
	else
	{
		block->data->refIdxL0 = -1;
		block->data->refIdxL1 = -1;
	}

	ENC_ASSERT((skip_ok) || (!block->mb_skip_flag));
	ENC_ASSERT((!force_qp_block) || (block->qp == qp));
#if 0//def _DEBUG
	if (gEncoderState.is_B_slice && gEncoderState.total_frame_num == 34)
	{
		if (block->mb_type >= MB_TYPE_B_INxN)
		{
			ENC_TRACE("%4d: Mode:%2d Ref(%2d %2d)\n", block->mb_index, block->mb_skip_flag ? 0: block->mb_type, block->data->refIdxL0, block->data->refIdxL1);
		}
		else
		{
			ENC_TRACE("%4d: Mode:%2d Ref(%2d %2d) L0(%4d %4d) L1(%4d %4d)\n", block->mb_index, block->mb_skip_flag ? 0: block->mb_type, block->data->refIdxL0, block->data->refIdxL1, 
				block->data->mv_l0[0][0][0], block->data->mv_l0[1][0][0], block->data->mv_l1[0][0][0], block->data->mv_l1[1][0][0]);
		}
	}
#endif

Encode_MB:

	//	if (!gEncoderState.multi_threading_on)
	ENC_START_TIMER(4,"CABAC Encoding");
	CABAC_Encode_MB_Layer(block);
	ENC_PAUSE_TIMER(4,false);
}

int RC_ProcessIDR(YUVFrameType *frame, MacroBlockType *block)
{
	int cbp_luma;
	// we don't have neighbor blocks so let's just use the source blocks

	// need to set up neighbors
	block->mbAddrA = (block->mb_x > 0) ? block-1 : NULL;			// left
	block->mbAddrB =  (block->mb_y > 0) ? &sCurrentMacroBlockSet[block->mb_index-gEncoderState.mb_width] : NULL;	// above

	__declspec(align(64)) uint8_t y_frame_block_buffer_a[16*16];			// cached input frame data for better cpu cache usage (since it is referenced often esp in ME)
	__declspec(align(64)) uint8_t y_frame_block_buffer_b[16*16];			// cached input frame data for better cpu cache usage (since it is referenced often esp in ME)
	if (block->mbAddrA)	// horizontal
	{
		uint8_t *start_pixel_dst = y_frame_block_buffer_a;
		uint8_t *start_pixel_src = frame->Y_frame_buffer + ((frame->stride * block->mb_y + block->mb_x) * 16) - 1;
		for (int i = 0; i < 16; i++)
		{
			start_pixel_dst += 16;
			start_pixel_src += frame->stride;
			start_pixel_dst[15] = *start_pixel_src;
		}
		block->mbAddrA->stride = block->stride;
		block->mbAddrA->Y_frame_buffer = y_frame_block_buffer_a;
		block->mbAddrA->Y_block_start_index = block->Y_block_start_index - 16;
		block->mbAddrA->Y_frame_reconstructed = block->Y_frame_reconstructed;
	}
	if (block->mbAddrB)	// vertical
	{
		uint8_t *start_pixel_dst = &y_frame_block_buffer_b[15*16];	// last row
		uint8_t *start_pixel_src = frame->Y_frame_buffer + ((frame->stride * block->mb_y + block->mb_x) * 16) - frame->stride;
		memcpy(start_pixel_dst, start_pixel_src, 16);

		block->mbAddrB->stride = block->stride;
		block->mbAddrB->Y_frame_buffer = y_frame_block_buffer_b;
		block->mbAddrB->Y_block_start_index = block->Y_block_start_index - (frame->stride_padded * 16);
		block->mbAddrB->Y_frame_reconstructed = block->Y_frame_reconstructed;
	}

	LumaPrediction(block);
	block->coeff_cost = 0;
	cbp_luma  = ProcessResidual_16x16_Luma(block);

	return cbp_luma;
}

int RC_ProcessMacroBlock(YUVFrameType *frame, int index, int qp, int *mad_est)
{
	MacroBlockType *block;
	int mb_x, mb_y;
	__declspec(align(64)) uint8_t y_frame_block_buffer[16*16];			// cached input frame data for better cpu cache usage (since it is referenced often esp in ME)
	__declspec(align(64)) ChromaPixelType uv_frame_block_buffer[8*8];	// cached input frame data for better cpu cache usage (since it is referenced often esp in ME)

	ENC_START_TIMER(11,"MB Setup");

	MacroBlockType *current_mb_set = sCurrentMacroBlockSet;
	int frame_mb_width = gEncoderState.mb_width;
	mb_x = index % frame_mb_width;
	mb_y = index / frame_mb_width;

	block = &current_mb_set[index];

	block->mb_x = mb_x;
	block->mb_y = mb_y;
	block->mb_index = index;

	block->mbAddrA = NULL;			// left
	block->mbAddrB = NULL;	// above
	block->mbAddrC = NULL;	// above-right
	block->mbAddrD = NULL ;				// above-left;
	ENC_ASSERT((qp >= 0) && (qp <= 51));
	block->qp = qp;

	block->cbf_bits[0] = 0;

	block->stride = frame->stride_padded;
	block->uv_stride = frame->uv_width_padded;	// use frame->uv_width_padded because block->uv_stride is used with ChromaPixelType
	block->Y_frame_buffer = y_frame_block_buffer;
	block->UV_frame_buffer = uv_frame_block_buffer;

	FastCopyToBlock(y_frame_block_buffer, frame->Y_frame_buffer + ((frame->stride * mb_y + mb_x) * 16), frame->stride);
	FastCopyToBlock16x8(uv_frame_block_buffer, frame->UV_frame_buffer + ((frame->uv_stride * mb_y + mb_x) * 8), frame->stride);

	block->qparam = GetQuantParams();

	block->Y_pred_pixels = (u8_16x16_Type *) (frame->mb_aligned_data_buffer + ((sizeof(u8_16x16_Type) + sizeof(Chroma_8x8_Type) + sizeof(MBDataType)) * index));
	block->UV_pred_pixels = (Chroma_8x8_Type *) (block->Y_pred_pixels + 1);
	block->data = (MBDataType *) (block->UV_pred_pixels + 1);
	block->luma_bit_flags = 0;
	block->chroma_bit_flags = 0;

	// pointer to reconstructed frame buffer
	block->Y_frame_reconstructed = frame->Y_frame_reconstructed;
	block->UV_frame_reconstructed = frame->UV_frame_reconstructed;

	// compute index at which current block starts within reconstructed frame
	int block_pixel_x = block->mb_x << 4; // assume 16x16 block
	int block_pixel_y = block->mb_y << 4; // assume 16x16 block
	block->Y_block_start_index = block_pixel_y * frame->stride_padded + block_pixel_x;
	block->UV_block_start_index = (block_pixel_y >> 1) * frame->uv_width_padded + (block_pixel_x >> 1);

#ifdef USE_QUARTER_PIXEL_ME
	block->Y_quarter_buffer = frame->Y_quarter_buffer + ((frame->stride * mb_y + mb_x) * 16);
#endif

	frame->lambda_factor = 1;

	int cbp_luma;

	if (gEncoderState.idr)
	{
		cbp_luma = RC_ProcessIDR(frame, block);
	}
	else
	{
		ENC_PAUSE_TIMER(11,false);

		MotionVector mv;
		mv.mv_x = 0;
		mv.mv_y = 0;

		// we don't have a predicted mv so just take 0,0 and test for PSKIP
		SetMotionVectors16x16((uint32_t *) block->data->mv_l0[0], mv.mv_x, mv.mv_y);
		ME_Prediction(block, ME_PRED_DIR_BACK);

		// using SSD (sum of squared differences)
		int score_l  = SumSquaredDifference_16x16(block->Y_frame_buffer, MB_BLOCK_STRIDE, block->Y_pred_pixels->block[0], MB_BLOCK_STRIDE);
		int score_cr = 0;//SumSquaredDifference_16x16((uint8_t*) block->UV_frame_buffer, MB_BLOCK_STRIDE, block->skip_dst_uv_buffer, MB_BLOCK_STRIDE, 8);

		int pskip_ssd = score_l + score_cr; // bit cost factor (fake lambda)

		ENC_START_TIMER(10,"Motion Estimation");

		int C_p = 0x7FFFFFFF;
		block->mb_partition_type = 1; // P16x16
		int me_mad;
		mv.mv_x = 0;
		mv.mv_y = 0;
		C_p = ME_ComputeMotionVector(block, frame, frame->prev_ref_frame, &mv, &me_mad, true);

		SetMotionVectors16x16((uint32_t *) block->data->mv_l0[0], mv.mv_x, mv.mv_y);
		SetMotionVectorDeltas16x16((uint32_t *) block->data->mvd_l0[0], mv.mv_x, mv.mv_y);

		ME_Prediction(block, ME_PRED_DIR_BACK);
		block->isI16x16 = false;
		ENC_PAUSE_TIMER(10,false);

		block->intra_pred_mode = INTRA_PRED_MODE_DC;
		block->chroma_pred_mode = CHROMA_PRED_MODE_DC;
		block->data->refIdxL0 = 0;

		ENC_START_TIMER(3,"MB Transform");
		block->coeff_cost = 0;
		cbp_luma  = ProcessResidual_16x16_Luma(block);
	//	cbp_chroma  = ProcessResidual_8x8_Chroma(block);	// U & V
		block->coded_block_pattern_luma   = cbp_luma;
	//	block->coded_block_pattern_chroma = (cbp_chroma & 0x2) ? 2 : cbp_chroma;

		int me_ssd = SumSquaredDifference_16x16(&block->Y_frame_reconstructed[block->Y_block_start_index], frame->stride_padded, block->Y_frame_buffer, MB_BLOCK_STRIDE);

		int additional_cost = 0;
		int adj_cost;

		if (cbp_luma)
			additional_cost += block->coeff_cost * COEFF_BITCOST_LAMBDA;

		additional_cost += ((block->me_cost - block->me_sad) * MV_BITCOST_LAMBDA);

		adj_cost = (additional_cost * block->qp) >> 5;	// scale by qp to allow for more distortion 

		int ipred_ssd = 0;
		int mode = 0;
		int min_coeff_cost = block->coeff_cost;

		if (pskip_ssd < (me_ssd + adj_cost))
		{
			// now do IPRED check
			cbp_luma = RC_ProcessIDR(frame, block);

			ipred_ssd = SumSquaredDifference_16x16(&block->Y_frame_reconstructed[block->Y_block_start_index], frame->stride_padded, block->Y_frame_buffer, MB_BLOCK_STRIDE);
			additional_cost = IPRED_COST;
			if (cbp_luma)
				additional_cost += block->coeff_cost * COEFF_BITCOST_LAMBDA;

			adj_cost = (additional_cost * block->qp) >> 5;	// scale by qp to allow for more distortion

			if (pskip_ssd < (ipred_ssd + adj_cost))
			{
				if (min_coeff_cost > block->coeff_cost)
					min_coeff_cost = block->coeff_cost;

				block->coeff_cost = min_coeff_cost / 2;
				mode = 2;
			}
			else
				mode = 0;
		}
		else
			mode = 1;

//		ENC_TRACE("RC mb:%4d intra: %5d  me: %5d  skip: %5d mode:%d addcost:%5d adjcost: %5d coeff:%4d (%4d %4d)\n", block->mb_index, ipred_ssd, me_ssd, pskip_ssd, mode, additional_cost, adj_cost, block->coeff_cost, block->data->mv_l0[0][0][0], block->data->mv_l0[1][0][0]);
	}

	// calc SAD of reconstructed pixels and estimate residual (coefficient) cost
	int mad = ComputeSAD_SSE2(&block->Y_frame_reconstructed[block->Y_block_start_index], frame->stride_padded, block->Y_frame_buffer, MB_BLOCK_STRIDE, true);
	*mad_est = mad;

	// calc coeff estimate cost
	int residual_cost_est = 0;
	int num_of_coeff = 0;
	if (block->luma_bit_flags)
	{
		num_of_coeff = block->coeff_cost;
	}

	residual_cost_est = num_of_coeff;	// temp

	ENC_PAUSE_TIMER(3,false);

	return residual_cost_est;
}

#pragma warning(pop)