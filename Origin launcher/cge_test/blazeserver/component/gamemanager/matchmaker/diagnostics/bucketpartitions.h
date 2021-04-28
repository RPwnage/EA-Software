/*! ************************************************************************************************/
/*!
    \file bucketpartitions.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKER_DIAGNOSTICS_BUCKET_PARTITIONS_H
#define BLAZE_MATCHMAKER_DIAGNOSTICS_BUCKET_PARTITIONS_H


namespace Blaze
{

namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*! \brief Helper to track bucket partitions for a range
    ****************************************************************************************************/
    class BucketPartitions
    {
    public:
        BucketPartitions() : mMinRange(0), mMaxRange(0), mPartitions(0), mNonTailBucketCapacity(0), mTailBucketCapacity(0), mDisplayDigitsPadding(0)
        {
        }

        bool initialize(int64_t minRange, int64_t maxRange, uint64_t partitions, const char8_t* logContext);
        const char8_t* getDisplayString(eastl::string& buf, int64_t preBucketValue) const;

        uint64_t getPartitions() const { return mPartitions; }
        int64_t getMinRange() const { return mMinRange; }
        int64_t getMaxRange() const { return mMaxRange; }

        bool calcBucketIndex(uint64_t& index, int64_t preBucketValue) const;
    private:
        bool validateInitialized() const;
        bool calcBucketBounds(int64_t& bucketLowerBound, int64_t& bucketUpperBound, int64_t preBucketValue) const;
        bool initBucketsCapacities(uint64_t& nonTailCapacity, uint64_t& tailCapacity, int64_t minRange, int64_t maxRange, uint64_t partitions) const;

    private:
        int64_t mMinRange;
        int64_t mMaxRange;
        uint64_t mPartitions;
        uint64_t mNonTailBucketCapacity;
        uint64_t mTailBucketCapacity;//tail bucket can be larger
        eastl::string mLogContext;
        uint64_t mDisplayDigitsPadding;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKER_DIAGNOSTICS_BUCKET_PARTITIONS_H
