#ifndef __MACROBLOCK_H
#define __MACROBLOCK_H

#include <stdint.h>
#include "Frame.h"
#include "Quantize.h"

//#define ENABLE_AVAILABLE_FLAG

#define MB_BLOCK_STRIDE 16

enum MB_TYPE_enum
{
	MB_TYPE_INxN = 0,
    MB_TYPE_I16x16_2_0_0 = 2,
	MB_TYPE_I16x16_2_2_1 = 23,

	MB_TYPE_P16x16 = 0,

	MB_TYPE_P_INxN = 5,
	MB_TYPE_P_I16x16_0_0_0 = 6,

	MB_TYPE_B_DIRECT_16x16 = 0,
	MB_TYPE_B_L0_16x16,
	MB_TYPE_B_L1_16x16,
	MB_TYPE_B_Bi_16x16,
	MB_TYPE_B_INxN = 23,
};

enum MB_Candidate_TYPE_enum
{
	MB_CAND_TYPE_I16x16 = 0,
	MB_CAND_TYPE_PSKIP = 1,
};


enum ADJACENT_MB_DIRECTION_enum
{
	ADJACENT_MB_LEFT = 0,
	ADJACENT_MB_ABOVE
};

enum INTRA_PRED_MODE_enum
{
	INTRA_PRED_MODE_V = 0,
	INTRA_PRED_MODE_H,
	INTRA_PRED_MODE_DC,
	INTRA_PRED_MODE_PLANE
};

enum CHROMA_PRED_MODE_enum
{
	CHROMA_PRED_MODE_DC = 0,
	CHROMA_PRED_MODE_H,
	CHROMA_PRED_MODE_V,
	CHROMA_PRED_MODE_PLANE
};

enum
{
	LUMA_DC_BIT = (1L << 16),
	CHROMA_DC_BIT = (1L << 16),
};

typedef struct  
{
	uint8_t block[16][16];
} u8_16x16_Type;

typedef struct  
{
	ChromaPixelType block[8][8];
} Chroma_8x8_Type;

typedef struct  
{
	int16_t level_run_dc[2][32];		// luma dc [level/run][data]
	int16_t level_run_ac[4][4][2][32];	// luma ac [4x4blocks across][4x4blocks down][level/run][data]

	int16_t level_run_dc_uv[2][2][8];		// chroma dc [u or v][level/run][data]
	int16_t level_run_ac_uv[2][2][2][2][32];	// chroma ac [u or v][4x4blocks across][4x4blocks down][level/run][data]

	int16_t mv_l0[2][4][4];		// motion vectors for list 0 [(x/y)][sub-block y][sub-block x]
	int16_t mvd_l0[2][4][4];	// motion vector deltas for list 0 [(x/y)][sub-block y][sub-block x]
	int16_t mv_l1[2][4][4];		// motion vectors for list 1 [(x/y)][sub-block y][sub-block x]
	int16_t mvd_l1[2][4][4];	// motion vector deltas for list 1 [(x/y)][sub-block y][sub-block x]

	int refIdxL0;
	int refIdxL1;

	int dummy[2];	// keep aligned to 16 bytes
} MBDataType;

typedef struct MacroBlock
{
	int mb_x, mb_y, mb_index;
	// pointers to neighbor MBs 
	struct MacroBlock *mbAddrA;	// left
	struct MacroBlock *mbAddrB;	// above
	struct MacroBlock *mbAddrC;	// above-right
	struct MacroBlock *mbAddrD;	// above-left;
	struct MacroBlock *mb3rdNeighbor;	// for places where mbAddrC is NULL, use mbAddrD
	int qp;
	int64_t cbf_bits[1];	// for storing coded block flag info for neighboring MBs only use [0] [1..2] is for YUV444 I believe 

	// from frame buffer
	int stride;
	int uv_stride;
	uint8_t *Y_frame_buffer;
	ChromaPixelType *UV_frame_buffer;

	QuantizeParamType *qparam;

	u8_16x16_Type *Y_pred_pixels;
	Chroma_8x8_Type *UV_pred_pixels;
	MBDataType *data;		// move level/run and mvd data to separate buffers to try to avoid cache miss/evictions in macroblock struct

	uint32_t luma_bit_flags;	 // holds bits to say if there are non-zero coefficients to encode (16 - DC, 0-15 AC)
	uint32_t chroma_bit_flags;   // holds bits to say if there are non-zero coefficients to encode (U: 0-3 AC  V: 4-7 AC)

	int luma_ipred_sad;
	int chroma_ipred_sad;
	int ipred_ssd;
	int me_sad;
	int me_cost;
	int me_ssd;
	int pskip_ssd;
	int pskip_sad;
	int coeff_cost;

	// full reconstructed frame buffer
	uint8_t *Y_frame_reconstructed;
	ChromaPixelType *UV_frame_reconstructed;
	int Y_block_start_index; // start index of this block within reconstructed frame
	int UV_block_start_index;

    uint8_t *Y_quarter_buffer;

	// pointers to local (stack) memory where skip pixels are copied to (in particular for BSKIP where pixels are averaged)
	uint8_t *skip_dst_y_buffer;
	uint8_t *skip_dst_uv_buffer;

	bool mb_skip_flag;
	bool mb_direct_flag;

	int est_bit_cost;	// estimated bit cost of macroblock

#ifdef ENABLE_AVAILABLE_FLAG
    bool available;
#endif
	
	bool isI16x16;

	int intra_pred_mode;
	int chroma_pred_mode;
	int coded_block_pattern_luma;
	int coded_block_pattern_chroma;
	int mb_type;

	int mb_partition_type; // used for motion estimation
	int qp_delta;
	int prev_qp_delta;

	int coeff_bits;
	int header_bits;

	int pmv[2];
	int pmv_skip[2][2];	// saved from CalcSkip() function
	int mv[2];
	int8_t direct_ref[2];

	// pointers to where to get data for skip pixels in reference frame (if faster, may become cache space where data is stored - BSKIP)
	uint8_t *skip_y_buffer;
	uint8_t *skip_uv_buffer;

} MacroBlockType;

void InitMacroBlocks();
void ResetMacroBlock(MacroBlockType *block);
void ProcessMacroBlock(YUVFrameType *frame, int index, int qp);
int  RC_ProcessMacroBlock(YUVFrameType *frame, int index, int qp, int *mad_est);	// just for rate control
MacroBlockType *HasAdjacent4x4SubBlock(MacroBlockType *mb, int direction, int *subx, int *suby, bool luma);
bool HasAdjacentMacroBlock(MacroBlockType *mb, int direction, int *adj_mb_type);
bool HasAdjacentMacroBlock_cpm(MacroBlockType *mb, int direction, int *adj_chroma_pred_mode);

void SetFrameForMBProcessing(YUVFrameType *frame);
MacroBlockType *GetCurrentMacroBlockSet();
MacroBlockType *GetQueuedMacroBlockSet(int index);
void FreeMacroBlockSet(MacroBlockType *set);
MacroBlockType *GetMacroBlock(int index);

int SumSquaredDifference_16x16(uint8_t *aligned_src1, int stride1, uint8_t *unaligned_src2, int stride2, int rows );

#ifdef ENABLE_AVAILABLE_FLAG
#define AVAILABLE(mb) ((mb)->available)
#else
#define AVAILABLE(mb) (true)
#endif

#endif