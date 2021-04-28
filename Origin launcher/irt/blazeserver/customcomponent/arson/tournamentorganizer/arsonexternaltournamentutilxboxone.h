/*! ************************************************************************************************/
/*!
    \file arsonexternaltournamentutilxboxone.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef ARSONCOMPONENT_EXTERNAL_TOURNAMENT_UTIL_XBOXONE_H
#define ARSONCOMPONENT_EXTERNAL_TOURNAMENT_UTIL_XBOXONE_H

#include "arson/tournamentorganizer/arsonexternaltournamentutil.h"
#include "arson/tournamentorganizer/tdf/arsontournamentorganizer.h"
#include "framework/protocol/restprotocolutil.h" //for getContentTypeFromEncoderType() in callTournamentHub

namespace Blaze
{

class InetAddress;
class OutboundHttpConnectionManager;
class UserIdentification;

namespace XBLServices 
{
    class MultiplayerSessionRequestHeader;
    class MultiplayerSessionErrorResponse;
    class TournamentsHubErrorResponse;
    class TournamentsHubRequestHeader;
}
namespace ArsonXBLServices 
{
    class ArenaMultiplayerSessionResponse;
    class TOArenaTeamParams;
    typedef EA::TDF::TdfStructVector<TOArenaTeamParams > TOArenaTeamParamsList;
    class ArsonArenaServersArbitrationPropertiesSystem;
    class ArsonArenaTournamentTeamInfo;
    class ArsonArenaCommonMatchInfo;
    class ArsonTournamentQueryResult;
}
namespace GameManager
{
    class TournamentIdentification; 
    class ExternalSessionUtilXboxOne;
}

namespace Arson
{
    class ArsonExternalTournamentUtilXboxOne : public ArsonExternalTournamentUtil
    {
    public:
        //Note: we're making a guess as of 12-13-19 that Arena will work similarly for Gen5 here, and just allowing passing in a 'platform' to differentiate. XboxSeriesX Arena docs/info N/A currently.
        ArsonExternalTournamentUtilXboxOne(Blaze::ClientPlatformType platform, const Blaze::Arson::ArsonTournamentOrganizerConfig& config);
        ~ArsonExternalTournamentUtilXboxOne() override;

        BlazeRpcError createTournament(const CreateTournamentParameters& params, ArsonExternalTournamentInfo* result) override;
        BlazeRpcError updateTournament(const UpdateTournamentParameters& params, ArsonExternalTournamentInfo* result) override;
        BlazeRpcError getTournament(const GetTournamentParameters& params, ArsonExternalTournamentInfo& result) override;
        BlazeRpcError removeTournament(const RemoveTournamentParameters& params) override;
        BlazeRpcError createTeam(const CreateTournamentTeamParameters& params, TournamentTeamInfo* result) override;
        BlazeRpcError joinTeam(const JoinTournamentTeamParameters& params, TournamentTeamInfo* result) override;
        BlazeRpcError leaveTeam(const LeaveTournamentTeamParameters& params) override;
        BlazeRpcError updateTeam(const UpdateTournamentTeamParameters& params, TournamentTeamInfo* result) override;
        BlazeRpcError getTeamForUser(const GetTournamentTeamParameters& params, TournamentTeamInfo& result) override;
        BlazeRpcError getTeamsForUser(const GetTournamentTeamsParameters& params, TournamentTeamInfoList& result) override;
        BlazeRpcError getTeamByName(const GetTournamentTeamParameters& params, Blaze::Arson::TournamentTeamInfo& result) override;
        BlazeRpcError setTeamMatch(const SetTournamentTeamMatchParameters& params, TournamentTeamInfo* result) override;
        BlazeRpcError submitResult(const SubmitTournamentMatchResultParameters& params) override;
        BlazeRpcError getTournaments(const GetTournamentsParameters& params, GetTournamentsResult& result) override;

        BlazeRpcError getMps(const char8_t* sessionTemplateName, const char8_t* sessionName, ArsonXBLServices::ArenaMultiplayerSessionResponse& result) const;

    private:
        static const size_t MAX_SERVICE_UNAVAILABLE_RETRIES = 2;
        static const uint64_t SERVICE_RETRY_WAIT_SECONDS = 2;

    private:
        bool isDisabled() const { return isDisabled(getExternalSessionConfig().getExternalTournamentsContractVersion()); }
        static bool isDisabled(const char8_t* contractVersion) { return ((contractVersion == nullptr) || (contractVersion[0] == '\0')); }
        static bool isRetryError(BlazeRpcError xblCallErr);
        
        BlazeRpcError getAuthInfo(const UserIdentification* caller, ExternalUserAuthInfo& authInfo, bool omitUserclaim = true);
        BlazeRpcError getTournamentInternal(const UserIdentification* caller, const char8_t* organizerId, const char8_t* tournamentId, ArsonExternalTournamentInfo& result);

        BlazeRpcError clearTeamMatch(const Blaze::Arson::SetTournamentTeamMatchParameters& params, ArsonXBLServices::ArsonArenaTournamentTeamInfo& rsp);
        BlazeRpcError setTeamMatchToBye(const Blaze::Arson::SetTournamentTeamMatchParameters& params, ArsonXBLServices::ArsonArenaTournamentTeamInfo& rsp);
        BlazeRpcError setTeamMatchToGame(const Blaze::Arson::SetTournamentTeamMatchParameters& params, ArsonXBLServices::ArsonArenaTournamentTeamInfo& rsp);
        BlazeRpcError updateTeamsLastGame(const UserIdentification& caller, const GameManager::TournamentIdentification& tournament, const ArsonXBLServices::TOArenaTeamParamsList& teamParamsList, const ArsonXBLServices::ArenaMultiplayerSessionResponse& lastGame, const ExternalSessionIdentification& lastGameIdentification);

        BlazeRpcError getTeamByNameInternal(const GetTournamentTeamParameters& params, ArsonXBLServices::ArsonArenaTournamentTeamInfo& result);

        BlazeRpcError getTournamentsInternal(const GetTournamentsParameters& params, GetTournamentsResult& resultToAppendTo, eastl::string& continuationToken);
        
        const char8_t* findTeamOutcomeInLastGameResults(const char8_t* teamName, const ArsonXBLServices::ArsonArenaServersArbitrationPropertiesSystem& lastGameData, uint64_t& ranking) const;
        
        BlazeRpcError validateJoinTeamParams(const TournamentTeamIdentification& team, const GameManager::TournamentIdentification& tournament, const UserIdentificationList& joiners);
        BlazeRpcError validateTeamIdentification(const TournamentTeamIdentification& teamIdentification) const;
        BlazeRpcError validateTournamentIdentification(const GameManager::TournamentIdentification& identification) const;
        bool validateStarted(const ArsonXBLServices::ArenaMultiplayerSessionResponse& gameMps) const;

        bool validateTournamentState(const char8_t* state) const;
        bool validateTournamentSchedule(const Arson::ArsonExternalTournamentScheduleInfo& schedule, bool required) const;
        bool validateUtcTimeString(const char8_t* timeString, const char8_t* logContext) const;
        
        void initReqTeamBodyFromGetRsp(ArsonXBLServices::ArsonArenaTournamentTeamInfo& reqTeamInfo, const ArsonXBLServices::ArsonArenaTournamentTeamInfo& getRsp) const;
        void initReqTeamMatchBodyFromGetRsp(ArsonXBLServices::ArsonArenaCommonMatchInfo& reqBody, const ArsonXBLServices::ArsonArenaCommonMatchInfo& getRsp) const;
        void initDefaultFormatString(Arson::ArsonArenaFormattedString& output) const;

        void toTournamentTeamInfo(const ArsonXBLServices::ArsonArenaTournamentTeamInfo& xblTeam, const GameManager::TournamentIdentification& tournamentIdentification, Arson::TournamentTeamInfo& result) const;
        const char8_t* toOutcome(TournamentMatchResult blazeResult) const;
        const char8_t* toRegistrationState(TeamRegistrationState blazeRegistrationState) const;
        TeamRegistrationState parseXblRegistrationState(const char8_t* xblRegistrationState) const;

        const char8_t* toArbitrationStatusLogStr(const ArsonXBLServices::ArenaMultiplayerSessionResponse& mpsRsp) const;

        BlazeRpcError getTokenAndHeaders(XBLServices::TournamentsHubRequestHeader& header, const UserIdentification* caller);

        // connections to XBL:
        void initializeConnManager(const char8_t* serverAddress, InetAddress** inetAddress, OutboundHttpConnectionManager** connManager, int32_t poolSize, const char8_t* serverDesc) const;
        void initializeConnManagerPtrs();
        BlazeRpcError callTournamentHub(const UserIdentification* caller, const CommandInfo& cmdInfo, XBLServices::TournamentsHubRequestHeader& reqHeader, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, RawBuffer* rspRaw, XBLServices::TournamentsHubErrorResponse* rspErr = nullptr, bool addEncodedPayload = true, const char8_t* contentType = RestProtocolUtil::getContentTypeFromEncoderType(Encoder::JSON));
        BlazeRpcError callTournamentHubInternal(const CommandInfo& cmdInfo, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, RawBuffer* rspRaw, XBLServices::TournamentsHubErrorResponse* rspErr, const char8_t* contentType, bool addEncodedPayload);
        BlazeRpcError callTournamentOrganizerHub(const UserIdentification* caller, const CommandInfo& cmdInfo, XBLServices::TournamentsHubRequestHeader& reqHeader, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, RawBuffer* rspRaw, XBLServices::TournamentsHubErrorResponse* rspErr = nullptr, bool addEncodedPayload = true, const char8_t* contentType = RestProtocolUtil::getContentTypeFromEncoderType(Encoder::JSON));
        BlazeRpcError callTournamentOrganizerHubInternal(const CommandInfo& cmdInfo, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, RawBuffer* rspRaw, XBLServices::TournamentsHubErrorResponse* rspErr, const char8_t* contentType, bool addEncodedPayload);
        BlazeRpcError callMpsd(const UserIdentification& caller, const CommandInfo& cmdInfo, ExternalUserAuthInfo& authInfo, XBLServices::MultiplayerSessionRequestHeader& reqHeader, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, RawBuffer* rspRaw, XBLServices::MultiplayerSessionErrorResponse* rspErr = nullptr, bool addEncodedPayload = true, const char8_t* contentType = RestProtocolUtil::getContentTypeFromEncoderType(Encoder::JSON));
        BlazeRpcError callMpsdInternal(const CommandInfo& cmdInfo, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, RawBuffer* rspRaw, XBLServices::MultiplayerSessionErrorResponse* rspErr, const char8_t* contentType, bool addEncodedPayload);
        BlazeRpcError callClientMpsd(const CommandInfo& cmdInfo, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, RawBuffer* rspRaw, XBLServices::MultiplayerSessionErrorResponse* rspErr = nullptr, bool addEncodedPayload = true, const char8_t* contentType = RestProtocolUtil::getContentTypeFromEncoderType(Encoder::JSON));
        BlazeRpcError callClientMpsdInternal(const CommandInfo& cmdInfo, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, RawBuffer* rspRaw, XBLServices::MultiplayerSessionErrorResponse* rspErr, const char8_t* contentType, bool addEncodedPayload);

    private:
        Blaze::GameManager::ExternalSessionUtilXboxOne* mExternalSessionUtil;
        Blaze::TitleId mTitleId;
        InetAddress* mTournamentHubAddr;
        InetAddress* mTournamentOrganizerHubAddr;
        InetAddress* mSessDirAddr;
        HttpConnectionManagerPtr mTournamentHubConnManagerPtr;
        HttpConnectionManagerPtr mTournamentOrganizerHubConnManagerPtr;
        HttpConnectionManagerPtr mSessDirConnManagerPtr;
        mutable eastl::string mArbitrationStatusLogBuf;
        Blaze::ClientPlatformType mPlatform;
    };
}
}

#endif
