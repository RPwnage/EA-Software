/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_DECODER_H
#define BLAZE_REDIS_DECODER_H

/*** Include files *******************************************************************************/
#include "EASTL/utility.h"
#include "EASTL/string.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace EA
{
    namespace TDF
    {
        class Tdf;
    }
}

namespace Blaze
{

struct RedisId;
struct RedisSliver;

const char8_t REDIS_PAIR_DELIMITER         = ('.');
const char8_t REDIS_PAIR_DELIMITER_ESCAPE  = ('\\');

class RedisDecoder
{
    NON_COPYABLE(RedisDecoder);

public:
    RedisDecoder() : mEscaping(0) {}

    const char8_t* decodeValue(const char8_t* begin, const char8_t* end, int16_t& value);
    const char8_t* decodeValue(const char8_t* begin, const char8_t* end, uint16_t& value);
    const char8_t* decodeValue(const char8_t* begin, const char8_t* end, int32_t& value);
    const char8_t* decodeValue(const char8_t* begin, const char8_t* end, uint32_t& value);
    const char8_t* decodeValue(const char8_t* begin, const char8_t* end, int64_t& value);
    const char8_t* decodeValue(const char8_t* begin, const char8_t* end, uint64_t& value);
    const char8_t* decodeValue(const char8_t* begin, const char8_t* end, double& value);
    const char8_t* decodeValue(const char8_t* begin, const char8_t* end, eastl::string& value);
    const char8_t* decodeValue(const char8_t* begin, const char8_t* end, EA::TDF::Tdf& value);
    const char8_t* decodeValue(const char8_t* begin, const char8_t* end, RedisId& value);
    const char8_t* decodeValue(const char8_t* begin, const char8_t* end, RedisSliver& value);
    template<typename A, typename B>
    const char8_t* decodeValue(const char8_t* begin, const char8_t* end, eastl::pair<A, B>& value)
    {
        ++mEscaping;

        begin = decodeValue(begin, end, value.first);
        if (begin != nullptr)
        {
            if (begin < end)
            {
                if (*begin == REDIS_PAIR_DELIMITER)
                {
                    begin = decodeValue(begin + 1, end, value.second);
                }
                else
                {
                    BLAZE_ASSERT_LOG(Log::REDIS, "RedisDecoder.decodeValue: Expected to find a (" << REDIS_PAIR_DELIMITER << ") character, but found (" << *begin << ")");
                    begin = nullptr;
                }
            }
        }

        --mEscaping;

        return begin;
    }

private:
    int64_t mEscaping;
};

} // Blaze

#endif // BLAZE_REDIS_DECODER_H

