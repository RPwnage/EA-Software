#include <stdio.h>

#include "Timing.h"

static bool sTimerInitialized = false;
static LARGE_INTEGER sTimingFreq;
static TimerType sTimers[TIMER_MAX];
static int sTimersUsed = 0;

FILE *gLogfile = NULL;

void InitLog()
{
	static int count = 0;

	char str[256];
	sprintf_s(str, "o264_log_%d.txt", count++);

	fopen_s(&gLogfile, str, "w+");
}

void ExitLog()
{
	if (gLogfile)
	{
		fflush(gLogfile);
		fclose(gLogfile);
	}
	gLogfile = NULL;
	
}

void Log(char *str)
{
	if (gLogfile && (str != NULL))
	{
		fprintf(gLogfile, str);
	}
}

void InitTimer()
{
	if (QueryPerformanceFrequency(&sTimingFreq))
		sTimerInitialized = true;

	for (int i = 0; i < TIMER_MAX; i++)
		sTimers[i].in_use = false;
}

void StartTimer(int index, char *label)
{
	if ((index < 0 ) || (index >= TIMER_MAX))
		return;

	TimerType *t = &sTimers[index];

	if (t->in_use == false)
	{
		t->in_use = true;
		t->total_time.QuadPart = 0;
		t->interval_time.QuadPart = 0;
		strcpy_s(t->label, label);
		t->num_intervals = 0;
		t->lap_intervals = 0;
		t->max_interval = 0;
		t->max_at_frame = 0;
	}
	else
	{
		if (!t->running)
		{
			ResumeTimer(index);
			return;
		}
	}
	t->running = true;

	QueryPerformanceCounter(&t->start_time);    // start time
}

void PauseTimer(int index, bool output)
{
	if ((index < 0 ) || (index >= TIMER_MAX))
		return;

	TimerType *t = &sTimers[index];

	if (t->running == false)
		return;

	t->running = false;
	QueryPerformanceCounter(&t->end_time);    // end of interval time
	t->total_time.QuadPart    += t->end_time.QuadPart - t->start_time.QuadPart;
	t->interval_time.QuadPart += t->end_time.QuadPart - t->start_time.QuadPart;
	float interval_time = static_cast<float>(t->interval_time.QuadPart) / sTimingFreq.QuadPart;
	if (interval_time > t->max_interval)
	{
		t->max_interval = interval_time;
		t->max_at_frame = t->lap_intervals;
	}

	if (output)
	{
		double interval = 1000.0f * static_cast<double>(t->end_time.QuadPart - t->start_time.QuadPart) / sTimingFreq.QuadPart;
		double total = 1000.0f * static_cast<double>(t->total_time.QuadPart) / sTimingFreq.QuadPart;
		char str[512];

		sprintf_s(str, "******************************************************************* Interval Timer: %22s : %8.4f / %8.4f ms\n", t->label, interval, total);
		OutputDebugString(str);
	}
}

float GetTimer_Time(int index, bool use_interval)
{
	if ((index < 0 ) || (index >= TIMER_MAX))
		return 0.0f;

	TimerType *t = &sTimers[index];

	if (t->running)
		QueryPerformanceCounter(&t->end_time);    // end of interval time

	float interval;

	if (use_interval)	// get the accumulated time for current lap
	{
		LARGE_INTEGER int_time = t->interval_time;
		if (t->running)
			int_time.QuadPart += (t->end_time.QuadPart - t->start_time.QuadPart);

		interval = (float) (1000.0 * static_cast<double>(int_time.QuadPart) / sTimingFreq.QuadPart);
	}
	else
		interval = (float) (1000.0 * static_cast<double>(t->end_time.QuadPart - t->start_time.QuadPart) / sTimingFreq.QuadPart);

	return interval;
}

void ResumeTimer(int index)
{
	if ((index < 0 ) || (index >= TIMER_MAX))
		return;

	TimerType *t = &sTimers[index];

	if (t->running == true)
		return;

	t->num_intervals++;
	t->running = true;

	QueryPerformanceCounter(&t->start_time);    // start time
}

void EndTimer(int index, bool output)
{
	if ((index < 0 ) || (index >= TIMER_MAX))
		return;

	TimerType *t = &sTimers[index];

	t->in_use = false;

	if (t->running)
	{
		QueryPerformanceCounter(&t->end_time);    // end time
		t->total_time.QuadPart = t->end_time.QuadPart - t->start_time.QuadPart;
		t->num_intervals++;
	}
	t->running = false;

	if (output)
	{
		double interval = 1000.0 * static_cast<double>(t->total_time.QuadPart) / sTimingFreq.QuadPart;
		char str[512];
		char disp_total_time[64];
		char disp_num_int[64];

		if (interval > 1000)
			sprintf_s(disp_total_time, "%8.3f s ", interval / 1000.0f);
		else
			sprintf_s(disp_total_time, "%8.3f ms", interval);

		if (t->num_intervals < 10000)
			sprintf_s(disp_num_int, "%4d ", t->num_intervals);
		else
			sprintf_s(disp_num_int, "%4dk", t->num_intervals/1000);

		if (t->lap_intervals > 0)
			t->num_intervals = t->lap_intervals;

		if (t->num_intervals > 1)
			sprintf_s(str, "Timer: %22s : %s # intervals: %s  avg time: %9.4f  max:%8.4f @ %d\n", t->label, disp_total_time, disp_num_int, interval / ((float) t->num_intervals), t->max_interval * 1000.0f, t->max_at_frame);
		else
			sprintf_s(str, "Timer: %22s : %s\n", t->label, disp_total_time);
		OutputDebugString(str);
		Log(str);
	}
}

void LapTimer(int index)
{
	if ((index < 0 ) || (index >= TIMER_MAX))
		return;

	TimerType *t = &sTimers[index];

	t->interval_time.QuadPart = 0;
	t->lap_intervals++;
}