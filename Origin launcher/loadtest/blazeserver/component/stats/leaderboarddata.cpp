/*************************************************************************************************/;
/*!
    \file leaderboard.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"

#include "leaderboarddata.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace Stats
{
    

/*************************************************************************************************/
/*!
    \brief getRankByEntryFull

    Helper function that looks up the entity rank data using secondary ranking stats. 
    Only valid when MultiRankData is used as the data type. 
*/
/*************************************************************************************************/
bool MultiRankCompare( const Entry<MultiRankData>& a, const Entry<MultiRankData>& b, const RankingData& data )
{      
    for( int32_t pos = 0; pos < data.rankingStatsUsed; ++pos )
    {
        bool ascending = data.isAscending(pos);
        if (data.isInt(pos))
        {
            const int64_t& valA = a.value.data[pos].intValue;
            const int64_t& valB = b.value.data[pos].intValue;
            if (valA < valB)
                return ascending;
            if (valA > valB)
                return !ascending;
        }
        else
        {
            const float& valA = a.value.data[pos].floatValue;
            const float& valB = b.value.data[pos].floatValue;
            if (valA < valB)
                return ascending;
            if (valA > valB)
                return !ascending;
        }
    }    

    return a.entityId > b.entityId;
}

template <>
uint32_t LeaderboardData<MultiRankData>::getRankByEntryFull(const Entry<MultiRankData>& entry) const 
{ 
    if (mCount == mCapacity) 
    { 
        const Entry<MultiRankData>& e = getEntryByRank(mCapacity); 
        if (MultiRankCompare(e, entry, mRankingData)) 
            return 0; 
    } 
    uint32_t index = 0; 
    uint32_t d = mCount; 
    while (d > 0) 
    { 
        uint32_t d2 = d >> 1; 
        uint32_t i = index + d2; 
        const Entry<MultiRankData>& e = getEntryByRank(i + 1); 
        if (MultiRankCompare(e, entry, mRankingData)) 
        { 
            index = ++i; 
            d -= d2 + 1; 
        } 
        else 
            d = d2; 
    } 
    return index + 1; 
}

/*************************************************************************************************/
/*!
    \brief getRankByEntry

    Helper function that calls the appropriate version based on the Ranking data.  
*/
/*************************************************************************************************/
template <>
uint32_t LeaderboardData<MultiRankData>::getRankByEntry(const Entry<MultiRankData>& entry) const
{
    EA_ASSERT (mRankingData.rankingStatsUsed > 1);
    return getRankByEntryFull(entry);
}

} // Stats
} // Blaze
