/*! ************************************************************************************************/
/*!
    \file   gaussianfunction.cpp


    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/gaussianfunction.h"

#include <math.h>

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    GaussianFunction::GaussianFunction()
          : mPrecalculatedVariance(-1), 
            mBellWidth(1), 
            mCacheType(CACHE_TYPE_HASHTABLE), 
            mGaussianArray(nullptr), 
            mGaussianArraySize(0)
    {
    }

    GaussianFunction::~GaussianFunction()
    {
        if (mGaussianArray != nullptr)
        {
            delete[] mGaussianArray;
        }
    }

    bool GaussianFunction::initialize(uint32_t bellWidth, CacheType cacheType, uint32_t preCalcCacheRange)
    {
        mBellWidth = bellWidth;
        mCacheType = cacheType;
        bool isCleanValidation = true;
        if (mBellWidth == 0)
        {
            // must change value (or we'll get a divide by zero when evaluating)
            WARN_LOG("[GaussianFunction].initialize() - mBellWidth configed to be 0, using mBellWidth of 1.");
            mBellWidth = 1;
            isCleanValidation = false;
        }

        // pre-calculated denominator of the gausian function for efficiency based upon the bellWidth which
        // doesn't usually change.
        // mPreCalculatedVariance is (1 / (B x 1.2011225)) ^2 where B is the width of the bell curve for the
        // Gaussian function.
        mPrecalculatedVariance = 1.0f / (mBellWidth * 1.2011225f);
        mPrecalculatedVariance *= mPrecalculatedVariance;

        // pre-calculate all of the Gaussian values between 0 and preCalcCacheRange
        if (cacheType == CACHE_TYPE_HASHTABLE)
        {
            for (uint32_t i = 0; i < preCalcCacheRange; ++i)
            {
                mGaussianHashMap.insert(eastl::make_pair(i, calcFitPercent(i)));
            }
        }
        else
        {
            EA_ASSERT(mGaussianArray == nullptr); // don't call init multiple times
            EA_ASSERT(preCalcCacheRange > 0); // can't have a zero size array cache

            mGaussianArraySize = preCalcCacheRange;
            mGaussianArray = BLAZE_NEW_ARRAY(float, mGaussianArraySize);
            for (uint32_t i = 0; i < mGaussianArraySize; ++i)
            {
                mGaussianArray[i] = calcFitPercent(i);
            }
        }

        return isCleanValidation;
    }

    float_t GaussianFunction::calculate(int32_t x, int32_t b) const
    {
        // make sure difference is 0 or positive for the cache
        uint32_t difference = (x - b) < 0 ? b - x : x - b;


        if (mCacheType == CACHE_TYPE_ARRAY)
        {
            return getFitPercentFromArrayCache(difference);
        }
        else
        {
            return getFitPercentFromHashMapCache(difference);
        }
    }

    float_t GaussianFunction::getFitPercentFromArrayCache(uint32_t difference) const
    {
        if (mGaussianArray !=nullptr && difference < mGaussianArraySize)
        {
            return mGaussianArray[difference];
        }
        else
        {
            TRACE_LOG("[GaussianFunction].getFitPercentFromArrayCache() - trying to calculate difference (" << difference << ") that is outside of our array cache size (" 
                      << mGaussianArraySize << "); falling back to hash cache.");
            return getFitPercentFromHashMapCache(difference);
        }
    }

    float_t GaussianFunction::getFitPercentFromHashMapCache(uint32_t difference) const
    {
        // look up in the cache and return the cached value if it exists.
        GaussianCache::const_iterator gaussianValueItr = mGaussianHashMap.find(difference);
        if (gaussianValueItr != mGaussianHashMap.end())
        {
            return gaussianValueItr->second;
        }

        // calc the value and cache it for future lookups
        float_t fitPercent = calcFitPercent(difference);
        mGaussianHashMap[difference] = fitPercent;
        return fitPercent;
    }

    float_t GaussianFunction::calcFitPercent(uint32_t difference) const
    {
        int64_t squaredDifference = difference * difference;
        return expf( -(squaredDifference * mPrecalculatedVariance) );
    }

    uint32_t GaussianFunction::inverse(float_t gaussianValue) const
    {
        EA_ASSERT(gaussianValue >= 0.0f && gaussianValue <= 1.0f); // NOTE: log(0) is undefined (approaches infinity)

        // Clamp the value between 0,UINT16_MAX as <=0 is undefined and =>1 is 0.
        if (gaussianValue >= 1.0f) return 0;
        if (gaussianValue <= 0.0f) return UINT32_MAX;

        float_t delta = static_cast<float_t>(sqrt( -(log(gaussianValue) / mPrecalculatedVariance) ));

        return (delta > (float_t)UINT32_MAX) ? UINT32_MAX : (uint16_t)delta;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
