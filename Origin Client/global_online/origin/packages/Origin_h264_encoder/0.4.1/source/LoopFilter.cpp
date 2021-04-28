#include "LoopFilter.h"
#include "Origin_h264_encoder.h"
#include "MacroBlock.h"
#include "Frame.h"
#include "Tables.h"
#include "Trace.h"

#ifdef USE_SSE2
#include <emmintrin.h>
#endif

// use LOPS = 1 and LOPS_UV = 1 to use just left and top edges for 16x16 blocks as described in 
// a paper titled, "A Novel Deblocking Filter Algorithm In H.264 for Real Time Implementation" by Yuan Li, Ning Han, & Chen Chen 2009
#define LOPS	4	// 4 for full luma filtering
#define LOPS_UV 2	// 2 for full chroma filtering

// from Table 8-16
static const uint8_t Alpha[52]  = {0,0,0,0,0,0, // qp: 0-5
								   0,0,0,0,0,0, // qp: 6-11
								   0,0,0,0,4,4,	// qp: 12-17
								   5,6,7,8,9,10,		// qp: 18-23
								   12,13,15,17,20,22,	// qp: 24-29
								   25,28,32,36,40,45,	// qp: 30-35
								   50,56,63,71,80,90,   // qp: 36-41
								   101,113,127,144,162,182, // qp: 42-47
								   203,226,255,255			// qp: 48-51

} ;

static const uint8_t  Beta[52]  = { 0,0,0,0,0,0, // qp: 0-5
									0,0,0,0,0,0, // qp: 6-11
									0,0,0,0,2,2,	// qp: 12-17
									2,3,3,3,3,4,	// qp: 18-23
									4,4,6,6,7,7,	// qp: 24-29
									8,8,9,9,10,10,	// qp: 30-35
									11,11,12,12,13,13,  // qp: 36-41
									14,14,15,15,16,16,	// qp: 42-47
									17,17,18,18			// qp: 48-51
};

// from Table 8-17
static const uint8_t Clip_Table[52][5]  =
{
	{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},
	{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},
	{ 0, 0, 0, 0, 0},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 1, 1, 1},{ 0, 0, 1, 1, 1},{ 0, 1, 1, 1, 1},
	{ 0, 1, 1, 1, 1},{ 0, 1, 1, 1, 1},{ 0, 1, 1, 1, 1},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 2, 3, 3},
	{ 0, 1, 2, 3, 3},{ 0, 2, 2, 3, 3},{ 0, 2, 2, 4, 4},{ 0, 2, 3, 4, 4},{ 0, 2, 3, 4, 4},{ 0, 3, 3, 5, 5},{ 0, 3, 4, 6, 6},{ 0, 3, 4, 6, 6},
	{ 0, 4, 5, 7, 7},{ 0, 4, 5, 8, 8},{ 0, 4, 6, 9, 9},{ 0, 5, 7,10,10},{ 0, 6, 8,11,11},{ 0, 6, 8,13,13},{ 0, 7,10,14,14},{ 0, 8,11,16,16},
	{ 0, 9,12,18,18},{ 0,10,13,20,20},{ 0,11,15,23,23},{ 0,13,17,25,25}
} ;

#define FILTER_BUFFER_STRIDE 32

typedef struct  
{
	uint8_t *y_buffer;
	uint8_t *uv_buffer;
	int indexA[2];
	int alpha[2];
	int beta[2];
	int strength_h[4][4];
	int strength_v[4][4];
} FilterParamType;

// currently only supporting 16x16 mbs so we are only calculating the edge strength for the 4 vertical boundaries
bool CalcEdgeStrength (MacroBlockType *mb, bool vert, int edge_strength[4][4])
{
	MacroBlock *neighbor = (vert) ? mb->mbAddrA : mb->mbAddrB;
	bool has_strength = false;

	if ((mb->isI16x16) || (neighbor && neighbor->isI16x16))	// if p or q are I blocks
	{
		for (int i = 0; i < 4; i++)
		{
			edge_strength[0][i] = (neighbor) ? 4 : 0;
			edge_strength[1][i] = 3;
			edge_strength[2][i] = 3;
			edge_strength[3][i] = 3;
		}
		return true;
	}

	// 
	if ((mb->coded_block_pattern_luma == 0) && (mb->coded_block_pattern_chroma == 0)) // (mb->luma_bit_flags == 0)
	{
		if (neighbor)
		{
			int same_mv = ((ABS(mb->data->mv_l0[0][0][0] - neighbor->data->mv_l0[0][0][0]) < 4) && (ABS(mb->data->mv_l0[1][0][0] - neighbor->data->mv_l0[1][0][0]) < 4)) ? 0 : 1;

			has_strength |= (same_mv != 0);

			if (neighbor->luma_bit_flags == 0)
			{
				edge_strength[0][0] = edge_strength[0][1] = edge_strength[0][2] = edge_strength[0][3] = same_mv;	// if the same (assumes same ref0)
			}
			else
			{	// has coef
				if (vert)
				{
					for (int i = 0; i < 16; i += 4)
					{
						if (neighbor->luma_bit_flags & (0x8 << i))
						{
							edge_strength[0][i>>2] = 2;
							has_strength = true;
						}
						else
							edge_strength[0][i>>2] = same_mv;
					}
				}
				else
				{
					for (int i = 0; i < 4; i++)
					{
						if (neighbor->luma_bit_flags & (1 << (12+i)))
						{
							edge_strength[0][i] = 2;
							has_strength = true;
						}
						else
							edge_strength[0][i] = same_mv;
					}
				}
			}
		}
		else
		{
			edge_strength[0][0] = edge_strength[0][1] = edge_strength[0][2] = edge_strength[0][3] = 0;	// frame border so no filtering
		}

		// rest are 0 zero because internally (edges 1-3) there are no coefficients and the mvs will be the same
		edge_strength[1][0] = edge_strength[1][1] = edge_strength[1][2] = edge_strength[1][3] = 0;	// 
		edge_strength[2][0] = edge_strength[2][1] = edge_strength[2][2] = edge_strength[2][3] = 0;	// 
		edge_strength[3][0] = edge_strength[3][1] = edge_strength[3][2] = edge_strength[3][3] = 0;	// 
	}
	else
	{
		if (neighbor)
		{
			int same_mv = ((ABS(mb->data->mv_l0[0][0][0] - neighbor->data->mv_l0[0][0][0]) < 4) && (ABS(mb->data->mv_l0[1][0][0] - neighbor->data->mv_l0[1][0][0]) < 4)) ? 0 : 1;
			has_strength |= (same_mv != 0);

			if (vert)
			{
				for (int i = 0; i < 16; i += 4)
				{
					if ((neighbor->luma_bit_flags & (0x8 << i)) || (mb->luma_bit_flags & (1L << i)))
					{
						edge_strength[0][i>>2] = 2;
						has_strength = true;
					}
					else
						edge_strength[0][i>>2] = same_mv;
				}
			}
			else
			{
				for (int i = 0; i < 4; i++)
				{
					if ((neighbor->luma_bit_flags & (1 << (12+i))) || (mb->luma_bit_flags & (1 << i)))
					{
						edge_strength[0][i] = 2;
						has_strength = true;
					}
					else
						edge_strength[0][i] = same_mv;
				}
			}
		}
		else
		{
			edge_strength[0][0] = edge_strength[0][1] = edge_strength[0][2] = edge_strength[0][3] = 0;	// frame border so no filtering
		}

		for (int j = 1; j < 4; j++ )
		{
			if (vert)
			{
				for (int i = 0; i < 16; i += 4)
				{
					if ((mb->luma_bit_flags & (1L << (i+(j-1))) || (mb->luma_bit_flags & (1L << (i+j)))))
					{
						edge_strength[j][i>>2] = 2;
						has_strength = true;
					}
					else
						edge_strength[j][i>>2] = 0;	// assumes internal edges have same mv
				}
			}
			else
			{
				for (int i = 0; i < 4; i++)
				{
					if ((mb->luma_bit_flags & (1L << (i+((j-1)<<2)))) || (mb->luma_bit_flags & (1L << (i+((j<<2))))))
					{
						edge_strength[j][i] = 2;
						has_strength = true;
					}
					else
						edge_strength[j][i] = 0;	// assumes internal edges have same mv
				}
			}
		}
	}

	return has_strength;
}

// offsets from rp to 
#define	P0 3
#define	Q0 4
#define	P1 2
#define	Q1 5
#define	P2 1
#define	Q2 6
#define	P3 0
#define	Q3 7

#define LF_ABS(x) (midABS_Table[(x)])

void ApplyFilter_Luma_V(FilterParamType *params)
{
	int16_t *midABS_Table = &gABS_Table[1<<15];

	uint8_t *fp = params->y_buffer;

	for (int y = 0; y < 16; y++, fp += FILTER_BUFFER_STRIDE)
	{
		uint8_t *rp = fp - 4;

		for (int x = 0, i = 0; x < LOPS; x++, rp += 4, i = 1)
		{
			int alpha  = params->alpha[i];
			int beta   = params->beta[i];
			const uint8_t *clip_table = Clip_Table[params->indexA[i]];

			if (!alpha && !beta)
				continue;

			int str = params->strength_v[x][y >> 2];
			if (str == 4)	// strong filtering
			{
				int p3 = rp[P3];	// just to read in the cache line
				int p2 = rp[P2];
				int p1 = rp[P1];
				int p0 = rp[P0];
				int q0 = rp[Q0];

				// read from left-most possible to the right to avoid costly cache misses
				if (LF_ABS(q0 - p0) < alpha)
				{
					int q1 = rp[Q1];

					if ((LF_ABS(q0 - q1) < beta) && (LF_ABS(p0 - p1) < beta))
					{
						int q2 = rp[Q2];

						int pq0 = p0 + q0;
						int adj = (LF_ABS(q0 - p0) < ((alpha >> 2) + 2));
						int aq = (LF_ABS(q0 - q2) < beta) & adj;
						int ap = (LF_ABS(p0 - p2) < beta) & adj;

						if (ap)
						{
                            rp[P0] = static_cast<uint8_t>((q1 + ((p1 + pq0) << 1) + p2 + 4) >> 3);
							rp[P1] = static_cast<uint8_t>((p2 + p1 + pq0 + 2) >> 2);
                            rp[P2] = static_cast<uint8_t>((((p3 + p2) << 1) + p2 + p1 + pq0 + 4) >> 3);
						}
						else
						{
							rp[P0] = static_cast<uint8_t>(((p1 << 1) + p0 + q1 + 2) >> 2);
						}

						if (aq)
						{
							int q3 = rp[Q3];

							rp[Q0] = static_cast<uint8_t>((p1 + ((q1 + pq0) << 1) + q2 + 4) >> 3);
							rp[Q1] = static_cast<uint8_t>((q2 + pq0 + q1 + 2) >> 2);
							rp[Q2] = static_cast<uint8_t>((((q3 + q2) << 1) + q2 + q1 + pq0 + 4) >> 3);
						}
						else
						{
							rp[Q0] = static_cast<uint8_t>(((q1 << 1) + q0 + p1 + 2) >> 2);
						}
					}
				}
			}
			else
			if (str != 0)	// 1-3 strength
			{
				int c0 = clip_table[str];
				int p2 = rp[P2];
				int p1 = rp[P1];
				int p0 = rp[P0];
				int q0 = rp[Q0];

				if (LF_ABS(q0-p0) < alpha)
				{
					int q1 = rp[Q1];
					if ((LF_ABS(q0 - q1) < beta) && (LF_ABS(p0 - p1) < beta))
					{
						int q2 = rp[Q2];

						int pq0 = (p0 + q0 + 1) >> 1;
						int aq = LF_ABS(q0 - q2) < beta;
						int ap = LF_ABS(p0 - p2) < beta;
						int tc0 = (c0 + ap + aq);
						int diff = CLIP3(-tc0, tc0, (((q0-p0) << 2) + (p1 - q1) + 4) >> 3);

						if (ap)
							rp[P1] += static_cast<uint8_t>(CLIP3(-c0, c0, (p2 + pq0 - (p1 << 1)) >> 1));

						if (diff != 0)
						{
							rp[P0] = static_cast<uint8_t>(CLIP3(0, 255, p0 + diff));
							rp[Q0] = static_cast<uint8_t>(CLIP3(0, 255, q0 - diff));
						}

						if (aq)
							rp[Q1] += static_cast<uint8_t>(CLIP3(-c0, c0, (q2 + pq0 - (q1 << 1)) >> 1));

					}
				}
			}
		}
	}
}

#define H_P3 (0)
#define H_Q3 (FILTER_BUFFER_STRIDE*7)
#define H_P2 (FILTER_BUFFER_STRIDE)
#define H_Q2 (FILTER_BUFFER_STRIDE*6)
#define H_P1 (FILTER_BUFFER_STRIDE*2)
#define H_Q1 (FILTER_BUFFER_STRIDE*5)
#define H_P0 (FILTER_BUFFER_STRIDE*3)
#define H_Q0 (FILTER_BUFFER_STRIDE*4)

void ApplyFilter_Luma_H(FilterParamType *params)
{
	int16_t *midABS_Table = &gABS_Table[1<<15];

	uint8_t *fp = params->y_buffer;

	for (int y = 0; y < 16; y++, fp++)
	{
		uint8_t *rp = fp - (4 * FILTER_BUFFER_STRIDE);

		for (int x = 0, i = 0; x < LOPS; x++, rp += (4 * FILTER_BUFFER_STRIDE), i = 1)
		{
			int alpha  = params->alpha[i];
			int beta   = params->beta[i];
			const uint8_t *clip_table = Clip_Table[params->indexA[i]];

			if (!alpha && !beta)
				continue;

			int str = params->strength_h[x][y >> 2];
			if (str == 4)	// strong filtering
			{
				int p0 = rp[H_P0];
				int q0 = rp[H_Q0];

				// read from left-most possible to the right to avoid costly cache misses
				if (LF_ABS(q0 - p0) < alpha)
				{
					int p1 = rp[H_P1];
					int q1 = rp[H_Q1];

					if ((LF_ABS(q0 - q1) < beta) && (LF_ABS(p0 - p1) < beta))
					{
						int p2 = rp[H_P2];
						int q2 = rp[H_Q2];

						int pq0 = p0 + q0;
						int adj = (LF_ABS(q0 - p0) < ((alpha >> 2) + 2));
						int aq = (LF_ABS(q0 - q2) < beta) & adj;
						int ap = (LF_ABS(p0 - p2) < beta) & adj;

						if (ap)
						{
							int p3 = rp[H_P3];

                            rp[H_P0] = static_cast<uint8_t>((q1 + ((p1 + pq0) << 1) + p2 + 4) >> 3);
							rp[H_P1] = static_cast<uint8_t>((p2 + p1 + pq0 + 2) >> 2);
                            rp[H_P2] = static_cast<uint8_t>((((p3 + p2) << 1) + p2 + p1 + pq0 + 4) >> 3);
						}
						else
						{
							rp[H_P0] = static_cast<uint8_t>(((p1 << 1) + p0 + q1 + 2) >> 2);
						}

						if (aq)
						{
							int q3 = rp[H_Q3];

							rp[H_Q0] = static_cast<uint8_t>((p1 + ((q1 + pq0) << 1) + q2 + 4) >> 3);
							rp[H_Q1] = static_cast<uint8_t>((q2 + pq0 + q1 + 2) >> 2);
							rp[H_Q2] = static_cast<uint8_t>((((q3 + q2) << 1) + q2 + q1 + pq0 + 4) >> 3);
						}
						else
						{
							rp[H_Q0] = static_cast<uint8_t>(((q1 << 1) + q0 + p1 + 2) >> 2);
						}
					}
				}
			}
			else
			if (str != 0)	// 1-3 strength
			{
				int c0 = clip_table[str];
				int p0 = rp[H_P0];
				int q0 = rp[H_Q0];

				if (LF_ABS(q0-p0) < alpha)
				{
					int p1 = rp[H_P1];
					int q1 = rp[H_Q1];
					if ((LF_ABS(q0 - q1) < beta) && (LF_ABS(p0 - p1) < beta))
					{
						int p2 = rp[H_P2];
						int q2 = rp[H_Q2];

						int pq0 = (p0 + q0 + 1) >> 1;
						int aq = LF_ABS(q0 - q2) < beta;
						int ap = LF_ABS(p0 - p2) < beta;
						int tc0 = (c0 + ap + aq);
						int diff = static_cast<int>(CLIP3(-tc0, tc0, (((q0-p0) << 2) + (p1 - q1) + 4) >> 3));

						if (ap)
							rp[H_P1] += static_cast<uint8_t>(CLIP3(-c0, c0, (p2 + pq0 - (p1 << 1)) >> 1));

						if (diff != 0)
						{
							rp[H_P0] = static_cast<uint8_t>(CLIP3(0, 255, p0 + diff));
							rp[H_Q0] = static_cast<uint8_t>(CLIP3(0, 255, q0 - diff));
						}

						if (aq)
							rp[H_Q1] += static_cast<uint8_t>(CLIP3(-c0, c0, (q2 + pq0 - (q1 << 1)) >> 1));
					}
				}
			}
		}
	}
}


#define FILTER_BUFFER_STRIDE_UV (FILTER_BUFFER_STRIDE / 2)

void ApplyFilter_Chroma_V(FilterParamType *params)
{
	int16_t *midABS_Table = &gABS_Table[1<<15];

	ChromaPixelType *fp = (ChromaPixelType *) params->uv_buffer;

	for (int y = 0; y < 8; y++, fp += (FILTER_BUFFER_STRIDE_UV))
	{
		ChromaPixelType *rp = fp - 4;

		for (int x = 0, i = 0; x < LOPS_UV; x++, rp += 4, i = 1)
		{
			int alpha  = params->alpha[i];
			int beta   = params->beta[i];
			const uint8_t *clip_table = Clip_Table[params->indexA[i]];

			if (!alpha && !beta)
				continue;

			int str = params->strength_v[x << 1][y >> 1];
			if (str)
			{
				ChromaPixelType p1 = rp[P1];
				ChromaPixelType p0 = rp[P0];
				ChromaPixelType q0 = rp[Q0];
				int diff = q0.u - p0.u;

				// do U first
				if (LF_ABS(diff) < alpha)
				{
					ChromaPixelType q1 = rp[Q1];
					if (LF_ABS(q0.u - q1.u) < beta)
					{
						if (LF_ABS(p0.u - p1.u) < beta)
						{
							if (str == 4)	// strong
							{
								rp[P0].u = (((p1.u << 1) + p0.u + q1.u + 2) >> 2);
								rp[Q0].u = (((q1.u << 1) + q0.u + p1.u + 2) >> 2);
							}
							else
							{
								int tc0 = clip_table[str] + 1;
								int d = CLIP3(-tc0, tc0, (((diff << 2) + (p1.u - q1.u) + 4) >> 3));

								if (d)
								{
									rp[P0].u = static_cast<uint8_t>(CLIP1(p0.u + d));
									rp[Q0].u = static_cast<uint8_t>(CLIP1(q0.u - d));
								}
							}
						}
					}
				}

				// then V
				diff = q0.v - p0.v;
				if (LF_ABS(diff) < alpha)
				{
					ChromaPixelType q1 = rp[Q1];
					if (LF_ABS(q0.v - q1.v) < beta)
					{
						if (LF_ABS(p0.v - p1.v) < beta)
						{
							if (str == 4)	// strong
							{
								rp[P0].v = (((p1.v << 1) + p0.v + q1.v + 2) >> 2);
								rp[Q0].v = (((q1.v << 1) + q0.v + p1.v + 2) >> 2);
							}
							else
							{
								int tc0 = clip_table[str] + 1;
								int d = CLIP3(-tc0, tc0, (((diff << 2) + (p1.v - q1.v) + 4) >> 3));

								if (d)
								{
									rp[P0].v = static_cast<uint8_t>(CLIP1(p0.v + d));
									rp[Q0].v = static_cast<uint8_t>(CLIP1(q0.v - d));
								}
							}
						}
					}
				}
			}
		}
	}
}


#define H_P3_CR (0)
#define H_Q3_CR (FILTER_BUFFER_STRIDE_UV*7)
#define H_P2_CR (FILTER_BUFFER_STRIDE_UV)
#define H_Q2_CR (FILTER_BUFFER_STRIDE_UV*6)
#define H_P1_CR (FILTER_BUFFER_STRIDE_UV*2)
#define H_Q1_CR (FILTER_BUFFER_STRIDE_UV*5)
#define H_P0_CR (FILTER_BUFFER_STRIDE_UV*3)
#define H_Q0_CR (FILTER_BUFFER_STRIDE_UV*4)

void ApplyFilter_Chroma_H(FilterParamType *params)
{
	int16_t *midABS_Table = &gABS_Table[1<<15];

	ChromaPixelType *fp = (ChromaPixelType *) params->uv_buffer;

	for (int y = 0; y < 8; y++, fp++)
	{
		ChromaPixelType *rp = fp - (4 * FILTER_BUFFER_STRIDE_UV);

		for (int x = 0, i = 0; x < LOPS_UV; x++, rp += (4 * FILTER_BUFFER_STRIDE_UV), i = 1)
		{
			int alpha  = params->alpha[i];
			int beta   = params->beta[i];
			const uint8_t *clip_table = Clip_Table[params->indexA[i]];

			if (!alpha && !beta)
				continue;

			int str = params->strength_h[x << 1][y >> 1];
			if (str)
			{
				ChromaPixelType p1 = rp[H_P1_CR];
				ChromaPixelType p0 = rp[H_P0_CR];
				ChromaPixelType q0 = rp[H_Q0_CR];
				int diff = q0.u - p0.u;

				// do U first
				if (LF_ABS(diff) < alpha)
				{
					ChromaPixelType q1 = rp[H_Q1_CR];
					if (LF_ABS(q0.u - q1.u) < beta)
					{
						if (LF_ABS(p0.u - p1.u) < beta)
						{
							if (str == 4)	// strong
							{
								rp[H_P0_CR].u = (((p1.u << 1) + p0.u + q1.u + 2) >> 2);
								rp[H_Q0_CR].u = (((q1.u << 1) + q0.u + p1.u + 2) >> 2);
							}
							else
							{
								int tc0 = clip_table[str] + 1;
								int d = CLIP3(-tc0, tc0, (((diff << 2) + (p1.u - q1.u) + 4) >> 3));

								if (d)
								{
									rp[H_P0_CR].u = static_cast<uint8_t>(CLIP1(p0.u + d));
									rp[H_Q0_CR].u = static_cast<uint8_t>(CLIP1(q0.u - d));
								}
							}
						}
					}
				}

				// then V
				diff = q0.v - p0.v;
				if (LF_ABS(diff) < alpha)
				{
					ChromaPixelType q1 = rp[H_Q1_CR];
					if (LF_ABS(q0.v - q1.v) < beta)
					{
						if (LF_ABS(p0.v - p1.v) < beta)
						{
							if (str == 4)	// strong
							{
								rp[H_P0_CR].v = (((p1.v << 1) + p0.v + q1.v + 2) >> 2);
								rp[H_Q0_CR].v = (((q1.v << 1) + q0.v + p1.v + 2) >> 2);
							}
							else
							{
								int tc0 = clip_table[str] + 1;
								int d = CLIP3(-tc0, tc0, (((diff << 2) + (p1.v - q1.v) + 4) >> 3));

								if (d)
								{
									rp[H_P0_CR].v = static_cast<uint8_t>(CLIP1(p0.v + d));
									rp[H_Q0_CR].v = static_cast<uint8_t>(CLIP1(q0.v - d));
								}
							}
						}
					}
				}
			}
		}
	}
}

void FilterMacroBlock(int index)
{
	int strength[4][4];
	MacroBlockType *mb = &GetCurrentMacroBlockSet()[index];

	// Do vertical edges first
	if (CalcEdgeStrength(mb, true, strength))
	{
//		ApplyFilter_Luma_V(mb, strength);
//		ApplyFilter_Chroma_V(mb, strength);
	}

	// Then do horizontal edges
	if (CalcEdgeStrength(mb, false, strength))
	{
//		ApplyFilter_Luma_H(mb, strength);
//		ApplyFilter_Chroma_H(mb, strength);
	}
}

#if 0 // UNUSED
static void FastCopyToBlock16x4s_32w(void *dst, void *src, int src_stride, int rows_by_4)
{
#ifdef _x64	// no _asm{} in 64-bit - thanks VS
	uint8_t *src8 = (uint8_t *) src;
	uint8_t *dst8 = (uint8_t *) dst;
	__m128i mm0;

	for (; rows_by_4 > 0; rows_by_4--)
	{
		mm0 = _mm_loadu_si128((__m128i*)src8);
		src8 += src_stride;
		_mm_store_si128((__m128i *) dst8, mm0);
		dst8 += MB_BLOCK_STRIDE * 2;
		mm0 = _mm_loadu_si128((__m128i*)src8);
		src8 += src_stride;
		_mm_store_si128((__m128i *) dst8, mm0);
		dst8 += MB_BLOCK_STRIDE * 2;
		mm0 = _mm_loadu_si128((__m128i*)src8);
		src8 += src_stride;
		_mm_store_si128((__m128i *) dst8, mm0);
		dst8 += MB_BLOCK_STRIDE * 2;
		mm0 = _mm_loadu_si128((__m128i*)src8);
		src8 += src_stride;
		_mm_store_si128((__m128i *) dst8, mm0);
		dst8 += MB_BLOCK_STRIDE * 2;
	}
#else
	_asm
	{
		mov esi,src
		mov edi,dst
		mov eax,src_stride
		mov ecx,rows_by_4
loop_start:
		movdqa xmm0, 0[esi]
		movdqa  0[edi],xmm0
		movdqa xmm0, 0[esi+eax]
		movdqa 32[edi],xmm0
		movdqa xmm0, 0[esi+eax*2]
		movdqa 64[edi],xmm0
		lea esi,[esi+eax*2]
		movdqa xmm0, 0[esi+eax]
		movdqa 96[edi],xmm0
		lea esi,[esi+eax*2]
		add edi,FILTER_BUFFER_STRIDE*4
		dec ecx
		jnz loop_start
	}
#endif
}
#endif

void FastCopyToBlock32x4s(void *dst, void *src, int src_stride, int rows_by_4)
{
#ifdef _x64	// no _asm{} in 64-bit - thanks VS
	uint8_t *src8 = (uint8_t *) src;
	uint8_t *dst8 = (uint8_t *) dst;
	__m128i mm0, mm1;

	for (; rows_by_4 > 0; rows_by_4--)
	{
		mm0 = _mm_loadu_si128((__m128i*)src8);
		mm1 = _mm_loadu_si128((__m128i*)(src8+16));
		src8 += src_stride;
		_mm_store_si128((__m128i *) dst8, mm0);
		_mm_store_si128((__m128i *) (dst8+16), mm1);
		dst8 += MB_BLOCK_STRIDE * 2;
		mm0 = _mm_loadu_si128((__m128i*)src8);
		mm1 = _mm_loadu_si128((__m128i*)(src8+16));
		src8 += src_stride;
		_mm_store_si128((__m128i *) dst8, mm0);
		_mm_store_si128((__m128i *) (dst8+16), mm1);
		dst8 += MB_BLOCK_STRIDE * 2;
		mm0 = _mm_loadu_si128((__m128i*)src8);
		mm1 = _mm_loadu_si128((__m128i*)(src8+16));
		src8 += src_stride;
		_mm_store_si128((__m128i *) dst8, mm0);
		_mm_store_si128((__m128i *) (dst8+16), mm1);
		dst8 += MB_BLOCK_STRIDE * 2;
		mm0 = _mm_loadu_si128((__m128i*)src8);
		mm1 = _mm_loadu_si128((__m128i*)(src8+16));
		src8 += src_stride;
		_mm_store_si128((__m128i *) dst8, mm0);
		_mm_store_si128((__m128i *) (dst8+16), mm1);
		dst8 += MB_BLOCK_STRIDE * 2;
	}
#else
	_asm
	{
		mov esi,src
		mov edi,dst
		mov eax,src_stride
		mov ecx,rows_by_4
loop_start:
		movdqa xmm0, 0[esi]
		movdqa xmm1,16[esi]
		movdqa  0[edi],xmm0
		movdqa 16[edi],xmm1
		movdqa xmm0, 0[esi+eax]
		movdqa xmm1,16[esi+eax]
		movdqa 32[edi],xmm0
		movdqa 48[edi],xmm1
		movdqa xmm0, 0[esi+eax*2]
		movdqa xmm1,16[esi+eax*2]
		movdqa 64[edi],xmm0
		movdqa 80[edi],xmm1
		lea esi,[esi+eax*2]
		movdqa xmm0, 0[esi+eax]
		movdqa xmm1,16[esi+eax]
		movdqa  96[edi],xmm0
		movdqa 112[edi],xmm1
		lea esi,[esi+eax*2]
		add edi,FILTER_BUFFER_STRIDE*4
		dec ecx
		jnz loop_start
	}
#endif
}

#if 0 // UNUSED
static void FastCopyFromBlock16x4s_32w(void *src, void *dst, int dst_stride, int rows_by_4)
{
#ifdef _x64	// no _asm{} in 64-bit - thanks VS
	uint8_t *src8 = (uint8_t *) src;
	uint8_t *dst8 = (uint8_t *) dst;
	__m128i mm0;

	for (; rows_by_4 > 0; rows_by_4--)
	{
		mm0 = _mm_loadu_si128((__m128i*)src8);
		src8 += MB_BLOCK_STRIDE * 2;
		_mm_store_si128((__m128i *) dst8, mm0);
		dst8 += dst_stride;
		mm0 = _mm_loadu_si128((__m128i*)src8);
		src8 += MB_BLOCK_STRIDE * 2;
		_mm_store_si128((__m128i *) dst8, mm0);
		dst8 += dst_stride;
		mm0 = _mm_loadu_si128((__m128i*)src8);
		src8 += MB_BLOCK_STRIDE * 2;
		_mm_store_si128((__m128i *) dst8, mm0);
		dst8 += dst_stride;
		mm0 = _mm_loadu_si128((__m128i*)src8);
		src8 += MB_BLOCK_STRIDE * 2;
		_mm_store_si128((__m128i *) dst8, mm0);
		dst8 += dst_stride;
	}
#else
	_asm
	{
		mov esi,src
		mov edi,dst
		mov eax,dst_stride
		mov ecx,rows_by_4
loop_start:
		movdqa xmm0, 0[esi]
		movdqa  0[edi],xmm0
		movdqa xmm0, FILTER_BUFFER_STRIDE[esi]
		movdqa  0[edi+eax],xmm0
		movdqa xmm0, FILTER_BUFFER_STRIDE+FILTER_BUFFER_STRIDE[esi]
		movdqa  0[edi+eax*2],xmm0
		lea edi,[edi+eax*2]
		movdqa xmm0, FILTER_BUFFER_STRIDE+FILTER_BUFFER_STRIDE+FILTER_BUFFER_STRIDE[esi]
		movdqa  0[edi+eax],xmm0
		lea edi,[edi+eax*2]
		add esi,FILTER_BUFFER_STRIDE*4
		dec ecx
		jnz loop_start
	}
#endif
}
#endif

static void FastCopyFromBlock32x4s(void *src, void *dst, int dst_stride, int rows_by_4)
{
#ifdef _x64	// no _asm{} in 64-bit - thanks VS
	uint8_t *src8 = (uint8_t *) src;
	uint8_t *dst8 = (uint8_t *) dst;
	__m128i mm0, mm1;

	for (; rows_by_4 > 0; rows_by_4--)
	{
		mm0 = _mm_loadu_si128((__m128i*)src8);
		mm1 = _mm_loadu_si128((__m128i*)(src8+16));
		src8 += MB_BLOCK_STRIDE * 2;
		_mm_store_si128((__m128i *) dst8, mm0);
		_mm_store_si128((__m128i *) (dst8+16), mm1);
		dst8 += dst_stride;
		mm0 = _mm_loadu_si128((__m128i*)src8);
		mm1 = _mm_loadu_si128((__m128i*)(src8+16));
		src8 += MB_BLOCK_STRIDE * 2;
		_mm_store_si128((__m128i *) dst8, mm0);
		_mm_store_si128((__m128i *) (dst8+16), mm1);
		dst8 += dst_stride;
		mm0 = _mm_loadu_si128((__m128i*)src8);
		mm1 = _mm_loadu_si128((__m128i*)(src8+16));
		src8 += MB_BLOCK_STRIDE * 2;
		_mm_store_si128((__m128i *) dst8, mm0);
		_mm_store_si128((__m128i *) (dst8+16), mm1);
		dst8 += dst_stride;
		mm0 = _mm_loadu_si128((__m128i*)src8);
		mm1 = _mm_loadu_si128((__m128i*)(src8+16));
		src8 += MB_BLOCK_STRIDE * 2;
		_mm_store_si128((__m128i *) dst8, mm0);
		_mm_store_si128((__m128i *) (dst8+16), mm1);
		dst8 += dst_stride;
	}
#else
	_asm
	{
		mov esi,src
		mov edi,dst
		mov eax,dst_stride
		mov ecx,rows_by_4
loop_start:
		movdqa xmm0, 0[esi]
		movdqa xmm1,16[esi]
		movdqa  0[edi],xmm0
		movdqa 16[edi],xmm1
		movdqa xmm0, 0+FILTER_BUFFER_STRIDE[esi]
		movdqa xmm1,16+FILTER_BUFFER_STRIDE[esi]
		movdqa  0[edi+eax],xmm0
		movdqa 16[edi+eax],xmm1
		movdqa xmm0, 0+FILTER_BUFFER_STRIDE+FILTER_BUFFER_STRIDE[esi]
		movdqa xmm1,16+FILTER_BUFFER_STRIDE+FILTER_BUFFER_STRIDE[esi]
		movdqa  0[edi+eax*2],xmm0
		movdqa 16[edi+eax*2],xmm1
		lea edi,[edi+eax*2]
		movdqa xmm0, 0+FILTER_BUFFER_STRIDE+FILTER_BUFFER_STRIDE+FILTER_BUFFER_STRIDE[esi]
		movdqa xmm1,16+FILTER_BUFFER_STRIDE+FILTER_BUFFER_STRIDE+FILTER_BUFFER_STRIDE[esi]
		movdqa  0[edi+eax],xmm0
		movdqa 16[edi+eax],xmm1
		lea edi,[edi+eax*2]
		add esi,FILTER_BUFFER_STRIDE*4
		dec ecx
		jnz loop_start
	}
#endif
}

void FilterMacroBlockRow(int row)
{
	if (gEncoderState.current_frame->nal_ref_idc == 0)
		return;	// not a reference frame

	MacroBlockType *mb = &GetCurrentMacroBlockSet()[row * gEncoderState.mb_width];
	__declspec(align(64)) uint8_t filter_buffer[FILTER_BUFFER_STRIDE*(20+12)];	// 20 for luma (16+4) 12 for chroma (8+4)
	uint8_t *uv_filter_buffer = &filter_buffer[FILTER_BUFFER_STRIDE*20];
	FilterParamType params;
	int uv_stride_in_bytes = mb->uv_stride * 2;

	params.y_buffer  =    &filter_buffer[(FILTER_BUFFER_STRIDE * 4) + 16];
	params.uv_buffer = &uv_filter_buffer[(FILTER_BUFFER_STRIDE * 4) + 16];

	for (int i = 0; i < gEncoderState.mb_width; i++, mb++)
	{
		bool has_v_strength = CalcEdgeStrength(mb, true, params.strength_v);
		bool has_h_strength = CalcEdgeStrength(mb, false, params.strength_h);

		if (!has_h_strength && !has_v_strength)
			continue;

		FastCopyToBlock32x4s(   &filter_buffer[0], mb->Y_frame_reconstructed  + mb->Y_block_start_index  - (mb->stride * 4)    - 16, mb->stride, 5);
		FastCopyToBlock32x4s(&uv_filter_buffer[0], mb->UV_frame_reconstructed + mb->UV_block_start_index - (mb->uv_stride * 4) -  8, uv_stride_in_bytes, 3);

		int qp = mb->qp;	// later we can average them

		int indexA = CLIP3(0, MAX_QP, qp + gEncoderState.current_frame->slice_alpha_c0_offset);
		int indexB = CLIP3(0, MAX_QP, qp + gEncoderState.current_frame->slice_beta_offset);

		params.alpha[1] = Alpha[indexA];
		params.beta[1]  = Beta [indexB];
		params.indexA[1] = indexA;

		// keep luma together for better cache usage
		if (has_v_strength)
		{

			// set edge params
			if (mb->mbAddrA)
			{
				int qp = (mb->qp + mb->mbAddrA->qp + 1) >> 1;	// average
				int indexA = CLIP3(0, MAX_QP, qp + gEncoderState.current_frame->slice_alpha_c0_offset);
				int indexB = CLIP3(0, MAX_QP, qp + gEncoderState.current_frame->slice_beta_offset);
				params.alpha[0] = Alpha[indexA];
				params.beta[0]  = Beta [indexB];
				params.indexA[0] = indexA;
			}
			else
			{
				params.alpha[0] = Alpha[indexA];
				params.beta[0]  = Beta [indexB];
				params.indexA[0] = indexA;
			}

			ApplyFilter_Luma_V(&params);
		}

#if 0//def _DEBUG
		FastCopyFromBlock32x4s(   &filter_buffer[0], mb->Y_frame_reconstructed  + mb->Y_block_start_index  - (mb->stride * 4)    - 16, mb->stride, 5);
		FastCopyFromBlock32x4s(&uv_filter_buffer[0], mb->UV_frame_reconstructed + mb->UV_block_start_index - (mb->uv_stride * 4) -  8, uv_stride_in_bytes, 3);
		{
		int index = (row * gEncoderState.mb_width) + i;
		if ((index <= 1) || (index == 80))
		{
			char str[256];
			sprintf_s(str, "after db Vert mb %d", index);
			DisplayBlock(0, str);
		}

		}

#endif

		if (has_h_strength)
		{
			// set edge params
			if (mb->mbAddrB)
			{
				int qp = (mb->qp + mb->mbAddrB->qp + 1) >> 1;	// average
				int indexA = CLIP3(0, MAX_QP, qp + gEncoderState.current_frame->slice_alpha_c0_offset);
				int indexB = CLIP3(0, MAX_QP, qp + gEncoderState.current_frame->slice_beta_offset);
				params.alpha[0] = Alpha[indexA];
				params.beta[0]  = Beta [indexB];
				params.indexA[0] = indexA;
			}
			else
			{
                params.alpha[0] = params.alpha[1];
                params.beta[0]  = params.beta[1];
                params.indexA[0] = params.indexA[1];
			}
			ApplyFilter_Luma_H(&params);	
		}

		static const byte qp_chroma_scale[52]=
		{
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
			16,17,18,19,20,21,22,23,24,25, 26, 27, 28, 29, 29, 30,
			31,32,32,33,34,34,35,35,36,36, 37, 37, 37, 38, 38, 38,
			39,39,39,39
		};

		qp = qp_chroma_scale[mb->qp];	// later we can average them

		indexA = CLIP3(0, MAX_QP, qp + gEncoderState.current_frame->slice_alpha_c0_offset);
		indexB = CLIP3(0, MAX_QP, qp + gEncoderState.current_frame->slice_beta_offset);

		params.alpha[1] = Alpha[indexA];
		params.beta[1]  = Beta [indexB];
		params.indexA[1] = indexA;

        if (has_v_strength)
		{
			// set edge params
			if (mb->mbAddrA)
			{
				int qp_cr = (qp_chroma_scale[mb->qp] + qp_chroma_scale[mb->mbAddrA->qp] + 1) >> 1;	// average
				int indexA_cr = CLIP3(0, MAX_QP, qp_cr + gEncoderState.current_frame->slice_alpha_c0_offset);
				int indexB_cr = CLIP3(0, MAX_QP, qp_cr + gEncoderState.current_frame->slice_beta_offset);
				params.alpha[0] = Alpha[indexA_cr];
				params.beta[0]  = Beta [indexB_cr];
				params.indexA[0] = indexA_cr;
			}
			else
			{
				params.alpha[0] = Alpha[indexA];
				params.beta[0]  = Beta [indexB];
				params.indexA[0] = indexA;
			}
			ApplyFilter_Chroma_V(&params);
		}

		if (has_h_strength)
		{
			// set edge params
			if (mb->mbAddrB)
			{
				int qp_cr = (qp_chroma_scale[mb->qp] + qp_chroma_scale[mb->mbAddrB->qp] + 1) >> 1;	// average
				int indexA_cr = CLIP3(0, MAX_QP, qp_cr + gEncoderState.current_frame->slice_alpha_c0_offset);
				int indexB_cr = CLIP3(0, MAX_QP, qp_cr + gEncoderState.current_frame->slice_beta_offset);
				params.alpha[0] = Alpha[indexA_cr];
				params.beta[0]  = Beta [indexB_cr];
				params.indexA[0] = indexA_cr;
			}
			else
			{
				params.alpha[0] = params.alpha[1];
				params.beta[0]  = params.beta[1];
				params.indexA[0] = params.indexA[1];
			}
			ApplyFilter_Chroma_H(&params);
		}

#if 1	// with new frame padding, we have off-frame pixels
		FastCopyFromBlock32x4s(   &filter_buffer[0], mb->Y_frame_reconstructed  + mb->Y_block_start_index  - (mb->stride * 4)    - 16, mb->stride, 5);
		FastCopyFromBlock32x4s(&uv_filter_buffer[0], mb->UV_frame_reconstructed + mb->UV_block_start_index - (mb->uv_stride * 4) -  8, uv_stride_in_bytes, 3);
#else
		if ((row != 0) && (i != 0))
		{
			FastCopyFromBlock32x4s(   &filter_buffer[0], mb->Y_frame_reconstructed  + mb->Y_block_start_index  - (mb->stride * 4)    - 16, mb->stride, 5);
			FastCopyFromBlock32x4s(&uv_filter_buffer[0], mb->UV_frame_reconstructed + mb->UV_block_start_index - (mb->uv_stride * 4) -  8, uv_stride_in_bytes, 3);
		}
		else
		if ((row == 0) && (i != 0))
		{
			// first row has no top 4 lines
			FastCopyFromBlock32x4s(   &filter_buffer[(FILTER_BUFFER_STRIDE*4)], mb->Y_frame_reconstructed   + mb->Y_block_start_index - 16, mb->stride, 4);	// copy 32x16
			FastCopyFromBlock32x4s(&uv_filter_buffer[(FILTER_BUFFER_STRIDE*4)], mb->UV_frame_reconstructed + mb->UV_block_start_index -  8, uv_stride_in_bytes, 2);	// copy 32x8
		}
		else
		if ((row != 0) && (i == 0))
		{	// first column has no left 4 lines
			FastCopyFromBlock16x4s_32w(   &filter_buffer[16], mb->Y_frame_reconstructed  + mb->Y_block_start_index  - (mb->stride * 4), mb->stride, 5);	// copy 16x20
			FastCopyFromBlock16x4s_32w(&uv_filter_buffer[16], mb->UV_frame_reconstructed + mb->UV_block_start_index - (mb->uv_stride * 4), uv_stride_in_bytes, 3);	// copy 16x12
		}
		else
		{	// i == 0 && row == 0
			// first block of first row has no top 4 lines or left 4 lines
			FastCopyFromBlock16x4s_32w(   &filter_buffer[(FILTER_BUFFER_STRIDE*4)+16], mb->Y_frame_reconstructed  + mb->Y_block_start_index,  mb->stride, 4);	// copy 16x16
			FastCopyFromBlock16x4s_32w(&uv_filter_buffer[(FILTER_BUFFER_STRIDE*4)+16], mb->UV_frame_reconstructed + mb->UV_block_start_index, uv_stride_in_bytes, 2);	// copy 16x8
		}
#endif

#if 0
		int index = (row * gEncoderState.mb_width) + i;
		if ((index <= 1) || (index == 80))
		{
			char str[256];
			sprintf_s(str, "after db mb %d", index);
			DisplayBlock(0, str);
		}
#endif
	}
}

void FilterAllMacroBlocks()
{
	for (int row = 0; row < gEncoderState.mb_height; row++)
		FilterMacroBlockRow(row);
}
