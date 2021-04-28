/*************************************************************************************************/
    /*!
    \file   osdktournamentsslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_ROOMS_TOURNAMENTSSLAVEIMPL_H
#define BLAZE_ROOMS_TOURNAMENTSSLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "framework/usersessions/usersessionmanager.h"

#include "osdktournaments/tdf/osdktournamentstypes.h"
#include "osdktournaments/tdf/osdktournamentstypes_server.h"
#include "osdktournaments/rpc/osdktournamentsslave_stub.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"

#include "osdktournamentsdatabase.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace OSDKTournaments
{

typedef eastl::hash_map<TournamentId, TournamentData*> TournamentsMap;
typedef eastl::hash_map<UserSessionId, TournamentId> ParticipantsMap;

class OSDKTournamentsSlaveImpl : public OSDKTournamentsSlaveStub,
    public ExtendedDataProvider,
    public UserSessionSubscriber
{
public:
    OSDKTournamentsSlaveImpl();
    virtual ~OSDKTournamentsSlaveImpl();

    bool onConfigure();
	bool onResolve();
    virtual bool onValidateConfig(OsdkTournamentsConfig& config, const OsdkTournamentsConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const;
    virtual void getStatusInfo(ComponentStatus& status) const;
    uint16_t getDbSchemaVersion() const { return 2; }

    const TournamentsMap& getTournamentsMap() const { return mTournamentsMap; }
    
    // resets all members in a tournament.
    BlazeRpcError resetAllTournamentMembers(TournamentId tournamentid, bool setActive);

    // removes current member from the tournament
    BlazeRpcError leaveTournament(TournamentMemberId memberId, TournamentId tournamentId);

    TournamentMemberId getCurrentUserMemberIdForTournament(TournamentId tournamentId);
    
    // custom hooks
    virtual TournamentMemberId getCurrentUserMemberIdForMode(TournamentMode tournamentMode);
    bool getMemberName(TournamentMemberId memberId, TournamentMode tournamentMode, eastl::string &memberName);

	TournamentData *getTournamentDataById(TournamentId id) 
    {
        TournamentsMap::iterator itr = mTournamentsMap.find(id);
        return itr != mTournamentsMap.end() ? (itr->second) : NULL; 
    }

    TournamentId getTournamentIdForUser(BlazeId userId);
    BlazeRpcError updateMemberUserExtendedData(BlazeId userId, TournamentId tournamentId);
    
    BlazeRpcError addUserExtendedData(BlazeId userId, TournamentId tournamentId);
    BlazeRpcError removeUserExtendedData(BlazeId userId, TournamentId tournamentId);
    BlazeRpcError reportMatchResult(TournamentId tournId, TournamentMemberId memberOneId, TournamentMemberId memberTwoId, TeamId memberOneTeam, 
        TeamId memberTwoTeam, uint32_t memberOneScore, uint32_t memberTwoScore, const char8_t* memberOneMetaData, const char8_t* memberTwoMetaData);

private:
    static const char8_t* DEFAULT_DB_NAME;

    friend class GetMyTournamentDetailsCommand; //for health checks
    struct TournamentsSlaveMetrics
    {
        TournamentsSlaveMetrics() 
        :   mTotalMatchesReported(0),
            mTotalTreeRetrievals(0),
            mTotalMatchesSaved(0),
            mTotalJoins(0),
            mTotalLeaves(0),
            mGaugeOnlineMembers(0),
            mGaugeTotalMembers(0)
        {}

        uint32_t mTotalMatchesReported;
        uint32_t mTotalTreeRetrievals;
        uint32_t mTotalMatchesSaved;
        uint32_t mTotalJoins;
        uint32_t mTotalLeaves;
        uint32_t mGaugeOnlineMembers;
        uint32_t mGaugeTotalMembers;
    };

    GetTournamentsError::Error processGetTournaments(const GetTournamentsRequest &request, GetTournamentsResponse &response, const Message* message);
    GetAllTournamentsError::Error processGetAllTournaments(GetAllTournamentsResponse &response, const Message* message);
    GetMemberCountsError::Error processGetMemberCounts(const GetMemberCountsRequest &request, GetMemberCountsResponse &response, const Message* message);

    // helper
    void assembleTournamentList(TournamentDataList &toList, uint32_t locale, TournamentMode mode = 0);

    //ExtendedDataProvider implementation
    BlazeRpcError onLoadExtendedData(const UserInfoData &data, UserSessionExtendedData &extendedData);
    bool lookupExtendedData(const UserSession& session, const char8_t* key, UserExtendedDataValue& value) const { return false; }
    BlazeRpcError onRefreshExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData) { return ERR_OK; }
    
    //UserSessionSubscriber implementation
    void onUserSessionExistence(const UserSession& userSession);
    void onUserSessionExtinction(const UserSession& userSession);

    TournamentId getTournamentIdFromSession(UserSessionId userSessionId);
    inline void incrementOnlineMemberCount(TournamentId tournId);
    inline void decrementOnlineMemberCount(TournamentId tournId);

    virtual void onJoinTournamentNotification(const Blaze::OSDKTournaments::JoinTournamentNotification &notification, Blaze::UserSession *);
    virtual void onLeaveTournamentNotification(const Blaze::OSDKTournaments::LeaveTournamentNotification &notification, Blaze::UserSession *);
    virtual void onTournamentMemberLoginNotification(const Blaze::OSDKTournaments::TournamentMemberLoginNotification& data, Blaze::UserSession *);    

    void handleMemberLogin(UserSessionId memberId);
    void handleMemberLogout(UserSessionId memberId);

    // custom hooks
    virtual void getMemberMetaData(TournamentMemberId memberId, TournamentMode mode, char8_t* memberMetaData, size_t memberMetaDataBufferSize);

    virtual void onLoadExtendedDataCustom(const UserInfoData &data, UserSessionExtendedData &extendedData);
    virtual void onMemberWonCustom(TournamentMemberId winnerId, TournamentMode mode);
    virtual void onMemberEliminatedCustom(TournamentMemberId eliminateeId, TournamentMode mode, int32_t eliminatedLevel);
    virtual void onMemberAdvancedCustom(TournamentMemberId memberId, TournamentMode mode, int32_t newLevel);       
    
	virtual const char8_t* getDbName() const;
	
    void onMemberCustomStatus(TournamentMemberId memberId, TournamentMode mode, Blaze::OSDKSeasonalPlay::TournamentStatus status, int32_t tournamentLevel=0);
    
    void releaseMemoryForTournamentNodeMap(TournamentNodeMap& tournamentNodeMap);
    
    uint32_t mDbId;
    TournamentsSlaveMetrics mMetrics;
    TournamentsMap mTournamentsMap;
    ParticipantsMap mParticipantsMap;
}; //class OSDKTournamentsSlaveImpl

} // OSDKTournaments
} // Blaze

#endif // BLAZE_ROOMS_TOURNAMENTSSLAVEIMPL_H
