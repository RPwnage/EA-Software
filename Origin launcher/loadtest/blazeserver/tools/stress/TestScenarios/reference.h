//  *************************************************************************************************
//
//   File:    reference.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef REFERENCE_H
#define REFERENCE_H

#include "playerinstance.h"
//  #include "player.h"
#include "playermodule.h"

//   Components.
#include "authentication/rpc/authenticationslave.h"
#include "associationlists/rpc/associationlistsslave.h"
#include "framework/rpc/usersessionsslave.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/gamemanagerconfig_server.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "gamereporting/rpc/gamereportingslave.h"
#include "customcomponent/proclubscustom/rpc/proclubscustomslave.h"
#include "util/rpc/utilslave.h"
#include "stats/rpc/statsslave.h"
#include "bytevault/rpc/bytevaultslave.h"
#include "stats/tdf/stats.h"
#include "gamemanager/tdf/gamebrowser.h"
#include "clubs/rpc/clubsslave.h"
#include "messaging/tdf/messagingtypes.h"
#include "messaging/rpc/messagingslave.h"
#include "framework/util/shared/outputstream.h"  //   for OutputStream, base64 encoding
#include "censusdata/rpc/censusdataslave.h"
#include "osdksettings/rpc/osdksettingsslave.h"
//#include "mail/rpc/mailslave.h"
#include "osdktournaments/rpc/osdktournamentsslave.h"
#include "fifacups/rpc/fifacupsslave.h"
#include "fifagroups/rpc/fifagroupsslave.h"
#include "fifastats/rpc/fifastatsslave.h"
#include "fifagroups/tdf/fifagroupstypes.h"
#include "sponsoredevents/rpc/sponsoredeventsslave.h"
#include "coopseason/rpc/coopseasonslave.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave.h"
#include "easfc/rpc/easfcslave.h"
#include "clubs/tdf/clubs_base.h"
#include "useractivitytracker/tdf/useractivitytrackertypes.h"
#include "useractivitytracker/rpc/useractivitytrackerslave.h"
#include "customcomponent/thirdpartyoptin/rpc/thirdpartyoptinslave.h"
#include "customcomponent/thirdpartyoptin/tdf/thirdpartyoptintypes.h"

// xblsystem
#include "proxycomponent/xblsystemconfigs/rpc/xblsystemconfigsslave.h"

// Clubs
#include "clubs/rpc/clubsslave.h"
// OSDK Settings
#include "customcomponent/osdksettings/rpc/osdksettingsslave.h"

// OsdkSeasonalPlay
#include "customcomponent/osdkseasonalplay/rpc/osdkseasonalplayslave.h"
#include "customcomponent/osdkseasonalplay/tdf/osdkseasonalplaytypes.h"

// Tournament
#include "customcomponent/osdktournaments/rpc/osdktournamentsslave.h"
// #include "component/tournaments/tdf/tournaments_custom.h"
// To include sponsoredevents component
#include "customcomponent/sponsoredevents/rpc/sponsoredeventsslave.h"
#include "fifacups/rpc/fifacupsslave.h"
// PC Shield
#include "customcomponent/perf/rpc/perfslave.h"

#include "framework/rpc/blazecontroller_defines.h"

#include "EASTL/shared_ptr.h"
#include "gpscontentcontroller/rpc/gpscontentcontrollerslave.h"
#include "gpscontentcontroller/tdf/gpscontentcontrollertypes.h"
#include "EATDF/tdfobjectid.h"
#include "framework/blaze.h"
#include "EATDF/time.h"
#include "framework/protocol/shared/httpprotocolutil.h"
#include "framework/util/shared/xmlbuffer.h"
#include "framework/util/random.h"
#include "framework/protocol/outboundhttpresult.h"
#include "gamereporting/rpc/gamereporting_defines.h"
#include "gamereporting/tdf/gamereporting_server.h"
#include "perf/tdf/perftypes.h"
#include "customcomponent/proclubscustom/rpc/proclubscustomslave.h"
#include "customcomponent/proclubscustom/tdf/proclubscustomtypes.h"

#include "customcomponent/contentreporter/rpc/contentreporterslave.h"
#include "customcomponent/contentreporter/tdf/contentreportertypes.h"


using namespace Blaze::GameManager;
using namespace Blaze::Authentication;
using namespace Blaze::GameReporting;
using namespace Blaze::Association;
using namespace Blaze::Util;
using namespace Blaze::Stats;
using namespace Blaze::ByteVault;
using namespace Blaze::Clubs;

using namespace eastl;

namespace Blaze {
namespace Stress {

class Player;
class PlayerInstance;
class PlayerModule;
class StressPlayerInfo;
class PlayerDetails;
class ArubaProxyHandler;
class UPSProxyHandler;
class PINProxyHandler;
class StatsProxyHandler;
class ShieldExtension;

//   Default strings
#define FRIEND_LIST                         "friendList"
#define COMPANIONFRIEND_LIST                "companionFriendList"

//   Default ints
#define GETITEMS_VERSION                    0

#define MAX_ROLE_LENGTH                     16      //   Longest role string for Omaha is "commander" and the other one is "soldier" - and both are < 10. Keeping 16 to account for buffer.

static const char8_t* PLATFORM_STRINGS[] =  {
    "pc",
    "xbox",
    "ps3",
    "ps4",
    "xone",
	"ps5",
	"xbsx"
};

typedef enum
{
    ONLINESEASONS = 0,
    DEDICATEDSERVER,
    SINGLEPLAYER,
    COOPPLAYER,
	LIVECOMPETITIONS,
	FRIENDLIES,
	FUTPLAYER,
	CLUBSPLAYER,
	DROPINPLAYER,
	KICKOFFPLAYER,
	SSFPLAYER,
	SSFSEASONS,
	ESLTOURNAMENTS,
	ESLSERVER,
    INVALID
}PlayerType;

// Reco TriggerIds
enum RecoTriggerIDs
{
	DC_TRIGGER_SKILLGAME			= 3040,
	DC_TRIGGER_JOURNEY_OFFBOARDING	= 3042
};

enum ArubaTriggerIDs
{
	DC_TRIGGER_LIVEMSG				= 100,
	DC_TRIGGER_FIFAHUB				= 101,
	DC_TRIGGER_EATV_FIFA			= 102,
	DC_TRIGGER_EATV_FUT				= 103,
	DC_TRIGGER_CAREERLOAD			= 104,
	DC_TRIGGER_FUTLOAD				= 105,
	DC_TRIGGER_FUT_TILE				= 106,
	DC_TRIGGER_FUTHUB				= 111,
	DC_TRIGGER_DRAFT_OFFLINE		= 112,
	DC_TRIGGER_DRAFT_ONLINE			= 113,	
	DC_TRIGGER_UPS					= 114
};

struct LocationInfo
{
public:
	double latitude;
	double longitude;
	LocationInfo()
	{
		latitude = 0.0;
		longitude = 0.0;
	}
};

enum GameGroupType
{
    HOMEGROUP = 0,
    VOIPGROUP,
    GAMEGROUP
};

typedef eastl::map<EA::TDF::EntityId,LocationInfo> PlayerLocationMap;
typedef eastl::map<eastl::string,LocationInfo> CityInfoMap;
static const float EARTH_RADIUS = 3963.1f;

typedef eastl::vector<eastl::string> StringList;
typedef eastl::map<eastl::string, uint8_t> KickOffDistributionMap;
typedef eastl::map<eastl::string, uint8_t> OfflineDistributionMap;
typedef eastl::map<RecoTriggerIDs, uint8_t> RecoTriggerIdsDistributionMap;
typedef eastl::map<RecoTriggerIDs, eastl::string> RecoTriggerIdsToStringMap;
typedef eastl::map<ArubaTriggerIDs, uint8_t> ArubaTriggerIdsDistributionMap;
typedef eastl::map<ArubaTriggerIDs, eastl::string> ArubaTriggerIdsToStringMap;

typedef eastl::map<BlazeId, eastl::string>                          PlayerMap;
typedef eastl::map<uint32_t, BlazeId>								CoopPlayerMap;
typedef eastl::set<BlazeId>											PlayerInGameMap;
typedef eastl::map<eastl::string, uint32_t>						EadpStatsMap;


struct FriendlistSizeDistribution
{
    public:
        uint32_t size;
        uint32_t probability;
        uint32_t count;

        FriendlistSizeDistribution() {
            size = 0;
            probability = 0;
            count = 0;
        }
};

typedef eastl::map<uint32_t, FriendlistSizeDistribution> FriendlistSizeDistributionMap;

typedef eastl::map<uint32_t, uint32_t> EventIdProbabilityMap;

typedef eastl::map<eastl::string, uint32_t> PinSiteProbabilityMap;


enum ScenarioType
{
    CREATEGAME = 0,
    SEASONS,
	SEASONSCUP,
	FUTDRAFTMODE,
	FUTCHAMPIONMODE,
	FUTFRIENDLIES,
	FUTTOURNAMENT,
	FUTONLINESINGLEMATCH,
	FUTRIVALS,
	CLUBMATCH,
	CLUBSCUP,
	CLUBSPRACTICE,
	CLUBFRIENDLIES,
	SP_KICKOFF,
	SP_OFFLINE_EXIBHITION,
	SP_TOURNAMENTS,
	SP_CAREER,
	FUT_OFFLINE_DRAFT,
	FUT_OFFLINE_SEASONS,
	FUT_OFFLINE_PAF,
	FUT_OFFLINE_SQUAD_BATTLES,
	FUT_OFFLINE_TOTW,
	KICKOFF_MATCHDAY_ON,
	KICKOFF_MATCHDAY_OFF,
	FUT_PLAY_ONLINE,
	SSF_SEASONS = 27,
	SSF_WORLD_TOUR = 28,
	SSF_TWIST_MATCH = 29,
	SSF_SINGLE_MATCH = 30,
	SSF_SOLO_MATCH = 34,
	FUT_ONLINE_SQUAD_BATTLE = 31,
	VOLTA_DROPIN = 32,
	VOLTA_PWF = 33,
	VOLTA_FUTSAL = 35,	
	VOLTA_5v5 = 36,
	VOLTA_SOLO = 37,
	FUT_COOP_RIVALS = 38,
	FRIENDLY = 39,
	FRRB = 40,
    NONE
};

typedef enum
{
    PLATFORM_PC = 0,
    PLATFORM_XBOX,
    PLATFORM_PS3,
    PLATFORM_PS4,
    PLATFORM_XONE,
	PLATFORM_PS5,
	PLATFORM_XBSX,
} Platform;

typedef GameManager::GameState PlayerState;

//  TODO provide public getter function instead of public data, so as to make data read only
struct GameplayConfig    //  common Gameplay Configuration
{
    uint32_t GameStateIterationsMax;
    uint32_t GameStateSleepInterval;
    uint32_t cancelMatchMakingProbability;
    uint32_t MatchMakingSessionDuration;
    uint32_t LogoutProbability;
    uint32_t reLoginWaitTime;
	uint32_t loginRateTime;
    uint32_t maxLobbyWaitTime;
    uint16_t maxPlayerCount;
    uint32_t minPlayerCount;
    uint32_t friendsListUpdateProbability;
    uint32_t maxPlayerStateDurationMs;
    uint32_t maxGamesPerSession;
    uint32_t processInvitesFiberSleep;
    uint64_t maxLoggedInDuration;
    uint32_t maxSetUsersToListCount;
    uint32_t setUsersToListProbability;
    uint32_t missionCompleteProbability;
    uint32_t setUsersToListAddPlayerCount;
    uint32_t setUsersToListAddPlayerDeviationCount;
    uint32_t playerJoinTimeoutWait;
};

struct OnlineSeasonsGamePlayConfig : GameplayConfig
{
    uint32_t            roundInGameDuration;
    uint32_t            inGameDeviation;
    uint32_t            HostLeaveProbability;
    uint32_t            JoinerLeaveProbability;
    uint32_t            KickPlayerProbability;
    uint32_t            inGameReportTelemetryProbability;
};

struct FutPlayerGamePlayConfig : GameplayConfig
{
    uint32_t            GameSize;
    uint32_t            roundInGameDuration;
    uint32_t            inGameDeviation;
    uint32_t            HostLeaveProbability;
    uint32_t            JoinerLeaveProbability;
    uint32_t            KickPlayerProbability;
    uint32_t            inGameReportTelemetryProbability;
};


struct FriendliesGamePlayConfig : GameplayConfig
{
    uint32_t            roundInGameDuration;
    uint32_t            inGameDeviation;
    uint32_t            HostLeaveProbability;
    uint32_t            JoinerLeaveProbability;
	uint32_t			maxPlayerStateDurationMs;
	uint32_t			inGameReportTelemetryProbability;
	uint32_t			GameStateSleepInterval;
	uint32_t			GameStateIterationsMax;   
};

struct LiveCompetitionsGamePlayConfig : GameplayConfig
{
    uint32_t            GameSize;
    uint32_t            roundInGameDuration;
    uint32_t            inGameDeviation;
    uint32_t            HostLeaveProbability;
    uint32_t            JoinerLeaveProbability;
    uint16_t            mQueueCapacity;
    uint16_t            mPublicSpectatorCapacity;
    uint16_t            processGameQueueInterval;
    uint32_t            GameResetableProbability;
    bool                IsEnabledResetDedicatedServer;
    uint32_t            KickPlayerProbability;   
    uint32_t            inGameReportTelemetryProbability;
    uint32_t            gameRoundsPerGamePlay;
    int32_t             mRequestPlatformHostProbability;
};

struct CoopGamePlayconfig : GameplayConfig
{
    GameReporting::GameReportingConfig GameReportConfig;
    uint32_t			roundInGameDuration;
    uint32_t			inGameDeviation;
    uint32_t			JoinerLeaveProbability;
    uint32_t			RoundInGameDuration;
    uint32_t			PostGameMaxTickerCount;
    uint32_t			maxStatsForOfflineGameReport;
	uint32_t			inGameReportTelemetryProbability;
};

struct DedicatedServerGamePlayconfig : GameplayConfig
{
    GameReporting::GameReportingConfig GameReportConfig;
    uint32_t		GameStateSleepInterval;
    uint16_t		MinPlayerCount;
    uint16_t		MaxPlayerCount;
    uint32_t		GameStateIterationsMax;
	uint32_t		roundInGameDuration;
    uint32_t		inGameDeviation;
	uint32_t		inGameReportTelemetryProbability;
};

struct ClubsPlayerGamePlayconfig : GameplayConfig
{
    uint32_t            roundInGameDuration;
    uint32_t            inGameDeviation;
    uint32_t            JoinerLeaveProbability;
    uint32_t            inGameReportTelemetryProbability;
	uint32_t			acceptInvitationProbablity;
	uint32_t			cancelMatchMakingProbability;
	uint32_t			GameStateIterationsMax;
	uint32_t			LogoutProbability;
	uint32_t			SeasonLevel;
	uint32_t			ClubDomainId;
	uint32_t			ClubSizeMax;
	uint32_t			ClubsNameIndex;
	uint32_t			PlayerWaitingCounter;
	uint32_t			MinTeamSize;
	typedef		eastl::map<uint8_t, uint8_t> ClubSizeProbability;
	ClubSizeProbability ClubSizeDistribution;
	uint8_t				getRandomTeamSize() const;
};

struct DropinPlayerGamePlayconfig : GameplayConfig
{
	uint32_t            roundInGameDuration;
	uint32_t            inGameDeviation;
	uint32_t            JoinerLeaveProbability;
	uint32_t            inGameReportTelemetryProbability;
	uint32_t			acceptInvitationProbablity;
	uint32_t			cancelMatchMakingProbability;
	uint32_t			GameStateIterationsMax;
	uint32_t			LogoutProbability;
	uint32_t			SeasonLevel;
	uint32_t			ClubDomainId;
	uint32_t			ClubSizeMax;
	uint32_t			ClubsNameIndex;
	uint32_t			PlayerWaitingCounter;
	uint32_t			MinTeamSize;
	typedef		eastl::map<uint8_t, uint8_t> ClubSizeProbability;
	ClubSizeProbability ClubSizeDistribution;
	uint8_t				getRandomTeamSize() const;
};

struct SSFPlayerGamePlayConfig : GameplayConfig
{
	GameReporting::GameReportingConfig GameReportConfig;
	uint32_t			roundInGameDuration;
	uint32_t			inGameDeviation;
	uint32_t			maxStatsSSFGameReport;
	uint32_t			joinerLeaveProbability;
	uint32_t			quitFromGameProbability;
	uint32_t			leaderboardCallProbability;
	uint32_t			inGameReportTelemetryProbability;
	uint32_t			acceptInvitationProbablity;
	uint32_t			gameStateIterationsMax;
	uint32_t			logoutProbability;
	uint32_t			clubSizeMax;
	uint32_t			seasonLevel;
	uint32_t			clubDomainId;
	uint32_t			clubsNameIndex;
	uint32_t			playerWaitingCounter;
	uint32_t			minTeamSize;
	uint32_t			enableResetDedicatedServer;
	typedef				eastl::map<uint8_t, uint8_t> ClubSizeProbability;
	ClubSizeProbability ClubSizeDistribution;
	uint8_t				getRandomTeamSize() const;
};

struct SsfSeasonsGamePlayConfig : GameplayConfig
{
	uint32_t            roundInGameDuration;
	uint32_t            inGameDeviation;
	uint32_t            HostLeaveProbability;
	uint32_t            JoinerLeaveProbability;
	uint32_t            KickPlayerProbability;
	uint32_t            inGameReportTelemetryProbability;
	uint32_t			clubSizeMax;
};

struct SingleplayerGamePlayConfig : GameplayConfig
{
    GameReporting::GameReportingConfig GameReportConfig;
    uint32_t		RoundInGameDuration;
    uint32_t		InGameDeviation;
    uint16_t		maxSeasonsToPlay;
    uint32_t		maxStatsForOfflineGameReport;
    uint32_t		QuitFromGameProbability;
	uint16_t		LeaderboardCallProbability;
	uint32_t		inGameReportTelemetryProbability;
	// Game Type Probabilities
	int32_t			mGameType83Probability;
	int32_t			mGameType85Probability;
	int32_t			mGameType87Probability;
	int32_t			mGameType89Probability;
	int32_t			mGameType90Probability;
	int32_t			mGameType92Probability;
	int32_t			mGameType95Probability;
	int32_t			mGameType100Probability;
	int32_t			mGameType101Probability;	
};

struct KickoffplayerGamePlayConfig : GameplayConfig
{
	GameReporting::GameReportingConfig GameReportConfig;
	uint32_t		RoundInGameDuration;
	uint32_t		InGameDeviation;
	uint16_t		maxSeasonsToPlay;
	uint32_t		maxStatsForOfflineGameReport;
	uint32_t		QuitFromGameProbability;
	uint16_t		LeaderboardCallProbability;
	uint32_t		inGameReportTelemetryProbability;
	// Game Type Probabilities
	int32_t			mGameType83Probability;
	int32_t			mGameType85Probability;
	int32_t			mGameType87Probability;
	int32_t			mGameType89Probability;
	int32_t			mGameType90Probability;
	int32_t			mGameType92Probability;
	int32_t			mGameType95Probability;
	int32_t			mGameType100Probability;
	int32_t			mGameType101Probability;
};

typedef eastl::vector<EA::TDF::TdfString> PingSitesList;

class StressConfigManager {
        NON_COPYABLE(StressConfigManager);

    public:
        static StressConfigManager& getInstance();
        virtual ~StressConfigManager();

        bool                            initialize(const ConfigMap& config, const uint32_t totalConnections);


        const char8_t*                  getPlayerEmailFormat() const                    {
            return mEmailFormat.c_str();
        }
        const char8_t*                  getPlayerPersonaNamespace() const               {
            return mNamespace.c_str();
        }
        const char8_t*                  getPlayerPersonaFormat() const                  {
            return mPersonaFormat.c_str();
        }
        const char8_t*                  getPlayerPassword() const                       {
            return mPassword.c_str();
        }

        //  Nucleus login info
        const char8_t*                  getNucleusPlayerEmailFormat() const             {
            return mNucleusEmailFormat.c_str();
        }
        const char8_t*                  getNucleusPlayerPassword() const                {
            return mNucleusPassword.c_str();
        }
        const char8_t*                  getNucleusPlayerGamertagFormat() const          {
            return mNucleusGamertagFormat.c_str();
        }
        uint64_t                        getNucleusPlayerStartXuid() const               {
            return mNucleusStartXuid;
        }

        Platform                        getPlatform() const                             {
            return mPlatform;
        }

		const char8_t*					getTestClientId() {
			return mTestClientId.c_str();
		}

		const char8_t*					getClientSecret() {
			return mClientSecret.c_str();
		}
        inline bool                     isPlatformPC()                                  {
            return mPlatform == PLATFORM_PC;
        }

        //   Authentication
        const GroupNameList&            getEntGroupNameList() const                     {
            return mEntGroupNameList;
        }
        const char8_t*                  getEntGroupName() const                         {
            return mEntGroupName.c_str();
        }
        const char8_t*                  getEntitlementTag() const                       {
            return mEntitlementTag.c_str();
        }
        const char8_t*                  getProjectId() const                            {
            return mProjectId.c_str();
        }
        uint16_t                        getNumEntitlementsToShow() const                {
            return mNumEntitlementsToShow;
        }

        uint32_t                        getGameStateInitializingDuration() const        {
            return mGameStateInitializingDurationMs;
        }
        uint32_t                        getGameStatePreGameDuration() const             {
            return mGameStatePreGameDurationMs;
        }
        uint32_t                        getGameStateInGameDuration() const              {
            return mGameStateInGameDurationMs;
        }
        uint32_t                        getGameStateInGameDeviation() const             {
            return mGameStateInGameDeviationMs;
        }
        uint32_t                        getRandomDelayMin() const             {
            return mRandomDelayMinMs;
        }
        uint32_t                        getRandomDelayMax() const             {
            return mRandomDelayMaxMs;
        }

        AccountId                       getStartAccountId() {
            return mStartAccountId;
        }

        PlayerType                      getRandomPlayerType(StressPlayerInfo* playerData);
		bool							isOnlineScenario(ScenarioType type) const;

        void                            recyclePlayerType(PlayerType type, eastl::string pingSite);
        ScenarioType                    getRandomScenarioByPlayerType(PlayerType type) const;
        const char8_t*                  getGameProtocolVersionString() const {
            return mGameProtocolVersionString;
        }
		eastl::string                  getPlayerGeoIPFilePath() const {
            return geoipfilepath;
        }
		bool							isGeoLocation_MM_Enabled(){
			return geolocation_mm;
		}
		bool                            isGetStatsEnabled() {
            return mStatsEnabled;
        }
        CoopGamePlayconfig*             getCoopGamePlayConfig() const;
        OnlineSeasonsGamePlayConfig*    getOnlineSeasonsGamePlayConfig() const;
		FriendliesGamePlayConfig*		getFriendliesGamePlayConfig() const;
        LiveCompetitionsGamePlayConfig* getLiveCompetitionsGamePlayConfig() const;
		DedicatedServerGamePlayconfig*  getDedicatedServerGamePlayconfig() const;
        SingleplayerGamePlayConfig*     getSingleplayerGamePlayConfig() const;
		KickoffplayerGamePlayConfig*    getKickoffplayerGamePlayConfig() const;
		SSFPlayerGamePlayConfig*		getSSFPlayerGamePlayConfig() const;
		FutPlayerGamePlayConfig*		getFutPlayerGamePlayConfig() const;	
		ClubsPlayerGamePlayconfig*		getClubsPlayerGamePlayConfig() const;
		DropinPlayerGamePlayconfig*		getDropinPlayerGamePlayConfig() const;
		SsfSeasonsGamePlayConfig*       getSsfSeasonsGamePlayConfig() const;
        GameplayConfig*                 getGamePlayConfig(PlayerType type) const;
        GameplayConfig*                 getCommonConfig() const {
            return mCommonConfigPtr;
        }
        uint32_t                        getLogoutProbability(PlayerType type) const;

        bool                            IsIncrementalLoginEnabled() {
            return mIsIncrementalLogin;
        }
        uint64_t                        getIncrementalLoginIdDelta() {
            return  mIncrementalLoginIdDelta;
        }
        uint64_t                        getMaxIncrementalLoginsPerConnection() {
            return mMaxIncrementalLoginsPerConnection;
        }
        uint64_t                        getBaseAccountId() {
            return mBaseAccountId;
        }
        bool                            multiplayerDisabled() {
            return (mPlayerTypeDistribution[ONLINESEASONS] == 0);
        }
        uint32_t                        getMatchMakingSessionTimeoutMs() const {
            return mMatchMakingSessionTimeoutMs;
        }
        uint32_t                        getReportTelemetryPeriodMs() const {
            return mReportTelemetryPeriodMs;
        }
        uint32_t                        getSinglePlayerChance() {
            return mSinglePlayerChance;
        }
        uint32_t                        getMaxSetUsersToListCount() {
            return mCommonConfigPtr->maxSetUsersToListCount;
        }
        uint32_t                        getSetUsersToListProbability() {
            return mCommonConfigPtr->setUsersToListProbability;
        }
        uint32_t                        getSetUsersToListAddPlayerCount() {
            return mCommonConfigPtr->setUsersToListAddPlayerCount;
        }
        uint32_t                        getSetUsersToListAddPlayerDeviationCount() {
            return mCommonConfigPtr->setUsersToListAddPlayerDeviationCount;
        }
		EventIdProbabilityMap& getEventIdProbabilityMap(){
            return eventIdMap;
        }
		PinSiteProbabilityMap& getPingSiteProbabilityMap(){
            return mPingSiteMap;
        }
		KickOffDistributionMap& getKickOffScenarioDistribution() {
			return mKickOffScenarioDistribution;
		}
		OfflineDistributionMap& getOfflineScenarioDistribution() {
			return mOfflineScenarioDistribution;
		}
		ArubaTriggerIdsDistributionMap & getArubaTriggerIdsDistributionMap() {
			return arubaTriggerIdsDistributionMap;
		}

		RecoTriggerIdsDistributionMap & getRecoTriggerIdsDistributionMap() {
			return recoTriggerIdsDistributionMap;
		}

		bool                            getArubaEnabled() {
            return mArubaEnabled;
        }		
		const char8_t*                   getArubaServerUri() {
            return mArubaServerUri.c_str();
        }
        const char8_t*                   getArubaGetMessageUri() {
            return mArubaGetMessageUri.c_str();
        }
        const char8_t*                   getArubaTrackingUri() {
            return mArubaTrackingUri.c_str();
        }
		bool                            getUPSEnabled() {
			return mUPSEnabled;
		}
		const char8_t*                   getUPSServerUri() {
			return mUPSServerUri.c_str();
		}
		const char8_t*                   getUPSGetMessageUri() {
			return mUPSGetMessageUri.c_str();
		}
		const char8_t*                   getUPSTrackingUri() {
			return mUPSTrackingUri.c_str();
		}

		bool                            getRecoEnabled() {
			return mRecoEnabled;
		}
		const char8_t*                   getRecoServerUri() {
			return mRecoServerUri.c_str();
		}
		const char8_t*                   getRecoGetMessageUri() {
			return mRecoGetMessageUri.c_str();
		}
		const char8_t*                   getRecoTrackingUri() {
			return mRecoTrackingUri.c_str();
		}

		bool                            getPINEnabled() {
			return mPINEnabled;
		}
		const char8_t*                   getPINServerUri() {
			return mPINServerUri.c_str();
		}
		const char8_t*                   getPINGetMessageUri() {
			return mPINGetMessageUri.c_str();
		}
		const char8_t*                   getPINTrackingUri() {
			return mPINTrackingUri.c_str();
		}

		// Reading Kick Off Config
		const char8_t*                   getKickOffServerUri() {
			return mKickOffServerUri.c_str();
		}
		const char8_t*                   getKickOffGetMessageUri() {
			return mKickOffGetMessageUri.c_str();
		}
		const char8_t*                   getKickOffTrackingUri() {
			return mKickOffTrackingUri.c_str();
		}
		// Reading groups Config
		bool                            getGroupsEnabled() {
			return mGroupsEnabled;
		}
		const char8_t*                   getGroupsServerUri() {
			return mGroupsServerUri.c_str();
		}
		const char8_t*                   getGroupsSearchUri() {
			return mGroupsSearchUri.c_str();
		}

		bool                            getStatsProxyEnabled() {
			return mStatsProxyEnabled;
		}
		const char8_t*                   getStatsProxyServerUri() {
			return mStatsProxyServerUri.c_str();
		}
		const char8_t*                   getStatsProxyGetMessageUri() {
			return mStatsProxyGetMessageUri.c_str();
		}
		const char8_t*                   getStatsPlayerHealthUri() {
			return mStatsPlayerHealthUri.c_str();
		}
		uint32_t							getOptInProbability() {
			return mOptInProbability;
		}
		uint32_t							getEadpStatsProbability() {
			return mEadpStatsProbability;
		}
		uint32_t							getOfflineStatsProbability() {
			return mOfflineStatsProbability;
		}
		bool                            getPTSEnabled() {
			return mPlayTimeStatsEnabled;
		}
		bool                            isManualStatsEnabled() {
			return mUpdateManualStats;
		}

		// ESL related functions
		const char8_t*                   getESLServerUri() {
			return mESLProxyServerUri.c_str();
		}
		EA::TDF::TdfString				getRandomPersonaNames();

		//RPC Tuning
		bool                            isRPCTuningEnabled() {
			return mIsRPCTuningEnabled;
		}

		//Profanity related functions
		uint32_t							getProfanityAvatarProbability() {
			return mProfanityAvatarProbability;
		}
		uint32_t							getProfanityClubnameProbability() {
			return mProfanityClubnameProbability;
		}
		uint32_t							getProfanityMessageProbability() {
			return mProfanityMessageProbability;
		}

		uint32_t							getFileAvatarNameProbability() {
			return mFileAvatarNameProbability;
		}
		uint32_t							getFileClubNameProbability() {
			return mFileClubNameProbability;
		}
		uint32_t							getFileAIPlayerNamesProbability() {
			return mFileAIPlayerNamesProbability;
		}
		uint32_t							getAcceptPetitionProbability() {
			return mAcceptPetitionProbability;
		}

		eastl::string						getRandomAvatarFirstName();
		eastl::string						getRandomAvatarLastName();
		eastl::string						getRandomClub();
		eastl::string						getRandomMessage();

		eastl::string						getRandomAIPlayerName();

		//Sine wave related config
		bool                            isSineWaveEnabled() {
			return mIsSineWaveEnabled;
		}

    private:
        bool                            parsePlatform(const ConfigMap& config);
        bool                            parseUserInfoConfig(const ConfigMap& config);
        bool                            parseAuthenticationConfig(const ConfigMap& config);
        bool                            parseNucleusLoginConfig(const ConfigMap& config);
        bool                            parsePlayerTypeDistribution(const ConfigMap& config, uint32_t totalConnections);
        bool                            parseFriendlistSizeDistribution(const ConfigMap& config, uint32_t totalConnections);
		bool                            parseEventIdDistribution(const ConfigMap& config);
		bool							parsePingSitesDistribution(const ConfigMap& config);
        bool                            parseGameplayUserInfo(const ConfigMap& guConfig);
        bool                            parseScenarioDistributionByPlayerType(const ConfigMap& config, PlayerType type);
        bool                            parseCommonGamePlayConfig(const ConfigMap& config);
        bool                            parseGamePlayConfigByPlayerType(const ConfigMap& config, PlayerType type);
		bool							parsePlayerGeoIPDataConfig(const ConfigMap& config);
		bool							parseKickOffScenarioDistribution(const ConfigMap& config);
		bool                            parseOfflineScenarioDistribution(const ConfigMap& config);
		bool							parseArubaTriggerIdsDistributionMap(const ConfigMap & config);
		bool							parseRecoTriggerIdsDistributionMap(const ConfigMap & config);
        void                            UpdateCommonGamePlayConfig(const ConfigMap* ptrConfigMap, GameplayConfig* ptrGamePlayConfig, GameplayConfig* ptrDefaultConfig);
		bool                            parseArubaConfig(const ConfigMap& config);
		bool							parseUPSConfig(const ConfigMap& config);
		bool							parseRecoConfig(const ConfigMap& config);
		bool							parsePINConfig(const ConfigMap& config);
		bool							parseStatsConfig(const ConfigMap& config);
		bool							parseESLConfig(const ConfigMap& config);
		bool							parseKickOffConfig(const ConfigMap& config);
		bool							parseSSFConfig(const ConfigMap& config);
		bool							parseGroupsConfig(const ConfigMap& config);
		bool							parseProfanityConfig(const ConfigMap& config);
		bool							parsePersonaNames(const ConfigMap& config);
		bool							parseShieldConfiguration(const ConfigMap& config);


        //   Member variables
        typedef eastl::map<PlayerType, uint32_t> PlayerTypeDistributionMap;
        PlayerTypeDistributionMap mPlayerTypeDistribution;   //   player % distribution
        PlayerTypeDistributionMap mPlayerCountDistribution;   //   player count distribution - % distribution converted into player count

        FriendlistSizeDistributionMap mFriendlistDistribution;
		OfflineDistributionMap mOfflineScenarioDistribution;
		KickOffDistributionMap	mKickOffScenarioDistribution;

        typedef eastl::map<ScenarioType, uint8_t> ScenarioTypeDistributionMap;

        typedef eastl::map<PlayerType, ScenarioTypeDistributionMap> PlayerScenarioDistributionMap;
        PlayerScenarioDistributionMap mPlayerScenarioDistribution;

        typedef eastl::map<PlayerType, GameplayConfig*> PlayerGamePlayConfigMap;
        PlayerGamePlayConfigMap mPlayerGamePlayConfigMap;

		typedef eastl::vector<EA::TDF::TdfString> PersonaNames;
		PersonaNames mPersonaNames;

        //
        //   Non-Nucleus config
        //
        //   Gameplay user config
        eastl::string                       mEmailFormat;
        eastl::string                       mPersonaFormat;
        eastl::string                       mNamespace;
        eastl::string                       mPassword;
		eastl::string						mTestClientId;
		eastl::string						mClientSecret;

        //   Gameplay user config
        eastl::string                       mNucleusEmailFormat;
        eastl::string                       mNucleusPassword;
        eastl::string                       mNucleusGamertagFormat;
        uint64_t                            mNucleusStartXuid;

        Platform                            mPlatform;

		eastl::string                       geoipfilepath;
		bool								geolocation_mm;
		bool								mStatsEnabled;

        //   Authentication
        eastl::string                       mEntGroupName;
        GroupNameList                       mEntGroupNameList;
        eastl::string                       mEntitlementTag;
        eastl::string                       mProjectId;
        uint16_t                            mNumEntitlementsToShow;

        uint32_t                            mMatchMakingSessionTimeoutMs;
        uint32_t                            mReportTelemetryPeriodMs;
        uint32_t                            mGameStateInitializingDurationMs;
        uint32_t                            mGameStateInGameDeviationMs;
        uint32_t                            mGameStatePreGameDurationMs;
        uint32_t                            mGameStateInGameDurationMs;

        uint32_t                            mRandomDelayMinMs;
        uint32_t                            mRandomDelayMaxMs;

        AccountId                           mStartAccountId;
        eastl::map<eastl::string, int>      mStringToEnumMap;
        char8_t                             mGameProtocolVersionString[64];
        GameplayConfig*                     mCommonConfigPtr;
		const ConfigMap*					mShieldMap;


        //  SinglePlayer scenario run chance(s)
        int32_t                             mSinglePlayerChance;

        //  configs used fro DB population
        bool                               mIsIncrementalLogin;
        uint64_t                           mIncrementalLoginIdDelta;
        uint64_t                           mMaxIncrementalLoginsPerConnection;
        uint64_t                           mBaseAccountId;
        int32_t                            mMaxGamesPerSession;
        mutable EA::Thread::Mutex		   mMutex;

		//  aruba related config
        eastl::string                      mArubaServerUri;
        eastl::string                      mArubaGetMessageUri;
        eastl::string                      mArubaTrackingUri;
        bool                               mArubaEnabled;

		// UPS related config value
		eastl::string                      mUPSServerUri;
		eastl::string                      mUPSGetMessageUri;
		eastl::string                      mUPSTrackingUri;
		bool                               mUPSEnabled;

		// Recommendation related config value
		eastl::string                      mRecoServerUri;
		eastl::string                      mRecoGetMessageUri;
		eastl::string                      mRecoTrackingUri;
		bool                               mRecoEnabled;

		//PIN related config
		eastl::string                       mPINServerUri;
		eastl::string                       mPINGetMessageUri;
		eastl::string                       mPINTrackingUri;
		bool                                mPINEnabled;

		//KickOff related config
		eastl::string                       mKickOffServerUri;
		eastl::string                       mKickOffGetMessageUri;
		eastl::string                       mKickOffTrackingUri;
		bool                                mKickOffEnabled;

		// Groups related config value
		eastl::string                       mGroupsServerUri;
		eastl::string                       mGroupsSearchUri;
		bool                                mGroupsEnabled;

		// Stats related config value
		eastl::string                       		mStatsProxyServerUri;
		eastl::string                       		mStatsProxyGetMessageUri;
		eastl::string                       		mStatsPlayerHealthUri;
		uint32_t									mOptInProbability;
		uint32_t									mEadpStatsProbability;
		uint32_t									mOfflineStatsProbability;
		bool                                		mStatsProxyEnabled;
		bool                                		mPlayTimeStatsEnabled;
		bool                                		mUpdateManualStats;

		//ESL Tornaments related config
		eastl::string						mESLServerUri;
		eastl::string						mESLProxyServerUri;
		eastl::string						mESLProxyGetMatchResultsUri;
		eastl::string						mESLProxyEnabled;

		//RPC Tuning related config
		bool								mIsRPCTuningEnabled;

		//Profanity related config
		uint32_t								mProfanityAvatarProbability;
		uint32_t								mProfanityClubnameProbability;
		uint32_t								mProfanityMessageProbability;

		//Abuse Reporting Related Config
		uint32_t								mFileAvatarNameProbability;
		uint32_t								mFileClubNameProbability;
		uint32_t								mFileAIPlayerNamesProbability;
		uint32_t								mAcceptPetitionProbability;

		typedef eastl::vector<eastl::string>	AvatarFirstNames;
		typedef eastl::vector<eastl::string>	AvatarLastNames;
		typedef eastl::vector<eastl::string>	ClubNames;
		typedef eastl::vector<eastl::string>	InGameMessages;
		typedef eastl::vector<eastl::string>	AIPlayerNames;

		AvatarFirstNames mAvatarFirstNames;
		AvatarLastNames mAvatarLastNames;
		ClubNames mClubNames;
		InGameMessages mInGameMessages;
		AIPlayerNames mAIPlayerNames;

		//Sine wave related config
		bool									mIsSineWaveEnabled;

        StressConfigManager();
		EventIdProbabilityMap eventIdMap;
		PinSiteProbabilityMap mPingSiteMap;
		ArubaTriggerIdsDistributionMap arubaTriggerIdsDistributionMap;
		RecoTriggerIdsDistributionMap recoTriggerIdsDistributionMap;
};

#define StressConfigInst   StressConfigManager::getInstance()
//  typedef eastl::vector_map<PlayerId, ConnectionGroupId> PlayerConnectionGroupIdMap;

#define ONLINESEASONS_GAMEPLAY_CONFIG		StressConfigManager::getInstance().getOnlineSeasonsGamePlayConfig()
#define FRIENDLIES_GAMEPLAY_CONFIG			StressConfigManager::getInstance().getFriendliesGamePlayConfig()
#define LIVECOMPETITIONS_GAMEPLAY_CONFIG	StressConfigManager::getInstance().getLiveCompetitionsGamePlayConfig()
#define COOP_GAMEPLAY_CONFIG				StressConfigManager::getInstance().getCoopGamePlayConfig()
#define DEDICATEDSERVER_GAMEPLAY_CONFIG		StressConfigManager::getInstance().getDedicatedServerGamePlayconfig()
#define SINGLE_GAMEPLAY_CONFIG				StressConfigManager::getInstance().getSingleplayerGamePlayConfig()
#define KICKOFF_GAMEPLAY_CONFIG				StressConfigManager::getInstance().getKickoffplayerGamePlayConfig()
#define FUT_GAMEPLAY_CONFIG					StressConfigManager::getInstance().getFutPlayerGamePlayConfig()
#define CLUBS_GAMEPLAY_CONFIG				StressConfigManager::getInstance().getClubsPlayerGamePlayConfig()
#define DROPIN_GAMEPLAY_CONFIG				StressConfigManager::getInstance().getDropinPlayerGamePlayConfig()
#define SSF_GAMEPLAY_CONFIG					StressConfigManager::getInstance().getSSFPlayerGamePlayConfig()
#define SSFSEASONS_GAMEPLAY_CONFIG			StressConfigManager::getInstance().getSsfSeasonsGamePlayConfig()

#define OS_CONFIG		(*(ONLINESEASONS_GAMEPLAY_CONFIG))
#define FR_CONFIG		(*(FRIENDLIES_GAMEPLAY_CONFIG))
#define LC_CONFIG		(*(LIVECOMPETITIONS_GAMEPLAY_CONFIG))
#define CO_CONFIG		(*(COOP_GAMEPLAY_CONFIG))
#define DS_CONFIG		(*(DEDICATEDSERVER_GAMEPLAY_CONFIG))
#define SP_CONFIG		(*(SINGLE_GAMEPLAY_CONFIG))
#define KICKOFF_CONFIG   (*(KICKOFF_GAMEPLAY_CONFIG))
#define FUT_CONFIG   (*(FUT_GAMEPLAY_CONFIG))
#define CLUBS_CONFIG   (*(CLUBS_GAMEPLAY_CONFIG))
#define DROPIN_CONFIG   (*(DROPIN_GAMEPLAY_CONFIG))
#define SSF_CONFIG		(*(SSF_GAMEPLAY_CONFIG))
#define SSFSEASONS_CONFIG		(*(SSFSEASONS_GAMEPLAY_CONFIG))

class ComponentProxy {
       NON_COPYABLE(ComponentProxy);

    public:
        ComponentProxy() {}
        ComponentProxy(StressConnection* connection) {
		mCensusDataProxy	= BLAZE_NEW CensusData::CensusDataSlave(*connection);
		mMessagingProxy		= BLAZE_NEW Messaging::MessagingSlave(*connection);
		mUserSessionsProxy  = BLAZE_NEW UserSessionsSlave(*connection);
		mClubsProxy			= BLAZE_NEW Clubs::ClubsSlave(*connection);
		mUserActivityTrackerProxy = BLAZE_NEW UserActivityTracker::UserActivityTrackerSlave(*connection);
		mUtilProxy			= BLAZE_NEW Util::UtilSlave(*connection);
		mOsdkSettingsProxy  = BLAZE_NEW OSDKSettings::OSDKSettingsSlave(*connection);
		mAuthenticationProxy = BLAZE_NEW Authentication::AuthenticationSlave(*connection);
		mStatsProxy			= BLAZE_NEW Stats::StatsSlave(*connection);
		//mMailProxy			= BLAZE_NEW Mail::MailSlave(*connection);
		mGameManagerProxy   = BLAZE_NEW GameManager::GameManagerSlave(*connection);
		mOsdkSeasonalPlayProxy   = BLAZE_NEW OSDKSeasonalPlay::OSDKSeasonalPlaySlave(*connection);
		mTournamentProxy	= BLAZE_NEW OSDKTournaments::OSDKTournamentsSlave(*connection);
		mFIFACupsProxy		= BLAZE_NEW FifaCups::FifaCupsSlave(*connection);
		mFifaGroupsProxy	= BLAZE_NEW FifaGroups::FifaGroupsSlave(*connection);
		mFifaStatsProxy		= BLAZE_NEW FifaStats::FifaStatsSlave(*connection);
		mAssociationListsProxy  = BLAZE_NEW Association::AssociationListsSlave(*connection);
		mSponsoredEventsProxy	= BLAZE_NEW SponsoredEvents::SponsoredEventsSlave(*connection);
		mCoopSeasonProxy	= BLAZE_NEW CoopSeason::CoopSeasonSlave(*connection);
		mGameReportingProxy = BLAZE_NEW GameReporting::GameReportingSlave(*connection); 
		mVProGrowthUnlocksProxy = BLAZE_NEW VProGrowthUnlocks::VProGrowthUnlocksSlave(*connection);
		mXBLSystemConfigsProxy = BLAZE_NEW XBLSystem::XBLSystemConfigsSlave(*connection);
		mEASFCProxy = BLAZE_NEW EASFC::EasfcSlave(*connection);
		mProClubsCustomSlaveProxy = BLAZE_NEW proclubscustom::proclubscustomSlave(*connection);
		mPerfProxy = BLAZE_NEW Perf::PerfSlave(*connection);
		mThirdPartyOptInProxy = BLAZE_NEW ThirdPartyOptIn::ThirdPartyOptInSlave(*connection);
		//mVProSPManagementComponentProxy = BLAZE_NEW VProSPManagement::VProSPManagementSlave(*connection);
		mContentReporterSlaveProxy = BLAZE_NEW ContentReporter::ContentReporterSlave(*connection);
        }
        ~ComponentProxy() {
		    delete mAuthenticationProxy;
            mAuthenticationProxy = NULL;
            delete mUserSessionsProxy;
            mUserSessionsProxy = NULL;
            delete mGameManagerProxy;
            mGameManagerProxy = NULL;
            delete mAssociationListsProxy;
            mAssociationListsProxy = NULL;
            delete mUtilProxy;
            mUtilProxy = NULL;
            delete mStatsProxy;
            mStatsProxy = NULL;            
            delete mClubsProxy;
            mClubsProxy = NULL;
			delete mUserActivityTrackerProxy;
			mUserActivityTrackerProxy = NULL;
            delete mMessagingProxy;
            mMessagingProxy = NULL;
            delete mCensusDataProxy;
            mCensusDataProxy = NULL;
            delete mOsdkSettingsProxy;
            mOsdkSettingsProxy = NULL;					
			//delete mMailProxy;
            //mMailProxy = NULL;					
			delete mOsdkSeasonalPlayProxy;
            mOsdkSeasonalPlayProxy = NULL;				
			delete mTournamentProxy;
            mTournamentProxy = NULL;				
			delete mFIFACupsProxy;
            mFIFACupsProxy = NULL;		
			delete mSponsoredEventsProxy;
            mSponsoredEventsProxy = NULL;									
			delete mCoopSeasonProxy;
            mCoopSeasonProxy = NULL;	
			delete mGameReportingProxy;
            mGameReportingProxy = NULL;	
			delete mVProGrowthUnlocksProxy;
			mVProGrowthUnlocksProxy = NULL;
			delete mXBLSystemConfigsProxy;
			mXBLSystemConfigsProxy = NULL;
			delete mEASFCProxy;
			mEASFCProxy = NULL;
			delete mProClubsCustomSlaveProxy;
			mProClubsCustomSlaveProxy = NULL;
			delete mPerfProxy;
			mPerfProxy = NULL;
			delete mThirdPartyOptInProxy;
			mThirdPartyOptInProxy = NULL;
			/*delete mVProSPManagementComponentProxy;
            mVProSPManagementComponentProxy = NULL;	*/
			delete mContentReporterSlaveProxy;
			mContentReporterSlaveProxy = NULL;
        }

        //   Proxy objects for different components.
		AuthenticationSlave*                   	mAuthenticationProxy;
        Association::AssociationListsSlave*    	mAssociationListsProxy;
        UserSessionsSlave*                     	mUserSessionsProxy;
        GameManagerSlave*                      	mGameManagerProxy;
        UtilSlave*                             	mUtilProxy;
        StatsSlave*                            	mStatsProxy;
        ClubsSlave*                            	mClubsProxy;
		UserActivityTracker::UserActivityTrackerSlave*		mUserActivityTrackerProxy;
		FifaGroups::FifaGroupsSlave*			mFifaGroupsProxy;
		CensusData::CensusDataSlave*			mCensusDataProxy;

		Messaging::MessagingSlave*				mMessagingProxy;
		OSDKSettings::OSDKSettingsSlave*		mOsdkSettingsProxy;				
		//Mail::MailSlave*						mMailProxy;
		OSDKSeasonalPlay::OSDKSeasonalPlaySlave*	mOsdkSeasonalPlayProxy;
		OSDKTournaments::OSDKTournamentsSlave*	mTournamentProxy;
		FifaStats::FifaStatsSlave*				mFifaStatsProxy;
		FifaCups::FifaCupsSlave*				mFIFACupsProxy;
		SponsoredEvents::SponsoredEventsSlave*	mSponsoredEventsProxy;
		CoopSeason::CoopSeasonSlave*			mCoopSeasonProxy;
		GameReporting::GameReportingSlave*		mGameReportingProxy;
		VProGrowthUnlocks::VProGrowthUnlocksSlave* mVProGrowthUnlocksProxy;
		XBLSystem::XBLSystemConfigsSlave*	mXBLSystemConfigsProxy;
		EASFC::EasfcSlave*                      mEASFCProxy;
		proclubscustom::proclubscustomSlave*	mProClubsCustomSlaveProxy;
		Perf::PerfSlave*						mPerfProxy;
		ThirdPartyOptIn::ThirdPartyOptInSlave*	mThirdPartyOptInProxy;
		ContentReporter::ContentReporterSlave*	mContentReporterSlaveProxy;
		//VProSPManagement::VProSPManagementSlave*	mVProSPManagementComponentProxy;

    private:
};

class StressPlayerInfo {
        NON_COPYABLE(StressPlayerInfo);

    public:
        StressPlayerInfo();
        virtual ~StressPlayerInfo();

        BlazeId             getPlayerId()                                     {
            return playerId;
        }
        void                setPlayerId(BlazeId id)                           {
            playerId = id;
        }
        ComponentProxy*     getComponentProxy()                               {
            return componentProxy;
        }
        void                setComponentProxy(ComponentProxy* proxy)          {
            componentProxy = proxy;
        }
        StressConnection*   getConnection()                                   {
            return stressConnection;
        }
        void                setConnection(StressConnection* connection)       {
            stressConnection = connection;
        }
        BlazeObjectId       getConnGroupId()                                  {
            return connGroupId;
        }
        void                setConnGroupId(BlazeObjectId groupId)             {
            connGroupId = groupId;
        }
        StressModule*       getOwner()                                        {
            return owner;
        }
        void                setOwner(StressModule* moduleOwner)               {
            owner = moduleOwner;
        }
        PlayerInstance*     getMyPlayerInstance()                             {
            return mMyPlayerInstance;
        }
        void                setMyPlayerInstance(PlayerInstance* playerInstance) {
            mMyPlayerInstance = playerInstance;
        }
		Blaze::Stress::ShieldExtension*     getShieldExtension() {
			return mShield;
		}
		void                setShieldExtension(Blaze::Stress::ShieldExtension* se) {
			mShield = se;
		}
        Login*              getLogin()                                        {
            return login;
        }
        void                setLogin(Login* mLogin)                           {
            login = mLogin;
        }
        eastl::string       getPersonaName()                                  {
            return personaName;
        }
        void                setPersonaName(eastl::string name)                {
            personaName = name;
        }
        ExternalId          getExternalId()                                   {
            return extId;
        }
        void                setExternalId(ExternalId id)                      {
            extId = id;
        }
		void				setAccountId(BlazeId nucId)						  {
			nucleusId = nucId;
		}
		BlazeId          getAccountId()										  {
			return nucleusId;
		}
        void                IncrementLoginCount()                             {
            ++mTotalLogins;
        }
        uint32_t            getTotalLogins()                                  {
            return mTotalLogins;
        }
        void                setIdent(int32_t ident)                           {
            mIdent = ident;
        }
        int32_t             getIdent()                                        {
            return mIdent;
        }
        uint32_t            getViewId()                                       {
            return mViewId;
        }
        void                setViewId(uint32_t viewId)                        {
            mViewId = viewId;
        }
        uint32_t            getFriendlistSize()                               {
            return mFriendlistSize;
        }
        void                setFriendlistSize(uint32_t size)                  {
            mFriendlistSize = size;
        }

        TimeValue           getStatsAsyncTime()                               {
            return mTimeValue;
        }
        void                setStatsAsyncTime(TimeValue timeValue)            {
            mTimeValue = timeValue;
        }

        void                incrementViewId()                                 {
            mViewId++;
        }
        bool                isConnected();
        bool                isFirstLogin() const                              {
            return mIsFirstLogin;
        }
        void                clearFirstLogin()                                 {
            mIsFirstLogin = false;
        }
        void                setReconnectAttempt(bool state)                   {
            mAttemptReconnect = state;
        }
        bool                isReconnectAttempt()                              {
            return mAttemptReconnect;
        }
        void                setPlayerActiveState(bool bState)                 {
            mIsPlayerActive = bState;
        }
        bool                isPlayerActive()                                  {
            return mIsPlayerActive;
        }
        bool                queuePlayerToUpdate(BlazeId blazeId);
        void                removeFromUpdatedUsersSet(BlazeId blazeId);
        void                setStatGroupCalled(bool called)                   {
            mGetStatGroupCalled = called;
        }
        bool                isStatGroupCalled()                               {
            return  mGetStatGroupCalled;
        }
        uint32_t            getGamesPerSessionCount()                         {
            return  mGamesPerSessionCount;
        }
        void                setGamesPerSessionCount(uint32_t count)           {
            mGamesPerSessionCount = count;
        }
        void                incrementGamesPerSessionCount()                   {
            ++mGamesPerSessionCount;
        }
        BlazeIdList& getFriendList()                                   {
            return mFriendList;
        }
        ExternalIdList& getFriendExternalIdList()                      {
            return mFriendExternalIdList;
        }
        eastl::vector<int64_t>& getMyPartyMembers()                           {
            return mMyPartyMembers;
        }
        GameManager::GameId              getMyPartyGameId()                                {
            return mMyPartyGameId;
        }
        void                setMyPartyGameId(GameManager::GameId gameID)                   {
            mMyPartyGameId = gameID;
        }
        GameManager::GameActivity&       getPartyGameActivity()                            {
            return mPartyGameActivity;
        }
        PlayerDetails*      getMyDetailsfromCommonMap()                       {
            return mMyDetails;
        }
        void                setMyDetailsInCommonMap(PlayerDetails* playerDetails)             {
            mMyDetails = playerDetails;
        }
        void                setLogInTime()                                    {
            mLogInTime = TimeValue::getTimeOfDay().getMillis();
        }
        void                resetLogInTime()                                  {
            mLogInTime = 0;
        }
        bool                isLoggedInForTooLong();
        const char8_t*       getPlayerPingSite()    {
            return mPingSite;
        }
        void                setPlayerPingSite(const char8_t* pingSite)  {
            blaze_strnzcpy(mPingSite, pingSite, MAX_PINGSITEALIAS_LEN);
        }
		PingSiteLatencyByAliasMap& getPingSiteLatencyMap(){ 
			return mPingSiteLatencyMap; 
		}
		void  setPlayerGameName( char* val){ 
			 mGameName = val; 
		}

		char*	getPlayerGameName( ){
			return mGameName;
		}

        void setLocation(LocationInfo info){
            mLocationInfo = info;
        }

        LocationInfo getLocation(){
            return mLocationInfo;
        }
		ArubaProxyHandler*		getArubaProxyHandler() {
            return mArubaProxy;
        }
		UPSProxyHandler* getUPSProxyHandler() {
			return mUPSProxy;
		}
		PINProxyHandler* getPINProxyHandler() {
			return mPINProxy;
		}
		StatsProxyHandler* getStatsProxyHandler() {
			return mStatsProxy;
		}
        void setArubaProxyHandler(ArubaProxyHandler* ap){
            mArubaProxy = ap;
        }
		void setUPSProxyHandler(UPSProxyHandler * uph) {
			mUPSProxy = uph;
		}
		void setPINProxyHandler(PINProxyHandler* pph) {
			mPINProxy = pph;
		}
		void setStatsProxyHandler(StatsProxyHandler* sph) {
			mStatsProxy = sph;
		}
		void setKickOffGroupId(eastl::string gUID)
		{
			mKickOffGuid = gUID;
		}
		eastl::string getKickOffGroupId()
		{
			return mKickOffGuid;
		}
    protected:
        BlazeId             playerId;
		BlazeId				nucleusId;
        eastl::string       personaName;
        ComponentProxy*     componentProxy;
        StressConnection*   stressConnection;
        BlazeObjectId       connGroupId;
        StressModule*       owner;
        PlayerInstance*     mMyPlayerInstance;
		Blaze::Stress::ShieldExtension*	mShield;
        Login*              login;
        ExternalId          extId;
        uint32_t            mTotalLogins;
        int32_t             mIdent;
        uint32_t            mViewId;
        uint32_t            mFriendlistSize;
        TimeValue           mTimeValue;
        bool                mIsFirstLogin;
        bool                mAttemptReconnect;
        bool                mIsPlayerActive;
        bool                mGetStatGroupCalled;
        uint32_t            mGamesPerSessionCount;
        GameManager::GameId mMyPartyGameId;
        eastl::vector<int64_t>  mMyPartyMembers;
        GameManager::GameActivity        mPartyGameActivity;  //   needed to call GameManagerComponent::updatePrimaryExternalSessionForUser on xone
        eastl::set<BlazeId> updatedUsersSet;  //  set of blazeIds recieved from NOTIFY_USERADDED, NOTIFY_USERUPDATED and NOTIFY_USERSESSIONEXTENDEDDATAUPDATE notifications
        BlazeIdList			mFriendList;
        ExternalIdList		mFriendExternalIdList;
        PlayerDetails*      mMyDetails;
        int64_t             mLogInTime;
        char8_t             mPingSite[MAX_PINGSITEALIAS_LEN];
		PingSiteLatencyByAliasMap   mPingSiteLatencyMap;
		char* 				mGameName;
//        int                 mOperationIndex;
		LocationInfo		mLocationInfo;
		ArubaProxyHandler*	mArubaProxy;
		UPSProxyHandler*	mUPSProxy;
		PINProxyHandler*	mPINProxy;
		StatsProxyHandler*  mStatsProxy;
		eastl::string		mKickOffGuid;
};

struct UserInfo
{
    UserInfo(const char8_t* strPersonaName, ExternalId ExtId, ConnectionGroupId ConnGroupId, const char8_t* role, GameManager::SlotType roleType = SLOT_PUBLIC_PARTICIPANT, GameManager::TeamIndex teamIndex = 0)
        : extId(ExtId),
          connGroupId(ConnGroupId),
          userRoleType(roleType),
          teamIndex(teamIndex) {
        personaName[0] = '\0';
        roleName[0] = '\0';
        if (NULL != strPersonaName) {
            blaze_strcpyFixedLengthToNullTerminated(personaName, MAX_PERSONA_LENGTH, strPersonaName, strlen(strPersonaName));
        }
        if (NULL != role) {
            blaze_strcpyFixedLengthToNullTerminated(roleName, MAX_ROLE_LENGTH, role, strlen(role));
        }
    }

    char8_t                                 personaName[MAX_PERSONA_LENGTH];
    ExternalId                              extId;
    ConnectionGroupId                       connGroupId;
    char8_t                                 roleName[MAX_ROLE_LENGTH];
    GameManager::SlotType                                userRoleType;
    GameManager::TeamIndex                               teamIndex;
};

class PlayerDetails {
    public:
		PlayerDetails();
        PlayerDetails(StressPlayerInfo *playersData);
        void                setPlayerDead();
        void                setPlayerAlive(StressPlayerInfo* playerInfo);
        PlayerDetails*      getPlayerDetails()                              {
            return ((mIsAlive) ? this : NULL);
        }
        BlazeId             getPlayerId()                                   {
            return mMyInfo->getPlayerId();
        }
		BlazeId             getAccountId() {
			return mMyInfo->getLogin()->getUserLoginInfo()->getAccountId();
		}
        ExternalId          getPlayersExternalId()                          {
            return mMyInfo->getExternalId();
        }
        eastl::string       getplayerPersonaName()                          {
            return mMyInfo->getPersonaName();
        }
        bool                isInAParty()                                    {
            return ((mPartyGameId) ?  true : false);
        }
        void                setPartyInvite(BlazeId invitersId)              {
            mPartyInvite = invitersId;
        }
        bool                checkPartyInvite()                              {
            return ((mPartyInvite) ? true : false);
        }
        void                sendPartyInvite(PlayerDetails *friendsDetails, BlazeId inviteId);
        BlazeId             getPartyFriend()                                {
            return mPartyFriend;
        }
        void                setPartyFriend(BlazeId partyFriendId)           {
            mPartyFriend = partyFriendId;
        }
        void                setOnlineSeasonsGamdId(GameManager::GameId multiplayerGameId)  {
            mOnlineSeasonsGameId = multiplayerGameId;
        }
        GameManager::GameId              getOnlineSeasonsGamdId()                          {
            return mOnlineSeasonsGameId;
        }

    private:
        bool                mIsAlive;
        StressPlayerInfo*   mMyInfo;
        BlazeId             mFriendsInvite;
        BlazeId             mPartyInvite;
        BlazeId             mPartyGameId;
        BlazeId             mPartyFriend;
        GameManager::GameId mOnlineSeasonsGameId;
};

typedef eastl::vector_set<PlayerId> PlayerIdSet;
typedef eastl::hash_map<BlazeId, PlayerDetails> PlayerDetailsMap;

typedef eastl::vector_map<PlayerId, UserInfo*>	PlayerIdListMap;

typedef eastl::shared_ptr<UserIdentification>	UserIdentPtr;
typedef eastl::vector_map<BlazeId, UserIdentPtr> UserIdentMap;
typedef eastl::list<UserIdentPtr>		UserIdentList;
typedef eastl::vector<GameManager::GameId>			CreatedGamesList;
typedef eastl::vector<BlazeId>			CoopFriendsList;
typedef eastl::vector<BlazeId>			FutFriendsList;
typedef eastl::vector<BlazeId>			VoltaFriendsList;
typedef eastl::map<ClubId, uint32_t>	GlobalClubsMap;
typedef eastl::hash_map<ClubId, bool>	ClubsGameStatusMap;

typedef eastl::hash_map<BlazeId, BlazeId> PlayerAccountMap;		//Map to store blazeID and accountID of a player

class StressGameInfo {
        NON_COPYABLE(StressGameInfo);

    public:
        StressGameInfo() {
            mGameId = 0;
            mGameReportingId = 0;
            mGameState = NEW_STATE;
            mPlayerCount = 0;
            mPersistedGameId.set("");
            mGameName = "";
            mMaxPlayerCapacity = 0;
            mInGameDurationeSec = 0;
        }
        virtual ~StressGameInfo() {}

        inline GameManager::GameId          getGameId()                                 {
            return mGameId;
        }
        inline void                         setGameId(GameManager::GameId id)                        {
            mGameId = id;
        }
        inline GameReportingId              getGameReportingId()                        {
            return mGameReportingId;
        }
        inline void                         setGameReportingId(GameReportingId id)      {
            mGameReportingId = id;
        }
        inline PlayerId                     getTopologyHostPlayerId()                   {
            return mTopologyHostPlayerId;
        }
        inline void                         setTopologyHostPlayerId(PlayerId id)        {
            mTopologyHostPlayerId = id;
        }
        inline PlayerId                     getPlatformHostPlayerId()                   {
            return mPlatformHostPlayerId;
        }
        inline void                         setPlatformHostPlayerId(PlayerId id)        {
            mPlatformHostPlayerId = id;
        }
        //  inline PlayerIdList&              getPlayerIdList()                           { return payerIdList;                                           }
        inline PlayerIdList&                getInvitePlayerIdList()                     {
            return invitePlayerIdList;
        }
        //  inline ConnectionGroupId          getPlatformHostConnectionGroupId()          { return mPlatformHostConnectionGroupId;                        }
        //  inline void                           setPlatformHostConnectionGroupId(ConnectionGroupId groupId)  { mPlatformHostConnectionGroupId = groupId;    }
        inline ConnectionGroupId            getTopologyHostConnectionGroupId()          {
            return mTopologyHostConnectionGroupId;
        }
        inline void                         setTopologyHostConnectionGroupId(ConnectionGroupId groupId)  {
            mTopologyHostConnectionGroupId = groupId;
        }
        inline void                         setDedicatedServerHostConnectionGroupId(ConnectionGroupId groupId)  {
            mDedicatedServerHostConnectionGroupId = groupId;
        }
        inline ConnectionGroupId            getDedicatedServerHostConnectionGroupId()   {
            return mDedicatedServerHostConnectionGroupId;
        }
        //    inline void                         resetInGameStartTime()                      { mInGameStartTime = 0;                                         }
        //  inline PlayerConnectionGroupIdMap&  getPlayerConnectionGroupMap()             { return mPlayerConnectionGroupId;                              }
        inline GameState                    getGameState()                              {
            return mGameState;
        }
        inline void                         setGameState(GameState state)               {
            mGameState = state;
        }
        inline uint16_t                     getPlayerCount()                            {
            return mPlayerCount;
        }
        inline void                         setPlayerCount(uint16_t count)              {
            mPlayerCount = count;
        }
        inline PlayerIdListMap&             getPlayerIdListMap()                        {
            return playerListMap;
        }
        void                                setGameAttributes(const Collections::AttributeMap& GameAttributes);  //  this will add/update atributes in the collection
        inline const Collections::AttributeMap& getGameAttributes()                     {
            return mGameAttributeMap;
        }
        inline void                         setPersistedGameId(const char8_t*   val)    {
            mPersistedGameId.set(val);
        }
        inline const char8_t*               getPersistedGameId()                        {
            return mPersistedGameId.c_str();
        }
        inline void                         setGameName(const char8_t*  name)           {
            mGameName.set(name);
        }
        inline GameName                     getGameName()                               {
            return mGameName;
        }
        inline void                         setPlayerCapacity(uint16_t capacity)        {
            mMaxPlayerCapacity = capacity;
        }
        inline uint16_t                     getPlayerCapacity()                         {
            return mMaxPlayerCapacity;
        }
        inline void                         setInGameDuration(double durationSec)       {
            mInGameDurationeSec = durationSec;
        }
        inline double                       getInGameDuration()                         {
            return mInGameDurationeSec;
        }
        inline size_t                       getNumPlayersInQueue() const                {
            return mQueuedPlayerList.size();
        }
        inline PlayerIdListMap&             getQueuedPlayerList()                       {
            return mQueuedPlayerList;
        }
        PlayerId                            getRandomPlayer();
        uint32_t                            getGameSettings()                           {
            return mGameSettings;
        }
        void                                setGameSettings(uint32_t gameSettings)      {
            mGameSettings = gameSettings;
        }


    protected:
        GameManager::GameId                 mGameId;
        GameReportingId                     mGameReportingId;
        GameState                           mGameState;
        uint16_t                            mPlayerCount;           //  Player Count on single stress exe.
        //  PlayerIdList                      payerIdList;
        PlayerIdList                        invitePlayerIdList;     //  Invite list contains the players who are trying to join this game by invite
        ConnectionGroupId                   mDedicatedServerHostConnectionGroupId;
        PlayerId                            mTopologyHostPlayerId;
        ConnectionGroupId                   mTopologyHostConnectionGroupId;
        PlayerId                            mPlatformHostPlayerId;
        //  ConnectionGroupId                 mPlatformHostConnectionGroupId;
        //    int64_t                             mInGameStartTime;
        //  PlayerConnectionGroupIdMap            mPlayerConnectionGroupId;
        PlayerIdListMap                     playerListMap;
        Collections::AttributeMap           mGameAttributeMap;
        PersistedGameId                     mPersistedGameId;
        uint16_t                            mMaxPlayerCapacity;
        GameName                            mGameName;
        double                              mInGameDurationeSec;
        PlayerIdListMap                     mQueuedPlayerList;
        uint32_t                            mGameSettings;
        //  GameTypeName                  mGameTypeName;
        //  GameProtocolVersionString     mGameProtocolVersionString;
        //  PingSiteAlias                 mPingSiteAlias;
        //  GameNetworkTopology           mNetworkTopology;
        //  TeamIdVector                  mTeamIds;
        //  RoleInformation               mRoleInformation;
        //  uint16_t                      mQueueCapacity;
        //  SlotCapacitiesVector          mSlotCapacities;
};

typedef eastl::vector_map<GameManager::GameId, StressGameInfo*> GameInfoMap;

class StressGameData {
        NON_COPYABLE(StressGameData);

    public:
        StressGameData();
        StressGameData(bool isDedicatedServer);
        virtual ~StressGameData();
        //  void     copyData();


        bool                                                isGameIdExist(GameManager::GameId gameId);
        StressGameInfo*                                     addGameData(NotifyGameSetup* gameSetup);
        StressGameInfo*                                     getGameData(GameManager::GameId gameId);
        bool                                                removeGameData(GameManager::GameId gameId);
        bool                                                addPlayerToGameInfo(NotifyPlayerJoining* joiningPlayer);
        bool                                                addPlayerToGameInfo(PlayerId playerId, UserInfo* playerInfo, GameManager::GameId gameId);
        bool                                                addQueuedPlayerToGameInfo(NotifyPlayerJoining* joiningPlayer);
        bool                                                removeQueuedPlayerFromGameInfo(PlayerId playerId, GameManager::GameId gameId);
        bool                                                addInvitedPlayer(PlayerId playerId, GameManager::GameId gameId);
        bool                                                removePlayerFromGameData(PlayerId playerId, GameManager::GameId gameId);
        bool                                                removeInvitedPlayer(PlayerId playerId, GameManager::GameId gameId);
        PlayerIdListMap*                                    getPlayerMap(GameManager::GameId gameId);
        bool                                                isPlayerExistInGame(PlayerId playerId, GameManager::GameId gameId);
        bool                                                isPublicSpectatorPlayer(PlayerId playerId, GameManager::GameId gameId);
        bool                                                isInvitedPlayerExist(PlayerId playerId, GameManager::GameId gameId);
        bool                                                isEmpty();
        bool                                                addPlayerConnectionGroupId(PlayerId playerId, ConnectionGroupId conId, GameManager::GameId gameId);
        void                                                removePlayerConnectionGroupId(PlayerId playerId, GameManager::GameId gameId);
        void                                                setGameState(GameManager::GameId gameId, GameState state);
        GameState                                           getGameState(GameManager::GameId gameId);
        void                                                setPlayerCount(GameManager::GameId gameId, uint16_t count);
        uint16_t                                            getPlayerCount(GameManager::GameId gameId);
        ConnectionGroupId                                   getPlayerConnectionGroupId(PlayerId playerId, GameManager::GameId gameId);
        eastl::string                                       getPlayerPersonaName(PlayerId playerId, GameManager::GameId gameId);
        ExternalId                                          getPlayerExternalId(PlayerId playerId, GameManager::GameId gameId);
	    UserInfo*			                                getPlayerData(PlayerId playerId,GameManager::GameId gameId);
        PlayerIdList&                                       getInvitePlayerIdListByGame(GameManager::GameId gameId);
        GameManager::GameId                                              getRandomGameId(uint32_t maxGameSize = 0);
        GameInfoMap&                                        getGameInfoMap() {
            return mGameDataMap;    //  TODO returning a ref to private data, has to be removed later
        }
    private:
        GameInfoMap::iterator           findGameIdInGameDataMap(GameManager::GameId gameId);

        GameInfoMap                     mGameDataMap;
        mutable EA::Thread::Mutex     mMutex;
        bool                            mIsDedicatedServer;
};

inline StressGameData& DedicatedServerGameDataRef(StressPlayerInfo* ptrPlayerInfo);
inline StressGameData& ESLServerGameDataRef(StressPlayerInfo* ptrPlayerInfo);
inline StressGameData& OnlineSeasonsGameDataRef(StressPlayerInfo* ptrPlayerInfo);
inline StressGameData& FriendliesGameDataRef(StressPlayerInfo* ptrPlayerInfo);
inline StressGameData& LiveCompetitionsGameDataRef(StressPlayerInfo* ptrPlayerInfo);
inline StressGameData& CoopplayerGameDataRef(StressPlayerInfo* ptrPlayerInfo);
inline StressGameData& FutPlayerGameDataRef(StressPlayerInfo* ptrPlayerInfo);
inline StressGameData& ClubsPlayerGameDataRef(StressPlayerInfo* ptrPlayerInfo);
inline StressGameData& DropinPlayerGameDataRef(StressPlayerInfo* ptrPlayerInfo);
inline StressGameData& SsfSeasonsGameDataRef(StressPlayerInfo* ptrPlayerInfo);

inline bool IsPlayerPresentInList(PlayerModule* module, PlayerId  id, PlayerType type);

class PlayerFactory {
        NON_COPYABLE(PlayerFactory);
        PlayerFactory() {}
        ~PlayerFactory() {}

    public:
        static Player*			create(PlayerType type, StressPlayerInfo* playerData);
        static PlayerType		getPlayerType(const char8_t* name);
        static Player*			createRandomPlayer(StressPlayerInfo* playerData, PlayerType* playerType);
        static void				recyclePlayerType(PlayerType type, eastl::string pingSite);
};

class RoleCompare {
    public:
        RoleCompare(const char* strRole): mRole(strRole) {}
        bool operator() (const eastl::pair<PlayerId, UserInfo*>& pairInfo) {
            return (blaze_strcmp((pairInfo.second)->roleName, mRole.c_str()) == 0);
        }

    private:
        const eastl::string mRole;
        RoleCompare& operator=(const RoleCompare &obj);
};

class PartyManager
{
    int64_t             mOpenPartyPersistedId;
    eastl::vector<int64_t>  mOpenPartyMembers;
    uint32_t            mMaxOpenPartyMembers;
    void                resetOpenParty();
    eastl::map<int64_t, eastl::vector<int64_t>> mReadyParties;
public:
    PartyManager() : mOpenPartyPersistedId(0), mMaxOpenPartyMembers(0) {}
    int64_t             createOrJoinAParty(BlazeId playerId, uint32_t memberCount);
    bool                isPartyReady(BlazeId playerId);
    void                partyIsOver(BlazeId playerId);
    eastl::vector<int64_t> getPartyMembers(BlazeId playerId);
};

class EncodedBuffer: public OutputStream {
    public:
        EncodedBuffer(char *resultBuf, size_t resultBufLen) : mResultBuf(resultBuf), mResultBufLen(resultBufLen), mNumWritten(0) {}
        virtual ~EncodedBuffer() {}
        virtual uint32_t write(uint8_t data) {
            if (mResultBufLen <= mNumWritten) {
                return 0;
            }
            mResultBuf[mNumWritten++] = data;
            return 1;
        }
        char* getResultBuf() {
            return mResultBuf;
        }
    private:
        char* mResultBuf;
        size_t mResultBufLen;
        size_t mNumWritten;
};

//   Helper methods
eastl::string IteratePlayers(PlayerIdListMap& playerList);
eastl::string IteratePlayers(PlayerIdList& playerList);
void delay(uint32_t timeInMs);
void delayRandomTime(uint32_t minMs, uint32_t maxMs);
float getRandomFloatVal(float fMin, float fMax);
bool RollDice(const int32_t& threshold);

template <typename String>size_t tokenize(const String& s, const typename String::value_type* pDelimiters, String* resultArray, size_t resultArraySize);


class ReportTelemtryFiber {
    public:
        ReportTelemtryFiber() {
            reset();
        }
        virtual bool executeReportTelemetry() = 0;
        void startReportTelemtery() {
            if (!mIsStarted) {
                runReportTelemetry();
            }
        }
        void runReportTelemetry() {
            if (!mStopReporting) {
                mIsStarted = true;
                if (executeReportTelemetry()) {
                    //  commenting the rescheduling so that only one reportTelemtry call will be made instead of the repeatetive calls
                    //  which might be included in future.
                    //  TimeValue now = TimeValue::getTimeOfDay();
                    //  TimeValue Period = StressConfigInst.getReportTelemetryPeriodMs()*1000;  //  telemtry period TODO: get from config
                    //  TimeValue nextRunTime = now + Period;
                    //  mFiberId = gSelector->scheduleFiberTimerCall(nextRunTime, this, &ReportTelemtryFiber::runReportTelemetry,
                    //    "ReportTelemtryFiber::runReportTelemetry", Fiber::CreateParams(Fiber::STACK_LARGE));
                }
                else
                {
                    reset();
                }
            }
        }

        void         reset() {
            mStopReporting = false;
            mFiberId = 0;
            mIsStarted = false;
        }
        void         stopReportTelemtery()  {
            mStopReporting = true;
        }
        virtual ~ReportTelemtryFiber() {
            //        mStopReporting = false;
            //        if (mFiberId)
            //            delay(30000);  //   if there's a fiber scheduled just wait here before destructing
        }

    protected:
        bool        mStopReporting;
        TimerId     mFiberId;
        bool        mIsStarted;
};

}  // namespace Stress
}  // namespace Blaze

#endif  //   REFERENCE_H
