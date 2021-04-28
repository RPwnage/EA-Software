#ifndef __MB_PREDICTION_H
#define __MB_PREDICTION_H

#include <stdint.h>
#include "MacroBlock.h"
#include "Origin_h264_encoder.h"
#ifdef USE_SSE2
#include <emmintrin.h>
#endif

int LumaPrediction(MacroBlockType *block);
int ChromaPrediction(MacroBlockType *block);

#endif
