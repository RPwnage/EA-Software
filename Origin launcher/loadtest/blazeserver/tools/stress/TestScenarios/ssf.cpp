//  *************************************************************************************************
//
//   File:    ssf.cpp
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************
#include "ssf.h"
#include "playermodule.h"
#include "gamesession.h"
#include "framework/blaze.h"
#include "customcode/component/gamereporting/fifa/tdf/ssfseasonsreport.h"

namespace Blaze {
namespace Stress {

SSFPlayer::SSFPlayer(StressPlayerInfo* playerData)
	: GamePlayUser(playerData), mConfig(SSF_GAMEPLAY_CONFIG), mPlayerData(playerData)
{

}
SSFPlayer::~SSFPlayer()
{

}
EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> fillSkillEvent(int dribblepast,int count,int skilltype)
{
	EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent1 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent();

	skillEvent1->setDribblePast((uint16_t)dribblepast);
	skillEvent1->setCount((uint16_t)count);
	skillEvent1->setSkillType(skilltype);
	return skillEvent1;
}

BlazeRpcError SSFPlayer::simulateLoad()
{
	BlazeRpcError err = ERR_OK;
	//bool result = false;
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SSF::simulateLoad]:" << mPlayerInfo->getPlayerId());

	mScenarioType = StressConfigInst.getRandomScenarioByPlayerType(SSFPLAYER);
	const char8_t* gameType = NULL;
	if (!mConfig)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][SSF::simulateLoad]: Failed to Simulate Load, Configuration is not loaded");
		sleepRandomTime(30000, 60000);
		return ERR_SYSTEM;
	}
	switch (mScenarioType)
	{
		/*case SSF_EVENT_MATCH:
		{
			gameType = "gameType";
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SSF Players::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type=" << SSF_EVENT_MATCH);
		}
		break;*/
		case SSF_WORLD_TOUR:
		{
			gameType = "gameType30";
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SSF Players::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type=" << SSF_WORLD_TOUR);
		}
		break;
		case SSF_TWIST_MATCH:
		{
			gameType = "gameType29";
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SSF Players::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type=" << SSF_TWIST_MATCH);
		}
		break;
		case SSF_SINGLE_MATCH:
		{
			gameType = "gameType30";
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SSF Players::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type=" << SSF_SINGLE_MATCH);
		}
		break;
		case SSF_SOLO_MATCH:
		{
			gameType = "gameType34";
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SSF Players::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type=" << SSF_TWIST_MATCH);
		}
		break;

		//case SSF_SKILL_GAMES:
		//{
		//	//gameType = "gameType";
		//	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SSF Players::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type=" << SSF_SKILL_GAMES);
		//}
		//break;
		default:
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SSF Players::simulateLoad]" << mPlayerData->getPlayerId() << " with scenario type=" << "Default");
			return err;
		}
	}

	simulateMenuFlow();
	
	int64_t gameLifeSpan = StressConfigInst.getSSFPlayerGamePlayConfig()->roundInGameDuration;
	int64_t gameTime = gameLifeSpan + (StressConfigInst.getSSFPlayerGamePlayConfig()->inGameDeviation > 0 ? (rand() % StressConfigInst.getSSFPlayerGamePlayConfig()->inGameDeviation) : 0);
	mIsInSSFPlayerGame = true;
	mSSFPlayerGameStartTime = TimeValue::getTimeOfDay();

	// PlayerTimeStats RPC call
	if (StressConfigInst.getPTSEnabled())
	{
		incrementMatchPlayed(mPlayerInfo);
	}

	while (mIsInSSFPlayerGame)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[SSFPlayer::simulateLoad]: " << mPlayerInfo->getPlayerId() << " IN_GAME state ");
		if (!mPlayerInfo->isConnected())
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[SSFPlayer::SSFPlayerGame::User Disconnected = " << mPlayerInfo->getPlayerId());
			return ERR_DISCONNECTED;
		}

		// If user game time has elapsed, send offline report and start next game cycle
		TimeValue elpasedTime = TimeValue::getTimeOfDay() - mSSFPlayerGameStartTime;

		// game over
		if (elpasedTime.getMillis() >= gameTime || RollDice(SP_CONFIG.QuitFromGameProbability))
		{
			BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "Offline report will be sent");
			//userSettingsSave(mPlayerInfo, "cust");
			sleep(60000);
			
			eastl::string personaId = eastl::string().sprintf("%" PRIu64 "", mPlayerInfo->getPlayerId()).c_str();
			if (mPlayerInfo->getPlayerId() != (GameManager::PlayerId)0 || personaId != "0")
			{
				submitOfflineGameReport(gameType);
			}

			mSSFPlayerGameStartTime = TimeValue::getTimeOfDay();
			sleep(60000);
			break;
		}
	}
	return ERR_OK;
}

BlazeRpcError SSFPlayer::submitOfflineGameReport(const char8_t* gametype)
{
	Blaze::BlazeRpcError err = ERR_OK;
	//uint16_t homesubNum = (uint16_t)Blaze::Random::getRandomNumber(10);
	Blaze::GameReporting::OSDKGameReportBase::OSDKPlayerReport* playerReport = NULL;
	Blaze::GameReporting::Fifa::SoloPlayerReport *soloPlayerReport = NULL;
	//Blaze::GameReporting::Fifa::SoloGameReport* soloGameReport = NULL;
	Blaze::GameReporting::SSFSeasonsReportBase::SSFSeasonsGameReport* ssfSeasonsGameReport = NULL;
	Blaze::GameReporting::SSFSeasonsReportBase::SSFSeasonsPlayerReport* ssfPlayerReport = NULL;
	//Blaze::GameReporting::Fifa::Substitution *substitution = NULL;
	Blaze::GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = NULL;
	Blaze::GameReporting::SSFSeasonsReportBase::SSFTeamSummaryReport *ssfTeamSummaryReport = NULL;

	GameReporting::SubmitGameReportRequest request;

	// Set status of FNSH
	request.setFinishedStatus(Blaze::GameReporting::GAMEREPORT_FINISHED_STATUS_DEFAULT);

	// Get gamereport object which needs to be filled with game data
	GameReporting::GameReport &gamereport = request.getGameReport();

	// Set GRID
	gamereport.setGameReportingId(0);
	//soloGameReport = BLAZE_NEW Blaze::GameReporting::Fifa::SoloGameReport;
	ssfSeasonsGameReport = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SSFSeasonsGameReport;
	if (ssfSeasonsGameReport == NULL) {
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[SSFGameReportInstance] startgamereporting function call failed, failed to allocate memory for variable soloGameReport");
		return ERR_SYSTEM;
	}
	osdkReport = BLAZE_NEW Blaze::GameReporting::OSDKGameReportBase::OSDKReport;
	//substitution = BLAZE_NEW Blaze::GameReporting::Fifa::Substitution;
	if (osdkReport == NULL) {
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[SSFGameReportInstance] startgamereporting function call failed, failed to allocate memory for variable osdkReport");
		return ERR_SYSTEM;
	}
	/*if (substitution == NULL) {
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[SSFGameReportInstance] startgamereporting function call failed, failed to allocate memory for variable substitution");
		return ERR_SYSTEM;
	}*/
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "MOMO : inside gamereport : memory alloc successful");

	// Game report contains osdk report, adding osdkreport to game report
	gamereport.setReport(*osdkReport);
	gamereport.setGameReportName(gametype);

	// Fill the osdkgamereport fields
	Blaze::GameReporting::OSDKGameReportBase::OSDKGameReport& osdkGamereport = osdkReport->getGameReport();
	//Blaze::GameReporting::OSDKGameReportBase::OSDKGameReport& osdkGamereport = osdkReport->getCustomReport();

	// GTIM
	osdkGamereport.setGameTime(360);
	osdkGamereport.setGameType(gametype);

	// Set GTYP && TYPE
	osdkGamereport.setFinishedStatus(0);
	osdkGamereport.setArenaChallengeId(0);
	osdkGamereport.setCategoryId(0);
	osdkGamereport.setGameReportId(0);
	osdkGamereport.setSimulate(false);
	osdkGamereport.setLeagueId(0);
	osdkGamereport.setRank(false);
	osdkGamereport.setRoomId(0);
	//osdkGamereport.setSponsoredEventId(0);
	osdkGamereport.setFinishedStatus(1);
	// For now updating values as per client logs.TODO: need to add values according to server side settings
	ssfSeasonsGameReport->setMom("");
	GameManager::PlayerId playerId = mPlayerInfo->getPlayerId();
	Blaze::GameReporting::SSFSeasonsReportBase::GoalEventVector& goalSummary = ssfSeasonsGameReport->getGoalSummary();
	Blaze::GameReporting::SSFSeasonsReportBase::GoalEvent goal0, goal1, goal2, goal3, goal4, goal5, goal6, goal7;
	
	for (int count = 0; count <= 7 || count < goalSummary.size(); count++)
	{
		if (mScenarioType == SSF_SINGLE_MATCH)
		{
			if (count == 0)
			{
				goal0.getPrimaryAssistFlags().clear();
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal0.getPrimaryAssist();
				avid1.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				avid1.setPlayerId(2);
				avid1.setSlotId(rand() % 10);
				goal0.setPrimaryAssistType(rand() % 10);
				goal0.getSecondaryAssistFlags().clear();
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal0.getSecondaryAssist();
				avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
				avid2.setPlayerId(0);
				avid2.setSlotId(rand() % 10);
				goal0.setSecondaryAssistType(rand() % 10);
				goal0.getGoalFlags().push_back(20);
				//goal0.getGoalFlags().push_back(20);
				goal0.setGoalEventTime(2);
				goal0.setGoalType(rand() % 10);
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal0.getScorer();
				avid3.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				avid3.setPlayerId(0);
				avid3.setSlotId(rand() % 10);
				goal0.setScoringTeam(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				goalSummary.push_back(&goal0);
			}
			else if (count == 1)
			{
				goal1.getPrimaryAssistFlags().push_back(0);
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal1.getPrimaryAssist();
				avid1.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				avid1.setPlayerId(1);
				avid1.setSlotId(rand() % 10);
				goal1.setPrimaryAssistType(rand() % 10);
				goal1.getSecondaryAssistFlags().clear();
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal1.getSecondaryAssist();
				avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
				avid2.setPlayerId(0);
				avid2.setSlotId(rand() % 10);
				goal1.setSecondaryAssistType(rand() % 10);
				goal1.getGoalFlags().push_back(20);
				goal1.setGoalEventTime(8);
				goal1.setGoalType(0);
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal1.getScorer();
				avid3.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				avid3.setPlayerId(2);
				avid3.setSlotId(2);
				goal1.setScoringTeam(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				goalSummary.push_back(&goal1);
			}
			else if (count == 2)
			{
				goal2.getPrimaryAssistFlags().push_back(0);
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal2.getPrimaryAssist();
				avid1.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				avid1.setPlayerId(2);
				avid1.setSlotId(rand() % 10);
				goal2.setPrimaryAssistType(rand() % 12);
				goal2.getSecondaryAssistFlags().clear();
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal2.getSecondaryAssist();
				avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
				avid2.setPlayerId(0);
				avid2.setSlotId(rand() % 10);
				goal2.setSecondaryAssistType(rand() % 10);
				goal2.getGoalFlags().push_back(20);
				goal2.setGoalEventTime(18);
				goal2.setGoalType(0);
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal2.getScorer();
				avid3.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				avid3.setPlayerId(1);
				avid3.setSlotId(rand() % 12);
				goal2.setScoringTeam(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				goalSummary.push_back(&goal2);
			}
			else if (count == 3)
			{
				goal3.getPrimaryAssistFlags().clear();
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal3.getPrimaryAssist();
				avid1.setPersonaId(0);
				avid1.setPlayerId(0);
				avid1.setSlotId(rand() % 10);
				goal3.setPrimaryAssistType(rand() % 10);
				goal3.getSecondaryAssistFlags().clear();
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal3.getSecondaryAssist();
				avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
				avid2.setPlayerId(0);
				avid2.setSlotId(rand() % 10);
				goal3.setSecondaryAssistType(rand() % 10);
				goal3.getGoalFlags().clear();
				goal3.setGoalEventTime(35);
				goal3.setGoalType(rand() % 10);
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal3.getScorer();
				avid3.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				avid3.setPlayerId(1);
				avid3.setSlotId(rand() % 10);
				goal3.setScoringTeam(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				goalSummary.push_back(&goal3);
			}
			else if (count == 4)
			{
				goal4.getPrimaryAssistFlags().push_back(0);
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal4.getPrimaryAssist();
				avid1.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				avid1.setPlayerId(0);
				avid1.setSlotId(rand() % 10);
				goal4.setPrimaryAssistType(rand() % 12);
				goal4.getSecondaryAssistFlags().clear();
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal4.getSecondaryAssist();
				avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
				avid2.setPlayerId(0);
				avid2.setSlotId(rand() % 10);
				goal4.setSecondaryAssistType(rand() % 10);
				goal4.getGoalFlags().push_back(20);
				goal4.setGoalEventTime(46);
				goal4.setGoalType(0);
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal4.getScorer();
				avid3.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				avid3.setPlayerId(2);
				avid3.setSlotId(rand() % 10);
				goal4.setScoringTeam(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				goalSummary.push_back(&goal4);
			}
		}
		else if (mScenarioType == SSF_WORLD_TOUR)
		{
			if (count == 0)
			{
				goal0.getPrimaryAssistFlags().clear();
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal0.getPrimaryAssist();
				//avid1.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				avid1.setPersonaId(0);
				avid1.setPlayerId(0);
				avid1.setSlotId(rand() % 10);
				goal0.setPrimaryAssistType(rand() % 10);
				goal0.getSecondaryAssistFlags().clear();
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal0.getSecondaryAssist();
				avid2.setPersonaId(rand() % 10);// getLogin()->getUserLoginInfo()->getBlazeId());
				avid2.setPlayerId(rand() % 10);
				avid2.setSlotId(rand() % 10);
				goal0.setSecondaryAssistType(rand() % 10);
				goal0.getGoalFlags().push_back(17);
				goal0.getGoalFlags().push_back(20);
				goal0.setGoalEventTime(51);
				goal0.setGoalType(0);
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal0.getScorer();
				avid3.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				avid3.setPlayerId(1);
				avid3.setSlotId(rand() % 10);
				goal0.setScoringTeam(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
				goalSummary.push_back(&goal0);
			}
			else if (count == 1)
			{
				goal1.getPrimaryAssistFlags().clear();
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal1.getPrimaryAssist();
				avid1.setPersonaId(0);
				avid1.setPlayerId(0);
				avid1.setSlotId(rand() % 10);
				goal1.setPrimaryAssistType(rand() % 10);
				goal1.getSecondaryAssistFlags().clear();
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal1.getSecondaryAssist();
				avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
				avid2.setPlayerId(0);
				avid2.setSlotId(rand() % 10);
				goal1.setSecondaryAssistType(rand() % 10);
				goal1.getGoalFlags().push_back(16);
				goal1.setGoalEventTime(60);
				goal1.setGoalType(0);
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal1.getScorer();
				avid3.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
				avid3.setPlayerId(4);
				avid3.setSlotId(0);
				goal1.setScoringTeam(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
				goalSummary.push_back(&goal1);
			}
			else if (count == 2)
			{
				goal2.getPrimaryAssistFlags().push_back(0);
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal2.getPrimaryAssist();
				avid1.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
				avid1.setPlayerId(1);
				avid1.setSlotId(rand() % 12);
				goal2.setPrimaryAssistType(1);
				goal2.getSecondaryAssistFlags().clear();
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal2.getSecondaryAssist();
				avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
				avid2.setPlayerId(0);
				avid2.setSlotId(rand() % 10);
				goal2.setSecondaryAssistType(rand() % 10);
				goal2.getGoalFlags().clear();
				goal2.setGoalEventTime(144);
				goal2.setGoalType(rand() % 10);
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal2.getScorer();
				avid3.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
				avid3.setPlayerId(4);
				avid3.setSlotId(rand() % 10);
				goal2.setScoringTeam(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
				goalSummary.push_back(&goal2);
			}
			else if (count == 3)
			{
				goal3.getPrimaryAssistFlags().push_back(0);
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal3.getPrimaryAssist();
				avid1.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
				avid1.setPlayerId(4);
				avid1.setSlotId(rand() % 10);
				goal3.setPrimaryAssistType(rand() % 10);
				goal3.getSecondaryAssistFlags().clear();
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal3.getSecondaryAssist();
				avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
				avid2.setPlayerId(0);
				avid2.setSlotId(rand() % 10);
				goal3.setSecondaryAssistType(rand() % 10);
				goal3.getGoalFlags().push_back(16);
				goal3.setGoalEventTime(166);
				goal3.setGoalType(0);
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal3.getScorer();
				avid3.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
				avid3.setPlayerId(2);
				avid3.setSlotId(4);
				goal3.setScoringTeam(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
				goalSummary.push_back(&goal3);
			}
			else if (count == 4)
			{
				goal4.getPrimaryAssistFlags().clear();
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal4.getPrimaryAssist();
				avid1.setPersonaId(0);
				avid1.setPlayerId(0);
				avid1.setSlotId(rand() % 10);
				goal4.setPrimaryAssistType(rand() % 10);
				goal4.getSecondaryAssistFlags().clear();
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal4.getSecondaryAssist();
				avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
				avid2.setPlayerId(0);
				avid2.setSlotId(rand() % 10);
				goal4.setSecondaryAssistType(rand() % 10);
				goal4.getGoalFlags().clear();
				goal4.setGoalEventTime(rand() % 76);
				goal4.setGoalType(rand() % 10);
				Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal4.getScorer();
				avid3.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
				avid3.setPlayerId(4);
				avid3.setSlotId(rand() % 10);
				goal4.setScoringTeam(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
				goalSummary.push_back(&goal4);
			}
			//else if (count == 5)
			//{
			//	goal5.getPrimaryAssistFlags().push_back(4);
			//	goal5.getPrimaryAssistFlags().push_back(10);
			//	Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal5.getPrimaryAssist();
			//	avid1.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
			//	avid1.setPlayerId(1);
			//	avid1.setSlotId(3);
			//	goal5.setPrimaryAssistType(1);
			//	goal5.getSecondaryAssistFlags().clear();
			//	Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal5.getSecondaryAssist();
			//	avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
			//	avid2.setPlayerId(0);
			//	avid2.setSlotId(0);
			//	goal5.setSecondaryAssistType(0);
			//	goal5.getGoalFlags().push_back(14);
			//	goal5.setGoalEventTime(308);
			//	goal5.setGoalType(0);
			//	Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal5.getScorer();
			//	avid3.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
			//	avid3.setPlayerId(2);
			//	avid3.setSlotId(1);
			//	goal5.setScoringTeam(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
			//	goalSummary.push_back(&goal5);
			//}
		}
		else if (mScenarioType == SSF_TWIST_MATCH)
		{
		if (count == 0)
		{
			goal0.getPrimaryAssistFlags().push_back(0);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal0.getPrimaryAssist();
			//avid1.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
			avid1.setPersonaId(0);
			avid1.setPlayerId(0);
			avid1.setSlotId(rand() % 76);
			goal0.setPrimaryAssistType(rand() % 10);
			goal0.getSecondaryAssistFlags().clear();
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal0.getSecondaryAssist();
			avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
			avid2.setPlayerId(0);
			avid2.setSlotId(0);
			goal0.setSecondaryAssistType(rand() % 10);
			goal0.getGoalFlags().push_back(17);
			//goal0.getGoalFlags().push_back(20);
			goal0.setGoalEventTime(10);
			goal0.setGoalType(rand() % 10);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal0.getScorer();
			avid3.setPersonaId(0);
			avid3.setPlayerId(2);
			avid3.setSlotId(255);
			goal0.setScoringTeam(0);
			goalSummary.push_back(&goal0);
		}
		else if (count == 1)
		{
			goal1.getPrimaryAssistFlags().clear();
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal1.getPrimaryAssist();
			avid1.setPersonaId(0);
			avid1.setPlayerId(0);
			avid1.setSlotId(0);
			goal1.setPrimaryAssistType(rand() % 10);
			goal1.getSecondaryAssistFlags().clear();
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal1.getSecondaryAssist();
			avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
			avid2.setPlayerId(0);
			avid2.setSlotId(rand() % 10);
			goal1.setSecondaryAssistType(rand() % 10);
			goal1.getGoalFlags().push_back(16);
			goal1.setGoalEventTime(60);
			goal1.setGoalType(rand() % 10);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal1.getScorer();
			avid3.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			avid3.setPlayerId(4);
			avid3.setSlotId(rand() % 15);
			goal1.setScoringTeam(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			goalSummary.push_back(&goal1);
		}
		else if (count == 2)
		{
			goal2.getPrimaryAssistFlags().push_back(0);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal2.getPrimaryAssist();
			avid1.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			avid1.setPlayerId(1);
			avid1.setSlotId(rand() % 15);
			goal2.setPrimaryAssistType(rand() % 10);
			goal2.getSecondaryAssistFlags().clear();
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal2.getSecondaryAssist();
			avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
			avid2.setPlayerId(0);
			avid2.setSlotId(rand() % 10);
			goal2.setSecondaryAssistType(rand() % 15);
			goal2.getGoalFlags().clear();
			goal2.setGoalEventTime(144);
			goal2.setGoalType(0);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal2.getScorer();
			avid3.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			avid3.setPlayerId(4);
			avid3.setSlotId(rand() % 10);
			goal2.setScoringTeam(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			goalSummary.push_back(&goal2);
		}
		else if (count == 3)
		{
			goal3.getPrimaryAssistFlags().push_back(0);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal3.getPrimaryAssist();
			avid1.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			avid1.setPlayerId(4);
			avid1.setSlotId(rand() % 10);
			goal3.setPrimaryAssistType(rand() % 15);
			goal3.getSecondaryAssistFlags().clear();
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal3.getSecondaryAssist();
			avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
			avid2.setPlayerId(rand() % 10);
			avid2.setSlotId(rand() % 10);
			goal3.setSecondaryAssistType(0);
			goal3.getGoalFlags().push_back(16);
			goal3.setGoalEventTime(166);
			goal3.setGoalType(0);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal3.getScorer();
			avid3.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			avid3.setPlayerId(2);
			avid3.setSlotId(rand() % 10);
			goal3.setScoringTeam(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			goalSummary.push_back(&goal3);
		}
		else if (count == 4)
		{
			goal4.getPrimaryAssistFlags().clear();
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal4.getPrimaryAssist();
			avid1.setPersonaId(0);
			avid1.setPlayerId(0);
			avid1.setSlotId(rand() % 10);
			goal4.setPrimaryAssistType(rand() % 10);
			goal4.getSecondaryAssistFlags().clear();
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal4.getSecondaryAssist();
			avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
			avid2.setPlayerId(0);
			avid2.setSlotId(rand() % 10);
			goal4.setSecondaryAssistType(0);
			goal4.getGoalFlags().clear();
			goal4.setGoalEventTime(174);
			goal4.setGoalType(rand() % 10);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal4.getScorer();
			avid3.setPersonaId(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			avid3.setPlayerId(4);
			avid3.setSlotId(rand() % 10);
			goal4.setScoringTeam(mPlayerData->getLogin()->getUserLoginInfo()->getBlazeId());
			goalSummary.push_back(&goal4);
		}
		else if (count == 5)
		{
			goal5.getPrimaryAssistFlags().push_back(rand() % 10);
			goal5.getPrimaryAssistFlags().push_back(rand() % 12);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal5.getPrimaryAssist();
			avid1.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
			avid1.setPlayerId(1);
			avid1.setSlotId(rand() % 10);
			goal5.setPrimaryAssistType(rand() % 10);
			goal5.getSecondaryAssistFlags().clear();
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal5.getSecondaryAssist();
			avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
			avid2.setPlayerId(0);
			avid2.setSlotId(rand() % 10);
			goal5.setSecondaryAssistType(rand() % 10);
			goal5.getGoalFlags().push_back(14);
			goal5.setGoalEventTime(308);
			goal5.setGoalType(rand() % 10);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal5.getScorer();
			avid3.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
			avid3.setPlayerId(2);
			avid3.setSlotId(rand() % 10);
			goal5.setScoringTeam(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
			goalSummary.push_back(&goal5);
		}
		else if (count == 6)
		{
			goal6.getPrimaryAssistFlags().push_back(rand() % 10);
			goal6.getPrimaryAssistFlags().push_back(rand() % 12);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal6.getPrimaryAssist();
			avid1.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
			avid1.setPlayerId(1);
			avid1.setSlotId(rand() % 10);
			goal6.setPrimaryAssistType(rand() % 10);
			goal6.getSecondaryAssistFlags().clear();
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal6.getSecondaryAssist();
			avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
			avid2.setPlayerId(0);
			avid2.setSlotId(rand() % 10);
			goal6.setSecondaryAssistType(rand() % 10);
			goal6.getGoalFlags().push_back(14);
			goal6.setGoalEventTime(308);
			goal6.setGoalType(rand() % 10);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal6.getScorer();
			avid3.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
			avid3.setPlayerId(2);
			avid3.setSlotId(rand() % 12);
			goal6.setScoringTeam(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
			goalSummary.push_back(&goal6);
		}
		else if (count == 7)
		{
			goal7.getPrimaryAssistFlags().push_back(rand() % 10);
			goal7.getPrimaryAssistFlags().push_back(rand() % 12);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid1 = goal7.getPrimaryAssist();
			avid1.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
			avid1.setPlayerId(1);
			avid1.setSlotId(rand() % 12);
			goal7.setPrimaryAssistType(rand() % 12);
			goal7.getSecondaryAssistFlags().clear();
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid2 = goal7.getSecondaryAssist();
			avid2.setPersonaId(0);// getLogin()->getUserLoginInfo()->getBlazeId());
			avid2.setPlayerId(0);
			avid2.setSlotId(rand() % 10);
			goal7.setSecondaryAssistType(rand() % 10);
			goal7.getGoalFlags().push_back(14);
			goal7.setGoalEventTime(308);
			goal7.setGoalType(rand() % 10);
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avid3 = goal7.getScorer();
			avid3.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
			avid3.setPlayerId(2);
			avid3.setSlotId(rand() % 10);
			goal7.setScoringTeam(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
			goalSummary.push_back(&goal7);
		}
		}
	}

	// fill the common game report value
	//Blaze::GameReporting::Fifa::CommonGameReport& commonGameReport = soloGameReport->getCommonGameReport();
	Blaze::GameReporting::SSF::CommonGameReport& commonGameReport = ssfSeasonsGameReport->getCommonGameReport();
	// WNPK
	commonGameReport.setWentToPk(rand() % 10);
	commonGameReport.setIsRivalMatch(false);
	// For now updating values as per client logs.TODO: need to add values according to server side settings
	commonGameReport.setBallId(rand() % 10);
	commonGameReport.setAwayCrestId(rand() % 10);
	commonGameReport.setHomeCrestId(rand() % 10);
	commonGameReport.setStadiumId(rand() % 12);
	/*const int MAX_TEAM_COUNT = 11;
	const int MAX_PLAYER_VALUE = 100000;
	substitution->setPlayerSentOff(Blaze::Random::getRandomNumber(250000));
	substitution->setPlayerSubbedIn(Blaze::Random::getRandomNumber(250000));
	substitution->setSubTime(56);
	//if (homesubNum > 2)
		//commonGameReport.getAwaySubList().push_back(substitution);
	// fill awayStartingXI and homeStartingXI values here
	for (int iteration = 0; iteration < MAX_TEAM_COUNT; iteration++) {
		//commonGameReport.getAwayStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
	}
	for (int iteration = 0; iteration < MAX_TEAM_COUNT; iteration++) {
		//commonGameReport.getHomeStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
	}*/
	osdkGamereport.setCustomGameReport(*ssfSeasonsGameReport);
	// populate player data in playermap
	// OsdkPlayerReportsMap = BLAZE_NEW Blaze::GameReporting::OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap; // = osdkReport->getPlayerReports();
	Blaze::GameReporting::OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = osdkReport->getPlayerReports();
	OsdkPlayerReportsMap.reserve(1);
	playerReport = OsdkPlayerReportsMap.allocate_element();
	soloPlayerReport = BLAZE_NEW Blaze::GameReporting::Fifa::SoloPlayerReport;
	ssfPlayerReport = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SSFSeasonsPlayerReport;
	if (ssfPlayerReport == NULL) {
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[SSFGameReportInstance] startgamereporting function call failed, failed to allocate memory for variable ssfPlayerReport");
		return ERR_SYSTEM;
	}
	Blaze::GameReporting::SSF::CommonStatsReport& commonPlayerReport = ssfPlayerReport->getCommonPlayerReport();
	//Blaze::GameReporting::Fifa::SoloCustomPlayerData& soloCustomPlayerData = soloPlayerReport->getSoloCustomPlayerData();
	//Blaze::GameReporting::Fifa::SeasonalPlayData& seasonalPlayData = soloPlayerReport->getSeasonalPlayData();

	// set the commonplayer report attribute values
	commonPlayerReport.setAveragePossessionLength(rand() % 10);
	commonPlayerReport.setAssists(rand() % 15);
	commonPlayerReport.setBlocks(1); //Added as per logs
	commonPlayerReport.setGoalsConceded(rand() % 10);
	commonPlayerReport.setHasCleanSheets(rand() % 10);
	commonPlayerReport.setCorners(rand() % 10);
	commonPlayerReport.setPassAttempts(rand() % 157);
	commonPlayerReport.setPassesMade(rand() % 150);
	commonPlayerReport.setRating((float)(0.00000000));
	commonPlayerReport.setSaves(rand() % 10);
	commonPlayerReport.setShots(rand() % 15);
	commonPlayerReport.setTackleAttempts(rand() % 12);
	commonPlayerReport.setTacklesMade(rand() % 12);
	commonPlayerReport.setFouls(rand() % 10);
	commonPlayerReport.setAverageFatigueAfterNinety(rand() % 10);
	commonPlayerReport.setGoals(rand() % 15);
	commonPlayerReport.setInjuriesRed(rand() % 10);
	commonPlayerReport.setInterceptions(rand() % 15);  // Added newly
	commonPlayerReport.setHasMOTM(rand() % 10);
	commonPlayerReport.setOffsides(rand() % 10);
	commonPlayerReport.setOwnGoals(rand() % 10);
	commonPlayerReport.setPkGoals(rand() % 10);
	commonPlayerReport.setPossession(rand() % 78);
	commonPlayerReport.setPassesIntercepted(rand() % 10);
	commonPlayerReport.setPenaltiesAwarded(rand() % 10);
	commonPlayerReport.setPenaltiesMissed(rand() % 10);
	commonPlayerReport.setPenaltiesScored(rand() % 10);
	commonPlayerReport.setPenaltiesSaved(rand() % 10);
	commonPlayerReport.setRedCard(rand() % 10);
	commonPlayerReport.setShotsOnGoal(rand() % 10);
	commonPlayerReport.setUnadjustedScore(rand() % 10);  // Added newly
	commonPlayerReport.setYellowCard(rand() % 10);

	ssfPlayerReport->setManOfTheMatch(rand() % 10);
	ssfPlayerReport->setPos(rand() % 10);
	ssfPlayerReport->setCleanSheetsAny(rand() % 10);
	ssfPlayerReport->setCleanSheetsDef(rand() % 10);
	ssfPlayerReport->setCleanSheetsGoalKeeper(rand() % 12);
	ssfPlayerReport->setIsCaptain(rand() % 12);
	ssfPlayerReport->setTeamEntityId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
	ssfPlayerReport->setSsfEndResult(Blaze::GameReporting::SSFSeasonsReportBase::SsfMatchEndResult::SSF_END_INVALID);// missing
	ssfPlayerReport->setUserEndSubReason(Blaze::GameReporting::SSFSeasonsReportBase::SsfUserEndSubReason::SSF_USER_RESULT_NONE); // missing

	/*soloCustomPlayerData.setGoalAgainst(rand() % 10);
	soloCustomPlayerData.setLosses(0);  // Added newly
	soloCustomPlayerData.setResult(rand() % 10);  // Added newly
	soloCustomPlayerData.setShotsAgainst(rand() % 15);
	soloCustomPlayerData.setShotsAgainstOnGoal(rand() % 10);
	soloCustomPlayerData.setTeam(rand() % 10);
	soloCustomPlayerData.setTies(0);  // Added newly
	soloCustomPlayerData.setWins(1);  // Added newly

	seasonalPlayData.setCurrentDivision(0);
	seasonalPlayData.setCup_id(0);
	seasonalPlayData.setUpdateDivision(Blaze::GameReporting::Fifa::CompetitionStatus::NO_UPDATE);
	seasonalPlayData.setGameNumber(0);
	seasonalPlayData.setSeasonId(0);
	seasonalPlayData.setWonCompetition(0);
	seasonalPlayData.setWonLeagueTitle(false);*/
	playerReport->setClientScore(rand() % 10);
	playerReport->setAccountCountry(rand() % 12);
	playerReport->setFinishReason(rand() % 10);
	playerReport->setGameResult(rand() % 10);
	playerReport->setHome(true);
	playerReport->setLosses(rand() % 10);
	playerReport->setName("FIFA21ONLINE1_01");
	playerReport->setOpponentCount(rand() % 10);
	playerReport->setExternalId(0);
	playerReport->setNucleusId(0);
	playerReport->setPersona("");
	playerReport->setPointsAgainst(rand() % 10);
	playerReport->setUserResult(rand() % 10);
	playerReport->setCustomDnf(rand() % 10);
	playerReport->setScore(rand() % 15);
	playerReport->setSkill(rand() % 10);
	playerReport->setSkillPoints(rand() % 10);
	playerReport->setTeam(Blaze::Random::getRandomNumber(200000));
	playerReport->setTies(rand() % 10);
	playerReport->setWinnerByDnf(rand() % 10);
	playerReport->setWins(rand() % 10);
	playerReport->setCustomPlayerReport(*ssfPlayerReport);

	/*GameReporting::OSDKGameReportBase::IntegerAttributeMap& privateAttrMap = playerReport->getPrivatePlayerReport().getPrivateIntAttributeMap();
	if (mScenarioType == SSF_EVENT_MATCH)
	{
		privateAttrMap["aiAst"] = (int64_t)0;
		privateAttrMap["aiCS"] = (int64_t)1;
		privateAttrMap["aiCo"] = (int64_t)0;
		privateAttrMap["aiFou"] = (int64_t)0;
		privateAttrMap["aiGC"] = (int64_t)0;
		privateAttrMap["aiGo"] = (int64_t)0;
		privateAttrMap["aiInt"] = (int64_t)0;
		privateAttrMap["aiMOTM"] = (int64_t)0;
		privateAttrMap["aiOG"] = (int64_t)0;
		privateAttrMap["aiOff"] = (int64_t)0;
		privateAttrMap["aiPA"] = (int64_t)118;
		privateAttrMap["aiPKG"] = (int64_t)0;
		privateAttrMap["aiPM"] = (int64_t)77;
		privateAttrMap["aiPo"] = (int64_t)55;
		privateAttrMap["aiRC"] = (int64_t)0;
		privateAttrMap["aiRa"] = (int64_t)0;
		privateAttrMap["aiSav"] = (int64_t)0;
		privateAttrMap["aiSoG"] = (int64_t)0;
		privateAttrMap["aiTA"] = (int64_t)11;
		privateAttrMap["aiTM"] = (int64_t)9;
		privateAttrMap["aiYC"] = (int64_t)0;
	}
	else if (mScenarioType == SSF_SINGLE_MATCH)
	{
		privateAttrMap["aiAst"] = (int64_t)0;
		privateAttrMap["aiCS"] = (int64_t)0;
		privateAttrMap["aiCo"] = (int64_t)2;
		privateAttrMap["aiFou"] = (int64_t)0;
		privateAttrMap["aiGC"] = (int64_t)3;
		privateAttrMap["aiGo"] = (int64_t)2;
		privateAttrMap["aiInt"] = (int64_t)0;
		privateAttrMap["aiMOTM"] = (int64_t)0;
		privateAttrMap["aiOG"] = (int64_t)0;
		privateAttrMap["aiOff"] = (int64_t)0;
		privateAttrMap["aiPA"] = (int64_t)53;
		privateAttrMap["aiPKG"] = (int64_t)0;
		privateAttrMap["aiPM"] = (int64_t)36;
		privateAttrMap["aiPo"] = (int64_t)22;
		privateAttrMap["aiRC"] = (int64_t)0;
		privateAttrMap["aiRa"] = (int64_t)0;
		privateAttrMap["aiSav"] = (int64_t)0;
		privateAttrMap["aiSoG"] = (int64_t)3;
		privateAttrMap["aiTA"] = (int64_t)3;
		privateAttrMap["aiTM"] = (int64_t)1;
		privateAttrMap["aiYC"] = (int64_t)0;
	}
	else if (mScenarioType == SSF_SINGLE_MATCH)
	{
		privateAttrMap["aiAst"] = (int64_t)0;
		privateAttrMap["aiCS"] = (int64_t)0;
		privateAttrMap["aiCo"] = (int64_t)2;
		privateAttrMap["aiFou"] = (int64_t)0;
		privateAttrMap["aiGC"] = (int64_t)3;
		privateAttrMap["aiGo"] = (int64_t)2;
		privateAttrMap["aiInt"] = (int64_t)0;
		privateAttrMap["aiMOTM"] = (int64_t)0;
		privateAttrMap["aiOG"] = (int64_t)0;
		privateAttrMap["aiOff"] = (int64_t)0;
		privateAttrMap["aiPA"] = (int64_t)53;
		privateAttrMap["aiPKG"] = (int64_t)0;
		privateAttrMap["aiPM"] = (int64_t)36;
		privateAttrMap["aiPo"] = (int64_t)22;
		privateAttrMap["aiRC"] = (int64_t)0;
		privateAttrMap["aiRa"] = (int64_t)0;
		privateAttrMap["aiSav"] = (int64_t)0;
		privateAttrMap["aiSoG"] = (int64_t)3;
		privateAttrMap["aiTA"] = (int64_t)3;
		privateAttrMap["aiTM"] = (int64_t)1;
		privateAttrMap["aiYC"] = (int64_t)0;
	}
	else if (mScenarioType == SSF_TWIST_MATCH)
	{
		privateAttrMap["aiAst"] = (int64_t)0;
		privateAttrMap["aiCS"] = (int64_t)0;
		privateAttrMap["aiCo"] = (int64_t)0;
		privateAttrMap["aiFou"] = (int64_t)0;
		privateAttrMap["aiGC"] = (int64_t)0;
		privateAttrMap["aiGo"] = (int64_t)1;
		privateAttrMap["aiInt"] = (int64_t)0;
		privateAttrMap["aiMOTM"] = (int64_t)0;
		privateAttrMap["aiOG"] = (int64_t)0;
		privateAttrMap["aiOff"] = (int64_t)0;
		privateAttrMap["aiPA"] = (int64_t)0;
		privateAttrMap["aiPKG"] = (int64_t)0;
		privateAttrMap["aiPM"] = (int64_t)0;
		privateAttrMap["aiPo"] = (int64_t)18;
		privateAttrMap["aiRC"] = (int64_t)0;
		privateAttrMap["aiRa"] = (int64_t)0;
		privateAttrMap["aiSav"] = (int64_t)0;
		privateAttrMap["aiSoG"] = (int64_t)1;
		privateAttrMap["aiTA"] = (int64_t)0;
		privateAttrMap["aiTM"] = (int64_t)0;
		privateAttrMap["aiYC"] = (int64_t)0;
	}
	//if (isFUTPlayer()) {
	//	privateAttrMap["futMatchResult"] = (int64_t)Random::getRandomNumber(3) + 1;
	//	privateAttrMap["futMatchTime"] = (int64_t)1488800000 + (int64_t)Random::getRandomNumber(100000);
	//}
	privateAttrMap["gid"] = (int64_t)1488800000 + (int64_t)Random::getRandomNumber(100000);

	//if (mOwner->getIsFutPlayer()) {
	//    // adding pirvate player report for FUT scenarios.
	//    Blaze::GameReporting::OSDKGameReportBase::OSDKPrivatePlayerReport& osdkPrivatePlayerReport = playerReport->getPrivatePlayerReport();
	//    Blaze::GameReporting::OSDKGameReportBase::IntegerAttributeMap& integerAttributeMap = osdkPrivatePlayerReport.getPrivateIntAttributeMap();
	//    integerAttributeMap["futMatchResult"] = rand() % 10;
	//    integerAttributeMap["futMatchTime"] = osdkGamereport.getGameTime();
	//}*/

	/*if (mScenarioType == SSF_SINGLE_MATCH || mScenarioType == SSF_TWIST_MATCH)
	{*/
	Blaze::GameReporting::OSDKGameReportBase::OSDKPrivatePlayerReport& osdkPrivatePlayerReport = playerReport->getPrivatePlayerReport();
	Blaze::GameReporting::OSDKGameReportBase::IntegerAttributeMap& integerAttributeMap = osdkPrivatePlayerReport.getPrivateIntAttributeMap();
	integerAttributeMap["matchStatus"] = (int64_t)0;
	//}

	OsdkPlayerReportsMap[playerId] = playerReport;
	ssfTeamSummaryReport = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SSFTeamSummaryReport;
	if (ssfTeamSummaryReport == NULL) {
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[SSFGameReportInstance] startgamereporting function call failed, failed to allocate memory for variable ssfTeamSummaryReport");
		return ERR_SYSTEM;
	}
	Blaze::GameReporting::SSFSeasonsReportBase::SSFTeamReportMap& ssfTeamReportMap = ssfTeamSummaryReport->getSSFTeamReportMap();
	ssfTeamReportMap.reserve(rand() % 12);
	Blaze::GameReporting::SSFSeasonsReportBase::SSFTeamReport *teamReport1 = ssfTeamReportMap.allocate_element();
	teamReport1->setSquadDisc(rand() % 10);
	teamReport1->setHome(false);
	teamReport1->setLosses(0);
	teamReport1->setGameResult(0);
	teamReport1->setScore(0);
	teamReport1->setTies(0);
	teamReport1->setSkillMoveCount(0); //missing
	teamReport1->setWinnerByDnf(0);
	teamReport1->setWins(0);
	Blaze::GameReporting::SSFSeasonsReportBase::AvatarVector& avVector = teamReport1->getAvatarVector();
	Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry1 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
	Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId1 = avEntry1->getAvatarId();
	avatarId1.setPersonaId(0);
	avatarId1.setPlayerId(0);
	avatarId1.setSlotId(255);
	Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport1 = avEntry1->getAvatarStatReport();
	avatarStatsReport1.setAveragePossessionLength(0);
	avatarStatsReport1.setAssists(rand() % 10);
	//avatarStatsReport1.setBlocks(0); // missing
	avatarStatsReport1.setGoalsConceded(rand() % 10);
	avatarStatsReport1.setHasCleanSheets(rand() % 10);
	avatarStatsReport1.setCorners(rand() % 10);
	avatarStatsReport1.setPassAttempts(rand() % 18);
	avatarStatsReport1.setPassesMade(rand() % 19);
	avatarStatsReport1.setRating((float)(7.30000019));
	avatarStatsReport1.setSaves(rand() % 10);
	avatarStatsReport1.setShots(rand() % 10);
	avatarStatsReport1.setTackleAttempts(rand() % 10);
	avatarStatsReport1.setTacklesMade(rand() % 10);
	avatarStatsReport1.setFouls(rand() % 10);
	avatarStatsReport1.setAverageFatigueAfterNinety(rand() % 10);
	avatarStatsReport1.setGoals(rand() % 10);
	avatarStatsReport1.setInjuriesRed(rand() % 15);
	avatarStatsReport1.setInterceptions(rand() % 12);
	avatarStatsReport1.setHasMOTM(rand() % 12);
	avatarStatsReport1.setOffsides(rand() % 10);
	avatarStatsReport1.setOwnGoals(rand() % 12);
	avatarStatsReport1.setPkGoals(rand() % 12);
	avatarStatsReport1.setPossession(rand() % 12);
	avatarStatsReport1.setPassesIntercepted(rand() % 10);
	avatarStatsReport1.setPenaltiesAwarded(rand() % 10);
	avatarStatsReport1.setPenaltiesMissed(rand() % 10);
	avatarStatsReport1.setPenaltiesScored(rand() % 10);
	avatarStatsReport1.setPenaltiesSaved(rand() % 10);
	avatarStatsReport1.setRedCard(rand() % 10);
	avatarStatsReport1.setShotsOnGoal(rand() % 12);
	avatarStatsReport1.setUnadjustedScore(rand() % 10);
	avatarStatsReport1.setYellowCard(rand() % 10);
	avVector.push_back(avEntry1);	//0

	//avatarvector entry 2
	Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry2 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
	Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId2 = avEntry2->getAvatarId();
	avatarId2.setPersonaId(0);
	avatarId2.setPlayerId(1);
	avatarId2.setSlotId(255);
	Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport2 = avEntry2->getAvatarStatReport();
	avatarStatsReport2.setAveragePossessionLength(rand() % 10);
	avatarStatsReport2.setAssists(rand() % 12);
	avatarStatsReport2.setBlocks(rand() % 10);
	avatarStatsReport2.setGoalsConceded(rand() % 10);
	avatarStatsReport2.setHasCleanSheets(rand() % 15);
	avatarStatsReport2.setCorners(rand() % 15);
	avatarStatsReport2.setPassAttempts(rand() % 13);
	avatarStatsReport2.setPassesMade(rand() % 10);
	avatarStatsReport2.setRating((float)(6.90000010));
	avatarStatsReport2.setSaves(rand() % 10);
	avatarStatsReport2.setShots(rand() % 10);
	avatarStatsReport2.setTackleAttempts(rand() % 10);
	avatarStatsReport2.setTacklesMade(rand() % 10);
	avatarStatsReport2.setFouls(rand() % 10);
	avatarStatsReport2.setAverageFatigueAfterNinety(rand() % 10);
	avatarStatsReport2.setGoals(rand() % 10);
	avatarStatsReport2.setInjuriesRed(rand() % 10);
	avatarStatsReport2.setInterceptions(rand() % 10);
	avatarStatsReport2.setHasMOTM(rand() % 10);
	avatarStatsReport2.setOffsides(rand() % 10);
	avatarStatsReport2.setOwnGoals(rand() % 10);
	avatarStatsReport2.setPkGoals(rand() % 10);
	avatarStatsReport2.setPossession(rand() % 10);
	avatarStatsReport2.setPassesIntercepted(rand() % 10);
	avatarStatsReport2.setPenaltiesAwarded(rand() % 10);
	avatarStatsReport2.setPenaltiesMissed(rand() % 10);
	avatarStatsReport2.setPenaltiesScored(rand() % 10);
	avatarStatsReport2.setPenaltiesSaved(rand() % 10);
	avatarStatsReport2.setRedCard(rand() % 10);
	avatarStatsReport2.setShotsOnGoal(rand() % 10);
	avatarStatsReport2.setUnadjustedScore(rand() % 10);
	avatarStatsReport2.setYellowCard(rand() % 10);
	if (mScenarioType == SSF_SINGLE_MATCH)
	{
		Blaze::GameReporting::SSFSeasonsReportBase::SkillTimeBucketMap &avatarSkillTime = avEntry2->getSkillTimeBucketMap();
		avatarSkillTime.clear();
		/*avatarSkillTime.initMap(1);
		EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent1 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent();
		skillEvent1->setCount(1);
		skillEvent1->setDribblePast(0);
		skillEvent1->setSkillType(2);*/
		/*EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent2 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent();
		skillEvent2->setCount(3);
		skillEvent2->setDribblePast(1);
		skillEvent2->setSkillType(33);*/
		//Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap1 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
		//Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType1 = 2;
		//Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType2 = 33;
		//skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType,EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType1, skillEvent1)));
		//skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType,EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType2, skillEvent2)));
		//for (auto it = avatarSkillTime.begin(); it != avatarSkillTime.end(); ++it)
		//{
		//	it->first = rand() % 12;
		//	it->second->setTimePeriod(6);
		//	//skillEventmap1->copyInto(it->second->getSkillMap());
		//}
	}
	/*if (mScenarioType == SSF_SINGLE_MATCH)
	{
		Blaze::GameReporting::SSFSeasonsReportBase::SkillTimeBucketMap &avatarSkillTime = avEntry2->getSkillTimeBucketMap();
		avatarSkillTime.initMap(2);
		Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap1 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
		Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap2 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
		Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType11 = 7;
		Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType12 = 25;
		Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType13 = 29;
		Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType21 = 29;

		EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent11 = fillSkillEvent(0, 1, 7);
		EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent12 = fillSkillEvent(0, 1, 25);
		EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent13 = fillSkillEvent(0, 2, 29);
		EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent21 = fillSkillEvent(0, 2, 29);

		skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType11, skillEvent11)));
		skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType12, skillEvent12)));
		skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType13, skillEvent13)));
		skillEventmap2->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType21, skillEvent21)));

		auto it = avatarSkillTime.begin();
		it->first = 2;
		it->second->setTimePeriod(2);
		skillEventmap1->copyInto(it->second->getSkillMap());
		++it;
		it->first = 5;
		it->second->setTimePeriod(5);
		skillEventmap2->copyInto(it->second->getSkillMap());
	}*/
	avVector.push_back(avEntry2);	// 1

	//avatarvector entry 3
	//if (mScenarioType == SSF_TWIST_MATCH || mScenarioType == SSF_SINGLE_MATCH)
	//{
	Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry3 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
	Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId3 = avEntry3->getAvatarId();
	avatarId3.setPersonaId(0);
	avatarId3.setPlayerId(2);
	avatarId3.setSlotId(255);
	Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport3 = avEntry3->getAvatarStatReport();
	avatarStatsReport3.setAveragePossessionLength(rand() % 10);
	avatarStatsReport3.setAssists(rand() % 10);
	avatarStatsReport3.setBlocks(rand() % 10); // missing
	avatarStatsReport3.setGoalsConceded(rand() % 10);
	avatarStatsReport3.setHasCleanSheets(rand() % 10);
	avatarStatsReport3.setCorners(rand() % 10);
	avatarStatsReport3.setPassAttempts(rand() % 12);
	avatarStatsReport3.setPassesMade(rand() % 12);
	avatarStatsReport3.setRating((float)(7.59999990));
	avatarStatsReport3.setSaves(rand() % 10);
	avatarStatsReport3.setShots(rand() % 10);
	avatarStatsReport3.setTackleAttempts(rand() % 10);
	avatarStatsReport3.setTacklesMade(rand() % 10);
	avatarStatsReport3.setFouls(rand() % 10);
	avatarStatsReport3.setAverageFatigueAfterNinety(rand() % 10);
	avatarStatsReport3.setGoals(rand() % 10);
	avatarStatsReport3.setInjuriesRed(rand() % 10);
	avatarStatsReport3.setInterceptions(rand() % 10);
	avatarStatsReport3.setHasMOTM(rand() % 12);
	avatarStatsReport3.setOffsides(rand() % 10);
	avatarStatsReport3.setOwnGoals(rand() % 10);
	avatarStatsReport3.setPkGoals(rand() % 10);
	avatarStatsReport3.setPossession(rand() % 10);
	avatarStatsReport3.setPassesIntercepted(rand() % 10);
	avatarStatsReport3.setPenaltiesAwarded(rand() % 10);
	avatarStatsReport3.setPenaltiesMissed(rand() % 10);
	avatarStatsReport3.setPenaltiesScored(rand() % 10);
	avatarStatsReport3.setPenaltiesSaved(rand() % 10);
	avatarStatsReport3.setRedCard(rand() % 10);
	avatarStatsReport3.setShotsOnGoal(rand() % 10);
	avatarStatsReport3.setUnadjustedScore(rand() % 15);
	avatarStatsReport3.setYellowCard(rand() % 10);

	if (mScenarioType == SSF_SINGLE_MATCH)
	{
		Blaze::GameReporting::SSFSeasonsReportBase::SkillTimeBucketMap &avatarSkillTime = avEntry3->getSkillTimeBucketMap();
		avatarSkillTime.clear();
		/*EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent1 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent();
		skillEvent1->setCount(2);
		skillEvent1->setDribblePast(0);
		skillEvent1->setSkillType(2);*/
		//Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap1 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
		//Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType1 = 2;
		//Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType2 = 33;
		//skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType1, skillEvent1)));
		//for (auto it = avatarSkillTime.begin(); it != avatarSkillTime.end(); ++it)
		//{
		//	it->first = rand() % 12;
		//	it->second->setTimePeriod(4);
		//	//skillEventmap1->copyInto(it->second->getSkillMap());
		//}
	}

	//avVector.push_back(avEntry3);	// 2
	//}


	//if (mScenarioType == SSF_SINGLE_MATCH)
	//{
		//avatarvector entry 4
		Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry4 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
		Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId4 = avEntry4->getAvatarId();
		avatarId4.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
		avatarId4.setPlayerId(0);
		avatarId4.setSlotId(1);
		Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport4 = avEntry4->getAvatarStatReport();
		avatarStatsReport4.setAveragePossessionLength(rand() % 10);
		avatarStatsReport4.setAssists(rand() % 12);
		avatarStatsReport4.setBlocks(rand() % 12); // missing
		avatarStatsReport4.setGoalsConceded(rand() % 10);
		avatarStatsReport4.setHasCleanSheets(rand() % 10);
		avatarStatsReport4.setCorners(rand() % 10);
		avatarStatsReport4.setPassAttempts(rand() % 18);
		avatarStatsReport4.setPassesMade(rand() % 10);
		avatarStatsReport4.setRating((float)(6.09999990));
		avatarStatsReport4.setSaves(rand() % 10);
		avatarStatsReport4.setShots(rand() % 12);
		avatarStatsReport4.setTackleAttempts(rand() % 10);
		avatarStatsReport4.setTacklesMade(rand() % 10);
		avatarStatsReport4.setFouls(rand() % 10);
		avatarStatsReport4.setAverageFatigueAfterNinety(rand() % 10);
		avatarStatsReport4.setGoals(rand() % 10);
		avatarStatsReport4.setInjuriesRed(rand() % 10);
		avatarStatsReport4.setInterceptions(rand() % 10);
		avatarStatsReport4.setHasMOTM(rand() % 10);
		avatarStatsReport4.setOffsides(rand() % 10);
		avatarStatsReport4.setOwnGoals(rand() % 12);
		avatarStatsReport4.setPkGoals(rand() % 12);
		avatarStatsReport4.setPossession(rand() % 10);
		avatarStatsReport4.setPassesIntercepted(rand() % 10);
		avatarStatsReport4.setPenaltiesAwarded(rand() % 12);
		avatarStatsReport4.setPenaltiesMissed(rand() % 15);
		avatarStatsReport4.setPenaltiesScored(rand() % 12);
		avatarStatsReport4.setPenaltiesSaved(rand() % 12);
		avatarStatsReport4.setRedCard(rand() % 10);
		avatarStatsReport4.setShotsOnGoal(rand() % 10);
		avatarStatsReport4.setUnadjustedScore(rand() % 10);
		avatarStatsReport4.setYellowCard(rand() % 10);
		if (mScenarioType == SSF_SINGLE_MATCH)
		{
			Blaze::GameReporting::SSFSeasonsReportBase::SkillTimeBucketMap &avatarSkillTime = avEntry4->getSkillTimeBucketMap();
			avatarSkillTime.clear();
			/*EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent1 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent();
			skillEvent1->setCount(2);
			skillEvent1->setDribblePast(0);
			skillEvent1->setSkillType(2);*/
			/*EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent2 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent();
			skillEvent2->setCount(3);
			skillEvent2->setDribblePast(1);
			skillEvent2->setSkillType(33);*/
			//Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap1 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
			//Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType1 = 2;
			//Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType2 = 33;
			//skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType1, skillEvent1)));
			//skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType2, skillEvent2)));
			//for (auto it = avatarSkillTime.begin(); it != avatarSkillTime.end(); ++it)
			//{
			//	it->first = rand() % 12;
			//	it->second->setTimePeriod(4);
			//	//skillEventmap1->copyInto(it->second->getSkillMap());
			//}
		}
		avVector.push_back(avEntry4);	//3
	//}
		if (mScenarioType == SSF_WORLD_TOUR)
		{
			//avatarvector entry 5
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry5 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
			Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId5 = avEntry5->getAvatarId();
			avatarId5.setPersonaId(0);
			avatarId5.setPlayerId(3);
			avatarId5.setSlotId(255);
			Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport5 = avEntry5->getAvatarStatReport();
			avatarStatsReport5.setAveragePossessionLength(0);
			avatarStatsReport5.setAssists(rand() % 10);
			//avatarStatsReport5.setBlocks(0); // missing
			avatarStatsReport5.setGoalsConceded(0);
			avatarStatsReport5.setHasCleanSheets(0);
			avatarStatsReport5.setCorners(rand() % 15);
			avatarStatsReport5.setPassAttempts(rand() % 15);
			avatarStatsReport5.setPassesMade(rand() % 12);
			avatarStatsReport5.setRating((float)(rand() % 10));
			avatarStatsReport5.setSaves(rand() % 10);
			avatarStatsReport5.setShots(rand() % 12);
			avatarStatsReport5.setTackleAttempts(rand() % 12);
			avatarStatsReport5.setTacklesMade(rand() % 15);
			avatarStatsReport5.setFouls(rand() % 12);
			avatarStatsReport5.setAverageFatigueAfterNinety(0);
			avatarStatsReport5.setGoals(rand() % 15);
			avatarStatsReport5.setInjuriesRed(rand() % 10);
			avatarStatsReport5.setInterceptions(rand() % 10);
			avatarStatsReport5.setHasMOTM(rand() % 10);
			avatarStatsReport5.setOffsides(rand() % 15);
			avatarStatsReport5.setOwnGoals(rand() % 10);
			avatarStatsReport5.setPkGoals(rand() % 10);
			avatarStatsReport5.setPossession(rand() % 15);
			avatarStatsReport5.setPassesIntercepted(rand() % 10);
			avatarStatsReport5.setPenaltiesAwarded(rand() % 10);
			avatarStatsReport5.setPenaltiesMissed(rand() % 12);
			avatarStatsReport5.setPenaltiesScored(rand() % 10);
			avatarStatsReport5.setPenaltiesSaved(rand() % 12);
			avatarStatsReport5.setRedCard(rand() % 12);
			avatarStatsReport5.setShotsOnGoal(rand() % 15);
			avatarStatsReport5.setUnadjustedScore(rand() % 10);
			avatarStatsReport5.setYellowCard(rand() % 12);
			avVector.push_back(avEntry5);	//4

		}
	//}

	Blaze::GameReporting::SSF::CommonStatsReport& commonTeamReport = teamReport1->getCommonTeamReport();
	commonTeamReport.setAveragePossessionLength(rand() % 10);
	commonTeamReport.setAssists(rand() % 10);
	commonTeamReport.setBlocks(rand() % 10); // Added as per logs
	commonTeamReport.setGoalsConceded(rand() % 10);
	commonTeamReport.setHasCleanSheets(rand() % 10);
	commonTeamReport.setCorners(rand() % 10);
	commonTeamReport.setPassAttempts(rand() % 10);
	commonTeamReport.setPassesMade(rand() % 12);
	commonTeamReport.setRating((float)(6.40000010));
	commonTeamReport.setSaves(rand() % 10);
	commonTeamReport.setShots(rand() % 10);
	commonTeamReport.setTackleAttempts(rand() % 10);
	commonTeamReport.setTacklesMade(rand() % 10);
	commonTeamReport.setFouls(rand() % 12);
	commonTeamReport.setAverageFatigueAfterNinety(rand() % 12);
	commonTeamReport.setGoals(rand() % 12);
	commonTeamReport.setInjuriesRed(rand() % 12);
	commonTeamReport.setInterceptions(rand() % 12);
	commonTeamReport.setHasMOTM(rand() % 12);
	commonTeamReport.setOffsides(rand() % 10);
	commonTeamReport.setOwnGoals(rand() % 10);
	commonTeamReport.setPkGoals(rand() % 10);
	commonTeamReport.setPossession(rand() % 10);
	commonTeamReport.setPassesIntercepted(rand() % 10);
	commonTeamReport.setPenaltiesAwarded(rand() % 10);
	commonTeamReport.setPenaltiesMissed(rand() % 10);
	commonTeamReport.setPenaltiesScored(rand() % 10);
	commonTeamReport.setPenaltiesSaved(rand() % 10);
	commonTeamReport.setRedCard(rand() % 10);
	commonTeamReport.setShotsOnGoal(rand() % 12);
	commonTeamReport.setUnadjustedScore(rand() % 10);
	commonTeamReport.setYellowCard(rand() % 10);
	ssfTeamReportMap[0] = teamReport1;

	Blaze::GameReporting::SSFSeasonsReportBase::SSFTeamReport *teamReport2 = ssfTeamReportMap.allocate_element();
	teamReport2->setSquadDisc(rand() % 10);
	teamReport2->setHome(true);
	teamReport2->setLosses(rand() % 10);
	teamReport2->setGameResult(rand() % 15);
	teamReport2->setScore(rand() % 10);
	teamReport2->setTies(rand() % 12);
	teamReport2->setSkillMoveCount(rand() % 10); //missing
	teamReport2->setWinnerByDnf(rand() % 12);
	teamReport2->setWins(rand() % 10);
	Blaze::GameReporting::SSFSeasonsReportBase::AvatarVector& avVector2 = teamReport2->getAvatarVector();
	Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry11 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
	Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId11 = avEntry11->getAvatarId();
	avatarId11.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
	avatarId11.setPlayerId(2);
	avatarId11.setSlotId(rand() % 10);
	Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport11 = avEntry11->getAvatarStatReport();
	avatarStatsReport11.setAveragePossessionLength(rand() % 10);
	avatarStatsReport11.setAssists(rand() % 15);
	avatarStatsReport11.setBlocks(rand() % 15); // missing
	avatarStatsReport11.setGoalsConceded(rand() % 10);
	avatarStatsReport11.setHasCleanSheets(rand() % 10);
	avatarStatsReport11.setCorners(rand() % 10);
	avatarStatsReport11.setPassAttempts(rand() % 10);
	avatarStatsReport11.setPassesMade(rand() % 10);
	avatarStatsReport11.setRating((float)(5.80000019));
	avatarStatsReport11.setSaves(rand() % 10);
	avatarStatsReport11.setShots(rand() % 12);
	avatarStatsReport11.setTackleAttempts(rand() % 12);
	avatarStatsReport11.setTacklesMade(rand() % 12);
	avatarStatsReport11.setFouls(rand() % 10);
	avatarStatsReport11.setAverageFatigueAfterNinety(rand() % 10);
	avatarStatsReport11.setGoals(rand() % 10);
	avatarStatsReport11.setInjuriesRed(rand() % 10);
	avatarStatsReport11.setInterceptions(rand() % 10);
	avatarStatsReport11.setHasMOTM(rand() % 12);
	avatarStatsReport11.setOffsides(rand() % 10);
	avatarStatsReport11.setOwnGoals(rand() % 10);
	avatarStatsReport11.setPkGoals(rand() % 10);
	avatarStatsReport11.setPossession(rand() % 10);
	avatarStatsReport11.setPassesIntercepted(rand() % 10);
	avatarStatsReport11.setPenaltiesAwarded(rand() % 10);
	avatarStatsReport11.setPenaltiesMissed(rand() % 10);
	avatarStatsReport11.setPenaltiesScored(rand() % 10);
	avatarStatsReport11.setPenaltiesSaved(rand() % 10);
	avatarStatsReport11.setRedCard(rand() % 10);
	avatarStatsReport11.setShotsOnGoal(rand() % 10);
	avatarStatsReport11.setUnadjustedScore(rand() % 10);
	avatarStatsReport11.setYellowCard(rand() % 10);
	if (mScenarioType == SSF_SINGLE_MATCH)
	{
		Blaze::GameReporting::SSFSeasonsReportBase::SkillTimeBucketMap &avatarSkillTime = avEntry11->getSkillTimeBucketMap();
		avatarSkillTime.clear();
		//Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap1 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
		//Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap2 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
		/*Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap3 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
		Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap4 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
		Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap5 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();*/
		
		/*Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType11 = 7;
		Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType21 = 25;
		Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType22 = 40;
		Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType23 = 59;
		Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType31 = 6;
		
		EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent1 = fillSkillEvent(0,1,7);
		EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent21 = fillSkillEvent(0, 1, 25);
		EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent22 = fillSkillEvent(0, 1, 40);
		EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent23 = fillSkillEvent(0, 4, 59);
		EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent31 = fillSkillEvent(0, 1, 7);
		
		EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent32 = fillSkillEvent(0, 1, 40);
		EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent33 = fillSkillEvent(0, 4, 59);
		EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent34 = fillSkillEvent(0, 1, 7);
		
		skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType11, skillEvent1)));
		skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType21, skillEvent21)));
		skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType22, skillEvent22)));
		skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType23, skillEvent23)));
		skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType31, skillEvent31)));
		
		skillEventmap2->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType22, skillEvent32)));
		skillEventmap2->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType23, skillEvent33)));
		skillEventmap2->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType31, skillEvent34)));*/
	//	for (auto it = avatarSkillTime.begin(); it != avatarSkillTime.end(); ++it)
	//	{
	//		it->first = rand() % 7;
	//		it->second->setTimePeriod(2);
	//		skillEventmap1->copyInto(it->second->getSkillMap());
	//		++it;
	//		it->first = rand() % 12;
	//		it->second->setTimePeriod(6);
	//		skillEventmap2->copyInto(it->second->getSkillMap());
	//		/*++it;
	//		it->first = 7;
	//		it->second->setTimePeriod(7);
	//		skillEventmap3->copyInto(it->second->getSkillMap());
	//		it->first = 2;
	//		it->second->setTimePeriod(2);
	//		skillEventmap4->copyInto(it->second->getSkillMap());
	//		++it;
	//		it->first = 2;
	//		it->second->setTimePeriod(2);
	//		skillEventmap5->copyInto(it->second->getSkillMap());
	//		++it;*/
	}
	avVector2.push_back(avEntry11);		// 0

	//avatarvector entry 2
	Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry12 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
	Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId12 = avEntry12->getAvatarId();
	avatarId12.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
	avatarId12.setPlayerId(1);
	avatarId12.setSlotId(2);
	Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport12 = avEntry12->getAvatarStatReport();
	avatarStatsReport12.setAveragePossessionLength(rand() % 10);
	avatarStatsReport12.setAssists(rand() % 10);
	avatarStatsReport12.setBlocks(rand() % 15);
	avatarStatsReport12.setGoalsConceded(rand() % 15);
	avatarStatsReport12.setHasCleanSheets(rand() % 10);
	avatarStatsReport12.setCorners(rand() % 10);
	avatarStatsReport12.setPassAttempts(rand() % 128);
	avatarStatsReport12.setPassesMade(rand() % 107);
	avatarStatsReport12.setRating((float)(0.00000000));
	avatarStatsReport12.setSaves(rand() % 10);
	avatarStatsReport12.setShots(rand() % 10);
	avatarStatsReport12.setTackleAttempts(rand() % 10);
	avatarStatsReport12.setTacklesMade(rand() % 12);
	avatarStatsReport12.setFouls(rand() % 10);
	avatarStatsReport12.setAverageFatigueAfterNinety(rand() % 10);
	avatarStatsReport12.setGoals(rand() % 10);
	avatarStatsReport12.setInjuriesRed(rand() % 10);
	avatarStatsReport12.setInterceptions(rand() % 10);
	avatarStatsReport12.setHasMOTM(rand() % 10);
	avatarStatsReport12.setOffsides(rand() % 10);
	avatarStatsReport12.setOwnGoals(rand() % 12);
	avatarStatsReport12.setPkGoals(rand() % 12);
	avatarStatsReport12.setPossession(rand() % 75);
	avatarStatsReport12.setPassesIntercepted(rand() % 12);
	avatarStatsReport12.setPenaltiesAwarded(rand() % 10);
	avatarStatsReport12.setPenaltiesMissed(rand() % 10);
	avatarStatsReport12.setPenaltiesScored(rand() % 10);
	avatarStatsReport12.setPenaltiesSaved(rand() % 10);
	avatarStatsReport12.setRedCard(rand() % 10);
	avatarStatsReport12.setShotsOnGoal(rand() % 10);
	avatarStatsReport12.setUnadjustedScore(rand() % 10);
	avatarStatsReport12.setYellowCard(rand() % 10);
	if (mScenarioType == SSF_SINGLE_MATCH)
	{
		Blaze::GameReporting::SSFSeasonsReportBase::SkillTimeBucketMap &avatarSkillTime = avEntry12->getSkillTimeBucketMap();
		avatarSkillTime.clear();
		//Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap1 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
		//Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap2 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
		/*Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType11 = 1;
		Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType12 = 3;
		Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType13 = 39;
		Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType21 = 40;*/
		//Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType22 = 29;

		/*EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent11 = fillSkillEvent(0, 2, 25);
		EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent12 = fillSkillEvent(0, 1, 29);
		EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent13 = fillSkillEvent(0, 2, 33);
		EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent21 = fillSkillEvent(0, 2, 25);*/
		//EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent22 = fillSkillEvent(0, 1, 29);

		//skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType11, skillEvent11)));
		//skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType12, skillEvent12)));
		//skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType13, skillEvent13)));
		//skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType21, skillEvent21)));
		//skillEventmap2->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType22, skillEvent22)));

		/*auto it = avatarSkillTime.begin();
		it->first = 11;
		it->second->setTimePeriod(5);
		skillEventmap1->copyInto(it->second->getSkillMap());*/
		/*++it;
		it->first = 8;
		it->second->setTimePeriod(8);
		skillEventmap2->copyInto(it->second->getSkillMap());*/


	}
	avVector2.push_back(avEntry12);		// 1

	//avatarvector entry 3
	Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry13 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
	Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId13 = avEntry13->getAvatarId();
	avatarId13.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
	avatarId13.setPlayerId(2);
	avatarId13.setSlotId(rand() % 10);
	Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport13 = avEntry13->getAvatarStatReport();
	avatarStatsReport13.setAveragePossessionLength(rand() % 12);
	avatarStatsReport13.setAssists(rand() % 10);
	avatarStatsReport13.setBlocks(rand() % 10); // missing
	avatarStatsReport13.setGoalsConceded(rand() % 10);
	avatarStatsReport13.setHasCleanSheets(rand() % 10);
	avatarStatsReport13.setCorners(rand() % 10);
	avatarStatsReport13.setPassAttempts(rand() % 105);
	avatarStatsReport13.setPassesMade(rand() % 107);
	avatarStatsReport13.setRating((float)(0.00000000));
	avatarStatsReport13.setSaves(rand() % 10);
	avatarStatsReport13.setShots(rand() % 10);
	avatarStatsReport13.setTackleAttempts(rand() % 15);
	avatarStatsReport13.setTacklesMade(rand() % 10);
	avatarStatsReport13.setFouls(rand() % 12);
	avatarStatsReport13.setAverageFatigueAfterNinety(rand() % 10);
	avatarStatsReport13.setGoals(rand() % 10);
	avatarStatsReport13.setInjuriesRed(rand() % 10);
	avatarStatsReport13.setInterceptions(rand() % 10);
	avatarStatsReport13.setHasMOTM(rand() % 12);
	avatarStatsReport13.setOffsides(rand() % 10);
	avatarStatsReport13.setOwnGoals(rand() % 15);
	avatarStatsReport13.setPkGoals(rand() % 10);
	avatarStatsReport13.setPossession(rand() % 10);
	avatarStatsReport13.setPassesIntercepted(rand() % 15);
	avatarStatsReport13.setPenaltiesAwarded(rand() % 10);
	avatarStatsReport13.setPenaltiesMissed(rand() % 15);
	avatarStatsReport13.setPenaltiesScored(rand() % 10);
	avatarStatsReport13.setPenaltiesSaved(rand() % 15);
	avatarStatsReport13.setRedCard(rand() % 10);
	avatarStatsReport13.setShotsOnGoal(rand() % 15);
	avatarStatsReport13.setUnadjustedScore(rand() % 10);
	avatarStatsReport13.setYellowCard(rand() % 10);
	if (mScenarioType == SSF_SINGLE_MATCH)
	{
		Blaze::GameReporting::SSFSeasonsReportBase::SkillTimeBucketMap &avatarSkillTime = avEntry13->getSkillTimeBucketMap();
		avatarSkillTime.clear();
		/*Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap1 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
		Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap2 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
		Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType11 = 17;
		Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType12 = 34;
		Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType13 = 45;
		Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType21 = 3;
		Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType22 = 59;

		EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent11 = fillSkillEvent(0, 2, 7);
		EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent12 = fillSkillEvent(0, 2, 7);
		EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent13 = fillSkillEvent(0, 2, 7);
		
		EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent21 = fillSkillEvent(0, 1, 25);
		EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent22 = fillSkillEvent(0, 1, 29);

		skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType11, skillEvent11)));
		skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType12, skillEvent12)));
		skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType13, skillEvent13)));
		
		skillEventmap2->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType21, skillEvent21)));
		skillEventmap2->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType22, skillEvent22)));

		auto it = avatarSkillTime.begin();
		it->first = 5;
		it->second->setTimePeriod(1);
		skillEventmap1->copyInto(it->second->getSkillMap());
		++it;
		it->first = 10;
		it->second->setTimePeriod(2);
		skillEventmap2->copyInto(it->second->getSkillMap());*/

	}
	//if (mScenarioType != SSF_WORLD_TOUR)
	//{
	avVector2.push_back(avEntry13);		// 2
	//}
	//if (mScenarioType != SSF_TWIST_MATCH && mScenarioType != SSF_WORLD_TOUR)
	{
		//avatarvector entry 4
		Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry14 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
		Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId14 = avEntry14->getAvatarId();
		avatarId14.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
		avatarId14.setPlayerId(3);
		avatarId14.setSlotId(rand() % 10);
		Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport14 = avEntry14->getAvatarStatReport();
		avatarStatsReport14.setAveragePossessionLength(rand() % 10);
		avatarStatsReport14.setAssists(rand() % 10);
		avatarStatsReport14.setBlocks(rand() % 12); // missing
		avatarStatsReport14.setGoalsConceded(rand() % 10);
		avatarStatsReport14.setHasCleanSheets(rand() % 15);
		avatarStatsReport14.setCorners(rand() % 12);
		avatarStatsReport14.setPassAttempts(rand() % 105);
		avatarStatsReport14.setPassesMade(rand() % 105);
		avatarStatsReport14.setRating((float)(0.00000000));
		avatarStatsReport14.setSaves(rand() % 10);
		avatarStatsReport14.setShots(rand() % 10);
		avatarStatsReport14.setTackleAttempts(rand() % 15);
		avatarStatsReport14.setTacklesMade(rand() % 15);
		avatarStatsReport14.setFouls(rand() % 15);
		avatarStatsReport14.setAverageFatigueAfterNinety(rand() % 10);
		avatarStatsReport14.setGoals(rand() % 10);
		avatarStatsReport14.setInjuriesRed(rand() % 10);
		avatarStatsReport14.setInterceptions(rand() % 10);
		avatarStatsReport14.setHasMOTM(rand() % 10);
		avatarStatsReport14.setOffsides(rand() % 12);
		avatarStatsReport14.setOwnGoals(rand() % 10);
		avatarStatsReport14.setPkGoals(rand() % 10);
		avatarStatsReport14.setPossession(rand() % 10);
		avatarStatsReport14.setPassesIntercepted(rand() % 15);
		avatarStatsReport14.setPenaltiesAwarded(rand() % 10);
		avatarStatsReport14.setPenaltiesMissed(rand() % 10);
		avatarStatsReport14.setPenaltiesScored(rand() % 10);
		avatarStatsReport14.setPenaltiesSaved(rand() % 10);
		avatarStatsReport14.setRedCard(rand() % 10);
		avatarStatsReport14.setShotsOnGoal(rand() % 10);
		avatarStatsReport14.setUnadjustedScore(rand() % 10);
		avatarStatsReport14.setYellowCard(rand() % 10);
		if (mScenarioType == SSF_SINGLE_MATCH)
		{
			Blaze::GameReporting::SSFSeasonsReportBase::SkillTimeBucketMap &avatarSkillTime = avEntry14->getSkillTimeBucketMap();
			avatarSkillTime.initMap(2);
			/*Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap1 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
			Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap2 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
			
			Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType11 = 1;
			Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType12 = 13;
			Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType21 = 25;
			Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType22 = 29;

			EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent11 = fillSkillEvent(0, 2, 33);
			EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent12 = fillSkillEvent(1, 4, 60);

			skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType11, skillEvent11)));
			skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType12, skillEvent12)));
			
			skillEventmap2->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType21, skillEvent11)));
			skillEventmap2->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType22, skillEvent12)));

			auto it = avatarSkillTime.begin();
			it->first = 3;
			it->second->setTimePeriod(3);
			skillEventmap1->copyInto(it->second->getSkillMap());
			++it;
			it->first = 5;
			it->second->setTimePeriod(5);
			skillEventmap2->copyInto(it->second->getSkillMap());*/
		}
		//avVector2.push_back(avEntry14);	// 3

	}

	if (mScenarioType == SSF_WORLD_TOUR)
	{
		Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry *avEntry15 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::AvatarEntry;
		Blaze::GameReporting::SSFSeasonsReportBase::AvatarId &avatarId15 = avEntry15->getAvatarId();
		avatarId15.setPersonaId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
		avatarId15.setPlayerId(3);
		avatarId15.setSlotId(0);
		Blaze::GameReporting::SSF::CommonStatsReport &avatarStatsReport15 = avEntry15->getAvatarStatReport();
		avatarStatsReport15.setAveragePossessionLength(rand() % 10);
		avatarStatsReport15.setAssists(rand() % 10);
		//avatarStatsReport15.setBlocks(0); // missing
		avatarStatsReport15.setGoalsConceded(rand() % 10);
		avatarStatsReport15.setHasCleanSheets(rand() % 10);
		avatarStatsReport15.setCorners(rand() % 15);
		avatarStatsReport15.setPassAttempts(rand() % 15);
		avatarStatsReport15.setPassesMade(rand() % 12);
		avatarStatsReport15.setRating((float)(rand() % 10));
		avatarStatsReport15.setSaves(rand() % 10);
		avatarStatsReport15.setShots(rand() % 12);
		avatarStatsReport15.setTackleAttempts(rand() % 12);
		avatarStatsReport15.setTacklesMade(rand() % 15);
		avatarStatsReport15.setFouls(rand() % 12);
		avatarStatsReport15.setAverageFatigueAfterNinety(rand() % 10);
		avatarStatsReport15.setGoals(rand() % 15);
		avatarStatsReport15.setInjuriesRed(0);
		avatarStatsReport15.setInterceptions(rand() % 10);
		avatarStatsReport15.setHasMOTM(0);
		avatarStatsReport15.setOffsides(rand() % 15);
		avatarStatsReport15.setOwnGoals(rand() % 10);
		avatarStatsReport15.setPkGoals(rand() % 10);
		avatarStatsReport15.setPossession(rand() % 15);
		avatarStatsReport15.setPassesIntercepted(0);
		avatarStatsReport15.setPenaltiesAwarded(0);
		avatarStatsReport15.setPenaltiesMissed(0);
		avatarStatsReport15.setPenaltiesScored(0);
		avatarStatsReport15.setPenaltiesSaved(0);
		avatarStatsReport15.setRedCard(rand() % 12);
		avatarStatsReport15.setShotsOnGoal(rand() % 15);
		avatarStatsReport15.setUnadjustedScore(rand() % 10);
		avatarStatsReport15.setYellowCard(rand() % 12);
		
		/*if (mScenarioType == SSF_WORLD_TOUR)
		{
			Blaze::GameReporting::SSFSeasonsReportBase::SkillTimeBucketMap &avatarSkillTime = avEntry15->getSkillTimeBucketMap();
			avatarSkillTime.initMap(2);
			Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap1 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
			Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap *skillEventmap2 = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap();
			Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType11 = 7;
			Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType12 = 25;
			Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType13 = 29;
			Blaze::GameReporting::SSFSeasonsReportBase::SkillType objSkillType21 = 29;
		
			EA::TDF::tdf_ptr < Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent11 = fillSkillEvent(0, 1, 7);
			EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent12 = fillSkillEvent(0, 1, 25);
			EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent13 = fillSkillEvent(0, 2, 29);
			EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent> skillEvent21 = fillSkillEvent(0, 2, 29);
		
			skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType11, skillEvent11)));
			skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType12, skillEvent12)));
			skillEventmap1->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType13, skillEvent13)));
			skillEventmap2->insert(Blaze::GameReporting::SSFSeasonsReportBase::SkillEventMap::value_type(eastl::pair<Blaze::GameReporting::SSFSeasonsReportBase::SkillType, EA::TDF::tdf_ptr<Blaze::GameReporting::SSFSeasonsReportBase::SkillEvent>>(objSkillType21, skillEvent21)));
		
			auto it = avatarSkillTime.begin();
			it->first = 2;
			it->second->setTimePeriod(2);
			skillEventmap1->copyInto(it->second->getSkillMap());
			++it;
			it->first = 5;
			it->second->setTimePeriod(5);
			skillEventmap2->copyInto(it->second->getSkillMap());
		}*/
		
		avVector2.push_back(avEntry15);	// 4
	}

	Blaze::GameReporting::SSF::CommonStatsReport& commonTeamReport2 = teamReport2->getCommonTeamReport();
	commonTeamReport2.setAveragePossessionLength(rand() % 10);
	commonTeamReport2.setAssists(rand() % 10);
	commonTeamReport2.setBlocks(rand() % 10); // missing
	commonTeamReport2.setGoalsConceded(rand() % 12);
	commonTeamReport2.setHasCleanSheets(rand() % 12);
	commonTeamReport2.setCorners(rand() % 12);
	commonTeamReport2.setPassAttempts(rand() % 105);
	commonTeamReport2.setPassesMade(rand() % 17);
	commonTeamReport2.setRating((float)(0.00000000));
	commonTeamReport2.setSaves(rand() % 10);
	commonTeamReport2.setShots(rand() % 17);
	commonTeamReport2.setTackleAttempts(rand() % 15);
	commonTeamReport2.setTacklesMade(rand() % 10);
	commonTeamReport2.setFouls(rand() % 10);
	commonTeamReport2.setAverageFatigueAfterNinety(rand() % 15);
	commonTeamReport2.setGoals(rand() % 10);
	commonTeamReport2.setInjuriesRed(rand() % 10);
	commonTeamReport2.setInterceptions(rand() % 10);
	commonTeamReport2.setHasMOTM(rand() % 12);
	commonTeamReport2.setOffsides(rand() % 10);
	commonTeamReport2.setOwnGoals(rand() % 10);
	commonTeamReport2.setPkGoals(rand() % 10);
	commonTeamReport2.setPossession(rand() % 15);
	commonTeamReport2.setPassesIntercepted(rand() % 15);
	commonTeamReport2.setPenaltiesAwarded(rand() % 10);
	commonTeamReport2.setPenaltiesMissed(rand() % 12);
	commonTeamReport2.setPenaltiesScored(rand() % 10);
	commonTeamReport2.setPenaltiesSaved(rand() % 12);
	commonTeamReport2.setRedCard(rand() % 10);
	commonTeamReport2.setShotsOnGoal(rand() % 10);
	commonTeamReport2.setUnadjustedScore(rand() % 10);
	commonTeamReport2.setYellowCard(rand() % 12);
	ssfTeamReportMap[mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId()] = teamReport2;

	osdkReport->setTeamReports(*ssfTeamSummaryReport);

	//char8_t buf[50240];
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
}
}