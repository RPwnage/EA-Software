#include <stdlib.h>
#include <memory.h>
#include "Origin_h264_encoder.h"
#include "Tables.h"
#include "MacroBlock.h"
#include "Trace.h"

#ifdef USE_SSE2
#include <emmintrin.h>
#endif

#define ENABLE_PLANE_PREDICTION_MODE  // on as it has been optimized some
//#define ENABLE_PLANE_PREDICTION_MODE_CHROMA  // off for now as it takes 1ms but is hardly used - perhaps we turn it on for higher quality modes

enum
{
	IPM_DC_AB,  // left and above
	IPM_DC_A,	// horizontal
	IPM_DC_B,   // vertical
	IPM_DC_DEF  // default
};

static inline int Calc_IPM_DC(MacroBlockType *block, uint8_t *v_pixels, uint8_t *h_pixels)
{
	int mb_x, mb_y;

	mb_x = block->mb_x;
	mb_y = block->mb_y;

	int pred_value = 0;
	// test left and above
	if ((mb_x > 0) && (mb_y > 0) && block->mbAddrA && block->mbAddrB)
	{
#ifdef USE_SSE2
        __m128i mm0, mm1, zero;
        zero = _mm_setzero_si128();
        mm0 = _mm_load_si128((__m128i *) (v_pixels));
        mm0 = _mm_sad_epu8(mm0, zero);
		mm1 = _mm_load_si128((__m128i *) (h_pixels));
		mm1 = _mm_sad_epu8(mm1, zero);
		mm0 = _mm_add_epi16(mm0, mm1);
        pred_value = mm0.m128i_i16[0] + mm0.m128i_i16[4];
#else
		MacroBlockType *pred_block_h = block->mbAddrA;
		MacroBlockType *pred_block_v = block->mbAddrB;
		for (int i = 0; i < 16; i++)
		{
			int idx_h = pred_block_h->Y_block_start_index + (i * pred_block_h->stride) + 15;
			int idx_v = pred_block_v->Y_block_start_index + (15 * pred_block_h->stride) + i;
			pred_value += pred_block_h->Y_frame_reconstructed[idx_h] + pred_block_v->Y_frame_reconstructed[idx_v];
		}
#endif
		pred_value = (pred_value + 16) >> 5;
	}
	else
	if ((mb_x > 0) && block->mbAddrA)	// horizontal
	{
#ifdef USE_SSE2
		__m128i mm0, zero;
		zero = _mm_setzero_si128();
		mm0 = _mm_load_si128((__m128i *) (h_pixels));
		mm0 = _mm_sad_epu8(mm0, zero);
		pred_value = mm0.m128i_i16[0] + mm0.m128i_i16[4];
#else
		MacroBlockType *pred_block = block->mbAddrA;
        uint8_t *ptr = &pred_block->Y_frame_reconstructed[pred_block->Y_block_start_index + 15];
        for (int i = 0; i < 16; i++, ptr += pred_block->stride_padded)
        {
            pred_value += *ptr;
        }
#endif
		pred_value = (pred_value + 8) >> 4;
	}
	else
	if ((mb_y > 0) && block->mbAddrB)	// vertical
	{
#ifdef USE_SSE2
        __m128i mm0, zero;
        zero = _mm_setzero_si128();
        mm0 = _mm_load_si128((__m128i *) v_pixels);
        mm0 = _mm_sad_epu8(mm0, zero);
        pred_value = mm0.m128i_i16[0] + mm0.m128i_i16[4];
#else
		MacroBlockType *pred_block = block->mbAddrB;
		for (int i = 0; i < 16; i++)
		{
			int idx = pred_block->Y_block_start_index + (15 * pred_block->stride_padded) + i; // can be computed outside of loop
			pred_value += pred_block->Y_frame_reconstructed[idx];
		}
#endif
		pred_value = (pred_value + 8) >> 4;
	}
	else
	{
		pred_value = (1<<7);
	}

	return pred_value;
}

static inline bool Calc_IPM_H(MacroBlockType *block, uint8_t pixels[16])
{
	if ((block->mb_x > 0) && block->mbAddrA)	// horizontal
	{
		MacroBlockType *pred_block = block->mbAddrA;

        uint8_t *ptr = &pred_block->Y_frame_reconstructed[pred_block->Y_block_start_index + 15];
        for (int i = 0; i < 16; i++, ptr += pred_block->stride)
        {
            pixels[i] = *ptr;
        }
		
		return true;
	}

	return false;
}

static inline bool Calc_IPM_V(MacroBlockType *block, uint8_t **pixels)
{
	if ((block->mb_y > 0) && block->mbAddrB)	// vertical
	{
		MacroBlockType *pred_block = block->mbAddrB;
		int idx = pred_block->Y_block_start_index + (15 * pred_block->stride);
		*pixels = &pred_block->Y_frame_reconstructed[idx];

		return true;
	}

	return false;
}

static inline bool Calc_IPM_Plane(MacroBlockType *block, uint8_t y_pred_pixels[16][16], uint8_t *h_pixels, uint8_t *v_pixels)//, int *tmp_score)
{
	// INTRA_PRED_MODE_PLANE (clause 8.3.3.4)
	if ((block->mb_x > 0) && (block->mb_y > 0) && block->mbAddrA && block->mbAddrB && block->mbAddrD)
	{
		int h = 0;
		int v = 0; 
#ifdef USE_SSE2
		//
		// 0 - [ 8]-[6]
		// 1 - [ 9]-[5]
		// 2 - [10]-[4]
		// 3 - [11]-[3]
		// 4 - [12]-[2]
		// 5 - [13]-[1]
		// 6 - [14]-[0]
		h +=  v_pixels[ 8] - v_pixels[6];
		v +=  h_pixels[ 8] - h_pixels[6];
		h += (v_pixels[ 9] - v_pixels[5]) * 2;
		v += (h_pixels[ 9] - h_pixels[5]) * 2;
		h += (v_pixels[10] - v_pixels[4]) * 3;
		v += (h_pixels[10] - h_pixels[4]) * 3;
		h += (v_pixels[11] - v_pixels[3]) * 4;
		v += (h_pixels[11] - h_pixels[3]) * 4;
		h += (v_pixels[12] - v_pixels[2]) * 5;
		v += (h_pixels[12] - h_pixels[2]) * 5;
		h += (v_pixels[13] - v_pixels[1]) * 6;
		v += (h_pixels[13] - h_pixels[1]) * 6;
		h += (v_pixels[14] - v_pixels[0]) * 7;
		v += (h_pixels[14] - h_pixels[0]) * 7;
		h += (v_pixels[15] - h_pixels[-1]) * 8;
		v += (h_pixels[15] - h_pixels[-1]) * 8;

		int a = 16 * (v_pixels[15] + h_pixels[15]);
		int b = (5 * v + 32) >> 6;
		int c = (5 * h + 32) >> 6;

#if 1

//		uint8_t *frame_buf = block->Y_frame_buffer;
//		__m128i frm;

		__declspec(align(64)) int16_t pred_pixels[8];

		__m128i mm0, mm1, sum, zero, mm_max;
		zero = sum = _mm_setzero_si128();
		mm_max = _mm_set1_epi16(255);

		for (int j = 0; j < 16; j++)
		{
			int ac = a + (c * (j - 7)) + 16;

			for (int i = 0; i < 8; i++)
			{
				pred_pixels[i] = static_cast<int16_t>(ac + (b * (i - 7)));
			}

			mm0 = _mm_load_si128((__m128i *) pred_pixels);
			mm0 = _mm_srai_epi16(mm0, 5);
			mm0 = _mm_max_epi16(mm0, zero);
			mm0 = _mm_min_epi16(mm0, mm_max);

			for (int i = 8; i < 16; i++)
			{
				pred_pixels[i-8] = static_cast<int16_t>(ac + (b * (i - 7)));
			}

			mm1 = _mm_load_si128((__m128i *) pred_pixels);
			mm1 = _mm_srai_epi16(mm1, 5);
			mm1 = _mm_max_epi16(mm1, zero);
			mm1 = _mm_min_epi16(mm1, mm_max);
			mm0 = _mm_packus_epi16(mm0, mm1);

			_mm_store_si128((__m128i *) &y_pred_pixels[j], mm0);	// store 16 pixels
#if 0
			frm = _mm_load_si128((__m128i *) frame_buf);
			frame_buf += block->stride_padded;
			mm0 = _mm_sad_epu8(mm0, frm);
			sum = _mm_add_epi16(sum, mm0);
#endif
		}

//		*tmp_score = sum.m128i_i16[0] + sum.m128i_i16[4];

#else
		for (int j = 0; j < 16; j++)
		{
			for (int i = 0; i < 16; i++)
			{
				y_pred_pixels[j][i] = CLIP1((a + (b * (i - 7)) + (c * (j - 7)) + 16) >> 5);
			}
		}
#endif
#else
		int save_h = h;
		int save_v = v;
		h = 0;
		v = 0;
		MacroBlockType *pred_blockA = block->mbAddrA;
		MacroBlockType *pred_blockB = block->mbAddrB;

		for (int i = 0; i < 7; i++)
		{
			int idx_h0 = pred_blockB->Y_block_start_index + (15 * pred_blockB->stride) + (i + 8);
			int idx_h1 = pred_blockB->Y_block_start_index + (15 * pred_blockB->stride) + (6 - i);
			int idx_v0 = pred_blockA->Y_block_start_index + ( (i + 8) * pred_blockA->stride) + 15;
			int idx_v1 = pred_blockA->Y_block_start_index + ( (6 - i) * pred_blockA->stride) + 15;

			h += (i + 1) * (pred_blockB->Y_frame_reconstructed[idx_h0] - pred_blockB->Y_frame_reconstructed[idx_h1]);
			v += (i + 1) * (pred_blockA->Y_frame_reconstructed[idx_v0] - pred_blockA->Y_frame_reconstructed[idx_v1]);
		}
		int idx_h0 = pred_blockB->Y_block_start_index + (15 * pred_blockB->stride) + (7 + 8);
		int idx_h1 = block->mbAddrD->Y_block_start_index + (15 * block->mbAddrD->stride) + 15;
		int idx_v0 = pred_blockA->Y_block_start_index + ( (7 + 8) * pred_blockA->stride) + 15;
		int idx_v1 = block->mbAddrD->Y_block_start_index + ( 15 * block->mbAddrD->stride) + 15;

		h += (7 + 1) * (pred_blockB->Y_frame_reconstructed[idx_h0]  - block->mbAddrD->Y_frame_reconstructed[idx_h1]);
		v += (7 + 1) * (pred_blockA->Y_frame_reconstructed[idx_v0]  - block->Y_frame_reconstructed[idx_v1]);

		int idx_a0 = pred_blockA->Y_block_start_index + (15 * pred_blockA->stride) + 15;
		int idx_a1 = pred_blockB->Y_block_start_index + (15 * pred_blockB->stride) + 15;

		int ax = 16 * (pred_blockA->Y_frame_reconstructed[idx_a0] + pred_blockB->Y_frame_reconstructed[idx_a1]);
		int bx = (5 * h + 32) >> 6;
		int cx = (5 * v + 32) >> 6;

		if (ax != a)
		{
			ENC_TRACE("");
		}
		if (bx != b)
		{
			ENC_TRACE("");
		}
		if (cx != c)
		{
			ENC_TRACE("");
		}

		for (int j = 0; j < 16; j++)
		{
			for (int i = 0; i < 16; i++)
			{
				y_pred_pixels[j][i] = CLIP1((ax + (bx * (i - 7)) + (cx * (j - 7)) + 16) >> 5);
			}
		}
#endif
		
		return true;
	}

	return false;
}

// Chroma versions
static inline void Calc_IPM_DC_UV(MacroBlockType *block, ChromaPixelType pred_pixels[4])
{
	int sum_left[2][4] = {{ 0, 0, 0, 0}, { 0, 0, 0, 0}};
	int sum_up[2][4]   = {{ 0, 0, 0, 0}, { 0, 0, 0, 0}};

	// test left and above
	if ((block->mb_x > 0) && (block->mb_y > 0) && block->mbAddrA && block->mbAddrB)
	{
		MacroBlockType *pred_block_h = block->mbAddrA;
		MacroBlockType *pred_block_v = block->mbAddrB;

		for (int i = 0; i < 4; i++)
		{
			int idx_h0 = pred_block_h->UV_block_start_index + (i * pred_block_h->uv_stride) + 7;
			int idx_h1 = pred_block_h->UV_block_start_index + ( (i + 4) * pred_block_h->uv_stride) + 7;
			int idx_v0 = pred_block_v->UV_block_start_index + (7 * pred_block_v->uv_stride) + i;
			int idx_v1 = pred_block_v->UV_block_start_index + (7 * pred_block_v->uv_stride) + (i + 4);

			sum_left[0][0] += pred_block_h->UV_frame_reconstructed[idx_h0].u;
			//				sum_left[0][1] += pred_block_h->UV_frame_reconstructed[idx_h0].u;
			sum_left[0][2] += pred_block_h->UV_frame_reconstructed[idx_h1].u;
			sum_left[0][3] += pred_block_h->UV_frame_reconstructed[idx_h1].u;

			sum_left[1][0] += pred_block_h->UV_frame_reconstructed[idx_h0].v;
			//				sum_left[1][1] += pred_block_h->UV_frame_reconstructed[idx_h0].v;
			sum_left[1][2] += pred_block_h->UV_frame_reconstructed[idx_h1].v;
			sum_left[1][3] += pred_block_h->UV_frame_reconstructed[idx_h1].v;

			sum_up[0][0] += pred_block_v->UV_frame_reconstructed[idx_v0].u;
			sum_up[0][1] += pred_block_v->UV_frame_reconstructed[idx_v1].u;
			//				sum_up[0][2] += pred_block_v->UV_frame_reconstructed[idx_v0].u;
			sum_up[0][3] += pred_block_v->UV_frame_reconstructed[idx_v1].u;

			sum_up[1][0] += pred_block_v->UV_frame_reconstructed[idx_v0].v;
			sum_up[1][1] += pred_block_v->UV_frame_reconstructed[idx_v1].v;
			//				sum_up[1][2] += pred_block_v->UV_frame_reconstructed[idx_v0].v;
			sum_up[1][3] += pred_block_v->UV_frame_reconstructed[idx_v1].v;
		}

		pred_pixels[0].u = static_cast<uint8_t>((sum_left[0][0] + sum_up[0][0] + 4) >> 3);
		pred_pixels[0].v = static_cast<uint8_t>((sum_left[1][0] + sum_up[1][0] + 4) >> 3);
		pred_pixels[1].u = static_cast<uint8_t>(                 (sum_up[0][1] + 2) >> 2);
		pred_pixels[1].v = static_cast<uint8_t>(                 (sum_up[1][1] + 2) >> 2);
		pred_pixels[2].u = static_cast<uint8_t>((sum_left[0][2] + 2) >> 2);
		pred_pixels[2].v = static_cast<uint8_t>((sum_left[1][2] + 2) >> 2);
		pred_pixels[3].u = static_cast<uint8_t>((sum_left[0][3] + sum_up[0][3] + 4) >> 3);
		pred_pixels[3].v = static_cast<uint8_t>((sum_left[1][3] + sum_up[1][3] + 4) >> 3);
#if 0
		for (int x = 0; x < 4; x++)
		{
			int x_off = ((x & 1) * 4);
			int y_off = (x >> 1) * 4;
			for (int i = 0; i < 4; i++)
			{
				uv_pred_pixels[y_off+i][x_off].u = uv_pred_pixels[y_off+i][x_off+1].u = uv_pred_pixels[y_off+i][x_off+2].u = uv_pred_pixels[y_off+i][x_off+3].u = pred_value_u[x];
				uv_pred_pixels[y_off+i][x_off].v = uv_pred_pixels[y_off+i][x_off+1].v = uv_pred_pixels[y_off+i][x_off+2].v = uv_pred_pixels[y_off+i][x_off+3].v = pred_value_v[x];
			}
		}
#endif
	}
	else
	if ((block->mb_x > 0) && block->mbAddrA)	// horizontal
	{
		MacroBlockType *pred_block_h = block->mbAddrA;
		for (int i = 0; i < 4; i++)
		{
			int idx_h0 = pred_block_h->UV_block_start_index + (i * pred_block_h->uv_stride) + 7;
			int idx_h1 = pred_block_h->UV_block_start_index + ( (i + 4) * pred_block_h->uv_stride) + 7;

			sum_left[0][0] += pred_block_h->UV_frame_reconstructed[idx_h0].u;
			sum_left[0][1] += pred_block_h->UV_frame_reconstructed[idx_h0].u;
			sum_left[0][2] += pred_block_h->UV_frame_reconstructed[idx_h1].u;
			sum_left[0][3] += pred_block_h->UV_frame_reconstructed[idx_h1].u;

			sum_left[1][0] += pred_block_h->UV_frame_reconstructed[idx_h0].v;
			sum_left[1][1] += pred_block_h->UV_frame_reconstructed[idx_h0].v;
			sum_left[1][2] += pred_block_h->UV_frame_reconstructed[idx_h1].v;
			sum_left[1][3] += pred_block_h->UV_frame_reconstructed[idx_h1].v;
		}

		for (int i = 0; i < 4; i++)
		{
			pred_pixels[i].u = static_cast<uint8_t>((sum_left[0][i] + 2) >> 2);
			pred_pixels[i].v = static_cast<uint8_t>((sum_left[1][i] + 2) >> 2);
		}
		
#if 0
		for (int x = 0; x < 4; x++)
		{
			int x_off = ((x & 1) * 4);
			int y_off = (x >> 1) * 4;
			for (int i = 0; i < 4; i++)
			{
				uv_pred_pixels[y_off+i][x_off].u = uv_pred_pixels[y_off+i][x_off+1].u = uv_pred_pixels[y_off+i][x_off+2].u = uv_pred_pixels[y_off+i][x_off+3].u = (sum_left[0][x] + 2) >> 2;
				uv_pred_pixels[y_off+i][x_off].v = uv_pred_pixels[y_off+i][x_off+1].v = uv_pred_pixels[y_off+i][x_off+2].v = uv_pred_pixels[y_off+i][x_off+3].v = (sum_left[1][x] + 2) >> 2;
			}
		}
#endif
	}
	else
	if ((block->mb_y > 0) && block->mbAddrB)	// vertical
	{
		MacroBlockType *pred_block_v = block->mbAddrB;
		for (int i = 0; i < 4; i++)
		{
			int idx_v0 = pred_block_v->UV_block_start_index + (7 * pred_block_v->uv_stride) + i;
			int idx_v1 = pred_block_v->UV_block_start_index + (7 * pred_block_v->uv_stride) + (i + 4);

			sum_up[0][0] += pred_block_v->UV_frame_reconstructed[idx_v0].u;
			sum_up[0][1] += pred_block_v->UV_frame_reconstructed[idx_v1].u;
			sum_up[0][2] += pred_block_v->UV_frame_reconstructed[idx_v0].u;
			sum_up[0][3] += pred_block_v->UV_frame_reconstructed[idx_v1].u;

			sum_up[1][0] += pred_block_v->UV_frame_reconstructed[idx_v0].v;
			sum_up[1][1] += pred_block_v->UV_frame_reconstructed[idx_v1].v;
			sum_up[1][2] += pred_block_v->UV_frame_reconstructed[idx_v0].v;
			sum_up[1][3] += pred_block_v->UV_frame_reconstructed[idx_v1].v;
		}

		for (int i = 0; i < 4; i++)
		{
			pred_pixels[i].u = static_cast<uint8_t>((sum_left[0][i] + 2) >> 2);
			pred_pixels[i].v = static_cast<uint8_t>((sum_left[1][i] + 2) >> 2);
		}
#if 0
		for (int x = 0; x < 4; x++)
		{
			int x_off = ((x & 1) * 4);
			int y_off = (x >> 1) * 4;
			for (int i = 0; i < 4; i++)
			{
				uv_pred_pixels[y_off+i][x_off].u = uv_pred_pixels[y_off+i][x_off+1].u = uv_pred_pixels[y_off+i][x_off+2].u = uv_pred_pixels[y_off+i][x_off+3].u = (sum_up[0][x] + 2) >> 2;
				uv_pred_pixels[y_off+i][x_off].v = uv_pred_pixels[y_off+i][x_off+1].v = uv_pred_pixels[y_off+i][x_off+2].v = uv_pred_pixels[y_off+i][x_off+3].v = (sum_up[1][x] + 2) >> 2;
			}
		}
#endif
	}
	else
	{
		// set to same value
		// NEEDS SSE2 OPT
		memset((uint8_t *)pred_pixels, 1 << 7, 8);
	}
}

static inline bool Calc_IPM_H_UV(MacroBlockType *block, ChromaPixelType pixels[8])
{
	if ((block->mb_x > 0) && block->mbAddrA)	// horizontal
	{
		MacroBlockType *pred_block = block->mbAddrA;

        ChromaPixelType *ptr = pred_block->UV_frame_reconstructed + pred_block->UV_block_start_index + 7;
		for (int i = 0; i < 8; i++, ptr += pred_block->uv_stride)
		{
			pixels[i] = *ptr;
		}

		return true;
	}

	return false;
}

static inline bool Calc_IPM_V_UV(MacroBlockType *block, ChromaPixelType **pixels)
{
	if ((block->mb_y > 0) && block->mbAddrB)	// vertical
	{
		MacroBlockType *pred_block = block->mbAddrB;
		int idx = pred_block->UV_block_start_index + (7 * pred_block->uv_stride);
		*pixels = &pred_block->UV_frame_reconstructed[idx];

		return true;
	}

	return false;
}

static inline bool Calc_IPM_Plane_UV(MacroBlockType *block, ChromaPixelType uv_pred_pixels[8][8])
{
	// Chroma Plane Mode (clause 8.3.4.4)
	// will need to be adjusted for ChromaArrayType > 1 (assumes ChromaArrayType == 1)
	//		const int chroma_array_type_temp = 1;
	if ((block->mb_x > 0) && (block->mb_y > 0) && block->mbAddrA && block->mbAddrB && block->mbAddrD)
	{
		MacroBlockType *pred_blockA = block->mbAddrA;
		MacroBlockType *pred_blockB = block->mbAddrB;

		int h = 0;
		int v = 0; 
		int hv = 0;
		int vv = 0; 

		for (int i = 0; i < 3; i++)
		{
			int idx_h0 = pred_blockB->UV_block_start_index + (7 * pred_blockB->uv_stride) + (i + 4);
			int idx_h1 = pred_blockB->UV_block_start_index + (7 * pred_blockB->uv_stride) + (2 - i);
			// NEEDS SSE2 OPT
			h  += (i + 1) * (pred_blockB->UV_frame_reconstructed[idx_h0].u  - pred_blockB->UV_frame_reconstructed[idx_h1].u);
			hv += (i + 1) * (pred_blockB->UV_frame_reconstructed[idx_h0].v  - pred_blockB->UV_frame_reconstructed[idx_h1].v);
		}

		int idx_h0 = pred_blockB->UV_block_start_index + (7 * pred_blockB->uv_stride) + (3 + 4);
		int idx_h1 = block->mbAddrD->UV_block_start_index + (7 * block->mbAddrD->uv_stride) + 7;
		// NEEDS SSE2 OPT
		h  += (3 + 1) * (pred_blockB->UV_frame_reconstructed[idx_h0].u - block->mbAddrD->UV_frame_reconstructed[idx_h1].u);
		hv += (3 + 1) * (pred_blockB->UV_frame_reconstructed[idx_h0].v - block->mbAddrD->UV_frame_reconstructed[idx_h1].v);

		for (int i = 0; i < 3; i++)
		{
			int idx_v0 = pred_blockA->UV_block_start_index + ( (4 + i) * pred_blockA->uv_stride) + 7;
			int idx_v1 = pred_blockA->UV_block_start_index + ( (2 - i) * pred_blockA->uv_stride) + 7;
			// NEEDS SSE2 OPT
			v  += (i + 1) * (pred_blockA->UV_frame_reconstructed[idx_v0].u  - pred_blockA->UV_frame_reconstructed[idx_v1].u);
			vv += (i + 1) * (pred_blockA->UV_frame_reconstructed[idx_v0].v  - pred_blockA->UV_frame_reconstructed[idx_v1].v);
		}
		int idx_v0 = pred_blockA->UV_block_start_index + ( (4 + 3) * pred_blockA->uv_stride) + 7;
		int idx_v1 = block->mbAddrD->UV_block_start_index + ( 7 * block->mbAddrD->uv_stride) + 7;
		// NEEDS SSE2 OPT
		v  += (3 + 1) * (pred_blockA->UV_frame_reconstructed[idx_v0].u  - block->mbAddrD->UV_frame_reconstructed[idx_v1].u);
		vv += (3 + 1) * (pred_blockA->UV_frame_reconstructed[idx_v0].v  - block->mbAddrD->UV_frame_reconstructed[idx_v1].v);

		int idx_a0 = pred_blockA->UV_block_start_index + ( 7 * pred_blockA->uv_stride) + 7;
		int idx_a1 = pred_blockB->UV_block_start_index + ( 7 * pred_blockB->uv_stride) + 7;

		// NEEDS SSE2 OPT
		int a  = 16 * (pred_blockA->UV_frame_reconstructed[idx_a0].u + pred_blockB->UV_frame_reconstructed[idx_a1].u);
		int av = 16 * (pred_blockA->UV_frame_reconstructed[idx_a0].v + pred_blockB->UV_frame_reconstructed[idx_a1].v);

		int b = ((34 ) * h + 32) >> 6;
		int c = ((34 ) * v + 32) >> 6;
		int bv = ((34 ) * hv + 32) >> 6;
		int cv = ((34 ) * vv + 32) >> 6;

		for (int j = 0; j < 8; j++)
		{
			for (int i = 0; i < 8; i++)
			{
				uv_pred_pixels[j][i].u = static_cast<uint8_t>(CLIP1((a  + (b  * (i - 3)) + (c  * (j - 3)) + 16) >> 5));
				uv_pred_pixels[j][i].v = static_cast<uint8_t>(CLIP1((av + (bv * (i - 3)) + (cv * (j - 3)) + 16) >> 5));
			}
		}

		return true;
	}

	return false;
}

int LumaPrediction(MacroBlockType *block)
{
	int score = 0x7fffffff;
	int best_method = CHROMA_PRED_MODE_DC;

	int tmp_score;
	uint8_t *frame_buf;
    int dc_pixel;
    __m128i setpixels;
    __m128i mm0, mm1, sum;
    __m128i mm_v_pixels = _mm_setzero_si128();

	// start with H - can use h_pixels later in both DC and Plane
	__declspec(align(64)) uint8_t h_pixels[16];

	if (Calc_IPM_H(block, h_pixels))
	{
		frame_buf = block->Y_frame_buffer;	// must reset each time

		setpixels = _mm_set1_epi8(h_pixels[0]);
		mm0 = _mm_load_si128((__m128i *) frame_buf);
		sum = _mm_sad_epu8(mm0, setpixels);
		for (int i = 1; i < 16; i++)
		{
			frame_buf += MB_BLOCK_STRIDE;
			mm0 = _mm_load_si128((__m128i *) frame_buf);
			setpixels = _mm_set1_epi8(h_pixels[i]);
			mm1 = _mm_sad_epu8(mm0, setpixels);
			sum = _mm_add_epi16(sum, mm1);
		}

		score = sum.m128i_i16[0] + sum.m128i_i16[4];
		best_method = INTRA_PRED_MODE_H;
	}

	// next try V
	__declspec(align(64)) uint8_t *v_pixels;

	if (Calc_IPM_V(block, &v_pixels))
	{
		frame_buf = block->Y_frame_buffer;	// must reset each time

		mm_v_pixels = _mm_load_si128((__m128i *) v_pixels);
		mm0 = _mm_load_si128((__m128i *) frame_buf);
		sum = _mm_sad_epu8(mm0, mm_v_pixels);
		for (int i = 1; i < 16; i++)
		{
			frame_buf += MB_BLOCK_STRIDE;
			mm0 = _mm_load_si128((__m128i *) frame_buf);
			mm1 = _mm_sad_epu8(mm0, mm_v_pixels);
			sum = _mm_add_epi16(sum, mm1);
		}

		tmp_score = sum.m128i_i16[0] + sum.m128i_i16[4];
		if (tmp_score < score)
		{
			score = tmp_score;
			best_method = INTRA_PRED_MODE_V;
		}
	}


	// try DC 3rd
	frame_buf = block->Y_frame_buffer;
	dc_pixel = Calc_IPM_DC(block, v_pixels, h_pixels);

	setpixels = _mm_set1_epi8(static_cast<char>(dc_pixel));
	mm0 = _mm_load_si128((__m128i *) frame_buf);
	sum = _mm_sad_epu8(mm0, setpixels);

	for (int i = 1; i < 16; i++)
	{
		frame_buf += MB_BLOCK_STRIDE;
		mm0 = _mm_load_si128((__m128i *) frame_buf);
		mm1 = _mm_sad_epu8(mm0, setpixels);
		sum = _mm_add_epi16(sum, mm1);
	}

	tmp_score = sum.m128i_i16[0] + sum.m128i_i16[4];
	if (tmp_score < score)
	{
		score = tmp_score;
		best_method = INTRA_PRED_MODE_DC;
	}

#ifdef ENABLE_PLANE_PREDICTION_MODE
	// finally try Plane mode - this time we will load the pixels into the Y_pred_pixels buffer so if this is chosen, we will leave it.  
	if (Calc_IPM_Plane(block, block->Y_pred_pixels->block, v_pixels, h_pixels))
	{
#if 1	// do it in Calc_IPM_Plane() to avoid extra loads
		frame_buf = block->Y_frame_buffer;	// must reset each time

		setpixels = _mm_load_si128((__m128i *) block->Y_pred_pixels->block[0]);
		mm0 = _mm_load_si128((__m128i *) frame_buf);
		sum = _mm_sad_epu8(mm0, setpixels);
		for (int i = 1; i < 16; i++)
		{
			setpixels = _mm_load_si128((__m128i *) block->Y_pred_pixels->block[i]);
			frame_buf += MB_BLOCK_STRIDE;
			mm0 = _mm_load_si128((__m128i *) frame_buf);
			mm1 = _mm_sad_epu8(mm0, setpixels);
			sum = _mm_add_epi16(sum, mm1);
		}

		tmp_score = sum.m128i_i16[0] + sum.m128i_i16[4];
#endif
		if (tmp_score < score)
		{
			block->intra_pred_mode = INTRA_PRED_MODE_PLANE;
			return tmp_score;	// we are done
		}
	}
#endif

	// apply pixels
	if (best_method == INTRA_PRED_MODE_DC)
	{
		memset(block->Y_pred_pixels->block, dc_pixel, 16*16);
		block->intra_pred_mode = INTRA_PRED_MODE_DC;
	}
	else
	if (best_method == INTRA_PRED_MODE_H)
	{
		for (int i = 0; i < 16; i++)
		{
			memset(block->Y_pred_pixels->block[i], h_pixels[i], 16);
		}
		block->intra_pred_mode = INTRA_PRED_MODE_H;
	}
	else
	if (best_method == INTRA_PRED_MODE_V)
	{
		for (int i = 0; i < 16; i++)
		{
			_mm_store_si128((__m128i *) block->Y_pred_pixels->block[i], mm_v_pixels);
		}
		block->intra_pred_mode = INTRA_PRED_MODE_V;
	}
	else
	{
		ENC_ASSERT(0);	// shouldn't get here
	}
	return score;
}

int ChromaPrediction(MacroBlockType *block)
{
	int score;
	int best_method = CHROMA_PRED_MODE_DC;

	int tmp_score;
	ChromaPixelType *frame_buf;
    uint16_t *ptr16;
    __m128i mm0, mm1, sum, dc_setpixels0, dc_setpixels1;
	/*ChromaPixelType **/frame_buf = block->UV_frame_buffer;

	// start with DC
	__declspec(align(64)) ChromaPixelType dc_pixels[4];
	Calc_IPM_DC_UV(block, dc_pixels);

	/*uint16_t **/ptr16 = (uint16_t *) dc_pixels;

	/*__m128i*/ mm0 = _mm_set1_epi16(ptr16[0]);
	mm0 = _mm_slli_si128(mm0, 8);
	mm0 = _mm_srli_si128(mm0, 8);
	/*__m128i*/ mm1 = _mm_set1_epi16(ptr16[1]);
	mm1 = _mm_slli_si128(mm1, 8);

	/*__m128i*/ dc_setpixels0 = _mm_or_si128(mm0, mm1);	// save for possible application
	// bottom half
	mm0 = _mm_set1_epi16(ptr16[2]);
	mm0 = _mm_slli_si128(mm0, 8);
	mm0 = _mm_srli_si128(mm0, 8);
	mm1 = _mm_set1_epi16(ptr16[3]);
	mm1 = _mm_slli_si128(mm1, 8);

	/*__m128i*/ dc_setpixels1 = _mm_or_si128(mm0, mm1);	// save for possible application

	mm0 = _mm_load_si128((__m128i *) frame_buf);
	sum = _mm_sad_epu8(mm0, dc_setpixels0);

	for (int i = 1; i < 4; i++)
	{
		frame_buf += 8;
		mm0 = _mm_load_si128((__m128i *) frame_buf);
		mm1 = _mm_sad_epu8(mm0, dc_setpixels0);
		sum = _mm_add_epi16(sum, mm1);
	}
	for (int i = 0; i < 4; i++)
	{
		frame_buf += 8;
		mm0 = _mm_load_si128((__m128i *) frame_buf);
		mm1 = _mm_sad_epu8(mm0, dc_setpixels1);
		sum = _mm_add_epi16(sum, mm1);
	}

	score = sum.m128i_i16[0] + sum.m128i_i16[4];

	// next try H
	__declspec(align(64)) ChromaPixelType h_pixels[8];

	if (Calc_IPM_H_UV(block, h_pixels))
	{
		frame_buf = block->UV_frame_buffer;	// must reset for each test

		ptr16 = (uint16_t *)h_pixels;

		__m128i setpixels = _mm_set1_epi16(ptr16[0]);
		mm0 = _mm_load_si128((__m128i *) frame_buf);
		sum = _mm_sad_epu8(mm0, setpixels);
		for (int i = 1; i < 8; i++)
		{
			frame_buf += 8;
			mm0 = _mm_load_si128((__m128i *) frame_buf);
			setpixels = _mm_set1_epi16(ptr16[i]);
			mm1 = _mm_sad_epu8(mm0, setpixels);
			sum = _mm_add_epi16(sum, mm1);
		}

		tmp_score = sum.m128i_i16[0] + sum.m128i_i16[4];

		if (tmp_score < score)
		{
			score = tmp_score;
			best_method = CHROMA_PRED_MODE_H;
		}
	}

	// next try V
	__declspec(align(64)) ChromaPixelType *v_pixels;
	__m128i mm_v_pixels = _mm_setzero_si128();

	if (Calc_IPM_V_UV(block, &v_pixels))
	{
		frame_buf = block->UV_frame_buffer;	// must reset for each test

		mm_v_pixels = _mm_load_si128((__m128i *) v_pixels);
		mm0 = _mm_load_si128((__m128i *) frame_buf);
		sum = _mm_sad_epu8(mm0, mm_v_pixels);
		for (int i = 1; i < 8; i++)
		{
			frame_buf += 8;
			mm0 = _mm_load_si128((__m128i *) frame_buf);
			mm1 = _mm_sad_epu8(mm0, mm_v_pixels);
			sum = _mm_add_epi16(sum, mm1);
		}

		tmp_score = sum.m128i_i16[0] + sum.m128i_i16[4];

		if (tmp_score < score)
		{
			score = tmp_score;
			best_method = CHROMA_PRED_MODE_V;
		}
	}

#ifdef ENABLE_PLANE_PREDICTION_MODE_CHROMA
	// finally try Plane mode - this time we will load the pixels into the UV_pred_pixels buffer so if this is chosen, we will leave it.  
	if (Calc_IPM_Plane_UV(block, block->UV_pred_pixels->block))
	{
		frame_buf = block->UV_frame_buffer;	// must reset for each test

		__m128i setpixels = _mm_load_si128((__m128i *) block->UV_pred_pixels->block[0]);
		mm0 = _mm_load_si128((__m128i *) frame_buf);
		sum = _mm_sad_epu8(mm0, setpixels);
		for (int i = 1; i < 8; i++)
		{
			setpixels = _mm_load_si128((__m128i *) block->UV_pred_pixels->block[i]);
			frame_buf += block->uv_stride;
			mm0 = _mm_load_si128((__m128i *) frame_buf);
			mm1 = _mm_sad_epu8(mm0, setpixels);
			sum = _mm_add_epi16(sum, mm1);
		}

		tmp_score = sum.m128i_i16[0] + sum.m128i_i16[4];
		if (tmp_score < score)
		{
			block->chroma_pred_mode = CHROMA_PRED_MODE_PLANE;
			return tmp_score;	// we are done
		}
	}
#endif
	// apply pixels
	if (best_method == CHROMA_PRED_MODE_DC)
	{
		for (int i = 0; i < 4; i++)
		{
			_mm_store_si128((__m128i *) block->UV_pred_pixels->block[i  ], dc_setpixels0);
			_mm_store_si128((__m128i *) block->UV_pred_pixels->block[i+4], dc_setpixels1);
		}
		block->chroma_pred_mode = CHROMA_PRED_MODE_DC;
	}
	else
	if (best_method == CHROMA_PRED_MODE_H)
	{
		ptr16 = (uint16_t *)h_pixels;
		__m128i setpixels;

		for (int i = 0; i < 8; i++)
		{
			setpixels = _mm_set1_epi16(ptr16[i]);
			_mm_store_si128((__m128i *) block->UV_pred_pixels->block[i], setpixels);
		}
		block->chroma_pred_mode = CHROMA_PRED_MODE_H;
	}
	else
	if (best_method == CHROMA_PRED_MODE_V)
	{
		for (int i = 0; i < 8; i++)
		{
			_mm_store_si128((__m128i *) block->UV_pred_pixels->block[i], mm_v_pixels);
		}
		block->chroma_pred_mode = CHROMA_PRED_MODE_V;
	}
	else
	{
		ENC_ASSERT(0);	// shouldn't get here
	}
	return score;
}
