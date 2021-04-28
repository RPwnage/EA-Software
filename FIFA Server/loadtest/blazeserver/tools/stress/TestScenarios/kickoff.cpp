//  *************************************************************************************************
//
//   File:    kickoff.cpp
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "kickoff.h"
#include "playermodule.h"
#include "gamesession.h"
#include "framework/blaze.h"
#include "loginmanager.h"
#include "gamemanager/tdf/matchmaker.h"

namespace Blaze {
namespace Stress {

UserIdentMap         KickOffPlayer::kickOffPlayerList;
EA::Thread::Mutex    KickOffPlayer::kickOffPlayerListMutex;

KickOffPlayer::KickOffPlayer(StressPlayerInfo* playerData)
    : GamePlayUser(playerData), mConfig(KICKOFF_GAMEPLAY_CONFIG)
{
	mNucleusAccessToken		= "";
	
	mFriendsGroup_Name		= "";
	mOverallGroupA_Name		= "";
	mOverallGroupB_Name		= "";
	mRivalGroup_Name		= "";

	mFriendsGroup_ID		= "";
	mOverallGroupA_ID		= "";
	mOverallGroupB_ID		= "";
	mRivalGroup_ID			= "";

	mLevel					= DifficultyLevel::EASY;
	mScenarioType			= KO_Scenario::TEAM_1_1;
	mPlayersType			= KO_PlayersType::TEAM_ALL_REG;

	mGroupsProxyHandler		= BLAZE_NEW GroupsProxyHandler(playerData);
	mStatsProxyHandler		= BLAZE_NEW StatsProxyHandler(playerData);

	mKickOffScenarioType = "\0";
}

KickOffPlayer::~KickOffPlayer()
{
	if (mGroupsProxyHandler != NULL)
	{
		delete mGroupsProxyHandler;
	}
	if (mStatsProxyHandler != NULL)
	{
		delete mStatsProxyHandler;
	}
}

BlazeRpcError KickOffPlayer::simulateLoad()
{
	BlazeRpcError err = ERR_OK;
	bool result = false;
    //BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::simulateLoad]:" << mPlayerInfo->getPlayerId());
	gameTypeSetter();

	/*KickOffDistributionMap distributionReference = StressConfigInst.getKickOffScenarioDistribution();
	KickOffDistributionMap::iterator it = distributionReference.begin();
	uint8_t sumProb = 0;
	uint8_t randNumber = (uint8_t)Random::getRandomNumber(100);
	while (it != distributionReference.end())
	{
		sumProb += it->second;
		if (randNumber < sumProb)
		{
			mKickOffScenarioType = it->first;
			break;
		}
		it++;
	}*/

	if (mPlayerInfo->getLogin()->isStressLogin())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][Kickoff::simulateLoad]: Kickoff scenario requires nucleus login");
		sleepRandomTime(120000, 180000);
		return ERR_SYSTEM;
	}
    if (!mConfig)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][Kickoff::simulateLoad]: Failed to Simulate Load, Configuration is not loaded");
		sleepRandomTime(30000, 60000);
        return ERR_SYSTEM;
    }
	// Read accessToken and save to mNucleusAccessToken
	if (!getMyAccessToken())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][Kickoff::SetAccessToken]: Failed to get Access Token!!");
		sleepRandomTime(60000, 120000);
		return ERR_SYSTEM;
	}

	simulateMenuFlow();
	lobbyRpcCalls();
	initScenarioType();
	//mScenarioType = KO_Scenario::TEAM_2_2;
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::simulateLoad]:" << mPlayerInfo->getPlayerId() << " mScenarioType= " << getScearioType());

	initPlayersType();
	//mPlayersType = TEAM_ALL_REG;
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::simulateLoad]:" << mPlayerInfo->getPlayerId() << " mPlayersType= " << getKO_PlayersType());

	initDifficultyLevel();
	//mLevel = DifficultyLevel::EASY;
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::simulateLoad]:" << mPlayerInfo->getPlayerId() << " DifficultyLevel= " << getDifficultyLevel());

	// playerIDs for Registered and Un-Registered list
	result = initFriendPlayerIDs();
	if (!result) 
	{
		sleepRandomTime(30000, 60000);
		return ERR_SYSTEM; 
	}

	// Create and Join Groups
	result = createAndJoinGroups();
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::simulateLoad]:" << mPlayerInfo->getPlayerId() << " mFriendsGroupName= " << mFriendsGroup_Name);
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::simulateLoad]:" << mPlayerInfo->getPlayerId() << " mOverallGroup_AsideName= " << mOverallGroupA_Name);
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::simulateLoad]:" << mPlayerInfo->getPlayerId() << " mOverallGroup_BsideName= " << mOverallGroupB_Name);
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::simulateLoad]:" << mPlayerInfo->getPlayerId() << " mRivalGroup_Name= " << mRivalGroup_Name);
	if (!result)
	{
		sleepRandomTime(30000, 60000);
		return ERR_SYSTEM; 
	}
	
	sleepRandomTime(5000, 10000);

	lookupUsersByPersonaNames(mPlayerInfo, "");
	setClientState(mPlayerInfo, ClientState::MODE_SINGLE_PLAYER);

	int64_t gameLifeSpan = KICKOFF_CONFIG.RoundInGameDuration;
    int64_t gameTime = gameLifeSpan + (KICKOFF_CONFIG.InGameDeviation > 0 ? (rand() % KICKOFF_CONFIG.InGameDeviation) : 0);

	mSinglePlayerGameStartTime = TimeValue::getTimeOfDay();   

	// PlayerTimeStats RPC call
	if (StressConfigInst.getPTSEnabled())
	{
		incrementMatchPlayed(mPlayerInfo);
	}

	while (1)
	{
		if (!mPlayerInfo->isConnected())
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[SinglePlayer::SinglePlayerGame::User Disconnected = " << mPlayerInfo->getPlayerId());
			return ERR_DISCONNECTED;
		}
		//Sleep
		sleepRandomTime(30000, 60000);

		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "KickOffPlayer::simulateLoad - IN_GAME")

		//   if user game time has elapsed, send offline report and start next game cycle
		TimeValue elpasedTime = TimeValue::getTimeOfDay() - mSinglePlayerGameStartTime;
		//   game over
		if (elpasedTime.getMillis() >= gameTime || RollDice(KICKOFF_CONFIG.QuitFromGameProbability))
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "KickOffPlayer::simulateLoad - POST_GAME")
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "KickOffPlayer::simulateLoad - submitOfflineGameReport will be sent")
			submitOfflineGameReport();
			break;
		}
	}//while

	//After long time token will become invaild, so calling again
	//Read accessToken and save to mNucleusAccessToken
	if (!getMyAccessToken())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][Kickoff::SetAccessToken-2]: Failed to get Access Token!!");
		return ERR_SYSTEM;
	}

	//sleepRandomTime(5000, 10000);
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "KickOffPlayer::simulateLoad - updateStats will be invoked");
	 //Rival
	updateStats_GroupID(EADP_GROUPS_RIVAL_TYPE);
	//Overall (for overall group A and overall group B)
	updateStats_GroupID(EADP_GROUPS_OVERALL_TYPE_A);	 
	//UserCustom (for all players in the game)
	updateStats_GroupID(EADP_GROUPS_FRIEND_TYPE);

	//sleepRandomTime(5000, 10000);

	 //setMultipleInstanceAttributes
	 //Update the last time accessed attribute for each group
	 setMultipleInstanceAttributes(EADP_GROUPS_RIVAL_TYPE);
	 setMultipleInstanceAttributes(EADP_GROUPS_OVERALL_TYPE_A);
	 //CPU then skip this
	 if (mScenarioType != TEAM_1_CPU && mScenarioType != TEAM_2_CPU)
	 {
		 setMultipleInstanceAttributes(EADP_GROUPS_OVERALL_TYPE_B);
	 }	 
	 
	 //View Stats from EADP Stats
	 BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "KickOffPlayer::simulateLoad - viewStats will be invoked");
	 viewStats();

	 //Search for group members list
	 BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "KickOffPlayer::simulateLoad - calling GroupId with the PageSize");
	 searchGroupMembers();

	 sleepRandomTime(5000, 10000);

	return err;
}

BlazeRpcError KickOffPlayer::lobbyRpcCalls()
{
	BlazeRpcError err = ERR_OK;
	FetchConfigResponse response;
	SponsoredEvents::CheckUserRegistrationResponse checkUserRegistrationResponse;
	GetStatsByGroupRequest::EntityIdList entityList;
	Blaze::BlazeIdList friendsList;
	friendsList.push_back(mPlayerInfo->getPlayerId());
	//char configSection[64];
	uint32_t viewId = mPlayerInfo->getViewId();
	entityList.push_back(mPlayerInfo->getPlayerId());
	getStatGroup(mPlayerInfo, "VProPosition");
	//getStatsByGroupAsync(mPlayerInfo, entityList, "VProPosition", getMyClubId());
	mPlayerInfo->setViewId(++viewId);
	setConnectionState(mPlayerInfo);
	setClientState(mPlayerInfo, Blaze::ClientState::MODE_SINGLE_PLAYER, Blaze::ClientState::STATUS_SUSPENDED);
	unsubscribeFromCensusData(mPlayerInfo);
	return err;
}

void KickOffPlayer::initScenarioType()
{
	// Randomly choose one of the scenario type
	uint16_t randomValue = (uint16_t)Blaze::Random::getRandomNumber(100);
	if (randomValue < 30)
	{
		mScenarioType = KO_Scenario::TEAM_1_1;
	}
	else if (randomValue < 50)
	{
		mScenarioType = KO_Scenario::TEAM_1_2;
	}
	else if (randomValue < 70)
	{
		mScenarioType = KO_Scenario::TEAM_2_2;
	}
	else if (randomValue < 90)
	{
		mScenarioType = KO_Scenario::TEAM_1_CPU;
	}
	else //if (randomValue < 100)
	{
		mScenarioType = KO_Scenario::TEAM_2_CPU;
	}
}

bool KickOffPlayer::createAndJoinGroups()
{
	bool result = true;
	BlazeRpcError err = ERR_OK;
	//Create Group Names for all types
	result = createGroupNames();
	if (!result) { return result; }
	
	//ReadGroupIDs from EADP Groups service using HTTP calls
	result = readGroupIDs();
	sleepRandomTime(5000, 10000);
	if (!result)
	{
		//CreateGroups, if read groups failed from EADP stats
		if (mFriendsGroup_ID == "") {
			err = createInstance(EADP_GROUPS_FRIEND_TYPE);
			if (err != ERR_OK || mFriendsGroup_ID == "") 
			{ 
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::simulateload]" << mPlayerInfo->getPlayerId() << " createInstance failed");
				return false; 
			}
		}
		if (mOverallGroupA_ID == "") {
			err = createInstance(EADP_GROUPS_OVERALL_TYPE_A);
			if (err != ERR_OK || mOverallGroupA_ID == "") 
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createAndJoinGroups]" << mPlayerInfo->getPlayerId() << " createInstance failed");
				return false; 
			}
		}
		// Opponent CPU no need to create
		if (mScenarioType != TEAM_1_CPU && mScenarioType != TEAM_2_CPU)
		{
			if (mOverallGroupB_ID == "") {
				err = createInstance(EADP_GROUPS_OVERALL_TYPE_B);
				if (err != ERR_OK || mOverallGroupB_ID == "") 
				{ 
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createAndJoinGroups]" << mPlayerInfo->getPlayerId() << " createInstance failed");
					return false; 
				}
			}
		}
		if (mRivalGroup_ID == "") {
			err = createInstance(EADP_GROUPS_RIVAL_TYPE);
			if (err != ERR_OK || mRivalGroup_ID == "") 
			{ 
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createAndJoinGroups]" << mPlayerInfo->getPlayerId() << " createInstance failed"); 
				return false; 
			}
		}
	}
	//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::simulateLoad]:" << mPlayerInfo->getPlayerId() << " mFriendsGroup_ID= " << mFriendsGroup_ID);
	//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::simulateLoad]:" << mPlayerInfo->getPlayerId() << " mOverallGroupA_ID= " << mOverallGroupA_ID);
	//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::simulateLoad]:" << mPlayerInfo->getPlayerId() << " mOverallGroupB_ID= " << mOverallGroupB_ID);
	//BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::simulateLoad]:" << mPlayerInfo->getPlayerId() << " mRivalGroup_ID= " << mRivalGroup_ID);

	sleepRandomTime(5000, 10000);

	//JoinGroupRequest only for Friends Group
	//CreateAndJoin RPC is called by Blaze to Groups
	//Failing with VALIDATION_ERROR if faked persona id used
	if (mPlayersType == TEAM_ALL_REG)
	{
		switch (mScenarioType)
		{
			case TEAM_1_1:
			case TEAM_2_CPU:
			{
				err = joinGroup(mFriendsGroup_ID, mRegisteredPlayersList[0]);
			}
			break;
			case TEAM_1_2:
			{
				err = joinGroup(mFriendsGroup_ID, mRegisteredPlayersList[0]);
				err = joinGroup(mFriendsGroup_ID, mRegisteredPlayersList[1]);
			}
			break;
			case TEAM_2_2:
			{
				err = joinGroup(mFriendsGroup_ID, mRegisteredPlayersList[0]);
				err = joinGroup(mFriendsGroup_ID, mRegisteredPlayersList[1]);
				err = joinGroup(mFriendsGroup_ID, mRegisteredPlayersList[2]);
			}
			break;
			case TEAM_1_CPU:
			{
			}
			break;
			default:
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createAndJoinGroups]" << mPlayerInfo->getPlayerId() << " In default state");
				return false;
			}
			break;
		}
	}
	sleepRandomTime(5000, 10000);
	if (mPlayersType == TEAM_ALL_UNREG)
	{
		//setMultipleInstanceAttributes	 TBD-check for number of players	
		err = setMultipleInstanceAttributes(EADP_GROUPS_FRIEND_TYPE);
	}
	return true;
}

bool KickOffPlayer::initFriendPlayerIDs()
{
	//Create Registered Player List
	//TODO - Get Persona ID from Guest Login
	// Xone Guest PersonaIDs in IAD1 - 1100042071363, 1100011029451, 1100061114378, 1100027863033, 1100057119306,
	// 1100056524282, 1100022459669, 1100023225573, 1100079911536, 1100042890123, 1100041452943, 1100038630016
	// PS4 Guest PersonaIDs - TBD

	//Xone
	//Guest accounts
	//vector<BlazeId> XonePlayersList = { 1100042071363, 1100011029451, 1100061114378, 1100027863033, 1100057119306, 1100056524282, 1100022459669, 1100023225573, 1100079911536, 1100042890123, 1100041452943, 1100038630016 };
	//Fake accounts
	vector<BlazeId> XonePlayersList = {1100042071363,1100011029451,1100061114378,1100027863033,1100057119306,1100056524282,1100022459669,1100023225573,1100079911536,1100042890123,1100041452943,1100038630016,2000597350001,2000597350002,2000597350003,2000597350004,2000597350005,2000597350006,2000597350007,2000597350008,2000597350009,2000597350010,2000597350011,2000597350012,2000597350013,2000597350014,2000597350015,2000597350016,2000597350017,2000597350018,2000597350019,2000597350020,2000597350021,2000597350022,2000597350023,2000597350024,2000597350025,2000597350026,2000597350027,2000597350028,2000597350029,2000597350030,2000597350031,2000597350032,2000597350033,2000597350034,2000597350035,2000597350036,2000597350037,2000597350038,2000597350039,2000597350040,2000597350041,2000597350042,2000597350043,2000597350044,2000597350045,2000597350046,2000597350047,2000597350048,2000597350049,2000597350050,2000597350051,2000597350052,2000597350053,2000597350054,2000597350055,2000597350056,2000597350057,2000597350058,2000597350059,2000597350060,2000597350061,2000597350062,2000597350063,2000597350064,2000597350065,2000597350066,2000597350067,2000597350068,2000597350069,2000597350070,2000597350071,2000597350072,2000597350073,2000597350074,2000597350075,2000597350076,2000597350077,2000597350078,2000597350079,2000597350080,2000597350081,2000597350082,2000597350083,2000597350084,2000597350085,2000597350086,2000597350087,2000597350088,2000597350089,2000597350090,2000597350091,2000597350092,2000597350093,2000597350094,2000597350095,2000597350096,2000597350097,2000597350098,2000597350099,2000597350100,2000597350101,2000597350102,2000597350103,2000597350104,2000597350105,2000597350106,2000597350107,2000597350108,2000597350109,2000597350110,2000597350111,2000597350112,2000597350113,2000597350114,2000597350115,2000597350116,2000597350117,2000597350118,2000597350119,2000597350120,2000597350121,2000597350122,2000597350123,2000597350124,2000597350125,2000597350126,2000597350127,2000597350128,2000597350129,2000597350130,2000597350131,2000597350132,2000597350133,2000597350134,2000597350135,2000597350136,2000597350137,2000597350138,2000597350139,2000597350140,2000597350141,2000597350142,2000597350143,2000597350144,2000597350145,2000597350146,2000597350147,2000597350148,2000597350149,2000597350150,2000597350151,2000597350152,2000597350153,2000597350154,2000597350155,2000597350156,2000597350157,2000597350158,2000597350159,2000597350160,2000597350161,2000597350162,2000597350163,2000597350164,2000597350165,2000597350166,2000597350167,2000597350168,2000597350169,2000597350170,2000597350171,2000597350172,2000597350173,2000597350174,2000597350175,2000597350176,2000597350177,2000597350178,2000597350179,2000597350180,2000597350181,2000597350182,2000597350183,2000597350184,2000597350185,2000597350186,2000597350187,2000597350188,2000597350189,2000597350190,2000597350191,2000597350192,2000597350193,2000597350194,2000597350195,2000597350196,2000597350197,2000597350198,2000597350199,2000597350200,2000597350201,2000597350202,2000597350203,2000597350204,2000597350205,2000597350206,2000597350207,2000597350208,2000597350209,2000597350210,2000597350211,2000597350212,2000597350213,2000597350214,2000597350215,2000597350216,2000597350217,2000597350218,2000597350219,2000597350220,2000597350221,2000597350222,2000597350223,2000597350224,2000597350225,2000597350226,2000597350227,2000597350228,2000597350229,2000597350230,2000597350231,2000597350232,2000597350233,2000597350234,2000597350235,2000597350236,2000597350237,2000597350238,2000597350239,2000597350240,2000597350241,2000597350242,2000597350243,2000597350244,2000597350245,2000597350246,2000597350247,2000597350248,2000597350249,2000597350250,2000597350251,2000597350252,2000597350253,2000597350254,2000597350255,2000597350256,2000597350257,2000597350258,2000597350259,2000597350260,2000597350261,2000597350262,2000597350263,2000597350264,2000597350265,2000597350266,2000597350267,2000597350268,2000597350269,2000597350270,2000597350271,2000597350272,2000597350273,2000597350274,2000597350275,2000597350276,2000597350277,2000597350278,2000597350279,2000597350280,2000597350281,2000597350282,2000597350283,2000597350284,2000597350285,2000597350286,2000597350287,2000597350288,2000597350289,2000597350290,2000597350291,2000597350292,2000597350293,2000597350294,2000597350295,2000597350296,2000597350297,2000597350298,2000597350299,2000597350300,2000597350301,2000597350302,2000597350303,2000597350304,2000597350305,2000597350306,2000597350307,2000597350308,2000597350309,2000597350310,2000597350311,2000597350312,2000597350313,2000597350314,2000597350315,2000597350316,2000597350317,2000597350318,2000597350319,2000597350320,2000597350321,2000597350322,2000597350323,2000597350324,2000597350325,2000597350326,2000597350327,2000597350328,2000597350329,2000597350330,2000597350331,2000597350332,2000597350333,2000597350334,2000597350335,2000597350336,2000597350337,2000597350338,2000597350339,2000597350340,2000597350341,2000597350342,2000597350343,2000597350344,2000597350345,2000597350346,2000597350347,2000597350348,2000597350349,2000597350350,2000597350351,2000597350352,2000597350353,2000597350354,2000597350355,2000597350356,2000597350357,2000597350358,2000597350359,2000597350360,2000597350361,2000597350362,2000597350363,2000597350364,2000597350365,2000597350366,2000597350367,2000597350368,2000597350369,2000597350370,2000597350371,2000597350372,2000597350373,2000597350374,2000597350375,2000597350376,2000597350377,2000597350378,2000597350379,2000597350380,2000597350381,2000597350382,2000597350383,2000597350384,2000597350385,2000597350386,2000597350387,2000597350388,2000597350389,2000597350390,2000597350391,2000597350392,2000597350393,2000597350394,2000597350395,2000597350396,2000597350397,2000597350398,2000597350399,2000597350400};
	
	//PS4
	//Fake accounts
	vector<BlazeId> PS4PlayersList = { 1100026291282,1100026244328,1100060916573,1100046259392,1100062315282,1100020826398,1100060118094,1100026673552,1100020431861,1100029437591,2000597350001,2000597350002,2000597350003,2000597350004,2000597350005,2000597350006,2000597350007,2000597350008,2000597350009,2000597350010,2000597350011,2000597350012,2000597350013,2000597350014,2000597350015,2000597350016,2000597350017,2000597350018,2000597350019,2000597350020,2000597350021,2000597350022,2000597350023,2000597350024,2000597350025,2000597350026,2000597350027,2000597350028,2000597350029,2000597350030,2000597350031,2000597350032,2000597350033,2000597350034,2000597350035,2000597350036,2000597350037,2000597350038,2000597350039,2000597350040,2000597350041,2000597350042,2000597350043,2000597350044,2000597350045,2000597350046,2000597350047,2000597350048,2000597350049,2000597350050,2000597350051,2000597350052,2000597350053,2000597350054,2000597350055,2000597350056,2000597350057,2000597350058,2000597350059,2000597350060,2000597350061,2000597350062,2000597350063,2000597350064,2000597350065,2000597350066,2000597350067,2000597350068,2000597350069,2000597350070,2000597350071,2000597350072,2000597350073,2000597350074,2000597350075,2000597350076,2000597350077,2000597350078,2000597350079,2000597350080,2000597350081,2000597350082,2000597350083,2000597350084,2000597350085,2000597350086,2000597350087,2000597350088,2000597350089,2000597350090,2000597350091,2000597350092,2000597350093,2000597350094,2000597350095,2000597350096,2000597350097,2000597350098,2000597350099,2000597350100,2000597350101,2000597350102,2000597350103,2000597350104,2000597350105,2000597350106,2000597350107,2000597350108,2000597350109,2000597350110,2000597350111,2000597350112,2000597350113,2000597350114,2000597350115,2000597350116,2000597350117,2000597350118,2000597350119,2000597350120,2000597350121,2000597350122,2000597350123,2000597350124,2000597350125,2000597350126,2000597350127,2000597350128,2000597350129,2000597350130,2000597350131,2000597350132,2000597350133,2000597350134,2000597350135,2000597350136,2000597350137,2000597350138,2000597350139,2000597350140,2000597350141,2000597350142,2000597350143,2000597350144,2000597350145,2000597350146,2000597350147,2000597350148,2000597350149,2000597350150,2000597350151,2000597350152,2000597350153,2000597350154,2000597350155,2000597350156,2000597350157,2000597350158,2000597350159,2000597350160,2000597350161,2000597350162,2000597350163,2000597350164,2000597350165,2000597350166,2000597350167,2000597350168,2000597350169,2000597350170,2000597350171,2000597350172,2000597350173,2000597350174,2000597350175,2000597350176,2000597350177,2000597350178,2000597350179,2000597350180,2000597350181,2000597350182,2000597350183,2000597350184,2000597350185,2000597350186,2000597350187,2000597350188,2000597350189,2000597350190,2000597350191,2000597350192,2000597350193,2000597350194,2000597350195,2000597350196,2000597350197,2000597350198,2000597350199,2000597350200,2000597350201,2000597350202,2000597350203,2000597350204,2000597350205,2000597350206,2000597350207,2000597350208,2000597350209,2000597350210,2000597350211,2000597350212,2000597350213,2000597350214,2000597350215,2000597350216,2000597350217,2000597350218,2000597350219,2000597350220,2000597350221,2000597350222,2000597350223,2000597350224,2000597350225,2000597350226,2000597350227,2000597350228,2000597350229,2000597350230,2000597350231,2000597350232,2000597350233,2000597350234,2000597350235,2000597350236,2000597350237,2000597350238,2000597350239,2000597350240,2000597350241,2000597350242,2000597350243,2000597350244,2000597350245,2000597350246,2000597350247,2000597350248,2000597350249,2000597350250,2000597350251,2000597350252,2000597350253,2000597350254,2000597350255,2000597350256,2000597350257,2000597350258,2000597350259,2000597350260,2000597350261,2000597350262,2000597350263,2000597350264,2000597350265,2000597350266,2000597350267,2000597350268,2000597350269,2000597350270,2000597350271,2000597350272,2000597350273,2000597350274,2000597350275,2000597350276,2000597350277,2000597350278,2000597350279,2000597350280,2000597350281,2000597350282,2000597350283,2000597350284,2000597350285,2000597350286,2000597350287,2000597350288,2000597350289,2000597350290,2000597350291,2000597350292,2000597350293,2000597350294,2000597350295,2000597350296,2000597350297,2000597350298,2000597350299,2000597350300,2000597350301,2000597350302,2000597350303,2000597350304,2000597350305,2000597350306,2000597350307,2000597350308,2000597350309,2000597350310,2000597350311,2000597350312,2000597350313,2000597350314,2000597350315,2000597350316,2000597350317,2000597350318,2000597350319,2000597350320,2000597350321,2000597350322,2000597350323,2000597350324,2000597350325,2000597350326,2000597350327,2000597350328,2000597350329,2000597350330,2000597350331,2000597350332,2000597350333,2000597350334,2000597350335,2000597350336,2000597350337,2000597350338,2000597350339,2000597350340,2000597350341,2000597350342,2000597350343,2000597350344,2000597350345,2000597350346,2000597350347,2000597350348,2000597350349,2000597350350,2000597350351,2000597350352,2000597350353,2000597350354,2000597350355,2000597350356,2000597350357,2000597350358,2000597350359,2000597350360,2000597350361,2000597350362,2000597350363,2000597350364,2000597350365,2000597350366,2000597350367,2000597350368,2000597350369,2000597350370,2000597350371,2000597350372,2000597350373,2000597350374,2000597350375,2000597350376,2000597350377,2000597350378,2000597350379,2000597350380,2000597350381,2000597350382,2000597350383,2000597350384,2000597350385,2000597350386,2000597350387,2000597350388,2000597350389,2000597350390,2000597350391,2000597350392,2000597350393,2000597350394,2000597350395,2000597350396,2000597350397,2000597350398,2000597350399,2000597350400 };

	//To avoid crash
	mRegisteredPlayersList.push_back(0);
	mRegisteredPlayersList.push_back(0);
	mRegisteredPlayersList.push_back(0);

	vector<BlazeId> playersList;
	Platform platType = StressConfigInst.getPlatform();
	if (platType == PLATFORM_PS4)
	{
		playersList = PS4PlayersList;
	}
	else if (platType == PLATFORM_XONE)
	{
		playersList = XonePlayersList;
	}
	if (playersList.size() < 3)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::initFriendPlayerIDs]" << mPlayerInfo->getPlayerId() << " playersList size error");
		return false;
	}

	uint16_t listIndex = 0;
	while (1)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::initFriendPlayerIDs]:" << mPlayerInfo->getPlayerId() << " listIndex= " << listIndex);
		uint16_t index = (uint16_t)Blaze::Random::getRandomNumber(playersList.size());
		if (playersList[index] != mRegisteredPlayersList[0] && playersList[index] != mRegisteredPlayersList[1] && playersList[index] != mRegisteredPlayersList[2])
		{
			mRegisteredPlayersList[listIndex] = playersList[index];
			listIndex++;
		}
		if(listIndex == 3)
		{
			break;
		}
		sleep(2000);
	}
	////for UT, remove
	//mRegisteredPlayersList[0] = 1100018683054;
	//mRegisteredPlayersList[1] = 1100011029451;
	//mRegisteredPlayersList[2] = 1100061114378;

	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::initFriendPlayerIDs]:" << mPlayerInfo->getPlayerId() << " mRegisteredPlayersList[0]= " << mRegisteredPlayersList[0]);
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::initFriendPlayerIDs]:" << mPlayerInfo->getPlayerId() << " mRegisteredPlayersList[1]= " << mRegisteredPlayersList[1]);
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::initFriendPlayerIDs]:" << mPlayerInfo->getPlayerId() << " mRegisteredPlayersList[2]= " << mRegisteredPlayersList[2]);

	//Create Un-Registered Player List
	//Create new playerids from primary user id
	//mPlayerInfo->getPlayerId()-1, mPlayerInfo->getPlayerId()-2, ... etc
	//Read unregistered players list from Friends Group and increment the value
	eastl::string friendID = "";
	friendID = eastl::string().sprintf("%" PRIu64 """-1", mPlayerInfo->getPlayerId()).c_str();
	mUnRegisteredStringPlayersList.push_back(friendID.c_str());
	friendID = eastl::string().sprintf("%" PRIu64 """-2", mPlayerInfo->getPlayerId()).c_str();
	mUnRegisteredStringPlayersList.push_back(friendID.c_str());
	friendID = eastl::string().sprintf("%" PRIu64 """-3", mPlayerInfo->getPlayerId()).c_str();
	mUnRegisteredStringPlayersList.push_back(friendID.c_str());
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::initFriendPlayerIDs]:" << mPlayerInfo->getPlayerId() << " mUnRegisteredStringPlayersList[0]= " << mUnRegisteredStringPlayersList[0]);
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::initFriendPlayerIDs]:" << mPlayerInfo->getPlayerId() << " mUnRegisteredStringPlayersList[1]= " << mUnRegisteredStringPlayersList[1]);
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::initFriendPlayerIDs]:" << mPlayerInfo->getPlayerId() << " mUnRegisteredStringPlayersList[2]= " << mUnRegisteredStringPlayersList[2]);

	return true;
}

void KickOffPlayer::initDifficultyLevel()
{
	//Randomly choose game difficulty level;
	uint16_t randomValue = (uint16_t)Blaze::Random::getRandomNumber(100);
	if (randomValue < 20)
	{
		mLevel = DifficultyLevel::EASY;
	}
	else if (randomValue < 60)
	{
		mLevel = DifficultyLevel::NORMAL;
	}
	else if (randomValue < 90)
	{
		mLevel = DifficultyLevel::DIFFICULT;
	}
	else //if (randomValue < 100)
	{
		mLevel = DifficultyLevel::HARD;
	}
}

void KickOffPlayer::initPlayersType()
{
	// Randomly choose one of the players type
	uint16_t randomValue = (uint16_t)Blaze::Random::getRandomNumber(100);
	if (randomValue < 30)
	{
		mPlayersType = TEAM_ALL_REG;
	}
	else //if (randomValue < 60)
	{
		mPlayersType = TEAM_ALL_UNREG;
	}
	/*else //if (randomValue < 100)
	{
		mPlayersType = TEAM_REG_UNREG;
	}*/
}

bool KickOffPlayer::getMyAccessToken()
{
	bool result = false;
	eastl::string authCode = "";
	eastl::string accessToken = "";
	NucleusProxy * nucleusProxy = new NucleusProxy(mPlayerInfo);
	result = nucleusProxy->getPlayerAuthCode(authCode);
	if (!result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][Kickoff::getMyAccessToken]: Auth code failed");
	}
	else
	{
		result = nucleusProxy->getPlayerAccessToken(authCode, accessToken);
		if (!result)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][Kickoff::getMyAccessToken]: Access token failed");
		}
		else
		{
			mNucleusAccessToken = accessToken;
		}
	}
	delete nucleusProxy;
	return result;
}

eastl::string	KickOffPlayer::getGroupsType(EADPGroupsType type)
{
	eastl::string groupString = "";
	Platform platType = StressConfigInst.getPlatform();
	switch (type)
	{
	case EADP_GROUPS_FRIEND_TYPE:
	{
		switch (platType)
		{
		case PLATFORM_PS4:
		{
			groupString = "FIFA21FriendsPS4";
		}
		break;
		case PLATFORM_XONE:
		{
			groupString = "FIFA21FriendsXOne";
		}
		break;
		default:
		{
		}
		break;
		}
	}
	break;
	case EADP_GROUPS_OVERALL_TYPE_A:
	case EADP_GROUPS_OVERALL_TYPE_B:
	{
		switch (platType)
		{
		case PLATFORM_PS4:
		{
			groupString = "FIFA21OverallPS4";
		}
		break;
		case PLATFORM_XONE:
		{
			groupString = "FIFA21OverallXOne";
		}
		break;
		default:
		{
		}
		break;
		}
	}
	break;
	case EADP_GROUPS_RIVAL_TYPE:
	{
		switch (platType)
		{
		case PLATFORM_PS4:
		{
			groupString = "FIFA21RivalPS4";
		}
		break;
		case PLATFORM_XONE:
		{
			groupString = "FIFA21RivalXOne";
		}
		break;
		default:
		{
		}
		break;
		}
	}
	break;
	default:
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::getGroupsType]" << mPlayerInfo->getPlayerId() << " In default state");
	}
	break;
	}
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::getGroupsType]" << mPlayerInfo->getPlayerId() << " with type " << type << " response= " << groupString);
	return groupString;
}

BlazeRpcError KickOffPlayer::submitOfflineGameReport()
{
	Blaze::BlazeRpcError err = ERR_OK;
	uint16_t homesubNum = (uint16_t)Blaze::Random::getRandomNumber(10);
	Blaze::GameReporting::OSDKGameReportBase::OSDKPlayerReport* playerReport = NULL;
	Blaze::GameReporting::Fifa::SoloPlayerReport *soloPlayerReport = NULL;
	Blaze::GameReporting::Fifa::SoloGameReport* soloGameReport = NULL;
	Blaze::GameReporting::Fifa::Substitution *substitution = NULL;
	Blaze::GameReporting::OSDKGameReportBase::OSDKReport *osdkReport = NULL;

	GameReporting::SubmitGameReportRequest request;

	// set status of FNSH
	request.setFinishedStatus(Blaze::GameReporting::GAMEREPORT_FINISHED_STATUS_DEFAULT);
	// get gamereport object which needs to be filled with game data
	GameReporting::GameReport &gamereport = request.getGameReport();
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
	// Game report contains osdk report, adding osdkreport to game report
	gamereport.setReport(*osdkReport);
	// fill the osdkgamereport fields
	Blaze::GameReporting::OSDKGameReportBase::OSDKGameReport& osdkGamereport = osdkReport->getGameReport();
	// GTIM
	osdkGamereport.setGameTime(52225);

	gamereport.setGameReportName(mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str());
	osdkGamereport.setGameType(mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str());

	osdkGamereport.setFinishedStatus(0);
	osdkGamereport.setArenaChallengeId(0);
	osdkGamereport.setCategoryId(0);
	osdkGamereport.setGameReportId(0);
	osdkGamereport.setSimulate(false);
	osdkGamereport.setLeagueId(0);
	osdkGamereport.setRank(false);
	osdkGamereport.setRoomId(0);
	// osdkGamereport.setSponsoredEventId(0);
	osdkGamereport.setFinishedStatus(1);
	// For now updating values as per client logs.TODO: need to add values according to server side settings
	soloGameReport->setOpponentTeamId(111140);
	soloGameReport->setMatchDifficulty(4);
	soloGameReport->setProfileDifficulty(4);
	// fill the common game report value
	Blaze::GameReporting::Fifa::CommonGameReport& commonGameReport = soloGameReport->getCommonGameReport();
	// WNPK
	commonGameReport.setWentToPk(0);
	commonGameReport.setIsRivalMatch(false);
	// For now updating values as per client logs.TODO: need to add values according to server side settings
	commonGameReport.setBallId(0);
	commonGameReport.setAwayKitId(4550);
	commonGameReport.setHomeKitId(1827);
	commonGameReport.setStadiumId(32);
	const int MAX_TEAM_COUNT = 11;
	const int MAX_PLAYER_VALUE = 100000;
	substitution->setPlayerSentOff(Blaze::Random::getRandomNumber(250000));
	substitution->setPlayerSubbedIn(Blaze::Random::getRandomNumber(250000));
	substitution->setSubTime(56);
	if (homesubNum > 2)
		commonGameReport.getAwaySubList().push_back(substitution);
	// fill awayStartingXI and homeStartingXI values here
	for (int iteration = 0; iteration < MAX_TEAM_COUNT; iteration++) {
		commonGameReport.getAwayStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
	}
	for (int iteration = 0; iteration < MAX_TEAM_COUNT; iteration++) {
		commonGameReport.getHomeStartingXI().push_back(Random::getRandomNumber(MAX_PLAYER_VALUE));
	}
	osdkGamereport.setCustomGameReport(*soloGameReport);
	// populate player data in playermap
	// OsdkPlayerReportsMap = BLAZE_NEW Blaze::GameReporting::OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap; // = osdkReport->getPlayerReports();
	Blaze::GameReporting::OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = osdkReport->getPlayerReports();
	OsdkPlayerReportsMap.reserve(1);
	GameManager::PlayerId playerId = mPlayerInfo->getPlayerId();
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
	commonPlayerReport.setGoalsConceded(rand() % 10);
	commonPlayerReport.setCorners(rand() % 15);
	commonPlayerReport.setPassAttempts(rand() % 15);
	commonPlayerReport.setPassesMade(rand() % 12);
	commonPlayerReport.setRating((float)(rand() % 10));
	commonPlayerReport.setSaves(rand() % 10);
	commonPlayerReport.setShots(rand() % 12);
	commonPlayerReport.setTackleAttempts(rand() % 12);
	commonPlayerReport.setTacklesMade(rand() % 15);
	commonPlayerReport.setFouls(rand() % 12);
	commonPlayerReport.setGoals(rand() % 15);
	commonPlayerReport.setInterceptions(rand() % 10);  // Added newly
	commonPlayerReport.setOffsides(rand() % 15);
	commonPlayerReport.setOwnGoals(rand() % 10);
	commonPlayerReport.setPkGoals(rand() % 10);
	commonPlayerReport.setPossession(rand() % 15);
	commonPlayerReport.setRedCard(rand() % 12);
	commonPlayerReport.setShotsOnGoal(rand() % 15);
	commonPlayerReport.setUnadjustedScore(rand() % 10);  // Added newly
	commonPlayerReport.setYellowCard(rand() % 12);
	commonPlayerReport.getCommondetailreport().setAveragePossessionLength((uint16_t)Random::getRandomNumber(50));
	commonPlayerReport.getCommondetailreport().setInjuriesRed(0);
	commonPlayerReport.getCommondetailreport().setPassesIntercepted(rand() % 15);
	commonPlayerReport.getCommondetailreport().setPenaltiesAwarded(rand() % 10);
	commonPlayerReport.getCommondetailreport().setPenaltiesMissed(rand() % 12);
	commonPlayerReport.getCommondetailreport().setPenaltiesScored(rand() % 10);
	commonPlayerReport.getCommondetailreport().setPenaltiesSaved(rand() % 12);

	soloCustomPlayerData.setGoalAgainst(rand() % 10);
	soloCustomPlayerData.setLosses(0);  // Added newly
	soloCustomPlayerData.setResult(rand() % 10);  // Added newly
	soloCustomPlayerData.setShotsAgainst(rand() % 15);
	soloCustomPlayerData.setShotsAgainstOnGoal(rand() % 10);
	soloCustomPlayerData.setTeam(rand() % 10);
	soloCustomPlayerData.setTies(1);  // Added newly
	soloCustomPlayerData.setWins(0);  // Added newly

	seasonalPlayData.setCurrentDivision(0);
	seasonalPlayData.setCup_id(0);
	seasonalPlayData.setUpdateDivision(Blaze::GameReporting::Fifa::CompetitionStatus::NO_UPDATE);
	seasonalPlayData.setGameNumber(0);
	seasonalPlayData.setSeasonId(0);
	seasonalPlayData.setWonCompetition(0);
	seasonalPlayData.setWonLeagueTitle(false);

	playerReport->setCustomDnf(0);
	playerReport->setScore(rand() % 15);
	playerReport->setAccountCountry(0);
	playerReport->setFinishReason(0);
	playerReport->setGameResult(0);
	playerReport->setHome(false);
	playerReport->setLosses(0);
	playerReport->setName(mPlayerInfo->getPersonaName().c_str());
	playerReport->setOpponentCount(0);
	playerReport->setExternalId(0);
	playerReport->setNucleusId(mPlayerInfo->getLogin()->getUserLoginInfo()->getBlazeId());
	playerReport->setPersona(mPlayerInfo->getPersonaName().c_str());
	playerReport->setPointsAgainst(0);
	playerReport->setUserResult(0);
	playerReport->setClientScore(0);
	// playerReport->setSponsoredEventAccountRegion(0);
	playerReport->setSkill(0);
	playerReport->setSkillPoints(0);
	playerReport->setTeam(0);
	playerReport->setTies(0);
	playerReport->setWinnerByDnf(0);
	playerReport->setWins(0);
	playerReport->setCustomPlayerReport(*soloPlayerReport);

	if (!strcmp("gameType90", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
	{
		//in gameType90
		GameReporting::OSDKGameReportBase::IntegerAttributeMap& privateAttrMap = playerReport->getPrivatePlayerReport().getPrivateIntAttributeMap();
		privateAttrMap["aiAst"] = (int64_t)0;
		privateAttrMap["aiCS"] = (int64_t)1;
		privateAttrMap["aiCo"] = (int64_t)0;
		privateAttrMap["aiFou"] = (int64_t)3;
		privateAttrMap["aiGC"] = (int64_t)0;
		privateAttrMap["aiGo"] = (int64_t)0;
		privateAttrMap["aiInt"] = (int64_t)0;
		privateAttrMap["aiMOTM"] = (int64_t)0;
		privateAttrMap["aiOG"] = (int64_t)0;
		privateAttrMap["aiOff"] = (int64_t)0;
		privateAttrMap["aiPA"] = (int64_t)12;
		privateAttrMap["aiPKG"] = (int64_t)0;
		privateAttrMap["aiPM"] = (int64_t)4;
		privateAttrMap["aiPo"] = (int64_t)8;
		privateAttrMap["aiRC"] = (int64_t)0;
		privateAttrMap["aiRa"] = (int64_t)0;
		privateAttrMap["aiSav"] = (int64_t)1;
		privateAttrMap["aiSoG"] = (int64_t)0;
		privateAttrMap["aiTA"] = (int64_t)10;
		privateAttrMap["aiTM"] = (int64_t)7;
		privateAttrMap["aiYC"] = (int64_t)0;

		privateAttrMap["gid"] = (int64_t)1488800000 + (int64_t)Random::getRandomNumber(100000);
	}
	else 
	{
		GameReporting::OSDKGameReportBase::IntegerAttributeMap& privateAttrMap = playerReport->getPrivatePlayerReport().getPrivateIntAttributeMap();
		privateAttrMap["aiAst"] = (int64_t)0;
		privateAttrMap["aiCS"] = (int64_t)1;
		privateAttrMap["aiCo"] = (int64_t)0;
		privateAttrMap["aiFou"] = (int64_t)0;
		privateAttrMap["aiGC"] = (int64_t)0;
		privateAttrMap["aiGo"] = (int64_t)0;
		privateAttrMap["aiInt"] = (int64_t)5;
		privateAttrMap["aiMOTM"] = (int64_t)0;
		privateAttrMap["aiOG"] = (int64_t)0;
		privateAttrMap["aiOff"] = (int64_t)2;
		privateAttrMap["aiPA"] = (int64_t)44;
		privateAttrMap["aiPKG"] = (int64_t)0;
		privateAttrMap["aiPM"] = (int64_t)35;
		privateAttrMap["aiPo"] = (int64_t)23;
		privateAttrMap["aiRC"] = (int64_t)0;
		privateAttrMap["aiRa"] = (int64_t)0;
		privateAttrMap["aiSav"] = (int64_t)0;
		privateAttrMap["aiSoG"] = (int64_t)1;
		privateAttrMap["aiTA"] = (int64_t)7;
		privateAttrMap["aiTM"] = (int64_t)4;
		privateAttrMap["aiYC"] = (int64_t)0;
		privateAttrMap["gid"] = (int64_t)1488800000 + (int64_t)Random::getRandomNumber(100000);
	}


    OsdkPlayerReportsMap[playerId] = playerReport;
    //char8_t buf[10240];
    //BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "-> [submitofflinegamereport][" << mPlayerInfo->getPlayerId() << "]::submitofflinegamereport" << "\n" << request.print(buf, sizeof(buf)));

    // submit offline game report
	err = mPlayerInfo->getComponentProxy()->mGameReportingProxy->submitOfflineGameReport(request);
    if (err != ERR_OK) {
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "submitofflinegamereport failed. Error(" << ErrorHelp::getErrorName(err) << ")");
        return err;
    }
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[submitofflinegamereport]submitted successfully " <<  mPlayerInfo->getPlayerId());
    return err;
}

bool KickOffPlayer::viewStats()
{
	bool result = true;
	eastl::string viewName = "";
	eastl::string entityIDName = "";

	//[STATS] Get the stats for rival group(7f134ad7 - e92e - 4594 - a077 - 54177e30a7b8)
	//https ://stats.int.gameservices.ea.com:11000/api/1.0/contexts/FIFA21/views/RivalStatsView/entities/stats?entity_ids=7f134ad7%2De92e%2D4594%2Da077%2D54177e30a7b8 
	//[STATS] Get the stats for overall group A and overall group B(d15e3b34 - 87b1 - 4877 - ad07 - def6307b5afe and 8894262f - 09e2 - 4c19 - b4ea - dd8048b48c56)
	//https ://stats.int.gameservices.ea.com:11000/api/1.0/contexts/FIFA21/views/OverallStatsView/entities/stats?entity_ids=d15e3b34%2D87b1%2D4877%2Dad07%2Ddef6307b5afe%2C8894262f%2D09e2%2D4c19%2Db4ea%2Ddd8048b48c56
	//[STATS] Get the controller stats for all users in the match(1000240594789 and 1000031072799 - 58)
	//https ://stats.int.gameservices.ea.com:11000/api/1.0/contexts/FIFA21/views/ControllerStatsView/entities/stats?entity_ids=1000031072799%2D58%2C1000240594789

	//[STATS] Get user info of all the registered users
	//https ://stats.int.gameservices.ea.com:11000/api/1.0/contexts/FIFA21/views/UserCustomStatsView/entities/stats?entity_ids=1000031072799%2D54%2C1000031072799%2D53%2C1000031072799%2D52%2C1000031072799%2D51%2C1000031072799%2D50%2C1000031072799%2D49%2C1000031072799%2D48%2C1000031072799%2D47%2C1000031072799%2D46%2C1000031072799%2D45%2C1000031072799%2D44%2C1000031072799%2D43%2C1000031072799%2D42%2C1000031072799%2D41%2C1000031072799%2D40%2C1000031072799%2D39%2C1000031072799%2D38%2C1000031072799%2D37%2C1000031072799%2D36%2C1000031072799%2D35%2C1000031072799%2D34%2C1000031072799%2D33%2C1000031072799%2D32%2C1000031072799%2D31%2C1000031072799%2D29%2C1000031072799%2D28%2C1000031072799%2D27%2C1000031072799%2D26%2C1000031072799%2D25%2C1000031072799%2D23%2C1000031072799%2D21%2C1000031072799%2D20%2C1000031072799%2D19%2C1000031072799%2D17%2C1000031072799%2D16%2C1000031072799%2D15%2C1000031072799%2D9%2C1000031072799%2D8%2C1000031072...

	//RivalStatsView
	viewName		= "RivalStatsView";
	entityIDName	= mRivalGroup_ID;
	result = getStatsProxyHandler()->viewEntityStats(viewName, entityIDName, mNucleusAccessToken);
	if (!result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][KickOffPlayer::viewStats::RivalStatsView]: Failed to get stats");
	}

	sleepRandomTime(5000, 10000);

	//OverallStatsView
	//overall group A and overall group B
	viewName = "OverallStatsView";
	entityIDName.clear();
	if(mScenarioType != TEAM_1_CPU && TEAM_2_CPU)
	{
		entityIDName = mOverallGroupA_ID + "," + mOverallGroupB_ID;
	}
	else
	{
		entityIDName = mOverallGroupA_ID;
	}
	
	result = getStatsProxyHandler()->viewEntityStats(viewName, entityIDName, mNucleusAccessToken);
	if (!result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][KickOffPlayer::viewStats::OverallStatsView]: Failed to get stats");
	}

	//ControllerStatsView
	//all users in the match both registered and unregistered
	viewName = "ControllerStatsView";
	entityIDName.clear();
	entityIDName = eastl::string().sprintf(",""%" PRIu64 "", mPlayerInfo->getPlayerId()).c_str();
	switch (mScenarioType)
	{
		case TEAM_1_CPU:
		{
		}
		break;
		case TEAM_1_1:
		case TEAM_2_CPU:
		{
			if (mPlayersType == TEAM_ALL_REG)
			{
				entityIDName += eastl::string().sprintf(",""%" PRIu64 "", mRegisteredPlayersList[0]).c_str();
			}
			else if (mPlayersType == TEAM_ALL_UNREG)
			{
				entityIDName += "," + mUnRegisteredStringPlayersList[0];
			}
		}
		break;
		case TEAM_1_2:
		{
			if (mPlayersType == TEAM_ALL_REG)
			{
				entityIDName += eastl::string().sprintf(",""%" PRIu64 "", mRegisteredPlayersList[0]).c_str();
				entityIDName += eastl::string().sprintf(",""%" PRIu64 "", mRegisteredPlayersList[1]).c_str();
			}
			else if (mPlayersType == TEAM_ALL_UNREG)
			{
				entityIDName += "," + mUnRegisteredStringPlayersList[0];
				entityIDName += "," + mUnRegisteredStringPlayersList[1];
			}
		}
		break;
		case TEAM_2_2:
		{
			if (mPlayersType == TEAM_ALL_REG)
			{
				entityIDName += eastl::string().sprintf(",""%" PRIu64 "", mRegisteredPlayersList[0]).c_str();
				entityIDName += eastl::string().sprintf(",""%" PRIu64 "", mRegisteredPlayersList[1]).c_str();
				entityIDName += eastl::string().sprintf(",""%" PRIu64 "", mRegisteredPlayersList[2]).c_str();
			}
			else if (mPlayersType == TEAM_ALL_UNREG)
			{
				entityIDName += "," + mUnRegisteredStringPlayersList[0];
				entityIDName += "," + mUnRegisteredStringPlayersList[1];
				entityIDName += "," + mUnRegisteredStringPlayersList[2];
			}
		}
		break;
		default:
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::viewStats]" << mPlayerInfo->getPlayerId() << " In default state");
			return false;
		}
		break;
	}
	result = getStatsProxyHandler()->viewEntityStats(viewName, entityIDName, mNucleusAccessToken);
	if (!result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][KickOffPlayer::viewStats::ControllerStatsView]: Failed to get stats");
	}

	return result;
}

bool KickOffPlayer::readGroupIDs() 
{
	bool result = true;
	bool output = true;

	eastl::string groupTypeName = "";
	eastl::string groupName = "";

	//Get Instance list for FIFA21FriendsPS4 group name
	groupTypeName	= getGroupsType(EADP_GROUPS_FRIEND_TYPE);
	groupName		= mFriendsGroup_Name;
	result = getGroupsProxyHandler()->getGroupIDInfo(groupName, groupTypeName, mFriendsGroup_ID, mNucleusAccessToken);
	if (!result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][KickOffPlayer::readGroupIDs]: Failed to get Group Name");
		output = false;
	}
	else
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::readGroupIDs]" << mPlayerInfo->getPlayerId() << " mFriendsGroup_ID= " << mFriendsGroup_ID);
	}

	//Get Instance list for FIFA21Overall group name for Team A
	groupTypeName	= getGroupsType(EADP_GROUPS_OVERALL_TYPE_A);
	groupName		= mOverallGroupA_Name;
	result = getGroupsProxyHandler()->getGroupIDInfo(groupName, groupTypeName, mOverallGroupA_ID, mNucleusAccessToken);
	if (!result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][KickOffPlayer::readGroupIDs::getGroupIDInfo]: Failed to get Group Name for "<< groupName);
		output = false;
	}
	else
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::readGroupIDs]" << mPlayerInfo->getPlayerId() << " mOverallGroupA_ID= " << mOverallGroupA_ID);
	}

	// CPU don't send the request
	if (mScenarioType != TEAM_1_CPU && mScenarioType != TEAM_2_CPU)
	{
		//Get Instance list for FIFA21Overall group name for Team B
		groupTypeName = getGroupsType(EADP_GROUPS_OVERALL_TYPE_B);
		groupName = mOverallGroupB_Name;
		result = getGroupsProxyHandler()->getGroupIDInfo(groupName, groupTypeName, mOverallGroupB_ID, mNucleusAccessToken);
		if (!result)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][KickOffPlayer::readGroupIDs: Failed to get Group Name");

			output = false;
		}
		else
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::readGroupIDs]" << mPlayerInfo->getPlayerId() << " mOverallGroupB_ID= " << mOverallGroupB_ID);
		}
	}

	//Get Instance list for Rival Group - FIFA21RivalPS4 
	groupTypeName	= getGroupsType(EADP_GROUPS_RIVAL_TYPE);
	groupName		= mRivalGroup_Name;
	result = getGroupsProxyHandler()->getGroupIDInfo(groupName, groupTypeName, mRivalGroup_ID, mNucleusAccessToken);
	if (!result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][Kickoff::readGroupIDs::getGroupIDInfo]: Failed to get Group Name");
		output = false;
	}
	else
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::readGroupIDs]" << mPlayerInfo->getPlayerId() << " mRivalGroup_ID= " << mRivalGroup_ID);
	}

	return output;
}

bool KickOffPlayer::searchGroupMembers()
{
	bool result = true;

	//[GROUPS] Get Member List for friends group(94a6d10f - ccb4 - 4fd8 - ae4d - dcff1370bf30)
	//https: //groups.integration.gameservices.ea.com:443/group/instance/94a6d10f-ccb4-4fd8-ae4d-dcff1370bf30/members?pagesize=50

	//[GROUPS] search the overall group B members if group exists(8894262f - 09e2 - 4c19 - b4ea - dd8048b48c56)
	//https: //groups.integration.gameservices.ea.com:443/group/instance/8894262f-09e2-4c19-b4ea-dd8048b48c56/members?pagesize=50

	//[GROUPS] Get all rival groups for lead user(1000031072799) based on access time
	//https ://groups.integration.gameservices.ea.com:443/group/instance/search?name=1000031072799&pagesize=50&sortBy=access%5Ftimestamp&sortDir=desc&typeId=FIFA21RivalPS4

	//Get Member List for Friends group ID
	StringList membersList1;
	result = getGroupsProxyHandler()->getGroupIDList(mFriendsGroup_ID, membersList1, mNucleusAccessToken);
	if (!result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][KickOffPlayer::searchGroupMembers::getGroupIDList]: Failed to get friends list");
		result = false;
	}
	else
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::searchGroupMembers]" << mPlayerInfo->getPlayerId() << " Friends Member List successful ");
	}

	sleepRandomTime(5000, 10000);

	//Get Member List for overall group A members
	StringList membersList2;
	result = getGroupsProxyHandler()->getGroupIDList(mOverallGroupA_ID, membersList2, mNucleusAccessToken);
	if (!result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][KickOffPlayer::searchGroupMembers::getGroupIDList]: Failed to get Group A list");
		result = false;
	}
	else
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::searchGroupMembers]" << mPlayerInfo->getPlayerId() << " Group-A Member List successful ");
	}

	sleepRandomTime(5000, 10000);

	// CPU don't send the request
	if (mScenarioType != TEAM_1_CPU && mScenarioType != TEAM_2_CPU)
	{
		//Get Member List for overall group B members
		StringList membersList3;
		result = getGroupsProxyHandler()->getGroupIDList(mOverallGroupB_ID, membersList3, mNucleusAccessToken);
		if (!result)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][KickOffPlayer::searchGroupMembers::getGroupIDList]: Failed to get Group B list");
			result = false;
		}
		else
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::searchGroupMembers]" << mPlayerInfo->getPlayerId() << " Group-B Member List successful ");
		}
	}

	//Get Rival group list for primary user
	eastl::string groupTypeName = getGroupsType(EADP_GROUPS_RIVAL_TYPE);
	StringList membersList4;	
	result = getGroupsProxyHandler()->getGroupIDRivalList(mFriendsGroup_Name, groupTypeName, membersList4, mNucleusAccessToken);
	if (!result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][KickOffPlayer::searchGroupMembers::getGroupIDRivalList]: Failed to get Rival group list");
		result = false;
	}
	else
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::searchGroupMembers]" << mPlayerInfo->getPlayerId() << "Rival groupList successful ");
	}
	return result;
}


bool KickOffPlayer::createGroupNames()
{
	bool result = true;
	//Create groupNames for EADP_GROUPS_FRIEND_TYPE, EADP_GROUPS_OVERALL_TYPE, EADP_GROUPS_RIVAL_TYPE
	mFriendsGroup_Name = eastl::string().sprintf("%" PRIu64 "", mPlayerInfo->getPlayerId()).c_str();

	if (mRegisteredPlayersList.size() < 3)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][KickOffPlayer::createGroupNames]: mRegisteredPlayersList size is not valid");
		return false;
	}

	if (mUnRegisteredStringPlayersList.size() < 3)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerInfo->getPlayerId() << "][KickOffPlayer::createGroupNames]: mUnRegisteredStringPlayersList size is not valid");
		return false;
	}

	//PlayerID highest becomes GroupA
	// 2 > 1 -> 2__1__L - 1000182113008__1000180361244__0
	// player vs player then level should be 0 (2vs2)
	// player vs CPU then level should be changed as per the random 1-5
	// 2vs2 -> TEAMA__TEAMB__L -> USERA_USERB__USERC_USERD__DIFFICULTY -> GroupAName__GroupBName_L
	// 1vsCPU -> No overall Group Name of CPU -> USERA____0 -> 1000182113008____0 
	//mRivalGroup_Name = eastl::string().sprintf("%" PRIu64 """__""%" PRIu64 """__""%" PRIu64 "", mPlayerInfo->getPlayerId(), mRegisteredPlayersList.front(), (int64_t)getDifficultyLevel()).c_str();
	
	switch (mScenarioType)
	{
		case TEAM_1_1:
		{
			mOverallGroupA_Name = eastl::string().sprintf("%" PRIu64 "", mPlayerInfo->getPlayerId()).c_str();
			switch (mPlayersType)
			{
				case TEAM_ALL_REG:
				{				
					mOverallGroupB_Name = eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[0]).c_str();
					mRivalGroup_Name = eastl::string().sprintf("%" PRIu64 """__""%" PRIu64 """__0", mPlayerInfo->getPlayerId(), mRegisteredPlayersList[0]).c_str();
				}
				break;
				case TEAM_ALL_UNREG:
				{
					mOverallGroupB_Name = mUnRegisteredStringPlayersList[0].c_str();
					mRivalGroup_Name = eastl::string().sprintf("%" PRIu64 """__""%s__0", mPlayerInfo->getPlayerId(), mUnRegisteredStringPlayersList[0].c_str()).c_str();
				}
				break;
				default:
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createGroupNames]" << mPlayerInfo->getPlayerId() << " In default state");
					return false;
				}
				break;
			}			
		}
		break;
		case TEAM_1_2:
		{
			mOverallGroupA_Name = eastl::string().sprintf("%" PRIu64 "", mPlayerInfo->getPlayerId()).c_str();
			switch (mPlayersType)
			{
				case TEAM_ALL_REG:
				{
					mOverallGroupB_Name = eastl::string().sprintf("%" PRIu64 """__""%" PRIu64 "", mRegisteredPlayersList[0], mRegisteredPlayersList[1]).c_str();
					mRivalGroup_Name = eastl::string().sprintf("%" PRIu64 """__""%" PRIu64 """__""%" PRIu64 """__0", mPlayerInfo->getPlayerId(), mRegisteredPlayersList[0], mRegisteredPlayersList[1]).c_str();
				}
				break;
				case TEAM_ALL_UNREG:
				{
					mOverallGroupB_Name = eastl::string().sprintf("%s__%s", mUnRegisteredStringPlayersList[0].c_str(), mUnRegisteredStringPlayersList[1].c_str()).c_str();
					mRivalGroup_Name = eastl::string().sprintf("%" PRIu64 """__%s__%s__0", mPlayerInfo->getPlayerId(), mUnRegisteredStringPlayersList[0].c_str(), mUnRegisteredStringPlayersList[1].c_str()).c_str();
				}
				break;
				default:
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createGroupNames]" << mPlayerInfo->getPlayerId() << " In default state");
					return false;
				}
				break;
			}
		}
		break;
		case TEAM_2_2:
		{	
			switch (mPlayersType)
			{
				case TEAM_ALL_REG:
				{
					mOverallGroupA_Name = eastl::string().sprintf("%" PRIu64 """__""%" PRIu64 "", mPlayerInfo->getPlayerId(), mRegisteredPlayersList[0]).c_str();
					mOverallGroupB_Name = eastl::string().sprintf("%" PRIu64 """__""%" PRIu64 "", mRegisteredPlayersList[1], mRegisteredPlayersList[2]).c_str();
					mRivalGroup_Name = eastl::string().sprintf("%" PRIu64 """__""%" PRIu64 """__""%" PRIu64 """__""%" PRIu64 "__0", mPlayerInfo->getPlayerId(), mRegisteredPlayersList[0], mRegisteredPlayersList[1], mRegisteredPlayersList[2]).c_str();
				}
				break;
				case TEAM_ALL_UNREG:
				{
					mOverallGroupA_Name = eastl::string().sprintf("%" PRIu64 """__%s", mPlayerInfo->getPlayerId(), mUnRegisteredStringPlayersList[0].c_str()).c_str();
					mOverallGroupB_Name = eastl::string().sprintf("%s__%s", mUnRegisteredStringPlayersList[1].c_str(), mUnRegisteredStringPlayersList[2].c_str()).c_str();
					mRivalGroup_Name = eastl::string().sprintf("%" PRIu64 """__%s__%s__%s__0", mPlayerInfo->getPlayerId(), mUnRegisteredStringPlayersList[0].c_str(), mUnRegisteredStringPlayersList[1].c_str(), mUnRegisteredStringPlayersList[2].c_str()).c_str();
				}
				break;
				default:
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createGroupNames]" << mPlayerInfo->getPlayerId() << " In default state");
					return false;
				}
				break;
			}

		}
		break;
		case TEAM_1_CPU:
		{
			mOverallGroupA_Name = eastl::string().sprintf("%" PRIu64 "", mPlayerInfo->getPlayerId()).c_str();		
			mOverallGroupB_Name = "";
			mRivalGroup_Name = eastl::string().sprintf("%" PRIu64 """__""%" PRIu64 "", mPlayerInfo->getPlayerId(), (int64_t)getDifficultyLevel()).c_str();			
		}
		break;
		case TEAM_2_CPU:
		{
			mOverallGroupB_Name = "";
			switch (mPlayersType)
			{
				case TEAM_ALL_REG:
				{
					mOverallGroupA_Name = eastl::string().sprintf("%" PRIu64 """__""%" PRIu64 "", mPlayerInfo->getPlayerId(), mRegisteredPlayersList[0]).c_str();
					mRivalGroup_Name = eastl::string().sprintf("%" PRIu64 """__""%" PRIu64 """__""%" PRIu64 "", mPlayerInfo->getPlayerId(), mRegisteredPlayersList[0], (int64_t)getDifficultyLevel()).c_str();
				}
				break;
				case TEAM_ALL_UNREG:
				{
					mOverallGroupA_Name = eastl::string().sprintf("%" PRIu64 """__%s", mPlayerInfo->getPlayerId(), mUnRegisteredStringPlayersList[0].c_str()).c_str();
					mRivalGroup_Name = eastl::string().sprintf("%" PRIu64 """__%s__""%" PRIu64 "", mPlayerInfo->getPlayerId(), mUnRegisteredStringPlayersList[0].c_str(), (int64_t)getDifficultyLevel()).c_str();
				}
				break;
				default:
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createGroupNames]" << mPlayerInfo->getPlayerId() << " In default state");
					return false;
				}
				break;
			}
		}
		break;
		default:
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createGroupNames]" << mPlayerInfo->getPlayerId() << " In default state");
			return false;
		}
		break;
	}
	return result;
}

BlazeRpcError KickOffPlayer::createInstance(EADPGroupsType type)
{
	BlazeRpcError err = ERR_OK;
	bool result = false;
	FifaGroups::CreateInstanceRequest request;
	Blaze::FifaGroups::CreateInstanceResponse response;
	char8_t creatorUserId[256];
	memset(creatorUserId, '\0', sizeof(creatorUserId));
	blaze_snzprintf(creatorUserId, sizeof(creatorUserId), "%" PRId64, mPlayerInfo->getPlayerId());
	
	request.setCreatorUserId(mPlayerInfo->getPlayerId());

	eastl::string groupTypeName = getGroupsType(type);
	eastl::string timeStamp = eastl::string().sprintf("%" PRIu64 "", TimeValue::getTimeOfDay().getSec()).c_str();

	Blaze::FifaGroups::Attribute mAttributes1;
	Blaze::FifaGroups::Attribute mAttributes2;
	Blaze::FifaGroups::Attribute mAttributes3;
	Blaze::FifaGroups::Attribute mAttributes4;
	Blaze::FifaGroups::Attribute mAttributes5;
	Blaze::FifaGroups::Attribute mAttributes6;
	Blaze::FifaGroups::Attribute mAttributes7;
	Blaze::FifaGroups::Attribute mAttributes8;
	Blaze::FifaGroups::Attribute mAttributes9;
	eastl::string groupTypeId = "";
	eastl::string groupName = "";
	eastl::string groupShortName = "";
	switch (type)
	{
		case EADP_GROUPS_FRIEND_TYPE:
		{
			groupName = mFriendsGroup_Name.c_str();
			groupShortName = mFriendsGroup_Name.c_str();
			groupTypeId = groupTypeName.c_str();
			
			request.setGroupName(mFriendsGroup_Name.c_str());
			request.setGroupShortName(mFriendsGroup_Name.c_str());
			request.setGroupType(groupTypeName.c_str());
		}
		break;
		case EADP_GROUPS_OVERALL_TYPE_A:
		{
			groupName = mOverallGroupA_Name.c_str();
			groupShortName = mOverallGroupA_Name.c_str();
			groupTypeId = groupTypeName.c_str();

			request.setGroupName(mOverallGroupA_Name.c_str());
			request.setGroupShortName(mOverallGroupA_Name.c_str());
			request.setGroupType(groupTypeName.c_str());

			mAttributes1.setKey("lead_persona");
			mAttributes1.setIValue(0);
			mAttributes1.setSValue(eastl::string().sprintf("%" PRIu64 "", mPlayerInfo->getPlayerId()).c_str());
			mAttributes1.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
			request.getAttributes().push_back(&mAttributes1);

			mAttributes2.setKey("team");
			mAttributes2.setIValue(0);
			//Add all users in the teamA
			eastl::string teamAList = eastl::string().sprintf("%" PRIu64 "", mPlayerInfo->getPlayerId()).c_str();
			switch (mScenarioType)
			{
			case TEAM_1_CPU:
			case TEAM_1_1:
			case TEAM_1_2:
			{}
			break;
			case TEAM_2_CPU:
			case TEAM_2_2:
			{
				teamAList.append(",");
				if (mPlayersType == TEAM_ALL_REG)
				{
					teamAList.append(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[0]).c_str());
				}
				else if (mPlayersType == TEAM_ALL_UNREG)
				{
					teamAList.append(mUnRegisteredStringPlayersList[0]);
				}
			}
			break;
			default:
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createInstance]" << mPlayerInfo->getPlayerId() << " In default state");
				return ERR_SYSTEM;
			}
			break;
			}
			//Add members list
			mAttributes2.setSValue(teamAList.c_str());
			mAttributes2.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
			request.getAttributes().push_back(&mAttributes2);

			//Add members list (UnRegistered Players)
			if (mPlayersType == TEAM_ALL_UNREG)
			{
				switch (mScenarioType)
				{
				case TEAM_1_CPU:
				{}
				break;
				case TEAM_2_CPU:
				case TEAM_1_1:
				{
					mAttributes7.setKey("unregistered");
					mAttributes7.setIValue(0);
					//add unregistered player
					mAttributes7.setSValue(mUnRegisteredStringPlayersList[0].c_str());
					mAttributes7.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes7);
				}
				break;
				case TEAM_1_2:
				{
					mAttributes7.setKey("unregistered");
					mAttributes7.setIValue(0);
					//add unregistered player
					mAttributes7.setSValue(mUnRegisteredStringPlayersList[0].c_str());
					mAttributes7.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes7);

					mAttributes8.setKey("unreg0");
					mAttributes8.setIValue(0);
					//add unregistered player
					mAttributes8.setSValue(mUnRegisteredStringPlayersList[1].c_str());
					mAttributes8.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes8);
				}
				break;
				case TEAM_2_2:
				{
					mAttributes7.setKey("unregistered");
					mAttributes7.setIValue(0);
					//add unregistered player
					mAttributes7.setSValue(mUnRegisteredStringPlayersList[0].c_str());
					mAttributes7.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes7);

					mAttributes8.setKey("unreg0");
					mAttributes8.setIValue(0);
					//add unregistered player
					mAttributes8.setSValue(mUnRegisteredStringPlayersList[1].c_str());
					mAttributes8.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes8);

					mAttributes9.setKey("unreg1");
					mAttributes9.setIValue(0);
					//add unregistered player
					mAttributes9.setSValue(mUnRegisteredStringPlayersList[2].c_str());
					mAttributes9.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes9);
				}
				break;
				default:
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createInstance]" << mPlayerInfo->getPlayerId() << " In default state");
					return ERR_SYSTEM;
				}
				break;
				}
			}
			
			mAttributes3.setKey("access_timestamp");
			mAttributes3.setIValue(0);
			mAttributes3.setSValue(timeStamp.c_str());
			mAttributes3.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
			request.getAttributes().push_back(&mAttributes3);

			//Add members list
			if (mPlayersType == TEAM_ALL_REG)
			{
				switch (mScenarioType)
				{
					case TEAM_1_1:
					case TEAM_1_2:
					case TEAM_1_CPU:
					{}
					break;
					case TEAM_2_2:
					case TEAM_2_CPU:
					{
						request.getMembers().push_back(mRegisteredPlayersList[0]);
					}
					break;
					default:
					{
						BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createInstance]" << mPlayerInfo->getPlayerId() << " In default state");
						return ERR_SYSTEM;
					}
					break;
				}
			}
		}
		break;
		case EADP_GROUPS_OVERALL_TYPE_B:
		{
			groupName = mOverallGroupB_Name.c_str();
			groupShortName = mOverallGroupB_Name.c_str();
			groupTypeId = groupTypeName.c_str();

			request.setGroupName(mOverallGroupB_Name.c_str());
			request.setGroupShortName(mOverallGroupB_Name.c_str());
			request.setGroupType(groupTypeName.c_str());
			
			//mAttributes1.setKey("lead_persona");
			//mAttributes1.setIValue(0);
			//mAttributes1.setSValue(eastl::string().sprintf("%" PRIu64 "", mPlayerInfo->getPlayerId()).c_str());
			//mAttributes1.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
			//request.getAttributes().push_back(&mAttributes1);

			mAttributes2.setKey("team");
			mAttributes2.setIValue(0);
			//Add all users in the teamB
			eastl::string teamBList = "";
			switch (mScenarioType)
			{
			case TEAM_1_CPU:
			case TEAM_2_CPU:
			{}
			break;
			case TEAM_1_1:
			{
				if (mPlayersType == TEAM_ALL_REG)
				{
					teamBList.append(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[0]).c_str());
				}
				else if (mPlayersType == TEAM_ALL_UNREG)
				{
					teamBList.append(mUnRegisteredStringPlayersList[0]);
				}
			}
			break;
			case TEAM_1_2:
			{
				if (mPlayersType == TEAM_ALL_REG)
				{
					teamBList.append(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[0]).c_str());
					teamBList.append(",");
					teamBList.append(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[1]).c_str());
				}
				else if (mPlayersType == TEAM_ALL_UNREG)
				{
					teamBList.append(mUnRegisteredStringPlayersList[0]);
					teamBList.append(",");
					teamBList.append(mUnRegisteredStringPlayersList[1]);
				}
			}
			break;
			case TEAM_2_2:
			{
				if (mPlayersType == TEAM_ALL_REG)
				{
					teamBList.append(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[1]).c_str());
					teamBList.append(",");
					teamBList.append(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[2]).c_str());
				}
				else if (mPlayersType == TEAM_ALL_UNREG)
				{
					teamBList.append(mUnRegisteredStringPlayersList[1]);
					teamBList.append(",");
					teamBList.append(mUnRegisteredStringPlayersList[2]);
				}
			}
			break;
			default:
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createInstance]" << mPlayerInfo->getPlayerId() << " In default state");
				return ERR_SYSTEM;
			}
			break;
			}
			mAttributes2.setSValue(teamBList.c_str());
			mAttributes2.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
			request.getAttributes().push_back(&mAttributes2);

			//Add members list (UnRegistered Players)
			if (mPlayersType == TEAM_ALL_UNREG)
			{
				switch (mScenarioType)
				{
				case TEAM_1_CPU:
				{}
				break;
				case TEAM_2_CPU:
				case TEAM_1_1:
				{
					mAttributes7.setKey("unregistered");
					mAttributes7.setIValue(0);
					//add unregistered player
					mAttributes7.setSValue(mUnRegisteredStringPlayersList[0].c_str());
					mAttributes7.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes7);

				}
				break;
				case TEAM_1_2:
				{
					mAttributes7.setKey("unregistered");
					mAttributes7.setIValue(0);
					//add unregistered player
					mAttributes7.setSValue(mUnRegisteredStringPlayersList[0].c_str());
					mAttributes7.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes7);

					mAttributes8.setKey("unreg0");
					mAttributes8.setIValue(0);
					//add unregistered player
					mAttributes8.setSValue(mUnRegisteredStringPlayersList[1].c_str());
					mAttributes8.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes8);
				}
				break;
				case TEAM_2_2:
				{
					mAttributes7.setKey("unregistered");
					mAttributes7.setIValue(0);
					//add unregistered player
					mAttributes7.setSValue(mUnRegisteredStringPlayersList[0].c_str());
					mAttributes7.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes7);

					mAttributes8.setKey("unreg0");
					mAttributes8.setIValue(0);
					//add unregistered player
					mAttributes8.setSValue(mUnRegisteredStringPlayersList[1].c_str());
					mAttributes8.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes8);

					mAttributes9.setKey("unreg1");
					mAttributes9.setIValue(0);
					//add unregistered player
					mAttributes9.setSValue(mUnRegisteredStringPlayersList[2].c_str());
					mAttributes9.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes9);
				}
				break;
				default:
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createInstance]" << mPlayerInfo->getPlayerId() << " In default state");
					return ERR_SYSTEM;
				}
				break;
				}
			}

			mAttributes3.setKey("access_timestamp");
			mAttributes3.setIValue(0);
			mAttributes3.setSValue(timeStamp.c_str());
			mAttributes3.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
			request.getAttributes().push_back(&mAttributes3);

			//Add members list
			if (mPlayersType == TEAM_ALL_REG)
			{
				switch (mScenarioType)
				{
					case TEAM_1_CPU:
					case TEAM_2_CPU:
					{}
					break;
					case TEAM_1_1:
					{
						request.getMembers().push_back(mRegisteredPlayersList[0]);
					}
					case TEAM_1_2:
					case TEAM_2_2:					
					{
						request.getMembers().push_back(mRegisteredPlayersList[0]);
						request.getMembers().push_back(mRegisteredPlayersList[1]);
					}
					break;
					default:
					{
						BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createInstance]" << mPlayerInfo->getPlayerId() << " In default state");
						return ERR_SYSTEM;
					}
					break;
				}
			}
		}
		break;
		case EADP_GROUPS_RIVAL_TYPE:
		{
			groupName = mRivalGroup_Name.c_str();
			groupShortName = mRivalGroup_Name.c_str();
			groupTypeId = groupTypeName.c_str();

			request.setGroupName(mRivalGroup_Name.c_str());
			request.setGroupShortName(mRivalGroup_Name.c_str());
			request.setGroupType(groupTypeName.c_str());
			
			mAttributes1.setKey("ai_difficulty");
			mAttributes1.setIValue(1);
			mAttributes1.setSValue("");
			mAttributes1.setType(Blaze::FifaGroups::AttributeType::TYPE_INT);
			request.getAttributes().push_back(&mAttributes1);

			mAttributes2.setKey("access_timestamp");
			mAttributes2.setIValue(0);
			mAttributes2.setSValue(timeStamp.c_str());
			mAttributes2.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
			request.getAttributes().push_back(&mAttributes2);

			mAttributes3.setKey("overall_group_0_id");
			mAttributes3.setIValue(0);
			mAttributes3.setSValue(mOverallGroupA_ID.c_str());
			mAttributes3.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
			request.getAttributes().push_back(&mAttributes3);

			mAttributes4.setKey("overall_group_1_id");
			mAttributes4.setIValue(0);
			mAttributes4.setSValue(mOverallGroupB_ID.c_str());
			mAttributes4.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
			request.getAttributes().push_back(&mAttributes4);

			mAttributes5.setKey("team0");
			mAttributes5.setIValue(0);
			//Add all users in the teamA
			eastl::string teamAList = eastl::string().sprintf("%" PRIu64 "", mPlayerInfo->getPlayerId()).c_str();
			switch (mScenarioType)
			{
				case TEAM_1_CPU:
				case TEAM_1_1:
				case TEAM_1_2:
				{}
				break;
				case TEAM_2_CPU:				
				case TEAM_2_2:
				{
					teamAList.append(",");
					if (mPlayersType == TEAM_ALL_REG)
					{
						teamAList.append(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[0]).c_str());
					}
					else if (mPlayersType == TEAM_ALL_UNREG)
					{
						teamAList.append(mUnRegisteredStringPlayersList[0]);
					}					
				}
				break;
				default:
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createInstance]" << mPlayerInfo->getPlayerId() << " In default state");
					return ERR_SYSTEM;
				}
				break;
			}
			//Add members list
			mAttributes5.setSValue(teamAList.c_str());
			mAttributes5.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
			request.getAttributes().push_back(&mAttributes5);

			mAttributes6.setKey("team1");
			mAttributes6.setIValue(0);
			//Add all users in the teamB
			eastl::string teamBList = "";
			switch (mScenarioType)
			{
				case TEAM_1_CPU:
				case TEAM_2_CPU:
				{}
				break;
				case TEAM_1_1:
				{
					if (mPlayersType == TEAM_ALL_REG)
					{
						teamBList.append(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[0]).c_str());
					}
					else if (mPlayersType == TEAM_ALL_UNREG)
					{
						teamBList.append(mUnRegisteredStringPlayersList[0]);
					}
				}
				break;
				case TEAM_1_2:
				{					
					if (mPlayersType == TEAM_ALL_REG)
					{
						teamBList.append(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[0]).c_str());
						teamBList.append(",");
						teamBList.append(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[1]).c_str());
					}
					else if (mPlayersType == TEAM_ALL_UNREG)
					{
						teamBList.append(mUnRegisteredStringPlayersList[0]);
						teamBList.append(",");
						teamBList.append(mUnRegisteredStringPlayersList[1]);
					}
				}
				break;
				case TEAM_2_2:
				{
					if (mPlayersType == TEAM_ALL_REG)
					{
						teamBList.append(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[1]).c_str());
						teamBList.append(",");
						teamBList.append(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[2]).c_str());
					}
					else if (mPlayersType == TEAM_ALL_UNREG)
					{
						teamBList.append(mUnRegisteredStringPlayersList[1]);
						teamBList.append(",");
						teamBList.append(mUnRegisteredStringPlayersList[2]);
					}
				}
				break;
				default:
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createInstance]" << mPlayerInfo->getPlayerId() << " In default state");
					return ERR_SYSTEM;
				}
				break;
			}
			mAttributes6.setSValue(teamBList.c_str());
			mAttributes6.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
			request.getAttributes().push_back(&mAttributes6);

			//Add members list (UnRegistered Players)
			if (mPlayersType == TEAM_ALL_UNREG)
			{
				switch (mScenarioType)
				{
				case TEAM_1_CPU:
				{}
				break;
				case TEAM_2_CPU:
				case TEAM_1_1:
				{
					mAttributes7.setKey("unreg0");
					mAttributes7.setIValue(0);
					//add unregistered player
					mAttributes7.setSValue(mUnRegisteredStringPlayersList[0].c_str());
					mAttributes7.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes7);

				}
				break;
				case TEAM_1_2:
				{
					mAttributes7.setKey("unreg0");
					mAttributes7.setIValue(0);
					//add unregistered player
					mAttributes7.setSValue(mUnRegisteredStringPlayersList[0].c_str());
					mAttributes7.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes7);

					mAttributes8.setKey("unreg1");
					mAttributes8.setIValue(0);
					//add unregistered player
					mAttributes8.setSValue(mUnRegisteredStringPlayersList[1].c_str());
					mAttributes8.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes8);
				}
				break;
				case TEAM_2_2:
				{
					mAttributes7.setKey("unreg0");
					mAttributes7.setIValue(0);
					//add unregistered player
					mAttributes7.setSValue(mUnRegisteredStringPlayersList[0].c_str());
					mAttributes7.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes7);

					mAttributes8.setKey("unreg1");
					mAttributes8.setIValue(0);
					//add unregistered player
					mAttributes8.setSValue(mUnRegisteredStringPlayersList[1].c_str());
					mAttributes8.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes8);

					mAttributes9.setKey("unreg2");
					mAttributes9.setIValue(0);
					//add unregistered player
					mAttributes9.setSValue(mUnRegisteredStringPlayersList[2].c_str());
					mAttributes9.setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
					request.getAttributes().push_back(&mAttributes9);
				}
				break;
				default:
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createInstance]" << mPlayerInfo->getPlayerId() << " In default state");
					return ERR_SYSTEM;
				}
				break;
				}
			}
			//Add members list (Registered Players)
			if (mPlayersType == TEAM_ALL_REG)
			{
				switch (mScenarioType)
				{
				case TEAM_1_CPU:
				{}
				break;
				case TEAM_2_CPU:				
				case TEAM_1_1:
				{
					request.getMembers().push_back(mRegisteredPlayersList[0]);
				}
				break;
				case TEAM_1_2:
				{
					request.getMembers().push_back(mRegisteredPlayersList[0]);
					request.getMembers().push_back(mRegisteredPlayersList[1]);
				}
				break;
				case TEAM_2_2:
				{
					request.getMembers().push_back(mRegisteredPlayersList[0]);
					request.getMembers().push_back(mRegisteredPlayersList[1]);
					request.getMembers().push_back(mRegisteredPlayersList[2]);
				}
				break;
				default:
				{
					BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createInstance]" << mPlayerInfo->getPlayerId() << " In default state");
					return ERR_SYSTEM;
				}
				break;
				}
			}
		}
		break;
		default:
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createInstance]" << mPlayerInfo->getPlayerId() << " In default state");
			return ERR_SYSTEM;
		}
		break;
	}	

	result = mGroupsProxyHandler->createInstance(creatorUserId, groupName, groupShortName, groupTypeId, mNucleusAccessToken);
	//char8_t buf1[20240];
	//BLAZE_TRACE_LOG(BlazeRpcLog::fifagroups, "KickOffPlayer::createInstancerequest data: playerId " << mPlayerInfo->getPlayerId() << "]::createInstance" << "\n" << request.print(buf1, sizeof(buf1)));

	// Commenting for now as createInstance rpc is not being invoked properly
	// err = mPlayerInfo->getComponentProxy()->mFifaGroupsProxy->createInstance(request, response);
	if (!result)
	{
		err = ERR_SYSTEM;
		BLAZE_ERR_LOG(BlazeRpcLog::fifagroups, "KickOffPlayer::createInstance: playerId " << mPlayerInfo->getPlayerId() << " failed to invoke createInstance HTTP call");
		return err;
	}
	/*if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::fifagroups, "KickOffPlayer::createInstance: playerId " << mPlayerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
		return err;
	}*/
	
	//char8_t buf[10240];
	//BLAZE_TRACE_LOG(BlazeRpcLog::fifagroups, "KickOffPlayer::createInstanceResponse: playerId " << mPlayerInfo->getPlayerId() << "]::createInstance" << "\n" << response.print(buf, sizeof(buf)));
	
	//Copy groupID
	eastl::string gUid = mPlayerInfo->getKickOffGroupId();

	switch (type)
	{
		case EADP_GROUPS_FRIEND_TYPE:
		{
			mFriendsGroup_ID = gUid;
			//mFriendsGroup_ID = response.getGUId();
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::createInstance]:" << mPlayerInfo->getPlayerId() << " mFriendsGroup_ID= " << mFriendsGroup_ID);
		}
		break;
		case EADP_GROUPS_OVERALL_TYPE_A:
		{
			mOverallGroupA_ID = gUid;
			//mOverallGroupA_ID = response.getGUId();
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::createInstance]:" << mPlayerInfo->getPlayerId() << " mOverallGroupA_ID= " << mOverallGroupA_ID);
		}
		break;
		case EADP_GROUPS_OVERALL_TYPE_B:
		{
			mOverallGroupB_ID = gUid;
			// mOverallGroupB_ID = response.getGUId();
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::createInstance]:" << mPlayerInfo->getPlayerId() << " mOverallGroupB_ID= " << mOverallGroupB_ID);
		}
		break;
		case EADP_GROUPS_RIVAL_TYPE:
		{
			mRivalGroup_ID = gUid;
			// mRivalGroup_ID = response.getGUId();
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[Kickoff::createInstance]:" << mPlayerInfo->getPlayerId() << " mRivalGroup_ID= " << mRivalGroup_ID);
		}
		break;
		default:
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::createInstance]" << mPlayerInfo->getPlayerId() << " In default state2");
			return ERR_SYSTEM;
		}
		break;
	}
	return err;
}

BlazeRpcError KickOffPlayer::setMultipleInstanceAttributes(EADPGroupsType type)
{
	BlazeRpcError err = ERR_OK;
	Blaze::FifaGroups::SetMultipleInstanceAttributesRequest request;
	Blaze::FifaGroups::SetMultipleInstanceAttributesResponse response;

	eastl::string groupIDName = "";
	eastl::string timeStamp = eastl::string().sprintf("%" PRIu64 "", TimeValue::getTimeOfDay().getSec()).c_str();

	switch (type)
	{
	case EADP_GROUPS_FRIEND_TYPE:
	{
		groupIDName = mFriendsGroup_ID;
	}
	break;
	case EADP_GROUPS_OVERALL_TYPE_A:
	{
		groupIDName = mOverallGroupA_ID;
	}
	break;
	case EADP_GROUPS_OVERALL_TYPE_B:
	{
		groupIDName = mOverallGroupB_ID;
	}
	break;
	case EADP_GROUPS_RIVAL_TYPE:
	{		
		groupIDName = mRivalGroup_ID;
	}
	break;
	default:
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::setMultipleInstanceAttributes]" << mPlayerInfo->getPlayerId() << " In default state");
		return ERR_SYSTEM;
	}
	break;
	}
	
	//set group ID
	request.setGroupGUId(groupIDName.c_str());

	if (type == EADP_GROUPS_FRIEND_TYPE)
	{
		Blaze::FifaGroups::AttributeList &attributeList = request.getAttributes();
		Blaze::FifaGroups::Attribute* attribute1 = request.getAttributes().pull_back();
		Blaze::FifaGroups::Attribute* attribute2 = request.getAttributes().pull_back();
		Blaze::FifaGroups::Attribute* attribute3 = request.getAttributes().pull_back();
		Blaze::FifaGroups::Attribute* attribute4 = request.getAttributes().pull_back();
		//TODO TEAM_1_1
		attribute1->setKey("access_timestamp");
		attribute1->setIValue(0);
		//sValue = "60,59,58,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,29,28,27,26,25,23,21,20,19,17,16,15,9,8,7,6,5"
		attribute1->setSValue("");
		attribute1->setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
		
		attribute2->setKey("membershiporders");
		attribute2->setIValue(0);
		//sValue = "1000240594789_58.01,1000269575449_57.02,1000240716544_57.01,1000027818794_56.01,1000028200791_0.00,1000031072799_0.00"
		eastl::string membersOrder = eastl::string().sprintf("%" PRIu64 """_0.01,""%" PRIu64 """_0.00", mPlayerInfo->getPlayerId(), mRegisteredPlayersList[0]).c_str();
		attribute2->setSValue(membersOrder.c_str());
		attribute2->setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
		
		attribute3->setKey("unregistered1");
		attribute3->setIValue(0);
		attribute3->setSValue("");
		attribute3->setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);
		
		attribute4->setKey("unregcount");
		attribute4->setIValue(0);
		//iValue = 60
		attribute4->setSValue("");
		attribute4->setType(Blaze::FifaGroups::AttributeType::TYPE_INT);

		attributeList.push_back(attribute1);
		attributeList.push_back(attribute2);
		attributeList.push_back(attribute3);
		attributeList.push_back(attribute4);

		err = mPlayerInfo->getComponentProxy()->mFifaGroupsProxy->setMultipleInstanceAttributes(request, response);
	}
	else
	{
		//Blaze::FifaGroups::AttributeList &attributeList = request.getAttributes();
		Blaze::FifaGroups::Attribute* attribute = request.getAttributes().pull_back();
		attribute->setKey("access_timestamp");
		attribute->setIValue(0);
		attribute->setSValue(timeStamp.c_str());
		attribute->setType(Blaze::FifaGroups::AttributeType::TYPE_STRING);

		err = mPlayerInfo->getComponentProxy()->mFifaGroupsProxy->setMultipleInstanceAttributes(request, response);
	}
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "KickOffPlayer::setMultipleInstanceAttributes" << type);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::fifagroups, "KickOffPlayer::setMultipleInstanceAttributes: playerId " << mPlayerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}
	/*char8_t buf[10240];
	BLAZE_TRACE_LOG(BlazeRpcLog::fifagroups, "KickOffPlayer::setMultipleInstanceAttributesResponse: playerId " << mPlayerInfo->getPlayerId() << "]::setMultipleInstanceAttributes" << "\n" << response.print(buf, sizeof(buf)));

	char8_t buf1[10240];
	BLAZE_TRACE_LOG(BlazeRpcLog::fifagroups, "KickOffPlayer::setMultipleInstanceAttributesRequest data: playerId " << mPlayerInfo->getPlayerId() << "]::setMultipleInstanceAttributes" << "\n" << request.print(buf1, sizeof(buf1)));*/
	return err;
}

BlazeRpcError KickOffPlayer::joinGroup(eastl::string guid, PlayerId playerID)
{
	BlazeRpcError err = ERR_OK;
	Blaze::FifaGroups::JoinGroupRequest request;
	Blaze::FifaGroups::JoinGroupResponse response;
	request.setGroupGUId(guid.c_str());
	request.setInviteKey("");
	request.setPassword("");
	request.setTargetUserId(playerID);
	err = mPlayerInfo->getComponentProxy()->mFifaGroupsProxy->joinGroup(request, response);
	if (err != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::fifagroups, "KickOffPlayer::joinGroup: playerId " << mPlayerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
	}

	/*char8_t buf1[10240];
	BLAZE_TRACE_LOG(BlazeRpcLog::fifagroups, "KickOffPlayer::JoinGroupRequest data: playerId " << mPlayerInfo->getPlayerId() << "]" << "\n" << request.print(buf1, sizeof(buf1)));

	char8_t buf[10240];
	BLAZE_INFO_LOG(BlazeRpcLog::fifagroups, "KickOffPlayer::JoinGroupResponse: playerId " << mPlayerInfo->getPlayerId() << " "<< "\n" << response.print(buf, sizeof(buf)));*/

	return err;
}

BlazeRpcError KickOffPlayer::updateStats_GroupID(EADPGroupsType type)
{
	BlazeRpcError err = ERR_OK;
	Blaze::FifaStats::UpdateStatsResponse updateStatsResponse;
	Blaze::FifaStats::UpdateStatsRequest updateStatsRequest;
	
	updateStatsRequest.setContextOverrideType(Blaze::FifaStats::ContextOverrideType::CONTEXT_NO_OVERRIDE);
	updateStatsRequest.setServiceName("");
	int64_t uniqueID = TimeValue::getTimeOfDay().getSec() + Random::getRandomNumber(10000);
	eastl::string updateId = eastl::string().sprintf("%" PRIu64 """-""%" PRIu64 "", mPlayerInfo->getPlayerId(), uniqueID).c_str();
	updateStatsRequest.setUpdateId(updateId.c_str());

	// Rival Stats
	if (type == EADP_GROUPS_RIVAL_TYPE && mRivalGroup_ID != "")
	{
		updateStatsRequest.setCategoryId("Rival");

		Blaze::FifaStats::Entity *entity = updateStatsRequest.getEntities().pull_back();
		Blaze::FifaStats::StatsUpdateList &statUpdateList = entity->getStatsUpdates();
		//set group ID
		entity->setEntityId(mRivalGroup_ID.c_str());
		if (!strcmp("gameType100", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
		{
			//Seen more than 397 event in request for 194
			Blaze::FifaStats::StatsUpdate *statsUpdate[387];
			for (int i = 0; i < 387; i++)
			{
				statsUpdate[i] = entity->getStatsUpdates().pull_back();
				statsUpdate[i]->setOperator(Blaze::FifaStats::StatsOperator::OPERATOR_REPLACE);
				statsUpdate[i]->setStatsFValue(0.00000000);
				statsUpdate[i]->setType(Blaze::FifaStats::StatsType::TYPE_INT);
			}
			statsUpdate[0]->setStatsIValue(5);
			statsUpdate[0]->setStatsName("T_0_g");
			statsUpdate[1]->setStatsIValue(0);
			statsUpdate[1]->setStatsName("SSF_T_0_g");
			statsUpdate[2]->setStatsIValue(0);
			statsUpdate[2]->setStatsName("T_0_og");
			statsUpdate[3]->setStatsIValue(0);
			statsUpdate[3]->setStatsName("SSF_T_0_og");
			statsUpdate[4]->setStatsIValue(0);
			statsUpdate[4]->setStatsName("T_0_pkg");
			statsUpdate[5]->setStatsIValue(0);
			statsUpdate[5]->setStatsName("SSF_T_0_pkg");
			statsUpdate[6]->setStatsIValue(55);
			statsUpdate[6]->setStatsName("T_0_pos");
			statsUpdate[7]->setStatsIValue(0);
			statsUpdate[7]->setStatsName("SSF_T_0_pos");
			statsUpdate[8]->setStatsIValue(14);
			statsUpdate[8]->setStatsName("T_0_s");
			statsUpdate[9]->setStatsIValue(0);
			statsUpdate[9]->setStatsName("SSF_T_0_s");
			statsUpdate[10]->setStatsIValue(9);
			statsUpdate[10]->setStatsName("T_0_sog");
			statsUpdate[11]->setStatsIValue(0);
			statsUpdate[11]->setStatsName("SSF_T_0_sog");
			statsUpdate[12]->setStatsIValue(74);
			statsUpdate[12]->setStatsName("T_0_pa");
			statsUpdate[13]->setStatsIValue(0);
			statsUpdate[13]->setStatsName("SSF_T_0_pa");
			statsUpdate[14]->setStatsIValue(60);
			statsUpdate[14]->setStatsName("T_0_pm");
			statsUpdate[15]->setStatsIValue(0);
			statsUpdate[15]->setStatsName("SSF_T_0_pm");
			statsUpdate[16]->setStatsIValue(9);
			statsUpdate[16]->setStatsName("T_0_ta");
			statsUpdate[17]->setStatsIValue(0);
			statsUpdate[17]->setStatsName("SSF_T_0_ta");
			statsUpdate[18]->setStatsIValue(1);
			statsUpdate[18]->setStatsName("T_0_tm");
			statsUpdate[19]->setStatsIValue(0);
			statsUpdate[19]->setStatsName("SSF_T_0_tm");
			statsUpdate[20]->setStatsIValue(0);
			statsUpdate[20]->setStatsName("T_0_gc");
			statsUpdate[21]->setStatsIValue(0);
			statsUpdate[21]->setStatsName("SSF_T_0_gc");
			statsUpdate[22]->setStatsIValue(0);
			statsUpdate[22]->setStatsName("T_0_f");
			statsUpdate[23]->setStatsIValue(0);
			statsUpdate[23]->setStatsName("SSF_T_0_f");
			statsUpdate[24]->setStatsIValue(0);
			statsUpdate[24]->setStatsName("T_0_yc");
			statsUpdate[25]->setStatsIValue(0);
			statsUpdate[25]->setStatsName("SSF_T_0_yc");
			statsUpdate[26]->setStatsIValue(0);
			statsUpdate[26]->setStatsName("T_0_rc");
			statsUpdate[27]->setStatsIValue(0);
			statsUpdate[27]->setStatsName("SSF_T_0_rc");
			statsUpdate[28]->setStatsIValue(1);
			statsUpdate[28]->setStatsName("T_0_c");
			statsUpdate[29]->setStatsIValue(0);
			statsUpdate[29]->setStatsName("SSF_T_0_c");
			statsUpdate[30]->setStatsIValue(1);
			statsUpdate[30]->setStatsName("T_0_os");
			statsUpdate[31]->setStatsIValue(0);
			statsUpdate[31]->setStatsName("SSF_T_0_os");
			statsUpdate[32]->setStatsIValue(1);
			statsUpdate[32]->setStatsName("T_0_cs");
			statsUpdate[33]->setStatsIValue(0);
			statsUpdate[33]->setStatsName("SSF_T_0_cs");
			statsUpdate[34]->setStatsIValue(3);
			statsUpdate[34]->setStatsName("T_0_sa");
			statsUpdate[35]->setStatsIValue(0);
			statsUpdate[35]->setStatsName("SSF_T_0_sa");
			statsUpdate[36]->setStatsIValue(2);
			statsUpdate[36]->setStatsName("T_0_saog");
			statsUpdate[37]->setStatsIValue(0);
			statsUpdate[37]->setStatsName("SSF_T_0_saog");
			statsUpdate[38]->setStatsIValue(19);
			statsUpdate[38]->setStatsName("T_0_int");
			statsUpdate[39]->setStatsIValue(0);
			statsUpdate[39]->setStatsName("SSF_T_0_int");
			statsUpdate[40]->setStatsIValue(0);
			statsUpdate[40]->setStatsName("T_0_pka");
			statsUpdate[41]->setStatsIValue(0);
			statsUpdate[41]->setStatsName("SSF_T_0_pka");
			statsUpdate[42]->setStatsIValue(0);
			statsUpdate[42]->setStatsName("T_0_pks");
			statsUpdate[43]->setStatsIValue(0);
			statsUpdate[43]->setStatsName("SSF_T_0_pks");
			statsUpdate[44]->setStatsIValue(0);
			statsUpdate[44]->setStatsName("T_0_pkv");
			statsUpdate[45]->setStatsIValue(0);
			statsUpdate[45]->setStatsName("SSF_T_0_pkv");
			statsUpdate[46]->setStatsIValue(0);
			statsUpdate[46]->setStatsName("T_0_pkm");
			statsUpdate[47]->setStatsIValue(0);
			statsUpdate[47]->setStatsName("SSF_T_0_pkm");
			statsUpdate[48]->setStatsIValue(14);
			statsUpdate[48]->setStatsName("T_0_pi");
			statsUpdate[49]->setStatsIValue(0);
			statsUpdate[49]->setStatsName("SSF_T_0_pi");
			statsUpdate[50]->setStatsIValue(0);
			statsUpdate[50]->setStatsName("T_0_ir");
			statsUpdate[51]->setStatsIValue(0);
			statsUpdate[51]->setStatsName("SSF_T_0_ir");
			statsUpdate[52]->setStatsIValue(27);
			statsUpdate[52]->setStatsName("T_0_atklp");
			statsUpdate[53]->setStatsIValue(0);
			statsUpdate[53]->setStatsName("SSF_T_0_atklp");
			statsUpdate[54]->setStatsIValue(47);
			statsUpdate[54]->setStatsName("T_0_atkmp");
			statsUpdate[55]->setStatsIValue(0);
			statsUpdate[55]->setStatsName("SSF_T_0_atkmp");
			statsUpdate[56]->setStatsIValue(26);
			statsUpdate[56]->setStatsName("T_0_atkrp");
			statsUpdate[57]->setStatsIValue(0);
			statsUpdate[57]->setStatsName("SSF_T_0_atkrp");
			statsUpdate[58]->setStatsIValue(1);
			statsUpdate[58]->setStatsName("T_0_g15");
			statsUpdate[59]->setStatsIValue(0);
			statsUpdate[59]->setStatsName("SSF_T_0_g15");
			statsUpdate[60]->setStatsIValue(1);
			statsUpdate[60]->setStatsName("T_0_g30");
			statsUpdate[61]->setStatsIValue(0);
			statsUpdate[61]->setStatsName("SSF_T_0_g30");
			statsUpdate[62]->setStatsIValue(1);
			statsUpdate[62]->setStatsName("T_0_g45");
			statsUpdate[63]->setStatsIValue(0);
			statsUpdate[63]->setStatsName("SSF_T_0_g45");
			statsUpdate[64]->setStatsIValue(1);
			statsUpdate[64]->setStatsName("T_0_g60");
			statsUpdate[65]->setStatsIValue(0);
			statsUpdate[65]->setStatsName("SSF_T_0_g60");
			statsUpdate[66]->setStatsIValue(0);
			statsUpdate[66]->setStatsName("T_0_g75");
			statsUpdate[67]->setStatsIValue(0);
			statsUpdate[67]->setStatsName("SSF_T_0_g75");
			statsUpdate[68]->setStatsIValue(1);
			statsUpdate[68]->setStatsName("T_0_g90");
			statsUpdate[69]->setStatsIValue(0);
			statsUpdate[69]->setStatsName("SSF_T_0_g90");
			statsUpdate[70]->setStatsIValue(0);
			statsUpdate[70]->setStatsName("T_0_g90Plus");
			statsUpdate[71]->setStatsIValue(0);
			statsUpdate[71]->setStatsName("SSF_T_0_g90Plus");
			statsUpdate[72]->setStatsIValue(0);
			statsUpdate[72]->setStatsName("T_0_gob");
			statsUpdate[73]->setStatsIValue(0);
			statsUpdate[73]->setStatsName("SSF_T_0_gob");
			statsUpdate[74]->setStatsIValue(5);
			statsUpdate[74]->setStatsName("T_0_gib");
			statsUpdate[75]->setStatsIValue(0);
			statsUpdate[75]->setStatsName("SSF_T_0_gib");
			statsUpdate[76]->setStatsIValue(0);
			statsUpdate[76]->setStatsName("T_0_gfk");
			statsUpdate[77]->setStatsIValue(0);
			statsUpdate[77]->setStatsName("SSF_T_0_gfk");
			statsUpdate[78]->setStatsIValue(0);
			statsUpdate[78]->setStatsName("T_0_g2pt");
			statsUpdate[79]->setStatsIValue(0);
			statsUpdate[79]->setStatsName("SSF_T_0_g2pt");
			statsUpdate[80]->setStatsIValue(0);
			statsUpdate[80]->setStatsName("T_0_g3pt");
			statsUpdate[81]->setStatsIValue(0);
			statsUpdate[81]->setStatsName("SSF_T_0_g3pt");
			statsUpdate[82]->setStatsIValue(24);
			statsUpdate[82]->setStatsName("T_0_posl");
			statsUpdate[83]->setStatsIValue(0);
			statsUpdate[83]->setStatsName("SSF_T_0_posl");
			statsUpdate[84]->setStatsIValue(50);
			statsUpdate[84]->setStatsName("T_0_cdr");
			statsUpdate[85]->setStatsIValue(0);
			statsUpdate[85]->setStatsName("SSF_T_0_cdr");
			statsUpdate[86]->setStatsIValue(1);
			statsUpdate[86]->setStatsName("S_0_ow");
			statsUpdate[87]->setStatsIValue(0);
			statsUpdate[87]->setStatsName("S_0_od");
			statsUpdate[88]->setStatsIValue(0);
			statsUpdate[88]->setStatsName("S_0_ol");
			statsUpdate[89]->setStatsIValue(1);
			statsUpdate[89]->setStatsName("S_0_smw");
			statsUpdate[90]->setStatsIValue(0);
			statsUpdate[90]->setStatsName("S_0_smd");
			statsUpdate[91]->setStatsIValue(0);
			statsUpdate[91]->setStatsName("S_0_sml");
			statsUpdate[92]->setStatsIValue(0);
			statsUpdate[92]->setStatsName("S_0_cw");
			statsUpdate[93]->setStatsIValue(0);
			statsUpdate[93]->setStatsName("S_0_cd");
			statsUpdate[94]->setStatsIValue(0);
			statsUpdate[94]->setStatsName("S_0_cl");
			statsUpdate[95]->setStatsIValue(0);
			statsUpdate[95]->setStatsName("S_0_suw");
			statsUpdate[96]->setStatsIValue(0);
			statsUpdate[96]->setStatsName("S_0_sud");
			statsUpdate[97]->setStatsIValue(0);
			statsUpdate[97]->setStatsName("S_0_sul");
			statsUpdate[98]->setStatsIValue(0);
			statsUpdate[98]->setStatsName("S_0_baw");
			statsUpdate[99]->setStatsIValue(0);
			statsUpdate[99]->setStatsName("S_0_bad");
			statsUpdate[100]->setStatsIValue(0);
			statsUpdate[100]->setStatsName("S_0_bal");
			statsUpdate[101]->setStatsIValue(0);
			statsUpdate[101]->setStatsName("S_0_fxw");
			statsUpdate[102]->setStatsIValue(0);
			statsUpdate[102]->setStatsName("S_0_fxd");
			statsUpdate[103]->setStatsIValue(0);
			statsUpdate[103]->setStatsName("S_0_fxl");
			statsUpdate[104]->setStatsIValue(0);
			statsUpdate[104]->setStatsName("S_0_nrw");
			statsUpdate[105]->setStatsIValue(0);
			statsUpdate[105]->setStatsName("S_0_nrd");
			statsUpdate[106]->setStatsIValue(0);
			statsUpdate[106]->setStatsName("S_0_nrl");
			statsUpdate[107]->setStatsIValue(0);
			statsUpdate[107]->setStatsName("S_0_hvw");
			statsUpdate[108]->setStatsIValue(0);
			statsUpdate[108]->setStatsName("S_0_hvd");
			statsUpdate[109]->setStatsIValue(0);
			statsUpdate[109]->setStatsName("S_0_hvl");
			statsUpdate[110]->setStatsIValue(0);
			statsUpdate[110]->setStatsName("S_0_mbw");
			statsUpdate[111]->setStatsIValue(0);
			statsUpdate[111]->setStatsName("S_0_mbd");
			statsUpdate[112]->setStatsIValue(0);
			statsUpdate[112]->setStatsName("S_0_mbl");
			statsUpdate[113]->setStatsIValue(0);
			statsUpdate[113]->setStatsName("S_0_tww");
			statsUpdate[114]->setStatsIValue(0);
			statsUpdate[114]->setStatsName("S_0_twd");
			statsUpdate[115]->setStatsIValue(0);
			statsUpdate[115]->setStatsName("S_0_twl");
			statsUpdate[116]->setStatsIValue(0);
			statsUpdate[116]->setStatsName("S_0_tlw");
			statsUpdate[117]->setStatsIValue(0);
			statsUpdate[117]->setStatsName("S_0_tll");
			statsUpdate[118]->setStatsIValue(0);
			statsUpdate[118]->setStatsName("S_0_bxw");
			statsUpdate[119]->setStatsIValue(0);
			statsUpdate[119]->setStatsName("S_0_bxl");
			statsUpdate[120]->setStatsIValue(1);
			statsUpdate[120]->setStatsName("S_0_lws");
			statsUpdate[121]->setStatsIValue(1);
			statsUpdate[121]->setStatsName("S_0_cst");
			statsUpdate[122]->setStatsIValue(780);
			statsUpdate[122]->setStatsName("S_0_fgs");
			statsUpdate[123]->setStatsIValue(0);
			statsUpdate[123]->setStatsName("S_0_fgt");
			statsUpdate[124]->setStatsIValue(111139);
			statsUpdate[124]->setStatsName("S_0_fgmt");
			statsUpdate[125]->setStatsIValue(111651);
			statsUpdate[125]->setStatsName("S_0_fgot");
			statsUpdate[126]->setStatsIValue(0);
			statsUpdate[126]->setStatsName("S_0_ssfow");
			statsUpdate[127]->setStatsIValue(0);
			statsUpdate[127]->setStatsName("S_0_ssfod");
			statsUpdate[128]->setStatsIValue(0);
			statsUpdate[128]->setStatsName("S_0_ssfol");
			statsUpdate[129]->setStatsIValue(0);
			statsUpdate[129]->setStatsName("S_0_ssfsmw");
			statsUpdate[130]->setStatsIValue(0);
			statsUpdate[130]->setStatsName("S_0_ssfsmd");
			statsUpdate[131]->setStatsIValue(0);
			statsUpdate[131]->setStatsName("S_0_ssfsml");
			statsUpdate[132]->setStatsIValue(0);
			statsUpdate[132]->setStatsName("S_0_ssfsuw");
			statsUpdate[133]->setStatsIValue(0);
			statsUpdate[133]->setStatsName("S_0_ssfsud");
			statsUpdate[134]->setStatsIValue(0);
			statsUpdate[134]->setStatsName("S_0_ssfsul");
			statsUpdate[135]->setStatsIValue(0);
			statsUpdate[135]->setStatsName("S_0_ssffxw");
			statsUpdate[136]->setStatsIValue(0);
			statsUpdate[136]->setStatsName("S_0_ssffxd");
			statsUpdate[137]->setStatsIValue(0);
			statsUpdate[137]->setStatsName("S_0_ssffxl");
			statsUpdate[138]->setStatsIValue(0);
			statsUpdate[138]->setStatsName("S_0_ssfnrw");
			statsUpdate[139]->setStatsIValue(0);
			statsUpdate[139]->setStatsName("S_0_ssfnrd");
			statsUpdate[140]->setStatsIValue(0);
			statsUpdate[140]->setStatsName("S_0_ssfnrl");
			statsUpdate[141]->setStatsIValue(0);
			statsUpdate[141]->setStatsName("S_0_ssflws");
			statsUpdate[142]->setStatsIValue(0);
			statsUpdate[142]->setStatsName("S_0_ssfcst");
			statsUpdate[143]->setStatsIValue(0);
			statsUpdate[143]->setStatsName("S_0_ssffgs");
			statsUpdate[144]->setStatsIValue(0);
			statsUpdate[144]->setStatsName("S_0_ssffgt");
			statsUpdate[145]->setStatsIValue(0);
			statsUpdate[145]->setStatsName("S_0_ssffgmt");
			statsUpdate[146]->setStatsIValue(0);
			statsUpdate[146]->setStatsName("S_0_ssffgot");
			statsUpdate[147]->setStatsIValue(0);
			statsUpdate[147]->setStatsName("SSF_H_0_0");
			statsUpdate[148]->setStatsIValue(1);
			statsUpdate[148]->setStatsName("H_0_0_0");
			statsUpdate[149]->setStatsIValue(0);
			statsUpdate[149]->setStatsName("H_0_0_1");
			statsUpdate[150]->setStatsIValue(0);
			statsUpdate[150]->setStatsName("SSF_H_0_1");
			statsUpdate[151]->setStatsIValue(1);
			statsUpdate[151]->setStatsName("H_0_1_0");
			statsUpdate[152]->setStatsIValue(0);
			statsUpdate[152]->setStatsName("H_0_1_1");
			statsUpdate[153]->setStatsIValue(0);
			statsUpdate[153]->setStatsName("SSF_H_0_2");
			statsUpdate[154]->setStatsIValue(2);
			statsUpdate[154]->setStatsName("H_0_2_0");
			statsUpdate[155]->setStatsIValue(1);
			statsUpdate[155]->setStatsName("H_0_2_1");
			statsUpdate[156]->setStatsIValue(0);
			statsUpdate[156]->setStatsName("T_1_g");
			statsUpdate[157]->setStatsIValue(0);
			statsUpdate[157]->setStatsName("SSF_T_1_g");
			statsUpdate[158]->setStatsIValue(0);
			statsUpdate[158]->setStatsName("T_1_og");
			statsUpdate[159]->setStatsIValue(0);
			statsUpdate[159]->setStatsName("SSF_T_1_og");
			statsUpdate[160]->setStatsIValue(0);
			statsUpdate[160]->setStatsName("T_1_pkg");
			statsUpdate[161]->setStatsIValue(0);
			statsUpdate[161]->setStatsName("SSF_T_1_pkg");
			statsUpdate[162]->setStatsIValue(45);
			statsUpdate[162]->setStatsName("T_1_pos");
			statsUpdate[163]->setStatsIValue(0);
			statsUpdate[163]->setStatsName("SSF_T_1_pos");
			statsUpdate[164]->setStatsIValue(3);
			statsUpdate[164]->setStatsName("T_1_s");
			statsUpdate[165]->setStatsIValue(0);
			statsUpdate[165]->setStatsName("SSF_T_1_s");
			statsUpdate[166]->setStatsIValue(2);
			statsUpdate[166]->setStatsName("T_1_sog");
			statsUpdate[167]->setStatsIValue(0);
			statsUpdate[167]->setStatsName("SSF_T_1_sog");
			statsUpdate[168]->setStatsIValue(94);
			statsUpdate[168]->setStatsName("T_1_pa");
			statsUpdate[169]->setStatsIValue(0);
			statsUpdate[169]->setStatsName("SSF_T_1_pa");
			statsUpdate[170]->setStatsIValue(75);
			statsUpdate[170]->setStatsName("T_1_pm");
			statsUpdate[171]->setStatsIValue(0);
			statsUpdate[171]->setStatsName("SSF_T_1_pm");
			statsUpdate[172]->setStatsIValue(4);
			statsUpdate[172]->setStatsName("T_1_ta");
			statsUpdate[173]->setStatsIValue(0);
			statsUpdate[173]->setStatsName("SSF_T_1_ta");
			statsUpdate[174]->setStatsIValue(1);
			statsUpdate[174]->setStatsName("T_1_tm");
			statsUpdate[175]->setStatsIValue(0);
			statsUpdate[175]->setStatsName("SSF_T_1_tm");
			statsUpdate[176]->setStatsIValue(5);
			statsUpdate[176]->setStatsName("T_1_gc");
			statsUpdate[177]->setStatsIValue(0);
			statsUpdate[177]->setStatsName("SSF_T_1_gc");
			statsUpdate[178]->setStatsIValue(1);
			statsUpdate[178]->setStatsName("T_1_f");
			statsUpdate[179]->setStatsIValue(0);
			statsUpdate[179]->setStatsName("SSF_T_1_f");
			statsUpdate[180]->setStatsIValue(1);
			statsUpdate[180]->setStatsName("T_1_yc");
			statsUpdate[181]->setStatsIValue(0);
			statsUpdate[181]->setStatsName("SSF_T_1_yc");
			statsUpdate[182]->setStatsIValue(0);
			statsUpdate[182]->setStatsName("T_1_rc");
			statsUpdate[183]->setStatsIValue(0);
			statsUpdate[183]->setStatsName("SSF_T_1_rc");
			statsUpdate[184]->setStatsIValue(0);
			statsUpdate[184]->setStatsName("T_1_c");
			statsUpdate[185]->setStatsIValue(0);
			statsUpdate[185]->setStatsName("SSF_T_1_c");
			statsUpdate[186]->setStatsIValue(0);
			statsUpdate[186]->setStatsName("T_1_os");
			statsUpdate[187]->setStatsIValue(0);
			statsUpdate[187]->setStatsName("SSF_T_1_os");
			statsUpdate[188]->setStatsIValue(0);
			statsUpdate[188]->setStatsName("T_1_cs");
			statsUpdate[189]->setStatsIValue(0);
			statsUpdate[189]->setStatsName("SSF_T_1_cs");
			statsUpdate[190]->setStatsIValue(14);
			statsUpdate[190]->setStatsName("T_1_sa");
			statsUpdate[191]->setStatsIValue(0);
			statsUpdate[191]->setStatsName("SSF_T_1_sa");
			statsUpdate[192]->setStatsIValue(9);
			statsUpdate[192]->setStatsName("T_1_saog");
			statsUpdate[193]->setStatsIValue(0);
			statsUpdate[193]->setStatsName("SSF_T_1_saog");
			statsUpdate[194]->setStatsIValue(14);
			statsUpdate[194]->setStatsName("T_1_int");
			statsUpdate[195]->setStatsIValue(0);
			statsUpdate[195]->setStatsName("SSF_T_1_int");
			statsUpdate[196]->setStatsIValue(0);
			statsUpdate[196]->setStatsName("T_1_pka");
			statsUpdate[197]->setStatsIValue(0);
			statsUpdate[197]->setStatsName("SSF_T_1_pka");
			statsUpdate[198]->setStatsIValue(0);
			statsUpdate[198]->setStatsName("T_1_pks");
			statsUpdate[199]->setStatsIValue(0);
			statsUpdate[199]->setStatsName("SSF_T_1_pks");
			statsUpdate[200]->setStatsIValue(0);
			statsUpdate[200]->setStatsName("T_1_pkv");
			statsUpdate[201]->setStatsIValue(0);
			statsUpdate[201]->setStatsName("SSF_T_1_pkv");
			statsUpdate[202]->setStatsIValue(0);
			statsUpdate[202]->setStatsName("T_1_pkm");
			statsUpdate[203]->setStatsIValue(0);
			statsUpdate[203]->setStatsName("SSF_T_1_pkm");
			statsUpdate[204]->setStatsIValue(19);
			statsUpdate[204]->setStatsName("T_1_pi");
			statsUpdate[205]->setStatsIValue(0);
			statsUpdate[205]->setStatsName("SSF_T_1_pi");
			statsUpdate[206]->setStatsIValue(0);
			statsUpdate[206]->setStatsName("T_1_ir");
			statsUpdate[207]->setStatsIValue(0);
			statsUpdate[207]->setStatsName("SSF_T_1_ir");
			statsUpdate[208]->setStatsIValue(36);
			statsUpdate[208]->setStatsName("T_1_atklp");
			statsUpdate[209]->setStatsIValue(0);
			statsUpdate[209]->setStatsName("SSF_T_1_atklp");
			statsUpdate[210]->setStatsIValue(47);
			statsUpdate[210]->setStatsName("T_1_atkmp");
			statsUpdate[211]->setStatsIValue(0);
			statsUpdate[211]->setStatsName("SSF_T_1_atkmp");
			statsUpdate[212]->setStatsIValue(17);
			statsUpdate[212]->setStatsName("T_1_atkrp");
			statsUpdate[213]->setStatsIValue(0);
			statsUpdate[213]->setStatsName("SSF_T_1_atkrp");
			statsUpdate[214]->setStatsIValue(0);
			statsUpdate[214]->setStatsName("T_1_g15");
			statsUpdate[215]->setStatsIValue(0);
			statsUpdate[215]->setStatsName("SSF_T_1_g15");
			statsUpdate[216]->setStatsIValue(0);
			statsUpdate[216]->setStatsName("T_1_g30");
			statsUpdate[217]->setStatsIValue(0);
			statsUpdate[217]->setStatsName("SSF_T_1_g30");
			statsUpdate[218]->setStatsIValue(0);
			statsUpdate[218]->setStatsName("T_1_g45");
			statsUpdate[219]->setStatsIValue(0);
			statsUpdate[219]->setStatsName("SSF_T_1_g45");
			statsUpdate[220]->setStatsIValue(0);
			statsUpdate[220]->setStatsName("T_1_g60");
			statsUpdate[221]->setStatsIValue(0);
			statsUpdate[221]->setStatsName("SSF_T_1_g60");
			statsUpdate[222]->setStatsIValue(0);
			statsUpdate[222]->setStatsName("T_1_g75");
			statsUpdate[223]->setStatsIValue(0);
			statsUpdate[223]->setStatsName("SSF_T_1_g75");
			statsUpdate[224]->setStatsIValue(0);
			statsUpdate[224]->setStatsName("T_1_g90");
			statsUpdate[225]->setStatsIValue(0);
			statsUpdate[225]->setStatsName("SSF_T_1_g90");
			statsUpdate[226]->setStatsIValue(0);
			statsUpdate[226]->setStatsName("T_1_g90Plus");
			statsUpdate[227]->setStatsIValue(0);
			statsUpdate[227]->setStatsName("SSF_T_1_g90Plus");
			statsUpdate[228]->setStatsIValue(0);
			statsUpdate[228]->setStatsName("T_1_gob");
			statsUpdate[229]->setStatsIValue(0);
			statsUpdate[229]->setStatsName("SSF_T_1_gob");
			statsUpdate[230]->setStatsIValue(0);
			statsUpdate[230]->setStatsName("T_1_gib");
			statsUpdate[231]->setStatsIValue(0);
			statsUpdate[231]->setStatsName("SSF_T_1_gib");
			statsUpdate[232]->setStatsIValue(0);
			statsUpdate[232]->setStatsName("T_1_gfk");
			statsUpdate[233]->setStatsIValue(0);
			statsUpdate[233]->setStatsName("SSF_T_1_gfk");
			statsUpdate[234]->setStatsIValue(0);
			statsUpdate[234]->setStatsName("T_1_g2pt");
			statsUpdate[235]->setStatsIValue(0);
			statsUpdate[235]->setStatsName("SSF_T_1_g2pt");
			statsUpdate[236]->setStatsIValue(0);
			statsUpdate[236]->setStatsName("T_1_g3pt");
			statsUpdate[237]->setStatsIValue(0);
			statsUpdate[237]->setStatsName("SSF_T_1_g3pt");
			statsUpdate[238]->setStatsIValue(21);
			statsUpdate[238]->setStatsName("T_1_posl");
			statsUpdate[239]->setStatsIValue(0);
			statsUpdate[239]->setStatsName("SSF_T_1_posl");
			statsUpdate[240]->setStatsIValue(70);
			statsUpdate[240]->setStatsName("T_1_cdr");
			statsUpdate[241]->setStatsIValue(0);
			statsUpdate[241]->setStatsName("SSF_T_1_cdr");
			statsUpdate[242]->setStatsIValue(0);
			statsUpdate[242]->setStatsName("S_1_ow");
			statsUpdate[243]->setStatsIValue(0);
			statsUpdate[243]->setStatsName("S_1_od");
			statsUpdate[244]->setStatsIValue(1);
			statsUpdate[244]->setStatsName("S_1_ol");
			statsUpdate[245]->setStatsIValue(0);
			statsUpdate[245]->setStatsName("S_1_smw");
			statsUpdate[246]->setStatsIValue(0);
			statsUpdate[246]->setStatsName("S_1_smd");
			statsUpdate[247]->setStatsIValue(1);
			statsUpdate[247]->setStatsName("S_1_sml");
			statsUpdate[248]->setStatsIValue(0);
			statsUpdate[248]->setStatsName("S_1_cw");
			statsUpdate[249]->setStatsIValue(0);
			statsUpdate[249]->setStatsName("S_1_cd");
			statsUpdate[250]->setStatsIValue(0);
			statsUpdate[250]->setStatsName("S_1_cl");
			statsUpdate[251]->setStatsIValue(0);
			statsUpdate[251]->setStatsName("S_1_suw");
			statsUpdate[252]->setStatsIValue(0);
			statsUpdate[252]->setStatsName("S_1_sud");
			statsUpdate[253]->setStatsIValue(0);
			statsUpdate[253]->setStatsName("S_1_sul");
			statsUpdate[254]->setStatsIValue(0);
			statsUpdate[254]->setStatsName("S_1_baw");
			statsUpdate[255]->setStatsIValue(0);
			statsUpdate[255]->setStatsName("S_1_bad");
			statsUpdate[256]->setStatsIValue(0);
			statsUpdate[256]->setStatsName("S_1_bal");
			statsUpdate[257]->setStatsIValue(0);
			statsUpdate[257]->setStatsName("S_1_fxw");
			statsUpdate[258]->setStatsIValue(0);
			statsUpdate[258]->setStatsName("S_1_fxd");
			statsUpdate[259]->setStatsIValue(0);
			statsUpdate[259]->setStatsName("S_1_fxl");
			statsUpdate[260]->setStatsIValue(0);
			statsUpdate[260]->setStatsName("S_1_nrw");
			statsUpdate[261]->setStatsIValue(0);
			statsUpdate[261]->setStatsName("S_1_nrd");
			statsUpdate[262]->setStatsIValue(0);
			statsUpdate[262]->setStatsName("S_1_nrl");
			statsUpdate[263]->setStatsIValue(0);
			statsUpdate[263]->setStatsName("S_1_hvw");
			statsUpdate[264]->setStatsIValue(0);
			statsUpdate[264]->setStatsName("S_1_hvd");
			statsUpdate[265]->setStatsIValue(0);
			statsUpdate[265]->setStatsName("S_1_hvl");
			statsUpdate[266]->setStatsIValue(0);
			statsUpdate[266]->setStatsName("S_1_mbw");
			statsUpdate[267]->setStatsIValue(0);
			statsUpdate[267]->setStatsName("S_1_mbd");
			statsUpdate[268]->setStatsIValue(0);
			statsUpdate[268]->setStatsName("S_1_mbl");
			statsUpdate[269]->setStatsIValue(0);
			statsUpdate[269]->setStatsName("S_1_tww");
			statsUpdate[270]->setStatsIValue(0);
			statsUpdate[270]->setStatsName("S_1_twd");
			statsUpdate[271]->setStatsIValue(0);
			statsUpdate[271]->setStatsName("S_1_twl");
			statsUpdate[272]->setStatsIValue(0);
			statsUpdate[272]->setStatsName("S_1_tlw");
			statsUpdate[273]->setStatsIValue(0);
			statsUpdate[273]->setStatsName("S_1_tll");
			statsUpdate[274]->setStatsIValue(0);
			statsUpdate[274]->setStatsName("S_1_bxw");
			statsUpdate[275]->setStatsIValue(0);
			statsUpdate[275]->setStatsName("S_1_bxl");
			statsUpdate[276]->setStatsIValue(0);
			statsUpdate[276]->setStatsName("S_1_lws");
			statsUpdate[277]->setStatsIValue(-1);
			statsUpdate[277]->setStatsName("S_1_cst");
			statsUpdate[278]->setStatsIValue(0);
			statsUpdate[278]->setStatsName("S_1_fgs");
			statsUpdate[279]->setStatsIValue(0);
			statsUpdate[279]->setStatsName("S_1_fgt");
			statsUpdate[280]->setStatsIValue(0);
			statsUpdate[280]->setStatsName("S_1_fgmt");
			statsUpdate[281]->setStatsIValue(0);
			statsUpdate[281]->setStatsName("S_1_fgot");
			statsUpdate[282]->setStatsIValue(0);
			statsUpdate[282]->setStatsName("S_1_ssfow");
			statsUpdate[283]->setStatsIValue(0);
			statsUpdate[283]->setStatsName("S_1_ssfod");
			statsUpdate[284]->setStatsIValue(0);
			statsUpdate[284]->setStatsName("S_1_ssfol");
			statsUpdate[285]->setStatsIValue(0);
			statsUpdate[285]->setStatsName("S_1_ssfsmw");
			statsUpdate[286]->setStatsIValue(0);
			statsUpdate[286]->setStatsName("S_1_ssfsmd");
			statsUpdate[287]->setStatsIValue(0);
			statsUpdate[287]->setStatsName("S_1_ssfsml");
			statsUpdate[288]->setStatsIValue(0);
			statsUpdate[288]->setStatsName("S_1_ssfsuw");
			statsUpdate[289]->setStatsIValue(0);
			statsUpdate[289]->setStatsName("S_1_ssfsud");
			statsUpdate[290]->setStatsIValue(0);
			statsUpdate[290]->setStatsName("S_1_ssfsul");
			statsUpdate[291]->setStatsIValue(0);
			statsUpdate[291]->setStatsName("S_1_ssffxw");
			statsUpdate[292]->setStatsIValue(0);
			statsUpdate[292]->setStatsName("S_1_ssffxd");
			statsUpdate[293]->setStatsIValue(0);
			statsUpdate[293]->setStatsName("S_1_ssffxl");
			statsUpdate[294]->setStatsIValue(0);
			statsUpdate[294]->setStatsName("S_1_ssfnrw");
			statsUpdate[295]->setStatsIValue(0);
			statsUpdate[295]->setStatsName("S_1_ssfnrd");
			statsUpdate[296]->setStatsIValue(0);
			statsUpdate[296]->setStatsName("S_1_ssfnrl");
			statsUpdate[297]->setStatsIValue(0);
			statsUpdate[297]->setStatsName("S_1_ssflws");
			statsUpdate[298]->setStatsIValue(0);
			statsUpdate[298]->setStatsName("S_1_ssfcst");
			statsUpdate[299]->setStatsIValue(0);
			statsUpdate[299]->setStatsName("S_1_ssffgs");
			statsUpdate[300]->setStatsIValue(0);
			statsUpdate[300]->setStatsName("S_1_ssffgt");
			statsUpdate[301]->setStatsIValue(0);
			statsUpdate[301]->setStatsName("S_1_ssffgmt");
			statsUpdate[302]->setStatsIValue(0);
			statsUpdate[302]->setStatsName("S_1_ssffgot");
			statsUpdate[303]->setStatsIValue(0);
			statsUpdate[303]->setStatsName("SSF_H_1_0");
			statsUpdate[304]->setStatsIValue(0);
			statsUpdate[304]->setStatsName("H_1_0_0");
			statsUpdate[305]->setStatsIValue(0);
			statsUpdate[305]->setStatsName("H_1_0_1");
			statsUpdate[306]->setStatsIValue(0);
			statsUpdate[306]->setStatsName("SSF_H_1_1");
			statsUpdate[307]->setStatsIValue(0);
			statsUpdate[307]->setStatsName("H_1_1_0");
			statsUpdate[308]->setStatsIValue(0);
			statsUpdate[308]->setStatsName("H_1_1_1");
			statsUpdate[309]->setStatsIValue(0);
			statsUpdate[309]->setStatsName("SSF_H_1_2");
			statsUpdate[310]->setStatsIValue(0);
			statsUpdate[310]->setStatsName("H_1_2_0");
			statsUpdate[311]->setStatsIValue(0);
			statsUpdate[311]->setStatsName("H_1_2_1");
			statsUpdate[312]->setStatsIValue(1);
			statsUpdate[312]->setStatsName("G_O0_gn");
			statsUpdate[313]->setStatsIValue(0);
			statsUpdate[313]->setStatsName("G_W0_gn");
			statsUpdate[314]->setStatsIValue(0);
			statsUpdate[314]->setStatsName("G_W1_gn");
			statsUpdate[315]->setStatsIValue(0);
			statsUpdate[315]->setStatsName("SSF_G_W0_gn");
			statsUpdate[316]->setStatsIValue(0);
			statsUpdate[316]->setStatsName("SSF_G_W1_gn");
			statsUpdate[317]->setStatsIValue(0);
			statsUpdate[317]->setStatsName("G_O0_gt");
			statsUpdate[318]->setStatsIValue(0);
			statsUpdate[318]->setStatsName("G_W0_gt");
			statsUpdate[319]->setStatsIValue(0);
			statsUpdate[319]->setStatsName("G_W1_gt");
			statsUpdate[320]->setStatsIValue(0);
			statsUpdate[320]->setStatsName("SSF_G_W0_gt");
			statsUpdate[321]->setStatsIValue(0);
			statsUpdate[321]->setStatsName("SSF_G_W1_gt");
			statsUpdate[322]->setStatsIValue(0);
			statsUpdate[322]->setStatsName("G_O0_gmd");
			statsUpdate[323]->setStatsIValue(0);
			statsUpdate[323]->setStatsName("G_W0_gmd");
			statsUpdate[324]->setStatsIValue(0);
			statsUpdate[324]->setStatsName("G_W1_gmd");
			statsUpdate[325]->setStatsIValue(0);
			statsUpdate[325]->setStatsName("SSF_G_W0_gmd");
			statsUpdate[326]->setStatsIValue(0);
			statsUpdate[326]->setStatsName("SSF_G_W1_gmd");
			statsUpdate[327]->setStatsIValue(5);
			statsUpdate[327]->setStatsName("G_O0_t0g");
			statsUpdate[328]->setStatsIValue(5);
			statsUpdate[328]->setStatsName("G_W0_t0g");
			statsUpdate[329]->setStatsIValue(0);
			statsUpdate[329]->setStatsName("G_W1_t0g");
			statsUpdate[330]->setStatsIValue(0);
			statsUpdate[330]->setStatsName("SSF_G_W0_t0g");
			statsUpdate[331]->setStatsIValue(0);
			statsUpdate[331]->setStatsName("SSF_G_W1_t0g");
			statsUpdate[332]->setStatsIValue(0);
			statsUpdate[332]->setStatsName("G_O0_t1g");
			statsUpdate[333]->setStatsIValue(0);
			statsUpdate[333]->setStatsName("G_W0_t1g");
			statsUpdate[334]->setStatsIValue(0);
			statsUpdate[334]->setStatsName("G_W1_t1g");
			statsUpdate[335]->setStatsIValue(0);
			statsUpdate[335]->setStatsName("SSF_G_W0_t1g");
			statsUpdate[336]->setStatsIValue(0);
			statsUpdate[336]->setStatsName("SSF_G_W1_t1g");
			statsUpdate[337]->setStatsIValue(0);
			statsUpdate[337]->setStatsName("G_O0_t0pk");
			statsUpdate[338]->setStatsIValue(0);
			statsUpdate[338]->setStatsName("G_W0_t0pk");
			statsUpdate[339]->setStatsIValue(0);
			statsUpdate[339]->setStatsName("G_W1_t0pk");
			statsUpdate[340]->setStatsIValue(0);
			statsUpdate[340]->setStatsName("SSF_G_W0_t0pk");
			statsUpdate[341]->setStatsIValue(0);
			statsUpdate[341]->setStatsName("SSF_G_W1_t0pk");
			statsUpdate[342]->setStatsIValue(0);
			statsUpdate[342]->setStatsName("G_O0_t1pk");
			statsUpdate[343]->setStatsIValue(0);
			statsUpdate[343]->setStatsName("G_W0_t1pk");
			statsUpdate[344]->setStatsIValue(0);
			statsUpdate[344]->setStatsName("G_W1_t1pk");
			statsUpdate[345]->setStatsIValue(0);
			statsUpdate[345]->setStatsName("SSF_G_W0_t1pk");
			statsUpdate[346]->setStatsIValue(0);
			statsUpdate[346]->setStatsName("SSF_G_W1_t1pk");
			statsUpdate[347]->setStatsIValue(0);
			statsUpdate[347]->setStatsName("G_O0_t02pt");
			statsUpdate[348]->setStatsIValue(0);
			statsUpdate[348]->setStatsName("G_W0_t02pt");
			statsUpdate[349]->setStatsIValue(0);
			statsUpdate[349]->setStatsName("G_W1_t02pt");
			statsUpdate[350]->setStatsIValue(0);
			statsUpdate[350]->setStatsName("SSF_G_W0_t02pt");
			statsUpdate[351]->setStatsIValue(0);
			statsUpdate[351]->setStatsName("SSF_G_W1_t02pt");
			statsUpdate[352]->setStatsIValue(0);
			statsUpdate[352]->setStatsName("G_O0_t12pt");
			statsUpdate[353]->setStatsIValue(0);
			statsUpdate[353]->setStatsName("G_W0_t12pt");
			statsUpdate[354]->setStatsIValue(0);
			statsUpdate[354]->setStatsName("G_W1_t12pt");
			statsUpdate[355]->setStatsIValue(0);
			statsUpdate[355]->setStatsName("SSF_G_W0_t12pt");
			statsUpdate[356]->setStatsIValue(0);
			statsUpdate[356]->setStatsName("SSF_G_W1_t12pt");
			statsUpdate[357]->setStatsIValue(0);
			statsUpdate[357]->setStatsName("G_O0_t03pt");
			statsUpdate[358]->setStatsIValue(0);
			statsUpdate[358]->setStatsName("G_W0_t03pt");
			statsUpdate[359]->setStatsIValue(0);
			statsUpdate[359]->setStatsName("G_W1_t03pt");
			statsUpdate[360]->setStatsIValue(0);
			statsUpdate[360]->setStatsName("SSF_G_W0_t03pt");
			statsUpdate[361]->setStatsIValue(0);
			statsUpdate[361]->setStatsName("SSF_G_W1_t03pt");
			statsUpdate[362]->setStatsIValue(0);
			statsUpdate[362]->setStatsName("G_O0_t13pt");
			statsUpdate[363]->setStatsIValue(0);
			statsUpdate[363]->setStatsName("G_W0_t13pt");
			statsUpdate[364]->setStatsIValue(0);
			statsUpdate[364]->setStatsName("G_W1_t13pt");
			statsUpdate[365]->setStatsIValue(0);
			statsUpdate[365]->setStatsName("SSF_G_W0_t13pt");
			statsUpdate[366]->setStatsIValue(0);
			statsUpdate[366]->setStatsName("SSF_G_W1_t13pt");
			statsUpdate[367]->setStatsIValue(0);
			statsUpdate[367]->setStatsName("G_O0_ht");
			statsUpdate[368]->setStatsIValue(0);
			statsUpdate[368]->setStatsName("G_W0_ht");
			statsUpdate[369]->setStatsIValue(0);
			statsUpdate[369]->setStatsName("G_W1_ht");
			statsUpdate[370]->setStatsIValue(0);
			statsUpdate[370]->setStatsName("SSF_G_W0_ht");
			statsUpdate[371]->setStatsIValue(0);
			statsUpdate[371]->setStatsName("SSF_G_W1_ht");
			statsUpdate[372]->setStatsIValue(111139);
			statsUpdate[372]->setStatsName("G_O0_t0id");
			statsUpdate[373]->setStatsIValue(111139);
			statsUpdate[373]->setStatsName("G_W0_t0id");
			statsUpdate[374]->setStatsIValue(0);
			statsUpdate[374]->setStatsName("G_W1_t0id");
			statsUpdate[375]->setStatsIValue(0);
			statsUpdate[375]->setStatsName("SSF_G_W0_t0id");
			statsUpdate[376]->setStatsIValue(0);
			statsUpdate[376]->setStatsName("SSF_G_W1_t0id");
			statsUpdate[377]->setStatsIValue(111651);
			statsUpdate[377]->setStatsName("G_O0_t1id");
			statsUpdate[378]->setStatsIValue(111651);
			statsUpdate[378]->setStatsName("G_W0_t1id");
			statsUpdate[379]->setStatsIValue(0);
			statsUpdate[379]->setStatsName("G_W1_t1id");
			statsUpdate[380]->setStatsIValue(0);
			statsUpdate[380]->setStatsName("SSF_G_W0_t1id");
			statsUpdate[381]->setStatsIValue(0);
			statsUpdate[381]->setStatsName("SSF_G_W1_t1id");
			statsUpdate[382]->setStatsIValue(0);
			statsUpdate[382]->setStatsName("mbxm");
			statsUpdate[383]->setStatsIValue(1);
			statsUpdate[383]->setStatsName("ovgc");
			statsUpdate[384]->setStatsIValue(0);
			statsUpdate[384]->setStatsName("ssfovgc");
			statsUpdate[385]->setStatsIValue(0);
			statsUpdate[385]->setStatsName("tlgc");
			statsUpdate[386]->setStatsIValue(0);
			statsUpdate[386]->setStatsName("bxgc");

			for (int i = 0; i < 387; i++)
			{
				statUpdateList.push_back(statsUpdate[i]);
			}
		}

		if (!strcmp("gameType90", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
		{
			//Seen  443 events
			Blaze::FifaStats::StatsUpdate *statsUpdate[443];
			for (int i = 0; i < 443; i++)
			{
				statsUpdate[i] = entity->getStatsUpdates().pull_back();
				statsUpdate[i]->setOperator(Blaze::FifaStats::StatsOperator::OPERATOR_REPLACE);
				statsUpdate[i]->setStatsFValue(0.00000000);
				statsUpdate[i]->setType(Blaze::FifaStats::StatsType::TYPE_INT);
			}
			statsUpdate[0]->setStatsIValue(9);
			statsUpdate[0]->setStatsName("T_0_g");
			statsUpdate[1]->setStatsIValue(0);
			statsUpdate[1]->setStatsName("SSF_T_0_g");
			statsUpdate[2]->setStatsIValue(0);
			statsUpdate[2]->setStatsName("T_0_og");
			statsUpdate[3]->setStatsIValue(0);
			statsUpdate[3]->setStatsName("SSF_T_0_og");
			statsUpdate[4]->setStatsIValue(0);
			statsUpdate[4]->setStatsName("T_0_pkg");
			statsUpdate[5]->setStatsIValue(0);
			statsUpdate[5]->setStatsName("SSF_T_0_pkg");
			statsUpdate[6]->setStatsIValue(3);
			statsUpdate[6]->setStatsName("T_0_pos");
			statsUpdate[7]->setStatsIValue(0);
			statsUpdate[7]->setStatsName("SSF_T_0_pos");
			statsUpdate[8]->setStatsIValue(3);
			statsUpdate[8]->setStatsName("T_0_s");
			statsUpdate[9]->setStatsIValue(0);
			statsUpdate[9]->setStatsName("SSF_T_0_s");
			statsUpdate[10]->setStatsIValue(2);
			statsUpdate[10]->setStatsName("T_0_sog");
			statsUpdate[11]->setStatsIValue(0);
			statsUpdate[11]->setStatsName("SSF_T_0_sog");
			statsUpdate[12]->setStatsIValue(1);
			statsUpdate[12]->setStatsName("T_0_pa");
			statsUpdate[13]->setStatsIValue(0);
			statsUpdate[13]->setStatsName("SSF_T_0_pa");
			statsUpdate[14]->setStatsIValue(9);
			statsUpdate[14]->setStatsName("T_0_pm");
			statsUpdate[15]->setStatsIValue(0);
			statsUpdate[15]->setStatsName("SSF_T_0_pm");
			statsUpdate[16]->setStatsIValue(8);
			statsUpdate[16]->setStatsName("T_0_ta");
			statsUpdate[17]->setStatsIValue(0);
			statsUpdate[17]->setStatsName("SSF_T_0_ta");
			statsUpdate[18]->setStatsIValue(3);
			statsUpdate[18]->setStatsName("T_0_tm");
			statsUpdate[19]->setStatsIValue(0);
			statsUpdate[19]->setStatsName("SSF_T_0_tm");
			statsUpdate[20]->setStatsIValue(0);
			statsUpdate[20]->setStatsName("T_0_gc");
			statsUpdate[21]->setStatsIValue(0);
			statsUpdate[21]->setStatsName("SSF_T_0_gc");
			statsUpdate[22]->setStatsIValue(0);
			statsUpdate[22]->setStatsName("T_0_f");
			statsUpdate[23]->setStatsIValue(0);
			statsUpdate[23]->setStatsName("SSF_T_0_f");
			statsUpdate[24]->setStatsIValue(0);
			statsUpdate[24]->setStatsName("T_0_yc");
			statsUpdate[25]->setStatsIValue(0);
			statsUpdate[25]->setStatsName("SSF_T_0_yc");
			statsUpdate[26]->setStatsIValue(0);
			statsUpdate[26]->setStatsName("T_0_rc");
			statsUpdate[27]->setStatsIValue(0);
			statsUpdate[27]->setStatsName("SSF_T_0_rc");
			statsUpdate[28]->setStatsIValue(4);
			statsUpdate[28]->setStatsName("T_0_c");
			statsUpdate[29]->setStatsIValue(0);
			statsUpdate[29]->setStatsName("SSF_T_0_c");
			statsUpdate[30]->setStatsIValue(0);
			statsUpdate[30]->setStatsName("T_0_os");
			statsUpdate[31]->setStatsIValue(0);
			statsUpdate[31]->setStatsName("SSF_T_0_os");
			statsUpdate[32]->setStatsIValue(5);
			statsUpdate[32]->setStatsName("T_0_cs");
			statsUpdate[33]->setStatsIValue(0);
			statsUpdate[33]->setStatsName("SSF_T_0_cs");
			statsUpdate[34]->setStatsIValue(1);
			statsUpdate[34]->setStatsName("T_0_sa");
			statsUpdate[35]->setStatsIValue(0);
			statsUpdate[35]->setStatsName("SSF_T_0_sa");
			statsUpdate[36]->setStatsIValue(6);
			statsUpdate[36]->setStatsName("T_0_saog");
			statsUpdate[37]->setStatsIValue(0);
			statsUpdate[37]->setStatsName("SSF_T_0_saog");
			statsUpdate[38]->setStatsIValue(5);
			statsUpdate[38]->setStatsName("T_0_int");
			statsUpdate[39]->setStatsIValue(0);
			statsUpdate[39]->setStatsName("SSF_T_0_int");
			statsUpdate[40]->setStatsIValue(0);
			statsUpdate[40]->setStatsName("T_0_pka");
			statsUpdate[41]->setStatsIValue(0);
			statsUpdate[41]->setStatsName("SSF_T_0_pka");
			statsUpdate[42]->setStatsIValue(0);
			statsUpdate[42]->setStatsName("T_0_pks");
			statsUpdate[43]->setStatsIValue(0);
			statsUpdate[43]->setStatsName("SSF_T_0_pks");
			statsUpdate[44]->setStatsIValue(0);
			statsUpdate[44]->setStatsName("T_0_pkv");
			statsUpdate[45]->setStatsIValue(0);
			statsUpdate[45]->setStatsName("SSF_T_0_pkv");
			statsUpdate[46]->setStatsIValue(0);
			statsUpdate[46]->setStatsName("T_0_pkm");
			statsUpdate[47]->setStatsIValue(0);
			statsUpdate[47]->setStatsName("SSF_T_0_pkm");
			statsUpdate[48]->setStatsIValue(1);
			statsUpdate[48]->setStatsName("T_0_pi");
			statsUpdate[49]->setStatsIValue(0);
			statsUpdate[49]->setStatsName("SSF_T_0_pi");
			statsUpdate[50]->setStatsIValue(0);
			statsUpdate[50]->setStatsName("T_0_ir");
			statsUpdate[51]->setStatsIValue(0);
			statsUpdate[51]->setStatsName("SSF_T_0_ir");
			statsUpdate[52]->setStatsIValue(9);
			statsUpdate[52]->setStatsName("T_0_atklp");
			statsUpdate[53]->setStatsIValue(0);
			statsUpdate[53]->setStatsName("SSF_T_0_atklp");
			statsUpdate[54]->setStatsIValue(5);
			statsUpdate[54]->setStatsName("T_0_atkmp");
			statsUpdate[55]->setStatsIValue(0);
			statsUpdate[55]->setStatsName("SSF_T_0_atkmp");
			statsUpdate[56]->setStatsIValue(1);
			statsUpdate[56]->setStatsName("T_0_atkrp");
			statsUpdate[57]->setStatsIValue(0);
			statsUpdate[57]->setStatsName("SSF_T_0_atkrp");
			statsUpdate[58]->setStatsIValue(0);
			statsUpdate[58]->setStatsName("T_0_g15");
			statsUpdate[59]->setStatsIValue(0);
			statsUpdate[59]->setStatsName("SSF_T_0_g15");
			statsUpdate[60]->setStatsIValue(6);
			statsUpdate[60]->setStatsName("T_0_g30");
			statsUpdate[61]->setStatsIValue(0);
			statsUpdate[61]->setStatsName("SSF_T_0_g30");
			statsUpdate[62]->setStatsIValue(3);
			statsUpdate[62]->setStatsName("T_0_g45");
			statsUpdate[63]->setStatsIValue(0);
			statsUpdate[63]->setStatsName("SSF_T_0_g45");
			statsUpdate[64]->setStatsIValue(0);
			statsUpdate[64]->setStatsName("T_0_g60");
			statsUpdate[65]->setStatsIValue(0);
			statsUpdate[65]->setStatsName("SSF_T_0_g60");
			statsUpdate[66]->setStatsIValue(0);
			statsUpdate[66]->setStatsName("T_0_g75");
			statsUpdate[67]->setStatsIValue(0);
			statsUpdate[67]->setStatsName("SSF_T_0_g75");
			statsUpdate[68]->setStatsIValue(0);
			statsUpdate[68]->setStatsName("T_0_g90");
			statsUpdate[69]->setStatsIValue(0);
			statsUpdate[69]->setStatsName("SSF_T_0_g90");
			statsUpdate[70]->setStatsIValue(0);
			statsUpdate[70]->setStatsName("T_0_g90Plus");
			statsUpdate[71]->setStatsIValue(0);
			statsUpdate[71]->setStatsName("SSF_T_0_g90Plus");
			statsUpdate[72]->setStatsIValue(0);
			statsUpdate[72]->setStatsName("T_0_gob");
			statsUpdate[73]->setStatsIValue(0);
			statsUpdate[73]->setStatsName("SSF_T_0_gob");
			statsUpdate[74]->setStatsIValue(9);
			statsUpdate[74]->setStatsName("T_0_gib");
			statsUpdate[75]->setStatsIValue(0);
			statsUpdate[75]->setStatsName("SSF_T_0_gib");
			statsUpdate[76]->setStatsIValue(0);
			statsUpdate[76]->setStatsName("T_0_gfk");
			statsUpdate[77]->setStatsIValue(0);
			statsUpdate[77]->setStatsName("SSF_T_0_gfk");
			statsUpdate[78]->setStatsIValue(0);
			statsUpdate[78]->setStatsName("T_0_g2pt");
			statsUpdate[79]->setStatsIValue(0);
			statsUpdate[79]->setStatsName("SSF_T_0_g2pt");
			statsUpdate[80]->setStatsIValue(0);
			statsUpdate[80]->setStatsName("T_0_g3pt");
			statsUpdate[81]->setStatsIValue(0);
			statsUpdate[81]->setStatsName("SSF_T_0_g3pt");
			statsUpdate[82]->setStatsIValue(8);
			statsUpdate[82]->setStatsName("T_0_posl");
			statsUpdate[83]->setStatsIValue(0);
			statsUpdate[83]->setStatsName("SSF_T_0_posl");
			statsUpdate[84]->setStatsIValue(6);
			statsUpdate[84]->setStatsName("T_0_cdr");
			statsUpdate[85]->setStatsIValue(0);
			statsUpdate[85]->setStatsName("SSF_T_0_cdr");
			statsUpdate[86]->setStatsIValue(1);
			statsUpdate[86]->setStatsName("S_0_ow");
			statsUpdate[87]->setStatsIValue(4);
			statsUpdate[87]->setStatsName("S_0_od");
			statsUpdate[88]->setStatsIValue(0);
			statsUpdate[88]->setStatsName("S_0_ol");
			statsUpdate[89]->setStatsIValue(0);
			statsUpdate[89]->setStatsName("S_0_smw");
			statsUpdate[90]->setStatsIValue(4);
			statsUpdate[90]->setStatsName("S_0_smd");
			statsUpdate[91]->setStatsIValue(0);
			statsUpdate[91]->setStatsName("S_0_sml");
			statsUpdate[92]->setStatsIValue(0);
			statsUpdate[92]->setStatsName("S_0_cw");
			statsUpdate[93]->setStatsIValue(0);
			statsUpdate[93]->setStatsName("S_0_cd");
			statsUpdate[94]->setStatsIValue(0);
			statsUpdate[94]->setStatsName("S_0_cl");
			statsUpdate[95]->setStatsIValue(0);
			statsUpdate[95]->setStatsName("S_0_suw");
			statsUpdate[96]->setStatsIValue(0);
			statsUpdate[96]->setStatsName("S_0_sud");
			statsUpdate[97]->setStatsIValue(0);
			statsUpdate[97]->setStatsName("S_0_sul");
			statsUpdate[98]->setStatsIValue(0);
			statsUpdate[98]->setStatsName("S_0_baw");
			statsUpdate[99]->setStatsIValue(0);
			statsUpdate[99]->setStatsName("S_0_bad");
			statsUpdate[100]->setStatsIValue(0);
			statsUpdate[100]->setStatsName("S_0_bal");
			statsUpdate[101]->setStatsIValue(0);
			statsUpdate[101]->setStatsName("S_0_fxw");
			statsUpdate[102]->setStatsIValue(0);
			statsUpdate[102]->setStatsName("S_0_fxd");
			statsUpdate[103]->setStatsIValue(0);
			statsUpdate[103]->setStatsName("S_0_fxl");
			statsUpdate[104]->setStatsIValue(1);
			statsUpdate[104]->setStatsName("S_0_nrw");
			statsUpdate[105]->setStatsIValue(0);
			statsUpdate[105]->setStatsName("S_0_nrd");
			statsUpdate[106]->setStatsIValue(0);
			statsUpdate[106]->setStatsName("S_0_nrl");
			statsUpdate[107]->setStatsIValue(0);
			statsUpdate[107]->setStatsName("S_0_hvw");
			statsUpdate[108]->setStatsIValue(0);
			statsUpdate[108]->setStatsName("S_0_hvd");
			statsUpdate[109]->setStatsIValue(0);
			statsUpdate[109]->setStatsName("S_0_hvl");
			statsUpdate[110]->setStatsIValue(0);
			statsUpdate[110]->setStatsName("S_0_mbw");
			statsUpdate[111]->setStatsIValue(0);
			statsUpdate[111]->setStatsName("S_0_mbd");
			statsUpdate[112]->setStatsIValue(0);
			statsUpdate[112]->setStatsName("S_0_mbl");
			statsUpdate[113]->setStatsIValue(0);
			statsUpdate[113]->setStatsName("S_0_tww");
			statsUpdate[114]->setStatsIValue(0);
			statsUpdate[114]->setStatsName("S_0_twd");
			statsUpdate[115]->setStatsIValue(0);
			statsUpdate[115]->setStatsName("S_0_twl");
			statsUpdate[116]->setStatsIValue(0);
			statsUpdate[116]->setStatsName("S_0_tlw");
			statsUpdate[117]->setStatsIValue(0);
			statsUpdate[117]->setStatsName("S_0_tll");
			statsUpdate[118]->setStatsIValue(0);
			statsUpdate[118]->setStatsName("S_0_bxw");
			statsUpdate[119]->setStatsIValue(0);
			statsUpdate[119]->setStatsName("S_0_bxl");
			statsUpdate[120]->setStatsIValue(1);
			statsUpdate[120]->setStatsName("S_0_lws");
			statsUpdate[121]->setStatsIValue(0);
			statsUpdate[121]->setStatsName("S_0_cst");
			statsUpdate[122]->setStatsIValue(9);
			statsUpdate[122]->setStatsName("S_0_fgs");
			statsUpdate[123]->setStatsIValue(7);
			statsUpdate[123]->setStatsName("S_0_fgt");
			statsUpdate[124]->setStatsIValue(1);
			statsUpdate[124]->setStatsName("S_0_fgmt");
			statsUpdate[125]->setStatsIValue(3);
			statsUpdate[125]->setStatsName("S_0_fgot");
			statsUpdate[126]->setStatsIValue(0);
			statsUpdate[126]->setStatsName("S_0_ssfow");
			statsUpdate[127]->setStatsIValue(0);
			statsUpdate[127]->setStatsName("S_0_ssfod");
			statsUpdate[128]->setStatsIValue(0);
			statsUpdate[128]->setStatsName("S_0_ssfol");
			statsUpdate[129]->setStatsIValue(0);
			statsUpdate[129]->setStatsName("S_0_ssfsmw");
			statsUpdate[130]->setStatsIValue(0);
			statsUpdate[130]->setStatsName("S_0_ssfsmd");
			statsUpdate[131]->setStatsIValue(0);
			statsUpdate[131]->setStatsName("S_0_ssfsml");
			statsUpdate[132]->setStatsIValue(0);
			statsUpdate[132]->setStatsName("S_0_ssfsuw");
			statsUpdate[133]->setStatsIValue(0);
			statsUpdate[133]->setStatsName("S_0_ssfsud");
			statsUpdate[134]->setStatsIValue(0);
			statsUpdate[134]->setStatsName("S_0_ssfsul");
			statsUpdate[135]->setStatsIValue(0);
			statsUpdate[135]->setStatsName("S_0_ssffxw");
			statsUpdate[136]->setStatsIValue(0);
			statsUpdate[136]->setStatsName("S_0_ssffxd");
			statsUpdate[137]->setStatsIValue(0);
			statsUpdate[137]->setStatsName("S_0_ssffxl");
			statsUpdate[138]->setStatsIValue(0);
			statsUpdate[138]->setStatsName("S_0_ssfnrw");
			statsUpdate[139]->setStatsIValue(0);
			statsUpdate[139]->setStatsName("S_0_ssfnrd");
			statsUpdate[140]->setStatsIValue(0);
			statsUpdate[140]->setStatsName("S_0_ssfnrl");
			statsUpdate[141]->setStatsIValue(0);
			statsUpdate[141]->setStatsName("S_0_ssflws");
			statsUpdate[142]->setStatsIValue(0);
			statsUpdate[142]->setStatsName("S_0_ssfcst");
			statsUpdate[143]->setStatsIValue(0);
			statsUpdate[143]->setStatsName("S_0_ssffgs");
			statsUpdate[144]->setStatsIValue(0);
			statsUpdate[144]->setStatsName("S_0_ssffgt");
			statsUpdate[145]->setStatsIValue(0);
			statsUpdate[145]->setStatsName("S_0_ssffgmt");
			statsUpdate[146]->setStatsIValue(0);
			statsUpdate[146]->setStatsName("S_0_ssffgot");
			statsUpdate[147]->setStatsIValue(0);
			statsUpdate[147]->setStatsName("SSF_H_0_0");
			statsUpdate[148]->setStatsIValue(1);
			statsUpdate[148]->setStatsName("H_0_0_0");
			statsUpdate[149]->setStatsIValue(0);
			statsUpdate[149]->setStatsName("H_0_0_1");
			statsUpdate[150]->setStatsIValue(0);
			statsUpdate[150]->setStatsName("SSF_H_0_1");
			statsUpdate[151]->setStatsIValue(1);
			statsUpdate[151]->setStatsName("H_0_1_0");
			statsUpdate[152]->setStatsIValue(1);
			statsUpdate[152]->setStatsName("H_0_1_1");
			statsUpdate[153]->setStatsIValue(0);
			statsUpdate[153]->setStatsName("SSF_H_0_2");
			statsUpdate[154]->setStatsIValue(5);
			statsUpdate[154]->setStatsName("H_0_2_0");
			statsUpdate[155]->setStatsIValue(1);
			statsUpdate[155]->setStatsName("H_0_2_1");
			statsUpdate[156]->setStatsIValue(0);
			statsUpdate[156]->setStatsName("T_1_g");
			statsUpdate[157]->setStatsIValue(0);
			statsUpdate[157]->setStatsName("SSF_T_1_g");
			statsUpdate[158]->setStatsIValue(0);
			statsUpdate[158]->setStatsName("T_1_og");
			statsUpdate[159]->setStatsIValue(0);
			statsUpdate[159]->setStatsName("SSF_T_1_og");
			statsUpdate[160]->setStatsIValue(0);
			statsUpdate[160]->setStatsName("T_1_pkg");
			statsUpdate[161]->setStatsIValue(0);
			statsUpdate[161]->setStatsName("SSF_T_1_pkg");
			statsUpdate[162]->setStatsIValue(1);
			statsUpdate[162]->setStatsName("T_1_pos");
			statsUpdate[163]->setStatsIValue(0);
			statsUpdate[163]->setStatsName("SSF_T_1_pos");
			statsUpdate[164]->setStatsIValue(1);
			statsUpdate[164]->setStatsName("T_1_s");
			statsUpdate[165]->setStatsIValue(0);
			statsUpdate[165]->setStatsName("SSF_T_1_s");
			statsUpdate[166]->setStatsIValue(6);
			statsUpdate[166]->setStatsName("T_1_sog");
			statsUpdate[167]->setStatsIValue(0);
			statsUpdate[167]->setStatsName("SSF_T_1_sog");
			statsUpdate[168]->setStatsIValue(1);
			statsUpdate[168]->setStatsName("T_1_pa");
			statsUpdate[169]->setStatsIValue(0);
			statsUpdate[169]->setStatsName("SSF_T_1_pa");
			statsUpdate[170]->setStatsIValue(8);
			statsUpdate[170]->setStatsName("T_1_pm");
			statsUpdate[171]->setStatsIValue(0);
			statsUpdate[171]->setStatsName("SSF_T_1_pm");
			statsUpdate[172]->setStatsIValue(3);
			statsUpdate[172]->setStatsName("T_1_ta");
			statsUpdate[173]->setStatsIValue(0);
			statsUpdate[173]->setStatsName("SSF_T_1_ta");
			statsUpdate[174]->setStatsIValue(2);
			statsUpdate[174]->setStatsName("T_1_tm");
			statsUpdate[175]->setStatsIValue(0);
			statsUpdate[175]->setStatsName("SSF_T_1_tm");
			statsUpdate[176]->setStatsIValue(9);
			statsUpdate[176]->setStatsName("T_1_gc");
			statsUpdate[177]->setStatsIValue(0);
			statsUpdate[177]->setStatsName("SSF_T_1_gc");
			statsUpdate[178]->setStatsIValue(5);
			statsUpdate[178]->setStatsName("T_1_f");
			statsUpdate[179]->setStatsIValue(0);
			statsUpdate[179]->setStatsName("SSF_T_1_f");
			statsUpdate[180]->setStatsIValue(1);
			statsUpdate[180]->setStatsName("T_1_yc");
			statsUpdate[181]->setStatsIValue(0);
			statsUpdate[181]->setStatsName("SSF_T_1_yc");
			statsUpdate[182]->setStatsIValue(0);
			statsUpdate[182]->setStatsName("T_1_rc");
			statsUpdate[183]->setStatsIValue(0);
			statsUpdate[183]->setStatsName("SSF_T_1_rc");
			statsUpdate[184]->setStatsIValue(0);
			statsUpdate[184]->setStatsName("T_1_c");
			statsUpdate[185]->setStatsIValue(0);
			statsUpdate[185]->setStatsName("SSF_T_1_c");
			statsUpdate[186]->setStatsIValue(3);
			statsUpdate[186]->setStatsName("T_1_os");
			statsUpdate[187]->setStatsIValue(0);
			statsUpdate[187]->setStatsName("SSF_T_1_os");
			statsUpdate[188]->setStatsIValue(4);
			statsUpdate[188]->setStatsName("T_1_cs");
			statsUpdate[189]->setStatsIValue(0);
			statsUpdate[189]->setStatsName("SSF_T_1_cs");
			statsUpdate[190]->setStatsIValue(3);
			statsUpdate[190]->setStatsName("T_1_sa");
			statsUpdate[191]->setStatsIValue(0);
			statsUpdate[191]->setStatsName("SSF_T_1_sa");
			statsUpdate[192]->setStatsIValue(2);
			statsUpdate[192]->setStatsName("T_1_saog");
			statsUpdate[193]->setStatsIValue(0);
			statsUpdate[193]->setStatsName("SSF_T_1_saog");
			statsUpdate[194]->setStatsIValue(1);
			statsUpdate[194]->setStatsName("T_1_int");
			statsUpdate[195]->setStatsIValue(0);
			statsUpdate[195]->setStatsName("SSF_T_1_int");
			statsUpdate[196]->setStatsIValue(0);
			statsUpdate[196]->setStatsName("T_1_pka");
			statsUpdate[197]->setStatsIValue(0);
			statsUpdate[197]->setStatsName("SSF_T_1_pka");
			statsUpdate[198]->setStatsIValue(0);
			statsUpdate[198]->setStatsName("T_1_pks");
			statsUpdate[199]->setStatsIValue(0);
			statsUpdate[199]->setStatsName("SSF_T_1_pks");
			statsUpdate[200]->setStatsIValue(0);
			statsUpdate[200]->setStatsName("T_1_pkv");
			statsUpdate[201]->setStatsIValue(0);
			statsUpdate[201]->setStatsName("SSF_T_1_pkv");
			statsUpdate[202]->setStatsIValue(0);
			statsUpdate[202]->setStatsName("T_1_pkm");
			statsUpdate[203]->setStatsIValue(0);
			statsUpdate[203]->setStatsName("SSF_T_1_pkm");
			statsUpdate[204]->setStatsIValue(5);
			statsUpdate[204]->setStatsName("T_1_pi");
			statsUpdate[205]->setStatsIValue(0);
			statsUpdate[205]->setStatsName("SSF_T_1_pi");
			statsUpdate[206]->setStatsIValue(0);
			statsUpdate[206]->setStatsName("T_1_ir");
			statsUpdate[207]->setStatsIValue(0);
			statsUpdate[207]->setStatsName("SSF_T_1_ir");
			statsUpdate[208]->setStatsIValue(1);
			statsUpdate[208]->setStatsName("T_1_atklp");
			statsUpdate[209]->setStatsIValue(0);
			statsUpdate[209]->setStatsName("SSF_T_1_atklp");
			statsUpdate[210]->setStatsIValue(8);
			statsUpdate[210]->setStatsName("T_1_atkmp");
			statsUpdate[211]->setStatsIValue(0);
			statsUpdate[211]->setStatsName("SSF_T_1_atkmp");
			statsUpdate[212]->setStatsIValue(3);
			statsUpdate[212]->setStatsName("T_1_atkrp");
			statsUpdate[213]->setStatsIValue(0);
			statsUpdate[213]->setStatsName("SSF_T_1_atkrp");
			statsUpdate[214]->setStatsIValue(0);
			statsUpdate[214]->setStatsName("T_1_g15");
			statsUpdate[215]->setStatsIValue(0);
			statsUpdate[215]->setStatsName("SSF_T_1_g15");
			statsUpdate[216]->setStatsIValue(0);
			statsUpdate[216]->setStatsName("T_1_g30");
			statsUpdate[217]->setStatsIValue(0);
			statsUpdate[217]->setStatsName("SSF_T_1_g30");
			statsUpdate[218]->setStatsIValue(0);
			statsUpdate[218]->setStatsName("T_1_g45");
			statsUpdate[219]->setStatsIValue(0);
			statsUpdate[219]->setStatsName("SSF_T_1_g45");
			statsUpdate[220]->setStatsIValue(0);
			statsUpdate[220]->setStatsName("T_1_g60");
			statsUpdate[221]->setStatsIValue(0);
			statsUpdate[221]->setStatsName("SSF_T_1_g60");
			statsUpdate[222]->setStatsIValue(0);
			statsUpdate[222]->setStatsName("T_1_g75");
			statsUpdate[223]->setStatsIValue(0);
			statsUpdate[223]->setStatsName("SSF_T_1_g75");
			statsUpdate[224]->setStatsIValue(0);
			statsUpdate[224]->setStatsName("T_1_g90");
			statsUpdate[225]->setStatsIValue(0);
			statsUpdate[225]->setStatsName("SSF_T_1_g90");
			statsUpdate[226]->setStatsIValue(0);
			statsUpdate[226]->setStatsName("T_1_g90Plus");
			statsUpdate[227]->setStatsIValue(0);
			statsUpdate[227]->setStatsName("SSF_T_1_g90Plus");
			statsUpdate[228]->setStatsIValue(0);
			statsUpdate[228]->setStatsName("T_1_gob");
			statsUpdate[229]->setStatsIValue(0);
			statsUpdate[229]->setStatsName("SSF_T_1_gob");
			statsUpdate[230]->setStatsIValue(0);
			statsUpdate[230]->setStatsName("T_1_gib");
			statsUpdate[231]->setStatsIValue(0);
			statsUpdate[231]->setStatsName("SSF_T_1_gib");
			statsUpdate[232]->setStatsIValue(0);
			statsUpdate[232]->setStatsName("T_1_gfk");
			statsUpdate[233]->setStatsIValue(0);
			statsUpdate[233]->setStatsName("SSF_T_1_gfk");
			statsUpdate[234]->setStatsIValue(0);
			statsUpdate[234]->setStatsName("T_1_g2pt");
			statsUpdate[235]->setStatsIValue(0);
			statsUpdate[235]->setStatsName("SSF_T_1_g2pt");
			statsUpdate[236]->setStatsIValue(0);
			statsUpdate[236]->setStatsName("T_1_g3pt");
			statsUpdate[237]->setStatsIValue(0);
			statsUpdate[237]->setStatsName("SSF_T_1_g3pt");
			statsUpdate[238]->setStatsIValue(7);
			statsUpdate[238]->setStatsName("T_1_posl");
			statsUpdate[239]->setStatsIValue(0);
			statsUpdate[239]->setStatsName("SSF_T_1_posl");
			statsUpdate[240]->setStatsIValue(1);
			statsUpdate[240]->setStatsName("T_1_cdr");
			statsUpdate[241]->setStatsIValue(0);
			statsUpdate[241]->setStatsName("SSF_T_1_cdr");
			statsUpdate[242]->setStatsIValue(0);
			statsUpdate[242]->setStatsName("S_1_ow");
			statsUpdate[243]->setStatsIValue(4);
			statsUpdate[243]->setStatsName("S_1_od");
			statsUpdate[244]->setStatsIValue(1);
			statsUpdate[244]->setStatsName("S_1_ol");
			statsUpdate[245]->setStatsIValue(0);
			statsUpdate[245]->setStatsName("S_1_smw");
			statsUpdate[246]->setStatsIValue(4);
			statsUpdate[246]->setStatsName("S_1_smd");
			statsUpdate[247]->setStatsIValue(0);
			statsUpdate[247]->setStatsName("S_1_sml");
			statsUpdate[248]->setStatsIValue(0);
			statsUpdate[248]->setStatsName("S_1_cw");
			statsUpdate[249]->setStatsIValue(0);
			statsUpdate[249]->setStatsName("S_1_cd");
			statsUpdate[250]->setStatsIValue(0);
			statsUpdate[250]->setStatsName("S_1_cl");
			statsUpdate[251]->setStatsIValue(0);
			statsUpdate[251]->setStatsName("S_1_suw");
			statsUpdate[252]->setStatsIValue(0);
			statsUpdate[252]->setStatsName("S_1_sud");
			statsUpdate[253]->setStatsIValue(0);
			statsUpdate[253]->setStatsName("S_1_sul");
			statsUpdate[254]->setStatsIValue(0);
			statsUpdate[254]->setStatsName("S_1_baw");
			statsUpdate[255]->setStatsIValue(0);
			statsUpdate[255]->setStatsName("S_1_bad");
			statsUpdate[256]->setStatsIValue(0);
			statsUpdate[256]->setStatsName("S_1_bal");
			statsUpdate[257]->setStatsIValue(0);
			statsUpdate[257]->setStatsName("S_1_fxw");
			statsUpdate[258]->setStatsIValue(0);
			statsUpdate[258]->setStatsName("S_1_fxd");
			statsUpdate[259]->setStatsIValue(0);
			statsUpdate[259]->setStatsName("S_1_fxl");
			statsUpdate[260]->setStatsIValue(0);
			statsUpdate[260]->setStatsName("S_1_nrw");
			statsUpdate[261]->setStatsIValue(0);
			statsUpdate[261]->setStatsName("S_1_nrd");
			statsUpdate[262]->setStatsIValue(1);
			statsUpdate[262]->setStatsName("S_1_nrl");
			statsUpdate[263]->setStatsIValue(0);
			statsUpdate[263]->setStatsName("S_1_hvw");
			statsUpdate[264]->setStatsIValue(0);
			statsUpdate[264]->setStatsName("S_1_hvd");
			statsUpdate[265]->setStatsIValue(0);
			statsUpdate[265]->setStatsName("S_1_hvl");
			statsUpdate[266]->setStatsIValue(0);
			statsUpdate[266]->setStatsName("S_1_mbw");
			statsUpdate[267]->setStatsIValue(0);
			statsUpdate[267]->setStatsName("S_1_mbd");
			statsUpdate[268]->setStatsIValue(0);
			statsUpdate[268]->setStatsName("S_1_mbl");
			statsUpdate[269]->setStatsIValue(0);
			statsUpdate[269]->setStatsName("S_1_tww");
			statsUpdate[270]->setStatsIValue(0);
			statsUpdate[270]->setStatsName("S_1_twd");
			statsUpdate[271]->setStatsIValue(0);
			statsUpdate[271]->setStatsName("S_1_twl");
			statsUpdate[272]->setStatsIValue(0);
			statsUpdate[272]->setStatsName("S_1_tlw");
			statsUpdate[273]->setStatsIValue(0);
			statsUpdate[273]->setStatsName("S_1_tll");
			statsUpdate[274]->setStatsIValue(0);
			statsUpdate[274]->setStatsName("S_1_bxw");
			statsUpdate[275]->setStatsIValue(0);
			statsUpdate[275]->setStatsName("S_1_bxl");
			statsUpdate[276]->setStatsIValue(0);
			statsUpdate[276]->setStatsName("S_1_lws");
			statsUpdate[277]->setStatsIValue(0);
			statsUpdate[277]->setStatsName("S_1_cst");
			statsUpdate[278]->setStatsIValue(0);
			statsUpdate[278]->setStatsName("S_1_fgs");
			statsUpdate[279]->setStatsIValue(0);
			statsUpdate[279]->setStatsName("S_1_fgt");
			statsUpdate[280]->setStatsIValue(0);
			statsUpdate[280]->setStatsName("S_1_fgmt");
			statsUpdate[281]->setStatsIValue(0);
			statsUpdate[281]->setStatsName("S_1_fgot");
			statsUpdate[282]->setStatsIValue(0);
			statsUpdate[282]->setStatsName("S_1_ssfow");
			statsUpdate[283]->setStatsIValue(0);
			statsUpdate[283]->setStatsName("S_1_ssfod");
			statsUpdate[284]->setStatsIValue(0);
			statsUpdate[284]->setStatsName("S_1_ssfol");
			statsUpdate[285]->setStatsIValue(0);
			statsUpdate[285]->setStatsName("S_1_ssfsmw");
			statsUpdate[286]->setStatsIValue(0);
			statsUpdate[286]->setStatsName("S_1_ssfsmd");
			statsUpdate[287]->setStatsIValue(0);
			statsUpdate[287]->setStatsName("S_1_ssfsml");
			statsUpdate[288]->setStatsIValue(0);
			statsUpdate[288]->setStatsName("S_1_ssfsuw");
			statsUpdate[289]->setStatsIValue(0);
			statsUpdate[289]->setStatsName("S_1_ssfsud");
			statsUpdate[290]->setStatsIValue(0);
			statsUpdate[290]->setStatsName("S_1_ssfsul");
			statsUpdate[291]->setStatsIValue(0);
			statsUpdate[291]->setStatsName("S_1_ssffxw");
			statsUpdate[292]->setStatsIValue(0);
			statsUpdate[292]->setStatsName("S_1_ssffxd");
			statsUpdate[293]->setStatsIValue(0);
			statsUpdate[293]->setStatsName("S_1_ssffxl");
			statsUpdate[294]->setStatsIValue(0);
			statsUpdate[294]->setStatsName("S_1_ssfnrw");
			statsUpdate[295]->setStatsIValue(0);
			statsUpdate[295]->setStatsName("S_1_ssfnrd");
			statsUpdate[296]->setStatsIValue(0);
			statsUpdate[296]->setStatsName("S_1_ssfnrl");
			statsUpdate[297]->setStatsIValue(0);
			statsUpdate[297]->setStatsName("S_1_ssflws");
			statsUpdate[298]->setStatsIValue(0);
			statsUpdate[298]->setStatsName("S_1_ssfcst");
			statsUpdate[299]->setStatsIValue(0);
			statsUpdate[299]->setStatsName("S_1_ssffgs");
			statsUpdate[300]->setStatsIValue(0);
			statsUpdate[300]->setStatsName("S_1_ssffgt");
			statsUpdate[301]->setStatsIValue(0);
			statsUpdate[301]->setStatsName("S_1_ssffgmt");
			statsUpdate[302]->setStatsIValue(0);
			statsUpdate[302]->setStatsName("S_1_ssffgot");
			statsUpdate[303]->setStatsIValue(0);
			statsUpdate[303]->setStatsName("SSF_H_1_0");
			statsUpdate[304]->setStatsIValue(0);
			statsUpdate[304]->setStatsName("H_1_0_0");
			statsUpdate[305]->setStatsIValue(0);
			statsUpdate[305]->setStatsName("H_1_0_1");
			statsUpdate[306]->setStatsIValue(0);
			statsUpdate[306]->setStatsName("SSF_H_1_1");
			statsUpdate[307]->setStatsIValue(0);
			statsUpdate[307]->setStatsName("H_1_1_0");
			statsUpdate[308]->setStatsIValue(0);
			statsUpdate[308]->setStatsName("H_1_1_1");
			statsUpdate[309]->setStatsIValue(0);
			statsUpdate[309]->setStatsName("SSF_H_1_2");
			statsUpdate[310]->setStatsIValue(0);
			statsUpdate[310]->setStatsName("H_1_2_0");
			statsUpdate[311]->setStatsIValue(0);
			statsUpdate[311]->setStatsName("H_1_2_1");
			statsUpdate[312]->setStatsIValue(1);
			statsUpdate[312]->setStatsName("G_O0_gn");
			statsUpdate[313]->setStatsIValue(2);
			statsUpdate[313]->setStatsName("G_O1_gn");
			statsUpdate[314]->setStatsIValue(3);
			statsUpdate[314]->setStatsName("G_O2_gn");
			statsUpdate[315]->setStatsIValue(4);
			statsUpdate[315]->setStatsName("G_O3_gn");
			statsUpdate[316]->setStatsIValue(5);
			statsUpdate[316]->setStatsName("G_O4_gn");
			statsUpdate[317]->setStatsIValue(0);
			statsUpdate[317]->setStatsName("G_W0_gn");
			statsUpdate[318]->setStatsIValue(0);
			statsUpdate[318]->setStatsName("G_W1_gn");
			statsUpdate[319]->setStatsIValue(0);
			statsUpdate[319]->setStatsName("SSF_G_W0_gn");
			statsUpdate[320]->setStatsIValue(0);
			statsUpdate[320]->setStatsName("SSF_G_W1_gn");
			statsUpdate[321]->setStatsIValue(0);
			statsUpdate[321]->setStatsName("G_O0_gt");
			statsUpdate[322]->setStatsIValue(0);
			statsUpdate[322]->setStatsName("G_O1_gt");
			statsUpdate[323]->setStatsIValue(0);
			statsUpdate[323]->setStatsName("G_O2_gt");
			statsUpdate[324]->setStatsIValue(7);
			statsUpdate[324]->setStatsName("G_O3_gt");
			statsUpdate[325]->setStatsIValue(0);
			statsUpdate[325]->setStatsName("G_O4_gt");
			statsUpdate[326]->setStatsIValue(7);
			statsUpdate[326]->setStatsName("G_W0_gt");
			statsUpdate[327]->setStatsIValue(0);
			statsUpdate[327]->setStatsName("G_W1_gt");
			statsUpdate[328]->setStatsIValue(0);
			statsUpdate[328]->setStatsName("SSF_G_W0_gt");
			statsUpdate[329]->setStatsIValue(0);
			statsUpdate[329]->setStatsName("SSF_G_W1_gt");
			statsUpdate[330]->setStatsIValue(0);
			statsUpdate[330]->setStatsName("G_O0_gmd");
			statsUpdate[331]->setStatsIValue(0);
			statsUpdate[331]->setStatsName("G_O1_gmd");
			statsUpdate[332]->setStatsIValue(0);
			statsUpdate[332]->setStatsName("G_O2_gmd");
			statsUpdate[333]->setStatsIValue(0);
			statsUpdate[333]->setStatsName("G_O3_gmd");
			statsUpdate[334]->setStatsIValue(0);
			statsUpdate[334]->setStatsName("G_O4_gmd");
			statsUpdate[335]->setStatsIValue(0);
			statsUpdate[335]->setStatsName("G_W0_gmd");
			statsUpdate[336]->setStatsIValue(0);
			statsUpdate[336]->setStatsName("G_W1_gmd");
			statsUpdate[337]->setStatsIValue(0);
			statsUpdate[337]->setStatsName("SSF_G_W0_gmd");
			statsUpdate[338]->setStatsIValue(0);
			statsUpdate[338]->setStatsName("SSF_G_W1_gmd");
			statsUpdate[339]->setStatsIValue(0);
			statsUpdate[339]->setStatsName("G_O0_t0g");
			statsUpdate[340]->setStatsIValue(0);
			statsUpdate[340]->setStatsName("G_O1_t0g");
			statsUpdate[341]->setStatsIValue(0);
			statsUpdate[341]->setStatsName("G_O2_t0g");
			statsUpdate[342]->setStatsIValue(9);
			statsUpdate[342]->setStatsName("G_O3_t0g");
			statsUpdate[343]->setStatsIValue(0);
			statsUpdate[343]->setStatsName("G_O4_t0g");
			statsUpdate[344]->setStatsIValue(9);
			statsUpdate[344]->setStatsName("G_W0_t0g");
			statsUpdate[345]->setStatsIValue(0);
			statsUpdate[345]->setStatsName("G_W1_t0g");
			statsUpdate[346]->setStatsIValue(0);
			statsUpdate[346]->setStatsName("SSF_G_W0_t0g");
			statsUpdate[347]->setStatsIValue(0);
			statsUpdate[347]->setStatsName("SSF_G_W1_t0g");
			statsUpdate[348]->setStatsIValue(0);
			statsUpdate[348]->setStatsName("G_O0_t1g");
			statsUpdate[349]->setStatsIValue(0);
			statsUpdate[349]->setStatsName("G_O1_t1g");
			statsUpdate[350]->setStatsIValue(0);
			statsUpdate[350]->setStatsName("G_O2_t1g");
			statsUpdate[351]->setStatsIValue(0);
			statsUpdate[351]->setStatsName("G_O3_t1g");
			statsUpdate[352]->setStatsIValue(0);
			statsUpdate[352]->setStatsName("G_O4_t1g");
			statsUpdate[353]->setStatsIValue(0);
			statsUpdate[353]->setStatsName("G_W0_t1g");
			statsUpdate[354]->setStatsIValue(0);
			statsUpdate[354]->setStatsName("G_W1_t1g");
			statsUpdate[355]->setStatsIValue(0);
			statsUpdate[355]->setStatsName("SSF_G_W0_t1g");
			statsUpdate[356]->setStatsIValue(0);
			statsUpdate[356]->setStatsName("SSF_G_W1_t1g");
			statsUpdate[357]->setStatsIValue(0);
			statsUpdate[357]->setStatsName("G_O0_t0pk");
			statsUpdate[358]->setStatsIValue(0);
			statsUpdate[358]->setStatsName("G_O1_t0pk");
			statsUpdate[359]->setStatsIValue(0);
			statsUpdate[359]->setStatsName("G_O2_t0pk");
			statsUpdate[360]->setStatsIValue(0);
			statsUpdate[360]->setStatsName("G_O3_t0pk");
			statsUpdate[361]->setStatsIValue(0);
			statsUpdate[361]->setStatsName("G_O4_t0pk");
			statsUpdate[362]->setStatsIValue(0);
			statsUpdate[362]->setStatsName("G_W0_t0pk");
			statsUpdate[363]->setStatsIValue(0);
			statsUpdate[363]->setStatsName("G_W1_t0pk");
			statsUpdate[364]->setStatsIValue(0);
			statsUpdate[364]->setStatsName("SSF_G_W0_t0pk");
			statsUpdate[365]->setStatsIValue(0);
			statsUpdate[365]->setStatsName("SSF_G_W1_t0pk");
			statsUpdate[366]->setStatsIValue(0);
			statsUpdate[366]->setStatsName("G_O0_t1pk");
			statsUpdate[367]->setStatsIValue(0);
			statsUpdate[367]->setStatsName("G_O1_t1pk");
			statsUpdate[368]->setStatsIValue(0);
			statsUpdate[368]->setStatsName("G_O2_t1pk");
			statsUpdate[369]->setStatsIValue(0);
			statsUpdate[369]->setStatsName("G_O3_t1pk");
			statsUpdate[370]->setStatsIValue(0);
			statsUpdate[370]->setStatsName("G_O4_t1pk");
			statsUpdate[371]->setStatsIValue(0);
			statsUpdate[371]->setStatsName("G_W0_t1pk");
			statsUpdate[372]->setStatsIValue(0);
			statsUpdate[372]->setStatsName("G_W1_t1pk");
			statsUpdate[373]->setStatsIValue(0);
			statsUpdate[373]->setStatsName("SSF_G_W0_t1pk");
			statsUpdate[374]->setStatsIValue(0);
			statsUpdate[374]->setStatsName("SSF_G_W1_t1pk");
			statsUpdate[375]->setStatsIValue(0);
			statsUpdate[375]->setStatsName("G_O0_t02pt");
			statsUpdate[376]->setStatsIValue(0);
			statsUpdate[376]->setStatsName("G_O1_t02pt");
			statsUpdate[377]->setStatsIValue(0);
			statsUpdate[377]->setStatsName("G_O2_t02pt");
			statsUpdate[378]->setStatsIValue(0);
			statsUpdate[378]->setStatsName("G_O3_t02pt");
			statsUpdate[379]->setStatsIValue(0);
			statsUpdate[379]->setStatsName("G_O4_t02pt");
			statsUpdate[380]->setStatsIValue(0);
			statsUpdate[380]->setStatsName("G_W0_t02pt");
			statsUpdate[381]->setStatsIValue(0);
			statsUpdate[381]->setStatsName("G_W1_t02pt");
			statsUpdate[382]->setStatsIValue(0);
			statsUpdate[382]->setStatsName("SSF_G_W0_t02pt");
			statsUpdate[383]->setStatsIValue(0);
			statsUpdate[383]->setStatsName("SSF_G_W1_t02pt");
			statsUpdate[384]->setStatsIValue(0);
			statsUpdate[384]->setStatsName("G_O0_t12pt");
			statsUpdate[385]->setStatsIValue(0);
			statsUpdate[385]->setStatsName("G_O1_t12pt");
			statsUpdate[386]->setStatsIValue(0);
			statsUpdate[386]->setStatsName("G_O2_t12pt");
			statsUpdate[387]->setStatsIValue(0);
			statsUpdate[387]->setStatsName("G_O3_t12pt");
			statsUpdate[388]->setStatsIValue(0);
			statsUpdate[388]->setStatsName("G_O4_t12pt");
			statsUpdate[389]->setStatsIValue(0);
			statsUpdate[389]->setStatsName("G_W0_t12pt");
			statsUpdate[390]->setStatsIValue(0);
			statsUpdate[390]->setStatsName("G_W1_t12pt");
			statsUpdate[391]->setStatsIValue(0);
			statsUpdate[391]->setStatsName("SSF_G_W0_t12pt");
			statsUpdate[392]->setStatsIValue(0);
			statsUpdate[392]->setStatsName("SSF_G_W1_t12pt");
			statsUpdate[393]->setStatsIValue(0);
			statsUpdate[393]->setStatsName("G_O0_t03pt");
			statsUpdate[394]->setStatsIValue(0);
			statsUpdate[394]->setStatsName("G_O1_t03pt");
			statsUpdate[395]->setStatsIValue(0);
			statsUpdate[395]->setStatsName("G_O2_t03pt");
			statsUpdate[396]->setStatsIValue(0);
			statsUpdate[396]->setStatsName("G_O3_t03pt");
			statsUpdate[397]->setStatsIValue(0);
			statsUpdate[397]->setStatsName("G_O4_t03pt");
			statsUpdate[398]->setStatsIValue(0);
			statsUpdate[398]->setStatsName("G_W0_t03pt");
			statsUpdate[399]->setStatsIValue(0);
			statsUpdate[399]->setStatsName("G_W1_t03pt");
			statsUpdate[400]->setStatsIValue(0);
			statsUpdate[400]->setStatsName("SSF_G_W0_t03pt");
			statsUpdate[401]->setStatsIValue(0);
			statsUpdate[401]->setStatsName("SSF_G_W1_t03pt");
			statsUpdate[402]->setStatsIValue(0);
			statsUpdate[402]->setStatsName("G_O0_t13pt");
			statsUpdate[403]->setStatsIValue(0);
			statsUpdate[403]->setStatsName("G_O1_t13pt");
			statsUpdate[404]->setStatsIValue(0);
			statsUpdate[404]->setStatsName("G_O2_t13pt");
			statsUpdate[405]->setStatsIValue(0);
			statsUpdate[405]->setStatsName("G_O3_t13pt");
			statsUpdate[406]->setStatsIValue(0);
			statsUpdate[406]->setStatsName("G_O4_t13pt");
			statsUpdate[407]->setStatsIValue(0);
			statsUpdate[407]->setStatsName("G_W0_t13pt");
			statsUpdate[408]->setStatsIValue(0);
			statsUpdate[408]->setStatsName("G_W1_t13pt");
			statsUpdate[409]->setStatsIValue(0);
			statsUpdate[409]->setStatsName("SSF_G_W0_t13pt");
			statsUpdate[410]->setStatsIValue(0);
			statsUpdate[410]->setStatsName("SSF_G_W1_t13pt");
			statsUpdate[411]->setStatsIValue(0);
			statsUpdate[411]->setStatsName("G_O0_ht");
			statsUpdate[412]->setStatsIValue(0);
			statsUpdate[412]->setStatsName("G_O1_ht");
			statsUpdate[413]->setStatsIValue(0);
			statsUpdate[413]->setStatsName("G_O2_ht");
			statsUpdate[414]->setStatsIValue(0);
			statsUpdate[414]->setStatsName("G_O3_ht");
			statsUpdate[415]->setStatsIValue(0);
			statsUpdate[415]->setStatsName("G_O4_ht");
			statsUpdate[416]->setStatsIValue(0);
			statsUpdate[416]->setStatsName("G_W0_ht");
			statsUpdate[417]->setStatsIValue(0);
			statsUpdate[417]->setStatsName("G_W1_ht");
			statsUpdate[418]->setStatsIValue(0);
			statsUpdate[418]->setStatsName("SSF_G_W0_ht");
			statsUpdate[419]->setStatsIValue(0);
			statsUpdate[419]->setStatsName("SSF_G_W1_ht");
			statsUpdate[420]->setStatsIValue(1);
			statsUpdate[420]->setStatsName("G_O0_t0id");
			statsUpdate[421]->setStatsIValue(1);
			statsUpdate[421]->setStatsName("G_O1_t0id");
			statsUpdate[422]->setStatsIValue(1);
			statsUpdate[422]->setStatsName("G_O2_t0id");
			statsUpdate[423]->setStatsIValue(1);
			statsUpdate[423]->setStatsName("G_O3_t0id");
			statsUpdate[424]->setStatsIValue(1);
			statsUpdate[424]->setStatsName("G_O4_t0id");
			statsUpdate[425]->setStatsIValue(1);
			statsUpdate[425]->setStatsName("G_W0_t0id");
			statsUpdate[426]->setStatsIValue(0);
			statsUpdate[426]->setStatsName("G_W1_t0id");
			statsUpdate[427]->setStatsIValue(0);
			statsUpdate[427]->setStatsName("SSF_G_W0_t0id");
			statsUpdate[428]->setStatsIValue(0);
			statsUpdate[428]->setStatsName("SSF_G_W1_t0id");
			statsUpdate[429]->setStatsIValue(3);
			statsUpdate[429]->setStatsName("G_O0_t1id");
			statsUpdate[430]->setStatsIValue(3);
			statsUpdate[430]->setStatsName("G_O1_t1id");
			statsUpdate[431]->setStatsIValue(3);
			statsUpdate[431]->setStatsName("G_O2_t1id");
			statsUpdate[432]->setStatsIValue(3);
			statsUpdate[432]->setStatsName("G_O3_t1id");
			statsUpdate[433]->setStatsIValue(3);
			statsUpdate[433]->setStatsName("G_O4_t1id");
			statsUpdate[434]->setStatsIValue(3);
			statsUpdate[434]->setStatsName("G_W0_t1id");
			statsUpdate[435]->setStatsIValue(0);
			statsUpdate[435]->setStatsName("G_W1_t1id");
			statsUpdate[436]->setStatsIValue(0);
			statsUpdate[436]->setStatsName("SSF_G_W0_t1id");
			statsUpdate[437]->setStatsIValue(0);
			statsUpdate[437]->setStatsName("SSF_G_W1_t1id");
			statsUpdate[438]->setStatsIValue(0);
			statsUpdate[438]->setStatsName("mbxm");
			statsUpdate[439]->setStatsIValue(5);
			statsUpdate[439]->setStatsName("ovgc");
			statsUpdate[440]->setStatsIValue(0);
			statsUpdate[440]->setStatsName("ssfovgc");
			statsUpdate[441]->setStatsIValue(0);
			statsUpdate[441]->setStatsName("tlgc");
			statsUpdate[442]->setStatsIValue(0);
			statsUpdate[442]->setStatsName("bxgc");

			for (int i = 0; i < 443; i++)
			{
				statUpdateList.push_back(statsUpdate[i]);
			}
		}

		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "KickOffPlayer::updateStats_GroupID - before updateStats" << type);
		err = mPlayerInfo->getComponentProxy()->mFifaStatsProxy->updateStats(updateStatsRequest, updateStatsResponse);
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "KickOffPlayer::updateStats_GroupID - after updateStats"<< type);

		if (ERR_OK != err)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::fifastats, "KickOffPlayer::updateStats_GroupID:EADP_GROUPS_RIVAL_TYPE: playerId " << mPlayerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
		}
	}
	// Overall stats
	else if ((type == EADP_GROUPS_OVERALL_TYPE_A || type == EADP_GROUPS_OVERALL_TYPE_B) && mOverallGroupA_ID != "")
	{
		updateStatsRequest.setCategoryId("Overall");

		Blaze::FifaStats::Entity *entity1 = updateStatsRequest.getEntities().pull_back();
		Blaze::FifaStats::StatsUpdateList &statUpdateList1 = entity1->getStatsUpdates();

		Blaze::FifaStats::StatsUpdate *statsUpdate1[147];

		//set group ID A
		entity1->setEntityId(mOverallGroupA_ID.c_str());

		if (!strcmp("gameType100", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
		{
			//Seen 147 events
			for (int i = 0; i < 147; i++)
			{
				statsUpdate1[i] = entity1->getStatsUpdates().pull_back();
				statsUpdate1[i]->setOperator(Blaze::FifaStats::StatsOperator::OPERATOR_REPLACE);
				statsUpdate1[i]->setStatsFValue(0.00000000);
				statsUpdate1[i]->setType(Blaze::FifaStats::StatsType::TYPE_INT);
			}
			statsUpdate1[0]->setStatsIValue(5);
			statsUpdate1[0]->setStatsName("T_0_g");
			statsUpdate1[1]->setStatsIValue(0);
			statsUpdate1[1]->setStatsName("SSF_T_0_g");
			statsUpdate1[2]->setStatsIValue(0);
			statsUpdate1[2]->setStatsName("T_0_og");
			statsUpdate1[3]->setStatsIValue(0);
			statsUpdate1[3]->setStatsName("SSF_T_0_og");
			statsUpdate1[4]->setStatsIValue(0);
			statsUpdate1[4]->setStatsName("T_0_pkg");
			statsUpdate1[5]->setStatsIValue(0);
			statsUpdate1[5]->setStatsName("SSF_T_0_pkg");
			statsUpdate1[6]->setStatsIValue(55);
			statsUpdate1[6]->setStatsName("T_0_pos");
			statsUpdate1[7]->setStatsIValue(0);
			statsUpdate1[7]->setStatsName("SSF_T_0_pos");
			statsUpdate1[8]->setStatsIValue(14);
			statsUpdate1[8]->setStatsName("T_0_s");
			statsUpdate1[9]->setStatsIValue(0);
			statsUpdate1[9]->setStatsName("SSF_T_0_s");
			statsUpdate1[10]->setStatsIValue(9);
			statsUpdate1[10]->setStatsName("T_0_sog");
			statsUpdate1[11]->setStatsIValue(0);
			statsUpdate1[11]->setStatsName("SSF_T_0_sog");
			statsUpdate1[12]->setStatsIValue(74);
			statsUpdate1[12]->setStatsName("T_0_pa");
			statsUpdate1[13]->setStatsIValue(0);
			statsUpdate1[13]->setStatsName("SSF_T_0_pa");
			statsUpdate1[14]->setStatsIValue(60);
			statsUpdate1[14]->setStatsName("T_0_pm");
			statsUpdate1[15]->setStatsIValue(0);
			statsUpdate1[15]->setStatsName("SSF_T_0_pm");
			statsUpdate1[16]->setStatsIValue(9);
			statsUpdate1[16]->setStatsName("T_0_ta");
			statsUpdate1[17]->setStatsIValue(0);
			statsUpdate1[17]->setStatsName("SSF_T_0_ta");
			statsUpdate1[18]->setStatsIValue(1);
			statsUpdate1[18]->setStatsName("T_0_tm");
			statsUpdate1[19]->setStatsIValue(0);
			statsUpdate1[19]->setStatsName("SSF_T_0_tm");
			statsUpdate1[20]->setStatsIValue(0);
			statsUpdate1[20]->setStatsName("T_0_gc");
			statsUpdate1[21]->setStatsIValue(0);
			statsUpdate1[21]->setStatsName("SSF_T_0_gc");
			statsUpdate1[22]->setStatsIValue(0);
			statsUpdate1[22]->setStatsName("T_0_f");
			statsUpdate1[23]->setStatsIValue(0);
			statsUpdate1[23]->setStatsName("SSF_T_0_f");
			statsUpdate1[24]->setStatsIValue(0);
			statsUpdate1[24]->setStatsName("T_0_yc");
			statsUpdate1[25]->setStatsIValue(0);
			statsUpdate1[25]->setStatsName("SSF_T_0_yc");
			statsUpdate1[26]->setStatsIValue(0);
			statsUpdate1[26]->setStatsName("T_0_rc");
			statsUpdate1[27]->setStatsIValue(0);
			statsUpdate1[27]->setStatsName("SSF_T_0_rc");
			statsUpdate1[28]->setStatsIValue(1);
			statsUpdate1[28]->setStatsName("T_0_c");
			statsUpdate1[29]->setStatsIValue(0);
			statsUpdate1[29]->setStatsName("SSF_T_0_c");
			statsUpdate1[30]->setStatsIValue(1);
			statsUpdate1[30]->setStatsName("T_0_os");
			statsUpdate1[31]->setStatsIValue(0);
			statsUpdate1[31]->setStatsName("SSF_T_0_os");
			statsUpdate1[32]->setStatsIValue(1);
			statsUpdate1[32]->setStatsName("T_0_cs");
			statsUpdate1[33]->setStatsIValue(0);
			statsUpdate1[33]->setStatsName("SSF_T_0_cs");
			statsUpdate1[34]->setStatsIValue(3);
			statsUpdate1[34]->setStatsName("T_0_sa");
			statsUpdate1[35]->setStatsIValue(0);
			statsUpdate1[35]->setStatsName("SSF_T_0_sa");
			statsUpdate1[36]->setStatsIValue(2);
			statsUpdate1[36]->setStatsName("T_0_saog");
			statsUpdate1[37]->setStatsIValue(0);
			statsUpdate1[37]->setStatsName("SSF_T_0_saog");
			statsUpdate1[38]->setStatsIValue(19);
			statsUpdate1[38]->setStatsName("T_0_int");
			statsUpdate1[39]->setStatsIValue(0);
			statsUpdate1[39]->setStatsName("SSF_T_0_int");
			statsUpdate1[40]->setStatsIValue(0);
			statsUpdate1[40]->setStatsName("T_0_pka");
			statsUpdate1[41]->setStatsIValue(0);
			statsUpdate1[41]->setStatsName("SSF_T_0_pka");
			statsUpdate1[42]->setStatsIValue(0);
			statsUpdate1[42]->setStatsName("T_0_pks");
			statsUpdate1[43]->setStatsIValue(0);
			statsUpdate1[43]->setStatsName("SSF_T_0_pks");
			statsUpdate1[44]->setStatsIValue(0);
			statsUpdate1[44]->setStatsName("T_0_pkv");
			statsUpdate1[45]->setStatsIValue(0);
			statsUpdate1[45]->setStatsName("SSF_T_0_pkv");
			statsUpdate1[46]->setStatsIValue(0);
			statsUpdate1[46]->setStatsName("T_0_pkm");
			statsUpdate1[47]->setStatsIValue(0);
			statsUpdate1[47]->setStatsName("SSF_T_0_pkm");
			statsUpdate1[48]->setStatsIValue(14);
			statsUpdate1[48]->setStatsName("T_0_pi");
			statsUpdate1[49]->setStatsIValue(0);
			statsUpdate1[49]->setStatsName("SSF_T_0_pi");
			statsUpdate1[50]->setStatsIValue(0);
			statsUpdate1[50]->setStatsName("T_0_ir");
			statsUpdate1[51]->setStatsIValue(0);
			statsUpdate1[51]->setStatsName("SSF_T_0_ir");
			statsUpdate1[52]->setStatsIValue(27);
			statsUpdate1[52]->setStatsName("T_0_atklp");
			statsUpdate1[53]->setStatsIValue(0);
			statsUpdate1[53]->setStatsName("SSF_T_0_atklp");
			statsUpdate1[54]->setStatsIValue(47);
			statsUpdate1[54]->setStatsName("T_0_atkmp");
			statsUpdate1[55]->setStatsIValue(0);
			statsUpdate1[55]->setStatsName("SSF_T_0_atkmp");
			statsUpdate1[56]->setStatsIValue(26);
			statsUpdate1[56]->setStatsName("T_0_atkrp");
			statsUpdate1[57]->setStatsIValue(0);
			statsUpdate1[57]->setStatsName("SSF_T_0_atkrp");
			statsUpdate1[58]->setStatsIValue(1);
			statsUpdate1[58]->setStatsName("T_0_g15");
			statsUpdate1[59]->setStatsIValue(0);
			statsUpdate1[59]->setStatsName("SSF_T_0_g15");
			statsUpdate1[60]->setStatsIValue(1);
			statsUpdate1[60]->setStatsName("T_0_g30");
			statsUpdate1[61]->setStatsIValue(0);
			statsUpdate1[61]->setStatsName("SSF_T_0_g30");
			statsUpdate1[62]->setStatsIValue(1);
			statsUpdate1[62]->setStatsName("T_0_g45");
			statsUpdate1[63]->setStatsIValue(0);
			statsUpdate1[63]->setStatsName("SSF_T_0_g45");
			statsUpdate1[64]->setStatsIValue(1);
			statsUpdate1[64]->setStatsName("T_0_g60");
			statsUpdate1[65]->setStatsIValue(0);
			statsUpdate1[65]->setStatsName("SSF_T_0_g60");
			statsUpdate1[66]->setStatsIValue(0);
			statsUpdate1[66]->setStatsName("T_0_g75");
			statsUpdate1[67]->setStatsIValue(0);
			statsUpdate1[67]->setStatsName("SSF_T_0_g75");
			statsUpdate1[68]->setStatsIValue(1);
			statsUpdate1[68]->setStatsName("T_0_g90");
			statsUpdate1[69]->setStatsIValue(0);
			statsUpdate1[69]->setStatsName("SSF_T_0_g90");
			statsUpdate1[70]->setStatsIValue(0);
			statsUpdate1[70]->setStatsName("T_0_g90Plus");
			statsUpdate1[71]->setStatsIValue(0);
			statsUpdate1[71]->setStatsName("SSF_T_0_g90Plus");
			statsUpdate1[72]->setStatsIValue(0);
			statsUpdate1[72]->setStatsName("T_0_gob");
			statsUpdate1[73]->setStatsIValue(0);
			statsUpdate1[73]->setStatsName("SSF_T_0_gob");
			statsUpdate1[74]->setStatsIValue(5);
			statsUpdate1[74]->setStatsName("T_0_gib");
			statsUpdate1[75]->setStatsIValue(0);
			statsUpdate1[75]->setStatsName("SSF_T_0_gib");
			statsUpdate1[76]->setStatsIValue(0);
			statsUpdate1[76]->setStatsName("T_0_gfk");
			statsUpdate1[77]->setStatsIValue(0);
			statsUpdate1[77]->setStatsName("SSF_T_0_gfk");
			statsUpdate1[78]->setStatsIValue(0);
			statsUpdate1[78]->setStatsName("T_0_g2pt");
			statsUpdate1[79]->setStatsIValue(0);
			statsUpdate1[79]->setStatsName("SSF_T_0_g2pt");
			statsUpdate1[80]->setStatsIValue(0);
			statsUpdate1[80]->setStatsName("T_0_g3pt");
			statsUpdate1[81]->setStatsIValue(0);
			statsUpdate1[81]->setStatsName("SSF_T_0_g3pt");
			statsUpdate1[82]->setStatsIValue(24);
			statsUpdate1[82]->setStatsName("T_0_posl");
			statsUpdate1[83]->setStatsIValue(0);
			statsUpdate1[83]->setStatsName("SSF_T_0_posl");
			statsUpdate1[84]->setStatsIValue(50);
			statsUpdate1[84]->setStatsName("T_0_cdr");
			statsUpdate1[85]->setStatsIValue(0);
			statsUpdate1[85]->setStatsName("SSF_T_0_cdr");
			statsUpdate1[86]->setStatsIValue(1);
			statsUpdate1[86]->setStatsName("S_0_ow");
			statsUpdate1[87]->setStatsIValue(0);
			statsUpdate1[87]->setStatsName("S_0_od");
			statsUpdate1[88]->setStatsIValue(0);
			statsUpdate1[88]->setStatsName("S_0_ol");
			statsUpdate1[89]->setStatsIValue(1);
			statsUpdate1[89]->setStatsName("S_0_smw");
			statsUpdate1[90]->setStatsIValue(0);
			statsUpdate1[90]->setStatsName("S_0_smd");
			statsUpdate1[91]->setStatsIValue(0);
			statsUpdate1[91]->setStatsName("S_0_sml");
			statsUpdate1[92]->setStatsIValue(0);
			statsUpdate1[92]->setStatsName("S_0_cw");
			statsUpdate1[93]->setStatsIValue(0);
			statsUpdate1[93]->setStatsName("S_0_cd");
			statsUpdate1[94]->setStatsIValue(0);
			statsUpdate1[94]->setStatsName("S_0_cl");
			statsUpdate1[95]->setStatsIValue(0);
			statsUpdate1[95]->setStatsName("S_0_suw");
			statsUpdate1[96]->setStatsIValue(0);
			statsUpdate1[96]->setStatsName("S_0_sud");
			statsUpdate1[97]->setStatsIValue(0);
			statsUpdate1[97]->setStatsName("S_0_sul");
			statsUpdate1[98]->setStatsIValue(0);
			statsUpdate1[98]->setStatsName("S_0_baw");
			statsUpdate1[99]->setStatsIValue(0);
			statsUpdate1[99]->setStatsName("S_0_bad");
			statsUpdate1[100]->setStatsIValue(0);
			statsUpdate1[100]->setStatsName("S_0_bal");
			statsUpdate1[101]->setStatsIValue(0);
			statsUpdate1[101]->setStatsName("S_0_fxw");
			statsUpdate1[102]->setStatsIValue(0);
			statsUpdate1[102]->setStatsName("S_0_fxd");
			statsUpdate1[103]->setStatsIValue(0);
			statsUpdate1[103]->setStatsName("S_0_fxl");
			statsUpdate1[104]->setStatsIValue(0);
			statsUpdate1[104]->setStatsName("S_0_nrw");
			statsUpdate1[105]->setStatsIValue(0);
			statsUpdate1[105]->setStatsName("S_0_nrd");
			statsUpdate1[106]->setStatsIValue(0);
			statsUpdate1[106]->setStatsName("S_0_nrl");
			statsUpdate1[107]->setStatsIValue(0);
			statsUpdate1[107]->setStatsName("S_0_hvw");
			statsUpdate1[108]->setStatsIValue(0);
			statsUpdate1[108]->setStatsName("S_0_hvd");
			statsUpdate1[109]->setStatsIValue(0);
			statsUpdate1[109]->setStatsName("S_0_hvl");
			statsUpdate1[110]->setStatsIValue(0);
			statsUpdate1[110]->setStatsName("S_0_mbw");
			statsUpdate1[111]->setStatsIValue(0);
			statsUpdate1[111]->setStatsName("S_0_mbd");
			statsUpdate1[112]->setStatsIValue(0);
			statsUpdate1[112]->setStatsName("S_0_mbl");
			statsUpdate1[113]->setStatsIValue(0);
			statsUpdate1[113]->setStatsName("S_0_tww");
			statsUpdate1[114]->setStatsIValue(0);
			statsUpdate1[114]->setStatsName("S_0_twd");
			statsUpdate1[115]->setStatsIValue(0);
			statsUpdate1[115]->setStatsName("S_0_twl");
			statsUpdate1[116]->setStatsIValue(0);
			statsUpdate1[116]->setStatsName("S_0_tlw");
			statsUpdate1[117]->setStatsIValue(0);
			statsUpdate1[117]->setStatsName("S_0_tll");
			statsUpdate1[118]->setStatsIValue(0);
			statsUpdate1[118]->setStatsName("S_0_bxw");
			statsUpdate1[119]->setStatsIValue(0);
			statsUpdate1[119]->setStatsName("S_0_bxl");
			statsUpdate1[120]->setStatsIValue(1);
			statsUpdate1[120]->setStatsName("S_0_lws");
			statsUpdate1[121]->setStatsIValue(1);
			statsUpdate1[121]->setStatsName("S_0_cst");
			statsUpdate1[122]->setStatsIValue(780);
			statsUpdate1[122]->setStatsName("S_0_fgs");
			statsUpdate1[123]->setStatsIValue(0);
			statsUpdate1[123]->setStatsName("S_0_fgt");
			statsUpdate1[124]->setStatsIValue(111139);
			statsUpdate1[124]->setStatsName("S_0_fgmt");
			statsUpdate1[125]->setStatsIValue(111651);
			statsUpdate1[125]->setStatsName("S_0_fgot");
			statsUpdate1[126]->setStatsIValue(0);
			statsUpdate1[126]->setStatsName("S_0_ssfow");
			statsUpdate1[127]->setStatsIValue(0);
			statsUpdate1[127]->setStatsName("S_0_ssfod");
			statsUpdate1[128]->setStatsIValue(0);
			statsUpdate1[128]->setStatsName("S_0_ssfol");
			statsUpdate1[129]->setStatsIValue(0);
			statsUpdate1[129]->setStatsName("S_0_ssfsmw");
			statsUpdate1[130]->setStatsIValue(0);
			statsUpdate1[130]->setStatsName("S_0_ssfsmd");
			statsUpdate1[131]->setStatsIValue(0);
			statsUpdate1[131]->setStatsName("S_0_ssfsml");
			statsUpdate1[132]->setStatsIValue(0);
			statsUpdate1[132]->setStatsName("S_0_ssfsuw");
			statsUpdate1[133]->setStatsIValue(0);
			statsUpdate1[133]->setStatsName("S_0_ssfsud");
			statsUpdate1[134]->setStatsIValue(0);
			statsUpdate1[134]->setStatsName("S_0_ssfsul");
			statsUpdate1[135]->setStatsIValue(0);
			statsUpdate1[135]->setStatsName("S_0_ssffxw");
			statsUpdate1[136]->setStatsIValue(0);
			statsUpdate1[136]->setStatsName("S_0_ssffxd");
			statsUpdate1[137]->setStatsIValue(0);
			statsUpdate1[137]->setStatsName("S_0_ssffxl");
			statsUpdate1[138]->setStatsIValue(0);
			statsUpdate1[138]->setStatsName("S_0_ssfnrw");
			statsUpdate1[139]->setStatsIValue(0);
			statsUpdate1[139]->setStatsName("S_0_ssfnrd");
			statsUpdate1[140]->setStatsIValue(0);
			statsUpdate1[140]->setStatsName("S_0_ssfnrl");
			statsUpdate1[141]->setStatsIValue(0);
			statsUpdate1[141]->setStatsName("S_0_ssflws");
			statsUpdate1[142]->setStatsIValue(0);
			statsUpdate1[142]->setStatsName("S_0_ssfcst");
			statsUpdate1[143]->setStatsIValue(0);
			statsUpdate1[143]->setStatsName("S_0_ssffgs");
			statsUpdate1[144]->setStatsIValue(0);
			statsUpdate1[144]->setStatsName("S_0_ssffgt");
			statsUpdate1[145]->setStatsIValue(0);
			statsUpdate1[145]->setStatsName("S_0_ssffgmt");
			statsUpdate1[146]->setStatsIValue(0);
			statsUpdate1[146]->setStatsName("S_0_ssffgot");

			for (int i = 0; i < 147; i++)
			{
				statUpdateList1.push_back(statsUpdate1[i]);
			}
		}
		if (!strcmp("gameType90", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
		{
			// Seen 147 events
			for (int i = 0; i < 147; i++)
			{
				statsUpdate1[i] = entity1->getStatsUpdates().pull_back();
				statsUpdate1[i]->setOperator(Blaze::FifaStats::StatsOperator::OPERATOR_REPLACE);
				statsUpdate1[i]->setStatsFValue(0.00000000);
				statsUpdate1[i]->setType(Blaze::FifaStats::StatsType::TYPE_INT);
			}
			statsUpdate1[0]->setStatsIValue(9);
			statsUpdate1[0]->setStatsName("T_0_g");
			statsUpdate1[1]->setStatsIValue(0);
			statsUpdate1[1]->setStatsName("SSF_T_0_g");
			statsUpdate1[2]->setStatsIValue(0);
			statsUpdate1[2]->setStatsName("T_0_og");
			statsUpdate1[3]->setStatsIValue(0);
			statsUpdate1[3]->setStatsName("SSF_T_0_og");
			statsUpdate1[4]->setStatsIValue(0);
			statsUpdate1[4]->setStatsName("T_0_pkg");
			statsUpdate1[5]->setStatsIValue(0);
			statsUpdate1[5]->setStatsName("SSF_T_0_pkg");
			statsUpdate1[6]->setStatsIValue(3);
			statsUpdate1[6]->setStatsName("T_0_pos");
			statsUpdate1[7]->setStatsIValue(0);
			statsUpdate1[7]->setStatsName("SSF_T_0_pos");
			statsUpdate1[8]->setStatsIValue(3);
			statsUpdate1[8]->setStatsName("T_0_s");
			statsUpdate1[9]->setStatsIValue(0);
			statsUpdate1[9]->setStatsName("SSF_T_0_s");
			statsUpdate1[10]->setStatsIValue(2);
			statsUpdate1[10]->setStatsName("T_0_sog");
			statsUpdate1[11]->setStatsIValue(0);
			statsUpdate1[11]->setStatsName("SSF_T_0_sog");
			statsUpdate1[12]->setStatsIValue(1);
			statsUpdate1[12]->setStatsName("T_0_pa");
			statsUpdate1[13]->setStatsIValue(0);
			statsUpdate1[13]->setStatsName("SSF_T_0_pa");
			statsUpdate1[14]->setStatsIValue(9);
			statsUpdate1[14]->setStatsName("T_0_pm");
			statsUpdate1[15]->setStatsIValue(0);
			statsUpdate1[15]->setStatsName("SSF_T_0_pm");
			statsUpdate1[16]->setStatsIValue(8);
			statsUpdate1[16]->setStatsName("T_0_ta");
			statsUpdate1[17]->setStatsIValue(0);
			statsUpdate1[17]->setStatsName("SSF_T_0_ta");
			statsUpdate1[18]->setStatsIValue(3);
			statsUpdate1[18]->setStatsName("T_0_tm");
			statsUpdate1[19]->setStatsIValue(0);
			statsUpdate1[19]->setStatsName("SSF_T_0_tm");
			statsUpdate1[20]->setStatsIValue(0);
			statsUpdate1[20]->setStatsName("T_0_gc");
			statsUpdate1[21]->setStatsIValue(0);
			statsUpdate1[21]->setStatsName("SSF_T_0_gc");
			statsUpdate1[22]->setStatsIValue(0);
			statsUpdate1[22]->setStatsName("T_0_f");
			statsUpdate1[23]->setStatsIValue(0);
			statsUpdate1[23]->setStatsName("SSF_T_0_f");
			statsUpdate1[24]->setStatsIValue(0);
			statsUpdate1[24]->setStatsName("T_0_yc");
			statsUpdate1[25]->setStatsIValue(0);
			statsUpdate1[25]->setStatsName("SSF_T_0_yc");
			statsUpdate1[26]->setStatsIValue(0);
			statsUpdate1[26]->setStatsName("T_0_rc");
			statsUpdate1[27]->setStatsIValue(0);
			statsUpdate1[27]->setStatsName("SSF_T_0_rc");
			statsUpdate1[28]->setStatsIValue(4);
			statsUpdate1[28]->setStatsName("T_0_c");
			statsUpdate1[29]->setStatsIValue(0);
			statsUpdate1[29]->setStatsName("SSF_T_0_c");
			statsUpdate1[30]->setStatsIValue(0);
			statsUpdate1[30]->setStatsName("T_0_os");
			statsUpdate1[31]->setStatsIValue(0);
			statsUpdate1[31]->setStatsName("SSF_T_0_os");
			statsUpdate1[32]->setStatsIValue(5);
			statsUpdate1[32]->setStatsName("T_0_cs");
			statsUpdate1[33]->setStatsIValue(0);
			statsUpdate1[33]->setStatsName("SSF_T_0_cs");
			statsUpdate1[34]->setStatsIValue(1);
			statsUpdate1[34]->setStatsName("T_0_sa");
			statsUpdate1[35]->setStatsIValue(0);
			statsUpdate1[35]->setStatsName("SSF_T_0_sa");
			statsUpdate1[36]->setStatsIValue(6);
			statsUpdate1[36]->setStatsName("T_0_saog");
			statsUpdate1[37]->setStatsIValue(0);
			statsUpdate1[37]->setStatsName("SSF_T_0_saog");
			statsUpdate1[38]->setStatsIValue(5);
			statsUpdate1[38]->setStatsName("T_0_int");
			statsUpdate1[39]->setStatsIValue(0);
			statsUpdate1[39]->setStatsName("SSF_T_0_int");
			statsUpdate1[40]->setStatsIValue(0);
			statsUpdate1[40]->setStatsName("T_0_pka");
			statsUpdate1[41]->setStatsIValue(0);
			statsUpdate1[41]->setStatsName("SSF_T_0_pka");
			statsUpdate1[42]->setStatsIValue(0);
			statsUpdate1[42]->setStatsName("T_0_pks");
			statsUpdate1[43]->setStatsIValue(0);
			statsUpdate1[43]->setStatsName("SSF_T_0_pks");
			statsUpdate1[44]->setStatsIValue(0);
			statsUpdate1[44]->setStatsName("T_0_pkv");
			statsUpdate1[45]->setStatsIValue(0);
			statsUpdate1[45]->setStatsName("SSF_T_0_pkv");
			statsUpdate1[46]->setStatsIValue(0);
			statsUpdate1[46]->setStatsName("T_0_pkm");
			statsUpdate1[47]->setStatsIValue(0);
			statsUpdate1[47]->setStatsName("SSF_T_0_pkm");
			statsUpdate1[48]->setStatsIValue(1);
			statsUpdate1[48]->setStatsName("T_0_pi");
			statsUpdate1[49]->setStatsIValue(0);
			statsUpdate1[49]->setStatsName("SSF_T_0_pi");
			statsUpdate1[50]->setStatsIValue(0);
			statsUpdate1[50]->setStatsName("T_0_ir");
			statsUpdate1[51]->setStatsIValue(0);
			statsUpdate1[51]->setStatsName("SSF_T_0_ir");
			statsUpdate1[52]->setStatsIValue(9);
			statsUpdate1[52]->setStatsName("T_0_atklp");
			statsUpdate1[53]->setStatsIValue(0);
			statsUpdate1[53]->setStatsName("SSF_T_0_atklp");
			statsUpdate1[54]->setStatsIValue(5);
			statsUpdate1[54]->setStatsName("T_0_atkmp");
			statsUpdate1[55]->setStatsIValue(0);
			statsUpdate1[55]->setStatsName("SSF_T_0_atkmp");
			statsUpdate1[56]->setStatsIValue(1);
			statsUpdate1[56]->setStatsName("T_0_atkrp");
			statsUpdate1[57]->setStatsIValue(0);
			statsUpdate1[57]->setStatsName("SSF_T_0_atkrp");
			statsUpdate1[58]->setStatsIValue(0);
			statsUpdate1[58]->setStatsName("T_0_g15");
			statsUpdate1[59]->setStatsIValue(0);
			statsUpdate1[59]->setStatsName("SSF_T_0_g15");
			statsUpdate1[60]->setStatsIValue(6);
			statsUpdate1[60]->setStatsName("T_0_g30");
			statsUpdate1[61]->setStatsIValue(0);
			statsUpdate1[61]->setStatsName("SSF_T_0_g30");
			statsUpdate1[62]->setStatsIValue(3);
			statsUpdate1[62]->setStatsName("T_0_g45");
			statsUpdate1[63]->setStatsIValue(0);
			statsUpdate1[63]->setStatsName("SSF_T_0_g45");
			statsUpdate1[64]->setStatsIValue(0);
			statsUpdate1[64]->setStatsName("T_0_g60");
			statsUpdate1[65]->setStatsIValue(0);
			statsUpdate1[65]->setStatsName("SSF_T_0_g60");
			statsUpdate1[66]->setStatsIValue(0);
			statsUpdate1[66]->setStatsName("T_0_g75");
			statsUpdate1[67]->setStatsIValue(0);
			statsUpdate1[67]->setStatsName("SSF_T_0_g75");
			statsUpdate1[68]->setStatsIValue(0);
			statsUpdate1[68]->setStatsName("T_0_g90");
			statsUpdate1[69]->setStatsIValue(0);
			statsUpdate1[69]->setStatsName("SSF_T_0_g90");
			statsUpdate1[70]->setStatsIValue(0);
			statsUpdate1[70]->setStatsName("T_0_g90Plus");
			statsUpdate1[71]->setStatsIValue(0);
			statsUpdate1[71]->setStatsName("SSF_T_0_g90Plus");
			statsUpdate1[72]->setStatsIValue(0);
			statsUpdate1[72]->setStatsName("T_0_gob");
			statsUpdate1[73]->setStatsIValue(0);
			statsUpdate1[73]->setStatsName("SSF_T_0_gob");
			statsUpdate1[74]->setStatsIValue(9);
			statsUpdate1[74]->setStatsName("T_0_gib");
			statsUpdate1[75]->setStatsIValue(0);
			statsUpdate1[75]->setStatsName("SSF_T_0_gib");
			statsUpdate1[76]->setStatsIValue(0);
			statsUpdate1[76]->setStatsName("T_0_gfk");
			statsUpdate1[77]->setStatsIValue(0);
			statsUpdate1[77]->setStatsName("SSF_T_0_gfk");
			statsUpdate1[78]->setStatsIValue(0);
			statsUpdate1[78]->setStatsName("T_0_g2pt");
			statsUpdate1[79]->setStatsIValue(0);
			statsUpdate1[79]->setStatsName("SSF_T_0_g2pt");
			statsUpdate1[80]->setStatsIValue(0);
			statsUpdate1[80]->setStatsName("T_0_g3pt");
			statsUpdate1[81]->setStatsIValue(0);
			statsUpdate1[81]->setStatsName("SSF_T_0_g3pt");
			statsUpdate1[82]->setStatsIValue(8);
			statsUpdate1[82]->setStatsName("T_0_posl");
			statsUpdate1[83]->setStatsIValue(0);
			statsUpdate1[83]->setStatsName("SSF_T_0_posl");
			statsUpdate1[84]->setStatsIValue(6);
			statsUpdate1[84]->setStatsName("T_0_cdr");
			statsUpdate1[85]->setStatsIValue(0);
			statsUpdate1[85]->setStatsName("SSF_T_0_cdr");
			statsUpdate1[86]->setStatsIValue(1);
			statsUpdate1[86]->setStatsName("S_0_ow");
			statsUpdate1[87]->setStatsIValue(4);
			statsUpdate1[87]->setStatsName("S_0_od");
			statsUpdate1[88]->setStatsIValue(0);
			statsUpdate1[88]->setStatsName("S_0_ol");
			statsUpdate1[89]->setStatsIValue(0);
			statsUpdate1[89]->setStatsName("S_0_smw");
			statsUpdate1[90]->setStatsIValue(4);
			statsUpdate1[90]->setStatsName("S_0_smd");
			statsUpdate1[91]->setStatsIValue(0);
			statsUpdate1[91]->setStatsName("S_0_sml");
			statsUpdate1[92]->setStatsIValue(0);
			statsUpdate1[92]->setStatsName("S_0_cw");
			statsUpdate1[93]->setStatsIValue(0);
			statsUpdate1[93]->setStatsName("S_0_cd");
			statsUpdate1[94]->setStatsIValue(0);
			statsUpdate1[94]->setStatsName("S_0_cl");
			statsUpdate1[95]->setStatsIValue(0);
			statsUpdate1[95]->setStatsName("S_0_suw");
			statsUpdate1[96]->setStatsIValue(0);
			statsUpdate1[96]->setStatsName("S_0_sud");
			statsUpdate1[97]->setStatsIValue(0);
			statsUpdate1[97]->setStatsName("S_0_sul");
			statsUpdate1[98]->setStatsIValue(0);
			statsUpdate1[98]->setStatsName("S_0_baw");
			statsUpdate1[99]->setStatsIValue(0);
			statsUpdate1[99]->setStatsName("S_0_bad");
			statsUpdate1[100]->setStatsIValue(0);
			statsUpdate1[100]->setStatsName("S_0_bal");
			statsUpdate1[101]->setStatsIValue(0);
			statsUpdate1[101]->setStatsName("S_0_fxw");
			statsUpdate1[102]->setStatsIValue(0);
			statsUpdate1[102]->setStatsName("S_0_fxd");
			statsUpdate1[103]->setStatsIValue(0);
			statsUpdate1[103]->setStatsName("S_0_fxl");
			statsUpdate1[104]->setStatsIValue(1);
			statsUpdate1[104]->setStatsName("S_0_nrw");
			statsUpdate1[105]->setStatsIValue(0);
			statsUpdate1[105]->setStatsName("S_0_nrd");
			statsUpdate1[106]->setStatsIValue(0);
			statsUpdate1[106]->setStatsName("S_0_nrl");
			statsUpdate1[107]->setStatsIValue(0);
			statsUpdate1[107]->setStatsName("S_0_hvw");
			statsUpdate1[108]->setStatsIValue(0);
			statsUpdate1[108]->setStatsName("S_0_hvd");
			statsUpdate1[109]->setStatsIValue(0);
			statsUpdate1[109]->setStatsName("S_0_hvl");
			statsUpdate1[110]->setStatsIValue(0);
			statsUpdate1[110]->setStatsName("S_0_mbw");
			statsUpdate1[111]->setStatsIValue(0);
			statsUpdate1[111]->setStatsName("S_0_mbd");
			statsUpdate1[112]->setStatsIValue(0);
			statsUpdate1[112]->setStatsName("S_0_mbl");
			statsUpdate1[113]->setStatsIValue(0);
			statsUpdate1[113]->setStatsName("S_0_tww");
			statsUpdate1[114]->setStatsIValue(0);
			statsUpdate1[114]->setStatsName("S_0_twd");
			statsUpdate1[115]->setStatsIValue(0);
			statsUpdate1[115]->setStatsName("S_0_twl");
			statsUpdate1[116]->setStatsIValue(0);
			statsUpdate1[116]->setStatsName("S_0_tlw");
			statsUpdate1[117]->setStatsIValue(0);
			statsUpdate1[117]->setStatsName("S_0_tll");
			statsUpdate1[118]->setStatsIValue(0);
			statsUpdate1[118]->setStatsName("S_0_bxw");
			statsUpdate1[119]->setStatsIValue(0);
			statsUpdate1[119]->setStatsName("S_0_bxl");
			statsUpdate1[120]->setStatsIValue(1);
			statsUpdate1[120]->setStatsName("S_0_lws");
			statsUpdate1[121]->setStatsIValue(0);
			statsUpdate1[121]->setStatsName("S_0_cst");
			statsUpdate1[122]->setStatsIValue(9);
			statsUpdate1[122]->setStatsName("S_0_fgs");
			statsUpdate1[123]->setStatsIValue(7);
			statsUpdate1[123]->setStatsName("S_0_fgt");
			statsUpdate1[124]->setStatsIValue(1);
			statsUpdate1[124]->setStatsName("S_0_fgmt");
			statsUpdate1[125]->setStatsIValue(3);
			statsUpdate1[125]->setStatsName("S_0_fgot");
			statsUpdate1[126]->setStatsIValue(0);
			statsUpdate1[126]->setStatsName("S_0_ssfow");
			statsUpdate1[127]->setStatsIValue(0);
			statsUpdate1[127]->setStatsName("S_0_ssfod");
			statsUpdate1[128]->setStatsIValue(0);
			statsUpdate1[128]->setStatsName("S_0_ssfol");
			statsUpdate1[129]->setStatsIValue(0);
			statsUpdate1[129]->setStatsName("S_0_ssfsmw");
			statsUpdate1[130]->setStatsIValue(0);
			statsUpdate1[130]->setStatsName("S_0_ssfsmd");
			statsUpdate1[131]->setStatsIValue(0);
			statsUpdate1[131]->setStatsName("S_0_ssfsml");
			statsUpdate1[132]->setStatsIValue(0);
			statsUpdate1[132]->setStatsName("S_0_ssfsuw");
			statsUpdate1[133]->setStatsIValue(0);
			statsUpdate1[133]->setStatsName("S_0_ssfsud");
			statsUpdate1[134]->setStatsIValue(0);
			statsUpdate1[134]->setStatsName("S_0_ssfsul");
			statsUpdate1[135]->setStatsIValue(0);
			statsUpdate1[135]->setStatsName("S_0_ssffxw");
			statsUpdate1[136]->setStatsIValue(0);
			statsUpdate1[136]->setStatsName("S_0_ssffxd");
			statsUpdate1[137]->setStatsIValue(0);
			statsUpdate1[137]->setStatsName("S_0_ssffxl");
			statsUpdate1[138]->setStatsIValue(0);
			statsUpdate1[138]->setStatsName("S_0_ssfnrw");
			statsUpdate1[139]->setStatsIValue(0);
			statsUpdate1[139]->setStatsName("S_0_ssfnrd");
			statsUpdate1[140]->setStatsIValue(0);
			statsUpdate1[140]->setStatsName("S_0_ssfnrl");
			statsUpdate1[141]->setStatsIValue(0);
			statsUpdate1[141]->setStatsName("S_0_ssflws");
			statsUpdate1[142]->setStatsIValue(0);
			statsUpdate1[142]->setStatsName("S_0_ssfcst");
			statsUpdate1[143]->setStatsIValue(0);
			statsUpdate1[143]->setStatsName("S_0_ssffgs");
			statsUpdate1[144]->setStatsIValue(0);
			statsUpdate1[144]->setStatsName("S_0_ssffgt");
			statsUpdate1[145]->setStatsIValue(0);
			statsUpdate1[145]->setStatsName("S_0_ssffgmt");
			statsUpdate1[146]->setStatsIValue(0);
			statsUpdate1[146]->setStatsName("S_0_ssffgot");

			for (int i = 0; i < 147; i++)
			{
				statUpdateList1.push_back(statsUpdate1[i]);
			}
		}

		if (mScenarioType != TEAM_1_CPU && mScenarioType != TEAM_2_CPU && mOverallGroupB_ID != "")
		{
			Blaze::FifaStats::Entity *entity2 = updateStatsRequest.getEntities().pull_back();
			Blaze::FifaStats::StatsUpdateList &statUpdateList2 = entity2->getStatsUpdates();
			Blaze::FifaStats::StatsUpdate *statsUpdate2[147];

			//set group ID B
			entity2->setEntityId(mOverallGroupB_ID.c_str());
			if (!strcmp("gameType100", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
			{
				for (int i = 0; i < 147; i++)
				{
					statsUpdate2[i] = entity1->getStatsUpdates().pull_back();
					statsUpdate2[i]->setOperator(Blaze::FifaStats::StatsOperator::OPERATOR_REPLACE);
					statsUpdate2[i]->setStatsFValue(0.00000000);
					statsUpdate2[i]->setType(Blaze::FifaStats::StatsType::TYPE_INT);
				}
				statsUpdate2[0]->setStatsIValue(0);
				statsUpdate2[0]->setStatsName("T_0_g");
				statsUpdate2[1]->setStatsIValue(0);
				statsUpdate2[1]->setStatsName("SSF_T_0_g");
				statsUpdate2[2]->setStatsIValue(0);
				statsUpdate2[2]->setStatsName("T_0_og");
				statsUpdate2[3]->setStatsIValue(0);
				statsUpdate2[3]->setStatsName("SSF_T_0_og");
				statsUpdate2[4]->setStatsIValue(0);
				statsUpdate2[4]->setStatsName("T_0_pkg");
				statsUpdate2[5]->setStatsIValue(0);
				statsUpdate2[5]->setStatsName("SSF_T_0_pkg");
				statsUpdate2[6]->setStatsIValue(7);
				statsUpdate2[6]->setStatsName("T_0_pos");
				statsUpdate2[7]->setStatsIValue(0);
				statsUpdate2[7]->setStatsName("SSF_T_0_pos");
				statsUpdate2[8]->setStatsIValue(0);
				statsUpdate2[8]->setStatsName("T_0_s");
				statsUpdate2[9]->setStatsIValue(0);
				statsUpdate2[9]->setStatsName("SSF_T_0_s");
				statsUpdate2[10]->setStatsIValue(0);
				statsUpdate2[10]->setStatsName("T_0_sog");
				statsUpdate2[11]->setStatsIValue(0);
				statsUpdate2[11]->setStatsName("SSF_T_0_sog");
				statsUpdate2[12]->setStatsIValue(3);
				statsUpdate2[12]->setStatsName("T_0_pa");
				statsUpdate2[13]->setStatsIValue(0);
				statsUpdate2[13]->setStatsName("SSF_T_0_pa");
				statsUpdate2[14]->setStatsIValue(2);
				statsUpdate2[14]->setStatsName("T_0_pm");
				statsUpdate2[15]->setStatsIValue(0);
				statsUpdate2[15]->setStatsName("SSF_T_0_pm");
				statsUpdate2[16]->setStatsIValue(6);
				statsUpdate2[16]->setStatsName("T_0_ta");
				statsUpdate2[17]->setStatsIValue(0);
				statsUpdate2[17]->setStatsName("SSF_T_0_ta");
				statsUpdate2[18]->setStatsIValue(1);
				statsUpdate2[18]->setStatsName("T_0_tm");
				statsUpdate2[19]->setStatsIValue(0);
				statsUpdate2[19]->setStatsName("SSF_T_0_tm");
				statsUpdate2[20]->setStatsIValue(0);
				statsUpdate2[20]->setStatsName("T_0_gc");
				statsUpdate2[21]->setStatsIValue(0);
				statsUpdate2[21]->setStatsName("SSF_T_0_gc");
				statsUpdate2[22]->setStatsIValue(0);
				statsUpdate2[22]->setStatsName("T_0_f");
				statsUpdate2[23]->setStatsIValue(0);
				statsUpdate2[23]->setStatsName("SSF_T_0_f");
				statsUpdate2[24]->setStatsIValue(0);
				statsUpdate2[24]->setStatsName("T_0_yc");
				statsUpdate2[25]->setStatsIValue(0);
				statsUpdate2[25]->setStatsName("SSF_T_0_yc");
				statsUpdate2[26]->setStatsIValue(0);
				statsUpdate2[26]->setStatsName("T_0_rc");
				statsUpdate2[27]->setStatsIValue(0);
				statsUpdate2[27]->setStatsName("SSF_T_0_rc");
				statsUpdate2[28]->setStatsIValue(0);
				statsUpdate2[28]->setStatsName("T_0_c");
				statsUpdate2[29]->setStatsIValue(0);
				statsUpdate2[29]->setStatsName("SSF_T_0_c");
				statsUpdate2[30]->setStatsIValue(0);
				statsUpdate2[30]->setStatsName("T_0_os");
				statsUpdate2[31]->setStatsIValue(0);
				statsUpdate2[31]->setStatsName("SSF_T_0_os");
				statsUpdate2[32]->setStatsIValue(1);
				statsUpdate2[32]->setStatsName("T_0_cs");
				statsUpdate2[33]->setStatsIValue(0);
				statsUpdate2[33]->setStatsName("SSF_T_0_cs");
				statsUpdate2[34]->setStatsIValue(5);
				statsUpdate2[34]->setStatsName("T_0_sa");
				statsUpdate2[35]->setStatsIValue(0);
				statsUpdate2[35]->setStatsName("SSF_T_0_sa");
				statsUpdate2[36]->setStatsIValue(1);
				statsUpdate2[36]->setStatsName("T_0_saog");
				statsUpdate2[37]->setStatsIValue(0);
				statsUpdate2[37]->setStatsName("SSF_T_0_saog");
				statsUpdate2[38]->setStatsIValue(9);
				statsUpdate2[38]->setStatsName("T_0_int");
				statsUpdate2[39]->setStatsIValue(0);
				statsUpdate2[39]->setStatsName("SSF_T_0_int");
				statsUpdate2[40]->setStatsIValue(0);
				statsUpdate2[40]->setStatsName("T_0_pka");
				statsUpdate2[41]->setStatsIValue(0);
				statsUpdate2[41]->setStatsName("SSF_T_0_pka");
				statsUpdate2[42]->setStatsIValue(0);
				statsUpdate2[42]->setStatsName("T_0_pks");
				statsUpdate2[43]->setStatsIValue(0);
				statsUpdate2[43]->setStatsName("SSF_T_0_pks");
				statsUpdate2[44]->setStatsIValue(0);
				statsUpdate2[44]->setStatsName("T_0_pkv");
				statsUpdate2[45]->setStatsIValue(0);
				statsUpdate2[45]->setStatsName("SSF_T_0_pkv");
				statsUpdate2[46]->setStatsIValue(0);
				statsUpdate2[46]->setStatsName("T_0_pkm");
				statsUpdate2[47]->setStatsIValue(0);
				statsUpdate2[47]->setStatsName("SSF_T_0_pkm");
				statsUpdate2[48]->setStatsIValue(8);
				statsUpdate2[48]->setStatsName("T_0_pi");
				statsUpdate2[49]->setStatsIValue(0);
				statsUpdate2[49]->setStatsName("SSF_T_0_pi");
				statsUpdate2[50]->setStatsIValue(0);
				statsUpdate2[50]->setStatsName("T_0_ir");
				statsUpdate2[51]->setStatsIValue(0);
				statsUpdate2[51]->setStatsName("SSF_T_0_ir");
				statsUpdate2[52]->setStatsIValue(0);
				statsUpdate2[52]->setStatsName("T_0_atklp");
				statsUpdate2[53]->setStatsIValue(0);
				statsUpdate2[53]->setStatsName("SSF_T_0_atklp");
				statsUpdate2[54]->setStatsIValue(0);
				statsUpdate2[54]->setStatsName("T_0_atkmp");
				statsUpdate2[55]->setStatsIValue(0);
				statsUpdate2[55]->setStatsName("SSF_T_0_atkmp");
				statsUpdate2[56]->setStatsIValue(0);
				statsUpdate2[56]->setStatsName("T_0_atkrp");
				statsUpdate2[57]->setStatsIValue(0);
				statsUpdate2[57]->setStatsName("SSF_T_0_atkrp");
				statsUpdate2[58]->setStatsIValue(0);
				statsUpdate2[58]->setStatsName("T_0_g15");
				statsUpdate2[59]->setStatsIValue(0);
				statsUpdate2[59]->setStatsName("SSF_T_0_g15");
				statsUpdate2[60]->setStatsIValue(0);
				statsUpdate2[60]->setStatsName("T_0_g30");
				statsUpdate2[61]->setStatsIValue(0);
				statsUpdate2[61]->setStatsName("SSF_T_0_g30");
				statsUpdate2[62]->setStatsIValue(0);
				statsUpdate2[62]->setStatsName("T_0_g45");
				statsUpdate2[63]->setStatsIValue(0);
				statsUpdate2[63]->setStatsName("SSF_T_0_g45");
				statsUpdate2[64]->setStatsIValue(0);
				statsUpdate2[64]->setStatsName("T_0_g60");
				statsUpdate2[65]->setStatsIValue(0);
				statsUpdate2[65]->setStatsName("SSF_T_0_g60");
				statsUpdate2[66]->setStatsIValue(0);
				statsUpdate2[66]->setStatsName("T_0_g75");
				statsUpdate2[67]->setStatsIValue(0);
				statsUpdate2[67]->setStatsName("SSF_T_0_g75");
				statsUpdate2[68]->setStatsIValue(0);
				statsUpdate2[68]->setStatsName("T_0_g90");
				statsUpdate2[69]->setStatsIValue(0);
				statsUpdate2[69]->setStatsName("SSF_T_0_g90");
				statsUpdate2[70]->setStatsIValue(0);
				statsUpdate2[70]->setStatsName("T_0_g90Plus");
				statsUpdate2[71]->setStatsIValue(0);
				statsUpdate2[71]->setStatsName("SSF_T_0_g90Plus");
				statsUpdate2[72]->setStatsIValue(0);
				statsUpdate2[72]->setStatsName("T_0_gob");
				statsUpdate2[73]->setStatsIValue(0);
				statsUpdate2[73]->setStatsName("SSF_T_0_gob");
				statsUpdate2[74]->setStatsIValue(0);
				statsUpdate2[74]->setStatsName("T_0_gib");
				statsUpdate2[75]->setStatsIValue(0);
				statsUpdate2[75]->setStatsName("SSF_T_0_gib");
				statsUpdate2[76]->setStatsIValue(0);
				statsUpdate2[76]->setStatsName("T_0_gfk");
				statsUpdate2[77]->setStatsIValue(0);
				statsUpdate2[77]->setStatsName("SSF_T_0_gfk");
				statsUpdate2[78]->setStatsIValue(0);
				statsUpdate2[78]->setStatsName("T_0_g2pt");
				statsUpdate2[79]->setStatsIValue(0);
				statsUpdate2[79]->setStatsName("SSF_T_0_g2pt");
				statsUpdate2[80]->setStatsIValue(0);
				statsUpdate2[80]->setStatsName("T_0_g3pt");
				statsUpdate2[81]->setStatsIValue(0);
				statsUpdate2[81]->setStatsName("SSF_T_0_g3pt");
				statsUpdate2[82]->setStatsIValue(1);
				statsUpdate2[82]->setStatsName("T_0_posl");
				statsUpdate2[83]->setStatsIValue(0);
				statsUpdate2[83]->setStatsName("SSF_T_0_posl");
				statsUpdate2[84]->setStatsIValue(1);
				statsUpdate2[84]->setStatsName("T_0_cdr");
				statsUpdate2[85]->setStatsIValue(0);
				statsUpdate2[85]->setStatsName("SSF_T_0_cdr");
				statsUpdate2[86]->setStatsIValue(0);
				statsUpdate2[86]->setStatsName("S_0_ow");
				statsUpdate2[87]->setStatsIValue(1);
				statsUpdate2[87]->setStatsName("S_0_od");
				statsUpdate2[88]->setStatsIValue(0);
				statsUpdate2[88]->setStatsName("S_0_ol");
				statsUpdate2[89]->setStatsIValue(0);
				statsUpdate2[89]->setStatsName("S_0_smw");
				statsUpdate2[90]->setStatsIValue(1);
				statsUpdate2[90]->setStatsName("S_0_smd");
				statsUpdate2[91]->setStatsIValue(0);
				statsUpdate2[91]->setStatsName("S_0_sml");
				statsUpdate2[92]->setStatsIValue(0);
				statsUpdate2[92]->setStatsName("S_0_cw");
				statsUpdate2[93]->setStatsIValue(0);
				statsUpdate2[93]->setStatsName("S_0_cd");
				statsUpdate2[94]->setStatsIValue(0);
				statsUpdate2[94]->setStatsName("S_0_cl");
				statsUpdate2[95]->setStatsIValue(0);
				statsUpdate2[95]->setStatsName("S_0_suw");
				statsUpdate2[96]->setStatsIValue(0);
				statsUpdate2[96]->setStatsName("S_0_sud");
				statsUpdate2[97]->setStatsIValue(0);
				statsUpdate2[97]->setStatsName("S_0_sul");
				statsUpdate2[98]->setStatsIValue(0);
				statsUpdate2[98]->setStatsName("S_0_baw");
				statsUpdate2[99]->setStatsIValue(0);
				statsUpdate2[99]->setStatsName("S_0_bad");
				statsUpdate2[100]->setStatsIValue(0);
				statsUpdate2[100]->setStatsName("S_0_bal");
				statsUpdate2[101]->setStatsIValue(0);
				statsUpdate2[101]->setStatsName("S_0_fxw");
				statsUpdate2[102]->setStatsIValue(0);
				statsUpdate2[102]->setStatsName("S_0_fxd");
				statsUpdate2[103]->setStatsIValue(0);
				statsUpdate2[103]->setStatsName("S_0_fxl");
				statsUpdate2[104]->setStatsIValue(0);
				statsUpdate2[104]->setStatsName("S_0_nrw");
				statsUpdate2[105]->setStatsIValue(0);
				statsUpdate2[105]->setStatsName("S_0_nrd");
				statsUpdate2[106]->setStatsIValue(0);
				statsUpdate2[106]->setStatsName("S_0_nrl");
				statsUpdate2[107]->setStatsIValue(0);
				statsUpdate2[107]->setStatsName("S_0_hvw");
				statsUpdate2[108]->setStatsIValue(0);
				statsUpdate2[108]->setStatsName("S_0_hvd");
				statsUpdate2[109]->setStatsIValue(0);
				statsUpdate2[109]->setStatsName("S_0_hvl");
				statsUpdate2[110]->setStatsIValue(0);
				statsUpdate2[110]->setStatsName("S_0_mbw");
				statsUpdate2[111]->setStatsIValue(0);
				statsUpdate2[111]->setStatsName("S_0_mbd");
				statsUpdate2[112]->setStatsIValue(0);
				statsUpdate2[112]->setStatsName("S_0_mbl");
				statsUpdate2[113]->setStatsIValue(0);
				statsUpdate2[113]->setStatsName("S_0_tww");
				statsUpdate2[114]->setStatsIValue(0);
				statsUpdate2[114]->setStatsName("S_0_twd");
				statsUpdate2[115]->setStatsIValue(0);
				statsUpdate2[115]->setStatsName("S_0_twl");
				statsUpdate2[116]->setStatsIValue(0);
				statsUpdate2[116]->setStatsName("S_0_tlw");
				statsUpdate2[117]->setStatsIValue(0);
				statsUpdate2[117]->setStatsName("S_0_tll");
				statsUpdate2[118]->setStatsIValue(0);
				statsUpdate2[118]->setStatsName("S_0_bxw");
				statsUpdate2[119]->setStatsIValue(0);
				statsUpdate2[119]->setStatsName("S_0_bxl");
				statsUpdate2[120]->setStatsIValue(0);
				statsUpdate2[120]->setStatsName("S_0_lws");
				statsUpdate2[121]->setStatsIValue(0);
				statsUpdate2[121]->setStatsName("S_0_cst");
				statsUpdate2[122]->setStatsIValue(0);
				statsUpdate2[122]->setStatsName("S_0_fgs");
				statsUpdate2[123]->setStatsIValue(0);
				statsUpdate2[123]->setStatsName("S_0_fgt");
				statsUpdate2[124]->setStatsIValue(0);
				statsUpdate2[124]->setStatsName("S_0_fgmt");
				statsUpdate2[125]->setStatsIValue(0);
				statsUpdate2[125]->setStatsName("S_0_fgot");
				statsUpdate2[126]->setStatsIValue(0);
				statsUpdate2[126]->setStatsName("S_0_ssfow");
				statsUpdate2[127]->setStatsIValue(0);
				statsUpdate2[127]->setStatsName("S_0_ssfod");
				statsUpdate2[128]->setStatsIValue(0);
				statsUpdate2[128]->setStatsName("S_0_ssfol");
				statsUpdate2[129]->setStatsIValue(0);
				statsUpdate2[129]->setStatsName("S_0_ssfsmw");
				statsUpdate2[130]->setStatsIValue(0);
				statsUpdate2[130]->setStatsName("S_0_ssfsmd");
				statsUpdate2[131]->setStatsIValue(0);
				statsUpdate2[131]->setStatsName("S_0_ssfsml");
				statsUpdate2[132]->setStatsIValue(0);
				statsUpdate2[132]->setStatsName("S_0_ssfsuw");
				statsUpdate2[133]->setStatsIValue(0);
				statsUpdate2[133]->setStatsName("S_0_ssfsud");
				statsUpdate2[134]->setStatsIValue(0);
				statsUpdate2[134]->setStatsName("S_0_ssfsul");
				statsUpdate2[135]->setStatsIValue(0);
				statsUpdate2[135]->setStatsName("S_0_ssffxw");
				statsUpdate2[136]->setStatsIValue(0);
				statsUpdate2[136]->setStatsName("S_0_ssffxd");
				statsUpdate2[137]->setStatsIValue(0);
				statsUpdate2[137]->setStatsName("S_0_ssffxl");
				statsUpdate2[138]->setStatsIValue(0);
				statsUpdate2[138]->setStatsName("S_0_ssfnrw");
				statsUpdate2[139]->setStatsIValue(0);
				statsUpdate2[139]->setStatsName("S_0_ssfnrd");
				statsUpdate2[140]->setStatsIValue(0);
				statsUpdate2[140]->setStatsName("S_0_ssfnrl");
				statsUpdate2[141]->setStatsIValue(0);
				statsUpdate2[141]->setStatsName("S_0_ssflws");
				statsUpdate2[142]->setStatsIValue(0);
				statsUpdate2[142]->setStatsName("S_0_ssfcst");
				statsUpdate2[143]->setStatsIValue(0);
				statsUpdate2[143]->setStatsName("S_0_ssffgs");
				statsUpdate2[144]->setStatsIValue(0);
				statsUpdate2[144]->setStatsName("S_0_ssffgt");
				statsUpdate2[145]->setStatsIValue(0);
				statsUpdate2[145]->setStatsName("S_0_ssffgmt");
				statsUpdate2[146]->setStatsIValue(0);
				statsUpdate2[146]->setStatsName("S_0_ssffgot");

				for (int i = 0; i < 147; i++)
				{
					statUpdateList2.push_back(statsUpdate2[i]);
				}
				
			}
			if (!strcmp("gameType90", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
			{
				for (int i = 0; i < 147; i++)
				{
					statsUpdate2[i] = entity1->getStatsUpdates().pull_back();
					statsUpdate2[i]->setOperator(Blaze::FifaStats::StatsOperator::OPERATOR_REPLACE);
					statsUpdate2[i]->setStatsFValue(0.00000000);
					statsUpdate2[i]->setType(Blaze::FifaStats::StatsType::TYPE_INT);
				}
				statsUpdate2[0]->setStatsIValue(9);
				statsUpdate2[0]->setStatsName("T_0_g");
				statsUpdate2[1]->setStatsIValue(0);
				statsUpdate2[1]->setStatsName("SSF_T_0_g");
				statsUpdate2[2]->setStatsIValue(0);
				statsUpdate2[2]->setStatsName("T_0_og");
				statsUpdate2[3]->setStatsIValue(0);
				statsUpdate2[3]->setStatsName("SSF_T_0_og");
				statsUpdate2[4]->setStatsIValue(0);
				statsUpdate2[4]->setStatsName("T_0_pkg");
				statsUpdate2[5]->setStatsIValue(0);
				statsUpdate2[5]->setStatsName("SSF_T_0_pkg");
				statsUpdate2[6]->setStatsIValue(3);
				statsUpdate2[6]->setStatsName("T_0_pos");
				statsUpdate2[7]->setStatsIValue(0);
				statsUpdate2[7]->setStatsName("SSF_T_0_pos");
				statsUpdate2[8]->setStatsIValue(3);
				statsUpdate2[8]->setStatsName("T_0_s");
				statsUpdate2[9]->setStatsIValue(0);
				statsUpdate2[9]->setStatsName("SSF_T_0_s");
				statsUpdate2[10]->setStatsIValue(2);
				statsUpdate2[10]->setStatsName("T_0_sog");
				statsUpdate2[11]->setStatsIValue(0);
				statsUpdate2[11]->setStatsName("SSF_T_0_sog");
				statsUpdate2[12]->setStatsIValue(1);
				statsUpdate2[12]->setStatsName("T_0_pa");
				statsUpdate2[13]->setStatsIValue(0);
				statsUpdate2[13]->setStatsName("SSF_T_0_pa");
				statsUpdate2[14]->setStatsIValue(9);
				statsUpdate2[14]->setStatsName("T_0_pm");
				statsUpdate2[15]->setStatsIValue(0);
				statsUpdate2[15]->setStatsName("SSF_T_0_pm");
				statsUpdate2[16]->setStatsIValue(8);
				statsUpdate2[16]->setStatsName("T_0_ta");
				statsUpdate2[17]->setStatsIValue(0);
				statsUpdate2[17]->setStatsName("SSF_T_0_ta");
				statsUpdate2[18]->setStatsIValue(3);
				statsUpdate2[18]->setStatsName("T_0_tm");
				statsUpdate2[19]->setStatsIValue(0);
				statsUpdate2[19]->setStatsName("SSF_T_0_tm");
				statsUpdate2[20]->setStatsIValue(0);
				statsUpdate2[20]->setStatsName("T_0_gc");
				statsUpdate2[21]->setStatsIValue(0);
				statsUpdate2[21]->setStatsName("SSF_T_0_gc");
				statsUpdate2[22]->setStatsIValue(0);
				statsUpdate2[22]->setStatsName("T_0_f");
				statsUpdate2[23]->setStatsIValue(0);
				statsUpdate2[23]->setStatsName("SSF_T_0_f");
				statsUpdate2[24]->setStatsIValue(0);
				statsUpdate2[24]->setStatsName("T_0_yc");
				statsUpdate2[25]->setStatsIValue(0);
				statsUpdate2[25]->setStatsName("SSF_T_0_yc");
				statsUpdate2[26]->setStatsIValue(0);
				statsUpdate2[26]->setStatsName("T_0_rc");
				statsUpdate2[27]->setStatsIValue(0);
				statsUpdate2[27]->setStatsName("SSF_T_0_rc");
				statsUpdate2[28]->setStatsIValue(4);
				statsUpdate2[28]->setStatsName("T_0_c");
				statsUpdate2[29]->setStatsIValue(0);
				statsUpdate2[29]->setStatsName("SSF_T_0_c");
				statsUpdate2[30]->setStatsIValue(0);
				statsUpdate2[30]->setStatsName("T_0_os");
				statsUpdate2[31]->setStatsIValue(0);
				statsUpdate2[31]->setStatsName("SSF_T_0_os");
				statsUpdate2[32]->setStatsIValue(5);
				statsUpdate2[32]->setStatsName("T_0_cs");
				statsUpdate2[33]->setStatsIValue(0);
				statsUpdate2[33]->setStatsName("SSF_T_0_cs");
				statsUpdate2[34]->setStatsIValue(1);
				statsUpdate2[34]->setStatsName("T_0_sa");
				statsUpdate2[35]->setStatsIValue(0);
				statsUpdate2[35]->setStatsName("SSF_T_0_sa");
				statsUpdate2[36]->setStatsIValue(6);
				statsUpdate2[36]->setStatsName("T_0_saog");
				statsUpdate2[37]->setStatsIValue(0);
				statsUpdate2[37]->setStatsName("SSF_T_0_saog");
				statsUpdate2[38]->setStatsIValue(5);
				statsUpdate2[38]->setStatsName("T_0_int");
				statsUpdate2[39]->setStatsIValue(0);
				statsUpdate2[39]->setStatsName("SSF_T_0_int");
				statsUpdate2[40]->setStatsIValue(0);
				statsUpdate2[40]->setStatsName("T_0_pka");
				statsUpdate2[41]->setStatsIValue(0);
				statsUpdate2[41]->setStatsName("SSF_T_0_pka");
				statsUpdate2[42]->setStatsIValue(0);
				statsUpdate2[42]->setStatsName("T_0_pks");
				statsUpdate2[43]->setStatsIValue(0);
				statsUpdate2[43]->setStatsName("SSF_T_0_pks");
				statsUpdate2[44]->setStatsIValue(0);
				statsUpdate2[44]->setStatsName("T_0_pkv");
				statsUpdate2[45]->setStatsIValue(0);
				statsUpdate2[45]->setStatsName("SSF_T_0_pkv");
				statsUpdate2[46]->setStatsIValue(0);
				statsUpdate2[46]->setStatsName("T_0_pkm");
				statsUpdate2[47]->setStatsIValue(0);
				statsUpdate2[47]->setStatsName("SSF_T_0_pkm");
				statsUpdate2[48]->setStatsIValue(1);
				statsUpdate2[48]->setStatsName("T_0_pi");
				statsUpdate2[49]->setStatsIValue(0);
				statsUpdate2[49]->setStatsName("SSF_T_0_pi");
				statsUpdate2[50]->setStatsIValue(0);
				statsUpdate2[50]->setStatsName("T_0_ir");
				statsUpdate2[51]->setStatsIValue(0);
				statsUpdate2[51]->setStatsName("SSF_T_0_ir");
				statsUpdate2[52]->setStatsIValue(9);
				statsUpdate2[52]->setStatsName("T_0_atklp");
				statsUpdate2[53]->setStatsIValue(0);
				statsUpdate2[53]->setStatsName("SSF_T_0_atklp");
				statsUpdate2[54]->setStatsIValue(5);
				statsUpdate2[54]->setStatsName("T_0_atkmp");
				statsUpdate2[55]->setStatsIValue(0);
				statsUpdate2[55]->setStatsName("SSF_T_0_atkmp");
				statsUpdate2[56]->setStatsIValue(1);
				statsUpdate2[56]->setStatsName("T_0_atkrp");
				statsUpdate2[57]->setStatsIValue(0);
				statsUpdate2[57]->setStatsName("SSF_T_0_atkrp");
				statsUpdate2[58]->setStatsIValue(0);
				statsUpdate2[58]->setStatsName("T_0_g15");
				statsUpdate2[59]->setStatsIValue(0);
				statsUpdate2[59]->setStatsName("SSF_T_0_g15");
				statsUpdate2[60]->setStatsIValue(6);
				statsUpdate2[60]->setStatsName("T_0_g30");
				statsUpdate2[61]->setStatsIValue(0);
				statsUpdate2[61]->setStatsName("SSF_T_0_g30");
				statsUpdate2[62]->setStatsIValue(3);
				statsUpdate2[62]->setStatsName("T_0_g45");
				statsUpdate2[63]->setStatsIValue(0);
				statsUpdate2[63]->setStatsName("SSF_T_0_g45");
				statsUpdate2[64]->setStatsIValue(0);
				statsUpdate2[64]->setStatsName("T_0_g60");
				statsUpdate2[65]->setStatsIValue(0);
				statsUpdate2[65]->setStatsName("SSF_T_0_g60");
				statsUpdate2[66]->setStatsIValue(0);
				statsUpdate2[66]->setStatsName("T_0_g75");
				statsUpdate2[67]->setStatsIValue(0);
				statsUpdate2[67]->setStatsName("SSF_T_0_g75");
				statsUpdate2[68]->setStatsIValue(0);
				statsUpdate2[68]->setStatsName("T_0_g90");
				statsUpdate2[69]->setStatsIValue(0);
				statsUpdate2[69]->setStatsName("SSF_T_0_g90");
				statsUpdate2[70]->setStatsIValue(0);
				statsUpdate2[70]->setStatsName("T_0_g90Plus");
				statsUpdate2[71]->setStatsIValue(0);
				statsUpdate2[71]->setStatsName("SSF_T_0_g90Plus");
				statsUpdate2[72]->setStatsIValue(0);
				statsUpdate2[72]->setStatsName("T_0_gob");
				statsUpdate2[73]->setStatsIValue(0);
				statsUpdate2[73]->setStatsName("SSF_T_0_gob");
				statsUpdate2[74]->setStatsIValue(9);
				statsUpdate2[74]->setStatsName("T_0_gib");
				statsUpdate2[75]->setStatsIValue(0);
				statsUpdate2[75]->setStatsName("SSF_T_0_gib");
				statsUpdate2[76]->setStatsIValue(0);
				statsUpdate2[76]->setStatsName("T_0_gfk");
				statsUpdate2[77]->setStatsIValue(0);
				statsUpdate2[77]->setStatsName("SSF_T_0_gfk");
				statsUpdate2[78]->setStatsIValue(0);
				statsUpdate2[78]->setStatsName("T_0_g2pt");
				statsUpdate2[79]->setStatsIValue(0);
				statsUpdate2[79]->setStatsName("SSF_T_0_g2pt");
				statsUpdate2[80]->setStatsIValue(0);
				statsUpdate2[80]->setStatsName("T_0_g3pt");
				statsUpdate2[81]->setStatsIValue(0);
				statsUpdate2[81]->setStatsName("SSF_T_0_g3pt");
				statsUpdate2[82]->setStatsIValue(8);
				statsUpdate2[82]->setStatsName("T_0_posl");
				statsUpdate2[83]->setStatsIValue(0);
				statsUpdate2[83]->setStatsName("SSF_T_0_posl");
				statsUpdate2[84]->setStatsIValue(6);
				statsUpdate2[84]->setStatsName("T_0_cdr");
				statsUpdate2[85]->setStatsIValue(0);
				statsUpdate2[85]->setStatsName("SSF_T_0_cdr");
				statsUpdate2[86]->setStatsIValue(1);
				statsUpdate2[86]->setStatsName("S_0_ow");
				statsUpdate2[87]->setStatsIValue(4);
				statsUpdate2[87]->setStatsName("S_0_od");
				statsUpdate2[88]->setStatsIValue(0);
				statsUpdate2[88]->setStatsName("S_0_ol");
				statsUpdate2[89]->setStatsIValue(0);
				statsUpdate2[89]->setStatsName("S_0_smw");
				statsUpdate2[90]->setStatsIValue(4);
				statsUpdate2[90]->setStatsName("S_0_smd");
				statsUpdate2[91]->setStatsIValue(0);
				statsUpdate2[91]->setStatsName("S_0_sml");
				statsUpdate2[92]->setStatsIValue(0);
				statsUpdate2[92]->setStatsName("S_0_cw");
				statsUpdate2[93]->setStatsIValue(0);
				statsUpdate2[93]->setStatsName("S_0_cd");
				statsUpdate2[94]->setStatsIValue(0);
				statsUpdate2[94]->setStatsName("S_0_cl");
				statsUpdate2[95]->setStatsIValue(0);
				statsUpdate2[95]->setStatsName("S_0_suw");
				statsUpdate2[96]->setStatsIValue(0);
				statsUpdate2[96]->setStatsName("S_0_sud");
				statsUpdate2[97]->setStatsIValue(0);
				statsUpdate2[97]->setStatsName("S_0_sul");
				statsUpdate2[98]->setStatsIValue(0);
				statsUpdate2[98]->setStatsName("S_0_baw");
				statsUpdate2[99]->setStatsIValue(0);
				statsUpdate2[99]->setStatsName("S_0_bad");
				statsUpdate2[100]->setStatsIValue(0);
				statsUpdate2[100]->setStatsName("S_0_bal");
				statsUpdate2[101]->setStatsIValue(0);
				statsUpdate2[101]->setStatsName("S_0_fxw");
				statsUpdate2[102]->setStatsIValue(0);
				statsUpdate2[102]->setStatsName("S_0_fxd");
				statsUpdate2[103]->setStatsIValue(0);
				statsUpdate2[103]->setStatsName("S_0_fxl");
				statsUpdate2[104]->setStatsIValue(1);
				statsUpdate2[104]->setStatsName("S_0_nrw");
				statsUpdate2[105]->setStatsIValue(0);
				statsUpdate2[105]->setStatsName("S_0_nrd");
				statsUpdate2[106]->setStatsIValue(0);
				statsUpdate2[106]->setStatsName("S_0_nrl");
				statsUpdate2[107]->setStatsIValue(0);
				statsUpdate2[107]->setStatsName("S_0_hvw");
				statsUpdate2[108]->setStatsIValue(0);
				statsUpdate2[108]->setStatsName("S_0_hvd");
				statsUpdate2[109]->setStatsIValue(0);
				statsUpdate2[109]->setStatsName("S_0_hvl");
				statsUpdate2[110]->setStatsIValue(0);
				statsUpdate2[110]->setStatsName("S_0_mbw");
				statsUpdate2[111]->setStatsIValue(0);
				statsUpdate2[111]->setStatsName("S_0_mbd");
				statsUpdate2[112]->setStatsIValue(0);
				statsUpdate2[112]->setStatsName("S_0_mbl");
				statsUpdate2[113]->setStatsIValue(0);
				statsUpdate2[113]->setStatsName("S_0_tww");
				statsUpdate2[114]->setStatsIValue(0);
				statsUpdate2[114]->setStatsName("S_0_twd");
				statsUpdate2[115]->setStatsIValue(0);
				statsUpdate2[115]->setStatsName("S_0_twl");
				statsUpdate2[116]->setStatsIValue(0);
				statsUpdate2[116]->setStatsName("S_0_tlw");
				statsUpdate2[117]->setStatsIValue(0);
				statsUpdate2[117]->setStatsName("S_0_tll");
				statsUpdate2[118]->setStatsIValue(0);
				statsUpdate2[118]->setStatsName("S_0_bxw");
				statsUpdate2[119]->setStatsIValue(0);
				statsUpdate2[119]->setStatsName("S_0_bxl");
				statsUpdate2[120]->setStatsIValue(1);
				statsUpdate2[120]->setStatsName("S_0_lws");
				statsUpdate2[121]->setStatsIValue(0);
				statsUpdate2[121]->setStatsName("S_0_cst");
				statsUpdate2[122]->setStatsIValue(9);
				statsUpdate2[122]->setStatsName("S_0_fgs");
				statsUpdate2[123]->setStatsIValue(7);
				statsUpdate2[123]->setStatsName("S_0_fgt");
				statsUpdate2[124]->setStatsIValue(1);
				statsUpdate2[124]->setStatsName("S_0_fgmt");
				statsUpdate2[125]->setStatsIValue(3);
				statsUpdate2[125]->setStatsName("S_0_fgot");
				statsUpdate2[126]->setStatsIValue(0);
				statsUpdate2[126]->setStatsName("S_0_ssfow");
				statsUpdate2[127]->setStatsIValue(0);
				statsUpdate2[127]->setStatsName("S_0_ssfod");
				statsUpdate2[128]->setStatsIValue(0);
				statsUpdate2[128]->setStatsName("S_0_ssfol");
				statsUpdate2[129]->setStatsIValue(0);
				statsUpdate2[129]->setStatsName("S_0_ssfsmw");
				statsUpdate2[130]->setStatsIValue(0);
				statsUpdate2[130]->setStatsName("S_0_ssfsmd");
				statsUpdate2[131]->setStatsIValue(0);
				statsUpdate2[131]->setStatsName("S_0_ssfsml");
				statsUpdate2[132]->setStatsIValue(0);
				statsUpdate2[132]->setStatsName("S_0_ssfsuw");
				statsUpdate2[133]->setStatsIValue(0);
				statsUpdate2[133]->setStatsName("S_0_ssfsud");
				statsUpdate2[134]->setStatsIValue(0);
				statsUpdate2[134]->setStatsName("S_0_ssfsul");
				statsUpdate2[135]->setStatsIValue(0);
				statsUpdate2[135]->setStatsName("S_0_ssffxw");
				statsUpdate2[136]->setStatsIValue(0);
				statsUpdate2[136]->setStatsName("S_0_ssffxd");
				statsUpdate2[137]->setStatsIValue(0);
				statsUpdate2[137]->setStatsName("S_0_ssffxl");
				statsUpdate2[138]->setStatsIValue(0);
				statsUpdate2[138]->setStatsName("S_0_ssfnrw");
				statsUpdate2[139]->setStatsIValue(0);
				statsUpdate2[139]->setStatsName("S_0_ssfnrd");
				statsUpdate2[140]->setStatsIValue(0);
				statsUpdate2[140]->setStatsName("S_0_ssfnrl");
				statsUpdate2[141]->setStatsIValue(0);
				statsUpdate2[141]->setStatsName("S_0_ssflws");
				statsUpdate2[142]->setStatsIValue(0);
				statsUpdate2[142]->setStatsName("S_0_ssfcst");
				statsUpdate2[143]->setStatsIValue(0);
				statsUpdate2[143]->setStatsName("S_0_ssffgs");
				statsUpdate2[144]->setStatsIValue(0);
				statsUpdate2[144]->setStatsName("S_0_ssffgt");
				statsUpdate2[145]->setStatsIValue(0);
				statsUpdate2[145]->setStatsName("S_0_ssffgmt");
				statsUpdate2[146]->setStatsIValue(0);
				statsUpdate2[146]->setStatsName("S_0_ssffgot");

				for (int i = 0; i < 147; i++)
				{
					statUpdateList2.push_back(statsUpdate2[i]);
				}
			}
		}
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "KickOffPlayer::updateStats_GroupID - before updateStats" << type);
		err = mPlayerInfo->getComponentProxy()->mFifaStatsProxy->updateStats(updateStatsRequest, updateStatsResponse);
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "KickOffPlayer::updateStats_GroupID - after updateStats" << type);
		if (ERR_OK != err)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::fifastats, "KickOffPlayer::updateStats_GroupID:Overall: playerId " << mPlayerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
		}
	}
	// UserCustom for all player ids in the game
	else if (type == EADP_GROUPS_FRIEND_TYPE && mFriendsGroup_Name != "")
	{
		updateStatsRequest.setCategoryId("UserCustom");

		Blaze::FifaStats::Entity *entity1 = updateStatsRequest.getEntities().pull_back();
		Blaze::FifaStats::StatsUpdateList &statUpdateList1 = entity1->getStatsUpdates();
		Blaze::FifaStats::StatsUpdate *statsUpdate1;
		//Player1
		entity1->setEntityId(mFriendsGroup_Name.c_str());

		statsUpdate1 = entity1->getStatsUpdates().pull_back();
		statsUpdate1->setOperator(Blaze::FifaStats::StatsOperator::OPERATOR_REPLACE);
		statsUpdate1->setStatsFValue(0.00000000);
		statsUpdate1->setType(Blaze::FifaStats::StatsType::TYPE_INT);
		statsUpdate1->setStatsName("gplayed");
		if (!strcmp("gameType100", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
		{
			statsUpdate1->setStatsIValue(1);
		}
		if (!strcmp("gameType90", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
		{
			statsUpdate1->setStatsIValue(5);
		}
		statUpdateList1.push_back(statsUpdate1);

		switch (mScenarioType)
		{
			case TEAM_1_CPU:
			{
			}
			break;
			case TEAM_1_1:
			case TEAM_2_CPU:
			{
				Blaze::FifaStats::Entity *entity2 = updateStatsRequest.getEntities().pull_back();
				Blaze::FifaStats::StatsUpdateList &statUpdateList2 = entity2->getStatsUpdates();
				Blaze::FifaStats::StatsUpdate *statsUpdate2;
				//Player2
				if (mPlayersType == TEAM_ALL_REG)
				{
					entity2->setEntityId(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[0]).c_str());
				}
				else if (mPlayersType == TEAM_ALL_UNREG)
				{
					entity2->setEntityId(mUnRegisteredStringPlayersList[0].c_str());
				}
				statsUpdate2 = entity2->getStatsUpdates().pull_back();
				statsUpdate2->setOperator(Blaze::FifaStats::StatsOperator::OPERATOR_REPLACE);
				statsUpdate2->setStatsFValue(0.00000000);
				statsUpdate2->setType(Blaze::FifaStats::StatsType::TYPE_INT);
				statsUpdate2->setStatsName("gplayed");
				if (!strcmp("gameType100", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
				{
					statsUpdate2->setStatsIValue(1);
				}
				if (!strcmp("gameType90", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
				{
					statsUpdate2->setStatsIValue(2);
				}
				statUpdateList2.push_back(statsUpdate2);
			}
			break;
			case TEAM_1_2:
			{			
				Blaze::FifaStats::Entity *entity2 = updateStatsRequest.getEntities().pull_back();
				Blaze::FifaStats::StatsUpdateList &statUpdateList2 = entity2->getStatsUpdates();
				Blaze::FifaStats::StatsUpdate *statsUpdate2;
				//Player2
				if (mPlayersType == TEAM_ALL_REG)
				{
					entity2->setEntityId(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[0]).c_str());
				}
				else if (mPlayersType == TEAM_ALL_UNREG)
				{
					entity2->setEntityId(mUnRegisteredStringPlayersList[0].c_str());
				}

				statsUpdate2 = entity2->getStatsUpdates().pull_back();
				statsUpdate2->setOperator(Blaze::FifaStats::StatsOperator::OPERATOR_REPLACE);
				statsUpdate2->setStatsFValue(0.00000000);
				statsUpdate2->setType(Blaze::FifaStats::StatsType::TYPE_INT);
				statsUpdate2->setStatsName("gplayed");
				if (!strcmp("gameType100", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
				{
					statsUpdate2->setStatsIValue(1);
				}
				if (!strcmp("gameType90", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
				{
					statsUpdate2->setStatsIValue(2);
				}
				statUpdateList2.push_back(statsUpdate2);

				Blaze::FifaStats::Entity *entity3 = updateStatsRequest.getEntities().pull_back();
				Blaze::FifaStats::StatsUpdateList &statUpdateList3 = entity3->getStatsUpdates();
				Blaze::FifaStats::StatsUpdate *statsUpdate3;
				//Player3					
				if (mPlayersType == TEAM_ALL_REG)
				{
					entity3->setEntityId(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[1]).c_str());
				}
				else if (mPlayersType == TEAM_ALL_UNREG)
				{
					entity3->setEntityId(mUnRegisteredStringPlayersList[1].c_str());
				}

				statsUpdate3 = entity3->getStatsUpdates().pull_back();
				statsUpdate3->setOperator(Blaze::FifaStats::StatsOperator::OPERATOR_REPLACE);
				statsUpdate3->setStatsFValue(0.00000000);
				statsUpdate3->setType(Blaze::FifaStats::StatsType::TYPE_INT);
				statsUpdate3->setStatsName("gplayed");
				if (!strcmp("gameType100", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
				{
					statsUpdate3->setStatsIValue(1);
				}
				if (!strcmp("gameType90", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
				{
					statsUpdate3->setStatsIValue(2);
				}
				statUpdateList3.push_back(statsUpdate3);
			}
			break;
			case TEAM_2_2:
			{
				Blaze::FifaStats::Entity *entity2 = updateStatsRequest.getEntities().pull_back();
				Blaze::FifaStats::StatsUpdateList &statUpdateList2 = entity2->getStatsUpdates();
				Blaze::FifaStats::StatsUpdate *statsUpdate2;
				//Player2		
				if (mPlayersType == TEAM_ALL_REG)
				{
					entity2->setEntityId(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[0]).c_str());
				}
				else if (mPlayersType == TEAM_ALL_UNREG)
				{
					entity2->setEntityId(mUnRegisteredStringPlayersList[0].c_str());
				}

				statsUpdate2 = entity2->getStatsUpdates().pull_back();
				statsUpdate2->setOperator(Blaze::FifaStats::StatsOperator::OPERATOR_REPLACE);
				statsUpdate2->setStatsFValue(0.00000000);
				statsUpdate2->setType(Blaze::FifaStats::StatsType::TYPE_INT);
				statsUpdate2->setStatsName("gplayed");
				if (!strcmp("gameType100", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
				{
					statsUpdate2->setStatsIValue(1);
				}
				if (!strcmp("gameType90", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
				{
					statsUpdate2->setStatsIValue(2);
				}
				statUpdateList2.push_back(statsUpdate2);

				Blaze::FifaStats::Entity *entity3 = updateStatsRequest.getEntities().pull_back();
				Blaze::FifaStats::StatsUpdateList &statUpdateList3 = entity3->getStatsUpdates();
				Blaze::FifaStats::StatsUpdate *statsUpdate3;
				//Player3					
				if (mPlayersType == TEAM_ALL_REG)
				{
					entity3->setEntityId(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[1]).c_str());
				}
				else if (mPlayersType == TEAM_ALL_UNREG)
				{
					entity3->setEntityId(mUnRegisteredStringPlayersList[1].c_str());
				}

				statsUpdate3 = entity3->getStatsUpdates().pull_back();
				statsUpdate3->setOperator(Blaze::FifaStats::StatsOperator::OPERATOR_REPLACE);
				statsUpdate3->setStatsFValue(0.00000000);
				statsUpdate3->setType(Blaze::FifaStats::StatsType::TYPE_INT);
				statsUpdate3->setStatsName("gplayed");
				if (!strcmp("gameType100", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
				{
					statsUpdate3->setStatsIValue(1);
				}
				if (!strcmp("gameType90", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
				{
					statsUpdate3->setStatsIValue(2);
				}
				statUpdateList3.push_back(statsUpdate3);

				Blaze::FifaStats::Entity *entity4 = updateStatsRequest.getEntities().pull_back();
				Blaze::FifaStats::StatsUpdateList &statUpdateList4 = entity4->getStatsUpdates();
				Blaze::FifaStats::StatsUpdate *statsUpdate4;
				//Player4

				if (mPlayersType == TEAM_ALL_REG)
				{
					entity4->setEntityId(eastl::string().sprintf("%" PRIu64 "", mRegisteredPlayersList[2]).c_str());
				}
				else if (mPlayersType == TEAM_ALL_UNREG)
				{
					entity4->setEntityId(mUnRegisteredStringPlayersList[2].c_str());
				}

				statsUpdate4 = entity4->getStatsUpdates().pull_back();
				statsUpdate4->setOperator(Blaze::FifaStats::StatsOperator::OPERATOR_REPLACE);
				statsUpdate4->setStatsFValue(0.00000000);
				statsUpdate4->setType(Blaze::FifaStats::StatsType::TYPE_INT);
				statsUpdate4->setStatsName("gplayed");
				if (!strcmp("gameType100", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
				{
					statsUpdate4->setStatsIValue(1);
				}
				if (!strcmp("gameType90", mKickOffScenarioType.substr(0, (mKickOffScenarioType.length())).c_str()))
				{
					statsUpdate4->setStatsIValue(2);
				}
				statUpdateList4.push_back(statsUpdate4);
			}
			break;
			default:
			{
				BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[KickOffPlayer::updateStats_GroupID]" << mPlayerInfo->getPlayerId() << " In default state");
				return false;
			}
			break;
		}
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "KickOffPlayer::updateStats_GroupID - Before updateStats" << type);
		err = mPlayerInfo->getComponentProxy()->mFifaStatsProxy->updateStats(updateStatsRequest, updateStatsResponse);
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "KickOffPlayer::updateStats_GroupID - After updateStats" << type);
		if (ERR_OK != err)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::fifastats, "KickOffPlayer::updateStats_GroupID:UserCustom: playerId " << mPlayerInfo->getPlayerId() << " failed with error = " << Blaze::ErrorHelp::getErrorName(err));
		}
	}

	/*char8_t buf1[80240];
	BLAZE_TRACE_LOG(BlazeRpcLog::fifagroups, "KickOffPlayer::updateStats_GroupID: playerId " << mPlayerInfo->getPlayerId() << "]::update Stats" << "\n" << updateStatsRequest.print(buf1, sizeof(buf1)));

	char8_t buf[1024];
	BLAZE_TRACE_LOG(BlazeRpcLog::fifagroups, "KickOffPlayer::updateStats_GroupID: playerId " << mPlayerInfo->getPlayerId() << "]::updateStatsResponse" << "\n" << updateStatsResponse.print(buf, sizeof(buf)));*/

	return err;
}

void KickOffPlayer::removePlayerFromList()
{
    //  remove player from universal list first
    Player::removePlayerFromList();
	EA::Thread::AutoMutex am(kickOffPlayerListMutex);
    const UserIdentMap::iterator plyerItr = kickOffPlayerList.find(mPlayerInfo->getPlayerId());
    if (plyerItr != kickOffPlayerList.end())
    {
		kickOffPlayerList.erase(plyerItr);
    }
}

void KickOffPlayer::addPlayerToList()
{
    //  add player to universal list first
    Player::addPlayerToList();
    UserIdentPtr uid(BLAZE_NEW UserIdentification());
    uid->setName(mPlayerInfo->getPersonaName().c_str());
    uid->setBlazeId(mPlayerInfo->getPlayerId());
    {
        EA::Thread::AutoMutex am(kickOffPlayerListMutex);
		kickOffPlayerList.insert(eastl::pair<BlazeId, UserIdentPtr>(mPlayerInfo->getPlayerId(), uid));
    }
}
//   This function would be called from StressInstance::login()
void KickOffPlayer::onLogin(Blaze::BlazeRpcError result)
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

void KickOffPlayer::gameTypeSetter()
{
	KickOffDistributionMap distributionReference = StressConfigInst.getKickOffScenarioDistribution();
	KickOffDistributionMap::iterator it = distributionReference.begin();
	uint8_t sumProb = 0;
	uint8_t randNumber = (uint8_t)Random::getRandomNumber(100);
	while (it != distributionReference.end())
	{
		sumProb += it->second;
		if (randNumber < sumProb)
		{
			mKickOffScenarioType = it->first;
			break;
		}
		it++;
	}
}

}  // namespace Stress
}  // namespace Blaze
