/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_DEDICATEDSERVERMODULE_H
#define BLAZE_STRESS_DEDICATEDSERVERMODULE_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#include "stressmodule.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "gamemanagerutil.h"
#include "gamemanager/tdf/legacyrules.h"

namespace Blaze
{
namespace Stress
{

class StressInstance;
class Login;
class DedicatedServerInstance;
class DedicatedServerModule;

struct RegisteredMachines
{
    uint16_t sPlayerCapacity;
    char8_t sMachineName[Blaze::MAX_UUID_LEN];
    char8_t sGameProtocolVersionString[32];
};

enum Role
{
    ROLE_INVALID = -1,
    ROLE_DSCREATOR, // regular (non-dynamic)GM game creator. top host
    ROLE_GAMERESETER  // GM reseter (platform host in all cases)
};

enum GameEndMode { INVALID_UNSET, DESTROY, RETURN_TO_POOL };

class DedicatedServerModule : public StressModule
{
    NON_COPYABLE(DedicatedServerModule);

public:
    ~DedicatedServerModule() override;

    bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil) override;
    StressInstance* createInstance(StressConnection* connection, Login* login) override;
    const char8_t* getModuleName() override {return "dedicatedserver";}   
    static StressModule* create();

    bool saveStats() override;

    // Probability of user's Role 
    uint16_t getRoleProbability_DsCreator() const { return mRoleProbability_DSCreator; }
    uint16_t getRoleProbability_GameReseter() const { return mRoleProbability_GameReseter; }

    // Incrementor values for GameName
    uint32_t getGameNameIncrementor() const {return mGameNameIncrementor; }
    void incrementGameNameIncrementor() { ++mGameNameIncrementor; }

    // Settings for ResetDedicatedServer
    const CreateGameRequestPtr getCreateGameRequest() const { return mCreateGameRequest;}    

    // Settings for StartMatchMakingSession
    const StartMatchmakingRequestPtr getStartMatchmakingRequest() const { return  mStartMatchmakingRequest; }

    //Settings for DSCreator Information
    RegisteredMachines getDSCreatorRegisterMachines() { return mDDSCreatorRegisteredMachines; }

    uint32_t getGameLifespanCycles() const { return mGameLifespan; }
    uint32_t getGameLifespanDeviation() const { return mGameLifespanDeviation; }

    // Min time before (dynamic/non-dynamic) DS creator will reregister/recreate machines in seconds
    uint32_t getMinMachineRegistrationLifespanCycles() const { return mMachineRegistrationLifespanMin; }
    uint32_t getMachineRegistrationLifespanDeviation() const { return mMachineRegistrationLifespanDeviation; }
    GameEndMode getGameEndMode() const { return mGameEndMode; }
    
    void IncrementNoOfTimesGameCreated() { mNoOfTimesGameCreated += 1; }
    void IncrementNoOfTimesResetDedicatedServerCalled() { mNoOfTimesResetDedicatedServerCalled += 1; }
    void IncrementNoOfTimesStartMatchMakingCalled() { mNoOfTimesStartMatchMakingCalled += 1; }

    const char8_t* getTemplateName() { return mTemplateName; }

private:

    DedicatedServerModule();
    bool parseConfig(const ConfigMap& config);  
    bool parseConfig_RoleProbabilities(const ConfigMap& config);
    bool parseConfig_MachineRegistration(const ConfigMap& config);
    bool parseConfig_CreateGameRequest(const ConfigMap& config);
    bool parseConfig_StartMatchmakingRequest(const ConfigMap& config);
    bool parseConfig_GameEndMode(const ConfigMap& config);
    bool openLogs();

    // Probability of user's Role 
    uint16_t mRoleProbability_DSCreator;
    uint16_t mRoleProbability_GameReseter;

    RegisteredMachines mDDSCreatorRegisteredMachines;

    CreateGameRequestPtr mCreateGameRequest;
    StartMatchmakingRequestPtr mStartMatchmakingRequest; //STRESS_AUDIT: MM role not implemented. For testing MM, may be easier to just reuse mm module (rem code from here)
    
    uint32_t mGameNameIncrementor;  

    uint32_t mNoOfTimesGameCreated;
    uint32_t mNoOfTimesResetDedicatedServerCalled;
    uint32_t mNoOfTimesStartMatchMakingCalled;

    uint32_t mTotalConns;
    FILE* mLogFile;

    GameEndMode mGameEndMode; // Platform host's operation after game lifespan expires
    uint32_t mGameLifespan;
    uint32_t mGameLifespanDeviation;
    uint32_t mMachineRegistrationLifespanMin;
    uint32_t mMachineRegistrationLifespanDeviation;

    char8_t mTemplateName[64];
};


class DedicatedServerInstance : public StressInstance, public AsyncHandler
{
    NON_COPYABLE(DedicatedServerInstance);
    friend class DedicatedServerModule;

public:
    ~DedicatedServerInstance() override;

    void start() override;

    // AsyncHandler implementations
    void onAsync(uint16_t component, uint16_t type, RawBuffer* payload) override;

    // StressInstance implementations
    void onLogin(BlazeRpcError result) override;
    void onLogout(BlazeRpcError result) override;

protected:

    const char8_t *getName() const override;
    BlazeRpcError execute() override;

    const char8_t *getBestPingSiteAlias() override { return mBestPingSiteAlias; }
    void onQosSettingsUpdated() override;

    DedicatedServerInstance(DedicatedServerModule* owner, StressConnection* connection, Login* login);

private:
    DedicatedServerModule* mOwner;
    GameManagerSlave* mProxy;
    const char8_t *mName;
    //GameManagerUtilInstance *mGameManagerInstance; //STRESS_AUDIT: may be able to consolidate much of network related rpcs with GM util's.
    Role mRole;

    BlazeId mBlazeId;
    Blaze::GameManager::GameId mGameId; // for non-DSCreator role
    Blaze::GameManager::NotifyGameSetup mGameInfo; // current game player is in, received at notifyGameSetup
    uint32_t mCyclesLeft; // For DSCreator role, determined by machine registration lifespan. For non-DSCreator role, determined by game lifespan.
    bool mGotPingSiteLatencyUpdate;
    bool mIsWaitingForGameInfo; // flag to indicate waiting for the mGameInfo to be updated.
    const char8_t *mBestPingSiteAlias; // game ping site alias override based on server qos settings (used only if UseServerQosSettings is enabled)
    bool mBestPingSiteChanged;

private:

    Role initRole();
    
    BlazeRpcError actionResetDedicatedServer();
    BlazeRpcError actionDestroyGame();
    BlazeRpcError actionReturnDsToPool();
    BlazeRpcError actionCreateGame_DSCreator(const Blaze::GameManager::CreateGameRequest &createGameRequest, const char8_t* templateName = nullptr);
    BlazeRpcError internalCreateGame(const Blaze::GameManager::CreateGameRequest &createGameRequest, Blaze::GameManager::CreateGameResponse &createGameResponse);
    BlazeRpcError internalCreateGameByTemplateName(const Blaze::GameManager::CreateGameRequest &createGameRequest, const char8_t* templateName,
        Blaze::GameManager::CreateGameResponse &createGameResponse);

    void finalizeGameCreation(GameId gameId, bool platformHost, bool topologyHost);
    void updateHostConnection(GameId gameId, ConnectionGroupId sourceGroupId, ConnectionGroupId topologyGroupId, PlayerNetConnectionStatus connStatus);
    void updatePlayerConns(GameId gameId, UpdateMeshConnectionRequest request);


    BlazeId getBlazeId() const;
    BlazeId getBlazeUserId() const { return getBlazeId(); }  // DEPRECATED
    
    // gets game player is currently in/joining. non-zero value used to block game-players from trying to join multiple games at a time.
    Blaze::GameManager::GameId getGameId() const { return mGameId; }
    void setGameId(Blaze::GameManager::GameId gameId) { mGameId = gameId; mGameInfo.getGameData().setGameId(gameId); }
    Blaze::GameManager::GameState getGameState() const { return mGameInfo.getGameData().getGameState(); }
    void setGameState(Blaze::GameManager::GameState newState) { mGameInfo.getGameData().setGameState(newState); }

    void setIsWaitingForGameInfo() { mIsWaitingForGameInfo = true; }
    void clearIsWaitingForGameInfo() { mIsWaitingForGameInfo = false; }
    bool isWaitingForGameInfo() const { return mIsWaitingForGameInfo; }

    //Find the machines from Registered DS Creator 
    template<class InputIterator, class T>
    uint32_t find ( InputIterator first, InputIterator last, const T& value )
    {
        uint32_t position =-1;
        for ( ;first != nullptr && last != nullptr  && first != last; first++) 
        {
            position++;
            if ( *first==value )
            {  
                break;
            }
        }
        return position;
    }

    static uint32_t generateRandomValue(uint32_t minValue, uint32_t deviation) { return (minValue + deviation - ((deviation > 0)? (rand() % (deviation*2)) : 0)); }

    bool validatePingSiteLatenciesUpdated();
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_DEDICATEDSERVERMODULE_H

