//  *************************************************************************************************
//
//   File:    utility.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************
#include "utility.h"
#include "framework/util/random.h"
#include "clubs/tdf/clubs.h"
//#include "gamemanager/stress/gamemanagermodule.h"
#include "censusdata/tdf/censusdata.h"
#include <sstream>
#define _USE_MATH_DEFINES
#include "math.h"
#include "vprogrowthunlocks/tdf/vprogrowthunlockstypes.h"
#include "proxycomponent/xblsystemconfigs/tdf/xblsystemconfigs.h"
#include "easfc/tdf/easfctypes.h"
#include "fifagroups/tdf/fifagroupstypes.h"

namespace Blaze {
namespace Stress {
	bool playerHasSubscribed = false;
//  UserSessions
BlazeRpcError updateNetworkInfo(StressPlayerInfo* playerInfo, Util::NatType natType, uint32_t updateNetowkOption)
{
	UpdateNetworkInfoRequest info;
	Util::NetworkQosData &qosData = info.getNetworkInfo().getQosData();
	if ((playerInfo->getPingSiteLatencyMap().empty()))
	{
		playerInfo->getPingSiteLatencyMap()["aws-cdg"] = Blaze::Random::getRandomNumber(500);
		playerInfo->getPingSiteLatencyMap()["aws-fra"] = Blaze::Random::getRandomNumber(500);
		playerInfo->getPingSiteLatencyMap()["aws-nrt"] = Blaze::Random::getRandomNumber(500);
		playerInfo->getPingSiteLatencyMap()["aws-pdx"] = Blaze::Random::getRandomNumber(500);
		
		/*playerInfo->getPingSiteLatencyMap()["bio-dub"] = Blaze::Random::getRandomNumber(500);
		playerInfo->getPingSiteLatencyMap()["bio-iad"] = Blaze::Random::getRandomNumber(500);
		playerInfo->getPingSiteLatencyMap()["bio-sjc"] = Blaze::Random::getRandomNumber(500);
		playerInfo->getPingSiteLatencyMap()["bio-syd"] = Blaze::Random::getRandomNumber(500);
		playerInfo->getPingSiteLatencyMap()["m3d-brz"] = Blaze::Random::getRandomNumber(500);
		playerInfo->getPingSiteLatencyMap()["m3d-nrt"] = Blaze::Random::getRandomNumber(500);
*/
		/*playerInfo->getPingSiteLatencyMap()["aws-fra"] = Blaze::Random::getRandomNumber(500);
		playerInfo->getPingSiteLatencyMap()["ea-ams"]  = Blaze::Random::getRandomNumber(500);
		playerInfo->getPingSiteLatencyMap()["i3d-dxb"] = Blaze::Random::getRandomNumber(500);
		playerInfo->getPingSiteLatencyMap()["i3d-hkg"] = Blaze::Random::getRandomNumber(500);
		playerInfo->getPingSiteLatencyMap()["i3d-jnb"] = Blaze::Random::getRandomNumber(500);
		playerInfo->getPingSiteLatencyMap()["mpl-maa"] = Blaze::Random::getRandomNumber(500);
		playerInfo->getPingSiteLatencyMap()["mpl-mex"] = Blaze::Random::getRandomNumber(500);
		playerInfo->getPingSiteLatencyMap()["mpl-sin"] = Blaze::Random::getRandomNumber(500);*/

		playerInfo->getPingSiteLatencyMap()[playerInfo->getPlayerPingSite()] = 6;
	}
	playerInfo->getPingSiteLatencyMap().copyInto(info.getNetworkInfo().getPingSiteLatencyByAliasMap());
	//  TODO: verify below parameters with logs as they are not fixed in current logs
	qosData.setDownstreamBitsPerSecond(0);
	qosData.setNatType(natType);
	qosData.setUpstreamBitsPerSecond(0);
	qosData.setNatErrorCode(0);
	qosData.setBandwidthErrorCode(0);
	info.getOpts().setBits(updateNetowkOption);
	BlazeRpcError err = playerInfo->getComponentProxy()->mUserSessionsProxy->updateNetworkInfo(info);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "updateNetworkInfo: playerId " << playerInfo->getPlayerId() << " failed with error = " << ErrorHelp::getErrorName(err) << ")");
	}
	return err;
}

BlazeRpcError getTelemetryServer(StressPlayerInfo* playerInfo)
{
	//  Util::getTelemetryServer
	Blaze::Util::GetTelemetryServerRequest getTelemetryServerReq;
	Blaze::Util::GetTelemetryServerResponse getTelemetryServerResp;
	getTelemetryServerReq.setServiceName("");
	BlazeRpcError err = playerInfo->getComponentProxy()->mUtilProxy->getTelemetryServer(getTelemetryServerReq, getTelemetryServerResp);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "Util::getTelemetryServer: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError updateHardwareFlags(StressPlayerInfo* playerInfo)
{
	//  updateHardwareFlags
	UpdateHardwareFlagsRequest updateHardwareFlagsreq;
	updateHardwareFlagsreq.getHardwareFlags().setBits(0);
	BlazeRpcError err = playerInfo->getComponentProxy()->mUserSessionsProxy->updateHardwareFlags(updateHardwareFlagsreq);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "updateHardwareFlags: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError fetchClientConfig(StressPlayerInfo* playerInfo, const char8_t* configSection, FetchConfigResponse& response)
{
	//  fetchClientConfig
	FetchClientConfigRequest request;
	request.setConfigSection(configSection);
	BlazeRpcError err = playerInfo->getComponentProxy()->mUtilProxy->fetchClientConfig(request, response);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "fetchClientConfig: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError localizeStrings(StressPlayerInfo* playerInfo, uint32_t locale, StringList& playList)
{
	BlazeRpcError err = ERR_OK;
	Util::LocalizeStringsRequest  localStringReq;
	Util::LocalizeStringsResponse localStringResp;
	localStringReq.setLocale(locale);
	StringList::iterator it = playList.begin();
	for (; it != playList.end(); it++)
	{
		localStringReq.getStringIds().push_back(it->c_str());
	}
	err = playerInfo->getComponentProxy()->mUtilProxy->localizeStrings(localStringReq, localStringResp);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "localizeStrings: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError listUserEntitlements2(StressPlayerInfo* playerInfo, BlazeId playerId)
{
	BlazeRpcError err = ERR_OK;
	Blaze::Authentication::Entitlements     listUserEntitlements2Resp;
	err = listUserEntitlements2(playerInfo, playerId, listUserEntitlements2Resp);
	return err;
}

BlazeRpcError listUserEntitlements2(StressPlayerInfo* playerInfo, BlazeId playerId, Blaze::Authentication::Entitlements& listUserEntitlements2Resp)
{
	BlazeRpcError err = ERR_OK;
	if (playerInfo != NULL && !playerInfo->getOwner()->isStressLogin())
	{
		const GroupNameList &groupNameList = StressConfigInst.getEntGroupNameList();
		Blaze::Authentication::ListUserEntitlements2Request listUserEntitlements2Req;
		listUserEntitlements2Req.setBlazeId(playerId);
		listUserEntitlements2Req.setProductId("");
		listUserEntitlements2Req.setProjectId((StressConfigManager::getInstance().getProjectId()));
		listUserEntitlements2Req.setHasAuthorizedPersona(false);
		listUserEntitlements2Req.setStartGrantDate("");
		listUserEntitlements2Req.setEndGrantDate("");
		listUserEntitlements2Req.setStartTerminationDate("");
		listUserEntitlements2Req.setEndTerminationDate("");
		listUserEntitlements2Req.setPageNo(1);
		listUserEntitlements2Req.setPageSize(StressConfigInst.getNumEntitlementsToShow());
		listUserEntitlements2Req.setRecursiveSearch(false);
		listUserEntitlements2Req.setStatus(Blaze::Nucleus::EntitlementStatus::UNKNOWN);
		//  not reading the EntitlementTag from config as its NULL in listUserEntitlements2 req in logs while its set in grantEntitlement2 req. so grantEntitlement2 req is reading EntitlementTag from config
		listUserEntitlements2Req.setEntitlementTag("");
		groupNameList.copyInto(listUserEntitlements2Req.getGroupNameList());
		err = playerInfo->getComponentProxy()->mAuthenticationProxy->listUserEntitlements2(listUserEntitlements2Req, listUserEntitlements2Resp);
		if (ERR_OK != err)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::authentication, "listUserEntitlements2: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
		}
	}
	return err;
}
BlazeRpcError   validateString(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::XBLSystem::ValidateStringsRequest request;
	Blaze::XBLSystem::ValidateStringsResponse response;
	err = playerInfo->getComponentProxy()->mXBLSystemConfigsProxy->validateStrings(request, response);
	return err;
}


BlazeRpcError   lookupUsersIdentification(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_OK;
	LookupUsersRequest request;
	LookupUsersResponse response;
	request.setLookupType(LookupUsersRequest::LookupType::PERSONA_NAME);
	//UserIdentification* uidv = BLAZE_NEW UserIdentification();
	//uidv->setName(StressConfigInst.getRandomPersonaNames());
	UserIdentification* uid = request.getUserIdentificationList().pull_back();
	uid->setAccountId(0);
	PlatformInfo &platformInfo = uid->getPlatformInfo();
	platformInfo.getEaIds().setOriginPersonaName("");
	platformInfo.getEaIds().setNucleusAccountId(0);
	platformInfo.getEaIds().setOriginPersonaId(0);
	platformInfo.getExternalIds().setPsnAccountId(0);
	platformInfo.getExternalIds().setXblAccountId(0);
	uid->setAccountLocale(0);
	uid->setAccountCountry(0);
	uid->setExternalId(0);
	uid->setName("fifa_onl021");
	uid->setPersonaNamespace("");
	uid->setOriginPersonaId(0);
	uid->setPidId(0);
	uid->setBlazeId(playerInfo->getPlayerId());
	uid->getPlatformInfo().setClientPlatform(Blaze::ClientPlatformType::INVALID);
	request.getUserIdentificationList().push_back(uid);
	err = playerInfo->getComponentProxy()->mUserSessionsProxy->lookupUsersIdentification(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "lookupUsersIdentification:playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}


BlazeRpcError   grantEntitlement2(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_OK;
	if (!playerInfo->getOwner()->isStressLogin())
	{
		Blaze::Authentication::GrantEntitlement2Request grantEntitlement2Req;
		Blaze::Authentication::GrantEntitlement2Response grantEntitlement2Resp;
		grantEntitlement2Req.setBlazeId(playerInfo->getPlayerId());
		grantEntitlement2Req.setProjectId((StressConfigManager::getInstance().getProjectId()));
		grantEntitlement2Req.setProductId("");
		grantEntitlement2Req.setWithPersona(0);
		grantEntitlement2Req.setTermination("");
		grantEntitlement2Req.setUseCount("");
		grantEntitlement2Req.setIsSearch(0);
		grantEntitlement2Req.setManagedLifecycle(0);
		grantEntitlement2Req.setDeviceId("");
		grantEntitlement2Req.setGrantDate("");
		grantEntitlement2Req.setEntitlementTag(StressConfigInst.getEntitlementTag());
		grantEntitlement2Req.setGroupName(StressConfigInst.getEntGroupName());
		err = playerInfo->getComponentProxy()->mAuthenticationProxy->grantEntitlement2(grantEntitlement2Req, grantEntitlement2Resp);
		if (ERR_OK != err)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::authentication, " grantEntitlement2: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
		}
	}
	return err;
}

BlazeRpcError removeMember(StressPlayerInfo * playerInfo, ClubId clubId)
{
	RemoveMemberRequest req;
	BlazeRpcError err;
	req.setBlazeId(playerInfo->getPlayerId());
	req.setClubId(clubId);
	err = playerInfo->getComponentProxy()->mClubsProxy->removeMember(req);
	if (err == ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::clubs, " removeMember: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError setConnectionState(StressPlayerInfo * playerInfo)
{
	SetConnectionStateRequest req;
	req.setIsActive(true);
	BlazeRpcError err = playerInfo->getComponentProxy()->mUtilProxy->setConnectionState(req);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "setConnectionState " << playerInfo->getPlayerId() << " failed with error " << (Blaze::ErrorHelp::getErrorName(err)));
	}
	return err;
}

BlazeRpcError setClientState(StressPlayerInfo* playerInfo, ClientState::Mode mode, ClientState::Status status)
{
	Blaze::ClientState clientState;
	clientState.setStatus(status);
	clientState.setMode(mode);
	BlazeRpcError err = playerInfo->getComponentProxy()->mUtilProxy->setClientState(clientState);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "setClientState: playerId " << playerInfo->getPlayerId() << " failed with error " << (Blaze::ErrorHelp::getErrorName(err)));
	}
	return err;
}
BlazeRpcError setVProStats(StressPlayerInfo* playerInfo)
{
	EASFC::SetVProStatsRequest setVProStatsRequest;
	setVProStatsRequest.setMemberId(playerInfo->getPlayerId());
	EASFC::StatNameValueMapInt &statIntmap = setVProStatsRequest.getStatIntValues();
	statIntmap["prevYearPct"] = 0;
	EASFC::StatNameValueMapStr &statmap = setVProStatsRequest.getStatValues();
	statmap["accomplishments"] = "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";
	statmap["consecutiveInGameStats"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";
	statmap["defRoleStats"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";
	statmap["fwdRoleStats"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";
	statmap["gkRoleStats"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";
	statmap["midRoleStats"] = "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";

	setVProStatsRequest.setMemberType(EASFC::EASFC_MEMBERTYPE_CLUB);
	char8_t buf[10240];
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "-> [Easfc::SetSeasonal][" << "\n" << setVProStatsRequest.print(buf, sizeof(buf)));
	BlazeRpcError err = playerInfo->getComponentProxy()->mEASFCProxy->setVProStats(setVProStatsRequest);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "setVProStats:  " << playerInfo->getPlayerId() << " failed with error " << (Blaze::ErrorHelp::getErrorName(err)));
	}
	return err;
}
BlazeRpcError setSeasonsData(StressPlayerInfo* playerInfo)
{
	EASFC::SetSeasonalPlayStatsRequest playStatsRequest;
	EASFC::SetNormalGameStats  *setNormalgameStats = NULL;
	setNormalgameStats = BLAZE_NEW Blaze::EASFC::SetNormalGameStats;

	playStatsRequest.setMemberType(EASFC::EASFC_MEMBERTYPE_USER);
	playStatsRequest.setMemberId(playerInfo->getPlayerId());
	//EASFC::NormalGameStatsList &normalStatList = playStatsRequest.getNormalGameStats();
	//EASFC::StatNameValueMapInt &normalGstats = setNormalgameStats->getNormalGameStats();
	//EASFC::StatNameValueMapInt &normalKscopes = setNormalgameStats->getNormalKeyScopes();

/*	normalKscopes["accountcountry"] = 17747;
	normalKscopes["controls"] = 2;

	normalGstats["beststreak"] = 0 ;
	normalGstats["corners"] = 8 ;
	normalGstats["currCompPts"] = 176 ;
	normalGstats["currPerfPts"] = 81;
	normalGstats["eqTCompPts"] = 176 ;
	normalGstats["eqTPerfPts"] = 81;
	normalGstats["fouls"] = 19;
	normalGstats["goalsAgainst"] = 12;
	normalGstats["losses"] =11 ;
	normalGstats["lstreak"] = 0;
	normalGstats["offsides"] = 2;
	normalGstats["opponentLevel"] = 33;
	normalGstats["ownGoals"] = 3;
	normalGstats["passAttempts"] = 710 ;
	normalGstats["passesMade"] = 541;
	normalGstats["possession"] = 42 ;
	normalGstats["prevCompPts"] = 136 ;
	normalGstats["prevPerfPts"] =64 ;
	normalGstats["ptsGainedThisLevel"] = 57;
	normalGstats["ptsTillNextLevel"] = 43 ;
	normalGstats["redCards"] = 0 ;
	normalGstats["shotsAgainst"] = 24 ;
	normalGstats["shotsAgainstOnGoal"] = 133 ;
	normalGstats["shotsFor"] = 893;
	normalGstats["shotsForOnGoal"] = 45 ;
	normalGstats["skill"] = 1;
	normalGstats["skillPrev"] =1 ;
	normalGstats["starLevel"] = 0 ;
	normalGstats["tacklesAttempted"] = 79 ;
	normalGstats["tacklesMade"] = 67;
	normalGstats["ties"] = 0;
	normalGstats["totalGamesNotFinished"] = 0;
	normalGstats["totalGamesPlayed"] = 15;
	normalGstats["tournamentLevel"] = 0;
	normalGstats["wins"] = 4;
	normalGstats["winsCup"] = 0;
	normalGstats["wstreak"] = 0;
	normalGstats["yellowCards"] = 12;

	*/

	//EASFC::StatNameValueMapInt &normalKeyScopes = 
	EASFC::StatNameValueMapInt &divValues = playStatsRequest.getSPDivValues();
	divValues["cupLosses"] = 0;
	divValues["cupRound"] = 0;
	divValues["cupWins"] = 0;
	divValues["curDivision"] = 0;
	divValues["extraMatches"] = 0;
	divValues["gamesPlayed"] = 0;
	divValues["goals"] = 0;
	divValues["goalsAgainst"] = 0;
	divValues["points"] = 0;
	divValues["prevProjectedPts"] = 0;
	divValues["prevSeasonLosses"] = 0;
	divValues["prevSeasonTies"] = 0;
	divValues["prevSeasonWins"] = 0;
	divValues["projectedPts"] = 0;
	divValues["rankingPoints"] = 0;
	divValues["seasonLosses"] = 0;
	divValues["seasonTies"] = 0;
	divValues["seasonWins"] = 0;
	divValues["seasons"] = 0;

	EASFC::StatNameValueMapInt &poValues = playStatsRequest.getSPOValues();
	poValues["alltimeGoals"] = 0;
	poValues["alltimeGoalsAgainst"] = 0;
	poValues["bestDivision"] = 0;
	poValues["bestPoints"] = 0;
	poValues["cupsElim1"] = 0;
	poValues["cupsElim2"] = 0;
	poValues["cupsElim3"] = 0;
	poValues["cupsElim4"] = 0;
	poValues["cupsElim5"] = 0;
	poValues["cupsEntered"] = 0;
	poValues["cupsWon1"] = 0;
	poValues["cupsWon2"] = 0;
	poValues["cupsWon3"] = 0;
	poValues["cupsWon4"] = 0;
	poValues["cupsWon5"] = 0;
	poValues["curDivision"] = 1;//added as per logs
	poValues["curSeasonMov"] = -2;
	poValues["divsWon1"] = 0;
	poValues["divsWon2"] = 0;
	poValues["divsWon3"] = 0;
	poValues["divsWon4"] = 0;
	poValues["holds"] = 0;
	poValues["leaguesWon"] = 0;
	poValues["maxDivision"] = 0;
	poValues["prevDivision"] = 0;
	poValues["promotions"] = 0;
	poValues["rankingPoints"] = 0;
	poValues["relegations"] = 0;
	poValues["seasons"] = 0;

	//normalStatList.push_back(setNormalgameStats);

	//char8_t buf[10240];
	//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "-> [Easfc::SetSeasonal][" << playerInfo->getPlayerId() << "]\n" << playStatsRequest.print(buf, sizeof(buf)));
	BlazeRpcError err = playerInfo->getComponentProxy()->mEASFCProxy->setSeasonsData(playStatsRequest);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "Easfc::SetSeasonal playerId " << playerInfo->getPlayerId() << " failed with error " << (Blaze::ErrorHelp::getErrorName(err)));
	}
	return err;
}


BlazeRpcError setUserOptions(StressPlayerInfo* playerInfo, Util::TelemetryOpt optPar)
{
	Blaze::Util::UserOptions userOptions;
	userOptions.setBlazeId(playerInfo->getPlayerId());
	userOptions.setTelemetryOpt(optPar);
	BlazeRpcError err = playerInfo->getComponentProxy()->mUtilProxy->setUserOptions(userOptions);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "setUserOptions: playerId " << playerInfo->getPlayerId() << " failed with error = " << (Blaze::ErrorHelp::getErrorName(err)));
	}
	return err;
}

BlazeRpcError userSettingsLoadAll(StressPlayerInfo* playerInfo)
{
	Blaze::Util::UserSettingsLoadAllResponse userSettingsResp;
	BlazeRpcError err = playerInfo->getComponentProxy()->mUtilProxy->userSettingsLoadAll(userSettingsResp);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "userSettingsLoadAll: playerId " << playerInfo->getPlayerId() << " failed with error = " << (Blaze::ErrorHelp::getErrorName(err)));
	}
	return err;
}

BlazeRpcError getAuthToken(StressPlayerInfo* playerInfo)
{
	//Authentication::GetAuthTokenResponse resp;
	BlazeRpcError err = ERR_OK; //playerInfo->getComponentProxy()->mAuthenticationProxy->getAuthToken(resp);
	/*if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::authentication, "getAuthToken: playerId " << playerInfo->getPlayerId() << " failed with error = " << ErrorHelp::getErrorName(err));
	}*/
	return err;
}

BlazeRpcError userSettingsLoad(StressPlayerInfo* playerInfo, const char8_t* key)
{
	Blaze::Util::UserSettingsLoadRequest userSettingsLoadReq;
	Blaze::Util::UserSettingsResponse userSettingsLoadResp;
	userSettingsLoadReq.setKey(key);
	userSettingsLoadReq.setBlazeId(0);
	BlazeRpcError err = playerInfo->getComponentProxy()->mUtilProxy->userSettingsLoad(userSettingsLoadReq, userSettingsLoadResp);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "userSettingsLoad: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError setUserInfoAttribute(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_SYSTEM;
	SetUserInfoAttributeRequest userInfoAttributeReq;
	userInfoAttributeReq.setMaskBits(0xFFFF000000FFFFFF);
	//  TODO: check if attribute bits can be made dynamic
	userInfoAttributeReq.setAttributeBits(0x0000000002000000);
	uint16_t componentId = playerInfo->getComponentProxy()->mUserSessionsProxy->getId();
	BlazeObjectId blazeobjid(componentId, 1, playerInfo->getPlayerId());
	BlazeObjectIdList &blzobjlist = userInfoAttributeReq.getBlazeObjectIdList();
	blzobjlist.push_back(blazeobjid);
	//  My_Soldier 6.4 Scenario
	err = playerInfo->getComponentProxy()->mUserSessionsProxy->setUserInfoAttribute(userInfoAttributeReq);
	return err;
}

BlazeRpcError resetUserGeoIPData(StressPlayerInfo* playerInfo) {
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::ResetUserGeoIPDataRequest request;
	Blaze::UserDataResponse response;
	request.setBlazeId(0);
	err = playerInfo->getComponentProxy()->mUserSessionsProxy->resetUserGeoIPData(request);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "resetUserGeoIPData: playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getAccount(StressPlayerInfo* playerInfo) {
	BlazeRpcError err;
	Blaze::Authentication::AccountInfo accountInfoResponse;
	err = playerInfo->getComponentProxy()->mAuthenticationProxy->getAccount(accountInfoResponse);
	return err;
}

BlazeRpcError getStatsByGroupAsync(StressPlayerInfo* playerInfo, const char8_t* statGroupName)
{
	GetStatsByGroupRequest getStatsByGroupRequest;
	getStatsByGroupRequest.setGroupName(statGroupName);
	getStatsByGroupRequest.getEntityIds().push_back(playerInfo->getPlayerId());
	getStatsByGroupRequest.setPeriodCtr(1);
	getStatsByGroupRequest.setPeriodOffset(0);
	getStatsByGroupRequest.setPeriodId(0);
	getStatsByGroupRequest.setPeriodType(0);
	getStatsByGroupRequest.setTime(0);
	getStatsByGroupRequest.setViewId(playerInfo->getViewId());
	//  playerInfo->setStatsAsyncTime( TimeValue::getTimeOfDay());  
	//char8_t buf[20240];
	//BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GetStatsByGroupAsync RPC : \n" << getStatsByGroupRequest.print(buf, sizeof(buf)));
	BlazeRpcError err = playerInfo->getComponentProxy()->mStatsProxy->getStatsByGroupAsync(getStatsByGroupRequest);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "getStatsGroupByGroupAsync: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	playerInfo->incrementViewId();
	return err;
}

BlazeRpcError getStatsByGroupAsync(StressPlayerInfo* playerInfo, Blaze::BlazeIdList friendList, const char8_t* statGroupName)
{
	BlazeRpcError err = ERR_SYSTEM;
	GetStatsByGroupRequest getStatsByGroupRequest;
	getStatsByGroupRequest.setGroupName(statGroupName);
	for (eastl::vector<Blaze::BlazeId>::iterator it = friendList.begin(); it != friendList.end(); it++)
	{
		getStatsByGroupRequest.getEntityIds().push_back((*it));
	}
	getStatsByGroupRequest.setPeriodCtr(1);
	getStatsByGroupRequest.setPeriodOffset(0);
	getStatsByGroupRequest.setPeriodId(0);
	getStatsByGroupRequest.setPeriodType(0);
	getStatsByGroupRequest.setTime(0);
	getStatsByGroupRequest.setViewId(playerInfo->getViewId());
	playerInfo->setStatsAsyncTime(TimeValue::getTimeOfDay());
	//char8_t buf[20240];
	//BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "GetStatsByGroupAsync RPC : \n" << getStatsByGroupRequest.print(buf, sizeof(buf)));
	err = playerInfo->getComponentProxy()->mStatsProxy->getStatsByGroupAsync(getStatsByGroupRequest);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "getStatsGroupByGroupAsync: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	playerInfo->incrementViewId();
	return err;
}

BlazeRpcError getStatsByGroupAsync(StressPlayerInfo* playerInfo, Blaze::BlazeIdList friendList, const char8_t* statGroupName, ClubId clubId)
{
	BlazeRpcError err = ERR_SYSTEM;
	GetStatsByGroupRequest getStatsByGroupRequest;

	for (eastl::vector<Blaze::BlazeId>::iterator it = friendList.begin(); it != friendList.end(); it++)
	{
		getStatsByGroupRequest.getEntityIds().push_back((*it));
	}
	Blaze::Stats::ScopeNameValueMap& valueMap = getStatsByGroupRequest.getKeyScopeNameValueMap();
	valueMap["club"] = clubId;

	getStatsByGroupRequest.setGroupName(statGroupName);
	getStatsByGroupRequest.setPeriodCtr(1);
	getStatsByGroupRequest.setPeriodOffset(0);
	getStatsByGroupRequest.setPeriodId(0);
	getStatsByGroupRequest.setPeriodType(0);
	getStatsByGroupRequest.setTime(0);
	getStatsByGroupRequest.setViewId(playerInfo->getViewId());
	//playerInfo->setStatsAsyncTime(TimeValue::getTimeOfDay());
	err = playerInfo->getComponentProxy()->mStatsProxy->getStatsByGroupAsync(getStatsByGroupRequest);
	char8_t buf[20240];
	BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "GetStatsByGroupAsync RPC : \n" << getStatsByGroupRequest.print(buf, sizeof(buf)));
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "getStatsGroupByGroupAsync: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	playerInfo->incrementViewId();
	return err;
}

BlazeRpcError getEntityRank(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_SYSTEM;
	FilteredLeaderboardStatsRequest req;
	Blaze::Stats::GetEntityRankResponse res;

	req.setEnforceCutoffStatValue(false);
	req.setIncludeStatlessEntities(true);
	req.setBoardId(0);
	req.setLimit(4294967295);
	if (StressConfigInst.getPlatform() == Platform::PLATFORM_PS4)
	{
		req.setBoardName("bundes_seasonal_currentPS4");
	}
	else
	{
		req.setBoardName("bundes_seasonalplay_currentXONE");
	}
	req.setPeriodOffset(0);
	req.setPeriodId(0);
	req.setTime(1520461322);
	req.getListOfIds().push_back(playerInfo->getPlayerId());
	BlazeObjectId userSetId = BlazeObjectId(EA::TDF::OBJECT_TYPE_INVALID, 0);
	req.setUserSetId(userSetId);
	//char8_t buf[20240];
	//BLAZE_TRACE_LOG(BlazeRpcLog::stats, "GetEntityRank RPC : \n" << req.print(buf, sizeof(buf)));
	err = playerInfo->getComponentProxy()->mStatsProxy->getEntityRank(req, res);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "GetEntityRank: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getStatsByGroupAsync(StressPlayerInfo* playerInfo, const char8_t* groupName, Blaze::Stats::GetStatsByGroupRequest::EntityIdList& entityList, const char8_t* ksumName, EntityId ksumValue, int32_t periodType) {
	BlazeRpcError err = ERR_OK;
	Blaze::Stats::GetStatsByGroupRequest request;
	entityList.copyInto(request.getEntityIds());
	request.setGroupName(groupName);
	request.setViewId(playerInfo->getViewId());
	request.setPeriodId(0);
	request.setPeriodCtr(1);
	request.setTime(0);
	request.setPeriodOffset(0);
	request.setPeriodType(periodType);
	if (ksumName != NULL) {
		Blaze::Stats::ScopeNameValueMap& valueMap = request.getKeyScopeNameValueMap();
		valueMap[ksumName] = ksumValue;
	}
	//char8_t buf[20240];
	//BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "GetStatsByGroupAsync RPC : \n" << request.print(buf, sizeof(buf)));
	err = playerInfo->getComponentProxy()->mStatsProxy->getStatsByGroupAsync(request);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::stats, "getStatsByGroupAsync(): playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}

	playerInfo->incrementViewId();
	return err;
}


BlazeRpcError getStatGroupList(StressPlayerInfo* playerInfo) {
	BlazeRpcError err = ERR_SYSTEM;  // ERR_SYSTEM represents General system error.
	Blaze::Stats::StatGroupList statsGroupresponse;
	err = playerInfo->getComponentProxy()->mStatsProxy->getStatGroupList(statsGroupresponse);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "getStatGroupList: playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getStatGroup(StressPlayerInfo* playerInfo, const char8_t* name) {
	BlazeRpcError err = ERR_OK;
	Blaze::Stats::GetStatGroupRequest request;
	Blaze::Stats::StatGroupResponse response;
	request.setName(name);
	err = playerInfo->getComponentProxy()->mStatsProxy->getStatGroup(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "getStatGroup: playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getLeaderboard(StressPlayerInfo* playerInfo, const char8_t* bName) {
	BlazeRpcError err = ERR_OK;
	Blaze::Stats::LeaderboardStatsRequest request;
	Blaze::Stats::LeaderboardStatValues response;
	request.setBoardName(bName);
	request.setCount(100);
	request.setBoardId(0);
	request.setPeriodId(0);
	request.setPeriodOffset(0);
	request.setRankStart(0);
	err = playerInfo->getComponentProxy()->mStatsProxy->getLeaderboard(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "statsGetLeaderboard: playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getLeaderboard(StressPlayerInfo* playerInfo, int32_t iboardId, int32_t iCount, const char8_t* bName) {
	BlazeRpcError err = ERR_OK;
	Blaze::Stats::LeaderboardStatsRequest request;
	Blaze::Stats::LeaderboardStatValues response;
	request.setBoardName(bName);
	request.setCount(iCount);
	request.setBoardId(iboardId);
	request.setPeriodId(0);
	request.setPeriodOffset(0);
	request.setRankStart(0);
	err = playerInfo->getComponentProxy()->mStatsProxy->getLeaderboard(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "statsGetLeaderboard: playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getLeaderboardGroup(StressPlayerInfo* playerInfo, int32_t boardId, const char8_t* boardName) {
	BlazeRpcError err = ERR_OK;
	Blaze::Stats::LeaderboardGroupRequest request;
	Blaze::Stats::LeaderboardGroupResponse response;
	request.setBoardName(boardName);
	request.setBoardId(boardId);

	err = playerInfo->getComponentProxy()->mStatsProxy->getLeaderboardGroup(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "statsGetLeaderboardGroup: playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}

	return err;
}



BlazeRpcError getLeaderboardTreeAsync(StressPlayerInfo* playerInfo, const char8_t* name) {
	BlazeRpcError err = ERR_OK;
	Blaze::Stats::GetLeaderboardTreeRequest request;
	request.setFolderName(name);
	/*if (BLAZE_IS_TRACE_RPC_ENABLED(BlazeRpcLog::stats)) {
		char8_t buf[1024];
		BLAZE_TRACE_RPC_LOG(BlazeRpcLog::stats, "-> [fifautilities][" << getIdent() << "]::getLeaderboardTreeAsync" << "\n" << request.print(buf, sizeof(buf)));
	}*/
	err = playerInfo->getComponentProxy()->mStatsProxy->getLeaderboardTreeAsync(request);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "getCenteredLeaderboard: playerId " << playerInfo->getPlayerId() << " failed with error %d" << err);
	}
	return err;
}

BlazeRpcError getCenteredLeaderboard(StressPlayerInfo* playerInfo, int32_t iboardId, int32_t iCount, BlazeId UserBlazeId, const char8_t* strboardName) {
	BlazeRpcError err = ERR_OK;
	//BlazeObjectId messageObject;
	Blaze::Stats::CenteredLeaderboardStatsRequest request;
	Blaze::Stats::LeaderboardStatValues response;
	request.setShowAtBottomIfNotFound(true);
	request.setBoardId(iboardId);
	request.setCount(100);
	request.setBoardName(strboardName);
	request.setCenter(UserBlazeId);
	request.setPeriodId(0);
	request.setPeriodOffset(0);
	request.setTime(0);
	//request.setUserSetId(messageObject);
	err = playerInfo->getComponentProxy()->mStatsProxy->getCenteredLeaderboard(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "getCenteredLeaderboard: playerId " << playerInfo->getPlayerId() << " failed with error %d" << err);
	}
	return err;
}

BlazeRpcError getKeyScopesMap(StressPlayerInfo* playerInfo) {
	BlazeRpcError err = ERR_OK;
	Blaze::Stats::KeyScopes response;
	err = playerInfo->getComponentProxy()->mStatsProxy->getKeyScopesMap(response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "statsGetKeyScopesMap: playerId " << playerInfo->getPlayerId() << " failed with error %d" << err);
	}
	return err;
}

BlazeRpcError getPeriodIds(StressPlayerInfo* playerInfo) {
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::Stats::PeriodIds perIdResponse;
	err = playerInfo->getComponentProxy()->mStatsProxy->getPeriodIds(perIdResponse);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "getPeriodIds: playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError   lookupUserByExternalId(StressPlayerInfo* playerInfo, ExternalId ExtId)
{
	//   UserSessionsComponent::lookupUser
	if (ExtId <= 0)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "Usersession::lookupUserByExternalId failed to invoke  lookupUser RPC, Reason: External Id is 0 for Player " << playerInfo->getPlayerId());
		return ERR_SYSTEM;
	}
	UserIdentification req;
	UserData           resp;
	req.setExternalId(ExtId);
	BlazeRpcError err = playerInfo->getComponentProxy()->mUserSessionsProxy->lookupUser(req, resp);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "lookupUserByExternalId: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError lookupUsersByPersonaNames(StressPlayerInfo* PlayerInfo, Blaze::PersonaNameList& personaNameList)
{
	BLAZE_INFO_LOG(Log::SYSTEM, "[utility]  LookupUsersByPersonaNames: ");
	BlazeRpcError err = ERR_OK;
	if (PlayerInfo)
	{
		Blaze::LookupUsersByPersonaNamesRequest request;
		Blaze::UserDataResponse   response;
		
		if (personaNameList.size() == 0)
		{
			return err;
		}
		if (personaNameList.size() > 0 && personaNameList[0] == "")
		{
			request.getPersonaNameList().push_back("");
		}
		else if (personaNameList.size() > 0)
		{
			personaNameList.copyInto(request.getPersonaNameList());
		}
		
		Platform platType = StressConfigInst.getPlatform();

		if (platType == PLATFORM_XONE)
		{
			request.setPersonaNamespace("xbox");
		}
		else if (platType == PLATFORM_PS4)
		{
			request.setPersonaNamespace("ps3");
		}
		//else
		//{
		//	request.setPersonaNamespace("cem_ea_id");
		//}
		
		err = PlayerInfo->getComponentProxy()->mUserSessionsProxy->lookupUsersByPersonaNames(request, response);
		//char8_t buff[2048];
		//BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "lookupusersbypersonaname request:" << "\n" << request.print(buff, sizeof(buff)));
		if (ERR_OK != err)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "lookupUsersByPersonaNames failed with error = " << Blaze::ErrorHelp::getErrorName(err));
		}
	}
	return err;
}

BlazeRpcError lookupUsersByPersonaNames(StressPlayerInfo* playerInfo, const char8_t* strPersonaName)
{
	LookupUsersByPersonaNamesRequest	req;
	UserDataResponse					resp;
	//if (!strcmp(strPersonaName,""))
	//	return ERR_OK;
	req.getPersonaNameList().push_back(strPersonaName);
	//char8_t buff[2048];
	//BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "getPetitions RPC" << "\n" << req.print(buff, sizeof(buff)));

	BlazeRpcError err = playerInfo->getComponentProxy()->mUserSessionsProxy->lookupUsersByPersonaNames(req, resp);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "lookupUsersByPersonaNames: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError lookupUsers(StressPlayerInfo* playerInfo, const char8_t* strPersonaName)
{
	Blaze::LookupUsersRequest request;
	Blaze::UserDataResponse response;
	request.setLookupType(Blaze::LookupUsersRequest::EXTERNAL_ID);
	Blaze::UserIdentificationList& userIdentificationLst = request.getUserIdentificationList();
	Blaze::UserIdentification* userIdent = BLAZE_NEW Blaze::UserIdentification();
	userIdent->setName(strPersonaName);
	userIdentificationLst.push_back(userIdent);
	BlazeRpcError err = playerInfo->getComponentProxy()->mUserSessionsProxy->lookupUsers(request, response);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "lookupUser: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}

//  call UserSessionsProxy::lookupUsers RPC with random list of users
BlazeRpcError   lookupRandomUsersByPersonaNames(StressPlayerInfo* playerInfo)
{
	//   UserSessionsComponent::lookupUser
	BlazeRpcError err = ERR_SYSTEM;
	LookupUsersByPersonaNamesRequest request;
	UserDataResponse response;
	PersonaNameList& personaList = request.getPersonaNameList();
	UserIdentificationList tempuIdentificationList;
	uint32_t friendListSize = Blaze::Random::getRandomNumber(FRIEND_LIST_SIZE_AVG) + 1;
	err = createRandomUserList(playerInfo, &tempuIdentificationList, friendListSize);
	if (ERR_OK == err)
	{
		for (UserIdentificationList::iterator tempuidIterator = tempuIdentificationList.begin(); tempuidIterator != tempuIdentificationList.end(); tempuidIterator++)
		{
			personaList.push_back((*tempuidIterator)->getName());
		}
		err = playerInfo->getComponentProxy()->mUserSessionsProxy->lookupUsersByPersonaNames(request, response);
	}
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "lookupUsers: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}

	return err;
}

BlazeRpcError   lookupRandomUserByBlazeId(StressPlayerInfo* playerInfo, UserIdentificationList* uidListOut /* = NULL */)
{
	//   UserSessionsComponent::lookupUser
	BlazeRpcError err = ERR_SYSTEM;
	UserIdentification req;
	UserData           resp;
	UserIdentificationList tempuIdentificationList;
	uint32_t friendListSize = Blaze::Random::getRandomNumber(FRIEND_LIST_SIZE_AVG) + 1;
	err = createRandomUserList(playerInfo, &tempuIdentificationList, friendListSize);
	if (ERR_OK == err && (tempuIdentificationList.size() > 0))
	{
		BlazeId userId = tempuIdentificationList[Random::getRandomNumber(tempuIdentificationList.size())]->getBlazeId();
		req.setBlazeId(userId);
		err = playerInfo->getComponentProxy()->mUserSessionsProxy->lookupUser(req, resp);
	}
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "lookupUsers: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	else
	{
		//   Copy the uid list to out param.
		if (uidListOut != NULL)
		{
			tempuIdentificationList.copyInto(*uidListOut);
		}
	}
	return err;
}


BlazeRpcError createRandomUserList(StressPlayerInfo* playerInfo, UserIdentificationList *uIdentificationList, uint32_t maxUsers)
{
	BlazeRpcError err = ERR_SYSTEM;
	int32_t totalUsers = Random::getRandomNumber(maxUsers) + 1;
	PlayerDetailsMap playerMap = ((Blaze::Stress::PlayerModule*)playerInfo->getOwner())->getCommonMap();
	size_t playerSize = playerMap.size();
	if (playerSize > 1)
	{
		err = ERR_OK;
		int32_t index = Random::getRandomNumber(playerSize);
		UserIdentification *uIdentification = uIdentificationList->pull_back();
		//   Choose a starting random index in the map and pick next 'x'number of players starting from that index.
		for (PlayerDetailsMap::iterator itr = playerMap.begin(); itr != playerMap.end(); ++itr)
		{
			if (index-- > 0) {
				continue;
			}
			if (totalUsers > 0)
			{
				if (playerInfo->getPlayerId() != (*itr).first)
				{
					uIdentification->setBlazeId((*itr).first);
					uIdentification->setName((*itr).second.getplayerPersonaName().c_str());
					totalUsers--;
				}
			}
			else
			{
				break;
			}
		}
	}
	return err;
}

BlazeRpcError  actionPostNews(StressPlayerInfo* playerInfo, ClubId mClubId, uint32_t newsCount)
{
	BlazeRpcError err;
	PostNewsRequest request;
	request.setClubId(mClubId);
	ClubNews &news = request.getClubNews();
	news.setType(CLUBS_MEMBER_POSTED_NEWS);
	eastl::string newsString;
	newsString.sprintf("newsBody", newsCount);
	eastl::string txt;
	txt.sprintf(" User: %" PRId64 ", Club: %d", playerInfo->getPlayerId(), mClubId);
	newsString.append(txt);
	news.setText(newsString.c_str());
	news.getUser().setBlazeId(playerInfo->getPlayerId());
	err = playerInfo->getComponentProxy()->mClubsProxy->postNews(request);
	if (ERR_OK != err)
	{
		BLAZE_ERR(BlazeRpcLog::clubs, "Post news failed with error = %s.", ErrorHelp::getErrorName(err));
	}
	return err;

}

BlazeRpcError lookupUsersByBlazeId(StressPlayerInfo* playerInfo, const Blaze::BlazeId &playerId)
{
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::LookupUsersRequest req;
	Blaze::UserDataResponse resp;
	req.setLookupType(req.BLAZE_ID);
	UserIdentification* uid = req.getUserIdentificationList().pull_back();
	uid->setAccountId(0);
	PlatformInfo &platformInfo = uid->getPlatformInfo();
	platformInfo.getEaIds().setOriginPersonaName("");
	platformInfo.getEaIds().setNucleusAccountId(0);
	platformInfo.getEaIds().setOriginPersonaId(0);
	platformInfo.getExternalIds().setPsnAccountId(0);
	platformInfo.getExternalIds().setXblAccountId(0);
	platformInfo.setClientPlatform(Blaze::ClientPlatformType::INVALID);
	uid->setAccountLocale(0);
	uid->setAccountCountry(0);
	uid->setExternalId(0);
	uid->setName("");
	uid->setPersonaNamespace("");
	uid->setOriginPersonaId(0);
	uid->setPidId(0);
	uid->setBlazeId(playerId);
	uid->getPlatformInfo().setClientPlatform(Blaze::ClientPlatformType::INVALID);
	err = playerInfo->getComponentProxy()->mUserSessionsProxy->lookupUsers(req, resp);
	if (ERR_OK != err)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[lookupUsersByBlazeId][%" PRIu64 "]: looking up user [%" PRIu64 "] failed with error = %s.", playerInfo->getPlayerId(), playerId, ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError lookupRandomUsersByExternalId(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_SYSTEM;
	if (playerInfo->getFriendExternalIdList().size() == 0)
	{
		return err;
	}
	Blaze::LookupUsersRequest req;
	Blaze::UserDataResponse resp;
	ExternalId externalId = playerInfo->getFriendExternalIdList()[Random::getRandomNumber(playerInfo->getFriendExternalIdList().size())];
	req.setLookupType(req.EXTERNAL_ID);
	UserIdentification* uid = req.getUserIdentificationList().pull_back();
	uid->setExternalId(externalId);
	err = playerInfo->getComponentProxy()->mUserSessionsProxy->lookupUsers(req, resp);
	if (ERR_OK != err)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[lookupUsersByExternalId][%" PRIu64 "]: looking up user [%" PRIu64 "] failed with error = %s.", playerInfo->getPlayerId(), externalId,
			ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError lookupUserByBlazeId(StressPlayerInfo* playerInfo, Blaze::BlazeId playerId)
{
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::UserIdentification req;
	Blaze::UserData resp;
	req.setBlazeId(playerId);
	err = playerInfo->getComponentProxy()->mUserSessionsProxy->lookupUser(req, resp);
	if (ERR_OK != err)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[lookupUserByBlazeId][%" PRIu64 "]: looking up user [%" PRIu64 "] failed with error = %s.", playerInfo->getPlayerId(), playerId, ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError updateExtendedDataAttribute(StressPlayerInfo* playerInfo, const Blaze::BlazeId &playerId)
{
	BlazeRpcError err = ERR_SYSTEM;
	UpdateExtendedDataAttributeRequest req;
	req.setValue((UserExtendedDataValue)playerId);
	err = playerInfo->getComponentProxy()->mUserSessionsProxy->updateExtendedDataAttribute(req);
	if (ERR_OK != err)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[updateExtendedDataAttribute][%" PRIu64 "]: updateExtendedDataAttribute for user [%" PRIu64 "] failed with error = %s.", playerInfo->getPlayerId(), playerId,
			ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getStatsByGroupAsync(StressPlayerInfo* playerInfo, PlayerId playerId, uint32_t viewId, const char8_t* statGroupName)
{
	GetStatsByGroupRequest getStatsByGroupRequest;
	getStatsByGroupRequest.setGroupName(statGroupName);
	// Add all clubs member Entity IDs here - TODO
	getStatsByGroupRequest.getEntityIds().push_back(playerId);
	getStatsByGroupRequest.setPeriodCtr(1);
	getStatsByGroupRequest.setPeriodOffset(0);
	getStatsByGroupRequest.setPeriodId(0);
	getStatsByGroupRequest.setPeriodType(0);
	getStatsByGroupRequest.setTime(0);
	getStatsByGroupRequest.setViewId(viewId);
	BlazeRpcError err = playerInfo->getComponentProxy()->mStatsProxy->getStatsByGroupAsync(getStatsByGroupRequest);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "getStatsGroupByGroupAsync: playerId " << playerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}

//  associationlists calls
BlazeRpcError associationGetLists(StressPlayerInfo* playerInfo)
{
	Blaze::Association::GetListsRequest AssociationListReq;
	Blaze::Association::Lists AssociationListResp;
	AssociationListReq.setOffset(0);
	AssociationListReq.setMaxResultCount(4294967295);// As per log it is 4294967295

	Blaze::Association::ListInfoVector& listInfoVector = AssociationListReq.getListInfoVector();
	Blaze::Association::ListInfo *listInfo = BLAZE_NEW Blaze::Association::ListInfo;
	BlazeObjectId objectId = BlazeObjectId(EA::TDF::OBJECT_TYPE_INVALID, 0);
	listInfo->setBlazeObjId(objectId);
	Blaze::Association::ListIdentification& lstIdentification = listInfo->getId();
	lstIdentification.setListName("");
	lstIdentification.setListType(1);
	listInfo->setMaxSize(0);
	Blaze::Association::ListStatusFlags& lstStatusFlags = listInfo->getStatusFlags();// Not set status flags
	lstStatusFlags.setBits(1);
	listInfo->setPairId(0);
	listInfo->setPairName("");
	listInfo->setPairMaxSize(0);
	listInfoVector.push_back(listInfo);


	listInfo = BLAZE_NEW Blaze::Association::ListInfo;
	BlazeObjectId objectId1 = BlazeObjectId(EA::TDF::OBJECT_TYPE_INVALID, 0);
	listInfo->setBlazeObjId(objectId1);
	Blaze::Association::ListIdentification& lstIdentification1 = listInfo->getId();
	lstIdentification1.setListName("OSDKPreferredPlayerList");
	lstIdentification1.setListType(0);
	listInfo->setMaxSize(0);
	Blaze::Association::ListStatusFlags& lstStatusFlags1 = listInfo->getStatusFlags();// Not set status flags
	lstStatusFlags1.setBits(1);
	listInfo->setPairId(0);
	listInfo->setPairName("");
	listInfo->setPairMaxSize(0);
	listInfoVector.push_back(listInfo);

	listInfo = BLAZE_NEW Blaze::Association::ListInfo;
	BlazeObjectId objectId2 = BlazeObjectId(EA::TDF::OBJECT_TYPE_INVALID, 0);
	listInfo->setBlazeObjId(objectId2);
	Blaze::Association::ListIdentification& lstIdentification2 = listInfo->getId();
	lstIdentification2.setListName("OSDKAvoidPlayerList");
	lstIdentification2.setListType(0);
	listInfo->setMaxSize(0);
	Blaze::Association::ListStatusFlags& lstStatusFlags2 = listInfo->getStatusFlags();// Not set status flags
	lstStatusFlags2.setBits(1);
	listInfo->setPairId(0);
	listInfo->setPairName("");
	listInfo->setPairMaxSize(0);
	listInfoVector.push_back(listInfo);
	BlazeRpcError err = playerInfo->getComponentProxy()->mAssociationListsProxy->getLists(AssociationListReq, AssociationListResp);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::associationlists, " AssociationLists::getLists: playerId " << playerInfo->getPlayerId() << " failed with error = " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError associationSubscribeToLists(StressPlayerInfo* playerInfo, const char8_t* listName, Association::ListType listType)
{
	Blaze::Association::UpdateListsRequest subscribeToListsReq;
	ListIdentification* listIdent = subscribeToListsReq.getListIdentificationVector().pull_back();
	listIdent->setListType(listType);
	listIdent->setListName(listName);
	BlazeRpcError err = playerInfo->getComponentProxy()->mAssociationListsProxy->subscribeToLists(subscribeToListsReq);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::associationlists, " subscribeToLists: playerId " << playerInfo->getPlayerId() << " failed with error = " << ErrorHelp::getErrorName(err));
	}
	return err;
}


BlazeRpcError clearLists(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_OK;
	Blaze::Association::UpdateListsRequest updateReq;
	updateReq.setBlazeId(0);
	ListIdentificationVector&  listIdVec = updateReq.getListIdentificationVector();
	ListIdentification* listId = listIdVec.allocate_element();
	listId->setListName("friendList");
	listId->setListType(1);
	listIdVec.push_back(listId);

	err = playerInfo->getComponentProxy()->mAssociationListsProxy->clearLists(updateReq);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "clearLists: playerId " << playerInfo->getPlayerId() << " failed with error :  " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError setUsersToList(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::Association::UpdateListMembersRequest updateListMembersReq;
	Blaze::Association::UpdateListMembersResponse updateListMembersResp;
	updateListMembersReq.getListIdentification().setListName("OSDKPlatformFriendList");
	updateListMembersReq.getListIdentification().setListType(Blaze::Association::LIST_TYPE_FRIEND);
	updateListMembersReq.setValidateDelete(true);
	updateListMembersReq.setBlazeId(0);
	updateListMembersReq.getAttributesMask().setBits(0);

	err = playerInfo->getComponentProxy()->mAssociationListsProxy->setUsersToList(updateListMembersReq, updateListMembersResp);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[setUsersToList][" << playerInfo->getPlayerId() << "]: AssociationLists::setUsersToList failed with error = " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError setUsersToList(StressPlayerInfo* playerInfo, const UserIdentificationList& uidList)
{
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::Association::UpdateListMembersRequest updateListMembersReq;
	Blaze::Association::UpdateListMembersResponse updateListMembersResp;
	updateListMembersReq.getListIdentification().setListName(FRIEND_LIST);
	updateListMembersReq.getListIdentification().setListType(Blaze::Association::LIST_TYPE_FRIEND);
	Blaze::Association::ListMemberIdVector& listMemberIds = updateListMembersReq.getListMemberIdVector();
	if (uidList.size() > 0)
	{
		Blaze::UserIdentificationList::const_iterator citStart = uidList.begin();
		Blaze::UserIdentificationList::const_iterator citEnd = uidList.end();
		Blaze::UserIdentificationList::const_iterator cit = citStart;
		for (; cit != citEnd; ++cit)
		{
			Association::ListMemberId *listMemberId = BLAZE_NEW Association::ListMemberId();
			if (NULL != listMemberId)
			{
				const char8_t* personaName = (*cit)->getName();
				listMemberId->setExternalSystemId(Blaze::INVALID);
				listMemberId->getUser().setName(personaName);
				listMemberId->getUser().setExternalId(0);
				if (NULL != personaName)
				{
					listMemberIds.push_back(listMemberId);
				}
			}
		}
	}
	err = playerInfo->getComponentProxy()->mAssociationListsProxy->setUsersToList(updateListMembersReq, updateListMembersResp);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[setUsersToList][" << playerInfo->getPlayerId() << "]: AssociationLists::setUsersToList failed with error = " << ErrorHelp::getErrorName(err));
	}
	//   Free the allocated memory.
	//for (size_t i = 0; i < listMemberIds.size(); i++)
	//{
		//delete listMemberIds[i];
	//}
	listMemberIds.clear();
	return err;
}

BlazeRpcError userSettingsSave(StressPlayerInfo* playerInfo, const char8_t* key, const char8_t* data)
{
	Blaze::Util::UserSettingsSaveRequest request;
	uint16_t randomKey = (uint16_t)rand() % 100;
	if (randomKey <= 70)
	{
		request.setKey(key);
		request.setData(data);
	}
	else
	{
		request.setData("CLQMV|O_NT=12&O_MAX=222&FIFADIV=12&FIFACLUBNUMPLAY=02&FIFACLUBMMANY=02&FIFACLUBMMGK=0H2H_QM|O_NT=25&O_JS=05&O_HM=05&O_HV=35&O_PM=15&O_GM=05&O_MIN=25&O_MAX=25&O_DSR=25&O_DNF=-15&O_PR=05&O_AI=15&O_SS=05&O_SD=05&O_CP=15&O_LN=45&O_MYPT=05&O_OPPT=05&O_ARNA=05&O_TT=15&SLVL=-15&FIFAHFLH=45&FIFACSCTRL=05&FIFAGMSPEED=15&FIFACOOPDIV=15&FIFACUP=45&FIFAROSTER=05&FIFAPPT=05&FIFAGKCTRL=15&FIFACLUBNUMPLAY=05&FIFACLUBLEAGUE=155&FIFACLUBMMANY=05&FIFACLUBMMGK=05&FIFAOVREX=05&FIFAMCT=05&FIFAMG=05&M_HASH=30995575&FIFACUSTVER=15&OSDK_clubId=05&OSDK_tourTier=1H2H_CS|O_NT=27&O_GEO=16&O_JS=06&O_HM=06&O_HV=36&O_PM=16&O_GM=06&O_MIN=26&O_MAX=26&O_DSR=26&O_DNF=-16&O_PR=06&O_AI=16&O_SS=16&O_SD=16&O_CP=16&O_LN=46&O_MYPT=6&O_OPPT=6&O_ARNA=6&O_TT=6&SLVL=-16&FIFAHFLH=46&FIFACSCTRL=06&FIFAGMSPEED=16&FIFAWOMEN=06&FIFACOOPDIV=16&FIFACUP=46&FIFAROSTER=06&FIFAPPT=06&FIFAGKCTRL=-16&FIFACLUBNUMPLAY=06&FIFACLUBLEAGUE=166&FIFACLUBMMANY=06&FIFACLUBMMGK=06&FIFAOVREX=06&FIFAMCT");
		request.setKey("MUPSET");
	}

	BlazeRpcError err = playerInfo->getComponentProxy()->mUtilProxy->userSettingsSave(request);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "userSettingsSave: playerId " << playerInfo->getPlayerId() << " failed with error = " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError filterForProfanity(StressPlayerInfo* playerInfo, Blaze::Util::FilterUserTextResponse *request)
{
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::Util::FilterUserTextResponse *response = BLAZE_NEW Blaze::Util::FilterUserTextResponse();

	err = playerInfo->getComponentProxy()->mUtilProxy->filterForProfanity(*request, *response);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, " filterForProfanity: playerId " << playerInfo->getPlayerId() << " failed with error = " << ErrorHelp::getErrorName(err));
	}
	delete response;
	return err;
}

BlazeRpcError getGameDataByUser(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_OK;
	GameManager::GetGameDataByUserRequest getGameDataByUserReq;
	GameManager::GameBrowserDataList gameBrowserDataList;
	//  getGameDataByUserReq.setListConfigName(StressConfigInst.getServerBrowserSnapshotListConfigName());
	getGameDataByUserReq.setUserSetId(playerInfo->getConnGroupId());
	err = getGameDataByUser(playerInfo, getGameDataByUserReq, gameBrowserDataList);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, " getGameDataByUser: playerId " << playerInfo->getPlayerId() << " failed with error = " << ErrorHelp::getErrorName(err));
	}

	return err;
}

BlazeRpcError getGameDataByUser(StressPlayerInfo* playerInfo, BlazeId blazeid, eastl::string persona)
{
	BlazeRpcError err = ERR_OK;
	GameManager::GetGameDataByUserRequest getGameDataByUserReq;
	GameManager::GameBrowserDataList gameBrowserDataList;
	//  need to make ListConfigName configurable
	//  getGameDataByUserReq.setListConfigName("userInfoCheck");
	getGameDataByUserReq.getUser().setBlazeId(blazeid);
	getGameDataByUserReq.getUser().setName(persona.c_str());
	err = getGameDataByUser(playerInfo, getGameDataByUserReq, gameBrowserDataList);
	return err;
}

BlazeRpcError getGameDataByUser(StressPlayerInfo* playerInfo, GameManager::GetGameDataByUserRequest& getGameDataByUserReq, GameManager::GameBrowserDataList& gameBrowserDataList)
{
	BlazeRpcError err = ERR_OK;
	err = playerInfo->getComponentProxy()->mGameManagerProxy->getGameDataByUser(getGameDataByUserReq, gameBrowserDataList);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManager::getGameDataByUser][" << playerInfo->getPlayerId() << "] failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getGameDataByUser(StressPlayerInfo* playerInfo, const char8_t* strConfigName, const char8_t* strPersonaName, BlazeId blzId, ExternalId extId)
{
	BlazeRpcError err = ERR_OK;
	GameManager::GetGameDataByUserRequest getGameDataByUserReq;
	GameManager::GameBrowserDataList gameBrowserDataList;
	getGameDataByUserReq.setListConfigName(strConfigName);
	getGameDataByUserReq.getUser().setExternalId(extId);
	getGameDataByUserReq.getUser().setBlazeId(blzId);
	getGameDataByUserReq.getUser().setName(strPersonaName);
	err = playerInfo->getComponentProxy()->mGameManagerProxy->getGameDataByUser(getGameDataByUserReq, gameBrowserDataList);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManager::getGameDataByUser][" << playerInfo->getPlayerId() << "] failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getGameDataFromId(StressPlayerInfo* playerInfo, const Blaze::GameManager::GameId gameId, const char8_t* listConfigName)
{
	GameManager::GetGameDataFromIdRequest req;
	GameManager::GameBrowserDataList resp;
	BlazeRpcError result = ERR_SYSTEM;
	if (NULL == listConfigName)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[FAIL] [getGameDataFromId ] " << playerInfo->getIdent() << "::BrowserListConfigName is null");
		return result;
	}
	req.setListConfigName(listConfigName);
	req.getGameIds().push_back(gameId);
	result = playerInfo->getComponentProxy()->mGameManagerProxy->getGameDataFromId(req, resp);
	if (ERR_OK != result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[FAIL]::getGameDataFromId:" << listConfigName);
	}
	else
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SUCCESS]::getGameDataFromId");
	}
	return result;
}


BlazeRpcError getFullGameData(StressPlayerInfo* playerInfo, const Blaze::GameManager::GameId gameId, GameManager::GetFullGameDataResponse* GetFullGameDataResp /* = NULL */)
{
	BlazeRpcError err = ERR_SYSTEM;
	GameManager::GetFullGameDataRequest req;
	GameManager::GetFullGameDataResponse resp;
	req.getGameIdList().push_back(gameId);  //  add game id
	err = playerInfo->getComponentProxy()->mGameManagerProxy->getFullGameData(req, ((GetFullGameDataResp) ? (*GetFullGameDataResp) : resp));
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[getFullGameData][" << playerInfo->getPlayerId() << "] GameManager::getFullGameData failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getMemberHash(StressPlayerInfo* mPlayerData, const char8_t* DefaultListName)
{
	GetMemberHashRequest hashReq;
	GetMemberHashResponse hashResp;
	BlazeRpcError err;
	hashReq.setBlazeId(0);
	hashReq.getListIdentification().setListType(Blaze::Association::LIST_TYPE_FRIEND);
	hashReq.getListIdentification().setListName(DefaultListName);
	err = mPlayerData->getComponentProxy()->mAssociationListsProxy->getMemberHash(hashReq, hashResp);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[FAIL] " << mPlayerData->getPlayerId() << "::getMemberHash failed with error " << (Blaze::ErrorHelp::getErrorName(err)));
	}
	return err;
}

BlazeRpcError updateGameHostMigrationStatus(StressPlayerInfo* playerInfo, const GameManager::GameId gameId, const GameManager::HostMigrationType migrationType)
{
	BlazeRpcError err = ERR_SYSTEM;
	UpdateGameHostMigrationStatusRequest migrationReq;
	migrationReq.setGameId(gameId);
	migrationReq.setHostMigrationType(migrationType);
	err = playerInfo->getComponentProxy()->mGameManagerProxy->updateGameHostMigrationStatus(migrationReq);
	if (ERR_OK != err)
	{
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[OnNotifyHostMigrationStart][%" PRIu64 "]: updateGameHostMigrationStatus() failed with error %s", playerInfo->getPlayerId(), ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError requestPlatformHost(StressPlayerInfo* playerInfo, const Blaze::GameManager::GameId gameId, PlayerId playerId)
{
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::GameManager::MigrateHostRequest request;
	request.setGameId(gameId);
	request.setNewHostPlayer(playerId);
	err = playerInfo->getComponentProxy()->mGameManagerProxy->requestPlatformHost(request);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[GameManager::requestPlatformHost][" << playerInfo->getPlayerId() << "] failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError filePetitionForServer(StressPlayerInfo* playerInfo, GameManager::GetFullGameDataResponse* GetFullGameDataResp)
{
	BlazeRpcError err = ERR_SYSTEM;
	if (GetFullGameDataResp == NULL)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "GetFullGameDataResp parameter is NULL");
		return err;
	}
	int numGames = GetFullGameDataResp->getGames().size();
	if (numGames > 0 && numGames < INT_MAX)
	{
		int gameIndex = Random::getRandomNumber(numGames);
		ListGameData *gameData = GetFullGameDataResp->getGames().at(gameIndex);
		if (gameData != NULL) {
			err = filePetition(playerInfo, gameData->getGame().getPersistedGameId());
		}
		else
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "filePetitionForServer - Couldn't get valid game at" << gameIndex << "even though num games are " << numGames << "for player " << playerInfo->getPlayerId());
		}
	}
	return err;
}

BlazeRpcError filePetition(StressPlayerInfo* playerInfo, const char8_t* PetitionDetail, const char8_t* complaintType, const char8_t* subject, const char8_t* timeZone)
{
	BlazeRpcError err = ERR_SYSTEM;
	return err;
}

//BlazeRpcError createOrJoinPartyGame(StressPlayerInfo* playerInfo, JoinGameResponse& resp, Blaze::BlazeId persistedGameId /*if = 0 then use your own playerId*/)
//{
//    BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[createOrJoinGame]: In create Game");
//    BlazeRpcError err = ERR_OK;
//    uint16_t partyPlayerCount = StressConfigInst.getPartyPlayerCount();
//    GameManager::CreateGameRequest req;
//    //   UserGroupId (BTPL)
//    //   NetworkAddressList
//    req.getCommonGameData().setGameType(GAME_TYPE_GROUP);
//	req.getCommonGameData().setGameProtocolVersionString(StressConfigInst.getGameProtocolVersionString());
//	if (!playerInfo->isConnected())
//    {
//        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, " createOrJoinPartyGame::User Disconnected = " << playerInfo->getPlayerId());
//        return ERR_SYSTEM;
//    }
//	NetworkAddress& hostAddress = req.getCommonGameData().getPlayerNetworkAddress();
//	
//	if (PLATFORM_XONE == StressConfigInst.getPlatform()) {
//		hostAddress.switchActiveMember(NetworkAddress::MEMBER_XBOXCLIENTADDRESS);
//		hostAddress.getXboxClientAddress()->setXuid(playerInfo->getExternalId());
//	}
//	hostAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
//	hostAddress.getIpPairAddress()->getExternalAddress().setIp(playerInfo->getConnection()->getAddress().getIp());
//	hostAddress.getIpPairAddress()->getExternalAddress().setPort(playerInfo->getConnection()->getAddress().getPort());
//	hostAddress.getIpPairAddress()->getExternalAddress().copyInto(hostAddress.getIpPairAddress()->getInternalAddress());
//
//    //TODO: check kite logs for this parameter.For now Commenting out this parameter to resolve errors after kite integration
//    /*ScenarioInfo& scenarioInfo = req.getCommonGameData().getScenarioInfo();
//    scenarioInfo.setScenarioName("");
//    scenarioInfo.setScenarioVersion(0);
//    scenarioInfo.setScenarioVariant(Blaze::GameManager::ScenarioVariant::A);
//    scenarioInfo.setSubSessionName("");*/
//    
//    req.setGamePingSiteAlias("");
//
//	//req.getGameCreationData().getGameAttribs().insert(eastl::pair<EA::TDF::TdfString, EA::TDF::TdfString>("mode", "Party"));
//	req.getGameCreationData().setGameModRegister(0);
//    req.getGameCreationData().setGameName("");
//    req.getGameCreationData().getGameSettings().setBits(1050653);
//    //  following are settings that are being applied through above call.
//    //  req.getGameCreationData().getGameSettings().setOpenToBrowsing();  //  0x1
//    //  req.getGameCreationData().getGameSettings().setOpenToInvites();  //  0x4
//	//  req.getGameCreationData().getGameSettings().setOpenToJoinByPlayer();  //  0x8
//	//  req.getGameCreationData().getGameSettings().setHostMigratable();  //  0x10
//    //  req.getGameCreationData().getGameSettings().setEnablePersistedGameId();  //  0x800
//	//  req.getGameCreationData().getGameSettings().setAllowMemberGameAttributeEdit();//0x00100000
//	req.getGameCreationData().setNetworkTopology(NETWORK_DISABLED);
//	req.getGameCreationData().setMaxPlayerCapacity(partyPlayerCount);
//    req.getGameCreationData().setMinPlayerCapacity(1);
//    req.getGameCreationData().setPresenceMode(PRESENCE_MODE_NONE);
//    req.getGameCreationData().setQueueCapacity(0);
//	req.getGameCreationData().setVoipNetwork(VOIP_DISABLED);
//
//	//  req.setGameTypeName("");
//    req.setGameStatusUrl("");
//    
//	req.setServerNotResetable(false);
//    //   SlotCapacities
//    req.getSlotCapacities().clear();
//    req.getSlotCapacities().push_back(partyPlayerCount);
//    req.getSlotCapacities().push_back(0);
//    req.getSlotCapacities().push_back(0);
//    req.getSlotCapacities().push_back(0);
//    if (!(persistedGameId))
//    {
//        persistedGameId = playerInfo->getPlayerId();
//    }
//    std::stringstream ss;
//    ss << persistedGameId;
//    req.setPersistedGameId(ss.str().c_str());
//    
//	req.getPlayerJoinData().setGroupId(playerInfo->getConnGroupId());
//    PerPlayerJoinData* perPlayerJoinData = req.getPlayerJoinData().getPlayerDataList().pull_back();
//    req.getPlayerJoinData().setGameEntryType(GAME_ENTRY_TYPE_DIRECT);
//	perPlayerJoinData->setIsOptionalPlayer(false);
//	perPlayerJoinData->getUser().setAccountId( playerInfo->getPlayerId());
//	perPlayerJoinData->getUser().setExternalId(playerInfo->getExternalId());
//	perPlayerJoinData->getUser().setBlazeId(playerInfo->getPlayerId());
//    perPlayerJoinData->getUser().setName(playerInfo->getPersonaName().c_str());
//	
//    //   SlotType
//    req.getPlayerJoinData().setSlotType(SLOT_PUBLIC_PARTICIPANT);
//    req.getPlayerJoinData().setTeamId(ANY_TEAM_ID);
//    req.getPlayerJoinData().setTeamIndex(0);
//	
//    err = playerInfo->getComponentProxy()->mGameManagerProxy->createOrJoinGame(req, resp);
//    if (err != ERR_OK)
//    {
//        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "createOrJoinGame[" << playerInfo->getPlayerId() << "]: failed with err=" << ErrorHelp::getErrorName(err));
//    }
//    else
//    {
//        Collections::AttributeMap map;
//        map["ModMask"] = "1";
//        setPlayerAttributes(playerInfo, resp.getGameId(), playerInfo->getPlayerId(), map);
//        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "createOrJoinGame[" << playerInfo->getPlayerId() << "]: created game of " << req.getGameCreationData().getMaxPlayerCapacity() << " players with game id: " <<
//                       resp.getGameId() << " and Persist id:" << req.getPersistedGameId() );
//    }
//    return err;
//}

BlazeRpcError createCoopPartyGame(StressPlayerInfo* playerInfo, CreateGameResponse& resp)
{
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[createOrJoinGame]: In create Game");
	BlazeRpcError err = ERR_OK;
	GameManager::CreateGameRequest req;
	//   UserGroupId (BTPL)
	req.getPlayerJoinData().setGroupId(playerInfo->getConnGroupId());
	req.setGamePingSiteAlias("");
	req.getPlayerJoinData().setGameEntryType(GAME_ENTRY_TYPE_DIRECT);
	req.getCommonGameData().setGameType(GAME_TYPE_GROUP);
	req.getGameCreationData().setGameModRegister(0);
	req.getGameCreationData().setGameName("");
	//  Game Settings = 31 (0x0000001F)
	req.getGameCreationData().getGameSettings().setOpenToBrowsing();  //  0x1
	req.getGameCreationData().getGameSettings().setOpenToMatchmaking();  //  0x2
	req.getGameCreationData().getGameSettings().setOpenToInvites();  //  0x4
	req.getGameCreationData().getGameSettings().setOpenToJoinByPlayer();  //  0x8
	req.getGameCreationData().getGameSettings().setHostMigratable();  //  0x10
	req.setGameStatusUrl("");
	if (!playerInfo->isConnected())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "[gamemanager::createOrJoinGame::User Disconnected = " << playerInfo->getPlayerId());
		return ERR_SYSTEM;
	}
	//   NetworkAddressList
	NetworkAddress& hostAddress = req.getCommonGameData().getPlayerNetworkAddress();
	hostAddress.switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
	hostAddress.getIpPairAddress()->getExternalAddress().setIp(playerInfo->getConnection()->getAddress().getIp());
	hostAddress.getIpPairAddress()->getExternalAddress().setPort(playerInfo->getConnection()->getAddress().getPort());
	hostAddress.getIpPairAddress()->getExternalAddress().copyInto(hostAddress.getIpPairAddress()->getInternalAddress());

	req.getGameCreationData().getGameSettings().clearIgnoreEntryCriteriaWithInvite();
	req.setServerNotResetable(false);
	req.getGameCreationData().setNetworkTopology(NETWORK_DISABLED);
	//   SlotCapacities
	req.getSlotCapacities().clear();
	req.getSlotCapacities().push_back(8);
	req.getSlotCapacities().push_back(0);
	req.getSlotCapacities().push_back(0);
	req.getSlotCapacities().push_back(0);
	req.setPersistedGameId("");
	req.getGameCreationData().getGameAttribs().insert(eastl::pair<EA::TDF::TdfString, EA::TDF::TdfString>("mode", "OriginParty"));
	req.getGameCreationData().setMaxPlayerCapacity(8);
	req.getGameCreationData().setMinPlayerCapacity(1);
	req.getGameCreationData().setPresenceMode(PRESENCE_MODE_NONE);
	req.getGameCreationData().setQueueCapacity(0);
	PerPlayerJoinData* perPlayerJoinData = req.getPlayerJoinData().getPlayerDataList().pull_back();
	perPlayerJoinData->setIsOptionalPlayer(false);
	perPlayerJoinData->getUser().setAccountId(playerInfo->getPlayerId());
	perPlayerJoinData->getUser().setExternalId(playerInfo->getExternalId());
	perPlayerJoinData->getUser().setBlazeId(playerInfo->getPlayerId());
	perPlayerJoinData->getUser().setName(playerInfo->getPersonaName().c_str());
	//   SlotType
	//req.getPlayerJoinData().setSlotType(SLOT_PUBLIC_PARTICIPANT);
	//  TeamIds
	//  req.getTeamIds().clear();
	//  req.getTeamIds().push_back(ANY_TEAM_ID);
	//req.getPlayerJoinData().setTeamIndex(0);
	req.getGameCreationData().setVoipNetwork(VOIP_DISABLED);
	req.getCommonGameData().setGameProtocolVersionString(StressConfigInst.getGameProtocolVersionString());
	err = playerInfo->getComponentProxy()->mGameManagerProxy->createGame(req, resp);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "createPartyGame: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	else
	{
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "createPartyGame[" << playerInfo->getPlayerId() << "]: created game of " << req.getGameCreationData().getMaxPlayerCapacity() << " players with game id: " <<
			resp.getGameId());
	}
	return err;
}

BlazeRpcError sendMessage(StressPlayerInfo* playerInfo, BlazeId targetId, Blaze::Messaging::MessageAttrMap& attrMap, uint32_t messageType, EA::TDF::ObjectType targetType)
{
	BlazeRpcError err = ERR_SYSTEM;
	Messaging::ClientMessage message;
	Messaging::SendMessageResponse response;
	message.getFlags().setBits(2);
	message.setTargetType(targetType);
	message.getTargetIds().push_back(targetId);
	message.setStatus(0);
	message.setTag(2);
	message.setType(messageType);
	Blaze::Messaging::MessageAttrMap::const_iterator start = attrMap.begin();
	for (; start != attrMap.end(); ++start)
	{
		message.getAttrMap()[start->first] = start->second.c_str();
	}
	//  message.getAttrMap()[2] = notification1;
	char8_t buff[1024];
	BLAZE_TRACE_RPC_LOG(BlazeRpcLog::messaging, " SendMessageRequest for [" << playerInfo->getPlayerId() << "]" << message.print(buff, sizeof(buff)));
	err = playerInfo->getComponentProxy()->mMessagingProxy->sendMessage(message, response);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::messaging, "sendMessage - FAILED for player " << playerInfo->getPlayerId() << " to player " << targetId);
	}
	return err;
}

BlazeRpcError sendMessage(StressPlayerInfo* playerInfo, BlazeId targetId, Blaze::Messaging::MessageAttrMap& attrMap, Blaze::Messaging::MessageTag msgTag, uint32_t messageType, EA::TDF::ObjectType targetType)
{
	BlazeRpcError err = ERR_SYSTEM;
	Messaging::ClientMessage message;
	Messaging::SendMessageResponse response;
	message.getFlags().setBits(2);
	message.setTargetType(targetType);
	message.getTargetIds().push_back(targetId);
	message.setStatus(0);
	message.setTag(msgTag);
	message.setType(messageType);
	Blaze::Messaging::MessageAttrMap::const_iterator start = attrMap.begin();
	for (; start != attrMap.end(); ++start)
	{
		message.getAttrMap()[start->first] = start->second.c_str();
	}
	//  message.getAttrMap()[2] = notification1;
	//char8_t buff[1024];
	//BLAZE_TRACE_RPC_LOG(BlazeRpcLog::messaging, " SendMessageRequest for [" << playerInfo->getPlayerId() << "]" << message.print(buff, sizeof(buff)));
	err = playerInfo->getComponentProxy()->mMessagingProxy->sendMessage(message, response);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::messaging, "sendMessage - FAILED for player " << playerInfo->getPlayerId() << " to player " << targetId);
	}
	return err;
}

BlazeRpcError fetchMessages(StressPlayerInfo* playerInfo, uint32_t flags, Blaze::Messaging::MessageType messageType) {
	BlazeRpcError err = ERR_OK;
	Blaze::Messaging::FetchMessageRequest request;
	Blaze::Messaging::FetchMessageResponse response;
	request.getFlags().setBits(flags);
	//request.getFlags().setMatchType();  // 4
	request.setMessageId(0);
	request.setPageIndex(0);
	request.setPageSize(0);
	request.setStatusMask(0);
	request.setOrderBy(Blaze::Messaging::ORDER_DEFAULT);
	request.setStatus(0);
	BlazeObjectId source = BlazeObjectId(EA::TDF::OBJECT_TYPE_INVALID, 0);
	request.setSource(source);
	request.setType(messageType);
	BlazeObjectId target = BlazeObjectId(EA::TDF::OBJECT_TYPE_INVALID, 0);
	request.setTarget(target);

	err = playerInfo->getComponentProxy()->mMessagingProxy->fetchMessages(request, response);
	// no messages in inbox, consider as no problem
	//if (err == MESSAGING_ERR_MATCH_NOT_FOUND) {
	//    err = ERR_OK;
	//}
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "msgFetchMessage:" << playerInfo->getPlayerId() << " fetchMessages:Failed with error" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getMessages(StressPlayerInfo* playerInfo, Blaze::Messaging::MessageType messageType)
{
	BlazeRpcError err = ERR_OK;
	Blaze::Messaging::FetchMessageRequest request;
	Blaze::Messaging::GetMessagesResponse response;
	request.getFlags().setBits(32);
	// request.getFlags().setMatchTypes();  // 32
	request.setMessageId(0);
	request.setPageIndex(0);
	request.setPageSize(0);
	request.setStatusMask(0);
	request.setOrderBy(Blaze::Messaging::ORDER_DEFAULT);
	request.setType(messageType);

	request.setStatus(0);
	BlazeObjectId source = BlazeObjectId(EA::TDF::OBJECT_TYPE_INVALID, 0);
	request.setSource(source);
	BlazeObjectId target = BlazeObjectId(EA::TDF::OBJECT_TYPE_INVALID, 0);
	request.setTarget(target);

	err = playerInfo->getComponentProxy()->mMessagingProxy->getMessages(request, response);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::util, "GetMessages: playerId " << playerInfo->getPlayerId() << " Failed with error" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError fetchSettingsGroups(StressPlayerInfo* playerInfo) {
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::OSDKSettings::FetchSettingsGroupsRequest osdkSettingsrequest;
	Blaze::OSDKSettings::FetchSettingsGroupsResponse osdkSettingsresponse;
	err = playerInfo->getComponentProxy()->mOsdkSettingsProxy->fetchSettingsGroups(osdkSettingsrequest, osdkSettingsresponse);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "osdksettingsFetchSettingsGroups(): failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError updateMailSettings(StressPlayerInfo* playerInfo) {
	BlazeRpcError err = ERR_SYSTEM;  // ERR_SYSTEM represents General system error.
	//Blaze::Mail::UpdateMailSettingsRequest mailSettingsrequest;
	//Blaze::Mail::MailSettings &mailSettings = mailSettingsrequest.getMailSettings();
	//Blaze::Mail::EmailOptInFlags &emailOptInFlags = mailSettings.getEmailOptInFlags();
	//mailSettings.setEmailFormatPref(Mail::EMAIL_FORMAT_TEXT);
	//emailOptInFlags.setEmailOptinTitleFlags(0);
	//err = playerInfo->getComponentProxy()->mMailProxy->updateMailSettings(mailSettingsrequest);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "updateMailSettings: playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getInvitations(StressPlayerInfo* playerInfo, Clubs::ClubId clubId, Clubs::InvitationsType val) {
	BlazeRpcError err = ERR_SYSTEM;  // ERR_SYSTEM represents General system error.
	Clubs::GetInvitationsRequest request;
	Clubs::GetInvitationsResponse response;
	request.setInvitationsType(val);
	request.setClubId(clubId);
	request.setSortType(Clubs::CLUBS_SORT_TIME_DESC);
	err = playerInfo->getComponentProxy()->mClubsProxy->getInvitations(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "getInvitations: playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getPetitions(StressPlayerInfo* playerInfo, Clubs::ClubId clubId, Clubs::PetitionsType val) {
	BlazeRpcError err = ERR_SYSTEM;
	Clubs::GetPetitionsRequest request;
	Clubs::GetPetitionsResponse response;
	request.setPetitionsType(val);
	request.setClubId(clubId);
	request.setSortType(Clubs::CLUBS_SORT_TIME_DESC);
	err = playerInfo->getComponentProxy()->mClubsProxy->getPetitions(request, response);
	/*char8_t buff[2048];
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager , "getPetitions RPC" << "\n" << request.print(buff, sizeof(buff)));*/
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "getPetitions: playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

//BlazeRpcError subscribeToCensusData(StressPlayerInfo* playerInfo) {
//    BlazeRpcError err = ERR_SYSTEM;  
//    err = playerInfo->getComponentProxy()->mCensusDataProxy->subscribeToCensusData();
//    // Can ignore the following error
//    if (err == CENSUSDATA_ERR_PLAYER_ALREADY_SUBSCRIBED) {
//        err = ERR_OK;
//    }
//    if (err != ERR_OK) {
//        BLAZE_ERR_LOG(BlazeRpcLog::util, "subscribeToCensusData(): failed with error " << ErrorHelp::getErrorName(err));
//    }
//    return err;
//}

BlazeRpcError subscribeToCensusDataUpdates(StressPlayerInfo* playerInfo) {
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::CensusData::SubscribeToCensusDataUpdatesRequest request;
	Blaze::CensusData::SubscribeToCensusDataUpdatesResponse response;
	request.setResubscribe(false);
	err = playerInfo->getComponentProxy()->mCensusDataProxy->subscribeToCensusDataUpdates(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "subscribeToCensusDataUpdates:playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	else
		playerHasSubscribed = true;

	return err;
}

BlazeRpcError unsubscribeFromCensusData(StressPlayerInfo* playerInfo) {
	BlazeRpcError err = ERR_SYSTEM;
	if (playerHasSubscribed == false)
		return ERR_OK;
	//else
	//	playerHasSubscribed = false;
	err = playerInfo->getComponentProxy()->mCensusDataProxy->unsubscribeFromCensusData();
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "unsubscribeFromCensusData:playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	else {
		playerHasSubscribed = false;
	}
	return err;
}

BlazeRpcError getClubsComponentSettings(StressPlayerInfo* playerInfo) {
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::Clubs::ClubsComponentSettings clubsResponse;
	/*if (BLAZE_IS_TRACE_RPC_ENABLED(BlazeRpcLog::clubs)) {
		BLAZE_TRACE_RPC_LOG(BlazeRpcLog::clubs, "-> [Utilityities][" << playerInfo->getIdent() << "]::getClubsComponentSettings" << "\n");
	}*/
	err = playerInfo->getComponentProxy()->mClubsProxy->getClubsComponentSettings(clubsResponse);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "getClubsComponentSettings:playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getClubs(StressPlayerInfo* playerInfo, ClubId clubId) {
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::Clubs::GetClubsRequest request;
	Blaze::Clubs::GetClubsResponse response;

	if (clubId <= 0) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "getClubs: invalid clubid " << clubId);
		return ERR_SYSTEM;
	}
	request.getClubIdList().push_back(clubId);
	request.setIncludeClubTags(true);
	request.setMaxResultCount(0xFFFFFFFF);
	request.setClubsOrder(Blaze::Clubs::CLUBS_NO_ORDER);
	request.setOrderMode(Blaze::Clubs::ASC_ORDER);
	request.setSkipCalcDbRows(false);
	err = playerInfo->getComponentProxy()->mClubsProxy->getClubs(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "getClubs:playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError localizeStrings(StressPlayerInfo* playerInfo) {
	BlazeRpcError err = ERR_OK;
	Util::LocalizeStringsRequest request;
	Util::LocalizeStringsResponse response;
	request.setLocale(1701729619);
	request.getStringIds().push_back("TXT_CONTACT_INFO_TEAM_1");
	request.getStringIds().push_back("TXT_CONTACT_INFO_LEAGUE_13");
	request.getStringIds().push_back("TXT_CONTACT_INFO_FIFA_GAME");
	request.getStringIds().push_back("TeamName_1");
	request.getStringIds().push_back("LeagueName_13");
	request.getStringIds().push_back("FEDERATIONINTERNATIONALEDEFOOTBALLASSOCIATION");
	request.getStringIds().push_back("TXT_CONTACT_INFO_UEFA");
	request.getStringIds().push_back("TXT_UEFA");
	err = playerInfo->getComponentProxy()->mUtilProxy->localizeStrings(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "utilLocalizeStrings:playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError fetchSettings(StressPlayerInfo* playerInfo) {
	BlazeRpcError err = ERR_SYSTEM;  // ERR_SYSTEM represents General system error.
	const Blaze::OSDKSettings::FetchSettingsRequest request;
	Blaze::OSDKSettings::FetchSettingsResponse response;
	err = playerInfo->getComponentProxy()->mOsdkSettingsProxy->fetchSettings(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "fetchSettings:playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getClubMembershipForUsers(StressPlayerInfo* playerInfo, Clubs::GetClubMembershipForUsersResponse& response) {
	BlazeRpcError err = ERR_OK;
	Blaze::Clubs::GetClubMembershipForUsersRequest request;
	request.getBlazeIdList().push_back(playerInfo->getPlayerId());
	//if (BLAZE_IS_TRACE_RPC_ENABLED(BlazeRpcLog::gamemanager)) {
	//	char8_t buf[1024];
	//	BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "getClubsComponentSettings RPC : \n" << "\n" << request.print(buf, sizeof(buf)));
	//}
	err = playerInfo->getComponentProxy()->mClubsProxy->getClubMembershipForUsers(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::clubs, "getClubMembershipForUsers:playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError updateAIPlayerCustomization(StressPlayerInfo* playerInfo, Blaze::Clubs::ClubId clubId)
{
	BlazeRpcError err = ERR_OK;
	Blaze::proclubscustom::updateAIPlayerCustomizationRequest request;
	Blaze::proclubscustom::updateAIPlayerCustomizationResponse response;
	request.setClubId(clubId);
	request.setSlotId(0);
	Blaze::proclubscustom::AIPlayerCustomization &aipl = request.getAIPlayerCustomization();
	aipl.setCommentaryid((uint32_t)rand() % 10);
	aipl.setNationality((uint32_t)rand() % 4);
	aipl.setEyebrowcode((uint32_t)rand() % 10);
	aipl.setFacialhaircolorcode((uint32_t)rand() % 2);
	aipl.setFacialhairtypecode((uint32_t)rand() % 2);
	aipl.setHaircolorcode((uint32_t)rand() % 10);
	aipl.setHairtypecode((uint32_t)rand() % 100);
	char8_t playername[20];
	playername[19] = '\0';
	blaze_snzprintf(playername, sizeof(playername), "eli%" PRId64 "", playerInfo->getPlayerId());
	aipl.setKitname(playername);
	aipl.setAccessories_color_1((uint32_t)rand() % 2);
	aipl.setAccessories_color_2((uint32_t)rand() % 10);
	aipl.setAccessories_color_3((uint32_t)rand() % 10);
	aipl.setYear((uint32_t)rand() % 10000);
	aipl.setMonth((uint32_t)rand() % 10);
	aipl.setDay((uint32_t)rand() % 31);
	Blaze::proclubscustom::AIPlayerCustomization::MorphRegionList &morphlist = aipl.getMorphs();
	Blaze::proclubscustom::MorphRegion* morphregions[8];
	for (int i = 0; i < 8; i++)
	{
		morphregions[i] = BLAZE_NEW Blaze::proclubscustom::MorphRegion();
		morphregions[i]->setTweak_a(0);
		morphregions[i]->setTweak_b(0);
		morphregions[i]->setTweak_c(0);
		morphregions[i]->setTweak_d(0);
		morphregions[i]->setTweak_e(0);
		morphregions[i]->setRegion_type(i);
		morphregions[i]->setPreset(Blaze::Random::getRandomNumber(20));
		morphlist.push_back(morphregions[i]);
	}
	aipl.setFirstname(playername);
	aipl.setLastname("Dyer");
	aipl.setCommonname("");
	aipl.setShoecolorcode1((uint32_t)rand() % 10);
	aipl.setShoecolorcode2((uint32_t)rand() % 10);
	aipl.setShoedesigncode((uint32_t)rand() % 2);
	aipl.setGkglovetypecode((uint32_t)rand() % 100);
	aipl.setShoetypecode((uint32_t)rand() % 10);
	aipl.setSocklengthcode((uint32_t)rand() % 2);
	aipl.setAnimfreekickstartposcode((uint32_t)rand() % 2);
	aipl.setKitnumber((uint32_t)rand() % 10);
	aipl.setAnimpenaltiesstartposcode((uint32_t)rand() % 10);
	aipl.setRunstylecode((uint32_t)rand() % 10);
	aipl.setRunningcode1((uint32_t)rand() % 10);
	aipl.setFinishingcode1((uint32_t)rand() % 10);
	aipl.setJerseyfit((uint32_t)rand() % 10);
	aipl.setHasseasonaljersey((uint32_t)rand() % 10);
	aipl.setSkintonecode((uint32_t)rand() % 10);
	aipl.setJerseystylecode((uint32_t)rand() % 2);
	aipl.setJerseysleevelengthcode((uint32_t)rand() % 2);
	aipl.setAccessories_code_0((uint32_t)rand() % 10);
	aipl.setAccessories_color_0((uint32_t)rand() % 10);
	aipl.setAccessories_code_1((uint32_t)rand() % 10);
	aipl.setAccessories_code_2((uint32_t)rand() % 10);
	aipl.setAccessories_code_3((uint32_t)rand() % 10);
	aipl.setPreferredfoot((uint32_t)rand() % 2);
	aipl.setPriority((uint32_t)rand() % 2);
	aipl.setPosition((uint32_t)rand() % 10);
	aipl.setShortstyle((uint32_t)rand() % 10);
	aipl.setSlotId((uint32_t)rand() % 10);
	aipl.setEyecolorcode((uint32_t)rand() % 10);

	//BLAZE_INFO_LOG(BlazeRpcLog::util,request);
	err = playerInfo->getComponentProxy()->mProClubsCustomSlaveProxy->updateAIPlayerCustomization(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "updateAIPlayerCustomization(): failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError fetchCustomTactics(StressPlayerInfo* playerInfo, Blaze::Clubs::ClubId clubId)
{
	BlazeRpcError err = ERR_OK;
	Blaze::proclubscustom::getCustomTacticsRequest request;
	Blaze::proclubscustom::getCustomTacticsResponse response;
	request.setClubId(clubId);
	request.setSlotId(0);
	err = playerInfo->getComponentProxy()->mProClubsCustomSlaveProxy->fetchCustomTactics(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "fetchCustomTactics(): failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError updateCustomTactics(StressPlayerInfo* playerInfo, Blaze::Clubs::ClubId clubId, Blaze::Clubs::ClubId tacticSlotId)
{
	BlazeRpcError err = ERR_OK;
	Blaze::proclubscustom::updateCustomTacticsRequest request;
	Blaze::proclubscustom::updateCustomTacticsResponse response;
	request.setClubId(clubId);
	request.getCustomTactics().setClubId(clubId);
	request.getCustomTactics().setTacticSlotId(7);
	//request.getCustomTactics().setTacticsDataBlob();
	err = playerInfo->getComponentProxy()->mProClubsCustomSlaveProxy->updateCustomTactics(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "updateCustomTactics(): failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError fetchAIPlayerCustomization(StressPlayerInfo* playerInfo, Blaze::Clubs::ClubId clubId)
{
	BlazeRpcError err = ERR_OK;
	Blaze::proclubscustom::getAIPlayerCustomizationRequest request;
	Blaze::proclubscustom::getAIPlayerCustomizationResponse response;
	request.setClubId(clubId);
	err = playerInfo->getComponentProxy()->mProClubsCustomSlaveProxy->fetchAIPlayerCustomization(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "fetchAIPlayerCustomization(): failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getAvatarData(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_OK;
	Blaze::proclubscustom::getAvatarDataRequest request;
	Blaze::proclubscustom::getAvatarDataResponse response;
	request.setBlazeId(playerInfo->getPlayerId());
	err = playerInfo->getComponentProxy()->mProClubsCustomSlaveProxy->getAvatarData(request, response);

	//char8_t buf[5240];
	//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[ClubsPlayer::getAvatarData Response] : \n" << response.print(buf, sizeof(buf)));

	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "getAvatarData(): failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError updateAvatarName(StressPlayerInfo* playerInfo, const char8_t* firstName, const char8_t* lastName)
{
	BlazeRpcError err = ERR_OK;
	Blaze::proclubscustom::updateAvatarNameRequest request;
	//Blaze::proclubscustom::updateAvatarNameResponse response;
	request.setFirstname(firstName);
	request.setLastname(lastName);
	err = playerInfo->getComponentProxy()->mProClubsCustomSlaveProxy->updateAvatarName(request);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "updateAvatarName(): failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError fileOnlineReport(StressPlayerInfo* playerInfo, Blaze::Clubs::ClubId clubId, BlazeId targetPlayerID, BlazeId targetNucleusID, Blaze::ContentReporter::ReportContentType contentType, Blaze::ContentReporter::OnlineReportType reportType, const char8_t* subject, const char8_t* contentText)
{
	BlazeRpcError err = ERR_OK;
	Blaze::ContentReporter::FileOnlineReportRequest request;
	Blaze::ContentReporter::FileReportResponse response;

	request.setClubId(clubId);
	request.getCommonAbuseReport().setContentType(contentType);	//Blaze::ContentReporter::ReportContentType::NAME
	request.getCommonAbuseReport().setSubject(subject);
	request.getCommonAbuseReport().setTargetBlazeId(targetPlayerID);
	request.getCommonAbuseReport().setTargetNucleusId(targetNucleusID);
	request.setContentText(contentText);
	request.setOnlineReportType(reportType);		//Blaze::ContentReporter::OnlineReportType::AVATAR_NAME

	//char8_t buf[5024];
	//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "fileOnlineReport RPC : \n" << request.print(buf, sizeof(buf))); 

	err = playerInfo->getComponentProxy()->mContentReporterSlaveProxy->fileOnlineReport(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "fileOnlineReport(): failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError acceptPetition(StressPlayerInfo* playerInfo, uint32_t petitionId) {
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::Clubs::ProcessPetitionRequest request;
	//Blaze::Clubs::GetClubsResponse response;

	request.setPetitionId(petitionId);

	err = playerInfo->getComponentProxy()->mClubsProxy->acceptPetition(request);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "acceptPetition:playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError initializeUserActivityTrackerRequest(StressPlayerInfo* playerInfo, bool8_t flag)
{
	BlazeRpcError err = ERR_OK;
	Blaze::UserActivityTracker::InitializeUserActivityTrackerRequest request;
	Blaze::UserActivityTracker::InitializeUserActivityTrackerResponse response;
	request.setOptIn(flag);
	/*char8_t buf[1024];
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "PlayerHealth InitializeUserActivityTrackerRequest RPC : \n" << request.print(buf, sizeof(buf))); */
	err = playerInfo->getComponentProxy()->mUserActivityTrackerProxy->initializeUserAcivityTracker(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::useractivitytracker, "initializeUserActivityTrackerRequest:playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError incrementMatchPlayed(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_OK;
	Blaze::UserActivityTracker::IncrementMatchPlayedResponse response;
	err = playerInfo->getComponentProxy()->mUserActivityTrackerProxy->incrementMatchPlayed(response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::useractivitytracker, "IncrementMatchPlayed:playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError submitUserActivity(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_OK;
	Blaze::UserActivityTracker::SubmitUserActivityRequest request;
	Blaze::UserActivityTracker::SubmitUserActivityResponse response;
	Blaze::UserActivityTracker::UserActivityList& userActivityList = request.getUserActivityList();// BLAZE_NEW Blaze::UserActivityTracker::UserActivityList();
	Blaze::UserActivityTracker::UserActivity *userActivity1 = BLAZE_NEW Blaze::UserActivityTracker::UserActivity;
	userActivity1->setActivityPeriodType(Blaze::UserActivityTracker::ActivityPeriodType::TYPE_CURRENT);
	userActivity1->setNumMatches((uint16_t)Random::getRandomNumber(50)+1);
	userActivity1->setTimeDuration((uint16_t)Random::getRandomNumber(1000) + 2000);

	Blaze::UserActivityTracker::UserActivity *userActivity2 = BLAZE_NEW Blaze::UserActivityTracker::UserActivity;
	userActivity2->setActivityPeriodType(Blaze::UserActivityTracker::ActivityPeriodType::TYPE_PREVIOUS);
	userActivity2->setNumMatches((uint16_t)Random::getRandomNumber(50) + 1);
	userActivity2->setTimeDuration((uint16_t)Random::getRandomNumber(1000) + 4000);

	userActivityList.push_back(userActivity1);
	userActivityList.push_back(userActivity2);

	int64_t timeStamp = TimeValue::getTimeOfDay().getSec();

	request.setTimeStamp(timeStamp);

	/*char8_t buf[1024];
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "PlayerHealth SubmitUserActivity RPC : \n" << request.print(buf, sizeof(buf)));*/

	err = playerInfo->getComponentProxy()->mUserActivityTrackerProxy->submitUserActivity(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::useractivitytracker, "SubmitUserActivityRequest:playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError updateEadpStats(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_OK;
	FifaStats::UpdateStatsRequest request;
	FifaStats::UpdateStatsResponse response;

	request.setContextOverrideType(Blaze::FifaStats::ContextOverrideType::CONTEXT_OVERRIDE_TYPE_PLAYER_HEALTH);
	//request.setCategoryId("PlayerHealthStats");
	request.setCategoryId("PlayTimeStats");
	request.setServiceName("");
	int64_t timeStamp = TimeValue::getTimeOfDay().getSec();
	eastl::string updateId = eastl::string().sprintf("%" PRIu64 """-""%" PRIu64 "", playerInfo->getPlayerId(), timeStamp).c_str();
	request.setUpdateId(updateId.c_str());

	Blaze::FifaStats::Entity *entity = request.getEntities().pull_back();
	//Blaze::FifaStats::StatsUpdateList &statUpdateList = entity->getStatsUpdates();

	char8_t playerIdString[MAX_GAMENAME_LEN] = { '\0' };
	blaze_snzprintf(playerIdString, sizeof(playerIdString), "%" PRId64, playerInfo->getPlayerId());
	entity->setEntityId(playerIdString);

	if (!StressConfigInst.isManualStatsEnabled())
	{
		EadpStatsMap mEadpStatsMap = ((Blaze::Stress::PlayerModule*)playerInfo->getOwner())->eadpStatsMap;
		const int mapSize = mEadpStatsMap.size();

		#define mapConst mapSize;
		if (mapSize > 0)
		{
			//Blaze::FifaStats::StatsUpdate *statsUpdate = BLAZE_NEW Blaze::FifaStats::StatsUpdate[mapSize];
			/*const char8_t* mStatsName = "";
			EadpStatsMap mEadpStatsMapValue;
			EadpStatsMap::iterator itr;
			for (itr = mEadpStatsMap.begin(); itr != mEadpStatsMap.end(); ++itr)
			{
				mEadpStatsMapValue.insert(eastl::pair<eastl::string, int>(itr->first, itr->second));
			}*/

			Blaze::FifaStats::StatsUpdate *statsUpdate[4];
			EadpStatsMap::iterator statsMapStart = mEadpStatsMap.begin();
			EadpStatsMap::iterator statsMapEnd = mEadpStatsMap.end();
			int statsUpdateIndex = 0;
			//for (itr = mEadpStatsMapValue.begin(); itr != mEadpStatsMapValue.end(); ++itr)
			//{
			//	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[updateEadpStats::setValue]:Map = " << (itr->first).c_str() << " : " << itr->second);
			//	//statsUpdate[statsUpdateIndex]->setStatsName((itr->first).c_str());
			//	mStatsName = ((itr->first).c_str());
			//}

			for (int i = 0; i < 4; i++)
			{
				statsUpdate[i] = entity->getStatsUpdates().pull_back();
				statsUpdate[i]->setOperator(Blaze::FifaStats::StatsOperator::OPERATOR_REPLACE);
				statsUpdate[i]->setStatsFValue((float)0.00000000);
				statsUpdate[i]->setType(Blaze::FifaStats::StatsType::TYPE_INT);
			}

			for (; statsMapStart != statsMapEnd; ++statsMapStart)
			{
				statsUpdate[statsUpdateIndex]->setStatsName((statsMapStart->first).c_str());
				statsUpdate[statsUpdateIndex++]->setStatsIValue(statsMapStart->second);
			}

		}
	}
	
	//Manually updating the stats
	if (StressConfigInst.isManualStatsEnabled())
	{
		Blaze::FifaStats::StatsUpdate *statsUpdate[4];
		for (int i = 0; i < 4; i++)
		{
			statsUpdate[i] = entity->getStatsUpdates().pull_back();
			statsUpdate[i]->setOperator(Blaze::FifaStats::StatsOperator::OPERATOR_REPLACE);
			statsUpdate[i]->setStatsFValue((float)0.00000000);
			statsUpdate[i]->setType(Blaze::FifaStats::StatsType::TYPE_INT);
		}

		statsUpdate[0]->setStatsIValue((int64_t)Random::getRandomNumber(500) + 1);
		statsUpdate[0]->setStatsName("li_m_c_a_m");

		statsUpdate[1]->setStatsIValue((int64_t)Random::getRandomNumber(50) + 1);
		statsUpdate[1]->setStatsName("li_pa");

		statsUpdate[2]->setStatsIValue((int64_t)Random::getRandomNumber(1000) + 60000);
		statsUpdate[2]->setStatsName("li_po");

		statsUpdate[3]->setStatsIValue(1);
		statsUpdate[3]->setStatsName("opt_in");
	}

	//char8_t buf[2024];
	//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "PlayerHealth UpdateEadpStats RPC : \n" << request.print(buf, sizeof(buf)));

	err = playerInfo->getComponentProxy()->mUserActivityTrackerProxy->updateEadpStats(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::useractivitytracker, "UpdateEadpStatsRequest:playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError submitOfflineStats(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_OK;
	getPeriodIds(playerInfo);
	err = submitUserActivity(playerInfo);
	return err;
}


BlazeRpcError listEntitlements(StressPlayerInfo* playerInfo) {
	BlazeRpcError err;
	Blaze::Authentication::ListEntitlementsRequest entitlementsRequest;
	Blaze::Authentication::Entitlements entitlementsResponse;
	entitlementsRequest.setBlazeId(0);
	entitlementsRequest.setPageNo(0);
	entitlementsRequest.setPageSize(0);
	entitlementsRequest.getEntitlementSearchFlag().setBits(3);
	//Check for PS4, Xone, PC values
	if (StressConfigInst.getPlatform() == PLATFORM_PS4)
	{
		entitlementsRequest.getGroupNameList().push_back("FIFA22PS4BoxContent");
		entitlementsRequest.getGroupNameList().push_back("FIFA21PS4");
		//entitlementsRequest.getGroupNameList().push_back("FIFA19PS4");
	}
	else
	{
		entitlementsRequest.getGroupNameList().push_back("FIFA22XONEBoxContent");
		entitlementsRequest.getGroupNameList().push_back("FIFA21X360");
		entitlementsRequest.getGroupNameList().push_back("FIFA21XONE");
	}
	err = playerInfo->getComponentProxy()->mAuthenticationProxy->listEntitlements(entitlementsRequest, entitlementsResponse);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::clubs, "listEntitlements:playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError lookupUserGeoIPData(StressPlayerInfo* playerInfo) {
	BlazeRpcError err = ERR_OK;

	UserIdentification reqGeoIpDataLookup;
	reqGeoIpDataLookup.setAccountId(0);
	reqGeoIpDataLookup.setAccountLocale(0);
	reqGeoIpDataLookup.setAccountCountry(0);
	//reqGeoIpDataLookup.getExternalBlob();
	reqGeoIpDataLookup.setExternalId(0);
	reqGeoIpDataLookup.setBlazeId(playerInfo->getPlayerId());
	reqGeoIpDataLookup.setName("");
	reqGeoIpDataLookup.setPersonaNamespace("");
	reqGeoIpDataLookup.setOriginPersonaId(0);
	reqGeoIpDataLookup.setPidId(0);
	GeoLocationData rspGeoIpDataLookup;

	char8_t buf[1024];
	BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, " lookupUserGeoIPData RPC : \n" << reqGeoIpDataLookup.print(buf, sizeof(buf)));

	err = playerInfo->getComponentProxy()->mUserSessionsProxy->lookupUserGeoIPData(reqGeoIpDataLookup, rspGeoIpDataLookup);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::clubs, "lookupUserGeoIPData:playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getEventsURL(StressPlayerInfo* playerInfo) {
	BlazeRpcError err = ERR_OK;
	Blaze::SponsoredEvents::URLResponse urlResponse;
	err = playerInfo->getComponentProxy()->mSponsoredEventsProxy->getEventsURL(urlResponse);
	// const char8_t* stringURL = urlResponse.getURL();
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "getEventsURL:playerId " << playerInfo->getPlayerId() << " failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}

bool stringToUInt64(const char8_t *text, uint64_t &value)
{
	const int32_t DECIMAL_RADIX = 10;
	const char8_t *ends = text;
	value = (uint64_t)strtoull(text, const_cast<char8_t **>(&ends), DECIMAL_RADIX);
	return (ends != NULL && ends != text);
}

BlazeRpcError userSessionsOverrideUserGeoIPData(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_SYSTEM;
	GeoLocationData req;
	// set lat/longitude
	req.setBlazeId(playerInfo->getPlayerId());
	req.setLatitude((float)playerInfo->getLocation().latitude);
	req.setLongitude((float)playerInfo->getLocation().longitude);
	req.setIsOverridden(true);
	req.setOptIn(true);
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Utility][" << playerInfo->getPlayerId() << "] overriding user's GeoIP data with " << playerInfo->getLocation().latitude << " " << playerInfo->getLocation().longitude);
	// override User's GeoIP Data by calling UserSessions::overrideUserGeoIpData()
	err = playerInfo->getComponentProxy()->mUserSessionsProxy->overrideUserGeoIPData(req);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "overrideUserGeoIpData: playerId " << playerInfo->getPlayerId() << " failed with err= " << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError invokeUdatePrimaryExternalSessionForUser(StressPlayerInfo* playerInfo, GameId gameId, eastl::string externalSessionName, eastl::string externalSessionTemplateName, GameId changedGameId, UpdatePrimaryExternalSessionReason change, GameType gameType,
	GameManager::PlayerState playerState)
{
	BlazeRpcError err = ERR_OK;
	UpdateExternalSessionPresenceForUserRequest updatePrimaryExternalSessionForUserRequest;
	UpdateExternalSessionPresenceForUserResponse updatePrimaryExternalSessionForUserResponse;
	UpdatePrimaryExternalSessionForUserErrorInfo error;

	updatePrimaryExternalSessionForUserRequest.setChangedGameId(changedGameId);
	updatePrimaryExternalSessionForUserRequest.setChange(change);

	if (change != GAME_LEAVE)
	{
		Blaze::GameManager::GameActivity* gameActivity = updatePrimaryExternalSessionForUserRequest.getCurrentGames().pull_back();
		gameActivity->getSessionIdentification().getPs4().setNpSessionId("");
		gameActivity->getSessionIdentification().getXone().setCorrelationId("");
		gameActivity->getSessionIdentification().getXone().setSessionName(externalSessionName.c_str());
		gameActivity->getSessionIdentification().getXone().setTemplateName(externalSessionTemplateName.c_str());
		gameActivity->setGameId(gameId);
		gameActivity->setGameType(gameType);
		gameActivity->setPlayerState(playerState);
		gameActivity->setPresenceMode(Blaze::PresenceMode::PRESENCE_MODE_NONE);
		gameActivity->setExternalSessionName(externalSessionName.c_str());
		gameActivity->setExternalSessionTemplateName(externalSessionTemplateName.c_str());
		updatePrimaryExternalSessionForUserRequest.setOldPrimaryGameId(0);
	}
	else
	{
		updatePrimaryExternalSessionForUserRequest.setOldPrimaryGameId(gameId);
	}

	updatePrimaryExternalSessionForUserRequest.getUserIdentification().setAccountId(playerInfo->getPlayerId());
	updatePrimaryExternalSessionForUserRequest.getUserIdentification().setAccountLocale(1684358213);
	updatePrimaryExternalSessionForUserRequest.getUserIdentification().setAccountCountry(17477);
	updatePrimaryExternalSessionForUserRequest.getUserIdentification().setBlazeId(playerInfo->getPlayerId());
	updatePrimaryExternalSessionForUserRequest.getUserIdentification().setExternalId(playerInfo->getExternalId());
	updatePrimaryExternalSessionForUserRequest.getUserIdentification().setName("ungezielt_Zent"); // Need to check other log
	Platform platType = StressConfigInst.getPlatform();
	if (platType == PLATFORM_XONE)
	{
		updatePrimaryExternalSessionForUserRequest.getUserIdentification().setPersonaNamespace("xbox");
	}
	else if (platType == PLATFORM_PS4)
	{
		updatePrimaryExternalSessionForUserRequest.getUserIdentification().setPersonaNamespace("ps3");
	}
	updatePrimaryExternalSessionForUserRequest.getUserIdentification().setOriginPersonaId(0);
	updatePrimaryExternalSessionForUserRequest.getUserIdentification().setPidId(0);
	updatePrimaryExternalSessionForUserRequest.getUserIdentification().getPlatformInfo().getEaIds().setOriginPersonaName("tuilkk");
	updatePrimaryExternalSessionForUserRequest.getUserIdentification().getPlatformInfo().getEaIds().setNucleusAccountId(playerInfo->getPlayerId());
	updatePrimaryExternalSessionForUserRequest.getUserIdentification().getPlatformInfo().getEaIds().setOriginPersonaId(playerInfo->getExternalId());
	updatePrimaryExternalSessionForUserRequest.getUserIdentification().getPlatformInfo().getExternalIds().setPsnAccountId(0);
	updatePrimaryExternalSessionForUserRequest.getUserIdentification().getPlatformInfo().getExternalIds().setSteamAccountId(0);
	updatePrimaryExternalSessionForUserRequest.getUserIdentification().getPlatformInfo().getExternalIds().setSwitchId("");
	updatePrimaryExternalSessionForUserRequest.getUserIdentification().getPlatformInfo().getExternalIds().setXblAccountId(0);

	if (platType == PLATFORM_XONE)
	{
		updatePrimaryExternalSessionForUserRequest.getUserIdentification().getPlatformInfo().setClientPlatform(Blaze::ClientPlatformType::xone);
	}
	else if (platType == PLATFORM_PS4)
	{
		updatePrimaryExternalSessionForUserRequest.getUserIdentification().getPlatformInfo().setClientPlatform(Blaze::ClientPlatformType::ps4);
	}
	
	//char8_t buf[20240];
	//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "updatePrimaryExternalSessionForUser RPC :\n" << updatePrimaryExternalSessionForUserRequest.print(buf, sizeof(buf)));
	err = playerInfo->getComponentProxy()->mGameManagerProxy->updatePrimaryExternalSessionForUser(updatePrimaryExternalSessionForUserRequest, updatePrimaryExternalSessionForUserResponse, error);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "updatePrimaryExternalSessionForUser: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError updatePrimaryExternalSessionForUserRequest(StressPlayerInfo* playerInfo, GameId gameId, GameId changedGameId, UpdatePrimaryExternalSessionReason change, GameType gameType,
	GameManager::PlayerState playerState, Blaze::PresenceMode presenceMode, GameId oldGameId, bool8_t flag, GameType gameleaveType)
{
	BlazeRpcError err = ERR_OK;
	UpdateExternalSessionPresenceForUserRequest req;
	UpdateExternalSessionPresenceForUserResponse res;
	UpdateExternalSessionPresenceForUserErrorInfo error;

	req.setChangedGameId(0);
	req.setChange(change);

	req.getChangedGame().setGameId(gameId);
	req.getChangedGame().setGameType(gameType);
	req.getChangedGame().setPlayerState(playerState);
	req.getChangedGame().getPlayer().setPlayerState(playerState);
	req.getChangedGame().getPlayer().setJoinedGameTimestamp(TimeValue::getTimeOfDay().getSec());
	req.getChangedGame().setJoinedGameTimestamp(TimeValue::getTimeOfDay().getSec());
	req.getChangedGame().setPresenceMode(presenceMode);

	/*if (gameType == GAME_TYPE_GAMESESSION && change == UPDATE_PRESENCE_REASON_GAME_JOIN)
	{

	}*/
	if (change != UPDATE_PRESENCE_REASON_GAME_LEAVE)
	{
		Blaze::GameManager::GameActivity* gameActivity = BLAZE_NEW Blaze::GameManager::GameActivity(); //req.getCurrentGames().pull_back();
		gameActivity->getSessionIdentification().getPs4().setNpSessionId("");
		gameActivity->getSessionIdentification().getXone().setCorrelationId("73d336d6-1e06-0cf0-81a0-9652fc081890");
		gameActivity->getSessionIdentification().getXone().setSessionName("OTP");
		gameActivity->setGameId(gameId);
		gameActivity->setGameType(gameType);
		gameActivity->setPlayerState(playerState);
		gameActivity->getPlayer().setPlayerState(playerState);
		gameActivity->getPlayer().setJoinedGameTimestamp(TimeValue::getTimeOfDay().getSec());
		gameActivity->setJoinedGameTimestamp(TimeValue::getTimeOfDay().getSec());
		gameActivity->setPresenceMode(presenceMode);
		gameActivity->setExternalSessionName("");
		gameActivity->setExternalSessionTemplateName("");
		req.getCurrentGames().push_back(gameActivity);

		if(flag && gameType == GAME_TYPE_GAMESESSION)
		{
			Blaze::GameManager::GameActivity* gameActivity2 = BLAZE_NEW Blaze::GameManager::GameActivity();
			gameActivity2->getSessionIdentification().getPs4().setNpSessionId("");
			gameActivity2->getSessionIdentification().getXone().setCorrelationId("73d336d6-1e06-0cf0-81a0-9652fc081890");
			gameActivity2->getSessionIdentification().getXone().setSessionName("test_fifa-2020-xone_4_1_00000000000000424974");
			gameActivity2->getSessionIdentification().getXone().setSessionName("OTP");
			gameActivity2->setGameId(oldGameId);
			gameActivity2->setGameType(GAME_TYPE_GROUP);
			gameActivity2->setPlayerState(ACTIVE_CONNECTED);
			gameActivity2->getPlayer().setPlayerState(ACTIVE_CONNECTED);
			gameActivity2->setPresenceMode(PRESENCE_MODE_PRIVATE);
			gameActivity2->setExternalSessionName("");
			gameActivity2->setExternalSessionTemplateName("");
			req.getCurrentGames().push_back(gameActivity2);
		}
		req.setOldPrimaryGameId(oldGameId);
	}
	else
	{
		if (gameType == GAME_TYPE_GAMESESSION)
		{
			Blaze::GameManager::GameActivity* gameActivity2 = BLAZE_NEW Blaze::GameManager::GameActivity();
			gameActivity2->getSessionIdentification().getPs4().setNpSessionId("");
			gameActivity2->getSessionIdentification().getXone().setCorrelationId("73d336d6-1e06-0cf0-81a0-9652fc081890");
			gameActivity2->getSessionIdentification().getXone().setSessionName("test_fifa-2020-xone_4_1_00000000000000424974");
			gameActivity2->getSessionIdentification().getXone().setSessionName("OTP");
			gameActivity2->setGameId(gameId);
			gameActivity2->setGameType(gameleaveType);
			gameActivity2->setPlayerState(ACTIVE_CONNECTED);
			gameActivity2->getPlayer().setPlayerState(ACTIVE_CONNECTED);
			gameActivity2->setPresenceMode(PRESENCE_MODE_PRIVATE);
			gameActivity2->setExternalSessionName("");
			gameActivity2->setExternalSessionTemplateName("");
			req.getCurrentGames().push_back(gameActivity2);
		}
		req.setOldPrimaryGameId(oldGameId);
	}

	req.getUserIdentification().setAccountId(0);
	req.getUserIdentification().setAccountLocale(1701729619);
	req.getUserIdentification().setAccountCountry(21843);
	req.getUserIdentification().setBlazeId(playerInfo->getPlayerId());
	req.getUserIdentification().setExternalId(playerInfo->getExternalId());
	req.getUserIdentification().setName("2 Dev 137989629"); // Need to check other log
	Platform platType = StressConfigInst.getPlatform();
	if (platType == PLATFORM_XONE)
	{
		req.getUserIdentification().setPersonaNamespace("xbox");
		req.getUserIdentification().getPlatformInfo().setClientPlatform(Blaze::ClientPlatformType::xone);	

	}
	else if (platType == PLATFORM_PS4)
	{
		req.getUserIdentification().setPersonaNamespace("ps3");
		req.getUserIdentification().getPlatformInfo().setClientPlatform(Blaze::ClientPlatformType::ps4);	

	}
	req.getUserIdentification().setOriginPersonaId(0);
	req.getUserIdentification().setPidId(0);
	//req.getUserIdentification().getPlatformInfo().setClientPlatform(Blaze::ClientPlatformType::INVALID);	// Added as per logs
	//char8_t buf[20240];
	//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "UpdatePrimaryExternalSessionForUserRequest RPC :\n" << req.print(buf, sizeof(buf)));
	err = playerInfo->getComponentProxy()->mGameManagerProxy->updatePrimaryExternalSessionForUser(req, res, error);

	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "UpdatePrimaryExternalSessionForUserRequest: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}
BlazeRpcError setGameSettingsMask(StressPlayerInfo* playerInfo, GameId gameId, uint32_t settings, uint32_t mask)
{
	BlazeRpcError err = ERR_OK;
	Blaze::GameManager::SetGameSettingsRequest request;
	request.setGameId(gameId);
	request.getGameSettings().setBits(settings);
	if (mask)
	{
		request.getGameSettingsMask().setBits(mask);
	}
	err = playerInfo->getComponentProxy()->mGameManagerProxy->setGameSettings(request);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "setGameSettingsMask failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;

}

BlazeRpcError userSessionsLookupUserGeoIPData(StressPlayerInfo* playerInfo, BlazeId playerId, LocationInfo& location)
{
	BlazeRpcError err = ERR_SYSTEM;
	UserIdentification reqGeoIpDataLookup;
	reqGeoIpDataLookup.setAccountId(0);
	reqGeoIpDataLookup.setAccountLocale(0);
	reqGeoIpDataLookup.setAccountCountry(0);
	reqGeoIpDataLookup.setExternalId(0);
	reqGeoIpDataLookup.setName("");
	reqGeoIpDataLookup.setPersonaNamespace("");
	reqGeoIpDataLookup.setBlazeId(playerId);
	GeoLocationData rspGeoIpDataLookup;
	err = playerInfo->getComponentProxy()->mUserSessionsProxy->lookupUserGeoIPData(reqGeoIpDataLookup, rspGeoIpDataLookup);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "lookupUserGeoIPData:playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	else
	{
		location.latitude = rspGeoIpDataLookup.getLatitude();
		location.longitude = rspGeoIpDataLookup.getLongitude();
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "lookupUserGeoIPData: PlayerID= " << playerInfo->getPlayerId() << " longtitude= " << location.longitude << " latitude= " << location.latitude);
	}
	return err;
}

double haversine(double lat1, double lon1, double lat2, double lon2)
{

	double dLat = (lat2 - lat1)*((float)M_PI / 180.0f);
	double dLon = (lon2 - lon1)*((float)M_PI / 180.0f);
	lat1 = (lat1)*((float)M_PI / 180.0f);
	lat2 = (lat2)*((float)M_PI / 180.0f);
	double a = pow(sin(dLat / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(dLon / 2), 2);
	double b = 2 * asin(sqrt(a));
	double result = EARTH_RADIUS * b;
	return result;
}

double calculateLatency(double distnace)
{
	// Latency Claculation (distnace inMiles)/(speed of light in miles)
	int pingVariation = Blaze::Random::getRandomNumber(120);
	return (((distnace) / ((0.4)*(186282))) * 1000) + pingVariation;
}

BlazeRpcError getAllCoopIds(StressPlayerInfo* mPlayerInfo, CoopSeason::GetAllCoopIdsResponse& response)
{
	BlazeRpcError err = ERR_OK;
	Blaze::CoopSeason::GetAllCoopIdsRequest getAllCoopIdsRequest;
	getAllCoopIdsRequest.setTargetBlazeId(mPlayerInfo->getPlayerId());

	err = mPlayerInfo->getComponentProxy()->mCoopSeasonProxy->getAllCoopIds(getAllCoopIdsRequest, response);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "util: getAllCoopIds failed with err=" << ErrorHelp::getErrorName(err) << " " << mPlayerInfo->getPlayerId());
	}
	return err;
}
BlazeRpcError getCupInfo(StressPlayerInfo* playerInfo, FifaCups::MemberType type, FifaCups::CupInfo& response, ClubId clubId) {
	BlazeRpcError err = ERR_OK;
	FifaCups::GetCupInfoRequest request;

	if (clubId != 0)
	{
		request.setMemberId(clubId);
	}
	else
	{
		if (type == FifaCups::MemberType::FIFACUPS_MEMBERTYPE_COOP)
		{
			request.setMemberId(0);
		}
		else
		{
			request.setMemberId(playerInfo->getPlayerId());
		}
	}


	request.setMemberType(type);
	err = playerInfo->getComponentProxy()->mFIFACupsProxy->getCupInfo(request, response);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "getCupInfo: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	else
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::fifacups, "getCupInfo:Playerid= " << playerInfo->getPlayerId() << " Cup Id= " << response.getCupId());
		BLAZE_TRACE_LOG(BlazeRpcLog::fifacups, "getCupInfo:Playerid= " << playerInfo->getPlayerId() << " TournamentStatus= " << TournamentStatusToString(response.getTournamentStatus()));
	}
	return err;
}

BlazeRpcError getMyTournamentId(StressPlayerInfo* playerInfo, OSDKTournaments::TournamentModes mode, OSDKTournaments::GetMyTournamentIdResponse& response, int64_t memberIdOverride) {
	BlazeRpcError err = ERR_SYSTEM;
	OSDKTournaments::GetMyTournamentIdRequest request;
	request.setMode(mode);
	request.setMemberIdOverride(memberIdOverride);
	//char8_t buf[20240];
	//BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "getMyTournamentId RPC : \n" << request.print(buf, sizeof(buf)));
	err = playerInfo->getComponentProxy()->mTournamentProxy->getMyTournamentId(request, response);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "getMyTournamentId failed with err=" << ErrorHelp::getErrorName(err));
	}
	else
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::osdktournaments, "getMyTournamentId: Playerid= " << playerInfo->getPlayerId() << " TournamentId= " << response.getTournamentId());
	}
	return err;
}

BlazeRpcError getTournaments(StressPlayerInfo* playerInfo, OSDKTournaments::TournamentModes mode, OSDKTournaments::GetTournamentsResponse& response) {
	BlazeRpcError err = ERR_OK;
	Blaze::OSDKTournaments::GetTournamentsRequest request;
	request.setMode(mode);
	err = playerInfo->getComponentProxy()->mTournamentProxy->getTournaments(request, response);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "getTournaments failed with err=" << ErrorHelp::getErrorName(err));
	}
	else
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::osdktournaments, "getTournaments: Playerid= " << playerInfo->getPlayerId() << " TournamentId= " << response.getTournamentList().at(0)->getTournamentId());
	}
	return err;
}

BlazeRpcError getMyTournamentDetails(StressPlayerInfo* playerInfo, OSDKTournaments::TournamentModes mode, OSDKTournaments::TournamentId tournamentId, OSDKTournaments::GetMyTournamentDetailsResponse& response, int64_t memberIdOverride) {
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::OSDKTournaments::GetMyTournamentDetailsRequest request;
	request.setMode(mode);
	request.setMemberIdOverride(memberIdOverride);
	request.setTournamentId(tournamentId);
	//char8_t buf[20240];
	//BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "GetStatsByGroupAsync RPC : \n" << request.print(buf, sizeof(buf)));
	err = playerInfo->getComponentProxy()->mTournamentProxy->getMyTournamentDetails(request, response);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "getMyTournamentDetails: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	else
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::osdktournaments, "getMyTournamentDetails: Playerid= " << playerInfo->getPlayerId() << " level= " << response.getLevel());
	}
	return err;
}

BlazeRpcError setActiveCupId(StressPlayerInfo* playerInfo, uint32_t currentCupId, FifaCups::MemberType type) {
	BlazeRpcError err = ERR_SYSTEM;
	FifaCups::SetActiveCupIdRequest request;
	request.setCupId(currentCupId);
	request.setMemberId(playerInfo->getPlayerId());
	request.setMemberType(type);
	err = playerInfo->getComponentProxy()->mFIFACupsProxy->setActiveCupId(request);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "setActiveCupId: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError fetchObjectiveConfig(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_OK;
	VProGrowthUnlocks::FetchObjectiveConfigResponse response;
	err = playerInfo->getComponentProxy()->mVProGrowthUnlocksProxy->fetchObjectiveConfig(response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::vprogrowthunlocks, "fetchObjectiveConfig: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError fetchObjectiveProgress(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::VProGrowthUnlocks::FetchObjectiveProgressRequest request;
	VProGrowthUnlocks::FetchObjectiveProgressResponse response;
	request.getUserIdList().push_back(playerInfo->getPlayerId());
	err = playerInfo->getComponentProxy()->mVProGrowthUnlocksProxy->fetchObjectiveProgress(request, response);
	if (err != ERR_OK) {
		//BLAZE_ERR_LOG(BlazeRpcLog::vprogrowthunlocks, "fetchLoadOuts: playerId " << playerInfo->GetPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError fetchPlayerGrowthConfig(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::VProGrowthUnlocks::FetchPlayerGrowthConfigResponse response;
	err = playerInfo->getComponentProxy()->mVProGrowthUnlocksProxy->fetchPlayerGrowthConfig(response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::vprogrowthunlocks, "fetchPlayerGrowthConfig: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError fetchSkillTreeConfig(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::VProGrowthUnlocks::FetchSkillTreeConfigResponse response;
	err = playerInfo->getComponentProxy()->mVProGrowthUnlocksProxy->fetchSkillTreeConfig(response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::vprogrowthunlocks, "fetchSkillTreeConfig: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError fetchPerksConfig(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::VProGrowthUnlocks::FetchSkillTreeConfigResponse response;
	err = playerInfo->getComponentProxy()->mVProGrowthUnlocksProxy->fetchSkillTreeConfig(response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::vprogrowthunlocks, "fetchPerksConfig: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError fetchLoadOuts(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::VProGrowthUnlocks::FetchLoadOutsRequest request;
	VProGrowthUnlocks::FetchLoadOutsResponse response;
	request.getUserIdList().push_back(playerInfo->getPlayerId());
	err = playerInfo->getComponentProxy()->mVProGrowthUnlocksProxy->fetchLoadOuts(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::vprogrowthunlocks, "fetchLoadOuts: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError resetLoadOuts(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::VProGrowthUnlocks::ResetLoadOutsRequest request;
	VProGrowthUnlocks::ResetLoadOutsResponse response;
	request.setLoadOutId(-1);
	request.setUserId(playerInfo->getPlayerId());
	err = playerInfo->getComponentProxy()->mVProGrowthUnlocksProxy->resetLoadOuts(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::vprogrowthunlocks, "resetLoadOuts: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}

//BlazeRpcError coopSeasonSetCoopIdData(StressPlayerInfo* playerInfo, BlazeId BlazeID1, BlazeId BlazeID2, int64_t &coopId) {
//    BlazeRpcError err = ERR_SYSTEM;   // ERR_SYSTEM represents General system error.
//    Blaze::CoopSeason::SetCoopIdDataRequest CPIDDatarequest;
//    Blaze::CoopSeason::SetCoopIdDataResponse CPIDDataresponse;
//    CPIDDatarequest.setMemberOneBlazeId(BlazeID1);
//    CPIDDatarequest.setMemberTwoBlazeId(BlazeID2);
//
//    err = playerInfo->getComponentProxy()->mCoopSeasonProxy->setCoopIdData(CPIDDatarequest, CPIDDataresponse);
//
//    if (err != ERR_OK) {
//		BLAZE_ERR_LOG(BlazeRpcLog::vprogrowthunlocks, "resetLoadOuts: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
//		return err;
//    } 
//	char8_t buf[20240];
//	BLAZE_TRACE_RPC_LOG(BlazeRpcLog::gamemanager, "GetStatsByGroupAsync RPC : \n" << CPIDDataresponse.print(buf, sizeof(buf)));
//    coopId = CPIDDataresponse.getCoopId();
//    return err;
//}

BlazeRpcError getFilteredLeaderboard(StressPlayerInfo* playerInfo, FilteredLeaderboardStatsRequest::EntityIdList& idList, const char8_t* name, uint32_t limit)
{
	BlazeRpcError err = ERR_OK;
	//BlazeObjectId messageObject;
	Blaze::Stats::FilteredLeaderboardStatsRequest request;
	Blaze::Stats::LeaderboardStatValues response;
	//idList.copyInto(request.getListOfIds());
	request.setEnforceCutoffStatValue(false);
	request.setIncludeStatlessEntities(true);
	request.setBoardId(0);
	request.getListOfIds().push_back(playerInfo->getPlayerId());
	request.setBoardName(name);
	request.setLimit(limit);
	request.setPeriodOffset(0);
	request.setPeriodId(0);
	request.setTime(0);
	//request.setUserSetId(messageObject);
	//char8_t buf[20240];
	//BLAZE_TRACE_RPC_LOG(BlazeRpcLog::stats, "getFilteredLeaderboard RPC : \n" << request.print(buf, sizeof(buf)));
	err = playerInfo->getComponentProxy()->mStatsProxy->getFilteredLeaderboard(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::stats, "getFilteredLeaderboard: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
		return err;
	}

	return err;
}

BlazeRpcError getFilteredLeaderboard(StressPlayerInfo* playerInfo, FilteredLeaderboardStatsRequest::EntityIdList& idList, const char8_t* name, uint32_t limit, uint32_t offset)
{
	BlazeRpcError err = ERR_OK;
	//BlazeObjectId messageObject;
	Blaze::Stats::FilteredLeaderboardStatsRequest request;
	Blaze::Stats::LeaderboardStatValues response;
	//idList.copyInto(request.getListOfIds());
	request.setEnforceCutoffStatValue(false);
	request.setIncludeStatlessEntities(true);
	request.setBoardId(0);
	request.getListOfIds().push_back(playerInfo->getPlayerId());
	request.setBoardName(name);
	request.setLimit(limit);
	request.setPeriodOffset(offset);
	request.setPeriodId(0);
	request.setTime(0);
	//request.setUserSetId(messageObject);
	//char8_t buf[20240];
	//BLAZE_TRACE_RPC_LOG(BlazeRpcLog::stats, "getFilteredLeaderboard RPC : \n" << request.print(buf, sizeof(buf)));
	err = playerInfo->getComponentProxy()->mStatsProxy->getFilteredLeaderboard(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::stats, "getFilteredLeaderboard: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
		return err;
	}

	return err;
}

BlazeRpcError actionGetAwards(StressPlayerInfo* playerInfo, ClubId clubId)
{
	GetClubAwardsRequest request;
	GetClubAwardsResponse response;

	request.setClubId(clubId);

	BlazeRpcError err = playerInfo->getComponentProxy()->mClubsProxy->getClubAwards(request, response);

	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::clubs, "actionGetAwards: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}

	return err;
}

BlazeRpcError getStatsByGroup(StressPlayerInfo* playerInfo, const char8_t* name)
{
	BlazeRpcError err = ERR_OK;
	Blaze::Stats::GetStatsByGroupRequest  request;
	Blaze::Stats::GetStatsResponse response;
	request.setGroupName(name);
	request.getEntityIds().push_back(playerInfo->getPlayerId());
	request.setPeriodId(Stats::STAT_PERIOD_ALL_TIME);

	err = playerInfo->getComponentProxy()->mStatsProxy->getStatsByGroup(request, response);

	return err;
}

BlazeRpcError updateLoadOutsPeripherals(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_SYSTEM;
	int32_t count = 0;
	Blaze::VProGrowthUnlocks::UpdateLoadOutsPeripheralsRequest request;
	VProGrowthUnlocks::UpdateLoadOutsPeripheralsResponse response;
	request.setUserId(playerInfo->getPlayerId());
	int32_t randomLoadOutListCount = Random::getRandomNumber(5) + 1;
	// update request with ,multiple random values for load outs 3 to 5 push backs
	VProGrowthUnlocks::VProLoadOutList& myLoadOutList = request.getLoadOutList();
	myLoadOutList.clear();
	while (count != randomLoadOutListCount)
	{
		VProGrowthUnlocks::VProLoadOutPtr loadOut = myLoadOutList.pull_back();
		loadOut->setLoadOutName("");
		loadOut->setSPConsumed(0);
		loadOut->setSPGranted(0);
		loadOut->setUnlocks1(0);
		loadOut->setUnlocks2(0);
		loadOut->setVProFoot(1);
		loadOut->setVProPosition(25);
		loadOut->setVProWeight(75);
		loadOut->setLoadOutId(count);
		count++;
	}

	err = playerInfo->getComponentProxy()->mVProGrowthUnlocksProxy->updateLoadOutsPeripherals(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::vprogrowthunlocks, "updateLoadOutsPeripherals: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getGameReportViewInfo(StressPlayerInfo* playerInfo, const char8_t* viewName) {
	BlazeRpcError err = ERR_OK;
	Blaze::GameReporting::GetGameReportViewInfo request;
	Blaze::GameReporting::GameReportViewInfo response;
	request.setName(viewName);
	err = playerInfo->getComponentProxy()->mGameReportingProxy->getGameReportViewInfo(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "getGameReportViewInfo:playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getGameReportView(StressPlayerInfo* playerInfo, const char8_t* viewName, const char8_t* queryString) {
	BlazeRpcError err = ERR_OK;
	Blaze::GameReporting::GetGameReportViewRequest request;
	Blaze::GameReporting::GetGameReportViewResponse response;
	request.setName(viewName);
	request.setMaxRows(0);
	if (queryString == NULL)
	{
		char8_t playerIdString[32];
		blaze_snzprintf(playerIdString, sizeof(playerIdString), "%" PRId64 "", playerInfo->getPlayerId());
		request.getQueryVarValues().push_back(playerIdString);
	}
	else
	{
		request.getQueryVarValues().push_back(queryString);
	}
	err = playerInfo->getComponentProxy()->mGameReportingProxy->getGameReportView(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "getGameReportView: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError getGameReportView(StressPlayerInfo* playerInfo, const char8_t* viewName, Blaze::GameReporting::QueryVarValuesList& values) {
	BlazeRpcError err = ERR_OK;
	Blaze::GameReporting::GetGameReportViewRequest request;
	Blaze::GameReporting::GetGameReportViewResponse response;
	request.setName(viewName);
	request.setMaxRows(1);
	values.copyInto(request.getQueryVarValues());

	err = playerInfo->getComponentProxy()->mGameReportingProxy->getGameReportView(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "getGameReportView: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError joinTournament(StressPlayerInfo* playerInfo, OSDKTournaments::TournamentId tournamentId, EntityId memberIdOverride) {
	BlazeRpcError err = ERR_SYSTEM;
	Blaze::OSDKTournaments::JoinTournamentRequest request;
	Blaze::OSDKTournaments::JoinTournamentResponse response;
	request.setTeam(-1);
	request.setTournAttribute("0");
	if (memberIdOverride == 0)
	{
		request.setMemberIdOverride(playerInfo->getPlayerId());
	}
	else
	{
		request.setMemberIdOverride(memberIdOverride);
	}
	request.setTournamentId(tournamentId);
	err = playerInfo->getComponentProxy()->mTournamentProxy->joinTournament(request, response);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "joinTournament: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	else
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::osdktournaments, "joinTournament:Playerid= " << playerInfo->getPlayerId() << " joined the tournament= " << tournamentId);
	}
	return err;
}

BlazeRpcError resetCupStatus(StressPlayerInfo* playerInfo, FifaCups::MemberType type) {
	BlazeRpcError err = ERR_SYSTEM;
	FifaCups::ResetCupStatusRequest request;
	request.setMemberId(playerInfo->getPlayerId());
	request.setMemberType(type);
	err = playerInfo->getComponentProxy()->mFIFACupsProxy->resetCupStatus(request);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "getMyTournamentDetails: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
		return err;
	}
	BLAZE_TRACE_LOG(BlazeRpcLog::osdktournaments, "resetCupStatus successful:Playerid= " << playerInfo->getPlayerId());
	return err;
}

BlazeRpcError osdkResetTournament(StressPlayerInfo* playerInfo, Blaze::OSDKTournaments::TournamentId tournamentId, Blaze::OSDKTournaments::TournamentModes tourMode) {
	BlazeRpcError err = ERR_OK;
	//  BLAZE_INFO(BlazeRpcLog::osdktournaments, "ClubMatch %d: Player Id=%" PRId64 " ResetTournament", getIdent(),mUserBlazeId);
	Blaze::OSDKTournaments::ResetTournamentRequest request;
	request.setTournamentId(tournamentId);
	request.setMode(tourMode);

	err = playerInfo->getComponentProxy()->mTournamentProxy->resetTournament(request);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "osdkResetTournament: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
		return err;
	}
	return err;
}

BlazeRpcError registerUser(StressPlayerInfo* playerInfo, int eventId) {
	BlazeRpcError err = ERR_SYSTEM;
	SponsoredEvents::RegisterUserRequest request;
	SponsoredEvents::RegisterUserResponse response;
	request.setCountry("enUS");
	request.setEventData("1");
	request.setEventID(eventId);
	if (PLATFORM_PS4 == StressConfigInst.getPlatform())
	{
		request.setGamePlatform("PS4"); //check ps4 log
	}
	else if (PLATFORM_XONE == StressConfigInst.getPlatform())
	{
		request.setGamePlatform("XONE");
	}
	request.setGameTitle("FIFA");
	request.setUserID(playerInfo->getPlayerId());
	err = playerInfo->getComponentProxy()->mSponsoredEventsProxy->registerUser(request, response);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::sponsoredevents, "registeruser:playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError setMetaData2(StressPlayerInfo* playerInfo, ClubId clubId, const char8_t* personaName, const char8_t* metadataString)
{
	BlazeRpcError err = ERR_OK;
	Clubs::SetMetadataRequest req;
	req.setClubId(clubId);
	req.setMetaData(personaName);
	req.getMetaDataUnion().setMetadataString(metadataString);
	err = playerInfo->getComponentProxy()->mClubsProxy->setMetadata2(req);

	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::clubs, "setMetaData2: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
	}
	return err;
}

BlazeRpcError updateMemberOnlineStatus(StressPlayerInfo* playerInfo, ClubId clubId, Blaze::Clubs::MemberOnlineStatus memberStatus) {
	BlazeRpcError err = ERR_OK;
	Blaze::Clubs::UpdateMemberOnlineStatusRequest request;
	request.setClubId(clubId);
	request.setNewStatus(memberStatus);
	err = playerInfo->getComponentProxy()->mClubsProxy->updateMemberOnlineStatus(request);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::clubs, "updateMemberOnlineStatus: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
		return err;
	}
	return err;
}

BlazeRpcError destroyGameList(StressPlayerInfo* playerInfo, Blaze::GameManager::GameBrowserListId  gameListId) {
	BlazeRpcError err = ERR_OK;
	Blaze::GameManager::DestroyGameListRequest request;
	// list id
	request.setListId(gameListId);
	err = playerInfo->getComponentProxy()->mGameManagerProxy->destroyGameList(request);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::clubs, "destroyGameList: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
		return err;
	}
	return err;
}

//Clubs::ClubId getMyClubID(StressPlayerInfo *mPlayerInfo)
//{
//	Clubs::ClubsSlave* mClubsSlave;
//	Clubs::FindClubsRequest request;
//	Clubs::FindClubsResponse response;
//
//	request.setMaxResultCount(10);
//	request.setAnyTeamId(true);
//	request.setAnyDomain(true);
//	request.setName("");
//
//	// Current user id
//	BlazeId currentUserId = mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId();
//	request.getMemberFilterList().push_back(currentUserId);
//	request.setSkipMetadata(1);
//	
//	BlazeRpcError result = mClubsSlave->findClubs(request, response);
//
//	Clubs::ClubId id = 0;
//	if (result == Blaze::ERR_OK)
//	{
//		Clubs::ClubList& clubList = response.getClubList();
//		if (clubList.size() > 0)
//		{
//			Clubs::Club* club = clubList.front();
//			id = club->getClubId();
//		}
//	}
//	return id;
//}
BlazeRpcError getMembers(StressPlayerInfo* playerInfo, ClubId clubId, int32_t maxCount, Blaze::Clubs::GetMembersResponse& response)
{
	BlazeRpcError err = ERR_OK;
	Blaze::Clubs::GetMembersRequest request;
	if (clubId <= 0) {
		BLAZE_ERR_LOG(BlazeRpcLog::util, "clubsGetMembers: invalid clubid " << clubId);
		return ERR_SYSTEM;
	}
	request.setClubId(clubId);
	request.setPersonaNamePattern("");
	request.setMemberType(Blaze::Clubs::ALL_MEMBERS);
	request.setMaxResultCount(maxCount);
	request.setOffset(0);
	request.setOrderMode(OrderMode::ASC_ORDER);
	request.setOrderType(MemberOrder::MEMBER_NO_ORDER);
	request.setSkipCalcDbRows(false);

	err = playerInfo->getComponentProxy()->mClubsProxy->getMembers(request, response);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::clubs, "getMembers: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
		return err;
	}

	return err;
}

BlazeRpcError findClubsAsync(StressPlayerInfo* playerInfo, ClubId clubId)
{
	BlazeRpcError err = ERR_OK;
	Blaze::Clubs::FindClubsRequest request;
	Blaze::Clubs::FindClubsAsyncResponse response;
	request.getClubFilterList().push_back(clubId);
	request.setLanguage("en");
	request.setNonUniqueName("");
	request.setMaxResultCount(80);
	request.setAnyDomain(true);
	request.setAnyTeamId(true);
	request.getTagList().push_back("PosMID");
	request.setIncludeClubTags(true);
	request.setClubsOrder(Blaze::Clubs::CLUBS_NO_ORDER);
	request.setTagSearchOperation(Blaze::Clubs::CLUB_TAG_SEARCH_MATCH_ALL);
	// #TODO: Check what need to retrieve from the onSync()

	err = playerInfo->getComponentProxy()->mClubsProxy->findClubsAsync(request, response);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::clubs, "findClubsAsync: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
		return err;
	}
	return err;
}

BlazeRpcError findClubs(StressPlayerInfo* playerInfo)
{
	BlazeRpcError err = ERR_OK;
	Blaze::Clubs::FindClubsRequest request;
	Blaze::Clubs::FindClubsResponse response;
	request.getAcceptanceFlags().setBits(0);
	request.getAcceptanceMask().setBits(0);
	request.setAnyDomain(true);
	request.setAnyTeamId(true);
	request.setIncludeClubTags(true);
	request.setTagSearchOperation(Blaze::Clubs::CLUB_TAG_SEARCH_IGNORE);
	request.setName("");
	request.setClubsOrder(Blaze::Clubs::CLUBS_NO_ORDER);
	request.setRegion(0);
	request.setClubDomainId(0);
	request.setJoinAcceptance(CLUB_ACCEPTS_UNSPECIFIED);
	request.setLanguage("");
	request.setLastGameTimeOffset(0);
	request.setMaxMemberCount(0);
	request.setMinMemberCount(0);
	request.setMaxResultCount(80);
	request.setNonUniqueName("");
	request.setOrderMode(ASC_ORDER);
	request.setOffset(0);
	request.setPetitionAcceptance(CLUB_ACCEPTS_UNSPECIFIED);
	request.setPasswordOption(CLUB_PASSWORD_IGNORE);
	request.setSeasonLevel(0);
	request.setSkipCalcDbRows(false);
	request.setSkipMetadata(0);
	request.setTeamId(0);
	request.getMemberFilterList().push_back(playerInfo->getPlayerId());
	// #TODO: Check what need to retrieve from the onSync()

	err = playerInfo->getComponentProxy()->mClubsProxy->findClubs(request, response);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::clubs, "findClubs: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
		return err;
	}
	return err;
}

BlazeRpcError checkUserRegistration(StressPlayerInfo* playerInfo, int eventId, Blaze::SponsoredEvents::CheckUserRegistrationResponse &response) {
	BlazeRpcError err = ERR_SYSTEM;
	SponsoredEvents::CheckUserRegistrationRequest request;
	request.setEventID(eventId);
	request.setUserID(playerInfo->getPlayerId());
	err = playerInfo->getComponentProxy()->mSponsoredEventsProxy->checkUserRegistration(request, response);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::sponsoredevents, "checkUserRegistration: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
		return err;
	}
	return err;
}

BlazeRpcError setStatus(StressPlayerInfo* playerInfo, int teamid) {
	BlazeRpcError err = ERR_SYSTEM;  // ERR_SYSTEM represents General system error.
	Blaze::ThirdPartyOptIn::ThirdPartyOptInSetStatusRequest* request;
	request = BLAZE_NEW Blaze::ThirdPartyOptIn::ThirdPartyOptInSetStatusRequest();
	request->setLeagueId(13);
	request->setTeamId(teamid);
	err = playerInfo->getComponentProxy()->mThirdPartyOptInProxy->setStatus(*request);
	if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::thirdpartyoptin, "FIFAUtil::thirdPartySetStatus(): failed with error " << ErrorHelp::getErrorName(err));
	}
	return err;
}
//BlazeRpcError updateVProSPForUser(StressPlayerInfo* playerInfo) {
//    BlazeRpcError err = ERR_SYSTEM;
//    Blaze::VProSPManagement::UpdateVProSPForUserRequest request;
//	Blaze::VProSPManagement::UpdateVProSPForUserResponse response;
//	request.setUserId(playerInfo->getPlayerId());
//    err = playerInfo->getComponentProxy()->mVProSPManagementComponentProxy->updateVProSPForUser(request, response);
//	if (err != ERR_OK) {
//		BLAZE_ERR_LOG(BlazeRpcLog::sponsoredevents, "updateVProSPForUser: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
//		return err;
//    } 
//	return err;
//}

//BlazeRpcError getVProSPForUser(StressPlayerInfo* playerInfo) {
//    BlazeRpcError err = ERR_SYSTEM;
//    Blaze::VProSPManagement::GetVProSPForUserRequest request;
//	Blaze::VProSPManagement::GetVProSPForUserResponse response;
//	request.setUserId(playerInfo->getPlayerId());
//    err = playerInfo->getComponentProxy()->mVProSPManagementComponentProxy->getVProSPForUser(request, response);
//	if (err != ERR_OK) {
//		BLAZE_ERR_LOG(BlazeRpcLog::sponsoredevents, "getVProSPForUser: playerId " << playerInfo->getPlayerId() << " failed with err=" << ErrorHelp::getErrorName(err));
//		return err;
//    } 
//	return err;
//}

}  // namespace Stress
}  // namespace Blaze
