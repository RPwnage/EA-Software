//  *************************************************************************************************
//
//   File:    gameplayuser.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef PT_OPTIN
#define PT_OPTIN 1 
#endif

#ifndef PT_OPTOUT
#define PT_OPTOUT 0 
#endif

#include "player.h"
#include "stats/tdf/stats.h"
#include "userpersonalizedservices.h"
#include "pinproxy.h"
#include "reference.h"
#include "gameplayuser.h"

namespace Blaze {
namespace Stress {
using namespace Blaze::Util;
using namespace Blaze::Stats;
using namespace Blaze::Association;

GamePlayUser::GamePlayUser(StressPlayerInfo* PlayerData)
	: Player(PlayerData, CLIENT_TYPE_GAMEPLAY_USER)
{
	mStatsProxyHandler = BLAZE_NEW StatsProxyHandler(PlayerData);
}

GamePlayUser::~GamePlayUser()
{
	if (mStatsProxyHandler != NULL)
	{
		delete mStatsProxyHandler;
	}
}

void GamePlayUser::onAsync(uint16_t component, uint16_t type, RawBuffer* payload)
{
	//if (getPlayerType() == SINGLEPLAYER || getPlayerType() == KICKOFFPLAYER)
	{
		//  This player type does not have a session object(like MPgamesession or coopgamesession so call the players default asyncHandler)
		Player::onAsync(component, type, payload);
	}
}

BlazeRpcError GamePlayUser::simulateMenuFlow()
{
	BlazeRpcError	err = ERR_OK;
	bool result = true;
	eastl::string entityIDName = "";
	mPlayerInfo->setViewId(0);

	if (!mPlayerInfo->getOwner()->isStressLogin())
	{
		// Making Aruba and Reco to be hit 60%
		if ((int)Random::getRandomNumber(100) < 100)
		{
			// ARUBA Calls
			if (StressConfigInst.getArubaEnabled())
			{
				/*Blaze::ExternalId nucleusId = mPlayerInfo->getExternalId();
				BLAZE_ERR_LOG(Log::SYSTEM, "Heat2Decoder "<< nucleusId);*/
				mPlayerInfo->getArubaProxyHandler()->simulateArubaLoad();
			}

			// Reco Calls
			if (StressConfigInst.getRecoEnabled())
			{
				mPlayerInfo->getArubaProxyHandler()->simulateRecoLoad();
			}
		}
		// UPS Calls	
		if (StressConfigInst.getUPSEnabled())
		{
			mPlayerInfo->getUPSProxyHandler()->simulateUPSLoad();
		}
		// Let the PIN be hit 100%
		if (StressConfigInst.getPINEnabled())
		{
			mPlayerInfo->getPINProxyHandler()->simulatePINLoad();
		}
	}

	if (StressConfigInst.getPTSEnabled())
	{
		entityIDName = eastl::string().sprintf("""%" PRIu64 "", mPlayerInfo->getPlayerId()).c_str();
		result = getStatsProxyHandler()->trackPlayerHealth(entityIDName, "");

		if (!result)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][GamePlayeUser::playyerHealthTracker]: Failed to get optIn");
		}
		else
		{
			if (((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->optIn == PT_OPTIN)
			{
				if ((uint32_t)Random::getRandomNumber(100) < StressConfigInst.getOptInProbability())
				{
					initializeUserActivityTrackerRequest(mPlayerInfo);
				}
			}
		}
	}

	sleep(15000);

	//  check if the player has already loaded frontend once. if so, call only the RPCs which are called when player
	//   returns to frontend after playing a game
	if (!(isNewPlayerSession()))
	{
		//   ping site is updated for each round so need to call updateNetworkInfo
		//  need to update this with new log
		updateNetworkInfo(mPlayerInfo, Util::NatType::NAT_TYPE_NONE, 0x1);
		//updateNetworkInfo(mPlayerInfo, Util::NatType::NAT_TYPE_STRICT, 0x00000004);
		return ERR_OK;
	}

	/*if (getPlayerType() == ONLINESEASONS)
	{
		updateNetworkInfo(mPlayerInfo, Util::NatType::NAT_TYPE_MODERATE, 0x00000004);
	}
	else*/
	//{
		//if(getPlayerType()==SSFPLAYER&& StressConfigInst.getPlatform() == Platform::PLATFORM_XONE)
		//	updateNetworkInfo(mPlayerInfo, Util::NatType::NAT_TYPE_MODERATE, 0x00000004);
		//else
		updateNetworkInfo(mPlayerInfo, Util::NatType::NAT_TYPE_STRICT, 0x00000004);
	//}
	Blaze::PersonaNameList personaNameList;
	//personaNameList.push_back("2 Dev 416610777");   // 82805766		As per logs
	personaNameList.push_back("");
	updateHardwareFlags(mPlayerInfo);

	if (!mPlayerInfo->getOwner()->isStressLogin())
	{
		getAccount(mPlayerInfo);
	}
	setClientState(mPlayerInfo, Blaze::ClientState::MODE_MENU, Blaze::ClientState::STATUS_NORMAL);
	fetchMessages(mPlayerInfo, 4, 1634497140);
	fetchMessages(mPlayerInfo, 4, 1668052322);
	err = subscribeToCensusDataUpdates(mPlayerInfo);
	if (err == ERR_OK)
	{
		SetSubscriptionSatatus();
	}
	fetchMessages(mPlayerInfo, 4, 1734438245);
	//fetchMessages(mPlayerInfo, 4, 1919905645);
	sleep(1000);
	associationGetLists(mPlayerInfo);
	getClubsComponentSettings(mPlayerInfo);
	FetchConfigResponse response;
	fetchClientConfig(mPlayerInfo, "OSDK_TICKER", response);
	/*if (getPlayerType() == ONLINESEASONS)
	{
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	}*/

	// Need to put in clubs RPC
	//if (getMyClubId() > 0)
	//{
	//	getClubs(mPlayerInfo, objGameSession->getMyClubId()); //Hardcoded for now need to get clubid;
	//}
	fetchSettings(mPlayerInfo);
	getKeyScopesMap(mPlayerInfo);
	sleep(1000);

	// Read the clubID from RPC response getClubMembershipForUsers
	Blaze::Clubs::GetClubMembershipForUsersResponse clubMembershipForUsersResponse;
	
	if (getPlayerType() != CLUBSPLAYER)
	{
		err = getClubMembershipForUsers(mPlayerInfo, clubMembershipForUsersResponse);
	}
	if ((StressConfigInst.getPlatform() == Platform::PLATFORM_PS4) && (getPlayerType() == ESLTOURNAMENTS))
	{
		setUsersToList(mPlayerInfo);
	}
	
	fetchSettingsGroups(mPlayerInfo);
	getStatGroupList(mPlayerInfo);
	userSettingsLoadAll(mPlayerInfo);

	if (!mPlayerInfo->getOwner()->isStressLogin())
	{
		getAccount(mPlayerInfo);
	}
	resetUserGeoIPData(mPlayerInfo);
	if (getPlayerType() == SSFPLAYER || getPlayerType() == CLUBSPLAYER)	// Not called in OS, Friendlies, Club Frienlies/League, FUT
	{
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	}

	getPeriodIds(mPlayerInfo);
	getMemberHash(mPlayerInfo, "OSDKPlatformFriendList");
	setConnectionState(mPlayerInfo);
	//if (getMyClubId() > 0)
	//{
	//		getInvitations(mPlayerInfo, 0, CLUBS_SENT_TO_ME);
	//}

	userSettingsLoad(mPlayerInfo, "AchievementCache");
	setClientState(mPlayerInfo, Blaze::ClientState::MODE_SINGLE_PLAYER, Blaze::ClientState::STATUS_NORMAL);
	lookupUserGeoIPData(mPlayerInfo);
	
	if (getPlayerType() == CLUBSPLAYER)
	{
		setStatus(mPlayerInfo, 10);
	}

	if (!mPlayerInfo->getOwner()->isStressLogin())
	{
		listEntitlements(mPlayerInfo);
	}
	sleep(1000);
	fetchMessages(mPlayerInfo, 0, 0);

	if (getPlayerType() != SSFPLAYER && getPlayerType() != LIVECOMPETITIONS)
	{
		getPeriodIds(mPlayerInfo);
	}
	getEventsURL(mPlayerInfo);
	
	if (getPlayerType() != ONLINESEASONS && getPlayerType() != SINGLEPLAYER)
	{
		setUsersToList(mPlayerInfo);
	}
	if (getPlayerType() != ONLINESEASONS && getPlayerType() != FRIENDLIES && getPlayerType() != FUTPLAYER && getPlayerType() != SINGLEPLAYER && getPlayerType() != LIVECOMPETITIONS && getPlayerType() != CLUBSPLAYER)	// Not called in OS, Friendlies
	{
		personaNameList.clear();
		personaNameList.push_back("q36448b89fd-GBen");
		personaNameList.push_back("qdf3d34c996-USen");
		personaNameList.push_back("addafifa");
		personaNameList.push_back("walstab2021");
		personaNameList.push_back("Lorry_ONL");
		personaNameList.push_back("Cata_G");
		personaNameList.push_back("FIFA20Final_28");
		personaNameList.push_back("fifa21online1");
		personaNameList.push_back("FIFA20final_153");
		personaNameList.push_back("fifa212");
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	}
	
	initializeUserActivityTrackerRequest(mPlayerInfo, false);
	getPeriodIds(mPlayerInfo);
	lookupUsersByPersonaNames(mPlayerInfo, "");

	if (getPlayerType() == ONLINESEASONS)
	{
		lookupUsersByPersonaNames(mPlayerInfo, "");
	}
	
	if (getPlayerType() == ONLINESEASONS || getPlayerType() == ESLTOURNAMENTS || getPlayerType() == LIVECOMPETITIONS || getPlayerType() == SSFSEASONS)
	{
		personaNameList.clear();
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	}

	if (getPlayerType() == KICKOFFPLAYER)
	{
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		setUsersToList(mPlayerInfo);
	}

	if (getPlayerType() == SSFPLAYER)
	{
		personaNameList.clear();
		setConnectionState(mPlayerInfo);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		setClientState(mPlayerInfo, Blaze::ClientState::Mode::MODE_SINGLE_PLAYER, Blaze::ClientState::Status::STATUS_SUSPENDED);
		unsubscribeFromCensusData(mPlayerInfo);
	}

	if (getPlayerType() == DROPINPLAYER)
	{
		setConnectionState(mPlayerInfo);
		setConnectionState(mPlayerInfo);
		lookupUsersByPersonaNames(mPlayerInfo, "");
	}
	if (getPlayerType() == COOPPLAYER)
	{
		personaNameList.clear();
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	}
	if (getPlayerType() == CLUBSPLAYER)
	{
		personaNameList.clear();
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	}
    return err;
}

}  // namespace Stress
}  // namespace Blaze
