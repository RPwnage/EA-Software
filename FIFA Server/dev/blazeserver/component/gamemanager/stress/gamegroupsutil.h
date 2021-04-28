/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_GAMEGROUPS_UTIL
#define BLAZE_STRESS_GAMEGROUPS_UTIL

#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/gamebrowser.h"
#include "gamemanager/rpc/gamemanagerslave.h"

#include "stressmodule.h"
#include "stressinstance.h"
#include "attrvalueutil.h"

using namespace Blaze::GameManager;

namespace Blaze
{
namespace Stress
{

class GamegroupData;


///////////////////////////////////////////////////////////////////////////////
//  class GamegroupsUtil
//  
//  Facility for managing game groups within the stress framework.   Allows 
//  multiple StressModules to access existing game groups created during 
//  stress testing.
//  
class GamegroupsUtil
{
public:
    GamegroupsUtil();
    virtual ~GamegroupsUtil();

    //  parses Gamegroups-utility-specific configuration for setup.
    virtual bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil);

    virtual void dumpStats(StressModule *module);

    //  Looks up a game group.
    GamegroupData* getGamegroupData(GameId gid);

    size_t getGamegroupCount() const {
        return mGamegroups.size();
    }

    float getGamegroupsFreq() const {
        return mGamegroupsFreq;
    }
    uint32_t getGamegroupLifespan() const {
        return mGamegroupLifespan;
    }
    uint32_t getGamegroupMemberLifespan() const {
        return mGamegroupMemberLifespan;
    }

    bool getCreateOrJoinPref() const {
        return mCreateOrJoinMethod;
    }
    uint32_t getGamegroupMemberCapacity() const {
        return mGamegroupCapacity;
    }
    uint32_t getAvgNumAttrs() const {
        return mAvgNumAttrs;
    }
    uint32_t getAvgNumMemberAttrs() const {
        return mAvgNumMemberAttrs;
    }
    const char8_t* getAttrPrefix() const {
        return mGamegroupAttrPrefix;
    }
    const char8_t* getMemberAttrPrefix() const {
        return mGamegroupMemberAttrPrefix;
    }

    const AttrValues& getGamegroupAttrValues() const {
        return mGamegroupAttrValues;
    }
    const AttrValues& getMemberAttrValues() const {
        return mMemberAttrValues;
    }
    const AttrValueUpdateParams& getGamegroupAttrUpdateParams() const {
        return mGamegroupAttrUpdateParams;
    }
    const AttrValueUpdateParams& getMemberAttrUpdateParams() const {
        return mMemberAttrUpdateParams;
    }

//  These methods manage data inside the Utility.   For example GamegroupsUtilInstance objects call these methods
public:
    //  Allocates and registers a game group with the utility.
    GamegroupData* registerGamegroup(const ReplicatedGameData &data, ReplicatedGamePlayerList &replPlayerData);

    void unregisterGamegroup(GameId gid);

    size_t incrementPendingGamegroupCount() {
        return (++mPendingGamegroupCount);
    }
    size_t decrementPendingGamegroupCount() {
        return (--mPendingGamegroupCount);
    }

    //  Returns an available game group to join
    GamegroupData* pickGamegroup(bool open=true);

    //    metrics are shown when calling dumpStats
    //    diagnostics for utility not meant to measure load.
    enum Metric
    {
        METRIC_GAMEGROUPS_CREATED,
        METRIC_GAMEGROUPS_DESTROYED,
        METRIC_GAMEGROUPS_REGISTERED,
        METRIC_GAMEGROUPS_UNREGISTERED,
        METRIC_JOIN_GAMEGROUP_FULL,
        METRIC_JOIN_ATTEMPTS,
        METRIC_LEAVE_ATTEMPTS,
        NUM_METRICS
    };

    void addMetric(Metric metric);

    bool removeMember(GameId gid, BlazeId bid, size_t& membersLeft);

    const char8_t* getMockExternalSessions() const { return mMockExternalSessions.c_str(); }

private:
    typedef eastl::hash_map<GameId, GamegroupData*> GameIdToDataMap;
    GameIdToDataMap mGamegroups;
    GameIdToDataMap::iterator mNextAvailableGamegroupIt;
    
    AttrValueUpdateParams mGamegroupAttrUpdateParams;
    AttrValueUpdateParams mMemberAttrUpdateParams;
    AttrValues mGamegroupAttrValues;
    AttrValues mMemberAttrValues;
    
    size_t mPendingGamegroupCount;

    //  configuration parameters
    float mGamegroupsFreq;
    uint32_t mGamegroupLifespan;
    uint32_t mGamegroupCapacity;
    uint32_t mGamegroupMemberLifespan;
    bool mCreateOrJoinMethod;

    uint32_t mAvgNumAttrs;
    uint32_t mAvgNumMemberAttrs;

    char8_t mGamegroupAttrPrefix[48];
    char8_t mGamegroupMemberAttrPrefix[48];

    //    used to display metrics during stat dump
    uint64_t mMetricCount[NUM_METRICS];
    char8_t mLogBuffer[512];

    eastl::string mMockExternalSessions;
};


class GamegroupsUtilInstance : public AsyncHandler
{
public:
    //  each role has different logic in the execute implementation.
    enum Role
    {
        ROLE_NONE,          // NOP
        ROLE_LEADER,        // game group creator and destroyer (role may change during migration.)
        ROLE_JOINER         // game group joiner / leaver (role may change during migration.)
    };

public:
    GamegroupsUtilInstance(GamegroupsUtil *util);
    ~GamegroupsUtilInstance() override;

    //  updates the game groups utility simulation
    virtual BlazeRpcError execute();

    //  if instance logs out, invoke this method to reset state
    void onLogout();

    void setStressInstance(StressInstance *inst);
    void setJoinable(bool joinable);
    void clearStressInstance() {
        setStressInstance(nullptr);
    }
    Role getRole() const {
        return mRole;
    }
    void setRole(Role role) {
        mRole = role;
    }
    StressInstance* getStressInstance() {
        return mStressInst;
    }
    GameManagerSlave* getSlave() {
        return mProxy;
    }
    int32_t getIdent() const {
        return (mStressInst != nullptr) ? mStressInst->getIdent() : -1;
    }
    const GamegroupData* getGamegroupData() const {
        return mUtil->getGamegroupData(mGamegroupJoined);
    }
    BlazeId getBlazeId() const;

    GameId getGamegroupId() const {
        return mGamegroupJoined;
    }

    uint32_t getCyclesLeft() const { return mCyclesLeft; }
    
    void clearCycles() { mCyclesLeft = 0; }

    GamegroupsUtil* getUtil() const { return mUtil; }

protected:
    void onAsync(uint16_t component, uint16_t type, RawBuffer* payload) override;

private:
    BlazeRpcError createOrJoinGamegroup();

    void scheduleUpdateGamePresenceForLocalUser(GameId changedGameId, UpdateExternalSessionPresenceForUserReason change, PlayerRemovedReason removeReason = SYS_PLAYER_REMOVE_REASON_INVALID);
    void updateGamePresenceForLocalUser(GameId changedGameId, UpdateExternalSessionPresenceForUserReason change, PlayerRemovedReason removeReason = SYS_PLAYER_REMOVE_REASON_INVALID);
    void populateGameActivity(GameActivity& activity, const GamegroupData& game);

    GamegroupsUtil *mUtil;
    StressInstance *mStressInst;
    GameManagerSlave *mProxy;
    GameId mGamegroupJoined;
    Role mRole;
    uint32_t mCyclesLeft;
    GameId mPrimaryGameId;
};


class GamegroupData
{
public:
    GamegroupData(const ReplicatedGameData& data, const ReplicatedGamePlayerList &replPlayerData);
    ~GamegroupData();

    void setJoinable(bool joinable) {
        mJoinable = joinable;
    }

    GameId getGamegroupId() const {
        return mGamegroupId;
    }

    const char8_t* getPersistedGameId() const {
        return mPersistedGameId;
    }

    void setPendingHost(BlazeId id) {
        mPendingHostId = id;
    }

    bool updateHost();

    BlazeId getHost() const {
        return mHostId;
    }

    bool isAdmin(BlazeId id) const;

    // Returns a non-host admin, unless the game group's
    // only admin is also its host
    BlazeId getAdmin() const;

    bool hasMember(BlazeId id) const;
    BlazeId getMemberByIndex(size_t index) const;
    BlazeId getNonHostNonAdminMemberByIndex(size_t index) const;

    bool addAdmin(BlazeId blazeId);
    bool removeAdmin(BlazeId blazeId);
    bool addMember(BlazeId blazeId);
    bool removeMember(BlazeId id);

    size_t getMemberCount() const {
        return mMemberMap.size();
    }

    size_t getAdminCount() const {
        return mAdminCount;
    }

    size_t getNonHostNonAdminMemberCount() const;

    size_t getPendingMemberCount() const {
        return mPendingEntryCount;
    }

    //  call to check whether the instance can join a game group
    bool queryJoinGamegroup() const;

    size_t incrementPendingEntryCount() {
        return (++mPendingEntryCount);
    }
    size_t decrementPendingEntryCount() {
        return (--mPendingEntryCount);
    }

    const ExternalSessionIdentification& getExternalSessionIdentification() const { return mExternalSessionIdentification; }

private:
    GameId mGamegroupId;
    BlazeId mHostId;
    BlazeId mPendingHostId;
    char8_t mPersistedGameId[MAX_GAMENAME_LEN];
    uint16_t mMaxPlayerCapacity;

    size_t mAdminCount;
    size_t mPendingEntryCount;
    bool mJoinable;

    typedef eastl::vector_map<BlazeId, bool> MemberMap;
    MemberMap mMemberMap;

    ExternalSessionIdentification mExternalSessionIdentification;
};


}   // Stress
}   // Blaze

#endif
