//  *************************************************************************************************
//
//   File:    utility.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************


#ifndef UTILITY_H
#define UTILITY_H

#include "framework/tdf/userdefines.h"
#include "associationlists/tdf/associationlists.h"
#include "./reference.h"

#include "messaging/tdf/messagingtypes.h"

#include "authentication/tdf/authentication.h"
#include "osdksettings/tdf/osdksettingstypes.h"
#include "clubs/tdf/clubs.h"
#include "stats/tdf/stats.h"
//#include "util/tdf/utiltypes_luatypes.h"
//#include "authentication/tdf/accountdefines_luatypes.h"
//#include "mail/tdf/mail.h"
#include "sponsoredevents/tdf/sponsoredeventstypes.h"

#include "clubs/tdf/clubs.h"
#include "gamemanager/tdf/gamemanager_server.h"
#include "stressmodule.h"
#include "osdktournaments/tdf/osdktournamentstypes_custom.h"
//#include "customcomponent/vprospmanagement/rpc/vprospmanagementslave.h"
#include "fifacups/tdf/fifacupstypes.h"
#include "gamereporting/tdf/gamehistory.h"

namespace Blaze {
namespace Stress {

//   Default values
#define FRIEND_LIST_SIZE_AVG 2

//  UserSessions
BlazeRpcError   updateNetworkInfo(StressPlayerInfo* playerInfo, Util::NatType natType = Util::NAT_TYPE_NONE, uint32_t updateNetowkOption = 0);
BlazeRpcError   getTelemetryServer(StressPlayerInfo* PlayerInfo);
BlazeRpcError   updateHardwareFlags(StressPlayerInfo* PlayerInfo);
BlazeRpcError   setUserInfoAttribute(StressPlayerInfo* PlayerInfo);
BlazeRpcError   lookupUserByExternalId(StressPlayerInfo* PlayerInfo, ExternalId ExtId);
BlazeRpcError   lookupUser(StressPlayerInfo* PlayerInfo, const char8_t* strPersonaName);
BlazeRpcError   lookupUsersByPersonaNames(StressPlayerInfo* PlayerInfo, const char8_t* strPersonaName);
BlazeRpcError   lookupRandomUsersByPersonaNames(StressPlayerInfo* PlayerInfo);
BlazeRpcError   lookupRandomUserByBlazeId(StressPlayerInfo* playerInfo, UserIdentificationList* uidListOut  = NULL);
BlazeRpcError   createRandomUserList(StressPlayerInfo* PlayerInfo, UserIdentificationList *uIdentificationList, uint32_t maxUsers);
BlazeRpcError   lookupRandomUsersByExternalId(StressPlayerInfo* playerInfo);
BlazeRpcError   lookupUsersByBlazeId(StressPlayerInfo* playerInfo, const Blaze::BlazeId &playerId);
BlazeRpcError   lookupUserByBlazeId(StressPlayerInfo* playerInfo, Blaze::BlazeId playerId);
BlazeRpcError   lookupUsers(StressPlayerInfo* playerInfo, const char8_t* strPersonaName);
BlazeRpcError   updateExtendedDataAttribute(StressPlayerInfo* playerInfo, const Blaze::BlazeId &playerId);
BlazeRpcError   resetUserGeoIPData(StressPlayerInfo* PlayerInfo);
BlazeRpcError	lookupUsersByPersonaNames(StressPlayerInfo* PlayerInfo, Blaze::PersonaNameList& personaNameList);

//   AuthenticationComponent
BlazeRpcError   listUserEntitlements2(StressPlayerInfo* PlayerInfo, BlazeId playerId);
BlazeRpcError   listUserEntitlements2(StressPlayerInfo* PlayerInfo, BlazeId playerId, Blaze::Authentication::Entitlements& listUserEntitlements2Resp);
BlazeRpcError   grantEntitlement2(StressPlayerInfo* PlayerInfo);
BlazeRpcError   getAuthToken(StressPlayerInfo* PlayerInfo);

//   UtilComponent
BlazeRpcError   userSettingsLoad(StressPlayerInfo* PlayerInfo, const char8_t* key = "AchievementCache");
BlazeRpcError   userSettingsSave(StressPlayerInfo* PlayerInfo, const char8_t* key = "fav", const char8_t* data = NULL);
BlazeRpcError   userSettingsLoadAll(StressPlayerInfo* playerInfo);
BlazeRpcError   fetchClientConfig(StressPlayerInfo* PlayerInfo, const char8_t* configSection, FetchConfigResponse& response);
BlazeRpcError   localizeStrings(StressPlayerInfo* PlayerInfo,  uint32_t locale, StringList& playList);
BlazeRpcError   setClientState(StressPlayerInfo* PlayerInfo, ClientState::Mode mode, ClientState::Status status = ClientState::Status::STATUS_NORMAL);
BlazeRpcError	setConnectionState(StressPlayerInfo * playerInfo);
BlazeRpcError	getEntityRank(StressPlayerInfo* playerInfo);
BlazeRpcError   setUserOptions(StressPlayerInfo* playerInfo, Util::TelemetryOpt optPar = Util::TelemetryOpt::TELEMETRY_OPT_IN);
BlazeRpcError   listEntitlements(StressPlayerInfo* playerInfo);
BlazeRpcError   getAccount(StressPlayerInfo* PlayerInfo);
BlazeRpcError	filterForProfanity(StressPlayerInfo* PlayerInfo, Blaze::Util::FilterUserTextResponse *request);

//   StatsComponent
BlazeRpcError   getStatsByGroupAsync(StressPlayerInfo* PlayerInfo, const char8_t* statGroupName = "playerStatGroup_MP");
BlazeRpcError   getStatsByGroupAsync(StressPlayerInfo* PlayerInfo, Blaze::BlazeIdList friendList, const char8_t* statGroupName = "playerStatGroup_MP");
BlazeRpcError	getStatsByGroupAsync(StressPlayerInfo* PlayerInfo, const char8_t* groupName, Blaze::Stats::GetStatsByGroupRequest::EntityIdList& entityList,const char8_t* ksumName,EntityId ksumValue, int32_t periodType = 0);
BlazeRpcError   lookupUserByExternalId(StressPlayerInfo* PlayerInfo, ExternalId ExtId);
BlazeRpcError   getStatsByGroupAsync(StressPlayerInfo* PlayerInfo, PlayerId playerId, uint32_t viewId, const char8_t* statGroupName = "player_mpdefault");
BlazeRpcError	getStatsByGroupAsync(StressPlayerInfo* PlayerInfo, Blaze::BlazeIdList friendList, const char8_t* statGroupName, ClubId clubId);
BlazeRpcError   getLeaderboard(StressPlayerInfo* PlayerInfo, const char8_t* bName);
BlazeRpcError	getLeaderboard(StressPlayerInfo* playerInfo, int32_t iboardId, int32_t iCount, const char8_t* bName);
BlazeRpcError   getCenteredLeaderboard(StressPlayerInfo* PlayerInfo, int32_t iboardId, int32_t iCount, BlazeId UserBlazeId, const char8_t* strboardName);
BlazeRpcError   getStatGroupList(StressPlayerInfo* playerInfo);
BlazeRpcError	getLeaderboardGroup(StressPlayerInfo* playerInfo, int32_t boardId, const char8_t* boardName);
BlazeRpcError   getStatGroup(StressPlayerInfo* playerInfo, const char8_t* name);
BlazeRpcError   getKeyScopesMap(StressPlayerInfo* PlayerInfo);
BlazeRpcError   getPeriodIds(StressPlayerInfo* PlayerInfo);
BlazeRpcError   getLeaderboardTreeAsync(StressPlayerInfo* playerInfo, const char8_t* name);
BlazeRpcError	getFilteredLeaderboard(StressPlayerInfo* playerInfo, FilteredLeaderboardStatsRequest::EntityIdList& idList, const char8_t* name,uint32_t limit);
BlazeRpcError	getFilteredLeaderboard(StressPlayerInfo* playerInfo, FilteredLeaderboardStatsRequest::EntityIdList& idList, const char8_t* name, uint32_t limit, uint32_t offset);
BlazeRpcError	getStatsByGroup(StressPlayerInfo* playerInfo, const char8_t* name);
BlazeRpcError	initializeUserActivityTrackerRequest(StressPlayerInfo* playerInfo, bool8_t flag = true);
BlazeRpcError	submitUserActivity(StressPlayerInfo* playerInfo);
BlazeRpcError	submitOfflineStats(StressPlayerInfo* playerInfo);
BlazeRpcError	updateEadpStats(StressPlayerInfo* playerInfo);
BlazeRpcError	incrementMatchPlayed(StressPlayerInfo* playerInfo);

//  Association lists
BlazeRpcError associationGetLists(StressPlayerInfo* playerInfo);
BlazeRpcError associationSubscribeToLists(StressPlayerInfo* playerInfo, const char8_t* listName = FRIEND_LIST, Association::ListType listType = Association::LIST_TYPE_FRIEND);
BlazeRpcError setUsersToList(StressPlayerInfo* playerInfo);
BlazeRpcError setUsersToList(StressPlayerInfo* playerInfo, const UserIdentificationList& uidList);
BlazeRpcError clearLists(StressPlayerInfo* playerInfo );

//   GameManagerComponent
BlazeRpcError getGameDataByUser(StressPlayerInfo* playerInfo);
BlazeRpcError getGameDataByUser(StressPlayerInfo* playerInfo, BlazeId blazeid, eastl::string persona);
BlazeRpcError getGameDataByUser(StressPlayerInfo* playerInfo, const char8_t* strConfigName, const char8_t* strPersonaName, BlazeId blzId, ExternalId extId);
BlazeRpcError getGameDataByUser(StressPlayerInfo* playerInfo, GameManager::GetGameDataByUserRequest& getGameDataByUserReq, GameManager::GameBrowserDataList& gameBrowserDataList);
BlazeRpcError getGameDataFromId(StressPlayerInfo* playerInfo, const Blaze::GameManager::GameId gameId, const char8_t* listConfigName);
BlazeRpcError getFullGameData(StressPlayerInfo* playerInfo, const Blaze::GameManager::GameId gameId, GameManager::GetFullGameDataResponse* GetFullGameDataResp = NULL);
BlazeRpcError updateGameHostMigrationStatus(StressPlayerInfo* playerInfo, const GameManager::GameId gameId, const GameManager::HostMigrationType migrationType);
BlazeRpcError requestPlatformHost(StressPlayerInfo* playerInfo, const Blaze::GameManager::GameId gameId, PlayerId playerId = 0);
BlazeRpcError getMemberHash(StressPlayerInfo* mPlayerData, const char8_t* DefaultListName);
BlazeRpcError invokeUdatePrimaryExternalSessionForUser(StressPlayerInfo* playerInfo, const Blaze::GameManager::GameId gameId, eastl::string externalSessionName, eastl::string externalSessionTemplateName, GameId changedGameId, UpdatePrimaryExternalSessionReason change, GameType gameType,
        GameManager::PlayerState playerState);
BlazeRpcError updatePrimaryExternalSessionForUserRequest(StressPlayerInfo* playerInfo, const Blaze::GameManager::GameId gameId, GameId changedGameId, UpdatePrimaryExternalSessionReason change, GameType gameType,
	GameManager::PlayerState playerState, Blaze::PresenceMode presenceMode, GameId oldGameId =0, bool8_t flag =true, GameType gameleaveType =GAME_TYPE_GAMESESSION);
BlazeRpcError destroyGameList(StressPlayerInfo* playerInfo, Blaze::GameManager::GameBrowserListId  gameListId);
BlazeRpcError setGameSettingsMask(StressPlayerInfo* playerInfo, GameId gameId, uint32_t settings, uint32_t mask=0);

//  GpsContentController Component
BlazeRpcError filePetitionForServer(StressPlayerInfo* playerInfo, GameManager::GetFullGameDataResponse* GetFullGameDataResp);
BlazeRpcError filePetition(StressPlayerInfo* playerInfo, const char8_t* PetitionDetail, const char8_t* complaintType = NULL, const char8_t* subject = NULL, const char8_t* timeZone = NULL);

//  Messaging Component
BlazeRpcError sendMessage(StressPlayerInfo* playerInfo, BlazeId targetId, Blaze::Messaging::MessageAttrMap& attrMap, uint32_t messageType = 0, EA::TDF::ObjectType targetType = ENTITY_TYPE_USER);
BlazeRpcError sendMessage(StressPlayerInfo* playerInfo, BlazeId targetId, Blaze::Messaging::MessageAttrMap& attrMap, Blaze::Messaging::MessageTag msgTag, uint32_t messageType = 0, EA::TDF::ObjectType targetType = ENTITY_TYPE_USER);
BlazeRpcError fetchMessages(StressPlayerInfo* playerInfo, uint32_t flags = 4, Blaze::Messaging::MessageType messageType = 0);
BlazeRpcError getMessages(StressPlayerInfo* playerInfo,Blaze::Messaging::MessageType messageType);

// Mail Component
BlazeRpcError updateMailSettings(StressPlayerInfo* playerInfo);

// VProGrowthUnlocks  Component
BlazeRpcError fetchObjectiveConfig(StressPlayerInfo* playerInfo);
BlazeRpcError fetchObjectiveProgress(StressPlayerInfo* playerInfo);

//proclubscustom component
BlazeRpcError updateAIPlayerCustomization(StressPlayerInfo* playerInfo, Blaze::Clubs::ClubId clubId);
BlazeRpcError fetchCustomTactics(StressPlayerInfo* playerInfo, Blaze::Clubs::ClubId clubId);
BlazeRpcError updateCustomTactics(StressPlayerInfo* playerInfo, Blaze::Clubs::ClubId clubId, Blaze::Clubs::ClubId tacticSlotId);
BlazeRpcError fetchAIPlayerCustomization(StressPlayerInfo* playerInfo, Blaze::Clubs::ClubId clubId);
BlazeRpcError fetchLoadOuts(StressPlayerInfo* playerInfo);
BlazeRpcError fetchPlayerGrowthConfig(StressPlayerInfo* playerInfo);
BlazeRpcError fetchSkillTreeConfig(StressPlayerInfo* playerInfo);
BlazeRpcError fetchPerksConfig(StressPlayerInfo* playerInfo);
BlazeRpcError resetLoadOuts(StressPlayerInfo* playerInfo);
BlazeRpcError updateLoadOutsPeripherals(StressPlayerInfo* playerInfo);
BlazeRpcError getAvatarData(StressPlayerInfo* playerInfo);
BlazeRpcError updateAvatarName(StressPlayerInfo* playerInfo, const char8_t* firstName, const char8_t* lastName);
BlazeRpcError fileOnlineReport(StressPlayerInfo* playerInfo, Blaze::Clubs::ClubId clubId, BlazeId targetPlayerID, BlazeId targetNucleusID, Blaze::ContentReporter::ReportContentType contentType, Blaze::ContentReporter::OnlineReportType reportType, const char8_t* subject, const char8_t* contentText);

//easfc Component
BlazeRpcError setVProStats(StressPlayerInfo* playerInfo);
BlazeRpcError setSeasonsData(StressPlayerInfo* playerInfo);

//  rsp related functions
void RSPgetConfig(StressPlayerInfo* playerInfo);

BlazeRpcError getAllCoopIds(StressPlayerInfo* mPlayerInfo, CoopSeason::GetAllCoopIdsResponse& response);
BlazeRpcError   coopSeasonSetCoopIdData(StressPlayerInfo* playerInfo, BlazeId BlazeID1, BlazeId BlazeID2, int64_t &coopId);

//  LicenseManager related functions
BlazeRpcError getLicenses(StressPlayerInfo* playerInfo, bool forceClear);

BlazeRpcError	userSessionsOverrideUserGeoIPData(StressPlayerInfo* playerInfo);
BlazeRpcError	userSessionsLookupUserGeoIPData(StressPlayerInfo* playerInfo, BlazeId playerId, LocationInfo& location);
BlazeRpcError   lookupUserGeoIPData(StressPlayerInfo* playerInfo);
double			haversine(double lat1, double lon1, double lat2, double lon2);
double			calculateLatency(double distnace);

bool			stringToUInt64(const char8_t *text, uint64_t &value);
bool			mIsSubscribed;

BlazeRpcError	fetchSettingsGroups(StressPlayerInfo* playerInfo);
//BlazeRpcError	subscribeToCensusData(StressPlayerInfo* playerInfo);
BlazeRpcError	subscribeToCensusDataUpdates(StressPlayerInfo* playerInfo);
BlazeRpcError	unsubscribeFromCensusData(StressPlayerInfo* playerInfo);

BlazeRpcError	localizeStrings(StressPlayerInfo* playerInfo);
BlazeRpcError	fetchSettings(StressPlayerInfo* playerInfo);
BlazeRpcError	getEventsURL(StressPlayerInfo* playerInfo);
//BlazeRpcError	updateVProSPForUser(StressPlayerInfo* playerInfo);
//BlazeRpcError	getVProSPForUser(StressPlayerInfo* playerInfo);

//Clubs Component
BlazeRpcError	getInvitations(StressPlayerInfo* playerInfo, Clubs::ClubId clubId, Clubs::InvitationsType val);
BlazeRpcError	getPetitions(StressPlayerInfo* playerInfo, Clubs::ClubId clubId, Clubs::PetitionsType val) ;
BlazeRpcError	getClubsComponentSettings(StressPlayerInfo* playerInfo);
BlazeRpcError	getClubs(StressPlayerInfo* playerInfo, ClubId clubId);
BlazeRpcError	getClubMembershipForUsers(StressPlayerInfo* playerInfo, Clubs::GetClubMembershipForUsersResponse& response);
BlazeRpcError	updateMemberOnlineStatus(StressPlayerInfo* playerInfo, ClubId clubId, Clubs::MemberOnlineStatus memberStatus);
BlazeRpcError	setMetaData2(StressPlayerInfo* playerInfo,ClubId clubId, const char8_t* personaName , const char8_t* metadataString);
BlazeRpcError	getMembers(StressPlayerInfo* playerInfo,ClubId clubId,int32_t maxCount, Blaze::Clubs::GetMembersResponse& response); 
BlazeRpcError	findClubsAsync(StressPlayerInfo* playerInfo,ClubId clubId);
BlazeRpcError	findClubs(StressPlayerInfo* playerInfo);
BlazeRpcError   actionPostNews(StressPlayerInfo* playerInfo, ClubId mClubId, uint32_t newsCount);
BlazeRpcError   lookupUsersIdentification(StressPlayerInfo* playerInfo);
BlazeRpcError	removeMember(StressPlayerInfo * playerInfo, ClubId clubId);
BlazeRpcError	actionGetAwards(StressPlayerInfo* playerInfo, ClubId clubId);
void			getUnusedRandomIdList(BlazeIdList& idList, uint32_t listSize);
BlazeRpcError	acceptPetition(StressPlayerInfo* playerInfo, uint32_t petitionId);

BlazeRpcError validateString(StressPlayerInfo* playerInfo);

//FifaCups Component
BlazeRpcError	getCupInfo(StressPlayerInfo* playerInfo, Blaze::FifaCups::MemberType type, Blaze::FifaCups::CupInfo& response, ClubId clubId=0);
BlazeRpcError	getMyTournamentId(StressPlayerInfo* playerInfo, OSDKTournaments::TournamentModes mode, OSDKTournaments::GetMyTournamentIdResponse& response, int64_t memberIdOverride=0);
BlazeRpcError	getTournaments(StressPlayerInfo* playerInfo, OSDKTournaments::TournamentModes mode, OSDKTournaments::GetTournamentsResponse& response);
BlazeRpcError	getMyTournamentDetails(StressPlayerInfo* playerInfo, OSDKTournaments::TournamentModes mode, OSDKTournaments::TournamentId tournamentId, OSDKTournaments::GetMyTournamentDetailsResponse& response, int64_t memberIdOverride = 0);
BlazeRpcError	joinTournament(StressPlayerInfo* playerInfo, OSDKTournaments::TournamentId tournamentId, EntityId memberIdOverrid=0);
BlazeRpcError	resetCupStatus(StressPlayerInfo* playerInfo, FifaCups::MemberType type);
BlazeRpcError	setActiveCupId(StressPlayerInfo* playerInfo, uint32_t currentCupId, FifaCups::MemberType type);
BlazeRpcError	getGameReportViewInfo(StressPlayerInfo* playerInfo, const char8_t* viewName);
BlazeRpcError	getGameReportView(StressPlayerInfo* playerInfo, const char8_t* viewName,const char8_t* queryString = NULL);
BlazeRpcError	getGameReportView(StressPlayerInfo* playerInfo, const char8_t* viewName , Blaze::GameReporting::QueryVarValuesList& values);
BlazeRpcError	osdkResetTournament(StressPlayerInfo* playerInfo, Blaze::OSDKTournaments::TournamentId tournamentId, Blaze::OSDKTournaments::TournamentModes tourMode);

//Sponsered Events Component
BlazeRpcError registerUser(StressPlayerInfo* playerInfo, int eventId);
BlazeRpcError checkUserRegistration(StressPlayerInfo* playerInfo,int eventId, Blaze::SponsoredEvents::CheckUserRegistrationResponse &response);

BlazeRpcError	setStatus(StressPlayerInfo* playerInfo, int teamid);
}  // namespace Stress
}  // namespace Blaze
#endif  //   UTILITY_H
