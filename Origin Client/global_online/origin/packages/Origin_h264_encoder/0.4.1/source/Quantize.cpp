#include "Quantize.h"
#include <emmintrin.h>
#include <immintrin.h>	// for SSE3 intrinsics
#include "Trace.h"

#include "Tables.h"

static QuantizeParamType sQuantParams;

static const int Quantize_Coef[6][4][4] = {
	{{13107, 8066,13107, 8066},{ 8066, 5243, 8066, 5243},{13107, 8066,13107, 8066},{ 8066, 5243, 8066, 5243}},
	{{11916, 7490,11916, 7490},{ 7490, 4660, 7490, 4660},{11916, 7490,11916, 7490},{ 7490, 4660, 7490, 4660}},
	{{10082, 6554,10082, 6554},{ 6554, 4194, 6554, 4194},{10082, 6554,10082, 6554},{ 6554, 4194, 6554, 4194}},
	{{ 9362, 5825, 9362, 5825},{ 5825, 3647, 5825, 3647},{ 9362, 5825, 9362, 5825},{ 5825, 3647, 5825, 3647}},
	{{ 8192, 5243, 8192, 5243},{ 5243, 3355, 5243, 3355},{ 8192, 5243, 8192, 5243},{ 5243, 3355, 5243, 3355}},
	{{ 7282, 4559, 7282, 4559},{ 4559, 2893, 4559, 2893},{ 7282, 4559, 7282, 4559},{ 4559, 2893, 4559, 2893}}
};

const int Dequantize_Coef[6][4][4] = {
	{{10, 13, 10, 13},{ 13, 16, 13, 16},{10, 13, 10, 13},{ 13, 16, 13, 16}},
	{{11, 14, 11, 14},{ 14, 18, 14, 18},{11, 14, 11, 14},{ 14, 18, 14, 18}},
	{{13, 16, 13, 16},{ 16, 20, 16, 20},{13, 16, 13, 16},{ 16, 20, 16, 20}},
	{{14, 18, 14, 18},{ 18, 23, 18, 23},{14, 18, 14, 18},{ 18, 23, 18, 23}},
	{{16, 20, 16, 20},{ 20, 25, 20, 25},{16, 20, 16, 20},{ 20, 25, 20, 25}},
	{{18, 23, 18, 23},{ 23, 29, 23, 29},{18, 23, 18, 23},{ 23, 29, 23, 29}}
};

static const short Quantize_Intra_Default[16] =
{
	 6, 13, 20, 28,
	13, 20, 28, 32,
	20, 28, 32, 37,
	28, 32, 37, 42
};

static const short Quantize_Inter_Default[16] =
{
	10, 14, 20, 24,
	14, 20, 24, 27,
	20, 24, 27, 30,
	24, 27, 30, 34
};

static const short Quantize_Offset_Defaults[8] =
{
	682, // Intra Luma
	682, // Intra Chroma
	342, // Intra Inter Luma
	342, // Inter 
	682, // Intra Luma 8
	682, // Intra Chroma 8
	342, // Intra Inter Luma 8
	342, // Inter 8
};

QuantizeParamType *GetQuantParams()
{
	return &sQuantParams;
}

void InitQuantizeParams()
{
	int k;

	for(int k_mod = 0; k_mod <= MAX_QP; k_mod++)
	{
		k = k_mod % 6;
		for(int j = 0; j < 4; j++)
		{
			for(int i = 0; i < 4; i++)
			{
#if 1
				// Intra slices
				sQuantParams.Quant4x4_Scale[0][1][k_mod][j][i]    = Quantize_Coef[k][j][i];
				sQuantParams.Quant4x4_Scale_2x[0][1][k_mod][j][i]    = static_cast<int16_t>(Quantize_Coef[k][j][i]);
				sQuantParams.Quant4x4_Scale_2x[0][1][k_mod][j][i+4]  = static_cast<int16_t>(Quantize_Coef[k][j][i]);
				sQuantParams.Quant4x4_InvScale[0][1][k_mod][j][i] = Dequantize_Coef[k][j][i] << 4;
				// the next two sets are for chroma - duplicate for now in case we decide to allow custom values later
				sQuantParams.Quant4x4_Scale[1][1][k_mod][j][i]    = sQuantParams.Quant4x4_Scale[0][1][k_mod][j][i];
				sQuantParams.Quant4x4_Scale_2x[1][1][k_mod][j][i]    = static_cast<int16_t>(Quantize_Coef[k][j][i]);
				sQuantParams.Quant4x4_Scale_2x[1][1][k_mod][j][i+4]  = static_cast<int16_t>(Quantize_Coef[k][j][i]);
				sQuantParams.Quant4x4_InvScale[1][1][k_mod][j][i] = sQuantParams.Quant4x4_InvScale[0][1][k_mod][j][i];

				sQuantParams.Quant4x4_Scale[2][1][k_mod][j][i]    = sQuantParams.Quant4x4_Scale[1][1][k_mod][j][i];
				sQuantParams.Quant4x4_Scale_2x[2][1][k_mod][j][i]    = static_cast<int16_t>(Quantize_Coef[k][j][i]);
				sQuantParams.Quant4x4_Scale_2x[2][1][k_mod][j][i+4]  = static_cast<int16_t>(Quantize_Coef[k][j][i]);
				sQuantParams.Quant4x4_InvScale[2][1][k_mod][j][i] = sQuantParams.Quant4x4_InvScale[1][1][k_mod][j][i];

				// Inter slices
				sQuantParams.Quant4x4_Scale[0][0][k_mod][j][i]    = Quantize_Coef[k][j][i];
				sQuantParams.Quant4x4_Scale_2x[0][0][k_mod][j][i]    = static_cast<int16_t>(Quantize_Coef[k][j][i]);
				sQuantParams.Quant4x4_Scale_2x[0][0][k_mod][j][i+4]  = static_cast<int16_t>(Quantize_Coef[k][j][i]);
				sQuantParams.Quant4x4_InvScale[0][0][k_mod][j][i] = Dequantize_Coef[k][j][i] << 4;
				// the next two sets are for chroma - duplicate for now in case we decide to allow custom values later
				sQuantParams.Quant4x4_Scale[1][0][k_mod][j][i]    = sQuantParams.Quant4x4_Scale[0][0][k_mod][j][i];
				sQuantParams.Quant4x4_Scale_2x[1][0][k_mod][j][i]    = static_cast<int16_t>(Quantize_Coef[k][j][i]);
				sQuantParams.Quant4x4_Scale_2x[1][0][k_mod][j][i+4]  = static_cast<int16_t>(Quantize_Coef[k][j][i]);
				sQuantParams.Quant4x4_InvScale[1][0][k_mod][j][i] = sQuantParams.Quant4x4_InvScale[0][0][k_mod][j][i];

				sQuantParams.Quant4x4_Scale[2][0][k_mod][j][i]    = sQuantParams.Quant4x4_Scale[1][0][k_mod][j][i];
				sQuantParams.Quant4x4_Scale_2x[2][0][k_mod][j][i]    = static_cast<int16_t>(Quantize_Coef[k][j][i]);
				sQuantParams.Quant4x4_Scale_2x[2][0][k_mod][j][i+4]  = static_cast<int16_t>(Quantize_Coef[k][j][i]);
				sQuantParams.Quant4x4_InvScale[2][0][k_mod][j][i] = sQuantParams.Quant4x4_InvScale[1][0][k_mod][j][i];
#else
				int temp = (j << 2) + i;
				// Intra slices
				sQuantParams.Quant4x4_Scale[0][1][k_mod][j][i]    = (Quantize_Coef[k][j][i]<<4) / Quantize_Intra_Default[temp];
				sQuantParams.Quant4x4_InvScale[0][1][k_mod][j][i] = Dequantize_Coef[k][j][i] * Quantize_Intra_Default[temp];
				// the next two sets are for chroma - duplicate for now in case we decide to allow custom values later
				sQuantParams.Quant4x4_Scale[1][1][k_mod][j][i]    = sQuantParams.Quant4x4_Scale[0][1][k_mod][j][i];
				sQuantParams.Quant4x4_InvScale[1][1][k_mod][j][i] = sQuantParams.Quant4x4_InvScale[0][1][k_mod][j][i];

				sQuantParams.Quant4x4_Scale[2][1][k_mod][j][i]    = sQuantParams.Quant4x4_Scale[1][1][k_mod][j][i];
				sQuantParams.Quant4x4_InvScale[2][1][k_mod][j][i] = sQuantParams.Quant4x4_InvScale[1][1][k_mod][j][i];

				// Inter slices
				sQuantParams.Quant4x4_Scale[0][0][k_mod][j][i]    = (Quantize_Coef[k][j][i]<<4) / Quantize_Inter_Default[temp];
				sQuantParams.Quant4x4_InvScale[0][0][k_mod][j][i] = Dequantize_Coef[k][j][i] * Quantize_Inter_Default[temp];
				// the next two sets are for chroma - duplicate for now in case we decide to allow custom values later
				sQuantParams.Quant4x4_Scale[1][0][k_mod][j][i]    = sQuantParams.Quant4x4_Scale[0][0][k_mod][j][i];
				sQuantParams.Quant4x4_InvScale[1][0][k_mod][j][i] = sQuantParams.Quant4x4_InvScale[0][0][k_mod][j][i];

				sQuantParams.Quant4x4_Scale[2][0][k_mod][j][i]    = sQuantParams.Quant4x4_Scale[1][0][k_mod][j][i];
				sQuantParams.Quant4x4_InvScale[2][0][k_mod][j][i] = sQuantParams.Quant4x4_InvScale[1][0][k_mod][j][i];
#endif
			}
		}
	}

	// Init Offset parameters
	for (int i = 0; i < MAX_QP; i++)
	{
		int qp_bitshift = 15 + (i / 6) - 11;

		// 0 (INTRA4X4_LUMA_INTRA)
		for (int j = 0; j < 16; j++)
		{
			sQuantParams.Quant4x4_Offset[i][0][j] = Quantize_Offset_Defaults[0] << qp_bitshift;
			for (k = 1; k < 3; k++) // 1,2 (INTRA4X4_CHROMA_INTRA)
				sQuantParams.Quant4x4_Offset[i][k][j] = Quantize_Offset_Defaults[1] << qp_bitshift;
			for (k = 3; k < 9; k++) // 3,4,5,6,7,8 (INTRA4X4_LUMA/CHROMA_INTERP/INTERB)
				sQuantParams.Quant4x4_Offset[i][k][j] = Quantize_Offset_Defaults[2] << qp_bitshift;
			for (k = 9; k < 25; k++) // 9,10,11,12,13,14 (INTER4X4)
				sQuantParams.Quant4x4_Offset[i][k][j] = Quantize_Offset_Defaults[3] << qp_bitshift;
		}
		// 0 (INTRA8X8_LUMA_INTRA)
		for (int j = 0; j < 64; j++)
		{
			sQuantParams.Quant8x8_Offset[i][0][j] = Quantize_Offset_Defaults[4] << qp_bitshift;
			for (k = 1; k < 3; k++)  // 1,2 (INTRA8X8_LUMA_INTERP/INTERB)
				sQuantParams.Quant8x8_Offset[i][k][j] = Quantize_Offset_Defaults[6] << qp_bitshift;
			for (k = 3; k < 5; k++)  // 3,4 (INTER8X8_LUMA_INTERP/INTERB)
				sQuantParams.Quant8x8_Offset[i][k][j] = Quantize_Offset_Defaults[7] << qp_bitshift;

			// 5 (INTRA8X8_CHROMAU_INTRA)
			sQuantParams.Quant8x8_Offset[i][5][j] = Quantize_Offset_Defaults[5] << qp_bitshift;
			for (k = 6; k < 8; k++)  // 6,7 (INTRA8X8_CHROMAU_INTERP/INTERB)
				sQuantParams.Quant8x8_Offset[i][k][j] = Quantize_Offset_Defaults[6] << qp_bitshift;
			for (k = 8; k < 10; k++)  // 8,9 (INTER8X8_CHROMAU_INTERP/INTERB)
				sQuantParams.Quant8x8_Offset[i][k][j] = Quantize_Offset_Defaults[7] << qp_bitshift;

			// 10 (INTRA8X8_CHROMAV_INTRA)
			sQuantParams.Quant8x8_Offset[i][10][j] = Quantize_Offset_Defaults[5] << qp_bitshift;
			for (k = 11; k < 13; k++)  // 11,12 (INTRA8X8_CHROMAV_INTERP/INTERB)
				sQuantParams.Quant8x8_Offset[i][k][j] = Quantize_Offset_Defaults[6] << qp_bitshift;
			for (k = 13; k < 15; k++)  // 8,9 (INTER8X8_CHROMAV_INTERP/INTERB)
				sQuantParams.Quant8x8_Offset[i][k][j] = Quantize_Offset_Defaults[7] << qp_bitshift;
		}
	}
}


int Quantize4x4(int16_t dct4x4[16], uint16_t scale_params[16], uint16_t offset_params[16], int shift_bits)
{
	int non_zero = 0;
	int value;
	for( int i = 0; i < 16; i++ )
	{
		if( dct4x4[i] > 0 ) 
			value =  dct4x4[i] * scale_params[i];
		else 
			value = -dct4x4[i] * scale_params[i];

		dct4x4[i] = static_cast<int16_t>((value + offset_params[i]) >> shift_bits); 

		non_zero |= dct4x4[i];
	}
    return non_zero;
}

int Quantize4x4_AC(int16_t *dct4x4, int scale_params[16], int offset_params[16], int shift_bits, int array_width, int start_idx)
{
	int non_zero = 0;
	int value;
	int i;
	for( i = start_idx; i < 4; i++ )
	{
		if( dct4x4[i] > 0 ) 
		{
			value =  dct4x4[i] * scale_params[i];
			dct4x4[i] = static_cast<int16_t>((value + (offset_params[i])) >> shift_bits); 
		}
		else 
		{
			value = -dct4x4[i] * scale_params[i];
			dct4x4[i] = static_cast<int16_t>(-((value + (offset_params[i])) >> shift_bits)); 
		}

		non_zero |= dct4x4[i];
	}
	dct4x4 += array_width;
	for( int j = 1; j < 4; j++, dct4x4 += array_width )
	{
		for( int k = 0; k < 4; k++, i++ )
		{
			if( dct4x4[k] > 0 ) 
			{
				value =  dct4x4[k] * scale_params[i];
				dct4x4[k] = static_cast<int16_t>((value + (offset_params[i])) >> shift_bits); 
			}
			else 
			{
				value = -dct4x4[k] * scale_params[i];
				dct4x4[k] = static_cast<int16_t>(-((value + (offset_params[i])) >> shift_bits)); 
			}

			non_zero |= dct4x4[k];
		}
	}
	return non_zero;
}

int Quantize4x4_AC_32_SSE2(int *dct4x4, int scale_params[16], int offset_params[16], int shift_bits, int array_width, int start_idx)
{
	int non_zero = 0;
	int array_line = 0;

	__m128i zero_mm = _mm_setzero_si128();
	__m128i mask = _mm_set1_epi32(0x0000ffff);
	__m128i offset = _mm_set1_epi32(offset_params[0]);
	__m128i offset_plus1 = _mm_set1_epi32(offset_params[0] -  (1L<<shift_bits));	// to compensate for arithmetic shift for negative #s needing + 1
	__m128i mm0, mm1, mm2;

	for (int i = 0; i < 16; i+=4, array_line += array_width)
	{
		__m128i src    = _mm_load_si128((__m128i*)&dct4x4[array_line]);
		__m128i scale  = _mm_loadu_si128((__m128i*)&scale_params[i]);

		__m128i signmask = _mm_cmpgt_epi32(zero_mm, src);	// mask for negative values

		// for SSE2 we need to treat as multiple 16-bit signed multiplies to get upper and lower 16-bit results
		// first upper
		mm0 = _mm_mulhi_epi16(src,scale);
		mm0 = _mm_slli_epi32(mm0,16);	// shift high 16-bits up to top 16-bits
		// then mask top 16-bits and multiple for lower 16-bits
		mm1 = _mm_and_si128(src,mask);
		mm2 = _mm_mullo_epi16(mm1,scale);
		mm2 = _mm_or_si128(mm2, mm0);	// now have 32-bit results (sheesh)

		mm1 = _mm_add_epi32(mm2, offset);	// add the offset
		mm2 = _mm_sub_epi32(mm2, offset_plus1);	// subtract the offset for the negative value

		mm2 = _mm_and_si128(mm2, signmask);		// mask out negative
		mm1 = _mm_andnot_si128(signmask, mm1);	// mask out positive
		mm0 = _mm_or_si128(mm1,mm2);

		mm2 = _mm_srai_epi32(mm0, shift_bits);	// shift back down arithmetically
		_mm_store_si128((__m128i*)&dct4x4[array_line], mm2);	// store it

		mm1 = _mm_cmpeq_epi32(mm2,zero_mm);		// sets bit mask for 32-bit values equal to zero
		non_zero |= _mm_movemask_epi8(mm1) != 0xffff;	// sets bits for bytes with msb set so if all 16 bits are set then it is all zeros
	}
	return non_zero;
}

#define MID_TREAD_STEPSIZE 23

// process 16 4x4 blocks for speed
int Quantize16x16_Luma_AC_16_SSE2(int16_t *dct16x16, int16_t scale_params[32], int offset_params[16], int shift_bits, int non_zero[16], int preserve_dc, int md_stepsize)
{
	{
		int ret = 0;
		int movemask;
		int mid_tread_step_plus_one = (md_stepsize << (shift_bits - 4)) + 1;
		int16_t save_dc[4];
		int16_t *dct_block = NULL;
		__m128i step = _mm_set1_epi32(mid_tread_step_plus_one);

		__m128i zero_mm = _mm_setzero_si128();
		__m128i offset = _mm_set1_epi32(offset_params[0]);
		__m128i mm0, mm1, mm2, mm3;
		__m128i abs_src;
		__m128i src, scale;
		__m128i in_dead_zone_mask;
		__m128i sign_scale;	// for abs and restoration
		__m128i one_mm = _mm_set1_epi16(1);

		for (int j = 0; j < 4; j++)
		{
			// set up 16x4 processing
			non_zero[j*4] = non_zero[j*4 + 1] = non_zero[j*4 + 2] = non_zero[j*4 + 3] = 0;

			if (preserve_dc)	// for I blocks
			{
				save_dc[0] = dct16x16[0];
				save_dc[1] = dct16x16[4];
				save_dc[2] = dct16x16[8];
				save_dc[3] = dct16x16[12];
				dct16x16[0] = 0;
				dct16x16[4] = 0;
				dct16x16[8] = 0;
				dct16x16[12] = 0;

				dct_block = dct16x16;	// save start as we will need to restore later
			}

			for (int i = 0; i < 32; i+=8)
			{
				scale  = _mm_loadu_si128((__m128i*)&scale_params[i]);

				src    = _mm_load_si128((__m128i*)dct16x16);

				// need abs and way to restore sign
				sign_scale = _mm_cmplt_epi16(src, zero_mm);	// basically places -1 (0xffff) where negative values are
				mm0 = _mm_andnot_si128(sign_scale, one_mm);	// places 1 where positive values are
				sign_scale = _mm_or_si128(sign_scale, mm0);	// places 1 where positive and -1 where negative
				abs_src = _mm_mullo_epi16(src, sign_scale);	// absolute value

				// for SSE2 we need to treat as multiple 16-bit signed multiplies to get upper and lower 16-bit results
				// first upper
				mm0 = _mm_mulhi_epi16(abs_src,scale);
				mm1 = _mm_mullo_epi16(abs_src,scale);
				mm2 = _mm_unpackhi_epi16(mm1, mm0);	// make high 4 values 32-bit product
				mm3 = _mm_unpacklo_epi16(mm1, mm0);	// make low 4 values 32-bit product

				mm2 = _mm_add_epi32(mm2, offset);	// add the offset
				mm3 = _mm_add_epi32(mm3, offset);	// add the offset

				// figure out dead zone mask
				in_dead_zone_mask = _mm_cmplt_epi32(mm2, step);	    // mask for "less than step" values
				// zero out any values in the dead zone
				mm2 = _mm_andnot_si128(in_dead_zone_mask, mm2);
				mm2 = _mm_srai_epi32(mm2, shift_bits);	// shift back down arithmetically

				in_dead_zone_mask = _mm_cmplt_epi32(mm3, step);	    // mask for "less than step" values
				mm3 = _mm_andnot_si128(in_dead_zone_mask, mm3);
				mm3 = _mm_srai_epi32(mm3, shift_bits);	// shift back down arithmetically

				mm2 = _mm_packs_epi32(mm3,mm2);	// convert back to 8 16-bit values
				mm2 = _mm_mullo_epi16(mm2, sign_scale);	// restore sign

				_mm_store_si128((__m128i*)dct16x16, mm2);	// store it

				dct16x16 += 8;

				mm1 = _mm_cmpeq_epi16(mm2,zero_mm);		// sets bit mask for 16-bit values equal to zero
				movemask = _mm_movemask_epi8(mm1);
				non_zero[j*4]     |= (movemask & 0x00ff) != 0x00ff;	// sets bits for bytes with msb set so if all 16 bits are set then it is all zeros
				non_zero[j*4 + 1] |= (movemask & 0xff00) != 0xff00;	// sets bits for bytes with msb set so if all 16 bits are set then it is all zeros


				// next row of 8x4
				src    = _mm_load_si128((__m128i*)dct16x16);

				// need abs and way to restore sign
				sign_scale = _mm_cmplt_epi16(src, zero_mm);	// basically places -1 (0xffff) where negative values are
				mm0 = _mm_andnot_si128(sign_scale, one_mm);	// places 1 where positive values are
				sign_scale = _mm_or_si128(sign_scale, mm0);	// places 1 where positive and -1 where negative
				abs_src = _mm_mullo_epi16(src, sign_scale);	// absolute value

				// for SSE2 we need to treat as multiple 16-bit signed multiplies to get upper and lower 16-bit results
				// first upper
				mm0 = _mm_mulhi_epi16(abs_src,scale);
				mm1 = _mm_mullo_epi16(abs_src,scale);
				mm2 = _mm_unpackhi_epi16(mm1, mm0);	// make high 4 values 32-bit product
				mm3 = _mm_unpacklo_epi16(mm1, mm0);	// make low 4 values 32-bit product

				mm2 = _mm_add_epi32(mm2, offset);	// add the offset
				mm3 = _mm_add_epi32(mm3, offset);	// add the offset

				// figure out dead zone mask
				in_dead_zone_mask = _mm_cmplt_epi32(mm2, step);	    // mask for "less than step" values
				// zero out any values in the dead zone
				mm2 = _mm_andnot_si128(in_dead_zone_mask, mm2);
				mm2 = _mm_srai_epi32(mm2, shift_bits);	// shift back down arithmetically

				in_dead_zone_mask = _mm_cmplt_epi32(mm3, step);	    // mask for "less than step" values
				mm3 = _mm_andnot_si128(in_dead_zone_mask, mm3);
				mm3 = _mm_srai_epi32(mm3, shift_bits);	// shift back down arithmetically

				mm2 = _mm_packs_epi32(mm3,mm2);	// convert back to 8 16-bit values
				mm2 = _mm_mullo_epi16(mm2, sign_scale);	// restore sign

				_mm_store_si128((__m128i*)dct16x16, mm2);	// store it
				dct16x16 += 8;

				mm1 = _mm_cmpeq_epi16(mm2,zero_mm);		// sets bit mask for 16-bit values equal to zero
				movemask = _mm_movemask_epi8(mm1);
				non_zero[j*4 + 2] |= (movemask & 0x00ff) != 0x00ff;	// sets bits for bytes with msb set so if all 16 bits are set then it is all zeros
				non_zero[j*4 + 3] |= (movemask & 0xff00) != 0xff00;	// sets bits for bytes with msb set so if all 16 bits are set then it is all zeros
			}

			if (preserve_dc)	// for I blocks
			{
				dct_block[0] = save_dc[0];
				dct_block[4] = save_dc[1];
				dct_block[8] = save_dc[2];
				dct_block[12] = save_dc[3];
			}

			ret |= ((non_zero[j*4] + non_zero[j*4 + 1] + non_zero[j*4 + 2] + non_zero[j*4 + 3]) != 0) ? (1 << j) : 0;
		}

		return ret;
	}
}

int Quantize16x16_Luma_AC_16_SSSE3(int16_t *dct16x16, int16_t scale_params[32], int offset_params[16], int shift_bits, int non_zero[16], int preserve_dc, int md_stepsize)
{
	int ret = 0;
	int movemask;
	int mid_tread_step_plus_one = (md_stepsize << (shift_bits - 4)) + 1;
	int16_t save_dc[4];
	int16_t *dct_block = NULL;
	__m128i step = _mm_set1_epi32(mid_tread_step_plus_one);

	__m128i zero_mm = _mm_setzero_si128();
	__m128i offset = _mm_set1_epi32(offset_params[0]);
	__m128i mm0, mm1, mm2, mm3;
	__m128i abs_src;
	__m128i src, scale;
	__m128i in_dead_zone_mask;

	for (int j = 0; j < 4; j++)
	{
		// set up 16x4 processing
		non_zero[j*4] = non_zero[j*4 + 1] = non_zero[j*4 + 2] = non_zero[j*4 + 3] = 0;

		if (preserve_dc)	// for I blocks
		{
			save_dc[0] = dct16x16[0];
			save_dc[1] = dct16x16[4];
			save_dc[2] = dct16x16[8];
			save_dc[3] = dct16x16[12];
			dct16x16[0] = 0;
			dct16x16[4] = 0;
			dct16x16[8] = 0;
			dct16x16[12] = 0;

			dct_block = dct16x16;	// save start as we will need to restore later
		}

		for (int i = 0; i < 32; i+=8)
		{
			scale  = _mm_loadu_si128((__m128i*)&scale_params[i]);

			src    = _mm_load_si128((__m128i*)dct16x16);

			abs_src = _mm_abs_epi16(src);	// make absolute values (SSSE3 instr)
			// for SSE2 we need to treat as multiple 16-bit signed multiplies to get upper and lower 16-bit results
			// first upper
			mm0 = _mm_mulhi_epi16(abs_src,scale);
			mm1 = _mm_mullo_epi16(abs_src,scale);
			mm2 = _mm_unpackhi_epi16(mm1, mm0);	// make high 4 values 32-bit product
			mm3 = _mm_unpacklo_epi16(mm1, mm0);	// make low 4 values 32-bit product

			mm2 = _mm_add_epi32(mm2, offset);	// add the offset
			mm3 = _mm_add_epi32(mm3, offset);	// add the offset

			// figure out dead zone mask
			in_dead_zone_mask = _mm_cmplt_epi32(mm2, step);	    // mask for "less than step" values
			// zero out any values in the dead zone
			mm2 = _mm_andnot_si128(in_dead_zone_mask, mm2);
			mm2 = _mm_srai_epi32(mm2, shift_bits);	// shift back down arithmetically

			in_dead_zone_mask = _mm_cmplt_epi32(mm3, step);	    // mask for "less than step" values
			mm3 = _mm_andnot_si128(in_dead_zone_mask, mm3);
			mm3 = _mm_srai_epi32(mm3, shift_bits);	// shift back down arithmetically

			mm2 = _mm_hadd_epi16(mm3,mm2);	// convert back to 8 16-bit values (SSSE3 instr)
			mm2 = _mm_sign_epi16(mm2, src);	// restore sign (SSSE3 instr)

			_mm_store_si128((__m128i*)dct16x16, mm2);	// store it

			dct16x16 += 8;

			mm1 = _mm_cmpeq_epi16(mm2,zero_mm);		// sets bit mask for 16-bit values equal to zero
			movemask = _mm_movemask_epi8(mm1);
			non_zero[j*4]     |= (movemask & 0x00ff) != 0x00ff;	// sets bits for bytes with msb set so if all 16 bits are set then it is all zeros
			non_zero[j*4 + 1] |= (movemask & 0xff00) != 0xff00;	// sets bits for bytes with msb set so if all 16 bits are set then it is all zeros


			// next row of 8x4
			src    = _mm_load_si128((__m128i*)dct16x16);

			abs_src = _mm_abs_epi16(src);	// make absolute values (SSSE3 instr)
			// for SSE2 we need to treat as multiple 16-bit signed multiplies to get upper and lower 16-bit results
			// first upper
			mm0 = _mm_mulhi_epi16(abs_src,scale);
			mm1 = _mm_mullo_epi16(abs_src,scale);
			mm2 = _mm_unpackhi_epi16(mm1, mm0);	// make high 4 values 32-bit product
			mm3 = _mm_unpacklo_epi16(mm1, mm0);	// make low 4 values 32-bit product

			mm2 = _mm_add_epi32(mm2, offset);	// add the offset
			mm3 = _mm_add_epi32(mm3, offset);	// add the offset

			// figure out dead zone mask
			in_dead_zone_mask = _mm_cmplt_epi32(mm2, step);	    // mask for "less than step" values
			// zero out any values in the dead zone
			mm2 = _mm_andnot_si128(in_dead_zone_mask, mm2);
			mm2 = _mm_srai_epi32(mm2, shift_bits);	// shift back down arithmetically

			in_dead_zone_mask = _mm_cmplt_epi32(mm3, step);	    // mask for "less than step" values
			mm3 = _mm_andnot_si128(in_dead_zone_mask, mm3);
			mm3 = _mm_srai_epi32(mm3, shift_bits);	// shift back down arithmetically

			mm2 = _mm_hadd_epi16(mm3,mm2);	// convert back to 8 16-bit values (SSSE3 instr)
			mm2 = _mm_sign_epi16(mm2, src);	// restore sign (SSSE3 instr)

			_mm_store_si128((__m128i*)dct16x16, mm2);	// store it
			dct16x16 += 8;

			mm1 = _mm_cmpeq_epi16(mm2,zero_mm);		// sets bit mask for 16-bit values equal to zero
			movemask = _mm_movemask_epi8(mm1);
			non_zero[j*4 + 2] |= (movemask & 0x00ff) != 0x00ff;	// sets bits for bytes with msb set so if all 16 bits are set then it is all zeros
			non_zero[j*4 + 3] |= (movemask & 0xff00) != 0xff00;	// sets bits for bytes with msb set so if all 16 bits are set then it is all zeros
		}

		if (preserve_dc)	// for I blocks
		{
			dct_block[0] = save_dc[0];
			dct_block[4] = save_dc[1];
			dct_block[8] = save_dc[2];
			dct_block[12] = save_dc[3];
		}

		ret |= ((non_zero[j*4] + non_zero[j*4 + 1] + non_zero[j*4 + 2] + non_zero[j*4 + 3]) != 0) ? (1 << j) : 0;
	}

	return ret;
}

// process 8 4x4 blocks for speed
// dct8x16 is arranged as follows:
// u0 u0 u0 u0  u1 u1 u1 u1
// u0 u0 u0 u0  u1 u1 u1 u1
// u0 u0 u0 u0  u1 u1 u1 u1
// u0 u0 u0 u0  u1 u1 u1 u1
// u2 u2 u2 u2  u3 u3 u3 u3
// u2 u2 u2 u2  u3 u3 u3 u3
// u2 u2 u2 u2  u3 u3 u3 u3
// u2 u2 u2 u2  u3 u3 u3 u3

// v0 v0 v0 v0  v1 v1 v1 v1
// v0 v0 v0 v0  v1 v1 v1 v1
// v0 v0 v0 v0  v1 v1 v1 v1
// v0 v0 v0 v0  v1 v1 v1 v1
// v2 v2 v2 v2  v3 v3 v3 v3
// v2 v2 v2 v2  v3 v3 v3 v3
// v2 v2 v2 v2  v3 v3 v3 v3
// v2 v2 v2 v2  v3 v3 v3 v3

int Quantize8x16_Chroma_AC_16_SSE2(int16_t *dct8x16, int16_t scale_params[32], int offset_param, int shift_bits, int non_zero[8])
{
	int ret = 0;
	int movemask;
	int16_t save_dc[2];
	int16_t *dct_block;
	__m128i zero_mm = _mm_setzero_si128();
	__m128i offset = _mm_set1_epi32(offset_param);
	__m128i mm0, mm1, mm2, mm3;
	__m128i abs_src;
	__m128i src, scale;
	__m128i sign_scale;	// for abs and restoration
	__m128i one_mm = _mm_set1_epi16(1);

	for (int j = 0; j < 4; j++)
	{
		// set up 16x4 processing
		non_zero[j*2] = non_zero[j*2 + 1] = 0;

		save_dc[0] = dct8x16[0];
		save_dc[1] = dct8x16[4];
		dct8x16[0] = 0;
		dct8x16[4] = 0;

		dct_block = dct8x16;	// save start as we will need to restore later

		for (int i = 0; i < 32; i+=8)
		{
			scale  = _mm_loadu_si128((__m128i*)&scale_params[i]);
			src    = _mm_load_si128((__m128i*)dct8x16);

			// need abs and way to restore sign
			sign_scale = _mm_cmplt_epi16(src, zero_mm);	// basically places -1 (0xffff) where negative values are
			mm0 = _mm_andnot_si128(sign_scale, one_mm);	// places 1 where positive values are
			sign_scale = _mm_or_si128(sign_scale, mm0);	// places 1 where positive and -1 where negative
			abs_src = _mm_mullo_epi16(src, sign_scale);	// absolute value

			// for SSE2 we need to treat as multiple 16-bit signed multiplies to get upper and lower 16-bit results
			// first upper
			mm0 = _mm_mulhi_epi16(abs_src,scale);
			mm1 = _mm_mullo_epi16(abs_src,scale);
			mm2 = _mm_unpackhi_epi16(mm1, mm0);	// make high 4 values 32-bit product
			mm3 = _mm_unpacklo_epi16(mm1, mm0);	// make low 4 values 32-bit product

			mm2 = _mm_add_epi32(mm2, offset);	// add the offset
			mm3 = _mm_add_epi32(mm3, offset);	// add the offset

			mm2 = _mm_srai_epi32(mm2, shift_bits);	// shift back down arithmetically
			mm3 = _mm_srai_epi32(mm3, shift_bits);	// shift back down arithmetically

			mm2 = _mm_packs_epi32(mm3,mm2);	// convert back to 8 16-bit values
			mm2 = _mm_mullo_epi16(mm2, sign_scale);	// restore sign

			_mm_store_si128((__m128i*)dct8x16, mm2);	// store it

			dct8x16 += 8;

			mm1 = _mm_cmpeq_epi16(mm2,zero_mm);		// sets bit mask for 16-bit values equal to zero
			movemask = _mm_movemask_epi8(mm1);
			non_zero[j*2]     |= (movemask & 0x00ff) != 0x00ff;	// sets bits for bytes with msb set so if all 16 bits are set then it is all zeros
			non_zero[j*2 + 1] |= (movemask & 0xff00) != 0xff00;	// sets bits for bytes with msb set so if all 16 bits are set then it is all zeros
		}

		dct_block[0] = save_dc[0];
		dct_block[4] = save_dc[1];

		ret |= ((non_zero[j*2] + non_zero[j*2 + 1]) != 0) ? (1 << j) : 0;
	}

	return ret;
}

int Quantize8x16_Chroma_AC_16_SSSE3(int16_t *dct8x16, int16_t scale_params[32], int offset_param, int shift_bits, int non_zero[8])
{
	int ret = 0;
	int movemask;
	int16_t save_dc[2];
	int16_t *dct_block;
	__m128i zero_mm = _mm_setzero_si128();
	__m128i offset = _mm_set1_epi32(offset_param);
	__m128i mm0, mm1, mm2, mm3;
	__m128i abs_src;
	__m128i src, scale;

	for (int j = 0; j < 4; j++)
	{
		// set up 16x4 processing
		non_zero[j*2] = non_zero[j*2 + 1] = 0;

		save_dc[0] = dct8x16[0];
		save_dc[1] = dct8x16[4];
		dct8x16[0] = 0;
		dct8x16[4] = 0;

		dct_block = dct8x16;	// save start as we will need to restore later

		for (int i = 0; i < 32; i+=8)
		{
			scale  = _mm_loadu_si128((__m128i*)&scale_params[i]);
			src    = _mm_load_si128((__m128i*)dct8x16);

			abs_src = _mm_abs_epi16(src);	// make absolute values (SSSE3 instr)
			// for SSE2 we need to treat as multiple 16-bit signed multiplies to get upper and lower 16-bit results
			// first upper
			mm0 = _mm_mulhi_epi16(abs_src,scale);
			mm1 = _mm_mullo_epi16(abs_src,scale);
			mm2 = _mm_unpackhi_epi16(mm1, mm0);	// make high 4 values 32-bit product
			mm3 = _mm_unpacklo_epi16(mm1, mm0);	// make low 4 values 32-bit product

			mm2 = _mm_add_epi32(mm2, offset);	// add the offset
			mm3 = _mm_add_epi32(mm3, offset);	// add the offset

			mm2 = _mm_srai_epi32(mm2, shift_bits);	// shift back down arithmetically
			mm3 = _mm_srai_epi32(mm3, shift_bits);	// shift back down arithmetically

			mm2 = _mm_hadd_epi16(mm3,mm2);	// convert back to 8 16-bit values (SSSE3 instr)
			mm2 = _mm_sign_epi16(mm2, src);	// restore sign (SSSE3 instr)

			_mm_store_si128((__m128i*)dct8x16, mm2);	// store it

			dct8x16 += 8;

			mm1 = _mm_cmpeq_epi16(mm2,zero_mm);		// sets bit mask for 16-bit values equal to zero
			movemask = _mm_movemask_epi8(mm1);
			non_zero[j*2]     |= (movemask & 0x00ff) != 0x00ff;	// sets bits for bytes with msb set so if all 16 bits are set then it is all zeros
			non_zero[j*2 + 1] |= (movemask & 0xff00) != 0xff00;	// sets bits for bytes with msb set so if all 16 bits are set then it is all zeros
		}

		dct_block[0] = save_dc[0];
		dct_block[4] = save_dc[1];

		ret |= ((non_zero[j*2] + non_zero[j*2 + 1]) != 0) ? (1 << j) : 0;
	}

	return ret;
}



int Quantize4x4_Luma_AC_32_SSE2(int *dct4x4, int scale_params[16], int offset_params[16], int shift_bits, int array_width, int start_idx, int md_stepsize)
{
    int non_zero = 0;
    int array_line = 0;

    int mid_tread_step_plus_one = (md_stepsize << (shift_bits - 4)) + 1;
    __m128i step = _mm_set1_epi32(mid_tread_step_plus_one);
    __m128i step_negative = _mm_set1_epi32(-mid_tread_step_plus_one);

    __m128i zero_mm = _mm_setzero_si128();
    __m128i mask = _mm_set1_epi32(0x0000ffff);
    __m128i offset = _mm_set1_epi32(offset_params[0]);
    __m128i offset_plus1 = _mm_set1_epi32(offset_params[0] -  (1L<<shift_bits));	// to compensate for arithmetic shift for negative #s needing + 1
    __m128i mm0, mm1, mm2, mm3, mm4;

    for (int i = 0; i < 16; i+=4, array_line += array_width)
    {
        __m128i src    = _mm_load_si128((__m128i*)&dct4x4[array_line]);
        __m128i scale  = _mm_loadu_si128((__m128i*)&scale_params[i]);

        __m128i signmask = _mm_cmpgt_epi32(zero_mm, src);	// mask for negative values

        // for SSE2 we need to treat as multiple 16-bit signed multiplies to get upper and lower 16-bit results
        // first upper
        mm0 = _mm_mulhi_epi16(src,scale);
        mm0 = _mm_slli_epi32(mm0,16);	// shift high 16-bits up to top 16-bits
        // then mask top 16-bits and multiple for lower 16-bits
        mm1 = _mm_and_si128(src,mask);
        mm2 = _mm_mullo_epi16(mm1,scale);
        mm2 = _mm_or_si128(mm2, mm0);	// now have 32-bit results (sheesh)

        mm1 = _mm_add_epi32(mm2, offset);	// add the offset
        mm3 = _mm_sub_epi32(mm2, offset); // dead zone: subtract offset
        mm2 = _mm_sub_epi32(mm2, offset_plus1);	// subtract the offset for the negative value

        mm4 = _mm_and_si128(mm3, signmask);		// dead zone: mask out negative
        mm2 = _mm_and_si128(mm2, signmask);		// mask out negative
        mm1 = _mm_andnot_si128(signmask, mm1);	// mask out positive
        mm0 = _mm_or_si128(mm1,mm2);

        mm3 = _mm_or_si128(mm1,mm4); // dead zone: values

        // figure out dead zone mask
        __m128i in_dead_zone_mask = _mm_cmplt_epi32(mm3, step);	    // mask for "less than step" values
        __m128i more_mask = _mm_cmpgt_epi32(mm3, step_negative);// mask for "more than -step" values
        in_dead_zone_mask = _mm_and_si128(in_dead_zone_mask, more_mask);// mask for "in range" values

        // zero out any values in the dead zone
        mm0 = _mm_andnot_si128(in_dead_zone_mask, mm0);

        mm2 = _mm_srai_epi32(mm0, shift_bits);	// shift back down arithmetically
        _mm_store_si128((__m128i*)&dct4x4[array_line], mm2);	// store it

        mm1 = _mm_cmpeq_epi32(mm2,zero_mm);		// sets bit mask for 32-bit values equal to zero
        non_zero |= _mm_movemask_epi8(mm1) != 0xffff;	// sets bits for bytes with msb set so if all 16 bits are set then it is all zeros
    }
    return non_zero;
}

int Quantize4x4_Luma_AC_32(int16_t *dct4x4, int scale_params[16], int offset_params[16], int shift_bits, int array_width, int start_idx, int md_stepsize)
{
	int non_zero = 0;
	int value;
	int i;
	int offset = offset_params[0];
	int mid_tread_step = md_stepsize << (shift_bits - 4);

	for( i = start_idx; i < 4; i++ )
	{
		if( dct4x4[i] > 0 ) 
		{
			value =  (dct4x4[i] * scale_params[i]) + offset;
		}
		else
		{
			value =  (dct4x4[i] * scale_params[i]) - offset;
		}
		if (ABS(value) <= mid_tread_step)
			dct4x4[i] = 0;
		else
			dct4x4[i] = static_cast<int16_t>(value / (1L << shift_bits)); 

		non_zero |= dct4x4[i];
	}
	dct4x4 += array_width;
	for( int j = 1; j < 4; j++, dct4x4 += array_width )
	{
		for( int k = 0; k < 4; k++, i++ )
		{
			if( dct4x4[k] > 0 ) 
			{
				value =  (dct4x4[k] * scale_params[i]) + offset;
			}
			else
			{
				value =  (dct4x4[k] * scale_params[i]) - offset;
			}
			if (ABS(value) <= mid_tread_step)
				dct4x4[k] = 0;
			else
				dct4x4[k] = static_cast<int16_t>(value / (1L << shift_bits)); 

			non_zero |= dct4x4[k];
		}
	}

	return non_zero;
}


int Quantize4x4_AC_32(int *dct4x4, int scale_params[16], int offset_params[16], int shift_bits, int array_width, int start_idx)
{
	int non_zero = 0;
	int value;
	int i;
	int offset = offset_params[0];
#if 1
	for( i = start_idx; i < 4; i++ )
	{
		if( dct4x4[i] > 0 ) 
		{
			value =  (dct4x4[i] * scale_params[i]) + offset;
		}
		else
		{
			value =  (dct4x4[i] * scale_params[i]) - offset;
		}
		dct4x4[i] = value / (1L << shift_bits); 

		non_zero |= dct4x4[i];
	}
	dct4x4 += array_width;
	for( int j = 1; j < 4; j++, dct4x4 += array_width )
	{
		for( int k = 0; k < 4; k++, i++ )
		{
			if( dct4x4[k] > 0 ) 
			{
				value =  (dct4x4[k] * scale_params[i]) + offset;
			}
			else
			{
				value =  (dct4x4[k] * scale_params[i]) - offset;
			}
			dct4x4[k] = value / (1L << shift_bits); 

			non_zero |= dct4x4[k];
		}
	}

#else
	for( i = start_idx; i < 4; i++ )
	{
		if( dct4x4[i] > 0 ) 
		{
			value =  dct4x4[i] * scale_params[i];
			dct4x4[i] = (value + (offset_params[i])) >> shift_bits; 
		}
		else 
		{
			value = -dct4x4[i] * scale_params[i];
			dct4x4[i] = -((value + (offset_params[i])) >> shift_bits); 
		}

		non_zero |= dct4x4[i];
	}
	dct4x4 += array_width;
	for( int j = 1; j < 4; j++, dct4x4 += array_width )
	{
		for( int k = 0; k < 4; k++, i++ )
		{
			if( dct4x4[k] > 0 ) 
			{
				value =  dct4x4[k] * scale_params[i];
				dct4x4[k] = (value + (offset_params[i])) >> shift_bits; 
			}
			else 
			{
				value = -dct4x4[k] * scale_params[i];
				dct4x4[k] = -((value + (offset_params[i])) >> shift_bits); 
			}

			non_zero |= dct4x4[k];
		}
	}
#endif
	return non_zero;
}


int Quantize4x4_DC(int16_t dct4x4[16], int scale_param, int offset_param, int shift_bits)
{
	int non_zero = 0;
	int offset = offset_param << 1;
	int value;
	for( int i = 0; i < 16; i++ )
	{
		if( dct4x4[i] > 0 ) 
		{
			value =  dct4x4[i] * scale_param;
			dct4x4[i] = static_cast<int16_t>((value + offset) >> shift_bits);
		}
		else 
		{
			value = -dct4x4[i] * scale_param;
			dct4x4[i] = static_cast<int16_t>(-((value + offset) >> shift_bits));
		}

		non_zero |= dct4x4[i];
	}
	return non_zero;
}

int Quantize4x4_DC_32_SSE2(int dct4x4[16], int scale_param, int offset_param, int shift_bits)
{
	int non_zero = 0;

	offset_param <<= 1;
	__m128i zero_mm = _mm_setzero_si128();
	__m128i mask = _mm_set1_epi32(0x0000ffff);
	__m128i scale  = _mm_set1_epi32(scale_param);
	__m128i offset = _mm_set1_epi32(offset_param);
	__m128i offset_plus1 = _mm_set1_epi32(offset_param -  (1L<<shift_bits));	// to compensate for arithmetic shift for negative #s needing + 1
	__m128i mm0, mm1, mm2;

	for (int i = 0; i < 16; i+=4)
	{
		__m128i src    = _mm_load_si128((__m128i*)&dct4x4[i]);

		__m128i signmask = _mm_cmpgt_epi32(zero_mm, src);	// mask for negative values

		// for SSE2 we need to treat as multiple 16-bit signed multiplies to get upper and lower 16-bit results
		// first upper
		mm0 = _mm_mulhi_epi16(src,scale);
		mm0 = _mm_slli_epi32(mm0,16);	// shift high 16-bits up to top 16-bits
		// then mask top 16-bits and multiple for lower 16-bits
		mm1 = _mm_and_si128(src,mask);
		mm2 = _mm_mullo_epi16(mm1,scale);
		mm2 = _mm_or_si128(mm2, mm0);	// now have 32-bit results (sheesh)

		mm1 = _mm_add_epi32(mm2, offset);	// add the offset
		mm2 = _mm_sub_epi32(mm2, offset_plus1);	// subtract the offset for the negative value

		mm2 = _mm_and_si128(mm2, signmask);		// mask out negative
		mm1 = _mm_andnot_si128(signmask, mm1);	// mask out positive
		mm0 = _mm_or_si128(mm1,mm2);

		mm2 = _mm_srai_epi32(mm0, shift_bits);	// shift back down arithmetically
		_mm_store_si128((__m128i*)&dct4x4[i], mm2);	// store it

		mm1 = _mm_cmpeq_epi32(mm2,zero_mm);		// sets bit mask for 32-bit values equal to zero
		non_zero |= _mm_movemask_epi8(mm1) != 0xffff;	// sets bits for bytes with msb set so if all 16 bits are set then it is all zeros
	}
	return non_zero;
}

int Quantize4x4_DC_32(int dct4x4[16], int scale_param, int offset_param, int shift_bits)
{
	int non_zero = 0;
	int offset = offset_param << 1;
	int value;
	for( int i = 0; i < 16; i++ )
	{
		if( dct4x4[i] > 0 ) 
		{
			value =  dct4x4[i] * scale_param;
			dct4x4[i] = (value + offset) >> shift_bits;
		}
		else 
		{
			value = -dct4x4[i] * scale_param;
			dct4x4[i] = -((value + offset) >> shift_bits);
		}

		non_zero |= dct4x4[i];
	}
	return non_zero;
}

int Quantize4x4_DC_UV(int dct2x2[2][4], int scale_param, int offset_param, int shift_bits)
{
	int non_zero = 0;
	int offset = offset_param << 1;
	int value;

	for (int pl = 0; pl < 2; pl++)
	{
		for( int i = 0; i < 4; i++ )
		{
			if( dct2x2[pl][i] > 0 ) 
			{
				value =  dct2x2[pl][i] * scale_param;
				dct2x2[pl][i] = (value + offset) >> shift_bits;
			}
			else 
			{
				value = -dct2x2[pl][i] * scale_param;
				dct2x2[pl][i] = -((value + offset) >> shift_bits);
			}

			non_zero |= dct2x2[pl][i];
		}
	}
	return non_zero;
}

int Quantize4x4_DC_UV_SSE2(int dct2x2[2][4], int scale_param, int offset_param, int shift_bits)
{
	int non_zero = 0;

	offset_param <<= 1;
	__m128i zero_mm = _mm_setzero_si128();
	__m128i mask = _mm_set1_epi32(0x0000ffff);
	__m128i scale  = _mm_set1_epi32(scale_param);
	__m128i offset = _mm_set1_epi32(offset_param);
	__m128i offset_plus1 = _mm_set1_epi32(offset_param -  (1L<<shift_bits));	// to compensate for arithmetic shift for negative #s needing + 1
	__m128i mm0, mm1, mm2;

	for (int i = 0; i < 2; i++)
	{
		__m128i src    = _mm_load_si128((__m128i*)&dct2x2[i]);

		__m128i signmask = _mm_cmpgt_epi32(zero_mm, src);	// mask for negative values

		// for SSE2 we need to treat as multiple 16-bit signed multiplies to get upper and lower 16-bit results
		// first upper
		mm0 = _mm_mulhi_epi16(src,scale);
		mm0 = _mm_slli_epi32(mm0,16);	// shift high 16-bits up to top 16-bits
		// then mask top 16-bits and multiple for lower 16-bits
		mm1 = _mm_and_si128(src,mask);
		mm2 = _mm_mullo_epi16(mm1,scale);
		mm2 = _mm_or_si128(mm2, mm0);	// now have 32-bit results (sheesh)

		mm1 = _mm_add_epi32(mm2, offset);	// add the offset
		mm2 = _mm_sub_epi32(mm2, offset_plus1);	// subtract the offset for the negative value

		mm2 = _mm_and_si128(mm2, signmask);		// mask out negative
		mm1 = _mm_andnot_si128(signmask, mm1);	// mask out positive
		mm0 = _mm_or_si128(mm1,mm2);

		mm2 = _mm_srai_epi32(mm0, shift_bits);	// shift back down arithmetically
		_mm_store_si128((__m128i*)&dct2x2[i], mm2);	// store it

		mm1 = _mm_cmpeq_epi32(mm2,zero_mm);		// sets bit mask for 32-bit values equal to zero
		non_zero |= _mm_movemask_epi8(mm1) != 0xffff;	// sets bits for bytes with msb set so if all 16 bits are set then it is all zeros
	}
	return non_zero;
}


// run length zig-zag scan to tabulate values for encoding
static const int sZigZagMap_4x4[16] = { 0, 1, 4, 8, 5, 2, 3, 6, 9, 12, 13, 10, 7, 11, 14, 15 };

void RL_ZigZagScan_4x4(int16_t dct4x4[16], int16_t level_run[2][32])
{
	int16_t *level_buf = level_run[0];
	int16_t *run_buf   = level_run[1];
	int16_t run = 0;

	for (int i = 0; i < 16; i++)
	{
		if (dct4x4[sZigZagMap_4x4[i]])
		{
			*level_buf++ = (int16_t) dct4x4[sZigZagMap_4x4[i]];
			*run_buf++ = run;
			run = 0;
		}
		else
			run++;
	}
	*level_buf = 0;
}

int RL_ZigZagScan_4x4_32(int dct4x4[16], int16_t level_run[2][32])
{
	int16_t *level_buf = level_run[0];
	int16_t *run_buf   = level_run[1];
	int16_t run = 0;
	int coeff_cost = 0;

	for (int i = 0; i < 16; i++)
	{
		if (dct4x4[sZigZagMap_4x4[i]])
		{
			*level_buf++ = (int16_t) dct4x4[sZigZagMap_4x4[i]];
			*run_buf++ = run;
			coeff_cost += run + 1 + gCoeffBitCostTable[dct4x4[sZigZagMap_4x4[i]]+32768];	// one bit per run, +1 bit per non-zero level, + coeff bit cost
			run = 0;
		}
		else
			run++;
	}
	*level_buf = 0;

	return coeff_cost;
}

bool RL_ZigZagScan_2x2(int dct2x2[4], int16_t level_run[2][8], int *coeff_cost)
{
	int16_t *level_buf = level_run[0];
	int16_t *run_buf   = level_run[1];
	int16_t run = 0;
	bool non_zero = false;
	*coeff_cost = 0;

	for (int i = 0; i < 4; i++)
	{
		if (dct2x2[i])
		{
			*level_buf++ = (int16_t) dct2x2[i];
			*run_buf++ = run;
			run = 0;
			non_zero = true;
			*coeff_cost += run + 1 + gCoeffBitCostTable[dct2x2[i]+32768];	// one bit per run, +1 bit per non-zero level, + coeff bit cost
		}
		else
			run++;
	}
	*level_buf = 0;

	return non_zero;
}


// run length zig-zag scan through a 4x4 sub-block of a 16x16 macroblock to tabulate values for encoding
static const int sZigZagMap_Sub_4x4[16] = { 0, 1, 16, 32, 17, 2, 3, 18, 33, 48, 49, 34, 19, 35, 50, 51 };

void RL_ZigZagScan_Sub_4x4_AC(int16_t dct4x4[16], int16_t level_run[2][32], int *qinvscaling, int qp_shift, int start_idx)
{
	int16_t *level_buf = level_run[0];
	int16_t *run_buf   = level_run[1];
	int16_t run = 0;
	int zz_index;

	for (int i = start_idx; i < 16; i++)
	{
		zz_index = sZigZagMap_Sub_4x4[i];
		if (dct4x4[zz_index])
		{
			*level_buf++ = (int16_t) dct4x4[zz_index];
			dct4x4[zz_index] = static_cast<int16_t>((((dct4x4[zz_index] * qinvscaling[sZigZagMap_4x4[i]]) << qp_shift) + (1 << 3)) >> 4);
			*run_buf++ = run;
			run = 0;
		}
		else
			run++;
	}
	*level_buf = 0;
}


int RL_ZigZagScan_Sub_4x4_AC_16(int16_t dct4x4[16], int16_t level_run[2][32], int *qinvscaling, int qp_shift, int start_idx)
{
	int16_t *level_buf = level_run[0];
	int16_t *run_buf   = level_run[1];
	int16_t run = 0;
	int zz_index;
	int coeff_cost = 0;

	for (int i = start_idx; i < 16; i++)
	{
		zz_index = sZigZagMap_Sub_4x4[i];
		if (dct4x4[zz_index])
		{
			*level_buf++ = (int16_t) dct4x4[zz_index];
			dct4x4[zz_index] = static_cast<int16_t>((((dct4x4[zz_index] * qinvscaling[sZigZagMap_4x4[i]]) << qp_shift) + (1 << 3)) >> 4);
			ENC_ASSERT(!(ABS(dct4x4[zz_index]) & 0xffff0000));
			*run_buf++ = run;
			coeff_cost += run + 1 + gCoeffBitCostTable[dct4x4[zz_index]+32768];	// one bit per run, +1 bit per non-zero level, + coeff bit cost
			run = 0;
		}
		else
		{
			run++;
		}
	}
	*level_buf = 0;

	return coeff_cost;
}

int RL_ZigZagScan_Sub_4x4_AC_32(int dct4x4[16], int16_t level_run[2][32], int *qinvscaling, int qp_shift, int start_idx)
{
	int16_t *level_buf = level_run[0];
	int16_t *run_buf   = level_run[1];
	int16_t run = 0;
	int zz_index;
	int coeff_cost = 0;

	for (int i = start_idx; i < 16; i++)
	{
		zz_index = sZigZagMap_Sub_4x4[i];
		if (dct4x4[zz_index])
		{
			*level_buf++ = (int16_t) dct4x4[zz_index];
			dct4x4[zz_index] = (((dct4x4[zz_index] * qinvscaling[sZigZagMap_4x4[i]]) << qp_shift) + (1 << 3)) >> 4;
			*run_buf++ = run;
			coeff_cost += run + 1 + gCoeffBitCostTable[dct4x4[zz_index]+32768];	// one bit per run, +1 bit per non-zero level, + coeff bit cost
			run = 0;
		}
		else
		{
			run++;
		}
	}
	*level_buf = 0;

	return coeff_cost;
}


static const int sZigZagMap_Sub_4x4_UV[16] = { 0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 25, 18, 11, 19, 26, 27 };	// for 8x8

int RL_ZigZagScan_Sub_4x4_AC_UV_16(int16_t dct4x4[16], int16_t level_run[2][32], int *qinvscaling, int qp_shift)
{
	int16_t *level_buf = level_run[0];
	int16_t *run_buf   = level_run[1];
	int16_t run = 0;
	int zz_index;
	int coeff_cost = 0;

	for (int i = 1; i < 16; i++)
	{
		zz_index = sZigZagMap_Sub_4x4_UV[i];
		if (dct4x4[zz_index])
		{
			*level_buf++ = dct4x4[zz_index];
			dct4x4[zz_index] = static_cast<int16_t>((((dct4x4[zz_index] * qinvscaling[sZigZagMap_4x4[i]]) << qp_shift) + (1 << 3)) >> 4);
			*run_buf++ = run;
			coeff_cost += run + 1 + gCoeffBitCostTable[dct4x4[zz_index]+32768];	// one bit per run, +1 bit per non-zero level, + coeff bit cost
			run = 0;
		}
		else
			run++;
	}
	*level_buf = 0;

	return coeff_cost;
}


int RL_ZigZagScan_Sub_4x4_AC_UV(int dct4x4[16], int16_t level_run[2][32], int *qinvscaling, int qp_shift)
{
	int16_t *level_buf = level_run[0];
	int16_t *run_buf   = level_run[1];
	int16_t run = 0;
	int zz_index;
	int coeff_cost = 0;

	for (int i = 1; i < 16; i++)
	{
		zz_index = sZigZagMap_Sub_4x4_UV[i];
		if (dct4x4[zz_index])
		{
			*level_buf++ = (int16_t) dct4x4[zz_index];
			dct4x4[zz_index] = (((dct4x4[zz_index] * qinvscaling[sZigZagMap_4x4[i]]) << qp_shift) + (1 << 3)) >> 4;
			*run_buf++ = run;
			coeff_cost += run + 1 + gCoeffBitCostTable[dct4x4[zz_index]+32768];	// one bit per run, +1 bit per non-zero level, + coeff bit cost
			run = 0;
		}
		else
			run++;
	}
	*level_buf = 0;

	return coeff_cost;
}
