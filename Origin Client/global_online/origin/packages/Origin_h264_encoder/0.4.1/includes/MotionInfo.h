#ifndef __MOTIONINFO_H
#define __MOTIONINFO_H

#include <stdlib.h>
#include <stdint.h>

#define MOTION_VECTOR_FRAMES_MAX 3

struct MacroBlock;

typedef struct
{
    short x;
    short y;
} BlockPos;


typedef struct motion_vector
{
    short mv_x;
    short mv_y;

    motion_vector() : mv_x(0), mv_y(0) {}
} MotionVector;

typedef struct 
{
    MotionVector mv[2];
    char ref_idx[2]; // two reference lists
} MotionInfo;

extern MotionInfo*** sMotionInfo;
extern int gMotionInfoIndex;
extern int*** sDistortion;

void MI_Init();

bool CalcMPredVector(struct MacroBlock *mb, int *pmv_x, int *pmv_y);
bool CalcMPredVector_B_L0(struct MacroBlock *mb, int *pmv_x, int *pmv_y);
bool CalcMPredVector_B_L1(struct MacroBlock *mb, int *pmv_x, int *pmv_y);
bool CalcMPredVector_Direct_Spatial(struct MacroBlock *mb, int *pmv_L0, int *pmv_L1, int *ref, struct FRAME_STRUCT *frame);


#endif