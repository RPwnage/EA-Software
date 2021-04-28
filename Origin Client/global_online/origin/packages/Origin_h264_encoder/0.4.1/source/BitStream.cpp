#include "BitStream.h"


void InitBitStream(BitStreamType *bs, uint8_t *stream)
{
    bs->stream = stream;
    bs->byte_pos = 0;
    bs->bit_pos = 0;
    bs->bits_rem = 8;
    bs->current_8_bits = 0;
}

void WriteBits(BitStreamType *bs, uint32_t value, int num_bits)
{
    if (num_bits < bs->bits_rem)
    {
last_bits:
        bs->current_8_bits |= (value << (bs->bits_rem - num_bits));
        // advance write position
        bs->bit_pos += num_bits;
        bs->bits_rem -= num_bits;
    }
    else
    {
next_8:
        bs->current_8_bits |= (value >> (num_bits - bs->bits_rem));
        bs->stream[bs->byte_pos++] = bs->current_8_bits;
        bs->bit_pos += bs->bits_rem;
        num_bits -= bs->bits_rem;
        // move to next byte
        bs->bits_rem = 8;
        bs->current_8_bits = 0;

        if (num_bits > 0)
        {
            value <<= (32 - num_bits);
            value >>= (32 - num_bits);
            if (num_bits >= bs->bits_rem)
            {
                goto next_8;
            }
            else
            {
                goto last_bits;
            }
        }
    }
}

void FlushWriteBits(BitStreamType *bs)
{
    bs->stream[bs->byte_pos] = bs->current_8_bits;
}

unsigned int ReadBits_u(BitStreamType *bs, int num_bits)
{
    unsigned int value = 0;

    if (num_bits < bs->bits_rem)
    {
last_bits:
        value = bs->current_8_bits >> (8 - num_bits);
        // advance read position
        bs->bit_pos += num_bits;
        bs->bits_rem -= num_bits;
        bs->current_8_bits <<= num_bits;
    }
    else
    {
next_8:
        value |= bs->current_8_bits >> (8 - bs->bits_rem);
        bs->bit_pos += bs->bits_rem;
        num_bits -= bs->bits_rem;
        // move to next byte
        bs->bits_rem = 8;
        bs->byte_pos++;
        bs->stream++;
        bs->current_8_bits = *bs->stream;

        if (num_bits > 0)
        {
            if (num_bits >= bs->bits_rem)
            {
                value <<= 8;
                goto next_8;
            }
            else
            {
                value <<= (num_bits);
                goto last_bits;
            }
        }
    }

    return value;
}

unsigned int PeekBits_u(BitStreamType *bs, int num_bits)
{
    unsigned int value = 0;
    BitStreamType pbs = *bs;

    if (num_bits < pbs.bits_rem)
    {
last_bits:
        value = pbs.current_8_bits >> (8 - num_bits);
        pbs.current_8_bits <<= num_bits;
    }
    else
    {
next_8:
        value |= pbs.current_8_bits >> (8 - pbs.bits_rem);
        num_bits -= pbs.bits_rem;
        // move to next byte
        pbs.bits_rem = 8;
        pbs.stream++;
        pbs.current_8_bits = *bs->stream;

        if (num_bits > 0)
        {
            if (num_bits >= pbs.bits_rem)
            {
                value <<= 8;
                goto next_8;
            }
            else
            {
                value <<= (num_bits);
                goto last_bits;
            }
        }
    }

    return value;
}

// Exponential-Golomb coding
const uint8_t sUE_BitLength[256] =
{
#if 1
	1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
#else
    1, 1, 3, 3, 5, 5, 5, 5, 7, 7, 7, 7, 7, 7, 7, 7,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
    11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
    13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
    13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
    13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
    13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
    15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
#endif
};

void WriteBits_ue(BitStreamType *bs, uint16_t value)
{
	for (int i = (sUE_BitLength[value]-1); i > 0; i--)
	{
		WriteBits(bs, 0, 1);
	}
	WriteBits(bs, value + 1, sUE_BitLength[value]);
}

void WriteBits_se(BitStreamType *bs, int16_t value)
{
    if (value <= 0)
        value = -(value << 1);
    else
        value = (value << 1) - 1;
    WriteBits_ue(bs, value);
}

uint32_t ReadBits_ue(BitStreamType *bs)
{
    int bitlen = 0;
    uint32_t value = 0;

    while (ReadBits_u(bs, 1) == 0)
        bitlen++;

    if (bitlen == 0)
        return 0;

    value = ((1 << bitlen) | ReadBits_u(bs, bitlen)) - 1;

    return value;
}

int32_t ReadBits_se(BitStreamType *bs)
{
    int bitlen = 0;
    int32_t value = 0;

    while (ReadBits_u(bs, 1) == 0)
        bitlen++;

    if (bitlen == 0)
        return 0;

    value = ((1 << bitlen) | ReadBits_u(bs, bitlen)) - 1;

    if (value & 1)	// positive
        value = (value / 2) + 1;
    else
        value = -(value / 2);

    return value;
}
