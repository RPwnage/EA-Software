/*! ************************************************************************************************/
/*!
    \file bucketpartitions.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/diagnostics/bucketpartitions.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*! \param[in] partitions Number of range-partitioning buckets. All but possibly the tail bucket (which maybe larger)
    are same size. 0 or 1 uses 1 bucket.
    ***************************************************************************************************/
    bool BucketPartitions::initialize(int64_t minRange, int64_t maxRange,
        uint64_t partitions, const char8_t* logContext)
    {
        mLogContext.sprintf("[BucketPartitions:%s]", ((logContext != nullptr) ? logContext : ""));

        mMinRange = minRange;
        mMaxRange = maxRange;
        mPartitions = partitions == 0 ? 1 : partitions;

        // for convenience, auto-clamp partitions to range size
        if ((mMaxRange >= mMinRange) && ((uint64_t)(mMaxRange - mMinRange) < mPartitions - 1))
        {
            mPartitions = mMaxRange - mMinRange + 1;
        }

        if (!initBucketsCapacities(mNonTailBucketCapacity, mTailBucketCapacity,
            mMinRange, mMaxRange, mPartitions))
        {
            return false;
        }

        // validate our initialized settings
        uint64_t minBucketIndex = 0, maxBucketIndex = 0;
        if (!validateInitialized() || (EA_UNLIKELY(!calcBucketIndex(minBucketIndex, mMinRange) ||
            !calcBucketIndex(maxBucketIndex, mMaxRange))))
        {
            return false;
        }

        //+1 for potential '-' sign:
        mDisplayDigitsPadding = 1+ eastl::string().sprintf("%" PRIi64 "", abs(mMinRange) > abs(mMaxRange) ? abs(mMinRange) : abs(mMaxRange)).length();

        eastl::string buf1, buf2;
        INFO_LOG(mLogContext << ".initialize: range(" << mMinRange << "," << mMaxRange << "), partitions specified(" <<
            mPartitions << "), calculated bucket capacity(non-tail:" <<
            mNonTailBucketCapacity << ", tail:" << mTailBucketCapacity << "). Buckets: bucket" << minBucketIndex << "=" <<
            getDisplayString(buf1, mMinRange) << ",.., bucket" << maxBucketIndex << "=" << getDisplayString(buf2, mMaxRange) << ".");

        return true;
    }

    bool BucketPartitions::validateInitialized() const
    {
        if ((mMaxRange < mMinRange) || (mNonTailBucketCapacity == 0) || (mTailBucketCapacity == 0) || (mPartitions == 0))
        {
            ERR_LOG(mLogContext << ".validateInitialized: invalid/uninitialized buckets, range(" << mMinRange << "," << mMaxRange <<
                "), numBuckets(" << mPartitions << "), calculated bucket capacity(non-tail:" << mNonTailBucketCapacity <<
                ", tail:" << mTailBucketCapacity << ").");

            return false;
        }
        if (((mNonTailBucketCapacity * (mPartitions - 1)) + mTailBucketCapacity) >(size_t)(mMaxRange - mMinRange + 1))
        {
            ASSERT_LOG(mLogContext << ".validateInitialized: invalid calculated buckets capacity(" << mNonTailBucketCapacity <<
                "), for range(" << mMinRange << "," << mMaxRange << "), with numPartitions(" << mPartitions <<
                "). BucketCapacity * (numPartitions-1) = (" << (mNonTailBucketCapacity * (mPartitions - 1)) <<
                "), must be <= range magnitude(" << (mMaxRange - mMinRange + 1) << "). Diagnostics may not be updated correctly.");

            return false;
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Returns string identifying value's bucket, of form '<min>-<max>'. Used for diagnostic metric names
    ***************************************************************************************************/
    const char8_t* BucketPartitions::getDisplayString(eastl::string& buf, int64_t preBucketValue) const
    {
        int64_t bucketLowerBound = 0, bucketUpperBound = 0;
        if (!validateInitialized() || !calcBucketBounds(bucketLowerBound, bucketUpperBound, preBucketValue))
        {
            return "?";//logged
        }
        return buf.sprintf("%0*" PRIi64 "_to_%0*" PRIi64 "", mDisplayDigitsPadding, bucketLowerBound, mDisplayDigitsPadding, bucketUpperBound).c_str();
    }


    /*! ************************************************************************************************/
    /*! \brief calc index of the bucket for the input value
    ***************************************************************************************************/
    bool BucketPartitions::calcBucketIndex(uint64_t& index, int64_t preBucketValue) const
    {
        if (!validateInitialized())
        {
            return false;
        }
        if (preBucketValue > mMaxRange || preBucketValue < mMinRange)
        {
            WARN_LOG(mLogContext << ".calcBucketIndex: invalid input pre-bucket value(" << preBucketValue <<"), not in range(" <<
                mMinRange << "," << mMaxRange << "). Diagnostics may not be updated correctly.");

            return false;
        }

        // bucket index will start at 0 with most negative bucket.
        // index is distance from minRange, divided by bucket cap
        uint64_t distanceFromMin = (preBucketValue - mMinRange);
        index = (mNonTailBucketCapacity != 0 ? (uint64_t)(distanceFromMin / mNonTailBucketCapacity) : 0);

        if (index >= mPartitions)
        {
            // tail bucket that's larger than mNonTailBucketCapacity
            index = mPartitions - 1;
        }

        ASSERT_COND_LOG((index < mPartitions), mLogContext << "calcBucketIndex: calculated index(" << index << "), for pre-bucket value(" <<
            preBucketValue << "), was invalid for current bucket count(" << mPartitions << "). Was the pre-bucket value in range(" <<
            mMinRange << "," << mMaxRange << ")?  Diagnostics may not be updated correctly.");

        return (index < mPartitions);
    }

    /*! \brief calc bounds of the bucket for the input value, based on partition-count/bucket-capacity. */
    bool BucketPartitions::calcBucketBounds(int64_t& bucketLowerBound,
        int64_t& bucketUpperBound, int64_t preBucketValue) const
    {
        uint64_t bucketIndex = 0;
        if (!calcBucketIndex(bucketIndex, preBucketValue))
        {
            return false;
        }
        bucketLowerBound = mMinRange + (bucketIndex * mNonTailBucketCapacity);

        bucketUpperBound = bucketLowerBound + mNonTailBucketCapacity - 1;
        //tail bucket can be larger
        if (bucketIndex >= (mPartitions - 1))
        {
            bucketUpperBound = mMaxRange;
        }
        return true;
    }

    /*! \brief calc ranges of the buckets, based on partition count. */
    bool BucketPartitions::initBucketsCapacities(uint64_t& nonTailCapacity,
        uint64_t& tailCapacity, int64_t minRange, int64_t maxRange, uint64_t partitions) const
    {
        uint64_t rangeSize = maxRange - minRange + 1;
        if ((maxRange < minRange) || ((partitions != 0) && (partitions > rangeSize)))
        {
            ERR_LOG(mLogContext << ".initBucketsCapacities: Internal Error: invalid diagnostic range(" <<
                minRange << "," << maxRange << "), or partitions(" << partitions << ") for range magnitude=" <<
                rangeSize << "). Diagnostics may not be updated correctly.");

            return false;
        }
        nonTailCapacity = (partitions > 1 ? rangeSize / partitions : rangeSize);

        // tail bucket can be larger
        tailCapacity = (nonTailCapacity + (rangeSize - (nonTailCapacity * partitions)));
        return true;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
