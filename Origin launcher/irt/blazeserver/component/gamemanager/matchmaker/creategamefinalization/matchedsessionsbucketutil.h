/*! ************************************************************************************************/
/*!
    \file   matchedsessionsbucketutil.h

    \brief Util helpers for working with matched session buckets and matched sessions bucket map

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHED_SESSIONS_BUCKET_UTIL
#define BLAZE_MATCHED_SESSIONS_BUCKET_UTIL

#include "gamemanager/matchmaker/matchmakingsession.h"
#include "gamemanager/matchmaker/creategamefinalization/matchedsessionsbucket.h"
#include "gamemanager/matchmaker/rules/teamuedbalancerule.h"

namespace Blaze
{

namespace GameManager
{

namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*! \brief find the next available session in the bucket that wasn't already 'tried' for the team and
        pick number, in this finalization sequence of picks.
        Also auto-skips any session detected as un-joinable for the team (due to size/role etc).

        \param[in,out] iter PRE: iter points to location in the bucket.
            Note: starts *at* the iter. If the iter already points to a valid session, uses that.
        \param[in] pullingSession The matchmaking session which owns this rule and is attempting the finalization.
    ***************************************************************************************************/
    inline bool advanceBucketIterToNextJoinableSessionForTeam(MatchedSessionsBucket::BucketItemVector::const_iterator& iter,
        const MatchedSessionsBucket& owningBucket, const CreateGameFinalizationTeamInfo& teamInfo,
        const MatchmakingSession &autoJoinSession, const MatchmakingSession& pullingSession)
    {
        for (; iter != owningBucket.mItems.end(); ++iter)
        {
            const MatchmakingSession* session = (iter)->getMmSession();
            if (session != nullptr) 
            {
                const TeamIdSizeList* myTeamIds = session->getCriteria().getTeamIdSizeListFromRule();
                if (myTeamIds == nullptr)
                    continue;

                // Check all the teams for the given session:
                TeamIdSizeList::const_iterator teamIdSize = myTeamIds->begin();
                TeamIdSizeList::const_iterator teamIdSizeEnd = myTeamIds->end();
                for (; teamIdSize != teamIdSizeEnd; ++teamIdSize)
                {
                    TeamId teamId = teamIdSize->first;
                    if (pullingSession.isSessionJoinableForTeam(*session, teamInfo, autoJoinSession, teamId))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    inline bool advanceBucketIterToNextJoinableSessionForTeam(MatchedSessionsBucket::BucketItemVector::const_reverse_iterator& iter,
        const MatchedSessionsBucket& owningBucket, const CreateGameFinalizationTeamInfo& teamInfo, 
        const MatchmakingSession &autoJoinSession, const MatchmakingSession& pullingSession)
    {
        for (; iter != owningBucket.mItems.rend(); ++iter)
        {
            const MatchmakingSession* session = (iter)->getMmSession();
            if (session != nullptr) 
            {
                const TeamIdSizeList* myTeamIds = session->getCriteria().getTeamIdSizeListFromRule();
                if (myTeamIds == nullptr)
                    continue;

                // Check all the teams for the given session:
                TeamIdSizeList::const_iterator teamIdSize = myTeamIds->begin();
                TeamIdSizeList::const_iterator teamIdSizeEnd = myTeamIds->end();
                for (; teamIdSize != teamIdSizeEnd; ++teamIdSize)
                {
                    TeamId teamId = teamIdSize->first;
                    if (pullingSession.isSessionJoinableForTeam(*session, teamInfo, autoJoinSession, teamId))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

        
    /*! ************************************************************************************************/
    /*! \brief return the highest non empty bucket of mm sessions, in range [lowerLimit,upperLimit].
    ***************************************************************************************************/
    inline const MatchedSessionsBucket* getHighestSessionsBucketInMap(MatchedSessionsBucketingKey lowerLimit,
        MatchedSessionsBucketingKey upperLimit, const MatchedSessionsBucketMap& sessionsBucketMap)
    {
        // PRE: all the bucket map is ordered descending.
        const MatchedSessionsBucket* bucket = nullptr;
        for (MatchedSessionsBucketMap::const_iterator itr = sessionsBucketMap.begin(); (itr != sessionsBucketMap.end()); ++itr)
        {
            if (!itr->second.mItems.empty() && (itr->first <= upperLimit) && (itr->first >= lowerLimit))
            {
                bucket = &(itr->second);
                break;
            }
        }
        return bucket;
    }

    inline const MatchedSessionsBucket* getLowestSessionsBucketInMap(MatchedSessionsBucketingKey lowerLimit,
        MatchedSessionsBucketingKey upperLimit, const MatchedSessionsBucketMap& sessionsBucketMap)
    {
        // PRE: all the bucket map is ordered descending.
        const MatchedSessionsBucket* bucket = nullptr;
        for (MatchedSessionsBucketMap::const_reverse_iterator itr = sessionsBucketMap.rbegin(); (itr != sessionsBucketMap.rend()); ++itr)
        {
            if (!itr->second.mItems.empty() && (itr->first <= upperLimit) && (itr->first >= lowerLimit))
            {
                bucket = &(itr->second);
                break;
            }
        }
        return bucket;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_MATCHED_SESSIONS_BUCKET_UTIL
