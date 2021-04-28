#include <memory.h>
#include <stdint.h>
#include <assert.h>
#include "Origin_h264_encoder.h"
#include "Cabac_H264.h"
#include "MacroBlock.h"
#include "BitStream.h"
#include "NALU.h"
#include "Tables.h"
#include "Trace.h"

//#define DEBUG_CABAC_OUTPUT
//#define CABAC_TRACE_OUTPUT

#define CABAC_MAX_ROWSTREAM_MB_SIZE 2048

static CABACContextType sCABAC_Context;

static void CABAC_Encode_Bit_Output(CABAC_RowStreamType *stream, int symbol, int ctx_index );
static void CABAC_Encode_EQ_Bit_Output(CABAC_RowStreamType *stream, int symbol);
static void CABAC_Encode_FinalBit_Output(CABAC_RowStreamType *stream, int symbol);
static void CABAC_Start_Coeff_Tracking_Output(CABAC_RowStreamType *stream);
static void CABAC_End_Coeff_Tracking_Output(CABAC_RowStreamType *stream);
static void CABAC_Encode_Bit_Stream(CABAC_RowStreamType *stream, int symbol, int ctx_index );
static void CABAC_Encode_EQ_Bit_Stream(CABAC_RowStreamType *stream, int symbol);
static void CABAC_Encode_FinalBit_Stream(CABAC_RowStreamType *stream, int symbol);
static void CABAC_Start_Coeff_Tracking_Stream(CABAC_RowStreamType *stream);
static void CABAC_End_Coeff_Tracking_Stream(CABAC_RowStreamType *stream);

static CABAC_FunctionGroupType sEncodeFunctionGroups[2];

#define CABAC_STREAM_MAX_SIZE   (1L << 20)

// some definitions to increase the readability of the source code

#define B_BITS         10    // Number of bits to represent the whole coding interval
#define BITS_TO_LOAD   16
#define MAX_BITS       26          //(B_BITS + BITS_TO_LOAD)
#define MASK_BITS      18          //(MAX_BITS - 8)
#define ONE            0x04000000  //(1 << MAX_BITS)
#define ONE_M1         0x03FFFFFF  //(ONE - 1)
#define HALF           0x01FE      //(1 << (B_BITS-1)) - 2
#define QUARTER        0x0100      //(1 << (B_BITS-2))
#define MIN_BITS_TO_GO 0
#define B_LOAD_MASK    0xFFFF      // ((1<<BITS_TO_LOAD) - 1)

#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant (for AVAILABLE macro)

// Range table for LPS
static const __declspec(align(16)) uint8_t renorm_table_32[32]={6,5,4,4,3,3,3,3,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
static const __declspec(align(16)) uint8_t rLPS_table_64x4[64][4]=
{
	{ 128, 176, 208, 240},
	{ 128, 167, 197, 227},
	{ 128, 158, 187, 216},
	{ 123, 150, 178, 205},
	{ 116, 142, 169, 195},
	{ 111, 135, 160, 185},
	{ 105, 128, 152, 175},
	{ 100, 122, 144, 166},
	{  95, 116, 137, 158},
	{  90, 110, 130, 150},
	{  85, 104, 123, 142},
	{  81,  99, 117, 135},
	{  77,  94, 111, 128},
	{  73,  89, 105, 122},
	{  69,  85, 100, 116},
	{  66,  80,  95, 110},
	{  62,  76,  90, 104},
	{  59,  72,  86,  99},
	{  56,  69,  81,  94},
	{  53,  65,  77,  89},
	{  51,  62,  73,  85},
	{  48,  59,  69,  80},
	{  46,  56,  66,  76},
	{  43,  53,  63,  72},
	{  41,  50,  59,  69},
	{  39,  48,  56,  65},
	{  37,  45,  54,  62},
	{  35,  43,  51,  59},
	{  33,  41,  48,  56},
	{  32,  39,  46,  53},
	{  30,  37,  43,  50},
	{  29,  35,  41,  48},
	{  27,  33,  39,  45},
	{  26,  31,  37,  43},
	{  24,  30,  35,  41},
	{  23,  28,  33,  39},
	{  22,  27,  32,  37},
	{  21,  26,  30,  35},
	{  20,  24,  29,  33},
	{  19,  23,  27,  31},
	{  18,  22,  26,  30},
	{  17,  21,  25,  28},
	{  16,  20,  23,  27},
	{  15,  19,  22,  25},
	{  14,  18,  21,  24},
	{  14,  17,  20,  23},
	{  13,  16,  19,  22},
	{  12,  15,  18,  21},
	{  12,  14,  17,  20},
	{  11,  14,  16,  19},
	{  11,  13,  15,  18},
	{  10,  12,  15,  17},
	{  10,  12,  14,  16},
	{   9,  11,  13,  15},
	{   9,  11,  12,  14},
	{   8,  10,  12,  14},
	{   8,   9,  11,  13},
	{   7,   9,  11,  12},
	{   7,   9,  10,  12},
	{   7,   8,  10,  11},
	{   6,   8,   9,  11},
	{   6,   7,   9,  10},
	{   6,   7,   8,   9},
	{   2,   2,   2,   2}
};
static const __declspec(align(16)) uint8_t AC_next_state_MPS_64[64] =
{
	1,2,3,4,5,6,7,8,9,10,
	11,12,13,14,15,16,17,18,19,20,
	21,22,23,24,25,26,27,28,29,30,
	31,32,33,34,35,36,37,38,39,40,
	41,42,43,44,45,46,47,48,49,50,
	51,52,53,54,55,56,57,58,59,60,
	61,62,62,63
};

static const __declspec(align(16)) uint8_t AC_next_state_LPS_64[64] =
{
	0, 0, 1, 2, 2, 4, 4, 5, 6, 7,
	8, 9, 9,11,11,12,13,13,15,15,
	16,16,18,18,19,19,21,21,22,22,
	23,24,24,25,26,26,27,27,28,29,
	29,30,30,30,31,32,32,33,33,33,
	34,34,35,35,35,36,36,36,37,37,
	37,38,38,63
};

__declspec(align(16)) uint8_t sCABAC_ContextState[MAX_QP+1][2][4][1024];

#define Clip3(x,y,z) ((z < x) ? x : (z > y) ? y : z)

uint8_t *CABAC_GetBitStream(int *length)
{
	*length = sCABAC_Context.codestrm_bytepos;

	return (sCABAC_Context.stream);
}

static inline void put_one_byte_final(CABACContextType *cbc_enc, unsigned int b)
{
	ENC_ASSERT(cbc_enc->codestrm_bytepos < (CABAC_STREAM_MAX_SIZE-1));

	cbc_enc->stream[(cbc_enc->codestrm_bytepos)++] = (uint8_t) b;
}

static inline void put_buffer(CABACContextType *cbc_enc)
{
	while(cbc_enc->Epbuf>=0)
	{
		ENC_ASSERT(cbc_enc->codestrm_bytepos < (CABAC_STREAM_MAX_SIZE-1));
		cbc_enc->stream[(cbc_enc->codestrm_bytepos)++] =  (uint8_t) ((cbc_enc->Ebuffer>>((cbc_enc->Epbuf--)<<3))&0xFF); 
	}
	while(cbc_enc->C > 7)
	{
		cbc_enc->C-=8;
		++(cbc_enc->E);
	}
	cbc_enc->Ebuffer = 0; 
}

static inline void put_one_byte(CABACContextType *cbc_enc, int b) 
{ 
	if(cbc_enc->Epbuf == 3)
	{ 
		put_buffer(cbc_enc);
	} 
	cbc_enc->Ebuffer <<= 8;
	cbc_enc->Ebuffer += (b);

	++(cbc_enc->Epbuf);
}

static inline void put_one_word(CABACContextType *cbc_enc, int b) 
{
	if (cbc_enc->Epbuf >= 3)
	{ 
		put_buffer(cbc_enc);
	}
	cbc_enc->Ebuffer <<= 16;
	cbc_enc->Ebuffer += (b);

	cbc_enc->Epbuf += 2;
}

static inline void propagate_carry(CABACContextType *cbc_enc)
{
	++(cbc_enc->Ebuffer); 
	while (cbc_enc->Echunks_outstanding > 0) 
	{ 
		put_one_word(cbc_enc, 0); 
		--(cbc_enc->Echunks_outstanding); 
	}
}

static inline void put_last_chunk_plus_outstanding(CABACContextType *cbc_enc, unsigned int l) 
{
	while (cbc_enc->Echunks_outstanding > 0)
	{
		put_one_word(cbc_enc, 0xFFFF);
		--(cbc_enc->Echunks_outstanding);
	}
	put_one_word(cbc_enc, l);
}

static inline void put_last_chunk_plus_outstanding_final(CABACContextType *cbc_enc, unsigned int l) 
{
	while (cbc_enc->Echunks_outstanding > 0)
	{
		put_one_word(cbc_enc, 0xFFFF);
		--(cbc_enc->Echunks_outstanding);
	}
	put_one_byte(cbc_enc, l);
}

int CABAC_Encode_CountCoeffBits(int level);

void InitCABAC_ContextStateTable()
{
	int8_t pre_ctx_state;

#if 0	// make and print out coeff bits array table
	int column = 0;
	ENC_TRACE("\n{\n");
	for (int i = -32768; i <= 32767; i++)
	{
		int count = CABAC_Encode_CountCoeffBits(i);
		ENC_TRACE("%2d,", count);
		if (++column > 63)
		{
			ENC_TRACE(" // %d - %d\n", i - column + 1, i);
			column = 0;
		}
	}
	ENC_TRACE("},\n");
#endif
	for (int type = 0; type < 4; type++)
	{
		const int8_t *context_table = TABLE_GetCABAC_InitTable(type);

		for (int qp = 0; qp <= MAX_QP; qp++)
		{
			for (int i = 0; i < 1024; i++)
			{
				pre_ctx_state = static_cast<int8_t>(Clip3(1, 126, ((context_table[i * 2] * qp) >> 4) + context_table[i*2+1]));
				if (pre_ctx_state <= 63)
				{
					sCABAC_ContextState[qp][0][type][i] = 63 - pre_ctx_state;
					sCABAC_ContextState[qp][1][type][i] = 0;	// valMPS
				}
				else
				{
					sCABAC_ContextState[qp][0][type][i] = pre_ctx_state - 64;
					sCABAC_ContextState[qp][1][type][i] = 1;	// valMPS
				}
			}
		}
	}
}

void InitEncoding()
{
	CABACContextType *cbc_enc = &sCABAC_Context;

	cbc_enc->mb_row_stream = (CABAC_RowStreamType *) _aligned_malloc(sizeof(CABAC_RowStreamType) * gEncoderState.mb_height, 16);
	if (cbc_enc->mb_row_stream == NULL)
	{
		ENC_ASSERT(0);// need error handling
	}
	CABAC_ItemType *row_stream_buf = (CABAC_ItemType *) _aligned_malloc(sizeof(CABAC_ItemType) * gEncoderState.mb_height * gEncoderState.mb_width * CABAC_MAX_ROWSTREAM_MB_SIZE, 16);
	if (row_stream_buf == NULL)
	{
		ENC_ASSERT(0);// need error handling
	}

	sCABAC_Context.bitstream = (unsigned char*) _aligned_malloc(CABAC_STREAM_MAX_SIZE, 16);
	if (sCABAC_Context.bitstream == NULL)
	{
		ENC_ASSERT(0);// need error handling
	}

	// encode directly
	sEncodeFunctionGroups[0].encode_bit = CABAC_Encode_Bit_Output;
	sEncodeFunctionGroups[0].encode_eq_bit = CABAC_Encode_EQ_Bit_Output;
	sEncodeFunctionGroups[0].encode_finalbit = CABAC_Encode_FinalBit_Output;
	sEncodeFunctionGroups[0].start_coeff_tracking = CABAC_Start_Coeff_Tracking_Output;
	sEncodeFunctionGroups[0].end_coeff_tracking = CABAC_End_Coeff_Tracking_Output;

	// spool up encoding
	sEncodeFunctionGroups[1].encode_bit = CABAC_Encode_Bit_Stream;
	sEncodeFunctionGroups[1].encode_eq_bit = CABAC_Encode_EQ_Bit_Stream;
	sEncodeFunctionGroups[1].encode_finalbit = CABAC_Encode_FinalBit_Stream;
	sEncodeFunctionGroups[1].start_coeff_tracking = CABAC_Start_Coeff_Tracking_Stream;
	sEncodeFunctionGroups[1].end_coeff_tracking = CABAC_End_Coeff_Tracking_Stream;

	for(int i = 0; i < gEncoderState.mb_height; i++, row_stream_buf += (gEncoderState.mb_width * CABAC_MAX_ROWSTREAM_MB_SIZE))
	{
		cbc_enc->mb_row_stream[i].num_items = 0;
		cbc_enc->mb_row_stream[i].row_stream = row_stream_buf;
		cbc_enc->mb_row_stream[i].enc_env = cbc_enc;
		cbc_enc->mb_row_stream[i].coeff_bits = 0;
		if (gEncoderState.multi_threading_on)
			cbc_enc->mb_row_stream[i].func = &sEncodeFunctionGroups[(i >= gEncoderState.stream_encoding_row_start)];
		else
			cbc_enc->mb_row_stream[i].func = &sEncodeFunctionGroups[0];
	}
}

void ExitEncoding()
{
	CABACContextType *cbc_enc = &sCABAC_Context;

	if (cbc_enc->mb_row_stream)
	{
		if (cbc_enc->mb_row_stream[0].row_stream)
			_aligned_free(cbc_enc->mb_row_stream[0].row_stream);
		_aligned_free(cbc_enc->mb_row_stream);
		cbc_enc->mb_row_stream = NULL;
	}

	if (cbc_enc->bitstream)
	{
		_aligned_free(cbc_enc->bitstream);
		cbc_enc->bitstream = NULL;
	}
}

void CABAC_ContextInit(CABACContextType *cbc_enc, int slice_type, int qp)
{
	memcpy( cbc_enc->state, sCABAC_ContextState[qp][0][slice_type], 1024);//460 );
	memcpy( cbc_enc->mps,   sCABAC_ContextState[qp][1][slice_type], 1024);//460 );

	for(int i = 0; i < gEncoderState.mb_height; i++)
	{
		cbc_enc->mb_row_stream[i].num_items = 0;
		cbc_enc->mb_row_stream[i].coef_write_pos = 0;
		cbc_enc->mb_row_stream[i].coef_counter = 0;
		memset(cbc_enc->mb_row_stream[i].coef_buffer, 0, 64 * sizeof(int));
	}
}

void CABAC_ContextExit(CABACContextType *cbc_enc)
{

}

void CABAC_Start_EncodingStream(CABACContextType *cbc_enc, unsigned char *code_buffer, int code_len, int code_byte_pos)
{
	cbc_enc->Elow = 0;
	cbc_enc->Echunks_outstanding = 0;
	cbc_enc->Ebuffer = 0;
	cbc_enc->Epbuf = -1;  // to remove redundant chunk ^^
	cbc_enc->Ebits_to_go = BITS_TO_LOAD + 1; // to swallow first redundant bit

	cbc_enc->stream = code_buffer;
	cbc_enc->stream_end = code_buffer + code_len;
	cbc_enc->codestrm_bytepos = code_byte_pos;

	cbc_enc->Erange = HALF;
}

void CABAC_Reset_EC(CABACContextType *cbc_enc)
{
	cbc_enc->E = 0;
	cbc_enc->C = 0;
}

struct bit_counter
{
	int mb_total;
	unsigned short mb_mode;
	unsigned short mb_inter;
	unsigned short mb_cbp;
	unsigned short mb_delta_quant;
	int mb_y_coeff;
	int mb_uv_coeff;
	int mb_cb_coeff;
	int mb_cr_coeff;  
	int mb_stuffing;
};

void CABAC_Stop_EncodingStream(CABACContextType *cbc_enc)
{
	unsigned int low = cbc_enc->Elow;  
	int remaining_bits = BITS_TO_LOAD - cbc_enc->Ebits_to_go; // output (2 + remaining) bits for terminating the codeword + one stop bit
	unsigned char mask;
//	struct bit_counter bits, *mbBits = &bits;

	if (remaining_bits <= 5) // one terminating byte 
	{
//		mbBits->mb_stuffing += (5 - remaining_bits);
		mask = (unsigned char) (255 - ((1 << (6 - remaining_bits)) - 1)); 
		low = (low >> (MASK_BITS)) & mask; // mask out the (2+remaining_bits) MSBs
		low += (32 >> remaining_bits);       // put the terminating stop bit '1'

		put_last_chunk_plus_outstanding_final(cbc_enc, low);
		put_buffer(cbc_enc);
	}
	else 
	if(remaining_bits <= 13)            // two terminating bytes
	{
//		mbBits->mb_stuffing += (13 - remaining_bits);
		put_last_chunk_plus_outstanding_final(cbc_enc, ((low >> (MASK_BITS)) & 0xFF)); // mask out the 8 MSBs for output

		put_buffer(cbc_enc);
		if (remaining_bits > 6)
		{
			mask = (unsigned char) (255 - ((1 << (14 - remaining_bits)) - 1)); 
			low = (low >> (B_BITS)) & mask; 
			low += (0x2000 >> remaining_bits);     // put the terminating stop bit '1'
			put_one_byte_final(cbc_enc, low);
		}
		else
		{
			put_one_byte_final(cbc_enc, 128); // second byte contains terminating stop bit '1' only
		}
	}
	else             // three terminating bytes
	{ 
		put_last_chunk_plus_outstanding(cbc_enc, ((low >> B_BITS) & B_LOAD_MASK)); // mask out the 16 MSBs for output
		put_buffer(cbc_enc);
//		mbBits->mb_stuffing += (21 - remaining_bits);

		if (remaining_bits > 14)
		{
			mask = (unsigned char) (255 - ((1 << (22 - remaining_bits)) - 1)); 
			low = (low >> (MAX_BITS - 24)) & mask; 
			low += (0x200000 >> remaining_bits);       // put the terminating stop bit '1'
			put_one_byte_final(cbc_enc, low);
		}
		else
		{
			put_one_byte_final(cbc_enc, 128); // third byte contains terminating stop bit '1' only
		}
	}
	cbc_enc->Ebits_to_go = 8;
}

static inline int CABAC_Encode_BitsWritten(CABACContextType *cbc_enc)
{
	return (((cbc_enc->codestrm_bytepos) + cbc_enc->Epbuf + 1) << 3) + (cbc_enc->Echunks_outstanding * BITS_TO_LOAD) + BITS_TO_LOAD - cbc_enc->Ebits_to_go;
}

#ifdef DEBUG_STATS
int CABAC_Encode_BitsWritten_()
{
    return CABAC_Encode_BitsWritten(&sCABAC_Context);
}
#endif

#ifdef DEBUG_CABAC_OUTPUT

typedef union{
	struct {
	unsigned state : 8;
	unsigned bit : 1;
	unsigned mps : 1;
	unsigned eq : 1;
	unsigned final : 1;
	unsigned range : 20;
	};
	uint32_t value;
}DebugCABACType;

static DebugCABACType sDebugCABAC_Buffer[128*1024];
static int sDebugCABAC_Index = 0;
#endif
static bool trace_on = 0;

static void CABAC_Encode_Bit_Output(CABAC_RowStreamType *stream, int symbol, int ctx_index )
{
	CABACContextType *cbc_enc = stream->enc_env;
	unsigned int low = cbc_enc->Elow;
	unsigned int range = cbc_enc->Erange;
	int bl = cbc_enc->Ebits_to_go;
	int state = cbc_enc->state[ctx_index];
	unsigned int rLPS = rLPS_table_64x4[state][(range>>6) & 3]; 

	range -= rLPS;

	++(cbc_enc->C);
//	bi_ct->count += cbc_enc->p_Vid->cabac_encoding;

	/* covers all cases where code does not bother to shift down symbol to be 
	* either 0 or 1, e.g. in some cases for cbp, mb_Type etc the code simply 
	* masks off the bit position and passes in the resulting value */
	//symbol = (short) (symbol != 0);
#ifdef DEBUG_CABAC_OUTPUT
	sDebugCABAC_Buffer[sDebugCABAC_Index].bit = symbol & 1;
	sDebugCABAC_Buffer[sDebugCABAC_Index].mps = cbc_enc->mps[ctx_index];
	sDebugCABAC_Buffer[sDebugCABAC_Index].eq = 0;
	sDebugCABAC_Buffer[sDebugCABAC_Index].final = 0;
	sDebugCABAC_Buffer[sDebugCABAC_Index].state = state;
	sDebugCABAC_Buffer[sDebugCABAC_Index].range = cbc_enc->Elow;
	sDebugCABAC_Index++;
#endif
	if (trace_on)
	{
		ENC_TRACE("** %d 0x%02x %d 0x%08x **\n", symbol&1, state, cbc_enc->mps[ctx_index], low);
	}
//ENC_ASSERT(((symbol != 0) && (symbol & 1)) || (symbol == 0) );
	if ((symbol & 1) == cbc_enc->mps[ctx_index])  //MPS
	{
		cbc_enc->state[ctx_index] = AC_next_state_MPS_64[state]; // next state

		if( range >= QUARTER ) // no renorm
		{
			cbc_enc->Erange = range;
			return;
		}
		else 
		{   
			range<<=1;
			if( --bl > MIN_BITS_TO_GO )  // renorm once, no output
			{
				cbc_enc->Erange = range;
				cbc_enc->Ebits_to_go = bl;
				return;
			}
		}
	}
	else         //LPS
	{
		unsigned int renorm = renorm_table_32[(rLPS >> 3) & 0x1F];

		low += range << bl;
		range = (rLPS << renorm);
		bl -= renorm;

		if (!state)
			cbc_enc->mps[ctx_index] ^= 0x01;               // switch MPS if necessary

		cbc_enc->state[ctx_index] = AC_next_state_LPS_64[state]; // next state

		if (low >= ONE) // output of carry needed
		{
			low -= ONE;
			propagate_carry(cbc_enc);
		}

		if( bl > MIN_BITS_TO_GO )
		{
			cbc_enc->Elow   = low;
			cbc_enc->Erange = range;      
			cbc_enc->Ebits_to_go = bl;
			return;
		}
	}

	//renorm needed
	cbc_enc->Elow = (low << BITS_TO_LOAD )& (ONE_M1);
	low = (low >> B_BITS) & B_LOAD_MASK; // mask out the 8/16 MSBs for output

	if (low < B_LOAD_MASK) // no carry possible, output now
	{
		put_last_chunk_plus_outstanding(cbc_enc, low);
	}
	else          // low == "FF.."; kcbc_enc it, may affect future carry
	{
		++(cbc_enc->Echunks_outstanding);
	}
	cbc_enc->Erange = range;  
	cbc_enc->Ebits_to_go = bl + BITS_TO_LOAD;
}

static void CABAC_Encode_FinalBit_Output(CABAC_RowStreamType *stream, int symbol)
{
	CABACContextType *cbc_enc = stream->enc_env;

	unsigned int range = cbc_enc->Erange - 2;
	unsigned int low = cbc_enc->Elow;
	int bl = cbc_enc->Ebits_to_go; 

#ifdef DEBUG_CABAC_OUTPUT
	sDebugCABAC_Buffer[sDebugCABAC_Index].bit = symbol != 0;
	sDebugCABAC_Buffer[sDebugCABAC_Index].mps = 0;
	sDebugCABAC_Buffer[sDebugCABAC_Index].eq = 0;
	sDebugCABAC_Buffer[sDebugCABAC_Index].final = 1;
	sDebugCABAC_Buffer[sDebugCABAC_Index].state = 0;
	sDebugCABAC_Buffer[sDebugCABAC_Index].range = cbc_enc->Elow;
	sDebugCABAC_Index++;
#endif
	if (trace_on)
	{
		ENC_TRACE("** %d 0x%02x %d 0x%08x **\n", symbol&1, 0, 0, low);
	}

	++(cbc_enc->C);

	if (symbol == 0) // MPS
	{
		if( range >= QUARTER ) // no renorm
		{
			cbc_enc->Erange = range;
			return;
		}
		else 
		{   
			range<<=1;
			if( --bl > MIN_BITS_TO_GO )  // renorm once, no output
			{
				cbc_enc->Erange = range;
				cbc_enc->Ebits_to_go = bl;
				return;
			}
		}
	}
	else     // LPS
	{
		low += (range << bl);
		range = 2;

		if (low >= ONE) // output of carry needed
		{
			low -= ONE; // remove MSB, i.e., carry bit
			propagate_carry(cbc_enc);
		}
		bl -= 7; // 7 left shifts needed to renormalize

		range <<= 7;
		if( bl > MIN_BITS_TO_GO )
		{
			cbc_enc->Erange = range;
			cbc_enc->Elow   = low;
			cbc_enc->Ebits_to_go = bl;
			return;
		}
	}


	//renorm needed
	cbc_enc->Elow = (low << BITS_TO_LOAD ) & (ONE_M1);
	low = (low >> B_BITS) & B_LOAD_MASK; // mask out the 8/16 MSBs
	if (low < B_LOAD_MASK)
	{  // no carry possible, output now
		put_last_chunk_plus_outstanding(cbc_enc, low);
	}
	else
	{  // low == "FF"; kcbc_enc it, may affect future carry
		++(cbc_enc->Echunks_outstanding);
	}

	cbc_enc->Erange = range;
	bl += BITS_TO_LOAD;
	cbc_enc->Ebits_to_go = bl;
}

void CABAC_Encode_EQ_Bit_Output(CABAC_RowStreamType *stream, int symbol)
{
	CABACContextType *cbc_enc = stream->enc_env;

	unsigned int low = cbc_enc->Elow;
	--(cbc_enc->Ebits_to_go);  
	++(cbc_enc->C);

#ifdef DEBUG_CABAC_OUTPUT
	sDebugCABAC_Buffer[sDebugCABAC_Index].bit = symbol != 0;
	sDebugCABAC_Buffer[sDebugCABAC_Index].mps = 0;
	sDebugCABAC_Buffer[sDebugCABAC_Index].eq = 1;
	sDebugCABAC_Buffer[sDebugCABAC_Index].final = 0;
	sDebugCABAC_Buffer[sDebugCABAC_Index].state = 0;
	sDebugCABAC_Buffer[sDebugCABAC_Index].range = cbc_enc->Elow;
	sDebugCABAC_Index++;
#endif
	if (trace_on)
	{
	ENC_TRACE("EQ: %d\n", symbol);
	}

	if (symbol != 0)
	{
		low += cbc_enc->Erange << cbc_enc->Ebits_to_go;
		if (low >= ONE) // output of carry needed
		{
			low -= ONE;
			propagate_carry(cbc_enc);
		}
	}
	if(cbc_enc->Ebits_to_go == MIN_BITS_TO_GO)  // renorm needed
	{
		cbc_enc->Elow = (low << BITS_TO_LOAD )& (ONE_M1);
		low = (low >> B_BITS) & B_LOAD_MASK; // mask out the 8/16 MSBs for output
		if (low < B_LOAD_MASK)      // no carry possible, output now
		{
			put_last_chunk_plus_outstanding(cbc_enc, low);}
		else          // low == "FF"; keep it, may affect future carry
		{
			++(cbc_enc->Echunks_outstanding);
		}

		cbc_enc->Ebits_to_go = BITS_TO_LOAD;
		return;
	}
	else         // no renorm needed
	{
		cbc_enc->Elow = low;
		return;
	}
}

void CABAC_Start_Coeff_Tracking_Output(CABAC_RowStreamType *stream)
{
	stream->coeff_start_pos = CABAC_Encode_BitsWritten(stream->enc_env);
}

void CABAC_End_Coeff_Tracking_Output(CABAC_RowStreamType *stream)
{
	stream->coeff_bits += CABAC_Encode_BitsWritten(stream->enc_env) - stream->coeff_start_pos;
}

#if 1
void CABAC_Encode_RowStream(int row_index)
{
	CABACContextType *cbc_enc = &sCABAC_Context;
	CABAC_RowStreamType *stream = &cbc_enc->mb_row_stream[row_index];
	CABAC_ItemType *items = stream->row_stream;

	unsigned int low = cbc_enc->Elow;
	unsigned int range = cbc_enc->Erange;

	bool coeff_tracking_on = false;
	int coeff_start_pos = 0;

	for (unsigned int i = (unsigned int) stream->num_items; i > 0; i--, items++)
	{
		if (items->flags.encode_bit)
		{
//			low = cbc_enc->Elow;
//			unsigned int range = cbc_enc->Erange;
			int bl = cbc_enc->Ebits_to_go;
			int state = cbc_enc->state[items->flags.ctx_idx];
			unsigned int rLPS = rLPS_table_64x4[state][(range>>6) & 3]; 

			range -= rLPS;

			++(cbc_enc->C);

			ENC_ASSERT(cbc_enc->codestrm_bytepos < (CABAC_STREAM_MAX_SIZE-1));

			if (items->flags.bit == cbc_enc->mps[items->flags.ctx_idx])  //MPS
			{
				cbc_enc->state[items->flags.ctx_idx] = AC_next_state_MPS_64[state]; // next state

				if( range >= QUARTER ) // no renorm
				{
//					cbc_enc->Erange = range;
					continue;
				}
				else 
				{   
					range<<=1;
					if( --bl > MIN_BITS_TO_GO )  // renorm once, no output
					{
//						cbc_enc->Erange = range;
						cbc_enc->Ebits_to_go = bl;
						continue;
					}
				}
			}
			else         //LPS
			{
				unsigned int renorm = renorm_table_32[(rLPS >> 3) & 0x1F];

				low += range << bl;
				range = (rLPS << renorm);
				bl -= renorm;

				if (!state)
					cbc_enc->mps[items->flags.ctx_idx] ^= 0x01;               // switch MPS if necessary

				cbc_enc->state[items->flags.ctx_idx] = AC_next_state_LPS_64[state]; // next state

				if (low >= ONE) // output of carry needed
				{
					low -= ONE;
					propagate_carry(cbc_enc);
				}

				if( bl > MIN_BITS_TO_GO )
				{
//					cbc_enc->Elow   = low;
//					cbc_enc->Erange = range;      
					cbc_enc->Ebits_to_go = bl;
					continue;
				}
			}

			//renorm needed
//			cbc_enc->Elow = (low << BITS_TO_LOAD )& (ONE_M1);
			unsigned int tmp_low = low;
			low = (low << BITS_TO_LOAD )& (ONE_M1);
			tmp_low = (tmp_low >> B_BITS) & B_LOAD_MASK; // mask out the 8/16 MSBs for output

			if (tmp_low < B_LOAD_MASK) // no carry possible, output now
			{
				put_last_chunk_plus_outstanding(cbc_enc, tmp_low);
			}
			else          // low == "FF.."; kcbc_enc it, may affect future carry
			{
				++(cbc_enc->Echunks_outstanding);
			}
//			cbc_enc->Erange = range;  
			cbc_enc->Ebits_to_go = bl + BITS_TO_LOAD;
		}
		else
		if (items->flags.eq)
		{
			--(cbc_enc->Ebits_to_go);  
			++(cbc_enc->C);

			if (items->flags.bit != 0)
			{
				low += range << cbc_enc->Ebits_to_go;
				if (low >= ONE) // output of carry needed
				{
					low -= ONE;
					propagate_carry(cbc_enc);
				}
			}
			if(cbc_enc->Ebits_to_go == MIN_BITS_TO_GO)  // renorm needed
			{

//				cbc_enc->Elow = (low << BITS_TO_LOAD )& (ONE_M1);
				unsigned int tmp_low = low;
				low = (low << BITS_TO_LOAD )& (ONE_M1);
				tmp_low = (tmp_low >> B_BITS) & B_LOAD_MASK; // mask out the 8/16 MSBs for output

				if (tmp_low < B_LOAD_MASK)
				{
					put_last_chunk_plus_outstanding(cbc_enc, tmp_low);
				}
				else          // low == "FF"; keep it, may affect future carry
				{
					++(cbc_enc->Echunks_outstanding);
				}

				cbc_enc->Ebits_to_go = BITS_TO_LOAD;
			}
			else         // no renorm needed
			{
//				cbc_enc->Elow = low;
			}
		}
		else
		if (items->value == (1 << 13))	// coefficient bit tracking for rate control
		{
			if (coeff_tracking_on)
			{
				stream->coeff_bits += CABAC_Encode_BitsWritten(stream->enc_env) - coeff_start_pos;
				coeff_tracking_on = false;
			}
			else
			{
				coeff_start_pos = CABAC_Encode_BitsWritten(stream->enc_env);
				coeff_tracking_on = true;
			}
		}
		else
		{
			range = range - 2;
//			unsigned int low = cbc_enc->Elow;
			int bl = cbc_enc->Ebits_to_go; 

			++(cbc_enc->C);

			if (items->flags.bit == 0) // MPS
			{
				if( range >= QUARTER ) // no renorm
				{
//					cbc_enc->Erange = range;
					continue;
				}
				else 
				{   
					range<<=1;
					if( --bl > MIN_BITS_TO_GO )  // renorm once, no output
					{
//						cbc_enc->Erange = range;
						cbc_enc->Ebits_to_go = bl;
						continue;
					}
				}
			}
			else     // LPS
			{
				low += (range << bl);
				range = 2;

				if (low >= ONE) // output of carry needed
				{
					low -= ONE; // remove MSB, i.e., carry bit
					propagate_carry(cbc_enc);
				}
				bl -= 7; // 7 left shifts needed to renormalize

				range <<= 7;
				if( bl > MIN_BITS_TO_GO )
				{
//					cbc_enc->Erange = range;
//					cbc_enc->Elow   = low;
					cbc_enc->Ebits_to_go = bl;
					continue;
				}
			}


			//renorm needed
//			cbc_enc->Elow = (low << BITS_TO_LOAD ) & (ONE_M1);
			unsigned int tmp_low = low;
			low = (low << BITS_TO_LOAD )& (ONE_M1);
			tmp_low = (tmp_low >> B_BITS) & B_LOAD_MASK; // mask out the 8/16 MSBs for output

			if (tmp_low < B_LOAD_MASK)
			{  // no carry possible, output now
				put_last_chunk_plus_outstanding(cbc_enc, tmp_low);
			}
			else
			{  // low == "FF"; kcbc_enc it, may affect future carry
				++(cbc_enc->Echunks_outstanding);
			}

//			cbc_enc->Erange = range;
			bl += BITS_TO_LOAD;
			cbc_enc->Ebits_to_go = bl;
		}
	}

	cbc_enc->Elow   = low;
	cbc_enc->Erange = range;      
}
#else
void CABAC_Encode_RowStream(int row_index)
{
	CABACContextType *cbc_enc = &sCABAC_Context;
	CABAC_RowStreamType *stream = &cbc_enc->mb_row_stream[row_index];
	CABAC_ItemType *items = stream->row_stream;

	for (int i = 0; i < stream->num_items; i++, items++)
	{
		if (items->eq)
		{
			CABAC_Encode_EQ_Bit_Output(stream, items->bit);
		}
		else
		if (items->final)
		{
			CABAC_Encode_FinalBit_Output(stream, items->bit);
		}
		else
		{
			CABAC_Encode_Bit_Output(stream, items->bit, items->ctx_idx);
		}
	}
}
#endif
void CABAC_EncodeStream_Synchronous(CABACContextType *cbc_enc)
{
	for (int i = 0; i < gEncoderState.mb_height; i++)
	{
		CABAC_Encode_RowStream(i);
	}
}


// Collection into stream
static inline void CABAC_Encode_Bit_Stream(CABAC_RowStreamType *stream, int symbol, int ctx_index )
{
#if 1
	uint16_t value = static_cast<uint16_t>(ctx_index | ((symbol & 1) << 13) | 0x8000);

	ENC_ASSERT((ctx_index < (1 << 12)));	// make sure we don't overflow

	stream->row_stream[stream->num_items++].value = value;
#else
	stream->row_stream[stream->num_items].bit = symbol;
	stream->row_stream[stream->num_items].eq = 0;
	stream->row_stream[stream->num_items].encode_bit = 1;
	stream->row_stream[stream->num_items++].ctx_idx = ctx_index;
#endif
	ENC_ASSERT((stream->num_items < (gEncoderState.mb_width * CABAC_MAX_ROWSTREAM_MB_SIZE)));
}

static inline void CABAC_Encode_EQ_Bit_Stream(CABAC_RowStreamType *stream, int symbol)
{
	stream->row_stream[stream->num_items].flags.bit = symbol;
	stream->row_stream[stream->num_items].flags.eq = 1;
	stream->row_stream[stream->num_items++].flags.encode_bit = 0;

	ENC_ASSERT((stream->num_items < (gEncoderState.mb_width * CABAC_MAX_ROWSTREAM_MB_SIZE)));
}

static inline void CABAC_Encode_FinalBit_Stream(CABAC_RowStreamType *stream, int symbol)
{
	stream->row_stream[stream->num_items].flags.bit = symbol;
	stream->row_stream[stream->num_items].flags.eq = 0;
	stream->row_stream[stream->num_items++].flags.encode_bit = 0;

	ENC_ASSERT((stream->num_items < (gEncoderState.mb_width * CABAC_MAX_ROWSTREAM_MB_SIZE)));
}

static inline void CABAC_Start_Coeff_Tracking_Stream(CABAC_RowStreamType *stream)
{
	stream->row_stream[stream->num_items++].value = 1 << 13;

	ENC_ASSERT((stream->num_items < (gEncoderState.mb_width * CABAC_MAX_ROWSTREAM_MB_SIZE)));
}

static inline void CABAC_End_Coeff_Tracking_Stream(CABAC_RowStreamType *stream)
{
	stream->row_stream[stream->num_items++].value = 1 << 13;

	ENC_ASSERT((stream->num_items < (gEncoderState.mb_width * CABAC_MAX_ROWSTREAM_MB_SIZE)));
}

static void CABAC_Encode_exp_golomb_encode_EQ(CABAC_RowStreamType *stream, unsigned int symbol, int k)
{
	for(;;)
	{
		if (symbol >= (unsigned int)(1<<k))
		{
			stream->func->encode_eq_bit(stream, 1);   //first unary part
			symbol = symbol - (1<<k);
			k++;
		}
		else
		{
			stream->func->encode_eq_bit(stream, 0);   //now terminated zero of unary part
			while (k--)                               //next binary part
				stream->func->encode_eq_bit(stream, ((symbol>>k)&1));
			break;
		}
	}
}

static void CABAC_Encode_unary_exp_golomb(CABAC_RowStreamType *stream, unsigned int symbol, int ctxIdx, int ctx_offset)
{
	if (symbol == 0)
	{
		stream->func->encode_bit(stream, 0, ctxIdx );
		return;
	}
	else
	{
		stream->func->encode_bit(stream, 1, ctxIdx );
		ctxIdx += ctx_offset;
		while ((--symbol) > 0)
			stream->func->encode_bit(stream, 1, ctxIdx );
		stream->func->encode_bit(stream, 0, ctxIdx );
	}
}


static void CABAC_Encode_unary_exp_golomb_level(CABAC_RowStreamType *stream, unsigned int symbol, int ctxIdx)
{
	if (symbol == 0)
	{
		stream->func->encode_bit(stream, 0, ctxIdx );
		return;
	}
	else
	{
		unsigned int l=symbol;
		unsigned int k = 1;

		stream->func->encode_bit(stream, 1, ctxIdx );
		while (((--l)>0) && (++k <= 13))
			stream->func->encode_bit(stream, 1, ctxIdx );
		if (symbol < 13) 
			stream->func->encode_bit(stream, 0, ctxIdx );
		else
			CABAC_Encode_exp_golomb_encode_EQ(stream, symbol - 13, 0);
	}
}

static void CABAC_Encode_unary_exp_golomb_MV(CABAC_RowStreamType *stream, unsigned int symbol, int ctxIdx, unsigned int max_bin)
{  
	if (symbol == 0)
	{
		stream->func->encode_bit(stream, 0, ctxIdx );
		return;
	}
	else
	{
		unsigned int bin = 1;
		unsigned int l = symbol, k = 1;
		stream->func->encode_bit(stream, 1, ctxIdx++ );

		while (((--l)>0) && (++k <= 8))
		{
			stream->func->encode_bit(stream, 1, ctxIdx  );
			if ((++bin) == 2) 
				++ctxIdx;
			if (bin == max_bin) 
				++ctxIdx;
		}
		if (symbol < 8) 
			stream->func->encode_bit(stream, 0, ctxIdx);
		else 
			CABAC_Encode_exp_golomb_encode_EQ(stream, symbol - 8, 3);
	}
}

static const uint8_t ISlice_BinarizationTable[26][2] =	// { bin string, bit length } (Table 9-36)
{
	{ 0, 1 }, 
	{ 0x20, 5 }, // I_16x16_0_0_0
	{ 0x21, 5 }, // I_16x16_1_0_0
	{ 0x22, 5 }, // I_16x16_2_0_0
	{ 0x23, 5 }, // I_16x16_3_0_0
	{ 0x48, 6 }, // I_16x16_0_1_0
	{ 0x49, 6 }, // I_16x16_1_1_0
	{ 0x4A, 6 }, // I_16x16_2_1_0
	{ 0x4B, 6 }, // I_16x16_3_1_0
	{ 0x4C, 6 }, // I_16x16_0_2_0
	{ 0x4D, 6 }, // I_16x16_1_2_0	// 10
	{ 0x4E, 6 }, // I_16x16_2_2_0
	{ 0x4F, 6 }, // I_16x16_3_2_0
	{ 0x28, 5 }, // I_16x16_0_0_1
	{ 0x29, 5 }, // I_16x16_1_0_1
	{ 0x2A, 5 }, // I_16x16_2_0_1
	{ 0x2B, 5 }, // I_16x16_3_0_1
	{ 0x58, 6 }, // I_16x16_0_1_1
	{ 0x59, 6 }, // I_16x16_1_1_1
	{ 0x5A, 6 }, // I_16x16_2_1_1
	{ 0x5B, 6 }, // I_16x16_3_1_1   // 20
	{ 0x5C, 6 }, // I_16x16_0_2_1
	{ 0x5D, 6 }, // I_16x16_1_2_1
	{ 0x5E, 6 }, // I_16x16_2_2_1
	{ 0x5F, 6 }, // I_16x16_3_2_1
	{ 0x03, 2 }, // I_PCM
};

void CABAC_Encode_mb_type_I(MacroBlockType *mb, CABAC_RowStreamType *stream )
{
	int num_bits = ISlice_BinarizationTable[mb->mb_type][1];
	int ctx_idx_inc = 0;

	if (mb->mbAddrA && !mb->mbAddrA->mb_skip_flag && (mb->mbAddrA->mb_type != MB_TYPE_INxN))
		ctx_idx_inc++;
	if (mb->mbAddrB && !mb->mbAddrB->mb_skip_flag && (mb->mbAddrB->mb_type != MB_TYPE_INxN))
		ctx_idx_inc++;

	if (num_bits <= 2)
	{
		if (num_bits == 1)	// I_4x4 or I_8x8
			stream->func->encode_bit(stream, 0, ctx_idx_inc + 3);
		else
		{	// I_PCM
			stream->func->encode_bit(stream, 1, ctx_idx_inc + 3);
			stream->func->encode_finalbit(stream, 1);
		}
		return;
	}

	// Table 9-39 - bit - ctxIdxOffset magic
	int bin = ISlice_BinarizationTable[mb->mb_type][0];
	int i;
	stream->func->encode_bit(stream, 1, ctx_idx_inc+3);
	stream->func->encode_finalbit(stream, 0);
	for (i = 1; i < 3; i++)
	{
		stream->func->encode_bit(stream, (bin >> (num_bits - i - 1)) & 1, i+5);
	}

	ctx_idx_inc = 0;
	if (num_bits == 5)	// no chroma
		ctx_idx_inc++;

	for (; i < num_bits; i++)
	{
		stream->func->encode_bit(stream, (bin >> (num_bits - i - 1)) & 1, i+ctx_idx_inc+5);
	}
}

// mb_type ranges from 0-30 0-4 - P types, 5-30 I types (corresponding to I Slice MB types 0-25)
// currently only concerned with I types
void CABAC_Encode_mb_type_P(MacroBlockType *mb, CABAC_RowStreamType *stream)
{
	int ctx_idx = 14;	// table 9-11 (14..20)

	if (mb->mb_type < 5)
	{
		if (mb->mb_type == MB_TYPE_P16x16)
		{
			stream->func->encode_bit(stream, 0, ctx_idx);	// prefix bit to say we are in the 0-4 mb_type range (clause 9.3.2.5) (Table 9-34)
			stream->func->encode_bit(stream, 0, ctx_idx+1);
			stream->func->encode_bit(stream, 0, ctx_idx+2);
		}
		return;
	}

	stream->func->encode_bit(stream, 1, ctx_idx);	// prefix bit to say we are in the 5-30 mb_type range (clause 9.3.2.5) (Table 9-34)

	// Table 9-39 - bit -> ctxIdxOffset magic
	int num_bits = ISlice_BinarizationTable[mb->mb_type-5][1];

	if (num_bits <= 2)
	{
		if (num_bits == 1)	// I_4x4 or I_8x8
			stream->func->encode_bit(stream, 0, ctx_idx+3);
		else
		{	// I_PCM
			stream->func->encode_bit(stream, 1, ctx_idx+3);
			stream->func->encode_finalbit(stream, 1);
		}
		return;
	}

	int bin = ISlice_BinarizationTable[mb->mb_type-5][0];
	int i;
	stream->func->encode_bit(stream, 1, ctx_idx+3);
	stream->func->encode_finalbit(stream, 0);
	for (i = 1; i < 3; i++)
	{
		stream->func->encode_bit(stream, (bin >> (num_bits - i - 1)) & 1, i+ctx_idx+3);
	}

	if (num_bits == 6)	// has chroma
	{
		stream->func->encode_bit(stream, (bin >> (num_bits - i - 1)) & 1, ctx_idx+5);	// 19
		i++;
	}

	for (; i < num_bits; i++)
	{
		stream->func->encode_bit(stream, (bin >> (num_bits - i - 1)) & 1, ctx_idx+6);	// 20
	}
}

// mb_type ranges from 0-48, 0-22 B types 23-48 I types (corresponding to I Slice MB types 0-25)
void CABAC_Encode_mb_type_B(MacroBlockType *mb, CABAC_RowStreamType *stream)
{
	int ctx_idx = 27;	// table 9-11 (27..35)
	int ctx_idx_inc = 0;

	// for binIdx == 0, clause 9.3.3.1.1.3 (table 9-39)
	if (mb->mbAddrA && !mb->mbAddrA->mb_skip_flag && (mb->mbAddrA->mb_type != MB_TYPE_B_DIRECT_16x16))
		ctx_idx_inc++;
	if (mb->mbAddrB && !mb->mbAddrB->mb_skip_flag && (mb->mbAddrB->mb_type != MB_TYPE_B_DIRECT_16x16))
		ctx_idx_inc++;

	if (mb->mb_type < 23)
	{
		// from table 9-37
		switch (mb->mb_type)
		{
		case MB_TYPE_B_DIRECT_16x16:
			stream->func->encode_bit(stream, 0, ctx_idx + ctx_idx_inc);
			break;
		case MB_TYPE_B_L0_16x16:
			stream->func->encode_bit(stream, 1, ctx_idx + ctx_idx_inc);
			stream->func->encode_bit(stream, 0, ctx_idx+3);
			stream->func->encode_bit(stream, 0, ctx_idx+5);
			break;
		case MB_TYPE_B_L1_16x16:
			stream->func->encode_bit(stream, 1, ctx_idx + ctx_idx_inc);
			stream->func->encode_bit(stream, 0, ctx_idx+3);
			stream->func->encode_bit(stream, 1, ctx_idx+5);
			break;
		case MB_TYPE_B_Bi_16x16:
			stream->func->encode_bit(stream, 1, ctx_idx + ctx_idx_inc);
			stream->func->encode_bit(stream, 1, ctx_idx+3);
			stream->func->encode_bit(stream, 0, ctx_idx+4);
			stream->func->encode_bit(stream, 0, ctx_idx+5);
			stream->func->encode_bit(stream, 0, ctx_idx+5);
			stream->func->encode_bit(stream, 0, ctx_idx+5);
			break;
		default:
			ENC_ASSERT(0);	// we don't handle any others right now
		}

		return;
	}

	// for Intra MB types
	// long prefix sequence to say we are in the 23-48 I mb_type range (clause 9.3.2.5) (Table 9-34)
	stream->func->encode_bit(stream, 1, ctx_idx + ctx_idx_inc);	
	stream->func->encode_bit(stream, 1, ctx_idx+3);
	stream->func->encode_bit(stream, 1, ctx_idx+4);
	stream->func->encode_bit(stream, 1, ctx_idx+5);
	stream->func->encode_bit(stream, 0, ctx_idx+5);
	stream->func->encode_bit(stream, 1, ctx_idx+5);

	// Table 9-39 - bit -> ctxIdxOffset magic
	int num_bits = ISlice_BinarizationTable[mb->mb_type-23][1];

	ctx_idx = 32;	// ctxIdxOffset = 32 for B-slice I MB types

	if (num_bits <= 2)
	{
		if (num_bits == 1)	// I_4x4 or I_8x8
			stream->func->encode_bit(stream, 0, ctx_idx);
		else
		{	// I_PCM
			stream->func->encode_bit(stream, 1, ctx_idx);
			stream->func->encode_finalbit(stream, 1);
		}
		return;
	}

	int bin = ISlice_BinarizationTable[mb->mb_type-23][0];
	int i;
	stream->func->encode_bit(stream, 1, ctx_idx);
	stream->func->encode_finalbit(stream, 0);
	for (i = 1; i < 3; i++)
	{
		stream->func->encode_bit(stream, (bin >> (num_bits - i - 1)) & 1, ctx_idx + i);
	}

	if (num_bits == 6)	// has chroma
	{
		stream->func->encode_bit(stream, (bin >> (num_bits - i - 1)) & 1, ctx_idx + 2);
		i++;
	}

	for (; i < num_bits; i++)
	{
		stream->func->encode_bit(stream, (bin >> (num_bits - i - 1)) & 1, ctx_idx + 3);
	}
}

uint8_t sCABAC_Unary[8] =
{
	0x00, 0x02, 0x06, 0xe, 0x1e, 0x3e, 0x7e, 0xfe
};

void CABAC_Encode_intra_chroma_pred_mode(MacroBlockType *mb, CABAC_RowStreamType *stream, int pred_mode)
{
	int ctx_idx_inc = 0;

	// TODO: later will need to check for I_PCM, and Inter mb pred modes
	if (mb->mbAddrA && (mb->mbAddrA->chroma_pred_mode != 0))
		ctx_idx_inc++;
	if (mb->mbAddrB && (mb->mbAddrB->chroma_pred_mode != 0))
		ctx_idx_inc++;

	// Truncated Unary cmax = 3
	switch(pred_mode)
	{
	case 0:
		stream->func->encode_bit(stream, 0, 64 + ctx_idx_inc);
		break;
	case 1:
		stream->func->encode_bit(stream, 1, 64 + ctx_idx_inc);
		stream->func->encode_bit(stream, 0, 64 + 3);
		break;
	case 2:
		stream->func->encode_bit(stream, 1, 64 + ctx_idx_inc);
		stream->func->encode_bit(stream, 1, 64 + 3);
		stream->func->encode_bit(stream, 0, 64 + 3);
		break;
	case 3:
		stream->func->encode_bit(stream, 1, 64 + ctx_idx_inc);
		stream->func->encode_bit(stream, 1, 64 + 3);
		stream->func->encode_bit(stream, 1, 64 + 3);
		break;
	}
}

enum{
	COEF_TYPE_LUMA_DC = 0,
	COEF_TYPE_LUMA_AC,
	COEF_TYPE_LUMA_4x4,
	COEF_TYPE_CHROMA_DC,
	COEF_TYPE_CHROMA_AC,
	// 5-13 remain to be defined
	COEF_TYPE_MAX
};

void CABAC_Encode_Coded_Block_Flag_Bit(MacroBlockType* mb, CABAC_RowStreamType *stream, int coef_type, int cbf_bit, int subx, int suby, bool uv)
{
	static const int ctxIdxBlockCatOffset_cbf[COEF_TYPE_MAX] = { 0, 4, 8, 12, 16 }; // from Table 9-40
//	static const int ctxIdxOffset_cbf[COEF_TYPE_MAX] = { 85, 85, 85, 85, 85 };	// from Table 9-34 (currently only interested in lower 5 which are all 85)

	int ctxIdx = 85 + ctxIdxBlockCatOffset_cbf[coef_type];
	int ctxIdxInc = 3;	// default if intra
	if ((gEncoderState.is_P_slice && (mb->mb_type < MB_TYPE_P_INxN)) ||
		(gEncoderState.is_B_slice && (mb->mb_type < MB_TYPE_B_INxN)))
		ctxIdxInc = 0;

	int bit_shift = 0, bit_shift_offset = 0;

	switch(coef_type)
	{
	case COEF_TYPE_LUMA_DC:
		bit_shift = 0;
		break;
	case COEF_TYPE_LUMA_AC:
	case COEF_TYPE_LUMA_4x4:	// for non-I16x16 modes where there is no luma DC
		bit_shift = 1;
		bit_shift_offset = (subx >> 2) + suby;
		break;
	case COEF_TYPE_CHROMA_DC:
		bit_shift = uv ? 17 : 18;	// uv - true if u false if v
		break;
	case COEF_TYPE_CHROMA_AC:
		bit_shift = uv ? 19 : 35;
		bit_shift_offset = (subx >> 2) + suby;
		break;
	}
#if 0
	if (cbf_bit)
	{
		// other types need to be implemented (e.g. luma 8x8, 8x4, 4x8)
		ENC_ASSERT(mb->cbf_bits[0] & ((int64_t) 0x01 << (bit_shift + bit_shift_offset)));
	}
#endif
	if ((coef_type != 5) && (coef_type != 9) && (coef_type != 13))
	{
		if ((coef_type == COEF_TYPE_LUMA_DC) || (coef_type == COEF_TYPE_LUMA_AC) || (coef_type == COEF_TYPE_LUMA_4x4))
		{
			int x = subx >> 2, y = suby >> 2;
			MacroBlockType *mb_a = HasAdjacent4x4SubBlock(mb, ADJACENT_MB_LEFT, &x, &y, true);
			int dim_shift = 0;

			if (mb_a)
			{
				if ((coef_type == COEF_TYPE_LUMA_AC) || (coef_type == COEF_TYPE_LUMA_4x4))
				{
					dim_shift = (x + y*4);
				}

				if (mb_a->mb_skip_flag || !(mb_a->cbf_bits[0] & ((int64_t) 1 << (bit_shift + dim_shift))))	// test transBlockA
				{
					ctxIdxInc &= 0x2;
				}
				else
					ctxIdxInc |= 0x1;
			}

			x = subx >> 2, y = suby >> 2;
			MacroBlockType *mb_b = HasAdjacent4x4SubBlock(mb, ADJACENT_MB_ABOVE, &x, &y, true);

			if (mb_b)
			{
				if ((coef_type == COEF_TYPE_LUMA_AC) || (coef_type == COEF_TYPE_LUMA_4x4))
				{
					dim_shift = (x + y*4);
				}
				else
					dim_shift = 0;

				if (mb_b->mb_skip_flag || !(mb_b->cbf_bits[0] & ((int64_t) 1 << (bit_shift + dim_shift))))	// test transBlockB
				{
					ctxIdxInc &= 0x1;
				}
				else
					ctxIdxInc |= 0x2;
			}
		}
		else
			if ((coef_type == COEF_TYPE_CHROMA_DC) || (coef_type == COEF_TYPE_CHROMA_AC))
			{
				int x = subx >> 2, y = suby >> 2;
				MacroBlockType *mb_a = HasAdjacent4x4SubBlock(mb, ADJACENT_MB_LEFT, &x, &y, false);
				int dim_shift = 0;

				if (mb_a)
				{
					if (coef_type == COEF_TYPE_CHROMA_AC)
						dim_shift = (x + y*4);
					if (mb_a->mb_skip_flag || !(mb_a->cbf_bits[0] & ((int64_t) 1 << (bit_shift + dim_shift))))
					{
						ctxIdxInc &= 0x2;
					}
					else
						ctxIdxInc |= 0x1;
				}

				x = subx >> 2, y = suby >> 2;
				MacroBlockType *mb_b = HasAdjacent4x4SubBlock(mb, ADJACENT_MB_ABOVE, &x, &y, false);
				if (mb_b)
				{
					if (coef_type == COEF_TYPE_CHROMA_AC)
						dim_shift = (x + y*4);
					else
						dim_shift = 0;

					if (mb_b->mb_skip_flag || !(mb_b->cbf_bits[0] & ((int64_t) 1 << (bit_shift + dim_shift))))
					{
						ctxIdxInc &= 0x1;
					}
					else
						ctxIdxInc |= 0x2;
				}
			}

	}
#ifdef CABAC_TRACE_OUTPUT
	ENC_TRACE("CBF: ctxIdxInc: %d\n", ctxIdxInc);
#endif
	stream->func->encode_bit(stream, cbf_bit, ctxIdx + ctxIdxInc);
}

static const int sCtxIdxInc_CtxBlockCat_LumaDC[] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 14 
};

static const int sCtxIdxInc_CtxBlockCat_LumaAC[] =
{
	0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 13, 
};

static const int sCtxIdxInc_CtxBlockCat_ChromaDC[] =
{
	0, 1, 2, 2
};


static const int *sCtxIdxInc_CtxBlockCat[COEF_TYPE_MAX] =
{
	sCtxIdxInc_CtxBlockCat_LumaDC, 
	sCtxIdxInc_CtxBlockCat_LumaAC,
	sCtxIdxInc_CtxBlockCat_LumaDC,
	sCtxIdxInc_CtxBlockCat_ChromaDC,
	sCtxIdxInc_CtxBlockCat_LumaAC,
};

static const int maxNumCoeff[COEF_TYPE_MAX] = { 16, 15, 16, 4, 15 };	// from Table 9-42

void CABAC_Encode_SignificanceMap(CABAC_RowStreamType *stream, int coef_type, int *buffer, int count)
{
	static const bool isAC[COEF_TYPE_MAX] = { false, true, false, false, true};
	static const int ctxIdxBlockCatOffset_scf[COEF_TYPE_MAX] = { 0, 15, 29, 44, 47 }; // from Table 9-40
	int lscf_ctxIdx = 166;	// from Table 9-34
	int lscf_ctxIdxBlockCatOffset = ctxIdxBlockCatOffset_scf[coef_type];
	int scf_ctxIdx = 105;	// from Table 9-34
	int scf_ctxIdxBlockCatOffset = ctxIdxBlockCatOffset_scf[coef_type];
	int ctxIdxInc = 0;

	int start_index = 0;
	int numMaxCoeff = maxNumCoeff[coef_type] - 1;
	if (isAC[coef_type])
	{
		start_index++;
		numMaxCoeff++;
		buffer--;
	}

//ENC_TRACE("SignificanceMap (%d %d)\n  ",start_index, numMaxCoeff);

	for (int i = start_index; i < numMaxCoeff; i++)
	{
		ctxIdxInc = sCtxIdxInc_CtxBlockCat[coef_type][i];
		if (buffer[i])
		{
			stream->func->encode_bit(stream, 1, scf_ctxIdx + ctxIdxInc + scf_ctxIdxBlockCatOffset);	// significant_coeff_flag
			count--;
#ifdef CABAC_TRACE_OUTPUT
ENC_TRACE("(%d %d) ",1,(count==0));
#endif
			if (count)
				stream->func->encode_bit(stream, 0, lscf_ctxIdx + lscf_ctxIdxBlockCatOffset + ctxIdxInc);	// last_significant_coeff_flag
			else
			{
				stream->func->encode_bit(stream, 1, lscf_ctxIdx + lscf_ctxIdxBlockCatOffset + ctxIdxInc);	// last_significant_coeff_flag
				break;
			}
		}
		else
		{
#ifdef CABAC_TRACE_OUTPUT
			ENC_TRACE("(0) ");
#endif
			stream->func->encode_bit(stream, 0, scf_ctxIdx + ctxIdxInc + scf_ctxIdxBlockCatOffset);	// significant_coeff_flag
		}
	}
#ifdef CABAC_TRACE_OUTPUT
ENC_TRACE("\n");
#endif
}

void CABAC_Encode_SignificantCoefficients(CABAC_RowStreamType *stream, int coef_type, int *buffer, int count)
{
	static const short ctxIdxBlockCatOffset_perBlockCat[] = { 0,  10, 20, 30, 39, 0, 0, 10, 20, 0, 0, 10, 20, 0 };	// from Table 9-40

	int   absLevel;
	int   ctx;
	short sign;
	int greater_one;
	int   c1 = 1;
	int   c2 = 0;
	int   i;
	int cxtIdxInc = ctxIdxBlockCatOffset_perBlockCat[coef_type];

//ENC_TRACE("SignificanceCoeff (%d)\n  ",count);

	for (i = maxNumCoeff[coef_type]; (i >= 0) && (count); i--)
	{
		if (buffer[i] != 0)
		{
#ifdef CABAC_TRACE_OUTPUT
ENC_TRACE(" %d ",buffer[i]);
#endif
			count--;

			if (buffer[i] > 0)
			{
				absLevel =  buffer[i];  
				sign = 0;
			}
			else            
			{
				absLevel = -buffer[i];  
				sign = 1;
			}

			greater_one = (absLevel > 1);

			//--- if coefficient is one ---
			ctx = (c1 < 4) ? c1 : 4;
			stream->func->encode_bit (stream, greater_one, 227 + ctx + cxtIdxInc);

			if (greater_one)
			{
				ctx = (c2 < 4) ? c2 : 4;
				c2++;
				CABAC_Encode_unary_exp_golomb_level(stream, absLevel - 2, 227 + ctx + 5 + cxtIdxInc);
				c1 = 0;
			}
			else if (c1)
			{
				c1++;
			}
			stream->func->encode_eq_bit (stream, sign);
		}
	}
#ifdef CABAC_TRACE_OUTPUT
ENC_TRACE("\n");
#endif
}

// just to create bitcount table
int CABAC_Encode_CountCoeffBits(int level)
{
	int   absLevel;
	int   ctx;
	short sign;
	int greater_one;
	int   c1 = 1;
	int   c2 = 0;
	int coeff_count = 0;

	if (level > 0)
	{
		absLevel =  level;  
		sign = 0;
	}
	else            
	{
		absLevel = -level;  
		sign = 1;
	}

	greater_one = (absLevel > 1);

	//--- if coefficient is one ---
	ctx = (c1 < 4) ? c1 : 4;
	coeff_count++;

	if (greater_one)
	{
		ctx = (c2 < 4) ? c2 : 4;
		c2++;

		int symbol = absLevel - 2;
	if (symbol == 0)
	{
		coeff_count++;
	}
	else
	{
		unsigned int l=symbol;
		unsigned int k = 1;

		coeff_count++;
		while (((--l)>0) && (++k <= 13))
		{
		coeff_count++;
		}
		if (symbol < 13) 
		{
			coeff_count++;
		}
		else
		{
			symbol -= 13;
			k = 0;
			for(;;)
			{
				if ((unsigned int)symbol >= (unsigned int)(1<<k))
				{
					coeff_count++;
					symbol = symbol - (1<<k);
					k++;
				}
				else
				{
					coeff_count++;
					while (k--)                               //next binary part
						coeff_count++;
					break;
				}
			}
		}
	}
		c1 = 0;
	}
	else if (c1)
	{
		c1++;
	}
	coeff_count++;

	return coeff_count;
}

void CABAC_Encode_Coefficients (MacroBlockType* mb, CABAC_RowStreamType *stream, int coef, int run, int coef_type, int subx, int sub_y, bool uv)
{  
	CABACContextType *cbc_enc = &sCABAC_Context;
    H264_UNUSED(cbc_enc)

//	ENC_TRACE("%d ", coef);
	//--- accumulate run-level information ---
	if (coef != 0)
	{
		ENC_ASSERT((run >= 0) && (run <= 16));
		stream->coef_write_pos += run;
		stream->coef_buffer[stream->coef_write_pos++] = coef;
		stream->coef_counter++;
	}
	else
	{
//		ENC_TRACE("\n");
		//===== encode CBP-BIT =====
		if (stream->coef_counter > 0)
		{
			CABAC_Encode_Coded_Block_Flag_Bit(mb, stream, coef_type, 1, subx, sub_y, uv);
			//===== encode significance map =====
			CABAC_Encode_SignificanceMap(stream, coef_type, stream->coef_buffer, stream->coef_counter);
			//===== encode significant coefficients =====
			CABAC_Encode_SignificantCoefficients(stream, coef_type, stream->coef_buffer, stream->coef_counter);

			memset(stream->coef_buffer, 0 , 64 * sizeof(int));
		}
		else
			CABAC_Encode_Coded_Block_Flag_Bit(mb, stream, coef_type, 0, subx, sub_y, uv);	// slice-specific function

		//--- reset counters ---
		stream->coef_write_pos = stream->coef_counter = 0;
	}
}

void CABAC_Encode_16x16_MB_data(MacroBlockType *mb, CABAC_RowStreamType *stream)
{
	// Luminance data
	uint32_t bit_flag = mb->luma_bit_flags;
	int16_t *level;
	int16_t *run;

	// only pump out luma DC for I16x16 types
	if ((gEncoderState.is_I_slice && mb->mb_type > MB_TYPE_INxN) ||
		(gEncoderState.is_P_slice && mb->mb_type > MB_TYPE_P_INxN) ||
		(gEncoderState.is_B_slice && mb->mb_type > MB_TYPE_B_INxN))
	{
#ifdef CABAC_TRACE_OUTPUT
		ENC_TRACE("16x16 DC\n");
#endif
//		ENC_TRACE("16x16 DC\n");
		// DC 
		if (bit_flag & LUMA_DC_BIT)
		{
			level = mb->data->level_run_dc[0];
			run   = mb->data->level_run_dc[1];
			for (int i = 0; i <= 16; i++)
			{
				CABAC_Encode_Coefficients(mb, stream, level[i], run[i], COEF_TYPE_LUMA_DC);

				if (level[i] == 0)
					break;
			}
		}
		else
		{
			CABAC_Encode_Coefficients(mb, stream, 0, 0, COEF_TYPE_LUMA_DC);
		}
	}

	// AC
	// encode 4x4 AC blocks in following order:
	//    00 01 04 05 
	//    02 03 06 07
	//    08 09 12 13
	//    10 11 14 15

	if (mb->coded_block_pattern_luma)
	{
		int coef_type = mb->isI16x16 ? COEF_TYPE_LUMA_AC : COEF_TYPE_LUMA_4x4;
		int sub_x, sub_y;
		int cbp_index = 0;

		for (int mb_y = 0; mb_y < 4; mb_y += 2)
		{
			for (int mb_x = 0; mb_x < 4; mb_x += 2, cbp_index++)
			{
				if (!(mb->coded_block_pattern_luma & (1 << cbp_index)))
				{
					continue;
				}
				for (int j = mb_y; j < mb_y + 2; j++)
				{              
					sub_y = (short) (j << 2);

					for (int i = mb_x; i < mb_x+2; i++)
					{
						sub_x = (short) (i << 2);
						if (bit_flag & (1L << (sub_y + i)))
						{
							level = mb->data->level_run_ac[j][i][0];
							run   = mb->data->level_run_ac[j][i][1];
#ifdef CABAC_TRACE_OUTPUT
							ENC_TRACE("16x16 AC: (%d %d)\n",sub_x, sub_y);
#endif
//							ENC_TRACE("16x16 AC: (%d %d)\n",sub_x, sub_y);

							for (int k = 0; k <= 16; k++)
							{
								CABAC_Encode_Coefficients(mb, stream, level[k], run[k], coef_type, sub_x, sub_y);
								if (level[k] == 0)
									break;
							}
						}
						else
						{
							CABAC_Encode_Coefficients(mb, stream, 0, 0, coef_type, sub_x, sub_y);
						}
					}
				}
			}
		}
	}
}

void CABAC_Encode_8x8_MB_Chroma_data(MacroBlockType *mb, CABAC_RowStreamType *stream)
{
	int16_t *level;
	int16_t *run;
	int subx, suby;
	uint32_t bit_flags = mb->chroma_bit_flags;

	// DC
	if (mb->coded_block_pattern_chroma != 0)  // check if any chroma bits in coded block pattern is set
	{
		if (bit_flags & CHROMA_DC_BIT)
		{
			for (int uv = 0; uv < 2; uv++)
			{
				level = mb->data->level_run_dc_uv[uv][0];
				run   = mb->data->level_run_dc_uv[uv][1];
#ifdef CABAC_TRACE_OUTPUT
				ENC_TRACE("DC Chroma %d\n", uv);
#endif
//				ENC_TRACE("DC Chroma %d\n", uv);

				for (int i = 0; i <= 4; i++)
				{
					CABAC_Encode_Coefficients(mb, stream, level[i], run[i], COEF_TYPE_CHROMA_DC, 0, 0, (uv == 0));
					if (level[i] == 0)
						break;
				}
			}
		}
		else
		{
			CABAC_Encode_Coefficients(mb, stream, 0, 0, COEF_TYPE_CHROMA_DC, 0, 0, 1);
			CABAC_Encode_Coefficients(mb, stream, 0, 0, COEF_TYPE_CHROMA_DC, 0, 0, 0);
		}
	}

	// AC
	// encode 4x4 blocks in 2x2 pattern u then v
	// 
	// U00 U01
	// U02 U03
	// V00 V01
	// V02 V03
	if (mb->coded_block_pattern_chroma & 2) // 
	{
		for (int uv = 0; uv < 2; uv++)
		{
			for (int x = 0; x < 4; x++)
			{
				subx = x & 1;
				suby = x >> 1;

				if (bit_flags & (1L << ((uv * 4) + x)))
				{
					level = mb->data->level_run_ac_uv[uv][suby][subx][0];
					run   = mb->data->level_run_ac_uv[uv][suby][subx][1];
#ifdef CABAC_TRACE_OUTPUT
					ENC_TRACE("AC Chroma %d (%d %d)\n", uv, subx, suby);
#endif
//					ENC_TRACE("AC Chroma %d (%d %d)\n", uv, subx, suby);

					for (int i = 0; i < 16; i++)
					{
						CABAC_Encode_Coefficients(mb, stream, level[i], run[i], COEF_TYPE_CHROMA_AC, subx << 2, suby << 2, (uv == 0));
						if (level[i] == 0)
							break;
					}
				}
				else
					CABAC_Encode_Coefficients(mb, stream, 0, 0, COEF_TYPE_CHROMA_AC, subx << 2, suby << 2, (uv == 0));
			}
		}
	}
}

void CABAC_Encode_DeltaQP(MacroBlockType *mb, CABAC_RowStreamType *stream)
{
	// base ctxIdx for mb_qp_delta is 60..63 from Table 9-11
	// clause 9.3.2.7 for mb_qp_delta (Table 9-34) maxBinIdxCtx = 2, ctxIdxOffset = 60
	// unary binarization is used with Table 9-3
	// clause 9.3.3.1.1.5 - derivation of ctxIdxInc for mp_qp_delta
#if 1	// off for now until multi-threading bug is fixed
	int delta_qp = mb->qp_delta;
	int ctx_idx_offset = 0;
	ctx_idx_offset = (mb->prev_qp_delta != 0) ? 1 : 0;

	if (delta_qp == 0)
	{
		stream->func->encode_bit (stream, 0, 60 + ctx_idx_offset);
	}
	else
	{
		int sign    = (delta_qp <= 0) ? 0 : -1;
		int act_sym = (ABS(delta_qp) << 1) + sign;

		stream->func->encode_bit (stream, 1, 60 + ctx_idx_offset);

		CABAC_Encode_unary_exp_golomb(stream, act_sym - 1, 62, 1);
	}
#else
	stream->func->encode_bit (stream, 0, 60);
#endif
}

void CABAC_Encode_RefIdx(MacroBlockType *mb, CABAC_RowStreamType *stream)
{
	CABACContextType *cbc_enc = &sCABAC_Context;
    H264_UNUSED(cbc_enc)

	int ctx_idx = 54;
	int ctx_idx_inc = 0;

	if (mb->mbAddrA && mb->mbAddrA->data->refIdxL0 > 0)
		ctx_idx_inc = 1;
	if (mb->mbAddrB && mb->mbAddrB->data->refIdxL0 > 0)
		ctx_idx_inc += 2;

	int value = mb->data->refIdxL0;

	if (value == 0)
	{
		stream->func->encode_bit(stream, 0, ctx_idx + ctx_idx_inc);
	}
	else
	{
		stream->func->encode_bit(stream, 1, ctx_idx + ctx_idx_inc);
		value--;
		ctx_idx_inc = 4;
		CABAC_Encode_unary_exp_golomb(stream, value, ctx_idx + ctx_idx_inc, 1);
	}
}

void CABAC_Encode_MVD(MacroBlockType *mb, CABAC_RowStreamType *stream)
{
	CABACContextType *cbc_enc = &sCABAC_Context;
    H264_UNUSED(cbc_enc)

	int ctxIdx[2] = { 40, 47 };  // range 40..46 for x / 47..53 for y
	int x, y;
	int mv_absMvdComp[2] = { 0, 0};

	int mv[2];
	mv[0] = mb->data->mvd_l0[0][0][0];
	mv[1] = mb->data->mvd_l0[1][0][0];

	// more crazy rules from 9.3.3.1.1.7 - applies to mvd_l0 and mvd_l1
	// if mbAddrN is not avail, a SKIP mb, intra pred mb, or preModeEqualFlagN == 0, then absMvdCompN = 0
	// don't need to check for skip or intra pred as the value grabbed from the mb will be 0 anyway
	x = y = 0;
	MacroBlockType *mb_a = HasAdjacent4x4SubBlock(mb, ADJACENT_MB_LEFT, &x, &y, true);
	if (mb_a && AVAILABLE(mb_a) && !mb_a->mb_skip_flag && !mb_a->mb_direct_flag && (mb->mbAddrA->mb_type < MB_TYPE_P_INxN))
	{
		mv_absMvdComp[0] = ABS(mb_a->data->mvd_l0[0][x][y]);	// later this will come from an array of partitions from mb_a
		mv_absMvdComp[1] = ABS(mb_a->data->mvd_l0[1][x][y]);
	}

	x = y = 0;
	MacroBlockType *mb_b = HasAdjacent4x4SubBlock(mb, ADJACENT_MB_ABOVE, &x, &y, true);
	if (mb_b && AVAILABLE(mb_b) && !mb_b->mb_skip_flag && !mb_b->mb_direct_flag && (mb->mbAddrB->mb_type < MB_TYPE_P_INxN))
	{
		mv_absMvdComp[0] += ABS(mb_b->data->mvd_l0[0][x][y]);	// later this will come from an array of partitions from mb_b
		mv_absMvdComp[1] += ABS(mb_b->data->mvd_l0[1][x][y]);
	}

//	ENC_TRACE(">>>> MVD: mb:%d (%d %d) prev(%d %d)\n", mb->mb_index, mv[0], mv[1], pmv_x, pmv_y);

	for (int i = 0; i < 2; i++)
	{
		int ctxIdxInc;

		if (mv_absMvdComp[i] < 3)
			ctxIdxInc = 0;
		else
		if (mv_absMvdComp[i] > 32)
			ctxIdxInc = 2;
		else
			ctxIdxInc = 1;
#ifdef CABAC_TRACE_OUTPUT
		ENC_TRACE(">>>> MVD: mb:%d : %d  ctxIdxInc:%d  pmv_x:%d pmv_y:%d\n", mb->mb_index, mv[i], ctxIdxInc, pmv_x, pmv_y);
#endif

		if (mv[i])
		{
			stream->func->encode_bit(stream, 1, ctxIdx[i] + ctxIdxInc);	// non-zero

			CABAC_Encode_unary_exp_golomb_MV(stream, (ABS(mv[i]) - 1), ctxIdx[i]+3, 3);	// ueg3 encode mvd-1

			stream->func->encode_eq_bit(stream, (mv[i] < 0));	// sign bit
		}
		else
		{
			stream->func->encode_bit(stream, 0, ctxIdx[i] + ctxIdxInc);
		}
	}
}

void CABAC_Encode_MVD_l1(MacroBlockType *mb, CABAC_RowStreamType *stream)
{
	CABACContextType *cbc_enc = &sCABAC_Context;
    H264_UNUSED(cbc_enc)

	int ctxIdx[2] = { 40, 47 };  // range 40..46 for x / 47..53 for y
	int x, y;
	int mv_absMvdComp[2] = { 0, 0};

	int mv[2];
	mv[0] = mb->data->mvd_l1[0][0][0];
	mv[1] = mb->data->mvd_l1[1][0][0];

	// more crazy rules from 9.3.3.1.1.7 - applies to mvd_l0 and mvd_l1
	// if mbAddrN is not avail, a SKIP mb, intra pred mb, or preModeEqualFlagN == 0, then absMvdCompN = 0
	// don't need to check for skip or intra pred as the value grabbed from the mb will be 0 anyway
	x = y = 0;
	MacroBlockType *mb_a = HasAdjacent4x4SubBlock(mb, ADJACENT_MB_LEFT, &x, &y, true);
	if (mb_a && AVAILABLE(mb_a) && !mb_a->mb_skip_flag && !mb_a->mb_direct_flag && (mb->mbAddrA->mb_type < MB_TYPE_P_INxN))
	{
		mv_absMvdComp[0] = ABS(mb_a->data->mvd_l1[0][x][y]);	// later this will come from an array of partitions from mb_a
		mv_absMvdComp[1] = ABS(mb_a->data->mvd_l1[1][x][y]);
	}

	x = y = 0;
	MacroBlockType *mb_b = HasAdjacent4x4SubBlock(mb, ADJACENT_MB_ABOVE, &x, &y, true);
	if (mb_b && AVAILABLE(mb_b) && !mb_b->mb_skip_flag && !mb_b->mb_direct_flag && (mb->mbAddrB->mb_type < MB_TYPE_P_INxN))
	{
		mv_absMvdComp[0] += ABS(mb_b->data->mvd_l1[0][x][y]);	// later this will come from an array of partitions from mb_b
		mv_absMvdComp[1] += ABS(mb_b->data->mvd_l1[1][x][y]);
	}

	//	ENC_TRACE(">>>> MVD: mb:%d (%d %d) prev(%d %d)\n", mb->mb_index, mv[0], mv[1], pmv_x, pmv_y);

	for (int i = 0; i < 2; i++)
	{
		int ctxIdxInc;

		if (mv_absMvdComp[i] < 3)
			ctxIdxInc = 0;
		else
			if (mv_absMvdComp[i] > 32)
				ctxIdxInc = 2;
			else
				ctxIdxInc = 1;
#ifdef CABAC_TRACE_OUTPUT
		ENC_TRACE(">>>> MVD: mb:%d : %d  ctxIdxInc:%d  pmv_x:%d pmv_y:%d\n", mb->mb_index, mv[i], ctxIdxInc, pmv_x, pmv_y);
#endif

		if (mv[i])
		{
			stream->func->encode_bit(stream, 1, ctxIdx[i] + ctxIdxInc);	// non-zero

			CABAC_Encode_unary_exp_golomb_MV(stream, (ABS(mv[i]) - 1), ctxIdx[i]+3, 3);	// ueg3 encode mvd-1

			stream->func->encode_eq_bit(stream, (mv[i] < 0));	// sign bit
		}
		else
		{
			stream->func->encode_bit(stream, 0, ctxIdx[i] + ctxIdxInc);
		}
	}
}

void CABAC_Encode_Coded_Block_Pattern(MacroBlockType *mb, CABAC_RowStreamType *stream)
{
	int ctxIdx_luma = 73;	// range 73..76 (all slices) from table 9-11
	int ctxIdx_chroma = 77;	// range 77..84 (all slices) from table 9-11
	// Table 9-34 max bits for prefix: 3 suffix: 1   ctxIdxOffset is prefix: 73 suffix: 77
	// clause 9.3.2.6 - cbp luma is prefix using FL binarization (cMax = 15 4-bits) and for ChromaArrayType == 1 or 2, cbp chroma is suffix using TU binarization (cMax = 2 2-bits)
	// crazy binarization rules in clause 9.3.3.1.1.4 for luma and chroma
	// we don't support I_PCM so for now we don't check neighbors for it
	int ctx_idx_inc[4] = { 0, 0, 0, 0 };
	int ctx_idx_inc_chroma[2] = { 0, 4};
	// luma scan order: 0 1
	// of 8x8 blocks    2 3
	// correspond to luma bits 3210

	// for now, assume 16x16 so all 4 luma bits are all 0 or all 1 (0xf)
	if (mb->mbAddrA)
	{
		if (!(mb->mbAddrA->coded_block_pattern_luma & 0x2))	// lsb 2 is block 1
			ctx_idx_inc[0] = 1;
		if (!(mb->mbAddrA->coded_block_pattern_luma & 0x8))	// top bit is block 3
			ctx_idx_inc[2] = 1;

		if (mb->mbAddrA->coded_block_pattern_chroma != 0)
		{
			ctx_idx_inc_chroma[0] = 1;
			if (mb->mbAddrA->coded_block_pattern_chroma == 2)
				ctx_idx_inc_chroma[1] += 1;
		}
	}
	// crazy sub-rule for luma with neighboring block equal to current block (i.e. left blocks 0 & 2 and upper blocks 0 & 1)
	// PRIOR decoded bin must be 0 to set the condTermFlagN unless the prior bin is 0
	if (!(mb->coded_block_pattern_luma & 0x1))	// lsb is block 0
		ctx_idx_inc[1] = 1;
	if (!(mb->coded_block_pattern_luma & 0x4))	// msb 2 is block 2
		ctx_idx_inc[3] = 1;

	if (mb->mbAddrB)
	{
		if (!(mb->mbAddrB->coded_block_pattern_luma & 0x8))	// top bit is block 3
			ctx_idx_inc[1] += 2;
		if (!(mb->mbAddrB->coded_block_pattern_luma & 0x4))	// msb 2 is block 2
			ctx_idx_inc[0] += 2;

		if (mb->mbAddrB->coded_block_pattern_chroma != 0)
		{
			ctx_idx_inc_chroma[0] += 2;
			if (mb->mbAddrB->coded_block_pattern_chroma == 2)
				ctx_idx_inc_chroma[1] += 2;
		}
	}
	if (!(mb->coded_block_pattern_luma & 0x2))	// lsb 2 is block 1
		ctx_idx_inc[3] += 2;
	if (!(mb->coded_block_pattern_luma & 0x1))	// lsb is block 0
		ctx_idx_inc[2] += 2;

	for (int i = 0; i < 4; i++)
		stream->func->encode_bit(stream, mb->coded_block_pattern_luma >> i, ctxIdx_luma + ctx_idx_inc[i]);

	// now for chroma
	stream->func->encode_bit(stream, (mb->coded_block_pattern_chroma != 0), ctxIdx_chroma + ctx_idx_inc_chroma[0]);
	if (mb->coded_block_pattern_chroma != 0)
		stream->func->encode_bit(stream, (mb->coded_block_pattern_chroma == 2), ctxIdx_chroma + ctx_idx_inc_chroma[1]);
}

void CABAC_Encode_MB_Layer(MacroBlockType *mb)
{
	CABAC_RowStreamType *stream = &sCABAC_Context.mb_row_stream[mb->mb_y];

	if ((mb->mb_x != 0) || (mb->mb_y != 0))
		stream->func->encode_finalbit(stream, 0);

	// we check these too much to keep doing both checks
	mb->mbAddrA = (mb->mbAddrA && AVAILABLE(mb->mbAddrA)) ? (mb->mbAddrA) : NULL;
	mb->mbAddrB = (mb->mbAddrB && AVAILABLE(mb->mbAddrB)) ? (mb->mbAddrB) : NULL;
	mb->mbAddrC = (mb->mbAddrC && AVAILABLE(mb->mbAddrC)) ? (mb->mbAddrC) : NULL;
	mb->mbAddrD = (mb->mbAddrD && AVAILABLE(mb->mbAddrD)) ? (mb->mbAddrD) : NULL;

	// for P, SP, or B slices
	if (gEncoderState.is_P_slice || gEncoderState.is_B_slice)
	{
		int ctxIdxInc = 0;
		int ctxIdxOffset = 11;
		if (gEncoderState.slice_type == SLICE_TYPE_B)
		{
			ctxIdxOffset = 24;
		}

		if (mb->mbAddrA && !mb->mbAddrA->mb_skip_flag)
			ctxIdxInc += 1;
		if (mb->mbAddrB && !mb->mbAddrB->mb_skip_flag)
			ctxIdxInc += 1;

		stream->func->encode_bit(stream, mb->mb_skip_flag, ctxIdxOffset + ctxIdxInc); 	// or BSKIP

		if (mb->mb_skip_flag)
			return;
	}

	bool output_chroma_pred_mode = true;
	bool output_coded_block_pattern = false;
	bool output_mvd = false;

	if (gEncoderState.is_P_slice)
	{
		CABAC_Encode_mb_type_P(mb, stream);
		if (mb->mb_type < MB_TYPE_P_INxN)
		{
			output_chroma_pred_mode = false;
			output_mvd = true;
		}
		if (mb->mb_type <= MB_TYPE_P_INxN)
			output_coded_block_pattern = true;
	}
	else
	if (gEncoderState.is_B_slice)
	{
		CABAC_Encode_mb_type_B(mb, stream);
		if (mb->mb_type < MB_TYPE_B_INxN)
		{
			output_chroma_pred_mode = false;
			output_mvd = mb->mb_type != MB_TYPE_B_DIRECT_16x16;
		}
		if (mb->mb_type <= MB_TYPE_B_INxN)
			output_coded_block_pattern = true;
	}
	else
	if (gEncoderState.is_I_slice)
	{
		CABAC_Encode_mb_type_I(mb, stream);
		if (mb->mb_type == MB_TYPE_INxN)
			output_coded_block_pattern = true;
	}

	if (output_chroma_pred_mode)
	{
		// mb pred type (clause 7.3.5.1)
		CABAC_Encode_intra_chroma_pred_mode(mb, stream, mb->chroma_pred_mode);
	}

	if (output_mvd)
	{
		// if in more than one ref pic, then add ref list output (currently we just support P16x16 now)
		if (gPPS_Parameters.num_ref_idx_l0_default_active_minus1 > 0)
			CABAC_Encode_RefIdx(mb, stream);

		if (mb->data->refIdxL0 != -1)
			CABAC_Encode_MVD(mb, stream);
		if (mb->data->refIdxL1 != -1)
			CABAC_Encode_MVD_l1(mb, stream);
	}

	// encode coded_block_pattern if not I16x16 type
	if (output_coded_block_pattern)
	{
		CABAC_Encode_Coded_Block_Pattern(mb, stream);
		if ((mb->coded_block_pattern_luma == 0) && (mb->coded_block_pattern_chroma == 0))
		{
			return;	// done
		}
	}

	// encode delta_qp
	CABAC_Encode_DeltaQP(mb, stream);

	stream->func->start_coeff_tracking(stream);
//	int save_start = stream->coeff_bits;

	// encode luma coefficient values
	CABAC_Encode_16x16_MB_data(mb, stream);

	// Chroma data (u and v)
	CABAC_Encode_8x8_MB_Chroma_data(mb, stream);

	stream->func->end_coeff_tracking(stream);

#if 0	// for testing accuracy of coeff_cost estimate
	int coeff_bits = stream->coeff_bits - save_start;

	if (!gEncoderState.idr)
	{
		static int total_diff = 0;
		static int diff_count = 0;
		int diff = coeff_bits - (mb->coeff_cost*0.21f);

		total_diff += diff;
		diff_count++;
		if ((diff_count % 80) == 0)
		{
			ENC_TRACE("%d %d %7.3f\n", diff_count, total_diff, (float) total_diff / (float) diff_count);
		}
	}
#endif
}


typedef struct 
{
	int first_mb_in_slice;
	int slice_type;
	int pic_parameter_set_id;
	int colour_plane_id;
	int frame_num;
	int field_pic_flag;
	int bottom_field_flag;
	int idr_pic_id;
	int pic_order_cnt_lsb;
	int delta_pic_order_cnt_bottom;
	int delta_pic_order_cnt[2];
	int redundant_pic_cnt;
	int direct_spatial_mv_pred_flag;
	int num_ref_idx_active_override_flag;
	int num_ref_idx_l0_active_minus1;
	int num_ref_idx_l1_active_minus1;
	int ref_pic_list_modification_flag_l0;
	bool entropy_coding_mode_flag;
	int cabac_init_idc;
	int slice_qp_delta;
	int sp_for_switch_flag;
	int slice_qs_delta;
	int disable_deblocking_filter_idc;
	int slice_alpha_c0_offset_div2;
	int slice_beta_offset_div2;
	int slice_group_change_cycle;
} SliceHeaderType;


void WriteSliceHeader(BitStreamType *bs, SliceHeaderType *header)
{
	WriteBits_ue(bs, static_cast<uint16_t>(header->first_mb_in_slice));
	WriteBits_ue(bs, static_cast<uint16_t>(header->slice_type));
	WriteBits_ue(bs, static_cast<uint16_t>(header->pic_parameter_set_id));
	/* if (separate_colour_plan_flag == 1)
			s_header.colour_plane_id = ReadBits_u(&rbsp_bs, 2); */
	WriteBits(bs, header->frame_num, 4);
	/* if (!frame_mbs_only_flag) {
		s_header.field_pic_flag = ReadBits_u(&rbsp_bs, 1); 
		if (field_pic_flag)
			s_header.bottom_field_flag = ReadBits_u(&rbsp_bs, 1); }*/
	/* if (IdrPicFlag) */ // if NAL unit type == 5 (IDR)
	if (gEncoderState.idr)
		WriteBits_ue(bs, static_cast<uint16_t>(header->idr_pic_id));
	/* if( pic_order_cnt_type = = 0 ) { */
	if (gSPS_Parameters.pic_order_cnt_type == 0)	// pic_order_cnt_type = 0 for b slice turned on
	{
		WriteBits(bs, header->pic_order_cnt_lsb, gSPS_Parameters.log2_max_pic_order_cnt_lsb_minus4 + 4);	// display order (index * 2) % (2^(log2_max_pic_order_cnt_lsb_minus4+4))
	}
		/*if( bottom_field_pic_order_in_frame_present_flag && !field_pic_flag )
			s_header.delta_pic_order_cnt_bottom = ReadBits_se(&rbsp_bs); } */
	/* if( pic_order_cnt_type = = 1 && !delta_pic_order_always_zero_flag ) {
		s_header.delta_pic_order_cnt[0] = ReadBits_se(&rbsp_bs);
		if( bottom_field_pic_order_in_frame_present_flag && !field_pic_flag )
			s_header.delta_pic_order_cnt[1] = ReadBits_se(&rbsp_bs); } */
	/* if( redundant_pic_cnt_present_flag )
		s_header.redundant_pic_cnt = ReadBits_ue(&rbsp_bs); */
	/* if( slice_type = = B )
			s_header.direct_spatial_mv_pred_flag = ReadBits_u(&rbsp_bs, 2); */
	if (header->slice_type == SLICE_TYPE_B)
	{
		WriteBits_ue(bs, static_cast<uint16_t>(header->direct_spatial_mv_pred_flag));
	}

    if ((header->slice_type == SLICE_TYPE_P) || (header->slice_type == SLICE_TYPE_B))
	{
	/* if( slice_type = = P | | slice_type = = SP | | slice_type = = B ) {
			s_header.num_ref_idx_active_override_flag = ReadBits_u(&rbsp_bs, 2);
			if( num_ref_idx_active_override_flag ) {
				s_header.num_ref_idx_l0_active_minus1 = ReadBits_ue(&rbsp_bs);
				if( slice_type = = B )
					s_header.num_ref_idx_l1_active_minus1 = ReadBits_ue(&rbsp_bs); } } */
		WriteBits(bs, 0, 1);	// num_ref_idx_active_override_flag
	}
	/* if( nal_unit_type = = 20 | | nal_unit_type = = 21 )
			ref_pic_list_mvc_modification() 
		else */
	/*ref_pic_list_modification()  */
	if (header->slice_type != SLICE_TYPE_I && header->slice_type != SLICE_TYPE_SI) // && header->slice_type != 2 && header->slice_type != 4
	{
		WriteBits(bs, header->ref_pic_list_modification_flag_l0, 1);	// ref_pic_list_modification_flag_l0

		// if (ref_pic_list_modification_flag_10)
		if (header->ref_pic_list_modification_flag_l0)
		{	// need to check for multiple B frames so that P frame will not use the reference B frame as its prev_ref_frame
			// { do {
			//      modification_of_pic_nums_idc
			WriteBits_ue(bs, 0);	// modification_of_pic_nums_idc - to add an offset to the pic num
			//      if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1)
			WriteBits_ue(bs, 1);	// abs_diff_pic_num_minus1 - to add an offset to the pic num (+2 to poc)
			//      else if (modification_of_pic_nums_idc == 2)
			//				long_term_pic_num
			// } while (modification_of_pic_nums_idc != 3)
			//
			WriteBits_ue(bs, 3);	// modification_of_pic_nums_idc - 3 to break out of do/while loop
		}
	}
	if (header->slice_type == SLICE_TYPE_B)	// if (slice_type == 1)
	{
		WriteBits(bs, 0, 1);	// ref_pic_list_modification_flag_l1
	}

	//if((weighted_pred_flag && (slice_type == P || slice_type == SP) ) || (weighted_bipred_idc == 1 && slice_type == B))/*false*/

	// when NAL reference_idc (priority) is non-zero
	/* if (nal_ref_idc != 0)
			dec_ref_pic_marking();
				if (IdrPicFlag) */
	if (gEncoderState.nal_ref_idc != 0)
	{	// dec_ref_pic_marking() - only for reference frames IDR or P
		if (gEncoderState.idr)
		{
			WriteBits(bs, 0, 1);	// no_output_of_prior_pics_flag
			WriteBits(bs, 0, 1);	// long_term_reference_flag
		}
		else
		{	// when NAL reference_idc (priority) is non-zero
			WriteBits(bs, 0, 1);	// adaptive_ref_pic_marking_mode_flag
		}
	}

	if( gEncoderState.entropy_coding_mode_flag && (header->slice_type != SLICE_TYPE_I) && (header->slice_type != SLICE_TYPE_SI)) 
		WriteBits_ue(bs, static_cast<uint16_t>(gEncoderState.cabac_init_idc));
	/* s_header.cabac_init_idc = ReadBits_ue(&rbsp_bs);*/
	WriteBits_se(bs, static_cast<int16_t>(header->slice_qp_delta));
	/* if( slice_type = = SP | | slice_type = = SI ) {
			if( slice_type = = SP )
				s_header.sp_for_switch_flag = ReadBits_u(&rbsp_bs, 2);
			s_header.slice_qs_delta = ReadBits_se(&rbsp_bs); } */
	/* if( deblocking_filter_control_present_flag ) { */
			WriteBits_ue(bs, static_cast<uint16_t>(header->disable_deblocking_filter_idc));
			if( header->disable_deblocking_filter_idc != 1 ) 
			{
				WriteBits_se(bs, static_cast<int16_t>(header->slice_alpha_c0_offset_div2));
				WriteBits_se(bs, static_cast<int16_t>(header->slice_beta_offset_div2));
			}
	/* if( num_slice_groups_minus1 > 0 && slice_group_map_type >= 3 && slice_group_map_type <= 5)
	s_header.slice_group_change_cycle;*/
#if 1
	// byte align
    while(bs->bits_rem != 8)
		WriteBits(bs, 1, 1);
#endif
}


void CABAC_StartSlice(int slice_type)
{
	int cabac_type = 0;
	if ((slice_type == SLICE_TYPE_I) || (slice_type == SLICE_TYPE_SI))
		cabac_type = 0;
	else
		cabac_type = gEncoderState.cabac_init_idc + 1;	//???

	CABAC_ContextInit(&sCABAC_Context, cabac_type, gEncoderState.qp + gEncoderState.current_qp_delta);

	BitStreamType bs;
	// write Slice header before sending bitstream buffer to cabac encoding
	InitBitStream(&bs, sCABAC_Context.bitstream);

	SliceHeaderType slice_header;
	memset(&slice_header, 0, sizeof(SliceHeaderType));
	slice_header.slice_type = slice_type;
	slice_header.slice_qp_delta = gEncoderState.current_qp_delta;
	slice_header.frame_num = gEncoderState.current_frame_num;
	slice_header.idr_pic_id = gEncoderState.idr_pic_id;
	slice_header.entropy_coding_mode_flag = true;
	slice_header.cabac_init_idc = 0;

	slice_header.disable_deblocking_filter_idc = gEncoderState.current_frame->in_loop_filter_off;
	slice_header.slice_alpha_c0_offset_div2 = gEncoderState.current_frame->slice_alpha_c0_offset >> 1;
	slice_header.slice_beta_offset_div2 = gEncoderState.current_frame->slice_beta_offset >> 1;
	slice_header.pic_order_cnt_lsb = (gEncoderState.current_frame->pic_order_cnt * 2) % (1 << (gSPS_Parameters.log2_max_pic_order_cnt_lsb_minus4 + 4));	// currently only used when b-slice is on
	// TODO: when more than 2 b-slices are in a group the order might not be correct here.
	slice_header.ref_pic_list_modification_flag_l0 = 0;
	if (gEncoderState.b_slice_frames > 1)
	{
		if ((slice_type == SLICE_TYPE_P) && (gEncoderState.current_frame->prev_ref_frame->slice_type != SLICE_TYPE_I))
		{
			slice_header.ref_pic_list_modification_flag_l0 = 1;	// set to bypass B slice reference frame
		}
	}

	WriteSliceHeader(&bs, &slice_header);

	CABAC_Start_EncodingStream(&sCABAC_Context, sCABAC_Context.bitstream, CABAC_STREAM_MAX_SIZE, bs.byte_pos);
	CABAC_Reset_EC(&sCABAC_Context);
#ifdef DEBUG_CABAC_OUTPUT
	sDebugCABAC_Index = 0;
#endif
}

void CABAC_EndSlice()
{
	CABAC_Encode_FinalBit_Output(&sCABAC_Context.mb_row_stream[gEncoderState.mb_height-1], 1);	// write terminating bit

	CABAC_Stop_EncodingStream(&sCABAC_Context);

	CABAC_ContextExit(&sCABAC_Context);

	// calc frame bits and coeff bits
	gEncoderState.current_frame->total_encoded_bits = CABAC_Encode_BitsWritten(&sCABAC_Context);

	int coeff_bits = 0;
	for (int i = 0; i < gEncoderState.mb_height; i++)
	{
		coeff_bits += sCABAC_Context.mb_row_stream[i].coeff_bits;
		sCABAC_Context.mb_row_stream[i].coeff_bits = 0;	// reset
	}

	gEncoderState.current_frame->total_coeff_bits = coeff_bits;

#ifdef DEBUG_CABAC_OUTPUT
	if (gEncoderState.total_frame_num == 2)
	{
		ENC_TRACE("\n\n----------------------------------------------------------\n\n")
			ENC_TRACE("static const int sDebugCABAC_Size = %d;\n", sDebugCABAC_Index);
		ENC_TRACE("static const DebugCABACType sDebugCABAC_Buffer[%d] = {\n", sDebugCABAC_Index);
		for (int i = 0; i < sDebugCABAC_Index; i++)
		{
			ENC_TRACE("0x%08x, ", sDebugCABAC_Buffer[i].value);
			if (i && (i % 64)== 0)
				ENC_TRACE("\n");
		}
		ENC_TRACE("\n}; \n\n----------------------------------------------------------\n\n")
	}

#endif

}

#pragma warning(pop)