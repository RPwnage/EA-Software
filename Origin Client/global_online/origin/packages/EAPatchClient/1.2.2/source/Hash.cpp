///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Hash.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Hash.h>
#include <EAPatchClient/Debug.h>
#include <EASTL/type_traits.h>

namespace EA
{
namespace Patch
{


/////////////////////////////////////////////////////////////////////////////
// HashValueMD5
/////////////////////////////////////////////////////////////////////////////

HashValueMD5::HashValueMD5()
{
    EA::StdC::Memclear(mData, sizeof(mData));
}

void HashValueMD5::Clear()
{
    EA::StdC::Memclear(mData, sizeof(mData));
}

bool operator==(const HashValueMD5& a, const HashValueMD5& b)
{
    return memcmp(&a, &b, sizeof(HashValueMD5)) == 0;
}


/////////////////////////////////////////////////////////////////////////////
// HashValueMD5Brief
/////////////////////////////////////////////////////////////////////////////

HashValueMD5Brief::HashValueMD5Brief()
{
    EA::StdC::Memclear(mData, sizeof(mData));
}

void HashValueMD5Brief::Clear()
{
    EA::StdC::Memclear(mData, sizeof(mData));
}

bool operator==(const HashValueMD5Brief& a, const HashValueMD5Brief& b)
{
    return memcmp(&a, &b, sizeof(HashValueMD5Brief)) == 0;
}



// We xor the bytes of an MD5 hash to make a smaller version of it, though the result is less unique.
void GenerateBriefHash(const HashValueMD5& hashMD5, HashValueMD5Brief& hashMD5Brief)
{
    const size_t kMD5BriefCount = EAArrayCount(hashMD5Brief.mData);

    for(size_t i = 0; i < kMD5BriefCount; i++)
        hashMD5Brief.mData[i] = (uint8_t)(hashMD5.mData[i + 0] ^ hashMD5.mData[i + kMD5BriefCount]);
}



///////////////////////////////////////////////////////////////////////////////
// BlockHashWithPosition
///////////////////////////////////////////////////////////////////////////////

bool operator==(const BlockHashWithPosition& a, const BlockHashWithPosition& b)
{
    return (a.mChecksum  == b.mChecksum) && 
           (a.mHashValue == b.mHashValue) && 
           (a.mPosition  == b.mPosition);
}



///////////////////////////////////////////////////////////////////////////////
// Rolling checksum
///////////////////////////////////////////////////////////////////////////////

// What we do here is subtract prevBlockFirstByte from the checksum and add
// nextBlockLastByte to the checksum.
// 
// To consider: Make this function an inline function, as it can get called very
// many times for a file; as many as once per file byte.
uint32_t UpdateRollingChecksum(uint32_t prevChecksum, uint8_t prevBlockFirstByte, uint8_t nextBlockLastByte, size_t blockShift)
{
    EAPATCH_ASSERT((blockShift >= kDiffBlockShiftMin) && (blockShift <= kDiffBlockShiftMax));

    uint16_t low16  = (uint16_t)prevChecksum;
    uint16_t high16 = (uint16_t)(prevChecksum >> 16);

    low16  += (nextBlockLastByte - prevBlockFirstByte);
    high16 += low16 - (prevBlockFirstByte << blockShift);

    return (uint32_t)((high16 << 16) | low16);
}

//This is a one-shot function to get the rolling sum for a block multiple bytes.
uint32_t GetRollingChecksum(const uint8_t* pData, size_t dataLength)
{
    uint16_t low16 = 0;
    uint16_t high16 = 0;

    while (dataLength)
    {
        const uint8_t c = *pData++;
        low16 += (uint16_t)(c);
        high16 += (uint16_t)(c * dataLength);
        dataLength--;
    }

    return (uint32_t)((high16 << 16) | low16);
}

///////////////////////////////////////////////////////////////////////////////
// MD5
///////////////////////////////////////////////////////////////////////////////

void MD5::Begin()
{
    EA_COMPILETIME_ASSERT((sizeof(mWorkData) >= sizeof(MD5_CTX)) && EA_ALIGN_OF(uint64_t) >= EA_ALIGN_OF(MD5_CTX));
    MD5_Init(&mCtx);
}

void MD5::AddData(const void* data, size_t dataLength)
{
    EAPATCH_ASSERT(dataLength < INT32_MAX); // DritySDK is limited to int32_t data.
    MD5_Update(&mCtx, data, dataLength);

}

void MD5::End(HashValueMD5& hashValue)
{
    MD5_Final(hashValue.mData, &mCtx);
}

void MD5::GetHashValue(const void* data, size_t dataLength, HashValueMD5& hashValue)
{
    MD5 md5;

    md5.Begin();
    md5.AddData(data, dataLength);
    md5.End(hashValue);
}

void MD5::GetHashValue(const void* data1, size_t dataLength1, 
                         const void* data2, size_t dataLength2, HashValueMD5& hashValue)
{
    MD5 md5;

    md5.Begin();
    md5.AddData(data1, dataLength1);
    md5.AddData(data2, dataLength2);
    md5.End(hashValue);
}



} // namespace Patch
} // namespace EA








