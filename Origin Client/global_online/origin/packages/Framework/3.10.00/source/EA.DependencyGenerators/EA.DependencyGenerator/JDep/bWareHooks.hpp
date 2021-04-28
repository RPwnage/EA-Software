//
// bWareHooks.hpp
//
// I pasted selected bWare functions here because I couldn't get the library to link.  Grumble
//

#include "bChunk.hpp"

// From bCrc32.cpp
unsigned int bCalculateCrc32(const void *data, int size, unsigned int prev_crc32 = ~0);

// From bWare\Indep\Src\Tools\Parse.cpp
long FileTime(const char *filename);
void *ReadEntireFile(const char *filename, int *size);
long WriteChunkHeader(FILE *f, long chunk_id);
void WriteChunkSize(FILE *f, long chunk_start);

// From bDebug.cpp
void bInitTicker(float min_wraparound_time);
unsigned int bGetTicker(void);
float bGetTickerDifference(unsigned int start_ticks, unsigned int end_ticks);
inline float bGetTickerDifference(unsigned int start_ticks)
{
	return bGetTickerDifference(start_ticks, bGetTicker());
}

