/*************************************************************************************************/
/*!
    \file   osdktournamentsslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "customcomponent/osdktournaments/rpc/osdktournaments_defines.h"
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/usersessions/usersession.h"
#include "framework/util/localization.h"

#include "osdktournaments/tdf/osdktournamentstypes_server.h"

#include "osdktournamentsslaveimpl.h"
#include "osdktournamentsdatabase.h"

namespace Blaze
{
namespace OSDKTournaments
{

// static
const char8_t* OSDKTournamentsSlaveImpl::DEFAULT_DB_NAME = "main";

OSDKTournamentsSlave* OSDKTournamentsSlave::createImpl()
{
    return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "OSDKTournamentsSlaveImpl") OSDKTournamentsSlaveImpl();
}

OSDKTournamentsSlaveImpl::OSDKTournamentsSlaveImpl()
    : mDbId(DbScheduler::INVALID_DB_ID)
{
}

OSDKTournamentsSlaveImpl::~OSDKTournamentsSlaveImpl()
{
    gUserSessionManager->deregisterExtendedDataProvider(COMPONENT_ID);
    gUserSessionManager->removeSubscriber(*this);

    TournamentsMap::const_iterator itr = mTournamentsMap.begin();
    TournamentsMap::const_iterator end = mTournamentsMap.end();
    for (; itr != end; ++itr)
    {
        delete itr->second;
    }
    mTournamentsMap.clear();
}

typedef eastl::hash_set<TournamentId> TournamentIdSet;

bool OSDKTournamentsSlaveImpl::onValidateConfig(OsdkTournamentsConfig& config, const OsdkTournamentsConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    TournamentsConfigDataList::const_iterator it = config.getTournaments().begin();
    TournamentsConfigDataList::const_iterator endit = config.getTournaments().end();

    for (;it != endit; it++)
    { 
        const TournamentsConfigData* configData = *it;
        if (configData == NULL)
        {
            eastl::string msg;
            msg.sprintf("[OSDKTournamentsSlaveImpl:%p].onValidateConfig: configData is NULL", this);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
        else
        {
            if (true == configData->getEnable())
            {
                if (0 == blaze_strcmp(configData->getName(), ""))
                {
                    eastl::string msg;
                    msg.sprintf("[OSDKTournamentsSlaveImpl:%p].onValidateConfig: Tournament in config file has empty name.", this);
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                    break;
                }
                else
                {
                    TournamentsConfigDataList::const_iterator itr = config.getTournaments().begin();
                    TournamentsConfigDataList::const_iterator enditr = config.getTournaments().end();
                    for (; itr != enditr; itr++)
                    {
                        if (itr != it)
                        {
                            const TournamentsConfigData* configureData = *itr;
                            if (0 == blaze_stricmp(configureData->getName(), configData->getName()))
                            {
                                eastl::string msg;
                                msg.sprintf("[OSDKTournamentsSlaveImpl:%p].onValidateConfig: Tournaments in config file should not have duplicate names of [%s]", this, configData->getName());
                                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                                str.set(msg.c_str());
                                break;
                            }
                        }
                    }
                }

                if (0 == configData->getNumRounds())
                {
                    WARN_LOG("[OSDKTournamentsSlaveImpl:" << this << "].onValidateConfig: Tournament in config file must specify the number of rounds. Ignoring tournament with name: " << configData->getName());
                }
            }
        } // if ...
    } // for ...

    return validationErrors.getErrorMessages().empty();
}

const char8_t* OSDKTournamentsSlaveImpl::getDbName() const
{
    const char8_t* ret = getConfig().getDbName();

    if(NULL == ret || '\0' == ret[0])
    {
        ret = OSDKTournamentsSlaveImpl::DEFAULT_DB_NAME;
    }

    return ret;
}

bool OSDKTournamentsSlaveImpl::onResolve()
{
    gUserSessionManager->registerExtendedDataProvider(COMPONENT_ID, *this);
    gUserSessionManager->addSubscriber(*this);
    return true;
}

bool OSDKTournamentsSlaveImpl::onConfigure()
{
    const OsdkTournamentsConfig &config = getConfig();

    const char8_t* dbName = config.getDbName();
    mDbId = gDbScheduler->getDbId(dbName);
    if (DbScheduler::INVALID_DB_ID == mDbId)
    {
        ERR_LOG("[OSDKTournamentsSlaveImpl].onConfigure() - cannot init db <" << dbName << ">.");
        return false;
    }

    DbConnPtr dbConnection = gDbScheduler->getConnPtr(getDbId());
    if (dbConnection == NULL)
    {
        ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].onConfigure() - Failed to obtain connection.");
        return false;
    }

    OSDKTournamentsDatabase::initialize(getDbId());
    OSDKTournamentsDatabase tournamentsDb(dbConnection);

    // verify all tournament id's in the config
    TournamentIdSet addedTournamentIds;

    TournamentsConfigDataList::const_iterator it = config.getTournaments().begin();
    TournamentsConfigDataList::const_iterator endit = config.getTournaments().end();
    for (;it != endit; it++)
    { 
        // do not allow the same id to be in the config more than once - ignore any subsequent definitions
        TournamentId currentId = (*it)->getId();
        if ((INVALID_TOURNAMENT_ID == currentId) || (addedTournamentIds.find(currentId) != addedTournamentIds.end()))
        {
            WARN_LOG("[OSDKTournamentsSlaveImpl:" << this << "].onConfigure() - Tournament id " << currentId << " invalid or already parsed from config; ignoring");
            continue;
        }

        // don't do anything with disabled tournaments
        if (false == (*it)->getEnable())
        {
            TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].onConfigure(): Tournament id " << currentId << " is disabled in the config file, skipping");
        }
        else
        {
            if (0 == blaze_strcmp((*it)->getName(), ""))
            {
                continue;
            }

            if (0 == (*it)->getNumRounds())
            {
                continue;
            }

            TournamentData* tData = BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "TournamentData") TournamentData();
            tData->setTournamentId(currentId);
            tData->setName((*it)->getName());
            tData->setMode((*it)->getMode());
            tData->setDescription((*it)->getDescription()); 
            tData->setTrophyName((*it)->getTrophyName()); 
            tData->setTrophyMetaData((*it)->getTrophyMetaData()); 
            tData->setNumRounds((*it)->getNumRounds());

            tournamentsDb.upsertTournament(tData);
            mMetrics.mGaugeOnlineMembers += tData->getOnlineMemberCount();
            mMetrics.mGaugeTotalMembers += tData->getTotalMemberCount();
            addedTournamentIds.insert(tData->getTournamentId());

            TournamentsMap::iterator currentIdItr = mTournamentsMap.find(currentId);
            if (currentIdItr != mTournamentsMap.end())
            {
                delete currentIdItr->second;
            }
            
            mTournamentsMap[currentId] = tData;
        }
    }

    //go through the current items in the config file, remove any tournaments that were taken out since last config
    TournamentsMap::iterator itr = mTournamentsMap.begin();
    while (itr != mTournamentsMap.end())
    {
        TournamentId currentId = itr->second->getTournamentId();
        ++itr;
        if (addedTournamentIds.find(currentId) == addedTournamentIds.end())
        {
            delete mTournamentsMap[currentId];
            mTournamentsMap.erase(currentId);
        }
    }

    addedTournamentIds.clear();

    return true;
}


BlazeRpcError OSDKTournamentsSlaveImpl::resetAllTournamentMembers(TournamentId tournamentId, bool setActive)
{
    BlazeRpcError error = Blaze::ERR_OK;

    DbConnPtr conn = gDbScheduler->getConnPtr(mDbId);
    if (conn == NULL)
    {
        // early return here when the scheduler is unable to find a connection
        ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].resetAllTournamentMembers: unable to find a database connection");
        return Blaze::ERR_SYSTEM;
    }
    OSDKTournamentsDatabase tournDb(conn);

    // start transaction
    if (false == tournDb.startTransaction())
    {
        // early return when unable to start transaction
        return Blaze::ERR_SYSTEM;
    }

    // delete the tournament trees
    if (DB_ERR_OK != tournDb.deleteTournamentTrees(tournamentId))
    {
        // early return when unable to delete tournament trees
        tournDb.completeTransaction(false);
        return OSDKTOURN_ERR_TOURNAMENT_GENERIC;
    }

    // update the member status
    if (DB_ERR_OK != tournDb.updateMemberStatusByTournament(tournamentId, setActive, 1, 0, INVALID_TEAM_ID))
    {
        // early return when unable to determine member status
        tournDb.completeTransaction(false);
        return OSDKTOURN_ERR_TOURNAMENT_GENERIC;
    }

    if (false == tournDb.completeTransaction(true))
    {
        // unable to complete the transaction
        return Blaze::ERR_SYSTEM;
    }

    return error;
}

void OSDKTournamentsSlaveImpl::onJoinTournamentNotification(const Blaze::OSDKTournaments::JoinTournamentNotification &notification, Blaze::UserSession *)
{
    TournamentData* tData = getTournamentDataById(notification.getTournId());
    if (NULL != tData)
    {
        tData->setTotalMemberCount(tData->getTotalMemberCount() + 1);
        tData->setOnlineMemberCount(tData->getOnlineMemberCount() + 1);
        ++(mMetrics.mTotalJoins);

        if (false == notification.getWasInactive())
        {
            //if the user was already online, but inactive, they are already counted in the totals
            // so we dont want to count them twice.
            ++(mMetrics.mGaugeOnlineMembers);
            ++(mMetrics.mGaugeTotalMembers);
        }
    }
}

void OSDKTournamentsSlaveImpl::onLeaveTournamentNotification(const Blaze::OSDKTournaments::LeaveTournamentNotification &notification, Blaze::UserSession *)
{
    Blaze::OSDKTournaments::TournamentId tournId = notification.getTournId();
    TournamentData* tData = getTournamentDataById(tournId);
    if (NULL != tData)
    {
        decrementOnlineMemberCount(tournId);

        if (tData->getTotalMemberCount() > 0)
        {
            tData->setTotalMemberCount(tData->getTotalMemberCount() - 1);
        }
        
        if (mMetrics.mGaugeTotalMembers > 0)
        {
            --(mMetrics.mGaugeTotalMembers);
        }

        ++(mMetrics.mTotalLeaves);

    }
}

void OSDKTournamentsSlaveImpl::onTournamentMemberLoginNotification(const Blaze::OSDKTournaments::TournamentMemberLoginNotification& data, Blaze::UserSession *)
{
    UserSessionId sessionId = data.getSessionId();
    TournamentId tournId = data.getTournId();

    if (INVALID_TOURNAMENT_ID != tournId)
    {
        // cache TournamentId to participants map with UserSessionId as a key
        mParticipantsMap[sessionId] = tournId;
    }

    TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].handleMemberLogin tournId : " << tournId);
    incrementOnlineMemberCount(tournId);
}

void OSDKTournamentsSlaveImpl::handleMemberLogin(UserSessionId sessionId)
{
    TournamentId tournId = getTournamentIdFromSession(sessionId);

    if (INVALID_TOURNAMENT_ID != tournId)
    {
        TournamentMemberLoginNotification notification;
        notification.setSessionId(sessionId);
        notification.setTournId(tournId);

        sendTournamentMemberLoginNotificationToAllSlaves(&notification);
    }
}

void OSDKTournamentsSlaveImpl::handleMemberLogout(UserSessionId sessionId)
{
    // get from cache, because UED is already destroyed at this point
    TournamentId tournId = mParticipantsMap[sessionId];

    // remove cached item from mParticipantsMap
    mParticipantsMap.erase(sessionId);

    TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].handleMemberLogout tournId : " << tournId);
    decrementOnlineMemberCount(tournId);
}

void OSDKTournamentsSlaveImpl::onUserSessionExistence(const UserSession& userSession)
{
    TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].onUserSessi onLogin handling user login.");

    if (true == userSession.isLocal())
    {
        gSelector->scheduleFiberCall(this, &OSDKTournamentsSlaveImpl::handleMemberLogin, userSession.getUserSessionId(), "OSDKTournamentsSlaveImpl::handleMemberLogin");
    }
}

void OSDKTournamentsSlaveImpl::onUserSessionExtinction(const UserSession& userSession)
{
    TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].onUserSessionExtinction handling user logout.");

    gSelector->scheduleFiberCall(this, &OSDKTournamentsSlaveImpl::handleMemberLogout, userSession.getUserSessionId(), "OSDKTournamentsSlaveImpl::handleMemberLogout");
}

TournamentId OSDKTournamentsSlaveImpl::getTournamentIdFromSession(UserSessionId userSessionId)
{
	TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].getTournamentIdFromSession handling tournament id from session.");

    TournamentId tournId = INVALID_TOURNAMENT_ID;

    UserSessionExtendedData ued;
    BlazeRpcError err = gUserSessionManager->getRemoteUserExtendedData(userSessionId, ued);
    if (ERR_OK != err)
    {
        ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].getTournamentIdFromSession:  Failed to fetch UED for userSessionId(" << userSessionId << ")");
        return tournId;
    }

    const BlazeObjectIdList &list = ued.getBlazeObjectIdList();
    for (BlazeObjectIdList::const_iterator it = list.begin(); it != list.end(); it++)
    {

        if (ENTITY_TYPE_TOURNAMENT == (*it).type)
        {
            TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].getTournamentIdFromSession got tournamentId from UED.");
            tournId = static_cast<TournamentId>((*it).id);
            break;
        }
	}

    return tournId;
}

void OSDKTournamentsSlaveImpl::incrementOnlineMemberCount(TournamentId tournId)
{
    if (INVALID_TOURNAMENT_ID != tournId)
    {
        TournamentData* tournData = getTournamentDataById(tournId);
        if (NULL != tournData)
        {
            ++(mMetrics.mGaugeOnlineMembers);
            tournData->setOnlineMemberCount(tournData->getOnlineMemberCount() + 1);
        }
    }
}

void OSDKTournamentsSlaveImpl::decrementOnlineMemberCount(TournamentId tournId)
{
    if (INVALID_TOURNAMENT_ID != tournId)
    {
        TournamentData* tournData = getTournamentDataById(tournId);
        if (NULL != tournData)
        {
            if (tournData->getOnlineMemberCount() > 0)
            {
                tournData->setOnlineMemberCount(tournData->getOnlineMemberCount() - 1);
            }

            if (mMetrics.mGaugeOnlineMembers > 0)
            {
                --(mMetrics.mGaugeOnlineMembers);
            }
        }
    }
}

BlazeRpcError OSDKTournamentsSlaveImpl::leaveTournament(TournamentMemberId memberId, TournamentId tournamentId)
{
    // determine what member id to use depending on the tournament mode
    TournamentData* data = getTournamentDataById(tournamentId);
    if (NULL == data)
    {
        return OSDKTOURN_ERR_TOURNAMENT_NOT_FOUND;
    }
    
    DbConnPtr dbConnection = gDbScheduler->getConnPtr(getDbId());
    if (dbConnection == NULL)
    {
        ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].execute() - Failed to obtain connection.");
        return OSDKTOURN_ERR_TOURNAMENT_GENERIC;
    }

    OSDKTournamentsDatabase tournDb(dbConnection);
    if (false == tournDb.startTransaction())
    {
        return OSDKTOURN_ERR_TOURNAMENT_GENERIC;
    }

    // is this member even in this tournament?
    TournamentMemberData memberData;
    if (DB_ERR_OK != tournDb.getMemberStatus(memberId, data->getMode(), tournamentId, memberData))
    {
        // early return when unable to determine member status
        tournDb.completeTransaction(false);
        return OSDKTOURN_ERR_MEMBER_NOT_IN_TOURNAMENT;
    }

    DbError result = tournDb.deleteTree(memberId, tournamentId);
    if (DB_ERR_OK == result)
    {
        result = tournDb.removeMember(tournamentId, memberId);
    }   
    else
    {
        // early return when unable to delete the member's tree
        tournDb.completeTransaction(false);
        return ERR_SYSTEM;
    }

    LeaveTournamentNotification notification;
    notification.setTournId(tournamentId);
    notification.setMemberId(memberId);
    sendLeaveTournamentNotificationToAllSlaves(&notification);

    if (false == tournDb.completeTransaction(true))
    {
        // early return when unable to complete the transaction
        return OSDKTOURN_ERR_TOURNAMENT_GENERIC;
    }

    Blaze::BlazeRpcError leaveError = removeUserExtendedData(memberId, tournamentId);

    return leaveError;
}

TournamentMemberId OSDKTournamentsSlaveImpl::getCurrentUserMemberIdForTournament(TournamentId tournamentId)
{
    TournamentMemberId memberId = INVALID_TOURNAMENT_MEMBER_ID;

    // find out what mode the tournament is in
    DbConnPtr conn = gDbScheduler->getConnPtr(mDbId);
    if (conn == NULL)
    {
        // early return here when the scheduler is unable to find a connection
        ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].getCurrentUserMemberIdForTournament: unable to find a database connection");
        return memberId;
    }
    OSDKTournamentsDatabase tournDb(conn);

    TournamentData* tournData = getTournamentDataById(tournamentId);
    if (NULL != tournData)
    {
        TournamentMode tournMode = tournData->getMode();
        TournamentMemberId tempMemberId = getCurrentUserMemberIdForMode(tournMode);

        // now look up the member status
        TournamentMemberData memberData;
        if (INVALID_TOURNAMENT_MEMBER_ID != tempMemberId && 
            DB_ERR_OK == tournDb.getMemberStatus(tempMemberId, tournMode, tournamentId, memberData))
        {
            TournamentId tournId = memberData.getTournamentId();
            if (INVALID_TOURNAMENT_ID != tournId)
            {
                // valid member, valid tournament
                memberId = tempMemberId;
            }
        }
    }

    return memberId;
}

void OSDKTournamentsSlaveImpl::getStatusInfo(ComponentStatus& status) const
{
    ComponentStatus::InfoMap& map = status.getInfo();
    char8_t str[256];

    blaze_snzprintf(str, sizeof(str), "%d", static_cast<int>(mTournamentsMap.size()));
    map["GaugeActiveTournaments"] = str;

    blaze_snzprintf(str, sizeof(str), "%d", mMetrics.mTotalMatchesReported);
    map["TotalMatchesReported"] = str;
    blaze_snzprintf(str, sizeof(str), "%d", mMetrics.mTotalTreeRetrievals);
    map["TotalTreeRetrievals"] = str;
    blaze_snzprintf(str, sizeof(str), "%d", mMetrics.mTotalMatchesSaved);
    map["TotalMatchesSaved"] = str;
    
    blaze_snzprintf(str, sizeof(str), "%d", mMetrics.mGaugeOnlineMembers);
    map["GaugeOnlineTournamentMembers"] = str;
    blaze_snzprintf(str, sizeof(str), "%d", mMetrics.mGaugeTotalMembers);
    map["GaugeTotalTournamentMembers"] = str;
    blaze_snzprintf(str, sizeof(str), "%d", mMetrics.mTotalJoins);
    map["TotalJoins"] = str;
    blaze_snzprintf(str, sizeof(str), "%d", mMetrics.mTotalLeaves);
    map["TotalLeaves"] = str;
}

BlazeRpcError OSDKTournamentsSlaveImpl::onLoadExtendedData(const UserInfoData &data, UserSessionExtendedData &extendedData)
{
    BlazeId userId = data.getId();
    TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].onLoadExtendedData  user ID: " << userId );

    onLoadExtendedDataCustom(data, extendedData);
    return ERR_OK;
}

GetTournamentsError::Error OSDKTournamentsSlaveImpl::processGetTournaments(const GetTournamentsRequest &request, GetTournamentsResponse &response, const Message* message)
{
    assembleTournamentList(response.getTournamentList(), message->getPeerInfo().getLocale(), request.getMode());

    return GetTournamentsError::ERR_OK;
}

GetAllTournamentsError::Error OSDKTournamentsSlaveImpl::processGetAllTournaments(GetAllTournamentsResponse &response, const Message* message)
{
    assembleTournamentList(response.getTournamentList(), message->getPeerInfo().getLocale());

    return GetAllTournamentsError::ERR_OK;
}

void OSDKTournamentsSlaveImpl::assembleTournamentList(TournamentDataList &toList, uint32_t locale, TournamentMode mode)
{
    TournamentsMap::iterator itr = mTournamentsMap.begin();
    TournamentsMap::iterator end = mTournamentsMap.end();

    for (; itr != end; ++itr)
    {
        if (0 == mode || (0 != mode && itr->second->getMode() == mode))
        {
            TournamentData* tournament = BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "TournamentData") TournamentData();
            itr->second->copyInto(*tournament);
            tournament->setName(gLocalization->localize(tournament->getName(), locale));
            tournament->setDescription(gLocalization->localize(tournament->getDescription(), locale));
            tournament->setTrophyName(gLocalization->localize(tournament->getTrophyName(), locale));
            toList.push_back(tournament);
        }
    }
}

GetMemberCountsError::Error OSDKTournamentsSlaveImpl::processGetMemberCounts(const GetMemberCountsRequest &request, GetMemberCountsResponse &response, const Message* message)
{
    TournamentData* tournament = getTournamentDataById(request.getTournamentId());
    if (NULL == tournament)
    {
        return GetMemberCountsError::OSDKTOURN_ERR_TOURNAMENT_NOT_FOUND;
    }
    response.setTotalMemberCount(tournament->getTotalMemberCount());
    response.setOnlineMemberCount(tournament->getOnlineMemberCount());
    response.setTournamentId(request.getTournamentId());
    return GetMemberCountsError::ERR_OK;
}

BlazeRpcError OSDKTournamentsSlaveImpl::addUserExtendedData(BlazeId userId, TournamentId tournamentId)
{
    BlazeRpcError err = Blaze::ERR_OK;
    if (INVALID_TOURNAMENT_ID != tournamentId)
    {
        BlazeObjectId objId = BlazeObjectId(ENTITY_TYPE_TOURNAMENT, tournamentId);

        UserSessionIdList sessionIds;
        gUserSessionManager->getSessionIds(userId, sessionIds);

        UserSessionIdList::const_iterator sessionIdsIter = sessionIds.begin();
        UserSessionIdList::const_iterator sessionIdsEnd = sessionIds.end();

        for (; sessionIdsIter != sessionIdsEnd; ++sessionIdsIter )
        {
            err = gUserSessionManager->insertBlazeObjectIdForSession(*(sessionIdsIter), objId);

            if (ERR_OK != err)
            {
                ERR_LOG("[OSDKTournamentsSlaveImpl::addUserExtendedData]: insertBlazeObject failed for session " << *(sessionIdsIter));
            } else 
            {
                // cache into mParticipantsMap
                mParticipantsMap[*(sessionIdsIter)] = tournamentId;
            }
        }
    }
    return err;
}

BlazeRpcError OSDKTournamentsSlaveImpl::removeUserExtendedData(BlazeId userId, TournamentId tournamentId)
{
    BlazeObjectId objId = BlazeObjectId(ENTITY_TYPE_TOURNAMENT, tournamentId);

    BlazeRpcError err = Blaze::ERR_OK;

    UserSessionIdList sessionIds;
    gUserSessionManager->getSessionIds(userId, sessionIds);

    UserSessionIdList::const_iterator sessionIdsIter = sessionIds.begin();
    UserSessionIdList::const_iterator sessionIdsEnd = sessionIds.end();

    for (; sessionIdsIter != sessionIdsEnd; ++sessionIdsIter )
    {
        err = gUserSessionManager->removeBlazeObjectIdForSession(*(sessionIdsIter), objId);

        if (ERR_OK != err)
        {
            ERR_LOG("[OSDKTournamentsSlaveImpl::removeUserExtendedData]: removeBlazeObject failed for session " << *(sessionIdsIter));
        } else
        {
            // remove cached item from mParticipantsMap
            mParticipantsMap.erase(*(sessionIdsIter));
        }
    }

    return err;
}

BlazeRpcError OSDKTournamentsSlaveImpl::reportMatchResult(TournamentId tournId, TournamentMemberId memberOneId, TournamentMemberId memberTwoId, 
    TeamId memberOneTeam, TeamId memberTwoTeam, uint32_t memberOneScore, uint32_t memberTwoScore, const char8_t* memberOneMetaData, const char8_t* memberTwoMetaData)
{
    ++(mMetrics.mTotalMatchesReported);
    TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: TournId: " << tournId << ", memberOne: " << memberOneId << ", memberTwo: " << memberTwoId );

    TournamentData* tournData = getTournamentDataById(tournId);
    if (NULL == tournData)
    {
        ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: User reporting tournament match result for a tournament that doesn't exist."
            " TournId: " << tournId << ", memberOne: " << memberOneId << ", memberTwo: " << memberTwoId );
        return OSDKTOURN_ERR_TOURNAMENT_NOT_FOUND;
    }

    DbConnPtr conn = gDbScheduler->getConnPtr(mDbId);
    if (conn == NULL)
    {
        // early return here when the scheduler is unable to find a connection
        WARN_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: unable to acquire db connection");
        return ERR_SYSTEM;
    }
    OSDKTournamentsDatabase tournDb(conn);

    if (false == tournDb.startTransaction())
    {
        // early return here when the transaction is unable to start
        TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: could not start db transaction");
        return ERR_SYSTEM;
    }

    TournamentMemberData memberOneMemberData;
    TournamentMemberData memberTwoMemberData;
    if ((Blaze::DB_ERR_OK == tournDb.getMemberStatus(memberOneId, tournData->getMode(), tournId, memberOneMemberData)) &&
        (Blaze::DB_ERR_OK == tournDb.getMemberStatus(memberTwoId, tournData->getMode(), tournId, memberTwoMemberData)))
    {
        //make sure that the members are both in the tournament and active
        if ((tournId != memberOneMemberData.getTournamentId()) || (tournId != memberTwoMemberData.getTournamentId()) || 
            (false == memberOneMemberData.getIsActive()) || (false == memberTwoMemberData.getIsActive()))
        {
            WARN_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: Member reporting tournament match result for a tournament they are not in or are inactive in."
                " TournId: " << tournId << ", memberOne: " << memberOneId << ", memberTwo: " << memberTwoId );
            tournDb.completeTransaction(false);
            return OSDKTOURN_ERR_MEMBER_NOT_IN_TOURNAMENT;
        }

        if (memberOneScore == memberTwoScore)
        {
            ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: User reporting tournament match result for a tournament with a tie"
                "Ties are not supported TournId: " << tournId << ", memberOne: " << memberOneId << ", memberTwo: " << memberTwoId );
            tournDb.completeTransaction(false);
            return OSDKTOURN_ERR_TIES_NOT_SUPPORTED;
        }

        //need to retrieve the trees before inserting the new match
        TournamentNodeMap memberOneNodeMap;
        uint32_t memberOneLastMatchId = 0;
        if (DB_ERR_OK != tournDb.getTreeForMember(memberOneId, &memberOneMemberData, tournData, memberOneLastMatchId, memberOneNodeMap))
        {
            // early return here for a tree not being fetch-able for a member
            TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: unable to fetch tree for member " << memberOneId);
            tournDb.completeTransaction(false);
            return ERR_SYSTEM;
        }

        TournamentNodeMap memberTwoNodeMap;
        uint32_t memberTwoLastMatchId = 0;
        if (DB_ERR_OK != tournDb.getTreeForMember(memberTwoId, &memberTwoMemberData, tournData, memberTwoLastMatchId, memberTwoNodeMap))
        {
            // early return here for a tree not being fetch-able for a member
            TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: unable to fetch tree for member " << memberTwoId);
            tournDb.completeTransaction(false);
            releaseMemoryForTournamentNodeMap(memberOneNodeMap);
            return ERR_SYSTEM;
        }

        TournamentMemberId winnerId = INVALID_BLAZE_ID;
        TournamentMemberId loserId = INVALID_BLAZE_ID;
        uint32_t winnerLevel = 0;
        uint32_t loserLevel = 0;
        uint32_t memberOneRightSubtreeId = 0;
        uint32_t memberTwoRightSubtreeId = 0;
        uint32_t memberOneDepth = 0;
        uint32_t memberTwoDepth = 0;

        if (memberOneScore > memberTwoScore)
        {
            winnerId = memberOneId;
            loserId = memberTwoId;
            winnerLevel = memberOneMemberData.getLevel() + 1;
            loserLevel = memberTwoMemberData.getLevel();
            //each member takes on the other members tree as a subtree, but the tree will be cropped if their levels are different

            //if this is the members first match, we don't need to try to add a subtree
            if (false == memberOneNodeMap.empty())
            {
                if (DB_ERR_OK != tournDb.addSubtree(memberOneId, memberTwoLastMatchId, memberOneRightSubtreeId, &memberTwoNodeMap, winnerLevel - 1, memberOneDepth))
                {
                    // early return here for being unable to add a new subtree for a member
                    TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: unable to add subtree for member " << memberOneId);
                    tournDb.completeTransaction(false);
                    releaseMemoryForTournamentNodeMap(memberTwoNodeMap);
                    releaseMemoryForTournamentNodeMap(memberOneNodeMap);
                    return ERR_SYSTEM;
                }
            }
            if (false == memberTwoNodeMap.empty())
            {
                if (DB_ERR_OK != tournDb.addSubtree(memberTwoId, memberOneLastMatchId, memberTwoRightSubtreeId, &memberOneNodeMap, loserLevel, memberTwoDepth))
                {
                    // early return here for being unable to add a new subtree for a member
                    TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: unable to add subtree for member " << memberTwoId);
                    tournDb.completeTransaction(false);
                    releaseMemoryForTournamentNodeMap(memberTwoNodeMap);
                    releaseMemoryForTournamentNodeMap(memberOneNodeMap);
                    return ERR_SYSTEM;
                }
            }
        }
        else if (memberTwoScore > memberOneScore)
        {
            loserId = memberOneId;
            winnerId = memberTwoId;
            loserLevel = memberOneMemberData.getLevel();
            winnerLevel = memberTwoMemberData.getLevel() + 1;
            //each member takes on the other members tree as a subtree, but the tree will be cropped if their levels are different
            
            //if this is the members first match, we don't need to try to add a subtree
            if (false == memberTwoNodeMap.empty())
            {
                if (DB_ERR_OK != tournDb.addSubtree(memberTwoId, memberOneLastMatchId, memberTwoRightSubtreeId, &memberOneNodeMap, winnerLevel - 1, memberTwoDepth))
                {
                    // early return here for being unable to add a new subtree for a member
                    TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: unable to add subtree for member " << memberTwoId);
                    tournDb.completeTransaction(false);
                    releaseMemoryForTournamentNodeMap(memberTwoNodeMap);
                    releaseMemoryForTournamentNodeMap(memberOneNodeMap);
                    return ERR_SYSTEM;
                }
            }
            if (false == memberOneNodeMap.empty())
            {
                if (DB_ERR_OK != tournDb.addSubtree(memberOneId, memberTwoLastMatchId, memberOneRightSubtreeId, &memberTwoNodeMap, loserLevel, memberOneDepth))
                {
                    TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: unable to add subtree for member " << memberOneId);
                    tournDb.completeTransaction(false);
                    releaseMemoryForTournamentNodeMap(memberTwoNodeMap);
                    releaseMemoryForTournamentNodeMap(memberOneNodeMap);
                    return ERR_SYSTEM;
                }
            }
        }
        TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: Member " << winnerId << " won");

        // fill meta-data from custom functions if previously empty
        char8_t memberOneMetaDataCustom[TOURN_MATCH_META_DATA_MAX_LENGTH];
        if (0 == blaze_strcmp(memberOneMetaData, ""))
        {
            getMemberMetaData(memberOneId, tournData->getMode(), &memberOneMetaDataCustom[0], TOURN_MATCH_META_DATA_MAX_LENGTH-1);
        }
        else
        {
            // keep the old string
            strcpy(memberOneMetaDataCustom, memberOneMetaData);
        }

        char8_t memberTwoMetaDataCustom[TOURN_MATCH_META_DATA_MAX_LENGTH];
        if (0 == blaze_strcmp(memberTwoMetaData, ""))
        {
            getMemberMetaData(memberTwoId, tournData->getMode(), &memberTwoMetaDataCustom[0],TOURN_MATCH_META_DATA_MAX_LENGTH-1);
        }
        else
        {
            // keep the old string
            strcpy(memberTwoMetaDataCustom, memberTwoMetaData);
        }

        TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: MemberOne metadata: " << memberOneMetaDataCustom << ", MemberTwo metadata: " << memberTwoMetaDataCustom);

        TournamentMatchNode memberOneNode(tournId, memberOneId, memberTwoId, memberOneTeam, memberTwoTeam, memberOneScore, memberTwoScore, memberOneLastMatchId, 
            memberOneRightSubtreeId, memberOneMemberData.getTournAttribute(), memberTwoMemberData.getTournAttribute(), memberOneMetaDataCustom, memberTwoMetaDataCustom);

        TournamentMatchNode memberTwoNode(tournId, memberTwoId, memberOneId, memberTwoTeam, memberOneTeam, memberTwoScore, memberOneScore, memberTwoLastMatchId, 
            memberTwoRightSubtreeId, memberTwoMemberData.getTournAttribute(), memberOneMemberData.getTournAttribute(), memberTwoMetaDataCustom, memberOneMetaDataCustom);

        uint32_t memberOneNewMatchId = 0;
        if (DB_ERR_OK != tournDb.insertMatchNode(&memberOneNode, memberOneId, memberOneNewMatchId))
        {
            // early return here for being unable to insert a match for a member
            TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: unable to insert match node for memberOne");
            tournDb.completeTransaction(false);
            releaseMemoryForTournamentNodeMap(memberTwoNodeMap);
            releaseMemoryForTournamentNodeMap(memberOneNodeMap);
            return ERR_SYSTEM;
        }
        uint32_t memberTwoNewMatchId = 0;
        if (DB_ERR_OK != tournDb.insertMatchNode(&memberTwoNode, memberTwoId, memberTwoNewMatchId))
        {
            // early return here for being unable to insert a match for a member
            TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: unable to insert match node for memberTwo ");
            tournDb.completeTransaction(false);
            releaseMemoryForTournamentNodeMap(memberTwoNodeMap);
            releaseMemoryForTournamentNodeMap(memberOneNodeMap);
            return ERR_SYSTEM;
        }

        uint32_t winnerNewMatchId = 0;
        uint32_t loserNewMatchId = 0;
        TeamId winnerTeam = -1;
        TeamId loserTeam = -1;

        if (memberOneScore > memberTwoScore)
        {
            winnerNewMatchId = memberOneNewMatchId;
            loserNewMatchId = memberTwoNewMatchId;
            winnerTeam = memberOneTeam;
            loserTeam = memberTwoTeam;
        }
        else if (memberTwoScore > memberOneScore)
        {
            winnerNewMatchId = memberTwoNewMatchId;
            loserNewMatchId = memberOneNewMatchId;
            winnerTeam = memberTwoTeam;
            loserTeam = memberOneTeam;
        }

        //need to update the last match ids, level, active flags and award any trophies.
        if (winnerLevel > tournData->getNumRounds())
        {
            //the member has won all the tournament.  award a trophy and set them as inactive
            if (DB_ERR_OK != tournDb.awardTrophy(winnerId, tournId) ||
                DB_ERR_OK != tournDb.updateMemberStatus(winnerId, tournId, false, winnerLevel, winnerNewMatchId, winnerTeam))
            {
                // early return here - unable to award trophy or update member status
                TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: unable to award trophy or update winner member status for member " << winnerId);
                tournDb.completeTransaction(false);
                releaseMemoryForTournamentNodeMap(memberTwoNodeMap);
                releaseMemoryForTournamentNodeMap(memberOneNodeMap);
                return ERR_SYSTEM;
            }
            else
            {
                // notify custom code
                onMemberWonCustom(winnerId, tournData->getMode());
            }
        }
        else
        {
            if (DB_ERR_OK != tournDb.updateMemberStatus(winnerId, tournId, true, winnerLevel, winnerNewMatchId, winnerTeam))
            {
                // early return here for being unable to update member status
                TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: unable to update winner member status for member " << winnerId);
                tournDb.completeTransaction(false);
                releaseMemoryForTournamentNodeMap(memberTwoNodeMap);
                releaseMemoryForTournamentNodeMap(memberOneNodeMap);
                return ERR_SYSTEM;
            }
        }
        
        if (DB_ERR_OK != tournDb.updateMemberStatus(loserId, tournId, false, loserLevel, loserNewMatchId, loserTeam))
        {
            // early return here for being unable to update member status
            TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: unable to update loser member status for member " << loserId);
            tournDb.completeTransaction(false);
            releaseMemoryForTournamentNodeMap(memberTwoNodeMap);
            releaseMemoryForTournamentNodeMap(memberOneNodeMap);
            return ERR_SYSTEM;
        }
        
        // notify custom code - the winner advanced and the loser was eliminated
        onMemberEliminatedCustom(loserId, tournData->getMode(), loserLevel);
        onMemberAdvancedCustom(winnerId, tournData->getMode(), winnerLevel);

        // everything successful - complete transaction!
        tournDb.completeTransaction(true);
        ++(mMetrics.mTotalMatchesSaved);
        
        // clean-up
        releaseMemoryForTournamentNodeMap(memberTwoNodeMap);
        releaseMemoryForTournamentNodeMap(memberOneNodeMap);
    }
    else
    {
        // the status of either member is un-determinable
        TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].reportMatchResult: the status of either member is un-determinable");
        tournDb.completeTransaction(false);
        return ERR_SYSTEM;
    }

    return ERR_OK;
}

void OSDKTournamentsSlaveImpl::releaseMemoryForTournamentNodeMap(TournamentNodeMap& tournamentNodeMap)
{
    TournamentNodeMap::iterator mapItr = tournamentNodeMap.begin();
    TournamentNodeMap::iterator mapEnd = tournamentNodeMap.end();
    for (; mapItr != mapEnd; ++mapItr)
    {
        delete mapItr->second;
    }

}

} //namespace OSDKTournaments
} //namespace Blaze
