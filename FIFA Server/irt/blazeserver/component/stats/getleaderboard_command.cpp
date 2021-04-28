/*************************************************************************************************/
/*!
    \file   getleaderboard_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetLeaderboardCommand

    Retrieves ranked stats for pre-defined stat columns starting from given rank and for
    given number of entities.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/query.h"

#include "framework/userset/userset.h"

#include "statsconfig.h"
#include "statsslaveimpl.h"
#include "leaderboardhelper.h"

#include "stats/rpc/statsslave/getleaderboard_stub.h"
#include "stats/rpc/statsslave/getleaderboardraw_stub.h"
#include "stats/rpc/statsslave/getleaderboardraw2_stub.h"

namespace Blaze
{
namespace Stats
{

/*************************************************************************************************/
/*!
    \class GetLeaderboardBase

    Retrieves ranked stats for pre-defined stat columns starting from given rank and for
    given number of entities.
*/
/*************************************************************************************************/
class GetLeaderboardBase : public LeaderboardHelper
{
public:
    GetLeaderboardBase(LeaderboardStatsRequest& request, LeaderboardStatValues& response,
        StatsSlaveImpl* componentImpl, bool isStatRawVal = false)
        : LeaderboardHelper(componentImpl, response, isStatRawVal), mRequestBase(request) {}
    ~GetLeaderboardBase() override {}

protected:
    BlazeRpcError prepareEntitiesForQuery(EntityIdList& entityList) override;
    bool isEmptyQuery(EntityIdList& entityList) override;
    void previewDbLookupResults(uint32_t configuredSize, const DbRowVector& dbRowVector, EntityIdList& keys) override;
    BlazeRpcError getIndexEntries() override;

    BlazeRpcError executeBase();

private:
    void previewDbLookupResultsForUserSet(EA::TDF::ObjectId userSetId, const DbRowVector& dbRowVector, EntityIdList& keys);;

private:
    LeaderboardStatsRequest& mRequestBase;
};

BlazeRpcError GetLeaderboardBase::prepareEntitiesForQuery(EntityIdList& entityList)
{
    BlazeRpcError error = ERR_OK;
    if (mRequestBase.getUserSetId() == EA::TDF::OBJECT_ID_INVALID)
        return ERR_OK;

    BlazeIdList ids;
    error = gUserSetManager->getUserBlazeIds(mRequestBase.getUserSetId(), ids);
    if (error != ERR_OK)
    {
        return error;
    }

    entityList.reserve(ids.size());
    for (BlazeIdList::const_iterator idItr = ids.begin(); idItr != ids.end(); ++idItr)
    {
        entityList.push_back(*idItr);
    }

    return ERR_OK;
}

bool GetLeaderboardBase::isEmptyQuery(EntityIdList& entityList)
{
    return (mRequestBase.getUserSetId() != EA::TDF::OBJECT_ID_INVALID && entityList.empty());
}

/*************************************************************************************************/
/*!
    \brief previewDbLookupResults

    Build the entities we want in the response from DB results.
    Rows in the response are also created here.

    \param[in] - configuredSize - configured size of the leaderboard
    \param[in] - dbRowVector - the DB results (already sorted by rank stat value)
    \param[out] - entityList - the entities we want in the response

    \return - none
*/
/*************************************************************************************************/
void GetLeaderboardBase::previewDbLookupResults(uint32_t configuredSize, const DbRowVector& dbRowVector, EntityIdList& entityList)
{
    if (mRequestBase.getUserSetId() != EA::TDF::OBJECT_ID_INVALID)
    {
        previewDbLookupResultsForUserSet(mRequestBase.getUserSetId(), dbRowVector, entityList);
        return;
    }

    int32_t _configuredSize = static_cast<int32_t>(configuredSize);
    if (mRequestBase.getRankStart() > _configuredSize)
    {
        // fast complete the RPC
        return;
    }

    int32_t endRank = mRequestBase.getRankStart() + mRequestBase.getCount() - 1;
    if (endRank > _configuredSize)
    {
        endRank = _configuredSize;
    }

    int32_t rank = 0;
    DbRowVector::const_iterator dbItr = dbRowVector.begin();
    DbRowVector::const_iterator dbEnd = dbRowVector.end();

    int32_t size = endRank - mRequestBase.getRankStart() + 1;
    entityList.reserve(size);
    mValues.getRows().reserve(size);

    for (; dbItr != dbEnd && rank < endRank; ++dbItr)
    {
        ++rank;
        if (rank < mRequestBase.getRankStart())
        {
            continue;
        }
        const DbRow *row = *dbItr;
        EntityId entityId = row->getInt64("entity_id");
        entityList.push_back(entityId);

        // add to response
        LeaderboardStatValuesRow* statRow = mValues.getRows().pull_back();
        statRow->getUser().setBlazeId(entityId);
        statRow->setRank(rank);
    }
}

/*************************************************************************************************/
/*!
    \brief previewDbLookupResultsForUserSet

    Similar to previewDbLookupResults(), but specific handling for the userset case.
    Also, configured leaderboard size is not considered in this case.

    \param[in] - userSetId - userset ID that we're filtering on
    \param[in] - dbRowVector - the DB results (already sorted by rank stat value)
    \param[out] - entityList - the entities we want in the response

    \return - none
*/
/*************************************************************************************************/
void GetLeaderboardBase::previewDbLookupResultsForUserSet(EA::TDF::ObjectId userSetId, const DbRowVector& dbRowVector, EntityIdList& entityList)
{
    BlazeRpcError error = Blaze::ERR_OK;
    uint32_t uss = 0;
    error = gUserSetManager->countUsers(userSetId, uss);
    if (error != Blaze::ERR_OK)
    {
        // just ignore ... an earlier error should have prevented getting here anyway
        return;
    }
    int32_t userSetSize = static_cast<int32_t>(uss);
    if (userSetSize == 0)
    {
        return;
    }

    if (mRequestBase.getRankStart() > userSetSize)
    {
        return;
    }
    int32_t endRank = mRequestBase.getRankStart() + mRequestBase.getCount() - 1;
    if (endRank > userSetSize)
    {
        endRank = userSetSize;
    }

    EntitySet entitySet(dbRowVector.size());
    int32_t rank = 0;
    DbRowVector::const_iterator dbItr = dbRowVector.begin();
    DbRowVector::const_iterator dbEnd = dbRowVector.end();

    int32_t size = endRank - mRequestBase.getRankStart() + 1;
    entityList.reserve(size);
    mValues.getRows().reserve(size);

    for (; dbItr != dbEnd && rank < endRank; ++dbItr)
    {
        const DbRow *row = *dbItr;
        EntityId entityId = row->getInt64("entity_id");
        entitySet.insert(entityId);
        ++rank;
        if (rank < mRequestBase.getRankStart())
        {
            continue;
        }
        entityList.push_back(entityId);

        // add to response
        LeaderboardStatValuesRow* statRow = mValues.getRows().pull_back();
        statRow->getUser().setBlazeId(entityId);
        statRow->setRank(rank);
    }

    ++rank; // start to count unranked users

    // fill remaining request from the unranked users
    // -- which should be users who have no stats (and thus get default stats later)
    if (rank <= endRank)
    {
        BlazeIdList ids;
        error = gUserSetManager->getUserBlazeIds(userSetId, ids);
        if (error != Blaze::ERR_OK)
        {
            // just ignore ... an earlier error should have prevented getting here anyway
            return;
        }
        for (BlazeIdList::const_iterator idItr = ids.begin(); idItr != ids.end() && rank <= endRank; ++idItr)
        {
            EntityId entityId = *idItr;
            if (entitySet.find(entityId) != entitySet.end())
            {
                // ignore ranked user
                continue;
            }

            if (rank >= mRequestBase.getRankStart())
            {
                entityList.push_back(entityId);

                // add to response
                LeaderboardStatValuesRow* statRow = mValues.getRows().pull_back();
                statRow->getUser().setBlazeId(entityId);
                statRow->setRank(0);
            }
            // else skip because request may start in middle of unranked users
            ++rank;
        }
    }
}

BlazeRpcError GetLeaderboardBase::getIndexEntries()
{
    return mComponent->getLeaderboardEntries(*mLeaderboard, &mCombinedScopeNameValueListMap, mPeriodId,
        mRequestBase.getRankStart(), mRequestBase.getCount(), mRequestBase.getUserSetId(), mKeys, mValues, mSorted);
}

BlazeRpcError GetLeaderboardBase::executeBase()
{
    if (mRequestBase.getRankStart() <= 0)
    {
        mRequestBase.setRankStart(1);
    }
    if (mRequestBase.getCount() < 0)
    {
        ERR_LOG("[GetLeaderboardBase].executeBase: requested (rank <"
            << mRequestBase.getRankStart() << ">, count <" << mRequestBase.getCount() << ">) not available.");
        return STATS_ERR_RANK_OUT_OF_RANGE;
    }

    return fetchLeaderboard(mRequestBase.getBoardId(), mRequestBase.getBoardName(), mRequestBase.getPeriodOffset(),
        mRequestBase.getTime(), mRequestBase.getPeriodId(), mRequestBase.getCount(), mRequestBase.getKeyScopeNameValueMap());
}

/*************************************************************************************************/
/*!
    \class GetLeaderboardCommand

    Retrieves ranked stats for pre-defined stat columns starting from given rank and for
    given number of entities.  The stats are returned as strings in the TDF.
*/
/*************************************************************************************************/
class GetLeaderboardCommand : public GetLeaderboardCommandStub, GetLeaderboardBase
{
public:
    GetLeaderboardCommand(Message* message, LeaderboardStatsRequest* request, StatsSlaveImpl* componentImpl)
        : GetLeaderboardCommandStub(message, request), GetLeaderboardBase(mRequest, mResponse, componentImpl, false) {}
    ~GetLeaderboardCommand() override {}

private:
    /*************************************************************************************************/
    /*!
        \brief execute
        Command entry point
        \param[in] - none
        \return - none
    */
    /*************************************************************************************************/
    GetLeaderboardCommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(executeBase());
    }
};

DEFINE_GETLEADERBOARD_CREATE()

/*************************************************************************************************/
/*!
    \class GetLeaderboardRawCommand

    Retrieves ranked stats for pre-defined stat columns starting from given rank and for
    given number of entities.  The stats are returned as their raw type in the TDF.
*/
/*************************************************************************************************/
class GetLeaderboardRawCommand : public GetLeaderboardRawCommandStub, GetLeaderboardBase
{
public:
    GetLeaderboardRawCommand(Message* message, LeaderboardStatsRequest* request, StatsSlaveImpl* componentImpl)
        : GetLeaderboardRawCommandStub(message, request), GetLeaderboardBase(mRequest, mResponse, componentImpl, true) {}
    ~GetLeaderboardRawCommand() override {}

private:
    /*************************************************************************************************/
    /*!
        \brief execute
        Command entry point
        \param[in] - none
        \return - none
    */
    /*************************************************************************************************/
    GetLeaderboardRawCommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(executeBase());
    }
};

DEFINE_GETLEADERBOARDRAW_CREATE()


/*************************************************************************************************/
/*!
\class GetLeaderboardRaw2Command

Retrieves ranked stats for pre-defined stat columns starting from given rank and for
given number of entities.  The stats are returned as their raw type in the TDF.
*/
/*************************************************************************************************/
class GetLeaderboardRaw2Command : public GetLeaderboardRaw2CommandStub, GetLeaderboardBase
{
public:
    GetLeaderboardRaw2Command(Message* message, LeaderboardStatsRequest* request, StatsSlaveImpl* componentImpl)
        : GetLeaderboardRaw2CommandStub(message, request), GetLeaderboardBase(mRequest, mResponse, componentImpl, true) {}
    virtual ~GetLeaderboardRaw2Command() {}

private:
    /*************************************************************************************************/
    /*!
    \brief execute
    Command entry point
    \param[in] - none
    \return - none
    */
    /*************************************************************************************************/
    GetLeaderboardRaw2CommandStub::Errors execute()
    {
        return commandErrorFromBlazeError(executeBase());
    }
};

DEFINE_GETLEADERBOARDRAW2_CREATE()
} // Stats
} // Blaze
