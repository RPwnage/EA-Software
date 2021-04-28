#include <windows.h>
#include <string.h>

#include "Origin_h264_encoder.h"
#include "NALU.h"
#include "BitStream.h"
#include "Tables.h"
#include "Thread.h"
#include "Trace.h"

//#define DEBUG_NALU_OUTPUT

Origin_H264_VUI_InputParameter_Type gVUI_InputParameters;
SEQ_Parameter_Set_Type gSPS_Parameters;
Pic_Parameter_Set_Type gPPS_Parameters;

void SetBasicParameters(int width, int height, int qp, int max_bit_rate)
{
	gEncoderState.width = width;
	gEncoderState.height = height;
	gEncoderState.qp = qp;
	gEncoderState.max_bit_rate = max_bit_rate;

	NALU_Set_VUI_Input_Default();

	NALU_Set_SPS(&gSPS_Parameters, 0);
	NALU_Set_PPS(&gPPS_Parameters, &gSPS_Parameters, 0, 0, 0, 0, 0);
}

void NALU_Set_VUI_Input_Default()
{
	Origin_H264_VUI_InputParameter_Type *vui = &gVUI_InputParameters;

	memset(vui, 0, sizeof(Origin_H264_VUI_InputParameter_Type));

	vui->timing_info_present_flag = 1;
	// timing info
	vui->num_units_in_tick = 1;
	vui->time_scale = 2000;
	vui->fixed_frame_rate_flag = 0;

	vui->bitstream_restriction_flag = 1;
	// bitstream restriction info
	vui->motion_vectors_over_pic_boundaries_flag = 1;
	vui->max_bits_per_mb_denom = 0;
	vui->max_bytes_per_pic_denom = 0;
	vui->log2_max_mv_length_horizontal = 10;
	vui->log2_max_mv_length_vertical = 10;
	vui->num_reorder_frames = gEncoderState.b_slice_frames;//2;
	vui->max_dec_frame_buffering = gEncoderState.b_slice_frames * 2;//4;
}

static int getMaxDpbSize(SEQ_Parameter_Set_Type *active_sps)
{
	int pic_size = (active_sps->pic_width_in_mbs_minus1 + 1) * (active_sps->pic_height_in_map_units_minus1 + 1) * (active_sps->frame_mbs_only_flag?1:2) * 384;

	int size = 0;

	switch (active_sps->level_idc)
	{
	case 9:
		size = 152064;
		break;
	case 10:
		size = 152064;
		break;
	case 11:
		if (!(active_sps->profile_idc >= HIGH_PROFILE_IDC) && (active_sps->constraint_set3_flag == 1))
			size = 152064;
		else
			size = 345600;
		break;
	case 12:
		size = 912384;
		break;
	case 13:
		size = 912384;
		break;
	case 20:
		size = 912384;
		break;
	case 21:
		size = 1824768;
		break;
	case 22:
		size = 3110400;
		break;
	case 30:
		size = 3110400;
		break;
	case 31:
		size = 6912000;
		break;
	case 32:
		size = 7864320;
		break;
	case 40:
		size = 12582912;
		break;
	case 41:
		size = 12582912;
		break;
	case 42:
		size = 13369344;
		break;
	case 50:
		size = 42393600;
		break;
	case 51:
		size = 70778880;
		break;
	case 52:
		size = 70778880;
		break;
	default:
		ENC_ASSERT (0);
		break;
	}

	size /= pic_size;
	size = IMIN( size, 16);

	return size;
}

void NALU_Set_VUI(SEQ_Parameter_Set_Type *sps)
{
	unsigned int          SchedSelIdx;
	HRD_Parameters_Type     *nal_hrd = &(sps->vui_seq_parameters.nal_hrd_parameters);
	HRD_Parameters_Type     *vcl_hrd = &(sps->vui_seq_parameters.vcl_hrd_parameters);
	VUI_SEQ_Parameters_Type *vui     = &(sps->vui_seq_parameters);
	Origin_H264_VUI_InputParameter_Type *input_vui = &gVUI_InputParameters;

	vui->aspect_ratio_info_present_flag      = input_vui->aspect_ratio_info_present_flag;
	vui->aspect_ratio_idc                    = (unsigned int) input_vui->aspect_ratio_idc;
	vui->sar_width                           = (unsigned short) input_vui->sar_width;
	vui->sar_height                          = (unsigned short) input_vui->sar_height;
	vui->overscan_info_present_flag          = input_vui->overscan_info_present_flag;
	vui->overscan_appropriate_flag           = input_vui->overscan_appropriate_flag;
	vui->video_signal_type_present_flag      = input_vui->video_signal_type_present_flag;
	vui->video_format                        = (unsigned int) input_vui->video_format;
	vui->video_full_range_flag               = input_vui->video_full_range_flag;
	vui->colour_description_present_flag     = input_vui->colour_description_present_flag;
	vui->colour_primaries                    = (unsigned int) input_vui->colour_primaries;
	vui->transfer_characteristics            = (unsigned int) input_vui->transfer_characteristics;
	vui->matrix_coefficients                 = (unsigned int) input_vui->matrix_coefficients;
	vui->chroma_location_info_present_flag   = input_vui->chroma_location_info_present_flag;
	vui->chroma_sample_loc_type_top_field    = (unsigned int) input_vui->chroma_sample_loc_type_top_field;
	vui->chroma_sample_loc_type_bottom_field = (unsigned int) input_vui->chroma_sample_loc_type_bottom_field;
	vui->timing_info_present_flag            = input_vui->timing_info_present_flag;
	vui->num_units_in_tick                   = (unsigned int) input_vui->num_units_in_tick;
	vui->time_scale                          = (unsigned int) input_vui->time_scale;
	vui->fixed_frame_rate_flag               = input_vui->fixed_frame_rate_flag;  

	// NAL HRD parameters
	vui->nal_hrd_parameters_present_flag             = input_vui->nal_hrd_parameters_present_flag;  
	nal_hrd->cpb_cnt_minus1                          = (unsigned int) input_vui->nal_cpb_cnt_minus1;
	nal_hrd->bit_rate_scale                          = (unsigned int) input_vui->nal_bit_rate_scale;
	nal_hrd->cpb_size_scale                          = (unsigned int) input_vui->nal_cpb_size_scale;
	for ( SchedSelIdx = 0; SchedSelIdx <= nal_hrd->cpb_cnt_minus1; SchedSelIdx++ )
	{
		nal_hrd->bit_rate_value_minus1[SchedSelIdx]    = (unsigned int) input_vui->nal_bit_rate_value_minus1;
		nal_hrd->cpb_size_value_minus1[SchedSelIdx]    = (unsigned int) input_vui->nal_cpb_size_value_minus1;
		nal_hrd->cbr_flag[SchedSelIdx]                 = (unsigned int) input_vui->nal_vbr_cbr_flag;
	}
	nal_hrd->initial_cpb_removal_delay_length_minus1 = (unsigned int) input_vui->nal_initial_cpb_removal_delay_length_minus1;
	nal_hrd->cpb_removal_delay_length_minus1         = (unsigned int) input_vui->nal_cpb_removal_delay_length_minus1;
	nal_hrd->dpb_output_delay_length_minus1          = (unsigned int) input_vui->nal_dpb_output_delay_length_minus1;
	nal_hrd->time_offset_length                      = (unsigned int) input_vui->nal_time_offset_length;

	// VCL HRD parameters
	vui->vcl_hrd_parameters_present_flag             = input_vui->vcl_hrd_parameters_present_flag;  
	vcl_hrd->cpb_cnt_minus1                          = (unsigned int) input_vui->vcl_cpb_cnt_minus1;
	vcl_hrd->bit_rate_scale                          = (unsigned int) input_vui->vcl_bit_rate_scale;
	vcl_hrd->cpb_size_scale                          = (unsigned int) input_vui->vcl_cpb_size_scale;
	for ( SchedSelIdx = 0; SchedSelIdx <= vcl_hrd->cpb_cnt_minus1; SchedSelIdx++ )
	{
		vcl_hrd->bit_rate_value_minus1[SchedSelIdx]    = (unsigned int) input_vui->vcl_bit_rate_value_minus1;
		vcl_hrd->cpb_size_value_minus1[SchedSelIdx]    = (unsigned int) input_vui->vcl_cpb_size_value_minus1;
		vcl_hrd->cbr_flag[SchedSelIdx]                 = (unsigned int) input_vui->vcl_vbr_cbr_flag;
	}
	vcl_hrd->initial_cpb_removal_delay_length_minus1 = (unsigned int) input_vui->vcl_initial_cpb_removal_delay_length_minus1;
	vcl_hrd->cpb_removal_delay_length_minus1         = (unsigned int) input_vui->vcl_cpb_removal_delay_length_minus1;
	vcl_hrd->dpb_output_delay_length_minus1          = (unsigned int) input_vui->vcl_dpb_output_delay_length_minus1;
	vcl_hrd->time_offset_length                      = (unsigned int) input_vui->vcl_time_offset_length;

	vui->low_delay_hrd_flag                      = input_vui->low_delay_hrd_flag;
	vui->pic_struct_present_flag                 = input_vui->pic_struct_present_flag;
	vui->bitstream_restriction_flag              = input_vui->bitstream_restriction_flag;
	vui->motion_vectors_over_pic_boundaries_flag = input_vui->motion_vectors_over_pic_boundaries_flag;
	vui->max_bytes_per_pic_denom                 = (unsigned int) input_vui->max_bytes_per_pic_denom;
	vui->max_bits_per_mb_denom                   = (unsigned int) input_vui->max_bits_per_mb_denom;
	vui->log2_max_mv_length_horizontal           = (unsigned int) input_vui->log2_max_mv_length_horizontal;
	vui->log2_max_mv_length_vertical             = (unsigned int) input_vui->log2_max_mv_length_vertical;  
	vui->max_dec_frame_buffering                 = (unsigned int) IMIN( input_vui->max_dec_frame_buffering, getMaxDpbSize(sps) );
	vui->num_reorder_frames                      = (unsigned int) IMIN( input_vui->num_reorder_frames, (int)vui->max_dec_frame_buffering );
}

void NALU_Set_SPS(SEQ_Parameter_Set_Type *sps, int id)
{
	static const int SubWidthC  [4]= { 1, 2, 2, 1};
	static const int SubHeightC [4]= { 1, 2, 1, 1};
	unsigned i;

	// *************************************************************************
	// Sequence Parameter Set
	// *************************************************************************

	// See: G.7.4.2.1.1 Sequence parameter set data semantics (pg 451/732)

	sps->profile_idc = MAIN_PROFILE_IDC; // possibly high-profile - 100 
	sps->level_idc = 40;	// Level 4.0

	// needs to be set according to profile
	sps->constraint_set0_flag = false;
	sps->constraint_set1_flag = true;//false;
	sps->constraint_set2_flag = false;
	sps->constraint_set3_flag = false;

	// Parameter Set ID hard coded to zero
	sps->seq_parameter_set_id = id;

	// Fidelity Range Extensions stuff
	sps->bit_depth_luma_minus8   = 0;	// 8 bit
	sps->bit_depth_chroma_minus8 = 0;	// 8 bit
	sps->lossless_qpprime_flag = false;

	//! POC stuff:
	//! The following values are hard-coded in init_poc().  Apparently,
	//! the poc implementation covers only a subset of the poc functionality.
	//! Here, the same subset is implemented.  Changes in the POC stuff have
	//! also to be reflected here
	sps->log2_max_frame_num_minus4 = 0;	// number of bits (-4) to represent frame number in slice header
	sps->log2_max_pic_order_cnt_lsb_minus4 = 2; // exp for power of 2 (-4) to set the MaxPicOrderCntLsb value (e.g. 0 = 2^4 = 16)

	if (gEncoderState.b_slice_on)
		sps->pic_order_cnt_type = 0;	// POC clause 8.2.1
	else
		sps->pic_order_cnt_type = 2;	// POC clause 8.2.1
	sps->num_ref_frames_in_pic_order_cnt_cycle = 0;	// used when POC = 1
	sps->delta_pic_order_always_zero_flag = 0;	// used when POC = 1
	sps->offset_for_non_ref_pic = 0;	// used when POC = 1
	sps->offset_for_top_to_bottom_field = 0;	// used when POC = 1
#if 0
	for (i=0; i< num_ref_frames_in_pic_order_cnt_cycle; i++)
	{
		sps->offset_for_ref_frame[i] = 0;	// used when POC = 1
	}
#endif
	// End of POC stuff

	// Number of Reference Frames
	if (gEncoderState.b_slice_on)
		sps->max_num_ref_frames = (unsigned char) gEncoderState.b_slice_frames * 2;
	else
		sps->max_num_ref_frames = (unsigned char) 1;//4;	// slow has been observed to use 5

	//required_frame_num_update_behaviour_flag hardcoded to zero
	sps->gaps_in_frame_num_value_allowed_flag = false;    // double check

	sps->frame_mbs_only_flag = true;

	// Picture size, finally a simple one :-)
	sps->pic_width_in_mbs_minus1        = (( gEncoderState.width ) >> 4) -1;
	sps->pic_height_in_map_units_minus1 = (((gEncoderState.height) >> 4)/ (2 - sps->frame_mbs_only_flag)) - 1;

	// a couple of flags, simple
	sps->mb_adaptive_frame_field_flag = (bool) true;	// no interlacing supported
	sps->direct_8x8_inference_flag = (bool) true;

	// Sequence VUI 
	sps->chroma_format_idc = 1;	// we only support YUV
	sps->separate_colour_plane_flag = 0;	// only for YUV444
	sps->vui_parameters_present_flag = true;	// 

	if ( sps->vui_parameters_present_flag )
		NALU_Set_VUI(sps);

	// Fidelity Range Extensions stuff
#if 0
	if(frext_profile)
	{
		sps->seq_scaling_matrix_present_flag = (bool) (p_Inp->ScalingMatrixPresentFlag&1);
		n_ScalingList = (sps->chroma_format_idc != YUV444) ? 8 : 12;
		for(i=0; i<n_ScalingList; i++)
		{
			if(i<6)
				sps->seq_scaling_list_present_flag[i] = (p_Inp->ScalingListPresentFlag[i]&1);
			else
			{
				if(p_Inp->Transform8x8Mode)
					sps->seq_scaling_list_present_flag[i] = (p_Inp->ScalingListPresentFlag[i]&1);
				else
					sps->seq_scaling_list_present_flag[i] = 0;
			}
			if( sps->seq_scaling_matrix_present_flag == false )
				sps->seq_scaling_list_present_flag[i] = 0;
		}
	}
	else
#endif
	{
		sps->seq_scaling_matrix_present_flag = false;
		for(i = 0; i < 12; i++)
			sps->seq_scaling_list_present_flag[i] = 0;
	}

#if 0
	if (p_Vid->auto_crop_right || p_Vid->auto_crop_bottom)
	{
		sps->frame_cropping_flag = true;
		sps->frame_crop_left_offset=0;
		sps->frame_crop_top_offset=0;
		sps->frame_crop_right_offset=  (p_Vid->auto_crop_right / SubWidthC[sps->chroma_format_idc]);
		sps->frame_crop_bottom_offset= (p_Vid->auto_crop_bottom / (SubHeightC[sps->chroma_format_idc] * (2 - sps->frame_mbs_only_flag)));
		if (p_Vid->auto_crop_right % SubWidthC[sps->chroma_format_idc])
		{
			error("automatic frame cropping (width) not possible",500);
		}
		if (p_Vid->auto_crop_bottom % (SubHeightC[sps->chroma_format_idc] * (2 - sps->frame_mbs_only_flag)))
		{
			error("automatic frame cropping (height) not possible",500);
		}
	}
	else
#endif
	{
		sps->frame_cropping_flag = false;
	}
}

void NALU_Set_PPS( Pic_Parameter_Set_Type *pps, SEQ_Parameter_Set_Type *sps, int id,
	int WeightedPrediction,        //!< value of weighted_pred_flag
	int WeightedBiprediction,      //!< value of weighted_bipred_idc
	int cb_qp_index_offset,        //!< value of cb_qp_index_offset
	int cr_qp_index_offset         //!< value of cr_qp_index_offset
	)
{
	unsigned i;

	int frext_profile = 0;	 // 1 for High Profile (profile_idc == 100)

	// *************************************************************************
	// Picture Parameter Set
	// *************************************************************************

	pps->seq_parameter_set_id = sps->seq_parameter_set_id;
	pps->pic_parameter_set_id = id;
	pps->entropy_coding_mode_flag = true;	// set to false for CAVLC

#if 0
	// Fidelity Range Extensions stuff
	if(frext_profile)
	{
		pps->transform_8x8_mode_flag = false; // until supported
		pps->pic_scaling_matrix_present_flag = bool ((p_Inp->ScalingMatrixPresentFlag&2)>>1);
		n_ScalingList = (sps->chroma_format_idc != YUV444) ? 8 : 12;
		for(i=0; i<n_ScalingList; i++)
		{
			if(i<6)
				pps->pic_scaling_list_present_flag[i] = (p_Inp->ScalingListPresentFlag[i]&2)>>1;
			else
			{
				if(pps->transform_8x8_mode_flag)
					pps->pic_scaling_list_present_flag[i] = (p_Inp->ScalingListPresentFlag[i]&2)>>1;
				else
					pps->pic_scaling_list_present_flag[i] = 0;
			}
			if( pps->pic_scaling_matrix_present_flag == FALSE )
				pps->pic_scaling_list_present_flag[i] = 0;
		}
	}
	else
#endif
	{
		pps->pic_scaling_matrix_present_flag = FALSE;
		for(i=0; i < 12; i++)
			pps->pic_scaling_list_present_flag[i] = 0;

		pps->transform_8x8_mode_flag = FALSE;
	}

	pps->bottom_field_pic_order_in_frame_present_flag = 0;

	// Begin FMO stuff
	pps->num_slice_groups_minus1 = 0;
#if 0
	//! Following set the parameter for different slice group types
	if (pps->num_slice_groups_minus1 > 0)
	{
		if ((pps->slice_group_id = calloc ((sps->pic_height_in_map_units_minus1+1)*(sps->pic_width_in_mbs_minus1+1), sizeof(byte))) == NULL)
			no_mem_exit ("GeneratePictureParameterSet: slice_group_id");

		switch (p_Inp->slice_group_map_type)
		{
		case 0:
			pps->slice_group_map_type = 0;
			for(i=0; i<=pps->num_slice_groups_minus1; i++)
			{
				pps->run_length_minus1[i]=p_Inp->run_length_minus1[i];
			}
			break;
		case 1:
			pps->slice_group_map_type = 1;
			break;
		case 2:
			// i loops from 0 to num_slice_groups_minus1-1, because no info for background needed
			pps->slice_group_map_type = 2;
			unsigned int FrameSizeInMapUnits = (sps->pic_height_in_map_units_minus1) * (sps->pic_width_in_mbs_minus1 + 1);
			for(i=0; i<pps->num_slice_groups_minus1; i++)
			{
				// make sure the macroblock map does not lie outside the picture
				pps->top_left[i] = imin (p_Inp->top_left[i], FrameSizeInMapUnits);
				pps->bottom_right[i] = imin (p_Inp->bottom_right[i], FrameSizeInMapUnits);
			}
			break;
		case 3:
		case 4:
		case 5:
			pps->slice_group_map_type = p_Inp->slice_group_map_type;
			pps->slice_group_change_direction_flag = bool p_Inp->slice_group_change_direction_flag;
			pps->slice_group_change_rate_minus1 = p_Inp->slice_group_change_rate_minus1;
			break;
		case 6:
			pps->slice_group_map_type = 6;
			pps->pic_size_in_map_units_minus1 =
				(((p_Inp->output.height[0] + p_Vid->auto_crop_bottom)/MB_BLOCK_SIZE)/(2-sps->frame_mbs_only_flag))
				*((p_Inp->output.width[0]  + p_Vid->auto_crop_right)/MB_BLOCK_SIZE) -1;

			for (i=0;i<=pps->pic_size_in_map_units_minus1; i++)
				pps->slice_group_id[i] = p_Inp->slice_group_id[i];

			break;
		default:
			printf ("Parset.c: slice_group_map_type invalid, default\n");
			assert (0==1);
		}
	}
#endif
	// End FMO stuff

	pps->num_ref_idx_l0_default_active_minus1 = 0;//sps->frame_mbs_only_flag ? (sps->max_num_ref_frames - 1) : (2 * sps->max_num_ref_frames - 1) ;   // set defaults
	pps->num_ref_idx_l1_default_active_minus1 = 0;//sps->frame_mbs_only_flag ? (sps->max_num_ref_frames - 1) : (2 * sps->max_num_ref_frames - 1) ;   // set defaults

	pps->weighted_pred_flag  = 0;//(WeightedPrediction != 0);
	pps->weighted_bipred_idc = WeightedBiprediction;

	pps->pic_init_qp_minus26 = gEncoderState.qp - 26;
	pps->pic_init_qs_minus26 = 0;

	pps->chroma_qp_index_offset = cb_qp_index_offset;
	if (frext_profile)
	{
		pps->cb_qp_index_offset   = cb_qp_index_offset;
		pps->cr_qp_index_offset   = cr_qp_index_offset;
	}
	else
		pps->cb_qp_index_offset = pps->cr_qp_index_offset = pps->chroma_qp_index_offset;

	pps->deblocking_filter_control_present_flag = true;	// until implemented

	pps->constrained_intra_pred_flag = false;

	// if redundant slice is in use.
	pps->redundant_pic_cnt_present_flag = false;
}



void EndRawByteSequence(BitStreamType *bs)
{
	WriteBits(bs, 1, 1);
	if (bs->bits_rem != 8)	// byte align
		WriteBits(bs, 0, bs->bits_rem);
}


int NALU_Write_HRD(SEQ_Parameter_Set_Type *sps, BitStreamType *bs)
{
	// hrd_parameters()
	int len = 0;
	unsigned int SchedSelIdx = 0;
	HRD_Parameters_Type *hrd = &(sps->vui_seq_parameters.nal_hrd_parameters);

	WriteBits_ue(bs, static_cast<uint16_t>(hrd->cpb_cnt_minus1));
	WriteBits(bs, hrd->bit_rate_scale, 4);
	WriteBits(bs, hrd->cpb_size_scale, 4);

	for( SchedSelIdx = 0; SchedSelIdx <= (hrd->cpb_cnt_minus1); SchedSelIdx++ )
	{
		WriteBits_ue(bs, static_cast<uint16_t>(hrd->bit_rate_value_minus1[SchedSelIdx]));
		WriteBits_ue(bs, static_cast<uint16_t>(hrd->cpb_size_value_minus1[SchedSelIdx]));
		WriteBits(bs, hrd->cbr_flag[SchedSelIdx], 1);
	}

	WriteBits(bs, hrd->initial_cpb_removal_delay_length_minus1, 5);
	WriteBits(bs, hrd->cpb_removal_delay_length_minus1, 5);
	WriteBits(bs, hrd->dpb_output_delay_length_minus1, 5);
	WriteBits(bs, hrd->time_offset_length, 5);

	return len;
}

int NALU_Write_VUI(SEQ_Parameter_Set_Type *sps, BitStreamType *bs)
{
	int len=0;
	VUI_SEQ_Parameters_Type *vui = &(sps->vui_seq_parameters);

	WriteBits(bs, vui->aspect_ratio_info_present_flag, 1);
	if (vui->aspect_ratio_info_present_flag)
	{        
		WriteBits(bs, vui->aspect_ratio_idc, 8);
		if (vui->aspect_ratio_idc == 255)
		{
			WriteBits(bs, vui->sar_width,  16);
			WriteBits(bs, vui->sar_height, 16);
		}
	}  

	WriteBits(bs, vui->overscan_info_present_flag, 1);
	if (vui->overscan_info_present_flag)
	{
		WriteBits(bs, vui->overscan_appropriate_flag, 1);
	} 

	WriteBits(bs, vui->video_signal_type_present_flag, 1);
	if (vui->video_signal_type_present_flag)
	{
		WriteBits(bs, vui->video_format, 3);
		WriteBits(bs, vui->video_full_range_flag, 1);
		WriteBits(bs, vui->colour_description_present_flag, 1);
		if (vui->colour_description_present_flag)
		{
			WriteBits(bs, vui->colour_primaries, 8);
			WriteBits(bs, vui->transfer_characteristics, 8);
			WriteBits(bs, vui->matrix_coefficients, 8);
		}
	}

	WriteBits(bs, vui->chroma_location_info_present_flag, 1);
	if (vui->chroma_location_info_present_flag)
	{
		WriteBits_ue(bs, static_cast<uint16_t>(vui->chroma_sample_loc_type_top_field));
		WriteBits_ue(bs, static_cast<uint16_t>(vui->chroma_sample_loc_type_bottom_field));
	}

	WriteBits(bs, vui->timing_info_present_flag, 1);
	// timing parameters
	if (vui->timing_info_present_flag)
	{
		WriteBits(bs, vui->num_units_in_tick, 32);
		WriteBits(bs, vui->time_scale, 32);
		WriteBits(bs, vui->fixed_frame_rate_flag, 1);
	}
	// end of timing parameters
	// nal_hrd_parameters_present_flag
	WriteBits(bs, vui->nal_hrd_parameters_present_flag, 1);
	if ( vui->nal_hrd_parameters_present_flag )
	{
		NALU_Write_HRD(sps, bs);
	}
	// vcl_hrd_parameters_present_flag
	WriteBits(bs, vui->vcl_hrd_parameters_present_flag, 1);
	if ( vui->vcl_hrd_parameters_present_flag )
	{
		NALU_Write_HRD(sps, bs);
	}
	if ( vui->nal_hrd_parameters_present_flag || vui->vcl_hrd_parameters_present_flag )
	{
		WriteBits(bs, vui->low_delay_hrd_flag, 1 );
	}
	WriteBits(bs, vui->pic_struct_present_flag, 1);

	WriteBits(bs, vui->bitstream_restriction_flag, 1);
	if (vui->bitstream_restriction_flag)
	{
		WriteBits(bs, vui->motion_vectors_over_pic_boundaries_flag, 1);
		WriteBits_ue(bs, static_cast<uint16_t>(vui->max_bytes_per_pic_denom));
		WriteBits_ue(bs, static_cast<uint16_t>(vui->max_bits_per_mb_denom));
		WriteBits_ue(bs, static_cast<uint16_t>(vui->log2_max_mv_length_horizontal));
		WriteBits_ue(bs, static_cast<uint16_t>(vui->log2_max_mv_length_vertical));
		WriteBits_ue(bs, static_cast<uint16_t>(vui->num_reorder_frames));
		WriteBits_ue(bs, static_cast<uint16_t>(vui->max_dec_frame_buffering));
	}

	return len;
}

int NALU_Write_SPS(SEQ_Parameter_Set_Type *sps, uint8_t *stream_buffer)
{
	BitStreamType bs;
	int length_in_bytes;
	unsigned i;

	InitBitStream(&bs, stream_buffer);

#ifdef USE_BIN_DUMP_NALU
	for (int i = 0; i < 20; i++)
		WriteBits(&bs, sSPS[i], 8);	// ff_byte
#else

	WriteBits(&bs, sps->profile_idc, 8);

	WriteBits(&bs, sps->constraint_set0_flag, 1);
	WriteBits(&bs, sps->constraint_set1_flag, 1);
	WriteBits(&bs, sps->constraint_set2_flag, 1);
	WriteBits(&bs, sps->constraint_set3_flag, 1);
	WriteBits(&bs, 0, 4);	// reserved 4 bits - first two are for constraint flags set4 and set5 (MVC ext use)
	WriteBits(&bs, sps->level_idc, 8);
	WriteBits_ue(&bs, static_cast<uint16_t>(sps->seq_parameter_set_id));
#if 0
	// Fidelity Range Extensions stuff
	if( is_FREXT_profile(sps->profile_idc) )
	{
		len+=write_ue_v ("SPS: chroma_format_idc",                        sps->chroma_format_idc,                          bitstream);    
		if(sps->chroma_format_idc == YUV444) //if(p_Vid->yuv_format == YUV444)
			len+=write_u_1  ("SPS: separate_colour_plane_flag",             sps->separate_colour_plane_flag,                 bitstream);
		len+=write_ue_v ("SPS: bit_depth_luma_minus8",                    sps->bit_depth_luma_minus8,                      bitstream);
		len+=write_ue_v ("SPS: bit_depth_chroma_minus8",                  sps->bit_depth_chroma_minus8,                    bitstream);

		len+=write_u_1  ("SPS: lossless_qpprime_y_zero_flag",             sps->lossless_qpprime_flag,                      bitstream);
		//other chroma info to be added in the future

		len+=write_u_1 ("SPS: seq_scaling_matrix_present_flag",           sps->seq_scaling_matrix_present_flag,            bitstream);

		if(sps->seq_scaling_matrix_present_flag)
		{
			ScaleParameters *p_QScale = p_Vid->p_QScale;
			n_ScalingList = (sps->chroma_format_idc != YUV444) ? 8 : 12;
			for(i=0; i<n_ScalingList; i++)
			{
				len+=write_u_1 ("SPS: seq_scaling_list_present_flag",         sps->seq_scaling_list_present_flag[i],           bitstream);
				if(sps->seq_scaling_list_present_flag[i])
				{
					if(i<6)
						len+=Scaling_List(p_QScale->ScalingList4x4input[i], p_QScale->ScalingList4x4[i], 16, &p_QScale->UseDefaultScalingMatrix4x4Flag[i], bitstream);
					else
						len+=Scaling_List(p_QScale->ScalingList8x8input[i-6], p_QScale->ScalingList8x8[i-6], 64, &p_QScale->UseDefaultScalingMatrix8x8Flag[i-6], bitstream);
				}
			}
		}
	}
#endif

	WriteBits_ue(&bs, static_cast<uint16_t>(sps->log2_max_frame_num_minus4));
	WriteBits_ue(&bs, static_cast<uint16_t>(sps->pic_order_cnt_type));

	if (sps->pic_order_cnt_type == 0)
	{
		WriteBits_ue(&bs, static_cast<uint16_t>(sps->log2_max_pic_order_cnt_lsb_minus4));
	}
	else if (sps->pic_order_cnt_type == 1)
	{
		WriteBits(&bs, sps->delta_pic_order_always_zero_flag, 1);
		WriteBits_se(&bs, static_cast<int16_t>(sps->offset_for_non_ref_pic));
		WriteBits_se(&bs, static_cast<int16_t>(sps->offset_for_top_to_bottom_field));
		WriteBits_ue(&bs, static_cast<uint16_t>(sps->num_ref_frames_in_pic_order_cnt_cycle));
		for (i=0; i<sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
			WriteBits_se(&bs, static_cast<int16_t>(sps->offset_for_ref_frame[i]));
	}
	WriteBits_ue(&bs, static_cast<uint16_t>(sps->max_num_ref_frames));
	WriteBits(&bs, sps->gaps_in_frame_num_value_allowed_flag, 1);
	WriteBits_ue(&bs, static_cast<uint16_t>(sps->pic_width_in_mbs_minus1));
	WriteBits_ue(&bs, static_cast<uint16_t>(sps->pic_height_in_map_units_minus1));
	WriteBits(&bs, sps->frame_mbs_only_flag, 1);
	if (!sps->frame_mbs_only_flag)
	{
		WriteBits(&bs, sps->mb_adaptive_frame_field_flag, 1);
	}
	WriteBits(&bs, sps->direct_8x8_inference_flag, 1);

	WriteBits(&bs, sps->frame_cropping_flag, 1);
	if (sps->frame_cropping_flag)
	{
		WriteBits_ue(&bs, static_cast<uint16_t>(sps->frame_crop_left_offset));
		WriteBits_ue(&bs, static_cast<uint16_t>(sps->frame_crop_right_offset));
		WriteBits_ue(&bs, static_cast<uint16_t>(sps->frame_crop_top_offset));
		WriteBits_ue(&bs, static_cast<uint16_t>(sps->frame_crop_bottom_offset));
	}

	WriteBits(&bs, sps->vui_parameters_present_flag, 1);

	if (sps->vui_parameters_present_flag)
		NALU_Write_VUI(sps, &bs);

	EndRawByteSequence(&bs);
#endif
	length_in_bytes = bs.byte_pos;

	return length_in_bytes;
}


int NALU_Write_PPS(Pic_Parameter_Set_Type *pps, int id, uint8_t *stream_buffer)
{
	BitStreamType bs;
	int length_in_bytes;
	unsigned i;

	InitBitStream(&bs, stream_buffer);

#ifdef USE_BIN_DUMP_NALU
	for (int i = 0; i < 3; i++)
		WriteBits(&bs, sPPS[i], 8);	// ff_byte
#else
	pps->bottom_field_pic_order_in_frame_present_flag = 0;	// ???

	WriteBits_ue(&bs, static_cast<uint16_t>(pps->pic_parameter_set_id));
	WriteBits_ue(&bs, static_cast<uint16_t>(pps->seq_parameter_set_id));
	WriteBits(&bs, pps->entropy_coding_mode_flag, 1);
	WriteBits(&bs, pps->bottom_field_pic_order_in_frame_present_flag, 1);
	WriteBits_ue(&bs, static_cast<uint16_t>(pps->num_slice_groups_minus1));

	// FMO stuff
	if(pps->num_slice_groups_minus1 > 0 )
	{
		WriteBits_ue(&bs, static_cast<uint16_t>(pps->slice_group_map_type));
		if (pps->slice_group_map_type == 0)
			for (i=0; i<=pps->num_slice_groups_minus1; i++)
				WriteBits_ue(&bs, static_cast<uint16_t>(pps->run_length_minus1[i]));
		else if (pps->slice_group_map_type==2)
		{
			for (i=0; i<pps->num_slice_groups_minus1; i++)
			{
				WriteBits_ue(&bs, static_cast<uint16_t>(pps->top_left[i]));
				WriteBits_ue(&bs, static_cast<uint16_t>(pps->bottom_right[i]));
			}
		}
		else if (pps->slice_group_map_type == 3 ||
			pps->slice_group_map_type == 4 ||
			pps->slice_group_map_type == 5)
		{
			WriteBits(&bs, pps->slice_group_change_direction_flag, 1);
			WriteBits_ue(&bs, static_cast<uint16_t>(pps->slice_group_change_rate_minus1));
		}
		else if (pps->slice_group_map_type == 6)
		{
			unsigned NumberBitsPerSliceGroupId = 0;

			if (pps->num_slice_groups_minus1>=4)
				NumberBitsPerSliceGroupId=3;
			else if (pps->num_slice_groups_minus1>=2)
				NumberBitsPerSliceGroupId=2;
			else if (pps->num_slice_groups_minus1>=1)
				NumberBitsPerSliceGroupId=1;

			WriteBits_ue(&bs, static_cast<uint16_t>(pps->pic_size_in_map_units_minus1));
			for(i = 0; i <= pps->pic_size_in_map_units_minus1; i++)
				WriteBits(&bs, pps->slice_group_id[i], NumberBitsPerSliceGroupId);
		}
	}
	// End of FMO stuff

	WriteBits_ue(&bs, static_cast<uint16_t>(pps->num_ref_idx_l0_default_active_minus1));
	WriteBits_ue(&bs, static_cast<uint16_t>(pps->num_ref_idx_l1_default_active_minus1));
	WriteBits(&bs, pps->weighted_pred_flag, 1);
	WriteBits(&bs, pps->weighted_bipred_idc, 2);
	WriteBits_se(&bs, static_cast<int16_t>(pps->pic_init_qp_minus26));
	WriteBits_se(&bs, static_cast<int16_t>(pps->pic_init_qs_minus26));
#if 0	// for profile idc >= 100
	profile_idc = MAIN_PROFILE_IDC;
	if( is_FREXT_profile(profile_idc) )
		WriteBits_se(&bs, pps->cb_qp_index_offset);
	else
#endif
		WriteBits_se(&bs, static_cast<int16_t>(pps->chroma_qp_index_offset));

	WriteBits(&bs, pps->deblocking_filter_control_present_flag,    1);
	WriteBits(&bs, pps->constrained_intra_pred_flag,               1);
	WriteBits(&bs, pps->redundant_pic_cnt_present_flag,            1);
#if 0
	// Fidelity Range Extensions stuff
	if( is_FREXT_profile(profile_idc) )
	{
		WriteBits(&bs, pps->transform_8x8_mode_flag,                   1);
		WriteBits(&bs, pps->pic_scaling_matrix_present_flag,           1);

		if(pps->pic_scaling_matrix_present_flag)
		{
			ScaleParameters *p_QScale = p_Vid->p_QScale;
			unsigned n_ScalingList = 6 + ((p_Vid->active_sps->chroma_format_idc != 3) ? 2 : 6) * pps->transform_8x8_mode_flag;

			for(i=0; i<n_ScalingList; i++)  // SS-70226
			{
				WriteBits(&bs, pps->pic_scaling_list_present_flag[i],          1);

				if(pps->pic_scaling_list_present_flag[i])
				{
					if(i < 6)
						len += Scaling_List(p_QScale->ScalingList4x4input[i], p_QScale->ScalingList4x4[i], 16, &p_QScale->UseDefaultScalingMatrix4x4Flag[i], bitstream);
					else
						len += Scaling_List(p_QScale->ScalingList8x8input[i-6], p_QScale->ScalingList8x8[i-6], 64, &p_QScale->UseDefaultScalingMatrix8x8Flag[i-6], bitstream);
				}
			}
		}
		WriteBits_se(&bs, pps->cr_qp_index_offset);
	}
#endif

	EndRawByteSequence(&bs);
#endif
	length_in_bytes = bs.byte_pos;

	return length_in_bytes;
}


int NALU_Write_SEI(int id, uint8_t *stream_buffer)
{
	BitStreamType bs;
	int length_in_bytes;

	InitBitStream(&bs, stream_buffer);

	char sei_message[500] = "";
	char uuid_message[9] = "Random"; // This is supposed to be Random
	unsigned int i, message_size = 0;
	static LARGE_INTEGER start_time;
	static bool start_time_init = false;

	if (start_time_init == 0)
	{
		start_time_init = true;
		static LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		QueryPerformanceCounter(&start_time);    // start time
		if (start_time.HighPart < (1 << 16))		// make sure we don't get a start code in the body (i.e. 0x00 0x00 0x02)
			start_time.HighPart = freq.LowPart;
	}
#ifdef USE_BIN_DUMP_NALU
	for (int i = 0; i < 656; i++)
		WriteBits(&bs, sSEI[i], 8);	// ff_byte
#else
	message_size = 24;
	strncpy_s(sei_message,"Origin H.264/AVC Encoder",message_size);

	WriteBits(&bs, 5, 8); // last_payload_type_byte
	message_size += 17;
	while (message_size > 254)
	{
		WriteBits(&bs, 255, 8);	// ff_byte
		message_size -= 255;
	}
	WriteBits(&bs,  message_size, 8); // last_payload_size_byte

	// Lets randomize uuid based on time
	WriteBits(&bs, (int) start_time.HighPart, 32);	// uuid_iso_iec_11578
	WriteBits(&bs, (int) start_time.QuadPart, 32);	// uuid_iso_iec_11578
	WriteBits(&bs, (int) (uuid_message[0] << 24) + (uuid_message[1] << 16)  + (uuid_message[2] << 8) + (uuid_message[3] << 0), 32); // uuid_iso_iec_11578
	WriteBits(&bs, (int) (uuid_message[4] << 24) + (uuid_message[5] << 16)  + (uuid_message[6] << 8) + (uuid_message[7] << 0), 32); // uuid_iso_iec_11578
	for (i = 0; i < strlen(sei_message); i++)
		WriteBits(&bs, sei_message[i], 8); // user_data_payload_byte

	WriteBits(&bs, 0, 8);	// user_data_payload_byte
	EndRawByteSequence(&bs);
#endif

	length_in_bytes = bs.byte_pos;

	return length_in_bytes;
}


struct NAL_HEADER
{
	unsigned nal_unit_type : 5;
	unsigned nal_ref_idc : 2;
	unsigned zero_bit : 1;
};

#define MAX_NALU_SIZE (1L << 16)
static void *sOutputNALU = NULL;

int RBSPtoEBSP(uint8_t *NaluBuffer, unsigned char *rbsp, int rbsp_size)
{
	int j     = 0;
	int count = 0;
	int i;

	for(i = 0; i < rbsp_size; i++)
	{
		if(count == 2 && !(rbsp[i] & 0xFC))
		{
			NaluBuffer[j] = 0x03;
			j++;
			count = 0;
		}
		NaluBuffer[j] = rbsp[i];
		if(rbsp[i] == 0x00)
			count++;
		else
			count = 0;
		j++;
	}

	return j;
}

#define OUTPUT_NAL_TABLE_SIZE 10
#define OUTPUT_NAL_CIRC_BUFF_SIZE 30
NAL_Type *sOutputNALU_Table;

typedef struct  
{
	bool in_use;
	bool is_frame_data;
	bool to_be_exported;
	bool ready_for_export;
	int frame_type;
	int nal_count;
	uint32_t timestamp;

	uint8_t *buffer_start;
	uint32_t full_buffer_length;

	NAL_Type table[OUTPUT_NAL_TABLE_SIZE];
} NALU_TableType;

typedef struct
{
	uint8_t *buffer;
	uint32_t index;
	int free_space;
} NALU_BufferType;

#define OUTPUT_NALU_BUFFER_SIZE (512 * 1024 * OUTPUT_NAL_CIRC_BUFF_SIZE)

//#define OUTPUT_NALU_BUFFER_SIZE (2 * 1024 * 1024 * OUTPUT_NAL_CIRC_BUFF_SIZE)
static NALU_TableType sOutputNALU_CircularBuffer[OUTPUT_NAL_CIRC_BUFF_SIZE];
static int sOutputNALU_Count = 0;
static int sOutputNALU_CircBuffSize = 0;
static int sOutputNALU_CircBuffIndex = 0;
static EA::Thread::Futex sOutputFrameNALUMutex;
static NALU_BufferType sOutputNALU_Buffer;

void InitNALU(int circular_buffer_size)
{
	sOutputNALU_CircBuffIndex = 0;
	sOutputNALU_CircBuffSize = OUTPUT_NAL_CIRC_BUFF_SIZE;
	sOutputNALU_Table = NULL;

	sOutputNALU_Buffer.index = 0;
	sOutputNALU_Buffer.free_space = OUTPUT_NALU_BUFFER_SIZE;
	sOutputNALU_Buffer.buffer = (uint8_t *) _aligned_malloc(OUTPUT_NALU_BUFFER_SIZE, 16);
	if (sOutputNALU_Buffer.buffer == NULL)
	{
		ENC_ASSERT(0);	// need error handling
	}

	for (int i = 0; i < OUTPUT_NAL_CIRC_BUFF_SIZE; i++)
	{
		sOutputNALU_CircularBuffer[i].in_use = false;
		sOutputNALU_CircularBuffer[i].is_frame_data = false;
		sOutputNALU_CircularBuffer[i].to_be_exported = false;
		sOutputNALU_CircularBuffer[i].ready_for_export = false;
		sOutputNALU_CircularBuffer[i].nal_count = 0;

		sOutputNALU_CircularBuffer[i].buffer_start = NULL;
		sOutputNALU_CircularBuffer[i].full_buffer_length = 0;

		for (int j = 0; j < OUTPUT_NAL_TABLE_SIZE; j++)
		{
			sOutputNALU_CircularBuffer[i].table[j].p_payload = NULL;
		}
	}
}

void ExitNALU()
{
	if (sOutputNALU_Buffer.buffer)
	{
		_aligned_free(sOutputNALU_Buffer.buffer);
	}
#if 0	// don't need this anymore
	// free any remaining buffers
	for (int i = 0; i < OUTPUT_NAL_CIRC_BUFF_SIZE; i++)
	{
		if (sOutputNALU_CircularBuffer[i].in_use)
		{
			for (int j = 0; j < OUTPUT_NAL_TABLE_SIZE; j++)
			{
				if (sOutputNALU_CircularBuffer[i].table[j].p_payload)
				{
					free(sOutputNALU_CircularBuffer[i].table[j].p_payload);
					sOutputNALU_CircularBuffer[i].table[j].p_payload = NULL;
				}
			}
		}
	}
#endif
}

void ResetNALU_Table(uint32_t timestamp, bool frame_data, int frame_type)
{
	EA::Thread::AutoFutex m(sOutputFrameNALUMutex);

	if (sOutputNALU_Table == NULL)
	{
		sOutputNALU_CircBuffIndex = 0;
		sOutputNALU_Table = sOutputNALU_CircularBuffer[0].table;
	}
	else
	{
		sOutputNALU_CircBuffIndex = (sOutputNALU_CircBuffIndex + 1) % sOutputNALU_CircBuffSize;
		sOutputNALU_Table = sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].table;
		for (int i = 0; i < OUTPUT_NAL_TABLE_SIZE; i++)
		{
			if (sOutputNALU_Table[i].p_payload)
			{	// release 
//				free(sOutputNALU_Table[i].p_payload);
				sOutputNALU_Table[i].p_payload = NULL;
			}
		}
		
		// release from circular buffer
		sOutputNALU_Buffer.free_space += sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].full_buffer_length;
	}

    ENC_ASSERT(!(sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].in_use && sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].ready_for_export));

	sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].in_use = true;
	sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].ready_for_export = false;
	sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].to_be_exported = frame_data;
	sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].timestamp = timestamp;
	sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].is_frame_data = frame_data;
	sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].frame_type = frame_type;
	sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].nal_count = sOutputNALU_Count = 0;
	sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].buffer_start = &sOutputNALU_Buffer.buffer[sOutputNALU_Buffer.index];
	sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].full_buffer_length = 0;
}

void *GetNALU_Buffer(int length)
{
	// check for crossing over end of buffer
	if ((sOutputNALU_Buffer.index + length) >= OUTPUT_NALU_BUFFER_SIZE)
	{
		int diff = OUTPUT_NALU_BUFFER_SIZE - sOutputNALU_Buffer.index;

		sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].full_buffer_length += diff;
		sOutputNALU_Buffer.index = 0;
		sOutputNALU_Buffer.free_space -= diff;
	}

	if (sOutputNALU_Buffer.free_space < length)
	{
		ENC_ASSERT(0);	// need error handling
		return NULL;
	}

	sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].full_buffer_length += length;

	int index = sOutputNALU_Buffer.index;
	sOutputNALU_Buffer.index += length;
	sOutputNALU_Buffer.free_space -= length;

	ENC_ASSERT((sOutputNALU_Buffer.free_space >= 0) && (sOutputNALU_Buffer.free_space <= OUTPUT_NALU_BUFFER_SIZE));

	return(&sOutputNALU_Buffer.buffer[index]);
}

void AddTo_NALU_Table(uint8_t *bs, int bs_length, int nal_unit_type, int nal_ref_idc, bool copy_only)
{
	EA::Thread::AutoFutex m(sOutputFrameNALUMutex);

//	void *payload = malloc(bs_length + sizeof(struct NAL_HEADER) + 4 + 1000);
	void *payload = GetNALU_Buffer(bs_length + sizeof(struct NAL_HEADER) + 4 + 1000);
	uint8_t *buffer = (uint8_t *) payload;

	int length = 4;
	unsigned int bits_written = 0;
	static const uint8_t startcode[] = {0,0,0,1};

	memcpy(buffer, startcode, length);
	bits_written += (length << 3);

	struct NAL_HEADER header;

	header.zero_bit = 0;
	header.nal_unit_type = nal_unit_type;
	header.nal_ref_idc = nal_ref_idc;	
	memcpy(&buffer[4], &header, sizeof(header));
	bits_written += 8;

	if (copy_only)
	{
		memcpy(&buffer[5], bs, bs_length);
		length = bs_length;
	}
	else
		length = RBSPtoEBSP(&buffer[5], bs, bs_length);
	bits_written += (length << 3);
	sOutputNALU_Table[sOutputNALU_Count].i_payload = bits_written / 8;
//	ENC_TRACE("Bytes Written: %d\n", bits_written/8);

	ENC_ASSERT((bits_written/8) < (bs_length + sizeof(struct NAL_HEADER) + 4 + 1000));
	ENC_ASSERT(sOutputNALU_Count < OUTPUT_NAL_TABLE_SIZE);

	sOutputNALU_Table[sOutputNALU_Count].i_first_mb = 0;
	sOutputNALU_Table[sOutputNALU_Count].i_ref_idc = 3;
	sOutputNALU_Table[sOutputNALU_Count].i_type = nal_unit_type;	// e.g. slice_IDR
	sOutputNALU_Table[sOutputNALU_Count].b_long_startcode = 0;
	sOutputNALU_Table[sOutputNALU_Count].p_payload = (uint8_t *) payload;
	if ((nal_unit_type == 1) || (nal_unit_type == 5))
		sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].ready_for_export = true;	// mark as done and ready to be exported
	sOutputNALU_Count++;
	sOutputNALU_CircularBuffer[sOutputNALU_CircBuffIndex].nal_count++;

#if 0
	char str[256];
	sprintf_s(str, "\nstatic BYTE sTestNALU[%d] = {\n", sOutputNALU_Table[sOutputNALU_Count-1].i_payload-4);
	OutputDebugString(str);
	int count = 0;
	for (int j = 4; j < sOutputNALU_Table[sOutputNALU_Count-1].i_payload; ++j)
	{
		count++;
		if (count < 127)
			sprintf_s(str, "0x%02x,", buffer[j]);
		else
		{
			sprintf_s(str, "0x%02x,\n ", buffer[j]);
			count = 0;
		}
		OutputDebugString(str);
	}
	OutputDebugString("\n};\n");
#endif

}

void *GetNALUs(int *nal_count)
{
	*nal_count = sOutputNALU_Count;
	return sOutputNALU_Table;
}

static bool sOneFound = false;

int GetBufferedFrame_NALU(uint32_t timestamp, Origin_H264_EncoderOutputType *output)
{
	EA::Thread::AutoFutex m(sOutputFrameNALUMutex);

	int index;
	int num_of_buffered_frames_remaining = InputFramesInQueue();	// start out with the frames being processed
	bool found = false;

#ifdef DEBUG_NALU_OUTPUT
    ENC_TRACE("GetBufferedFrame_NALU: timestamp: %d", timestamp);
#endif
	for (int i = 1; i <= sOutputNALU_CircBuffSize; i++)
	{
		index = (sOutputNALU_CircBuffIndex + i) % sOutputNALU_CircBuffSize;

		if (sOutputNALU_CircularBuffer[index].in_use && sOutputNALU_CircularBuffer[index].ready_for_export && 
			sOutputNALU_CircularBuffer[index].to_be_exported && sOutputNALU_CircularBuffer[index].is_frame_data && 
			(sOutputNALU_CircularBuffer[index].nal_count > 0) && 
            ((timestamp - sOutputNALU_CircularBuffer[index].timestamp) >= static_cast<uint32_t>(gEncoderState.frame_delay_ms)))
		{
			if (!found)
			{
				output->frame_type = sOutputNALU_CircularBuffer[index].frame_type;
				output->nal_out = sOutputNALU_CircularBuffer[index].table;
				output->nal_count = sOutputNALU_CircularBuffer[index].nal_count;
				output->timestamp = sOutputNALU_CircularBuffer[index].timestamp;
				sOutputNALU_CircularBuffer[index].to_be_exported = false;	// mark as exported
                sOutputNALU_CircularBuffer[index].in_use = false;
                sOutputNALU_CircularBuffer[index].ready_for_export = false;

				found = true;

                sOneFound = true;
#ifdef DEBUG_NALU_OUTPUT
                ENC_TRACE("  FOUND! index: %d  ts:%d\n", index, output->timestamp);
#endif
			}

			num_of_buffered_frames_remaining++;
		}
	}

	if (!found)
	{
#if 1//def DEBUG_NALU_OUTPUT
        ENC_TRACE("GetBufferedFrame_NALU: timestamp: %d num_of_buffered_frames_remaining: %d - not found  \n", timestamp, num_of_buffered_frames_remaining);
#endif
		output->timestamp = 0;
	}

	return num_of_buffered_frames_remaining;
}