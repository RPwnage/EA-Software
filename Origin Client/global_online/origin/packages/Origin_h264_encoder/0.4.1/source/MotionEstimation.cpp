#include "MotionEstimation.h"
#include "Tables.h"
#include "MacroBlock.h"
#include "Frame.h"
#include "MotionInfo.h"
#include "Timing.h"
#include "Trace.h"
#include "Origin_h264_encoder.h"

#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <math.h>

#define USE_EXPAND_FACTOR

#ifdef USE_SSE2
#include <emmintrin.h>
#include <immintrin.h>
#endif

#ifdef SGIENG_DEBUG
#include <stdio.h>
#include <string.h>
extern FILE *sMVOutputFile;
#endif

typedef struct  
{
	YUVFrameType *frame;
	YUVFrameType *ref_frame;
	uint8_t *y_buffer_16x16;
	int cx;
	int cy;
	int mad;	// mean absolute diff (luma)
	int min_cost;
	int min_sad;
	int num_checks;
	MotionVector *pmv;
	MotionVector best_mv;
	MotionVector center_mv;
} ME_PredictionInfoType;

//#define SAD_TO_MV_BITS_FACTOR 20	// just for MV search - MV cost will be scaled back down for final decision mode
#define SAD_TO_MV_BITS_FACTOR	(frame->lambda_factor)

static int sMV_BitCostTable[MV_MAX * 2];
static int *sMV_BitCost = NULL;
static int sLambdaTable[3][2][MAX_QP][3];

TemporalMV_Type *gTemporalMV_Ref[2] = { NULL, NULL };

// search functions
bool ME_Search_Diamond(ME_PredictionInfoType *stype);
bool ME_Search_SmallDiamond(ME_PredictionInfoType *stype);
bool ME_EarlyTermination(ME_PredictionInfoType *stype);

SearchPattern smalldiamond;

// x and y coordinates of 4x4 blocks within a macroblock for blocktypes 0 thru 4
// { UL, UR, LL, LR }
static const short bx0[5][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,2,0,0}, {0,2,0,2}};
static const short by0[5][4] = {{0,0,0,0}, {0,0,0,0}, {0,2,0,0}, {0,0,0,0}, {0,0,2,2}};

static const short block_size[8][2] = 
{
    {16, 16},
    {16, 16},
    {16, 8},
    {8, 16},
    {8, 8},
    {8, 4},
    {4, 8},
    {4, 4}
};

// MV Prediction types
enum {
    MVPRED_MEDIAN   = 0,
    MVPRED_L        = 1,
    MVPRED_U        = 2,
    MVPRED_UR       = 3
} MVPredTypes;

static const MotionVector zero_mv;

// 8 block types
static const int MIN_THRES_BASE[8] = { 0, 64, 32, 32, 16, 8, 8, 4 };
static const int MED_THRES_BASE[8] = { 0, 256, 128, 128, 64, 32, 32, 16 };
static const int MAX_THRES_BASE[8] = { 0, 768, 384, 384, 192, 96, 96, 48 };
static const int MIN_THRES[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const int MED_THRES[8] = { 0, 8192, 4096, 4096, 2048, 1024, 1024, 512 };
static const int MAX_THRES[8] = { 0, 49152, 24576, 24576, 12288, 6144, 6144, 3072 };
#define DISTBLK_MAX 0x7FFFFFFF

int EPZSDetermineStopCriterion (int *prevSad, MacroBlockType *mb, int lambda_dist)
{
    int blocktype = mb->mb_partition_type;
    int blockshape_x = (block_size[blocktype][0] >> 2);
    int sadA, sadB, sadC, stopCriterion;
	
    sadA = (mb->mbAddrA != NULL) ? prevSad[-blockshape_x] : DISTBLK_MAX;
    sadB = (mb->mbAddrB != NULL) ? prevSad[0] : DISTBLK_MAX;
    sadC = (mb->mb3rdNeighbor != NULL) ? prevSad[blockshape_x] : DISTBLK_MAX;

    stopCriterion = IMIN (sadA, IMIN (sadB, sadC));
    stopCriterion = int(1.2f * stopCriterion + 128);

    return stopCriterion + lambda_dist;
}

// FOR NOW: assume 16x16
#ifdef USE_SSE2
int ComputeSAD_SSE2(uint8_t *prev, int stride_p, uint8_t *buffer, int stride_b, bool luma)
{
    int score = 0;
#ifdef ENABLE_TRACE
//	gEncoderState.current_frame->call_counter[0]++;
#endif
    __m128i mm_prev, mm_buff, mm_0, sum;

    mm_prev = _mm_loadu_si128((__m128i *) prev);
    mm_buff = _mm_load_si128((__m128i *) buffer);
    sum = _mm_sad_epu8(mm_prev, mm_buff);

#if 1	// unroll loop
    buffer += stride_b;
    prev += stride_p;
    mm_prev = _mm_loadu_si128((__m128i *) prev);
    mm_buff = _mm_load_si128((__m128i *) buffer);
    mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
    sum = _mm_add_epi16(sum, mm_0);

    buffer += stride_b;
    prev += stride_p;
    mm_prev = _mm_loadu_si128((__m128i *) prev);
    mm_buff = _mm_load_si128((__m128i *) buffer);
    mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
    sum = _mm_add_epi16(sum, mm_0);

    buffer += stride_b;
    prev += stride_p;
    mm_prev = _mm_loadu_si128((__m128i *) prev);
    mm_buff = _mm_load_si128((__m128i *) buffer);
    mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
    sum = _mm_add_epi16(sum, mm_0);

    buffer += stride_b;
    prev += stride_p;
    mm_prev = _mm_loadu_si128((__m128i *) prev);
    mm_buff = _mm_load_si128((__m128i *) buffer);
    mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
    sum = _mm_add_epi16(sum, mm_0);

    buffer += stride_b;
    prev += stride_p;
    mm_prev = _mm_loadu_si128((__m128i *) prev);
    mm_buff = _mm_load_si128((__m128i *) buffer);
    mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
    sum = _mm_add_epi16(sum, mm_0);

    buffer += stride_b;
    prev += stride_p;
    mm_prev = _mm_loadu_si128((__m128i *) prev);
    mm_buff = _mm_load_si128((__m128i *) buffer);
    mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
    sum = _mm_add_epi16(sum, mm_0);

    buffer += stride_b;
    prev += stride_p;
    mm_prev = _mm_loadu_si128((__m128i *) prev);
    mm_buff = _mm_load_si128((__m128i *) buffer);
    mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
    sum = _mm_add_epi16(sum, mm_0);

	if (!luma)
	{
		score = sum.m128i_i16[0] + sum.m128i_i16[4];

		return score;
	}

    buffer += stride_b;
    prev += stride_p;
    mm_prev = _mm_loadu_si128((__m128i *) prev);
    mm_buff = _mm_load_si128((__m128i *) buffer);
    mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
    sum = _mm_add_epi16(sum, mm_0);

    buffer += stride_b;
    prev += stride_p;
    mm_prev = _mm_loadu_si128((__m128i *) prev);
    mm_buff = _mm_load_si128((__m128i *) buffer);
    mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
    sum = _mm_add_epi16(sum, mm_0);

    buffer += stride_b;
    prev += stride_p;
    mm_prev = _mm_loadu_si128((__m128i *) prev);
    mm_buff = _mm_load_si128((__m128i *) buffer);
    mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
    sum = _mm_add_epi16(sum, mm_0);

    buffer += stride_b;
    prev += stride_p;
    mm_prev = _mm_loadu_si128((__m128i *) prev);
    mm_buff = _mm_load_si128((__m128i *) buffer);
    mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
    sum = _mm_add_epi16(sum, mm_0);

    buffer += stride_b;
    prev += stride_p;
    mm_prev = _mm_loadu_si128((__m128i *) prev);
    mm_buff = _mm_load_si128((__m128i *) buffer);
    mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
    sum = _mm_add_epi16(sum, mm_0);

    buffer += stride_b;
    prev += stride_p;
    mm_prev = _mm_loadu_si128((__m128i *) prev);
    mm_buff = _mm_load_si128((__m128i *) buffer);
    mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
    sum = _mm_add_epi16(sum, mm_0);

    buffer += stride_b;
    prev += stride_p;
    mm_prev = _mm_loadu_si128((__m128i *) prev);
    mm_buff = _mm_load_si128((__m128i *) buffer);
    mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
    sum = _mm_add_epi16(sum, mm_0);

    buffer += stride_b;
    prev += stride_p;
    mm_prev = _mm_loadu_si128((__m128i *) prev);
    mm_buff = _mm_load_si128((__m128i *) buffer);
    mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
    sum = _mm_add_epi16(sum, mm_0);

#else
    int imax = luma ? 16 : 8;
    for (int i = 1; i < imax; i++)
    {
        buffer += stride_b;
        prev += stride_p;
        mm_prev = _mm_loadu_si128((__m128i *) prev);
        mm_buff = _mm_load_si128((__m128i *) buffer);
        mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
        sum = _mm_add_epi16(sum, mm_0);
    }
#endif
    score = sum.m128i_i16[0] + sum.m128i_i16[4];

    return score;
}
#endif

int ComputeSAD(uint8_t *prev, uint8_t *buffer, int stride, bool luma)
{
    int diff = 0;

    int score = 0;
    int16_t *midABS_Table = &gABS_Table[1<<15];

    int imax = luma ? 16 : 8;
    for (int y = 0; y < imax; y++)
    {
        int start_index = y * stride;
        for (int x = 0; x < 16; x++)
        {
            diff = midABS_Table[(prev[start_index + x] - buffer[start_index + x])];
            score += diff;
        }
    }

    return score;
}

int ComputeSAD_SSE2_x(uint8_t *prev, int stride_p, uint8_t *buffer)
{
	const int stride_b = MB_BLOCK_STRIDE;
	int score = 0;
#ifdef ENABLE_TRACE
	gEncoderState.current_frame->call_counter[0]++;
#endif
	__m128i mm_prev, mm_buff, mm_0, sum;

	mm_prev = _mm_loadu_si128((__m128i *) prev);
	mm_buff = _mm_load_si128((__m128i *) buffer);
	sum = _mm_sad_epu8(mm_prev, mm_buff);

	buffer += stride_b;
	prev += stride_p;
	mm_prev = _mm_loadu_si128((__m128i *) prev);
	mm_buff = _mm_load_si128((__m128i *) buffer);
	mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
	sum = _mm_add_epi16(sum, mm_0);

	buffer += stride_b;
	prev += stride_p;
	mm_prev = _mm_loadu_si128((__m128i *) prev);
	mm_buff = _mm_load_si128((__m128i *) buffer);
	mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
	sum = _mm_add_epi16(sum, mm_0);

	buffer += stride_b;
	prev += stride_p;
	mm_prev = _mm_loadu_si128((__m128i *) prev);
	mm_buff = _mm_load_si128((__m128i *) buffer);
	mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
	sum = _mm_add_epi16(sum, mm_0);

	buffer += stride_b;
	prev += stride_p;
	mm_prev = _mm_loadu_si128((__m128i *) prev);
	mm_buff = _mm_load_si128((__m128i *) buffer);
	mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
	sum = _mm_add_epi16(sum, mm_0);

	buffer += stride_b;
	prev += stride_p;
	mm_prev = _mm_loadu_si128((__m128i *) prev);
	mm_buff = _mm_load_si128((__m128i *) buffer);
	mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
	sum = _mm_add_epi16(sum, mm_0);

	buffer += stride_b;
	prev += stride_p;
	mm_prev = _mm_loadu_si128((__m128i *) prev);
	mm_buff = _mm_load_si128((__m128i *) buffer);
	mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
	sum = _mm_add_epi16(sum, mm_0);

	buffer += stride_b;
	prev += stride_p;
	mm_prev = _mm_loadu_si128((__m128i *) prev);
	mm_buff = _mm_load_si128((__m128i *) buffer);
	mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
	sum = _mm_add_epi16(sum, mm_0);

	buffer += stride_b;
	prev += stride_p;
	mm_prev = _mm_loadu_si128((__m128i *) prev);
	mm_buff = _mm_load_si128((__m128i *) buffer);
	mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
	sum = _mm_add_epi16(sum, mm_0);

	buffer += stride_b;
	prev += stride_p;
	mm_prev = _mm_loadu_si128((__m128i *) prev);
	mm_buff = _mm_load_si128((__m128i *) buffer);
	mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
	sum = _mm_add_epi16(sum, mm_0);

	buffer += stride_b;
	prev += stride_p;
	mm_prev = _mm_loadu_si128((__m128i *) prev);
	mm_buff = _mm_load_si128((__m128i *) buffer);
	mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
	sum = _mm_add_epi16(sum, mm_0);

	buffer += stride_b;
	prev += stride_p;
	mm_prev = _mm_loadu_si128((__m128i *) prev);
	mm_buff = _mm_load_si128((__m128i *) buffer);
	mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
	sum = _mm_add_epi16(sum, mm_0);

	buffer += stride_b;
	prev += stride_p;
	mm_prev = _mm_loadu_si128((__m128i *) prev);
	mm_buff = _mm_load_si128((__m128i *) buffer);
	mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
	sum = _mm_add_epi16(sum, mm_0);

	buffer += stride_b;
	prev += stride_p;
	mm_prev = _mm_loadu_si128((__m128i *) prev);
	mm_buff = _mm_load_si128((__m128i *) buffer);
	mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
	sum = _mm_add_epi16(sum, mm_0);

	buffer += stride_b;
	prev += stride_p;
	mm_prev = _mm_loadu_si128((__m128i *) prev);
	mm_buff = _mm_load_si128((__m128i *) buffer);
	mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
	sum = _mm_add_epi16(sum, mm_0);

	buffer += stride_b;
	prev += stride_p;
	mm_prev = _mm_loadu_si128((__m128i *) prev);
	mm_buff = _mm_load_si128((__m128i *) buffer);
	mm_0 = _mm_sad_epu8(mm_prev, mm_buff);
	sum = _mm_add_epi16(sum, mm_0);

	score = sum.m128i_i16[0] + sum.m128i_i16[4];

	return score;
}


void ME_AddZeroPredictor(PredictorList &predictor_list)
{
    // check for duplicates
    bool exists = false;
    for( int j = 0; j < predictor_list.count; ++j )
    {
        if( (0 == predictor_list.point[j].mv_x) && (0 == predictor_list.point[j].mv_y) )
        {
            exists = true;
            break;
        }
    }

    if( !exists )
    {
        assert( predictor_list.count < PREDICTORS_MAX );
        predictor_list.point[predictor_list.count] = zero_mv;
        predictor_list.count++;
    }
}

#define DIAMOND_SIZE 8
#if 1 // big version
#define SHAPE_ITEMS 8
#define SHAPE_SIZE 1
static int diamond_x[SHAPE_ITEMS] = {SHAPE_SIZE,          0, -SHAPE_SIZE,          0, SHAPE_SIZE,  SHAPE_SIZE,-SHAPE_SIZE, -SHAPE_SIZE};
static int diamond_y[SHAPE_ITEMS] = {0,         -SHAPE_SIZE,           0, SHAPE_SIZE, SHAPE_SIZE, -SHAPE_SIZE, SHAPE_SIZE, -SHAPE_SIZE};
static int search_scale[4] = { 16, 32, 64, 128 };

void ME_AddExpandedDiamond(PredictorList &predictor_list)
{
	for( int j = 0; j < 4; ++j )
	{
		for( int i = 0; i < SHAPE_ITEMS; ++i )
		{
			MotionVector mv;
			mv.mv_x = static_cast<short>(diamond_x[i] * search_scale[j]);
			mv.mv_y = static_cast<short>(diamond_y[i] * search_scale[j]);

			bool exists = false;
#if 0	// check for duplicates
			for( int j = 0; j < predictor_list.count; ++j )
			{
				if( (mv.mv_x == predictor_list.point[j].mv_x) && (mv.mv_y == predictor_list.point[j].mv_y) )
				{
					exists = true;
					break;
				}
			}
#endif
			if( !exists )
			{
				assert( predictor_list.count < PREDICTORS_MAX );
				predictor_list.point[predictor_list.count].mv_x = mv.mv_x;
				predictor_list.point[predictor_list.count].mv_y = mv.mv_y;
				predictor_list.count++;
			}
		}

		int field_search_scale = ((search_scale[j] * 3) + 1) << 1;
		if (field_search_scale >= 128)
			continue;

		for( int i = 0; i < (SHAPE_ITEMS / 4); ++i )
		{
			MotionVector mv;
			mv.mv_x = static_cast<short>(diamond_x[i] * field_search_scale);
			mv.mv_y = static_cast<short>(diamond_y[i] * field_search_scale);

			bool exists = false;

			if( !exists )
			{
				assert( predictor_list.count < PREDICTORS_MAX );
				predictor_list.point[predictor_list.count].mv_x = mv.mv_x;
				predictor_list.point[predictor_list.count].mv_y = mv.mv_y;
				predictor_list.count++;
			}
		}
		for( int i = 4; i < SHAPE_ITEMS; ++i )
		{
			MotionVector mv;
			mv.mv_x = static_cast<short>(diamond_x[i] * field_search_scale);
			mv.mv_y = static_cast<short>(diamond_y[i] * search_scale[j]);

			bool exists = false;

			if( !exists )
			{
				assert( predictor_list.count < PREDICTORS_MAX );
				predictor_list.point[predictor_list.count].mv_x = mv.mv_x;
				predictor_list.point[predictor_list.count].mv_y = mv.mv_y;
				predictor_list.count++;
			}

			mv.mv_x = static_cast<short>(diamond_x[i] * search_scale[j]);
			mv.mv_y = static_cast<short>(diamond_y[i] * field_search_scale);

			if( !exists )
			{
				assert( predictor_list.count < PREDICTORS_MAX );
				predictor_list.point[predictor_list.count].mv_x = mv.mv_x;
				predictor_list.point[predictor_list.count].mv_y = mv.mv_y;
				predictor_list.count++;
			}
		}
	}
}
#else
static int diamond_x[4] = {DIAMOND_SIZE, 0, -DIAMOND_SIZE , 0};
static int diamond_y[4] = {0, -DIAMOND_SIZE, 0, DIAMOND_SIZE};
void ME_AddExpandedDiamond(PredictorList &predictor_list)
{
    for( int i = 0; i < 4; ++i )
    {
        MotionVector mv;
        mv.mv_x = diamond_x[i];
        mv.mv_y = diamond_y[i];

        bool exists = false;
        for( int j = 0; j < predictor_list.count; ++j )
        {
            if( (mv.mv_x == predictor_list.point[j].mv_x) && (mv.mv_y == predictor_list.point[j].mv_y) )
            {
                exists = true;
                break;
            }
        }

        if( !exists )
        {
            assert( predictor_list.count < PREDICTORS_MAX );
            predictor_list.point[predictor_list.count].mv_x = mv.mv_x;
            predictor_list.point[predictor_list.count].mv_y = mv.mv_y;
            predictor_list.count++;
        }
    }
}
#endif

void ME_AddSpatialPredictors(MacroBlockType *block, PredictorList &predictor_list, bool is_L0)
{
    if( block != NULL && !block->isI16x16 )
    {
        // check for duplicates
        MotionVector mv;
		if (is_L0)
		{
			mv.mv_x = block->data->mv_l0[0][0][0];
			mv.mv_y = block->data->mv_l0[1][0][0];
		}
		else
		{
			mv.mv_x = block->data->mv_l1[0][0][0];
			mv.mv_y = block->data->mv_l1[1][0][0];
		}

		bool exists = false;
        for( int j = 0; j < predictor_list.count; ++j )
        {
            if( (mv.mv_x == predictor_list.point[j].mv_x) && (mv.mv_y == predictor_list.point[j].mv_y) )
            {
                exists = true;
                break;
            }
        }

        if( !exists )
        {
            assert( predictor_list.count < PREDICTORS_MAX );
            predictor_list.point[predictor_list.count].mv_x = mv.mv_x;
            predictor_list.point[predictor_list.count].mv_y = mv.mv_y;
            predictor_list.count++;
        }
    }
}

void ME_SetRefFrame_TemporalMV(YUVFrameType *frame)
{
	int delta;

	if (frame->prev_ref_frame)
	{
		frame->prev_ref_frame->temporal_mv = gTemporalMV_Ref[0];

		// calc temporal distance scale
		delta = frame->pic_order_cnt - frame->prev_ref_frame->pic_order_cnt;
		frame->prev_ref_frame->temporal_mv_distscale = ((delta >> 1) + 256) / delta;
		frame->prev_ref_frame->temporal_ref_dist = delta;
	}

	if (frame->next_ref_frame)
	{
		frame->next_ref_frame->temporal_mv = gTemporalMV_Ref[1];

		// calc temporal distance scale
		delta = frame->next_ref_frame->pic_order_cnt - frame->pic_order_cnt;
		frame->next_ref_frame->temporal_mv_distscale = ((delta >> 1) + 256) / delta;
		frame->next_ref_frame->temporal_ref_dist = delta;
	}
}

void ME_AddTemporalPredictors(YUVFrameType *ref, int mb_index, PredictorList &predictor_list)
{
	// calculate mv by distance
	MotionVector mv;
	mv.mv_x = static_cast<short>((ref->temporal_ref_dist * ref->temporal_mv[mb_index].mx) >> 8);
	mv.mv_y = static_cast<short>((ref->temporal_ref_dist * ref->temporal_mv[mb_index].my) >> 8);

#if _DEBUG	
	if (gEncoderState.mb_debug_on)
	{
		ENC_TRACE("tmp: %5d [%5d %5d] [%5d %5d] %4d\n", mb_index, mv.mv_x, mv.mv_y, ref->temporal_mv[mb_index].mx, ref->temporal_mv[mb_index].my, ref->temporal_ref_dist);
	}
#endif

	if ((mv.mv_x == 0) && (mv.mv_y == 0))
		return;

	// check for duplicates
	bool exists = false;
	for( int j = 0; j < predictor_list.count; ++j )
	{
		if( (mv.mv_x == predictor_list.point[j].mv_x) && (mv.mv_y == predictor_list.point[j].mv_y) )
		{
			exists = true;
			break;
		}
	}

	if( !exists )
	{
		ENC_ASSERT( predictor_list.count < PREDICTORS_MAX );
		predictor_list.point[predictor_list.count].mv_x = mv.mv_x;
		predictor_list.point[predictor_list.count].mv_y = mv.mv_y;
		predictor_list.count++;
	}
}

#if _DEBUG
#define DEBUG_ME_CHECKPRED	\
{\
	if (gEncoderState.mb_debug_on)\
	{\
		ENC_TRACE("[%4d %4d] %5d %3d %5d %5d [%4d %4d]\n", mv.mv_x, mv.mv_y, cost, mv_cost, cost + mv_cost, info->min_cost, info->best_mv.mv_x, info->best_mv.mv_y);\
    }\
}

#define DEBUG_ME_SUBPIX(a,b)	\
{\
	if (gEncoderState.mb_debug_on)\
	{\
		ENC_TRACE("subpixel:(%2d %2d) [%4d %4d] %5d %3d %5d %5d [%4d %4d]\n", (a),(b), mv.mv_x, mv.mv_y, tmp_cost, mv_cost, tmp_cost + mv_cost, info->min_cost, info->best_mv.mv_x, info->best_mv.mv_y);\
	}\
}
#else
#define DEBUG_ME_CHECKPRED
#define DEBUG_ME_SUBPIX(a,b)
#endif

#define OFFSET_FROM_EDGE_LT -12
#define OFFSET_FROM_EDGE	4		// pixels in from right or bottom edge that we don't want to go near (buffer overflows)

// NOTE: turn this on when USE_SUB_PIXEL_ME is defined
#define USE_PREVIOUS_RECONSTRUCTED
// (cx, cy): pixel coordinates of current macroblock in frame
int ME_CheckPredictor(const MotionVector& mv, ME_PredictionInfoType *info)
{
    int cost = DISTBLK_MAX;
	int cx = info->cx;
	int cy = info->cy;
	YUVFrameType *frame = info->frame;
	YUVFrameType *prev_frame = info->ref_frame;

    // add predicted motion vector
    int px = cx + mv.mv_x;
    int py = cy + mv.mv_y;

	int stride =  frame->stride_padded;
	int prev_buffer_index = (py * stride) + px;

	// prefetch
	const char *pref = (const char *) &prev_frame->Y_frame_reconstructed[prev_buffer_index];
	_mm_prefetch(pref += stride, _MM_HINT_T0);
	_mm_prefetch(pref += stride, _MM_HINT_T0);
	_mm_prefetch(pref += stride, _MM_HINT_T0);
	_mm_prefetch(pref += stride, _MM_HINT_T0);
	_mm_prefetch(pref += stride, _MM_HINT_T0);
	_mm_prefetch(pref += stride, _MM_HINT_T0);
	_mm_prefetch(pref += stride, _MM_HINT_T0);
	_mm_prefetch(pref += stride, _MM_HINT_T0);
	_mm_prefetch(pref += stride, _MM_HINT_T0);
	_mm_prefetch(pref += stride, _MM_HINT_T0);
	_mm_prefetch(pref += stride, _MM_HINT_T0);
	_mm_prefetch(pref += stride, _MM_HINT_T0);
	_mm_prefetch(pref += stride, _MM_HINT_T0);
	_mm_prefetch(pref += stride, _MM_HINT_T0);
	_mm_prefetch(pref += stride, _MM_HINT_T0);
	_mm_prefetch(pref += stride, _MM_HINT_T0);


	int dx = mv.mv_x - (info->pmv->mv_x >> 2);
	int dy = mv.mv_y - (info->pmv->mv_y >> 2);

	info->mad = DISTBLK_MAX;

	if ((dx < -255) || (dx > 255) || (dy < -255) || (dy > 255))	// out of range (10-bits max)
		return DISTBLK_MAX;

    // TODO: Figure what to do if (px, py) is outside of frame.
    if( px >= OFFSET_FROM_EDGE_LT && px <= (frame->width - OFFSET_FROM_EDGE) && py >= OFFSET_FROM_EDGE_LT && py <= (frame->height - OFFSET_FROM_EDGE) )
    {
#ifdef USE_SSE2

		int mv_cost = (sMV_BitCost[(dx) << 2] + sMV_BitCost[(dy) << 2]) * SAD_TO_MV_BITS_FACTOR;

        // y
//        int curr_buffer_index = (cy * stride) + cx;
#ifdef USE_PREVIOUS_RECONSTRUCTED // compare against previous reconstructed frame
        cost = ComputeSAD_SSE2_x(&prev_frame->Y_frame_reconstructed[prev_buffer_index], stride, info->y_buffer_16x16);//, MB_BLOCK_STRIDE, true);
#else
        cost = ComputeSAD_SSE2(&prev_frame->Y_frame_buffer[prev_buffer_index], stride, info->y_buffer_16x16, MB_BLOCK_STRIDE, true);
#endif
		info->mad = cost;

		DEBUG_ME_CHECKPRED;

#if 0 //sgieng debug: compute chroma cost
        // uv
        int uv_stride = frame->uv_stride;
        int uv_stride_x2 = uv_stride * 2;
        int prev_buffer_index_chroma = ((py * uv_stride) + px) >> 1;
        int curr_buffer_index_chroma = ((cy * uv_stride) + cx) >> 1;
#if 1
#ifdef USE_PREVIOUS_RECONSTRUCTED // // compare against previous reconstructed frame
        cost += ComputeSAD_SSE2((uint8_t *)&prev_frame->UV_frame_reconstructed[prev_buffer_index_chroma], uv_stride_x2, (uint8_t *)&frame->UV_frame_buffer[curr_buffer_index_chroma], uv_stride_x2, false);
#else
        cost += ComputeSAD_SSE2((uint8_t *)&prev_frame->UV_frame_buffer[prev_buffer_index_chroma], uv_stride_x2, (uint8_t *)&frame->UV_frame_buffer[curr_buffer_index_chroma], uv_stride_x2, false);
#endif
#else
        // find sub-pixel type
        int sp_type = ((mv.mv_y & 1) << 1) | (mv.mv_x & 1);

        // absolute value
        int diff = 0;
        int score = 0;
        int16_t *midABS_Table = &gABS_Table[1<<15];

        switch(sp_type)
        {
        case 0:	// no sub-pixel
            cost += ComputeSAD_SSE2((uint8_t *)&prev_frame->UV_frame_reconstructed[prev_buffer_index_chroma], uv_stride_x2, (uint8_t *)&frame->UV_frame_buffer[curr_buffer_index_chroma], uv_stride_x2, false);
            break;
        case 1: // horizontal sub-pixel
            score = 0;
            for( int k = 0; k < 8; ++k )
            {
                int idx_p = k * prev_frame->uv_stride + prev_buffer_index_chroma;
                ChromaPixelType *p_prev = &prev_frame->UV_frame_reconstructed[idx_p];

                int idx_c = k * frame->uv_stride + curr_buffer_index_chroma;
                ChromaPixelType *p_curr = &frame->UV_frame_buffer[idx_c];

                for (int i = 0; i < 8; i++, p_curr++, p_prev++)
                {
                    int u_prev = (p_prev->u + (p_prev+1)->u + 1) >> 1;
                    int v_prev = (p_prev->v + (p_prev+1)->v + 1) >> 1;

                    diff = midABS_Table[p_curr->u - u_prev];
                    score += diff;
                    diff = midABS_Table[p_curr->v - v_prev];
                    score += diff;
                }
            }
            cost += score;
            break;
        case 2:	// vertical sub-pixel
            score = 0;
            for( int k = 0; k < 8; ++k )
            {
                int idx_p = k * prev_frame->uv_stride + prev_buffer_index_chroma;
                ChromaPixelType *p_prev = &prev_frame->UV_frame_reconstructed[idx_p];

                int idx_c = k * frame->uv_stride + curr_buffer_index_chroma;
                ChromaPixelType *p_curr = &frame->UV_frame_buffer[idx_c];

                for (int i = 0; i < 8; i++, p_curr++, p_prev++)
                {
                    int u_prev = (p_prev->u + (p_prev + prev_frame->uv_stride)->u + 1) >> 1;
                    int v_prev = (p_prev->v + (p_prev + prev_frame->uv_stride)->v + 1) >> 1;

                    diff = midABS_Table[p_curr->u - u_prev];
                    score += diff;
                    diff = midABS_Table[p_curr->v - v_prev];
                    score += diff;
                }
            }
            cost += score;
            break;
        case 3:	// horizontal & vertical sub-pixel
            score = 0;
            for( int k = 0; k < 8; ++k )
            {
                int idx_p = k * prev_frame->uv_stride + prev_buffer_index_chroma;
                ChromaPixelType *p_prev = &prev_frame->UV_frame_reconstructed[idx_p];

                int idx_c = k * frame->uv_stride + curr_buffer_index_chroma;
                ChromaPixelType *p_curr = &frame->UV_frame_buffer[idx_c];

                for (int i = 0; i < 8; i++, p_curr++, p_prev++)
                {
                    int u_prev = (p_prev->u + (p_prev+1)->u + (p_prev + prev_frame->uv_stride)->u + (p_prev + prev_frame->uv_stride + 1)->u + 2) >> 2;
                    int v_prev = (p_prev->v + (p_prev+1)->v + (p_prev + prev_frame->uv_stride)->v + (p_prev + prev_frame->uv_stride + 1)->v + 2) >> 2;

                    diff = midABS_Table[p_curr->u - u_prev];
                    score += diff;
                    diff = midABS_Table[p_curr->v - v_prev];
                    score += diff;
                }
            }
            cost += score;
            break;

        }
#endif
#endif // compute chroma cost

#else
        int stride =  frame->stride_padded;

        // luma
        int prev_buffer_index = (py * stride) + px;
        int curr_buffer_index = (cy * stride) + cx;
        cost = ComputeSAD(&prev_frame->Y_frame_reconstructed[prev_buffer_index], &frame->Y_frame_buffer[curr_buffer_index], stride, true);

        // chroma
        int uv_stride = frame->uv_stride;
        int prev_buffer_index_chroma = ((py * uv_stride) + px) >> 1;
        int curr_buffer_index_chroma = ((cy * uv_stride) + cx) >> 1;
        cost += ComputeSAD((uint8_t *)&prev_frame->UV_frame_reconstructed[prev_buffer_index_chroma], (uint8_t *)&frame->UV_frame_buffer[curr_buffer_index_chroma], uv_stride*2, false);
#endif

		cost += mv_cost;
    }

    return cost;
}

#define MAX_HALF_PIXEL_REF_SETS 4	// currently we only support four frames of half-pixel planes (which supports b-slice frames = 2)

int gLumaHalfPixelSetIndex = 0;
uint8_t *gLumaHalfPixelImages[MAX_HALF_PIXEL_REF_SETS][3] = {{ NULL, NULL, NULL }, { NULL, NULL, NULL }};
uint8_t * gLumaHalfPixelImage_H;
int16_t * sLumaHalfPixelImage_Tmp;
uint8_t * gLumaHalfPixelImage_V;
uint8_t * gLumaHalfPixelImage_D;

int CheckHalfPixel_H( ME_PredictionInfoType *info, MotionVector *half_mv)
{
    int min_cost = info->min_cost;
    int tmp_cost;

    int prev_buffer_index = 0;

	MotionVector mv = info->best_mv;
    int px = info->cx + mv.mv_x;
    int py = info->cy + mv.mv_y;
	YUVFrameType *frame = info->frame;
	YUVFrameType *ref_frame = info->ref_frame;

    // right
    if ((px < frame->width - OFFSET_FROM_EDGE))// && (mv.mv_x < SEARCH_RADIUS))
    {
        // get pointer to half-pixel block
        prev_buffer_index = (py *  frame->stride_padded) + px;
        tmp_cost = ComputeSAD_SSE2_x(&ref_frame->luma_buffers[ME_HALF_PIXEL_PLANE_H][prev_buffer_index],  frame->stride_padded, info->y_buffer_16x16);

		int mv_cost = (sMV_BitCost[(mv.mv_x << 2) - info->pmv->mv_x + 2] + sMV_BitCost[(mv.mv_y << 2) - info->pmv->mv_y]) * SAD_TO_MV_BITS_FACTOR;

		DEBUG_ME_SUBPIX(1,0);

        if( (tmp_cost + mv_cost) < min_cost )
        {
            min_cost = (tmp_cost + mv_cost);
			info->mad = info->min_sad = tmp_cost;
			info->min_cost = min_cost;
            half_mv->mv_x = 2;
            half_mv->mv_y = 0;
        }
    }

    // left
    if ((px > 0))// && (mv.mv_x > -SEARCH_RADIUS) )
    {
        // get pointer to half-pixel block
        int prev_buffer_index = (py *  frame->stride_padded) + (px - 1);
        tmp_cost = ComputeSAD_SSE2_x(&ref_frame->luma_buffers[ME_HALF_PIXEL_PLANE_H][prev_buffer_index],  frame->stride_padded, info->y_buffer_16x16);

		int mv_cost = (sMV_BitCost[(mv.mv_x << 2) - info->pmv->mv_x - 2] + sMV_BitCost[(mv.mv_y << 2) - info->pmv->mv_y]) * SAD_TO_MV_BITS_FACTOR;

		DEBUG_ME_SUBPIX(-1,0);

		if( (tmp_cost + mv_cost) < min_cost )
		{
			min_cost = (tmp_cost + mv_cost);
			info->mad = info->min_sad = tmp_cost;
			info->min_cost = min_cost;
            half_mv->mv_x = -2;
            half_mv->mv_y = 0;
        }
    }

    return min_cost;
}

int CheckHalfPixel_V( ME_PredictionInfoType *info, MotionVector *half_mv)
{
	int min_cost = info->min_cost;
	int tmp_cost;

	int prev_buffer_index = 0;

	MotionVector mv = info->best_mv;
	int px = info->cx + mv.mv_x;
	int py = info->cy + mv.mv_y;
	YUVFrameType *frame = info->frame;
	YUVFrameType *ref_frame = info->ref_frame;

    // bottom
    if ((py < frame->height - OFFSET_FROM_EDGE)) // && (mv.mv_y < SEARCH_RADIUS))
    {
        // get pointer to half-pixel block
        prev_buffer_index = (py *  frame->stride_padded) + px;
        tmp_cost = ComputeSAD_SSE2_x(&ref_frame->luma_buffers[ME_HALF_PIXEL_PLANE_V][prev_buffer_index],  frame->stride_padded, info->y_buffer_16x16);

		int mv_cost = (sMV_BitCost[(mv.mv_x << 2) - info->pmv->mv_x] + sMV_BitCost[(mv.mv_y << 2) - info->pmv->mv_y + 2]) * SAD_TO_MV_BITS_FACTOR;

		DEBUG_ME_SUBPIX(0,1);

		if( (tmp_cost + mv_cost) < min_cost )
		{
			min_cost = (tmp_cost + mv_cost);
			info->mad = info->min_sad = tmp_cost;
			info->min_cost = min_cost;
			half_mv->mv_x = 0;
			half_mv->mv_y = 2;
		}
    }

    // top
    if ((py > 0)) // && (mv.mv_y > -SEARCH_RADIUS) )
    {
        // get pointer to half-pixel block
        int prev_buffer_index = ((py - 1) *  frame->stride_padded) + px;
        tmp_cost = ComputeSAD_SSE2_x(&ref_frame->luma_buffers[ME_HALF_PIXEL_PLANE_V][prev_buffer_index],  frame->stride_padded, info->y_buffer_16x16);

		int mv_cost = (sMV_BitCost[(mv.mv_x << 2) - info->pmv->mv_x] + sMV_BitCost[(mv.mv_y << 2) - info->pmv->mv_y - 2]) * SAD_TO_MV_BITS_FACTOR;

		DEBUG_ME_SUBPIX(0,-1);

		if( (tmp_cost + mv_cost) < min_cost )
		{
			min_cost = (tmp_cost + mv_cost);
			info->mad = info->min_sad = tmp_cost;
			info->min_cost = min_cost;
			half_mv->mv_x = 0;
			half_mv->mv_y = -2;
		}
    }

    return min_cost;
}

int CheckHalfPixel_D( ME_PredictionInfoType *info, MotionVector *half_mv)
{
	int min_cost = info->min_cost;
	int tmp_cost;

	int prev_buffer_index = 0;

	MotionVector mv = info->best_mv;
	int px = info->cx + mv.mv_x;
	int py = info->cy + mv.mv_y;
	YUVFrameType *frame = info->frame;
	YUVFrameType *ref_frame = info->ref_frame;

    // bottom right
    if ((px < frame->width - OFFSET_FROM_EDGE) && (py < frame->height - OFFSET_FROM_EDGE)) // &&  (mv.mv_x < SEARCH_RADIUS) && (mv.mv_y < SEARCH_RADIUS))
    {
        // get pointer to half-pixel block
        prev_buffer_index = (py *  frame->stride_padded) + px;
        tmp_cost = ComputeSAD_SSE2_x(&ref_frame->luma_buffers[ME_HALF_PIXEL_PLANE_D][prev_buffer_index],  frame->stride_padded, info->y_buffer_16x16);

		int mv_cost = (sMV_BitCost[(mv.mv_x << 2) - info->pmv->mv_x + 2] + sMV_BitCost[(mv.mv_y << 2) - info->pmv->mv_y + 2]) * SAD_TO_MV_BITS_FACTOR;

		DEBUG_ME_SUBPIX(1,1);

		if( (tmp_cost + mv_cost) < min_cost )
		{
			min_cost = (tmp_cost + mv_cost);
			info->mad = info->min_sad = tmp_cost;
			info->min_cost = min_cost;
            half_mv->mv_x = 2;
            half_mv->mv_y = 2;
        }
    }

    // top right
    if ((px < frame->width - OFFSET_FROM_EDGE) && (py > 0)) // && (mv.mv_x < SEARCH_RADIUS) && (mv.mv_y > -SEARCH_RADIUS))
    {
        // get pointer to half-pixel block
        prev_buffer_index = ((py - 1) *  frame->stride_padded) + px;
        tmp_cost = ComputeSAD_SSE2_x(&ref_frame->luma_buffers[ME_HALF_PIXEL_PLANE_D][prev_buffer_index],  frame->stride_padded, info->y_buffer_16x16);

		int mv_cost = (sMV_BitCost[(mv.mv_x << 2) - info->pmv->mv_x + 2] + sMV_BitCost[(mv.mv_y << 2) - info->pmv->mv_y - 2]) * SAD_TO_MV_BITS_FACTOR;

		DEBUG_ME_SUBPIX(1,-1);

		if( (tmp_cost + mv_cost) < min_cost )
		{
			min_cost = (tmp_cost + mv_cost);
			info->mad = info->min_sad = tmp_cost;
			info->min_cost = min_cost;
            half_mv->mv_x = 2;
            half_mv->mv_y = -2;
        }
    }

    // bottom left
    if ((px > 0) && (py < frame->height - OFFSET_FROM_EDGE)) // && (mv.mv_y < SEARCH_RADIUS) && (mv.mv_x > -SEARCH_RADIUS))
    {
        // get pointer to half-pixel block
        int prev_buffer_index = (py *  frame->stride_padded) + (px - 1);
        tmp_cost = ComputeSAD_SSE2_x(&ref_frame->luma_buffers[ME_HALF_PIXEL_PLANE_D][prev_buffer_index],  frame->stride_padded, info->y_buffer_16x16);

		int mv_cost = (sMV_BitCost[(mv.mv_x << 2) - info->pmv->mv_x - 2] + sMV_BitCost[(mv.mv_y << 2) - info->pmv->mv_y + 2]) * SAD_TO_MV_BITS_FACTOR;

		DEBUG_ME_SUBPIX(-1,1);

		if( (tmp_cost + mv_cost) < min_cost )
		{
			min_cost = (tmp_cost + mv_cost);
			info->mad = info->min_sad = tmp_cost;
			info->min_cost = min_cost;
            half_mv->mv_x = -2;
            half_mv->mv_y = 2;
        }
    }

    // top left
    if ((px > 0) && (py > 0))	// &&  (mv.mv_y > -SEARCH_RADIUS) && (mv.mv_x > -SEARCH_RADIUS))
    {
        // get pointer to half-pixel block
        int prev_buffer_index = ((py - 1) *  frame->stride_padded) + (px - 1);
        tmp_cost = ComputeSAD_SSE2_x(&ref_frame->luma_buffers[ME_HALF_PIXEL_PLANE_D][prev_buffer_index],  frame->stride_padded, info->y_buffer_16x16);

		int mv_cost = (sMV_BitCost[(mv.mv_x << 2) - info->pmv->mv_x - 2] + sMV_BitCost[(mv.mv_y << 2) - info->pmv->mv_y - 2]) * SAD_TO_MV_BITS_FACTOR;

		DEBUG_ME_SUBPIX(-1,-1);

		if( (tmp_cost + mv_cost) < min_cost )
		{
			min_cost = (tmp_cost + mv_cost);
			info->mad = info->min_sad = tmp_cost;
			info->min_cost = min_cost;
            half_mv->mv_x = -2;
            half_mv->mv_y = -2;
        }
    }

    return min_cost;
}

int CheckHalfPixel( ME_PredictionInfoType *info, MotionVector *half_mv)
{
    // check half pixels
    CheckHalfPixel_H(info, half_mv);
    CheckHalfPixel_V(info, half_mv);
    CheckHalfPixel_D(info, half_mv);

    return info->min_cost;
}

#ifdef USE_QUARTER_PIXEL_ME
int ComputeSAD_Q2( uint8_t *prev0, uint8_t *prev1, uint8_t* prev, uint8_t *buffer, int stride )
{
    // average prev0 and prev1
    int stride_dst = 16;
    int idx_dst = 0;
    int idx_src = 0;
    for( int j = 0; j < 16; ++j, idx_dst += stride_dst, idx_src += stride )
    {
        for( int i = 0; i < 16; ++i )
        {
            prev[idx_dst + i] = (prev0[idx_src + i] + prev1[idx_src + i]) >> 1;
        }
    }

    int score = ComputeSAD_SSE2(prev, stride_dst, buffer, stride, true);

    return score;
}

int ComputeSAD_Q4( uint8_t *prev0, uint8_t *prev1, uint8_t *prev2, uint8_t *prev3, uint8_t *prev, uint8_t *buffer, int stride )
{
    // average prev0, prev1, prev2, prev3

    int stride_dst = 16;
    int idx_dst = 0;
    int idx_src = 0;
    for( int j = 0; j < 16; ++j, idx_dst += stride_dst, idx_src += stride )
    {
        for( int i = 0; i < 16; ++i )
        {
            prev[idx_dst + i] = (prev0[idx_src + i] + prev1[idx_src + i] + prev2[idx_src + i] + prev3[idx_src + i]) >> 2;
        }
    }

    int score = ComputeSAD_SSE2(prev, stride_dst, buffer, stride, true);

    return score;
}


const short quarter_pixels[8][2] =
{
    { -1, -1 }, { 0, -1 }, { 1, -1 },
    { -1, 0  },            { 1,  0 },
    { -1, 1  }, { 0,  1 }, { 1,  1 }
};

void GetQuarterPixelBuffers( int half_pixel, int quarter_pixel, int c_index, int stride, YUVFrameType* prev_frame, uint8_t **buffer_0, uint8_t **buffer_1, uint8_t **buffer_2, uint8_t **buffer_3)
{
    assert( half_pixel >= 0 && half_pixel <= 8 );
    assert( quarter_pixel >= 0 && quarter_pixel <= 7 );
    *buffer_0 = NULL;
    *buffer_1 = NULL;
    *buffer_2 = NULL;
    *buffer_3 = NULL;

    int h_index = 0, v_index = 0, d_index = 0;

    switch( half_pixel )
    {
    case 0:
        switch( quarter_pixel )
        {
        // horizontal
        case 3: // [ V(-1,-1) + D(-1,-1) ] / 2
            v_index = c_index - stride - 1;
            d_index = v_index;
            *buffer_0 = &gLumaHalfPixelImage_V[v_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        case 4: // [ D(-1,-1) + V(0,-1)  ] / 2
            v_index = c_index - stride;
            d_index = v_index - 1;
            *buffer_0 = &gLumaHalfPixelImage_V[v_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        // vertical
        case 1: // [ H(-1,-1) + D(-1,-1) ] / 2
            h_index = c_index - stride - 1;
            d_index = h_index;
            *buffer_0 = &gLumaHalfPixelImage_H[h_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        case 6: // [ D(-1,-1) + H(-1,0) ] / 2
            d_index = c_index - stride - 1;
            h_index = c_index - 1;
            *buffer_0 = &gLumaHalfPixelImage_H[h_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        // diagonal
        case 0: // [ (-1,-1) + H(-1,-1) + V(-1,-1) + D(-1,-1) ] / 4
            h_index = c_index - stride - 1;
            v_index = h_index;
            d_index = h_index;
            c_index = h_index;
            break;
        case 2: // [ H(-1,-1) + (0,-1) + D(-1,-1) + V(0,-1) ] / 4
            v_index = c_index - stride;
            h_index = v_index - 1;
            d_index = h_index;
            c_index = v_index;
            break;
        case 5: // [ V(-1,-1) + D(-1,-1) + (-1,0) + H(-1,0) ] / 4
            v_index = c_index - stride - 1;
            d_index = v_index;
            h_index = c_index - 1;
            c_index = h_index;
            break;
        case 7: // [ D(-1,-1) + V(0,-1) + H(-1,0) + (0,0) ] / 4
            v_index = c_index - stride;
            d_index = v_index - 1;
            h_index = c_index - 1;
            break;
        }
        break;
    case 1:
        switch( quarter_pixel )
        {
        // horizontal
        case 3: // [ D(-1, -1) + V(0, -1) ] / 2
            v_index = c_index - stride;
            d_index = v_index - 1;
            *buffer_0 = &gLumaHalfPixelImage_V[v_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        case 4: // [ D(0, -1) + V(0, -1) ] / 2
            v_index = c_index - stride;
            d_index = v_index;
            *buffer_0 = &gLumaHalfPixelImage_V[v_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        // vertical
        case 1: // [ (0, -1) + V(0, -1) ] / 2
            v_index = c_index - stride;
            c_index = v_index;
            *buffer_0 = &prev_frame->Y_frame_reconstructed[c_index];
            *buffer_1 = &gLumaHalfPixelImage_V[v_index];
            break;
        case 6: // [ (0, 0) + V(0, -1) ] / 2
            v_index = c_index - stride;
            *buffer_0 = &prev_frame->Y_frame_reconstructed[c_index];
            *buffer_1 = &gLumaHalfPixelImage_V[v_index];
            break;
        // diagonal
        case 0: // [ (0,-1) + H(-1,-1) + V(0,-1) + D(-1,-1) ] / 4
            v_index = c_index - stride;
            d_index = v_index - 1;
            h_index = d_index;
            c_index = v_index;
            break;
        case 2: // [ (0,-1) + H(0,-1) + V(0,-1) + D(0,-1) ] / 4
            v_index = c_index - stride;
            d_index = v_index;
            h_index = v_index;
            c_index = v_index;
            break;
        case 5: // [ (0,0) + H(-1,0) + V(0,-1) + D(-1,-1) ] / 4
            v_index = c_index - stride;
            d_index = v_index - 1;
            h_index = c_index - 1;
            break;
        case 7: // [ (0,0) + H(0,0) + V(0,-1) + D(0,-1) ] / 4
            v_index = c_index - stride;
            d_index = v_index;
            h_index = c_index;
            break;
        }
        break;
    case 2:
        switch( quarter_pixel )
        {
        // horizontal
        case 3: // [ D(0, -1) + V(0, -1) ] / 2
            v_index = c_index - stride;
            d_index = v_index;
            *buffer_0 = &gLumaHalfPixelImage_V[v_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        case 4: // [ D(0, -1) + V(1, -1) ] / 2
            d_index = c_index - stride;
            v_index = d_index - 1;
            *buffer_0 = &gLumaHalfPixelImage_V[v_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        // vertical
        case 1: // [ D(0, -1) + H(0, -1) ] / 2
            d_index = c_index - stride;
            h_index = d_index;
            *buffer_0 = &gLumaHalfPixelImage_H[h_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        case 6: // [ D(0, -1) + H(0, 0) ] / 2
            d_index = c_index - stride;
            h_index = c_index;
            *buffer_0 = &gLumaHalfPixelImage_H[h_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        // diagonal
        case 0: // [ (0,-1) + H(0,-1) + V(0,-1) + D(0,-1) ] / 4
            v_index = c_index - stride;
            d_index = v_index;
            h_index = v_index;
            c_index = v_index;
            break;
        case 2: // [ (1,-1) + H(0,-1) + V(1,-1) + D(0,-1) ] / 4
            h_index = c_index - stride;
            d_index = h_index;
            v_index = h_index + 1;
            c_index = v_index;
            break;
        case 5: // [ (0,0) + H(0,0) + V(0,-1) + D(0,-1) ] / 4
            v_index = c_index - stride;
            d_index = v_index;
            h_index = c_index;
            break;
        case 7: // [ (1,0) + H(0,0) + V(1,-1) + D(0,-1) ] / 4
            d_index = c_index - stride;
            v_index = d_index + 1;
            h_index = c_index;
            c_index = c_index + 1;
            break;
        }
        break;
    case 3:
        switch( quarter_pixel )
        {
        // horizontal
        case 3: // [ (-1,0) + H(-1,0) ] / 2
            h_index = c_index - 1;
            c_index = h_index;
            *buffer_0 = &prev_frame->Y_frame_reconstructed[c_index];
            *buffer_1 = &gLumaHalfPixelImage_H[h_index];
            break;
        case 4: // [ (0,0) + H(-1,0) ] / 2
            h_index = c_index - 1;
            *buffer_0 = &prev_frame->Y_frame_reconstructed[c_index];
            *buffer_1 = &gLumaHalfPixelImage_H[h_index];
            break;
        // vertical
        case 1: // [ D(-1,-1) + H(-1,0) ] / 2
            h_index = c_index - 1;
            d_index = h_index - stride;
            *buffer_0 = &gLumaHalfPixelImage_H[h_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        case 6: // [ D(-1,0) + H(-1,0) ] / 2
            h_index = c_index - 1;
            d_index = h_index;
            *buffer_0 = &gLumaHalfPixelImage_H[h_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        // diagonal
        case 0: // [ (-1,0) + H(-1,0) + V(-1,-1) + D(-1,-1) ] / 4
            h_index = c_index - 1;
            v_index = h_index - stride;
            d_index = v_index;
            c_index = h_index;
            break;
        case 2: // [ (0,0) + H(-1,0) + V(0,-1) + D(-1,-1) ] / 4
            v_index = c_index - stride;
            d_index = v_index - 1;
            h_index = c_index - 1;
            break;
        case 5: // [ (-1,0) + H(-1,0) + V(-1,0) + D(-1,0) ] / 4
            v_index = c_index - 1;
            d_index = v_index;
            h_index = v_index;
            c_index = v_index;
            break;
        case 7: // [ (0,0) + H(-1,0) + V(0,0) + D(-1,0) ] / 4
            d_index = c_index - 1;
            h_index = d_index;
            v_index = c_index;
            break;
        }
        break;
    case 4:
        switch( quarter_pixel )
        {
        // horizontal
        case 3: // [ H(-1,0) + (0,0) ] / 2
            h_index = c_index - 1;
            *buffer_0 = &prev_frame->Y_frame_reconstructed[c_index];
            *buffer_1 = &gLumaHalfPixelImage_H[h_index];
            break;
        case 4: // [ (0,0)   + H(0,0)] / 2
            h_index = c_index;
            *buffer_0 = &prev_frame->Y_frame_reconstructed[c_index];
            *buffer_1 = &gLumaHalfPixelImage_H[h_index];
            break;
        // vertical
        case 1: // [ V(0,-1) + (0,0) ] / 2
            v_index = c_index - stride;
            *buffer_0 = &prev_frame->Y_frame_reconstructed[c_index];
            *buffer_1 = &gLumaHalfPixelImage_V[v_index];
            break;
        case 6: // [ V(0,0)  + (0,0) ] / 2
            v_index = c_index;
            *buffer_0 = &prev_frame->Y_frame_reconstructed[c_index];
            *buffer_1 = &gLumaHalfPixelImage_V[v_index];
            break;
        // diagonal
        case 0: // [ D(-1,-1) + V(0,-1) + H(-1,0) + (0,0) ] / 4
            v_index = c_index - stride;
            d_index = v_index - 1;
            h_index = c_index - 1;
            break;
        case 2: // [ V(0,-1) + D(0,-1) + (0,0) + H(0,0) ] / 4
            v_index = c_index - stride;
            d_index = v_index;
            h_index = c_index;
            break;
        case 5: // [ H(-1,0) + (0,0) + D(-1,0) + V(0,0) ] / 4
            v_index = c_index;
            d_index = c_index - 1;
            h_index = d_index;
            break;
        case 7: // [ (0,0) + H(0,0) + V(0,0) + D(0,0) ] / 4
            v_index = c_index;
            d_index = c_index;
            h_index = c_index;
            break;
        }
        break;
    case 5:
        switch( quarter_pixel )
        {
        // horizontal
        case 3: // [ H(0,0) + (0,0) ] / 2
            h_index = c_index;
            *buffer_0 = &prev_frame->Y_frame_reconstructed[c_index];
            *buffer_1 = &gLumaHalfPixelImage_H[h_index];
            break;
        case 4: // [ H(0,0) + (1,0) ] / 2
            h_index = c_index;
            c_index = h_index + 1;
            *buffer_0 = &prev_frame->Y_frame_reconstructed[c_index];
            *buffer_1 = &gLumaHalfPixelImage_H[h_index];
            break;
        // vertical
        case 1: // [ H(0,0) + D(0,-1) ] / 2
            h_index = c_index;
            d_index = c_index - stride;
            *buffer_0 = &gLumaHalfPixelImage_H[h_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        case 6: // [ H(0,0) + D(0,0) ] / 2
            h_index = c_index;
            d_index = c_index;
            *buffer_0 = &gLumaHalfPixelImage_H[h_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        // diagonal
        case 0: // [ (0,0) + H(0,0) + V(0,-1) + D(0,-1) ] / 4
            v_index = c_index - stride;
            d_index = v_index;
            h_index = c_index;
            break;
        case 2: // [ (1,0) + H(0,0) + V(1,-1) + D(0,-1) ] / 4
            d_index = c_index - stride;
            v_index = d_index + 1;
            h_index = c_index;
            c_index = c_index + 1;
            break;
        case 5: // [ (0,0) + H(0,0) + V(0,0) + D(0,0) ] / 4
            v_index = c_index;
            h_index = c_index;
            d_index = c_index;
            break;
        case 7: // [ (1,0) + H(0,0) + V(1,0) + D(0,0) ] / 4
            v_index = c_index + 1;
            h_index = c_index;
            d_index = c_index;
            c_index = v_index;
            break;
        }
        break;
    case 6:
        switch( quarter_pixel )
        {
        // horizontal
        case 3: // [ V(-1,0) + D(-1,0) ] / 2
            v_index = c_index - 1;
            d_index = v_index;
            *buffer_0 = &gLumaHalfPixelImage_V[v_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        case 4: // [ V(0,0) + D(-1,0) ] / 2
            v_index = c_index;
            d_index = c_index - 1;
            *buffer_0 = &gLumaHalfPixelImage_V[v_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
            // vertical
        case 1: // [ H(-1,0) + D(-1,0) ] / 2
            h_index = c_index - 1;
            d_index = h_index;
            *buffer_0 = &gLumaHalfPixelImage_H[h_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        case 6: // [ H(-1,1) + D(-1,0) ] / 2
            d_index = c_index - 1;
            h_index = d_index + stride;
            *buffer_0 = &gLumaHalfPixelImage_H[h_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
            // diagonal
        case 0: // [ (-1,0) + H(-1,0) + V(-1,0) + D(-1,0) ] / 4
            v_index = c_index - 1;
            h_index = v_index;
            d_index = v_index;
            c_index = v_index;
            break;
        case 2: // [ (0,0) + H(-1,0) + V(0,0) + D(-1,0) ] / 4
            d_index = c_index - 1;
            h_index = d_index;
            v_index = c_index;
            break;
        case 5: // [ (-1,1) + H(-1,1) + V(-1,0) + D(-1,0) ] / 4
            v_index = c_index - 1;
            d_index = v_index;
            h_index = d_index + stride;
            c_index = h_index;
            break;
        case 7: // [ (0,1) + H(-1,1) + V(0,0) + D(-1,0) ] / 4
            d_index = c_index - 1;
            h_index = d_index + stride;
            v_index = c_index;
            c_index = c_index + stride;
            break;
        }
        break;
    case 7:
        switch( quarter_pixel )
        {
        // horizontal
        case 3: // [ V(0,0) + D(-1,0) ] / 2
            d_index = c_index - 1;
            v_index = c_index;
            *buffer_0 = &gLumaHalfPixelImage_V[v_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        case 4: // [ V(0,0) + D(0,0) ] / 2
            d_index = c_index;
            v_index = c_index;
            *buffer_0 = &gLumaHalfPixelImage_V[v_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        // vertical
        case 1: // [ V(0,0) + (0,0) ] / 2
            v_index = c_index;
            *buffer_0 = &prev_frame->Y_frame_reconstructed[c_index];
            *buffer_1 = &gLumaHalfPixelImage_V[v_index];
            break;
        case 6: // [ V(0,0) + (0,1) ] / 2
            v_index = c_index;
            c_index = c_index + stride;
            *buffer_0 = &prev_frame->Y_frame_reconstructed[c_index];
            *buffer_1 = &gLumaHalfPixelImage_V[v_index];
            break;
        // diagonal
        case 0: // [ (0,0) + H(-1,0) + V(0,0) + D(-1,0) ] / 4
            d_index = c_index - 1;
            h_index = d_index;
            v_index = c_index;
            break;
        case 2: // [ (0,0) + H(0,0) + V(0,0) + D(0,0) ] / 4
            v_index = c_index;
            h_index = c_index;
            d_index = c_index;
            break;
        case 5: // [ (0,1) + H(-1,1) + V(0,0) + D(-1,0) ] / 4
            d_index = c_index - 1;
            h_index = d_index + stride;
            v_index = c_index;
            c_index = c_index + stride;
            break;
        case 7: // [ (0,1) + H(0,1) + V(0,0) + D(0,0) ] / 4
            v_index = c_index;
            d_index = c_index;
            h_index = c_index + stride;
            c_index = h_index;
            break;
        }
        break;
    case 8:
        switch( quarter_pixel )
        {
        // horizontal
        case 3: // [ V(0,0) + D(0,0) ] / 2
            d_index = c_index;
            v_index = c_index;
            *buffer_0 = &gLumaHalfPixelImage_V[v_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        case 4: // [ V(1,0) + D(0,0) ] / 2
            d_index = c_index;
            v_index = c_index + 1;
            *buffer_0 = &gLumaHalfPixelImage_V[v_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        // vertical
        case 1: // [ H(0,0) + D(0,0) ] / 2
            h_index = c_index;
            d_index = c_index;
            *buffer_0 = &gLumaHalfPixelImage_H[h_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        case 6: // [ H(0,1) + D(0,0) ] / 2
            d_index = c_index;
            h_index = c_index + stride;
            *buffer_0 = &gLumaHalfPixelImage_H[h_index];
            *buffer_1 = &gLumaHalfPixelImage_D[d_index];
            break;
        // diagonal
        case 0: // [ (0,0) + H(0,0) + V(0,0) + D(0,0) ] / 4
            v_index = c_index;
            h_index = c_index;
            d_index = c_index;
            break;
        case 2: // [ (1,0) + H(0,0) + V(1,0) + D(0,0) ] / 4
            v_index = c_index + 1;
            d_index = c_index;
            h_index = c_index;
            c_index = v_index;
            break;
        case 5: // [ (0,1) + H(0,1) + V(0,0) + D(0,0) ] / 4
            v_index = c_index;
            d_index = c_index;
            h_index = c_index + stride;
            c_index = h_index;
            break;
        case 7: // [ (1,1) + H(0,1) + V(1,0) + D(0,0) ] / 4
            d_index = c_index;
            v_index = c_index + 1;
            h_index = c_index + stride;
            c_index = h_index + 1;
            break;
        }
        break;
    }

    assert( c_index >= 0 );
    assert( v_index >= 0 );
    assert( h_index >= 0 );
    assert( d_index >= 0 );

    if( quarter_pixel == 0 || quarter_pixel == 2 || quarter_pixel == 5 || quarter_pixel == 7 ) // diagonals
    {
        *buffer_0 = &prev_frame->Y_frame_reconstructed[c_index];
        *buffer_1 = &gLumaHalfPixelImage_H[h_index];
        *buffer_2 = &gLumaHalfPixelImage_V[v_index];
        *buffer_3 = &gLumaHalfPixelImage_D[d_index];
    }
}

int CheckQuarterPixel_( int half_pixel, uint8_t *qp_buffer[2], uint8_t **qp_final, int curr_buffer_index, int mvx, int mvy, int px, int py, int cost, MotionVector *quarter_mv, YUVFrameType *prev_frame, YUVFrameType *frame)
{
    int min_cost = cost;
    int prev_buffer_index = 0;
    int tmp_cost = 0x7fffffff;
    int quarter_pixel_index = -1;

    int qp_idx = 0;
    *qp_final = NULL;

    int c_index;
    uint8_t *buffer_0, *buffer_1, *buffer_2, *buffer_3, *buffer;

    buffer = &frame->Y_frame_buffer[curr_buffer_index];
    c_index = (py *  frame->stride_padded) + px;
    //c_buffer = &prev_frame->Y_frame_reconstructed[c_index];

    // check all 8 quarter pixels centered around half-pixel X
    // 0 1 2
    // 3 X 4
    // 5 6 7

    // horizontal: 3 and 4
    // 3:
    if( (px > 0) && (mvx > -SEARCH_RADIUS) )
    {
        GetQuarterPixelBuffers( half_pixel, 3, c_index,  frame->stride_padded, prev_frame, &buffer_0, &buffer_1, &buffer_2, &buffer_3 );
        tmp_cost = ComputeSAD_Q2( buffer_0, buffer_1, qp_buffer[qp_idx], buffer,  frame->stride_padded );

        if( tmp_cost < min_cost )
        {
            min_cost = tmp_cost;
            quarter_pixel_index = 3;
            *qp_final = qp_buffer[qp_idx];
            // update idx
            qp_idx++;
            qp_idx = qp_idx % 2;
        }
    }

    // 4:
    if( (px < frame->width - 16) && (mvx < SEARCH_RADIUS) )
    {
        GetQuarterPixelBuffers( half_pixel, 4, c_index,  frame->stride_padded, prev_frame, &buffer_0, &buffer_1, &buffer_2, &buffer_3 );
        tmp_cost = ComputeSAD_Q2( buffer_0, buffer_1, qp_buffer[qp_idx], buffer,  frame->stride_padded );

        if( tmp_cost < min_cost )
        {
            min_cost = tmp_cost;
            quarter_pixel_index = 4;
            *qp_final = qp_buffer[qp_idx];
            // update idx
            qp_idx++;
            qp_idx = qp_idx % 2;
        }
    }

    // vertical: 1 and 6
    // 1:
    if( (py > 0) && (mvy > -SEARCH_RADIUS) )
    {
        GetQuarterPixelBuffers( half_pixel, 1, c_index,  frame->stride_padded, prev_frame, &buffer_0, &buffer_1, &buffer_2, &buffer_3 );
        tmp_cost = ComputeSAD_Q2( buffer_0, buffer_1, qp_buffer[qp_idx], buffer,  frame->stride_padded );

        if( tmp_cost < min_cost )
        {
            min_cost = tmp_cost;
            quarter_pixel_index = 1;
            *qp_final = qp_buffer[qp_idx];
            // update idx
            qp_idx++;
            qp_idx = qp_idx % 2;
        }
    }

    // 6:
    if( (py < frame->height - 16) && (mvy < SEARCH_RADIUS) )
    {
        GetQuarterPixelBuffers( half_pixel, 6, c_index,  frame->stride_padded, prev_frame, &buffer_0, &buffer_1, &buffer_2, &buffer_3 );
        tmp_cost = ComputeSAD_Q2( buffer_0, buffer_1, qp_buffer[qp_idx], buffer,  frame->stride_padded );

        if( tmp_cost < min_cost )
        {
            min_cost = tmp_cost;
            quarter_pixel_index = 6;
            *qp_final = qp_buffer[qp_idx];
            // update idx
            qp_idx++;
            qp_idx = qp_idx % 2;
        }
    }

    // diagonal: 0, 2, 5, and 7
    // 0:
    if( (py > 0) && (px > 0) && (mvy > -SEARCH_RADIUS) && (mvx > -SEARCH_RADIUS) )
    {
        GetQuarterPixelBuffers( half_pixel, 0, c_index,  frame->stride_padded, prev_frame, &buffer_0, &buffer_1, &buffer_2, &buffer_3 );
        tmp_cost = ComputeSAD_Q4( buffer_0, buffer_1, buffer_2, buffer_3, qp_buffer[qp_idx], buffer,  frame->stride_padded );

        if( tmp_cost < min_cost )
        {
            min_cost = tmp_cost;
            quarter_pixel_index = 0;
            *qp_final = qp_buffer[qp_idx];
            // update idx
            qp_idx++;
            qp_idx = qp_idx % 2;
        }
    }

    // 2:
    if( (py > 0) && (px < frame->width - 16) && (mvy > -SEARCH_RADIUS) && (mvx < SEARCH_RADIUS) )
    {
        GetQuarterPixelBuffers( half_pixel, 2, c_index,  frame->stride_padded, prev_frame, &buffer_0, &buffer_1, &buffer_2, &buffer_3 );
        tmp_cost = ComputeSAD_Q4( buffer_0, buffer_1, buffer_2, buffer_3, qp_buffer[qp_idx], buffer,  frame->stride_padded );

        if( tmp_cost < min_cost )
        {
            min_cost = tmp_cost;
            quarter_pixel_index = 2;
            *qp_final = qp_buffer[qp_idx];
            // update idx
            qp_idx++;
            qp_idx = qp_idx % 2;
        }
    }

    // 5:
    if( (py < frame->height - 16) && (px > 0) && (mvy < SEARCH_RADIUS) && (mvx > -SEARCH_RADIUS) )
    {
        GetQuarterPixelBuffers( half_pixel, 5, c_index,  frame->stride_padded, prev_frame, &buffer_0, &buffer_1, &buffer_2, &buffer_3 );
        tmp_cost = ComputeSAD_Q4( buffer_0, buffer_1, buffer_2, buffer_3, qp_buffer[qp_idx], buffer,  frame->stride_padded );

        if( tmp_cost < min_cost )
        {
            min_cost = tmp_cost;
            quarter_pixel_index = 5;
            *qp_final = qp_buffer[qp_idx];
            // update idx
            qp_idx++;
            qp_idx = qp_idx % 2;
        }
    }

    // 7:
    if( (py < frame->height - 16) && (px < frame->width - 16) && (mvy < SEARCH_RADIUS) && (mvx < SEARCH_RADIUS) )
    {
        GetQuarterPixelBuffers( half_pixel, 7, c_index,  frame->stride_padded, prev_frame, &buffer_0, &buffer_1, &buffer_2, &buffer_3 );
        tmp_cost = ComputeSAD_Q4( buffer_0, buffer_1, buffer_2, buffer_3, qp_buffer[qp_idx], buffer,  frame->stride_padded );

        if( tmp_cost < min_cost )
        {
            min_cost = tmp_cost;
            quarter_pixel_index = 7;
            *qp_final = qp_buffer[qp_idx];
            // update idx
            qp_idx++;
            qp_idx = qp_idx % 2;
        }
    }

    if( quarter_pixel_index > -1 )
    {
        quarter_mv->mv_x = quarter_pixels[quarter_pixel_index][0];
        quarter_mv->mv_y = quarter_pixels[quarter_pixel_index][1];
    }

    return min_cost;
}

int CheckQuarterPixel( MacroBlockType *mb, int cx, int cy, int mvx, int mvy, int cost, MotionVector half_mv, MotionVector *quarter_mv, YUVFrameType *prev_frame, YUVFrameType *frame )
{
    // save the best luma values
    uint8_t *quarter_pixels_final = NULL;
    uint8_t *quarter_pixels[2];
    quarter_pixels[0] = (uint8_t *) _aligned_malloc(16 * 16, 32);
    quarter_pixels[1] = (uint8_t *) _aligned_malloc(16 * 16, 32);

    // which pixel (1 of 9) is the center of the quarter-pixel search
    // 0 1 2
    // 3 4 5
    // 6 7 8
    int half_pixel_idx = 0;
    if( half_mv.mv_y == 0 )
        half_pixel_idx = 3;
    else if( half_mv.mv_y > 0 )
        half_pixel_idx = 6;

    if( half_mv.mv_x == 0 )
        half_pixel_idx += 1;
    else if( half_mv.mv_x > 0 )
        half_pixel_idx += 2;

    // index of block of current frame
    int curr_buffer_index = (cy *  frame->stride_padded) + cx;
    int prev_buffer_index = 0;

    // integer position of predicted block
    int px = cx + mvx;
    int py = cy + mvy;

    cost = CheckQuarterPixel_( half_pixel_idx, quarter_pixels, &quarter_pixels_final, curr_buffer_index, mvx, mvy, px, py, cost, quarter_mv, prev_frame, frame);

    // copy the quarter pixels to the Y_pred_pixels
    if(quarter_pixels_final)
    {
        assert( (quarter_mv->mv_x != 0) || (quarter_mv->mv_y != 0) );

        int idx = 0;
        int idx_q = 0;
        for( int j = 0; j < 16; ++j, idx += 16, idx_q +=  frame->stride_padded )
        {
            memcpy(&mb->Y_quarter_buffer[idx_q], &quarter_pixels_final[idx], 16);
        }
    }

    _aligned_free( quarter_pixels[0] );
    _aligned_free( quarter_pixels[1] );

    return cost;
}
#endif

static inline int rshift_rnd_sf(int x, int a)
{
    return ((x + (1 << (a-1) )) >> a);
}

typedef enum{
    CHROMA_SUB = 0,
    CHROMA_FULL,
    CHROMA_FULL_X,
    CHROMA_FULL_Y
} CHROMA_TYPE;

#define CLIP_N_PACK(shift) 	mm_sum_lo = _mm_add_epi16(mm_sum_lo, mm_rounding);\
							mm_sum_lo = _mm_srai_epi16(mm_sum_lo, (shift));\
							mm_sum_lo = _mm_max_epi16(mm_sum_lo, mm_zero);\
							mm_sum_lo = _mm_min_epi16(mm_sum_lo, mm_max_8bit);\
							mm_sum_hi = _mm_add_epi16(mm_sum_hi, mm_rounding);\
							mm_sum_hi = _mm_srai_epi16(mm_sum_hi, (shift));\
							mm_sum_hi = _mm_max_epi16(mm_sum_hi, mm_zero);\
							mm_sum_hi = _mm_min_epi16(mm_sum_hi, mm_max_8bit);\
							mm_sum_lo = _mm_packus_epi16(mm_sum_lo, mm_sum_hi);

#define WEIGHT_PIXELS1(x,y,lo,hi,factorx,factory)	mm_tmp_x = _mm_unpacklo_epi8((x),mm_zero);\
													mm_tmp_x = _mm_mullo_epi16(mm_tmp_x,(factorx));\
													mm_tmp_y = _mm_unpacklo_epi8((y),mm_zero);\
													mm_tmp_y = _mm_mullo_epi16(mm_tmp_y,(factory));\
													(lo) = _mm_add_epi16(mm_tmp_x, mm_tmp_y);\
													mm_tmp_x = _mm_unpackhi_epi8((x),mm_zero);\
													mm_tmp_x = _mm_mullo_epi16(mm_tmp_x,(factorx));\
													mm_tmp_y = _mm_unpackhi_epi8((y),mm_zero);\
													mm_tmp_y = _mm_mullo_epi16(mm_tmp_y,(factory));\
													(hi) = _mm_add_epi16(mm_tmp_x, mm_tmp_y);

#define WEIGHT_PIXELS2(x,y,lo,hi,factorx,factory)	mm_tmp_x = _mm_unpacklo_epi8((x),mm_zero);\
													mm_tmp_x = _mm_mullo_epi16(mm_tmp_x,(factorx));\
													(lo) = _mm_add_epi16(mm_tmp_x, (lo));\
													mm_tmp_y = _mm_unpacklo_epi8((y),mm_zero);\
													mm_tmp_y = _mm_mullo_epi16(mm_tmp_y,(factory));\
													(lo) = _mm_add_epi16((lo), mm_tmp_y);\
													mm_tmp_x = _mm_unpackhi_epi8((x),mm_zero);\
													mm_tmp_x = _mm_mullo_epi16(mm_tmp_x,(factorx));\
													(hi) = _mm_add_epi16(mm_tmp_x, (hi));\
													mm_tmp_y = _mm_unpackhi_epi8((y),mm_zero);\
													mm_tmp_y = _mm_mullo_epi16(mm_tmp_y,(factory));\
													(hi) = _mm_add_epi16((hi), mm_tmp_y);

inline void CalcChromaHalfPixel_Weighted(uint8_t *dst, uint8_t *src, int stride, int weights[4])
{	// 16x8
	__m128i mm_src1, mm_src2, mm_src3, mm_src4;
	__m128i mm_tmp_x, mm_tmp_y, mm_sum_lo, mm_sum_hi;
	__m128i mm_zero = _mm_setzero_si128();
	__m128i mm_max_8bit = _mm_set1_epi16(255);
	__m128i mm_rounding = _mm_set1_epi16(1<<5);
	__m128i weight_00 = _mm_set1_epi16(static_cast<short>(weights[0]));
	__m128i weight_01 = _mm_set1_epi16(static_cast<short>(weights[1]));
	__m128i weight_10 = _mm_set1_epi16(static_cast<short>(weights[2]));
	__m128i weight_11 = _mm_set1_epi16(static_cast<short>(weights[3]));

	mm_src1 = _mm_loadu_si128((__m128i *) src);
	mm_src2 = _mm_loadu_si128((__m128i *) (src + 2));	// one uv pixel over
	for (int i = 0; i < 8; i++)
	{
		WEIGHT_PIXELS1(mm_src1, mm_src2, mm_sum_lo, mm_sum_hi, weight_00, weight_01);
		src += stride;
		mm_src3 = _mm_loadu_si128((__m128i *) src);
		mm_src4 = _mm_loadu_si128((__m128i *) (src + 2));	// one uv pixel over
		WEIGHT_PIXELS2(mm_src3, mm_src4, mm_sum_lo, mm_sum_hi, weight_10, weight_11);

		// clip and pack
		CLIP_N_PACK(6);
		_mm_storeu_si128((__m128i *) dst, mm_sum_lo);
		dst += MB_BLOCK_STRIDE;

		mm_src1 = mm_src3;	// we don't need to reload these pixels
		mm_src2 = mm_src4;
	}
}

inline void CalcChromaHalfPixel_H(uint8_t *dst, uint8_t *src, int stride)
{	// 16x8
	__m128i mm_src1, mm_src2;

	for (int i = 0; i < 8; i++, dst += MB_BLOCK_STRIDE, src += stride)
	{
		mm_src1 = _mm_loadu_si128((__m128i *) src);
		mm_src2 = _mm_loadu_si128((__m128i *) (src + 2));	// one uv pixel over
		mm_src1 = _mm_avg_epu8(mm_src1, mm_src2);
		_mm_store_si128((__m128i *) dst, mm_src1);
	}
}

inline void CalcChromaHalfPixel_V(uint8_t *dst, uint8_t *src, int stride)
{	// 16x8
	__m128i mm_src1, mm_src2, mm_avg;

	mm_src1 = _mm_loadu_si128((__m128i *) src);
	src += stride;
	mm_src2 = _mm_loadu_si128((__m128i *) src);	// one uv pixel down
	src += stride;
	mm_avg = _mm_avg_epu8(mm_src1, mm_src2);
	_mm_store_si128((__m128i *) dst, mm_avg);
	dst += MB_BLOCK_STRIDE;
	mm_src1 = _mm_loadu_si128((__m128i *) src);
	src += stride;
	mm_avg = _mm_avg_epu8(mm_src1, mm_src2);
	_mm_store_si128((__m128i *) dst, mm_avg);
	dst += MB_BLOCK_STRIDE;
	mm_src2 = _mm_loadu_si128((__m128i *) src);	// one uv pixel down
	src += stride;
	mm_avg = _mm_avg_epu8(mm_src1, mm_src2);
	_mm_store_si128((__m128i *) dst, mm_avg);
	dst += MB_BLOCK_STRIDE;

	mm_src1 = _mm_loadu_si128((__m128i *) src);
	src += stride;
	mm_avg = _mm_avg_epu8(mm_src1, mm_src2);
	_mm_store_si128((__m128i *) dst, mm_avg);
	dst += MB_BLOCK_STRIDE;
	mm_src2 = _mm_loadu_si128((__m128i *) src);	// one uv pixel down
	src += stride;
	mm_avg = _mm_avg_epu8(mm_src1, mm_src2);
	_mm_store_si128((__m128i *) dst, mm_avg);
	dst += MB_BLOCK_STRIDE;

	mm_src1 = _mm_loadu_si128((__m128i *) src);
	src += stride;
	mm_avg = _mm_avg_epu8(mm_src1, mm_src2);
	_mm_store_si128((__m128i *) dst, mm_avg);
	dst += MB_BLOCK_STRIDE;
	mm_src2 = _mm_loadu_si128((__m128i *) src);	// one uv pixel down
	src += stride;
	mm_avg = _mm_avg_epu8(mm_src1, mm_src2);
	_mm_store_si128((__m128i *) dst, mm_avg);
	dst += MB_BLOCK_STRIDE;

	mm_src1 = _mm_loadu_si128((__m128i *) src);
	mm_avg = _mm_avg_epu8(mm_src1, mm_src2);
	_mm_store_si128((__m128i *) dst, mm_avg);
}

#define SUM_LINES_16(src1,src2,dst_lo,dst_hi) (dst_lo) = _mm_unpacklo_epi8((src1),mm_zero);\
											  mm_tmp   = _mm_unpacklo_epi8((src2),mm_zero);\
											  (dst_lo) = _mm_add_epi16(mm_tmp, (dst_lo));\
											  (dst_hi) = _mm_unpackhi_epi8((src1),mm_zero);\
											  mm_tmp   = _mm_unpackhi_epi8((src2),mm_zero);\
											  (dst_hi) = _mm_add_epi16(mm_tmp, (dst_hi));

#define AVERAGE_4(src0_lo,src0_hi,src1_lo,src1_hi,dst) (src0_lo) = _mm_add_epi16((src0_lo),(src1_lo));\
													   (src0_lo) = _mm_add_epi16((src0_lo),mm_rounding);\
													   (src0_lo) = _mm_srai_epi16((src0_lo),2);\
													   (src0_hi) = _mm_add_epi16((src0_hi),(src1_hi));\
													   (src0_hi) = _mm_add_epi16((src0_hi),mm_rounding);\
													   (src0_hi) = _mm_srai_epi16((src0_hi),2);\
													   (dst) = _mm_packus_epi16((src0_lo),(src0_hi));

inline void CalcChromaHalfPixel_D(uint8_t *dst, uint8_t *src, int stride)
{	// 16x8
#if 0
	for (int j = 0; j < 8; j++, dst += MB_BLOCK_STRIDE, src += stride)
	{
		for (int i = 0; i < 16; i++)
		{
			dst[i] = (src[i] + src[i+2] + src[stride+i] + src[stride+i+2] + 2) >> 2;
		}
	}
#else
	__m128i mm_src1, mm_src2, mm_src3, mm_avg, mm_sum0_lo, mm_sum0_hi, mm_sum1_lo, mm_sum1_hi, mm_tmp;
	__m128i mm_zero = _mm_setzero_si128();
	__m128i mm_rounding = _mm_set1_epi16(2);

	mm_src1 = _mm_loadu_si128((__m128i *) src);
	mm_src3 = _mm_loadu_si128((__m128i *) (src + 2));	// one uv pixel over

	SUM_LINES_16(mm_src1,mm_src3,mm_sum0_lo,mm_sum0_hi);

	src += stride;
	mm_src2 = _mm_loadu_si128((__m128i *) src);		// one uv pixel down
	mm_src3 = _mm_loadu_si128((__m128i *) (src + 2));	// one uv pixel over
	SUM_LINES_16(mm_src2,mm_src3,mm_sum1_lo,mm_sum1_hi);

	src += stride;
	AVERAGE_4(mm_sum0_lo,mm_sum0_hi,mm_sum1_lo,mm_sum1_hi,mm_avg);		// average horizontal and vertical
	_mm_store_si128((__m128i *) dst, mm_avg);		// store line 0
	dst += MB_BLOCK_STRIDE;
	mm_src1 = _mm_loadu_si128((__m128i *) src);
	mm_src3 = _mm_loadu_si128((__m128i *) (src + 2));	// one uv pixel over
	SUM_LINES_16(mm_src1,mm_src3,mm_sum0_lo,mm_sum0_hi);
	src += stride;
	AVERAGE_4(mm_sum1_lo,mm_sum1_hi,mm_sum0_lo,mm_sum0_hi,mm_avg);		// average horizontal and vertical
	_mm_store_si128((__m128i *) dst, mm_avg);		// store line 1
	dst += MB_BLOCK_STRIDE;
	mm_src2 = _mm_loadu_si128((__m128i *) src);		// one uv pixel down
	mm_src3 = _mm_loadu_si128((__m128i *) (src + 2));	// one uv pixel over
	SUM_LINES_16(mm_src2,mm_src3,mm_sum1_lo,mm_sum1_hi);
	src += stride;
	AVERAGE_4(mm_sum0_lo,mm_sum0_hi,mm_sum1_lo,mm_sum1_hi,mm_avg);		// average horizontal and vertical
	_mm_store_si128((__m128i *) dst, mm_avg);		// store line 2
	dst += MB_BLOCK_STRIDE;

	mm_src1 = _mm_loadu_si128((__m128i *) src);
	mm_src3 = _mm_loadu_si128((__m128i *) (src + 2));	// one uv pixel over
	SUM_LINES_16(mm_src1,mm_src3,mm_sum0_lo,mm_sum0_hi);
	src += stride;
	AVERAGE_4(mm_sum1_lo,mm_sum1_hi,mm_sum0_lo,mm_sum0_hi,mm_avg);		// average horizontal and vertical
	_mm_store_si128((__m128i *) dst, mm_avg);		// store line 3
	dst += MB_BLOCK_STRIDE;
	mm_src2 = _mm_loadu_si128((__m128i *) src);		// one uv pixel down
	mm_src3 = _mm_loadu_si128((__m128i *) (src + 2));	// one uv pixel over
	SUM_LINES_16(mm_src2,mm_src3,mm_sum1_lo,mm_sum1_hi);
	src += stride;
	AVERAGE_4(mm_sum0_lo,mm_sum0_hi,mm_sum1_lo,mm_sum1_hi,mm_avg);		// average horizontal and vertical
	_mm_store_si128((__m128i *) dst, mm_avg);		// store line 4
	dst += MB_BLOCK_STRIDE;

	mm_src1 = _mm_loadu_si128((__m128i *) src);
	mm_src3 = _mm_loadu_si128((__m128i *) (src + 2));	// one uv pixel over
	SUM_LINES_16(mm_src1,mm_src3,mm_sum0_lo,mm_sum0_hi);
	src += stride;
	AVERAGE_4(mm_sum1_lo,mm_sum1_hi,mm_sum0_lo,mm_sum0_hi,mm_avg);		// average horizontal and vertical
	_mm_store_si128((__m128i *) dst, mm_avg);		// store line 5
	dst += MB_BLOCK_STRIDE;
	mm_src2 = _mm_loadu_si128((__m128i *) src);		// one uv pixel down
	mm_src3 = _mm_loadu_si128((__m128i *) (src + 2));	// one uv pixel over
	SUM_LINES_16(mm_src2,mm_src3,mm_sum1_lo,mm_sum1_hi);
	src += stride;
	AVERAGE_4(mm_sum0_lo,mm_sum0_hi,mm_sum1_lo,mm_sum1_hi,mm_avg);		// average horizontal and vertical
	_mm_store_si128((__m128i *) dst, mm_avg);		// store line 6
	dst += MB_BLOCK_STRIDE;

	mm_src1 = _mm_loadu_si128((__m128i *) src);
	mm_src3 = _mm_loadu_si128((__m128i *) (src + 2));	// one uv pixel over
	SUM_LINES_16(mm_src1,mm_src3,mm_sum0_lo,mm_sum0_hi);
	AVERAGE_4(mm_sum1_lo,mm_sum1_hi,mm_sum0_lo,mm_sum0_hi,mm_avg);		// average horizontal and vertical
	_mm_store_si128((__m128i *) dst, mm_avg);		// store line 7

#if 0	// verify
	uint8_t value;

	src -= (stride * 8);
	dst -= (MB_BLOCK_STRIDE * 7);

	for (int j = 0; j < 8; j++, dst += MB_BLOCK_STRIDE, src += stride)
	{
		for (int i = 0; i < 16; i++)
		{
			value = (src[i] + src[i+2] + src[stride+i] + src[stride+i+2] + 2) >> 2;
			ENC_ASSERT(dst[i] == value);
		}
	}
#endif

#endif
}

void (*ME_Prediction_Pixels) (MacroBlockType *mb, int flags);

void ME_Prediction(MacroBlockType *mb, int flags)
{
	ME_Prediction_Pixels(mb, flags);
}

void ME_Prediction_Normal (MacroBlockType *mb, int flags)
{
	// get motion vector
	int mvx;
	int mvy;

	// copy pixels from reference frame
	YUVFrameType *ref_frame;
	uint8_t *Y_reference_frame;
	u8_16x16_Type *luma_pixels;
	Chroma_8x8_Type *chroma_pixels;

	if (flags & ME_PRED_SKIP)	// skip
	{
		luma_pixels = (u8_16x16_Type *) mb->skip_dst_y_buffer;
		chroma_pixels = (Chroma_8x8_Type *) mb->skip_dst_uv_buffer;
	}
	else
	{
		luma_pixels = mb->Y_pred_pixels;
		chroma_pixels = mb->UV_pred_pixels;
	}

	if (!(flags & ME_PRED_DIR_FORWARD))	// back
	{	// back
		mvx = mb->data->mv_l0[0][0][0];
		mvy = mb->data->mv_l0[1][0][0];

		ref_frame = gEncoderState.current_frame->prev_ref_frame;
		Y_reference_frame = ref_frame->Y_frame_reconstructed;
	}
	else
	{	// forward
		mvx = mb->data->mv_l1[0][0][0];
		mvy = mb->data->mv_l1[1][0][0];

		ref_frame = gEncoderState.current_frame->next_ref_frame;
		Y_reference_frame = ref_frame->Y_frame_reconstructed;
	}

	int full_mvx = mvx / 4; // FOR NOW: assume same mv for whole 16x16 block
	int full_mvy = mvy / 4; // FOR NOW: assume same mv for whole 16x16 block

	// get absolute position that mv points to
	int luma_x = (mb->mb_x << 4) + full_mvx;
	int luma_y = (mb->mb_y << 4) + full_mvy;

	int chroma_x = luma_x >> 1;
	int chroma_y = luma_y >> 1;

	assert( luma_y >= 0 );
	assert( luma_x >= 0 );

	int Y_start_index = luma_y * ref_frame->stride_padded + luma_x;
	int idx = Y_start_index;
	for( int j = 0; j < 16; ++j, idx += ref_frame->stride_padded )
	{
		memcpy(&luma_pixels->block[j], &Y_reference_frame[idx], 16);
	}

	// find chroma sub-pixel type
	int sp_type = ((full_mvy & 1) << 1) | (full_mvx & 1);	
	int UV_start_index = chroma_y * ref_frame->uv_width_padded + chroma_x;

	switch(sp_type)
	{
	case 0:	// no sub-pixel
		idx = UV_start_index;
		for( int k = 0; k < 8; ++k, idx += ref_frame->uv_width_padded )
		{
			memcpy(&chroma_pixels->block[k], &ref_frame->UV_frame_reconstructed[idx], 16);
		}
		break;
	case 1: // horizontal sub-pixel
		CalcChromaHalfPixel_H((uint8_t *) chroma_pixels, (uint8_t *) &ref_frame->UV_frame_reconstructed[UV_start_index], ref_frame->uv_stride_padded );
		break;
	case 2:	// vertical sub-pixel
		CalcChromaHalfPixel_V((uint8_t *) chroma_pixels, (uint8_t *) &ref_frame->UV_frame_reconstructed[UV_start_index], ref_frame->uv_stride_padded );
		break;
	case 3:	// horizontal & vertical sub-pixel
		CalcChromaHalfPixel_D((uint8_t *) chroma_pixels, (uint8_t *) &ref_frame->UV_frame_reconstructed[UV_start_index], ref_frame->uv_stride_padded );
		break;

	}
}

void ME_Prediction_HalfPixel(MacroBlockType *mb, int flags)
{
    // get motion vector
    int mvx;
    int mvy;

    // copy pixels from reference frame
    YUVFrameType *ref_frame;
    uint8_t *Y_reference_frame;
	u8_16x16_Type *luma_pixels;
	Chroma_8x8_Type *chroma_pixels;

	if (flags & ME_PRED_SKIP)	// skip
	{
		luma_pixels = (u8_16x16_Type *) mb->skip_dst_y_buffer;
		chroma_pixels = (Chroma_8x8_Type *) mb->skip_dst_uv_buffer;
	}
	else
	{
		luma_pixels = mb->Y_pred_pixels;
		chroma_pixels = mb->UV_pred_pixels;
	}

	if (!(flags & ME_PRED_DIR_FORWARD))	// back
	{	// back
		mvx = mb->data->mv_l0[0][0][0];
		mvy = mb->data->mv_l0[1][0][0];

		ref_frame = gEncoderState.current_frame->prev_ref_frame;
		Y_reference_frame = ref_frame->Y_frame_reconstructed;
	}
	else
	{	// forward
		mvx = mb->data->mv_l1[0][0][0];
		mvy = mb->data->mv_l1[1][0][0];

		ref_frame = gEncoderState.current_frame->next_ref_frame;
		Y_reference_frame = ref_frame->Y_frame_reconstructed;
	}

	int full_mvx = mvx / 4; // FOR NOW: assume same mv for whole 16x16 block
	int full_mvy = mvy / 4; // FOR NOW: assume same mv for whole 16x16 block

    int quarter_mvx = 0, quarter_mvy = 0;

    int half_mvx = (mvx - quarter_mvx) & 0x3;
    int half_mvy = (mvy - quarter_mvy) & 0x3;
    if( mvx < 0 )
        half_mvx = -half_mvx;
    if( mvy < 0 )
        half_mvy = -half_mvy;


    int luma_x_offset = 0;
    int luma_y_offset = 0;
    if( half_mvx < 0 )
        luma_x_offset = -1;
    if( half_mvy < 0 )
        luma_y_offset = -1;

    // get absolute position that mv points to
    int luma_x = (mb->mb_x << 4) + full_mvx;
    int luma_y = (mb->mb_y << 4) + full_mvy;

    luma_x += luma_x_offset;
    luma_y += luma_y_offset;

    int chroma_x = luma_x >> 1;
    int chroma_y = luma_y >> 1;

    assert( luma_y >= 0 );
    assert( luma_x >= 0 );

    // choose reference frame
	// select base luma & chroma buffers based on mv
	int selector = (half_mvy & 2) | ((half_mvx & 0x2) >> 1);	// so 0 - base, 1 - half-pixel horizontal, 2 - half-pixel vertical, 3 - half-pixel diagonal

    Y_reference_frame = ref_frame->luma_buffers[selector];

    int Y_start_index = luma_y * ref_frame->stride_padded + luma_x;
	int idx = Y_start_index;
	for( int j = 0; j < 16; ++j, idx += ref_frame->stride_padded )
	{
        memcpy(&luma_pixels->block[j], &Y_reference_frame[idx], 16);
    }

    // compute dx and dy for chroma eighth pixel
    int dx = 0, dy = 0;
    if( full_mvx & 1 )
        dx += 4;
    if( full_mvy & 1 )
        dy += 4;

    dx += half_mvx;
    dy += half_mvy;
    if( dx < 0 ) dx += 8; // eighth pixel
    if( dy < 0 ) dy += 8; // eighth pixel

	int weight00 = (8-dx) * (8-dy);
	int weight10 = dx * (8-dy);
	int weight01 = dy * (8-dx);
	int weight11 = dx * dy;

    int UV_start_index = chroma_y * ref_frame->uv_width_padded + chroma_x;

    CHROMA_TYPE chroma_type = CHROMA_SUB;
    if( (dx == 0) && (dy == 0) ) chroma_type = CHROMA_FULL;
    else if( (dx == 0) && (dy != 0) ) chroma_type = CHROMA_FULL_X;
    else if( (dx != 0) && (dy == 0) ) chroma_type = CHROMA_FULL_Y;

    if( chroma_type == CHROMA_FULL ) // avoids access violation at right or bottom edge
    {
		idx = UV_start_index;
		for( int k = 0; k < 8; ++k, idx += ref_frame->uv_width_padded )
		{
			memcpy(&chroma_pixels->block[k], &ref_frame->UV_frame_reconstructed[idx], 16);
		}
    }
#if 1
	else
	{
		int weights[4] = { weight00, weight10, weight01, weight11 };
		CalcChromaHalfPixel_Weighted((uint8_t*) &chroma_pixels->block[0], (uint8_t*) &ref_frame->UV_frame_reconstructed[UV_start_index], ref_frame->uv_stride_padded, weights );
	}
#else
    else 
	if( chroma_type == CHROMA_FULL_X )
    {
        for( int k = 0; k < 8; ++k )
        {
            int idx = k * ref_frame->uv_stride_padded + UV_start_index;
            ChromaPixelType *p = &ref_frame->UV_frame_reconstructed[idx];

            for( int i = 0; i < 8; i++, p++ )
            {
                int u = p->u * weight00 +
                    (p+ref_frame->uv_stride_padded)->u * weight01;
                chroma_pixels->block[k][i].u = rshift_rnd_sf(u,6);

                int v = p->v * weight00 +
                    (p+ref_frame->uv_stride_padded)->v * weight01;
                chroma_pixels->block[k][i].v = rshift_rnd_sf(v,6);
            }
        }
    }
    else if( chroma_type == CHROMA_FULL_Y )
    {
        for( int k = 0; k < 8; ++k )
        {
            int idx = k * ref_frame->uv_stride_padded + UV_start_index;
            ChromaPixelType *p = &ref_frame->UV_frame_reconstructed[idx];

            for( int i = 0; i < 8; i++, p++ )
            {
                int u = p->u * weight00 +
                    (p+1)->u * weight10;
                chroma_pixels->block[k][i].u = rshift_rnd_sf(u,6);

                int v = p->v * weight00 +
                    (p+1)->v * weight10;
                chroma_pixels->block[k][i].v = rshift_rnd_sf(v,6);
            }
        }
    }
    else // chroma_type == CHROMA_SUB
    {
        for( int k = 0; k < 8; ++k )
        {
            int idx = k * ref_frame->uv_stride_padded + UV_start_index;
            ChromaPixelType *p = &ref_frame->UV_frame_reconstructed[idx];

            for( int i = 0; i < 8; i++, p++ )
            {
                int u = p->u * weight00 +
                    (p+1)->u * weight10 +
                    (p+ref_frame->uv_stride)->u * weight01 +
                    (p+ref_frame->uv_stride+1)->u * weight11;
                chroma_pixels->block[k][i].u = rshift_rnd_sf(u,6);

                int v = p->v * weight00 +
                    (p+1)->v * weight10 +
                    (p+ref_frame->uv_stride)->v * weight01 +
                    (p+ref_frame->uv_stride+1)->v * weight11;
                chroma_pixels->block[k][i].v = rshift_rnd_sf(v,6);
            }
        }
    }
#endif
}

void ME_Prediction_QuarterPixel(MacroBlockType *mb, int flags)
{
	// get motion vector
	int mvx;
	int mvy;

	// copy pixels from reference frame
	YUVFrameType *ref_frame;
	uint8_t *Y_reference_frame;
	u8_16x16_Type *luma_pixels;
	Chroma_8x8_Type *chroma_pixels;

	if (flags & ME_PRED_SKIP)	// skip
	{
		luma_pixels = (u8_16x16_Type *) mb->skip_dst_y_buffer;
		chroma_pixels = (Chroma_8x8_Type *) mb->skip_dst_uv_buffer;
	}
	else
	{
		luma_pixels = mb->Y_pred_pixels;
		chroma_pixels = mb->UV_pred_pixels;
	}

	if (!(flags & ME_PRED_DIR_FORWARD))	// back
	{	// back
		mvx = mb->data->mv_l0[0][0][0];
		mvy = mb->data->mv_l0[1][0][0];

		ref_frame = gEncoderState.current_frame->prev_ref_frame;
		Y_reference_frame = ref_frame->Y_frame_reconstructed;
	}
	else
	{	// forward
		mvx = mb->data->mv_l1[0][0][0];
		mvy = mb->data->mv_l1[1][0][0];

		ref_frame = gEncoderState.current_frame->next_ref_frame;
		Y_reference_frame = ref_frame->Y_frame_reconstructed;
	}


	int full_mvx = mvx / 4; // FOR NOW: assume same mv for whole 16x16 block
	int full_mvy = mvy / 4; // FOR NOW: assume same mv for whole 16x16 block

	int quarter_mvx = 0, quarter_mvy = 0;

	quarter_mvx = mvx & 0x1;
	quarter_mvy = mvy & 0x1;
	if( mvx < 0 )
		quarter_mvx = -quarter_mvx;
	if( mvy < 0 )
		quarter_mvy = -quarter_mvy;

	bool hasQuarterPixel = (quarter_mvx != 0) || (quarter_mvy != 0);

	int half_mvx = (mvx - quarter_mvx) & 0x3;
	int half_mvy = (mvy - quarter_mvy) & 0x3;
	if( mvx < 0 )
		half_mvx = -half_mvx;
	if( mvy < 0 )
		half_mvy = -half_mvy;


	int luma_x_offset = 0;
	int luma_y_offset = 0;
	if( half_mvx < 0 )
		luma_x_offset = -1;
	if( half_mvy < 0 )
		luma_y_offset = -1;

	// get absolute position that mv points to
	int luma_x = (mb->mb_x << 4) + full_mvx;
	int luma_y = (mb->mb_y << 4) + full_mvy;

	luma_x += luma_x_offset;
	luma_y += luma_y_offset;

	int chroma_x = luma_x >> 1;
	int chroma_y = luma_y >> 1;

	assert( luma_y >= 0 );
	assert( luma_x >= 0 );

	if( hasQuarterPixel ) // copy over saved quarter pixels
	{
		int idx = 0;
		for( int j = 0; j < 16; ++j, idx += ref_frame->stride_padded )
		{
			memcpy(&luma_pixels->block[j], &mb->Y_quarter_buffer[idx], 16);
		}
	}
	else
	{
		// choose reference frame
		// select base luma & chroma buffers based on mv
		int selector = (half_mvy & 2) | ((half_mvx & 0x2) >> 1);	// so 0 - base, 1 - half-pixel horizontal, 2 - half-pixel vertical, 3 - half-pixel diagonal

		Y_reference_frame = ref_frame->luma_buffers[selector];

		int Y_start_index = luma_y * ref_frame->stride_padded + luma_x;
		for( int j = 0; j < 16; ++j )
		{
			int idx = j * ref_frame->stride_padded + Y_start_index;
			memcpy(&luma_pixels->block[j], &Y_reference_frame[idx], 16);
		}
	}

	// compute dx and dy for chroma eighth pixel
	int dx = 0, dy = 0;
	if( full_mvx & 1 )
		dx += 4;
	if( full_mvy & 1 )
		dy += 4;

	dx += half_mvx;
	dy += half_mvy;
	dx += quarter_mvx;
	dy += quarter_mvy;

	if( dx < 0 ) dx += 8; // eighth pixel
	if( dy < 0 ) dy += 8; // eighth pixel

	int weight00 = (8-dx) * (8-dy);
	int weight10 = dx * (8-dy);
	int weight01 = dy * (8-dx);
	int weight11 = dx * dy;

	int UV_start_index = chroma_y * ref_frame->uv_stride_padded + chroma_x;

	CHROMA_TYPE chroma_type = CHROMA_SUB;
	if( (dx == 0) && (dy == 0) ) chroma_type = CHROMA_FULL;
	else if( (dx == 0) && (dy != 0) ) chroma_type = CHROMA_FULL_X;
	else if( (dx != 0) && (dy == 0) ) chroma_type = CHROMA_FULL_Y;

	if( chroma_type == CHROMA_FULL ) // avoids access violation at right or bottom edge
	{
		for( int k = 0; k < 8; ++k )
		{
			int idx = k * ref_frame->uv_stride_padded + UV_start_index;
			ChromaPixelType *p = &ref_frame->UV_frame_reconstructed[idx];

			for( int i = 0; i < 8; i++, p++ )
			{
				chroma_pixels->block[k][i].u = p->u;
				chroma_pixels->block[k][i].v = p->v;
			}
		}
	}
	else if( chroma_type == CHROMA_FULL_X )
	{
		for( int k = 0; k < 8; ++k )
		{
			int idx = k * ref_frame->uv_stride_padded + UV_start_index;
			ChromaPixelType *p = &ref_frame->UV_frame_reconstructed[idx];

			for( int i = 0; i < 8; i++, p++ )
			{
				int u = p->u * weight00 +
					(p+ref_frame->uv_stride_padded)->u * weight01;
				chroma_pixels->block[k][i].u = static_cast<uint8_t>(rshift_rnd_sf(u,6));

				int v = p->v * weight00 +
					(p+ref_frame->uv_stride_padded)->v * weight01;
				chroma_pixels->block[k][i].v = static_cast<uint8_t>(rshift_rnd_sf(v,6));
			}
		}
	}
	else if( chroma_type == CHROMA_FULL_Y )
	{
		for( int k = 0; k < 8; ++k )
		{
			int idx = k * ref_frame->uv_stride_padded + UV_start_index;
			ChromaPixelType *p = &ref_frame->UV_frame_reconstructed[idx];

			for( int i = 0; i < 8; i++, p++ )
			{
				int u = p->u * weight00 +
					(p+1)->u * weight10;
				chroma_pixels->block[k][i].u = static_cast<uint8_t>(rshift_rnd_sf(u,6));

				int v = p->v * weight00 +
					(p+1)->v * weight10;
				chroma_pixels->block[k][i].v = static_cast<uint8_t>(rshift_rnd_sf(v,6));
			}
		}
	}
	else // chroma_type == CHROMA_SUB
	{
		for( int k = 0; k < 8; ++k )
		{
			int idx = k * ref_frame->uv_stride_padded + UV_start_index;
			ChromaPixelType *p = &ref_frame->UV_frame_reconstructed[idx];

			for( int i = 0; i < 8; i++, p++ )
			{
				int u = p->u * weight00 +
					(p+1)->u * weight10 +
					(p+ref_frame->uv_stride_padded)->u * weight01 +
					(p+ref_frame->uv_stride_padded+1)->u * weight11;
				chroma_pixels->block[k][i].u = static_cast<uint8_t>(rshift_rnd_sf(u,6));

				int v = p->v * weight00 +
					(p+1)->v * weight10 +
					(p+ref_frame->uv_stride_padded)->v * weight01 +
					(p+ref_frame->uv_stride_padded+1)->v * weight11;
				chroma_pixels->block[k][i].v = static_cast<uint8_t>(rshift_rnd_sf(v,6));
			}
		}
	}
}

typedef enum
{
    SDIAMOND  = 0,
    SQUARE    = 1,
    EDIAMOND  = 2,
    LDIAMOND  = 3,
    SBDIAMOND = 4
} SearchPatternType;

//! Define EPZS Refinement patterns
static const short pattern_data[7][12][4] =
{
    { // Small Diamond pattern
        //{  0,  4,  3, 3 }, {  4,  0,  0, 3 }, {  0, -4,  1, 3 }, { -4,  0, 2, 3 }
        {  0,  1,  3, 3 }, {  1,  0,  0, 3 }, {  0, -1,  1, 3 }, { -1,  0, 2, 3 } // assume integer pixel (vs. subpixel) grid
    },
    { // Square pattern
        {  0,  4,  7, 3 }, {  4,  4,  7, 5 }, {  4,  0,  1, 3 }, {  4, -4, 1, 5 },
        {  0, -4,  3, 3 }, { -4, -4,  3, 5 }, { -4,  0,  5, 3 }, { -4,  4, 5, 5 }
    },
    { // Enhanced Diamond pattern
        { -4,  4, 10, 5 }, {  0,  8, 10, 8 }, {  0,  4, 10, 7 }, {  4,  4, 1, 5 },
        {  8,  0, 1,  8 }, {  4,  0,  1, 7 }, {  4, -4,  4, 5 }, {  0, -8, 4, 8 },
        {  0, -4, 4,  7 }, { -4, -4,  7, 5 }, { -8,  0,  7, 8 }, { -4,  0, 7, 7 }

    },
    { // Large Diamond pattern
        {  0,  8, 6,  5 }, {  4,  4, 0,  3 }, {  8,  0, 0,  5 }, {  4, -4, 2, 3 },
        {  0, -8, 2,  5 }, { -4, -4, 4,  3 }, { -8,  0, 4,  5 }, { -4,  4, 6, 3 }
    },
    { // Extended Subpixel pattern
        {  0,  8, 6, 12 }, {  4,  4, 0, 12 }, {  8,  0, 0, 12 }, {  4, -4, 2, 12 },
        {  0, -8, 2, 12 }, { -4, -4, 4, 12 }, { -8,  0, 4, 12 }, { -4,  4, 6, 12 },
        {  0,  2, 6, 12 }, {  2,  0, 0, 12 }, {  0, -2, 2, 12 }, { -2,  0, 4, 12 }
    }
};


// FOR NOW: small diamond
void ME_InitSearchPatterns()
{
    // small diamond pattern
    smalldiamond.searchPoints = 4;
    smalldiamond.points = (SearchPoint *)calloc( smalldiamond.searchPoints, sizeof(SearchPoint) );
    assert( smalldiamond.points );

    const int type = SDIAMOND;
    for( int i = 0; i < smalldiamond.searchPoints; ++i )
    {
        smalldiamond.points[i].motion.mv_x = pattern_data[type][i][0];
        smalldiamond.points[i].motion.mv_y = pattern_data[type][i][1];
        smalldiamond.points[i].startIndex = pattern_data[type][i][2];
        smalldiamond.points[i].numberOfPoints = pattern_data[type][i][3];
    }

    smalldiamond.stopSearch = true;
    smalldiamond.nextLast = true;
    smalldiamond.nextpattern = &smalldiamond;
}


static bool ME_Initted = false;

void ME_InitMotionEstimation(int type)
{
    if( !ME_Initted )
    {
        ME_Initted = true;

		if (type < 0 || type > 2)
			type = 0;

		switch(type)
		{
		case 0:
			ME_Prediction_Pixels = ME_Prediction_Normal;
			break;
		case 1:
			ME_Prediction_Pixels = ME_Prediction_HalfPixel;
			break;
		case 2:
			ME_Prediction_Pixels = ME_Prediction_QuarterPixel;
			break;
		}

        ME_InitSearchPatterns();
        MI_Init();

		// setup sMV_BitCost table
		int max_mv_bits = 21;	// based on max subpel = 263
		sMV_BitCost = &sMV_BitCostTable[MV_MAX-1];

		sMV_BitCost[0] = 1;
		for (int bits = 3; bits <= max_mv_bits; bits += 2)
		{
			int i_max = (short) (1 << (bits >> 1));
			int i_min = i_max >> 1;

			for (int i = i_min; i < i_max; i++)
				sMV_BitCost[-i] = sMV_BitCost[i] = bits;
		}

#define SHIFT_QP 12
#define  LAMBDA_ACCURACY_BITS         5
#define  LAMBDA_FACTOR(lambda)        ((int)((double)(1 << LAMBDA_ACCURACY_BITS) * lambda + 0.5))

		// Initialize lambda factor table
		for (int i = 0; i < 3; i++)
		{
			double qp_temp;
			double lambda_md;
			double lambda_me = 0;
			double lambda_scale;

			for (int mod = 0; mod < 2; mod++)
			{
				for (int qp = 0; qp <= MAX_QP; qp++)
				{
					switch(i)
					{
					case 0:	// I slice
						lambda_scale = 1.0 - CLIP3(0.0,0.5,0.05 * (double) gEncoderState.b_slice_frames);

						qp_temp = (double)qp - SHIFT_QP;

						if (mod == 1)	// for qp == base
						{
							lambda_md = 0.57 * pow (2.0, qp_temp/3.0); 
						}
						else
						if (gEncoderState.b_slice_frames > 0)
						{
							lambda_md = 0.68 * pow (2.0, qp_temp/3.0);
						}
						else
							lambda_md = 0.85 * pow (2.0, qp_temp/3.0);

						// if we use satd for me diff, modify this
						lambda_md = 0.95 * lambda_md * lambda_scale;

						lambda_me = sqrt(lambda_md);

						break;
					case 1:	// P slice
						lambda_scale = 1.0 - CLIP3(0.0,0.5,0.05 * (double) gEncoderState.b_slice_frames);

						qp_temp = (double)qp - SHIFT_QP;

						if (gEncoderState.b_slice_frames > 0)
						{
							lambda_md = 0.68 * pow (2.0, qp_temp/3.0);
						}
						else
							lambda_md = 0.85 * pow (2.0, qp_temp/3.0);

						// if we use satd for me diff, modify this
						lambda_md = 0.95 * lambda_md * lambda_scale;

						lambda_me = sqrt(lambda_md);

						break;
					case 2:	// B slice
						qp_temp = (double)qp - SHIFT_QP;

						if (gEncoderState.b_slice_frames > 0)
						{
							lambda_md = 0.68 * pow (2.0, qp_temp/3.0) * CLIP3(2.00, 4.00, (qp_temp / 6.0));
						}
						else
							lambda_md = 0.85 * pow (2.0, qp_temp/3.0);

						// if we use satd for me diff, modify this
						lambda_md = 0.95 * lambda_md;

						// for idc ref frames - 
						if (mod != 0)
							lambda_md *= 0.80;

						lambda_me = sqrt(lambda_md);

						break;
					}

					for (int pel_type = 0; pel_type < 3; pel_type++)
					{
						sLambdaTable[i][mod][qp][pel_type] = LAMBDA_FACTOR(lambda_me);
					}
				}
			}
		}

		ME_InitSubPixelImages();

		// allocate temporal mv ref table
		for (int i = 0; i < 2; i++)
		{
			gTemporalMV_Ref[i] = (TemporalMV_Type *) _aligned_malloc(sizeof(TemporalMV_Type) * gEncoderState.mb_max, 16);
			for (int m = 0; m < gEncoderState.mb_max; m++)
			{
				gTemporalMV_Ref[i][m].mx = 0;
				gTemporalMV_Ref[i][m].my = 0;
			}
		}
    }
}

void ME_ExitMotionEstimation()
{
	if (ME_Initted)
	{
		for (int i = 0; i < 2; i++)
		{
			if (gTemporalMV_Ref[i])
				_aligned_free(gTemporalMV_Ref[i]);
			gTemporalMV_Ref[i] = NULL;
		}

		ME_ExitSubPixelImages();
		ME_Initted = false;
	}
}

uint8_t * sChromaQuarterPixelImage_H;

static const int ONE_FOURTH_TAP[3] =
{
    20, -5, 1  // AVC Interpolation taps
};

void ME_InitSubPixelImages()
{
	gLumaHalfPixelSetIndex = 0;

	if (gEncoderState.subpixel_motion_est > 0)
	{
		for (int i = 0; i < MAX_HALF_PIXEL_REF_SETS; i++)
		{
			gLumaHalfPixelImages[i][0] = (uint8_t *) _aligned_malloc (gEncoderState.width_padded * gEncoderState.height_padded, 32 );
			gLumaHalfPixelImages[i][1] = (uint8_t *) _aligned_malloc (gEncoderState.width_padded * gEncoderState.height_padded, 32 );
			gLumaHalfPixelImages[i][2] = (uint8_t *) _aligned_malloc (gEncoderState.width_padded * gEncoderState.height_padded, 32 );
		}

		// point the buffer pointers to inside the padded area
		gLumaHalfPixelImage_H = gLumaHalfPixelImages[0][0] + (gEncoderState.width_padded * FRAME_HEIGHT_PAD) + FRAME_WIDTH_PAD;
		gLumaHalfPixelImage_V = gLumaHalfPixelImages[0][1] + (gEncoderState.width_padded * FRAME_HEIGHT_PAD) + FRAME_WIDTH_PAD;
		gLumaHalfPixelImage_D = gLumaHalfPixelImages[0][2] + (gEncoderState.width_padded * FRAME_HEIGHT_PAD) + FRAME_WIDTH_PAD;

		sLumaHalfPixelImage_Tmp = (int16_t *) _aligned_malloc (gEncoderState.width_padded * gEncoderState.height_padded * sizeof(int16_t), 32 );
	}
	else
	{
		gLumaHalfPixelImage_H = NULL;
		gLumaHalfPixelImage_V = NULL;
		gLumaHalfPixelImage_D = NULL;

		sLumaHalfPixelImage_Tmp = NULL;
	}
}

void ME_ExitSubPixelImages()
{
	for (int i = 0; i < MAX_HALF_PIXEL_REF_SETS; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (gLumaHalfPixelImages[i][j])
				_aligned_free( gLumaHalfPixelImages[i][j] );
		}
	}

	if (sLumaHalfPixelImage_Tmp)
	{
		_aligned_free(sLumaHalfPixelImage_Tmp);
		sLumaHalfPixelImage_Tmp = NULL;
	}
}

void SetHalfPixelFrameRef(YUVFrameType *frame)
{
	// set lambda table
	switch(frame->slice_type)
	{
	case SLICE_TYPE_P:
		frame->lambda_table = (int *) sLambdaTable[1][0];
		break;
	case SLICE_TYPE_B:
		if (frame->nal_ref_idc != 0)
			frame->lambda_table = (int *) sLambdaTable[2][1];
		else
			frame->lambda_table = (int *) sLambdaTable[2][0];
		break;
	default:
		frame->lambda_table = (int *) sLambdaTable[0][0];
		break;
	}

	if (frame->nal_ref_idc != 0)	// if this is a reference frame
	{
		frame->luma_buffers[1] = gLumaHalfPixelImages[gLumaHalfPixelSetIndex][0] + (gEncoderState.width_padded * FRAME_HEIGHT_PAD) + FRAME_WIDTH_PAD;
		frame->luma_buffers[2] = gLumaHalfPixelImages[gLumaHalfPixelSetIndex][1] + (gEncoderState.width_padded * FRAME_HEIGHT_PAD) + FRAME_WIDTH_PAD;
		frame->luma_buffers[3] = gLumaHalfPixelImages[gLumaHalfPixelSetIndex][2] + (gEncoderState.width_padded * FRAME_HEIGHT_PAD) + FRAME_WIDTH_PAD;
		gLumaHalfPixelSetIndex = (gLumaHalfPixelSetIndex + 1) % MAX_HALF_PIXEL_REF_SETS;
	}
	else
	{
		frame->luma_buffers[1] = NULL;
		frame->luma_buffers[2] = NULL;
		frame->luma_buffers[3] = NULL;
	}
}

#define VERIFY_HALF_PIXEL_OPT 

void ComputeSubImageSixTap_D(uint8_t *lumaHalfPixelImage_D)
{
    const int tap0 = ONE_FOURTH_TAP[0];
    const int tap1 = ONE_FOURTH_TAP[1];
    const int tap2 = ONE_FOURTH_TAP[2];

    int i, j;
    int is;
    uint8_t *wxLineDst;
    int16_t *srcImgA, *srcImgB, *srcImgC, *srcImgD, *srcImgE, *srcImgF;

    int y_start_idx = 0;

	uint8_t pixel;

    // branches within the j loop
    // top
    for (j = 0; j < 2; ++j, y_start_idx += gEncoderState.width)
    {
        int16_t * srcImg = &sLumaHalfPixelImage_Tmp[y_start_idx];

        // ---> increasing y
        // C B A _ D E F
        wxLineDst = &lumaHalfPixelImage_D[y_start_idx];
        srcImgA = srcImg;
        srcImgB = (j == 0) ? &srcImg[0] : (srcImg - gEncoderState.width);
        srcImgC = srcImgB;
        srcImgD = srcImg + gEncoderState.width;
        srcImgE = srcImg + (gEncoderState.width * 2);
        srcImgF = srcImg + (gEncoderState.width * 3);
        for (i = 0; i < gEncoderState.width; ++i)
        {
            is =
                (tap0 * (*srcImgA++ + *srcImgD++) +
                tap1 *  (*srcImgB++ + *srcImgE++) +
                tap2 *  (*srcImgC++ + *srcImgF++));
#ifdef VERIFY_HALF_PIXEL_OPT
			pixel = static_cast<uint8_t>(CLIP1( rshift_rnd_sf( is, 10 ) ));
			ENC_ASSERT(wxLineDst[i] == pixel);
#else
            wxLineDst[i] = static_cast<uint8_t>(CLIP1( rshift_rnd_sf( is, 10 ) ));
#endif
        }
    }
    // center
    for (j = 2; j < gEncoderState.height - 3; ++j, y_start_idx += gEncoderState.width)
    {
        int16_t * srcImg = &sLumaHalfPixelImage_Tmp[y_start_idx];

        wxLineDst = &lumaHalfPixelImage_D[y_start_idx];
        srcImgA = srcImg;
        srcImgB = srcImg - gEncoderState.width;      
        srcImgC = srcImg - (gEncoderState.width * 2);
        srcImgD = srcImg + gEncoderState.width;
        srcImgE = srcImg + (gEncoderState.width * 2);
        srcImgF = srcImg + (gEncoderState.width * 3);
        for (i = 0; i < gEncoderState.width; ++i)
        {
            is =
                (tap0 * (*srcImgA++ + *srcImgD++) +
                tap1 *  (*srcImgB++ + *srcImgE++) +
                tap2 *  (*srcImgC++ + *srcImgF++));

#ifdef VERIFY_HALF_PIXEL_OPT
			pixel = static_cast<uint8_t>(CLIP1( rshift_rnd_sf( is, 10 ) ));
			ENC_ASSERT(wxLineDst[i] == pixel);
#else
			wxLineDst[i] = static_cast<uint8_t>(CLIP1( rshift_rnd_sf( is, 10 ) ));
#endif
        }
    }

    // bottom
    int16_t * srcImg = &sLumaHalfPixelImage_Tmp[y_start_idx];
    int16_t * srcImgMax = srcImg + (gEncoderState.width * 2);
    for (j = gEncoderState.height - 3; j < gEncoderState.height; ++j, y_start_idx += gEncoderState.width)
    {
        int16_t *srcImg = &sLumaHalfPixelImage_Tmp[y_start_idx];

        wxLineDst = &lumaHalfPixelImage_D[y_start_idx];
        srcImgA = srcImg;
        srcImgB = srcImg - gEncoderState.width;      
        srcImgC = srcImg - (gEncoderState.width * 2);
        srcImgD = srcImg + gEncoderState.width;
        if( srcImgD > srcImgMax )
            srcImgD = srcImgMax;
        srcImgE = srcImg + (gEncoderState.width * 2);
        if( srcImgE > srcImgMax )
            srcImgE = srcImgMax;
        srcImgF = srcImgMax;

        for (i = 0; i < gEncoderState.width; ++i)
        {
            is =
                (tap0 * (*srcImgA++ + *srcImgD++) +
                tap1 *  (*srcImgB++ + *srcImgE++) +
                tap2 *  (*srcImgC++ + *srcImgF++));

#ifdef VERIFY_HALF_PIXEL_OPT
			pixel = static_cast<uint8_t>(CLIP1( rshift_rnd_sf( is, 10 ) ));
			ENC_ASSERT(wxLineDst[i] == pixel);
#else
			wxLineDst[i] = static_cast<uint8_t>(CLIP1( rshift_rnd_sf( is, 10 ) ));
#endif
        }
    }
}

void ComputeSubImageSixTap_V(uint8_t *lumaHalfPixelImage_V, YUVFrameType *frame)
{
    const int tap0 = ONE_FOURTH_TAP[0];
    const int tap1 = ONE_FOURTH_TAP[1];
    const int tap2 = ONE_FOURTH_TAP[2];

    int i, j;
    int is;
    uint8_t *wxLineDst;
    uint8_t *srcImgA, *srcImgB, *srcImgC, *srcImgD, *srcImgE, *srcImgF;

    int y_start_idx = 0;
	uint8_t pixel;

    // branches within the j loop
    // top
    for (j = 0; j < 2; ++j, y_start_idx += gEncoderState.width)
    {
        uint8_t * srcImg = &frame->Y_frame_reconstructed[y_start_idx];

        // ---> increasing y
        // C B A _ D E F
        wxLineDst = &lumaHalfPixelImage_V[y_start_idx];
        srcImgA = srcImg;
        srcImgB = (j == 0) ? &srcImg[0] : (srcImg - gEncoderState.width);
        srcImgC = srcImgB;
        srcImgD = srcImg + gEncoderState.width;
        srcImgE = srcImg + (gEncoderState.width * 2);
        srcImgF = srcImg + (gEncoderState.width * 3);
        for (i = 0; i < gEncoderState.width; ++i)
        {
            is =
                (tap0 * (*srcImgA++ + *srcImgD++) +
                tap1 *  (*srcImgB++ + *srcImgE++) +
                tap2 *  (*srcImgC++ + *srcImgF++));

#ifdef VERIFY_HALF_PIXEL_OPT
			pixel = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
			ENC_ASSERT(wxLineDst[i] == pixel);
#else
            wxLineDst[i] = static_cast<uint8_t>(CLIP1( rshift_rnd_sf( is, 5 ) ));
#endif
        }
    }
    // center
    for (j = 2; j < gEncoderState.height - 3; ++j, y_start_idx += gEncoderState.width)
    {
        uint8_t * srcImg = &frame->Y_frame_reconstructed[y_start_idx];

        wxLineDst = &lumaHalfPixelImage_V[y_start_idx];
        srcImgA = srcImg;
        srcImgB = srcImg - gEncoderState.width;      
        srcImgC = srcImg - (gEncoderState.width * 2);
        srcImgD = srcImg + gEncoderState.width;
        srcImgE = srcImg + (gEncoderState.width * 2);
        srcImgF = srcImg + (gEncoderState.width * 3);
        for (i = 0; i < gEncoderState.width; ++i)
        {
            is =
                (tap0 * (*srcImgA++ + *srcImgD++) +
                tap1 *  (*srcImgB++ + *srcImgE++) +
                tap2 *  (*srcImgC++ + *srcImgF++));

#ifdef VERIFY_HALF_PIXEL_OPT
			pixel = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
			ENC_ASSERT(wxLineDst[i] == pixel);
#else
			wxLineDst[i] = static_cast<uint8_t>(CLIP1( rshift_rnd_sf( is, 5 ) ));
#endif
        }
    }

    // bottom
    uint8_t * srcImg = &frame->Y_frame_reconstructed[y_start_idx];
    uint8_t * srcImgMax = srcImg + (gEncoderState.width * 2);
    for (j = gEncoderState.height - 3; j < gEncoderState.height; ++j, y_start_idx += gEncoderState.width)
    {
        srcImg = &frame->Y_frame_reconstructed[y_start_idx];

        wxLineDst = &lumaHalfPixelImage_V[y_start_idx];
        srcImgA = srcImg;
        srcImgB = srcImg - gEncoderState.width;      
        srcImgC = srcImg - (gEncoderState.width * 2);
        srcImgD = srcImg + gEncoderState.width;
        if( srcImgD > srcImgMax )
            srcImgD = srcImgMax;
        srcImgE = srcImg + (gEncoderState.width * 2);
        if( srcImgE > srcImgMax )
            srcImgE = srcImgMax;
        srcImgF = srcImgMax;

        for (i = 0; i < gEncoderState.width; ++i)
        {
            is =
                (tap0 * (*srcImgA++ + *srcImgD++) +
                tap1 *  (*srcImgB++ + *srcImgE++) +
                tap2 *  (*srcImgC++ + *srcImgF++));

#ifdef VERIFY_HALF_PIXEL_OPT
			pixel = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
			ENC_ASSERT(wxLineDst[i] == pixel);
#else
			wxLineDst[i] = static_cast<uint8_t>(CLIP1( rshift_rnd_sf( is, 5 ) ));
#endif
		}
    }
}

//#undef VERIFY_HALF_PIXEL_OPT

void ComputeSubImageSixTap_H(uint8_t *lumaHalfPixelImage_H, YUVFrameType *frame)
{
    const int tap0 = ONE_FOURTH_TAP[0];
    const int tap1 = ONE_FOURTH_TAP[1];
    const int tap2 = ONE_FOURTH_TAP[2];

    int is;
    uint8_t *wBufSrc, *wBufDst;
    int16_t *iBufDst;
    uint8_t *srcImgA, *srcImgB, *srcImgC, *srcImgD, *srcImgE, *srcImgF;

    int y_start_idx = 0;
    for( int j = 0; j < gEncoderState.height; ++j, y_start_idx += frame->stride_padded )
    {
        // C B A _ D E F
        wBufSrc = &(frame->Y_frame_reconstructed[y_start_idx]);
        wBufDst = &(lumaHalfPixelImage_H[y_start_idx]);
        iBufDst = &(sLumaHalfPixelImage_Tmp[y_start_idx]);
        srcImgA = &wBufSrc[0];
        srcImgB = &wBufSrc[0];
        srcImgC = &wBufSrc[0];
        srcImgD = &wBufSrc[1];
        srcImgE = &wBufSrc[2];
        srcImgF = &wBufSrc[3];

        // first two
        is = tap0 * (*srcImgA++ + *srcImgD++) +
             tap1 * (*srcImgB   + *srcImgE++) +
             tap2 * (*srcImgC   + *srcImgF++);

        *iBufDst++ = static_cast<int16_t>(is);
#ifdef VERIFY_HALF_PIXEL_OPT
		uint8_t pixel = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
        ENC_ASSERT(*wBufDst++ == pixel);
#else
		*wBufDst++ = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
#endif

        is = tap0 * (*srcImgA++ + *srcImgD++) +
             tap1 * (*srcImgB++ + *srcImgE++) +
             tap2 * (*srcImgC   + *srcImgF++);

        *iBufDst++ = static_cast<int16_t>(is);
#ifdef VERIFY_HALF_PIXEL_OPT
		pixel = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
		ENC_ASSERT(*wBufDst++ == pixel);
#else
		*wBufDst++ = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
#endif

        // center
        for (int i = 2; i < gEncoderState.width - 4; ++i)
        {
            is =
                (tap0 * (*srcImgA++ + *srcImgD++) +
                tap1 *  (*srcImgB++ + *srcImgE++) +
                tap2 *  (*srcImgC++ + *srcImgF++));

            *iBufDst++ =  static_cast<int16_t>(is);
#ifdef VERIFY_HALF_PIXEL_OPT
			pixel = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
			ENC_ASSERT(*wBufDst++ == pixel);
#else
			*wBufDst++ = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
#endif
        }

        is = (
            tap0 * (*srcImgA++ + *srcImgD++) +
            tap1 * (*srcImgB++ + *srcImgE++) +
            tap2 * (*srcImgC++ + *srcImgF  ));

        *iBufDst++ =  static_cast<int16_t>(is);
#ifdef VERIFY_HALF_PIXEL_OPT
		pixel = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
		ENC_ASSERT(*wBufDst++ == pixel);
#else
		*wBufDst++ = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
#endif

        // right padded area
        is = (
            tap0 * (*srcImgA++ + *srcImgD++) +
            tap1 * (*srcImgB++ + *srcImgE) +
            tap2 * (*srcImgC++ + *srcImgF));

        *iBufDst++ =  static_cast<int16_t>(is);
#ifdef VERIFY_HALF_PIXEL_OPT
		pixel = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
		ENC_ASSERT(*wBufDst++ == pixel);
#else
		*wBufDst++ = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
#endif

        is = (
            tap0 * (*srcImgA++ + *srcImgD) +
            tap1 * (*srcImgB++ + *srcImgE) +
            tap2 * (*srcImgC++ + *srcImgF));

        *iBufDst++ =  static_cast<int16_t>(is);
#ifdef VERIFY_HALF_PIXEL_OPT
		pixel = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
		ENC_ASSERT(*wBufDst++ == pixel);
#else
		*wBufDst++ = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
#endif

        is = (
            tap0 * (*srcImgA + *srcImgD) +
            tap1 * (*srcImgB + *srcImgE) +
            tap2 * (*srcImgC + *srcImgF));

        *iBufDst =  static_cast<int16_t>(is);
#ifdef VERIFY_HALF_PIXEL_OPT
		pixel = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
		ENC_ASSERT(*wBufDst++ == pixel);
#else
		*wBufDst++ = static_cast<uint8_t>(CLIP1 ( rshift_rnd_sf( is, 5 ) ));
#endif
    }
}

void ME_ComputeSubPixelImages(int type)
{
    YUVFrameType *frame = gEncoderState.current_frame;

	if (frame->nal_ref_idc == 0)
		return;	// not a reference frame

	// compute half-pixels
	switch (type)
	{
	case 0:	// threaded 0 - H and D
		CalcHalfPixelPlane_H(frame->luma_buffers[ME_HALF_PIXEL_PLANE_H], frame->Y_frame_reconstructed,  frame->stride_padded);
		CalcHalfPixelPlane_D(frame->luma_buffers[ME_HALF_PIXEL_PLANE_D], frame->Y_frame_reconstructed,  frame->stride_padded);	// for diagonal
		PadFrameEdges_Luma(frame, ME_HALF_PIXEL_PLANE_H);
		PadFrameEdges_Luma(frame, ME_HALF_PIXEL_PLANE_D);
		break;
	case 1:	// threaded 1 - V
		CalcHalfPixelPlane_V(frame->luma_buffers[ME_HALF_PIXEL_PLANE_V], frame->Y_frame_reconstructed,  frame->stride_padded);
		PadFrameEdges_Luma(frame, ME_HALF_PIXEL_PLANE_V);
		break;
	case 2:	// single threaded H, V, and D
		CalcHalfPixelPlane_H(frame->luma_buffers[ME_HALF_PIXEL_PLANE_H], frame->Y_frame_reconstructed,  frame->stride_padded);
		CalcHalfPixelPlane_V(frame->luma_buffers[ME_HALF_PIXEL_PLANE_V], frame->Y_frame_reconstructed,  frame->stride_padded);
		CalcHalfPixelPlane_D(frame->luma_buffers[ME_HALF_PIXEL_PLANE_D], frame->Y_frame_reconstructed,  frame->stride_padded);	// for diagonal

		PadFrameEdges_Luma(frame, ME_HALF_PIXEL_PLANE_H);
		PadFrameEdges_Luma(frame, ME_HALF_PIXEL_PLANE_V);
		PadFrameEdges_Luma(frame, ME_HALF_PIXEL_PLANE_D);
		break;
	}
}


// Assumption: macroblock size is 16x16 (type 1)
int ME_ComputeMotionVector(MacroBlockType *mb, YUVFrameType *frame, YUVFrameType *ref_frame, MotionVector *mv, int *mad, bool quick_mode)
{
	ENC_START_TIMER(19,"ME_ComputeMotionVector");

	int list = 0; // FOR NOW: P16x16, so use list 0
	int blocktype = mb->mb_partition_type;

	int threshold = 100;//256;

	int min_mcost = 0x7FFFFFFF; // max value
	int num_checks = 0;
	static int total_checks = 0;

	assert(blocktype > 0);
	int block4x4index = bx0[mb->mb_partition_type][0]; // FOR NOW: assume P16x16, use index 0 (i.e., upper left 8x8 block)
	block4x4index += (mb->mb_x << 2);
	int *prevSad = &sDistortion[list][blocktype-1][block4x4index];

	if (ref_frame == NULL)
		return min_mcost;

	// pixel coordinates of current macroblock
	int cx = mb->mb_x * 16;
	int cy = mb->mb_y * 16;

	// data to be passed into ME_CheckPredictor()
	ME_PredictionInfoType pred_info;

	pred_info.frame = frame;
	pred_info.ref_frame = ref_frame;
	pred_info.cx = cx;
	pred_info.cy = cy;
	pred_info.pmv = mv;
	pred_info.y_buffer_16x16 = mb->Y_frame_buffer;	// new 16x16 buffer

	// keep a list of predictors
	PredictorList predictor_list;
	predictor_list.count = 0;
	predictor_list.index = 0;

	// Get median predictor
	MotionVector mv_predictor, mv_best;
	mv_predictor.mv_x = mv->mv_x;
	mv_predictor.mv_y = mv->mv_y;

	// add predictor
	assert( predictor_list.count < PREDICTORS_MAX );
	predictor_list.point[predictor_list.count] = mv_predictor;
	predictor_list.count++;

	// save best result so far
	mv_best.mv_x = mv_predictor.mv_x >> 2;
	mv_best.mv_y = mv_predictor.mv_y >> 2;

	// Compute SAD

	SearchWindow searchWindow;
	searchWindow.setRange(cx, cy);

	int p = predictor_list.index;

	MotionVector mv_;
	mv_.mv_x = predictor_list.point[p].mv_x >> 2;
	mv_.mv_y = predictor_list.point[p].mv_y >> 2;
	int mcost = ME_CheckPredictor(mv_, &pred_info);
	int min_mad = pred_info.mad;
	pred_info.best_mv = mv_;

	num_checks++;
	predictor_list.index++;

	// save lowest cost so far
	if( mcost < min_mcost )
	{
		min_mcost = mcost;
		min_mad = pred_info.mad;
	}

	MotionVector half_mv;
	half_mv.mv_x = 0;
	half_mv.mv_y = 0;

#ifdef USE_QUARTER_PIXEL_ME
	MotionVector quarter_mv;
	quarter_mv.mv_x = 0;
	quarter_mv.mv_y = 0;
#endif

	// refine the search
	if( min_mcost > threshold )
	{
		int blocksize_x = block_size[mb->mb_partition_type][0];
        H264_UNUSED(blocksize_x)

		// get adaptive threshold
		threshold = EPZSDetermineStopCriterion(prevSad, mb, 0);

		// add (0,0)
		ME_AddZeroPredictor(predictor_list);

		// add expanded diamond around zero predictor to try to get out of local minima
//		ME_AddExpandedDiamond(predictor_list);

		if (!quick_mode)	// don't add these for quick mode estimation
		{
			bool is_L0 = frame->prev_ref_frame == ref_frame;
			// add predictors (set B): L, U, UR, collocated
			ME_AddSpatialPredictors(mb->mbAddrA, predictor_list, is_L0);
			ME_AddSpatialPredictors(mb->mbAddrB, predictor_list, is_L0);
			ME_AddSpatialPredictors(mb->mb3rdNeighbor, predictor_list, is_L0);
#if 1
			ME_AddTemporalPredictors(ref_frame, mb->mb_index, predictor_list);
			if (mb->mb_x < (gEncoderState.mb_width-1))
				ME_AddTemporalPredictors(ref_frame, mb->mb_index+1, predictor_list);
			if (mb->mb_y < (gEncoderState.mb_height-1))
				ME_AddTemporalPredictors(ref_frame, mb->mb_index+gEncoderState.mb_width, predictor_list);
#endif
		}

		// check all predictors to find best predictor
		for( int p = predictor_list.index; p < predictor_list.count; ++p )
		{
			mv_.mv_x = predictor_list.point[p].mv_x >> 2;
			mv_.mv_y = predictor_list.point[p].mv_y >> 2;
			mcost = ME_CheckPredictor(mv_, &pred_info);
			num_checks++;
//			ENC_TRACE("(%d %d) cost:%d\n", predictor_list.point[p].mv_x, predictor_list.point[p].mv_y, mcost);

			predictor_list.index++;

			if( mcost < min_mcost )
			{
				min_mcost = mcost;
				min_mad = pred_info.mad;
				mv_best = mv_;
			}
		}

//		if( min_mcost < threshold )
//			goto early_terminate;

#if 0	// currently off pending investigation on how best to use this - possibly use as quality/speed param (mfong: 11/4/13)
		// terminate early based on threshold
		//
		// The threshold is based on data that shows that when an I-block is chosen over a P-block,
		// most of the C_i values (used in Decision_I_or_P()) is less than 3500.
		// So terminate early to avoid wasting time on a possibly significant number (over 100) of
		// unnecessary block comparisons.
		if( min_mcost > 3500 )
			goto early_terminate;
#endif

		// refine the best predictor
		pred_info.center_mv = mv_best;
		pred_info.best_mv = mv_best;
		pred_info.min_cost = min_mcost;
		pred_info.min_sad = min_mad;
#if 0
		if (ME_EarlyTermination(&pred_info))
		{
			mv_best = pred_info.best_mv; // save minimum cost predictor
			min_mcost = pred_info.min_cost;
			min_mad = pred_info.min_sad;
			goto early_terminate;
		}

		pred_info.center_mv = pred_info.best_mv;
#endif
		if (ME_Search_Diamond(&pred_info))
//		if (ME_Search_SmallDiamond(&pred_info))
		{
			mv_best = pred_info.best_mv; // save minimum cost predictor
			min_mcost = pred_info.min_cost;
			min_mad = pred_info.min_sad;
		}
	}

//early_terminate:
	// clip mvs
	mv_best.mv_x = static_cast<short>(CLIP3(-cx, frame->width - OFFSET_FROM_EDGE - cx, mv_best.mv_x));
	mv_best.mv_y = static_cast<short>(CLIP3(-cy, frame->height - OFFSET_FROM_EDGE - cy, mv_best.mv_y));

	// check half-pixels
	if (gEncoderState.subpixel_motion_est > 0)
	{
		// refine the best predictor
		pred_info.center_mv = mv_best;
		pred_info.best_mv = mv_best;
		pred_info.min_cost = min_mcost;
		pred_info.min_sad = min_mad;

		// ME_PredictionInfoType *info
		min_mcost = CheckHalfPixel(&pred_info, &half_mv);

#ifdef USE_QUARTER_PIXEL_ME
		// check quarter-pixels
		if (gEncoderState.subpixel_motion_est > 1)
			min_mcost = CheckQuarterPixel(mb, cx, cy, mv_best.mv_x, mv_best.mv_y, min_mcost, half_mv, &quarter_mv, prev_frame, frame);
#endif

		mv_best = pred_info.best_mv; // save minimum cost predictor
		min_mad = pred_info.min_sad;
	}

	mv->mv_x = mv_best.mv_x << 2;
	mv->mv_y = mv_best.mv_y << 2;

	if (gEncoderState.subpixel_motion_est > 0)
	{
		mv->mv_x += half_mv.mv_x;
		mv->mv_y += half_mv.mv_y;
	}

#ifdef USE_QUARTER_PIXEL_ME
	mv->mv_x += quarter_mv.mv_x;
	mv->mv_y += quarter_mv.mv_y;
#endif

//	assert( (-128 <= mv->mv_x) && (mv->mv_x <= 128) );
//	assert( (-128 <= mv->mv_y) && (mv->mv_y <= 128) );
#if 0	// try it here to always grab latest best mv for this mb whether it is chosen or not
	// add to temporal mv table
	ref_frame->temporal_mv[mb->mb_index].mx = mv->mv_x * ref_frame->temporal_mv_distscale;
	ref_frame->temporal_mv[mb->mb_index].my = mv->mv_y * ref_frame->temporal_mv_distscale;
#endif
	// adjust min_mcost back down to base MV cost so we can fine tune against I block & PSKIP
	int mv_cost = min_mcost - min_mad;

	min_mcost = min_mad + (mv_cost / frame->lambda_factor);

	int ref = 0; // FOR NOW: assume ref index is 0
	if ((ref == 0) || (*prevSad > min_mcost))
		*prevSad = min_mcost;
	ENC_PAUSE_TIMER(19,false);
	static int total_cost = 0;

	total_cost += min_mcost;
	total_checks += num_checks;
//	ENC_TRACE("MB:%d cost:%d #:%d total:%d avg: %d  mad:%d\n", mb->mb_index, min_mcost, num_checks, total_checks, total_cost / (mb->mb_index + 1), min_mad);

	*mad = min_mad;
	return min_mcost;
}

// run standard diamond pattern search


const int kSmallDiamondPoints[4][2] = {
	{  0, -1 }, // top
	{  1,  0 }, // right
	{  0,  1 }, // bottom
	{ -1,  0 }, // left
};

const int kDiamondPoints[8][2] = {
	{  0, -2 }, // top
	{  1, -1 }, // top right
	{  2,  0 }, // right
	{  1,  1 }, // bottom right
	{  0,  2 }, // bottom
	{ -1,  1 }, // bottom left
	{ -2,  0 }, // left
	{ -1, -1 }, // top left
};

const int kDiamond2ndPatternIndex[8][6] = {
	{ 5,   0, 1, 2, 6, 7 },	// top
	{ 3,   0, 1, 2,-1,-1 },	// top right
	{ 5,   0, 1, 2, 3, 4 }, // right
	{ 3,   2, 3, 4,-1,-1 }, // bottom right
	{ 5,   2, 3, 4, 5, 6 }, // bottom
	{ 3,   4, 5, 6,-1,-1 }, // bottom left
	{ 5,   4, 5, 6, 7, 0 }, // left
	{ 3,   6, 7, 0,-1,-1 }, // top left
};

bool ME_Search_Diamond(ME_PredictionInfoType *stype)
{
	int mcost;
	int next_dir = -1;
	int radius_limit = 8;
	MotionVector mv;
	bool new_mv_found = false;

	// first check full pattern
	for (int i = 0; i < 8; i++)
	{
		mv.mv_x = static_cast<short>(stype->center_mv.mv_x + kDiamondPoints[i][0]);
		mv.mv_y = static_cast<short>(stype->center_mv.mv_y + kDiamondPoints[i][1]);

		mcost = ME_CheckPredictor(mv, stype);
		stype->num_checks++;

		if( mcost < stype->min_cost )
		{
			stype->best_mv = mv; // save minimum cost predictor
			stype->min_cost = mcost;
			stype->min_sad = stype->mad;
			next_dir = i;
			new_mv_found = true;
		}
	}

	// now keep checking for next best until the center is the best
	while ((next_dir != -1) && (--radius_limit > 0))
	{
		const int *pattern = kDiamond2ndPatternIndex[next_dir];
		stype->center_mv = stype->best_mv;
		next_dir = -1;
		// try direction that gave the best mv
		for (int i = pattern[0]; i > 0; i--)
		{
			int index = pattern[i];

			mv.mv_x = static_cast<short>(stype->center_mv.mv_x + kDiamondPoints[index][0]);
			mv.mv_y = static_cast<short>(stype->center_mv.mv_y + kDiamondPoints[index][1]);

			mcost = ME_CheckPredictor(mv, stype);
			stype->num_checks++;

			if( mcost < stype->min_cost )
			{
				stype->best_mv = mv; // save minimum cost predictor
				stype->min_cost = mcost;
				stype->min_sad = stype->mad;
				next_dir = index;
				new_mv_found = true;
			}
		}
	}

	stype->center_mv = stype->best_mv;

	// finally do a small diamond check for the best
	for (int i = 0; i < 4; i++)
	{
		mv.mv_x = static_cast<short>(stype->center_mv.mv_x + kSmallDiamondPoints[i][0]);
		mv.mv_y = static_cast<short>(stype->center_mv.mv_y + kSmallDiamondPoints[i][1]);

		mcost = ME_CheckPredictor(mv, stype);
		stype->num_checks++;

		if( mcost < stype->min_cost )
		{
			stype->best_mv = mv; // save minimum cost predictor
			stype->min_cost = mcost;
			stype->min_sad = stype->mad;
			next_dir = i;
			new_mv_found = true;
		}
	}

	return new_mv_found;
}

bool ME_Search_SmallDiamond(ME_PredictionInfoType *stype)
{
	int mcost;
	int next_dir = -1;
	int radius_limit = 16;
	MotionVector mv;
	bool new_mv_found = false;

	// first check full pattern
	for (int i = 0; i < 4; i++)
	{
		mv.mv_x = static_cast<short>(stype->center_mv.mv_x + kSmallDiamondPoints[i][0]);
		mv.mv_y = static_cast<short>(stype->center_mv.mv_y + kSmallDiamondPoints[i][1]);

		mcost = ME_CheckPredictor(mv, stype);
		stype->num_checks++;

		if( mcost < stype->min_cost )
		{
			stype->best_mv = mv; // save minimum cost predictor
			stype->min_cost = mcost;
			stype->min_sad = stype->mad;
			next_dir = i;
			new_mv_found = true;
		}
	}

	// now keep checking for next best until the center is the best
	while ((next_dir != -1) && (--radius_limit > 0))
	{
		stype->center_mv = stype->best_mv;
		next_dir = -1;
		// try direction that gave the best mv
		for (int i = 0; i < 4; i++)
		{
			int index = i;

			mv.mv_x = static_cast<short>(stype->center_mv.mv_x + kSmallDiamondPoints[index][0]);
			mv.mv_y = static_cast<short>(stype->center_mv.mv_y + kSmallDiamondPoints[index][1]);

			mcost = ME_CheckPredictor(mv, stype);
			stype->num_checks++;

			if( mcost < stype->min_cost )
			{
				stype->best_mv = mv; // save minimum cost predictor
				stype->min_cost = mcost;
				stype->min_sad = stype->mad;
				next_dir = index;
				new_mv_found = true;
			}
		}
	}

	stype->center_mv = stype->best_mv;

	return new_mv_found;
}

bool ME_EarlyTermination(ME_PredictionInfoType *stype)
{
	int mcost;
	int next_dir = -1;
	MotionVector mv, offset;
	bool new_mv_found = false;
	bool step2_mv_found = false;

	// do a small diamond check for (0,0) & Median Vector
	offset.mv_x = offset.mv_y = 0;
	for (int j = 0; j < 2; j++)
	{
		if (j == 1)
		{
			if ((stype->pmv->mv_x == 0) && (stype->pmv->mv_y == 0))
				continue;
			offset.mv_x = stype->pmv->mv_x >> 2;
			offset.mv_y = stype->pmv->mv_y >> 2;
		}
		for (int i = 0; i < 4; i++)
		{
			mv.mv_x = static_cast<short>(stype->center_mv.mv_x + kSmallDiamondPoints[i][0] + offset.mv_x);
			mv.mv_y = static_cast<short>(stype->center_mv.mv_y + kSmallDiamondPoints[i][1] + offset.mv_y);

			mcost = ME_CheckPredictor(mv, stype);
			stype->num_checks++;

			if( mcost < stype->min_cost )
			{
				stype->best_mv = mv; // save minimum cost predictor
				stype->min_cost = mcost;
				stype->min_sad = stype->mad;
				next_dir = i;
				step2_mv_found = true;
			}
		}
	}

	if (step2_mv_found)
		stype->center_mv = stype->best_mv;

	// do a small diamond check for the best
	for (int i = 0; i < 4; i++)
	{
		mv.mv_x = static_cast<short>(stype->center_mv.mv_x + kSmallDiamondPoints[i][0]);
		mv.mv_y = static_cast<short>(stype->center_mv.mv_y + kSmallDiamondPoints[i][1]);

		mcost = ME_CheckPredictor(mv, stype);
		stype->num_checks++;

		if( mcost < stype->min_cost )
		{
			stype->best_mv = mv; // save minimum cost predictor
			stype->min_cost = mcost;
			stype->min_sad = stype->mad;
			next_dir = i;
			new_mv_found = true;
		}
	}

	if (new_mv_found)
		return false;	// set new best and return saying keep going

	// check diamond radius 2
	for (int i = 0; i < 8; i++)
	{
		mv.mv_x = static_cast<short>(stype->center_mv.mv_x + kDiamondPoints[i][0]);
		mv.mv_y = static_cast<short>(stype->center_mv.mv_y + kDiamondPoints[i][1]);

		mcost = ME_CheckPredictor(mv, stype);
		stype->num_checks++;

		if( mcost < stype->min_cost )
		{
			stype->best_mv = mv; // save minimum cost predictor
			stype->min_cost = mcost;
			stype->min_sad = stype->mad;
			next_dir = i;
			new_mv_found = true;
		}
	}

	if (new_mv_found)
		return false;	// set new best and return saying keep going

	if (step2_mv_found)
	{
		int scale = 3;
		for (int j = 0; j < 3; j++, scale += 2)
		{
			for (int i = 0; i < 4; i++)
			{
				mv.mv_x = static_cast<short>(stype->center_mv.mv_x + kSmallDiamondPoints[i][0] * scale);
				mv.mv_y = static_cast<short>(stype->center_mv.mv_y + kSmallDiamondPoints[i][1] * scale);

				mcost = ME_CheckPredictor(mv, stype);
				stype->num_checks++;

				if( mcost < stype->min_cost )
				{
					stype->best_mv = mv; // save minimum cost predictor
					stype->min_cost = mcost;
					stype->min_sad = stype->mad;
					next_dir = i;
					new_mv_found = true;
				}
			}
		}
	}

	return new_mv_found;
}

#define TAP2(x,y,lo,hi) mm_tmp_x = _mm_unpacklo_epi8((x),mm_zero);\
						mm_tmp_y = _mm_unpacklo_epi8((y),mm_zero);\
						(lo) = _mm_add_epi16(mm_tmp_x, mm_tmp_y);\
						mm_tmp_x = _mm_unpackhi_epi8((x),mm_zero);\
						mm_tmp_y = _mm_unpackhi_epi8((y),mm_zero);\
						(hi) = _mm_add_epi16(mm_tmp_x, mm_tmp_y);

#define TAP0_1(x,y,lo,hi,factor)	mm_tmp_x = _mm_unpacklo_epi8((x),mm_zero);\
									mm_tmp_y = _mm_unpacklo_epi8((y),mm_zero);\
									mm_tmp_x = _mm_add_epi16(mm_tmp_x, mm_tmp_y);\
									mm_tmp_x = _mm_mullo_epi16(mm_tmp_x,(factor));\
									(lo) = _mm_add_epi16((lo), mm_tmp_x);\
									mm_tmp_x = _mm_unpackhi_epi8((x),mm_zero);\
									mm_tmp_y = _mm_unpackhi_epi8((y),mm_zero);\
									mm_tmp_x = _mm_add_epi16(mm_tmp_x, mm_tmp_y);\
									mm_tmp_x = _mm_mullo_epi16(mm_tmp_x,(factor));\
									(hi) = _mm_add_epi16((hi), mm_tmp_x);

void CalcHalfPixelPlane_V(uint8_t *dst, uint8_t *src, int stride)
{	// 16x16
	__m128i mm_src_A, mm_src_B, mm_src_C, mm_src_D, mm_src_E, mm_src_F;	// C-B-A><D-E-F
	__m128i mm_tmp_x, mm_tmp_y, mm_sum_lo, mm_sum_hi;
	__m128i mm_tap1_factor = _mm_set1_epi16(-5);
	__m128i mm_tap0_factor = _mm_set1_epi16(20);
	__m128i mm_zero = _mm_setzero_si128();
	__m128i mm_max_8bit = _mm_set1_epi16(255);
	__m128i mm_rounding = _mm_set1_epi16(16);

	uint8_t *src_x, *dst_x;

	for (int y = gEncoderState.height; y > 0; y--, dst += stride, src += stride)
	{
		src_x = src;
		dst_x = dst;
		for (int x = gEncoderState.width; x > 0; x -= 16, dst_x += 16, src_x += 16)
		{
			mm_src_C = _mm_loadu_si128((__m128i *) (src_x-(stride*2)));
			mm_src_F = _mm_loadu_si128((__m128i *) (src_x+(stride*3)));
			TAP2(mm_src_C, mm_src_F, mm_sum_lo, mm_sum_hi);
			mm_src_B = _mm_loadu_si128((__m128i *) (src_x-(stride*1)));
			mm_src_E = _mm_loadu_si128((__m128i *) (src_x+(stride*2)));
			TAP0_1(mm_src_B, mm_src_E, mm_sum_lo, mm_sum_hi, mm_tap1_factor);
			mm_src_A = _mm_loadu_si128((__m128i *) (src_x));
			mm_src_D = _mm_loadu_si128((__m128i *) (src_x+(stride)));
			TAP0_1(mm_src_A, mm_src_D, mm_sum_lo, mm_sum_hi, mm_tap0_factor);

			// clip and pack
			mm_sum_lo = _mm_add_epi16(mm_sum_lo, mm_rounding);
			mm_sum_lo = _mm_srai_epi16(mm_sum_lo, 5);
			mm_sum_lo = _mm_max_epi16(mm_sum_lo, mm_zero);		// if sum < 0 sum = 0
			mm_sum_lo = _mm_min_epi16(mm_sum_lo, mm_max_8bit);	// if sum > 255 sum = 255
			mm_sum_hi = _mm_add_epi16(mm_sum_hi, mm_rounding);
			mm_sum_hi = _mm_srai_epi16(mm_sum_hi, 5);
			mm_sum_hi = _mm_max_epi16(mm_sum_hi, mm_zero);		// if sum < 0 sum = 0
			mm_sum_hi = _mm_min_epi16(mm_sum_hi, mm_max_8bit);	// if sum > 255 sum = 255
			mm_sum_lo = _mm_packus_epi16(mm_sum_lo, mm_sum_hi);

			_mm_store_si128((__m128i *) dst_x, mm_sum_lo);
		}
	}
}

// SSSE3 version
#define H_TAP_2(src,cpy)	(cpy) = _mm_srli_si128((src), 1);\
							mm_tmp_x = _mm_maddubs_epi16((src), mm_tap_factor);\
							mm_tmp_x = _mm_hadd_epi16(mm_tmp_x, mm_zero);\
							mm_tmp_x = _mm_hadd_epi16(mm_tmp_x, mm_zero);


#define H_TAP_ASSIGN(x)  mm_tmp_y = _mm_and_si128(mm_tmp_x, mm_mask_X0);\
						 mm_tmp_y = _mm_slli_si128(mm_tmp_y, (x)*2+2);\
						 mm_sum_lo = _mm_or_si128(mm_sum_lo, mm_tmp_y);\
						 mm_tmp_x = _mm_and_si128(mm_tmp_x, mm_mask_0X);\
						 mm_tmp_x = _mm_slli_si128(mm_tmp_x, (x)*2);\
						 mm_sum_hi = _mm_or_si128(mm_sum_hi, mm_tmp_x);

void CalcHalfPixelPlane_H_SSSE3(uint8_t *dst, uint8_t *src, int stride)
{	// 16x16
	__m128i mm_src, mm_src1, mm_src2, mm_src_cpy;
	__m128i mm_tmp_x, mm_tmp_y, mm_sum_lo, mm_sum_hi;
	__m128i mm_zero = _mm_setzero_si128();
	__m128i mm_max_8bit = _mm_set1_epi16(255);
	__m128i mm_rounding = _mm_set1_epi16(16);
	__m128i mm_tap_factor;
	__m128i mm_mask_X0 = _mm_set_epi32(0, 0, 0, 0x0000ffff);
	__m128i mm_mask_0X = _mm_set_epi32(0, 0, 0, 0xffff0000);

	uint8_t *src_x, *dst_x;

	int16_t *tmp_dst = sLumaHalfPixelImage_Tmp + (gEncoderState.width_padded * FRAME_HEIGHT_PAD) + FRAME_WIDTH_PAD;
	int16_t *tmp_dst_x;

	// in 64-bit we must be explicit, _mm_set_epi32() seems to munge the order
	mm_tap_factor.m128i_u32[0] = mm_tap_factor.m128i_u32[2] = 0x1414fb01;
	mm_tap_factor.m128i_u32[1] = mm_tap_factor.m128i_u32[3] = 0x000001fb;

	for (int y = gEncoderState.height; y > 0; y--, dst += stride, src += stride, tmp_dst += stride)
	{
		src_x = src-2;
		dst_x = dst;
		tmp_dst_x = tmp_dst;

		for (int x = 0; x < gEncoderState.width; x += 16)
		{
			mm_src = _mm_loadu_si128((__m128i *) (src_x));
			src_x += 3;
			mm_src1 = _mm_loadu_si128((__m128i *) (src_x));
			mm_src2 = _mm_loadu_si128((__m128i *) (src_x+3));

			H_TAP_2(mm_src,mm_src_cpy);
			mm_sum_hi = _mm_srli_si128(mm_tmp_x, 2);			// half-pixel 8  |hp8|0|0|0|0|0|0|0|
			mm_sum_lo = _mm_and_si128(mm_tmp_x, mm_mask_X0);	// half-pixel 0  |hp0|0|0|0|0|0|0|0|

			H_TAP_2(mm_src_cpy,mm_src);
			mm_tmp_y = _mm_slli_epi32(mm_tmp_x, 16);
			mm_sum_lo = _mm_or_si128(mm_sum_lo, mm_tmp_y);		// half-pixel 1  |hp0|hp1|0|0|0|0|0|0|
			mm_tmp_x = _mm_and_si128(mm_tmp_x, mm_mask_0X);
			mm_sum_hi = _mm_or_si128(mm_sum_hi, mm_tmp_x);		// half-pixel 9  |hp8|hp9|0|0|0|0|0|0|

			H_TAP_2(mm_src,mm_src_cpy);
			H_TAP_ASSIGN(1);
			// half-pixel 2   |hp0|hp1|hp2|0|0|0|0|0|
			// half-pixel 10  |hp8|hp9|hp10|0|0|0|0|0|

			H_TAP_2(mm_src1,mm_src_cpy);
			H_TAP_ASSIGN(2);
			// half-pixel 3   |hp0|hp1|hp2 |hp3 |0|0|0|0|
			// half-pixel 11  |hp8|hp9|hp10|hp11|0|0|0|0|

			H_TAP_2(mm_src_cpy,mm_src);
			H_TAP_ASSIGN(3);
			// half-pixel 4   |hp0|hp1|hp2 |hp3 |hp4 |0|0|0|
			// half-pixel 12  |hp8|hp9|hp10|hp11|hp12|0|0|0|

			H_TAP_2(mm_src, mm_src_cpy);
			H_TAP_ASSIGN(4);
			// half-pixel 5   |hp0|hp1|hp2 |hp3 |hp4 |hp5 |0|0|
			// half-pixel 13  |hp8|hp9|hp10|hp11|hp12|hp13|0|0|

			src_x += 3;
			H_TAP_2(mm_src2,mm_src_cpy);
			H_TAP_ASSIGN(5);
			// half-pixel 6   |hp0|hp1|hp2 |hp3 |hp4 |hp5 |hp6 |0|
			// half-pixel 14  |hp8|hp9|hp10|hp11|hp12|hp13|hp14|0|

			H_TAP_2(mm_src_cpy,mm_src);
			H_TAP_ASSIGN(6);
			// half-pixel 7   |hp0|hp1|hp2 |hp3 |hp4 |hp5 |hp6 |hp7 |
			// half-pixel 15  |hp8|hp9|hp10|hp11|hp12|hp13|hp14|hp15|

			// save off full-sized horizontal tapped values for diagonal calculations
			_mm_storeu_si128((__m128i *)     tmp_dst_x, mm_sum_lo);
			_mm_storeu_si128((__m128i *) &tmp_dst_x[8], mm_sum_hi);

			// clip and pack
			mm_sum_lo = _mm_add_epi16(mm_sum_lo, mm_rounding);
			mm_sum_lo = _mm_srai_epi16(mm_sum_lo, 5);
			mm_sum_lo = _mm_max_epi16(mm_sum_lo, mm_zero);		// if sum < 0 sum = 0
			mm_sum_lo = _mm_min_epi16(mm_sum_lo, mm_max_8bit);	// if sum > 255 sum = 255
			mm_sum_hi = _mm_add_epi16(mm_sum_hi, mm_rounding);
			mm_sum_hi = _mm_srai_epi16(mm_sum_hi, 5);
			mm_sum_hi = _mm_max_epi16(mm_sum_hi, mm_zero);		// if sum < 0 sum = 0
			mm_sum_hi = _mm_min_epi16(mm_sum_hi, mm_max_8bit);	// if sum > 255 sum = 255
			mm_sum_lo = _mm_packus_epi16(mm_sum_lo, mm_sum_hi);

			_mm_storeu_si128((__m128i *) dst_x, mm_sum_lo);

			src_x += 2 + 8;
			dst_x += 16;
			tmp_dst_x += 16;
		}
	}
}

// SSE2 version
#define H_TAP_2_SSE2(src,cpy)   (cpy) = _mm_srli_si128((src), 1);\
								mm_tmp_x = _mm_unpacklo_epi8((src), mm_zero);\
								mm_tmp_y = _mm_srli_si128(mm_tmp_x,6);\
								mm_tmp_y = _mm_shufflelo_epi16(mm_tmp_y,0xc6);\
								mm_tmp_x = _mm_add_epi16(mm_tmp_x, mm_tmp_y);\
								mm_tmp_x = _mm_madd_epi16(mm_tmp_x, mm_tap_factor);\
								mm_tmp_y = _mm_srli_si128(mm_tmp_x, 4);\
								mm_tmp_lo = _mm_add_epi32(mm_tmp_x, mm_tmp_y);\
								mm_tmp_x = _mm_unpackhi_epi8((src), mm_zero);\
								mm_tmp_y = _mm_srli_si128(mm_tmp_x,6);\
								mm_tmp_y = _mm_shufflelo_epi16(mm_tmp_y,0xc6);\
								mm_tmp_x = _mm_add_epi16(mm_tmp_x, mm_tmp_y);\
								mm_tmp_x = _mm_madd_epi16(mm_tmp_x, mm_tap_factor);\
								mm_tmp_y = _mm_srli_si128(mm_tmp_x, 4);\
								mm_tmp_x = _mm_add_epi32(mm_tmp_x, mm_tmp_y);

#define H_TAP_ASSIGN_SSE2(x)	mm_tmp_y = _mm_and_si128(mm_tmp_lo, mm_mask_X0);\
								mm_tmp_y = _mm_slli_si128(mm_tmp_y, (x)*2+2);\
								mm_sum_lo = _mm_or_si128(mm_sum_lo, mm_tmp_y);\
								mm_tmp_x = _mm_and_si128(mm_tmp_x, mm_mask_X0);\
								mm_tmp_x = _mm_slli_si128(mm_tmp_x, (x)*2+2);\
								mm_sum_hi = _mm_or_si128(mm_sum_hi, mm_tmp_x);

void CalcHalfPixelPlane_H_SSE2(uint8_t *dst, uint8_t *src, int stride)
{	// 16x16
	__m128i mm_src, mm_src1, mm_src2, mm_src_cpy;
	__m128i mm_tmp_x, mm_tmp_y, mm_tmp_lo, mm_sum_lo, mm_sum_hi;
	__m128i mm_zero = _mm_setzero_si128();
	__m128i mm_max_8bit = _mm_set1_epi16(255);
	__m128i mm_rounding = _mm_set1_epi16(16);
	__m128i mm_tap_factor;
	__m128i mm_mask_X0 = _mm_set_epi32(0, 0, 0, 0x0000ffff);
	//__m128i mm_mask_0X = _mm_set_epi32(0, 0, 0, 0xffff0000);

	uint8_t *src_x, *dst_x;

	int16_t *tmp_dst = sLumaHalfPixelImage_Tmp + (gEncoderState.width_padded * FRAME_HEIGHT_PAD) + FRAME_WIDTH_PAD;
	int16_t *tmp_dst_x;

	mm_tap_factor.m128i_i16[0] = 1;
	mm_tap_factor.m128i_i16[1] = -5;
	mm_tap_factor.m128i_i16[2] = 20;
	mm_tap_factor.m128i_i16[3] = mm_tap_factor.m128i_i16[4] = mm_tap_factor.m128i_i16[5] = mm_tap_factor.m128i_i16[6] = mm_tap_factor.m128i_i16[7] = 0;

	for (int y = gEncoderState.height; y > 0; y--, dst += stride, src += stride, tmp_dst += stride)
	{
		src_x = src-2;
		dst_x = dst;
		tmp_dst_x = tmp_dst;

		for (int x = 0; x < gEncoderState.width; x += 16)
		{
			mm_src = _mm_loadu_si128((__m128i *) (src_x));
			src_x += 3;
			mm_src1 = _mm_loadu_si128((__m128i *) (src_x));
			mm_src2 = _mm_loadu_si128((__m128i *) (src_x+3));

			H_TAP_2_SSE2(mm_src,mm_src_cpy);
			mm_sum_hi = _mm_and_si128(mm_tmp_x,  mm_mask_X0);	// half-pixel 8  |hp8|0|0|0|0|0|0|0|
			mm_sum_lo = _mm_and_si128(mm_tmp_lo, mm_mask_X0);	// half-pixel 0  |hp0|0|0|0|0|0|0|0|

			H_TAP_2_SSE2(mm_src_cpy,mm_src);
			mm_tmp_lo = _mm_and_si128(mm_tmp_lo,  mm_mask_X0);
			mm_tmp_y  = _mm_slli_epi32(mm_tmp_lo, 16);
			mm_sum_lo = _mm_or_si128(mm_sum_lo, mm_tmp_y);		// half-pixel 1  |hp0|hp1|0|0|0|0|0|0|
			mm_tmp_x = _mm_and_si128(mm_tmp_x, mm_mask_X0);
			mm_tmp_x  = _mm_slli_epi32(mm_tmp_x, 16);
			mm_sum_hi = _mm_or_si128(mm_sum_hi, mm_tmp_x);		// half-pixel 9  |hp8|hp9|0|0|0|0|0|0|

			H_TAP_2_SSE2(mm_src,mm_src_cpy);
			H_TAP_ASSIGN_SSE2(1);
			// half-pixel 2   |hp0|hp1|hp2|0|0|0|0|0|
			// half-pixel 10  |hp8|hp9|hp10|0|0|0|0|0|

			H_TAP_2_SSE2(mm_src1,mm_src_cpy);
			H_TAP_ASSIGN_SSE2(2);
			// half-pixel 3   |hp0|hp1|hp2 |hp3 |0|0|0|0|
			// half-pixel 11  |hp8|hp9|hp10|hp11|0|0|0|0|

			H_TAP_2_SSE2(mm_src_cpy,mm_src);
			H_TAP_ASSIGN_SSE2(3);
			// half-pixel 4   |hp0|hp1|hp2 |hp3 |hp4 |0|0|0|
			// half-pixel 12  |hp8|hp9|hp10|hp11|hp12|0|0|0|

			H_TAP_2_SSE2(mm_src, mm_src_cpy);
			H_TAP_ASSIGN_SSE2(4);
			// half-pixel 5   |hp0|hp1|hp2 |hp3 |hp4 |hp5 |0|0|
			// half-pixel 13  |hp8|hp9|hp10|hp11|hp12|hp13|0|0|

			src_x += 3;
			H_TAP_2_SSE2(mm_src2,mm_src_cpy);
			H_TAP_ASSIGN_SSE2(5);
			// half-pixel 6   |hp0|hp1|hp2 |hp3 |hp4 |hp5 |hp6 |0|
			// half-pixel 14  |hp8|hp9|hp10|hp11|hp12|hp13|hp14|0|

			H_TAP_2_SSE2(mm_src_cpy,mm_src);
			H_TAP_ASSIGN_SSE2(6);
			// half-pixel 7   |hp0|hp1|hp2 |hp3 |hp4 |hp5 |hp6 |hp7 |
			// half-pixel 15  |hp8|hp9|hp10|hp11|hp12|hp13|hp14|hp15|

#if 0 // verify
			for (int i = 0; i < 8; i++)
			{
				ENC_ASSERT(tmp_dst_x[i]   == mm_sum_lo.m128i_i16[i]);
				ENC_ASSERT(tmp_dst_x[i+8] == mm_sum_hi.m128i_i16[i]);
			}
#endif

			// save off full-sized horizontal tapped values for diagonal calculations
			_mm_storeu_si128((__m128i *)     tmp_dst_x, mm_sum_lo);
			_mm_storeu_si128((__m128i *) &tmp_dst_x[8], mm_sum_hi);

			// clip and pack
			mm_sum_lo = _mm_add_epi16(mm_sum_lo, mm_rounding);
			mm_sum_lo = _mm_srai_epi16(mm_sum_lo, 5);
			mm_sum_lo = _mm_max_epi16(mm_sum_lo, mm_zero);		// if sum < 0 sum = 0
			mm_sum_lo = _mm_min_epi16(mm_sum_lo, mm_max_8bit);	// if sum > 255 sum = 255
			mm_sum_hi = _mm_add_epi16(mm_sum_hi, mm_rounding);
			mm_sum_hi = _mm_srai_epi16(mm_sum_hi, 5);
			mm_sum_hi = _mm_max_epi16(mm_sum_hi, mm_zero);		// if sum < 0 sum = 0
			mm_sum_hi = _mm_min_epi16(mm_sum_hi, mm_max_8bit);	// if sum > 255 sum = 255
			mm_sum_lo = _mm_packus_epi16(mm_sum_lo, mm_sum_hi);

			_mm_storeu_si128((__m128i *) dst_x, mm_sum_lo);

			src_x += 2 + 8;
			dst_x += 16;
			tmp_dst_x += 16;
		}
	}
}

void CalcHalfPixelPlane_H(uint8_t *dst, uint8_t *src, int stride)
{
	if (gEncoderState.SSSE3_enabled)
	{
		CalcHalfPixelPlane_H_SSSE3(dst, src, stride);
	}
	else
	{
		CalcHalfPixelPlane_H_SSE2(dst, src, stride);
	}
}

// unpack signed
// packed with zero in low 16-bits and use shift to sign extend
#define TAP2_16(x,y,lo,hi)  mm_tmp_x = _mm_add_epi16((x), (y));\
							mm_sum_lo = _mm_unpacklo_epi16(mm_zero, mm_tmp_x);\
							mm_sum_hi = _mm_unpackhi_epi16(mm_zero, mm_tmp_x);\
							mm_sum_lo = _mm_srai_epi32(mm_sum_lo, 16);\
							mm_sum_hi = _mm_srai_epi32(mm_sum_hi, 16);

#define TAP0_1_16(x,y,lo,hi,factor)	mm_tmp_x = _mm_unpacklo_epi16((x), mm_zero);\
									mm_tmp_y = _mm_unpacklo_epi16(mm_zero, (y));\
									mm_tmp_x = _mm_or_si128(mm_tmp_x, mm_tmp_y);\
									mm_tmp_y = _mm_madd_epi16(mm_tmp_x,(factor));\
									(lo) = _mm_add_epi32((lo), mm_tmp_y);\
									mm_tmp_x = _mm_unpackhi_epi16((x), mm_zero);\
									mm_tmp_y = _mm_unpackhi_epi16(mm_zero, (y));\
									mm_tmp_x = _mm_or_si128(mm_tmp_x, mm_tmp_y);\
									mm_tmp_x = _mm_madd_epi16(mm_tmp_x,(factor));\
									(hi) = _mm_add_epi32((hi), mm_tmp_x);

void CalcHalfPixelPlane_D(uint8_t *dst, uint8_t *dummy, int stride)
{	// 16x16
	const int tap0 = ONE_FOURTH_TAP[0];
	const int tap1 = ONE_FOURTH_TAP[1];

	int16_t *src = sLumaHalfPixelImage_Tmp + (gEncoderState.width_padded * FRAME_HEIGHT_PAD) + FRAME_WIDTH_PAD;

	dummy = dst;	// save for verification

	// pad top and bottom edges
	// top edge
	int16_t *pad_src = src;
	int16_t *pad_dst = sLumaHalfPixelImage_Tmp + FRAME_WIDTH_PAD;	// very top of buffer
	for (int i = 0; i < FRAME_HEIGHT_PAD; i++, pad_dst += stride)
	{
		memcpy(pad_dst, pad_src, gEncoderState.width * 2);
	}

	// bottom edge
	pad_src = src + (stride * (gEncoderState.height - 1));
	pad_dst = pad_src + (stride);
	for (int i = 0; i < FRAME_HEIGHT_PAD; i++, pad_dst += stride)
	{
		memcpy(pad_dst, pad_src, gEncoderState.width * 2);
	}

	__m128i mm_src_A, mm_src_B, mm_src_C, mm_src_D, mm_src_E, mm_src_F;	// C-B-A><D-E-F
	__m128i mm_tmp_x, mm_tmp_y, mm_sum_lo, mm_sum_hi;
	__m128i mm_tap1_factor = _mm_set1_epi16(static_cast<short>(tap1));
	__m128i mm_tap0_factor = _mm_set1_epi16(static_cast<short>(tap0));
	__m128i mm_zero = _mm_setzero_si128();
	__m128i mm_max_8bit = _mm_set1_epi16(255);
	__m128i mm_rounding = _mm_set1_epi32(1 << 9);
	__m128i mm_result;

	int16_t *src_x;
	uint8_t *dst_x;

	for (int y = gEncoderState.height; y > 0; y--, dst += stride, src += stride)
	{
		src_x = src;
		dst_x = dst;
		for (int x = gEncoderState.width; x > 0; x -= 16, dst_x += 16, src_x += 8)
		{
			// first 8 pixels
			mm_src_C = _mm_loadu_si128((__m128i *) (src_x-(stride*2)));
			mm_src_F = _mm_loadu_si128((__m128i *) (src_x+(stride*3)));
			TAP2_16(mm_src_C, mm_src_F, mm_sum_lo, mm_sum_hi);
			mm_src_B = _mm_loadu_si128((__m128i *) (src_x-(stride*1)));
			mm_src_E = _mm_loadu_si128((__m128i *) (src_x+(stride*2)));
			TAP0_1_16(mm_src_B, mm_src_E, mm_sum_lo, mm_sum_hi, mm_tap1_factor);
			mm_src_A = _mm_loadu_si128((__m128i *) (src_x));
			mm_src_D = _mm_loadu_si128((__m128i *) (src_x+(stride)));
			TAP0_1_16(mm_src_A, mm_src_D, mm_sum_lo, mm_sum_hi, mm_tap0_factor);

			// clip and pack
			mm_sum_lo = _mm_add_epi32(mm_sum_lo, mm_rounding);
			mm_sum_lo = _mm_srai_epi32(mm_sum_lo, 10);
			mm_sum_lo = _mm_max_epi16(mm_sum_lo, mm_zero);		// if sum < 0 sum = 0
			mm_sum_lo = _mm_min_epi16(mm_sum_lo, mm_max_8bit);	// if sum > 255 sum = 255
			mm_sum_hi = _mm_add_epi32(mm_sum_hi, mm_rounding);
			mm_sum_hi = _mm_srai_epi32(mm_sum_hi, 10);
			mm_sum_hi = _mm_max_epi16(mm_sum_hi, mm_zero);		// if sum < 0 sum = 0
			mm_sum_hi = _mm_min_epi16(mm_sum_hi, mm_max_8bit);	// if sum > 255 sum = 255
			mm_result = _mm_packs_epi32(mm_sum_lo, mm_sum_hi);

			// second 8 pixels
			src_x += 8;
			mm_src_C = _mm_loadu_si128((__m128i *) (src_x-(stride*2)));
			mm_src_F = _mm_loadu_si128((__m128i *) (src_x+(stride*3)));
			TAP2_16(mm_src_C, mm_src_F, mm_sum_lo, mm_sum_hi);
			mm_src_B = _mm_loadu_si128((__m128i *) (src_x-(stride*1)));
			mm_src_E = _mm_loadu_si128((__m128i *) (src_x+(stride*2)));
			TAP0_1_16(mm_src_B, mm_src_E, mm_sum_lo, mm_sum_hi, mm_tap1_factor);
			mm_src_A = _mm_loadu_si128((__m128i *) (src_x));
			mm_src_D = _mm_loadu_si128((__m128i *) (src_x+(stride)));
			TAP0_1_16(mm_src_A, mm_src_D, mm_sum_lo, mm_sum_hi, mm_tap0_factor);

			// clip and pack
			mm_sum_lo = _mm_add_epi32(mm_sum_lo, mm_rounding);
			mm_sum_lo = _mm_srai_epi32(mm_sum_lo, 10);
			mm_sum_lo = _mm_max_epi16(mm_sum_lo, mm_zero);		// if sum < 0 sum = 0
			mm_sum_lo = _mm_min_epi16(mm_sum_lo, mm_max_8bit);	// if sum > 255 sum = 255
			mm_sum_hi = _mm_add_epi32(mm_sum_hi, mm_rounding);
			mm_sum_hi = _mm_srai_epi32(mm_sum_hi, 10);
			mm_sum_hi = _mm_max_epi16(mm_sum_hi, mm_zero);		// if sum < 0 sum = 0
			mm_sum_hi = _mm_min_epi16(mm_sum_hi, mm_max_8bit);	// if sum > 255 sum = 255
			mm_sum_lo = _mm_packs_epi32(mm_sum_lo, mm_sum_hi);

			mm_result = _mm_packus_epi16(mm_result, mm_sum_lo);

			_mm_store_si128((__m128i *) dst_x, mm_result);
		}
	}

#if 0 // verification
	const int tap2 = ONE_FOURTH_TAP[2];

	int16_t *srcImgA;
	int16_t *srcImgB;
	int16_t *srcImgC;
	int16_t *srcImgD;
	int16_t *srcImgE;
	int16_t *srcImgF;
	int sum;

	src = sLumaHalfPixelImage_Tmp + (gEncoderState.width_padded * FRAME_HEIGHT_PAD) + FRAME_WIDTH_PAD;
	dst = dummy;
	uint8_t result;

	for (int y = gEncoderState.height; y > 0; y--, dst += stride, src += stride)
	{
		srcImgA = src;
		srcImgB = src - stride;
		srcImgC = src - stride - stride;
		srcImgD = src + stride;
		srcImgE = src + stride + stride;
		srcImgF = src + stride + stride + stride;

		for (int x = 0; x < gEncoderState.width; x++)
		{
			sum = (tap0 * (*srcImgA + *srcImgD) +
				   tap1 * (*srcImgB + *srcImgE) +
				   tap2 * (*srcImgC + *srcImgF));

			result = CLIP1( rshift_rnd_sf( sum, 10 ) );
			ENC_ASSERT(dst[x] == result);

			srcImgA++;srcImgD++;srcImgB++;srcImgE++;srcImgC++;srcImgF++;
		}
	}
#endif
}
