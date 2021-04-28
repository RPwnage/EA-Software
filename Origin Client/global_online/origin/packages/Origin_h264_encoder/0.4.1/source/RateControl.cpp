///////////////////////////////////////////////////////////////////////////////
// RateControl.cpp
// 
// Created by Mark Fong
// Copyright (c) 2014 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "math.h"
#include "Origin_h264_encoder.h"
#include "RateControl.h"
#include "Tables.h"
#include "Trace.h"
#include "Timing.h"

#define LOCAL_QP_MIN 0
#define LOCAL_QP_MAX 51

#define RATE_QP_BIAS 0.5f	// factor to "pull-back" over-aggresive qp to initial_qp (1.0 - set to initial_qp, 0.0 - leave it as-is) when bit_remaining is running close to low

#define MAX_MB_SAMPLES	100
#define MB_SAMPLES_PERCENTAGE	0.01f

#define EST_ACCURACY_FACTOR 0.08f	// might make this dynamic

#define B_STEP_OFFSET 0

static RateControlType sRateControl;
static int sRateControl_Sample_MB_Index[MAX_MB_SAMPLES];

void InitRateControl()
{
	sRateControl.total_num_frames = 0;
	sRateControl.total_num_IP_frames = 0;

	sRateControl.total_MAD = 0.0f;
	sRateControl.total_QP = 0;
	sRateControl.total_IP_QP = 0;

	sRateControl.bitrate = (float) gEncoderState.bit_rate * 1000.0f;
	sRateControl.frame_rate = (float) gEncoderState.fps;
	sRateControl.bits_per_frame = floor(sRateControl.bitrate / sRateControl.frame_rate + 0.5f);

	sRateControl.bits_remaining = 0;
	sRateControl.frame_group_size = 0;
	sRateControl.target_group_size = 0.0f;

	sRateControl.prev_window_size = 0;
	sRateControl.prev_MAD_window_size = 0;

	sRateControl.bound_min = 0;
	sRateControl.bound_max1 = 0x7fffffff;
	sRateControl.bound_max2 = 0x7fffffff;

	sRateControl.P_X1 = sRateControl.bitrate;
	sRateControl.P_X2 = 0.0f;

	sRateControl.prev_MAD = 1.0f;
	sRateControl.P_MAD_C1 = 1.0f;
	sRateControl.P_MAD_C2 = 0.0f;

	sRateControl.total_Qp_for_P_frames = 0;
	sRateControl.qp_of_last_group = -1;	// mark as first time

	// Initialize values
	for(int i = 0; i < RATE_CONTROL_HISTORY_SIZE; i++)
	{
		sRateControl.P_Qp_History[i] = 0;
		sRateControl.P_Rp_History[i] = 0.0f;
		sRateControl.MAD_P_Slice[i] = 0.0f;
	}

	//if (1)	// calculate starting qp
	{
		double L1, L2, L3, bits_per_pixel;

		bits_per_pixel = sRateControl.bitrate /(sRateControl.frame_rate * gEncoderState.width * gEncoderState.height);

		L1 = 0.6;
		L2 = 1.4;
		L3 = 2.4;

		if (bits_per_pixel <= L1)
			sRateControl.starting_qp = 35;
		else if(bits_per_pixel <= L2)
			sRateControl.starting_qp = 25;
		else if(bits_per_pixel <= L3)
			sRateControl.starting_qp = 20;
		else
			sRateControl.starting_qp = 10;

		sRateControl.initial_qp = sRateControl.starting_qp;
		sRateControl.last_qp = sRateControl.initial_qp;
	}

	// set up MB indices we want to sample from for estimating current frame cost
	int num_of_MB_samples = (int) (gEncoderState.mb_max * MB_SAMPLES_PERCENTAGE);
#if 1	// use multiple circles
	const int kNumCircles = 3;
	const float kFactors[kNumCircles] = { 0.6f, 0.3f, 0.1f };
	const float kRadius[kNumCircles]  = { 0.7f, 0.3f, 0.1f };

	// outer circle
	int sample_index = 0;

	for (int n = 0; n < kNumCircles; n++)
	{
		int num_in_circle = (int) (num_of_MB_samples * kFactors[n]);
		float width_radius  = gEncoderState.mb_width  * 0.5f * kRadius[n];
		float height_radius = gEncoderState.mb_height * 0.5f * kRadius[n];

		int mb_x, mb_y;
		for (int i = 0; i < num_in_circle; i++)
		{
			float radian = (float) i / (float) num_in_circle * 2.0f * 3.1415f;
			mb_x = (int)(cosf(radian) * width_radius)  + (gEncoderState.mb_width  / 2);
			mb_y = (int)(sinf(radian) * height_radius) + (gEncoderState.mb_height / 2);

			if ((mb_x < 0) || (mb_x >= gEncoderState.mb_width) || (mb_y < 0) || (mb_y >= gEncoderState.mb_height))
				continue;

			ENC_ASSERT(sample_index < MAX_MB_SAMPLES);
			sRateControl_Sample_MB_Index[sample_index++] = mb_x + (mb_y * gEncoderState.mb_width);
		}
	}

	sRateControl.num_of_MB_samples = sample_index;
#else
	// for now just a cheap random spread
	for (int i = 0; i < num_of_MB_samples; i++)
	{
		sRateControl_Sample_MB_Index[i] = rand() % gEncoderState.mb_max;
	}
	sRateControl.num_of_MB_samples = num_of_MB_samples;
#endif

	sRateControl.estimate_extrapolation_factor = (float) gEncoderState.mb_max / (float) sRateControl.num_of_MB_samples;
}

void ExitRateControl()
{
	if (sRateControl.total_num_frames > 0)
	{
		ENC_TRACE("Average MAD: %7.3f  Average QP: %6.2f  Average I and P QP: %6.2f\n", sRateControl.total_MAD / sRateControl.total_num_frames, (float) sRateControl.total_QP / sRateControl.total_num_frames, (float) sRateControl.total_IP_QP / sRateControl.total_num_IP_frames);
	}
}

int RateControl_GetFrameQP()
{
	if (!gEncoderState.rate_control)
	{
		return gEncoderState.qp + gEncoderState.current_qp_delta;
	}
	return sRateControl.qp;
}

void RateControl_Init_Frame()
{
	if (gEncoderState.idr)
	{
		int num_B_frames = (gEncoderState.b_slice_on ? (gEncoderState.b_slice_frames + 1) : 1);

		if (gEncoderState.total_frame_num == 0)
			sRateControl.num_of_P_frames = 1 + ((gEncoderState.idr_frame_interval - 2) / num_B_frames);
		else
			sRateControl.num_of_P_frames = (gEncoderState.idr_frame_interval - 1) / num_B_frames;
		sRateControl.total_num_of_P_frames = sRateControl.num_of_P_frames;
		sRateControl.num_of_B_frames = gEncoderState.idr_frame_interval - sRateControl.num_of_P_frames - 1;

		// if there is a surplus, halve if so that it doesn't accumulate and cause a spike in bitrate
		int64_t bits_per_interval = (uint64_t) (gEncoderState.idr_frame_interval * sRateControl.bits_per_frame);
		sRateControl.bits_remaining /= 2;	// 
		if (sRateControl.bits_remaining != 0)
		{
			if (sRateControl.bits_remaining > 0)
			{
				sRateControl.bits_remaining = IMIN(sRateControl.bits_remaining,(bits_per_interval/10));
			}
			else
			{
				sRateControl.bits_remaining = IMAX(sRateControl.bits_remaining,(-bits_per_interval/4));
			}
		}

		sRateControl.bound_min  = (int)(sRateControl.bits_remaining + sRateControl.bits_per_frame);
		sRateControl.bound_max1 = (int)(sRateControl.bits_remaining + (sRateControl.bitrate * 2.048));

		sRateControl.bits_remaining += bits_per_interval;	// must change for B slices

		if (sRateControl.qp_of_last_group == -1)
		{
			sRateControl.qp = sRateControl.starting_qp;
			sRateControl.qp_of_last_group = sRateControl.starting_qp;
		}
		else
		{
			int average_qp = (int) ((float) sRateControl.total_Qp_for_P_frames / (float) sRateControl.current_num_of_P_frames + 0.5f);

			average_qp = CLIP3(sRateControl.qp_of_last_group - 2, sRateControl.qp_of_last_group + 2, average_qp );

			sRateControl.qp = average_qp;
			sRateControl.starting_qp = average_qp;
			sRateControl.qp_of_last_group = average_qp;
		}
		sRateControl.total_Qp_for_P_frames = 0;
		sRateControl.current_num_of_P_frames = 0;
		sRateControl.current_num_of_B_frames = 0;

		sRateControl.idr_timestamp = timeGetTime();
	}
	else
	if (gEncoderState.is_P_slice)
	{
		if (sRateControl.current_num_of_P_frames > 0)
		{
			if (sRateControl.current_num_of_P_frames == 1)
			{
				sRateControl.target_group_size_delta = (float) sRateControl.frame_group_size / (float) (sRateControl.total_num_of_P_frames - 1);
				sRateControl.target_group_size = (float) sRateControl.frame_group_size - sRateControl.target_group_size_delta;
				sRateControl.frame_group_size = 0;	// reset it each interval
			}
			else
				sRateControl.target_group_size -= sRateControl.target_group_size_delta;

			// calc target bits for frame
			sRateControl.target_bits = (int) floor( sRateControl.P_ComplexityWeight * sRateControl.bits_remaining / (sRateControl.num_of_P_frames * sRateControl.P_ComplexityWeight + sRateControl.num_of_B_frames * sRateControl.B_ComplexityWeight) + 0.5f);
			int tmp  = IMAX(0, (int) floor(sRateControl.bits_per_frame - GAMMAP * (sRateControl.frame_group_size - sRateControl.target_group_size) + 0.5f));
			sRateControl.target_bits = (int) floor(BETAP * (sRateControl.target_bits - tmp) + tmp + 0.5f);

			sRateControl.target_bits = CLIP3(sRateControl.bound_min, sRateControl.bound_max2, sRateControl.target_bits);

			if (sRateControl.target_bits < 0)
				ENC_TRACE("");
		}
	}

	RateControl_UpdateQP();
}


double QP2Qstep( int QP )
{
  int i;
  double Qstep;
  static const double QP2QSTEP[6] = { 0.625, 0.6875, 0.8125, 0.875, 1.0, 1.125 };

  Qstep = QP2QSTEP[QP % 6];
  for( i=0; i<(QP/6); i++)
    Qstep *= 2;

  return Qstep;
}


int Qstep2QP( double Qstep, int qp_offset )
{
  int q_per = 0, q_rem = 0;

  if( Qstep < QP2Qstep(0))
    return 0;
  else if (Qstep > QP2Qstep(MAX_QP + qp_offset) )
    return (MAX_QP + qp_offset);

  while( Qstep > QP2Qstep(5) )
  {
    Qstep /= 2.0;
    q_per++;
  }

  if (Qstep <= 0.65625)
  {
    //Qstep = 0.625;
    q_rem = 0;
  }
  else if (Qstep <= 0.75)
  {
    //Qstep = 0.6875;
    q_rem = 1;
  }
  else if (Qstep <= 0.84375)
  {
    //Qstep = 0.8125;
    q_rem = 2;
  }
  else if (Qstep <= 0.9375)
  {
    //Qstep = 0.875;
    q_rem = 3;
  }
  else if (Qstep <= 1.0625)
  {
    //Qstep = 1.0;
    q_rem = 4;
  }
  else
  {
    //Qstep = 1.125;
    q_rem = 5;
  }

  return (q_per * 6 + q_rem);
}

static int sEstimateUsedToAdjustQP = 0;

int RateControl_UpdateQP()
{
	sEstimateUsedToAdjustQP = 0;
	sRateControl.frame_luma_coeff_cost_estimate = 0;

	if (gEncoderState.idr)
	{
		if (sRateControl.starting_qp < sRateControl.initial_qp)
		{
			sRateControl.qp = (sRateControl.starting_qp + sRateControl.initial_qp) / 2;	// average to never let the idr frame get too crazy low

			RateControl_EstimateFrame();	// assumes current frame is setup for MB processing
			sEstimateUsedToAdjustQP |= 8;

			float est_frame_bits = sRateControl.frame_luma_coeff_cost_estimate;
			if (est_frame_bits > (sRateControl.bits_per_frame * 2.0f))
			{
				sEstimateUsedToAdjustQP |= 2;
				float scale_factor = RATE_QP_BIAS;

				if (sRateControl.bits_per_frame > 0.0f)
				{
					float max_bits = sRateControl.bits_per_frame * 3.0f;
					if (est_frame_bits > max_bits)
						est_frame_bits = max_bits;	// cap it
					scale_factor = (est_frame_bits - sRateControl.bits_per_frame) / (max_bits - sRateControl.bits_per_frame);	// 0.0 - 1.0
					sEstimateUsedToAdjustQP |= 4;
				}

				sRateControl.qp += (int) (scale_factor * (sRateControl.initial_qp - sRateControl.qp));
			}
		}
		else
			sRateControl.qp = sRateControl.starting_qp;

		sRateControl.total_IP_QP += sRateControl.qp;

		return sRateControl.qp;
	}
	else
	if (gEncoderState.is_P_slice)
	{
		if (sRateControl.current_num_of_P_frames == 0)
		{
			sRateControl.qp = sRateControl.starting_qp;

			if (sRateControl.starting_qp < sRateControl.initial_qp)
			{
				RateControl_EstimateFrame();	// assumes current frame is setup for MB processing
				sEstimateUsedToAdjustQP |= 8;

				float est_frame_bits = sRateControl.frame_luma_coeff_cost_estimate;
				if (est_frame_bits > (sRateControl.bits_per_frame * 1.5f))
				{
					sEstimateUsedToAdjustQP |= 2;
					float scale_factor = RATE_QP_BIAS;

					if (sRateControl.bits_per_frame > 0.0f)
					{
						float max_bits = sRateControl.bits_per_frame * 3.0f;
						if (est_frame_bits > max_bits)
							est_frame_bits = max_bits;	// cap it
						scale_factor = (est_frame_bits - sRateControl.bits_per_frame) / (max_bits - sRateControl.bits_per_frame);	// 0.0 - 1.0
						sEstimateUsedToAdjustQP |= 4;
					}

					sRateControl.qp += (int) (scale_factor * (sRateControl.initial_qp - sRateControl.qp));
				}
			}

			sRateControl.prev_last_qp = sRateControl.last_qp;
			sRateControl.total_Qp_for_P_frames += sRateControl.qp;
			sRateControl.total_IP_QP += sRateControl.qp;
		}
		else
		{
			sRateControl.X1 = sRateControl.P_X1;
			sRateControl.X2 = sRateControl.P_X2;
			sRateControl.MAD_C1 = sRateControl.P_MAD_C1;
			sRateControl.MAD_C2 = sRateControl.P_MAD_C2;
			sRateControl.prev_MAD = sRateControl.MAD_P_Slice[0];

			sRateControl.MAD = sRateControl.MAD_C1 * sRateControl.prev_MAD + sRateControl.MAD_C2;

			if (sRateControl.target_bits < 0)
			{
				sEstimateUsedToAdjustQP |= 64;

				sRateControl.qp = sRateControl.P_qp + gEncoderState.max_qp_delta;
				sRateControl.qp = CLIP3(LOCAL_QP_MIN, LOCAL_QP_MAX, sRateControl.qp);
			}
			else
			{
				if ((sRateControl.bits_remaining < 0) && (sRateControl.P_qp < sRateControl.initial_qp))	// if bits_remaining goes negative we must not let our qp drop
				{																						// too far as it will kill our bit rate
					sEstimateUsedToAdjustQP |= 128;

					sRateControl.qp = sRateControl.P_qp + gEncoderState.max_qp_delta;
					sRateControl.qp = CLIP3(LOCAL_QP_MIN, LOCAL_QP_MAX, sRateControl.qp);
				}
				else
				{
					sEstimateUsedToAdjustQP |= 32;

					int target_residual_bits = sRateControl.target_bits - sRateControl.header_bits;
					target_residual_bits = IMAX(target_residual_bits, (int) (sRateControl.bits_per_frame * 0.25f));

					//
					double tmp, q_step;

					tmp = sRateControl.MAD * sRateControl.X1 * sRateControl.MAD * sRateControl.X1 + 
						   (4 * sRateControl.X2 * sRateControl.MAD * target_residual_bits);
					if ((sRateControl.X2 == 0.0f) || (tmp < 0) || ((sqrt (tmp) - sRateControl.X1 * sRateControl.MAD) <= 0.0)) // fall back 1st order mode
						q_step = (float) (sRateControl.X1 * sRateControl.MAD / (double) target_residual_bits);
					else // 2nd order mode
						q_step = (float) ((2 * sRateControl.X2 * sRateControl.MAD) / (sqrt (tmp) - sRateControl.X1 * sRateControl.MAD));

					sRateControl.qp = Qstep2QP(q_step, 0);

					sRateControl.qp = CLIP3(sRateControl.P_qp - gEncoderState.max_qp_delta, sRateControl.P_qp + gEncoderState.max_qp_delta, sRateControl.qp);
					sRateControl.qp = CLIP3(LOCAL_QP_MIN, LOCAL_QP_MAX, sRateControl.qp);

					// check for over-aggressive targeting
					if ((sRateControl.qp < sRateControl.initial_qp) && (sRateControl.num_of_P_frames > 0))
					{
						RateControl_EstimateFrame();	// assumes current frame is setup for MB processing
						sEstimateUsedToAdjustQP |= 8;
					
						float est_frame_bits = sRateControl.frame_luma_coeff_cost_estimate;// * EST_ACCURACY_FACTOR;
						float extrapolate_bits = sRateControl.bits_remaining - (sRateControl.bits_per_frame * sRateControl.num_of_P_frames) - est_frame_bits;
						if ((extrapolate_bits < 0) || (est_frame_bits > (sRateControl.bits_per_frame * 1.5f)))
						{
							sEstimateUsedToAdjustQP |= 2;
							float scale_factor = RATE_QP_BIAS;

							if ((sRateControl.bits_per_frame > 0.0f) && (extrapolate_bits >= 0))
							{
								float max_bits = sRateControl.bits_per_frame * 3.0f;
								if (est_frame_bits > max_bits)
									est_frame_bits = max_bits;	// cap it
								scale_factor = (est_frame_bits - sRateControl.bits_per_frame) / (max_bits - sRateControl.bits_per_frame);	// 0.0 - 1.0
								sEstimateUsedToAdjustQP |= 4;
							}

							sRateControl.qp += (int) (scale_factor * (sRateControl.initial_qp - sRateControl.qp));
						}
					}
				}
			}

			sRateControl.total_Qp_for_P_frames += sRateControl.qp;
			sRateControl.total_IP_QP += sRateControl.qp;
			sRateControl.prev_last_qp = sRateControl.last_qp;
			sRateControl.last_qp = sRateControl.qp;
		}

		sRateControl.P_qp = sRateControl.qp;

		return sRateControl.qp;
	}
	else
	if (gEncoderState.is_B_slice)
	{
		if (gEncoderState.b_slice_frames == 1)	// simple
		{
			sRateControl.qp = IMIN(sRateControl.qp + 2 + B_STEP_OFFSET, LOCAL_QP_MAX);
		}
		else
		{
			int b_frame_in_group = (sRateControl.current_num_of_B_frames + 1) % (gEncoderState.b_slice_frames);

			if (b_frame_in_group == 0)	// last in group?
				b_frame_in_group = (gEncoderState.b_slice_frames);

			int qp_step = 0;
			int delta_qp = sRateControl.last_qp - sRateControl.prev_last_qp;
			int num_b_frame_factor = (gEncoderState.b_slice_frames) * -2;

			if (delta_qp <= (num_b_frame_factor - 3))
				qp_step = -3;
			else
			if (delta_qp <= (num_b_frame_factor - 2))
				qp_step = -2;
			else
			if (delta_qp <= (num_b_frame_factor - 1))
				qp_step = -1;
			else
			if (delta_qp <= (num_b_frame_factor))
				qp_step = 0;
			else
			if (delta_qp <= (num_b_frame_factor + 1))
				qp_step = 1;
			else
				qp_step = 2;

			sRateControl.qp = sRateControl.prev_last_qp + qp_step + B_STEP_OFFSET;
			sRateControl.qp += CLIP3((b_frame_in_group - 1) * -2, (b_frame_in_group - 1) * 2, ((b_frame_in_group-1) * (delta_qp) / (gEncoderState.b_slice_frames-1)));
			sRateControl.qp = CLIP3(LOCAL_QP_MIN, LOCAL_QP_MAX, sRateControl.qp);
		}
	}
	return sRateControl.qp;
}

void RateControl_Estimate (int num_items, bool *rejected, bool update_p)
{
	int num_valid = num_items;
	double a00 = 0.0, a01 = 0.0, a10 = 0.0, a11 = 0.0, b0 = 0.0, b1 = 0.0;

	for (int i = 0; i < num_items; i++)
	{
		if (rejected[i])
			num_valid--;
	}

	sRateControl.X1 = sRateControl.X2 = 0.0;

	double sample_qp = 0;
	for (int i = 0; i < num_items; i++)
	{
		if (!rejected[i])
			sample_qp = sRateControl.Qp_History[i];
	}

	bool use_2nd = FALSE;

	for (int i = 0; i < num_items; i++)
	{
		if ((sRateControl.Qp_History[i] != sample_qp) && !rejected[i])
			use_2nd = TRUE;
		if (!rejected[i])
			sRateControl.X1 += (sRateControl.Qp_History[i] * sRateControl.Rp_History[i]) / num_valid;
	}

	// use 2nd
	if ((num_valid >= 1) && use_2nd)
	{
		for (int i = 0; i < num_items; i++)
		{
			if (!rejected[i])
			{
				a00  = a00 + 1.0;
				a01 += 1.0 / sRateControl.Qp_History[i];
				a10  = a01;
				a11 += 1.0 / (sRateControl.Qp_History[i] * sRateControl.Qp_History[i]);
				b0  += sRateControl.Qp_History[i] * sRateControl.Rp_History[i];
				b1  += sRateControl.Rp_History[i];
			}
		}
		// solve the equation of AX = B
		double value = a00 * a11 - a01 * a10;
		if (fabs(value) > 0.000001)
		{
			sRateControl.X1 = static_cast<float>((b0 * a11 - b1 * a01) / value);
			sRateControl.X2 = static_cast<float>((b1 * a00 - b0 * a10) / value);
		}
		else
		{
			sRateControl.X1 = static_cast<float>(b0 / a00);
			sRateControl.X2 = 0.0f;
		}
	}

	if (update_p)
	{
		sRateControl.P_X1 = sRateControl.X1;
		sRateControl.P_X2 = sRateControl.X2;
	}
}



void RateControl_MAD_Estimate (int num_items, bool *rejected)
{
	int num_valid = num_items;
	double a00 = 0.0, a01 = 0.0, a10 = 0.0, a11 = 0.0, b0 = 0.0, b1 = 0.0;

	for (int i = 0; i < num_items; i++)
	{
		if (rejected[i])
			num_valid--;
	}

	sRateControl.MAD_C1 = sRateControl.MAD_C2 = 0.0f;

	double sample_mad = 0;
	for (int i = 0; i < num_items; i++)
	{
		if (!rejected[i])
			sample_mad = sRateControl.MAD_Slice[i];
	}

	bool use_2nd = FALSE;

	for (int i = 0; i < num_items; i++)
	{
		if ((sRateControl.MAD_Slice[i] != sample_mad) && !rejected[i])
			use_2nd = TRUE;
		if (!rejected[i])
			sRateControl.MAD_C1 += (sRateControl.MAD_Slice[i] / (sRateControl.MAD_Reference[i] * num_valid));
	}

	// use 2nd
	if ((num_valid >= 1) && use_2nd)
	{
		for (int i = 0; i < num_items; i++)
		{
			if (!rejected[i])
			{
				a00  = a00 + 1.0;
				a01 += sRateControl.MAD_Reference[i];
				a10  = a01;
				a11 += sRateControl.MAD_Reference[i] * sRateControl.MAD_Reference[i];
				b0  += sRateControl.MAD_Slice[i];
				b1  += sRateControl.MAD_Slice[i] * sRateControl.MAD_Reference[i];
			}
		}
		// solve the equation of AX = B
		double value = a00 * a11 - a01 * a10;
		if (fabs(value) > 0.000001)
		{
			sRateControl.MAD_C2 = static_cast<float>((b0 * a11 - b1 * a01) / value);
			sRateControl.MAD_C1 = static_cast<float>((b1 * a00 - b0 * a10) / value);
		}
		else
		{
			sRateControl.MAD_C1 = static_cast<float>(b0 / a00);
			sRateControl.MAD_C2 = 0.0f;
		}
	}

	if (!gEncoderState.idr)
	{
		sRateControl.P_MAD_C1 = sRateControl.MAD_C1;
		sRateControl.P_MAD_C2 = sRateControl.MAD_C2;
	}
}

int sAccuracyCount = 0;
float sAccumulatedAccuracyRating = 0.0f;

#if 0	// turn on/off logging per frame
#ifdef ENC_TRACE
#undef ENC_TRACE
#define ENC_TRACE(...)
#endif
#endif

void RateControl_UpdateFrame(int frame_bits, int residual_bits)
{
	int delta = (frame_bits - (int) sRateControl.bits_per_frame); 
	sRateControl.bits_remaining -= frame_bits;
	sRateControl.frame_group_size += delta;

	sRateControl.bound_min  -= (int) delta;
	sRateControl.bound_max1 -= (int) delta;
	sRateControl.bound_max2  = (int)(0.9F * sRateControl.bound_max1);

	float frame_time = ENC_GET_HL_TIMER(0);
	float copy_alloc_time = ENC_GET_TIMER(15,true);	// 
	float process_mb_time = ENC_GET_TIMER(4,true);	// cabac
	float in_loop_filter_time = ENC_GET_TIMER(12,true) // p skip 15);
	float transform_time = ENC_GET_TIMER(3,true);
	float motion_est_time = ENC_GET_TIMER(10,true);

	sRateControl.total_num_frames++;
	sRateControl.total_MAD += ((float) gEncoderState.current_frame->frame_mad / (float) gEncoderState.mb_max);
	sRateControl.total_QP += sRateControl.qp;

	char report_str[256];

	if (gEncoderState.wait_for_frame_queue > 0)
	{
		sprintf_s(report_str, "%s%02d %02d %6d",((frame_time > 33.3f) ? "++?" : "  ?"), gEncoderState.frames_in_queue, sRateControl.num_of_P_frames, gEncoderState.current_frame->call_counter[0]);
	}
	else
	{
		sprintf_s(report_str, "%s%02d %02d %6d",((frame_time > 33.3f) ? "+++" : "   "), gEncoderState.frames_in_queue, sRateControl.num_of_P_frames, gEncoderState.current_frame->call_counter[0]);
	}

	if (gEncoderState.idr)
	{
		float overall_accuracy = 0.0f;
		if (sAccuracyCount)
		{
			overall_accuracy = sAccumulatedAccuracyRating / sAccuracyCount;
		}

		char est_str[256];
		sprintf_s(est_str, "%2x", sEstimateUsedToAdjustQP);
		ENC_TRACE("* %5d: TQP:%2d  Target:%7d Fr Size:%7d Res:%7d MAD:%6.2f %s Est Cost:%7.2f EstAcc:%7.2f ms:%6.2f (%5.2f %5.2f %5.2f %5.2f %5.2f)%s RemBits:%ld\n", 
			gEncoderState.total_frame_num, RateControl_GetFrameQP(), sRateControl.target_bits, frame_bits, residual_bits, sRateControl.MAD, 
			(sEstimateUsedToAdjustQP ? est_str : " "),
			sRateControl.frame_luma_coeff_cost_estimate,
			overall_accuracy, 
			frame_time, 
			copy_alloc_time, process_mb_time,in_loop_filter_time,transform_time, motion_est_time,
			report_str,
			sRateControl.bits_remaining);

		sRateControl.total_num_IP_frames++;
		return;
	}

	float acc = 0;

	if (gEncoderState.is_B_slice)
	{
		sRateControl.num_of_B_frames--;
		sRateControl.B_Complexity = frame_bits * sRateControl.qp;
		sRateControl.B_ComplexityWeight = (float) sRateControl.B_Complexity / THETA;
	}
	else
	if (gEncoderState.is_P_slice)
	{
		sRateControl.total_num_IP_frames++;

		// update complexity
		if (gEncoderState.realtime_mode)
		{	// calculate num_of_P_frames by time since last IDR
	#if 0	// off for now
			int time_diff = timeGetTime() - sRateControl.idr_timestamp;
			float ticks_per_frame = 1000.0f / (gEncoderState.fps);
			sRateControl.num_of_P_frames = ((gEncoderState.idr_frame_interval * ticks_per_frame) - time_diff) / ticks_per_frame;
	#else
			sRateControl.num_of_P_frames--;
	#endif
		}
		else
			sRateControl.num_of_P_frames--;

		sRateControl.P_Complexity = frame_bits * sRateControl.qp;
		sRateControl.P_ComplexityWeight = (float) sRateControl.P_Complexity;
		sRateControl.header_bits = frame_bits - residual_bits;
		sRateControl.current_num_of_P_frames++;

		// compute frame MAD
		sRateControl.MAD = ((float) gEncoderState.current_frame->frame_mad) / (gEncoderState.mb_width * gEncoderState.mb_height * 16 * 16);

		if (sRateControl.frame_luma_coeff_cost_estimate > 1000.0f)
		{
			acc = frame_bits / sRateControl.frame_luma_coeff_cost_estimate;
			sAccumulatedAccuracyRating += acc;
			sAccuracyCount++;
		}

		for (int i = (RATE_CONTROL_HISTORY_SIZE-2); i > 0; i--)
		{
			sRateControl.P_Qp_History[i] = sRateControl.P_Qp_History[i - 1];
			sRateControl.Qp_History[i]   = sRateControl.P_Qp_History[i];
			sRateControl.P_Rp_History[i] = sRateControl.P_Rp_History[i - 1];
			sRateControl.Rp_History[i]   = sRateControl.P_Rp_History[i];
		}

		sRateControl.P_Qp_History[0] = (float) QP2Qstep(sRateControl.qp);
		sRateControl.P_Rp_History[0] = (float) gEncoderState.current_frame->total_coeff_bits / sRateControl.MAD;

		sRateControl.Qp_History[0] = sRateControl.P_Qp_History[0];
		sRateControl.Rp_History[0] = sRateControl.P_Rp_History[0];
		sRateControl.X1 = sRateControl.P_X1;
		sRateControl.X2 = sRateControl.P_X2;

		int window_size = 0;
		int initial_window_size;
		bool rejected[RATE_CONTROL_HISTORY_SIZE];

		initial_window_size = (sRateControl.MAD > sRateControl.prev_MAD) ?
			(int)(sRateControl.prev_MAD / sRateControl.MAD * (RATE_CONTROL_HISTORY_SIZE-1)) :
			(int)(sRateControl.MAD / sRateControl.prev_MAD * (RATE_CONTROL_HISTORY_SIZE-1));
		window_size = CLIP3(1, sRateControl.current_num_of_P_frames, initial_window_size);
		window_size = IMIN(window_size, sRateControl.prev_window_size + 1);
		window_size = IMIN(window_size, (RATE_CONTROL_HISTORY_SIZE-1));

		sRateControl.prev_window_size = window_size;

		for (int i = 0; i < (RATE_CONTROL_HISTORY_SIZE-1); i++)
		{
			rejected[i] = FALSE;
		}

		RateControl_Estimate(window_size, rejected, !gEncoderState.idr);

		window_size = sRateControl.prev_window_size;

		float err[RATE_CONTROL_HISTORY_SIZE];
		float sum_of_sq_error = 0;
		float threshold;

		for (int i = 0; i < (int) window_size; i++)
		{
			err[i] = sRateControl.X1 / sRateControl.Qp_History[i] + sRateControl.X2 / (sRateControl.Qp_History[i] * sRateControl.Qp_History[i]) - sRateControl.Rp_History[i];
			sum_of_sq_error += err[i] * err[i];
		}
		threshold = (window_size == 2) ? 0 : sqrt (sum_of_sq_error / window_size);
		for (int i = 0; i < (int) window_size; i++)
		{
			if (fabs(err[i]) > threshold)
				rejected[i] = TRUE;
		}
		rejected[0] = FALSE;

		RateControl_Estimate(window_size, rejected, !gEncoderState.idr);

		if (sRateControl.current_num_of_P_frames > 1)
		{
			for (int i = (RATE_CONTROL_HISTORY_SIZE - 2); i > 0; i--)
			{
				sRateControl.MAD_P_Slice[i]   = sRateControl.MAD_P_Slice[i - 1];
				sRateControl.MAD_Slice[i]     = sRateControl.MAD_P_Slice[i];
				sRateControl.MAD_Reference[i] = sRateControl.MAD_Reference[i-1];
			}
			sRateControl.MAD_P_Slice[0] = sRateControl.MAD;
			sRateControl.MAD_Slice[0]   = sRateControl.MAD_P_Slice[0];

			sRateControl.MAD_Reference[0] = sRateControl.MAD_Slice[1];
		
			sRateControl.MAD_C1 = sRateControl.P_MAD_C1;
			sRateControl.MAD_C2 = sRateControl.P_MAD_C2;

			window_size = CLIP3(1, (sRateControl.current_num_of_P_frames - 1), initial_window_size);
			window_size = IMIN(window_size, IMIN(20, sRateControl.prev_MAD_window_size + 1));

			sRateControl.prev_MAD_window_size = window_size;
			for (int i = 0; i < (RATE_CONTROL_HISTORY_SIZE-1); i++)
			{
				rejected[i] = FALSE;
			}

			if (!gEncoderState.idr)
				sRateControl.prev_MAD = sRateControl.MAD;

			RateControl_MAD_Estimate(window_size, rejected);

			sum_of_sq_error = 0;

			for (int i = 0; i < window_size; i++)
			{
				err[i] = sRateControl.MAD_C1 * sRateControl.MAD_Reference[i] + sRateControl.MAD_C2 - sRateControl.MAD_Slice[i];
				sum_of_sq_error += err[i] * err[i];
			}
			threshold = (window_size == 2) ? 0 : sqrt (sum_of_sq_error / window_size);
			for (int i = 0; i < window_size; i++)
			{
				if (fabs(err[i]) > threshold)
					rejected[i] = TRUE;
			}
			rejected[0] = FALSE;

			RateControl_MAD_Estimate(window_size, rejected);
		}
		else
		{
			sRateControl.MAD_P_Slice[0] = sRateControl.MAD;
		}
	}

	char *stype = (gEncoderState.is_B_slice) ? "B" : "P";

	ENC_TRACE(" %s%5d: TQP:%2d  Target:%7d Fr Size:%7d Res:%7d MAD:%7.3f RemBits:%8ld", stype, gEncoderState.total_frame_num, RateControl_GetFrameQP(), sRateControl.target_bits, frame_bits, residual_bits, sRateControl.MAD, sRateControl.bits_remaining);

	if (sEstimateUsedToAdjustQP)
	{
		ENC_TRACE(" %02x", sEstimateUsedToAdjustQP);
	}
	else
	{
		ENC_TRACE("   ");
	}

	//	ENC_TRACE("(%7d %7d)", sRateControl.bound_min, sRateControl.bound_max2);

	ENC_TRACE("Est MAD:%7.2f Est Cost:%8.1f Acc:%5.2f ms:%6.2f (%5.2f %5.2f %5.2f %5.2f %5.2f)%s\n", 
		sRateControl.frame_complexity_estimate, sRateControl.frame_luma_coeff_cost_estimate, acc, 
		frame_time, copy_alloc_time, process_mb_time,in_loop_filter_time,transform_time, motion_est_time,
		report_str);

}

void RateControl_EstimateFrame()
{
    int mad = 0;
    int total_mad = 0;
	int cost_est = 0;

	for (int i = 0; i < sRateControl.num_of_MB_samples; i++)
	{
		mad = 0;
		cost_est += RC_ProcessMacroBlock(gEncoderState.current_frame, sRateControl_Sample_MB_Index[i], sRateControl.qp, &mad);
		total_mad += mad;
	}

	// extropolate
	sRateControl.frame_complexity_estimate = (float) mad * sRateControl.estimate_extrapolation_factor / 256;
	sRateControl.frame_luma_coeff_cost_estimate = (float) cost_est * sRateControl.estimate_extrapolation_factor * EST_ACCURACY_FACTOR;
}

void RateControl_DropFrames(int num_dropped)
{
//	sRateControl.num_of_P_frames -= num_dropped;
}