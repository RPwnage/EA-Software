/*************************************************************************************************/
/*!
    \file Clubsmodule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_CLUBSMODULE_H
#define BLAZE_STRESS_CLUBSMODULE_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#include "EASTL/map.h"
#include "stressmodule.h"
#include "EATDF/time.h"
#include "framework/tdf/attributes.h"
#include "clubs/rpc/clubsslave.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "framework/rpc/usersessionsslave.h"

#include "stressinstance.h"
#include "clubs/tdf/clubs.h"
#include "gamemanager/tdf/gamemanager.h"
#include "stats/statsconfig.h"

namespace Blaze
{
namespace Stress
{

class StressInstance;
class Login;
class ClubsInstance;

class ClubsModule : public StressModule
{
    NON_COPYABLE(ClubsModule);

public:
    enum Action 
    {
        ACTION_SIMULATE_PRODUCTION = 0,
        ACTION_GET_CLUBS,
        ACTION_GET_NEWS,
        ACTION_GET_AWARDS,
        ACTION_GET_CLUBRECORDBOOK,
        ACTION_CREATE_CLUB,
        ACTION_FIND_CLUBS,
        ACTION_FIND_CLUBS_BY_TAGS_ALL,
        ACTION_FIND_CLUBS_BY_TAGS_ANY,
        ACTION_FIND_CLUBS2,
        ACTION_FIND_CLUBS2_BY_TAGS_ALL,
        ACTION_FIND_CLUBS2_BY_TAGS_ANY,
        ACTION_PLAY_CLUB_GAME,
        ACTION_JOIN_CLUB,
        ACTION_JOIN_OR_PETITION_CLUB,
        ACTION_GET_MEMBERS,
        ACTION_GET_CLUBS_COMPONENT_SETTINGS,
        ACTION_GET_CLUB_MEMBERSHIP_FOR_USERS,
        ACTION_POST_NEWS,
        ACTION_PETITION,
        ACTION_INVITATION,
        ACTION_UPDATE_ONLINE_STATUS,
        ACTION_UPDATE_MEMBER_METADATA,
        ACTION_SET_METADATA,
        ACTION_UPDATE_CLUB_SETTINGS,
        ACTION_MAX_ENUM
    };

    ~ClubsModule() override;

    bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil) override;
    StressInstance* createInstance(StressConnection* connection, Login* login) override;
    const char8_t* getModuleName() override {return "clubs";}
    
    Action getAction() const { return mAction; }
    static StressModule* create();

    const char8_t* getRandomLang();
    const char8_t* actionToStr(Action) const;
    const char8_t* actionToContext(Action) const;

    eastl::string mPtrnPersonaName;
    eastl::string mPtrnEmailFormat;
    eastl::string mPtrnName;
    eastl::string mPtrnNonUniqueName;
    eastl::string mPtrnDesc;
    eastl::string mPtrnNewsBody;
    eastl::string mPassword;
    uint32_t mPasswordPercent;  // total percent of clubs that have password

    typedef eastl::map<uint32_t, uint32_t> MemberCountMap;
    typedef eastl::map<Blaze::Clubs::ClubDomainId, uint32_t> DomainPercentageMap;
    DomainPercentageMap mDomainPercentageMap;
    typedef eastl::vector<Blaze::Clubs::ClubDomainId> DomainIdVector;
    DomainIdVector mDomainIdVector;
    typedef eastl::map<Blaze::Clubs::ClubDomainId, Blaze::Clubs::ClubDomainPtr> ClubDomainMap;
    ClubDomainMap mClubDomainMap;
    typedef eastl::map<Blaze::Clubs::ClubDomainId, MemberCountMap*> DomainMemberCounts;
    DomainMemberCounts mDomainMemberCounts;
    typedef eastl::vector<BlazeId> BlazeIdVector;
    typedef eastl::map<Clubs::ClubId, BlazeIdVector> MembershipMap;
    MembershipMap mMembershipMap;
    uint32_t mTotalUsers;
    uint32_t mTotalClubs;
    uint32_t mTotalClubMembers;
    uint64_t mMinAccId; 

    struct FriendCountPercentage
    {
        uint32_t mMinCount;
        uint32_t mMaxCount;
        uint32_t mPercentage;
        // e.g. friendCountPercentages = [ [0,40,80], [41,100,20] ].
        // If current object is created from the first item(mPercentage is 80), mStartPercent is 0 and mEndPercent is 80;
        // else if current object is created from the last item(mPercentage is 20), mStartPercent is 80 and mEndPercent is 100.
        uint32_t mStartPercent; 
        uint32_t mEndPercent;

        FriendCountPercentage() : mMinCount(0), mMaxCount(0), mPercentage(0), mStartPercent(0), mEndPercent(0) {}
        void initialize(uint32_t minCount, uint32_t maxCount, uint32_t percentage, uint32_t startPercent)
        {
            mMinCount = minCount;
            mMaxCount = maxCount;
            mPercentage = percentage;
            mStartPercent = startPercent;
            mEndPercent = startPercent + percentage;
        }
    };
    typedef eastl::vector<FriendCountPercentage> FriendCountPercentageVector;
    FriendCountPercentageVector mFriendCountPercentageVector;
    
    static const uint32_t mMaxTeams = 20;
    static const uint32_t mMaxSeasonLevels = 10;
    static const uint32_t mMaxRegions = 10;
    static const uint32_t mMaxUsersPerClub = 100;
    
    uint32_t mNewsPerClub;
    uint32_t mAwardsPerClub;
    uint32_t mRecordsPerClub;
    uint32_t mTotalTags;
    uint32_t mClubTagPercent; // total percent of clubs that have tags

    // tags to use
    typedef eastl::map<uint32_t, eastl::string> TagMap;
    TagMap mTagMap;

    // for recreating the database
    typedef eastl::map<uint32_t, uint32_t> TagCountMap;
    TagCountMap mTagCountMap;

    // findClubs params
    uint32_t mFindClubsByName;
    uint32_t mFindClubsByLanguage;
    uint32_t mFindClubsByTeamId;
    uint32_t mFindClubsByNonUniqueName;
    uint32_t mFindClubsByAcceptanceFlags;
    uint32_t mFindClubsBySeasonLevel;
    uint32_t mFindClubsByRegion;
    uint32_t mFindClubsByMinMemberCount;
    uint32_t mFindClubsByMaxMemberCount;
    uint32_t mFindClubsByClubFilterList;
    uint32_t mFindClubsByMemberFilterList;
    uint32_t mFindClubsSkipMetadata;
    uint32_t mFindClubsByDomain;
    uint32_t mFindClubsByPassword;
    uint32_t mFindClubsByMemberOnlineStatusCounts;
    uint32_t mFindClubsByMemberOnlineStatusSum;
    uint32_t mFindClubsOrdered;
    uint32_t mFindClubsMaxResultCount;

    // getMembers params
    uint32_t mGetMembersOrdered;
    uint32_t mGetMembersFiltered;
    
    Action mAction;
    
    uint32_t mClubsToGet;
    uint32_t mClubMembershipsToGet;
    uint32_t mTagsToGet;

    uint32_t mActionWeightDistribution[ACTION_MAX_ENUM];
    uint32_t mActionWeightSum;
    
    uint32_t mMaxNoLeaveClubId;

    typedef eastl::list<uint32_t> TagIdList;
    void pickRandomTagIds(TagIdList &tagIdList);
    void pickRandomTagIds(uint32_t numTags, TagIdList &tagIdList);

    static Blaze::Clubs::MemberOnlineStatus getMemberOnlineStatusFromIndex(int statusIndex);
    const Stats::StatsConfigData *getStatsConfig() { return mStatsConfig; }

private:
    friend class ClubsInstance;

    typedef eastl::hash_map<int32_t,ClubsInstance*> InstancesById;

    InstancesById mActiveInstances;

    ClubsModule();
    
    bool parseServerConfig(const ConfigMap& config, const ConfigBootUtil* bootUtil);
    
    bool createDatabase();

    Action strToAction(const char8_t* actionStr) const;
    
    bool mRecreateDatabase;
    uint32_t mStatementBatchSize;
    eastl::string mDbHost;
    uint32_t mDbPort;
    eastl::string mDatabase;
    eastl::string mDbUser;
    eastl::string mDbPass;
    eastl::string mDbStatsTable;
    uint32_t mDbStatsEntityOffset;
    int32_t mRandSeed;

    Stats::StatsConfigData *mStatsConfig;
    Stats::StatsConfig mStatsConfigTdf;

    void pickRandomClubAndPlayer(MembershipMap &clubPlayerMap);
};

class ClubsInstance : public StressInstance,
    public AsyncHandler
{
    NON_COPYABLE(ClubsInstance);

public:
    ClubsInstance(ClubsModule* owner, StressConnection* connection, Login* login);

    ~ClubsInstance() override
    {
        getConnection()->removeAsyncHandler(Blaze::UserSessionsSlave::COMPONENT_ID, Blaze::UserSessionsSlave::NOTIFY_USERSESSIONEXTENDEDDATAUPDATE, this);
        getConnection()->removeAsyncHandler(Blaze::UserSessionsSlave::COMPONENT_ID, Blaze::UserSessionsSlave::NOTIFY_USERADDED, this);
        getConnection()->removeAsyncHandler(Blaze::GameManager::GameManagerSlave::COMPONENT_ID, Blaze::GameManager::GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE, this);
        getConnection()->removeAsyncHandler(Blaze::GameManager::GameManagerSlave::COMPONENT_ID, Blaze::GameManager::GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING, this);
        getConnection()->removeAsyncHandler(Blaze::GameManager::GameManagerSlave::COMPONENT_ID, Blaze::GameManager::GameManagerSlave::NOTIFY_NOTIFYGAMEREPORTINGIDCHANGE, this);

        delete mClubsProxy;
        delete mGameManagerProxy;
    }

    void onLogin(BlazeRpcError result) override;
    void onDisconnected() override;
    void onAsync(uint16_t component, uint16_t type, RawBuffer* payload) override;
    void onUserExtendedData(const Blaze::UserSessionExtendedData& extData);

    BlazeRpcError setValuesForFindClubsRequest(Blaze::Clubs::FindClubsRequest& request);
    BlazeRpcError setRequestForFindClubs(Blaze::Clubs::FindClubsRequest& request);
    BlazeRpcError setRequestForFindClubs2(Blaze::Clubs::FindClubs2Request& request);
    BlazeRpcError setRequestForFindClubsByTagsAll(Blaze::Clubs::FindClubsRequest& request);
    BlazeRpcError setRequestForFindClubsByTagsAny(Blaze::Clubs::FindClubsRequest& request);

    BlazeRpcError actionGetClubs();
    BlazeRpcError actionGetNews();
    BlazeRpcError actionGetAwards();
    BlazeRpcError actionGetClubRecordbook();
    BlazeRpcError actionCreateClub();
    BlazeRpcError actionRemoveMember(Clubs::ClubId clubId);
    BlazeRpcError actionFindClubs();
    BlazeRpcError actionFindClubsByTagsAll();
    BlazeRpcError actionFindClubsByTagsAny();
    BlazeRpcError actionFindClubs2();
    BlazeRpcError actionFindClubs2ByTagsAll();
    BlazeRpcError actionFindClubs2ByTagsAny();
    BlazeRpcError actionPlayClubGame();
    BlazeRpcError actionJoinClub();
    BlazeRpcError actionJoinOrPetitionClub();
    BlazeRpcError actionGetMembers();
    BlazeRpcError actionGetClubsComponentSettings();
    BlazeRpcError actionGetClubMembershipForUsers();
    BlazeRpcError actionPostNews();
    BlazeRpcError actionPetition();
    BlazeRpcError actionInvitation();
    BlazeRpcError actionUpdateOnlineStatus();
    BlazeRpcError actionUpdateMemberMetadata();
    BlazeRpcError actionSetMetadata();
    BlazeRpcError actionUpdateClubSettings();

private:
    const char8_t *getName() const override { return mName; }
    BlazeRpcError execute() override;

    Clubs::ClubId getRandomClub() const;

    ClubsModule* mOwner;
    TimerId mTimerId;
    Blaze::Clubs::ClubsSlave* mClubsProxy;
    Blaze::GameManager::GameManagerSlave* mGameManagerProxy;
    
    const char8_t *mName;

    typedef eastl::vector_set<Clubs::ClubId> ClubIdSet;
    ClubIdSet mClubIdSet;
    BlazeId mBlazeId;
    
    Clubs::ClubList mClubList;
    uint32_t mUsersJoinedGame;
    bool mClubGameOver;
    static uint32_t mInstanceOrdStatic;
    uint32_t mInstanceOrd;
    uint32_t mRunCount;
        
    uint8_t mCreateIt;
    GameManager::GameId mGameId;
    int32_t mResultNotificationError;

    bool tossTheDice(uint32_t expected);
    
    template<class T, class C>
    void copyList(T *destination, const T *source);

    template<class T>
    void destroyList(T *list);

    uint32_t mActionWeightDistribution[ClubsModule::ACTION_MAX_ENUM];
    uint32_t mOutstandingActions;
    ClubsModule::Action getWeightedRandomAction();
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_ClubsMODULE_H


