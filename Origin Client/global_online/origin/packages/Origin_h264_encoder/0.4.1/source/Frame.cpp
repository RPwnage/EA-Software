#include <memory.h>
#include "Origin_h264_encoder.h"
#include "Frame.h"
#include "MacroBlock.h"
#include "Cabac_H264.h"
#include "NALU.h"
#include "Tables.h"
#include "Trace.h"
#include "MotionEstimation.h"
#include "LoopFilter.h"
#include "Thread.h"
#include "RateControl.h"
#include "Timing.h"

#ifdef USE_SSE2
#include <emmintrin.h>
#endif

#define PSKIP_THRESHOLD  (256 + 64 + 64)	// max pixel difference to consider for P_SKIP MB type
#define MAX_PIXEL_DIFF 4	// 2 - starts to notice blocking
#define MAX_PIXEL_DIFF_EDGE 4	// 2 - starts to notice blocking
#define MAX_PIXEL_DIFF_CHROMA 8	// TODO: test if this can make a diff?  so far it doesn't seem so

#define MAX_FRAMES_IN_QUEUE 8

static YUVFrameType *sFrameQueue[MAX_FRAMES_IN_QUEUE];
static int sFrameQueueSize = 0;

// track frame data
static int sFrame_TargetSize = 0;
static int sFrame_LastFrameSize = 0;
static int sFrame_Qp = 0;

bool InitFrames()
{
	sFrameQueueSize = 0;

	int reconstructed_Y_frame_size = gEncoderState.width_padded * gEncoderState.height_padded;
	int reconstructed_UV_frame_size = gEncoderState.uv_width_padded * gEncoderState.uv_height_padded * 2;

	// pre-allocated frame queue
	for (int i = 0; i < MAX_FRAMES_IN_QUEUE; i++)
	{
		YUVFrameType *frame = (YUVFrameType *) _aligned_malloc(sizeof(YUVFrameType), 32);
		if (frame == NULL)
		{
			// need error condition
			ENC_ASSERT(0);
			return true;
		}

#ifdef USE_FRAME_BUFFERS_FROM_THREAD
		frame->frame_align_buffer = NULL;
		frame->Y_frame_buffer = NULL;
		frame->UV_frame_buffer = NULL;
#else
		frame->frame_align_buffer = (uint8_t *) _aligned_malloc ((reconstructed_Y_frame_size + reconstructed_UV_frame_size), 32);
		if (frame->frame_align_buffer == NULL)
		{
			// need error condition
			ENC_ASSERT(0);
			return true;
		}

		frame->Y_frame_buffer = (uint8_t *) frame->frame_align_buffer;
		frame->UV_frame_buffer = (ChromaPixelType *) (frame->frame_align_buffer + reconstructed_Y_frame_size);
#endif

		frame->reconstructed_align_buffer = (uint8_t *) _aligned_malloc ((reconstructed_Y_frame_size + reconstructed_UV_frame_size), 32 );
		if (frame->reconstructed_align_buffer == NULL)
		{
			// need error condition
			ENC_ASSERT(0);
			return true;
		}
		frame->Y_frame_reconstructed = (uint8_t *) frame->reconstructed_align_buffer + (FRAME_HEIGHT_PAD * gEncoderState.width_padded) + FRAME_WIDTH_PAD;	// offset to start of frame inside of padding
		frame->UV_frame_reconstructed = (ChromaPixelType *) (frame->reconstructed_align_buffer + (reconstructed_Y_frame_size));
		frame->UV_frame_reconstructed += (FRAME_HEIGHT_PAD * gEncoderState.uv_width_padded) + FRAME_WIDTH_PAD;	// offset to start of frame inside of padding


		frame->mb_set = (MacroBlockType *) _aligned_malloc(gEncoderState.mb_max * sizeof(MacroBlockType), 32);
		if (frame->mb_set == NULL)
		{
			// need error condition
			ENC_ASSERT(0);
			return true;
		}

		frame->mb_aligned_data_buffer  = (uint8_t *) _aligned_malloc((sizeof(u8_16x16_Type) + sizeof(Chroma_8x8_Type) + sizeof(MBDataType)) * gEncoderState.mb_max, 32);
		if (frame->mb_aligned_data_buffer == NULL)
		{
			// need error condition
			ENC_ASSERT(0);
			return true;
		}

#ifdef USE_QUARTER_PIXEL_ME
        frame->quarter_align_buffer = (uint8_t *) _aligned_malloc (gEncoderState.width * gEncoderState.height, 32 );
        if (frame->quarter_align_buffer == NULL)
        {
            // need error condition
            ENC_ASSERT(0);
            return true;
        }

        frame->Y_quarter_buffer = (uint8_t *) frame->quarter_align_buffer;
#endif

		sFrameQueue[i] = frame;
	}

	return false;
}

void FreeFrame(YUVFrameType *frame);

void ExitFrames()
{
	for (int i = 0; i < MAX_FRAMES_IN_QUEUE; i++)
	{
		FreeFrame(sFrameQueue[i]);
	}
}

YUVFrameType *GetQueuedFrame(int index)
{
    if ((index < 0) || (index >= sFrameQueueSize) || (sFrameQueue[index] == NULL))
    {
        //		ENC_ASSERT(0);
        return NULL;
    }

    return(sFrameQueue[index]);
}

void FreeFrame(YUVFrameType *frame)
{
    if (frame)
    {
		if( frame->frame_align_buffer )
			_aligned_free (frame->frame_align_buffer);

        if( frame->reconstructed_align_buffer )
            _aligned_free (frame->reconstructed_align_buffer);

        if( frame->mb_set )
            _aligned_free (frame->mb_set);

		if (frame->mb_aligned_data_buffer)
			_aligned_free(frame->mb_aligned_data_buffer);

#ifdef USE_QUARTER_PIXEL_ME
        if (frame->quarter_align_buffer)
            _aligned_free(frame->quarter_align_buffer);
#endif

        _aligned_free( frame );
    }
}

void AddCurrentFrameToQueue()
{
    if (gEncoderState.current_frame)
    {
        // move existing block sets down
        for(int i = 0; i < MAX_FRAMES_IN_QUEUE-1; i++)
        {
            sFrameQueue[MAX_FRAMES_IN_QUEUE - i - 1] = sFrameQueue[MAX_FRAMES_IN_QUEUE - i - 2];
        }

        sFrameQueue[0] = gEncoderState.current_frame;
		if (sFrameQueueSize < MAX_FRAMES_IN_QUEUE)
			sFrameQueueSize++;
    }
}
#if 1
int CompareBlockToFrame(uint8_t *prev, uint8_t *buffer, int stride, int plane)
{
	int diff = 0;

	if (plane == 0)	// luma
	{
		int score = 0;

		for (int y = 0; y < 16; y++)
		{
			for (int x = 0; x < 16; x++)
			{
				diff = prev[(y*stride)+x] - buffer[(y*stride)+x];
				score += (diff * diff);	// 
			}
		}

		return score;
	}
	else	// chroma
	{
		int score = 0;

		for (int y = 0; y < 8; y++)
		{
			for (int x = 0; x < 16; x++)
			{
				diff = prev[(y*stride)+x] - buffer[(y*stride)+x];
				score += (diff * diff);	// 
			}
		}

		return score;
	}
}
#else
int CompareBlockToFrame(uint8_t *prev, uint8_t *buffer, int stride, int plane)
{
	int diff = 0;

	if (plane == 0)	// luma
	{
		int score = 0;
		int16_t *midABS_Table = &gABS_Table[1<<15];

		// check edges more strictly than middle
		for (int x = 0; x < 16; x++)
		{
			diff = midABS_Table[(prev[x] - buffer[x])];	// top edge
			if (diff > MAX_PIXEL_DIFF_EDGE)
				return 0x7fff;	// No good
			///	score += diff;	// 
			diff = midABS_Table[(prev[(15*stride)+x] - buffer[(15*stride)+x])];	// bottom edge
			if (diff > MAX_PIXEL_DIFF_EDGE)
				return 0x7fff;	// No good
			//	score += diff;	// 
		}
		for (int y = 1; y < 15; y++)
		{
			diff = midABS_Table[(prev[y*stride] - buffer[y*stride])];	// left edge
			if (diff > MAX_PIXEL_DIFF_EDGE)
				return 0x7fff;	// No good
			//	score += diff;	// 
			diff = midABS_Table[(prev[(y*stride)+15] - buffer[(y*stride)+15])];	// right edge
			if (diff > MAX_PIXEL_DIFF_EDGE)
				return 0x7fff;	// No good
			//	score += diff;	// 
		}
		// possibly turn off/reduce center checks for lower quality setting			
		// non-edge
		for (int y = 1; y < 15; y++)
		{
			for (int x = 1; x < 15; x++)
			{
				diff = midABS_Table[(prev[(y*stride)+x] - buffer[(y*stride)+x])];
				if (diff > MAX_PIXEL_DIFF)
					return 0x7fff;	// No good
				//		score += diff;	// 
			}
		}

		return score;
	}
	else	// chroma
	{
		int score = 0;
		int16_t *midABS_Table = &gABS_Table[1<<15];

		for (int y = 0; y < 8; y++)
		{
			for (int x = 0; x < 16; x++)
			{
				diff = midABS_Table[(prev[(y*stride)+x] - buffer[(y*stride)+x])];
				if (diff > MAX_PIXEL_DIFF_CHROMA)
					return 0x7fff;	// No good
				//		score += diff >> 2;	// add one if greater than four
			}
		}

		return score;
	}
}
#endif

#ifdef WRITE_FRAMES_TO_FILE
extern FILE *sFrameDataFile;
extern uint8_t *gFrameBuffer;
extern size_t gFrameBufferIndex;
extern uint32_t gFrameBufferCount;

void OutputFrameDataToFile(YUVFrameType *frame)
{
    int width = frame->width;
    int height = frame->height;

#if 0 // save to buffer first
	const unsigned int max_buffer = WRITE_FRAME_BUFFER_SIZE;
	if ((gFrameBufferIndex + (width * height * 3 / 2)) >= max_buffer)
		return;	// no more

	gFrameBufferCount++;

    int pixelCount = width * height;
    memcpy(&gFrameBuffer[gFrameBufferIndex], frame->Y_frame_buffer, pixelCount);
    gFrameBufferIndex += pixelCount;

    pixelCount /= 2;
    memcpy(&gFrameBuffer[gFrameBufferIndex], frame->UV_frame_buffer, pixelCount);
    gFrameBufferIndex += pixelCount;
#else
    gFrameBufferCount++;

    // write Y
    fwrite(frame->Y_frame_buffer, sizeof(uint8_t), width * height, sFrameDataFile);

    // write UV
    fwrite(frame->UV_frame_buffer, sizeof(uint8_t), width * height / 2, sFrameDataFile);
#endif
}
#endif

void OutputFrameDataToConsole(YUVFrameType *frame)
{
	char str[256];
	sprintf_s(str, "\nstatic const uint8_t sTestLumaData[%d][%d] = {\n", gEncoderState.height, gEncoderState.width);
	OutputDebugString(str);

	for (int j = 0; j < gEncoderState.height; j++)
	{
		OutputDebugString("{ ");
		for (int i=0;i<gEncoderState.width;i++)
		{
			if (i < gEncoderState.width-1)
				sprintf_s(str, "%d,", frame->Y_frame_buffer[j*gEncoderState.width+i]);
			else
				sprintf_s(str, "%d",  frame->Y_frame_buffer[j*gEncoderState.width+i]);
			OutputDebugString(str);
		}
		if (j < (gEncoderState.height-1))
			OutputDebugString("}, \n");
		else
			OutputDebugString("} \n");
	}
	OutputDebugString("};\n");

	sprintf_s(str, "\nstatic const uint8_t sTestChromaData_U[%d][%d] = {\n", gEncoderState.height/2, gEncoderState.width/2);
	OutputDebugString(str);

	for (int j = 0; j < (gEncoderState.height/2); j++)
	{
		OutputDebugString("{ ");
		for (int i=0;i < gEncoderState.width/2;i++)
		{
			char str[256];
			if (i < (gEncoderState.width/2)-1)
				sprintf_s(str, "%d,", frame->UV_frame_buffer[j*(gEncoderState.width/2)+i].u);
			else
				sprintf_s(str, "%d", frame->UV_frame_buffer[j*(gEncoderState.width/2)+i].u);
			OutputDebugString(str);
		}
		if (j < ((gEncoderState.height/2)-1))
			OutputDebugString("}, \n");
		else
			OutputDebugString("} \n");
	}
	OutputDebugString("};\n");

	sprintf_s(str, "\nstatic const uint8_t sTestChromaData_V[%d][%d] = {\n", gEncoderState.height/2, gEncoderState.width/2);
	OutputDebugString(str);

	for (int j = 0; j < (gEncoderState.height/2); j++)
	{
		OutputDebugString("{ ");
		for (int i=0;i < (gEncoderState.width/2)-1;i++)
		{
			char str[256];
			if (i < (gEncoderState.width/2)-1)
				sprintf_s(str, "%d,", frame->UV_frame_buffer[j*(gEncoderState.width/2)+i].v);
			else
				sprintf_s(str, "%d", frame->UV_frame_buffer[j*(gEncoderState.width/2)+i].v);
			OutputDebugString(str);
		}
		if (j < ((gEncoderState.height/2)-1))
			OutputDebugString("}, \n");
		else
			OutputDebugString("} \n");
	}
	OutputDebugString("};\n");

}

void DebugMacroBlockSet()
{
	int mb_inc = 0;
	char str[256];
	MacroBlockType *mb = GetCurrentMacroBlockSet();

	sprintf_s(str, "FRAME %d --------------- ", gEncoderState.total_frame_num+1);
	OutputDebugString(str);
	if (gEncoderState.slice_type == SLICE_TYPE_P)
	{
		OutputDebugString(" P Slice -----------------------\n  ");
		mb_inc = 5;
	}
	else
		OutputDebugString(" I Slice -----------------------\n  ");
	for (int i = 0; i < gEncoderState.mb_max; i++)
	{
		if (i && (i % gEncoderState.mb_width == 0))
			OutputDebugString("\n  ");

		if (mb[i].mb_skip_flag && (gEncoderState.slice_type == SLICE_TYPE_P))
			OutputDebugString("(PSKIP) ");
		else
		{
			sprintf_s(str, "(I-%2d) ", mb[i].mb_type + mb_inc);
			OutputDebugString(str);
		}
	}

	OutputDebugString("\n\n");
}

void Convert444to420(YUVFrameType *frame, uint8_t *f, int start_height, int end_height)
{
	int src_stride = frame->width * 4;
	int width = frame->width;
	uint8_t *r1 = f + (start_height * src_stride);
	uint8_t *r2 = r1 + src_stride;	// next line 
	uint8_t *y1  = frame->Y_frame_buffer + (start_height * width);
	uint8_t *y2  = y1 + width;
	uint8_t *uv = ((uint8_t *) frame->UV_frame_buffer) + (start_height * width / 2);
	uint8_t *s1, *s2, *d1, *d2, *d_uv;
//	__m128i mask = _mm_set1_epi16(0x00ff);

	for (int y = start_height; y < end_height; y+=2, r1 += (src_stride * 2), r2 += (src_stride * 2), y1 += (width * 2), y2 += (width * 2), uv += (width) )
	{
		s1 = r1;
		s2 = r2;
		d1 = y1;
		d2 = y2;
		d_uv = uv;
#ifdef USE_SSE2
		for (int x = 0; x < width; x += 4, s1 += 16, s2 += 16, d1 += 4, d2 += 4, d_uv += 4)
		{
			__m128i src1, src2, mm0, mm1;

			src1 = _mm_loadu_si128((__m128i *) s1);
			src2 = _mm_loadu_si128((__m128i *) s2);
#if 1		// slightly faster yuxv packed version
			d1[0] = src1.m128i_u8[0];
			d1[1] = src1.m128i_u8[4];
			d1[2] = src1.m128i_u8[8];
			d1[3] = src1.m128i_u8[12];
			d2[0] = src2.m128i_u8[0];
			d2[1] = src2.m128i_u8[4];
			d2[2] = src2.m128i_u8[8];
			d2[3] = src2.m128i_u8[12];

			src1 = _mm_srli_epi16(src1, 8);	// make u & v - 16-bit while masking out y
			src2 = _mm_srli_epi16(src2, 8);	// make u & v - 16-bit while masking out y
			mm0  = _mm_srli_epi64(src1, 32);	// line up adjacent uv for addition
			mm1  = _mm_srli_epi64(src2, 32);	// line up adjacent uv for addition
			src1 = _mm_add_epi16(src1, mm0);
			src1 = _mm_add_epi16(src1, mm1);
			src1 = _mm_add_epi16(src1, src2);
			src1 = _mm_srli_epi16(src1, 2);	// average 4 pixels

			d_uv[0] = (uint8_t) src1.m128i_u16[0];
			d_uv[1] = (uint8_t) src1.m128i_u16[1];
			d_uv[2] = (uint8_t) src1.m128i_u16[4];
			d_uv[3] = (uint8_t) src1.m128i_u16[5];
#else
			d1[0] = src1.m128i_u8[1];
			d1[1] = src1.m128i_u8[5];
			d1[2] = src1.m128i_u8[9];
			d1[3] = src1.m128i_u8[13];
			d2[0] = src2.m128i_u8[1];
			d2[1] = src2.m128i_u8[5];
			d2[2] = src2.m128i_u8[9];
			d2[3] = src2.m128i_u8[13];

			src1 = _mm_and_si128(src1, mask);	// make u & v - 16-bit while masking out y
			src2 = _mm_and_si128(src2, mask);	// make u & v - 16-bit while masking out y
			mm0  = _mm_srli_epi64(src1, 32);	// line up adjacent uv for addition
			mm1  = _mm_srli_epi64(src2, 32);	// line up adjacent uv for addition
			src1 = _mm_add_epi16(src1, mm0);
			src1 = _mm_add_epi16(src1, mm1);
			src1 = _mm_add_epi16(src1, src2);
			src1 = _mm_srli_epi16(src1, 2);	// average 4 pixels

			d_uv[0] = src1.m128i_u16[0];
			d_uv[1] = src1.m128i_u16[1];
			d_uv[2] = src1.m128i_u16[4];
			d_uv[3] = src1.m128i_u16[5];
#endif
		}
#else
		int avg_u, avg_v;

		for (int x = 0; x < width; x += 2, s1 += 8, s2 += 8, d1 += 2, d2 += 2, d_uv += 2)
		{
#if 0	// YUV packed
			d1[0]  = s1[0];
			avg_u  = s1[1];
			avg_v  = s1[2];
			d1[1]  = s1[4];
			avg_u += s1[5];
			avg_v += s1[6];

			d2[0]  = s2[0];
			avg_u += s2[1];
			avg_v += s2[2];
			d2[1]  = s2[4];
			avg_u += s2[5];
			avg_v += s2[6];
			d_uv[0] = avg_u >> 2;
			d_uv[1] = avg_v >> 2;
#else	// UYV packed
			d1[0]  = s1[1];
			avg_u  = s1[0];
			avg_v  = s1[2];
			d1[1]  = s1[5];
			avg_u += s1[4];
			avg_v += s1[6];

			d2[0]  = s2[1];
			avg_u += s2[0];
			avg_v += s2[2];
			d2[1]  = s2[5];
			avg_u += s2[4];
			avg_v += s2[6];
			d_uv[0] = avg_u >> 2;
			d_uv[1] = avg_v >> 2;
#endif
		}
#endif
	}
}

// Rate Control
#define MAX_ROLLING_FRAMES 60
static int sRollingTotal = 0;
static int sRollingSize = 0;
static int sRollingIndex = 0;
static int sRollingSpaces = 0;
static int sRollingFrameSize[MAX_ROLLING_FRAMES];

void InitRateControl_old()
{
	int fps = gEncoderState.fps;
	if (fps > MAX_ROLLING_FRAMES)
		fps = MAX_ROLLING_FRAMES;
	sRollingIndex = 0;
	sRollingSpaces = fps;
	sRollingSize = fps;
	sRollingTotal = 0;

}

void UpdateRollingTotal(int frame_size)
{
	frame_size *= 8;	// make it in bits
//	ENC_TRACE("%d: frame_size: %d\n", gEncoderState.total_frame_num, frame_size);
	sFrame_LastFrameSize = frame_size;

	if (sRollingSize == 0)
	{
		int fps = gEncoderState.idr_frame_interval;
		if (fps > MAX_ROLLING_FRAMES)
			fps = MAX_ROLLING_FRAMES;
		sRollingIndex = 0;
		sRollingSpaces = fps;
		sRollingSize = fps;
		sRollingTotal = 0;
	}

	if (sRollingSpaces == 0)
	{
		if (sRollingIndex >= sRollingSize)
			sRollingIndex = 0;
		sRollingTotal -= sRollingFrameSize[sRollingIndex];
		sRollingFrameSize[sRollingIndex++] = frame_size;
		sRollingTotal += frame_size;
	}
	else
	{
		sRollingFrameSize[sRollingIndex++] = frame_size;
		sRollingTotal += frame_size;
		sRollingSpaces--;
	}
}

float GetRollingAverage()
{
	int frames_so_far = sRollingSize - sRollingSpaces;
	float rolling_avg;

	if (frames_so_far == 0)
		rolling_avg = 0.0f;
	else
		rolling_avg = (float) sRollingTotal / (float) frames_so_far;

	return rolling_avg;
}

int CalcTargetFrameSize()
{
	int frames_so_far = sRollingSize - sRollingSpaces;
	int rolling_avg;
	int target_frame_size = gEncoderState.bit_rate * 1000 / gEncoderState.fps;

	if (frames_so_far == 0)
		rolling_avg = target_frame_size;
	else
		rolling_avg = sRollingTotal / frames_so_far;


	int target_rate_diff = target_frame_size - rolling_avg;

	target_frame_size += (target_rate_diff / (gEncoderState.fps / 2));	// try to make up for it in half a second.

	sFrame_TargetSize = target_frame_size;
//	ENC_TRACE("%d: target_frame_size: %d\n", gEncoderState.total_frame_num, target_frame_size);

	return target_frame_size;
}

int FindTargetQP(int target_mb_size)
{
	if (gEncoderState.rate_control)
	{
		if (gEncoderState.total_frame_num == 0)
		{
			sFrame_Qp = 35;

			return 35;
		}

		static int divisor = 1000;
		static int top = 2740;
		int target_qp = (top - target_mb_size) * 115 / divisor;
		target_qp = CLIP3(13,39, target_qp);
		//	ENC_TRACE("Target QP: %d  target_mb_size: %d\n", target_qp, target_mb_size);
		sFrame_Qp = target_qp;

		return (target_qp);
	}

	sFrame_Qp = gEncoderState.qp + gEncoderState.current_qp_delta;
	return (gEncoderState.qp + gEncoderState.current_qp_delta);
}

void ExitRateControl_old()
{

}

void SetupReferenceFrames(YUVFrameType *frame, int frame_type)
{
	// set up reference frame pointers
	int i = 0;
	YUVFrameType *tmp = GetQueuedFrame(i++);
	if ((frame_type == ORIGIN_H264_IDR) || (frame_type == ORIGIN_H264_P_SLICE))
	{
		while (tmp && (tmp->slice_type != SLICE_TYPE_I && tmp->slice_type != SLICE_TYPE_P))
			tmp = GetQueuedFrame(i++);

		frame->prev_ref_frame = tmp;
		frame->next_ref_frame = NULL;
	}
	else
	{	// assume B Slice
		while (tmp && (tmp->nal_ref_idc == 0))	// find first ref frame (even if it is another B slice)
			tmp = GetQueuedFrame(i++);

		frame->next_ref_frame = tmp;	// for B slice this is the forward reference frame since we encode out of order
		// search back one more for the back reference frame
		tmp = GetQueuedFrame(i++);
		// find next ref frame that is I or P slice and whose pic order cnt is less than our current poc - because for b_frames > 1 there will be a 
		// P slice in the queue that is display after this frame which can't be used for a prev_ref_frame
		while (tmp && ((tmp->slice_type != SLICE_TYPE_I && tmp->slice_type != SLICE_TYPE_P) || ((tmp->slice_type == SLICE_TYPE_P) && (tmp->pic_order_cnt > frame->pic_order_cnt))))
			tmp = GetQueuedFrame(i++);

		frame->prev_ref_frame = tmp;
	}

	ME_SetRefFrame_TemporalMV(frame);
}

void PadFrameEdges_Luma(YUVFrameType *frame, int frame_selector)
{
	uint8_t *src = frame->luma_buffers[frame_selector];
	uint8_t *left = src - FRAME_WIDTH_PAD;
	uint8_t *right = src + frame->width;
	uint8_t value;

	// left and right edges
	for (int i = 0; i < frame->height; i++, src += frame->stride_padded, left += frame->stride_padded, right += frame->stride_padded)
	{
		value = src[0];
		memset(left, value, FRAME_WIDTH_PAD);
		value = right[-1];
		memset(right, value, FRAME_WIDTH_PAD);
	}

	// top edge
	src = frame->luma_buffers[frame_selector] - FRAME_WIDTH_PAD;
	uint8_t *dst = src - (FRAME_HEIGHT_PAD * frame->stride_padded);	// very top of buffer
	for (int i = 0; i < FRAME_HEIGHT_PAD; i++, dst += frame->stride_padded)
	{
		memcpy(dst, src, frame->stride_padded);
	}

	// bottom edge
	src = frame->luma_buffers[frame_selector] - FRAME_WIDTH_PAD + (frame->stride_padded * (frame->height - 1));
	dst = src + (frame->stride_padded);
	for (int i = 0; i < FRAME_HEIGHT_PAD; i++, dst += frame->stride_padded)
	{
		memcpy(dst, src, frame->stride_padded);
	}
}

void PadFrameEdges_Chroma(YUVFrameType *frame)
{
	uint16_t *src = (uint16_t *) frame->UV_frame_reconstructed;
	uint16_t *left = src - FRAME_WIDTH_PAD;
	uint16_t *right = src + frame->uv_width;
	uint16_t value;
	int uv_width_padded = frame->uv_width_padded;

	// left and right edges
	for (int i = 0; i < frame->uv_height; i++, src += uv_width_padded, left += uv_width_padded, right += uv_width_padded)
	{
		value = src[0];
		for (int j = 0; j < FRAME_WIDTH_PAD; j++)
			left[j] = value;
		value = right[-1];
		for (int j = 0; j < FRAME_WIDTH_PAD; j++)
			right[j] = value;
	}

	// top edge
	src = (uint16_t *) frame->UV_frame_reconstructed - FRAME_WIDTH_PAD;
	uint16_t *dst = src - (FRAME_HEIGHT_PAD * uv_width_padded);	// very top of buffer
	for (int i = 0; i < FRAME_HEIGHT_PAD; i++, dst += uv_width_padded)
	{
		memcpy(dst, src, frame->uv_stride_padded);
	}

	// bottom edge
	src = (uint16_t *) frame->UV_frame_reconstructed - FRAME_WIDTH_PAD + (uv_width_padded * (frame->uv_height - 1));
	dst = src + (uv_width_padded);
	for (int i = 0; i < FRAME_HEIGHT_PAD; i++, dst += uv_width_padded)
	{
		memcpy(dst, src, frame->uv_stride_padded);
	}
}

void DisplayBlock(int mb_index, char *str, bool show_pred)
{
	MacroBlockType *mb = GetMacroBlock(mb_index);

	ENC_TRACE("\nDisplaying MB: %4d  Frame: %d - %s\n\n", mb_index, gEncoderState.total_frame_num, (str == NULL) ? "" : str);

	for (int j = 0; j < 16; j++)
	{
		int block_start_y = j * mb->stride + mb->Y_block_start_index;

		for (int i = 0; i < 16; i++)
		{
			ENC_TRACE("%3d ", mb->Y_frame_reconstructed[block_start_y+i]);
			if ((i != 15) && (((i + 1) % 4) == 0))
				ENC_TRACE("| ");
		}
		ENC_TRACE("\n");
		if ((j != 15) && (((j + 1) % 4) == 0))
			ENC_TRACE("----------------+-----------------+-----------------+-----------------\n");
	}

	ENC_TRACE("\n");

	for(int uv = 0; uv < 2; uv++)
	{
		for (int j = 0; j < 8; j++)
		{
			int block_start_y = j * mb->uv_stride + mb->UV_block_start_index;

			for (int i = 0; i < 8; i++)
			{
				ENC_TRACE("%3d ", uv ? mb->UV_frame_reconstructed[block_start_y+i].v : mb->UV_frame_reconstructed[block_start_y+i].u);
				if (i == 3)
					ENC_TRACE("| ");
			}
			ENC_TRACE("\n");
			if (j == 3)
				ENC_TRACE("----------------+----------------\n");
		}
		ENC_TRACE("\n");
	}

	if (show_pred)
	{
		if (!mb->isI16x16)
			ME_Prediction(mb, ME_PRED_DIR_BACK);
		ENC_TRACE("Pred block:\n");
		for (int j = 0; j < 16; j++)
		{
			u8_16x16_Type *pb = mb->Y_pred_pixels;
			for (int i = 0; i < 16; i++)
			{
				ENC_TRACE("%3d ", pb->block[j][i]);
				if ((i != 15) && (((i + 1) % 4) == 0))
					ENC_TRACE("| ");
			}
			ENC_TRACE("\n");
			if ((j != 15) && (((j + 1) % 4) == 0))
				ENC_TRACE("----------------+-----------------+-----------------+-----------------\n");
		}

        ENC_TRACE("\n");

        Chroma_8x8_Type *pb = mb->UV_pred_pixels;
        for(int uv = 0; uv < 2; uv++)
        {
            for (int j = 0; j < 8; j++)
            {
                for (int i = 0; i < 8; i++)
                {
                    ENC_TRACE("%3d ", uv ? pb->block[j][i].v : pb->block[j][i].u);
                    if (i == 3)
                        ENC_TRACE("| ");
                }
                ENC_TRACE("\n");
                if (j == 3)
                    ENC_TRACE("----------------+----------------\n");
            }
            ENC_TRACE("\n");
        }
	}
}

void ProcessFrame(int width, int height, uint8_t *y_data, uint8_t *u_data, uint8_t *v_data, int frame_type, int b_group_index)
{
	ENC_START_TIMER(5,"Alloc and Copy Frame");

	YUVFrameType *frame = sFrameQueue[MAX_FRAMES_IN_QUEUE-1];

	frame->target_frame_size = CalcTargetFrameSize();

    frame->width = width;
    frame->height = height;  // for now assume height is 32-byte aligned
    frame->stride = width;   // for now assume width is 32-byte aligned
	frame->stride_padded = gEncoderState.width_padded;   // for now assume width is 32-byte aligned

    frame->uv_width = width / 2; // for now assume YUV420 format
	frame->uv_width_padded = gEncoderState.uv_width_padded;
    frame->uv_height = height / 2;
	frame->uv_stride = frame->uv_width;   // for now assume width is 32-byte aligned
	frame->uv_stride_padded = gEncoderState.uv_width_padded * 2;   // for now assume width is 32-byte aligned

	frame->in_loop_filter_off = !gEncoderState.in_loop_filtering_on;	// default
	frame->slice_alpha_c0_offset = 0;
	frame->slice_beta_offset     = 0;

	if (gEncoderState.b_slice_frames == 0)
		frame->pic_order_cnt = gEncoderState.total_frame_num;
	else
		frame->pic_order_cnt = gEncoderState.current_frame_display_index;
	frame->frame_number = gEncoderState.total_frame_num;

	frame->frame_mad = 0;

	// likely to change when RateControl_Init_Frame() supports checking for best b/p groups
	SetupReferenceFrames(frame, frame_type);

	frame->luma_buffers[0] = frame->Y_frame_reconstructed;

#ifdef ENABLE_TRACE
	frame->call_counter[0] = 0;
#endif

#ifdef USE_FRAME_BUFFERS_FROM_THREAD
	frame->Y_frame_buffer = y_data;
	frame->UV_frame_buffer = (ChromaPixelType *) u_data;
#else
	if (v_data)
	{
		memcpy(frame->Y_frame_buffer, y_data, width * height);
		for (int i = 0; i < (width * height / 4); i++)
		{
			frame->UV_frame_buffer[i].u = u_data[i];
			frame->UV_frame_buffer[i].v = v_data[i];
		}
	}
	else
	if (u_data)
	{
		memcpy(frame->Y_frame_buffer, y_data, width * height);
		memcpy(frame->UV_frame_buffer, u_data, width * height / 2);
	}
	else
	{
		if (gEncoderState.multi_threading_on)
			ManageConversion(frame, y_data);
		else
			Convert444to420(frame, y_data, 0, height);
	}
#endif
#ifdef WRITE_FRAMES_TO_FILE
    OutputFrameDataToFile(frame);
#endif

#if 0
	if (gEncoderState.total_frame_num == 0)
		OutputFrameDataToConsole(frame);
#endif

    int num_macro_blocks = width * height / (16 * 16);

	switch(frame_type)
	{
	case ORIGIN_H264_IDR:
		frame->slice_type = SLICE_TYPE_I;
		break;
	case ORIGIN_H264_P_SLICE:
		frame->slice_type = SLICE_TYPE_P;
		break;
	case ORIGIN_H264_B_SLICE:
		frame->slice_type = SLICE_TYPE_B;
		break;
	default:
		ENC_ASSERT(0);	// should not come here
	}
	gEncoderState.slice_type = frame->slice_type;

	gEncoderState.is_I_slice = gEncoderState.is_P_slice = gEncoderState.is_B_slice = false;
	switch(gEncoderState.slice_type)
	{
	case SLICE_TYPE_I:
	case SLICE_TYPE_SI:
		gEncoderState.is_I_slice = true;
		break;
	case SLICE_TYPE_P:
	case SLICE_TYPE_SP:
		gEncoderState.is_P_slice = true;
		break;
	case SLICE_TYPE_B:
		gEncoderState.is_B_slice = true;
		break;
	}

    gEncoderState.current_frame = frame;

	ENC_START_TIMER(1,"SliceTypeDecision");
    SetFrameForMBProcessing(frame);
	gEncoderState.mb_debug_on = false;
   
	gEncoderState.current_mb_set = GetCurrentMacroBlockSet();

	RateControl_Init_Frame();

	ENC_PAUSE_TIMER(1,false);
	ENC_PAUSE_TIMER(5,false);

	int target_row_size = frame->target_frame_size / (frame->height / 16);
	int target_mb_size = 100 * target_row_size / (frame->width / 16);
    H264_UNUSED(target_row_size)
    H264_UNUSED(target_mb_size)

	int row_qp = sFrame_Qp = RateControl_GetFrameQP();

	gEncoderState.current_qp_delta = row_qp - gEncoderState.qp;

	if (gEncoderState.idr)
		gEncoderState.nal_ref_idc = 3;
	else
	{
		if (gEncoderState.is_B_slice)
		{
			if (gEncoderState.b_slice_frames == 1)
				gEncoderState.nal_ref_idc = 0;
			else
				gEncoderState.nal_ref_idc = (b_group_index == (gEncoderState.b_slice_frames - 1)) ? 2 : 0;
		}
		else
			gEncoderState.nal_ref_idc = 2;
	}

	frame->nal_ref_idc = gEncoderState.nal_ref_idc;

	SetHalfPixelFrameRef(frame);

	ENC_START_TIMER(8,"CABAC_StartSlice");
	CABAC_StartSlice(frame->slice_type);
	ENC_PAUSE_TIMER(8,false);

	ENC_START_TIMER(9,"ProcessMacroBlocks");
	if (gEncoderState.multi_threading_on)
	{
		ManageThreads(frame);
	}
	else
	{
		for (int i = 0; i < num_macro_blocks; i++)
			ProcessMacroBlock(frame, i, row_qp);
#ifdef _DEBUG
#if 0
		if ((gEncoderState.total_frame_num == 0) || (gEncoderState.total_frame_num == 1))
		{
            DisplayBlock(1, "Pre-LP", true);
            DisplayBlock(80, "Pre-LP", true);
            DisplayBlock(81, "Pre-LP", true);
//            DisplayBlock(1810, "Pre-LP", true);
//            DisplayBlock(1811, "Pre-LP", true);
//            DisplayBlock(1890, "Pre-LP", true);
		}
#endif
#if 0
		if (gEncoderState.total_frame_num == 61)
		{
			ENC_TRACE("Pre-LP\n\n")
				DisplayBlock(87, "Pre-LP", true);

			for (int i = 0; i < 160; i++)
				DisplayBlock(i, "Pre-LP");
		}
#endif
#endif
		if (gEncoderState.in_loop_filtering_on)
		{
			ENC_START_TIMER(15,"In-Loop Filter");
			FilterAllMacroBlocks();
			ENC_PAUSE_TIMER(15,false);
		}

		PadFrameEdges_Luma(frame);
		PadFrameEdges_Chroma(frame);
	}
	ENC_PAUSE_TIMER(9,false);
	ENC_START_TIMER(14,"EndSlice");


#ifdef _DEBUG
#if 0
    if ((gEncoderState.total_frame_num == 0) || (gEncoderState.total_frame_num == 1))
    {
        DisplayBlock(1, "Post-LP");
        DisplayBlock(80, "Post-LP");
        DisplayBlock(81, "Post-LP");
//        DisplayBlock(1810, "Post-LP");
//        DisplayBlock(1811, "Post-LP");
//        DisplayBlock(1890, "Post-LP");
    }
#endif
#if 0
    if (gEncoderState.total_frame_num == 61)
    {
        ENC_TRACE("Final\n\n")
            for (int i = 0; i < 160; i++)
                DisplayBlock(i, "Post-LP");
    }
#endif
#endif
	CABAC_EndSlice();

	RateControl_UpdateFrame(frame->total_encoded_bits, frame->total_coeff_bits);

	ENC_LAP_TIMER(1);
	ENC_LAP_TIMER(2);
	ENC_LAP_TIMER(3);
	ENC_LAP_TIMER(4);
	ENC_LAP_TIMER(5);
	ENC_LAP_TIMER(9);
	ENC_LAP_TIMER(10);
	ENC_LAP_TIMER(11);
	ENC_LAP_TIMER(12);
	ENC_LAP_TIMER(13);
	ENC_LAP_TIMER(15);
	ENC_LAP_TIMER(17);

	ENC_LAP_TIMER(20);
	ENC_LAP_TIMER(21);

//	DebugMacroBlockSet();

	if (!gEncoderState.multi_threading_on && (gEncoderState.subpixel_motion_est > 0) && (frame->nal_ref_idc != 0))
	{
		ENC_START_TIMER(18, "ComputeSubPixelImages");
		ME_ComputeSubPixelImages(2);
		ENC_PAUSE_TIMER(18,false);
	}

	AddCurrentFrameToQueue();

	ENC_PAUSE_TIMER(14,false);

	ENC_START_TIMER(6,"Write out NALUs");
	ResetNALU_Table(gEncoderState.current_frame_timestamp, true, (gEncoderState.idr ? ORIGIN_H264_IDR : ORIGIN_H264_DEFAULT_TYPE));

	__declspec(align(16)) uint8_t sps_buffer[256];
	__declspec(align(16)) uint8_t pps_buffer[64];
	__declspec(align(16)) uint8_t sei_buffer[2048];
	int length;

	length = NALU_Write_SPS(&gSPS_Parameters, sps_buffer);
	AddTo_NALU_Table(sps_buffer, length, 7);
	length = NALU_Write_PPS(&gPPS_Parameters, 0, pps_buffer);
	AddTo_NALU_Table(pps_buffer, length, 8);
	length = NALU_Write_SEI(0, sei_buffer);
	AddTo_NALU_Table(sei_buffer, length, 6);
	ENC_PAUSE_TIMER(6,false);

	ENC_START_TIMER(7,"Write out Slice NALU");
	int cabac_len = 0;
	uint8_t *cabac_bs = CABAC_GetBitStream(&cabac_len);

	if (gEncoderState.idr)
		AddTo_NALU_Table(cabac_bs, cabac_len, 5, gEncoderState.nal_ref_idc, false);	// IDR
	else
		AddTo_NALU_Table(cabac_bs, cabac_len, 1, gEncoderState.nal_ref_idc, false);	// Slice

	int nal_count;
	NAL_Type *naltable = (NAL_Type *) GetNALUs(&nal_count);
	naltable[nal_count-1].i_last_mb = num_macro_blocks-1;

	UpdateRollingTotal(cabac_len);

	ENC_PAUSE_TIMER(7,false);

	ENC_LAP_TIMER(6);
	ENC_LAP_TIMER(7);
	ENC_LAP_TIMER(14);

}

