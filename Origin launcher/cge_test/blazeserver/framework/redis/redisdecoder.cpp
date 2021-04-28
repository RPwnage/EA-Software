/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "EASTL/string.h"
#include "EAStdC/EAString.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/redis/redisid.h"
#include "framework/redis/redissliver.h"
#include "framework/redis/redisdecoder.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

const char8_t* RedisDecoder::decodeValue(const char8_t* begin, const char8_t* end, int16_t& value)
{
    char8_t* term = nullptr;
    value = (int16_t) EA::StdC::StrtoI32(begin, &term, 10);
    return term;
}

const char8_t* RedisDecoder::decodeValue(const char8_t* begin, const char8_t* end, uint16_t& value)
{
    char8_t* term = nullptr;
    value = (uint16_t) EA::StdC::StrtoU32(begin, &term, 10);
    return term;
}

const char8_t* RedisDecoder::decodeValue(const char8_t* begin, const char8_t* end, int32_t& value)
{
    char8_t* term = nullptr;
    value = EA::StdC::StrtoI32(begin, &term, 10);
    return term;
}

const char8_t* RedisDecoder::decodeValue(const char8_t* begin, const char8_t* end, uint32_t& value)
{
    char8_t* term = nullptr;
    value = EA::StdC::StrtoU32(begin, &term, 10);
    return term;
}

const char8_t* RedisDecoder::decodeValue(const char8_t* begin, const char8_t* end, int64_t& value)
{
    char8_t* term = nullptr;
    value = EA::StdC::StrtoI64(begin, &term, 10);
    return term;
}

const char8_t* RedisDecoder::decodeValue(const char8_t* begin, const char8_t* end, uint64_t& value)
{
    char8_t* term = nullptr;
    value = EA::StdC::StrtoU64(begin, &term, 10);
    return term;
}

const char8_t* RedisDecoder::decodeValue(const char8_t* begin, const char8_t* end, double& value)
{
    char8_t* term = nullptr;
    value = EA::StdC::Strtod(begin, &term);
    return term;
}

const char8_t* RedisDecoder::decodeValue(const char8_t* begin, const char8_t* end, eastl::string& value)
{
    if (mEscaping != 0)
    {
        const char8_t* substringStart = begin;
        size_t substringLength = 0;

        for (char8_t current = *begin; (current != REDIS_PAIR_DELIMITER) && (current != '\0'); current = *(++begin))
        {
            if (current == REDIS_PAIR_DELIMITER_ESCAPE)
            {
                current = *(++begin);

                value.append(substringStart, substringLength);
                substringStart = begin;
                substringLength = 0;
            }

            ++substringLength;
        }

        value.append(substringStart, substringLength);
    }
    else
    {
        value.assign(begin, end);
        begin = end;
    }

    return begin;
}

const char8_t* RedisDecoder::decodeValue(const char8_t* begin, const char8_t* end, EA::TDF::Tdf& value)
{
    size_t size = end - begin;
    RawBuffer rawBuf((uint8_t*)begin, size, false);
    rawBuf.put(size);
    EA::TDF::TdfId id = 0;
    memcpy(&id, rawBuf.data(), sizeof(id));
    if (id == value.getTdfId())
    {
        rawBuf.pull(sizeof(id));
        Heat2Decoder dec;
        // TODO: Replace BlazeRpcError with bool
        BlazeRpcError rc = dec.decode(rawBuf, value, false);
        if (rc == ERR_OK)
        {
            // return head of data following the decode
            begin = (char8_t*) rawBuf.data();
        }
        else
            begin = nullptr; // fail;
    }
    else
    {
        // REDIS_TODO: replace assert with error (possily return nullptr and check for it then bail up further)
        BLAZE_ASSERT_LOG(Log::REDIS, "RedisDecoder.decodeValue: The Id of the expected Tdf (" << value.getTdfId() << ") does not match the encoded id (" << id << ") found in the buffer");
        begin = nullptr;
    }
    return begin;
}

const char8_t* RedisDecoder::decodeValue(const char8_t* begin, const char8_t* end, RedisId& value)
{
    return decodeValue(begin, end, value.mSchemaKeyPair);
}

const char8_t* RedisDecoder::decodeValue(const char8_t* begin, const char8_t* end, RedisSliver& value)
{
    bool isTagged = *begin == '{';
    if (isTagged)
    {
        ++begin;
    }
    char8_t* term = nullptr;
    value.isTagged = isTagged;
    value.sliverId = (uint16_t) EA::StdC::StrtoU32(begin, &term, 10);
    if (isTagged)
    {
        // NOTE: EA::StdC::StrtoU32() always sets term != nullptr
        if (*term == '}')
        {
            ++term;
        }
        else
        {
            BLAZE_ASSERT_LOG(Log::REDIS, "RedisDecoder.decodeValue: tagged RedisSliver missing '}'.");
            term = nullptr;
        }
    }
    return term;
}

} // Blaze
