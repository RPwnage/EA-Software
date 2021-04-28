/*! ************************************************************************************************/
/*!
    \file   matchedsessionsbucket.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/creategamefinalization/matchedsessionsbucket.h"
#include "gamemanager/matchmaker/matchmakingsession.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    MatchedSessionsBucket::BucketItem::BucketItem(const MatchmakingSessionMatchNode* matchNode) :
        mSortValue(matchNode->sMatchedToSession->getCriteria().getTeamUEDContributionValue()), mRemoved(false), mNode(matchNode) 
    {
        TRACE_LOG("[MatchedSessionBucket] Adding Session " << matchNode->sMatchedToSession->getMMSessionId() << " with UED value " << mSortValue << " to bucket");
    }

    const char8_t* MatchedSessionsBucket::toLogStr(const char8_t* prefix) const
    {
        mLogStr.clear();
        mLogStr.sprintf("Bucket: bucketKey=%" PRIi64 ",numItems=%" PRIsize " .. ", mKey, mItems.size());
        if (IS_LOGGING_ENABLED(Logging::SPAM))
        {
            const size_t maxToPrint = 20;
            size_t i = 0;
            for (BucketItemVector::const_iterator itr = mItems.begin(); ((itr != mItems.end()) && (i < maxToPrint)); ++itr, ++i)
            {
                const MatchmakingSession* sess = itr->getMmSession();
                if (sess == nullptr)
                    mLogStr.append_sprintf("{<session with teamUED=%" PRIi64 ",'removed'=%s}", itr->mSortValue, (itr->mRemoved? "true":"false"));
                else
                    mLogStr.append_sprintf("{sessionId=%" PRIu64 ",sessionSize=%u,teamUED(value=%" PRIi64 ",maxAcceptblImbal=%" PRIi64 ")}, ", sess->getMMSessionId(), sess->getPlayerCount(), itr->mSortValue, sess->getCriteria().getAcceptableTeamUEDImbalance());
            }
        }
        return mLogStr.c_str();
    }

    void MatchedSessionsBucket::addMatchToBucket(const MatchmakingSessionMatchNode* matchNode, MatchedSessionsBucketingKey bucketKey)
    {
        mKey = bucketKey;
        mItems.push_back(MatchedSessionsBucket::BucketItem(matchNode));

        // We sort after every insert to ensure our list is always sorted.  This allows us to remove items from the list at any time.
        sort();
    }

    bool MatchedSessionsBucket::removeMatchFromBucket(const MatchmakingSession& session, MatchedSessionsBucketingKey bucketKey)
    {
        // optimization: binary search to range with same UED value.  Assumes vector is sorted after every insert.
        UserExtendedDataValue uedValue = session.getCriteria().getTeamUEDContributionValue();

        MatchedSessionsBucket::BucketItemVector::iterator itr = eastl::lower_bound(mItems.begin(), mItems.end(), uedValue, MatchedSessionsBucket::BucketItemComparitor());

        if (itr == mItems.end())
        {
            TRACE_LOG("[MatchedSessionsBucket].removeFromMatchedToBucket: did not find session " << session.getMMSessionId() << " for removal from bucket(" << bucketKey << ")");
            return false;
        }

        for (; itr != mItems.end(); ++itr)
        {
            if (itr->mSortValue != uedValue)
                break;//no more

            if ((itr->getMmSession() != nullptr) && (itr->getMmSession()->getMMSessionId() == session.getMMSessionId()))
            {
                TRACE_LOG("[MatchedSessionsBucket].removeFromMatchedToBucket: found session " << session.getMMSessionId() << " with UED value " <<  uedValue << " for removal from bucket(" << bucketKey << "), " << (itr->mRemoved? "was already marked removed" : "marking as removed."));
                itr->mRemoved = true;
                return true;
            }
        }

        TRACE_LOG("[MatchedSessionsBucket]removeFromMatchedToBucket: did not find session " << session.getMMSessionId() << " with UED value " <<  uedValue << " for removal from bucket(" << bucketKey << ")");
        return false;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
