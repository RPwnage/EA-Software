/*************************************************************************************************/
/*!
    \file   sponsoredeventsslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SponsoredEventsSlaveImpl

    SponsoredEvents Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "sponsoredeventsslaveimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"

#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

// sponsoredevents includes
#include "sponsoredevents/tdf/sponsoredeventstypes.h"

//stats
#include "stats/statsslaveimpl.h"

//authentication
#include "authentication/authenticationimpl.h"

namespace Blaze
{
namespace SponsoredEvents
{

/*** Database prepared statements ****************************************************************/

// get user registration information
static const char8_t SPONSOREDEVENTS_GET_USER_REGISTRATION[] =
	"SELECT * "
	"FROM `sponsoredevents_registration` "
	"WHERE `userId` = ? AND `eventId` = ?";

// insert a new user registration
static const char8_t SPONSOREDEVENTS_REGISTER_USER[] =
	"INSERT INTO `sponsoredevents_registration` ("
	"`userId`, `eventId`, `eventData`, `title`, `platform`, `country`, `creationdate`, `sendinformation`, `whobanned`, `whybanned`"
	") VALUES(?,?,?,?,?,?,?,?,?,?)";

static const char8_t SPONSOREDEVENTS_BAN_USER[] =
	"UPDATE `sponsoredevents_registration` "
	"SET `ban` = ?, `whobanned` = ?, `whybanned` = ? "
	"WHERE `userID` = ? and `eventID` = ? and `title` = ? and `platform` = ?";

static const char8_t *SPONSOREDEVENTS_NUM_USERS[] = {
	"SELECT COUNT(*) FROM sponsoredevents_registration; ", 
	"SELECT COUNT(*) FROM sponsoredevents_registration WHERE `eventId` = ?"
};

static const char8_t SPONSOREDEVENTS_REMOVE_USER[] =
	"DELETE FROM sponsoredevents_registration WHERE userId = ? and eventId = ? and title = ? and platform = ? limit 1;";

static const char8_t SPONSOREDEVENTS_UPDATE_USER_FLAGS[] =
	"UPDATE sponsoredevents_registration SET flags = ? WHERE userId = ? and eventId = ? and title = ? and platform = ?; ";

static const char8_t SPONSOREDEVENTS_UPDATE_EVENT_DATA[] =
	"UPDATE sponsoredevents_registration SET eventData = ? WHERE userId = ? and eventId = ? and title = ? and platform = ?";

static const char8_t *SPONSOREDEVENTS_RETURN_USERS[] = {
	"SELECT * FROM sponsoredevents_registration LIMIT ?, ?; ",
	"SELECT * FROM sponsoredevents_registration WHERE userId = ? and eventId = ? and title = ? and platform = ?; ",
	"SELECT * FROM sponsoredevents_registration WHERE eventId = ? LIMIT ?, ?; "
};

/*** class statics ******************************************************************************/
EA_THREAD_LOCAL PreparedStatementId SponsoredEventsSlaveImpl::mDbCmdGetUserRegistration		= 0;
EA_THREAD_LOCAL PreparedStatementId SponsoredEventsSlaveImpl::mDbCmdRegisterUser			= 0;
EA_THREAD_LOCAL PreparedStatementId SponsoredEventsSlaveImpl::mDbCmdBanUser					= 0;
EA_THREAD_LOCAL PreparedStatementId SponsoredEventsSlaveImpl::mDbCmdNumUsers[2]				= {0, 0};
EA_THREAD_LOCAL PreparedStatementId SponsoredEventsSlaveImpl::mDbCmdRemoveUser				= 0;
EA_THREAD_LOCAL PreparedStatementId SponsoredEventsSlaveImpl::mDbCmdUpdateUserFlags			= 0;
EA_THREAD_LOCAL PreparedStatementId SponsoredEventsSlaveImpl::mDbCmdUpdateEventData			= 0;
EA_THREAD_LOCAL PreparedStatementId SponsoredEventsSlaveImpl::mDbCmdReturnUsers[3]			= {0, 0, 0};

SponsoredEventsSlave* SponsoredEventsSlave::createImpl()
{
    return BLAZE_NEW_NAMED("SponsoredEventsSlaveImpl") SponsoredEventsSlaveImpl();
}

/*** Public Methods ******************************************************************************/
SponsoredEventsSlaveImpl::SponsoredEventsSlaveImpl()
: mDbId(DbScheduler::INVALID_DB_ID)
{
}

SponsoredEventsSlaveImpl::~SponsoredEventsSlaveImpl()
{
}

int32_t SponsoredEventsSlaveImpl::CheckUserRegistrationCommand(const CheckUserRegistrationRequest &userData, CheckUserRegistrationResponse &response)
{
	mMetrics.mUserRegistrationChecks.add(1);

	response.setIsRegistered(eUnknown);
	response.setEventID(userData.getEventID());
	
	DbConnPtr dbConn = gDbScheduler->getConnPtr(mDbId);
	if(dbConn == NULL)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].GetUserRegistration(): Failed to get database connection");
		return  SPONSORED_EVENTS_ERR_DB;
	}

	PreparedStatement *statement = dbConn->getPreparedStatement(mDbCmdGetUserRegistration);
	if(NULL == statement)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].GetUserRegistration(): Failed to get prepared statement");
		return SPONSORED_EVENTS_ERR_DB;
	}

	statement->setInt64(0, userData.getUserID());
	statement->setInt32(1, userData.getEventID());

	DbResultPtr dbResult;
	BlazeRpcError dbError = dbConn->executePreparedStatement(statement, dbResult);
	if(DB_ERR_OK != dbError)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].GetUserRegistration(): Failed to execute statement");
		return SPONSORED_EVENTS_ERR_DB;
	}	

	if(dbResult->empty())
		response.setIsRegistered(eNotRegistered);
	else
	{
		DbRow* thisRow = *(dbResult->begin());
		const char8_t* eventData = thisRow->getString("eventData");
		const int isBanned = thisRow->getInt("ban");
		response.setIsRegistered((int8_t)(isBanned?eBanned:eRegistered));
		response.setEventData(eventData);
	}

	return ERR_OK;
}

int32_t SponsoredEventsSlaveImpl::RegisterUserCommand(const RegisterUserRequest &userData, RegisterUserResponse &response)
{
	mMetrics.mUserRegistrations.add(1);
	response.setEventID(userData.getEventID());
	
	DbConnPtr dbConn = gDbScheduler->getConnPtr(mDbId);
	if(dbConn == NULL)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].RegisterUserForEvent(): Failed to get database connection");
		return SPONSORED_EVENTS_ERR_DB;
	}

	PreparedStatement *statement = dbConn->getPreparedStatement(mDbCmdRegisterUser);
	if(NULL == statement)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].RegisterUserForEvent(): Failed to get prepared statement");
		return SPONSORED_EVENTS_ERR_DB;
	}

	//creationData
	time_t registrationTime = time(NULL);   // get time now
    struct tm *pLocalRegistrationTime = localtime(&registrationTime);
	eastl::string szLocalTime;
	szLocalTime.sprintf("%d-%d-%d", pLocalRegistrationTime->tm_year + 1900, pLocalRegistrationTime->tm_mon + 1, pLocalRegistrationTime->tm_mday);

	statement->setInt64(0, userData.getUserID());
	statement->setInt32(1, userData.getEventID());
	statement->setString(2, userData.getEventData());
	statement->setString(3, userData.getGameTitle());
	statement->setString(4, userData.getGamePlatform());
	statement->setString(5, userData.getCountry());
	statement->setString(6, szLocalTime.c_str());
	statement->setInt8(7, userData.getSendInformation());
	statement->setString(8, "");
	statement->setString(9, "");

	DbResultPtr dbResult;
	BlazeRpcError dbError = dbConn->executePreparedStatement(statement, dbResult);
	if(DB_ERR_OK != dbError)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].RegisterUserForEvent(): Failed to execute statement");
		return SPONSORED_EVENTS_ERR_DB;
	}

	return ERR_OK;
}

int32_t SponsoredEventsSlaveImpl::GetEventsURLCommand(URLResponse& eventsURL)
{
	mMetrics.mEventsURLRequests.add(1);

	const char8_t* url = getConfig().getEventsURL();
	if(url == NULL || blaze_strcmp(url, "") == 0)
	{
		return SPONSORED_EVENTS_ERR_DB;
	}

	eventsURL.setURL(url);

	return ERR_OK;
}

int32_t	SponsoredEventsSlaveImpl::ReturnUsersCommand(const ReturnUsersRequest &userData, ReturnUsersResponse &response)
{
	mMetrics.mReturnUsers.add(1);

	DbConnPtr dbConn = gDbScheduler->getConnPtr(mDbId);
	if(dbConn == NULL)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].ReturnUsersCommand(): Failed to get database connection");
		return SPONSORED_EVENTS_ERR_DB;
	}

	int32_t row_count = userData.getRowCount();
	static int32_t MAX_ROWS_RETURNED = 40;
	
	if ( row_count > MAX_ROWS_RETURNED ||  0 == row_count)
    {
        row_count = MAX_ROWS_RETURNED;
    }

	PreparedStatement *statement = NULL;
	
    if ( -1 == userData.getEventID() )
    {            
		TRACE_LOG("[ReturnUsersCommand:" << this << "].execute - return users of all the events.");
		statement = dbConn->getPreparedStatement(mDbCmdReturnUsers[0]);
		if(statement)
		{
			statement->setInt32(0, userData.getOffset());
			statement->setInt32(1, row_count);
		}
    }
    else if ( userData.getUserID() > 0 )
    {
        TRACE_LOG("[ReturnUsersCommand:" << this << "].execute - return one single record.");
		statement = dbConn->getPreparedStatement(mDbCmdReturnUsers[1]);
		if(statement)
		{
			statement->setInt64(0, userData.getUserID());
			statement->setInt32(1, userData.getEventID());
			statement->setString(2, userData.getGameTitle());
			statement->setString(3, userData.getGamePlatform());
		}
    }
    else
    {
        TRACE_LOG("[ReturnUsersCommand:" << this << "].execute - return records of event " << userData.getEventID() << ".");
		statement = dbConn->getPreparedStatement(mDbCmdReturnUsers[2]);
		if(statement)
		{
			statement->setInt32(0, userData.getEventID());
			statement->setInt32(1, userData.getOffset());
			statement->setInt32(2, row_count);
		}
    }

	if(NULL == statement)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].ReturnUsersCommand(): Failed to get prepared statement");
		return SPONSORED_EVENTS_ERR_DB;
	}

    int32_t result = 0;
	DbResultPtr dbResult;
    BlazeRpcError dbErr = dbConn->executePreparedStatement(statement, dbResult);

    if ( dbErr == DB_ERR_OK )
    {
        if ( dbResult->empty())
        {
            TRACE_LOG("[ReturnusersCommand:" << this << "].execute: no record has been found.");
            response.setNumUsers(result);
        }
        else
        {
            result += (int32_t)dbResult->size();
            response.setNumUsers(result);

            DbResult::const_iterator itr = dbResult->begin();
            DbResult::const_iterator end = dbResult->end();

            for (; itr != end; ++itr)
            {
                const DbRow *row = *itr;
                SponsoredEventsUser * onerow = BLAZE_NEW SponsoredEventsUser();

                /* convert the database row back to hex. This is because JSP (see getUserFlag.jsp)
                * calls this RPC expecting a hex. */
                onerow->setUserID( row->getInt64("uid") );

                onerow->setEventID( row->getInt("evid"));
                onerow->setGameTitle( row->getString("titl"));
                onerow->setGamePlatform( row->getString("pfrm"));
                onerow->setFlags( row->getUInt("flgs"));
                onerow->setIsBanned( static_cast<int8_t>(row->getInt("ban")) );
                
                onerow->setCountry( row->getString("co"));
				// @jusyoo - Urraca.1 upgrade
				// blaze no longer stores/caches DOB and email from Nucleus anymore (part of our PII redaction)
				//onerow->setDOB( row->getString("dob"));

                onerow->setWhoBan( row->getString("whob"));
                onerow->setWhyBan( row->getString("whyb"));

                onerow->setNote( row->getString("note"));
				onerow->setShareInfo( static_cast<int8_t>(row->getInt("sinf")) );
                onerow->setCreateDate( row->getString("cdat") );

                response.getRegisteredUsers().push_back(onerow);
            }
		}
	}

	return ERR_OK;
}

int32_t SponsoredEventsSlaveImpl::BanUserCommand(const BanUserRequest &userData, BanUserResponse &response)
{
	mMetrics.mUserBans.add(1);

	DbConnPtr dbConn = gDbScheduler->getConnPtr(mDbId);
	if(dbConn == NULL)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].BanUserCommand(): Failed to get database connection");
		return SPONSORED_EVENTS_ERR_DB;
	}

	PreparedStatement *statement = dbConn->getPreparedStatement(mDbCmdBanUser);
	if(NULL == statement)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].BanUserCommand(): Failed to get prepared statement");
		return SPONSORED_EVENTS_ERR_DB;
	}

	statement->setInt32(0, userData.getIsBanned());
	statement->setString(1, userData.getWhoBan());
	statement->setString(2, userData.getWhyBan());
	statement->setInt64(3, userData.getUserID());
	statement->setInt32(4, userData.getEventID());
	statement->setString(5, userData.getGameTitle());
	statement->setString(6, userData.getGamePlatform());

	DbResultPtr dbResult;
	BlazeRpcError dbError = dbConn->executePreparedStatement(statement, dbResult);
	if(DB_ERR_OK != dbError)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].BanUserCommand(): Failed to execute statement");
		return SPONSORED_EVENTS_ERR_DB;
	}

	return dbResult->getAffectedRowCount()?ERR_OK:SPONSORED_EVENTS_ERR_NO_UPDATE;
}

int32_t SponsoredEventsSlaveImpl::NumUsersCommand(const NumUsersRequest &userData, NumUsersResponse &response)
{
	mMetrics.mNumUsers.add(1);

	DbConnPtr dbConn = gDbScheduler->getConnPtr(mDbId);
	if(dbConn == NULL)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].NumUsersCommand(): Failed to get database connection");
		return SPONSORED_EVENTS_ERR_DB;
	}

	PreparedStatement *statement = dbConn->getPreparedStatement((userData.getEventID() == -1)?mDbCmdNumUsers[0]:mDbCmdNumUsers[1]);
	if(NULL == statement)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].NumUsersCommand(): Failed to get prepared statement");
		return SPONSORED_EVENTS_ERR_DB;
	}

	statement->setInt32(0, userData.getEventID());

	DbResultPtr dbResult;
	BlazeRpcError dbError = dbConn->executePreparedStatement(statement, dbResult);
	if(DB_ERR_OK != dbError)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].NumUsersCommand(): Failed to execute statement");
		return SPONSORED_EVENTS_ERR_DB;
	}

	return ERR_OK;
}

int32_t SponsoredEventsSlaveImpl::RemoveUserCommand(const RemoveUserRequest &userData, RemoveUserResponse &response)
{
	mMetrics.mRemoveUser.add(1);

	DbConnPtr dbConn = gDbScheduler->getConnPtr(mDbId);
	if(dbConn == NULL)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].RemoveUserCommand(): Failed to get database connection");
		return SPONSORED_EVENTS_ERR_DB;
	}

	PreparedStatement *statement = dbConn->getPreparedStatement(mDbCmdRemoveUser);
	if(NULL == statement)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].RemoveUserCommand(): Failed to get prepared statement");
		return SPONSORED_EVENTS_ERR_DB;
	}

	statement->setInt64(0, userData.getUserID());
	statement->setInt32(1, userData.getEventID());
	statement->setString(2, userData.getGameTitle());
	statement->setString(3, userData.getGamePlatform());

	DbResultPtr dbResult;
	BlazeRpcError dbError = dbConn->executePreparedStatement(statement, dbResult);
	if(DB_ERR_OK != dbError)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].RemoveUserCommand(): Failed to execute statement");
		return SPONSORED_EVENTS_ERR_DB;
	}

	return dbResult->getAffectedRowCount()?ERR_OK:SPONSORED_EVENTS_ERR_NO_UPDATE;
}

int32_t SponsoredEventsSlaveImpl::UpdateUserFlagsCommand(const UpdateUserFlagsRequest &userData, UpdateUserFlagsResponse &response)
{
	mMetrics.mUpdateUserFlags.add(1);

	DbConnPtr dbConn = gDbScheduler->getConnPtr(mDbId);
	if(dbConn == NULL)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].UpdateUserFlagsCommand(): Failed to get database connection");
		return SPONSORED_EVENTS_ERR_DB;
	}

	PreparedStatement *statement = dbConn->getPreparedStatement(mDbCmdUpdateUserFlags);
	if(NULL == statement)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].UpdateUserFlagsCommand(): Failed to get prepared statement");
		return SPONSORED_EVENTS_ERR_DB;
	}

	statement->setInt32(0, userData.getFlags());
	statement->setInt64(1, userData.getUserID());
	statement->setInt32(2, userData.getEventID());
	statement->setString(3, userData.getGameTitle());
	statement->setString(4, userData.getGamePlatform());

	DbResultPtr dbResult;
	BlazeRpcError dbError = dbConn->executePreparedStatement(statement, dbResult);
	if(DB_ERR_OK != dbError)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].UpdateUserFlagsCommand(): Failed to execute statement");
		return SPONSORED_EVENTS_ERR_DB;
	}

	return dbResult->getAffectedRowCount()?ERR_OK:SPONSORED_EVENTS_ERR_NO_UPDATE;
}

int32_t SponsoredEventsSlaveImpl::WipeUserStatsCommand(const WipeUserStatsRequest &userData, WipeUserStatsResponse &response)
{
	mMetrics.mWipeUserStats.add(1);

    if (gCurrentUserSession->isAuthorized(Blaze::Authorization::PERMISSION_WIPE_STATS) == false)
        return ERR_AUTHORIZATION_REQUIRED;

	// get stats component
    Stats::StatsSlave *stats = static_cast<Stats::StatsSlave*>(gController->getComponent(Stats::StatsSlave::COMPONENT_ID, false));

    if (stats == NULL)
    {
        ERR_LOG("[WipeUserStatsCommand:" << this << "] Clould not find stats component.");
        return ERR_SYSTEM;
    }
    
    eastl::string categoryName;
    categoryName.sprintf("%s%d", mStatsBaseName.c_str(), userData.getEventID());
    
    Stats::WipeStatsRequest wipeStatsReq;
    if (userData.getUserID() != 0)
    {
        // clear user id data
        wipeStatsReq.setCategoryName(categoryName.c_str());
        wipeStatsReq.setEntityId(static_cast<Blaze::EntityId>(userData.getUserID()));
        wipeStatsReq.setOperation(Stats::WipeStatsRequest::DELETE_BY_ENTITYID);             
    }
    else 
    {
        // clear all event stats
        wipeStatsReq.setCategoryName(categoryName.c_str());
        wipeStatsReq.setOperation(Stats::WipeStatsRequest::DELETE_BY_CATEGORY);
    }

    return (stats->wipeStats(wipeStatsReq) == Blaze::ERR_OK)?ERR_OK:ERR_SYSTEM;
}

int32_t SponsoredEventsSlaveImpl::UpdateEventDataCommand(const UpdateEventDataRequest &userData, UpdateEventDataResponse &response)
{
	mMetrics.mUpdateEventData.add(1);

	DbConnPtr dbConn = gDbScheduler->getConnPtr(mDbId);
	if(dbConn == NULL)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].UpdateEventDataCommand(): Failed to get database connection");
		return SPONSORED_EVENTS_ERR_DB;
	}

	PreparedStatement *statement = dbConn->getPreparedStatement(mDbCmdUpdateEventData);
	if(NULL == statement)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].UpdateEventDataCommand(): Failed to get prepared statement");
		return SPONSORED_EVENTS_ERR_DB;
	}
 
	statement->setString(0, userData.getEventData());
	statement->setInt64(1, userData.getUserID());
	statement->setInt32(2, userData.getEventID());
	statement->setString(3, userData.getGameTitle());
	statement->setString(4, userData.getGamePlatform());

	DbResultPtr dbResult;
	BlazeRpcError dbError = dbConn->executePreparedStatement(statement, dbResult);
	if(DB_ERR_OK != dbError)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].UpdateUserFlagsCommand(): Failed to execute statement");
		return SPONSORED_EVENTS_ERR_DB;
	}

	return dbResult->getAffectedRowCount()?ERR_OK:SPONSORED_EVENTS_ERR_NO_UPDATE;
}

int32_t	SponsoredEventsSlaveImpl::GetDbCredentialsCommand(DbCredentialsResponse &response)
{
	if (gCurrentUserSession->isAuthorized(Blaze::Authorization::PERMISSION_FIFA_SPONSORED_EVENT_ADMIN) == false)
		return ERR_AUTHORIZATION_REQUIRED;

	const DbConfig* dbConfig = gDbScheduler->getConfig(mDbId);
	if(!dbConfig)
		return SPONSORED_EVENTS_ERR_DB;

	response.setDbHost(dbConfig->getHost());
	response.setDbPort(dbConfig->getPort());
	response.setDbInstanceName(dbConfig->getName());
	response.setDbSchema(dbConfig->getDatabase());

	return ERR_OK;
}

/*** Private Methods ******************************************************************************/
bool SponsoredEventsSlaveImpl::initDB()
{
	const SponsoredEvents::SponsoredEventsConfig& config = getConfig();

	mDbName = config.getDbName();

	mDbId = gDbScheduler->getDbId(mDbName.c_str());
	if(mDbId == DbScheduler::INVALID_DB_ID)
	{
		ERR_LOG("[SponsoredEventsSlaveImpl].initDB(): Failed to initialize db");
		return false;
	}

	mStatsBaseName = config.getStatsBaseName();

	mDbCmdGetUserRegistration = gDbScheduler->registerPreparedStatement(mDbId,
		"sponsoredevents_get_user_registration", SPONSOREDEVENTS_GET_USER_REGISTRATION);

	mDbCmdRegisterUser = gDbScheduler->registerPreparedStatement(mDbId,
		"sponsoredevents_register_user", SPONSOREDEVENTS_REGISTER_USER);
	
	mDbCmdBanUser = gDbScheduler->registerPreparedStatement(mDbId,
		"sponsoredevents_ban_user", SPONSOREDEVENTS_BAN_USER);

	mDbCmdNumUsers[0] = gDbScheduler->registerPreparedStatement(mDbId,
		"sponsoredevents_num_users1", SPONSOREDEVENTS_NUM_USERS[0]);
	mDbCmdNumUsers[1] = gDbScheduler->registerPreparedStatement(mDbId,
		"sponsoredevents_num_users2", SPONSOREDEVENTS_NUM_USERS[1]);

	mDbCmdRemoveUser = gDbScheduler->registerPreparedStatement(mDbId,
		"sponsoredevents_remove_user", SPONSOREDEVENTS_REMOVE_USER);

	mDbCmdUpdateUserFlags = gDbScheduler->registerPreparedStatement(mDbId,
		"sponsoredevents_update_user_flags", SPONSOREDEVENTS_UPDATE_USER_FLAGS);

	mDbCmdUpdateEventData = gDbScheduler->registerPreparedStatement(mDbId,
		"sponsoredevents_update_event_data", SPONSOREDEVENTS_UPDATE_EVENT_DATA);
	
	mDbCmdReturnUsers[0] = gDbScheduler->registerPreparedStatement(mDbId,
		"sponsoredevents_return_users1", SPONSOREDEVENTS_RETURN_USERS[0]);
	mDbCmdReturnUsers[1] = gDbScheduler->registerPreparedStatement(mDbId,
		"sponsoredevents_return_users2", SPONSOREDEVENTS_RETURN_USERS[1]);
	mDbCmdReturnUsers[2] = gDbScheduler->registerPreparedStatement(mDbId,
		"sponsoredevents_return_users3", SPONSOREDEVENTS_RETURN_USERS[2]);

	return true;
}

bool SponsoredEventsSlaveImpl::onConfigure()
{
    TRACE_LOG("[SponsoredEventsSlaveImpl:" << this << "].configure: configuring component.");

	return initDB();
}


bool SponsoredEventsSlaveImpl::onReconfigure()
{
	TRACE_LOG("[SponsoredEventsSlaveImpl:" << this << "].reconfigure: reloading component.");

	return initDB();
}


void SponsoredEventsSlaveImpl::onVoidMasterNotification(UserSession *)
{
	TRACE_LOG("[SponsoredEventsSlaveImpl:" << this << "].onVoidMasterNotification");
}

void SponsoredEventsSlaveImpl::getStatusInfo(ComponentStatus& status) const
{
	TRACE_LOG("[SponsoredEventsSlaveImpl].getStatusInfo");

	mMetrics.report(&status);
}

} // SponsoredEvents
} // Blaze
