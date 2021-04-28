#include "MotionInfo.h"
#include "Origin_h264_encoder.h"
#include "Tables.h"

// keep all previously computed motion vectors
// - for P frames only (i.e., one reference frame list)
MotionInfo*** sMotionInfo = NULL; // sMotionInfo[frame][row][col]
int gMotionInfoIndex = 0;

// Store SAD values for adaptive thresholding (EPZS)
int ***sDistortion = NULL;

// map from index (0 .. total_macroblocks - 1) to (x, y) coordinates of 16x16 macroblocks
// TODO: probably can use 'gEncoderState.mb_set[index]->mb_x' and 'gEncodeState.mb_set[index]->mb_y'
BlockPos *sPicPos = NULL;

#define MB_BLOCK_SIZE 16
#define BLOCK_SIZE 4

void InitDistortion()
{
    int numRefLists = 2;
    int numBlockTypes = 8;
    int num4x4BlocksPerRow = (gEncoderState.width + MB_BLOCK_SIZE)/BLOCK_SIZE;
    sDistortion = (int ***) malloc( numRefLists * sizeof(int**) );
    for( int i = 0; i < numRefLists; ++i )
    {
        sDistortion[i] = (int **) malloc( (numBlockTypes - 1) * sizeof(int*) );
        for( int j = 0; j < (numBlockTypes - 1); ++j )
        {
            sDistortion[i][j] = (int *) malloc( num4x4BlocksPerRow * sizeof(int) );
            for( int k = 0; k < num4x4BlocksPerRow; ++k )
            {
                sDistortion[i][j][k] = 0;
            }
        }
    }
}

void FreeDistortion()
{
    // todo
}

void InitPicPos()
{
    sPicPos = (BlockPos *) malloc( (gEncoderState.mb_max + 1) * sizeof(BlockPos));

    for (int j = 0; j < (int) gEncoderState.mb_max + 1; j++)
    {
        sPicPos[j].x = (short) (j % gEncoderState.mb_width);
        sPicPos[j].y = (short) (j / gEncoderState.mb_width);
    }
}

void InitMotionInfo()
{
    // get number of 4x4 blocks (i.e., size of blocks at which only one motion vector can exist)
    int rows = gEncoderState.height / 4;
    int cols = gEncoderState.width / 4;
    sMotionInfo = (MotionInfo***) malloc( MOTION_VECTOR_FRAMES_MAX * sizeof(MotionInfo**) );
    for( int f = 0; f < MOTION_VECTOR_FRAMES_MAX; ++f )
    {
        sMotionInfo[f] = (MotionInfo**) malloc( rows * sizeof(MotionInfo*) );
        for( int r = 0; r < rows; ++r )
        {
            sMotionInfo[f][r] = (MotionInfo*) malloc( cols * sizeof(MotionInfo) );
            for( int c = 0; c < cols; ++c )
            {
                sMotionInfo[f][r][c].mv[0].mv_x = 0;
                sMotionInfo[f][r][c].mv[0].mv_y = 0;
                sMotionInfo[f][r][c].mv[1].mv_x = 0;
                sMotionInfo[f][r][c].mv[1].mv_y = 0;
                sMotionInfo[f][r][c].ref_idx[0] = 0;
                sMotionInfo[f][r][c].ref_idx[1] = 0;
            }
        }
    }
}

void FreeMotionInfo()
{
    int rows = gEncoderState.height / 4;
    int cols = gEncoderState.width / 4;

    for( int f = 0; f < MOTION_VECTOR_FRAMES_MAX; ++f )
    {
        for( int r = 0; r < rows; ++r )
        {
            for( int c = 0; c < cols; ++c )
            {
                free (&sMotionInfo[f][r][c]);
            }
            free (&sMotionInfo[f][r]);
        }
        free (&sMotionInfo[f]);
    }
}

void MI_Init()
{
    InitPicPos();
    InitDistortion();
}

static inline int GetMedianValue(int x, int y, int z)
{
	if (x < y)
	{
		if (y < z)
			return y;
		else
			if (x < z)
				return z;
			else
				return x;
	}
	else
	{	// x >= y
		if (z > x)
			return x;
		else
			if (y > z)
				return y;
			else
				return z;
	}
}

bool CalcMPredVector(MacroBlockType *mb, int *pmv_x, int *pmv_y)
{
	// from Clause 8.4.1.3
	*pmv_x = 0;
	*pmv_y = 0;

	if (mb->mb_skip_flag)
	{	// another damn special case - clause 8.4.1.1
		if ((mb->mbAddrA == NULL) || (mb->mbAddrB == NULL) || 
			((mb->mbAddrA->data->refIdxL0 == 0) && (mb->mbAddrA->data->mv_l0[0][0][0] == 0) && (mb->mbAddrA->data->mv_l0[1][0][0] == 0)) ||
			((mb->mbAddrB->data->refIdxL0 == 0) && (mb->mbAddrB->data->mv_l0[0][0][0] == 0) && (mb->mbAddrB->data->mv_l0[1][0][0] == 0)))
			return true;		// set to (0,0)
	}

	// weirdness - i guess if mbAddrC is not available, use mbAddrD (up and to the left)
	MacroBlockType *third_neighbor = (mb->mbAddrC == NULL) ? mb->mbAddrD : mb->mbAddrC;

	if ((mb->mbAddrB == NULL) && (third_neighbor == NULL))	// up and up-right not available?
	{
		if (mb->mbAddrA && (mb->mbAddrA->mb_skip_flag || (mb->mbAddrA->mb_type < MB_TYPE_P_INxN)))
		{
			*pmv_x = mb->mbAddrA->data->mv_l0[0][0][0];	// not supporting anything but P_L0_16x16 currently
			*pmv_y = mb->mbAddrA->data->mv_l0[1][0][0];	// 
		}
	}
	else
	{
		int16_t mv_array[3][2];
		int num_available = 0;
		int avail_index = 0;
		if (mb->mbAddrA && (mb->mbAddrA->mb_skip_flag || (mb->mbAddrA->mb_type < MB_TYPE_P_INxN)))
		{
			mv_array[0][0] = mb->mbAddrA->data->mv_l0[0][0][0];
			mv_array[0][1] = mb->mbAddrA->data->mv_l0[1][0][0];
			num_available++;
		}
		else
		{
			mv_array[0][0] = mv_array[0][1] = 0;
		}
		if (mb->mbAddrB && (mb->mbAddrB->mb_skip_flag || (mb->mbAddrB->mb_type < MB_TYPE_P_INxN)))
		{
			mv_array[1][0] = mb->mbAddrB->data->mv_l0[0][0][0];
			mv_array[1][1] = mb->mbAddrB->data->mv_l0[1][0][0];
			avail_index = 1;
			num_available++;
		}
		else
		{
			mv_array[1][0] = mv_array[1][1] = 0;
		}
		if (third_neighbor && (third_neighbor->mb_skip_flag || (third_neighbor->mb_type < MB_TYPE_P_INxN)))
		{
			mv_array[2][0] = third_neighbor->data->mv_l0[0][0][0];
			mv_array[2][1] = third_neighbor->data->mv_l0[1][0][0];
			avail_index = 2;
			num_available++;
		}
		else
		{
			mv_array[2][0] = mv_array[2][1] = 0;
		}

		if (num_available == 0)
			return false;

		if (num_available == 1)
		{
			*pmv_x = mv_array[avail_index][0];
			*pmv_y = mv_array[avail_index][1];
			return false;
		}
		*pmv_x = GetMedianValue(mv_array[0][0], mv_array[1][0], mv_array[2][0]);
		*pmv_y = GetMedianValue(mv_array[0][1], mv_array[1][1], mv_array[2][1]);
	}

	return false;
}


bool CalcMPredVector_B_L0(MacroBlockType *mb, int *pmv_x, int *pmv_y)
{
	// from Clause 8.4.1.3
	*pmv_x = 0;
	*pmv_y = 0;

	// weirdness - i guess if mbAddrC is not available, use mbAddrD (up and to the left)
	MacroBlockType *third_neighbor = (mb->mbAddrC == NULL) ? mb->mbAddrD : mb->mbAddrC;

	if ((mb->mbAddrB == NULL) && (third_neighbor == NULL))	// up and up-right not available?
	{
		if (mb->mbAddrA && ((mb->mbAddrA->mb_skip_flag || (mb->mbAddrA->mb_type < MB_TYPE_B_INxN)) && (mb->mbAddrA->data->refIdxL0 != -1)))
		{
			*pmv_x = mb->mbAddrA->data->mv_l0[0][0][0];	// 
			*pmv_y = mb->mbAddrA->data->mv_l0[1][0][0];	// 
		}
	}
	else
	{
		int16_t mv_array[3][2];
		int num_available = 0;
		int avail_index = 0;
		if (mb->mbAddrA && ((mb->mbAddrA->mb_skip_flag || (mb->mbAddrA->mb_type < MB_TYPE_B_INxN)) && (mb->mbAddrA->data->refIdxL0 != -1)))
		{
			mv_array[0][0] = mb->mbAddrA->data->mv_l0[0][0][0];
			mv_array[0][1] = mb->mbAddrA->data->mv_l0[1][0][0];
			num_available++;
		}
		else
		{
			mv_array[0][0] = mv_array[0][1] = 0;
		}
		if (mb->mbAddrB && ((mb->mbAddrB->mb_skip_flag || (mb->mbAddrB->mb_type < MB_TYPE_B_INxN)) && (mb->mbAddrB->data->refIdxL0 != -1)))
		{
			mv_array[1][0] = mb->mbAddrB->data->mv_l0[0][0][0];
			mv_array[1][1] = mb->mbAddrB->data->mv_l0[1][0][0];
			avail_index = 1;
			num_available++;
		}
		else
		{
			mv_array[1][0] = mv_array[1][1] = 0;
		}
		if (third_neighbor && ((third_neighbor->mb_skip_flag || (third_neighbor->mb_type < MB_TYPE_B_INxN)) && (third_neighbor->data->refIdxL0 != -1)))
		{
			mv_array[2][0] = third_neighbor->data->mv_l0[0][0][0];
			mv_array[2][1] = third_neighbor->data->mv_l0[1][0][0];
			avail_index = 2;
			num_available++;
		}
		else
		{
			mv_array[2][0] = mv_array[2][1] = 0;
		}

		if (num_available == 0)
			return false;

		if (num_available == 1)
		{
			*pmv_x = mv_array[avail_index][0];
			*pmv_y = mv_array[avail_index][1];
			return false;
		}
		*pmv_x = GetMedianValue(mv_array[0][0], mv_array[1][0], mv_array[2][0]);
		*pmv_y = GetMedianValue(mv_array[0][1], mv_array[1][1], mv_array[2][1]);
	}

	return false;
}


bool CalcMPredVector_B_L1(MacroBlockType *mb, int *pmv_x, int *pmv_y)
{
	// from Clause 8.4.1.3
	*pmv_x = 0;
	*pmv_y = 0;

	// weirdness - i guess if mbAddrC is not available, use mbAddrD (up and to the left)
	MacroBlockType *third_neighbor = (mb->mbAddrC == NULL) ? mb->mbAddrD : mb->mbAddrC;

	if ((mb->mbAddrB == NULL) && (third_neighbor == NULL))	// up and up-right not available?
	{
		if (mb->mbAddrA && ((mb->mbAddrA->mb_skip_flag || (mb->mbAddrA->mb_type < MB_TYPE_B_INxN)) && (mb->mbAddrA->data->refIdxL1 != -1)))
		{
			*pmv_x = mb->mbAddrA->data->mv_l1[0][0][0];	//
			*pmv_y = mb->mbAddrA->data->mv_l1[1][0][0];	// 
		}
	}
	else
	{
		int16_t mv_array[3][2];
		int num_available = 0;
		int avail_index = 0;
		if (mb->mbAddrA && ((mb->mbAddrA->mb_skip_flag || (mb->mbAddrA->mb_type < MB_TYPE_B_INxN)) && (mb->mbAddrA->data->refIdxL1 != -1)))
		{
			mv_array[0][0] = mb->mbAddrA->data->mv_l1[0][0][0];
			mv_array[0][1] = mb->mbAddrA->data->mv_l1[1][0][0];
			num_available++;
		}
		else
		{
			mv_array[0][0] = mv_array[0][1] = 0;
		}
		if (mb->mbAddrB && ((mb->mbAddrB->mb_skip_flag || (mb->mbAddrB->mb_type < MB_TYPE_B_INxN)) && (mb->mbAddrB->data->refIdxL1 != -1)))
		{
			mv_array[1][0] = mb->mbAddrB->data->mv_l1[0][0][0];
			mv_array[1][1] = mb->mbAddrB->data->mv_l1[1][0][0];
			avail_index = 1;
			num_available++;
		}
		else
		{
			mv_array[1][0] = mv_array[1][1] = 0;
		}
		if (third_neighbor && ((third_neighbor->mb_skip_flag || (third_neighbor->mb_type < MB_TYPE_B_INxN)) && (third_neighbor->data->refIdxL1 != -1)))
		{
			mv_array[2][0] = third_neighbor->data->mv_l1[0][0][0];
			mv_array[2][1] = third_neighbor->data->mv_l1[1][0][0];
			avail_index = 2;
			num_available++;
		}
		else
		{
			mv_array[2][0] = mv_array[2][1] = 0;
		}

		if (num_available == 0)
			return false;

		if (num_available == 1)
		{
			*pmv_x = mv_array[avail_index][0];
			*pmv_y = mv_array[avail_index][1];
			return false;
		}
		*pmv_x = GetMedianValue(mv_array[0][0], mv_array[1][0], mv_array[2][0]);
		*pmv_y = GetMedianValue(mv_array[0][1], mv_array[1][1], mv_array[2][1]);
	}

	return false;
}

// used for BSKIP calc
bool CalcMPredVector_Direct_Spatial(MacroBlockType *mb, int *pmv_L0, int *pmv_L1, int *ref, YUVFrameType *frame)
{
	// from Clause 8.4.1.3
	pmv_L0[0] = pmv_L0[1] = 0;
	pmv_L1[0] = pmv_L1[1] = 0;

	MacroBlockType *mb_a = mb->mbAddrA;
	MacroBlockType *mb_b = mb->mbAddrB;
	// weirdness - i guess if mbAddrC is not available, use mbAddrD (up and to the left)
	MacroBlockType *third_neighbor = (mb->mbAddrC == NULL) ? mb->mbAddrD : mb->mbAddrC;

	// first find lowest positive ref indices 
	int ref_l0 = -1, ref_l1 = -1;

	if (mb_a)
	{
		if (mb_a->data->refIdxL0 != -1)
			ref_l0 = mb_a->data->refIdxL0;
		if (mb_a->data->refIdxL1 != -1)
			ref_l1 = mb_a->data->refIdxL1;
	}

	if (mb_b)
	{
		ref_l0 = IMIN((unsigned int) ref_l0, (unsigned int) mb_b->data->refIdxL0);
		ref_l1 = IMIN((unsigned int) ref_l1, (unsigned int) mb_b->data->refIdxL1);
	}

	if (third_neighbor)
	{
		ref_l0 = IMIN((unsigned int) ref_l0, (unsigned int) third_neighbor->data->refIdxL0);
		ref_l1 = IMIN((unsigned int) ref_l1, (unsigned int) third_neighbor->data->refIdxL1);
	}

	// check for crazy movement rules
	if ((ref_l0 == -1) && (ref_l1 == -1))
	{
		ref[0] = 0;
		ref[1] = 0;
	}
	else
	{	// TODO: assume for now we only use 1 reference frame in each direction
		// for (ref_l0 == 0) || (ref_l1 == 0)
		bool is_moving_block;

		if (ref_l0 != -1)
			CalcMPredVector_B_L0(mb, &pmv_L0[0], &pmv_L0[1]);
		if (ref_l1 != -1)
			CalcMPredVector_B_L1(mb, &pmv_L1[0], &pmv_L1[1]);

		// TODO: only dealing with 16x16 for now
		MBDataType *data = frame->next_ref_frame->mb_set[mb->mb_index].data;
		is_moving_block = (((data->refIdxL0 == 0) && ((ABS(data->mv_l0[0][0][0]) >> 1) == 0) && ((ABS(data->mv_l0[1][0][0]) >> 1) == 0)) || 
						   ((data->refIdxL1 == 0) && ((ABS(data->mv_l1[0][0][0]) >> 1) == 0) && ((ABS(data->mv_l1[1][0][0]) >> 1) == 0) && (data->refIdxL0 == -1)));

		// get normal median vectors
		if (ref_l0 != -1)
		{
			if (is_moving_block)
			{
				pmv_L0[0] = pmv_L0[1] = 0;
			}

			ref[0] = ref_l0;
		}
		else
		{	// pmv is already zero here since we didn't call CalcMPredVector_B_Lx()
			ref[0] = -1;
		}

		if (ref_l1 != -1)
		{
			if (is_moving_block)
			{
				pmv_L1[0] = pmv_L1[1] = 0;
			}

			ref[1] = ref_l1;
		}
		else
		{	// pmv is already zero here since we didn't call CalcMPredVector_B_Lx()
			ref[1] = -1;
		}
	}

	return false;
}
