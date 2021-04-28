#ifndef __MOTIONESTIMATION_H
#define __MOTIONESTIMATION_H

#include <stdint.h>
#include "Frame.h"
#include "MotionInfo.h"

#define SEARCH_RADIUS 32
#define MV_MAX	1024

//#define USE_QUARTER_PIXEL_ME

struct MacroBlock;

typedef struct
{
    MotionVector motion;
    // info for next search iteration
    int startIndex; // index of SearchPoint to start at
    int numberOfPoints; // number of SearchPoints
} SearchPoint;

typedef struct TemporalMV
{
	// pre-scaled (256 / (frame dist))
	int mx;	
	int my;
} TemporalMV_Type;

struct searchPatternStruct
{
    int searchPoints; // number of search points
    SearchPoint *points;
    int stopSearch;
    int nextLast;
    searchPatternStruct *nextpattern;
};

typedef struct searchPatternStruct SearchPattern;

typedef struct 
{
    int min_x;
    int max_x;
    int min_y;
    int max_y;

    void setRange(int center_x, int center_y)
    {
        min_x = center_x - SEARCH_RADIUS;
        max_x = center_x + SEARCH_RADIUS;
        min_y = center_y - SEARCH_RADIUS;
        max_y = center_y + SEARCH_RADIUS;
    }

    bool inRange(int x, int y)
    {
        return ( x >= min_x && x <= max_x && y >= min_y && y <= max_y );
    }
} SearchWindow;

#define PREDICTORS_MAX 128
typedef struct
{
    MotionVector point[PREDICTORS_MAX];
    int count;
    int index;
} PredictorList;

void ME_InitMotionEstimation(int type);
void ME_ExitMotionEstimation();

void ME_InitSearchPatterns();
void ME_InitSearchWindow();
int ME_ComputeMotionVector(MacroBlock *mb, YUVFrameType *frame, YUVFrameType *ref_frame, MotionVector *mv, int *mad, bool quick_mode = false);
#ifdef USE_SSE2
int ComputeSAD_SSE2(uint8_t *prev, int stride_p, uint8_t *buffer, int stride_b, bool luma);
#endif
int ComputeSAD(uint8_t *prev, uint8_t *buffer, int stride, bool luma);
int ComputeSAD_SSE2_x(uint8_t *prev, int stride_p, uint8_t *buffer);

enum	// these are bit flags now
{
	ME_PRED_DIR_BACK = 0,
	ME_PRED_DIR_FORWARD,
	ME_PRED_SKIP = 2		// set when we want to copy to the skip buffers instead of the prediction buffers
};

enum
{
	ME_HALF_PIXEL_PLANE_H = 1,
	ME_HALF_PIXEL_PLANE_V = 2,
	ME_HALF_PIXEL_PLANE_D = 3
};

void ME_Prediction(MacroBlock *mb, int flags);

void ME_Update();
void ME_SavePSkipMV(MacroBlock *mb, int pmv_x, int pmv_y);

extern uint8_t * gLumaHalfPixelImage_H;
extern uint8_t * gLumaHalfPixelImage_V;
extern uint8_t * gLumaHalfPixelImage_D;

void ME_InitSubPixelImages();
void ME_ExitSubPixelImages();
void ME_ComputeSubPixelImages(int type);
void SetHalfPixelFrameRef(YUVFrameType *frame);

void CalcHalfPixelPlane_H(uint8_t *dst, uint8_t *src, int stride);
void CalcHalfPixelPlane_V(uint8_t *dst, uint8_t *src, int stride);
void CalcHalfPixelPlane_D(uint8_t *dst, uint8_t *src, int stride);

void ME_SetRefFrame_TemporalMV(YUVFrameType *frame);

#endif