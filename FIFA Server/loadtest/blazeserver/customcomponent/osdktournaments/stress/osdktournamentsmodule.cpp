/*************************************************************************************************/
/*!
    \file osdktournamentsmodule.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "osdktournamentsmodule.h"
#include "framework/config/config_map.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/util/random.h"
#include "loginmanager.h"
#include "osdktournaments/tdf/osdktournamentstypes.h"

#ifdef STATIC_MYSQL_DRIVER
#include "framework/database/mysql/blazemysql.h"
#endif

#define WIPEOUT_TABLE(table)  { BLAZE_INFO_LOG(BlazeRpcLog::osdktournaments, "Deleting all entries from " table " and resetting auto increment."); \
    int error = mysql_query(&mysql, "TRUNCATE " table "; ALTER TABLE " table " AUTO_INCREMENT=1;"); \
    if (error) \
    { \
        BLAZE_WARN_LOG(BlazeRpcLog::osdktournaments, "Error: " << mysql_error(&mysql)); \
        mysql_close(&mysql); \
        mysql_library_end(); \
        return false; \
    } \
    resetCommand(&mysql); }

namespace Blaze
{
namespace Stress
{

/*
* List of available actions that can be performed by the module.
*/
static const char8_t* ACTION_STR_JOIN_TOURNAMENT = "join";
static const char8_t* ACTION_STR_LEAVE_TOURNAMENT = "leave";
static const char8_t* ACTION_STR_GET_TOURNAMENTS = "getTournaments";
static const char8_t* ACTION_STR_GET_MY_TOURNAMENT_ID = "getTournId";
static const char8_t* ACTION_STR_GET_MY_TOURNAMENT_DETAILS = "getTournDetails";
static const char8_t* ACTION_STR_RESET_TOURNAMENT = "reset";
static const char8_t* ACTION_STR_PLAY_MATCH = "playMatch";
static const char8_t* ACTION_STR_GET_TROPHIES = "getTrophies";
static const char8_t* ACTION_STR_GET_MEMBER_COUNTS = "getMemberCounts";
static const char8_t* ACTION_STR_SIMULATE_PRODUCTION = "simulateProd";
static const char8_t* ACTION_STR_SLEEP = "sleep";

static const uint32_t MAX_TROPHIES = 10;

using namespace Blaze::OSDKTournaments;

/*** Public Methods ******************************************************************************/
// static
StressModule* OSDKTournamentsModule::create()
{
    return BLAZE_NEW OSDKTournamentsModule();
}

OSDKTournamentsModule::~OSDKTournamentsModule() 
{
	//clean up elems in mTournamentPlayerIds
	OSDKTournamentsModule::BlazeIdsByTournamentId::iterator it = mBlazeIdsByTournamentIdMap.begin();
	OSDKTournamentsModule::BlazeIdsByTournamentId::iterator itEnd = mBlazeIdsByTournamentIdMap.end();
	for (; it != itEnd; ++it)
	{
		OSDKTournamentsModule::PlayerBlazeIdSet* ptr = it->second;
		delete ptr;
	}	
}

// -------------------------------------------------------------------------
#ifdef STATIC_MYSQL_DRIVER

typedef struct {
   BlazeId blazeId;
   uint32_t frequency;
} PlayerIdFrequency;


static void resetCommand(MYSQL *mysql)
{
    do
    {
        MYSQL_RES *res = mysql_store_result(mysql);
        if (res)
            mysql_free_result(res);
        else
            mysql_affected_rows(mysql);
    } while (mysql_next_result(mysql) == 0);
}
#endif

bool OSDKTournamentsModule::populateDatabase()
{
#ifdef STATIC_MYSQL_DRIVER
	
	typedef eastl::vector<PlayerIdFrequency*> PlayerBlazeIdVector;
	PlayerBlazeIdVector playerBlazeIdVector;

    MYSQL mysql;

    mysql_init(&mysql);

    bool bSuccess = mysql_real_connect(
        &mysql,
        mDbHost.c_str(),
        mDbUser.c_str(),
        mDbPass.c_str(),
        mDatabase.c_str(),
        mDbPort,
        NULL,
        CLIENT_MULTI_STATEMENTS) != NULL;

    if (!bSuccess)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "Could not connect to db: " << mysql_error(&mysql));
        mysql_close(&mysql);
        mysql_library_end();
        return false;
    }

    BLAZE_INFO_LOG(BlazeRpcLog::osdktournaments, "Connected to db.");

	if (mWipeTables)
	{
		WIPEOUT_TABLE("osdktournaments_trophies");
		WIPEOUT_TABLE("osdktournaments_matches");
		WIPEOUT_TABLE("osdktournaments_members");		
	}	

	// populate the playerBlazeIdVector and insert into osdktournaments_members table first
    for (BlazeId userIndex = 0; userIndex < (BlazeId)mTotalPlayers; ++userIndex)
	{
		BlazeId effectiveBlazeId = userIndex + mPlayerBlazeIdBase;

		PlayerIdFrequency * ptrPlayerIdFrequencyInstance = BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "OSDKTournamentsModule::PlayerIdFrequency") PlayerIdFrequency;
		ptrPlayerIdFrequencyInstance->blazeId = effectiveBlazeId;
		ptrPlayerIdFrequencyInstance->frequency = 0;

		playerBlazeIdVector.push_back(ptrPlayerIdFrequencyInstance);	
		
		//randomize which tournament to pick for its member
		uint32_t playerTournamentIdPos = Random::getRandomNumber(static_cast<int32_t>(mTournamentsIdVector.size()));
		Blaze::OSDKTournaments::TournamentId playerTournamentId = mTournamentsIdVector[playerTournamentIdPos];

        eastl::string statement = "INSERT INTO `osdktournaments_members` (`blazeId`, `tournamentId`, `attribute`, `isActive`, `team`) VALUES";

		eastl::string argument;
		argument.sprintf(
			"(%d, %d, '%s', %d, %d)", 
			effectiveBlazeId, 
			playerTournamentId, 
			"playerAttrib",
			1,
			-1);

		statement.append(argument);	

		// insert entries into tour_matches
        int error = mysql_query(&mysql, statement.c_str());
        if (error)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "Error: " << mysql_error(&mysql));
            BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "Last statement: " << statement.c_str());
            mysql_close(&mysql);
            mysql_library_end();
            return false;
        }
	}
	
	// randomly select two non-equal entries from playerBlazeIdVector
	// also randomly pick non-equal scores for the two players
    for (int32_t playerBlazeIdVectorSize = playerBlazeIdVector.size(); playerBlazeIdVectorSize > 1; playerBlazeIdVectorSize = playerBlazeIdVector.size())
    {
		// pick player1's attribute
        int32_t player1_pos = Random::getRandomNumber(static_cast<int32_t>(playerBlazeIdVectorSize));
		int32_t player1_score = Random::getRandomNumber(100) + 1;

		// pick player2's attribute
		int32_t player2_pos = Random::getRandomNumber(static_cast<int32_t>(playerBlazeIdVectorSize));
		int32_t player2_score = Random::getRandomNumber(100) + 1;
		while (player2_pos == player1_pos)
		{
			player2_pos = Random::getRandomNumber(static_cast<int32_t>(playerBlazeIdVectorSize));
		}
		while (player2_score == player1_score)
		{
			player2_score = Random::getRandomNumber(100) + 1;
		}
       
		
		//----------------------------------------------------------------------------------
		// compose the INSERT sql statement for player1 being the treeOwner
        eastl::string statement = "INSERT INTO `osdktournaments_matches` (`memberOneBlazeId`, `memberTwoBlazeId`, `memberOneTeam`, `memberTwoTeam`, `memberOneScore`, `memberTwoScore`, `memberOneLastMatchId`, `memberTwoLastMatchId`, `memberOneAttribute`, `memberTwoAttribute`, `memberOneMetaData`, `memberTwoMetaData`, `treeOwnerBlazeId`) VALUES";

		eastl::string argument;
		argument.sprintf(
			"(%" PRId64 ", %" PRId64 ", %d, %d, %d, %d, %d, %d, '%s', '%s', '%s', '%s', %" PRId64 ")", 
			playerBlazeIdVector[player1_pos]->blazeId, playerBlazeIdVector[player2_pos]->blazeId, 
			1, 2, 
			player1_score, player2_score,
			1, 2,
			"user1Attrib","user2Attrib",
			"user1MetaData","user2MetaData", 
			playerBlazeIdVector[player1_pos]->blazeId);

		statement.append(argument);	

		// insert entries into tour_matches
        int error = mysql_query(&mysql, statement.c_str());
        if (error)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "Error: " << mysql_error(&mysql));
            BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "Last statement: " << statement.c_str());
            mysql_close(&mysql);
            mysql_library_end();
            return false;
        }
		
		//----------------------------------------------------------------------------------
		// compose the INSERT sql statement for player2 being the treeOwner
        statement = "INSERT INTO `osdktournaments_matches` (`memberOneBlazeId`, `memberTwoBlazeId`, `memberOneTeam`, `memberTwoTeam`, `memberOneScore`, `memberTwoScore`, `memberOneLastMatchId`, `memberTwoLastMatchId`, `memberOneAttribute`, `memberTwoAttribute`, `memberOneMetaData`, `memberTwoMetaData`, `treeOwnerBlazeId`) VALUES";

		argument.sprintf(
			"(%d, %d, %d, %d, %d, %d, %d, %d, '%s', '%s', '%s', '%s', %d)", 
			playerBlazeIdVector[player2_pos]->blazeId, playerBlazeIdVector[player1_pos]->blazeId, 
			2, 1, 
			player2_score, player1_score,
			2, 1,
			"user2Attrib","user1Attrib",
			"user2MetaData","user1MetaData", 
			playerBlazeIdVector[player2_pos]->blazeId);

		statement.append(argument);	

		// insert entries into tour_matches
        error = mysql_query(&mysql, statement.c_str());
        if (error)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "Error: " << mysql_error(&mysql));
            BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "Last statement: " << statement.c_str());
            mysql_close(&mysql);
            mysql_library_end();
            return false;
        }
		////////////////////////////////////////////////////////////////////////////////

		// update freq and remove item if frequency > max
		playerBlazeIdVector[player1_pos]->frequency = playerBlazeIdVector[player1_pos]->frequency + 1;
		playerBlazeIdVector[player2_pos]->frequency = playerBlazeIdVector[player2_pos]->frequency + 1;

		// remove entries from playerBlazeIdVector that have enough games populated already
		bool bDeletedPlayer1 = false;
		if (playerBlazeIdVector[player1_pos]->frequency >= mNumGamesToPopulatePerPlayer)
		{
			delete playerBlazeIdVector[player1_pos];
			playerBlazeIdVector.erase(playerBlazeIdVector.begin()+player1_pos);				
			bDeletedPlayer1 = true;
		}

		// take the offset into account for player2_pos if player1 has been removed 
		// and it's before player2_pos
		if (bDeletedPlayer1 && player2_pos > player1_pos)
		{
			player2_pos--;					
		}

		if (playerBlazeIdVector[player2_pos]->frequency >= mNumGamesToPopulatePerPlayer)
		{				
			delete playerBlazeIdVector[player2_pos];
			playerBlazeIdVector.erase(playerBlazeIdVector.begin()+player2_pos);		
		}

        resetCommand(&mysql);    		
    }

    mysql_close(&mysql);
    mysql_library_end();

	// delete the struct
	PlayerBlazeIdVector::iterator  it = playerBlazeIdVector.begin();
	PlayerBlazeIdVector::iterator itEnd = playerBlazeIdVector.end();
	for (; it != itEnd; ++it)
	{
		PlayerIdFrequency * ptr = *it;
		delete ptr;
	}	

    return true;
#else
BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "MySQL driver unavailable (did you forget to set STATIC_MYSQL_DRIVER=1?)");
    return false;
#endif
}
// -------------------------------------------------------------------------



bool OSDKTournamentsModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    if (!StressModule::initialize(config, bootUtil))
    {
        return false;
    }

    const char8_t* action = config.getString("action", ACTION_STR_GET_TOURNAMENTS);
    mAction = stringToAction(action);
    if (mAction == ACTION_MAX_ENUM)
    {
        return false;
    }

    BLAZE_INFO_LOG(BlazeRpcLog::osdktournaments, "OSDKTournamentsModule: " << action << " action selected.");
    memset(&mInTourneyProbabilities[0], 0, sizeof(uint32_t) * ACTION_MAX_ENUM);
    memset(&mNotInTourneyProbabilities[0], 0, sizeof(uint32_t) * ACTION_MAX_ENUM);

    const ConfigMap* simMap = config.getMap("sim.notInTournament");
    if (simMap != NULL)
    {
        uint32_t totalProbability = 0;
        while (simMap->hasNext())
        {
            const char8_t* actionKeyStr = simMap->nextKey();
            uint32_t actionKeyVal = (uint32_t)stringToAction(actionKeyStr);
            if (actionKeyVal == ACTION_MAX_ENUM)
            {
                BLAZE_WARN_LOG(BlazeRpcLog::osdktournaments, "action " << actionKeyStr << " is not a valid action string");
                continue;
            }
            uint32_t actionVal = simMap->getUInt32(actionKeyStr, 0);
            mNotInTourneyProbabilities[actionKeyVal] = actionVal;
            totalProbability += actionVal;
        }

        if (totalProbability > 100)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "OSDKTournamentsModule: in tournament probablities combined are greater than 100");
            return false;
        }

        for (int i = 1; i < ACTION_MAX_ENUM; ++i)
        {
            mNotInTourneyProbabilities[i] = mNotInTourneyProbabilities[i] + mNotInTourneyProbabilities[i-1];
        }
    }

    simMap = config.getMap("sim.inTournament");
    if (simMap != NULL)
    {
        uint32_t totalProbability = 0;
        while (simMap->hasNext())
        {
            const char8_t* actionKeyStr = simMap->nextKey();
            uint32_t actionKeyVal = (uint32_t)stringToAction(actionKeyStr);
            if (actionKeyVal == ACTION_MAX_ENUM)
            {
                BLAZE_WARN_LOG(BlazeRpcLog::osdktournaments, "action " << actionKeyStr << " is not a valid action string");
                continue;
            }
            uint32_t actionVal = simMap->getUInt32(actionKeyStr, 0);
            mInTourneyProbabilities[actionKeyVal] = actionVal;
            totalProbability += actionVal;
        }

        if (totalProbability > 100)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "OSDKTournamentsModule: in tournament probablities combined are greater than 100");
            return false;
        }

        for (int i = 1; i < ACTION_MAX_ENUM; ++i)
        {
            mInTourneyProbabilities[i] = mInTourneyProbabilities[i] + mInTourneyProbabilities[i-1];
        }
    }

    simMap = config.getMap("sim");
    if (simMap != NULL)
    {
        mMaxSleepTimeSecs = simMap->getUInt32("maxSleepSecs", 5);
    }

    mTournamentMode = config.getUInt32("TournamentMode", 1);

	// pickup database parameters
    mDbHost = config.getString("dbhost", "");
    mDbPort = config.getUInt32("dbport", 0);
    mDatabase = config.getString("database", "");
    mDbUser = config.getString("dbuser", "");
    mDbPass = config.getString("dbpass", "");
    mPopulateDatabase = config.getBool("populateDatabase", false);
    mTotalPlayers = config.getUInt64("totalPlayers", 0);
	mPlayerBlazeIdBase = config.getInt32("playerBlazeIdBase", 0);	//still 32 bit for 2.x
	mNumGamesToPopulatePerPlayer = config.getUInt32("numGamesToPopulatePerPlayer", 0);
    mWipeTables = config.getBool("wipeTables", true);
	
	if (mPopulateDatabase)
	{
		//populate the tournaments Id vector list
		//note: having more than one tournament id may lead to having to insert into db 
		//of matches for players that are not necessarily in the same tournament 
		//eg. two players may get inserted into the osdktournaments_matches table where they may be players
		//from two different tournament => data integrity problem
		//however for the sake of stressing the db it is fine
		const ConfigSequence* tournamentsIdsToUseList = config.getSequence("tournamentsIdsToUse");
		if (tournamentsIdsToUseList == NULL)
		{
            BLAZE_ERR_LOG(Log::SYSTEM, "No tournamentsId configured.");
			return false;
		}		

		while (tournamentsIdsToUseList->hasNext())
		{			
			mTournamentsIdVector.push_back(tournamentsIdsToUseList->nextInt32(0));
		}

		//populate database
		if (!populateDatabase())
		{
			BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "Could not populate database.");
			return false;
		}

		BLAZE_INFO_LOG(BlazeRpcLog::osdktournaments, "Finished data population.");
	}
	

	
    return true;
}

StressInstance* OSDKTournamentsModule::createInstance(StressConnection* connection, Login* login)
{
    return BLAZE_NEW OSDKTournamentsInstance(this, connection, login);
}

OSDKTournamentsModule::Action OSDKTournamentsModule::getWeightedAction(bool isInTournament) const
{
    uint32_t rand = static_cast<uint32_t>(Blaze::Random::getRandomNumber(100));
    if (isInTournament)
    {
        for (int i = 0; i < ACTION_MAX_ENUM; ++i)
        {
            uint32_t current = mInTourneyProbabilities[i];
            if (rand < current)
            {
                return static_cast<OSDKTournamentsModule::Action>(i);
            }
        }
    }
    else
    {
        for (int i = 0; i < ACTION_MAX_ENUM; ++i)
        {
            uint32_t current = mNotInTourneyProbabilities[i];
            if (rand < current)
            {
                return static_cast<OSDKTournamentsModule::Action>(i);
            }
        }
    }

    return ACTION_MAX_ENUM;
}


OSDKTournamentsModule::OSDKTournamentsModule()
:   mAction(ACTION_GET_TOURNAMENTS),
    mMaxSleepTimeSecs(0),
	mMinGameLengthSecs(0),
	mMaxGameLengthSecs(0),	
	mPopulateDatabase(false),
	mDbHost(""),
	mDbPort(0),
	mDatabase(""),
	mDbUser(""),
	mDbPass(""),
	mTotalPlayers(0),
	mNumGamesToPopulatePerPlayer(0),
	mPlayerBlazeIdBase(0),
	mWipeTables(false)
{
	Random::initializeSeed();	
}

OSDKTournamentsModule::Action OSDKTournamentsModule::stringToAction(const char8_t* action)
{
    if (blaze_stricmp(action, ACTION_STR_JOIN_TOURNAMENT) == 0)
    {
        return ACTION_JOIN_TOURNAMENT;
    }
    if (blaze_stricmp(action, ACTION_STR_LEAVE_TOURNAMENT) == 0)
    {
        return ACTION_LEAVE_TOURNAMENT;
    }
    else if (blaze_stricmp(action, ACTION_STR_GET_TOURNAMENTS) == 0)
    {
        return ACTION_GET_TOURNAMENTS;
    }
    else if (blaze_stricmp(action, ACTION_STR_GET_MY_TOURNAMENT_ID) == 0)
    {
        return ACTION_GET_MY_TOURNAMENT_ID;
    }
    else if (blaze_stricmp(action, ACTION_STR_GET_MY_TOURNAMENT_DETAILS) == 0)
    {
        return ACTION_GET_MY_TOURNAMENT_DETAILS;
    }
    else if (blaze_stricmp(action, ACTION_STR_RESET_TOURNAMENT) == 0)
    {
        return ACTION_RESET_TOURNAMENT;
    }
    else if (blaze_stricmp(action, ACTION_STR_PLAY_MATCH) == 0)
    {
        return ACTION_PLAY_MATCH;
    }
    else if (blaze_stricmp(action, ACTION_STR_GET_TROPHIES) == 0)
    {
        return ACTION_GET_TROPHIES;
    }
    else if (blaze_stricmp(action, ACTION_STR_GET_MEMBER_COUNTS) == 0)
    {
        return ACTION_GET_MEMBER_COUNTS;
    }
    else if (blaze_stricmp(action, ACTION_STR_SIMULATE_PRODUCTION) == 0)
    {
        return ACTION_SIMULATE_PRODUCTION;
    }
    else if (blaze_stricmp(action, ACTION_STR_SLEEP) == 0)
    {
        return ACTION_SLEEP;
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "OSDKTournamentsModule: unrecognized action: '" << action << "'");
        return ACTION_MAX_ENUM;
    }
}

const char8_t* OSDKTournamentsModule::actionToString(OSDKTournamentsModule::Action action)
{
    switch (action)
    {
        case OSDKTournamentsModule::ACTION_JOIN_TOURNAMENT:
            {
                return ACTION_STR_JOIN_TOURNAMENT;
            }
        case OSDKTournamentsModule::ACTION_LEAVE_TOURNAMENT:
            {
                return ACTION_STR_LEAVE_TOURNAMENT;
            }
        case OSDKTournamentsModule::ACTION_GET_TOURNAMENTS:
            {
                return ACTION_STR_GET_TOURNAMENTS;
            }
        case OSDKTournamentsModule::ACTION_GET_MY_TOURNAMENT_ID:
            {
                return ACTION_STR_GET_MY_TOURNAMENT_ID;
            }
        case OSDKTournamentsModule::ACTION_GET_MY_TOURNAMENT_DETAILS:
            {
                return ACTION_STR_GET_MY_TOURNAMENT_DETAILS;
            }
        case OSDKTournamentsModule::ACTION_RESET_TOURNAMENT:
            {
                return ACTION_STR_RESET_TOURNAMENT;
            }
        case OSDKTournamentsModule::ACTION_PLAY_MATCH:
            {
                return ACTION_STR_PLAY_MATCH;
            }
        case OSDKTournamentsModule::ACTION_GET_TROPHIES:
            {
                return ACTION_STR_GET_TROPHIES;
            }
        case OSDKTournamentsModule::ACTION_GET_MEMBER_COUNTS:
            {
                return ACTION_STR_GET_MEMBER_COUNTS;
            }
        case OSDKTournamentsModule::ACTION_SLEEP:
            {
                return ACTION_STR_SLEEP;
            }
        case OSDKTournamentsModule::ACTION_SIMULATE_PRODUCTION:
            {
                return ACTION_STR_SIMULATE_PRODUCTION;
            }
        case OSDKTournamentsModule::ACTION_MAX_ENUM:
            {
                return "";
            }
    }
    return "";
}

//add players whose is active to the tournament map
void OSDKTournamentsModule::addPlayerToTournamentPlayerIdMap(BlazeId playerId, Blaze::OSDKTournaments::TournamentId tourneyId)
{
	OSDKTournamentsModule::PlayerBlazeIdSet *& playerBlazeIdSet = mBlazeIdsByTournamentIdMap[tourneyId];	

	//if the cache doesn't have the tournament set, insert it
	if (!playerBlazeIdSet)
	{	
		playerBlazeIdSet = BLAZE_NEW_MGID(DEFAULT_BLAZE_MEMGROUP, "OSDKTournamentsModule::PlayerBlazeIdSet") OSDKTournamentsModule::PlayerBlazeIdSet();		
	}

	BLAZE_TRACE_LOG(BlazeRpcLog::osdktournaments, "addPlayerToTournamentPlayerIdMap(): tournament:" << tourneyId << " playerID map is having " << playerBlazeIdSet->size() << " players");
	playerBlazeIdSet->insert(playerId);			
}

//remove players from the tournament map (ie. who became inactive)
void OSDKTournamentsModule::removePlayerFromTournamentPlayerIdMap(BlazeId playerId, Blaze::OSDKTournaments::TournamentId tourneyId)
{
	OSDKTournamentsModule::PlayerBlazeIdSet * playerBlazeIdSet = mBlazeIdsByTournamentIdMap[tourneyId];
	if (playerBlazeIdSet)
	{
        BLAZE_TRACE_LOG(BlazeRpcLog::osdktournaments, "removePlayerFromTournamentPlayerIdMap(): tournament:" << tourneyId << " playerID map is having " << playerBlazeIdSet->size() << " players");
		playerBlazeIdSet->erase(playerId);				
	}	
}

// reserve active tournament users for playmatch
// the call will reserve (remove from the playermap) both yourself and the opponent if qualifies
bool OSDKTournamentsModule::reservePlayersForPlaymatch(Blaze::OSDKTournaments::TournamentId tourneyId, BlazeId selfBlazeId, BlazeId & oppBlazeId)
{
	oppBlazeId = INVALID_BLAZE_ID;

	//find the player set in the tournament player map
	OSDKTournamentsModule::PlayerBlazeIdSet * playerBlazeIdSet = mBlazeIdsByTournamentIdMap[tourneyId];

	//if the player map doesn't have the tournament set, err
	if (!playerBlazeIdSet)
	{
        BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "Player:" << selfBlazeId << " cannot find tournament: " << tourneyId << ". ");
return false;		
	}
	
	OSDKTournamentsModule::PlayerBlazeIdSet::const_iterator it = playerBlazeIdSet->begin();
	OSDKTournamentsModule::PlayerBlazeIdSet::const_iterator itEnd = playerBlazeIdSet->end();

	int32_t playerBlazeIdsSize = (int32_t)playerBlazeIdSet->size();
	BLAZE_TRACE_LOG(BlazeRpcLog::osdktournaments, "reservePlayersForPlaymatch(): tournament:" << tourneyId << " playerID map is having " << playerBlazeIdsSize << " players");

	//bail out if the size is NOT >= 2 (min two players);
	if (playerBlazeIdsSize <= 1)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::osdktournaments, "Player:" << selfBlazeId << " bailing out to submit a tournament report as the number of players <= 1 for tournament " << tourneyId << " !");
		return false;
	}

	//loop until the opp blaze id != selfBlazeId; odd should be slim given high PSU and small tournament set
	for (; it != itEnd; ++it)
	{
		oppBlazeId = *it;
		if (oppBlazeId != selfBlazeId)
		{			
			break;
		}
	}

	// make sure we didn't find yourself!
	if (oppBlazeId != selfBlazeId)
	{
		//oppBlazeId always is the loser, remove him
		removePlayerFromTournamentPlayerIdMap(oppBlazeId, tourneyId);	

		//remove yourself too from the map so other instances won't choose you
		removePlayerFromTournamentPlayerIdMap(selfBlazeId, tourneyId);	

		return true;
	}
	
	return false;
}


OSDKTournamentsInstance::OSDKTournamentsInstance(OSDKTournamentsModule* owner, StressConnection* connection, Login* login)
:   StressInstance(owner, connection, login, BlazeRpcLog::osdktournaments),
	mOwner(owner),
    mTournSlave(BLAZE_NEW OSDKTournamentsSlave(*connection)),
    mTournamentId(INVALID_TOURNAMENT_ID),
	mUserId(INVALID_BLAZE_ID)
{
    mName = "";

}

void OSDKTournamentsInstance::onLogin(Blaze::BlazeRpcError result)
{
	if (result != ERR_OK)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "onLogin: login failed");		
	}

	//saves off the player's blazeId when login is successful
	mUserId = getLogin()->getUserLoginInfo()->getBlazeId();

    GetTournamentsRequest request;
    request.setMode(mOwner->getTournamentMode()); 
    GetTournamentsResponse response;

	//get the tournament list via getTournaments rpc
    if (mTournSlave->getTournaments(request, response) == ERR_OK)
    {
        TournamentDataList::iterator itr = response.getTournamentList().begin();
        TournamentDataList::iterator itrEnd = response.getTournamentList().end();

        for (; itr != itrEnd; ++itr)
        {
            TournamentData* tourn = *itr;
            mTournamentIds.push_back(tourn->getTournamentId());
        }
    }

    GetMyTournamentIdRequest idRequest;
    idRequest.setMode(mOwner->getTournamentMode()); 
    GetMyTournamentIdResponse idResponse;

    if (mTournSlave->getMyTournamentId(idRequest, idResponse) == ERR_OK)
    {
        mTournamentId = idResponse.getTournamentId();
		//only add player to the player blaze id map if he's active (not a losing player)
		if (idResponse.getIsActive())
		{
			((OSDKTournamentsModule*)getOwner())->addPlayerToTournamentPlayerIdMap(mUserId, mTournamentId);
		}		
    }
}

BlazeRpcError OSDKTournamentsInstance::execute()
{
    BlazeRpcError result = ERR_OK;
    OSDKTournamentsModule::Action action = ((OSDKTournamentsModule*)getOwner())->getAction();
    if (action == OSDKTournamentsModule::ACTION_SIMULATE_PRODUCTION)
    {
        bool isInTournament = mTournamentId != INVALID_TOURNAMENT_ID;
        action = ((OSDKTournamentsModule*)getOwner())->getWeightedAction(isInTournament);
        BLAZE_TRACE_LOG(BlazeRpcLog::osdktournaments, "IsInTournament: " << isInTournament << ", Choosing action: " << (OSDKTournamentsModule::actionToString(action)));
    }
    switch (action)
    {
        case OSDKTournamentsModule::ACTION_JOIN_TOURNAMENT:
        {
            mName = ACTION_STR_JOIN_TOURNAMENT;
            if (mTournamentId == INVALID_TOURNAMENT_ID)
            {
                result = joinTournament();
            }
            break;
        }
        case OSDKTournamentsModule::ACTION_LEAVE_TOURNAMENT:
        {
            mName = ACTION_STR_LEAVE_TOURNAMENT;
            if (mTournamentId != INVALID_TOURNAMENT_ID)
            {
                result = leaveTournament();
            }
            break;
        }
        case OSDKTournamentsModule::ACTION_GET_TOURNAMENTS:
        {
            GetTournamentsRequest request;
			request.setMode(mOwner->getTournamentMode()); 
            GetTournamentsResponse response;
            result = mTournSlave->getTournaments(request, response);
            mName = ACTION_STR_GET_TOURNAMENTS;
            break;
        }
        case OSDKTournamentsModule::ACTION_GET_MY_TOURNAMENT_ID:
        {
            mName = ACTION_STR_GET_MY_TOURNAMENT_ID;
            GetMyTournamentIdRequest request;
		    request.setMode(mOwner->getTournamentMode()); 
            GetMyTournamentIdResponse response;
            result = mTournSlave->getMyTournamentId(request, response);
            break;
        }
        case OSDKTournamentsModule::ACTION_GET_MY_TOURNAMENT_DETAILS:
        {
            mName = ACTION_STR_GET_MY_TOURNAMENT_DETAILS;
            GetMyTournamentDetailsRequest request;
		    request.setMode(mOwner->getTournamentMode()); 
            GetMyTournamentDetailsResponse response;
            result = mTournSlave->getMyTournamentDetails(request, response);
            break;
        }
        case OSDKTournamentsModule::ACTION_RESET_TOURNAMENT:
        {
            mName = ACTION_STR_RESET_TOURNAMENT;
            ResetTournamentRequest request;
            mOwner->getTournamentMode();
            result = mTournSlave->resetTournament(request);
			if (result == ERR_OK)
			{
                BLAZE_TRACE_LOG(BlazeRpcLog::osdktournaments, "[" << getConnection()->getIdent() << "] OSDKTournamentsInstance::resetTournament() PlayerID:" << mUserId << " successfully resetted for tournament " << mTournamentId << "!");
			}

            break;
        }
        case OSDKTournamentsModule::ACTION_PLAY_MATCH:
        {
            mName = ACTION_STR_PLAY_MATCH;
			result = playMatch();
            break;
        }
        case OSDKTournamentsModule::ACTION_GET_TROPHIES:
        {
            mName = ACTION_STR_GET_TROPHIES;
            result = getTrophies();
            break;
        }
        case OSDKTournamentsModule::ACTION_GET_MEMBER_COUNTS:
        {
            mName = ACTION_STR_GET_MEMBER_COUNTS;
            GetMemberCountsRequest request;
            if (!mTournamentIds.empty())
            {
                int32_t index = Blaze::Random::getRandomNumber(static_cast<int32_t>(mTournamentIds.size()));
                Blaze::OSDKTournaments::TournamentId tournamentId = mTournamentIds.at(index);
                request.setTournamentId(tournamentId);
                GetMemberCountsResponse response;
                result = mTournSlave->getMemberCounts(request, response);
            }

            break;
        }
        case OSDKTournamentsModule::ACTION_SLEEP:
        {
            mName = ACTION_STR_SLEEP;
            sleep(0, ((OSDKTournamentsModule*)getOwner())->getMaxSleepTimeSecs());
            break;
        }
        case OSDKTournamentsModule::ACTION_SIMULATE_PRODUCTION:
        {
            //wont ever get in here, but have the case to get rid of compiler warning.
            break;
        }
        case OSDKTournamentsModule::ACTION_MAX_ENUM:
        {
            break;
        }
    }
    return result;
}

void OSDKTournamentsInstance::sleep(uint32_t minSleepTimeSecs, uint32_t maxSleepTimeSecs)
{
	uint32_t sleepTimeSecs = 0;

	//randomize only when min/max sleep times are not the same
	if (maxSleepTimeSecs != minSleepTimeSecs)
	{
		sleepTimeSecs = minSleepTimeSecs + Random::getRandomNumber(maxSleepTimeSecs-minSleepTimeSecs);		
	}
	else
	{
		sleepTimeSecs = minSleepTimeSecs;		
	}

	BLAZE_TRACE_LOG(BlazeRpcLog::osdktournaments, "[" << getConnection()->getIdent() << "] OSDKTournamentsInstance sleeping for " << sleepTimeSecs << " s!");
			
	StressInstance::sleep(sleepTimeSecs*1000);	
}

bool OSDKTournamentsInstance::isActive()
{ 
    Blaze::OSDKTournaments::GetMyTournamentIdRequest request;
    request.setMode(mOwner->getTournamentMode());
	Blaze::OSDKTournaments::GetMyTournamentIdResponse response;
	if (mTournSlave->getMyTournamentId(request, response) == ERR_OK)
	{
		return response.getIsActive();		
	}
	else
	{
		return false;
	}
}

BlazeRpcError OSDKTournamentsInstance::joinTournament()
{
    if (mTournamentIds.empty())
    {
        return OSDKTOURN_ERR_TOURNAMENT_NOT_FOUND;
    }

    int32_t index = Blaze::Random::getRandomNumber(static_cast<int32_t>(mTournamentIds.size()));
	Blaze::OSDKTournaments::TournamentId tournamentId = mTournamentIds.at(index);
    BLAZE_TRACE_LOG(BlazeRpcLog::osdktournaments, "[" << getConnection()->getIdent() << "] OSDKTournamentsInstance::joinTournament() joining tournament " << tournamentId << "!");

    JoinTournamentRequest request;
    request.setTournamentId(tournamentId);

	//set the game specific attribute associated with this user for this tournament
	int32_t metaTeam = GenerateMetaTeamId();
	char8_t gameSpecificTournAttribute[TOURN_ATTRIBUTE_MAX_LENGTH];
	blaze_snzprintf(gameSpecificTournAttribute, sizeof(gameSpecificTournAttribute), "%d", metaTeam);
	request.setTournAttribute(gameSpecificTournAttribute);

    JoinTournamentResponse response;
    BlazeRpcError result = mTournSlave->joinTournament(request, response);
    if (result == ERR_OK)
    {
		mTournamentId = tournamentId;
        ((OSDKTournamentsModule*)getOwner())->addPlayerToTournamentPlayerIdMap(mUserId, mTournamentId);		
		
		BLAZE_TRACE_LOG(BlazeRpcLog::osdktournaments, "[" << getConnection()->getIdent() << "] OSDKTournamentsInstance::joinTournament() PlayerID:" << mUserId << " successfully joined tournament " << mTournamentId << "!");
    }
	else if (result == OSDKTOURN_ERR_ALREADY_IN_TOURNAMENT)
	{
		GetMyTournamentDetailsRequest myTournamentRequest;
		GetMyTournamentDetailsResponse myTournamentResponse;
		myTournamentRequest.setMode(mOwner->getTournamentMode());
		result = mTournSlave->getMyTournamentDetails(myTournamentRequest, myTournamentResponse);
		if (result == ERR_OK)
		{
			mTournamentId = myTournamentResponse.getTournamentId();
		}
	}
    return result;
}

BlazeRpcError OSDKTournamentsInstance::leaveTournament()
{
    LeaveTournamentRequest request;
    request.setTournamentId(mTournamentId);
    BlazeRpcError result = mTournSlave->leaveTournament(request);	//note: we are not checking if the player is in a tournament here; check with game team if this is expected
    if (result == ERR_OK)
    {
		//remove player from the player blaze id map
		((OSDKTournamentsModule*)getOwner())->removePlayerFromTournamentPlayerIdMap(mUserId, mTournamentId);
		
        BLAZE_TRACE_LOG(BlazeRpcLog::osdktournaments, "[" << getConnection()->getIdent() << "] OSDKTournamentsInstance::leaveTournament() PlayerID:" << mUserId 
                        << " successfully left tournament " << mTournamentId << "!");
		mTournamentId = INVALID_TOURNAMENT_ID;			
    }

    return result;
}

Blaze::BlazeRpcError OSDKTournamentsInstance::getTrophies()
{
    GetTrophiesRequest request;
    request.setMemberId(getLogin()->getUserLoginInfo()->getBlazeId());
    request.setNumTrophies(MAX_TROPHIES);
    GetTrophiesResponse response;
    return mTournSlave->getTrophies(request, response);
}

BlazeRpcError OSDKTournamentsInstance::resetTournament()
{
    ResetTournamentRequest request;
	request.setTournamentId(mTournamentId);
    request.setMode(mOwner->getTournamentMode()); 
	BlazeRpcError result = mTournSlave->resetTournament(request);
	if (result == ERR_OK)
	{
		//add player to the map anyway (eg. for cases when he wasn't active)
		((OSDKTournamentsModule*)getOwner())->addPlayerToTournamentPlayerIdMap(mUserId, mTournamentId);		
	}
	return result;
}

BlazeRpcError OSDKTournamentsInstance::playMatch()
{
    BLAZE_ERR_LOG(BlazeRpcLog::osdktournaments, "[" << getConnection()->getIdent() << "] OSDKTournamentsInstance::playMatch() Cannot be run without arson component for tournament " 
                  << mTournamentId << " and PlayerID:" << mUserId);
	return ERR_SYSTEM;
}


int32_t OSDKTournamentsInstance::GenerateMetaTeamId()
{
    static const int validTeamIds[] = {
          974
        , 1318
        , 1319
        , 1320
        , 1321
        , 1322
        , 1323
        , 1324
        , 1325
        , 1327
        , 1328
        , 1329
        , 1330
        , 1331
        , 1332
        , 1333
        , 1334
        , 1335
        , 1336
        , 1337
        , 1338
        , 1341
        , 1342
        , 1343
        , 1344
        , 1345
        , 1346
        , 1349
        , 1350
        , 1352
        , 1353
        , 1354
        , 1355
        , 1356
        , 1357
        , 1359
        , 1360
        , 1361
        , 1362
        , 1363
        , 1364
        , 1365
        , 1366
        , 1367
        , 1369
        , 1370
        , 1375
        , 1377
        , 1383
        , 1386
        , 1387
        , 1391
        , 1393
        , 1395
        , 1411
        , 1413
        , 1415
        , 1667
        , 1886
        , 105013
        , 105022
        , 105035
        , 105042
        , 110081
        , 110082
        , 111099
        , 111107
        , 111108
        , 111109
        , 111110
        , 111111
        , 111112
        , 111113
        , 111114
        , 111115
        , 111130
        , 111205
        , 111391
        , 111392
        , 111448
        , 111449
        , 111450
        , 111451
        , 111452
        , 111453
        , 111454
        , 111455
        , 111456
        , 111458
        , 111459
        , 111461
        , 111462
        , 111463
        , 111464
        , 111465
        , 111466
        , 111467
        , 111468
        , 111469
        , 111471
        , 111472
        , 111473
        , 111475
        , 111476
        , 111477
        , 111478
        , 111479
        , 111480
        , 111481
        , 111482
        , 111483
        , 111484
        , 111485
        , 111486
        , 111487
        , 111488
        , 111489
        , 111501
        , 111502
        , 111505
        , 111506
        , 111510
        , 111512
        , 111513
        , 111514
        , 111516
        , 111517
        , 111518
        , 111519
        , 111520
        , 111521
        , 111522
        , 111523
        , 111524
        , 111525
        , 111527
        , 111528
        , 111529
        , 111530
        , 111531
        , 111532
        , 111533
        , 111535
        , 111536
        , 111537
        , 111545
        , 111546
        , 111547
        , 111548
        , 111549
        , 111550
        , 111551
        , 111552
        , 111553
        , 111554
        , 111555
        , 111556
        , 111592
        , 111740
        , 111760
        , 111997
        , 112030
        , 112031
        , 112032
        , 112033
        , 112034
        , 112035
        , 112036
        , 112037
        , 112038
        , 112039
        , 112040
        , 112041
        , 112042
        , 112043
        , 112044
        , 112045
        , 112046
        , 112047
        , 112048
        , 112049
        , 112050
        , 112051
        , 112052
        , 112053
        , 112054
        , 112055
        , 112056
        , 112057
        , 112059
        , 112060
        , 112061
        , 112062
        , 112063
        , 112064
        , 112066
        , 112067
        , 112068
        , 112069
        , 112070
        , 112137
        , 112190
    };
    static const int sizeOfTeamList = sizeof(validTeamIds)/sizeof(validTeamIds[0]);

	int index = Random::getRandomNumber(sizeOfTeamList);
    return validTeamIds[index];
}




} // Stress
} // Blaze

