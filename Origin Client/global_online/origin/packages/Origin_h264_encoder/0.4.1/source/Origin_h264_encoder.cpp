#include <stdio.h>

#include "Origin_h264_encoder.h"
#include "Tables.h"
#include "Frame.h"
#include "Quantize.h"
#include "Cabac_H264.h"
#include "NALU.h"
#include "Thread.h"
#include "Timing.h"
#include "MotionEstimation.h"
#include "RateControl.h"
#include "Trace.h"
#include <intrin.h>	// for __cpuid()

#ifdef _DEBUG
#define WRITEOUT_H264_STREAM
#endif

#define WRITE_FRAMES_FILENAME "bf4-src-long.dat" //"bf3-goin-hunting.dat"

EncoderStateType gEncoderState;

static bool sEncoderInitialized = false;

static FILE *sOutputStreamFile = NULL;
FILE *sFrameDataFile = NULL;
#ifdef WRITE_FRAMES_TO_FILE
uint8_t *gFrameBuffer = NULL;
size_t gFrameBufferIndex = 0;
uint32_t gFrameBufferCount = 0;
#endif
#ifdef SGIENG_DEBUG
FILE *sMVOutputFile = NULL;
#endif

void WriteoutNAL(FILE *f, NAL_Type *nal)
{
	fwrite(nal->p_payload, 1, nal->i_payload, f);
}

void WriteFrameDataHeader(FILE *f, int width, int height)
{
    // write width and height
    fwrite(&width, sizeof(int), 1, f);
    fwrite(&height, sizeof(int), 1, f);

    // write placeholder for number of frames
    int frame_count = 0;
    fwrite(&frame_count, sizeof(int), 1, f);

    // write format
    int format = 0;
    fwrite(&format, sizeof(int), 1, f);
}

void FlushFrameData(FILE *f)
{
#ifdef WRITE_FRAMES_TO_FILE
//    fwrite(gFrameBuffer, gFrameBufferIndex, 1, f);
#endif
}

void WriteFrameDataFrameCount(FILE *f, int frame_count)
{
    int sizeOfInt = sizeof(int);
    int offset = sizeOfInt + sizeOfInt;
    fseek(f, offset, SEEK_SET); // go to position of 'frame_count'
    fwrite(&frame_count, sizeOfInt, 1, f);
}

bool HasSSSE3Support()
{
	int info[4];
	__cpuid(info, 0);
	int nIds = info[0];
	bool HW_SSE3 = false;
	bool HW_SSSE3 = false;

	//  Detect Features
	if (nIds >= 0x00000001)
	{
		__cpuid(info,0x00000001);
		HW_SSE3   = (info[2] & ((int)1 <<  0)) != 0;
		HW_SSSE3  = (info[2] & ((int)1 <<  9)) != 0;
	}

	return HW_SSSE3;
}

void Origin_H264_Encoder_Init(OriginEncoder_InputParamsType *input_parameters)
{
	if (sEncoderInitialized)
		return;
#ifdef _DEBUG
	gEncoderState.realtime_mode = false;// turn off for testbed
#else
	gEncoderState.realtime_mode = true;	// turn on for release
#endif

#ifdef _DEBUG
	gEncoderState.multi_threading_on = false;
#else
	gEncoderState.multi_threading_on = true;
#endif
	if (gEncoderState.multi_threading_on && (DetectNumOfCores() < 2))
		gEncoderState.multi_threading_on = false;	// not enough cores

	gEncoderState.in_loop_filtering_on = input_parameters->filtering_on != 0;
	gEncoderState.subpixel_motion_est = input_parameters->subpixel_motion_est;	// use subpixel level motion vectors
	gEncoderState.variable_qp = input_parameters->variable_qp;	// vary qp by mb complexity
	if (input_parameters->b_slice_on)
	{
		gEncoderState.b_slice_on = true;	// use b-slice frames
		gEncoderState.b_slice_frames = input_parameters->b_slice_on;	// currently force to use this many b frames 
	}
	else
	{
		gEncoderState.b_slice_on = false;	// do not use b-slice frames
		gEncoderState.b_slice_frames = 0;
	}
	gEncoderState.rate_control = input_parameters->rate_control;
	gEncoderState.max_qp_delta = CLIP3(1, 12, input_parameters->max_qp_delta);

    // make it divisible by 16
    input_parameters->width  = input_parameters->width >> 4 << 4;
    input_parameters->height = input_parameters->height >> 4 << 4;

	gEncoderState.width = input_parameters->width;
	gEncoderState.height = input_parameters->height;

	gEncoderState.width_padded  = gEncoderState.width  + FRAME_WIDTH_PAD * 2;	// width + left and right pixel padding
	gEncoderState.height_padded = gEncoderState.height + FRAME_HEIGHT_PAD * 2;	// height + top and bottom pixel padding
	gEncoderState.uv_width_padded  = (gEncoderState.width / 2)  + FRAME_WIDTH_PAD * 2;
	gEncoderState.uv_height_padded = (gEncoderState.height / 2) + FRAME_HEIGHT_PAD * 2;

	gEncoderState.mb_width = input_parameters->width / 16;
	gEncoderState.mb_height = input_parameters->height / 16;

	gEncoderState.mb_max = gEncoderState.mb_width * gEncoderState.mb_height;

	gEncoderState.idr = true;	// first frame is always IDR
	gEncoderState.cabac_init_idc = 0;	// default
	gEncoderState.entropy_coding_mode_flag = true;
	gEncoderState.idr_pic_id = 0;
	gEncoderState.current_frame_num = 0;
	gEncoderState.qp = 28;

	gEncoderState.total_frame_num = 0;
	gEncoderState.idr_frame_interval = input_parameters->idr_frame_interval;
	if (gEncoderState.idr_frame_interval == 0)	// can't be zero
		gEncoderState.idr_frame_interval = 100;	// default

	gEncoderState.bit_rate = input_parameters->target_bitrate;
	gEncoderState.fps = input_parameters->target_frame_rate;
	gEncoderState.last_idr_frame_timestamp = 0L;
	gEncoderState.current_frame_timestamp = 0;
	gEncoderState.frame_delay_ms = (gEncoderState.b_slice_on) ? 800 : 400;	// 800 if B slices?

	gEncoderState.timeslice_ms = 0;//3;
	gEncoderState.timeslice_idle_ms = 3;

	gEncoderState.SSSE3_enabled = HasSSSE3Support();

	SetBasicParameters(input_parameters->width, input_parameters->height, gEncoderState.qp, 4000);

	InitLog();

	if (InitFrames())	// pre-allocated frame queue
	{
		return;	// error (need to handle gracefully)
	}
	InitThreads();	// must be called before InitEncoding()
	InitTables();
	InitNALU(10);
	InitQuantizeParams();
	InitCABAC_ContextStateTable();
	InitEncoding();
	InitTimer();
	InitRateControl();

	if (gEncoderState.total_frame_num == 0)
	{
		ENC_TRACE("FPS:%d Bitrate:%d Width:%d Height:%d B-Slice: %d In-loop Filtering:%s Variable QP:%s Sub-pixel ME:%s Multi-threading:%s # cores:%d Rate Control:%s  Real-time:%s  SSSE3:%s\n\n",
			gEncoderState.fps, gEncoderState.bit_rate, gEncoderState.width, gEncoderState.height, gEncoderState.b_slice_frames, 
			(gEncoderState.in_loop_filtering_on ? "ON":"OFF"),
			(gEncoderState.variable_qp ? "ON":"OFF"),
			(gEncoderState.subpixel_motion_est ? "ON":"OFF"),
			(gEncoderState.multi_threading_on ? "ON":"OFF"),
			gEncoderState.num_cpu_cores,
			(gEncoderState.rate_control ? "ON":"OFF"),
			(gEncoderState.realtime_mode ? "ON":"OFF"),
			(gEncoderState.SSSE3_enabled ? "YES":"NO"));
	}

	ME_InitMotionEstimation(gEncoderState.subpixel_motion_est);

	sEncoderInitialized = true;
	gEncoderState.initialized = true;

#ifdef WRITEOUT_H264_STREAM
	int nal_count;
	NAL_Type *nal_out;
	fopen_s(&sOutputStreamFile, "test.h264", "wb");

	if (sOutputStreamFile)
	{
		Origin_H264_Encode_Headers(&nal_count, (void **) &nal_out);
		for (int i = 0; i < 3; i++)
		{
			WriteoutNAL(sOutputStreamFile, &nal_out[i]);
		}
	}
#endif

#ifdef WRITE_FRAMES_TO_FILE
    fopen_s(&sFrameDataFile, WRITE_FRAMES_FILENAME, "wb");

    if (sFrameDataFile)
    {
		gFrameBufferCount = 0;
		gFrameBuffer = (uint8_t *) malloc(WRITE_FRAME_BUFFER_SIZE);
		assert(gFrameBuffer);
        WriteFrameDataHeader(sFrameDataFile, gEncoderState.width, gEncoderState.height);
    }
#endif

#ifdef SGIENG_DEBUG
    fopen_s(&sMVOutputFile, "i_p_SAD.txt", "w");
#endif
}

float GetRollingAverage();	// temp 

void EncodeFrame(Origin_H264_EncoderInputType *input, int b_group_index)
{
	gEncoderState.current_frame_timestamp = input->timestamp;
	gEncoderState.current_qp_delta = input->qp - gEncoderState.qp;

	int time_stamp = timeGetTime();
	//	if ((time_stamp - gEncoderState.last_idr_frame_timestamp) >= ((gEncoderState.idr_frame_interval - 1) * 1000 / gEncoderState.fps) || (input->frame_type == ORIGIN_H264_IDR) || (gEncoderState.total_frame_num == 0))
	//	if (((gEncoderState.total_frame_num % gEncoderState.idr_frame_interval) == 0) || (input->frame_type == ORIGIN_H264_IDR))
	if ((gEncoderState.total_frame_num == 0) || (input->frame_type == ORIGIN_H264_IDR))
	{
		gEncoderState.idr = true;
		gEncoderState.current_frame_num = 0;	// reset
		gEncoderState.current_frame_display_index = 0;

		if (gEncoderState.total_frame_num == 0)
			gEncoderState.last_idr_frame_timestamp = time_stamp;

		char time_str[256];
		float seconds = (float) gEncoderState.current_frame_timestamp / 1000.0f;
		int frame_c = (int) (gEncoderState.current_frame_timestamp % 1000) / 10;
		int seconds_int = ((int) seconds) % 60;
		int minutes_int = ((int) seconds) / 60;
		sprintf_s(time_str, "%02d:%02d.%02d", minutes_int, seconds_int, frame_c);
		ENC_TRACE("IDR - time:%s timestamp: %d interval time: %d  average:%7.3f\n", time_str, gEncoderState.current_frame_timestamp, time_stamp - gEncoderState.last_idr_frame_timestamp, GetRollingAverage());
		gEncoderState.last_idr_frame_timestamp = timeGetTime();
	}
	else
	if (gEncoderState.b_slice_on)
	{
		// use b_group_index to process the frame
		if (input->frame_type == ORIGIN_H264_P_SLICE)
		{	// assumption is that a P slice is the forward ref frame for a group of b-slices
			gEncoderState.current_frame_display_index += b_group_index + 1;
		}
		else
		{	// assumption is that this must be a B-slice and that each frame following the P-slice is reverse order in display
			gEncoderState.current_frame_display_index--;
		}
	}

	ENC_START_HL_TIMER(0,"ProcessFrame");

	ProcessFrame(gEncoderState.width, gEncoderState.height, input->y_data, input->u_data, input->v_data, input->frame_type, b_group_index);

	ENC_PAUSE_HL_TIMER(0,false);
	ENC_LAP_TIMER(0);

	if (gEncoderState.idr)
		gEncoderState.idr_pic_id = !gEncoderState.idr_pic_id;	// must not be the same as previous IDR (clause 7.4.3)
	else
	if (gEncoderState.b_slice_on)
	{
		if ((input->frame_type == ORIGIN_H264_B_SLICE) && (b_group_index == 0)) // last in the B group?
		{
			gEncoderState.current_frame_display_index += gEncoderState.b_slice_frames;
			gEncoderState.current_frame_num--;	// TODO: may not work for b-slice groups > 2
		}
	}

	gEncoderState.idr = false;
	gEncoderState.current_frame_num = (gEncoderState.current_frame_num + 1) & ((1 << (gSPS_Parameters.log2_max_frame_num_minus4 + 4)) - 1);	// rotate so it stays within the bit range
	gEncoderState.total_frame_num++;


}

int Origin_H264_Encoder_Encode(Origin_H264_EncoderInputType *input, Origin_H264_EncoderOutputType *output)
{
	int buffered_frames = 0;

	output->nal_count = 0;

	if (!sEncoderInitialized)
		return -1;	// error 
    if (input == NULL)
        ENC_TRACE("Origin_H264_Encoder_Encode - input is NULL\n");
	// send back buffered encoded frame
	uint32_t timestamp = input ? input->timestamp : 0;
	buffered_frames = GetBufferedFrame_NALU(timestamp, output);

#ifdef WRITEOUT_H264_STREAM
	if (sOutputStreamFile)
	{
		int nal_count = output->nal_count;
		NAL_Type *nal_out = (NAL_Type *) output->nal_out;
		if (nal_count)
		{
//			ENC_TRACE("%d %d %d\n", output->timestamp, nal_count, nal_out[nal_count-1].i_payload * 8);
			WriteoutNAL(sOutputStreamFile, (NAL_Type *) &nal_out[nal_count-1]);
		}
	}
#endif

	if (input)
	{
		int start_ts = timeGetTime();
		// queue up input frame
		int count = 0;
		while (!AddInputFrameToQueue(input))
		{
//			ENC_TRACE("Input Frame Queue Full\n");
			// queue is full so wait for it to either free up a spot or timeout
			if (WaitForSpaceInFrameQueue(gEncoderState.realtime_mode ? 10 : INFINITE))
				continue;	// done
			count++;

			// only do this for real-time encoding - TestBed will get broken on this
			if (gEncoderState.realtime_mode && (input->frame_type != ORIGIN_H264_IDR) && (count > 1))
			{
				// drop frame
				DropFrame();	// account for this in rate control
				break;
			}
		}

		gEncoderState.wait_for_frame_queue = timeGetTime() - start_ts;
	}
	else
	{
		gEncoderState.wait_for_frame_queue = 0;
		EmptyInputFrame();
	}

	return buffered_frames;
}

void Origin_H264_Encode_Headers(int *NAL_count, void *NAL_out[])
{
	if (!sEncoderInitialized)
		return;

	ResetNALU_Table(gEncoderState.current_frame_timestamp, false, -1);
	uint8_t sps_buffer[256];
	uint8_t pps_buffer[64];
	uint8_t sei_buffer[2048];
	int length;

	length = NALU_Write_SPS(&gSPS_Parameters, sps_buffer);
	AddTo_NALU_Table(sps_buffer, length, 7);
	length = NALU_Write_PPS(&gPPS_Parameters, 0, pps_buffer);
	AddTo_NALU_Table(pps_buffer, length, 8);
	length = NALU_Write_SEI(0, sei_buffer);
	AddTo_NALU_Table(sei_buffer, length, 6);

	*NAL_out = GetNALUs(NAL_count);
}

void Origin_H264_Encoder_Close()
{
	if (!sEncoderInitialized)
		return;

	ENC_END_HL_TIMER(0,true);
	ENC_END_TIMER(1,true);
	ENC_END_TIMER(11,true);
	ENC_END_TIMER(12,true);
	ENC_END_TIMER(2,true);
	ENC_END_TIMER(3,true);
	ENC_END_TIMER(13,true);	// track MB Transform Chroma
	ENC_END_TIMER(4,true);
	ENC_END_TIMER(7,true);
	ENC_END_TIMER(5,true);
	ENC_END_TIMER(6,true);
	ENC_END_TIMER(8,true);
	ENC_END_TIMER(9,true);
	ENC_END_TIMER(15,true);	// In-Loop Filtering
	ENC_END_TIMER(14,true);	// track EndSlice (aka encoding)
	ENC_END_TIMER(10,true);
	ENC_END_TIMER(17,true);
	ENC_END_TIMER(18,true);
//    ENC_END_TIMER(19,true);
	ENC_END_TIMER(20,true);
	ENC_END_TIMER(21,true);

#ifdef WRITEOUT_H264_STREAM
	if (sOutputStreamFile)
	{
		fclose(sOutputStreamFile);
		sOutputStreamFile = NULL;
	}
#endif

#ifdef WRITE_FRAMES_TO_FILE
    if (sFrameDataFile)
    {
        FlushFrameData(sFrameDataFile);
        WriteFrameDataFrameCount(sFrameDataFile, gFrameBufferCount);
        fclose(sFrameDataFile);
        sFrameDataFile = NULL;

		free(gFrameBuffer);
		gFrameBuffer = NULL;
    }
#endif

#ifdef SGIENG_DEBUG
    if( sMVOutputFile )
    {
        fclose(sMVOutputFile);
        sMVOutputFile = NULL;
    }
#endif

	ExitRateControl();
	ExitThreads();
	ExitEncoding();
	ExitFrames();	// free frame queue
	ExitNALU();
	ExitLog();

	ME_ExitMotionEstimation();

	sEncoderInitialized = false;

	// Free memory
}
