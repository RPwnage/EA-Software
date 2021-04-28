#ifndef __ORIGIN_H264_ENCODER_H
#define __ORIGIN_H264_ENCODER_H
#include <stdint.h>

// 
// PUT stuff for EXTERNAL USE below 
//

#define ORIGIN_H264_DEFAULT_TYPE  0
#define ORIGIN_H264_IDR  1


typedef struct  
{
	int width;
	int height;
	int target_frame_rate;	// in frames per second
	int target_bitrate;	// in kbps
	int max_bitrate;	// in kbps
	int idr_frame_interval;	// frames between idr frames

	int filtering_on;	// deblocking filter
	int subpixel_motion_est;	// use subpixel level motion vectors
	int variable_qp;	// vary qp by mb complexity
	int b_slice_on;		// use b-slice frames
	int rate_control;	// adjust settings dynamically to hit target bitrate
	int max_qp_delta;	// maximum change in frame QP between frames

} OriginEncoder_InputParamsType;

typedef struct
{
	int priority;
	int type;    
	int dummy;
	int mb_start_index;
	int mb_end_index;

	int     payload_size;
	uint8_t *payload;
} NAL_Type;

typedef struct  
{
	int frame_type;
	int qp;
	uint32_t timestamp;
	uint8_t *y_data;
	uint8_t *u_data;
	uint8_t *v_data;
} Origin_H264_EncoderInputType;


typedef struct  
{
	int frame_type;
	int nal_count;
	void *nal_out;
	uint32_t timestamp;
} Origin_H264_EncoderOutputType;

void Origin_H264_Encoder_Init(OriginEncoder_InputParamsType *input_parameters);
int  Origin_H264_Encoder_Encode(Origin_H264_EncoderInputType *input, Origin_H264_EncoderOutputType *output);
void Origin_H264_Encode_Headers(int *NAL_count, void *NAL_out[]);
void Origin_H264_Encoder_Close();
#endif