/*************************************************************************************************/
/*!
    \class ProclubsCustomSlaveImpl

    ProclubsCustomSlaveImpl implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "proclubscustomslaveimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

#include "proclubscustom/rpc/proclubscustomslave.h"
#include "proclubscustom/tdf/proclubscustomtypes.h"
#include "proclubscustom/tdf/proclubscustom_server.h"

namespace Blaze
{
namespace proclubscustom
{
// static
proclubscustomSlave* proclubscustomSlave::createImpl()
{
    return BLAZE_NEW_NAMED("ProclubsCustomSlaveImpl") ProclubsCustomSlaveImpl();
}


/*** Public Methods ******************************************************************************/
ProclubsCustomSlaveImpl::ProclubsCustomSlaveImpl():
	mDbId(DbScheduler::INVALID_DB_ID),
	mMaxAvatarNameResets(0)
{
}

ProclubsCustomSlaveImpl::~ProclubsCustomSlaveImpl()
{
}

void ProclubsCustomSlaveImpl::fetchSetting(uint64_t clubId, Blaze::proclubscustom::customizationSettings &customizationSettings, BlazeRpcError &outError)
{
	if (clubId != 0)
	{
		ProclubsCustomDb dbHelper = ProclubsCustomDb(this);
		DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
		outError = dbHelper.setDbConnection(dbConn);
		if (outError == Blaze::ERR_OK)
		{
			outError = dbHelper.fetchSetting(clubId, customizationSettings);
		}
	}
	else
	{
		ASSERT_LOG("[ProclubsCustomSlaveImpl].fetchSetting() invalid ClubId!");
		outError = ERR_SYSTEM;
	}
}

void ProclubsCustomSlaveImpl::fetchCustomTactics(uint64_t clubId, EA::TDF::TdfStructVector <Blaze::proclubscustom::CustomTactics> &customTactics, BlazeRpcError &outError)
{
	if (clubId != 0)
	{
		ProclubsCustomDb dbHelper = ProclubsCustomDb(this);
		DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
		outError = dbHelper.setDbConnection(dbConn);
		if (outError == Blaze::ERR_OK)
		{
			outError = dbHelper.fetchCustomTactics(clubId, customTactics);
		}
	}
	else
	{
		ASSERT_LOG("[ProclubsCustomSlaveImpl].fetchCustomTactics() invalid ClubId!");
		outError = ERR_SYSTEM;
	}
}

void ProclubsCustomSlaveImpl::fetchAIPlayerCustomization(uint64_t clubId, EA::TDF::TdfStructVector < Blaze::proclubscustom::AIPlayerCustomization> &aiPlayerCustomizationList, BlazeRpcError &outError)
{
	if (clubId != 0)
	{
		ProclubsCustomDb dbHelper = ProclubsCustomDb(this);
		DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
		outError = dbHelper.setDbConnection(dbConn);
		if (outError == Blaze::ERR_OK)
		{
			outError = dbHelper.fetchAIPlayerCustomization(clubId, aiPlayerCustomizationList);
		}
	}
	else
	{
		ASSERT_LOG("[ProclubsCustomSlaveImpl].fetchAIPlayerCustomization() invalid ClubId!");
		outError = ERR_SYSTEM;
	}
}

void ProclubsCustomSlaveImpl::updateSetting(uint64_t clubId, Blaze::proclubscustom::customizationSettings &customizationSettings, BlazeRpcError &outError)
{
	if (clubId != 0)
	{
		ProclubsCustomDb dbHelper = ProclubsCustomDb(this);
		DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
		outError = dbHelper.setDbConnection(dbConn);
		if (outError == Blaze::ERR_OK)
		{
			outError = dbHelper.updateSetting(clubId, customizationSettings);
		}
	}
	else
	{
		ASSERT_LOG("[ProclubsCustomSlaveImpl].updateSetting() invalid ClubId!");
		outError = ERR_SYSTEM;
	}
}

void ProclubsCustomSlaveImpl::updateAIPlayerCustomization(uint64_t clubId, int32_t slotId, const Blaze::proclubscustom::AIPlayerCustomization& updateAIPlayerCustomization, const Blaze::proclubscustom::AvatarCustomizationPropertyMap &modifiedItems, BlazeRpcError &outError)
{
	if (clubId != 0)
	{
		ProclubsCustomDb dbHelper = ProclubsCustomDb(this);
		DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());

		outError = dbHelper.setDbConnection(dbConn);
		if (outError == Blaze::ERR_OK)
		{
			outError = dbHelper.updateAIPlayerCustomization(clubId, slotId, updateAIPlayerCustomization, modifiedItems);
		}
	}
	else
	{
		ASSERT_LOG("[ProclubsCustomSlaveImpl].updateAIPlayerCustomization() invalid ClubId!");
		outError = ERR_SYSTEM;
	}
}

void ProclubsCustomSlaveImpl::updateCustomTactics(uint64_t clubId, uint32_t tacticsId, const Blaze::proclubscustom::CustomTactics &customTactics, BlazeRpcError &outError)
{
	if (clubId != 0)
	{
		ProclubsCustomDb dbHelper = ProclubsCustomDb(this);
		DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
		outError = dbHelper.setDbConnection(dbConn);
		if (outError == Blaze::ERR_OK)
		{
			outError = dbHelper.updateCustomTactics(clubId, tacticsId, customTactics);
		}
	}
	else
	{
		ASSERT_LOG("[ProclubsCustomSlaveImpl].updateCustomTactics() invalid ClubId!");
		outError = ERR_SYSTEM;
	}
}

void ProclubsCustomSlaveImpl::resetAiPlayerNames(uint64_t clubId, eastl::string& statusMessage, BlazeRpcError &outError)
{
	if (clubId != 0)
	{
		ProclubsCustomDb dbHelper = ProclubsCustomDb(this);
		DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
		if (dbConn != nullptr)
		{
			outError = dbHelper.setDbConnection(dbConn);
			if (outError == Blaze::ERR_OK)
			{
				outError = dbHelper.resetAiPlayerNames(clubId, statusMessage);
			}
		}
		else
		{
			ASSERT_LOG("[ProclubsCustomSlaveImpl].resetAiPlayerNames() no dbConn specified!");
			outError = ERR_SYSTEM;
		}
	}
	else
	{
		ASSERT_LOG("[ProclubsCustomSlaveImpl].resetAiPlayerNames() invalid ClubId!");
		outError = ERR_SYSTEM;
	}
}

void ProclubsCustomSlaveImpl::fetchAiPlayerNames(uint64_t clubId, eastl::string &playerNames, BlazeRpcError &outError)
{
	if (clubId != 0)
	{
		ProclubsCustomDb dbHelper = ProclubsCustomDb(this);
		DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
		if (dbConn != nullptr)
		{
			outError = dbHelper.setDbConnection(dbConn);
			if (outError == Blaze::ERR_OK)
			{
				outError = dbHelper.fetchAiPlayerNames(clubId, playerNames);
			}
		}
		else
		{
			ASSERT_LOG("[ProclubsCustomSlaveImpl].fetchAiPlayerNames() no dbConn specified!");
			outError = ERR_SYSTEM;
		}
	}
	else
	{
		ASSERT_LOG("[ProclubsCustomSlaveImpl].fetchAiPlayerNames() invalid ClubId!");
		outError = ERR_SYSTEM;
	}
}

void ProclubsCustomSlaveImpl::fetchAvatarData(int64_t userId, Blaze::proclubscustom::AvatarData& avatarData, BlazeRpcError& outError)
{
	if (userId != INVALID_BLAZE_ID)
	{
		ProclubsCustomDb dbHelper = ProclubsCustomDb(this);
		DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
		if (dbConn != nullptr)
		{
			outError = dbHelper.setDbConnection(dbConn);
			if (outError == Blaze::ERR_OK)
			{
				outError = dbHelper.fetchAvatarData(userId, avatarData, mMaxAvatarNameResets);
			}
		}
		else
		{
			ASSERT_LOG("[ProclubsCustomSlaveImpl].fetchAvatarData() no dbConn specified!");
			outError = ERR_SYSTEM;
		}
	}
	else
	{
		ASSERT_LOG("[ProclubsCustomSlaveImpl].fetchAvatarData() invalid blaze id!");
		outError = ERR_SYSTEM;
	}
}

void ProclubsCustomSlaveImpl::fetchAvatarName(int64_t userId, eastl::string &avatarFirstName, eastl::string &avatarLastName, BlazeRpcError &outError)
{
	if (userId != INVALID_BLAZE_ID)
	{
		ProclubsCustomDb dbHelper = ProclubsCustomDb(this);
		DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
		if (dbConn != nullptr)
		{
			outError = dbHelper.setDbConnection(dbConn);
			if (outError == Blaze::ERR_OK)
			{
				outError = dbHelper.fetchAvatarName(userId, avatarFirstName, avatarLastName);
			}
		}
		else
		{
			ASSERT_LOG("[ProclubsCustomSlaveImpl].fetchAvatarName() no dbConn specified!");
			outError = ERR_SYSTEM;
		}
	}
	else
	{
		ASSERT_LOG("[ProclubsCustomSlaveImpl].fetchAvatarName() invalid blaze id!");
		outError = ERR_SYSTEM;
	}
}

void ProclubsCustomSlaveImpl::updateAvatarName(int64_t userId, eastl::string avatarFirstName, eastl::string avatarLastName, BlazeRpcError &outError)
{
	if (userId != INVALID_BLAZE_ID)
	{
		ProclubsCustomDb dbHelper = ProclubsCustomDb(this);
		DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
		if (dbConn != nullptr)
		{
			outError = dbHelper.setDbConnection(dbConn);
			if (outError == Blaze::ERR_OK)
			{
				outError = dbHelper.insertAvatarName(userId, avatarFirstName, avatarLastName);
			}
		}
		else
		{
			ASSERT_LOG("[ProclubsCustomSlaveImpl].updateAvatarName() no dbConn specified!");
			outError = ERR_SYSTEM;
		}
	}
	else
	{
		ASSERT_LOG("[ProclubsCustomSlaveImpl].updateAvatarName() invalid blaze id!");
		outError = ERR_SYSTEM;
	}
}

void ProclubsCustomSlaveImpl::flagAvatarNameAsProfane(int64_t userId, BlazeRpcError &outError)
{
	if (userId != INVALID_BLAZE_ID)
	{
		ProclubsCustomDb dbHelper = ProclubsCustomDb(this);
		DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
		if (dbConn != nullptr)
		{
			outError = dbHelper.setDbConnection(dbConn);
			if (outError == Blaze::ERR_OK)
			{
				outError = dbHelper.flagProfaneAvatarName(userId);
			}
		}
		else
		{
			ASSERT_LOG("[ProclubsCustomSlaveImpl].flagAvatarNameAsProfane() no dbConn specified!");
			outError = ERR_SYSTEM;
		}
	}
	else
	{
		ASSERT_LOG("[ProclubsCustomSlaveImpl].flagAvatarNameAsProfane() invalid blaze id!");
		outError = ERR_SYSTEM;
	}
}

bool ProclubsCustomSlaveImpl::onConfigure()
{	
	bool success = configureHelper();	
	TRACE_LOG("[ProClubsCustomSlaveImpl:" << this << "].onConfigure: configuration " << (success ? "successful." : "failed."));
	return true;
}

bool ProclubsCustomSlaveImpl::onReconfigure()
{
	return onConfigure();
}

void ProclubsCustomSlaveImpl::onShutdown()
{
	TRACE_LOG("[ProclubsCustomSlaveImpl].onShutdown");
}

bool ProclubsCustomSlaveImpl::configureHelper()
{
	TRACE_LOG("[ProclubsCustomSlaveImpl].configureHelper");

	const proclubsConfiguration &config = getConfig();
	mMaxAvatarNameResets = config.getMaxAvatarNameResets();
	
	const char8_t* dbName = config.getDbName();
	mDbId = gDbScheduler->getDbId(dbName);
	if (mDbId == DbScheduler::INVALID_DB_ID)
	{
		ERR_LOG("[ProclubsCustomSlaveImpl].configureHelper(): Failed to initialize db");
		return false;
	}

	ProclubsCustomDb::initialize(mDbId);
	
	return true;
}

void ProclubsCustomSlaveImpl::onVoidMasterNotification(UserSession *)
{
    TRACE_LOG("[ProclubsCustomSlaveImpl:" << this << "].onVoidMasterNotification");
}

} // ProClubsCustom
} // Blaze
