
// runs memory tests

#include "framework/blaze.h"



#include "EASTL/map.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"

// imported from allocation.cpp
extern void initPerThreadMemTracker();
extern void shutdownPerThreadMemTracker();

namespace memtest
{
using namespace Blaze;


class MemTestHarness
{
public:
    static const size_t MAX_TEST_BUCKET = 10000;
    static const size_t MAX_MEM_ALLOCATION = 1000000000;

    MemTestHarness();
    ~MemTestHarness() { cleanup(); }
        
    void runFixedTimingTest(size_t passes, size_t allocationCount, size_t allocationSize);    
    void runRandomTimingTest(size_t passes, int32_t seed, size_t allocationCount, size_t minAllocationSize, size_t maxAllocationSize);

private:
    void runFixedTimingTestInternal(size_t allocationCount, size_t allocationSize);    
    void runRandomTimingTestInternal(int32_t seed, size_t allocationCount, size_t minAllocationSize, size_t maxAllocationSize);
    void init();
    void cleanup();

    typedef eastl::vector<eastl::string> TrackingNameArray;
    typedef eastl::vector<void*> BucketArray;
    BucketArray mBucketArray;
    TrackingNameArray mTrackingNameArray;
};


MemTestHarness::MemTestHarness()
:   mBucketArray(BlazeStlAllocator("MemBucket", MEM_GROUP_FRAMEWORK_DEFAULT)), 
    mTrackingNameArray(BlazeStlAllocator("TrackingNames", MEM_GROUP_FRAMEWORK_DEFAULT))
{    
    mBucketArray.reserve(MAX_TEST_BUCKET);
    mBucketArray.insert(mBucketArray.begin(), MAX_TEST_BUCKET, nullptr);

    init();
}

void MemTestHarness::init()
{
    srand(2147483647);
    mTrackingNameArray.reserve(10000);
    // preseed the tracking allocator with dummy allocation contexts, so that memory tracking has something to do
    for (int i = 0; i < 10000; ++i)
    {
        eastl::string& str = mTrackingNameArray.push_back();
        int trackingContextSize = 20 + rand() % 200;
        str.resize((size_t)trackingContextSize);
        str.sprintf("TrackingName %i", i);
        if (i % 1000 == 0)
            printf("tracking context: %p\n", str.c_str());
        void* data = BLAZE_ALLOC_MGID(48, DEFAULT_BLAZE_MEMGROUP, str.c_str());
        BLAZE_FREE(data);
    }
}

void MemTestHarness::runFixedTimingTest(size_t passes, size_t allocationCount, size_t allocationSize)
{
    TimeValue total = 0;

    printf("Running fixed size sequential allocation test: %d passes, %d allocations, %d bytes...\n", 
        (uint32_t) passes, (uint32_t) allocationCount, (uint32_t) allocationSize);
    for (size_t counter = 0; counter < passes; ++counter)
    {
        printf("Pass %d...", (uint32_t) counter + 1);
        TimeValue start = TimeValue::getTimeOfDay();
        runFixedTimingTestInternal(allocationCount, allocationSize);
        TimeValue elapsed = TimeValue::getTimeOfDay() - start;
        total += elapsed;

        printf("completed in %" PRId64 "us (%f seconds)\n", elapsed.getMicroSeconds(), elapsed.getMicroSeconds() / 1000000.0f);

        cleanup();
    }

    //Do the average
    total = total.getMicroSeconds() / passes;

    printf("Finished %d tests in an average of %" PRId64 "us (%f seconds) per test.\n\n", (uint32_t) passes, total.getMicroSeconds(), total.getMicroSeconds() / 1000000.0f);

}

void MemTestHarness::runFixedTimingTestInternal(size_t allocationCount, size_t allocationSize)
{
    size_t buckets = MAX_MEM_ALLOCATION / allocationSize;
    buckets = buckets > MAX_TEST_BUCKET ? MAX_TEST_BUCKET : buckets;

    size_t sweeps = allocationCount / buckets;
    for (size_t sweepCounter = 0; sweepCounter < sweeps; ++sweepCounter)
    {
        for (size_t counter = 0; counter < buckets; ++counter)
        {
            mBucketArray[counter] = BLAZE_ALLOC(allocationSize);
        }

        for (size_t counter = 0; counter < buckets; ++counter)
        {
            BLAZE_FREE(mBucketArray[counter]);
            mBucketArray[counter] = nullptr;
        }
    }
}

void MemTestHarness::runRandomTimingTest(size_t passes, int32_t seed, size_t allocationCount, size_t minAllocationSize, size_t maxAllocationSize)
{
    TimeValue total = 0;

    printf("Running random size, random order allocation test: %d passes, %d allocations, max %d bytes...\n", 
        (uint32_t) passes, (uint32_t) allocationCount, (uint32_t) maxAllocationSize);
    for (size_t counter = 0; counter < passes; ++counter)
    {
        printf("Pass %d...", (uint32_t) counter + 1);
        TimeValue start = TimeValue::getTimeOfDay();
        runRandomTimingTestInternal(seed, allocationCount, minAllocationSize, maxAllocationSize);
        TimeValue elapsed = TimeValue::getTimeOfDay() - start;
        total += elapsed;

        printf("completed in %" PRId64 "us (%f seconds)\n", elapsed.getMicroSeconds(), elapsed.getMicroSeconds() / 1000000.0f);

        cleanup();
    }

    //Do the average
    total = total.getMicroSeconds() / passes;

    printf("Finished %d tests in an average of %" PRId64 "us (%f seconds) per test.\n\n", (uint32_t) passes, total.getMicroSeconds(), total.getMicroSeconds() / 1000000.0f);

}

void MemTestHarness::runRandomTimingTestInternal(int32_t seed, size_t allocationCount, size_t minAllocationSize, size_t maxAllocationSize)
{
    //Seed the random number generator
    srand(seed);


    size_t bucketCount = MAX_TEST_BUCKET;
    for (size_t counter = 0; counter < allocationCount;)
    {
        //pick a bucket
        size_t bucket = (size_t) (double) (((double)rand() / (double) (RAND_MAX + 1.0)) * (double) bucketCount);
        void*& ptr = mBucketArray[bucket];
        if (ptr == nullptr)
        {
            size_t allocSize = (size_t) (((double)rand() / (double) (RAND_MAX + 1.0)) * (double) (maxAllocationSize - minAllocationSize + 1.0) + (double) minAllocationSize);
            ptr = BLAZE_ALLOC(allocSize);
            ++counter;
        }
        else
        {
            BLAZE_FREE(ptr);
            ptr = nullptr;
        }
    }
}

void MemTestHarness::cleanup()
{
    for (BucketArray::iterator itr = mBucketArray.begin(), end = mBucketArray.end(); itr != end; ++itr)
    {
        if (*itr != nullptr)
        {
            BLAZE_FREE(*itr);
            *itr = nullptr;
        }
    }
    
    mBucketArray.clear();
    mBucketArray.insert(mBucketArray.begin(), MAX_TEST_BUCKET, nullptr);
}

static int usage(const char8_t* prg)
{
    printf("Usage: %s fixed [passes] [allocation count] [allocation size]\n", prg);       
    printf("Usage: %s random [passes] [seed] [allocation count] [max allocation size] [min allocation size (optional)\n", prg);       
    return 1;
}

int memtest_main(int argc, char** argv)
{ 
    initPerThreadMemTracker();

    {
        MemTestHarness testHarness;

        if (argc < 2)
            return usage(argv[0]);

        if (blaze_strcmp(argv[1], "fixed") == 0)
        {
            if (argc != 5)
                return usage(argv[0]);

            size_t passes, count, size;
            blaze_str2int(argv[2], &passes);
            blaze_str2int(argv[3], &count);
            blaze_str2int(argv[4], &size);
            testHarness.runFixedTimingTest(passes, count, size);
        }
        else if (blaze_strcmp(argv[1], "random") == 0)
        {
            if (argc != 6 && argc != 7)
                return usage(argv[0]);

            size_t passes, seed, count, minSize, maxSize;
            blaze_str2int(argv[2], &passes);
            blaze_str2int(argv[3], &seed);
            blaze_str2int(argv[4], &count);        
            blaze_str2int(argv[5], &maxSize);

            if (argc == 7)
                blaze_str2int(argv[6], &minSize);
            else
                minSize = 0;

            testHarness.runRandomTimingTest(passes, seed, count, minSize, maxSize);
        }
        else
        {
            return usage(argv[0]);
        }

        printf("-- Finished --\n");
    }

    shutdownPerThreadMemTracker();


    return 0;
}

}

