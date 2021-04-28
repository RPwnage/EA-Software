//  *************************************************************************************************
//
//   File:    singleplayer.cpp
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "./singleplayer.h"
#include "./utility.h"
#include "framework/blaze.h"
#include "loginmanager.h"
#include "gamemanager/tdf/matchmaker.h"

namespace Blaze {
namespace Stress {

UserIdentMap         SinglePlayer::singlePlayerList;
EA::Thread::Mutex    SinglePlayer::singlePlayerListMutex;

SinglePlayer::SinglePlayer(StressPlayerInfo* playerData)
    : GamePlayUser(playerData), mConfig(SINGLE_GAMEPLAY_CONFIG),mIsInSinglePlayerGame(false),mOfflineGameType("\0"), mIsFutPlayer(false)
{
	gameTypeSetter();

}

SinglePlayer::~SinglePlayer()
{
}


BlazeRpcError SinglePlayer::navigateOfflineMode()
{

	/*if (!strcmp("gameType89", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str())) {
		setVProStats(mPlayerInfo);
		setSeasonsData(mPlayerInfo);
	}*/
	FetchConfigResponse response;
	fetchClientConfig(mPlayerInfo, "OSDK_TICKER", response);
	getStatGroupList(mPlayerInfo);
	/*setClientState(mPlayerInfo, ClientState::MODE_SINGLE_PLAYER, Blaze::ClientState::STATUS_NORMAL);
	fetchMessages(mPlayerInfo, 1919905645);
	fetchMessages(mPlayerInfo, 1634497140);
	fetchMessages(mPlayerInfo, 1668052322);
    associationGetLists(mPlayerInfo);
	FetchConfigResponse response;
    fetchClientConfig(mPlayerInfo, "OSDK_TICKER", response);
    fetchClientConfig(mPlayerInfo, "OSDK_ARENA", response);
    fetchClientConfig(mPlayerInfo, "OSDK_ROSTER", response);
	fetchSettingsGroups(mPlayerInfo);
	
	sleep(1000);
	resetUserGeoIPData(mPlayerInfo);
	userSettingsLoadAll(mPlayerInfo);
	if(!mPlayerInfo->getOwner()->isStressLogin())
	{
		getAccount(mPlayerInfo);
	}
	updateMailSettings(mPlayerInfo);
	setClientState(mPlayerInfo, ClientState::MODE_SINGLE_PLAYER, Blaze::ClientState::STATUS_SUSPENDED);*/
	return ERR_OK;
}

void SinglePlayer::gameTypeSetter()
{
	OfflineDistributionMap distributionReference = StressConfigInst.getOfflineScenarioDistribution();
	OfflineDistributionMap::iterator it = distributionReference.begin();
	uint8_t sumProb = 0;
	uint8_t randNumber = (uint8_t)Random::getRandomNumber(100);
	while( it != distributionReference.end())
	{
		sumProb += it->second;
		if(randNumber < sumProb)
		{
			mOfflineGameType = it->first;
			break;
		}
		it++;
	}
	if( mOfflineGameType != "\0")
	{
		if (mOfflineGameType.compare("gameType74") == 0 || mOfflineGameType.compare("gameType84") == 0) //if( !strcmp("gameType8" , mOfflineGameType.substr(0, (mOfflineGameType.length() - 1)).c_str() ) )
		{
			mIsFutPlayer = true;
		}
	}
}

BlazeRpcError SinglePlayer::viewSeasonLeaderboards() 
{
	BlazeRpcError result = ERR_OK;
    getLeaderboardTreeAsync(mPlayerInfo,"H2HSeasonalPlay");
    result = getLeaderboard(mPlayerInfo,"1v1_seasonalplay_overall");
	result = getCenteredLeaderboard(mPlayerInfo,0, 1, mPlayerInfo->getPlayerId(), "1v1_seasonalplay_overall");
	sleep(1000);
    //getLeaderboard(mPlayerInfo,0, 100, "1v1_seasonalplay_overall");
	// Implement this
	lookupUsers(mPlayerInfo, mPlayerInfo->getPersonaName().c_str());    
    //lookupRandomUsersByPersonaNames(mPlayerInfo);
    //result = getLeaderboard(mPlayerInfo,"1v1_seasonalplay_overall_fd");
	result  = getCenteredLeaderboard(mPlayerInfo,0, 1, mPlayerInfo->getPlayerId(), "1v1_seasonalplay_overall_fd");
	sleep(1000);
    result = getLeaderboard(mPlayerInfo,0, 100, "1v1_seasonalplay_overall_fd");
    //lookupRandomUsersByPersonaNames(mPlayerInfo);
	return result;
}

BlazeRpcError SinglePlayer::submitSkillGameReport(uint16_t skillGameNum) {
    // for skill games
    BlazeRpcError err = ERR_OK;
 
    // request object
    GameReporting::SubmitGameReportRequest request;
    // set status of FNSH
    request.setFinishedStatus(Blaze::GameReporting::GAMEREPORT_FINISHED_STATUS_DEFAULT);
    // get gamereport object which needs to be filled with game data
    GameReporting::GameReport &gamereport =   request.getGameReport();
	
    gamereport.setGameReportName("gameType21");
    gamereport.setGameReportingId(0);
    Blaze::GameReporting::Fifa::SkillGameReport* skillGameReport = BLAZE_NEW Blaze::GameReporting::Fifa::SkillGameReport;
    
	skillGameReport->setCountryCode("RO");
    skillGameReport->setSkillgame(skillGameNum);
    skillGameReport->setScore(Random::getRandomNumber(4000));
	skillGameReport->setPlayerid(mPlayerInfo->getPlayerId());
    gamereport.setReport(*skillGameReport);
    err = mPlayerInfo->getComponentProxy()->mGameReportingProxy->submitOfflineGameReport(request);
    if (err != ERR_OK)
	{
        BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][SinglePlayer::simulateLoad]: Failed to SubmitSkillGameRpeort with error "<<ErrorHelp::getErrorName(err));
        return err;
    }
    BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][SinglePlayer::simulateLoad]: SubmitSkillGameRpeort successful ");
    return err;
}

BlazeRpcError SinglePlayer::loadCOOPScreen() {
	BlazeRpcError result = ERR_OK;
	FetchConfigResponse configResponse;
    fetchClientConfig(mPlayerInfo, "FIFA_COOP_SEASONALPLAY",configResponse);
	FifaCups::CupInfo cupResponse;
    getCupInfo(mPlayerInfo, Blaze::FifaCups::FIFACUPS_MEMBERTYPE_USER,cupResponse);
    // Less priority implement look up for Random users
    //lookupRandomUsersByPersonaNames(mPlayerInfo);
	CoopSeason::GetAllCoopIdsResponse coopIdResponse;
	result = getStatGroup(mPlayerInfo,"CoopSeasonalPlay_StatGroup");
	sleep(1000);
	result = getAllCoopIds(mPlayerInfo, coopIdResponse);
    result = getStatsByGroupAsync(mPlayerInfo, "CoopSeasonalPlay_StatGroup");
	return result;
}

void SinglePlayer::viewDivisonStats(DivisionType divisionType) 
{
    //int divCount = -1;
	//int32_t divisionType = (int32_t)Random::getRandomNumber(100) + 1;
	switch(divisionType)
	{
	case COOPS :
			// Coop Division
			getStatGroup(mPlayerInfo,"SPCoopDivisionCount_StatGroup");
			getStatsByGroupAsync(mPlayerInfo,mPlayerInfo->getPlayerId(), mPlayerInfo->getViewId(),"SPCoopDivisionCount_StatGroup");
			mPlayerInfo->setViewId(mPlayerInfo->getViewId() + 1);
			//lookupRandomUsersByPersonaNames(mPlayerInfo);
		break;
	case SEASONS :
			// Seasons Division
			getStatGroup(mPlayerInfo,"SPDivisionCount");
			getStatsByGroupAsync(mPlayerInfo,mPlayerInfo->getPlayerId(), mPlayerInfo->getViewId(),"SPDivisionCount");
			mPlayerInfo->setViewId(mPlayerInfo->getViewId() + 1);
			//lookupRandomUsersByPersonaNames(mPlayerInfo);
		break;
	case CLUBS : 
			// ClubDivision
			getStatGroup(mPlayerInfo,"SPClubsDivisionCount");
			getStatsByGroupAsync(mPlayerInfo,mPlayerInfo->getPlayerId(), mPlayerInfo->getViewId(),"SPClubsDivisionCount");
			mPlayerInfo->setViewId(mPlayerInfo->getViewId() + 1);
			//lookupRandomUsersByPersonaNames(mPlayerInfo);
		break;
	default:
			// No Division
			getStatGroup(mPlayerInfo,"H2HSeasonalPlay");
			getStatsByGroupAsync(mPlayerInfo,mPlayerInfo->getPlayerId(), mPlayerInfo->getViewId(),"H2HSeasonalPlay");
			mPlayerInfo->setViewId(mPlayerInfo->getViewId() + 1);
			//lookupRandomUsersByPersonaNames(mPlayerInfo);
	break;
	}
}

BlazeRpcError SinglePlayer::viewClubLeaderBoards() 
{
	BlazeRpcError result = ERR_OK;
	getLeaderboardTreeAsync(mPlayerInfo, "Club");
	getLeaderboardGroup(mPlayerInfo,-1, "ClubOTPAllPlayerLB_03");
	getCenteredLeaderboard(mPlayerInfo, 0, 1, mPlayerInfo->getPlayerId(),"ClubOTPAllPlayerLB_03");
    getLeaderboard(mPlayerInfo, 0, 100, "ClubOTPAllPlayerLB_03");
	return result;
}

void SinglePlayer::loadSeasonsScreens() {
    getStatsByGroupAsync(mPlayerInfo,"H2HPreviousSeasonalPlay");
    lookupRandomUsersByPersonaNames(mPlayerInfo);
    getStatsByGroupAsync(mPlayerInfo,"H2HSeasonalPlay");
    //getCupDetails(mUserBlazeId, Blaze::FifaCups::FIFACUPS_MEMBERTYPE_USER, Blaze::OSDKTournaments::MODE_FIFA_CUPS_USER);
    lookupRandomUsersByPersonaNames(mPlayerInfo);
    getStatGroup(mPlayerInfo,"H2HSeasonalPlay");
    getStatsByGroupAsync(mPlayerInfo,"H2HSeasonalPlay");
	sleep(1000);
    //fetchClientConfig(mPlayerInfo,"FIFA_H2H_SEASONALPLAY");
    //GetCupDetails(mUserBlazeId, Blaze::FifaCups::FIFACUPS_MEMBERTYPE_USER, Blaze::OSDKTournaments::MODE_FIFA_CUPS_USER);
    //mFifaUtil.fifaCupsGetCupInfo(mUserBlazeId, Blaze::FifaCups::FIFACUPS_MEMBERTYPE_USER, mCupId, mUserBlazeId);
	SponsoredEvents::CheckUserRegistrationResponse response;
    int eventId = 1000;
    checkUserRegistration(mPlayerInfo,eventId, response);
    //lookupRandomUsersByPersonaNames(mPlayerInfo);
}

void SinglePlayer::viewCOOPLeaderboards() {
    getLeaderboardTreeAsync(mPlayerInfo,"CoopSeasonalPlay");    
    getLeaderboardGroup(mPlayerInfo,-1, "coop_seasonalplay_overall");
    //RpcDelay::getInstance()->delayRandomTime();
    getCenteredLeaderboard(mPlayerInfo,0, 1, mPlayerInfo->getPlayerId(), "coop_seasonalplay_overall");
    getLeaderboard(mPlayerInfo,0, 100, "coop_seasonalplay_overall");
	getStatsByGroupAsync(mPlayerInfo,mPlayerInfo->getPlayerId(),150, "CoopSeasonalPlay_StatGroup" );
	mPlayerInfo->setViewId(mPlayerInfo->getViewId() + 1);
    //GetCupDetails(mCoopId, Blaze::FifaCups::FIFACUPS_MEMBERTYPE_USER, Blaze::OSDKTournaments::MODE_FIFA_CUPS_USER);
   //lookupRandomUsersByPersonaNames(mPlayerInfo);
}

void SinglePlayer::viewSeasonHistory() {
    getGameReportViewInfo(mPlayerInfo,"NormalRecentGames");
    getGameReportView(mPlayerInfo,"NormalRecentGames");
    getGameReportViewInfo(mPlayerInfo,"CupSeasonRecentGames");
    getGameReportView(mPlayerInfo,"CupSeasonRecentGames");
    //lookupRandomUsersByPersonaNames(mPlayerInfo);

}

void SinglePlayer::viewGameHistory() {
	Blaze::GameReporting::QueryVarValuesList queryValues;
	queryValues.clear();
    char8_t queryVarValueStr[16];
    blaze_snzprintf(queryVarValueStr, sizeof(queryVarValueStr), "%" PRId64 "", mPlayerInfo->getPlayerId());
    queryValues.push_back(queryVarValueStr);
    queryValues.push_back("1");
    getGameReportViewInfo(mPlayerInfo,"VProDropInRecentGameRatings");
    getGameReportView(mPlayerInfo,"VProDropInRecentGameRatings", queryValues);
    getGameReportViewInfo(mPlayerInfo,"VProRegularRecentGameRatings");
    getGameReportView(mPlayerInfo,"VProRegularRecentGameRatings", queryValues);
    getGameReportViewInfo(mPlayerInfo,"VProCupRecentGameRatings");
	sleep(1000);
    getGameReportView(mPlayerInfo,"VProCupRecentGameRatings", queryValues);
    getGameReportViewInfo(mPlayerInfo,"VProDropInRecentGameRatings");
    getGameReportView(mPlayerInfo,"VProDropInRecentGameRatings", queryValues);
    getGameReportViewInfo(mPlayerInfo,"VProRegularRecentGameRatings");
    getGameReportView(mPlayerInfo,"VProRegularRecentGameRatings", queryValues);
    getGameReportViewInfo(mPlayerInfo,"VProCupRecentGameRatings");
    getGameReportView(mPlayerInfo,"VProCupRecentGameRatings", queryValues);
}

void SinglePlayer::statsExecutions()
{
		//viewSeasonLeaderboards();
		uint16_t leaderboardType = (uint16_t)Random::getRandomNumber(5) + 1 ;
		switch(leaderboardType)
		{
		case 1:
			// H2H Seasonal Play
			//lookupRandomUsersByPersonaNames(mPlayerInfo);			
			loadSeasonsScreens();
			viewSeasonHistory();
			sleep(1000);
			viewDivisonStats(SEASONS);
			viewSeasonLeaderboards();			
			break;
		case 2:
			// VproAve Matching
			getStatGroup(mPlayerInfo,"VProAveMatchRating");			
			getStatsByGroupAsync(mPlayerInfo, "VProAveMatchRating");
			viewDivisonStats(NO_DIVISION);
			sleep(1000);
			getStatsByGroupAsync(mPlayerInfo, "VProAveMatchRating");
			viewSeasonHistory();
			//lookupRandomUsersByPersonaNames(mPlayerInfo);
			break;
		case 3:
			// Coops Screen
			loadCOOPScreen();			
			viewDivisonStats(COOPS);
			sleep(1000);
			viewCOOPLeaderboards();
			break;
		case 4:
			// Clubs Leaderboard
			getStatGroup(mPlayerInfo,"ClubSeasonalPlay");
			getStatsByGroupAsync(mPlayerInfo,mPlayerInfo->getPlayerId(),mPlayerInfo->getViewId(), "ClubSeasonalPlay");
			//GetCupDetails(mClubId, Blaze::FifaCups::FIFACUPS_MEMBERTYPE_CLUB, Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB);
			//loadClubStats();
			//lookupRandomUsersByPersonaNames(mPlayerInfo);
			getGameReportViewInfo(mPlayerInfo,"ClubsSeasonRecentGames");
			getGameReportView(mPlayerInfo,"ClubsSeasonRecentGames");
			sleep(1000);
			getGameReportViewInfo(mPlayerInfo,"ClubsCupRecentGames");
			getGameReportView(mPlayerInfo,"ClubsCupRecentGames");
			fetchMessages(mPlayerInfo,4,5);
			getStatsByGroupAsync(mPlayerInfo,mPlayerInfo->getPlayerId(),mPlayerInfo->getViewId(), "ClubSeasonalPlay");
			viewDivisonStats(CLUBS);
			viewClubLeaderBoards();
			sleep(1000);
			viewGameHistory();
			break;
		case 5:
			getStatGroup(mPlayerInfo,"MyFriendlies");
			//getClubMembers
			getStatsByGroupAsync(mPlayerInfo,"MyFriendlies");
			getStatsByGroupAsync(mPlayerInfo,"MyFriendlies");
			viewSeasonHistory();
			break;
		default:
			viewDivisonStats(NO_DIVISION);
			sleep(1000);
			//viewSeasonLeaderboards();
			//lookupRandomUsersByPersonaNames(mPlayerInfo);
			break;
		}
}
BlazeRpcError SinglePlayer::updateStats()
{
	BlazeRpcError err = ERR_OK;
	Blaze::FifaStats::UpdateStatsResponse updateStatsResponse;
	Blaze::FifaStats::UpdateStatsRequest updateStatsRequest;
	updateStatsRequest.setCategoryId("GameplayStats");
	updateStatsRequest.setContextOverrideType(Blaze::FifaStats::ContextOverrideType::CONTEXT_OVERRIDE_TYPE_FUT);
	updateStatsRequest.setServiceName("");
	int64_t uniqueID = TimeValue::getTimeOfDay().getSec() + Random::getRandomNumber(10000);
	eastl::string updateId = eastl::string().sprintf("%" PRIu64 """-""%" PRIu64 "", mPlayerInfo->getPlayerId(), uniqueID).c_str();
	updateStatsRequest.setUpdateId(updateId.c_str());

	Blaze::FifaStats::Entity *entity = updateStatsRequest.getEntities().pull_back();
	//Blaze::FifaStats::StatsUpdateList &statUpdateList = entity->getStatsUpdates();
	//set group ID
	eastl::string entityId = eastl::string().sprintf("0_""%" PRIu64 "", mPlayerInfo->getPlayerId()).c_str();
	entity->setEntityId(entityId.c_str());
	
	//Seen more than 397 event in request for 194
	Blaze::FifaStats::StatsUpdate *statsUpdate[273];
	for (int i = 0; i < 273; i++)
	{
		statsUpdate[i] = entity->getStatsUpdates().pull_back();
		statsUpdate[i]->setOperator(Blaze::FifaStats::StatsOperator::OPERATOR_SUM);
		statsUpdate[i]->setStatsFValue(0.00000000);
		statsUpdate[i]->setType(Blaze::FifaStats::StatsType::TYPE_INT);
	}
	statsUpdate[0]->setStatsIValue(0);
	statsUpdate[0]->setStatsName("T_0_g");
	statsUpdate[1]->setStatsIValue(1);
	statsUpdate[1]->setStatsName("T_0_og");
	statsUpdate[2]->setStatsIValue(0);
	statsUpdate[2]->setStatsName("T_0_pkg");
	statsUpdate[3]->setStatsIValue(85);
	statsUpdate[3]->setStatsName("T_0_pos");
	statsUpdate[4]->setStatsIValue(0);
	statsUpdate[4]->setStatsName("T_0_s");
	statsUpdate[5]->setStatsIValue(0);
	statsUpdate[5]->setStatsName("T_0_sog");
	statsUpdate[6]->setStatsIValue(12);
	statsUpdate[6]->setStatsName("T_0_pa");
	statsUpdate[7]->setStatsIValue(8);
	statsUpdate[7]->setStatsName("T_0_pm");
	statsUpdate[8]->setStatsIValue(0);
	statsUpdate[8]->setStatsName("T_0_ta");
	statsUpdate[9]->setStatsIValue(0);
	statsUpdate[9]->setStatsName("T_0_tm");
	statsUpdate[10]->setStatsIValue(2);
	statsUpdate[10]->setStatsName("T_0_gc");
	statsUpdate[11]->setStatsIValue(0);
	statsUpdate[11]->setStatsName("T_0_f");
	statsUpdate[12]->setStatsIValue(0);
	statsUpdate[12]->setStatsName("T_0_yc");
	statsUpdate[13]->setStatsIValue(0);
	statsUpdate[13]->setStatsName("T_0_rc");
	statsUpdate[14]->setStatsIValue(0);
	statsUpdate[14]->setStatsName("T_0_c");
	statsUpdate[15]->setStatsIValue(0);
	statsUpdate[15]->setStatsName("T_0_os");
	statsUpdate[16]->setStatsIValue(0);
	statsUpdate[16]->setStatsName("T_0_cs");
	statsUpdate[17]->setStatsIValue(11);
	statsUpdate[17]->setStatsName("T_0_sa");
	statsUpdate[18]->setStatsIValue(5);
	statsUpdate[18]->setStatsName("T_0_saog");
	statsUpdate[19]->setStatsIValue(6);
	statsUpdate[19]->setStatsName("T_0_int");
	statsUpdate[20]->setStatsIValue(0);
	statsUpdate[20]->setStatsName("T_0_pka");
	statsUpdate[21]->setStatsIValue(0);
	statsUpdate[21]->setStatsName("T_0_pks");
	statsUpdate[22]->setStatsIValue(0);
	statsUpdate[22]->setStatsName("T_0_pkv");
	statsUpdate[23]->setStatsIValue(0);
	statsUpdate[23]->setStatsName("T_0_pkm");
	statsUpdate[24]->setStatsIValue(4);
	statsUpdate[24]->setStatsName("T_0_pi");
	statsUpdate[25]->setStatsIValue(0);
	statsUpdate[25]->setStatsName("T_0_ir");
	statsUpdate[26]->setStatsIValue(0);
	statsUpdate[26]->setStatsName("T_0_atklp");
	statsUpdate[27]->setStatsIValue(0);
	statsUpdate[27]->setStatsName("T_0_atkmp");
	statsUpdate[28]->setStatsIValue(0);
	statsUpdate[28]->setStatsName("T_0_atkrp");
	statsUpdate[29]->setStatsIValue(0);
	statsUpdate[29]->setStatsName("T_0_g15");
	statsUpdate[30]->setStatsIValue(0);
	statsUpdate[30]->setStatsName("T_0_g30");
	statsUpdate[31]->setStatsIValue(0);
	statsUpdate[31]->setStatsName("T_0_g45");
	statsUpdate[32]->setStatsIValue(0);
	statsUpdate[32]->setStatsName("T_0_g60");
	statsUpdate[33]->setStatsIValue(0);
	statsUpdate[33]->setStatsName("T_0_g75");
	statsUpdate[34]->setStatsIValue(0);
	statsUpdate[34]->setStatsName("T_0_g90");
	statsUpdate[35]->setStatsIValue(0);
	statsUpdate[35]->setStatsName("T_0_g90Plus");
	statsUpdate[36]->setStatsIValue(0);
	statsUpdate[36]->setStatsName("T_0_gob");
	statsUpdate[37]->setStatsIValue(0);
	statsUpdate[37]->setStatsName("T_0_gib");
	statsUpdate[38]->setStatsIValue(0);
	statsUpdate[38]->setStatsName("T_0_gfk");
	statsUpdate[39]->setStatsIValue(0);
	statsUpdate[39]->setStatsName("T_0_g2pt");
	statsUpdate[40]->setStatsIValue(0);
	statsUpdate[40]->setStatsName("T_0_g3pt");
	statsUpdate[41]->setStatsIValue(21);
	statsUpdate[41]->setStatsName("T_0_posl");
	statsUpdate[42]->setStatsIValue(1);
	statsUpdate[42]->setStatsName("T_0_cdr");
	statsUpdate[43]->setStatsIValue(0);
	statsUpdate[43]->setStatsName("T_0_ca");
	statsUpdate[44]->setStatsIValue(0);
	statsUpdate[44]->setStatsName("T_0_cm");
	statsUpdate[45]->setStatsIValue(1);
	statsUpdate[45]->setStatsName("T_0_bs");
	statsUpdate[46]->setStatsIValue(0);
	statsUpdate[46]->setStatsName("T_0_slta");
	statsUpdate[47]->setStatsIValue(0);
	statsUpdate[47]->setStatsName("T_0_sltm");
	statsUpdate[48]->setStatsIValue(0);
	statsUpdate[48]->setStatsName("T_0_stta");
	statsUpdate[49]->setStatsIValue(0);
	statsUpdate[49]->setStatsName("T_0_sttm");
	statsUpdate[50]->setStatsIValue(0);
	statsUpdate[50]->setStatsName("S_0_ow");
	statsUpdate[51]->setStatsIValue(0);
	statsUpdate[51]->setStatsName("S_0_od");
	statsUpdate[52]->setStatsIValue(1);
	statsUpdate[52]->setStatsName("S_0_ol");
	statsUpdate[53]->setStatsIValue(0);
	statsUpdate[53]->setStatsName("S_0_smw");
	statsUpdate[54]->setStatsIValue(0);
	statsUpdate[54]->setStatsName("S_0_smd");
	statsUpdate[55]->setStatsIValue(0);
	statsUpdate[55]->setStatsName("S_0_sml");
	statsUpdate[56]->setStatsIValue(0);
	statsUpdate[56]->setStatsName("S_0_cw");
	statsUpdate[57]->setStatsIValue(0);
	statsUpdate[57]->setStatsName("S_0_cd");
	statsUpdate[58]->setStatsIValue(0);
	statsUpdate[58]->setStatsName("S_0_cl");
	statsUpdate[59]->setStatsIValue(0);
	statsUpdate[59]->setStatsName("S_0_suw");
	statsUpdate[60]->setStatsIValue(0);
	statsUpdate[60]->setStatsName("S_0_sud");
	statsUpdate[61]->setStatsIValue(0);
	statsUpdate[61]->setStatsName("S_0_sul");
	statsUpdate[62]->setStatsIValue(0);
	statsUpdate[62]->setStatsName("S_0_baw");
	statsUpdate[63]->setStatsIValue(0);
	statsUpdate[63]->setStatsName("S_0_bad");
	statsUpdate[64]->setStatsIValue(0);
	statsUpdate[64]->setStatsName("S_0_bal");
	statsUpdate[65]->setStatsIValue(0);
	statsUpdate[65]->setStatsName("S_0_fxw");
	statsUpdate[66]->setStatsIValue(0);
	statsUpdate[66]->setStatsName("S_0_fxd");
	statsUpdate[67]->setStatsIValue(0);
	statsUpdate[67]->setStatsName("S_0_fxl");
	statsUpdate[68]->setStatsIValue(0);
	statsUpdate[68]->setStatsName("S_0_nrw");
	statsUpdate[69]->setStatsIValue(0);
	statsUpdate[69]->setStatsName("S_0_nrd");
	statsUpdate[70]->setStatsIValue(0);
	statsUpdate[70]->setStatsName("S_0_nrl");
	statsUpdate[71]->setStatsIValue(0);
	statsUpdate[71]->setStatsName("S_0_hvw");
	statsUpdate[72]->setStatsIValue(0);
	statsUpdate[72]->setStatsName("S_0_hvd");
	statsUpdate[73]->setStatsIValue(0);
	statsUpdate[73]->setStatsName("S_0_hvl");
	statsUpdate[74]->setStatsIValue(0);
	statsUpdate[74]->setStatsName("S_0_mbw");
	statsUpdate[75]->setStatsIValue(0);
	statsUpdate[75]->setStatsName("S_0_mbd");
	statsUpdate[76]->setStatsIValue(0);
	statsUpdate[76]->setStatsName("S_0_mbl");
	statsUpdate[77]->setStatsIValue(0);
	statsUpdate[77]->setStatsName("S_0_tww");
	statsUpdate[78]->setStatsIValue(0);
	statsUpdate[78]->setStatsName("S_0_twd");
	statsUpdate[79]->setStatsIValue(0);
	statsUpdate[79]->setStatsName("S_0_twl");
	statsUpdate[80]->setStatsIValue(0);
	statsUpdate[80]->setStatsName("S_0_tlw");
	statsUpdate[81]->setStatsIValue(0);
	statsUpdate[81]->setStatsName("S_0_tll");
	statsUpdate[82]->setStatsIValue(0);
	statsUpdate[82]->setStatsName("S_0_bxw");
	statsUpdate[83]->setStatsIValue(0);
	statsUpdate[83]->setStatsName("S_0_bxl");
	statsUpdate[84]->setStatsIValue(0);
	statsUpdate[84]->setStatsName("S_0_lws");
	statsUpdate[85]->setStatsIValue(0);
	statsUpdate[85]->setStatsName("S_0_cst");
	statsUpdate[86]->setStatsIValue(0);
	statsUpdate[86]->setStatsName("S_0_fgs");
	statsUpdate[87]->setStatsIValue(0);
	statsUpdate[87]->setStatsName("S_0_fgt");
	statsUpdate[88]->setStatsIValue(0);
	statsUpdate[88]->setStatsName("S_0_fgmt");
	statsUpdate[89]->setStatsIValue(0);
	statsUpdate[89]->setStatsName("S_0_fgot");
	statsUpdate[90]->setStatsIValue(0);
	statsUpdate[90]->setStatsName("S_0_");
	statsUpdate[91]->setStatsIValue(0);
	statsUpdate[91]->setStatsName("S_0_");
	statsUpdate[92]->setStatsIValue(0);
	statsUpdate[92]->setStatsName("S_0_");
	statsUpdate[93]->setStatsIValue(0);
	statsUpdate[93]->setStatsName("S_0_");
	statsUpdate[94]->setStatsIValue(0);
	statsUpdate[94]->setStatsName("S_0_");
	statsUpdate[95]->setStatsIValue(0);
	statsUpdate[95]->setStatsName("S_0_");
	statsUpdate[96]->setStatsIValue(0);
	statsUpdate[96]->setStatsName("S_0_");
	statsUpdate[97]->setStatsIValue(0);
	statsUpdate[97]->setStatsName("S_0_");
	statsUpdate[98]->setStatsIValue(0);
	statsUpdate[98]->setStatsName("S_0_");
	statsUpdate[99]->setStatsIValue(0);
	statsUpdate[99]->setStatsName("S_0_");
	statsUpdate[100]->setStatsIValue(0);
	statsUpdate[100]->setStatsName("S_0_");
	statsUpdate[101]->setStatsIValue(0);
	statsUpdate[101]->setStatsName("S_0_");
	statsUpdate[102]->setStatsIValue(0);
	statsUpdate[102]->setStatsName("S_0_");
	statsUpdate[103]->setStatsIValue(0);
	statsUpdate[103]->setStatsName("S_0_");
	statsUpdate[104]->setStatsIValue(0);
	statsUpdate[104]->setStatsName("S_0_");
	statsUpdate[105]->setStatsIValue(0);
	statsUpdate[105]->setStatsName("S_0_");
	statsUpdate[106]->setStatsIValue(0);
	statsUpdate[106]->setStatsName("S_0_");
	statsUpdate[107]->setStatsIValue(0);
	statsUpdate[107]->setStatsName("S_0_");
	statsUpdate[108]->setStatsIValue(0);
	statsUpdate[108]->setStatsName("S_0_");
	statsUpdate[109]->setStatsIValue(0);
	statsUpdate[109]->setStatsName("S_0_");
	statsUpdate[110]->setStatsIValue(0);
	statsUpdate[110]->setStatsName("S_0_");
	statsUpdate[111]->setStatsIValue(0);
	statsUpdate[111]->setStatsName("S_0_");
	statsUpdate[112]->setStatsIValue(0);
	statsUpdate[112]->setStatsName("S_0_");
	statsUpdate[113]->setStatsIValue(0);
	statsUpdate[113]->setStatsName("S_0_");
	statsUpdate[114]->setStatsIValue(0);
	statsUpdate[114]->setStatsName("S_0_");
	statsUpdate[115]->setStatsIValue(0);
	statsUpdate[115]->setStatsName("S_0_");
	statsUpdate[116]->setStatsIValue(0);
	statsUpdate[116]->setStatsName("S_0_");
	statsUpdate[117]->setStatsIValue(0);
	statsUpdate[117]->setStatsName("S_0_");
	statsUpdate[118]->setStatsIValue(0);
	statsUpdate[118]->setStatsName("S_0_");
	statsUpdate[119]->setStatsIValue(0);
	statsUpdate[119]->setStatsName("S_0_");
	statsUpdate[120]->setStatsIValue(0);
	statsUpdate[120]->setStatsName("S_0_");
	statsUpdate[121]->setStatsIValue(0);
	statsUpdate[121]->setStatsName("S_0_");
	statsUpdate[122]->setStatsIValue(0);
	statsUpdate[122]->setStatsName("S_0_");
	statsUpdate[123]->setStatsIValue(0);
	statsUpdate[123]->setStatsName("H_0_0_0");
	statsUpdate[124]->setStatsIValue(0);
	statsUpdate[124]->setStatsName("H_0_0_1");
	statsUpdate[125]->setStatsIValue(0);
	statsUpdate[125]->setStatsName("H_0_1_0");
	statsUpdate[126]->setStatsIValue(0);
	statsUpdate[126]->setStatsName("H_0_1_1");
	statsUpdate[127]->setStatsIValue(0);
	statsUpdate[127]->setStatsName("H_0_2_0");
	statsUpdate[128]->setStatsIValue(0);
	statsUpdate[128]->setStatsName("H_0_2_1");
	statsUpdate[129]->setStatsIValue(2);
	statsUpdate[129]->setStatsName("T_1_g");
	statsUpdate[130]->setStatsIValue(0);
	statsUpdate[130]->setStatsName("T_1_og");
	statsUpdate[131]->setStatsIValue(0);
	statsUpdate[131]->setStatsName("T_1_pkg");
	statsUpdate[132]->setStatsIValue(15);
	statsUpdate[132]->setStatsName("T_1_pos");
	statsUpdate[133]->setStatsIValue(11);
	statsUpdate[133]->setStatsName("T_1_s");
	statsUpdate[134]->setStatsIValue(5);
	statsUpdate[134]->setStatsName("T_1_sog");
	statsUpdate[135]->setStatsIValue(31);
	statsUpdate[135]->setStatsName("T_1_pa");
	statsUpdate[136]->setStatsIValue(19);
	statsUpdate[136]->setStatsName("T_1_pm");
	statsUpdate[137]->setStatsIValue(13);
	statsUpdate[137]->setStatsName("T_1_ta");
	statsUpdate[138]->setStatsIValue(11);
	statsUpdate[138]->setStatsName("T_1_tm");
	statsUpdate[139]->setStatsIValue(0);
	statsUpdate[139]->setStatsName("T_1_gc");
	statsUpdate[140]->setStatsIValue(1);
	statsUpdate[140]->setStatsName("T_1_f");
	statsUpdate[141]->setStatsIValue(0);
	statsUpdate[141]->setStatsName("T_1_yc");
	statsUpdate[142]->setStatsIValue(0);
	statsUpdate[142]->setStatsName("T_1_rc");
	statsUpdate[143]->setStatsIValue(1);
	statsUpdate[143]->setStatsName("T_1_c");
	statsUpdate[144]->setStatsIValue(0);
	statsUpdate[144]->setStatsName("T_1_os");
	statsUpdate[145]->setStatsIValue(1);
	statsUpdate[145]->setStatsName("T_1_cs");
	statsUpdate[146]->setStatsIValue(0);
	statsUpdate[146]->setStatsName("T_1_sa");
	statsUpdate[147]->setStatsIValue(0);
	statsUpdate[147]->setStatsName("T_1_saog");
	statsUpdate[148]->setStatsIValue(1);
	statsUpdate[148]->setStatsName("T_1_int");
	statsUpdate[149]->setStatsIValue(0);
	statsUpdate[149]->setStatsName("T_1_pka");
	statsUpdate[150]->setStatsIValue(0);
	statsUpdate[150]->setStatsName("T_1_pks");
	statsUpdate[151]->setStatsIValue(0);
	statsUpdate[151]->setStatsName("T_1_pkv");
	statsUpdate[152]->setStatsIValue(0);
	statsUpdate[152]->setStatsName("T_1_pkm");
	statsUpdate[153]->setStatsIValue(12);
	statsUpdate[153]->setStatsName("T_1_pi");
	statsUpdate[154]->setStatsIValue(0);
	statsUpdate[154]->setStatsName("T_1_ir");
	statsUpdate[155]->setStatsIValue(25);
	statsUpdate[155]->setStatsName("T_1_atklp");
	statsUpdate[156]->setStatsIValue(61);
	statsUpdate[156]->setStatsName("T_1_atkmp");
	statsUpdate[157]->setStatsIValue(14);
	statsUpdate[157]->setStatsName("T_1_atkrp");
	statsUpdate[158]->setStatsIValue(0);
	statsUpdate[158]->setStatsName("T_1_g15");
	statsUpdate[159]->setStatsIValue(0);
	statsUpdate[159]->setStatsName("T_1_g30");
	statsUpdate[160]->setStatsIValue(1);
	statsUpdate[160]->setStatsName("T_1_g45");
	statsUpdate[161]->setStatsIValue(0);
	statsUpdate[161]->setStatsName("T_1_g60");
	statsUpdate[162]->setStatsIValue(0);
	statsUpdate[162]->setStatsName("T_1_g75");
	statsUpdate[163]->setStatsIValue(0);
	statsUpdate[163]->setStatsName("T_1_g90");
	statsUpdate[164]->setStatsIValue(0);
	statsUpdate[164]->setStatsName("T_1_g90Plus");
	statsUpdate[165]->setStatsIValue(0);
	statsUpdate[165]->setStatsName("T_1_gob");
	statsUpdate[166]->setStatsIValue(1);
	statsUpdate[166]->setStatsName("T_1_gib");
	statsUpdate[167]->setStatsIValue(0);
	statsUpdate[167]->setStatsName("T_1_gfk");
	statsUpdate[168]->setStatsIValue(0);
	statsUpdate[168]->setStatsName("T_1_g2pt");
	statsUpdate[169]->setStatsIValue(0);
	statsUpdate[169]->setStatsName("T_1_g3pt");
	statsUpdate[170]->setStatsIValue(18);
	statsUpdate[170]->setStatsName("T_1_posl");
	statsUpdate[171]->setStatsIValue(31);
	statsUpdate[171]->setStatsName("T_1_cdr");
	statsUpdate[172]->setStatsIValue(5);
	statsUpdate[172]->setStatsName("T_1_ca");
	statsUpdate[173]->setStatsIValue(2);
	statsUpdate[173]->setStatsName("T_1_cm");
	statsUpdate[174]->setStatsIValue(0);
	statsUpdate[174]->setStatsName("T_1_bs");
	statsUpdate[175]->setStatsIValue(0);
	statsUpdate[175]->setStatsName("T_1_slta");
	statsUpdate[176]->setStatsIValue(0);
	statsUpdate[176]->setStatsName("T_1_sltm");
	statsUpdate[177]->setStatsIValue(13);
	statsUpdate[177]->setStatsName("T_1_stta");
	statsUpdate[178]->setStatsIValue(11);
	statsUpdate[178]->setStatsName("T_1_sttm");
	statsUpdate[179]->setStatsIValue(1);
	statsUpdate[179]->setStatsName("S_1_ow");
	statsUpdate[180]->setStatsIValue(0);
	statsUpdate[180]->setStatsName("S_1_od");
	statsUpdate[181]->setStatsIValue(0);
	statsUpdate[181]->setStatsName("S_1_ol");
	statsUpdate[182]->setStatsIValue(0);
	statsUpdate[182]->setStatsName("S_1_smw");
	statsUpdate[183]->setStatsIValue(0);
	statsUpdate[183]->setStatsName("S_1_smd");
	statsUpdate[184]->setStatsIValue(0);
	statsUpdate[184]->setStatsName("S_1_sml");
	statsUpdate[185]->setStatsIValue(0);
	statsUpdate[185]->setStatsName("S_1_cw");
	statsUpdate[186]->setStatsIValue(0);
	statsUpdate[186]->setStatsName("S_1_cd");
	statsUpdate[187]->setStatsIValue(0);
	statsUpdate[187]->setStatsName("S_1_cl");
	statsUpdate[188]->setStatsIValue(0);
	statsUpdate[188]->setStatsName("S_1_suw");
	statsUpdate[189]->setStatsIValue(0);
	statsUpdate[189]->setStatsName("S_1_sud");
	statsUpdate[190]->setStatsIValue(0);
	statsUpdate[190]->setStatsName("S_1_sul");
	statsUpdate[191]->setStatsIValue(0);
	statsUpdate[191]->setStatsName("S_1_baw");
	statsUpdate[192]->setStatsIValue(0);
	statsUpdate[192]->setStatsName("S_1_bad");
	statsUpdate[193]->setStatsIValue(0);
	statsUpdate[193]->setStatsName("S_1_bal");
	statsUpdate[194]->setStatsIValue(0);
	statsUpdate[194]->setStatsName("S_1_fxw");
	statsUpdate[195]->setStatsIValue(0);
	statsUpdate[195]->setStatsName("S_1_fxd");
	statsUpdate[196]->setStatsIValue(0);
	statsUpdate[196]->setStatsName("S_1_fxl");
	statsUpdate[197]->setStatsIValue(0);
	statsUpdate[197]->setStatsName("S_1_nrw");
	statsUpdate[198]->setStatsIValue(0);
	statsUpdate[198]->setStatsName("S_1_nrd");
	statsUpdate[199]->setStatsIValue(0);
	statsUpdate[199]->setStatsName("S_1_nrl");
	statsUpdate[200]->setStatsIValue(0);
	statsUpdate[200]->setStatsName("S_1_hvw");
	statsUpdate[201]->setStatsIValue(0);
	statsUpdate[201]->setStatsName("S_1_hvd");
	statsUpdate[202]->setStatsIValue(0);
	statsUpdate[202]->setStatsName("S_1_hvl");
	statsUpdate[203]->setStatsIValue(0);
	statsUpdate[203]->setStatsName("S_1_mbw");
	statsUpdate[204]->setStatsIValue(0);
	statsUpdate[204]->setStatsName("S_1_mbd");
	statsUpdate[205]->setStatsIValue(0);
	statsUpdate[205]->setStatsName("S_1_mbl");
	statsUpdate[206]->setStatsIValue(0);
	statsUpdate[206]->setStatsName("S_1_tww");
	statsUpdate[207]->setStatsIValue(0);
	statsUpdate[207]->setStatsName("S_1_twd");
	statsUpdate[208]->setStatsIValue(0);
	statsUpdate[208]->setStatsName("S_1_twl");
	statsUpdate[209]->setStatsIValue(0);
	statsUpdate[209]->setStatsName("S_1_tlw");
	statsUpdate[210]->setStatsIValue(0);
	statsUpdate[210]->setStatsName("S_1_tll");
	statsUpdate[211]->setStatsIValue(0);
	statsUpdate[211]->setStatsName("S_1_bxw");
	statsUpdate[212]->setStatsIValue(0);
	statsUpdate[212]->setStatsName("S_1_bxl");
	statsUpdate[213]->setStatsIValue(0);
	statsUpdate[213]->setStatsName("S_1_lws");
	statsUpdate[214]->setStatsIValue(0);
	statsUpdate[214]->setStatsName("S_1_cst");
	statsUpdate[215]->setStatsIValue(1991);
	statsUpdate[215]->setStatsName("S_1_fgs");
	statsUpdate[216]->setStatsIValue(0);
	statsUpdate[216]->setStatsName("S_1_fgt");
	statsUpdate[217]->setStatsIValue(130001);
	statsUpdate[217]->setStatsName("S_1_fgmt");
	statsUpdate[218]->setStatsIValue(130000);
	statsUpdate[218]->setStatsName("S_1_fgot");
	statsUpdate[219]->setStatsIValue(0);
	statsUpdate[219]->setStatsName("S_1_");
	statsUpdate[220]->setStatsIValue(0);
	statsUpdate[220]->setStatsName("S_1_");
	statsUpdate[221]->setStatsIValue(0);
	statsUpdate[221]->setStatsName("S_1_");
	statsUpdate[222]->setStatsIValue(0);
	statsUpdate[222]->setStatsName("S_1_");
	statsUpdate[223]->setStatsIValue(0);
	statsUpdate[223]->setStatsName("S_1_");
	statsUpdate[224]->setStatsIValue(0);
	statsUpdate[224]->setStatsName("S_1_");
	statsUpdate[225]->setStatsIValue(0);
	statsUpdate[225]->setStatsName("S_1_");
	statsUpdate[226]->setStatsIValue(0);
	statsUpdate[226]->setStatsName("S_1_");
	statsUpdate[227]->setStatsIValue(0);
	statsUpdate[227]->setStatsName("S_1_");
	statsUpdate[228]->setStatsIValue(0);
	statsUpdate[228]->setStatsName("S_1_");
	statsUpdate[229]->setStatsIValue(0);
	statsUpdate[229]->setStatsName("S_1_");
	statsUpdate[230]->setStatsIValue(0);
	statsUpdate[230]->setStatsName("S_1_");
	statsUpdate[231]->setStatsIValue(0);
	statsUpdate[231]->setStatsName("S_1_");
	statsUpdate[232]->setStatsIValue(0);
	statsUpdate[232]->setStatsName("S_1_");
	statsUpdate[233]->setStatsIValue(0);
	statsUpdate[233]->setStatsName("S_1_");
	statsUpdate[234]->setStatsIValue(0);
	statsUpdate[234]->setStatsName("S_1_");
	statsUpdate[235]->setStatsIValue(0);
	statsUpdate[235]->setStatsName("S_1_");
	statsUpdate[236]->setStatsIValue(0);
	statsUpdate[236]->setStatsName("S_1_");
	statsUpdate[237]->setStatsIValue(0);
	statsUpdate[237]->setStatsName("S_1_");
	statsUpdate[238]->setStatsIValue(0);
	statsUpdate[238]->setStatsName("S_1_");
	statsUpdate[239]->setStatsIValue(0);
	statsUpdate[239]->setStatsName("S_1_");
	statsUpdate[240]->setStatsIValue(0);
	statsUpdate[240]->setStatsName("S_1_");
	statsUpdate[241]->setStatsIValue(0);
	statsUpdate[241]->setStatsName("S_1_");
	statsUpdate[242]->setStatsIValue(0);
	statsUpdate[242]->setStatsName("S_1_");
	statsUpdate[243]->setStatsIValue(0);
	statsUpdate[243]->setStatsName("S_1_");
	statsUpdate[244]->setStatsIValue(0);
	statsUpdate[244]->setStatsName("S_1_");
	statsUpdate[245]->setStatsIValue(0);
	statsUpdate[245]->setStatsName("S_1_");
	statsUpdate[246]->setStatsIValue(0);
	statsUpdate[246]->setStatsName("S_1_");
	statsUpdate[247]->setStatsIValue(0);
	statsUpdate[247]->setStatsName("S_1_");
	statsUpdate[248]->setStatsIValue(0);
	statsUpdate[248]->setStatsName("S_1_");
	statsUpdate[249]->setStatsIValue(0);
	statsUpdate[249]->setStatsName("S_1_");
	statsUpdate[250]->setStatsIValue(0);
	statsUpdate[250]->setStatsName("S_1_");
	statsUpdate[251]->setStatsIValue(0);
	statsUpdate[251]->setStatsName("S_1_");
	statsUpdate[252]->setStatsIValue(1);
	statsUpdate[252]->setStatsName("H_1_0_0");
	statsUpdate[253]->setStatsIValue(0);
	statsUpdate[253]->setStatsName("H_1_0_1");
	statsUpdate[254]->setStatsIValue(0);
	statsUpdate[254]->setStatsName("H_1_1_0");
	statsUpdate[255]->setStatsIValue(0);
	statsUpdate[255]->setStatsName("H_1_1_1");
	statsUpdate[256]->setStatsIValue(0);
	statsUpdate[256]->setStatsName("H_1_2_0");
	statsUpdate[257]->setStatsIValue(0);
	statsUpdate[257]->setStatsName("H_1_2_1");
	statsUpdate[258]->setStatsIValue(1);
	statsUpdate[258]->setStatsName("G_0_gn");
	statsUpdate[259]->setStatsIValue(0);
	statsUpdate[259]->setStatsName("G_0_gt");
	statsUpdate[260]->setStatsIValue(0);
	statsUpdate[260]->setStatsName("G_0_gmd");
	statsUpdate[261]->setStatsIValue(0);
	statsUpdate[261]->setStatsName("G_0_t0g");
	statsUpdate[262]->setStatsIValue(2);
	statsUpdate[262]->setStatsName("G_0_t1g");
	statsUpdate[263]->setStatsIValue(0);
	statsUpdate[263]->setStatsName("G_0_t0pk");
	statsUpdate[264]->setStatsIValue(0);
	statsUpdate[264]->setStatsName("G_0_t1pk");
	statsUpdate[265]->setStatsIValue(0);
	statsUpdate[265]->setStatsName("G_0_t02pt");
	statsUpdate[266]->setStatsIValue(0);
	statsUpdate[266]->setStatsName("G_0_t12pt");
	statsUpdate[267]->setStatsIValue(0);
	statsUpdate[267]->setStatsName("G_0_t03pt");
	statsUpdate[268]->setStatsIValue(0);
	statsUpdate[268]->setStatsName("G_0_t13pt");
	statsUpdate[269]->setStatsIValue(1);
	statsUpdate[269]->setStatsName("G_0_ht");
	statsUpdate[270]->setStatsIValue(130000);
	statsUpdate[270]->setStatsName("G_0_t0id");
	statsUpdate[271]->setStatsIValue(130001);
	statsUpdate[271]->setStatsName("G_0_t1id");
	statsUpdate[272]->setStatsIValue(1);
	statsUpdate[272]->setStatsName("ovgc");

	//statUpdateList.clear();
	/*for (int i = 0; i < 30; i++)
	{
		statUpdateList.push_back(statsUpdate[i]);
	}
	for (int i = 30; i < 269; i++)
	{
		statUpdateList.push_back(statsUpdate[i]);
	}*/
	/*for (int i = 84; i < 269; i++)
	{
		statUpdateList.push_back(statsUpdate[i]);
	}*/

	/*char8_t buf[102400];
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "-> [UPDATESTATS]" << updateStatsRequest.print(buf, sizeof(buf)));*/
	err = mPlayerInfo->getComponentProxy()->mFifaStatsProxy->updateStats(updateStatsRequest, updateStatsResponse);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::fifastats, "SinglePlayer::updateStats:Overall: playerId " << mPlayerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	return err;
}
BlazeRpcError SinglePlayer::simulateLoad()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SinglePlayer::simulateLoad]:" << mPlayerInfo->getPlayerId());
 
    if (!mConfig)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][SinglePlayer::simulateLoad]: Failed to Simulate Load, Configuration is not loaded");
        return ERR_SYSTEM;
    }

	simulateMenuFlow();
	getInvitations(mPlayerInfo, 0, CLUBS_SENT_TO_ME);
	/*if (!strcmp("gameType89", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str()))
	{//added as per logs
		Blaze::PersonaNameList personaNameList;
		personaNameList.push_back("dr_hoo");
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);

		eastl::vector<eastl::string> stringIds;
		stringIds.push_back("TXT_CONTACT_INFO_TEAM_112885");
		stringIds.push_back("TXT_CONTACT_INFO_LEAGUE_39");
		stringIds.push_back("TXT_CONTACT_INFO_FIFA_GAME");
		stringIds.push_back("TeamName_112885");
		stringIds.push_back("LeagueName_39");
		stringIds.push_back("FEDERATIONINTERNATIONALEDEFOOTBALLASSOCIATION");
		stringIds.push_back("TXT_CONTACT_INFO_UEFA");
		stringIds.push_back("TXT_UEFA");
		localizeStrings(mPlayerInfo, 1701724993, stringIds);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		setConnectionState(mPlayerInfo);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	}*/
	
	if (!strcmp("gameType78", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str()))
	{//added as per logs squad battles
		lookupUsersByPersonaNames(mPlayerInfo, "");
		if (StressConfigInst.getPlatform() == Platform::PLATFORM_PS4)
		lookupUsersByPersonaNames(mPlayerInfo,"");

		/*Blaze::PersonaNameList personaNameList;
		personaNameList.push_back("q7764733053-GBen");
		personaNameList.push_back("CGFUT19_04");
		personaNameList.push_back("dr_hoo");
		personaNameList.push_back("CGFUT19_02");
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		personaNameList.pop_back();
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		personaNameList.push_back("CGFUT19_02");
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		personaNameList.pop_back();
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		personaNameList.push_back("CGFUT19_02");
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		setSeasonsData(mPlayerInfo);
*/
	}
	if (!strcmp("gameType74", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str()))
	{//added as per logs couch play
		if (StressConfigInst.getPlatform() == Platform::PLATFORM_XONE)
		{
			updateHardwareFlags(mPlayerInfo);
		}
		
		// Calling it for 5% of player as stats db is not much sofisticated to handle all players load.
		if (Random::getRandomNumber(100) < 100)
		{
			updateStats();
		}
		/*Blaze::PersonaNameList personaNameList;
		personaNameList.push_back("q7764733053-GBen");
		personaNameList.push_back("CGFUT19_04");
		personaNameList.push_back("dr_hoo");
		personaNameList.push_back("CGFUT19_02");
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		personaNameList.pop_back();
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		personaNameList.push_back("CGFUT19_02");
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		personaNameList.pop_back();
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		personaNameList.push_back("CGFUT19_02");
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);*/
	}
	Blaze::Clubs::GetMembersResponse getMemberResponse;
	//uint32_t clubID = getMyClubID(mPlayerInfo);
	//if (clubID > 0)
	//{
	//	getMembers(mPlayerInfo, clubID, 50, getMemberResponse); // Need to check the ClubId
	//}
	
	if (!strcmp("gameType100", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str()))
	{
		//getPetitions(mPlayerInfo, clubID, Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);
	}
	if (!strcmp("gameType84", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str())) {
		lookupUsersByPersonaNames(mPlayerInfo, "");
		lookupUsersByPersonaNames(mPlayerInfo, "");
		//getStatGroup(mPlayerInfo, "VProPosition");
		//getStatsByGroupAsync(mPlayerInfo, "VProPosition");
		// supply friend list fetched from getMembers for getStatsByGroupAsync below
		// to correctly replace getStatsByGroupAsync written above
		//Blaze::BlazeIdList friendList;
		//getStatsByGroupAsync(mPlayerInfo, friendList, "VProPosition");
	}
	
	sleepRandomTime(5000, 10000);
	
	setConnectionState(mPlayerInfo);
	//lookupRandomUsersByPersonaNames(mPlayerInfo);	
	
	/*if (strcmp("gameType74", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str())) {
		Blaze::PersonaNameList personaNameList;
		personaNameList.push_back("fut19mrtin");
		personaNameList.push_back("q7ca7112d23-USen");
		personaNameList.push_back("qc125233cf2-USen");
		personaNameList.push_back("qace59768af-GBen");
		lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
	} else if (strcmp("gameType84", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str())) {
		for (int i = 0; i < 11; i++)
		{
			lookupUsersByPersonaNames(mPlayerInfo, "dr_hoo");
		}
	}
	else {*/
	if(strcmp("gameType74", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str())&& strcmp("gameType84", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str()))
	{// if it is not singleplayer draft or couchplay
		lookupUsersByPersonaNames(mPlayerInfo, "");
		lookupUsersByPersonaNames(mPlayerInfo, "");
		lookupUsersByPersonaNames(mPlayerInfo, "");
		lookupUsersByPersonaNames(mPlayerInfo, "");
	}
	
	if (!strcmp("gameType21", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str())) 
	{
		//getStatGroup(mPlayerInfo, "SkillGameStats");

		//Blaze::Stats::GetStatsByGroupRequest::EntityIdList entityList;
		//entityList.push_back(mPlayerInfo->getPlayerId());
		EntityId id = 86;
		//getStatsByGroupAsync(mPlayerInfo, "SkillGameStats", entityList, "skillgame", id, 0);

		//const char8_t* bName = ("SkillGame" + std::to_string(id)).c_str();
		//getLeaderboardGroup(mPlayerInfo, -1, bName);
		//getCenteredLeaderboard(mPlayerInfo, 0, 100, mPlayerInfo->getPlayerId(), bName);

		if (StressConfigInst.getPTSEnabled())
		{
			// Submit Offline Stats
			if ((uint32_t)Random::getRandomNumber(100) < StressConfigInst.getOfflineStatsProbability())
			{
				submitOfflineStats(mPlayerInfo);
			}

			// Update Eadp Stats(Read and Update Limits)
			if ((uint32_t)Random::getRandomNumber(100) < StressConfigInst.getEadpStatsProbability())
			{
				updateEadpStats(mPlayerInfo);
			}
		}

		int64_t gameLifeSpan = StressConfigInst.getSingleplayerGamePlayConfig()->RoundInGameDuration;
		int64_t gameTime = gameLifeSpan + (StressConfigInst.getSingleplayerGamePlayConfig()->InGameDeviation > 0 ? (rand() % StressConfigInst.getSingleplayerGamePlayConfig()->InGameDeviation) : 0);
		mIsInSinglePlayerGame = true;

		// PlayerTimeStats RPC call
		if (StressConfigInst.getPTSEnabled())
		{
			incrementMatchPlayed(mPlayerInfo);
		}

		for (int i = 0; i < 4; i++)
		{
			while (mIsInSinglePlayerGame)
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SinglePlayer::simulateLoad]: " << mPlayerInfo->getPlayerId() << " IN_GAME state ");
				if (!mPlayerInfo->isConnected())
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[SinglePlayer::SinglePlayerGame::User Disconnected = " << mPlayerInfo->getPlayerId());
					return ERR_DISCONNECTED;
				}
				//int count = (int)Random::getRandomNumber(10);
				for (int val = 0; val < 35; val++) // Hard coding for now to check CPU usage
				{
					if (StressConfigInst.isGetStatsEnabled())
					{
						BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][SinglePlayer::simulateLoad]: isGetStatsEnabled = true");
						if ((int)Random::getRandomNumber(100) < 100)	//5
						{
							//STATS
							statsExecutions();
							sleepRandomTime(5000, 10000);
						}
						if ((int)Random::getRandomNumber(100) < 100) //80 :500K - 52K QPS
						{
							Blaze::PersonaNameList personaNameList;
							for (int k = 0; k < 10; k++)
							{
								personaNameList.push_back(StressConfigInst.getRandomPersonaNames());
							}
							lookupUsersByPersonaNames(mPlayerInfo, personaNameList);
						}
						//SLEEP 
						sleepRandomTime(5000, 30000);
					}
				}
				
				//   if user game time has elapsed, send offline report and start next game cycle
				TimeValue elpasedTime = TimeValue::getTimeOfDay() - mSinglePlayerGameStartTime;
				//   game over
				if (elpasedTime.getMillis() >= gameTime || RollDice(SP_CONFIG.QuitFromGameProbability))
				{
					BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "Offline report will be sent");
					//userSettingsSave(mPlayerInfo, "cust");

					submitSkillGameReport((uint16_t)id);
					lookupUsersByPersonaNames(mPlayerInfo, "");
					getStatGroup(mPlayerInfo, "SkillGameStats");
					Blaze::Stats::GetStatsByGroupRequest::EntityIdList entityList;
					entityList.push_back(mPlayerInfo->getPlayerId());
					getStatsByGroupAsync(mPlayerInfo, "SkillGameStats", entityList, "skillgame", id, 0);
					lookupUsersByPersonaNames(mPlayerInfo, "");
					const char8_t* bbName = ("SkillGame" + std::to_string(id)).c_str();
					getLeaderboardGroup(mPlayerInfo, -1, bbName);
					getCenteredLeaderboard(mPlayerInfo, 0, 100, mPlayerInfo->getPlayerId(), bbName);
					break;
				}
			}
			id++;
		}
	}
	else {
		int64_t gameLifeSpan = StressConfigInst.getSingleplayerGamePlayConfig()->RoundInGameDuration;
		int64_t gameTime = gameLifeSpan + (StressConfigInst.getSingleplayerGamePlayConfig()->InGameDeviation > 0 ? (rand() % StressConfigInst.getSingleplayerGamePlayConfig()->InGameDeviation) : 0);

		setClientState(mPlayerInfo, ClientState::MODE_SINGLE_PLAYER, ClientState::STATUS_SUSPENDED);
		unsubscribeFromCensusData(mPlayerInfo); // added according to new logs
		mIsInSinglePlayerGame = true;
		if (StressConfigInst.getPTSEnabled())
		{
			//if ((uint32_t)Random::getRandomNumber(100) < StressConfigInst.getEadpStatsProbability())
			//{
			incrementMatchPlayed(mPlayerInfo);
			//}
		}
		mSinglePlayerGameStartTime = TimeValue::getTimeOfDay();
		
		if (StressConfigInst.getPTSEnabled())
		{
			// Submit Offline Stats
			if ((uint32_t)Random::getRandomNumber(100) < StressConfigInst.getOfflineStatsProbability())
			{
				submitOfflineStats(mPlayerInfo);
			}

			// Update Eadp Stats(Read and Update Limits)
			if ((uint32_t)Random::getRandomNumber(100) < StressConfigInst.getEadpStatsProbability())
			{
				updateEadpStats(mPlayerInfo);
			}
		}

		while (mIsInSinglePlayerGame)
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SinglePlayer::simulateLoad]: " << mPlayerInfo->getPlayerId() << " IN_GAME state ");
			if (!mPlayerInfo->isConnected())
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[SinglePlayer::SinglePlayerGame::User Disconnected = " << mPlayerInfo->getPlayerId());
				return ERR_DISCONNECTED;
			}
			//SLEEP 
			sleepRandomTime(30000, 60000);

			//   if user game time has elapsed, send offline report and start next game cycle
			TimeValue elpasedTime = TimeValue::getTimeOfDay() - mSinglePlayerGameStartTime;
			//   game over
			if (elpasedTime.getMillis() >= gameTime || RollDice(SP_CONFIG.QuitFromGameProbability))
			{
				BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "Offline report will be sent");
				//userSettingsSave(mPlayerInfo, "cust");

				submitOfflineGameReport();
				mSinglePlayerGameStartTime = TimeValue::getTimeOfDay();
				break;
			}
			
		}
	}
	  
	return ERR_OK;
}

uint16_t SinglePlayer::viewStats()
{
	char8_t bName[20];
	uint16_t skillGameNum = (uint16_t)Blaze::Random::getRandomNumber(50);
	if (skillGameNum == 0) {
        skillGameNum++;
    }
    sprintf(bName, "SkillGame%d", skillGameNum);
	
    getStatGroup(mPlayerInfo, "SkillGameStats");
	getKeyScopesMap(mPlayerInfo);
   // getStatsByGroupAsync(mPlayerInfo, "SkillGameStats");
	getLeaderboardGroup(mPlayerInfo,-1,bName);
	getCenteredLeaderboard(mPlayerInfo, 0, 100, mPlayerInfo->getPlayerId(), bName);
	return skillGameNum;
}




BlazeRpcError SinglePlayer::submitOfflineGameReport()
{
	Blaze::BlazeRpcError err = ERR_OK;
	uint16_t homesubNum = (uint16_t)Blaze::Random::getRandomNumber(10);
    Blaze::GameReporting::OSDKGameReportBase::OSDKPlayerReport* playerReport = NULL;
    Blaze::GameReporting::Fifa::SoloPlayerReport *soloPlayerReport = NULL;
    Blaze::GameReporting::Fifa::SoloGameReport* soloGameReport = NULL;
	Blaze::GameReporting::Fifa::Substitution *substitution = NULL;
    Blaze::GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = NULL;
		
    GameReporting::SubmitGameReportRequest request;

	if (!strcmp("gameType90", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str())
		|| !strcmp("gameType84", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str())) {
		//|| !strcmp("gameType95", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str())) {
		uint16_t skillGameNum = uint16_t(Random::getRandomNumber(100)) + 1;
		if (skillGameNum < 50) {
			//mPlaySkillGame = true ;
			submitSkillGameReport(skillGameNum);
			viewStats();
		}

	}
    // set status of FNSH
    request.setFinishedStatus(Blaze::GameReporting::GAMEREPORT_FINISHED_STATUS_DEFAULT);
    // get gamereport object which needs to be filled with game data
    GameReporting::GameReport &gamereport =   request.getGameReport();
    // set GRID
    gamereport.setGameReportingId(0);
    soloGameReport = BLAZE_NEW Blaze::GameReporting::Fifa::SoloGameReport;
    if (soloGameReport == NULL) {
        BLAZE_ERR(BlazeRpcLog::gamemanager, "[OfflineGameReportInstance]startgamereporting function call failed, failed to allocate memory for variable soloGameReport");
        return ERR_SYSTEM;
    }
    osdkReport = BLAZE_NEW Blaze::GameReporting::OSDKGameReportBase::OSDKReport;
	substitution = BLAZE_NEW Blaze::GameReporting::Fifa::Substitution;
    if (osdkReport == NULL) {
        BLAZE_ERR(BlazeRpcLog::gamemanager, "[OfflineGameReportInstance]startgamereporting function call failed, failed to allocate memory for variable osdkReport");
        return ERR_SYSTEM;
    }
	if (substitution == NULL) {
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[OfflineGameReportInstance]startgamereporting function call failed, failed to allocate memory for variable substitution");
		return ERR_SYSTEM;
	}
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "MOMO : inside gamereport : memory alloc successful");
    // Game report contains osdk report, adding osdkreport to game report
    gamereport.setReport(*osdkReport);
    // fill the osdkgamereport fields
    Blaze::GameReporting::OSDKGameReportBase::OSDKGameReport& osdkGamereport = osdkReport->getGameReport();
    // GTIM
    osdkGamereport.setGameTime(52225);
    // set GTYP && TYPE
	if( mOfflineGameType != "\0")
	{
		gamereport.setGameReportName(mOfflineGameType.c_str());
		// STUS
		osdkGamereport.setGameType(mOfflineGameType.c_str());
	}
	else
	{
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[OfflineGameReportInstance]Invalid Game type encountered.");
		return ERR_SYSTEM;
	}
	GameManager::PlayerId playerId = mPlayerInfo->getPlayerId();
    osdkGamereport.setFinishedStatus(rand() % 10);
    osdkGamereport.setArenaChallengeId(rand() % 10);
    osdkGamereport.setCategoryId(rand() % 12);
    osdkGamereport.setGameReportId(playerId % 10);
    osdkGamereport.setSimulate(false);
    osdkGamereport.setLeagueId(rand() % 10);
    osdkGamereport.setRank(false);
    osdkGamereport.setRoomId(rand() % 10);
    // osdkGamereport.setSponsoredEventId(0);
    //osdkGamereport.setFinishedStatus(1);
    // For now updating values as per client logs.TODO: need to add values according to server side settings
    soloGameReport->setOpponentTeamId(rand() % 100);
    soloGameReport->setMatchDifficulty(4);
    soloGameReport->setProfileDifficulty(rand() % 12);

    // fill the common game report value
    Blaze::GameReporting::Fifa::CommonGameReport& commonGameReport = soloGameReport->getCommonGameReport();
    // WNPK
    commonGameReport.setWentToPk(0);
    commonGameReport.setIsRivalMatch(false);
    // For now updating values as per client logs.TODO: need to add values according to server side settings
    commonGameReport.setBallId(0);
    commonGameReport.setAwayKitId(905);
    commonGameReport.setHomeKitId(1410);
    commonGameReport.setStadiumId(32);
    const int MAX_TEAM_COUNT = 11;
    const int MAX_PLAYER_VALUE = 100000;
	substitution->setPlayerSentOff(Blaze::Random::getRandomNumber(250000));
	substitution->setPlayerSubbedIn(Blaze::Random::getRandomNumber(250000));
	substitution->setSubTime(56);
	if (homesubNum > 2)
	{
		if (StressConfigInst.getPlatform() == Platform::PLATFORM_XONE)// &&((!strcmp("gameType84", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str())) ||!(strcmp("gameType74", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str()))))
		{
			//if it is xone and either sp draft or couch play then dont push into awaysublist
			if (mIsFutPlayer)
			{
				commonGameReport.getAwaySubList().push_back(substitution);
				Blaze::GameReporting::Fifa::Substitution *substitution1 = BLAZE_NEW Blaze::GameReporting::Fifa::Substitution;
				substitution1->setPlayerSentOff(Blaze::Random::getRandomNumber(250000));
				substitution1->setPlayerSubbedIn(Blaze::Random::getRandomNumber(250000));
				substitution1->setSubTime(56);
				commonGameReport.getAwaySubList().push_back(substitution1);
				if (mOfflineGameType.compare("gameType74") == 0)
				{
					Blaze::GameReporting::Fifa::Substitution *substitution2 = BLAZE_NEW Blaze::GameReporting::Fifa::Substitution;
					substitution2->setPlayerSentOff(Blaze::Random::getRandomNumber(250000));
					substitution2->setPlayerSubbedIn(Blaze::Random::getRandomNumber(250000));
					substitution2->setSubTime(56);
					commonGameReport.getAwaySubList().push_back(substitution2);
				}
			}
		}
		//else
		//{
		//	if (!(StressConfigInst.getPlatform() == Platform::PLATFORM_PS4 && (!strcmp("gameType78", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str()))))
		//	{//if it is not gametype78 in ps4
		//		commonGameReport.getAwaySubList().push_back(substitution);
		//	}
		//	else
		//	{
		//		commonGameReport.getHomeSubList().push_back(substitution);
		//	}
		//}
	}
    // fill awayStartingXI and homeStartingXI values here
    for (int iteration = 0 ; iteration < MAX_TEAM_COUNT ; iteration++) {
        commonGameReport.getAwayStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
    }
    for (int iteration = 0 ; iteration < MAX_TEAM_COUNT ; iteration++) {
        commonGameReport.getHomeStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
    }
    osdkGamereport.setCustomGameReport(*soloGameReport);
    // populate player data in playermap
    // OsdkPlayerReportsMap = BLAZE_NEW Blaze::GameReporting::OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap; // = osdkReport->getPlayerReports();
    Blaze::GameReporting::OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = osdkReport->getPlayerReports();
    OsdkPlayerReportsMap.reserve(1);
    //GameManager::PlayerId playerId = mPlayerInfo->getPlayerId();
    playerReport = OsdkPlayerReportsMap.allocate_element();
    soloPlayerReport = BLAZE_NEW Blaze::GameReporting::Fifa::SoloPlayerReport;
    if (soloPlayerReport == NULL) {
        BLAZE_ERR(BlazeRpcLog::gamemanager, "[OfflineGameReportInstance]startgamereporting function call failed, failed to allocate memory for variable soloPlayerReport");
        return ERR_SYSTEM;
    }
    Blaze::GameReporting::Fifa::CommonPlayerReport& commonPlayerReport = soloPlayerReport->getCommonPlayerReport();
    Blaze::GameReporting::Fifa::SoloCustomPlayerData& soloCustomPlayerData = soloPlayerReport->getSoloCustomPlayerData();
	Blaze::GameReporting::Fifa::SeasonalPlayData& seasonalPlayData = soloPlayerReport->getSeasonalPlayData();

    // set the commonplayer report attribute values
    commonPlayerReport.setAssists(rand() % 10);
    commonPlayerReport.setGoalsConceded(rand() % 12);
	commonPlayerReport.setHasCleanSheets(rand() % 10); // newly added FIFA 20
    commonPlayerReport.setCorners(rand() % 10);
    commonPlayerReport.setPassAttempts(rand() % 15);
    commonPlayerReport.setPassesMade(rand() % 12);
    commonPlayerReport.setRating((float)(rand() % 10));
    commonPlayerReport.setSaves(rand() % 12);
    commonPlayerReport.setShots(rand() % 12);
    commonPlayerReport.setTackleAttempts(rand() % 12);
    commonPlayerReport.setTacklesMade(rand() % 10);
    commonPlayerReport.setFouls(rand() % 10);
    commonPlayerReport.setGoals(rand() % 10);
    commonPlayerReport.setInterceptions(rand() % 10);  // Added newly
	commonPlayerReport.setHasMOTM(0); // newly added FIFA 20
    commonPlayerReport.setOffsides(rand() % 10);
    commonPlayerReport.setOwnGoals(rand() % 10);
    commonPlayerReport.setPkGoals(rand() % 12);
    commonPlayerReport.setPossession(rand() % 10);
    commonPlayerReport.setRedCard(rand() % 10);
    commonPlayerReport.setShotsOnGoal(rand() % 10);
    commonPlayerReport.setUnadjustedScore(rand() % 10);  // Added newly
    commonPlayerReport.setYellowCard(rand() % 10);
	commonPlayerReport.getCommondetailreport().setAveragePossessionLength((uint16_t)Random::getRandomNumber(50));
	commonPlayerReport.getCommondetailreport().setAverageFatigueAfterNinety((uint16_t)Random::getRandomNumber(9)); // newly added FIFA 20
	commonPlayerReport.getCommondetailreport().setInjuriesRed(0);
	commonPlayerReport.getCommondetailreport().setPassesIntercepted(rand() % 10);
	commonPlayerReport.getCommondetailreport().setPenaltiesAwarded(rand() % 10);
	commonPlayerReport.getCommondetailreport().setPenaltiesMissed(rand() % 10);
	commonPlayerReport.getCommondetailreport().setPenaltiesScored(rand() % 10);
	commonPlayerReport.getCommondetailreport().setPenaltiesSaved(rand() % 10);


    soloCustomPlayerData.setGoalAgainst(rand() % 12);
    soloCustomPlayerData.setLosses(rand() % 10);  // Added newly
	soloCustomPlayerData.setOppGoal(rand() % 10); // newly added FIFA 20
    soloCustomPlayerData.setResult(rand() % 12);  // Added newly
    soloCustomPlayerData.setShotsAgainst(rand() % 10);
    soloCustomPlayerData.setShotsAgainstOnGoal(rand() % 12);
    soloCustomPlayerData.setTeam(rand() % 10);
    soloCustomPlayerData.setTies(rand() % 10);  // Added newly
    soloCustomPlayerData.setWins(rand() % 10);  // Added newly


	seasonalPlayData.setCurrentDivision(rand() % 10);
	seasonalPlayData.setCup_id(rand() % 10);
	seasonalPlayData.setUpdateDivision(Blaze::GameReporting::Fifa::CompetitionStatus::NO_UPDATE);
	seasonalPlayData.setGameNumber(rand() % 10);
	seasonalPlayData.setSeasonId(rand() % 10);
	seasonalPlayData.setWonCompetition(rand() % 10);
	seasonalPlayData.setWonLeagueTitle(false);

    playerReport->setCustomDnf(rand() % 10);
    playerReport->setScore(rand() % 10);
    playerReport->setAccountCountry(0);
    playerReport->setFinishReason(rand() % 10);
    playerReport->setGameResult(rand() % 10);
    playerReport->setHome(false);
	playerReport->setLosses(rand() % 10);
    playerReport->setName(mPlayerInfo->getPersonaName().c_str());
    playerReport->setOpponentCount(rand() % 10);
    playerReport->setExternalId(0);
	playerReport->setNucleusId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
    playerReport->setPersona(mPlayerInfo->getPersonaName().c_str());
    playerReport->setPointsAgainst(rand() % 10);
    playerReport->setUserResult(rand() % 10);
    playerReport->setClientScore(rand() % 10);
    // playerReport->setSponsoredEventAccountRegion(0);
    playerReport->setSkill(rand() % 10);
    playerReport->setSkillPoints(rand() % 10);
    playerReport->setTeam(rand() % 10);
    playerReport->setTies(rand() % 10);
    playerReport->setWinnerByDnf(rand() % 10);
    playerReport->setWins(rand() % 10);
	playerReport->setCustomPlayerReport(*soloPlayerReport);
	
	GameReporting::OSDKGameReportBase::IntegerAttributeMap& privateAttrMap = playerReport->getPrivatePlayerReport().getPrivateIntAttributeMap();
	privateAttrMap["aiAst"] = (uint16_t)Random::getRandomNumber(10);
	privateAttrMap["aiCS"] = (uint16_t)Random::getRandomNumber(10);
	privateAttrMap["aiCo"] = (uint16_t)Random::getRandomNumber(10);
	privateAttrMap["aiFou"] = (uint16_t)Random::getRandomNumber(10);
	privateAttrMap["aiGC"] = (uint16_t)Random::getRandomNumber(10);
	privateAttrMap["aiGo"] = (uint16_t)Random::getRandomNumber(10);
	privateAttrMap["aiInt"] = (uint16_t)Random::getRandomNumber(10);
	privateAttrMap["aiMOTM"] = (uint16_t)Random::getRandomNumber(10);
	privateAttrMap["aiOG"] = (uint16_t)Random::getRandomNumber(10);
	privateAttrMap["aiOff"] = (uint16_t)Random::getRandomNumber(10);
	privateAttrMap["aiPA"] = (uint16_t)Random::getRandomNumber(10);
	privateAttrMap["aiPKG"] = (uint16_t)Random::getRandomNumber(10);
	privateAttrMap["aiPM"] = (uint16_t)Random::getRandomNumber(10);
	privateAttrMap["aiPo"] = (uint16_t)Random::getRandomNumber(100);
	privateAttrMap["aiRC"] = (uint16_t)Random::getRandomNumber(10);
	privateAttrMap["aiRa"] = (uint16_t)Random::getRandomNumber(100);
	privateAttrMap["aiSav"] = (uint16_t)Random::getRandomNumber(100);
	privateAttrMap["aiSoG"] = (uint16_t)Random::getRandomNumber(100);
	privateAttrMap["aiTA"] = (uint16_t)Random::getRandomNumber(100);
	privateAttrMap["aiTM"] = (uint16_t)Random::getRandomNumber(100);
	privateAttrMap["aiYC"] = (uint16_t)Random::getRandomNumber(100);
	
	//if (isFUTPlayer()) {
		if(mIsFutPlayer)	//strcmp("gameType78", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str())&& strcmp("gameType95", mOfflineGameType.substr(0, (mOfflineGameType.length())).c_str()))
		{
			privateAttrMap["futMatchResult"] = (int64_t)Random::getRandomNumber(3) + 1;
			privateAttrMap["futMatchTime"] = (int64_t)1488800000 + (int64_t)Random::getRandomNumber(100000);
		}
	//}
	privateAttrMap["gid"] = (int64_t)1488800000 + (int64_t)Random::getRandomNumber(100000);

    //if (mOwner->getIsFutPlayer()) {
    //    // adding pirvate player report for FUT scenarios.
    //    Blaze::GameReporting::OSDKGameReportBase::OSDKPrivatePlayerReport& osdkPrivatePlayerReport = playerReport->getPrivatePlayerReport();
    //    Blaze::GameReporting::OSDKGameReportBase::IntegerAttributeMap& integerAttributeMap = osdkPrivatePlayerReport.getPrivateIntAttributeMap();
    //    integerAttributeMap["futMatchResult"] = rand() % 12;
    //    integerAttributeMap["futMatchTime"] = osdkGamereport.getGameTime();
    //}
    OsdkPlayerReportsMap[playerId] = playerReport;

	//char8_t buf[10240];
	//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "-> [submitofflinegamereport][" << mPlayerInfo->getPlayerId() << "]::submitofflinegamereport" << "\n" << request.print(buf, sizeof(buf)));
	
    // submit offline game report
	err = mPlayerInfo->getComponentProxy()->mGameReportingProxy->submitOfflineGameReport(request);
    if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "submitofflinegamereport failed. Error(" << ErrorHelp::getErrorName(err) << ")");
        return err;
    }
	BLAZE_INFO(BlazeRpcLog::gamemanager, "[offlinegamereporting][%d]offline game report for game type submitted successfully", mPlayerInfo->getIdent());
    return err;
}

void SinglePlayer::removePlayerFromList()
{
    //  remove player from universal list first
    Player::removePlayerFromList();
    EA::Thread::AutoMutex am(singlePlayerListMutex);
    const UserIdentMap::iterator plyerItr = singlePlayerList.find(mPlayerInfo->getPlayerId());
    if (plyerItr != singlePlayerList.end())
    {
        singlePlayerList.erase(plyerItr);
    }
}

//   This function would be called from StressInstance::login()
void SinglePlayer::onLogin(Blaze::BlazeRpcError result)
{
    if (result != ERR_OK)
    {
        if (AUTH_ERR_EXISTS == result)
        {
            BLAZE_ERR(BlazeRpcLog::gamemanager, "::onLogin: login failed. Error = %s", ErrorHelp::getErrorName(result));
        }
        return;
    }
    mPlayerInfo->setViewId(0);
}

void SinglePlayer::addPlayerToList()
{
    //  add player to universal list first
    Player::addPlayerToList();
    UserIdentPtr uid(BLAZE_NEW UserIdentification());
    uid->setName(mPlayerInfo->getPersonaName().c_str());
    uid->setBlazeId(mPlayerInfo->getPlayerId());
    {
        EA::Thread::AutoMutex am(singlePlayerListMutex);
        singlePlayerList.insert(eastl::pair<BlazeId, UserIdentPtr>(mPlayerInfo->getPlayerId(), uid));
    }
}

}  // namespace Stress
}  // namespace Blaze
