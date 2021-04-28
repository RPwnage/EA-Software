#ifndef __ORIGIN_H264_ENCODER_H
#define __ORIGIN_H264_ENCODER_H
#include <stdint.h>
#include "Frame.h"
#include "MacroBlock.h"

#define USE_SSE2

#ifndef H264_UNUSED
#define H264_UNUSED(x) (void)x;
#endif

#define READ_FRAMES_FROM_FILE
//#define WRITE_FRAMES_TO_FILE
#define WRITE_FRAME_BUFFER_SIZE (0x3ffffff0) //(0xffe00000) // about 60 seconds at 60 frames/second)

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
	bool initialized;
	bool realtime_mode;	// handle drop frames differently for real-time mode
	bool SSSE3_enabled;
	int num_cpu_cores;

	int width;
	int height;
	int width_padded;	// width + left and right pixel padding
	int height_padded;	// height + top and bottom pixel padding
	int uv_width_padded;
	int uv_height_padded;
	int mb_width;
	int mb_height;
	int mb_max;

	int slice_type;
	bool is_I_slice;
	bool is_P_slice;
	bool is_B_slice;
	int current_frame_num;	// resets every IDR frame
	int current_frame_display_index;	// for b-slice support - each I or P-frame.
	int total_frame_num;	// actual movie frame - does not reset
	int idr_frame_interval;
	int idr_pic_id;

	bool idr;
	uint8_t nal_ref_idc;
	int qp;
	int current_qp_delta;
	int max_qp_delta;
	int bit_rate;
	int max_bit_rate;
	int fps;
	int rate_control;

	int subpixel_motion_est;	// use subpixel level motion vectors
	int variable_qp;	// vary qp by mb complexity

	int cabac_init_idc;
	bool entropy_coding_mode_flag;	// CABAC on/off

	bool multi_threading_on;
	bool in_loop_filtering_on;
	bool b_slice_on;		// use b-slice
	int  b_slice_frames;	// number of frames to consider for b slice groups

	int stream_encoding_row_start;	// row to start stream encoding macroblocks

	MacroBlockType *current_mb_set;
	YUVFrameType *current_frame;

	int last_idr_frame_timestamp;
	uint32_t current_frame_timestamp;
	int frame_delay_ms;

	int wait_for_frame_queue;
	int frames_in_queue;

	int timeslice_ms;		// time to process in slice (0 - don't preempt)
	int timeslice_idle_ms;	// time to sleep between slices

	bool mb_debug_on;		// to log detailed encoding info

} EncoderStateType;

extern EncoderStateType gEncoderState;

#define ORIGIN_H264_P_SLICE  2
#define ORIGIN_H264_B_SLICE  3

// 
// PUT stuff for EXTERNAL USE below 
//

#define ORIGIN_H264_DEFAULT_TYPE  0
#define ORIGIN_H264_IDR  1

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