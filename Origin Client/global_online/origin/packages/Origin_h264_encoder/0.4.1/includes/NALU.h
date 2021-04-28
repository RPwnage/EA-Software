#ifndef __NALU_H
#define __NALU_H

#include <stdint.h>
#include "Origin_h264_encoder.h"

#define MAIN_PROFILE_IDC 77 // see A.2.2
#define	HIGH_PROFILE_IDC 100

// VUI Parameters
typedef struct 
{
	bool aspect_ratio_info_present_flag;
	int aspect_ratio_idc;
	int sar_width;
	int sar_height;
	bool overscan_info_present_flag;
	bool overscan_appropriate_flag;
	bool video_signal_type_present_flag;
	int video_format;
	bool video_full_range_flag;
	bool colour_description_present_flag;
	int colour_primaries;
	int transfer_characteristics; 
	int matrix_coefficients;
	bool chroma_location_info_present_flag;
	int chroma_sample_loc_type_top_field;
	int chroma_sample_loc_type_bottom_field;
	bool timing_info_present_flag;
	int num_units_in_tick;
	int time_scale;
	bool fixed_frame_rate_flag;
	bool nal_hrd_parameters_present_flag;
	int nal_cpb_cnt_minus1;
	int nal_bit_rate_scale;
	int nal_cpb_size_scale;
	int nal_bit_rate_value_minus1;
	int nal_cpb_size_value_minus1;
	bool nal_vbr_cbr_flag;
	int nal_initial_cpb_removal_delay_length_minus1;
	int nal_cpb_removal_delay_length_minus1;
	int nal_dpb_output_delay_length_minus1;
	int nal_time_offset_length;
	bool vcl_hrd_parameters_present_flag;
	int vcl_cpb_cnt_minus1;
	int vcl_bit_rate_scale;
	int vcl_cpb_size_scale;
	int vcl_bit_rate_value_minus1;
	int vcl_cpb_size_value_minus1;
	bool vcl_vbr_cbr_flag;
	int vcl_initial_cpb_removal_delay_length_minus1;
	int vcl_cpb_removal_delay_length_minus1;
	int vcl_dpb_output_delay_length_minus1;
	int vcl_time_offset_length;
	bool low_delay_hrd_flag;
	bool pic_struct_present_flag;
	bool bitstream_restriction_flag;
	bool motion_vectors_over_pic_boundaries_flag;
	int max_bytes_per_pic_denom;
	int max_bits_per_mb_denom;
	int log2_max_mv_length_vertical;
	int log2_max_mv_length_horizontal;
	int num_reorder_frames;
	int max_dec_frame_buffering;
} Origin_H264_VUI_InputParameter_Type;


#define MAXSPS  32
#define MAXPPS  256

#define MAXIMUMVALUEOFcpb_cnt   32
typedef struct
{
	unsigned int cpb_cnt_minus1;                                   // ue(v)
	unsigned int bit_rate_scale;                                   // u(4)
	unsigned int cpb_size_scale;                                   // u(4)
	unsigned int bit_rate_value_minus1 [MAXIMUMVALUEOFcpb_cnt];    // ue(v)
	unsigned int cpb_size_value_minus1 [MAXIMUMVALUEOFcpb_cnt];    // ue(v)
	unsigned int cbr_flag              [MAXIMUMVALUEOFcpb_cnt];    // u(1)
	unsigned int initial_cpb_removal_delay_length_minus1;          // u(5)
	unsigned int cpb_removal_delay_length_minus1;                  // u(5)
	unsigned int dpb_output_delay_length_minus1;                   // u(5)
	unsigned int time_offset_length;                               // u(5)
} HRD_Parameters_Type;


typedef struct
{
	bool      aspect_ratio_info_present_flag;                   // u(1)
	unsigned int aspect_ratio_idc;                                 // u(8)
	unsigned short sar_width;                                      // u(16)
	unsigned short sar_height;                                     // u(16)
	bool      overscan_info_present_flag;                       // u(1)
	bool      overscan_appropriate_flag;                        // u(1)
	bool      video_signal_type_present_flag;                   // u(1)
	unsigned int video_format;                                     // u(3)
	bool      video_full_range_flag;                            // u(1)
	bool      colour_description_present_flag;                  // u(1)
	unsigned int colour_primaries;                                 // u(8)
	unsigned int transfer_characteristics;                         // u(8)
	unsigned int matrix_coefficients;                              // u(8)
	bool      chroma_location_info_present_flag;                // u(1)
	unsigned int  chroma_sample_loc_type_top_field;                // ue(v)
	unsigned int  chroma_sample_loc_type_bottom_field;             // ue(v)
	bool      timing_info_present_flag;                         // u(1)
	unsigned int num_units_in_tick;                                // u(32)
	unsigned int time_scale;                                       // u(32)
	bool      fixed_frame_rate_flag;                            // u(1)
	bool      nal_hrd_parameters_present_flag;                  // u(1)
	HRD_Parameters_Type nal_hrd_parameters;                           // hrd_paramters_t
	bool      vcl_hrd_parameters_present_flag;                  // u(1)
	HRD_Parameters_Type vcl_hrd_parameters;                           // hrd_paramters_t
	// if ((nal_hrd_parameters_present_flag || (vcl_hrd_parameters_present_flag))
	bool      low_delay_hrd_flag;                               // u(1)
	bool      pic_struct_present_flag;                          // u(1)
	bool      bitstream_restriction_flag;                       // u(1)
	bool      motion_vectors_over_pic_boundaries_flag;          // u(1)
	unsigned int max_bytes_per_pic_denom;                          // ue(v)
	unsigned int max_bits_per_mb_denom;                            // ue(v)
	unsigned int log2_max_mv_length_vertical;                      // ue(v)
	unsigned int log2_max_mv_length_horizontal;                    // ue(v)
	unsigned int num_reorder_frames;                               // ue(v)
	unsigned int max_dec_frame_buffering;                          // ue(v)
} VUI_SEQ_Parameters_Type;


#define MAXnum_slice_groups_minus1  8
typedef struct
{
	bool   Valid;                  // indicates the parameter set is valid
	unsigned int pic_parameter_set_id;                             // ue(v)
	unsigned int seq_parameter_set_id;                             // ue(v)
	bool   entropy_coding_mode_flag;                            // u(1)
	bool   transform_8x8_mode_flag;                             // u(1)

	bool   pic_scaling_matrix_present_flag;                     // u(1)
	int       pic_scaling_list_present_flag[12];                   // u(1)
	int       ScalingList4x4[6][16];                               // se(v)
	int       ScalingList8x8[6][64];                               // se(v)
	bool   UseDefaultScalingMatrix4x4Flag[6];
	bool   UseDefaultScalingMatrix8x8Flag[6];

	// if( pic_order_cnt_type < 2 )  in the sequence parameter set
	bool      bottom_field_pic_order_in_frame_present_flag;                           // u(1)
	unsigned int num_slice_groups_minus1;                          // ue(v)
	unsigned int slice_group_map_type;                        // ue(v)
	// if( slice_group_map_type = = 0 )
	unsigned int run_length_minus1[MAXnum_slice_groups_minus1]; // ue(v)
	// else if( slice_group_map_type = = 2 )
	unsigned int top_left[MAXnum_slice_groups_minus1];         // ue(v)
	unsigned int bottom_right[MAXnum_slice_groups_minus1];     // ue(v)
	// else if( slice_group_map_type = = 3 || 4 || 5
	bool   slice_group_change_direction_flag;            // u(1)
	unsigned int slice_group_change_rate_minus1;               // ue(v)
	// else if( slice_group_map_type = = 6 )
	unsigned int pic_size_in_map_units_minus1;             // ue(v)
	uint8_t      *slice_group_id;                              // complete MBAmap u(v)

	int num_ref_idx_l0_default_active_minus1;                     // ue(v)
	int num_ref_idx_l1_default_active_minus1;                     // ue(v)
	bool   weighted_pred_flag;                               // u(1)
	unsigned int  weighted_bipred_idc;                              // u(2)
	int       pic_init_qp_minus26;                              // se(v)
	int       pic_init_qs_minus26;                              // se(v)
	int       chroma_qp_index_offset;                           // se(v)

	int       cb_qp_index_offset;                               // se(v)
	int       cr_qp_index_offset;                               // se(v)
	int       second_chroma_qp_index_offset;                    // se(v)

	bool   deblocking_filter_control_present_flag;           // u(1)
	bool   constrained_intra_pred_flag;                      // u(1)
	bool   redundant_pic_cnt_present_flag;                   // u(1)
	bool   vui_pic_parameters_flag;                          // u(1)
} Pic_Parameter_Set_Type;


#define MAXnum_ref_frames_in_pic_order_cnt_cycle  256
typedef struct
{
	bool   Valid;                  // indicates the parameter set is valid

	unsigned int profile_idc;                                       // u(8)
	bool   constraint_set0_flag;                                // u(1)
	bool   constraint_set1_flag;                                // u(1)
	bool   constraint_set2_flag;                                // u(1)
	bool   constraint_set3_flag;                                // u(1)
#if (MVC_EXTENSION_ENABLE)
	bool   constraint_set4_flag;                                // u(1)
	bool   constraint_set5_flag;                                // u(2)
#endif
	unsigned  int level_idc;                                        // u(8)
	unsigned  int seq_parameter_set_id;                             // ue(v)
	unsigned  int chroma_format_idc;                                // ue(v)

	bool   seq_scaling_matrix_present_flag;                   // u(1)
	int       seq_scaling_list_present_flag[12];                 // u(1)
	int       ScalingList4x4[6][16];                             // se(v)
	int       ScalingList8x8[6][64];                             // se(v)
	bool   UseDefaultScalingMatrix4x4Flag[6];
	bool   UseDefaultScalingMatrix8x8Flag[6];

	unsigned int bit_depth_luma_minus8;                            // ue(v)
	unsigned int bit_depth_chroma_minus8;                          // ue(v)
	unsigned int log2_max_frame_num_minus4;                        // ue(v)
	unsigned int pic_order_cnt_type;
	// if( pic_order_cnt_type == 0 )
	unsigned int log2_max_pic_order_cnt_lsb_minus4;                 // ue(v)
	// else if( pic_order_cnt_type == 1 )
	bool delta_pic_order_always_zero_flag;               // u(1)
	int     offset_for_non_ref_pic;                         // se(v)
	int     offset_for_top_to_bottom_field;                 // se(v)
	unsigned int num_ref_frames_in_pic_order_cnt_cycle;          // ue(v)
	// for( i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
	int   offset_for_ref_frame[MAXnum_ref_frames_in_pic_order_cnt_cycle];   // se(v)
	unsigned int max_num_ref_frames;                                   // ue(v)
	bool   gaps_in_frame_num_value_allowed_flag;             // u(1)
	unsigned int pic_width_in_mbs_minus1;                          // ue(v)
	unsigned int pic_height_in_map_units_minus1;                   // ue(v)
	bool   frame_mbs_only_flag;                              // u(1)
	// if( !frame_mbs_only_flag )
	bool   mb_adaptive_frame_field_flag;                   // u(1)
	bool   direct_8x8_inference_flag;                        // u(1)
	bool   frame_cropping_flag;                              // u(1)
	unsigned int frame_crop_left_offset;                // ue(v)
	unsigned int frame_crop_right_offset;               // ue(v)
	unsigned int frame_crop_top_offset;                 // ue(v)
	unsigned int frame_crop_bottom_offset;              // ue(v)
	bool   vui_parameters_present_flag;                      // u(1)
	VUI_SEQ_Parameters_Type vui_seq_parameters;                  // vui_seq_parameters_t
	unsigned  separate_colour_plane_flag;                       // u(1)
	int lossless_qpprime_flag;
} SEQ_Parameter_Set_Type;


typedef struct
{
	SEQ_Parameter_Set_Type sps;

	unsigned int bit_equal_to_one;
	int num_views_minus1;
	int *view_id;
	int *num_anchor_refs_l0;
	int **anchor_ref_l0;
	int *num_anchor_refs_l1;
	int **anchor_ref_l1;

	int *num_non_anchor_refs_l0;
	int **non_anchor_ref_l0;
	int *num_non_anchor_refs_l1;
	int **non_anchor_ref_l1;

	int num_level_values_signalled_minus1;
	int *level_idc;
	int *num_applicable_ops_minus1;
	int **applicable_op_temporal_id;
	int **applicable_op_num_target_views_minus1;
	int ***applicable_op_target_view_id;
	int **applicable_op_num_views_minus1;

	unsigned int mvc_vui_parameters_present_flag;
	bool   Valid;                  // indicates the parameter set is valid
	//	MVCVUI_t  MVCVUIParams;
} Subset_SEQ_Parameter_Set_Type;

typedef struct
{
    int i_ref_idc;  /* nal_priority_e */
    int i_type;     /* nal_unit_type_e */
    int b_long_startcode;
    int i_first_mb; /* If this NAL is a slice, the index of the first MB in the slice. */
    int i_last_mb;  /* If this NAL is a slice, the index of the last MB in the slice. */

    /* Size of payload in bytes. */
    int     i_payload;
    /* If param->b_annexb is set, Annex-B bytestream with startcode.
     * Otherwise, startcode is replaced with a 4-byte size.
     * This size is the size used in mp4/similar muxing; it is equal to i_payload-4 */
    uint8_t *p_payload;
} NAL_Type;

void SetBasicParameters(int width, int height, int qp, int max_bit_rate);
void NALU_Set_VUI_Input_Default();
void NALU_Set_VUI(SEQ_Parameter_Set_Type *sps);
void NALU_Set_SPS(SEQ_Parameter_Set_Type *sps, int id);
void NALU_Set_PPS(Pic_Parameter_Set_Type *pps, SEQ_Parameter_Set_Type *sps, int id,
	int WeightedPrediction,        //!< value of weighted_pred_flag
	int WeightedBiprediction,      //!< value of weighted_bipred_idc
	int cb_qp_index_offset,        //!< value of cb_qp_index_offset
	int cr_qp_index_offset         //!< value of cr_qp_index_offset
	);
int NALU_Write_SPS(SEQ_Parameter_Set_Type *sps, uint8_t *stream_buffer);
int NALU_Write_PPS(Pic_Parameter_Set_Type *pps, int id, uint8_t *stream_buffer);
int NALU_Write_SEI(int id, uint8_t *stream_buffer);

void InitNALU(int circular_buffer_size);
void ExitNALU();

void ResetNALU_Table(uint32_t timestamp, bool frame_data, int frame_type);
void AddTo_NALU_Table(uint8_t *bs, int bs_length, int nal_unit_type, int nal_ref_idc = 3, bool copy_only = false);
void *GetNALUs(int *nal_count);
int GetBufferedFrame_NALU(uint32_t timestamp, Origin_H264_EncoderOutputType *output);

extern SEQ_Parameter_Set_Type gSPS_Parameters;
extern Pic_Parameter_Set_Type gPPS_Parameters;

#endif