#ifndef __TIMING_H
#define __TIMING_H

#include <windows.h>

#define TIMER_MAX 64
#define TIMER_MAX_LABEL_SIZE 256

typedef struct  
{
	char label[TIMER_MAX_LABEL_SIZE];
	bool in_use;
	bool running;
	int num_intervals;
	int lap_intervals;
	int max_at_frame;
	float max_interval;
	LARGE_INTEGER start_time;
	LARGE_INTEGER end_time;
	LARGE_INTEGER total_time;
	LARGE_INTEGER interval_time;
} TimerType;

#define HIGH_LEVEL_TIMERS_ENABLED
#ifdef HIGH_LEVEL_TIMERS_ENABLED
#define ENC_START_HL_TIMER(i,label) StartTimer((i),(label));
#define ENC_END_HL_TIMER(i,output) EndTimer((i),(output));
#define ENC_PAUSE_HL_TIMER(i,output) PauseTimer((i),(output));
#define ENC_GET_HL_TIMER(i) GetTimer_Time((i),false);
#else
#define ENC_START_HL_TIMER(i,label) 
#define ENC_END_HL_TIMER(i,output) 
#define ENC_PAUSE_HL_TIMER(i,output) 
#define ENC_GET_HL_TIMER(i) 0.0f;
#endif

#define ENC_TIMERS_ENABLED

#ifdef ENC_TIMERS_ENABLED
#define ENC_START_TIMER(i,label) StartTimer((i),(label));
#define ENC_END_TIMER(i,output) EndTimer((i),(output));
#define ENC_PAUSE_TIMER(i,output) PauseTimer((i),(output));
#define ENC_RESUME_TIMER(i) ResumeTimer((i));
#define ENC_LAP_TIMER(i) LapTimer((i));
#define ENC_GET_TIMER(i,interval) GetTimer_Time((i),(interval));
#else
#define ENC_START_TIMER(i,label) 
#define ENC_END_TIMER(i,output) 
#define ENC_PAUSE_TIMER(i,output) 
#define ENC_RESUME_TIMER(i) 
#define ENC_LAP_TIMER(i) 
#define ENC_GET_TIMER(i,interval) 0.0f;
#endif

void InitLog();
void ExitLog();
void Log(char *str);

void InitTimer();
void StartTimer(int index, char *label);
void PauseTimer(int index, bool output);
void ResumeTimer(int index);
void EndTimer(int index, bool output);
void LapTimer(int index);
float GetTimer_Time(int index, bool interval);
#endif