/*************************************************************************************************/
/*!
    \file associationlistsmodule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_ASSOCIATIONLISTSMODULE_H
#define BLAZE_STRESS_ASSOCIATIONLISTSMODULE_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#include "EASTL/map.h"
#include "EASTL/hash_map.h"
#include "stressmodule.h"
#include "associationlists/rpc/associationlistsslave.h"
#include "messaging/rpc/messagingslave.h"
#include "framework/rpc/usersessionsslave.h"

#include "stressinstance.h"
#include "associationlists/tdf/associationlists.h"

namespace Blaze
{
namespace Stress
{

class StressInstance;
class Login;
class AssociationListsInstance;

class AssociationListsModule : public StressModule
{
    NON_COPYABLE(AssociationListsModule);

public:
    enum Action 
    {
        ACTION_SIMULATE_PRODUCTION = 0,
        ACTION_GET_LISTS,
        ACTION_UPDATE_FRIEND_LIST,
        ACTION_TOGGLE_SUBSCRIPTION,
        ACTION_ADD_RECENT_PLAYER,
        ACTION_FOLLOW_BALANCED,
        ACTION_FOLLOW_AS_CELEBRITY,
        ACTION_FOLLOW_AS_FAN,
        ACTION_MAX_ENUM
    };

    ~AssociationListsModule() override;

    bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil) override;
    StressInstance* createInstance(StressConnection* connection, Login* login) override;
    const char8_t* getModuleName() override {return "associationlists";}
    
    Action getAction() const { return mAction; }
    static StressModule* create();

    const char8_t* actionToStr(Action) const;
    const char8_t* actionToContext(Action) const;

    uint32_t mTotalUsers;
    uint32_t mTargetFriendListSize;
    uint64_t mMinAccId; 

    uint64_t mCelebrityStartAccId;
    uint64_t mCelebrityAccCount;
    // "rate" is based on the run count (AssociationListsInstance::mRunCount)
    uint32_t mFetchMessageRate;
    uint32_t mTransientMessageRate;
    uint32_t mPersistentMessageRate;

    // GLOBAL PERCENTAGES / NUMBERS OF THINGS GO HERE
    typedef eastl::hash_map<uint32_t, uint32_t> ListMaxSizeMap;
    ListMaxSizeMap mListMaxSizes;

    typedef eastl::set<BlazeId> BlazeIdList;
    BlazeIdList mBlazeIdList;

    Action mAction;
    
    uint32_t mActionWeightDistribution[ACTION_MAX_ENUM];
    uint32_t mActionWeightSum;
    
private:
    friend class AssociationListsInstance;

    typedef eastl::hash_map<int32_t, AssociationListsInstance*> InstancesById;

    InstancesById mActiveInstances;

    AssociationListsModule();
    
    bool parseServerConfig(const ConfigMap& config, const ConfigBootUtil* bootUtil);
    
    bool createDatabase();

    Action strToAction(const char8_t* actionStr) const;
    
    bool mRecreateDatabase;
    eastl::string mDbHost;
    uint32_t mDbPort;
    eastl::string mDatabase;
    eastl::string mDbUser;
    eastl::string mDbPass;
    eastl::string mDbStatsTable;
    int32_t mRandSeed;
};

class AssociationListsInstance : public StressInstance,
    public AsyncHandler
{
    NON_COPYABLE(AssociationListsInstance);

public:
    AssociationListsInstance(AssociationListsModule* owner, StressConnection* connection, Login* login);

    ~AssociationListsInstance() override
    {
        delete mAssociationListsProxy;
        delete mMessagingProxy;
        delete mUserSessionsProxy;
    }

    void onLogin(BlazeRpcError result) override;
    void onDisconnected() override;
    void onAsync(uint16_t component, uint16_t type, RawBuffer* payload) override;
    void onUserExtendedData(const Blaze::UserSessionExtendedData& extData);

    BlazeRpcError actionGetLists();
    BlazeRpcError actionUpdateFriendList();
    BlazeRpcError actionToggleSubscription();
    BlazeRpcError actionAddRecentPlayer();
    BlazeRpcError actionFollowBalanced();
    BlazeRpcError actionFollowAsCelebrity();
    BlazeRpcError actionFollowAsFan();

private:
    const char8_t *getName() const override { return mName; }
    BlazeRpcError execute() override;

    BlazeRpcError addFriend();
    BlazeRpcError removeFriend();

    AssociationListsModule* mOwner;
    TimerId mTimerId;
    eastl::hash_map<const char8_t*, TimeValue, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > mNextLogTime; // to minimize "spam" debug logging ('key' is arbitrary)
    Blaze::Association::AssociationListsSlave* mAssociationListsProxy;
    Blaze::Messaging::MessagingSlave* mMessagingProxy;
    UserSessionsSlave* mUserSessionsProxy;

    const char8_t *mName;

    BlazeId mBlazeId;

    typedef eastl::hash_map<BlazeId, Blaze::Association::ListMemberId> ListMemberIdMap;
    typedef eastl::hash_map<Blaze::Association::ListType, ListMemberIdMap> MemberMap;
    MemberMap mMemberMap;
    eastl::hash_map<Blaze::Association::ListType, bool> mSubscribed;

    uint32_t mPersistentMessagesReceived;
    uint32_t mTransientMessagesReceived;
    AssociationListsModule::BlazeIdList mCelebritiesToFollow;
    AssociationListsModule::BlazeIdList mCelebritiesFollowed;

    static uint32_t mInstanceOrdStatic;
    uint32_t mInstanceOrd;
    uint32_t mRunCount;

    uint8_t mCreateIt;
    int32_t mResultNotificationError;

    bool tossTheDice(uint32_t expected);

    uint32_t mActionWeightDistribution[AssociationListsModule::ACTION_MAX_ENUM];
    uint32_t mOutstandingActions;
    AssociationListsModule::Action getWeightedRandomAction();
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_ASSOCIATIONLISTSMODULE_H


