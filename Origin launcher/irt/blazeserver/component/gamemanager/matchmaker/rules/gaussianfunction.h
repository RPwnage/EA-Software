/*! ************************************************************************************************/
/*!
    \file gaussianfunction.h


    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_MATCHMAKING_GAUSSIAN_FUNCTION_H
#define BLAZE_MATCHMAKING_GAUSSIAN_FUNCTION_H

#include "EASTL/hash_map.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*!
        \class GausianFunction

        A Gaussian Function for computations in some of the matchmaking rules.  A better description of
        a Gaussian function can be found here: http://en.wikipedia.org/wiki/Gaussian_function.  This
        function is based off of the implementation used in the old Plasma Matchmaking.
    ***************************************************************************************************/
    class GaussianFunction
    {
    public:
        GaussianFunction();
        virtual ~GaussianFunction();

        enum CacheType
        {
            CACHE_TYPE_HASHTABLE, // can be initialized using the perCalcCacheRange, but will grow (using lazy initialization)
            CACHE_TYPE_ARRAY // NOTE: array caches must be initialized using the preCalcCacheRange (see initialize) NOTE: we fall back to the hashtable cache
                             //  if calculating something outside the array's bounds
        };

        /*! ************************************************************************************************/
        /*! \brief Initialize the GaussianFunction with the bell curve width and cache this off.
            This is mostly for performance as the width should not change often during calculations.

            \param[in] bellWidth controls the bell width in the Gaussian function. (must be > 0)
            \param[in] cacheType the type of caching to use for this gaussian function.
            \param[in] preCalcCacheRange - we init the cache using the range 0..preCaclCacheRange.
            \return true if initialization succeeds without any errors.
        ***************************************************************************************************/
        bool initialize(uint32_t bellWidth, CacheType cacheType, uint32_t preCalcCacheRange);

        /*! ************************************************************************************************/
        /*!
            \brief given two values determine the difference and plug that value into the Gaussian function,
            returning a Gaussian value between 0 and 1.

            At heart, we're evaluating: e^( - ((x-b)^2 / (c * 1.2011225)^2 )) where
                x is the center of the curve
                b is some point some distance away from the center
                c is the fiftyPercentFitValueDifference which determines the width of the bell curve
            
            however, we've precomputed the divisior as mPrecalculatedVariance.

            \param[in] x a value some distance from b to determine the Gaussian fit between x and b.
            \param[in] b the position of the center of the Gaussian peak for which to evaluate x.
            \return the Gaussian value for the distance of x from b given the bellWidth. (note: x,b is the same as b,x due to symmetry)
        ***************************************************************************************************/
        float_t calculate(int32_t x, int32_t b) const;

        /*! ************************************************************************************************/
        /*!
            \brief Calculate the inverse Gaussian function.  Calculate the delta of x to b that would 
            generate this Gaussian value.  Used to determine the acceptance range given a Gaussian Value.

            Note: The delta of x to b of a Gaussian value of zero is infinity, so return the max
            capacity in this case.  The gaussianValue is expected to be between 0. and 1.0 inclusive.  Any
            values outside of this range are clamped to the max and min size of the return value;

            \param[in] gaussianValue the gaussianValue to find the delta of.
            \return The delta of x to b.
        ***************************************************************************************************/
        uint32_t inverse(float_t gaussianValue) const;

        inline uint32_t getBellWidth() const { return mBellWidth; }

    private:
        inline float_t getFitPercentFromArrayCache(uint32_t difference) const;
        inline float_t getFitPercentFromHashMapCache(uint32_t difference) const;
        inline float_t calcFitPercent(uint32_t difference) const;
    private:

        float_t mPrecalculatedVariance;
        uint32_t mBellWidth;
        CacheType mCacheType;

        // hash cache info
        typedef eastl::hash_map<uint32_t, float_t> GaussianCache;
        mutable GaussianCache mGaussianHashMap;

        // array cache info (indexed by the difference)
        float *mGaussianArray;
        uint32_t mGaussianArraySize;

    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_GAUSSIAN_FUNCTION_H
