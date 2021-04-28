/*************************************************************************************************/
/*!
    \file osdktournamentsmodule.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_OSDKTOURNAMENTSMODULE_H
#define BLAZE_STRESS_OSDKTOURNAMENTSMODULE_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#include "stressmodule.h"
#include "osdktournaments/rpc/osdktournamentsslave.h"
#ifdef TARGET_arson
#include "arson/rpc/arsonslave.h"
#include "arson/tdf/arson.h"
#endif
#include "osdktournaments/tdf/osdktournamentstypes.h"
#include "stressinstance.h"
#include "EASTL/vector.h"
#include "EASTL/hash_set.h"

namespace Blaze
{
namespace Stress
{
class StressInstance;
class Login;
class OSDKTournamentsInstance;

class OSDKTournamentsModule : public StressModule
{
    NON_COPYABLE(OSDKTournamentsModule);

public:
    typedef enum
    {
        ACTION_JOIN_TOURNAMENT,
        ACTION_LEAVE_TOURNAMENT,
        ACTION_GET_TOURNAMENTS,
        ACTION_GET_MY_TOURNAMENT_ID,
        ACTION_GET_MY_TOURNAMENT_DETAILS,
        ACTION_RESET_TOURNAMENT,
        ACTION_PLAY_MATCH,
        ACTION_GET_TROPHIES,
        ACTION_GET_MEMBER_COUNTS,
        ACTION_SIMULATE_PRODUCTION,
        ACTION_SLEEP,
        ACTION_MAX_ENUM
    } Action;

    virtual ~OSDKTournamentsModule(); 	
	
    virtual bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil);

    static OSDKTournamentsModule::Action stringToAction(const char8_t* action);
    static const char8_t* actionToString(OSDKTournamentsModule::Action action);
    virtual StressInstance* createInstance(StressConnection* connection, Login* login);
    virtual const char8_t* getModuleName() { return "osdktournaments"; };

    Action getAction() const { return mAction; }
    uint32_t getTournamentMode() const { return mTournamentMode; }
    uint32_t getMaxSleepTimeSecs() const { return mMaxSleepTimeSecs; }
	uint32_t getMinGameLengthSecs() const { return mMinGameLengthSecs; }
	uint32_t getMaxGameLengthSecs() const { return mMaxGameLengthSecs; }
    Action getWeightedAction(bool isInTournament) const;
    static StressModule* create();

	typedef eastl::hash_set<Blaze::BlazeId> PlayerBlazeIdSet;
	typedef eastl::hash_map<Blaze::OSDKTournaments::TournamentId, PlayerBlazeIdSet*> BlazeIdsByTournamentId;
	
	void addPlayerToTournamentPlayerIdMap(BlazeId playerId, Blaze::OSDKTournaments::TournamentId tourneyId);
	void removePlayerFromTournamentPlayerIdMap(BlazeId playerId, Blaze::OSDKTournaments::TournamentId tourneyId);
	bool reservePlayersForPlaymatch(Blaze::OSDKTournaments::TournamentId tourneyId, Blaze::BlazeId ownerBlazeId, BlazeId & oppBlazeId);

private:
    Action mAction;  
    uint32_t mMaxSleepTimeSecs;
    uint32_t mTournamentMode;
    uint32_t mMinGameLengthSecs;
	uint32_t mMaxGameLengthSecs;
    uint32_t mInTourneyProbabilities[ACTION_MAX_ENUM];
    uint32_t mNotInTourneyProbabilities[ACTION_MAX_ENUM];

    OSDKTournamentsModule();

	BlazeIdsByTournamentId mBlazeIdsByTournamentIdMap;

	bool populateDatabase();
	bool mPopulateDatabase;
    eastl::string mDbHost;
    uint32_t mDbPort;
    eastl::string mDatabase;
    eastl::string mDbUser;
    eastl::string mDbPass;

	uint64_t mTotalPlayers;
	uint32_t mNumGamesToPopulatePerPlayer;
	BlazeId mPlayerBlazeIdBase;

	bool8_t mWipeTables;

	typedef eastl::vector<Blaze::OSDKTournaments::TournamentId> OSDKTournamentsIdVector;
	OSDKTournamentsIdVector mTournamentsIdVector;
};

class OSDKTournamentsInstance : public StressInstance
{
    NON_COPYABLE(OSDKTournamentsInstance);

public:

	static const int32_t MAX_PLAYER_SCORE = 200;

    OSDKTournamentsInstance(OSDKTournamentsModule* owner, StressConnection* connection, Login* login);

    virtual ~OSDKTournamentsInstance()
    {
    }

    virtual void onLogin(BlazeRpcError result);

private:
    const char8_t *getName() const { return mName; }
    BlazeRpcError execute();

    BlazeRpcError joinTournament();
    BlazeRpcError leaveTournament();
    BlazeRpcError getTrophies();
	BlazeRpcError getMyTournamentDetails();
	BlazeRpcError getMyTournamentId();
	BlazeRpcError playMatch();
	BlazeRpcError resetTournament();
	int32_t GenerateMetaTeamId();
	bool isActive();
    void sleep(uint32_t minSleepTimeSecs, uint32_t maxSleepSecs);

	OSDKTournamentsModule* mOwner;	//mOwner does not exist in StressModule base in 3.x
    Blaze::OSDKTournaments::OSDKTournamentsSlave* mTournSlave;
    const char8_t *mName;

    typedef eastl::vector<Blaze::OSDKTournaments::TournamentId> TournamentIdVector;
    TournamentIdVector mTournamentIds;
    Blaze::OSDKTournaments::TournamentId mTournamentId;	

	BlazeId mUserId;	
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_OSDKTOURNAMENTSMODULE_H

