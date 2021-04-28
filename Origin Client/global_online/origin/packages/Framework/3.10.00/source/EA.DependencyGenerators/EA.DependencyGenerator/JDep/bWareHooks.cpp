//
// bWareHooks.cpp
//
// I pasted selected bWare functions here because I couldn't get the library to link.  Grumble
//

#include "stdafx.h"
#include <Windows.h>

#include <sys/stat.h>	// For FileTime()

#include "bChunk.hpp"


// From bWare\Indep\Src\Tools\Parse.cpp
long FileTime(const char *filename)
{
	struct _stat s; 
	if(filename)
	{
		_stat(filename, &s);
		return static_cast<long>(s.st_mtime);
	}
	return 0; 
}

// From bWare\Indep\Src\Tools\Parse.cpp
void *ReadEntireFile(const char *filename, int *size)
{
	FILE *file = fopen(filename, "rb");
	if (!file)
	{
		*size = 0;
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	long filesize = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *buffer = new char[filesize];

	size_t num_read = fread(buffer, 1, filesize, file);

	fclose(file);

	if (size) *size = filesize;
	return (char *)buffer;
}

// From bWare\Indep\Src\Tools\Parse.cpp
long WriteChunkHeader(FILE *f, long chunk_id)
{
	long chunk_start = ftell(f);

	bChunk chunk;
	chunk.ID = chunk_id;
	chunk.Size = 0;	// Filled in by WriteChunkSize
	fwrite(&chunk, 1, sizeof(chunk), f);

	return chunk_start;
}

// From bWare\Indep\Src\Tools\Parse.cpp
void WriteChunkSize(FILE *f, long chunk_start)
{
	long chunk_end = ftell(f);
	fseek(f, chunk_start + 4, SEEK_SET);
	long size = chunk_end - chunk_start - sizeof(bChunk);
	long aligned_size = (size + 3) & ~3;

	fwrite(&aligned_size, 1, 4, f);
	
	fseek(f, chunk_end, SEEK_SET);
	if (aligned_size - size)
	{
		long zero = 0;
		fwrite(&zero, 1, aligned_size - size, f);
	}
}

static float TicksToMilliseconds;
static int TickerShift = 0;

void bInitTicker(float min_wraparound_time)
{
	LARGE_INTEGER ticker_frequency;
	QueryPerformanceFrequency( &ticker_frequency );
	TicksToMilliseconds = 1000.0f / (float)ticker_frequency.QuadPart;

	// bGetTicker() returns 32 bits, but we have 64 bits available.  We want to
	// optimize the range so we don't get wraparound.
	float wraparound_time = TicksToMilliseconds * (float)0x7fffffff;
	//float min_wraparound_time = 60.0f * 1000.0f;
	//bAssert(wraparound_time > 0.0f);

	TickerShift = 0;
	while (wraparound_time < min_wraparound_time)	
	{
		TickerShift++;
		wraparound_time = wraparound_time * 2.0f;
	}
}

unsigned int bGetTicker(void)
{
	// This is bad.  bGetTicker() needs to be called from within an exception handler (at least on XBOX)
	// and bInitTicker() does things that aren't good from within an exception handler.
	/*
	if (TicksToMilliseconds == 0.0f)
		bInitTicker();
	*/

	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);

	return (int)(time.QuadPart >> TickerShift);	
}

float bGetTickerDifference(unsigned int start_ticks, unsigned int end_ticks)
{
	if (TicksToMilliseconds == 0.0f)
	{
		bInitTicker(10.0f);
		return 0.f;
	}
	unsigned int ticks;

	if ( start_ticks <= end_ticks )
	{
		ticks = end_ticks - start_ticks;
	}
	else
	{
		ticks = (0xffffffff - start_ticks) + end_ticks + 1;
	}

	return ((float)ticks * TicksToMilliseconds * (1 << TickerShift));
}

