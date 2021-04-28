///////////////////////////////////////////////////////////////////////////////
// RateControl.h
// 
// Created by Mark Fong
// Copyright (c) 2014 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef __RATECONTROL_H
#define __RATECONTROL_H

#include <stdint.h>

#define RATE_CONTROL_HISTORY_SIZE 21

#define GAMMAP 0.5f
#define BETAP  0.5f
#define THETA  1.3636F;

typedef struct  
{
	int qp;
	int P_qp;
	int starting_qp;
	int initial_qp;	// base qp calculated from bitrate / fps
	int last_qp;		// from last P slice
	int prev_last_qp;	// from last last P slice (used for judging change for B slices)

	int target_bits;
	int frame_bits;
	int residual_bits;
	int header_bits;

	int total_num_of_P_frames;

	int num_of_P_frames;
	int num_of_B_frames;

	int current_num_of_P_frames;	// in local GOP
	int current_num_of_B_frames;	// in local GOP
	int total_Qp_for_P_frames;	
	int qp_of_last_group;

	int prev_window_size;
	int prev_MAD_window_size;

	int bound_min;
	int bound_max1;
	int bound_max2;

	int64_t bits_remaining;
	int64_t frame_group_size;	// bits used in current group of frames
	float target_group_size;
	float target_group_size_delta;
		
	float bitrate;
	float prev_bitrate;
	float frame_rate;
	float bits_per_frame;

	int P_Complexity;
	float P_ComplexityWeight;
	int B_Complexity;
	float B_ComplexityWeight;

	float X1;
	float X2;
	float P_X1;
	float P_X2;

	float MAD;
	float prev_MAD;
	float MAD_C1;
	float MAD_C2;
	float P_MAD_C1;
	float P_MAD_C2;

	float MAD_Slice[RATE_CONTROL_HISTORY_SIZE];
	float MAD_P_Slice[RATE_CONTROL_HISTORY_SIZE];
	float MAD_Reference[RATE_CONTROL_HISTORY_SIZE];
	float Qp_History[RATE_CONTROL_HISTORY_SIZE];
	float Rp_History[RATE_CONTROL_HISTORY_SIZE];
	float P_Qp_History[RATE_CONTROL_HISTORY_SIZE];
	float P_Rp_History[RATE_CONTROL_HISTORY_SIZE];

	int num_of_MB_samples;	// for estimating current frame complexity
	float estimate_extrapolation_factor;
	float frame_complexity_estimate;
	float frame_luma_coeff_cost_estimate;

	int idr_timestamp;	// tick count at last IDR

	float total_MAD;
	int total_QP;
	int total_IP_QP;	// QP for just I and P frames
	int total_num_frames;
	int total_num_IP_frames;
} RateControlType;

void InitRateControl();
void ExitRateControl();
int  RateControl_GetFrameQP();
void RateControl_Init_Frame();
int  RateControl_UpdateQP();
void RateControl_UpdateFrame(int frame_bits, int residual_bits);
void RateControl_EstimateFrame();
void RateControl_DropFrames(int num_dropped);

#endif