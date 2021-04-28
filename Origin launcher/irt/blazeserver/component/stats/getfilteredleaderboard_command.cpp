/*************************************************************************************************/
/*!
    \file   getfilteredleaderboard_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetFilteredLeaderboardCommand

    Returns the range of ranked statistics defined by the supplied entity list.

    Request
    name - entity name
    idls - list of entity IDs 

    Response
    rank - entity rank
    enid - entity ID
    enam - entity name
    rsta - ranked stat value
    stat - values for other stats in leaderboard

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/query.h"

#include "framework/userset/userset.h"

#include "statsconfig.h"
#include "statsslaveimpl.h"
#include "leaderboardhelper.h"

#include "stats/rpc/statsslave/getfilteredleaderboard_stub.h"
#include "stats/rpc/statsslave/getfilteredleaderboardraw_stub.h"
#include "stats/rpc/statsslave/getfilteredleaderboardraw2_stub.h"


namespace Blaze
{
namespace Stats
{

/*************************************************************************************************/
/*!
    \class GetFilteredLeaderboardBase

    Returns the range of ranked statistics defined by the supplied entity list.
*/
/*************************************************************************************************/
class GetFilteredLeaderboardBase : public LeaderboardHelper
{
public:
    GetFilteredLeaderboardBase(FilteredLeaderboardStatsRequest& request, LeaderboardStatValues& response,
        StatsSlaveImpl* componentImpl, bool isStatRawVal = false)
        : LeaderboardHelper(componentImpl, response,isStatRawVal), mRequestBase(request) {}
    ~GetFilteredLeaderboardBase() override {}

protected:
    BlazeRpcError prepareEntitiesForQuery(EntityIdList& entityList) override;
    bool isEmptyQuery(EntityIdList& entityList) override;
    void previewDbLookupResults(uint32_t configuredSize, const DbRowVector& dbRowVector, EntityIdList& keys) override;
    BlazeRpcError getIndexEntries() override;

    BlazeRpcError executeBase();

private:
    FilteredLeaderboardStatsRequest& mRequestBase;

    typedef eastl::hash_set<EntityId> EntityIdSet;
    EntityIdSet mEntityIdSet;
};

// Note, oddly we do _not_ populate anything into the entityList... we need to fetch all users
// from (potentially) all partitions in order to be able to determine the absolute ranks,
// we only perform the filtering later
BlazeRpcError GetFilteredLeaderboardBase::prepareEntitiesForQuery(EntityIdList& entityList)
{
    // mEntityIdSet has already been populated in executeBase()
    return ERR_OK;
}

bool GetFilteredLeaderboardBase::isEmptyQuery(EntityIdList& entityList)
{
    return (mEntityIdSet.empty());
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
void GetFilteredLeaderboardBase::previewDbLookupResults(uint32_t configuredSize, const DbRowVector& dbRowVector, EntityIdList& entityList)
{
    uint32_t limit = mRequestBase.getLimit();

    if (0 == limit)
    {
        ERR_LOG("[GetFilteredLeaderboardBase].previewDbLookupResults():The limit can not be set as 0.");
        return;
    }

    int32_t rank = 0;
    uint32_t size = limit;
    if (mRequestBase.getEnforceCutoffStatValue() || !mRequestBase.getIncludeStatlessEntities())
    {
        if (dbRowVector.size() < limit)
        {
            size = dbRowVector.size();
        }
        
    }
    else if (mRequestBase.getListOfIds().size() < limit)
    {
        size = mRequestBase.getListOfIds().size();
    }
    entityList.reserve(size);
    mValues.getRows().reserve(size);
    for (DbRowVector::const_iterator itr = dbRowVector.begin(); (itr!=dbRowVector.end()) && (limit>0); ++itr)
    {
        const DbRow *row = *itr;
        EntityId entityId = row->getInt64("entity_id");
        EntityIdSet::iterator eidItr = mEntityIdSet.find(entityId);
        if (eidItr != mEntityIdSet.end())
        {
            ++rank;
            entityList.push_back(entityId);

            // add to response
            LeaderboardStatValuesRow* statRow = mValues.getRows().pull_back();
            statRow->getUser().setBlazeId(entityId);
            statRow->setRank(rank);
            limit--;

            mEntityIdSet.erase(eidItr);
        }
    }
    if (!mRequestBase.getEnforceCutoffStatValue() && mRequestBase.getIncludeStatlessEntities())
    {
        FilteredLeaderboardStatsRequest::EntityIdList::const_iterator iterLst = mRequestBase.getListOfIds().begin();
        FilteredLeaderboardStatsRequest::EntityIdList::const_iterator iterLstEnd = mRequestBase.getListOfIds().end();
        for (; (iterLst!=iterLstEnd) && (limit>0); ++iterLst)
        {
            EntityId entityId = *iterLst;
            if (mEntityIdSet.find(entityId) != mEntityIdSet.end())
            {
                entityList.push_back(entityId);

                // add to response
                LeaderboardStatValuesRow* statRow = mValues.getRows().pull_back();
                statRow->getUser().setBlazeId(entityId);
                statRow->setRank(0);
                limit--;
            }
        }
    }
}

/*************************************************************************************************/
/*!
    \brief getIndexEntries
    Calls to retrieve filtered leaderboard entries
    \param[in] - none
    \return - error code
*/
/*************************************************************************************************/
BlazeRpcError GetFilteredLeaderboardBase::getIndexEntries()
{
    return mComponent->getFilteredLeaderboardEntries(*mLeaderboard, &mCombinedScopeNameValueListMap, mPeriodId,
        mRequestBase.getListOfIds().asVector(), mRequestBase.getLimit(), mKeys, mValues, mSorted);
}

BlazeRpcError GetFilteredLeaderboardBase::executeBase()
{
    BlazeRpcError error = Blaze::ERR_OK;
    EA::TDF::ObjectId userSetId = mRequestBase.getUserSetId();

    // It is perhaps confusing that we allow the client to request both a specified list of ids
    // and a user set at the same time.  Until that is revisited we ensure that there is no
    // duplication between the two by using a hash set to filter out the dups
    mEntityIdSet.insert(mRequestBase.getListOfIds().begin(), mRequestBase.getListOfIds().end());

    if (userSetId != EA::TDF::OBJECT_ID_INVALID)
    {
        BlazeIdList ids;
        error = gUserSetManager->getUserBlazeIds(userSetId, ids);
        if (error != Blaze::ERR_OK)
        {
            WARN_LOG("[GetFilteredLeaderboardBase].executeBase: object ID is invalid <"
                << userSetId.toString().c_str() << ">");
            return STATS_ERR_INVALID_OBJECT_ID;
        }

        for (BlazeIdList::const_iterator blazeIdItr = ids.begin(); blazeIdItr != ids.end(); ++blazeIdItr)
        {
            mEntityIdSet.insert(*blazeIdItr);
        }
    }

    // copy the hash set back to our request
    // but if no userset passed in and original list is same size as the set, then there are no dupes
    if (userSetId != EA::TDF::OBJECT_ID_INVALID || mEntityIdSet.size() != mRequestBase.getListOfIds().size())
    {
        mRequestBase.getListOfIds().clear();
        mRequestBase.getListOfIds().reserve(mEntityIdSet.size());
        mRequestBase.getListOfIds().insert(mRequestBase.getListOfIds().end(), mEntityIdSet.begin(), mEntityIdSet.end());
    }

    if (mRequestBase.getListOfIds().size() == 0)
    {
        TRACE_LOG("[GetFilteredLeaderboardBase].executeBase: board <"
            << mRequestBase.getBoardName() << "> of ZERO size requested");
        return ERR_OK;
    }

    return fetchLeaderboard(mRequestBase.getBoardId(), mRequestBase.getBoardName(), mRequestBase.getPeriodOffset(),
        mRequestBase.getTime(), mRequestBase.getPeriodId(), mRequestBase.getListOfIds().size(), mRequestBase.getKeyScopeNameValueMap(),
        mRequestBase.getIncludeStatlessEntities(), mRequestBase.getEnforceCutoffStatValue());
}

/*************************************************************************************************/
/*!
    \class GetFilteredLeaderboardCommand

    Returns the range of ranked statistics defined by the supplied entity list.
    The stats are returned as strings in the TDF.
*/
/*************************************************************************************************/
class GetFilteredLeaderboardCommand : public GetFilteredLeaderboardCommandStub, GetFilteredLeaderboardBase
{
public:
    GetFilteredLeaderboardCommand(Message* message, FilteredLeaderboardStatsRequest* request, StatsSlaveImpl* componentImpl)
        : GetFilteredLeaderboardCommandStub(message, request), GetFilteredLeaderboardBase(mRequest, mResponse, componentImpl, false) {}
    ~GetFilteredLeaderboardCommand() override {}

private:
    /*************************************************************************************************/
    /*!
        \brief execute
        Command entry point
        \param[in] - none
        \return - none
    */
    /*************************************************************************************************/
    GetFilteredLeaderboardCommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(executeBase());
    }
};

DEFINE_GETFILTEREDLEADERBOARD_CREATE()

/*************************************************************************************************/
/*!
    \class GetFilteredLeaderboardRawCommand

    Returns the range of ranked statistics defined by the supplied entity list.
    The stats are returned as their raw type in the TDF.
*/
/*************************************************************************************************/
class GetFilteredLeaderboardRawCommand : public GetFilteredLeaderboardRawCommandStub, GetFilteredLeaderboardBase
{
public:
    GetFilteredLeaderboardRawCommand(Message* message, FilteredLeaderboardStatsRequest* request, StatsSlaveImpl* componentImpl)
        : GetFilteredLeaderboardRawCommandStub(message, request), GetFilteredLeaderboardBase(mRequest, mResponse, componentImpl, true) {}
    ~GetFilteredLeaderboardRawCommand() override {}

private:
    /*************************************************************************************************/
    /*!
        \brief execute
        Command entry point
        \param[in] - none
        \return - none
    */
    /*************************************************************************************************/
    GetFilteredLeaderboardRawCommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(executeBase());
    }
};

DEFINE_GETFILTEREDLEADERBOARDRAW_CREATE()

/*************************************************************************************************/
/*!
\class GetFilteredLeaderboardRaw2Command

Returns the range of ranked statistics defined by the supplied entity list.
The stats are returned as their raw type in the TDF.
*/
/*************************************************************************************************/
class GetFilteredLeaderboardRaw2Command : public GetFilteredLeaderboardRaw2CommandStub, GetFilteredLeaderboardBase
{
public:
    GetFilteredLeaderboardRaw2Command(Message* message, FilteredLeaderboardStatsRequest* request, StatsSlaveImpl* componentImpl)
        : GetFilteredLeaderboardRaw2CommandStub(message, request), GetFilteredLeaderboardBase(mRequest, mResponse, componentImpl, true) {}
    virtual ~GetFilteredLeaderboardRaw2Command() {}

private:
    /*************************************************************************************************/
    /*!
    \brief execute
    Command entry point
    \param[in] - none
    \return - none
    */
    /*************************************************************************************************/
    GetFilteredLeaderboardRaw2CommandStub::Errors execute()
    {
        return commandErrorFromBlazeError(executeBase());
    }
};

DEFINE_GETFILTEREDLEADERBOARDRAW2_CREATE()

} // Stats
} // Blaze
