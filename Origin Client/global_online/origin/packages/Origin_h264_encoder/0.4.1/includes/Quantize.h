#ifndef __QUANTIZE_H
#define __QUANTIZE_H

#include <stdint.h>

#define MAX_Q_BITDEPTH 8

#define MAX_QP (6*MAX_Q_BITDEPTH + 3)

typedef struct  
{
	int Quant4x4_Scale[3][2][MAX_QP+1][4][4];
	int16_t Quant4x4_Scale_2x[3][2][MAX_QP+1][4][8];	// double each 4x row so we can read in two side-by-side 4x4 blocks and quant them together
	int Quant4x4_InvScale[3][2][MAX_QP+1][4][4];
	int Quant4x4_Offset[MAX_QP+1][25][16];

	uint16_t Quant8x8_Scale[3][2][MAX_QP+1][8][8];		// not set up yet
	uint16_t Quant8x8_InvScale[3][2][MAX_QP+1][8][8];	// not set up yet
	uint16_t Quant8x8_Offset[MAX_QP+1][15][64];

} QuantizeParamType;


QuantizeParamType *GetQuantParams();
void InitQuantizeParams();

int Quantize4x4(int16_t dct4x4[16], uint16_t scale_params[16], uint16_t offset_params[16], int shift_bits);
int Quantize4x4_AC(int16_t dct4x4[16], int scale_params[16], int offset_params[16], int shift_bits, int array_width = 16, int start_idx = 1);
int Quantize4x4_Luma_AC_32(int16_t *dct4x4, int scale_params[16], int offset_params[16], int shift_bits, int array_width, int start_idx, int md_stepsize = 23);
int Quantize16x16_Luma_AC_16_SSE2(int16_t *dct16x16, int16_t scale_params[32], int offset_params[16], int shift_bits, int non_zero[16], int preserve_dc, int md_stepsize);
int Quantize16x16_Luma_AC_16_SSSE3(int16_t *dct16x16, int16_t scale_params[32], int offset_params[16], int shift_bits, int non_zero[16], int preserve_dc, int md_stepsize);
int Quantize8x16_Chroma_AC_16_SSE2(int16_t *dct8x16, int16_t scale_params[32], int offset, int shift_bits, int non_zero[8]);
int Quantize8x16_Chroma_AC_16_SSSE3(int16_t *dct8x16, int16_t scale_params[32], int offset_param, int shift_bits, int non_zero[8]);

int Quantize4x4_Luma_AC_32_SSE2(int *dct4x4, int scale_params[16], int offset_params[16], int shift_bits, int array_width, int start_idx, int md_stepsize = 23);
int Quantize4x4_AC_32(int *dct4x4, int scale_params[16], int offset_params[16], int shift_bits, int array_width, int start_idx = 1);
int Quantize4x4_AC_32_SSE2(int *dct4x4, int scale_params[16], int offset_params[16], int shift_bits, int array_width, int start_idx = 1);
int Quantize4x4_DC(int16_t dct4x4[16], int scale_param, int offset_param, int shift_bits);
int Quantize4x4_DC_32(int dct4x4[16], int scale_param, int offset_param, int shift_bits);
int Quantize4x4_DC_32_SSE2(int dct4x4[16], int scale_param, int offset_param, int shift_bits);
void RL_ZigZagScan_Sub_4x4_AC(int16_t dct4x4[16], int16_t level_run[2][32], int *qinvscaling, int qp_shift, int start_idx = 1);
int RL_ZigZagScan_Sub_4x4_AC_16(int16_t dct4x4[16], int16_t level_run[2][32], int *qinvscaling, int qp_shift, int start_idx = 1);
int RL_ZigZagScan_Sub_4x4_AC_32(int dct4x4[16], int16_t level_run[2][32], int *qinvscaling, int qp_shift, int start_idx = 1);
int RL_ZigZagScan_Sub_4x4_AC_UV_16(int16_t dct4x4[16], int16_t level_run[2][32], int *qinvscaling, int qp_shift);
int RL_ZigZagScan_Sub_4x4_AC_UV(int dct4x4[16], int16_t level_run[2][32], int *qinvscaling, int qp_shift);
int Quantize4x4_DC_UV(int dct2x2[2][4], int scale_param, int offset_param, int shift_bits);
int Quantize4x4_DC_UV_SSE2(int dct2x2[2][4], int scale_param, int offset_param, int shift_bits);

void RL_ZigZagScan_4x4(int16_t dct4x4[16], int16_t level_run[2][32]);
int RL_ZigZagScan_4x4_32(int dct4x4[16], int16_t level_run[2][32]);
bool RL_ZigZagScan_2x2(int dct2x2[4], int16_t level_run[2][8], int *coeff_cost);
#endif