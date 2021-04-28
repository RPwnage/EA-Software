/*************************************************************************************************/;
/*!
    \file leaderboard.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_LEADERBOARDDATA_H
#define BLAZE_STATS_LEADERBOARDDATA_H

/*************************************************************************************************/
/*!
    \class LeaderboardData

    This class which encapsulates a single leaderboard cached in memory borrows much inspiration
    from the eastl deque, but optimizes insertions and removals by using circular subarrays to
    minimze the cost of memory shifting.

    In order to ensure consistency, and allow fast lookups, leaderboard caches will be ordered
    first by stat value, then by entity id.  We arbitrarily choose to sort by descending entity id.
    
    The LeaderboardData class deals with both ranks and indicies.  Ranks are the human readable
    positions within leaderboards, and thus 1-based, with the best entry being ranked 1, and an
    unranked entry being 0.  Indicies are simply rank - 1, where of course unranked entries are
    not actually contained thus we never have to be concerned with a negative index.  Logically
    one could think of the index as the location in a single array of entries, though in actuality
    the data is managed in smaller chunks by the SubArray class.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SubArray

    This class implements a circular array for use exclusively by the LeaderboardData class.
    The chief raison d'etre is to provide an efficient mechanism for inserting or removing entries
    especially in large leaderboards.

    This class does not know (or care) about the ordering of the data, it is entirely up to the
    caller to do the right thing, and maintain ordered data when inserting or otherwise modifying
    the data that a SubArray object contains.  That means the caller is responsible for never
    popping data from an empty SubArray, nor filling one beyond capacity.  It should be considered
    solely a helper class which provides a memory and performance efficient mechanism for managing
    a subset of the larger data collection which is owned by the LeaderboardData object.

    The methods on the SubArray class in general accept circular index arguments, that is to say,
    where to insert, remove, or update elements, is specified as an offset from the head, wrapping
    around to the beginning of the raw physical array if necessary.
*/
/*************************************************************************************************/

#include "framework/tdf/entity.h"

#include "EASTL/hash_map.h"
#include "stats/maxrankingstats.h"
#include "stats/statsslaveimpl.h"

// For leaderboards larger than the default subarray size, they will be chunked into multiple
// subarrays each of default subarray size (except potentially the last one which will be sized to fit).
//
// The value 4k was selected based on tuning tests using the unit test code in
// component/stats/test/test_leaderboarddata_tuning.cpp.  For most typical leaderboards in the
// 100 - 10,000 size range, this parameter is largely irrelevant as other overhead is more
// significant than subarray size.  For larger leaderboards, the selection of size becomes important.
// The size of 4k was selected because it is the optimal sweet spot for populating large leaderboards
// on the order of a million entries on server startup.  Any larger, and the memmoves to shift
// large amounts of entries slows things down, and any smaller the number of shuffles between
// subarrays becomes the limiting factor.
//
// It is not recommended that Blaze customers configure leaderboards any larger than one million
// entries without consulting with GOS.  Additional tuning tests will likely need to be done to
// determine the sweet spot.  It is likely that something on the order of 4 * sqrt(configuredSize)
// would be a good starting point.  Simply choosing the square root would balance off the number of
// shuffles between subarrays versus the amount of entries to be memmoved within subarrays, but the
// memmoves can be implemented more efficiently, so we bias towards somewhat larger subarray sizes
// than the square root to compensate.
#ifndef DEFAULT_SUB_ARRAY_SIZE
#define DEFAULT_SUB_ARRAY_SIZE 4096
#endif

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace Stats
{
    
template <typename T>
struct Entry
{
    EntityId entityId;
    T value;
};

struct MultiRankData
{
    MultiRankData(int unused=0)
    {
        memset(this, 0, sizeof(MultiRankData));
    }
    bool operator!=(const MultiRankData &rhs) const
    {
        return memcmp(this, &rhs, sizeof(MultiRankData)) != 0;
    }
    bool operator==(const MultiRankData &rhs) const
    {
        return memcmp(this, &rhs, sizeof(MultiRankData)) == 0;
    }
    union LeaderboardValue
    {
        float floatValue;
        int64_t intValue;
    };
    LeaderboardValue data[MAX_RANKING_STATS];
};

struct RankingData
{
    RankingData() : rankingStatsUsed(0), ascendingIsIntBitset(0) {}

    int8_t rankingStatsUsed; 
    int8_t ascendingIsIntBitset;       // 4 bits ascending, 4 bits is int

    static const int32_t FIRST_ASCENDING_BIT = 1;
    static const int32_t FIRST_IS_INT_BIT = 16;

    // Only valid up to MAX_RANKING_STATS-1
    inline bool isAscending(int32_t val) const { return (ascendingIsIntBitset & (FIRST_ASCENDING_BIT << val)) != 0; }
    inline bool isInt(int32_t val) const { return (ascendingIsIntBitset & (FIRST_IS_INT_BIT << val)) != 0; }

    inline void setAscendingIsInt( bool ascending, bool isInt, int32_t pos )
    {
        ascendingIsIntBitset |= (ascending ? RankingData::FIRST_ASCENDING_BIT : 0) << pos; 
        ascendingIsIntBitset |= (isInt ? RankingData::FIRST_IS_INT_BIT : 0) << pos;
    }

};

/*************************************************************************************************/
/*!
    \brief UpdateLeaderboardResult

    Contains the result of an in memory leaderboard cache update attempt
        that resulted from a stat update 
*/
/*************************************************************************************************/
struct UpdateLeaderboardResult
{
    enum RankedStatus
    {
        NOT_RANKED, // was not ranked before or after the update
        NO_LONGER_RANKED, //was ranked before the update, but not after
        NEWLY_RANKED, // was not ranked before the udpate, but is after
        STILL_RANKED, //was ranked before and after the update operation
        NONE
    };

    UpdateLeaderboardResult()
        : mStatus(NONE), mIsStatRemoved(false), mRemovedEntityId(0)
    {}

    RankedStatus mStatus;
    bool mIsStatRemoved; //true if a stat was bumped out of the leaderboard
    EntityId mRemovedEntityId; // the entityId of any stats bumped out of the leaderboard
};

// This function is only used by unit tests
template <typename T>
inline bool operator==(const Entry<T>& lhs, const Entry<T>& rhs)
{
    return ((lhs.entityId == rhs.entityId) && (lhs.value == rhs.value));
}

template <typename T>
class SubArray
{
public:
    SubArray(uint32_t capacity);
    ~SubArray();

    void reset();
    void remove(uint32_t index);
    void insert(uint32_t index, const Entry<T>& entry);
    void update(uint32_t srcIndex, uint32_t dstIndex, const Entry<T>& entry);

    inline const Entry<T>& getEntry(uint32_t index) const { return mData[(mHead + index) % mCapacity]; }
    inline void update(uint32_t index, const Entry<T>& entry) { mData[(mHead + index) % mCapacity] = entry; }

    Entry<T> popHead();
    Entry<T> popTail();
    void insertHead(const Entry<T>& entry);
    void insertTail(const Entry<T>& entry);


private:

    void shiftTowardsHead(uint32_t begin, uint32_t end);
    void shiftTowardsTail(uint32_t begin, uint32_t end);

    inline void incrHead() { ++mHead; if (EA_UNLIKELY(mHead == mCapacity)) mHead = 0; }
    inline void incrTail() { ++mTail; if (EA_UNLIKELY(mTail == mCapacity)) mTail = 0; }
    inline void decrHead() { (EA_UNLIKELY(mHead == 0)) ? mHead = mCapacity - 1 : --mHead; }
    inline void decrTail() { (EA_UNLIKELY(mTail == 0)) ? mTail = mCapacity - 1 : --mTail; }
    inline void resetHead() { mHead = 0; }
    inline void resetTail() { mTail = 0; }

    uint32_t mCount;
    uint32_t mCapacity;
    Entry<T>* mData;
    uint32_t mHead;
    uint32_t mTail;
};

template <typename T>
class LeaderboardData
{
public:
    LeaderboardData(uint32_t maxSize, uint32_t extraRows);
    ~LeaderboardData();

    // Set the ranking data before using this structure
    void setRankingData(const RankingData& rankingData) { mRankingData = rankingData; }

    // Though we may have more internal entries, external callers are only interested in the
    // count of entries up to the configured capacity, any extra rows are for internal purposes only
    inline uint32_t getCount() const { return (mCount > mConfiguredSize ? mConfiguredSize : mCount); }

    // When syncing leaderboards from master to slave on server startup, we need to know the full
    // size of the cache including extra rows, these methods should not typically be used otherwise
    inline uint32_t getTotalCount() const { return mCount; }
    inline uint32_t getCapacity() const { return mCapacity; }

    bool update(const Entry<T>& entry, UpdateLeaderboardResult& result);
    void remove(EntityId entityId, UpdateLeaderboardResult& result);
    void reset();

    inline void populate(const Entry<T>& entry) { mEntityMap[entry.entityId] = entry.value; mSubArrays[(mCount++)/mSubArraySize]->insertTail(entry); }

    uint32_t getRank(EntityId entityId) const;
    inline const Entry<T>& getEntryByRank(uint32_t rank) const { return (mSubArrays[(rank-1)/mSubArraySize]->getEntry((rank-1)%mSubArraySize)); }
    bool contains(EntityId entityId) const;

    uint64_t getMemoryBudget() const;
    uint64_t getMemoryAllocated() const;

private:

    // We can't simply use a preallocation system here due to the large amounts of LBs used by titles like FIFA.
    typedef eastl::hash_map<EntityId, T> EntityMap;

    uint32_t getRankByEntry(const Entry<T>& entry) const;
    uint32_t getRankByEntryAscending(const Entry<T>& entry) const;
    uint32_t getRankByEntryDescending(const Entry<T>& entry) const;
    uint32_t getRankByEntryFull(const Entry<T>& entry) const;
    uint32_t getRankByEntity(EntityId entityId, typename EntityMap::iterator* entityIter);
    void removeByIndex(uint32_t index);

    RankingData mRankingData;

    uint32_t mCount;
    uint32_t mConfiguredSize;
    uint32_t mCapacity;

    uint32_t mSubArraySize;
    uint32_t mNumSubArrays;
    SubArray<T>** mSubArrays;

    EntityMap mEntityMap;
};

/*************************************************************************************************/
/*!
    \brief SubArray

    Construct a new SubArray.

    \param[in]  capacity - maximum number of entries this object can hold
*/
/*************************************************************************************************/
template <typename T>
SubArray<T>::SubArray(uint32_t capacity)
: mCount(0), mCapacity(capacity), mHead(0), mTail(0)
{
    mData = BLAZE_NEW_ARRAY_STATS_LEADERBOARD(Entry<T>, mCapacity);
}

/*************************************************************************************************/
/*!
    \brief ~SubArray

    Clean up any resources owned by this object.
*/
/*************************************************************************************************/
template <typename T>
SubArray<T>::~SubArray()
{
    delete[] mData;
}

/*************************************************************************************************/
/*!
    \brief shiftTowardsHead

    Perform a circular shift of a specified range of entries towards the head.  Net effect is
    to shift the elements from indicies (begin, end] to [begin, end).  Note that the indicies
    passed in are physical indicies within the array, not relative to the head.

    \param[in]  begin - the absolute index within the array of entries of the location to shift to
    \param[in]  end - the absolute index within the array of entries of the last element to shift from
*/
/*************************************************************************************************/
template <typename T>
void SubArray<T>::shiftTowardsHead(uint32_t begin, uint32_t end)
{
    if (begin < end)
    {
        memmove(mData + begin, mData + begin + 1, (end - begin) * sizeof(*mData));
    }
    else
    {
        memmove(mData + begin, mData + begin + 1, ((mCapacity - 1) - begin) * sizeof(*mData));
        mData[mCapacity - 1] = mData[0];
        memmove(mData, mData + 1, end * sizeof(*mData));
    }
}

/*************************************************************************************************/
/*!
    \brief shiftTowardsTail

    Perform a circular shift of a specified range of entries towards the tail.  Net effect is
    to shift the elements from indicies [begin, end) to (begin, end].  Note that the indicies
    passed in are physical indicies within the array, not relative to the head.

    \param[in]  begin - the absolute index within the array of entries of the location to shift from
    \param[in]  end - the absolute index within the array of entries of the last element to shift to
*/
/*************************************************************************************************/
template <typename T>
void SubArray<T>::shiftTowardsTail(uint32_t begin, uint32_t end)
{
    if (begin < end)
    {
        memmove(mData + begin + 1, mData + begin, (end - begin) * sizeof(*mData));
    }
    else
    {
        memmove(mData + 1, mData, end * sizeof(*mData));
        mData[0] = mData[mCapacity - 1];
        memmove(mData + begin + 1, mData + begin, ((mCapacity - 1) - begin) * sizeof(*mData));
    }
}

/*************************************************************************************************/
/*!
    \brief reset

    Reset the array to the initial state.  Instead of removing all elements in the array, we 
    reset the head, the tail and the count.
*/
/*************************************************************************************************/
template <typename T>
void SubArray<T>::reset()
{
    resetHead();
    resetTail();
    mCount = 0;
}

/*************************************************************************************************/
/*!
    \brief remove

    Remove a single element from the array.  The element to be removed is specified by an offset
    from the head.  To maintain a contiguous circular array of entries with the least amount
    of memory moving, if the element to be removed is nearer the head we shift the elements in
    front of it one entry towards the tail, otherwise we shift the elements behind it towards the head.

    \param[in]  index - the circular index which is to be removed
*/
/*************************************************************************************************/
template <typename T>
void SubArray<T>::remove(uint32_t index)
{
    if (EA_UNLIKELY(index == 0))
    {
        incrHead();
    }
    else if (EA_UNLIKELY(index + 1 == mCount))
    {
        decrTail();
    }
    else
    {
        if (index > (mCount / 2))
        {
            decrTail();
            shiftTowardsHead((mHead + index) % mCapacity, mTail);
        }
        else
        {
            shiftTowardsTail(mHead, (mHead + index) % mCapacity);
            incrHead();
        }
    }
    --mCount;
}

/*************************************************************************************************/
/*!
    \brief insert

    Insert a single element into the array.  The location of the element to be inserted is
    specified by an offset from the head.  To maintain a contiguous circular array of entries
    with the least amount of memory moving, if the element to be inserted is nearer the head we
    shift the elements in front of it one entry towards the head, otherwise we shift the elements
    behind it towards the tail.

    \param[in]  index - the circular index where the insert is to occur
    \param[in]  entry - the entry to insert into the array
*/
/*************************************************************************************************/
template <typename T>
void SubArray<T>::insert(uint32_t index, const Entry<T>& entry)
{
    if (EA_UNLIKELY(index == 0))
    {
        decrHead();
        mData[mHead] = entry;
    }
    else if (EA_UNLIKELY(index == mCount))
    {
        mData[mTail] = entry;
        incrTail();
    }
    else
    {
        if (index > mCount / 2)
        {
            shiftTowardsTail((mHead + index) % mCapacity, mTail);
            incrTail();
        }
        else
        {
            decrHead();
            shiftTowardsHead(mHead, (mHead + index) % mCapacity);
        }

        mData[(mHead + index) % mCapacity] = entry;
    }

    ++mCount;
}

/*************************************************************************************************/
/*!
    \brief update

    Update an entry and shuffle it to another location within the array, shifting other entries
    as needed.  The source and destination locations of the element being moved are specified by
    offsets from the head.

    \param[in]  srcIndex - the circular index of the element being updated
    \param[in]  dstIndex - the circular index to put the updated element
    \param[in]  entry - the value to update the moved entry to
*/
/*************************************************************************************************/
template <typename T>
void SubArray<T>::update(uint32_t srcIndex, uint32_t dstIndex, const Entry<T>& entry)
{
    if ( ((dstIndex > srcIndex) && ((dstIndex - srcIndex) > (mCount / 2)))
        || ((srcIndex > dstIndex) && ((srcIndex - dstIndex) > (mCount / 2))))
    {
        // If the elements are far apart, faster to do a delete then insert
        remove(srcIndex);
        insert(dstIndex, entry);
    }
    else
    {
        if (dstIndex > srcIndex)
            shiftTowardsHead((mHead + srcIndex) % mCapacity, (mHead + dstIndex) % mCapacity);
        else
            shiftTowardsTail((mHead + dstIndex) % mCapacity, (mHead + srcIndex) % mCapacity);
        mData[(mHead + dstIndex) % mCapacity] = entry;
    }
}

/*************************************************************************************************/
/*!
    \brief popHead

    Pop an entry off the head of the array.

    \return - the entry popped off the head
*/
/*************************************************************************************************/
template <typename T>
Entry<T> SubArray<T>::popHead()
{
    --mCount;
    uint32_t index = mHead;
    incrHead();
    return mData[index];
}

/*************************************************************************************************/
/*!
    \brief popTail

    Pop an entry off the tail of the array.

    \return - the entry popped off the tail
*/
/*************************************************************************************************/
template <typename T>
Entry<T> SubArray<T>::popTail()
{
    --mCount;
    decrTail();
    return mData[mTail];
}

/*************************************************************************************************/
/*!
    \brief insertHead

    Insert a single element into the array at the head.

    \param[in]  entry - the entry to insert
*/
/*************************************************************************************************/
template <typename T>
void SubArray<T>::insertHead(const Entry<T>& entry)
{
    ++mCount;
    decrHead();
    mData[mHead] = entry;
}

/*************************************************************************************************/
/*!
    \brief insertTail

    Insert a single element into the array at the tail.

    \param[in]  entry - the entry to insert
*/
/*************************************************************************************************/
template <typename T>
void SubArray<T>::insertTail(const Entry<T>& entry)
{
    ++mCount;
    mData[mTail] = entry;
    incrTail();
}

/*************************************************************************************************/
/*!
    \brief LeaderboardData

    Construct a new LeaderboardData.

    \param[in]  configuredSize - configured maximum number of entries this object can hold
    \param[in]  extraRows - additional entries this object can hold
    \param[in]  ascending - true if this leaderboard should be sorted ascending, false otherwise
*/
/*************************************************************************************************/
template <typename T>
LeaderboardData<T>::LeaderboardData(uint32_t configuredSize, uint32_t extraRows)
: mCount(0),
  mConfiguredSize(configuredSize),
  mCapacity(configuredSize + extraRows)
{
    mSubArraySize = DEFAULT_SUB_ARRAY_SIZE;

    // For small leaderboards, reduce the subarray size to exactly match the capacity
    if (mCapacity < DEFAULT_SUB_ARRAY_SIZE)
        mSubArraySize = mCapacity;

    // TBD For very large leaderboards we'll probably want to increase the subarray
    // size depending on the results of tuning

    // All but the last sub array will have the same size, which will make it easy for us to
    // do a division to find the subarray a given rank belongs to
    mNumSubArrays = mCapacity / mSubArraySize;
    if (mCapacity % mSubArraySize > 0)
        ++mNumSubArrays;

    mSubArrays = BLAZE_NEW_ARRAY_STATS_LEADERBOARD (SubArray<T>*, mNumSubArrays);
    for (uint32_t i = 0; i < mNumSubArrays; ++i)
    {
        // Last subarray may have a smaller size, all others have the common configured size
        if (mNumSubArrays == (i + 1))
            mSubArrays[i] = BLAZE_NEW_STATS_LEADERBOARD SubArray<T>(mCapacity - ((mNumSubArrays - 1) * mSubArraySize));
        else
            mSubArrays[i] = BLAZE_NEW_STATS_LEADERBOARD SubArray<T>(mSubArraySize);
    }
}

/*************************************************************************************************/
/*!
    \brief ~LeaderboardData

    Clean up any resources owned by this object.
*/
/*************************************************************************************************/
template <typename T>
LeaderboardData<T>::~LeaderboardData()
{
    for (uint32_t i = 0; i < mNumSubArrays; ++ i)
        delete mSubArrays[i];
    delete[] mSubArrays;
}

/*************************************************************************************************/
/*!
    \brief update

    Update an entry in the leaderboard.  This is the main method that will be called when a stat
    update is peformed.  This method needs to consider four possibilities: first, the entity
    was not deserving of being ranked before or after the update; second, the entity used to be
    ranked but now falls out of the leaderboard; third, the entity was not previously ranked
    but is now deserving of a ranking; and fourth, the entity is currently in the leaderboard and
    moves higher or lower within the leaderboard (or potentially stays in exactly the same place).

    This method returns a simple boolean flag to indicate whether the user is in the leaderboard
    or not following the update, to allow the caller to determine whether they should cache
    additional info for this user or not.

    \param[in]  entry - the entry to update

    \return - true if the user is added to or remains in the leaderboard, false otherwise
*/
/*************************************************************************************************/
template <typename T>
bool LeaderboardData<T>::update(const Entry<T>& entry, UpdateLeaderboardResult& result)
{
    typename EntityMap::iterator entityIter = mEntityMap.end();
    uint32_t currRank = getRankByEntity(entry.entityId, &entityIter);
    uint32_t nextRank = getRankByEntry(entry);

    // Not previously ranked nor good enough to be ranked now
    if (currRank == 0 && nextRank == 0)
    {
        result.mStatus = UpdateLeaderboardResult::NOT_RANKED;
        return false;
    }

    // Dropping out of the leaderboard
    if (nextRank == 0)
    {
        uint32_t index = currRank - 1;
        mEntityMap.erase(entityIter);
        removeByIndex(index);
        result.mStatus = UpdateLeaderboardResult::NO_LONGER_RANKED;
        return false;
    }

    // Earning entry into the leaderboard
    if (currRank == 0)
    {
        result.mStatus = UpdateLeaderboardResult::NEWLY_RANKED;
        uint32_t index = nextRank - 1;
        mEntityMap[entry.entityId] = entry.value;

        // Note that if mCount is exactly a multiple of mSubArraySize, the last
        // subarray will in fact point to the first empty subarray, which is
        // normally what we want, but this computation may point us past the last
        // subarray so need to check for that
        uint32_t currSubArray = index / mSubArraySize;
        uint32_t lastSubArray = mCount / mSubArraySize;
        if (lastSubArray >= mNumSubArrays)
            lastSubArray = mNumSubArrays - 1;

        // If we are completely full, the worst entry is booted right out of the leaderboard
        if (mCount == mCapacity)
        {
            const Entry<T>& popped = mSubArrays[lastSubArray]->popTail();
            mEntityMap.erase(popped.entityId);
            result.mIsStatRemoved = true;
            result.mRemovedEntityId = popped.entityId;
        }
        else
        {
            ++mCount;
        }

        // Shuffle entries towards the end of the leaderboard if needed to make room
        for (uint32_t subArray = lastSubArray; subArray > currSubArray; --subArray)
            mSubArrays[subArray]->insertHead(mSubArrays[subArray - 1]->popTail());

        // Insert the new entry
        mSubArrays[currSubArray]->insert(index % mSubArraySize, entry);

        return true;
    }

    // At this point we know that the entry was previously in the leaderboard, and will
    // still be there later, now it is a matter of updating the entity map and making the move,
    // in any case first thing we can do is update the entity map
    result.mStatus = UpdateLeaderboardResult::STILL_RANKED;
    entityIter->second = entry.value;

    // If new rank is worse than current rank, remember we have not removed current entry yet,
    // so subtract one from next rank to take into account
    // the sliding up of all entries lower than current rank
    if (currRank < nextRank)
        --nextRank;

    uint32_t currIndex = currRank - 1;
    uint32_t nextIndex = nextRank - 1;

    // Stat may have changed even if rank remains the same
    if (currIndex == nextIndex)
    {
        mSubArrays[currIndex/mSubArraySize]->update(currIndex % mSubArraySize, entry);
        return true;
    }

    // At this point rank is changing, requires a shift of elements
    uint32_t currSubArray = currIndex / mSubArraySize;
    uint32_t nextSubArray = nextIndex / mSubArraySize;

    if (currSubArray == nextSubArray)
    {
        // In this case rank change merely requires a shuffling within a single subarray
        mSubArrays[currSubArray]->update(currIndex % mSubArraySize, nextIndex % mSubArraySize, entry);
    }
    else 
    {
        // Old rank and new are not in the same subarray, so yank old entry,
        // shuffle the entries in the interleaving subarrays,
        // this will free up space in the target subArray,
        // then finally insert the updated entry where it belongs
        mSubArrays[currSubArray]->remove(currIndex % mSubArraySize);
        if (currIndex > nextIndex)
        {
            for (uint32_t subArray = currSubArray; subArray > nextSubArray; --subArray)
                mSubArrays[subArray]->insertHead(mSubArrays[subArray - 1]->popTail());
        }
        else
        {
            for (uint32_t subArray = currSubArray; subArray < nextSubArray; ++subArray)
                mSubArrays[subArray]->insertTail(mSubArrays[subArray + 1]->popHead());
        }
        mSubArrays[nextSubArray]->insert(nextIndex % mSubArraySize, entry);
    }

    return true;
}

/*************************************************************************************************/
/*!
    \brief reset

    Reset the leaderboard data.
*/
/*************************************************************************************************/
template <typename T>
void LeaderboardData<T>::reset()
{
    for (uint32_t i = 0; i < mNumSubArrays; ++ i)
        mSubArrays[i]->reset();

    mCount = 0;
    mEntityMap.clear(true);
}

/*************************************************************************************************/
/*!
    \brief remove

    Remove an entry from the leaderboard.  This is the main method that will be called when a stat
    delete is peformed.

    \param[in]  entityId - the entity id owning the entry to remove
*/
/*************************************************************************************************/
template <typename T>
void LeaderboardData<T>::remove(EntityId entityId, UpdateLeaderboardResult& result)
{
    typename EntityMap::iterator entityIter = mEntityMap.end();
    uint32_t rank = getRankByEntity(entityId, &entityIter);
    if (rank == 0)
    {
        result.mStatus = UpdateLeaderboardResult::NOT_RANKED;
        return;
    }

    mEntityMap.erase(entityIter);
    removeByIndex(rank - 1);
    result.mStatus = UpdateLeaderboardResult::NO_LONGER_RANKED;
}

/*************************************************************************************************/
/*!
    \brief removeByIndex

    Helper method to remove by index.  An entity may need to be removed either when a deleteStats
    call is issued, or when a user's stats are updated and no longer good enough to be in the
    leaderboard.

    \param[in]  index - the index of the entry to remove
*/
/*************************************************************************************************/
template <typename T>
void LeaderboardData<T>::removeByIndex(uint32_t index)
{
    uint32_t subArray = index / mSubArraySize;
    mSubArrays[subArray]->remove(index % mSubArraySize);

    // Shuffle the remaining entries up to fill the gap, decrease the size first so that we
    // correctly compute the lastSubArray
    uint32_t lastSubArray = (--mCount / mSubArraySize);
    for ( ; subArray < lastSubArray; ++subArray)
        mSubArrays[subArray]->insertTail(mSubArrays[subArray + 1]->popHead());
}

/*************************************************************************************************/
/*!
    \brief getRankByEntry

    Lookup the rank associated with a stat value and entity id.  This method locates either an
    existing entry if there is an exact match, or tells us where the passed in argument should be
    ranked if not already there.  A return value of 0 indicates that the entry is not good enough
    to be ranked at all.

    Unlike a generic binary search, we assume that there is a significant likelihood that a given
    entity will not be ranked at all, and a major use-case of this method is looking for the
    insertion point of an entity when their stats are updated.  We thus check first if the
    entity is good enough to even be ranked at all first, rather than doing a full lg(N)
    operations to determine this.  The remainder of the algorithm is lifted nearly verbatim
    from eastl, the extra verbosity required to define the necessary items to allow us to use
    the eastl call itself would be unjustified.  We use the same bit-shift by 1 trick that
    eastl uses, as apparently some compilers do not optimize a division by 2 very well.

    Note that there is a reasonable enough performance gain that we implement two versions of the
    binary search, one for ascending leaderboards and one for descending.  This allows us to
    perform a boolean check once for ascending or descending order outside of the binary search
    method, rather than performing a boolean check lg(N) times inside.  In order to avoid
    copy-paste errors where one method gets modified and the other doesn't, we implement this
    method as one long macro, and then instantiate it twice.

    \param[in]  entry - the entry to lookup

    \return - the rank
*/
/*************************************************************************************************/
#define DEFINE_GET_RANK_BY_ENTRY(NAME, OP) template <typename T> \
uint32_t LeaderboardData<T>::getRankByEntry##NAME(const Entry<T>& entry) const \
{ \
    if (mCount == mCapacity) \
    { \
        const Entry<T>& e = getEntryByRank(mCapacity); \
        if (e.value OP entry.value || (e.value == entry.value && e.entityId > entry.entityId)) \
            return 0; \
    } \
    uint32_t index = 0; \
    uint32_t d = mCount; \
    while (d > 0) \
    { \
        uint32_t d2 = d >> 1; \
        uint32_t i = index + d2; \
        const Entry<T>& e = getEntryByRank(i + 1); \
        if (e.value OP entry.value || (e.value == entry.value && e.entityId > entry.entityId)) \
        { \
            index = ++i; \
            d -= d2 + 1; \
        } \
        else \
            d = d2; \
    } \
    return index + 1; \
}

DEFINE_GET_RANK_BY_ENTRY (Ascending, <)
DEFINE_GET_RANK_BY_ENTRY (Descending, >)

/*************************************************************************************************/
/*!
    \brief getRankByEntryFull

    Helper function that looks up the entity rank data using secondary ranking stats. 
    Only valid when MultiRankData is used as the data type. 
*/
/*************************************************************************************************/
template <>
uint32_t LeaderboardData<MultiRankData>::getRankByEntryFull(const Entry<MultiRankData>& entry) const;

/*************************************************************************************************/
/*!
    \brief getRankByEntry

    Helper function that calls the appropriate version based on the Ranking data.  
*/
/*************************************************************************************************/
template <typename T>
uint32_t LeaderboardData<T>::getRankByEntry(const Entry<T>& entry) const
{
    EA_ASSERT (mRankingData.rankingStatsUsed <= 1);
    return mRankingData.isAscending(0) ? getRankByEntryAscending(entry) : getRankByEntryDescending(entry);
}

template <>
uint32_t LeaderboardData<MultiRankData>::getRankByEntry(const Entry<MultiRankData>& entry) const;

/*************************************************************************************************/
/*!
    \brief getRankByEntity

    Lookup the rank associated with an entity id.  If the entity id in question is not currently
    in this ranklist, 0 is returned.  Also allows the caller to pass in a pointer to get back
    an iterator so that they have direct access to the located entity map entry and do not need
    to do another lookup later.

    \param[in]  entity - the entity to lookup
    \param[out] entityIter - iterator that refers to the entry in the entity map

    \return - the rank
*/
/*************************************************************************************************/
template <typename T>
uint32_t LeaderboardData<T>::getRankByEntity(EntityId entityId, typename EntityMap::iterator* entityIter)
{
    typename EntityMap::iterator iter = mEntityMap.find(entityId);

    // If the caller wants the iterator, pass it back to them
    if (entityIter != nullptr)
        *entityIter = iter;

    if (iter == mEntityMap.end())
        return 0;

    Entry<T> entry;
    entry.entityId = entityId;
    entry.value = iter->second;
    return getRankByEntry(entry);
}

/*************************************************************************************************/
/*!
    \brief getRank

    Lookup the rank associated with an entity id, only for entries in the configured size of
    the leaderboard.  If the entity id in question is not currently in this ranklist,
    or is located in the extra rows, 0 is returned.

    \param[in]  entity - the entity to lookup

    \return - the rank
*/
/*************************************************************************************************/
template <typename T>
uint32_t LeaderboardData<T>::getRank(EntityId entityId) const
{
    typename EntityMap::const_iterator iter = mEntityMap.find(entityId);
    if (iter == mEntityMap.end())
        return 0;

    Entry<T> entry;
    entry.entityId = entityId;
    entry.value = iter->second;
    uint32_t rank = getRankByEntry(entry);
    if (rank > mConfiguredSize)
        return 0;
    return rank;
}

/*************************************************************************************************/
/*!
    \brief contains

    Lookup whether an entity id is in this ranklist, regardless of configured size.

    \param[in]  entity - the entity to lookup

    \return - whether the entity is in this ranklist
*/
/*************************************************************************************************/
template <typename T>
bool LeaderboardData<T>::contains(EntityId entityId) const
{
    typename EntityMap::const_iterator iter = mEntityMap.find(entityId);
    return (iter != mEntityMap.end());
}

/*************************************************************************************************/
/*!
    \brief getMemoryBudget

    Returns the approximate amount of memory required for this leaderboard (for reporting purposes).

    \return - the estimated amount of memory needed for the sub arrays and entity map
*/
/*************************************************************************************************/
template <typename T>
uint64_t LeaderboardData<T>::getMemoryBudget() const
{
    // since the last sub array may have a smaller size, the reported amount is not quite exact (but close enough)
    // and there is also some neglible overhead with SubArray<T> and eastl::hash_map
    return (uint64_t) ((mSubArraySize * mNumSubArrays * sizeof(Entry<T>)) + (mCapacity * sizeof(typename EntityMap::node_type)));
}

/*************************************************************************************************/
/*!
    \brief getMemoryAllocated

    Returns the currently allocated amount of memory for this leaderboard (for reporting purposes).

    \return - the amount of memory allocated to the sub arrays and entity map
*/
/*************************************************************************************************/
template <typename T>
uint64_t LeaderboardData<T>::getMemoryAllocated() const
{
    // since the last sub array may have a smaller size, the reported amount is not quite exact (but close enough)
    // and there is also some neglible overhead with SubArray<T> and eastl::hash_map
    return (uint64_t) ((mSubArraySize * mNumSubArrays * sizeof(Entry<T>)) + (mCount * sizeof(typename EntityMap::node_type)));
}

} // Stats
} // Blaze
#endif 
