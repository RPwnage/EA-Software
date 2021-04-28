#ifndef __FRAME_H
#define __FRAME_H
#include <stdint.h>

#define FRAME_WIDTH_PAD 16
#define FRAME_HEIGHT_PAD 16

struct MacroBlock;

typedef struct  
{
	bool mb_skip_flag;
	bool available;
} MB_FastData_Type;

typedef struct  
{
	uint8_t u;
	uint8_t v;
} ChromaPixelType;

typedef struct FRAME_STRUCT
{
    int width;
    int height;
    int stride;
	int stride_padded;
    int uv_width;
	int uv_width_padded;
    int uv_height;
	int uv_stride;
	int uv_stride_padded;

	int slice_type;
	int nal_ref_idc;
	int pic_order_cnt;
	uint32_t frame_number;
	
	int frame_mad;
	int total_encoded_bits;
	int total_coeff_bits;

	bool in_loop_filter_off;
	int slice_alpha_c0_offset;
	int slice_beta_offset;

	int target_frame_size;	// in bytes

	uint8_t *frame_align_buffer;
    uint8_t *Y_frame_buffer;
	ChromaPixelType *UV_frame_buffer;

    // reconstructed frame
	uint8_t *reconstructed_align_buffer;
	uint8_t *Y_frame_reconstructed;
    ChromaPixelType *UV_frame_reconstructed;

	uint8_t *mb_aligned_data_buffer;

    // predicted quarter pixels
    uint8_t *quarter_align_buffer;
    uint8_t *Y_quarter_buffer;

    MacroBlock *mb_set;

	// currently only supporting one level of backward and forward reference frames
	struct FRAME_STRUCT *prev_ref_frame;	// for P or B slices
	struct FRAME_STRUCT *next_ref_frame;	// for B-slices

	// for sub-pixel buffer selection
	uint8_t *luma_buffers[4];

	int temporal_ref_dist;
	uint32_t temporal_mv_distscale;	// 256 / (frame count distance)
	struct TemporalMV *temporal_mv;

	int lambda_factor;
	int *lambda_table;

	// telemetry
	int call_counter[8];	// 0 - ComputeSAD_SSE2()
} YUVFrameType;

enum{	// subtract 5 if the slice type does not apply to the entire frame
	SLICE_TYPE_P = 5,  // 
	SLICE_TYPE_B = 6,
	SLICE_TYPE_I = 7,
	SLICE_TYPE_SP = 8,
	SLICE_TYPE_SI = 9
};


bool InitFrames();
void ExitFrames();

void Convert444to420(YUVFrameType *frame, uint8_t *f, int start_height, int end_height);

void SetupReferenceFrames(YUVFrameType *frame, int frame_type);
void PadFrameEdges_Luma(YUVFrameType *frame, int frame_selector = 0);
void PadFrameEdges_Chroma(YUVFrameType *frame);
void ProcessFrame(int width, int height, uint8_t *y_data, uint8_t *u_data, uint8_t *v_data, int frame_type, int b_group_index);

void *GetNALUs(int *nal_count);
int CompareBlockToFrame(uint8_t *prev, uint8_t *buffer, int stride, int plane);
YUVFrameType *GetQueuedFrame(int index);

void UpdateRollingTotal(int frame_size);
int CalcTargetFrameSize();
int FindTargetQP(int target_mb_size);

void DisplayBlock(int mb_index, char *str = 0, bool show_pred = false);

#endif