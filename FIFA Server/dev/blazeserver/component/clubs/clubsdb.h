/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_CLUBSDB_H
#define BLAZE_CLUBSDB_H

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#include "eathread/eathread_storage.h"
#include "framework/callback.h"
#include "blazerpcerrors.h"
#include "framework/database/dbconn.h"
#include "framework/tdf/userdefines.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/database/preparedstatement.h"
#include "clubs/rpc/clubs_defines.h"
#include "clubs/tdf/clubs.h"
#include "clubs/tdf/clubs_server.h"
#include "clubs/clubsconstants.h"

namespace Blaze
{

// forward declarations
class PreparedStatement;
class DbRow;
class Query;

namespace Clubs
{

typedef struct ClubRecordbookRecord
{
    ClubId clubId;
    ClubRecordId recordId;
    BlazeId blazeId;
    RecordStatType statType;
    int64_t statValueInt;
    float_t statValueFloat;
    eastl::string persona;
    uint32_t lastUpdate;
    eastl::string personaNamespace;
} ClubRecordbookRecord;

typedef eastl::map<ClubRecordId, ClubRecordbookRecord> ClubRecordbookRecordMap;

typedef enum ClubMetadataUpdateType
{
    CLUBS_METADATA_UPDATE, 
    CLUBS_METADATA_UPDATE_2 
}ClubMetadataUpdateType;

class ClubsDatabase
{
public:

    // API data types
    struct TickerMessage
    {
        ClubId mClubId;
        eastl::string mText;
        eastl::string mParams;
        eastl::string mMetadata;
        uint32_t mTimestamp;        
    };
    
    typedef eastl::list<TickerMessage> TickerMessageList;


    // contructor & destructor
    ClubsDatabase();
    virtual ~ClubsDatabase();
    
    // initialization
    static void initialize(uint32_t dbID);
    
    // public API
    BlazeRpcError insertClub(ClubDomainId domainId, const char8_t *name, const ClubSettings &clubSettings, ClubId &clubId);
    BlazeRpcError updateClubSettings(ClubId clubId, const ClubSettings &clubSettings, uint64_t& version);
    BlazeRpcError getClub(ClubId clubId, Club *club, uint64_t& version);
    BlazeRpcError getClubMembers(ClubId clubId, uint32_t maxResultCount, uint32_t offset, ClubMemberList &clubMemberList, MemberOrder orderType = MEMBER_NO_ORDER, OrderMode orderMode = ASC_ORDER, MemberTypeFilter memberType = ALL_MEMBERS, const char8_t* personNamePattern = nullptr, uint32_t *totalCount = nullptr);
    BlazeRpcError insertMember(ClubId clubId, const ClubMember &clubMember, const int32_t maxMemberCount);
	BlazeRpcError getClubNameResetCount(ClubId clubId, int32_t& resetCount);
    BlazeRpcError removeMember(ClubId clubId, BlazeId blazeId, MembershipStatus clubMembershipStatus);
    BlazeRpcError logUserLastLeaveClubTime(BlazeId blazeId, ClubDomainId domainId);
    BlazeRpcError getUserLastLeaveClubTime(BlazeId blazeId, ClubDomainId domainId, int64_t& lastLeaveTime);
    BlazeRpcError getClubs(const ClubIdList &clubIdList, ClubList &clubList, uint64_t*& versions,
        const uint32_t maxResultCount = UINT32_MAX, const uint32_t offset = 0, uint32_t* totalCount = nullptr,
        const ClubsOrder clubsOrder = CLUBS_NO_ORDER, const OrderMode orderMode = ASC_ORDER);
    BlazeRpcError getClubIdsByName(const char8_t* clubName, const char8_t* clubNonUniqueName, ClubIdList& clubIds, uint32_t& rowsFound);
    BlazeRpcError getClubMembershipsInDomain(ClubDomainId domainId, BlazeId blazeId, ClubMembershipList &clubMembershipList);
    BlazeRpcError removeClub(ClubId);
    BlazeRpcError promoteClubGM(BlazeId blazeId, ClubId clubId, const int32_t maxGmCount);
    BlazeRpcError demoteClubMember(BlazeId gmId, ClubId clubId);
    BlazeRpcError transferClubOwnership(BlazeId oldOwnerId, BlazeId newOwnerId, ClubId clubId, MembershipStatus newOwnerStatusBefore, MembershipStatus oldOwnerStatusNow, const int32_t maxGmCount);
    BlazeRpcError getClubOwner(BlazeId& owner, ClubId clubId);
    BlazeRpcError insertNews(ClubId clubId, BlazeId contentCreator, const ClubNews &news );
    BlazeRpcError countNews(ClubId clubId, uint16_t &newsCount );
    BlazeRpcError countNewsByType(ClubId clubId, uint16_t &newsCount, const NewsTypeList &typeFilters);
    BlazeRpcError countNewsByTypeForMultipleClubs(const ClubIdList& clubIdList, uint16_t &newsCount, const NewsTypeList &typeFilters);
    BlazeRpcError deleteOldestNews(ClubId clubId);
    BlazeRpcError getNews(
        ClubId clubId, 
        const NewsTypeList &typeFilters, 
        TimeSortType sortType,
        uint32_t maxResultCount,
        uint32_t offset,
        eastl::list<ClubServerNewsPtr> &clubNewsList,
        bool& getAssociateNews);
    BlazeRpcError getNews(
        const ClubIdList& clubIdList, 
        const NewsTypeList &typeFilters, 
        TimeSortType sortType,
        uint32_t maxResultCount,
        uint32_t offset,
        eastl::list<ClubServerNewsPtr> &clubNewsList,
        bool& getAssociateNews);
    BlazeRpcError insertClubMessage(ClubMessage &message, ClubDomainId domainId, uint32_t maxNumOfMessages);
    BlazeRpcError updateMetaData(
        ClubId clubId, 
        ClubMetadataUpdateType metadateUpdateType, 
        //MetaDataType metaDataType, 
        const MetadataUnion& metaData,
        bool updateLastActive = false);
    BlazeRpcError findClubs(
        const bool anyDomain,
        const ClubDomainId domainId,
        const char8_t* name,
        const char8_t* language,
        const bool anyTeamId,
        const TeamId teamId,
        const char8_t* nonUniqueName,
        const ClubAcceptanceFlags& acceptanceMask,
        const ClubAcceptanceFlags& acceptanceFlags,
        const SeasonLevel seasonLevel,
        const ClubRegion region,
        const uint32_t minMemberCount,
        const uint32_t maxMemberCount,
        const ClubIdList &clubIdFilterList,
        const BlazeIdList &blazeIdFilterList,
        const uint16_t customSettingsMask,
        const CustClubSettings* customSettings,
        const uint32_t lastGameTime,
        const uint32_t maxResultCount,
        const uint32_t offset,
        const bool skipMetadata,
        const TagSearchOperation tagSearchOperation,
        const ClubTagList &tagList,
        const PasswordOption passwordOpt,
        const SearchRequestorAcceptance& petitionAcceptance,
        const SearchRequestorAcceptance& joinAcceptance,
        ClubList &clubList,
        uint32_t* totalCount = nullptr,
        const ClubsOrder clubsOrder = CLUBS_ORDER_BY_NAME,
        const OrderMode orderMode = ASC_ORDER);
    BlazeRpcError getClubMessagesByUserReceived(BlazeId blazeId, MessageType messageType, 
        TimeSortType sortType, ClubMessageList &messageList);
    BlazeRpcError getClubMessagesByUserSentDoDBWork(BlazeId blazeId, MessageType messageType, 
        TimeSortType sortType, DbResultPtr &dbresult);
    BlazeRpcError getClubMessagesByUserSent(BlazeId blazeId, MessageType messageType, 
        TimeSortType sortType, ClubMessageList &messageList);
    
    BlazeRpcError getClubMessagesByClubDoDBWork(const ClubIdList& clubIdList, MessageType messageType, 
        TimeSortType sortType, DbResultPtr &dbresult);
    BlazeRpcError getClubMessagesByClub(ClubId clubId, MessageType messageType, 
        TimeSortType sortType, ClubMessageList &messageList);
    BlazeRpcError getClubMessagesByClubs(const ClubIdList& clubIdList, MessageType messageType, 
        TimeSortType sortType, ClubMessageList &messageList);

    BlazeRpcError getClubMessage(uint32_t messageTid, ClubMessage &message);
    BlazeRpcError deleteClubMessage(uint32_t messageTid);
    BlazeRpcError updateClubLastActive(ClubId clubId);
    BlazeRpcError getClubIdentities(const ClubIdList *clubId, ClubList *clubList);
    BlazeRpcError getClubIdentityByName(const char8_t *name, Club *club);
    BlazeRpcError lockClub(ClubId clubId, uint64_t& version, Club *club = nullptr);
    BlazeRpcError lockClubs(ClubIdList *clubIdList, ClubList *clubList = nullptr);
    BlazeRpcError lockUserMessages(BlazeId blazeId);
    BlazeRpcError lockMemberDomain(BlazeId blazeId, ClubDomainId domainId);
    BlazeRpcError getClubMembershipForUsers(const BlazeIdList &blazeIdList, ClubMembershipMap &clubMembershipMap);
	BlazeRpcError getClubMetadataForUsers(const BlazeIdList &blazeIdList, ClubMetadataMap &clubMetadataMap);
    BlazeRpcError getClubMetadata(const ClubId &clubId, ClubMetadata &clubMetadata);
    BlazeRpcError getMembershipInClub(const ClubId clubId, const BlazeId blazeId, ClubMembership &clubMembership);
    BlazeRpcError getMembershipInClub(const ClubId clubId, const BlazeIdList& blazeIds, MembershipStatusMap &clubMembershipStatusMap);
    BlazeRpcError updateLastGame(ClubId clubId, ClubId opponentId, const char8_t* gameResult);
    BlazeRpcError getClubRecords(ClubId clubId, ClubRecordbookRecordMap &clubRecordMap);
    BlazeRpcError getClubRecord(ClubRecordbookRecord &clubRecord);
    BlazeRpcError updateClubRecord(ClubRecordbookRecord &clubRecord);
    BlazeRpcError resetClubRecords(ClubId clubId, const ClubRecordIdList &recordIdList);
    BlazeRpcError getClubAwards(ClubId clubId, ClubAwardList &awardList);
    BlazeRpcError updateClubAward(ClubId clubId, ClubAwardId awardId);
    BlazeRpcError updateClubAwardCount(ClubId clubId);
    BlazeRpcError updateMemberMetaData(ClubId clubId, const ClubMember &member);
    BlazeRpcError updateSeasonState(SeasonState state, int64_t seasonStartTime);
    BlazeRpcError lockSeason();
    BlazeRpcError getSeasonState(SeasonState &state, int64_t &lastUpdate);
    BlazeRpcError listClubRivals(const ClubId clubId, ClubRivalList *clubRivalList);
    BlazeRpcError insertRival(const ClubId clubId, const ClubRival &clubRival);
    BlazeRpcError updateRival(const ClubId clubId, const ClubRival &clubRival);
    BlazeRpcError deleteRivals(const ClubId clubId, const ClubIdList &rivalIds);
    BlazeRpcError deleteOldestRival(const ClubId clubId);
    BlazeRpcError incrementRivalCount(const ClubId clubId); 
    BlazeRpcError deleteOldestTickerMsg(const uint32_t timestamp);
    BlazeRpcError insertTickerMsg(
        const ClubId clubId, 
        const BlazeId includeBlazeId, 
        const BlazeId excludeBlazeId, 
        const char8_t *text,
        const char8_t *params,
        const char8_t *metadata);
    BlazeRpcError getTickerMsgs(
        const ClubId clubId, 
        const BlazeId blazeId,
        const uint32_t timestamp,
        TickerMessageList &resultList);
    BlazeRpcError getTickerMsgs(
        const ClubIdList& requestorClubIdList, 
        const BlazeId blazeId,
        const uint32_t timestamp,
        TickerMessageList &resultList);

    BlazeRpcError updateNewsItemFlags(
        NewsId newsId,
        ClubNewsFlags mask, 
        ClubNewsFlags value);
    
    BlazeRpcError getServerNews(
        const NewsId newsId,
        ClubId &clubId,
        ClubServerNews &serverNews);

    BlazeRpcError changeClubStrings(
        const ClubId clubId, 
        const char8_t* name,
        const char8_t* nonUniqueName,
        const char8_t* description,
        uint64_t& version);

	BlazeRpcError changeIsNameProfaneFlag(
		const ClubId clubId,
		uint32_t isNameProfane);

        
    BlazeRpcError countMessages(
        const ClubId clubId, 
        const MessageType messageType, 
        uint32_t &messageCount);

    BlazeRpcError countMessages(
        const ClubIdList& clubIdList, 
        const MessageType messageType, 
        ClubMessageCountMap &messageCountMap);

    BlazeRpcError getClubBans(
        const ClubId clubId, 
        BlazeIdToBanStatusMap* blazeIdToBanStatusMap);

    BlazeRpcError getUserBans(
        const BlazeId blazeId, 
        ClubIdToBanStatusMap* clubIdToBanStatusMap);

    BlazeRpcError getBan(
        const ClubId clubId, 
        const BlazeId blazeId,
        BanStatus* banStatus);

    BlazeRpcError addBan(
        const ClubId clubId, 
        const BlazeId blazeId,
        const BanStatus banStatus);

    BlazeRpcError removeBan(
        const ClubId clubId, 
        const BlazeId blazeId);

    void setDbConn(Blaze::DbConnPtr& dbConn) { mDbConn = dbConn; }
    Blaze::DbConnPtr& getDbConn() { return mDbConn; }
    
    int32_t deleteAwards(ClubAwardIdList* awardIdList);

    BlazeRpcError requestorIsGmOfUser(
        BlazeId requestorId, 
        BlazeId blazeId, 
        ClubIdList *clubsIdsInWhichGm);

    BlazeRpcError getClubsComponentInfo(
        ClubsComponentInfo &info);
    
    BlazeRpcError findTag(const ClubTag& tagText, ClubTagId& tagId);
    BlazeRpcError insertTag(const ClubTag& tagText, ClubTagId& tagId);
    BlazeRpcError insertClubTags(const ClubId clubId, const ClubTagList& clubTagList);
    BlazeRpcError removeClubTags(const ClubId clubId);
    BlazeRpcError updateClubTags(const ClubId clubId, const ClubTagList& clubTagList);
    BlazeRpcError getTagsForClubs(ClubList& clubList);    

protected:
    
    BlazeRpcError checkClubExists(ClubId clubId);
    BlazeRpcError checkUserExists(BlazeId blazeId) const;
    BlazeRpcError checkClubAndUserExist(ClubId clubId, BlazeId blazeId);

private:

    Blaze::DbConnPtr mDbConn;
    
    static EA_THREAD_LOCAL PreparedStatementId mCmdInsertId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdSettingsUpdateId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdSelectClubsId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdRemoveClubId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdInsertMemberId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdRemoveMemberId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdLogUserLastLeaveClubTimeId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdPromoteClubGMId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdDemoteClubMemberId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdInsertNewsId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdDeleteOldestNewsId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdInsertMessageId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateLastActiveId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateMetadataId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateMetadata_2_Id;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateMetadataAndLastActiveId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateMetadata_2_AndLastActiveId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdDeleteMessageId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateLastGameId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdGetClubRecord;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateClubRecord;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateClubAwards;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateClubAward;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateClubAwardCount;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateMemberMetaData;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateSeasonState;
    static EA_THREAD_LOCAL PreparedStatementId mCmdGetSeasonState;
    static EA_THREAD_LOCAL PreparedStatementId mCmdInsertRival;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateRival;
    static EA_THREAD_LOCAL PreparedStatementId mCmdDeleteOldestRival;    
    static EA_THREAD_LOCAL PreparedStatementId mCmdIncrementRivalCount;    
    static EA_THREAD_LOCAL PreparedStatementId mCmdInsertTickerMsg;
    static EA_THREAD_LOCAL PreparedStatementId mCmdDeleteOldestTickerMsg;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateNewsItemFlags;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateMembership;
    static EA_THREAD_LOCAL PreparedStatementId mCmdGetClubMetadataId;
    

    static const uint32_t CLUBS_TAGS_RESULT_COUNT_MULTIPLIER = 100;
    static const uint32_t CLUBS_TAG_MAX_RESULT_COUNT = 10000;
    
    void clubSettingsToStmt(uint32_t &col, PreparedStatement* stmt, const ClubSettings *clubSettings) const;
    void clubDBRowToClubSettings(uint32_t &col, const DbRow* dbrow, ClubSettings &clubSettings, bool skipMetadata = false) const;
    void clubDBRowToClub(uint32_t &col, const DbRow* dbrow, Club *club) const;
    void clubDBRowToClubInfo(uint32_t &col, const DbRow* dbrow, ClubInfo &clubInfo) const;
    void clubDbRowToClubMessage(const DbRow* dbrow, ClubMessage *clubMessage) const;
    void clubDbRowToClubMembership(const DbRow* dbrow, ClubMembership *clubMembership) const;
	void clubDbRowToClubMetadata(const DbRow* dbrow, ClubMetadata *clubmetadata) const;
    void clubDbRowToClubMetadataWithouBlazeId(const DbRow* dbrow, ClubMetadata *clubmetadata) const;
    void encodeMemberMetaData(MemberMetaData &metadata, char* strMetadata) const;
    void decodeMemberMetaData(const MemberMetaData &metadata, char* strMetadata, size_t strMetadataLen) const; 
    void matchTagsFilter(QueryPtr& query, const ClubTagList &tagList, bool matchAllTags, uint32_t maxResultCount);
}; // ClubsDatabase


} // Clubs

} // Blaze

#endif //CLUBSDB

