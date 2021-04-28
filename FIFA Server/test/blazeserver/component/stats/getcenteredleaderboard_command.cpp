/*************************************************************************************************/
/*!
    \file   getcenteredleaderboard_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetCenteredLeaderboardCommand

    Returns the range of ranked statistics with supplied entity in the center.

    Request
    name - entity name
    cent - ID of the central entity 
    coun - number (count) of stat rows to return
    
    Response
    ldls - list of leaderboards, where
        rank - is a rank of entity
        enid - is entity ID
        enam - entity name
        rsta - ranked stat
        stat - list of other stats
    
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/query.h"

#include "framework/userset/userset.h"

#include "statsconfig.h"
#include "statsslaveimpl.h"
#include "leaderboardhelper.h"

#include "stats/rpc/statsslave/getcenteredleaderboard_stub.h"
#include "stats/rpc/statsslave/getcenteredleaderboardraw_stub.h"
#include "stats/rpc/statsslave/getcenteredleaderboardraw2_stub.h"
namespace Blaze
{
namespace Stats
{

/*************************************************************************************************/
/*!
    \class GetCenteredLeaderboardBase

    Returns the range of ranked statistics with supplied entity in the center.
*/
/*************************************************************************************************/
class GetCenteredLeaderboardBase : public LeaderboardHelper
{
public:
    GetCenteredLeaderboardBase(CenteredLeaderboardStatsRequest& request, LeaderboardStatValues& response,
        StatsSlaveImpl* componentImpl, bool isStatRawVal = false)
        : LeaderboardHelper(componentImpl, response, isStatRawVal), mRequestBase(request) {}
    ~GetCenteredLeaderboardBase() override {}

protected:
    BlazeRpcError prepareEntitiesForQuery(EntityIdList& entityList) override;
    bool isEmptyQuery(EntityIdList& entityList) override;
    void previewDbLookupResults(uint32_t configuredSize, const DbRowVector& dbRowVector, EntityIdList& keys) override;
    BlazeRpcError getIndexEntries() override;

    BlazeRpcError executeBase();

private:
    void previewDbLookupResultsForUserSet(EA::TDF::ObjectId userSetId, const DbRowVector& dbRowVector, EntityIdList& keys);

private:
    CenteredLeaderboardStatsRequest& mRequestBase;

    typedef eastl::hash_map<int32_t, EntityId> RankEntityMap;
    RankEntityMap mRankEntityMap;
};

BlazeRpcError GetCenteredLeaderboardBase::prepareEntitiesForQuery(EntityIdList& entityList)
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

    bool centerInUserSet = false;
    entityList.reserve(ids.size());
    for (BlazeIdList::const_iterator idItr = ids.begin(); idItr != ids.end(); ++idItr)
    {
        EntityId entityId = *idItr;
        entityList.push_back(entityId);

        if (entityId == mRequestBase.getCenter())
            centerInUserSet = true;
    }

    if (!centerInUserSet)
    {
        // fast complete the RPC
        entityList.clear();
    }

    return ERR_OK;
}

bool GetCenteredLeaderboardBase::isEmptyQuery(EntityIdList& entityList)
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
void GetCenteredLeaderboardBase::previewDbLookupResults(uint32_t configuredSize, const DbRowVector& dbRowVector, EntityIdList& entityList)
{
    if (mRequestBase.getUserSetId() != EA::TDF::OBJECT_ID_INVALID)
    {
        previewDbLookupResultsForUserSet(mRequestBase.getUserSetId(), dbRowVector, entityList);
        return;
    }

    int32_t _configuredSize = static_cast<int32_t>(configuredSize);
    int32_t count = mRequestBase.getCount();

    // find the center entity (and build the rank entity map while doing so)
    int32_t entityRank = 0;
    int32_t rank = 0;
    DbRowVector::const_iterator dbItr = dbRowVector.begin();
    DbRowVector::const_iterator dbEnd = dbRowVector.end();
    for (; dbItr != dbEnd && rank < _configuredSize; ++dbItr)
    {
        const DbRow *row = *dbItr;
        EntityId entityId = row->getInt64("entity_id");
        ++rank;
        mRankEntityMap[rank] = entityId;

        if (entityId == mRequestBase.getCenter())
        {
            entityRank = rank;
            // done
            break;
        }
    }

    if (entityRank == 0)
    {
        if (!mRequestBase.getShowAtBottomIfNotFound())
        {
            TRACE_LOG("[GetCenteredLeaderboard].previewDbLookupResults() entity: "
                << mRequestBase.getCenter() << " is not in the ranking table");
            return;
        }
        // if the center entity isn't in the db, we'll just center at the end of the existing entities
        --count;
    }

    int32_t size = static_cast<int32_t>(dbRowVector.size());
    if (size > _configuredSize)
        size = _configuredSize;

    int32_t startRank = rank - count/2;
    if (startRank < 1)
        startRank = 1;
    int32_t endRank = startRank + count - 1;
    if (endRank > size)
    {
        // offset start by how much end exceeds size
        startRank -= endRank - size;
        if (startRank < 1)
            startRank = 1;

        endRank = size;
    }

    // use the rank entity map to build the first 'half' of the response
    int32_t r;
    entityList.reserve(size);
    mValues.getRows().reserve(size);
    for (r = startRank; r <= rank; ++r)
    {
        EntityId entityId = mRankEntityMap[r];
        entityList.push_back(entityId);

        // add to response
        LeaderboardStatValuesRow* statRow = mValues.getRows().pull_back();
        statRow->getUser().setBlazeId(entityId);
        statRow->setRank(r);
    }

    if (entityRank == 0)
    {
        entityList.push_back(mRequestBase.getCenter());

        LeaderboardStatValuesRow* statRow = mValues.getRows().pull_back();
        statRow->setRank(0);
        statRow->getUser().setBlazeId(mRequestBase.getCenter());
        return;
    }

    // since the rank entity map only has entities/ranks up to the center entity (where it is ranked),
    // add the second 'half' (after the center) from the DB results
    if (dbItr == dbEnd)
        return;

    for (++dbItr; dbItr != dbEnd && r <= endRank; ++dbItr, ++r)
    {
        const DbRow *row = *dbItr;
        EntityId entityId = row->getInt64("entity_id");
        entityList.push_back(entityId);

        // add to response
        LeaderboardStatValuesRow* statRow = mValues.getRows().pull_back();
        statRow->getUser().setBlazeId(entityId);
        statRow->setRank(r);
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
void GetCenteredLeaderboardBase::previewDbLookupResultsForUserSet(EA::TDF::ObjectId userSetId, const DbRowVector& dbRowVector, EntityIdList& entityList)
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

    int32_t count = mRequestBase.getCount();

    // find the center entity (and build the rank entity map while doing so)
    EntitySet entitySet(dbRowVector.size());
    int32_t entityRank = 0;
    int32_t rank = 0;
    DbRowVector::const_iterator dbItr = dbRowVector.begin();
    DbRowVector::const_iterator dbEnd = dbRowVector.end();
    for (; dbItr != dbEnd; ++dbItr)
    {
        const DbRow *row = *dbItr;
        EntityId entityId = row->getInt64("entity_id");
        entitySet.insert(entityId);
        ++rank;
        mRankEntityMap[rank] = entityId;

        if (entityId == mRequestBase.getCenter())
        {
            entityRank = rank;
        }
    }

    int32_t rankedUserSetSize = static_cast<int32_t>(entitySet.size());
    int32_t startRank = entityRank - count/2;
    // if the center entity isn't in the db, we'll just center at the end of the existing entities
    if (entityRank == 0)
    {
        startRank = rankedUserSetSize + 1 - count/2;
    }
    int32_t endRank = startRank + mRequestBase.getCount() - 1;
    if (endRank > userSetSize)
    {
        // offset start by how much end exceeds size
        startRank -= endRank - userSetSize;
        if (startRank < 1)
            startRank = 1;

        endRank = userSetSize;
    }

    // in this case, rank entity map has all the ranked users...
    entityList.reserve((endRank - startRank) + 2);
    mValues.getRows().reserve((endRank - startRank) + 2);
    for (rank = startRank; rank <= endRank && rank <= rankedUserSetSize; ++rank)
    {
        EntityId entityId = mRankEntityMap[rank];
        entityList.push_back(entityId);

        // add to response
        LeaderboardStatValuesRow* statRow = mValues.getRows().pull_back();
        statRow->getUser().setBlazeId(entityId);
        statRow->setRank(rank);
    }

    if (entityRank == 0)
    {
        entityList.push_back(mRequestBase.getCenter());

        LeaderboardStatValuesRow* statRow = mValues.getRows().pull_back();
        statRow->setRank(0);
        statRow->getUser().setBlazeId(mRequestBase.getCenter());

        ++rank;
    }

    // fill remaining request from the unranked users
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
            if (entityId == mRequestBase.getCenter())
            {
                // ignore center user
                continue;
            }
            if (entitySet.find(entityId) != entitySet.end())
            {
                // ignore ranked user
                continue;
            }

            entityList.push_back(entityId);

            LeaderboardStatValuesRow* statRow = mValues.getRows().pull_back();
            statRow->setRank(0);
            statRow->getUser().setBlazeId(entityId);

            ++rank;
        }
    }
}

BlazeRpcError GetCenteredLeaderboardBase::getIndexEntries()
{
    return mComponent->getCenteredLeaderboardEntries(*mLeaderboard, &mCombinedScopeNameValueListMap, mPeriodId,
        mRequestBase.getCenter(), mRequestBase.getCount(), mRequestBase.getUserSetId(), mRequestBase.getShowAtBottomIfNotFound(),
        mKeys, mValues, mSorted);
}

BlazeRpcError GetCenteredLeaderboardBase::executeBase()
{
    return fetchLeaderboard(mRequestBase.getBoardId(), mRequestBase.getBoardName(), mRequestBase.getPeriodOffset(),
        mRequestBase.getTime(), mRequestBase.getPeriodId(), mRequestBase.getCount(), mRequestBase.getKeyScopeNameValueMap());
}

/*************************************************************************************************/
/*!
    \class GetCenteredLeaderboardCommand

    Returns the range of ranked statistics with supplied entity in the center.
    The stats are returned as strings in the TDF.
*/
/*************************************************************************************************/
class GetCenteredLeaderboardCommand : public GetCenteredLeaderboardCommandStub, GetCenteredLeaderboardBase
{
public:
    GetCenteredLeaderboardCommand(Message* message, CenteredLeaderboardStatsRequest* request, StatsSlaveImpl* componentImpl)
        : GetCenteredLeaderboardCommandStub(message, request), GetCenteredLeaderboardBase(mRequest, mResponse, componentImpl, false) {}
    ~GetCenteredLeaderboardCommand() override {}

private:
    /*************************************************************************************************/
    /*!
        \brief execute
        Command entry point
        \param[in] - none
        \return - none
    */
    /*************************************************************************************************/
    GetCenteredLeaderboardCommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(executeBase());
    }
};

DEFINE_GETCENTEREDLEADERBOARD_CREATE()

/*************************************************************************************************/
/*!
    \class GetCenteredLeaderboardRawCommand

    Returns the range of ranked statistics with supplied entity in the center.
    The stats are returned as their raw type in the TDF.
*/
/*************************************************************************************************/
class GetCenteredLeaderboardRawCommand : public GetCenteredLeaderboardRawCommandStub, GetCenteredLeaderboardBase
{
public:
    GetCenteredLeaderboardRawCommand(Message* message, CenteredLeaderboardStatsRequest* request, StatsSlaveImpl* componentImpl)
        : GetCenteredLeaderboardRawCommandStub(message, request), GetCenteredLeaderboardBase(mRequest, mResponse, componentImpl, true) {}
    ~GetCenteredLeaderboardRawCommand() override {}

private:
    /*************************************************************************************************/
    /*!
        \brief execute
        Command entry point
        \param[in] - none
        \return - none
    */
    /*************************************************************************************************/
    GetCenteredLeaderboardRawCommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(executeBase());
    }
};

DEFINE_GETCENTEREDLEADERBOARDRAW_CREATE()

/*************************************************************************************************/
/*!
\class GetCenteredLeaderboardRaw2Command

Returns the range of ranked statistics with supplied entity in the center.
The stats are returned as their raw type in the TDF.
*/
/*************************************************************************************************/
class GetCenteredLeaderboardRaw2Command : public GetCenteredLeaderboardRaw2CommandStub, GetCenteredLeaderboardBase
{
public:
    GetCenteredLeaderboardRaw2Command(Message* message, CenteredLeaderboardStatsRequest* request, StatsSlaveImpl* componentImpl)
        : GetCenteredLeaderboardRaw2CommandStub(message, request), GetCenteredLeaderboardBase(mRequest, mResponse, componentImpl, true) {}
    virtual ~GetCenteredLeaderboardRaw2Command() {}

private:
    /*************************************************************************************************/
    /*!
    \brief execute
    Command entry point
    \param[in] - none
    \return - none
    */
    /*************************************************************************************************/
    GetCenteredLeaderboardRaw2CommandStub::Errors execute()
    {
        return commandErrorFromBlazeError(executeBase());
    }
};

DEFINE_GETCENTEREDLEADERBOARDRAW2_CREATE()

} // Stats
} // Blaze
