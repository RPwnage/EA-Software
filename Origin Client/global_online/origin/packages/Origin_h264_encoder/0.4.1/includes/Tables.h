#ifndef __TABLES_H
#define __TABLES_H

#include <stdint.h>

extern int16_t gABS_Table[];	// for fast lookup of 16 bit signed values to absolute values
extern uint8_t gCoeffBitCostTable[];

void InitTables();
const int8_t *TABLE_GetCABAC_InitTable(int index);

#define IMIN(x,y) (((x) < (y)) ? (x) : (y))
#define IMAX(x,y) (((x) > (y)) ? (x) : (y))
#define CLIP1(x) (((x) > (255)) ? 255 : (((x) < 0) ? 0 : (x)))
#define CLIP3(min,max,value) (((value) <= (min))?(min):((value)>=(max))?(max):(value))

#if 0
#define ABS(x) ((x)^((x) >> 31) - ((x) >> 31))	// fast way (no branch)
#else
#define ABS(x) (((x) < 0) ? -(x) : (x))
#endif

#endif
