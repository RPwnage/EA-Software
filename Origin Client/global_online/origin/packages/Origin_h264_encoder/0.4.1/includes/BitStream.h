#ifndef __BITSTREAM_H
#define __BITSTREAM_H

#include <stdint.h>

typedef struct 
{
    uint8_t *stream;
    unsigned int byte_pos;
    unsigned int bit_pos;
    int bits_rem;
    uint8_t current_8_bits;
} BitStreamType;



void InitBitStream(BitStreamType *bs, uint8_t *stream);

unsigned int ReadBits_u(BitStreamType *bs, int num_bits);
unsigned int PeekBits_u(BitStreamType *bs, int num_bits);
void WriteBits(BitStreamType *bs, uint32_t value, int num_bits);
void WriteBits_ue(BitStreamType *bs, uint16_t value);
void WriteBits_se(BitStreamType *bs, int16_t value);
void FlushWriteBits(BitStreamType *bs);

uint32_t ReadBits_ue(BitStreamType *bs);
int32_t ReadBits_se(BitStreamType *bs);


#endif