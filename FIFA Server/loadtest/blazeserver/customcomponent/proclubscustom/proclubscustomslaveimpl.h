/*************************************************************************************************/
/*!
    \file   ProclubsCustomSlaveImpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_proclubscustom_SLAVEIMPL_H
#define BLAZE_proclubscustom_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "proclubscustom/rpc/proclubscustomslave_stub.h"
#include "proclubscustom/rpc/proclubscustommaster.h"
#include "framework/replication/replicationcallback.h"
#include "proclubscustom/tdf/proclubscustomtypes.h"
#include "framework/usersessions/usersessionmanager.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace proclubscustom
{

class ProclubsCustomSlaveImpl : 
	public proclubscustomSlaveStub
	
{
public:
    ProclubsCustomSlaveImpl();
    ~ProclubsCustomSlaveImpl();

	// Component Interface
	uint16_t getDbSchemaVersion() const override { return 8; }
	const char8_t* getDbName() const override { return "main";}

	void fetchSetting(uint64_t clubId, Blaze::proclubscustom::customizationSettings &customizationSettings, BlazeRpcError &outError);
	void fetchCustomTactics(uint64_t clubId, EA::TDF::TdfStructVector <Blaze::proclubscustom::CustomTactics> &customTactics, BlazeRpcError &outError);
	void fetchAIPlayerCustomization(uint64_t clubId, EA::TDF::TdfStructVector < Blaze::proclubscustom::AIPlayerCustomization> &aiPlayerCustomization, BlazeRpcError &outError);
	
	void updateSetting(uint64_t clubId,Blaze::proclubscustom::customizationSettings &customizationSettings, BlazeRpcError &outError);
	void updateAIPlayerCustomization(uint64_t clubId, int32_t slotId, const Blaze::proclubscustom::AIPlayerCustomization&, const Blaze::proclubscustom::AvatarCustomizationPropertyMap &modifiedItems, BlazeRpcError &outError);
	void updateCustomTactics(uint64_t clubId, uint32_t tactics, const Blaze::proclubscustom::CustomTactics &customTactics, BlazeRpcError &outError);

	void resetAiPlayerNames(uint64_t clubId, eastl::string& statusMessage, BlazeRpcError &outError);
	void fetchAiPlayerNames(uint64_t clubId, eastl::string &playerNames, BlazeRpcError &outError);

	void fetchAvatarData(int64_t userId, Blaze::proclubscustom::AvatarData &avatarData, BlazeRpcError &outError);
	void fetchAvatarName(int64_t userId, eastl::string &avatarFirstName, eastl::string &avatarLastName, BlazeRpcError &outError);
	void updateAvatarName(int64_t userId, eastl::string avatarFirstName, eastl::string avatarLastName, BlazeRpcError &outError);
	void flagAvatarNameAsProfane(int64_t userId, BlazeRpcError &outError);

private:
	//Reconfigure callbacks
	bool onReconfigure() override;

	bool onConfigure() override;
	void onShutdown() override;

	//! Called to perform tasks common to onConfigure and onReconfigure
	bool configureHelper();

    //VProGrowthUnlocksMasterListener interface
    virtual void onVoidMasterNotification(UserSession *);

	uint32_t mDbId;

	uint32_t mMaxAvatarNameResets;
};

} // ProClubsCustom
} // Blaze

#endif // BLAZE_ProclubsCustom_SLAVEIMPL_H

