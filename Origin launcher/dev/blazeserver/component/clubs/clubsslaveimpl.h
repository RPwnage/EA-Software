/*************************************************************************************************/
/*!
    \file   clubsslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CLUBSCOMPONENT_SLAVEIMPL_H
#define BLAZE_CLUBSCOMPONENT_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "EASTL/list.h"
#include "EASTL/hash_map.h"
#include "EASTL/string.h"
#include "EASTL/map.h"

#include "clubs/rpc/clubsslave_stub.h"
#include "clubs/tdf/clubs.h"
#include "clubs/clubsconstants.h"
#include "clubs/clubsdb.h"
#include "clubs/seasonrollover.h"
#include "clubs/clubsidentityprovider.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/userset/userset.h"
#include "framework/database/dbconn.h"
#include "framework/petitionablecontent/petitionablecontent.h"
#include "framework/util/random.h"
#include "framework/component/censusdata.h" // for CensusDataManager & CensusDataProvider
#include "framework/controller/transaction.h"
#include "framework/controller/drainlistener.h"
#include "framework/taskscheduler/taskschedulerslaveimpl.h"
#include "framework/metrics/metrics.h"

//stats
#include "stats/tdf/stats.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define CLUBS_MINUTE_INTERVAL 60000000LL
#define CLUBS_DAY_INTERVAL 86400000000LL

// Event log string keys
#define LOG_EVENT_CREATE_CLUB "createClub"
#define LOG_EVENT_JOIN_CLUB "joinClub"
#define LOG_EVENT_LEAVE_CLUB "leaveClub"
#define LOG_EVENT_REMOVED_FROM_CLUB "removedFromClub"
#define LOG_EVENT_AWARD_TROPHY "awardTrophy"
#define LOG_EVENT_GM_PROMOTED "gmPromoted"
#define LOG_EVENT_GM_DEMOTED "gmDemoted"
#define LOG_EVENT_OWNERSHIP_TRANSFERRED "ownershipTransferred"
#define LOG_EVENT_FORCED_CLUB_NAME_CHANGE "forcedClubNameChange"
#define LOG_EVENT_MAX_CLUB_NAME_CHANGES "maxClubNameChanges"

// We don't track offline users in our map of MemberOnlineStatus counts per club, so we use index 0
// (corresponding to MemberOnlineStatus CLUBS_MEMBER_OFFLINE) to store the total count of online users
#define CLUBS_TOTAL_STATUS_COUNT_INDEX 0

namespace Blaze
{

namespace Metrics
{
    namespace Tag
    {
        extern TagInfo<Clubs::ClubDomain>* club_domain;
    }
}
namespace Stats
{
    class StatsSlave;
}

namespace Clubs
{
class ClubsDbConnector;

class ClubsTransactionContext : public TransactionContext 
{
    NON_COPYABLE(ClubsTransactionContext);

public:
    DbConnPtr getDatabaseConnection() { return mDbConnPtr; }

private:
    friend class ClubsSlaveImpl;   

    ClubsTransactionContext(const char* description, ClubsSlaveImpl &component);
    ~ClubsTransactionContext() override;

protected:
    // following have to be implemented and are invoked only by Component class   
    BlazeRpcError commit() override;   
    BlazeRpcError rollback() override; 

private:
    DbConnPtr mDbConnPtr;
};


class ClubsIdentityProvider;

typedef struct ClubsEvent
{
    ClubId clubId;
    char8_t message[512];
    struct
    {
        NewsParamType type;
        char8_t value[32];
    } args[4];
    int32_t numArgs;
} ClubsEvent;

typedef struct ClubMemberStatus
{
    MembershipStatus membershipStatus;
    MemberOnlineStatus onlineStatus;
} ClubMemberStatus;

typedef eastl::map<ClubId, ClubMemberStatus> ClubMemberStatusList;

class ClubsSlaveImpl : public ClubsSlaveStub, 
    private ExtendedDataProvider,
    private LocalUserSessionSubscriber,
    private UserSetProvider,
    private TaskEventHandler,
    private PetitionableContentProvider,
    private CensusDataProvider,
    private DrainStatusCheckHandler
{
public:
    ClubsSlaveImpl();
    ~ClubsSlaveImpl() override;

    uint16_t getDbSchemaVersion() const override { return 9; }
    const DbNameByPlatformTypeMap* getDbNameByPlatformTypeMap() const override { return &getConfig().getDbNamesByPlatform(); }

    uint32_t getDbAttempts() const { return mDbAttempts; }
    uint32_t getFetchSize() const { return mFetchSize; }

    const ClubDomain* getClubDomain(ClubDomainId domainId) const;

    void removeIdentityCacheEntry(ClubId clubId);

    BlazeRpcError logEvent(ClubsDatabase* clubsDb, ClubId clubId, const char8_t* message,
            NewsParamType arg0type = NEWS_PARAM_NONE, const char8_t* arg0 = nullptr,
            NewsParamType arg1type = NEWS_PARAM_NONE, const char8_t* arg1 = nullptr,
            NewsParamType arg2type = NEWS_PARAM_NONE, const char8_t* arg2 = nullptr,
            NewsParamType arg3type = NEWS_PARAM_NONE, const char8_t* arg3 = nullptr);

    const char8_t* getEventString(const char8_t* key) const;

    // functions used to purge inactive/stale objects    
    uint32_t purgeInactiveClubs(DbConnPtr& conn, ClubDomainId domainId, uint32_t inactiveThreshold); //defined in purgeinactiveclubs.cpp
    BlazeRpcError purgeStaleTickerMessages(DbConnPtr& conn, uint32_t tickerMessageExpiryDay) const; //defined in clubsmessages.cpp

    BlazeRpcError logEventAsNews(ClubsDatabase* clubsDb, const ClubsEvent *clubEvent); //defined in logevents.cpp
    
    BlazeRpcError sendMessagingMsgToNotifyUser(BlazeId receiver) const;
    
    BlazeRpcError sendMessagingMsgToGMs(
        ClubId receiverClub, 
        const char8_t* clubName,
        PetitionMessageType msgType,
        uint32_t msgId,
        bool sendPersistent);

    BlazeRpcError sendMessagingMsgPetition(
        BlazeId receiver, 
        PetitionMessageType msgType,
        uint32_t msgId,
        const char8_t *clubName) const;
    
    // helper function used by custom code to insert a message into
    // clubs_tickermsgs table and send notifications to subscribed client 
    BlazeRpcError publishClubTickerMessage(
        ClubsDatabase *clubsDb,
        const bool notify,                 // set to true if you want to notify subscribed users
        const ClubId clubId,                  // club id, 0 to broatcast to all clubs 
        const BlazeId includeBlazeId,           // private message for blazeId, 0 to broadcast to all users
        const BlazeId excludeBlazeId,           // exclude user, 0 to broadcast to all users
        const char8_t *metadata,              // metadata
        const char8_t *text,                  // message text or message id for localized messages
        const char8_t *param1 = nullptr,         // optional message parameters for localized messages
        const char8_t *param2 = nullptr,
        const char8_t *param3 = nullptr,
        const char8_t *param4 = nullptr,
        const char8_t *param5 = nullptr);
    
    // defined in clusmessages.cpp
    BlazeRpcError setClubTickerSubscription(
        const ClubId clubId,
        const BlazeId blazeId,
        bool isSubscribed);

    bool processEndOfSeason(Stats::StatsSlave* component, DbConnPtr& dbConn);
    void startEndOfSeason();
    void doStartEndOfSeason(BlazeRpcError& error);

    // implemented in clubsmessages.cpp
    void onPublishClubTickerMessageNotification(const PublishClubTickerMessageRequest& message,Blaze::UserSession *) override;

    BlazeRpcError onMemberAdded(ClubsDatabase & clubsDb, 
        const Club& club, const ClubMember& member);
    BlazeRpcError onMemberRemoved(ClubsDatabase * clubsDb, 
        const ClubId clubId, const BlazeId blazeId, bool bLeave, bool bClubDelete);
    BlazeRpcError onMemberOnline(ClubsDatabase * clubsDb, 
        const ClubIdList &clubIdList, const BlazeId blazeId);
    BlazeRpcError onMemberMetadataUpdated(ClubsDatabase * clubsDb, 
        const ClubId clubId, const ClubMember* member);
    BlazeRpcError onSettingsUpdated(ClubsDatabase * clubsDb, const ClubId clubId,
        const ClubSettings* originalSettings, const ClubSettings* newSettings);
    BlazeRpcError onClubCreated(ClubsDatabase * clubsDb, 
        const char8_t* clubName, const ClubSettings& clubSettings, const ClubId clubId, const ClubMember& owner);
    BlazeRpcError onAwardUpdated(ClubsDatabase * clubsDb, const ClubId clubId,
        const ClubAwardId AwardId);
    BlazeRpcError onRecordUpdated(ClubsDatabase *clubsDb,
        const ClubRecordbookRecord *record);
    BlazeRpcError onMaxRivalsReached(ClubsDatabase *clubsDb,
        const ClubId clubId, bool &handled); 
    BlazeRpcError onInvitationSent(ClubsDatabase * clubsDb, 
            const ClubId clubId, const BlazeId inviterId, const BlazeId inviteeId);

    // notification custom code hooks
    BlazeRpcError notifyInvitationAccepted(
            const ClubId clubId, 
            const BlazeId accepterId);
    BlazeRpcError notifyInvitationDeclined(
            const ClubId clubId, 
            const BlazeId declinerId);
    BlazeRpcError notifyInvitationRevoked(
            const ClubId clubId, 
            const BlazeId revokerId, 
            const BlazeId inviteeId);
    BlazeRpcError notifyInvitationSent(
            const ClubId clubId, 
            const BlazeId inviterId,
            const BlazeId inviteeId);
    BlazeRpcError notifyPetitionSent( 
            const ClubId clubId, 
            const char8_t *clubName,
            const uint32_t petitionId, 
            const BlazeId petitionerId);
    BlazeRpcError notifyPetitionAccepted(
            const ClubId clubId, 
            const char8_t *clubName, 
            const uint32_t petitionId, 
            const BlazeId accepterId, 
            const BlazeId petitionerId);
    BlazeRpcError notifyPetitionRevoked(
            const ClubId clubId, 
            const uint32_t petitionId,
            const BlazeId petitionerId);
    BlazeRpcError notifyPetitionDeclined(
            const ClubId clubId, 
            const char8_t *clubName,
            const uint32_t petitionId,
            const BlazeId declinerId, 
            const BlazeId petitionerId);
    BlazeRpcError notifyUserJoined(
            const ClubId clubId, 
            const BlazeId joinerId);
    BlazeRpcError notifyNewsPublished(
            const ClubId clubId, 
            const BlazeId publisherId,
            const ClubNews &news);
    BlazeRpcError notifyMemberRemoved(
            const ClubId clubId, 
            const BlazeId removerId,
            const BlazeId removedMemberBlazeId);
    BlazeRpcError notifyMemberPromoted(
            const ClubId clubId, 
            const BlazeId promoterId,
            const BlazeId promotedBlazeId);
    BlazeRpcError notifyGmDemoted(
        const ClubId clubId, 
        const BlazeId demoterId,
        const BlazeId demotedBlazeId);
    BlazeRpcError notifyOwnershipTransferred(
            const ClubId clubId, 
            const BlazeId transferrerId,
            const BlazeId oldOwnerId,
            const BlazeId newOwnerId);

    // rival custom helper functions
    BlazeRpcError assignRival(ClubsDatabase * clubsDb, 
        const Club* club, bool useLeaderboard);  // implemented in custom/rivals.cpp
    BlazeRpcError setIsRankedOnRivalLeaderboard( 
        const ClubId clubId, bool isRanked); // implemented in custom/rivals.cpp
    BlazeRpcError checkIsRivalAvailable(ClubsDatabase * clubsDb, 
        const ClubId clubId, bool &isAvailable); // implemented in custom/rivals.cpp
     
    // utils (implemented in utils.cpp)
    BlazeRpcError checkClubStrings(
        const char8_t *clubName, 
        const char8_t *nonUniqueName,
        const char8_t *clubDescription,
        const char8_t *clubPassword = nullptr) const;
    
    BlazeRpcError checkClubTags(
        const ClubTagList& tagList) const;

	BlazeRpcError checkClubMetadata(
		const ClubSettings& clubSettings) const;

    // If requester has no privilege to access club passwords, those passwords should be set empty
    BlazeRpcError filterClubPasswords(ClubList& resultList);

    BlazeRpcError getNewsForClubs(
        const ClubIdList& requestorClubIdList, 
        const NewsTypeList& typeFilters,
        const TimeSortType sortType,
        const uint32_t maxResultCount,
        const uint32_t offSet,
        const uint32_t locale,
        ClubLocalizedNewsListMap& clubLocalizedNewsListMap,
        uint16_t& totalPages);

    BlazeRpcError getPetitionsForClubs(
        const ClubIdList& requestorClubIdList, 
        const PetitionsType petitionsType,
        const TimeSortType sortType,
        ClubMessageListMap& clubMessageListMap);

    BlazeRpcError transferClubServerNewsToLocalizedNews(
        const ClubId requestorClubId,
        const eastl::list<ClubServerNewsPtr>& clubServerNews,
        const uint32_t locale,
        bool& isHiddenNews,
        bool getHiddenNews,
        ClubLocalizedNewsList& clubLocalizedNewsList,
        const bool& getAssociateNews);

    BlazeRpcError getClubTickerMessagesForClubs(const ClubIdList& requestorClubIdList,
        const uint32_t oldestTimestamp,
        const uint32_t locale,
        ClubTickerMessageListMap& clubTickerMessageListMap);

    BlazeRpcError insertNewsAndDeleteOldest(ClubsDatabase& clubsDb, ClubId clubId, BlazeId contentCreator, const ClubNews &news);
    BlazeRpcError countMessagesForClubs(const ClubIdList& requestorClubIdList, const MessageType messageType, ClubMessageCountMap &messageCountMap);

    // replicated cache updates
    BlazeRpcError updateCachedDataOnNewUserAdded(
        ClubId clubId,
        const ClubName& clubName,
        ClubDomainId domainId,
        const ClubSettings& clubSettings,
        BlazeId blazeId,
        MembershipStatus membershipStatus,
        uint32_t memberSince,
        UpdateReason reason);

    void addClubDataToCache(
        ClubId clubId,
        ClubDomainId domainId,
        const ClubSettings& clubSettings,
        const ClubName& clubName,
        uint64_t version);
    BlazeRpcError addClubsDataToCache(ClubsDatabase *clubsDb, const ClubIdList& clubIds);

    void addClubMemberInfoToCache(BlazeId blazeId, ClubId clubId, MembershipStatus membershipStatus, uint32_t memberSince) const;

    bool isMock() const;

    typedef eastl::map<ClubId, CachedClubDataPtr> ClubDataCache;
    typedef eastl::map<BlazeId, CachedMemberInfoPtr> BlazeIdToMemberInfoMap;
    typedef eastl::map<ClubId, BlazeIdToMemberInfoMap> ClubMemberInfoCache;
    typedef eastl::vector<uint32_t> OnlineStatusCounts;
    typedef eastl::map<ClubId, OnlineStatusCounts*> OnlineStatusCountsForClub;
    typedef eastl::map<ClubId, MemberOnlineStatus> OnlineStatusInClub;
    typedef eastl::map<BlazeId, OnlineStatusInClub*> OnlineStatusForUserPerClubCache;

    const ClubDataCache& getClubDataCache() const { return mClubDataCache; }
    const OnlineStatusForUserPerClubCache& getOnlineStatusForUserPerClubCache() const { return mOnlineStatusForUserPerClubCache; }
    const OnlineStatusCountsForClub& getOnlineStatusCountsForClub() const { return mOnlineStatusCountsForClub; }

    void updateCachedClubInfo(uint64_t version, ClubId clubId, const Club& updatedClub);
    void invalidateCachedClubInfo(ClubId clubId, bool removeIdentityProviderCacheEntry = false);
    void invalidateCachedMemberInfo(BlazeId blazeId, ClubId clubId);

    BlazeRpcError updateCachedMamberInfoOnTransferOwnership(
        BlazeId oldOwner, 
        BlazeId newOwner,
        MembershipStatus exOwnersNewStatus,
        ClubId clubId);
        
    BlazeRpcError getMemberStatusInClub(
        const ClubId clubId,
        const BlazeId blazeId,
        ClubsDatabase &clubsDb,
        MembershipStatus &membershipStatus,
        MemberOnlineStatus &onlineStatus,
        TimeValue &memberSince);

    BlazeRpcError getMemberStatusInClub(
        const ClubId clubId,
        const BlazeIdList& blazeIdList,
        ClubsDatabase &clubsDb,
        MembershipStatusMap &membershipStatusMap);

    // Gets member status from cache if available, if not
    // gets member status from database unless forceDbLookup is
    // true when information is always looked up in db.
    // Use forced database lookups when you need to make decision
    // about changing club in database. Remember to lock the club
    // first.
    BlazeRpcError getMemberStatusInDomain(
        ClubDomainId domainId,
        BlazeId blazeId,
        ClubsDatabase &clubsDb,
        ClubMemberStatusList &statusList,
        bool forceDbLookup = false);
        
    MemberOnlineStatus getMemberOnlineStatus(
        const BlazeId blazeId,
        const ClubId clubId);

    void getMemberOnlineStatus(
        const BlazeIdList& blazeIdList,
        const ClubId clubId,
        MemberOnlineStatusMapForClub& statusMap);

    // Override online status based on the requestorId and his first party privacy status with the other users
    void overrideOnlineStatus(MemberOnlineStatusMapByExternalId& statusMap);
    void overrideOnlineStatus(ClubMemberList& memberList);

    bool allowOnlineStatusOverride() const;

    BlazeRpcError getClubDomainForClub(
        ClubId clubId, 
        ClubsDatabase &clubsDb,
        const ClubDomain* &clubDomain);

    // fetched online status counts from replicated map
    void getClubOnlineStatusCounts(ClubId clubId, MemberOnlineStatusCountsMap &map);

    BlazeRpcError checkPetitionAlreadySent(
        ClubsDatabase &clubsDb, 
        BlazeId requestorId,
        ClubId clubId);

    BlazeRpcError checkInvitationAlreadySent(
        ClubsDatabase &clubsDb,
        BlazeId blazeId,
        ClubId clubId) const;

    // extended user data helpers
    BlazeRpcError updateMemberUserExtendedData(BlazeId blazeId, ClubId clubId, UpdateReason reason, MemberOnlineStatus status);
    BlazeRpcError updateMemberUserExtendedData(BlazeId blazeId, ClubId clubId, ClubDomainId domainId, UpdateReason reason, MemberOnlineStatus status);
    BlazeRpcError removeMemberUserExtendedData(BlazeId blazeId, ClubId clubId, UpdateReason reason);

    // club rivals methods
    BlazeRpcError insertClubRival(DbConnPtr& dbCon, const ClubId clubId, const ClubRival &clubRival);
    BlazeRpcError updateClubRival(DbConnPtr& dbCon, const ClubId clubId, const ClubRival &clubRival) const;
    BlazeRpcError deleteClubRivals(DbConnPtr& dbCon, const ClubId clubId, const ClubIdList &rivalIdList) const;
    BlazeRpcError deleteOldestClubRival(DbConnPtr& dbCon, const ClubId clubId) const;

    // interface for the Reporting component {
    BlazeRpcError reportResults(
        DbConnPtr& dbConn,
        ClubId upperId,
        const char8_t *gameResultForUpperClub,
        ClubId lowerId,
        const char8_t *gameResultForLowerClub);

    BlazeRpcError getClub(DbConnPtr& dbConn, Club &club) const;

    BlazeRpcError getClubSettings(DbConnPtr& dbConn, ClubId clubid, ClubSettings &clubSettings);

    BlazeRpcError updateClubRecord(DbConnPtr& dbConn, ClubRecordbookRecord &record);

    BlazeRpcError resetClubRecords(DbConnPtr& dbCon, ClubId clubId, const ClubRecordIdList &recordIdList);

    BlazeRpcError lockClubs(DbConnPtr& dbConn, ClubIdList *clubsToLock);

    bool checkClubId(DbConnPtr& dbConn, ClubId clubId);
    bool checkMembership(DbConnPtr& dbConn, ClubId clubId, BlazeId blazeId);
    
    BlazeRpcError awardClub(
        DbConnPtr& dbConn,
        ClubId clubId, 
        ClubAwardId awardId);
        
    BlazeRpcError listRivals(DbConnPtr& dbConn, ClubId clubId, ClubRivalList &rivalList) const;

    BlazeRpcError getClubMembers(
        DbConnPtr& dbConn,
        ClubId clubId,
        uint32_t maxResultCount,
        uint32_t offset,
        MemberOrder orderType,
        OrderMode orderMode,
        MemberTypeFilter memberType,
        bool skipCalcDbRows,
        const char8_t* personaNamePattern,
        Clubs::GetMembersResponse &response);

    BlazeRpcError getClubsComponentSettings(ClubsComponentSettings &settings);
    // } interface for reporting component

    const ClubsComponentInfo& getClubsComponentInfo() { return mClubsCompInfo; }

    bool getXblSocialPeopleRelationshipHelper(const BlazeId ownerBlazeId, const ExternalId ownerExtId, const ExternalId memberExtId, bool forceRefresh) const;

    bool isClubGMFriend(const ClubId clubId, const BlazeId blazeId, ClubsDbConnector& dbc) const;
    bool isClubJoinAccepted(const ClubSettings& clubSettings, const bool isGMFriend) const;
    bool isClubPasswordCheckSkipped(const ClubSettings& clubSettings, const bool isGMFriend) const;
    bool isClubPetitionAcctepted(const ClubSettings& clubSettings, const bool isGMFriend) const;
    BlazeRpcError joinClub(const ClubId joinClubId, const bool skipJoinAcceptanceCheck, const bool petitionIfJoinFails, const bool skipPasswordCheck, const ClubPassword& password,
        const BlazeId rtorBlazeId, const bool isGMFriend, const uint32_t invitationId, ClubsDbConnector& dbc, uint32_t& messageId);
    BlazeRpcError petitionClub(const ClubId petitionClubId, const ClubDomain *domain, const BlazeId rtorBlazeId, bool skipMembershipCheck,
        ClubsDbConnector& dbc, uint32_t& messageId);

    // metrics
    Metrics::Gauge mPerfGetNewsTimeMsHighWatermark;
    Metrics::Gauge mPerfFindClubsTimeMsHighWatermark;
    Metrics::TaggedCounter<ClubDomain> mPerfClubsCreated;
    Metrics::Counter mPerfMessages;
    Metrics::Counter mPerfClubGamesFinished;
    Metrics::Counter mPerfAdminActions;
    Metrics::Counter mPerfJoinsLeaves;
    Metrics::Counter mPerfMemberLogins;
    Metrics::Counter mPerfMemberLogouts;
    Metrics::PolledGauge mPerfCachedClubs;
    Metrics::PolledGauge mPerfOnlineClubs;
    Metrics::PolledGauge mPerfOnlineMembers;
    Metrics::TaggedCounter<ClubDomain> mPerfInactiveClubsDeleted;
    Metrics::TaggedGauge<ClubDomain> mPerfUsersInClubs;
    Metrics::TaggedGauge<ClubDomain> mPerfClubs;

    void getStatusInfo(ComponentStatus& status) const override;

    typedef eastl::map<ClubId, eastl::list<BlazeId> > TickerSubscriptionList;
    TickerSubscriptionList mTickerSubscriptionList;

    // ticker message helper functions defined in clubsmessages.cpp
    void localizeTickerMessage(
        const char8_t *origText, 
        const char8_t *msgParams, 
        const uint32_t locale,
        eastl::string &text) const;

    uint32_t getNextAsyncSequenceId()
    {
        return ++mAsyncSequenceId;
    }

    MemberOnlineStatus getOnlineStatus(BlazeId blazeId) const;

    ///////////////////////////////////////////////////////////////////////////
    // UserSetProvider interface functions
    //
    /* \brief Find all UserSessions matching EA::TDF::ObjectId (fiber) */
    BlazeRpcError getSessionIds(const EA::TDF::ObjectId& bobjId, UserSessionIdList& ids) override;

    /* \brief Find all blazeIds matching EA::TDF::ObjectId (fiber) */
    BlazeRpcError getUserBlazeIds(const EA::TDF::ObjectId& bobjId, BlazeIdList& results) override;

    /* \brief Find all UserIdents matching EA::TDF::ObjectId (fiber) */
    BlazeRpcError getUserIds(const EA::TDF::ObjectId& bobjId, UserIdentificationList& results) override;

    /* \brief Count all UserSessions matching EA::TDF::ObjectId (fiber) */
    BlazeRpcError countSessions(const EA::TDF::ObjectId& bobjId, uint32_t& count) override;

    /* \brief Count all Users matching EA::TDF::ObjectId (fiber) */
    BlazeRpcError countUsers(const EA::TDF::ObjectId& bobjId, uint32_t& count) override;

    RoutingOptions getRoutingOptionsForObjectId(const EA::TDF::ObjectId& bobjId) override 
    { 
        // It's unclear what instance should be used to lookup club news and club information:  (Just using the local instance currently)
        return RoutingOptions::fromInstanceId(getLocalInstanceId()); 
    }

    inline const RecordSettingItemList& getRecordSettings() const { return getConfig().getRecordSettings(); }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////// CensusDataProvider interface ///////////////////////////////////////////
    BlazeRpcError getCensusData(CensusDataByComponent& censusData) override;

    bool isUsingClubKeyscopedStats();

    BlazeRpcError resetStadiumName(ClubId clubId, eastl::string& statusMessage);
    BlazeRpcError fetchStadiumName(ClubId clubId, eastl::string &stadiumName);

private:    
    typedef eastl::map<ClubDomainId, const ClubDomain *> ClubDomainsById;
    ClubDomainsById mClubDomains;

    ClubsIdentityProvider* mIdentityProvider;

    ClubsComponentInfo mClubsCompInfo;

    uint32_t mAsyncSequenceId;

    bool mFirstSnapshot;

    bool onConfigure() override;
    bool onConfigureReplication() override;
    void getClubsConfigValues();
    bool onResolve() override;
    void onShutdown() override;
    bool onCompleteActivation() override;
    bool isDrainComplete() override;
    bool onValidateConfig(ClubsConfig& config, const ClubsConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
    void validateClubUpdateStats(ClubsConfig& config, ConfigureValidationErrors& validationErrors) const;

    uint32_t mDbAttempts;
    uint32_t mFetchSize;

    SeasonRollover mSeasonRollover;
    
    void checkConfigUInt16(int16_t configVal, const char8_t *name, ConfigureValidationErrors& validationErrors) const;
    void checkConfigUInt32(int32_t configVal, const char8_t *name, ConfigureValidationErrors& validationErrors) const;
    void checkConfigString(const char8_t *configVal, const char8_t *name, ConfigureValidationErrors& validationErrors) const;
    
    // If the "club" keyscope is not defined, we can skip some stat processing attempts
    bool mIsUsingClubKeyscopedStats;
    bool mIsUsingClubKeyscopedStatsSet; 

    void checkEventMappingString(const EventStringsMap& eventStringsMap, const char8_t *key, ConfigureValidationErrors& validationErrors) const;
    
    // event handlers for User Session Manager events
    void onLocalUserSessionLogin(const UserSessionMaster& userSession) override;
    void onLocalUserSessionLogout(const UserSessionMaster& userSession) override;

    void doOnLocalUserSessionLogin(UserSessionMasterPtr userSession);
    void doOnLocalUserSessionLogout(BlazeId blazeId, UserSessionId sessionId);
    void updateOnlineStatusOnExtendedDataChange(ClubId clubId, BlazeId blazeId, MemberOnlineStatus status);
    RedisError broadcastOnlineStatusChange(const NotifyUpdateMemberOnlineStatus& request, MemberOnlineStatus status, bool refreshLocalOnlineStatusCache = false, bool confirmNoninteractive = false);

    void handleOnlineStatusUpdate(const NotifyUpdateMemberOnlineStatusPtr request, bool invalidateClubMemberInfoCache);

    // function called from scheduled fiber call 
    void purgeStaleObjectsFromDatabase();

    // event handlers for ExtendedDataProvider
    BlazeRpcError onLoadExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData) override;
    virtual bool lookupExtendedData(const UserSessionMaster& session, const char8_t* key, UserExtendedDataValue& value) const { return false; }
    BlazeRpcError onRefreshExtendedData(const Blaze::UserInfoData& ,Blaze::UserSessionExtendedData &) override { return ERR_OK; };
    void onInvalidateCacheNotification(const Blaze::Clubs::InvalidateCacheRequest& data, UserSession *associatedUserSession) override;
    void onUpdateCacheNotification(const Blaze::Clubs::UpdateCacheRequest& data, UserSession *associatedUserSession) override;
    void onUpdateClubsComponentInfo(const Blaze::Clubs::ClubsComponentInfo& data, UserSession *associatedUserSession) override;
    void onUpdateOnlineStatusNotification(const Blaze::Clubs::NotifyUpdateMemberOnlineStatus& data, UserSession *associatedUserSession) override;

    void doTakeComponentInfoSnapshot(BlazeRpcError& error);
    BlazeRpcError takeComponentInfoSnapshot();

    // transaction context implementation override
    BlazeRpcError createTransactionContext(uint64_t customData, TransactionContext*& result) override;
    
    // petitionable content provider interface
    /* \brief Fetch petitionable content hosted by PetitionableContentProvider regardless of visibility */
    BlazeRpcError fetchContent(const EA::TDF::ObjectId& bobjId, Collections::AttributeMap& attributeMap, eastl::string& url) override;
    /* \brief Show/Hide petitionable content hosted by PetitionableContentProvider */
    BlazeRpcError showContent(const EA::TDF::ObjectId& bobjId, bool visible) override;
    BlazeRpcError getClubMetadata(ClubId clubId, ClubMetadata& clubMetadata);

    Stats::StatsSlave* getStatsSlave(BlazeRpcError* waitResult) const;

    const Blaze::Stats::StatCategorySummary* findInStatCategoryList(const char8_t* categoryName, const Blaze::Stats::StatCategoryList::StatCategorySummaryList& categories) const;
    bool findInStatKeyScopeList(const char8_t* keyScopeName, const Blaze::Stats::StatCategorySummary::StringScopeNameList& keyScopeList) const;
    const Blaze::Stats::StatDescSummary* findInStatDescList(const char8_t* statName, const Blaze::Stats::StatDescs::StatDescSummaryList& statList) const;

    bool updateLocalOnlineStatusAndCounts(ClubId clubId, BlazeId blazeId, MemberOnlineStatus status, bool allowOverwriteWithNoninteractive = true, bool deleteOnlineStatusInClub = true);

    // These are marked as mutable because there are const methods
    // that may need to update the local cache if the cache doesn't contain the entry
    mutable ClubDataCache mClubDataCache;
    mutable ClubMemberInfoCache mClubMemberInfoCache;
    OnlineStatusCountsForClub mOnlineStatusCountsForClub;
    OnlineStatusForUserPerClubCache mOnlineStatusForUserPerClubCache;

    typedef RedisKeyFieldMap<BlazeId, ClubId, MemberOnlineStatusValue> OnlineStatusByClubIdByBlazeIdFieldMap;
    OnlineStatusByClubIdByBlazeIdFieldMap mOnlineStatusByClubIdByBlazeIdFieldMap;

    // TaskEventHandler Interface
    TaskSchedulerSlaveImpl *mScheduler;
    eastl::string mSeasonRolloverTaskName;
    TaskId mInactiveClubPurgeTaskId;
    TaskId mUpdateComponentInfoTaskId;

    TimerId mStartEndOfSeasonTimerId;
    FiberJobQueue mUpdateUserJobQueue;

    static const char8_t* SEASON_ROLLOVER_TASKNAME;
    static const char8_t* INACTIVE_CLUB_PURGE_TASKNAME;
    static const char8_t* FETCH_COMPONENTINFO_TASKNAME;
    static const char8_t* FETCH_COMPONENTINFO_TASKNAME_ONETIME;

    void onScheduledTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName) override;
    void onExecutedTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName) override;
    void onCanceledTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName) override;

    void addDomainMetric(ComponentStatus::InfoMap& map, const char8_t* metricName, const Metrics::TagPairList& tagList, uint64_t value) const;
};

} // Clubs
} // Blaze

#endif // BLAZE_CLUBSCOMPONENT_SLAVEIMPL_H

