/*********************************************************************************/
/*!
    \File random.cpp

    \Copyright
        (c) Electronic Arts. All Rights Reserved.

    \Version 1.0 11/12/2004 (tyrone@ea.com) First Version (Lobby project)
    \Version 1.0 03/09/2008 (divkovic@ea.com) First Version (Blaze project)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "framework/blaze.h"
#include "framework/util/random.h"

#include "framework/logger.h"

namespace Blaze
{

#ifdef EA_PLATFORM_LINUX
uint32_t Random::mRandomSeed = 0;
#endif


Random::Random()
{
#if defined(EA_PLATFORM_WINDOWS)
    EA_ASSERT_MSG(::CryptAcquireContextW(&mProviderHandle, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT), 
        "Random number generator initialization failed: Crypt context could not be acquired.");
#elif defined(EA_PLATFORM_LINUX)
    mRndFile = fopen("/dev/urandom", "rb");
    EA_ASSERT_MSG(mRndFile != 0, 
        "Random number generator initialization failed: file /dev/urandom cannot be openned.");
#endif

}

Random::~Random()
{
#if defined(EA_PLATFORM_WINDOWS)
    ::CryptReleaseContext(mProviderHandle, 0);
#elif defined(EA_PLATFORM_LINUX)
    fclose(mRndFile);
#endif

}

void Random::initializeSeed()
{
#ifdef EA_PLATFORM_LINUX
    Random::mRandomSeed = (uint32_t)TimeValue::getTimeOfDay().getMillis();
#endif
}

float Random::getRandomFloat(float minVal, float maxVal)
{
    // We cast to double here to avoid an initial loss of accuracy
    float randValue = (float)((double)Random::getRandomNumber(Random::MAX) / (double)Random::MAX);
    return randValue * (maxVal - minVal) + minVal;
}


int Random::getRandomNumber(int range)
{
    EA_ASSERT(range <= MAX);

    if (EA_UNLIKELY(range <= 1))
        return 0;

#ifdef EA_PLATFORM_LINUX
    // Note: while it's more efficient to use the low order bits (ie: return (rand() % range); ), 
    //   standard rand impls aren't so good; they tend to by cyclic in the low bits (amongst other problems)
    // http://www.azillionmonkeys.com/qed/random.html

    // we're doing two things here:
    // 1) re-rolling if we fall outside a uniformly divisible number for the supplied range.
    //     the idea here is to chop off the top (rand_max % range) values, since they lead to an invalid distribution.
    const int64_t upperBound = ((int64_t)RAND_MAX + 1) - (((int64_t)RAND_MAX + 1) % range);
    int randNum;

    do
    {
         randNum = rand_r(&Random::mRandomSeed);   
    }
    while (randNum >= upperBound);

    // 2) using floating point math to mitigate cyclic low bits (instead of using % range)
    // NOTE: On UNIX, RAND_MAX is a large number so calculations will be wrong unless we use a double.
    int returnRandNum = (int)(range * ((double)randNum / (double)upperBound));

    // Clamp this range so we guarantee we are within [0, range).
    if (returnRandNum >= range || returnRandNum < 0)
    {
        EA_ASSERT(returnRandNum < range || returnRandNum >= 0);
        // TODO: Add error logging for production.
        //ERR_LOG("getRandomNumber(): Calculated Random Number (" << returnRandNum << ") is out of range (" << range << ") returning range.");
        return range - 1;
    }

    return returnRandNum;
#else
    int randNum;
    rand_s((unsigned int*)&randNum);
    randNum = (randNum & INT_MAX) % range;
    return randNum;
#endif
}

int Random::getStrongRandomNumber(int range)
{
    EA_ASSERT(range <= MAX);

    if (EA_UNLIKELY(range <= 1))
        return 0;

    unsigned int result = 0;
    if (!getStrongRandomNumber(&result, sizeof(result)))
        return -1;

    result %= (unsigned int)range;
    return (int)result;
}

bool Random::getStrongRandomNumber256(Blaze::Int256Buffer& result)
{
    int64_t random[4];
    bool ret = getStrongRandomNumber256(random);

    if (ret)
    {
        result.setPart1(random[0]);
        result.setPart2(random[1]);
        result.setPart3(random[2]);
        result.setPart4(random[3]);
    }

    return ret;
}

/*! \brief output randomized 256 bits. returns whether succeeded */
bool Random::getStrongRandomNumber256(int64_t (&result)[4])
{
    const size_t len = 4 * sizeof(int64_t);
    if (!getStrongRandomNumber(result, len))
        return false;

    // Ensure positive
    result[0] &= INT64_MAX;
    return true;
}

/*! \brief output randomized n bits. returns whether succeeded */
bool Random::getStrongRandomNumber(void *buf, size_t bufLen)
{
    memset(buf, 0, bufLen);

#if defined(EA_PLATFORM_LINUX)
    const size_t numItems = fread(buf, bufLen, 1, mRndFile);
    if (numItems != 1)
    {
        BLAZE_ERR_LOG(Log::UTIL, "[Random].getStrongRandomNumber: random generator failed with result " << numItems << ".");
        return false;
    }
#elif defined(EA_PLATFORM_WINDOWS)
    if (::CryptGenRandom(mProviderHandle, bufLen, (BYTE*)buf) != TRUE)
    {
        BLAZE_ERR_LOG(Log::UTIL, "[Random].getStrongRandomNumber: random generator failed with result " << ((uint64_t)::GetLastError()) << ".");
        return false;
    }
#else
    EA_ASSERT_MSG(false, "Strong random number generator not defined on this platform!");
#endif

    return true;
}


} // Blaze
