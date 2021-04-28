/*************************************************************************************************/
/*!
    \file   sponsoredeventsslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SPONSORED_EVENTS_SLAVEIMPL_H
#define BLAZE_SPONSORED_EVENTS_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "sponsoredevents/rpc/sponsoredeventsslave_stub.h"
#include "sponsoredevents/tdf/sponsoredeventstypes.h"
#include "framework/replication/replicationcallback.h"

#include "framework/database/preparedstatement.h"
#include "framework/database/dbresult.h"

#include "sponsoredevents/slavemetrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace SponsoredEvents
{

class RegisterUserRequest;
class CheckUserRegistrationRequest;
class CheckUserRegistrationResponse;
class BanUserRequest;
class BanUserResponse;
class NumUsersRequest;
class NumUsersResponse;
class RemoveUserRequest;
class RemoveUserResponse;
class UpdateUserFlagsRequest;
class UpdateUserFlagsResponse;
class WipeUserStatsRequest;
class WipeUserStatsResponse;
class UpdateEventDataRequest;
class UpdateEventDataResponse;
class DbCredentialsRequest;
class DbCredentialsResponse;

class SponsoredEventsSlaveImpl : public SponsoredEventsSlaveStub
{
public:
	enum eUserRegistrationStatus
	{
		eUnknown = -1,
		eNotRegistered,
		eRegistered,
		eBanned,
	};

    								SponsoredEventsSlaveImpl();
	virtual							~SponsoredEventsSlaveImpl();

			int32_t					CheckUserRegistrationCommand(const CheckUserRegistrationRequest &userData, CheckUserRegistrationResponse &response);
			int32_t					RegisterUserCommand(const RegisterUserRequest &userData, RegisterUserResponse &response);

			int32_t					BanUserCommand(const BanUserRequest &userData, BanUserResponse &response);
			int32_t					NumUsersCommand(const NumUsersRequest &userData, NumUsersResponse &response);
			int32_t					RemoveUserCommand(const RemoveUserRequest &userData, RemoveUserResponse &response);
			int32_t					UpdateUserFlagsCommand(const UpdateUserFlagsRequest &userData, UpdateUserFlagsResponse &response);
			int32_t					WipeUserStatsCommand(const WipeUserStatsRequest &userData, WipeUserStatsResponse &response);
			int32_t					UpdateEventDataCommand(const UpdateEventDataRequest &userData, UpdateEventDataResponse &response);
			int32_t					GetDbCredentialsCommand(DbCredentialsResponse &response);
			int32_t					ReturnUsersCommand(const ReturnUsersRequest &userData, ReturnUsersResponse &response);
			
			int32_t					GetEventsURLCommand(URLResponse& eventsURL);

	// Component Interface
	virtual uint16_t				getDbSchemaVersion() const	{ return 3; }
	virtual	const char8_t*			getDbName() const			{ return mDbName.c_str(); }

protected:
			bool					onConfigure();
			bool					onReconfigure();
	virtual void					onVoidMasterNotification(UserSession *);
	virtual void					getStatusInfo(ComponentStatus& status) const;

			bool					initDB();

			uint32_t				mDbId;
			eastl::string			mDbName;
			eastl::string			mStatsBaseName;

			SlaveMetrics			mMetrics;

	static EA_THREAD_LOCAL PreparedStatementId  mDbCmdGetUserRegistration;
	static EA_THREAD_LOCAL PreparedStatementId  mDbCmdRegisterUser;
	static EA_THREAD_LOCAL PreparedStatementId  mDbCmdBanUser;
	static EA_THREAD_LOCAL PreparedStatementId  mDbCmdNumUsers[2];
	static EA_THREAD_LOCAL PreparedStatementId  mDbCmdRemoveUser;
	static EA_THREAD_LOCAL PreparedStatementId  mDbCmdUpdateUserFlags;
	static EA_THREAD_LOCAL PreparedStatementId  mDbCmdUpdateEventData;
	static EA_THREAD_LOCAL PreparedStatementId  mDbCmdReturnUsers[3];
};

} // SponsoredEvents
} // Blaze

#endif // BLAZE_SPONSORED_EVENTS_SLAVEIMPL_H

