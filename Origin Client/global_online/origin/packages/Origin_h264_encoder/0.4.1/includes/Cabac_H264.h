#ifndef __CABAC_H264_H
#define __CABAC_H264_H

//#define DEBUG_STATS

#include "MacroBlock.h"

typedef union{
	struct 
	{
		unsigned ctx_idx : 12;
		unsigned coeff_toggle : 1;
		unsigned bit : 1;
		unsigned eq : 1;
		unsigned encode_bit : 1;
	} flags;
	uint16_t value;
} CABAC_ItemType;


typedef struct  
{
	void (*encode_bit)      (struct rowstream *stream, int symbol, int ctx_index );
	void (*encode_eq_bit)   (struct rowstream *stream, int symbol);
	void (*encode_finalbit) (struct rowstream *stream, int symbol);
	void (*start_coeff_tracking) (struct rowstream *stream);
	void (*end_coeff_tracking) (struct rowstream *stream);
} CABAC_FunctionGroupType;

typedef struct rowstream
{
	int num_items;
	CABAC_ItemType *row_stream;
	const CABAC_FunctionGroupType *func;

	int coef_write_pos;
	__declspec(align(16)) int coef_buffer[64];
	int coef_counter;

	int coeff_bits;
	int coeff_start_pos;
	bool toggle_coeff_bit_tracking;

	struct encoding_environment *enc_env;
} CABAC_RowStreamType;

typedef struct encoding_environment
{
	unsigned int  Elow, Erange;
	unsigned int  Ebuffer;
	unsigned int  Ebits_to_go;
	unsigned int  Echunks_outstanding;
	int           Epbuf;
	uint8_t       *stream;
	uint8_t       *stream_end;
	int           codestrm_bytepos;
	int           C;
	int           E;
	__declspec(align(16)) uint8_t  state[1024];
	__declspec(align(16)) uint8_t    mps[1024];           // Least Probable Symbol 0/1 CP  
	CABAC_RowStreamType *mb_row_stream;	// allocate array for number of rows of macroblocks
	uint8_t      *bitstream;	// allocated buffer for bit stream
} CABACContextType;

void InitCABAC_ContextStateTable();
void InitEncoding();
void ExitEncoding();
void CABAC_ContextInit(CABACContextType *cbc_enc, int slice_type, int qp);
void CABAC_Start_EncodingStream(CABACContextType *cbc_enc, unsigned char *code_buffer, int code_len, int code_byte_pos);
void CABAC_Stop_EncodingStream(CABACContextType *cbc_enc);
void CABAC_Encode_RowStream(int row_index);
void CABAC_Reset_EC(CABACContextType *cbc_enc);

void CABAC_Encode_Coefficients (MacroBlockType* mb, CABAC_RowStreamType *stream, int coef, int run, int coef_type, int subx = 0, int sub_y = 0, bool uv = true);
void CABAC_Encode_MB_Layer(MacroBlockType *mb);

uint8_t *CABAC_GetBitStream(int *length);
void CABAC_StartSlice(int slice_type);
void CABAC_EndSlice();

#ifdef DEBUG_STATS
int CABAC_Encode_BitsWritten_();
#endif

#endif