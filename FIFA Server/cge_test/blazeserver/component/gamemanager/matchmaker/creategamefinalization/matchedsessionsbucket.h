/*! ************************************************************************************************/
/*!
    \file   matchedsessionsbucket.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHED_SESSIONS_BUCKET
#define BLAZE_MATCHED_SESSIONS_BUCKET

#include "gamemanager/matchmaker/matchlist.h"

namespace Blaze
{

namespace GameManager
{

namespace Matchmaker
{
    class MatchmakingSession;

    //! \brief Key type for the matched sessions buckets map.
    typedef uint16_t MatchedSessionsBucketingKey;//Values are matchmaking group sizes.
    typedef UserExtendedDataValue MatchedSessionsBucketSortType;//Values are matchmaking group's team UED values.

    /*! ************************************************************************************************/
    /*! \brief A bucket of sessions from the matched sessions list.
    ***************************************************************************************************/
    struct MatchedSessionsBucket
    {

        /*! \brief item in bucket, references an associated matchmaking session. */
        struct BucketItem
        {
            BucketItem(const MatchmakingSessionMatchNode* matchNode);
            virtual ~BucketItem() {}

            MatchedSessionsBucketSortType mSortValue;
            bool mRemoved;
            const MatchmakingSession* getMmSession() const { return (mRemoved? nullptr : mNode->sMatchedToSession); }
            const MatchmakingSessionMatchNode* getMatchNode() const { return (mRemoved? nullptr : mNode); }
        private:
            const MatchmakingSessionMatchNode* mNode;
        };

        /*! \brief sort in descending order. */
        struct BucketItemComparitor
        {
            bool operator()(const BucketItem &a, const BucketItem &b) const { return (a.mSortValue > b.mSortValue); }
            bool operator()(const BucketItem &a, const MatchedSessionsBucketSortType &b) const { return (a.mSortValue > b); }
            bool operator()(const MatchedSessionsBucketSortType &a, const BucketItem &b) const { return (a > b.mSortValue); }
        };

        void addMatchToBucket(const MatchmakingSessionMatchNode* matchNode, MatchedSessionsBucketingKey bucketKey);
        bool removeMatchFromBucket(const MatchmakingSession& session, MatchedSessionsBucketingKey bucketKey);

        void sort() { eastl::sort(mItems.begin(), mItems.end(), BucketItemComparitor()); }
        const char8_t* toLogStr(const char8_t* prefix = "") const;


        /*! \brief Vector of bucket items, Consider using a multiset in the future, or possibly some sort of intrusive object
            and get rid of the matchedTo/from list altogether
        */
        typedef eastl::vector<BucketItem> BucketItemVector;
        BucketItemVector mItems;
        MatchedSessionsBucketingKey mKey;

    private:
        mutable eastl::string mLogStr;
    };

    typedef eastl::map<MatchedSessionsBucketingKey, MatchedSessionsBucket, eastl::greater<MatchedSessionsBucketingKey> > MatchedSessionsBucketMap;

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_MATCHED_SESSIONS_BUCKET
