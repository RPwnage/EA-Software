/*************************************************************************************************/
/*!
    \file   psnmatchutil.h

    \attention
        (c) Electronic Arts. All Rights Reserved
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEREPORTING_PSNMATCHUTIL
#define BLAZE_GAMEREPORTING_PSNMATCHUTIL

#include "EASTL/vector.h"
#include "psnmatches/tdf/psnmatches.h"
#include "gamereporting/tdf/gamereporting_server.h"

namespace Blaze
{
namespace GameReporting
{

class GameReportingSlaveImpl;

namespace Utilities
{

class PsnMatchUtil;
typedef eastl::intrusive_ptr<PsnMatchUtil> PsnMatchUtilPtr;

class PsnMatchUtil
{
public:
    PsnMatchUtil(GameReportingSlaveImpl& component, ReportType reportType, const char8_t* reportName, const GameInfo& gameInfo);
    ~PsnMatchUtil();

    typedef eastl::map<eastl::string, float> MatchStatMap;

    BlazeRpcError addPlayerResult(BlazeId playerId, float score, int64_t coopValue, const MatchStatMap& matchStatMap);

    PSNServices::Matches::GetMatchDetailResponsePtr getMatchDetail();

    // accessors for custom report processing
    PSNServices::Matches::ReportMatchResultsRequestBody& getReportRequest()
    {
        return mReportRequest.getBody();
    }

    GameReportingSlaveImpl& getComponent()
    {
        return mComponent;
    }
    ReportType getReportType()
    {
        return mReportType;
    }
    const char8_t* getReportName()
    {
        return mReportName.c_str();
    }

    BlazeRpcError initializeReport();
    BlazeRpcError submitReport();
    BlazeRpcError cancelMatch();

    void addMatchStatFormat(const char8_t* statKey, const char8_t* statFormat);

private:
    // By verified PSN spec for Matches this should always be 0 currently:
    PsnServiceLabel getServiceLabel() const;
    BlazeRpcError callPSN(const UserIdentification* user, const CommandInfo& cmdInfo, PSNServices::PsnWebApiHeader& reqHeader, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, ExternalSessionErrorInfo* errorInfo);
    const char8_t* logPrefix() const { return (mLogPrefix.empty() ? mLogPrefix.sprintf("[PsnMatchUtil]").c_str() : mLogPrefix.c_str()); }

private:
    GameReportingSlaveImpl& mComponent;
    ReportType mReportType;
    eastl::string mReportName;

    const GameInfo& mGameInfo;

    PSNServices::Matches::GetMatchDetailResponsePtr mMatchDetail;
    PSNServices::Matches::ReportMatchResultsRequest mReportRequest;
    PSNServices::Matches::PsnCooperativeResultEnum mCooperativeResult;

    struct PlayerResult
    {
        BlazeId playerId;
        ExternalPsnAccountId externalPlayerId; /// @todo [ps5-gr] suitable for "playerId" in reportResults ???
        int32_t rank;
        float score; // should be double; however, TDF does not support it
        eastl::string teamId;

        // additional player statistics
        MatchStatMap statMap;

        PlayerResult(BlazeId _playerId, ExternalPsnAccountId _externalPlayerId, float _score, const char8_t* _teamId)
            : playerId(_playerId)
            , externalPlayerId(_externalPlayerId)
            , rank(0)
            , score(_score)
            , teamId(_teamId)
        {
        }
    };

    struct PlayerScoreDescendingCompare
    {
        bool operator() (const PlayerResult* a, const PlayerResult* b);
    };

    typedef eastl::vector<PlayerResult*> PlayerList; // using vector for sorting
    PlayerList mPlayerList;
    eastl::map<BlazeId, PlayerResult*> mPlayerMap;

    struct TeamResult
    {
        eastl::string teamId;
        int32_t rank;
        float score; // should be double; however, TDF does not support it
        PlayerList memberList;

        // additional team statistics
        MatchStatMap statMap;

        TeamResult(const eastl::string& _teamId)
            : teamId(_teamId)
            , rank(0)
            , score(0)
        {
        }
    };

    struct TeamScoreDescendingCompare
    {
        bool operator() (const TeamResult* a, const TeamResult* b);
    };

    typedef eastl::vector<TeamResult*> TeamList; // using vector for sorting
    TeamList mTeamList;
    eastl::map<eastl::string, TeamResult*> mTeamMap;

    typedef eastl::hash_map<eastl::string, eastl::string> MatchStatFormatMap;
    MatchStatFormatMap mMatchStatFormatMap;

    HttpConnectionManagerPtr mPsnConnMgrPtr;

    mutable eastl::string mLogPrefix;

    uint32_t mRefCount;

    friend void intrusive_ptr_add_ref(PsnMatchUtil* ptr);
    friend void intrusive_ptr_release(PsnMatchUtil* ptr);

    NON_COPYABLE(PsnMatchUtil);
};

inline void intrusive_ptr_add_ref(PsnMatchUtil* ptr)
{
    ptr->mRefCount++;
}

inline void intrusive_ptr_release(PsnMatchUtil* ptr)
{
    if (--ptr->mRefCount == 0)
        delete ptr;
}

} // namespace Utilities
} // namespace GameReporting
} // namespace Blaze

#endif  // BLAZE_GAMEREPORTING_PSNMATCHUTIL
