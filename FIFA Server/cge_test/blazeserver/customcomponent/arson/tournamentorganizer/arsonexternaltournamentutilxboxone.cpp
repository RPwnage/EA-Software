/*! ************************************************************************************************/
/*!
    \file arsonexternaltournamentutilxboxone.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "arson/tournamentorganizer/arsonexternaltournamentutilxboxone.h"

#include "framework/logger.h"
#include "arson/tournamentorganizer/proxycomponent/tdf/arsonxblorganizertournamentshub.h"
#include "arson/tournamentorganizer/proxycomponent/rpc/arsonxblorganizertournamentshubslave.h"
#include "arson/tournamentorganizer/proxycomponent/tdf/arsonxblsessiondirectory_arena.h"
#include "arson/tournamentorganizer/proxycomponent/rpc/arsonxblsessiondirectoryarenaslave.h"
#include "arson/tournamentorganizer/tdf/arsontournamentorganizer.h"
#include "arson/tournamentorganizer/tdf/arsontournamentorganizer_server.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizer_defines.h"
#include "component/gamemanager/externalsessions/externalsessionscommoninfo.h" // for isMockPlatformEnabled (for mock XBL testing)
#include "framework/connection/outboundhttpservice.h" // for gOutboundHttpService in callClientMpsdInternal
#include "gamemanager/externalsessions/externalsessionutilxboxone.h"
#include "framework/usersessions/usersessionmanager.h"

using namespace Blaze::ArsonXBLServices;
using namespace Blaze::XBLServices;

namespace Blaze
{
namespace Arson
{
    ArsonExternalTournamentUtilXboxOne::ArsonExternalTournamentUtilXboxOne(Blaze::ClientPlatformType platform, const ArsonTournamentOrganizerConfig& config)
        : ArsonExternalTournamentUtil(config), 
        mExternalSessionUtil(BLAZE_NEW GameManager::ExternalSessionUtilXboxOne(platform, config.getExternalSessions(), 0, true)),
        mTournamentHubAddr(nullptr), mTournamentOrganizerHubAddr(nullptr), mSessDirAddr(nullptr), mPlatform(platform)
    {
        if (isDisabled())
            INFO_LOG("[ArsonExternalTournamentUtilXboxOne] External tournaments service explicitly disabled.");

        bool titleIdIsHex;
        if (!GameManager::ExternalSessionUtilXboxOne::verifyParams(config.getExternalSessions(), titleIdIsHex,
            UINT16_MAX, false))
        {
            WARN_LOG("[ArsonExternalTournamentUtilXboxOne] External tournaments service param verification failed.");
        }

        if (titleIdIsHex)
        {
            // convert the titleId to decimal before caching it
            int32_t titleIdDec = strtol(config.getExternalSessions().getExternalSessionTitle(), nullptr, 16);
            char8_t buf[128] = "";
            blaze_snzprintf(buf, sizeof(buf), "%" PRId32, titleIdDec);
            mTitleId.set(buf);
        }
        else
            mTitleId.set(config.getExternalSessions().getExternalSessionTitle());

        initializeConnManagerPtrs();
    }

    ArsonExternalTournamentUtilXboxOne::~ArsonExternalTournamentUtilXboxOne()
    {
        if (mExternalSessionUtil != nullptr)
            delete mExternalSessionUtil;
        if (mTournamentHubConnManagerPtr != nullptr)
            mTournamentHubConnManagerPtr.reset();
        if (mTournamentOrganizerHubConnManagerPtr != nullptr)
            mTournamentOrganizerHubConnManagerPtr.reset();
        if (mSessDirConnManagerPtr != nullptr)
            mSessDirConnManagerPtr.reset();
        if (mTournamentHubAddr != nullptr)
            delete mTournamentHubAddr;
        if (mTournamentOrganizerHubAddr != nullptr)
            delete mTournamentOrganizerHubAddr;
        if (mSessDirAddr != nullptr)
            delete mSessDirAddr;
    }

    /*! ************************************************************************************************/
    /*! \brief get XBL authentication token
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::getAuthInfo(const UserIdentification* caller, ExternalUserAuthInfo& authInfo, bool omitUserclaim /*= true*/)
    {
        UserIdentification currUser;
        if ((caller == nullptr) || (caller->getPlatformInfo().getExternalIds().getXblAccountId() == INVALID_XBL_ACCOUNT_ID))
        {
            if (gCurrentLocalUserSession == nullptr)
            {
                ERR_LOG("[ArsonExternalTournamentUtilXboxOne].getAuthInfo: internal error: Cannot retrieve auth info, current local user session nullptr.");
                return ERR_AUTHORIZATION_REQUIRED;
            }
            UserInfo::filloutUserIdentification(gCurrentLocalUserSession->getUserInfo(), currUser);
            caller = &currUser;
        }
        ASSERT_COND_LOG((caller != nullptr && caller->getPlatformInfo().getExternalIds().getXblAccountId() == INVALID_XBL_ACCOUNT_ID), "[ArsonExternalTournamentUtilXboxOne].getAuthInfo: Invalid empty externalid/xuid for member, cannot get auth token for user.");

        eastl::string buf;
        authInfo.setServiceName(gCurrentUserSession->getServiceName());
        BlazeRpcError tokErr = mExternalSessionUtil->getAuthToken(*caller, authInfo.getServiceName(), buf);
        if (tokErr != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].getAuthInfo: could not get auth token for user(" << *caller << ". Error(" << ErrorHelp::getErrorName(tokErr) << ")");
            return tokErr;
        }

        // tournament hub calls replace the user claim part, i.e. 'XBL3.0 x=..;..' -> 'XBL3.0 x=-;..'
        if (omitUserclaim && !mExternalSessionUtil->stripUserClaim(buf))
        {
            return ERR_SYSTEM;//logged
        }

        authInfo.setCachedExternalSessionToken(buf.c_str());
        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief create tournament
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::createTournament(const CreateTournamentParameters& params,
        ArsonExternalTournamentInfo* result)
    {
        const char8_t* organizerId = params.getTournamentIdentification().getTournamentOrganizer();
        const char8_t* tournamentId = params.getTournamentIdentification().getTournamentId();
        
        if (isDisabled())
        {
            TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].createTournament: External user profile service disabled.");
            return ERR_OK;
        }
        BlazeRpcError tidErr = validateTournamentIdentification(params.getTournamentIdentification());
        if (tidErr != ERR_OK)
        {
            return tidErr;
        }
        // validate the required params for X1 spec
        if (!validateTournamentState(params.getArsonExternalTournamentInfo().getState()))
        {
            return ARSON_TO_ERR_INVALID_TOURNAMENT_STATE;
        }
        if (!validateTournamentSchedule(params.getArsonExternalTournamentInfo().getSchedule(), true))
        {
            return ARSON_TO_ERR_INVALID_TOURNAMENT_TIME;
        }
        if ((params.getArsonExternalTournamentInfo().getVisibility()[0] == '\0') ||
            (params.getArsonExternalTournamentInfo().getTitleId()[0] == '\0') ||
            (params.getArsonExternalTournamentInfo().getDefaultLanguage()[0] == '\0') ||
            (params.getArsonExternalTournamentInfo().getPlatforms().empty()) ||
            (params.getArsonExternalTournamentInfo().getRegions().empty()) ||
            (params.getArsonExternalTournamentInfo().getDisplayStrings().empty()) ||
            (params.getArsonExternalTournamentInfo().getImageMonikers().getBadgeArtMoniker()[0] == '\0') ||
            (params.getArsonExternalTournamentInfo().getImageMonikers().getHeroArtMoniker()[0] == '\0') || 
            (params.getArsonExternalTournamentInfo().getImageMonikers().getTileArtMoniker()[0] == '\0'))
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].createTournament: missing visibility, titleId, defaultLanguage, non-empty platforms, non-empty regions, sufficient number of display strings, or image/title/badge monikers");
            return ARSON_TO_ERR_INVALID_TOURNAMENT_PARAMETERS;
        }

        XBLServices::TournamentsHubErrorResponse errRsp;
        Arson::ArsonExternalTournamentInfo rsp;
        ArsonXBLServices::CreateArenaTournamentRequest req;

        req.setOrganizerId(organizerId);
        req.setTournamentId(tournamentId);
        params.getArsonExternalTournamentInfo().copyInto(req.getBody());
        // no need to populate req header here, its populated below in call helper
        TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].createTournament: creating tournament(" << tournamentId << ":organizer=" << organizerId << ")");

        const CommandInfo& cmd = ArsonXBLOrganizerTournamentsHubSlave::CMD_INFO_CREATEORUPDATEARENATOURNAMENT;
        BlazeRpcError err = callTournamentOrganizerHub(&params.getUser(), cmd, req.getHeader(), req, &rsp, nullptr, &errRsp);
        if (err != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].createTournament: Failed to create tournament(" << tournamentId <<
                ":organizer=" << organizerId << "), error(" << ErrorHelp::getErrorName(err) <<
                "). Error response: " << errRsp << ", Request: " << req);
        }
        else if (result != nullptr)
        {
            rsp.copyInto(*result);
        }
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief update tournament
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::updateTournament(const UpdateTournamentParameters& params,
        ArsonExternalTournamentInfo* result)
    {
        const char8_t* organizerId = params.getTournamentIdentification().getTournamentOrganizer();
        const char8_t* tournamentId = params.getTournamentIdentification().getTournamentId();
        
        if (isDisabled())
        {
            TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].updateTournament: External user profile service disabled.");
            return ERR_OK;
        }
        if (!params.getArsonExternalTournamentInfo().isSet())
        {
            WARN_LOG("[ArsonExternalTournamentUtilXboxOne].updateTournament: not updating tournament(" << tournamentId << ":organizer=" << organizerId << "), no updates were specified in request.");
            return ERR_OK;
        }

        BlazeRpcError tidErr = validateTournamentIdentification(params.getTournamentIdentification());
        if (tidErr != ERR_OK)
        {
            return tidErr;
        }
        if (!validateTournamentState(params.getArsonExternalTournamentInfo().getState()))
        {
            return ARSON_TO_ERR_INVALID_TOURNAMENT_STATE;
        }
        if (!validateTournamentSchedule(params.getArsonExternalTournamentInfo().getSchedule(), false))
        {
            return ARSON_TO_ERR_INVALID_TOURNAMENT_TIME;
        }
        
        // fetch the tournament to update
        Arson::ArsonExternalTournamentInfo getRsp;
        BlazeRpcError getErr = getTournamentInternal(&params.getUser(), organizerId, tournamentId, getRsp);
        if (getErr != ERR_OK)
        {
            return getErr;
        }

        XBLServices::TournamentsHubErrorResponse errRsp;
        Arson::ArsonExternalTournamentInfo rsp;
        ArsonXBLServices::UpdateArenaTournamentRequest req;

        req.setOrganizerId(params.getTournamentIdentification().getTournamentOrganizer());
        req.setTournamentId(tournamentId);
        // no need to populate req header here, its populated below in call helper

        // update body as appropriate
        getRsp.copyInto(req.getBody());

        if (params.getArsonExternalTournamentInfo().getState()[0] != '\0')
            req.getBody().setState(params.getArsonExternalTournamentInfo().getState());
        // set schedule as appropriate
        if (params.getArsonExternalTournamentInfo().getSchedule().getCheckinStart()[0] != '\0')
            req.getBody().getSchedule().setCheckinStart(params.getArsonExternalTournamentInfo().getSchedule().getCheckinStart());
        if (params.getArsonExternalTournamentInfo().getSchedule().getCheckinEnd()[0] != '\0')
            req.getBody().getSchedule().setCheckinEnd(params.getArsonExternalTournamentInfo().getSchedule().getCheckinEnd());
        if (params.getArsonExternalTournamentInfo().getSchedule().getPlayingStart()[0] != '\0')
            req.getBody().getSchedule().setPlayingStart(params.getArsonExternalTournamentInfo().getSchedule().getPlayingStart());
        if (params.getArsonExternalTournamentInfo().getSchedule().getPlayingEnd()[0] != '\0')
            req.getBody().getSchedule().setPlayingEnd(params.getArsonExternalTournamentInfo().getSchedule().getPlayingEnd());
        if (params.getArsonExternalTournamentInfo().getSchedule().getRegistrationStart()[0] != '\0')
            req.getBody().getSchedule().setRegistrationStart(params.getArsonExternalTournamentInfo().getSchedule().getRegistrationStart());
        if (params.getArsonExternalTournamentInfo().getSchedule().getRegistrationEnd()[0] != '\0')
            req.getBody().getSchedule().setRegistrationEnd(params.getArsonExternalTournamentInfo().getSchedule().getRegistrationEnd());
            WARN_LOG("[ArsonExternalTournamentUtilXboxOne].updateTournament: not updating tournament(" << tournamentId << ":organizer=" << organizerId << "), no updates were specified in request.");

        TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].updateTournament: updating tournament(" << tournamentId << ":organizer=" << organizerId << ")");

        const CommandInfo& cmd = ArsonXBLOrganizerTournamentsHubSlave::CMD_INFO_CREATEORUPDATEARENATOURNAMENT;
        BlazeRpcError err = callTournamentOrganizerHub(&params.getUser(), cmd, req.getHeader(), req, &rsp, nullptr, &errRsp);
        if (err != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].updateTournament: Failed to update tournament(" << tournamentId <<
                ":organizer=" << organizerId << "), error(" << ErrorHelp::getErrorName(err) <<
                "). Error response: " << errRsp << ", Request: " << req);
        }
        else if (result != nullptr)
        {
            rsp.copyInto(*result);
        }
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief get tournament
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::getTournament(const GetTournamentParameters& params,
        ArsonExternalTournamentInfo& result)
    {
        const char8_t* organizerId = params.getTournamentIdentification().getTournamentOrganizer();
        const char8_t* tournamentId = params.getTournamentIdentification().getTournamentId();
        return getTournamentInternal(&params.getUser(), organizerId, tournamentId, result);
    }

    BlazeRpcError ArsonExternalTournamentUtilXboxOne::getTournamentInternal(const UserIdentification* caller,
        const char8_t* organizerId, const char8_t* tournamentId, ArsonExternalTournamentInfo& result)
    {   
        TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].getTournament: getting (" << tournamentId << ":" << organizerId << ")");
        
        XBLServices::TournamentsHubErrorResponse errRsp;
        ArsonXBLServices::GetArenaTournamentRequest req;
        req.setOrganizerId(organizerId);
        req.setTournamentId(tournamentId);
        // no need to populate req header here, its populated below in call helper
        
        const CommandInfo& cmd = ArsonXBLOrganizerTournamentsHubSlave::CMD_INFO_GETARENATOURNAMENT;
        BlazeRpcError err = callTournamentOrganizerHub(caller, cmd, req.getHeader(), req, &result, nullptr, &errRsp);
        if (err != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].getTournament: Failed to getting (" << tournamentId << ":organizer=" <<
                organizerId << "), error(" << ErrorHelp::getErrorName(err) << "). Error response: " << errRsp << ".");
        }
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief cancel the tournament
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::removeTournament(const RemoveTournamentParameters& params)
    {
        const char8_t* organizerId = params.getTournamentIdentification().getTournamentOrganizer();
        const char8_t* tournamentId = params.getTournamentIdentification().getTournamentId();

        if (isDisabled())
        {
            TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].removeTournament: External user profile service disabled.");
            return ERR_OK;
        }
        BlazeRpcError tidErr = validateTournamentIdentification(params.getTournamentIdentification());
        if (tidErr != ERR_OK)
        {
            return tidErr;
        }

        // fetch the tournament to update
        Arson::ArsonExternalTournamentInfo getRsp;
        BlazeRpcError getErr = getTournamentInternal(&params.getUser(), organizerId, tournamentId, getRsp);
        if (getErr != ERR_OK)
        {
            return getErr;
        }

        // if tournament already completed/canceled ignore
        if (blaze_stricmp("canceled", getRsp.getState()) == 0)
        {
            TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].removeTournament: NOOP: tournament(" <<
                params.getTournamentIdentification().getTournamentId() << ") already in state(" << getRsp.getState() << ").");
            return ERR_OK;
        }

        XBLServices::TournamentsHubErrorResponse errRsp;
        Arson::ArsonExternalTournamentInfo rsp;
        ArsonXBLServices::UpdateArenaTournamentRequest req;

        req.setOrganizerId(params.getTournamentIdentification().getTournamentOrganizer());
        req.setTournamentId(tournamentId);
        // no need to populate req header here, its populated below in call helper

        // update body as appropriate
        getRsp.copyInto(req.getBody());
        req.getBody().setState("Canceled");
        if ((params.getPostRemoveNameSuffix()[0] != '\0') && !req.getBody().getDisplayStrings().empty())
        {
            // add optional name suffix for debugging
            auto& displayStr = req.getBody().getDisplayStrings().begin()->second;
            displayStr->setName(eastl::string().sprintf("%s%s", displayStr->getName(), params.getPostRemoveNameSuffix()).c_str());
        }
        TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].removeTournament: setting tournament(" << tournamentId << ":organizer=" << organizerId << "), from state(" << getRsp.getState() << "), to state(" << req.getBody().getState() << ")");

        const CommandInfo& cmd = ArsonXBLOrganizerTournamentsHubSlave::CMD_INFO_CREATEORUPDATEARENATOURNAMENT;
        BlazeRpcError err = callTournamentOrganizerHub(&params.getUser(), cmd, req.getHeader(), req, &rsp, nullptr, &errRsp);
        if (err != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].removeTournament: Failed to update tournament(" << tournamentId <<
                ":organizer=" << organizerId << "), error(" << ErrorHelp::getErrorName(err) << "). Error response: " << errRsp << ", Request: " << req);
        }
        return err;
    }

    /*!************************************************************************************************/
    /*! \brief creates the tournament team multi player session. Joins the creator
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::createTeam(const CreateTournamentTeamParameters& params,
        TournamentTeamInfo* result)
    {
        if (isDisabled() || params.getMembers().empty())
        {
            return ERR_OK;//no op
        }
        const UserIdentification& creator = *params.getMembers().front();
        const char8_t* teamSessionId = params.getTeamIdentification().getUniqueName();
        const char8_t* tournamentId = params.getTournamentIdentification().getTournamentId();
        const char8_t* organizerId = params.getTournamentIdentification().getTournamentOrganizer();

        // MS spec requires the following params to create a team, as of TournamentHubs V3 API:
        if (teamSessionId[0] == '\0' ||
            params.getTeamIdentification().getDisplayName()[0] == '\0' ||
            params.getStanding().getFormatMoniker()[0] == '\0' ||
            params.getStanding().getParams().empty() ||
            params.getMembers().empty() ||
            !validateUtcTimeString(params.getRegistrationDate(), "createTeam registrationDate"))
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].createTeam: Invalid/missing request params, can't create. MS requires:" <<
                " tournamentId, organizer, and team's: id, (display)name, registrationDate, standing.formaMoniker/.params, " <<
                " members. Attempted request: " << params);

            return ARSON_TO_ERR_INVALID_TOURNAMENT_TEAM;
        }

        BlazeRpcError err = validateJoinTeamParams(params.getTeamIdentification(), params.getTournamentIdentification(),
            params.getMembers());
        if (err != ERR_OK)
        {
            return err;//logged
        }

        // create team
        ArsonXBLServices::CreateArenaTournamentTeamRequest req;
        req.setTournamentId(tournamentId);
        req.setOrganizerId(organizerId);
        req.setTeamId(teamSessionId);
        // no need to populate req header here, its populated below in call helper

        req.getBody().setId(teamSessionId);
        req.getBody().setName(params.getTeamIdentification().getDisplayName());
        req.getBody().setRegistrationDate(params.getRegistrationDate());
        req.getBody().setState(toRegistrationState(params.getRegistrationState()));

        params.getStanding().copyInto(req.getBody().getStanding());

        for (auto& member : params.getMembers())
        {
            ArenaMemberDetails* reqMember = req.getBody().getMembers().pull_back();
            reqMember->setId(eastl::string(eastl::string::CtorSprintf(), "%" PRIu64 "", member->getPlatformInfo().getExternalIds().getXblAccountId()).c_str());
        }
        
        XBLServices::TournamentsHubErrorResponse errRsp;
        ArsonXBLServices::ArsonArenaTournamentTeamInfo rsp;

        // mock testing currently doesn't support mocking Tournament Hub APIs
        if (isMock())
        {
            req.getBody().copyInto(rsp);
        }
        else
        {
            const CommandInfo& cmd = ArsonXBLServices::ArsonXBLOrganizerTournamentsHubSlave::CMD_INFO_CREATEARENATOURNAMENTTEAM;
            err = callTournamentOrganizerHub(&creator, cmd, req.getHeader(), req, &rsp, nullptr, &errRsp);
        }

        if (err != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].createTeam: Failed to create team(" << teamSessionId <<
                "), for tournament(" << tournamentId << ":organizer=" << organizerId << "), error(" <<
                ErrorHelp::getErrorName(err) << "). Error response: " << errRsp << ".   Request: " << req);
        }
        else if (result != nullptr)
        {
            toTournamentTeamInfo(rsp, params.getTournamentIdentification(), *result);
        }
        return err;
    }

    /*!************************************************************************************************/
    /*! \brief joins user to the tournament team multi player session.
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::joinTeam(const JoinTournamentTeamParameters& params, TournamentTeamInfo* result)
    {
        if (isDisabled())
        {
            return ERR_OK;//no op
        }
        if (params.getUser().getPlatformInfo().getExternalIds().getXblAccountId() == INVALID_XBL_ACCOUNT_ID)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].joinTeam: Invalid input xuid, can't join user(" << params.getUser() << ").");
            return ARSON_TO_ERR_INVALID_USER;
        }
        
        const char8_t* teamSessionName = params.getTeamIdentification().getUniqueName();
        const char8_t* tournamentId = params.getTournamentIdentification().getTournamentId();
        const char8_t* organizerId = params.getTournamentIdentification().getTournamentOrganizer();


        // validate tournament and team identification, and joiner isn't already in a team for the tournament
        UserIdentificationList tmpUsers;
        params.getUser().copyInto(*tmpUsers.pull_back());
        BlazeRpcError err = validateJoinTeamParams(params.getTeamIdentification(), params.getTournamentIdentification(), tmpUsers);
        if (err != ERR_OK)
        {
            return err;
        }

        // validate team session exists and can be joined
        GetTournamentTeamParameters getParams;
        ArsonXBLServices::ArsonArenaTournamentTeamInfo getRsp;

        params.getUser().copyInto(getParams.getUser());
        params.getTournamentIdentification().copyInto(getParams.getTournamentIdentification());
        params.getTeamIdentification().copyInto(getParams.getTournamentTeamIdentification());
        err = getTeamByNameInternal(getParams, getRsp);
        if (err != ERR_OK)
        {
            return err;
        }

        char8_t timeBuf[64];
        TimeValue::getTimeOfDay().toAccountString(timeBuf, sizeof(timeBuf), true);

        ArsonXBLServices::CreateArenaTournamentTeamRequest req;
        req.setTournamentId(tournamentId);
        req.setOrganizerId(organizerId);
        req.setTeamId(teamSessionName);
        // no need to populate req header here, its populated below in call helper

        // copy old team info to new request
        initReqTeamBodyFromGetRsp(req.getBody(), getRsp);
        //add the new member to body
        req.getBody().getMembers().pull_back()->setId(eastl::string(eastl::string::CtorSprintf(), "%" PRIu64 "", params.getUser().getPlatformInfo().getExternalIds().getXblAccountId()).c_str());

        XBLServices::TournamentsHubErrorResponse errRsp;
        ArsonXBLServices::ArsonArenaTournamentTeamInfo rsp;

        // mock testing currently doesn't support mocking Tournament Hub APIs
        if (isMock())
        {
            req.getBody().copyInto(rsp);
        }
        else
        {
            const CommandInfo& cmd = ArsonXBLServices::ArsonXBLOrganizerTournamentsHubSlave::CMD_INFO_CREATEARENATOURNAMENTTEAM;
            err = callTournamentOrganizerHub(&params.getUser(), cmd, req.getHeader(), req, &rsp, nullptr, &errRsp);
        }

        if (err != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].joinTeam: Failed to create/join external session. titleId(" << mTitleId.c_str() << "), (" << teamSessionName << ", for tournament(" << params.getTournamentIdentification().getTournamentId() << ":organizer=" << params.getTournamentIdentification().getTournamentOrganizer() << "), with user(" << params.getUser() << ").  error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << "). Error response: " << errRsp << ", Request: " << req);
        }
        else if (result != nullptr)
        {
            toTournamentTeamInfo(rsp, params.getTournamentIdentification(), *result);
        }
        return err;
    }

    /*!************************************************************************************************/
    /*! \brief updates the tournament team multi player session.
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::leaveTeam(const LeaveTournamentTeamParameters& params)
    {
        if (isDisabled())
        {
            return ERR_OK;
        }

        const char8_t* teamSessionName = params.getTeamIdentification().getUniqueName();
        const char8_t* tournamentId = params.getTournamentIdentification().getTournamentId();
        const char8_t* organizerId = params.getTournamentIdentification().getTournamentOrganizer();

        GetTournamentTeamParameters getParams;
        ArsonXBLServices::ArsonArenaTournamentTeamInfo getRsp;

        params.getUser().copyInto(getParams.getUser());
        params.getTournamentIdentification().copyInto(getParams.getTournamentIdentification());
        params.getTeamIdentification().copyInto(getParams.getTournamentTeamIdentification());
        BlazeRpcError getErr = getTeamByNameInternal(getParams, getRsp);
        if (getErr != ERR_OK)
        {
            return getErr;
        }

        BlazeRpcError err = ERR_OK;

        ArsonXBLServices::CreateArenaTournamentTeamRequest req;
        req.setTournamentId(tournamentId);
        req.setOrganizerId(organizerId);
        req.setTeamId(teamSessionName);
        // no need to populate req header here, its populated below in call helper

        // copy old team info to new request
        initReqTeamBodyFromGetRsp(req.getBody(), getRsp);
        // then remove the new member from req body:
        eastl::string leavingXuidStr(eastl::string::CtorSprintf(), "%" PRIu64 "", params.getUser().getPlatformInfo().getExternalIds().getXblAccountId());
        for (XBLServices::ArenaMemberDetailsList::iterator itr = req.getBody().getMembers().begin(),
            end = req.getBody().getMembers().end(); itr != end; ++itr)
        {
            if (blaze_stricmp(leavingXuidStr.c_str(), (*itr)->getId()))
            {
                req.getBody().getMembers().erase(itr);
                break;
            }
        }

        XBLServices::TournamentsHubErrorResponse errRsp;
        ArsonXBLServices::ArsonArenaTournamentTeamInfo rsp;

        // mock testing currently doesn't support mocking Tournament Hub APIs
        if (isMock())
        {
            req.getBody().copyInto(rsp);
        }
        else
        {
            const CommandInfo& cmd = ArsonXBLServices::ArsonXBLOrganizerTournamentsHubSlave::CMD_INFO_CREATEARENATOURNAMENTTEAM;
            err = callTournamentOrganizerHub(&params.getUser(), cmd, req.getHeader(), req, &rsp, nullptr, &errRsp);
        }

        if (err != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].leaveTeam: Failed. titleId(" << mTitleId.c_str() << "), (" <<
                teamSessionName << ", for tournament(" << params.getTournamentIdentification().getTournamentId() <<
                ":organizer=" << params.getTournamentIdentification().getTournamentOrganizer() << "), with user(" <<
                params.getUser() << ").  error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << "). Error response: " << errRsp << ", Request: " << req);
        }
        
        return err;
    }

    /*!************************************************************************************************/
    /*! \brief updates the tournament team multi player session's registration state
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::updateTeam(const UpdateTournamentTeamParameters& params,
        TournamentTeamInfo* result)
    {
        if (isDisabled())
        {
            return ERR_OK;
        }

        const char8_t* teamSessionName = params.getTeamIdentification().getUniqueName();
        const char8_t* tournamentId = params.getTournamentIdentification().getTournamentId();
        const char8_t* organizerId = params.getTournamentIdentification().getTournamentOrganizer();

        GetTournamentTeamParameters getParams;
        ArsonXBLServices::ArsonArenaTournamentTeamInfo getRsp;

        params.getCaller().copyInto(getParams.getUser());
        params.getTournamentIdentification().copyInto(getParams.getTournamentIdentification());
        params.getTeamIdentification().copyInto(getParams.getTournamentTeamIdentification());
        BlazeRpcError getErr = getTeamByNameInternal(getParams, getRsp);
        if (getErr != ERR_OK)
        {
            return getErr;
        }

        BlazeRpcError err = ERR_OK;
        XBLServices::TournamentsHubErrorResponse errRsp;
        ArsonXBLServices::ArsonArenaTournamentTeamInfo rsp;

        ArsonXBLServices::CreateArenaTournamentTeamRequest req;
        req.setTournamentId(tournamentId);
        req.setOrganizerId(organizerId);
        req.setTeamId(teamSessionName);

        // copy old team info to new request
        initReqTeamBodyFromGetRsp(req.getBody(), getRsp);

        // then update the new request body below
        req.getBody().setState(toRegistrationState(params.getRegistrationState()));

        if (params.getTeamIdentification().getDisplayName()[0] != '\0')
        {
            req.getBody().setName(params.getTeamIdentification().getDisplayName());
        }
        if (params.getRegistrationStateReason()[0] != '\0')
        {
            req.getBody().setCompletedReason(params.getRegistrationStateReason());
        }
        if (params.getFinalResult().getRank() != INVALID_TOURNAMENT_RANK)
        {
            req.getBody().setRanking(params.getFinalResult().getRank());
        }
        if (params.getStanding().getFormatMoniker()[0] != '\0')
        {
            req.getBody().getStanding().setFormatMoniker(params.getStanding().getFormatMoniker());
        }
        if (!params.getStanding().getParams().empty())
        {
            params.getStanding().getParams().copyInto(req.getBody().getStanding().getParams());
        }
        // no need to populate req header here, its populated below in call helper

        // mock testing currently doesn't support mocking Tournament Hub APIs
        if (isMock())
        {
            req.getBody().copyInto(rsp);
        }
        else
        {
            const CommandInfo& cmd = ArsonXBLServices::ArsonXBLOrganizerTournamentsHubSlave::CMD_INFO_CREATEARENATOURNAMENTTEAM;
            err = callTournamentOrganizerHub(&params.getCaller(), cmd, req.getHeader(), req, &rsp, nullptr, &errRsp);
        }

        if (err != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].updateTeam: Failed " << teamSessionName << ", for tournament(" <<
                tournamentId << ":organizer=" << organizerId << "). error: " << ErrorHelp::getErrorName(err) << 
                " (" << ErrorHelp::getErrorDescription(err) << "). Error response: " << errRsp << ", Request: " << req);
        }
        else if (result != nullptr)
        {
            toTournamentTeamInfo(rsp, params.getTournamentIdentification(), *result);
        }
        return err;
    }

    /*!************************************************************************************************/
    /*! \brief get the user's team in the tournament. Returns ARSON_TO_ERR_INVALID_TOURNAMENT_TEAM if none found
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::getTeamForUser(const GetTournamentTeamParameters& params,
        TournamentTeamInfo& result)
    {
        if (isDisabled())
        {
            return ERR_OK;
        }
        BlazeRpcError err = validateTournamentIdentification(params.getTournamentIdentification());
        if (err != ERR_OK)
        {
            return err;
        }

        XBLServices::TournamentsHubErrorResponse errRsp;
        ArsonXBLServices::ArsonGetArenaTeamsForUserResponse rsp;
        XBLServices::GetArenaTeamForUserRequest req;
        // no need to populate req header here, its populated below in call helper

        req.setOrganizerId(params.getTournamentIdentification().getTournamentOrganizer());
        req.setTournamentId(params.getTournamentIdentification().getTournamentId());
        req.setMemberId(eastl::string(eastl::string::CtorSprintf(), "%" PRIu64 "", params.getUser().getPlatformInfo().getExternalIds().getXblAccountId()).c_str());

        // mock testing currently doesn't support mocking Tournament Hub APIs. Add what fields we can, other fields won't be testable
        if (isMock())
        {
            XBLServices::ArenaTeamParamsList teamParams;
            mExternalSessionUtil->addUserTeamParamIfMockPlatformEnabled(teamParams, params.getTournamentIdentification(),
                params.getUser());

            auto& teamRsp = *rsp.getValue().pull_back();
            teamRsp.setId((*teamParams.begin())->getUniqueName());
            teamRsp.setName((*teamParams.begin())->getDisplayName());
            teamRsp.getMembers().pull_back()->setId(req.getMemberId());
        }
        else
        {
            // note: this method is only available via the *client* tournament hub by MS spec, currently
            const CommandInfo& cmd = ArsonXBLServices::ArsonXBLOrganizerTournamentsHubSlave::CMD_INFO_GETARENATOURNAMENTTEAMSFORUSER;

            err = callTournamentHub(&params.getUser(), cmd, req.getHeader(), req, &rsp, nullptr, &errRsp);
        }

        if (err != ERR_OK && err != ARSON_XBLTOURNAMENTSHUB_RESOURCE_NOT_FOUND)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].getTeamForUser: Failed, error: " << ErrorHelp::getErrorName(err));
        }
        else
        {
            TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].getTeamForUser: found(" << (rsp.getValue().empty() ? "<NO TEAM>" :
                (*rsp.getValue().begin())->getId()) << "), for user(" << params.getUser().getPlatformInfo().getExternalIds().getXblAccountId() << "), tournament(" <<
                req.getTournamentId() << ":" << req.getOrganizerId() << ")");

            if (!rsp.getValue().empty())
            {
                const auto& teamRsp = **rsp.getValue().begin();

                params.getTournamentIdentification().copyInto(result.getTournamentIdentification());
                params.getTournamentTeamIdentification().copyInto(result.getTeamIdentification());

                result.setBye(teamRsp.getCurrentMatch().getBye());
                result.setRegistrationState(parseXblRegistrationState(teamRsp.getState()));
                result.setRegistrationStateReason(teamRsp.getCompletedReason());
                result.getFinalResult().setRank(teamRsp.getRanking());
                result.getStanding().setFormatMoniker(teamRsp.getStanding());
                result.getMatchIdentification().getXone().setSessionName(teamRsp.getCurrentMatch().getGameSessionRef().getName());
                result.getMatchIdentification().getXone().setTemplateName(teamRsp.getCurrentMatch().getGameSessionRef().getTemplateName());

                for (auto& member : teamRsp.getMembers())
                {
                    ExternalXblAccountId xblId = EA::StdC::AtoI64(member->getId());
                    convertToPlatformInfo(result.getMembers().pull_back()->getPlatformInfo(), xblId, nullptr, INVALID_ACCOUNT_ID, xone);
                }
            }
            else
            {
                err = ARSON_TO_ERR_INVALID_TOURNAMENT_TEAM;
            }
        }

        // convert to the platform-indep error code
        return (err == ARSON_XBLTOURNAMENTSHUB_RESOURCE_NOT_FOUND ? ARSON_TO_ERR_INVALID_TOURNAMENT_TEAM : err);
    }

    /*!************************************************************************************************/
    /*! \brief gets the user's teams for specified tournaments, if there are any.
        Returns ARSON_TO_ERR_INVALID_TOURNAMENT_TEAM if none found
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::getTeamsForUser(const GetTournamentTeamsParameters& params,
        TournamentTeamInfoList& results)
    {
        if (isDisabled())
        {
            return ERR_OK;
        }

        // MS's new tournament hub s2s api requires tournament identification
        if (params.getTournaments().empty())
        {
            WARN_LOG("[ArsonExternalTournamentUtilXboxOne].getTeamsForUser: possible test issue, no tournaments specified");
            return ERR_OK;
        }

        ExternalUserAuthInfo authInfo;
        BlazeRpcError err = getAuthInfo(&params.getUser(), authInfo);
        if (err != ERR_OK)
        {
            return err;
        }

        for (const auto& tournament : params.getTournaments())
        {
            // MS's new tournament hub s2s api requires tournament identification
            err = validateTournamentIdentification(*tournament);
            if (err != ERR_OK)
            {
                return err;
            }

            TournamentTeamInfo result;
            GetTournamentTeamParameters getTeamParams;
            params.getUser().copyInto(getTeamParams.getUser());
            tournament->copyInto(getTeamParams.getTournamentIdentification());

            err = getTeamForUser(getTeamParams, result);
            
            if (err != ERR_OK && err != ARSON_XBLTOURNAMENTSHUB_RESOURCE_NOT_FOUND)
            {
                WARN_LOG("[ArsonExternalTournamentUtilXboxOne].getTeamsForUser: error fetching team: " << ErrorHelp::getErrorName(err));
                return err;
            }
            else
            {
                result.copyInto(*results.pull_back());
            }

        }

        return (err == ARSON_XBLTOURNAMENTSHUB_RESOURCE_NOT_FOUND ? ARSON_TO_ERR_INVALID_TOURNAMENT_TEAM : err);
    }


    /*!************************************************************************************************/
    /*! \brief get the team in the tournament by its unique name. Returns ARSON_TO_ERR_INVALID_TOURNAMENT_TEAM if none found
    ***************************************************************************************************/
    Blaze::BlazeRpcError ArsonExternalTournamentUtilXboxOne::getTeamByName(const GetTournamentTeamParameters& params,
        Blaze::Arson::TournamentTeamInfo& result)
    {
        ArsonXBLServices::ArsonArenaTournamentTeamInfo rsp;
        BlazeRpcError err = getTeamByNameInternal(params, rsp);

        params.getTournamentIdentification().copyInto(result.getTournamentIdentification());
        params.getTournamentTeamIdentification().copyInto(result.getTeamIdentification());

        result.setBye(rsp.getCurrentMatch().getBye());
        result.setRegistrationState(parseXblRegistrationState(rsp.getState()));
        result.setRegistrationStateReason(rsp.getCompletedReason());
        result.getFinalResult().setRank(rsp.getRanking());
        rsp.getStanding().copyInto(result.getStanding());
        result.getMatchIdentification().getXone().setSessionName(rsp.getCurrentMatch().getGameSessionRef().getName());
        result.getMatchIdentification().getXone().setTemplateName(rsp.getCurrentMatch().getGameSessionRef().getTemplateName());
        //rsp.getCurrentMatch().getLabel().copyInto(result.getLabel());

        for (auto& member : rsp.getMembers())
        {
            UserIdentification* resultMember = result.getMembers().pull_back();
            ExternalXblAccountId xblId = EA::StdC::AtoI64(member->getId());
            convertToPlatformInfo(resultMember->getPlatformInfo(), xblId, nullptr, INVALID_ACCOUNT_ID, xone);
        }

        return (err == ARSON_XBLTOURNAMENTSHUB_RESOURCE_NOT_FOUND ? ARSON_TO_ERR_INVALID_TOURNAMENT_TEAM : err);
    }

    Blaze::BlazeRpcError ArsonExternalTournamentUtilXboxOne::getTeamByNameInternal(const GetTournamentTeamParameters& params,
        ArsonXBLServices::ArsonArenaTournamentTeamInfo& result)
    {
        const auto tournamentId = params.getTournamentIdentification().getTournamentId();
        const auto organizerId = params.getTournamentIdentification().getTournamentOrganizer();

        TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].getTeamByNameInternal: retrieving team(" <<
            params.getTournamentTeamIdentification().getUniqueName() << "), tournament(" << tournamentId <<
            ":" << organizerId << "), for user(" << params.getUser() << ")");

        BlazeRpcError tidErr = validateTournamentIdentification(params.getTournamentIdentification());
        if (tidErr != ERR_OK)
        {
            return tidErr;
        }

        BlazeRpcError err = ERR_OK;

        XBLServices::TournamentsHubErrorResponse errRsp;

        ArsonXBLServices::ArsonGetArenaTeamRequest req;
        // no need to populate req header here, its populated below in call helper

        req.setOrganizerId(organizerId);
        req.setTournamentId(tournamentId);
        req.setTeamId(params.getTournamentTeamIdentification().getUniqueName());

        // mock testing currently doesn't support mocking Tournament Hub APIs. Add what fields we can, other fields won't be testable
        if (isMock())
        {
            XBLServices::ArenaTeamParamsList teamParams;
            mExternalSessionUtil->addUserTeamParamIfMockPlatformEnabled(teamParams, params.getTournamentIdentification(), params.getUser());
            result.setId((*teamParams.begin())->getUniqueName());
            result.setName((*teamParams.begin())->getDisplayName());
            result.getMembers().pull_back()->setId(eastl::string(eastl::string::CtorSprintf(), "%" PRIu64 "", params.getUser().getPlatformInfo().getExternalIds().getXblAccountId()).c_str());
        }
        else
        {
            const CommandInfo& cmd = ArsonXBLServices::ArsonXBLOrganizerTournamentsHubSlave::CMD_INFO_GETARENATOURNAMENTTEAM;
            err = callTournamentOrganizerHub(&params.getUser(), cmd, req.getHeader(), req, &result, nullptr, &errRsp);
        }

        if (err != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].getTeamByNameInternal: Failed to get team for user(" << params.getUser() <<
                "), tournament(" << tournamentId << ":organizer=" << organizerId << "), error: " <<
                ErrorHelp::getErrorName(err) << "). Error response: " << errRsp << ", Request: " << req);
        }

        return err;
    }


    /*! ************************************************************************************************/
    /*! \brief set the next tournament match for the team
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::setTeamMatch(const Blaze::Arson::SetTournamentTeamMatchParameters& params,
        TournamentTeamInfo* result)
    {
        // mock testing currently doesn't support mocking Tournament Hub APIs
        if (isDisabled() || isMock())
        {
            return ERR_OK;//no op
        }

        BlazeRpcError err = ERR_OK;
        err = validateTeamIdentification(params.getTeamIdentification());
        if (err != ERR_OK)
            return err;
        err = validateTournamentIdentification(params.getTournamentIdentification());
        if (err != ERR_OK)
            return err;

        const bool isMatchSpecified = GameManager::isExtSessIdentSet(params.getMatchIdentification());
        ASSERT_COND_LOG(!(isMatchSpecified && params.getBye()), "[ArsonExternalTournamentUtilXboxOne].setTeamMatch: specified a match MPS(template=" << params.getMatchIdentification().getXone().getTemplateName() << ", sessionName=" << params.getMatchIdentification().getXone().getSessionName() << "), with bye 'true'. For Arena Teams, bye should not be 'true' if match is specified. Assuming 'bye' while ignoring setting match.");

        // if session isn't specified or a bye, we're clearing the rendezvous info from the team. Else adding game ref
        ArsonXBLServices::ArsonArenaTournamentTeamInfo rsp;
        if (!isMatchSpecified || params.getBye())
        {
            err = clearTeamMatch(params, rsp);
            if (err != ERR_OK)
                return err;

            if (params.getBye())
            {
                err = setTeamMatchToBye(params, rsp);
            }
        }
        else
        {
            err = setTeamMatchToGame(params, rsp);
        }

        if ((err == ERR_OK) && (result != nullptr))
        {
            toTournamentTeamInfo(rsp, params.getTournamentIdentification(), *result);
        }
        return err;
    }

    BlazeRpcError ArsonExternalTournamentUtilXboxOne::clearTeamMatch(const Blaze::Arson::SetTournamentTeamMatchParameters& params,
        ArsonXBLServices::ArsonArenaTournamentTeamInfo& rsp)
    {
        // mock testing currently doesn't support mocking Tournament Hub APIs
        if (isDisabled() || isMock())
        {
            return ERR_OK;//no op
        }

        const char8_t* teamSessionName = params.getTeamIdentification().getUniqueName();
        const char8_t* tournamentId = params.getTournamentIdentification().getTournamentId();
        const char8_t* organizerId = params.getTournamentIdentification().getTournamentOrganizer();

        GetTournamentTeamParameters getParams;
        ArsonXBLServices::ArsonArenaTournamentTeamInfo getRsp;
        params.getCaller().copyInto(getParams.getUser());
        params.getTournamentIdentification().copyInto(getParams.getTournamentIdentification());
        params.getTeamIdentification().copyInto(getParams.getTournamentTeamIdentification());
        BlazeRpcError err = getTeamByNameInternal(getParams, getRsp);
        if (err != ERR_OK)
        {
            return err;
        }

        ArsonXBLServices::CreateArenaTournamentTeamRequest req;
        req.setTournamentId(tournamentId);
        req.setOrganizerId(organizerId);
        req.setTeamId(teamSessionName);
        // no need to populate req header here, its populated below in call helper

        // 1st copy old team info to new request
        initReqTeamBodyFromGetRsp(req.getBody(), getRsp);
        
        // 2nd update the new request body
        req.getBody().getCurrentMatch().clearIsSetRecursive();

        XBLServices::TournamentsHubErrorResponse errRsp;

        const CommandInfo& cmd = ArsonXBLServices::ArsonXBLOrganizerTournamentsHubSlave::CMD_INFO_CREATEARENATOURNAMENTTEAM;
        err = callTournamentOrganizerHub(&params.getCaller(), cmd, req.getHeader(), req, &rsp, nullptr, &errRsp);
        if (err != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].clearTeamMatch: Failed " << teamSessionName << ", for tournament(" <<
                tournamentId << ":organizer=" << organizerId << "). error: " << ErrorHelp::getErrorName(err) <<
                " (" << ErrorHelp::getErrorDescription(err) << "). Error response: " << errRsp << ", Request: " << req);
        }
        return err;
    }

    BlazeRpcError ArsonExternalTournamentUtilXboxOne::setTeamMatchToBye(const Blaze::Arson::SetTournamentTeamMatchParameters& params,
        ArsonXBLServices::ArsonArenaTournamentTeamInfo& rsp)
    {
        const char8_t* teamSessionName = params.getTeamIdentification().getUniqueName();
        const char8_t* tournamentId = params.getTournamentIdentification().getTournamentId();
        const char8_t* organizerId = params.getTournamentIdentification().getTournamentOrganizer();

        GetTournamentTeamParameters getParams;
        ArsonXBLServices::ArsonArenaTournamentTeamInfo getRsp;
        params.getCaller().copyInto(getParams.getUser());
        params.getTournamentIdentification().copyInto(getParams.getTournamentIdentification());
        params.getTeamIdentification().copyInto(getParams.getTournamentTeamIdentification());
        BlazeRpcError err = getTeamByNameInternal(getParams, getRsp);
        if (err != ERR_OK)
        {
            return err;
        }

        ArsonXBLServices::CreateArenaTournamentTeamRequest req;
        req.setTournamentId(tournamentId);
        req.setOrganizerId(organizerId);
        req.setTeamId(teamSessionName);
        // no need to populate req header here, its populated below in call helper

        // 1st copy old team info to new request
        initReqTeamBodyFromGetRsp(req.getBody(), getRsp);
        
        // 2nd update the new request body
        req.getBody().getCurrentMatch().clearIsSetRecursive();
        req.getBody().getCurrentMatch().setBye(params.getBye());

        TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].setTeamMatchToBye: Request: " << req);

        XBLServices::TournamentsHubErrorResponse errRsp;

        const CommandInfo& cmd = ArsonXBLServices::ArsonXBLOrganizerTournamentsHubSlave::CMD_INFO_CREATEARENATOURNAMENTTEAM;
        err = callTournamentOrganizerHub(&params.getCaller(), cmd, req.getHeader(), req, &rsp, nullptr, &errRsp);
        if (err != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].setTeamMatchToBye: Failed " << teamSessionName << ", for tournament(" <<
                tournamentId << ":organizer=" << organizerId << "). error: " << ErrorHelp::getErrorName(err) <<
                " (" << ErrorHelp::getErrorDescription(err) << "). Error response: " << errRsp << ", Request: " << req);
        }
        return err;
    }

    BlazeRpcError ArsonExternalTournamentUtilXboxOne::setTeamMatchToGame(const Blaze::Arson::SetTournamentTeamMatchParameters& params,
        ArsonXBLServices::ArsonArenaTournamentTeamInfo& rsp)
    {
        const char8_t* teamSessionName = params.getTeamIdentification().getUniqueName();
        const char8_t* tournamentId = params.getTournamentIdentification().getTournamentId();
        const char8_t* organizerId = params.getTournamentIdentification().getTournamentOrganizer();
        TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].setTeamMatchToGame: set team(" << teamSessionName << "), for tournament(" <<
            tournamentId << ":organizer=" << organizerId << ")");

        GetTournamentTeamParameters getParams;
        ArsonXBLServices::ArsonArenaTournamentTeamInfo getRsp;
        params.getCaller().copyInto(getParams.getUser());
        params.getTournamentIdentification().copyInto(getParams.getTournamentIdentification());
        params.getTeamIdentification().copyInto(getParams.getTournamentTeamIdentification());

        BlazeRpcError getTargetTeamErr = getTeamByNameInternal(getParams, getRsp);
        if (getTargetTeamErr != ERR_OK)
        {
            return getTargetTeamErr;
        }

        ArsonXBLServices::ArenaMultiplayerSessionResponse gameMps;
        BlazeRpcError getMatchMpsErr = getMps(params.getMatchIdentification().getXone().getTemplateName(),
            params.getMatchIdentification().getXone().getSessionName(), gameMps);
        if (getMatchMpsErr != ERR_OK)
        {
            return getMatchMpsErr;//logged
        }


        BlazeRpcError err = ERR_OK;

        XBLServices::TournamentsHubErrorResponse errRsp;
        ArsonXBLServices::CreateArenaTournamentTeamRequest req;
        req.setTournamentId(tournamentId);
        req.setOrganizerId(organizerId);
        req.setTeamId(teamSessionName);
        // no need to populate req header here, its populated below in call helper

        // 1st copy old team info to new request
        initReqTeamBodyFromGetRsp(req.getBody(), getRsp);

        // 2nd update the new request body
        req.getBody().getCurrentMatch().clearIsSetRecursive();
        req.getBody().getCurrentMatch().getGameSessionRef().setName(params.getMatchIdentification().getXone().getSessionName());
        req.getBody().getCurrentMatch().getGameSessionRef().setTemplateName(params.getMatchIdentification().getXone().getTemplateName());
        req.getBody().getCurrentMatch().getGameSessionRef().setScid(getExternalSessionConfig().getScid());
        req.getBody().getCurrentMatch().setStartTime(gameMps.getServers().getArbitration().getConstants().getSystem().getStartTime());

        // label for current match must be specified. If missing set to a dummy default
        // TODO: should probably validate MS doesn't change API by checking the required fields are returned from getTeam call
        if (params.getLabel().getFormatMoniker()[0] == '\0')
        {
            initDefaultFormatString(req.getBody().getCurrentMatch().getLabel());
        }
        else
        {
            params.getLabel().copyInto(req.getBody().getCurrentMatch().getLabel());
        }

        for (auto& member : gameMps.getMembers())
        {
            auto* nextTeamId = member.second->getConstants().getSystem().getTeam();
            if (nextTeamId[0] != '\0')
            {
                ArsonArenaCommonMatchInfo::StringOpposingTeamIdsList::const_iterator found = eastl::find(
                    req.getBody().getCurrentMatch().getOpposingTeamIds().begin(),
                    req.getBody().getCurrentMatch().getOpposingTeamIds().end(), nextTeamId);
                if (found != req.getBody().getCurrentMatch().getOpposingTeamIds().end())
                {
                    continue;
                }

                req.getBody().getCurrentMatch().getOpposingTeamIds().push_back(member.second->getConstants().getSystem().getTeam());
            }
        }

        // MS requires a standing currently. If missing set to a dummy default:
        if (req.getBody().getStanding().getFormatMoniker()[0] == '\0')
        {
            initDefaultFormatString(req.getBody().getStanding());
        }

        // call XBL
        const CommandInfo& cmd = ArsonXBLServices::ArsonXBLOrganizerTournamentsHubSlave::CMD_INFO_CREATEARENATOURNAMENTTEAM;

        err = callTournamentOrganizerHub(&params.getCaller(), cmd, req.getHeader(), req, &rsp, nullptr, &errRsp);
        if (err != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].setTeamMatchToGame: Failed " << teamSessionName << ", for tournament(" <<
                tournamentId << ":organizer=" << organizerId << "). Error(" << ErrorHelp::getErrorName(err) <<
                " (" << ErrorHelp::getErrorDescription(err) << "). Error response: " << errRsp << ".  Request: " << req);
        }
        return err;
    }

    /*!************************************************************************************************/
    /*! \brief updates the external session's Arena game result. If this completes of arbitration, updates
        the associated team sessions
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::submitResult(const SubmitTournamentMatchResultParameters& params)
    {
        const XboxOneExternalSessionIdentification& matchIdentification = params.getMatchIdentification().getXone();
        if (isDisabled())
        {
            TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].submitResult: no-op call, util disabled.");
            return ERR_OK;
        }

        // get game MPS
        ArsonXBLServices::ArenaMultiplayerSessionResponse gameMps;
        BlazeRpcError geterr = getMps(matchIdentification.getTemplateName(), matchIdentification.getSessionName(), gameMps);
        if (geterr != ERR_OK)
        {
            return (geterr == ARSON_XBLSESSIONDIRECTORYARENA_NO_CONTENT ? ARSON_TO_ERR_EXTERNAL_SESSION_NOT_FOUND : geterr);//logged
        }

        if (!validateStarted(gameMps))
        {
            return ARSON_TO_ERR_INVALID_TOURNAMENT_TIME;
        }

        ArsonXBLServices::TOArenaTeamParamsList teamParamsList;
        
        ExternalUserAuthInfo authInfo;
        BlazeRpcError err = getAuthInfo(&params.getReporter(), authInfo, false);
        if (err != ERR_OK)
        {
            return err;
        }
        
        // setup request to update arbitration on the game MPS
        ArsonXBLServices::PutUpdateArenaGameRequest req;
        req.getHeader().setContractVersion(getExternalSessionConfig().getContractVersion());
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        req.getHeader().setOnBehalfOfTitle(mTitleId.c_str());
        req.setScid(getExternalSessionConfig().getScid());
        req.setSessionTemplateName(params.getMatchIdentification().getXone().getTemplateName());
        req.setSessionName(params.getMatchIdentification().getXone().getSessionName());

        // validate reports before adding member's result to request:
        req.getBody().getMembers()["me"] = req.getBody().getMembers().allocate_element();
        Arson::ArsonArenaResults& reqresults = req.getBody().getMembers()["me"]->getProperties().getSystem().getArbitration().getResults();
        for (auto& itr : params.getTeamResults())
        {
            // get Team to validate exists
            GetTournamentTeamParameters getTeamParams;
            params.getReporter().copyInto(getTeamParams.getUser());
            params.getTournamentIdentification().copyInto(getTeamParams.getTournamentIdentification());
            (itr)->getTeam().copyInto(getTeamParams.getTournamentTeamIdentification());

            ArsonXBLServices::ArsonArenaTournamentTeamInfo getTeamResult;
            BlazeRpcError getTeamErr = getTeamByNameInternal(getTeamParams, getTeamResult);
            if (getTeamErr != ERR_OK)
            {
                return ARSON_TO_ERR_INVALID_TOURNAMENT_TEAM;
            }
            // cache to list, in case we need to update final team results after arbitration, below.
            ArsonXBLServices::TOArenaTeamParams* teamParams = teamParamsList.pull_back();
            teamParams->setDisplayName(getTeamResult.getName());
            teamParams->setUniqueName(itr->getTeam().getUniqueName());
            
            if ((itr->getResult() == TOURNAMENT_MATCH_RESULT_RANK) && (itr->getRanking() <= 0))
            {
                ERR_LOG("[ArsonExternalTournamentUtilXboxOne].submitResult: invalid ranking, must be > 0. Failed for team(" <<
                    "), reporter(" << params.getReporter().getPlatformInfo().getExternalIds().getXblAccountId() << "), outcome(" << 
                    eastl::string().sprintf("%s:rank%" PRIu64 "", TournamentMatchResultToString(itr->getResult()), itr->getRanking()).c_str() << ").");

                return ARSON_TO_ERR_EXTERNAL_SESSION_INVALID_OUTCOME_RANKING;
            }

            // add result to req
            reqresults[teamParams->getUniqueName()] = reqresults.allocate_element();
            reqresults[teamParams->getUniqueName()]->setOutcome(toOutcome((itr)->getResult()));
            if ((itr)->getResult() == TOURNAMENT_MATCH_RESULT_RANK)
            {
                reqresults[teamParams->getUniqueName()]->setRanking((itr)->getRanking());
            }
        }

        // call XBL
        TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].submitResult: posting result for game(" <<
            GameManager::toLogStr(params.getMatchIdentification()) << "), reporter(" << params.getReporter() <<
            "). Game aribration status(" << toArbitrationStatusLogStr(gameMps) << ").");

        XBLServices::MultiplayerSessionErrorResponse errRsp;
        ArsonXBLServices::ArenaMultiplayerSessionResponse rsp;
        
        const CommandInfo& cmd = ArsonXBLServices::ArsonXblSessionDirectoryArenaSlave::CMD_INFO_PUTUPDATEARENAGAMEFORUSER;
        err = callMpsd(params.getReporter(), cmd, authInfo, req.getHeader(), req, &rsp, nullptr, &errRsp);
        if (err != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].submitResult: Failed to post result for game(" <<
                GameManager::toLogStr(params.getMatchIdentification()) << ", id=" <<
                gameMps.getConstants().getCustom().getExternalSessionId() << "), for reporter(" << params.getReporter() <<
                "). Error(" << ErrorHelp::getErrorName(err) << "). Error: " << errRsp << ", Request: " << req);

            return err;
        }

        // If the MPS has arbitration status completed, update the teams in the tournaments hub
        if (blaze_stricmp(rsp.getArbitration().getStatus(), "complete") == 0)
        {
            updateTeamsLastGame(params.getReporter(), params.getTournamentIdentification(), teamParamsList, rsp, params.getMatchIdentification());
        }
        return err;
    }

    /*!************************************************************************************************/
    /*! \brief call to check game started, before posting arena result. Used to generate granular error code (MS failure doesn't).
    ***************************************************************************************************/
    bool ArsonExternalTournamentUtilXboxOne::validateStarted(const ArsonXBLServices::ArenaMultiplayerSessionResponse& gameMps) const
    {
        const char8_t* startTime = gameMps.getServers().getArbitration().getConstants().getSystem().getStartTime();
        TimeValue st = ((startTime[0] == '\0') ? 0 : TimeValue::parseAccountTime(startTime));
        if (st == 0)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].validateStarted: failed to parse UTC start time(" << startTime <<
                ") for game(" << gameMps.getConstants().getCustom().getExternalSessionId() << ").");

            return false;
        }
        char8_t timeStr[64];
        memset(timeStr, 0, sizeof(timeStr));
        TimeValue currTime = TimeValue::parseAccountTime(TimeValue::getTimeOfDay().toAccountString(timeStr, sizeof(timeStr), true));
        if (currTime <= st)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].validateStarted: attempting to post Arena result for game(" <<
                gameMps.getConstants().getCustom().getExternalSessionId() << ") too early, before its UTC start time(" <<
                startTime << "). Current UTC time(" << currTime.toAccountString(timeStr, sizeof(timeStr), true) << ").");

            return false;
        }

        TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].validateStarted: validated game(" <<
            gameMps.getConstants().getCustom().getExternalSessionId() << ") started at time(" << startTime << ":value=" <<
            st.getMicroSeconds() << ",s=" << st.getSec() << ", ms = " << st.getMillis() << "). Current UTC time(" <<
            currTime.toAccountString(timeStr, sizeof(timeStr), true) << ":value=" << currTime.getMicroSeconds() << ",s=" <<
            currTime.getSec() << ",ms=" << currTime.getMillis() << ").");

        return true;
    }


    /*!************************************************************************************************/
    /*! \brief updates the PreviousMatch section of the teams that are in the specified teams list. Call after match completes.
        \param[in] caller (optional) If has valid ExternalId specified, will use this as the caller. Otherwise uses current user session. WAL calls for instance, may need to specify this.
        \param[in] teamParamsList List of the teams to update

    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::updateTeamsLastGame(const UserIdentification& caller,
        const GameManager::TournamentIdentification& tournament,
        const ArsonXBLServices::TOArenaTeamParamsList& teamParamsList,
        const ArsonXBLServices::ArenaMultiplayerSessionResponse& lastGame,
        const ExternalSessionIdentification& lastGameIdentification)
    {
        // mock testing currently doesn't support mocking Tournament Hub APIs
        if (isMock())
        {
            return ERR_OK;
        }

        BlazeRpcError err = ERR_OK;

        const char8_t* tournamentId = tournament.getTournamentId();
        const char8_t* organizerId = tournament.getTournamentOrganizer();

        for (auto& teamParams : teamParamsList)
        {
            const char8_t* teamSessionName = teamParams->getUniqueName();

            GetTournamentTeamParameters getParams;
            ArsonXBLServices::ArsonArenaTournamentTeamInfo getRsp;
            caller.copyInto(getParams.getUser());
            tournament.copyInto(getParams.getTournamentIdentification());
            getParams.getTournamentTeamIdentification().setUniqueName(teamSessionName);
            getParams.getTournamentTeamIdentification().setDisplayName(teamParams->getDisplayName());

            err = getTeamByNameInternal(getParams, getRsp);
            if (err != ERR_OK)
            {
                return err;
            }

            
            ArsonXBLServices::CreateArenaTournamentTeamRequest req;
            req.setTournamentId(tournamentId);
            req.setOrganizerId(organizerId);
            req.setTeamId(teamSessionName);
            // no need to populate req header here, its populated below in call helper
            
            // 1st copy old team info to new request
            initReqTeamBodyFromGetRsp(req.getBody(), getRsp);

            // 2nd update the new request body:

            req.getBody().getPreviousMatch().clearIsSetRecursive();

            // a descriptive label for PreviousMatch must be specified (displayed in UX)
            initDefaultFormatString(req.getBody().getPreviousMatch().getLabel());

            // get the outcome for the team to add to the req
            uint64_t rankingOutcome = UNSPECIFIED_RANKING;
            const auto* outcome = findTeamOutcomeInLastGameResults(teamParams->getUniqueName(),
                lastGame.getServers().getArbitration().getProperties().getSystem(), rankingOutcome);
            if (outcome == nullptr)
            {
                return ARSON_TO_ERR_EXTERNAL_SESSION_OUT_OF_SYNC;//logged
            }

            char8_t timeStr[64];
            memset(timeStr, 0, sizeof(timeStr));
            req.getBody().getPreviousMatch().setStartTime(lastGame.getServers().getArbitration().getConstants().getSystem().getStartTime());
            req.getBody().getPreviousMatch().setEndTime(TimeValue::getTimeOfDay().toAccountString(timeStr, sizeof(timeStr), true));
            req.getBody().getPreviousMatch().getResult().setOutcome(outcome);
            if (blaze_stricmp(outcome, toOutcome(TournamentMatchResult::TOURNAMENT_MATCH_RESULT_RANK)) == 0)
            {
                req.getBody().getPreviousMatch().getResult().setRanking(rankingOutcome);
            }

            // call XBL
            TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].updateTeamsLastGame: Updating team(" << req.getTeamId() <<
                ")'s PreviousMatch to be(" << lastGameIdentification.getXone().getSessionName() << ") with outcome(" <<
                (outcome != nullptr ? outcome : "<nullptr>") << "), endtime(" << req.getBody().getPreviousMatch().getEndTime() << ")");

            XBLServices::TournamentsHubErrorResponse errRsp;

            const CommandInfo& cmd = ArsonXBLServices::ArsonXBLOrganizerTournamentsHubSlave::CMD_INFO_CREATEARENATOURNAMENTTEAM;
            err = callTournamentOrganizerHub(&caller, cmd, req.getHeader(), req, &getRsp, nullptr, &errRsp);
            if (err != ERR_OK)
            {
                ERR_LOG("[ArsonExternalTournamentUtilXboxOne].updateTeamsLastGame: Failed to update team(" << teamSessionName <<
                    "), tournament(" << tournamentId << ":organizer=" << organizerId << "), to have PreviousMatch(" <<
                    lastGameIdentification.getXone().getSessionName() << "). Error(" << ErrorHelp::getErrorName(err) <<
                    "). Error response: " << errRsp << ".   Request: " << req);
                return err;
            }

        }

        return err;
    }

    /*!************************************************************************************************/
    /*! \brief get all tournaments for the title, and if specified, organizer
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::getTournaments(const GetTournamentsParameters& params,
        GetTournamentsResult& result)
    {
        if (isDisabled())
        {
            return ERR_OK;
        }
        // mock testing currently doesn't support mocking Tournament Hub APIs currently
        if (isMock())
        {
            return ERR_NOT_SUPPORTED;
        }

        eastl::string continuationToken;
        do
        {
            BlazeRpcError err = getTournamentsInternal(params, result, continuationToken);
            if (err != ERR_OK)
                return err;

        } while (!continuationToken.empty() && 
            ((params.getMaxResults() == 0) || (params.getMaxResults() > result.getTournamentDetailsList().size())));

        TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].getTournaments: found(" << result.getTournamentDetailsList().size() <<
            ") tournaments for title(" << mTitleId.c_str() << "), organizerFilter(" << params.getTournamentOrganizer() <<
            "), stateFilter(" << params.getStateFilter() << "), memberFilter(" << params.getMemberFilter() <<
            "), clubIdFilter(" << params.getClubIdFilter() << "), maxResults(" << params.getMaxResults() << ").");
        return ERR_OK;
    }

    BlazeRpcError ArsonExternalTournamentUtilXboxOne::getTournamentsInternal(const GetTournamentsParameters& params,
        GetTournamentsResult& resultToAppendTo, eastl::string& continuationToken)
    {
        XBLServices::TournamentsHubErrorResponse errRsp;
        ArsonXBLServices::ArsonGetArenaTournamentsResponse rsp;
        ArsonXBLServices::ArsonGetArenaTournamentsRequest req;
        req.setTitleId(mTitleId.c_str());
        // no need to populate req header here, its populated below in call helper

        // if specified, also apply filters
        if (params.getTournamentOrganizer()[0] != '\0')
        {
            req.setOrganizerIdQueryString(eastl::string(eastl::string::CtorSprintf(), "&organizer=%s", params.getTournamentOrganizer()).c_str());
        }
        if (params.getStateFilter()[0] != '\0')
        {
            req.setStateQueryString(eastl::string(eastl::string::CtorSprintf(), "&state=%s", params.getStateFilter()).c_str());
        }
        if (params.getMemberFilter().getPlatformInfo().getExternalIds().getXblAccountId() != Blaze::INVALID_XBL_ACCOUNT_ID)
        {
            req.setMemberFilterQueryString(eastl::string(eastl::string::CtorSprintf(), "&memberId=%" PRIu64 "", params.getMemberFilter().getPlatformInfo().getExternalIds().getXblAccountId()).c_str());
        }
        if (!continuationToken.empty())
        {
            req.setContinuationTokenQueryString(continuationToken.c_str());
        }
        // if club filter empty, only get non-clubs by setting clubId="0" (Side note: for removal flow: also workarounds having to see spam due to a known MS issue, which would fail canceling some old tournaments we created with an 'invalid' clubid "0" (using earlier buggy MS APIs)
        req.setClubIdQueryString(eastl::string(eastl::string::CtorSprintf(), "&clubId=%s",
            (params.getClubIdFilter()[0] != '\0' ? params.getClubIdFilter() : "0")).c_str());

        const CommandInfo& cmd = ArsonXBLServices::ArsonXBLOrganizerTournamentsHubSlave::CMD_INFO_GETARENATOURNAMENTS;
        BlazeRpcError err = callTournamentHub(&params.getUser(), cmd, req.getHeader(), req, &rsp, nullptr, &errRsp);
        if (err != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].getTournaments: XBL error(" << ErrorHelp::getErrorName(err) << ").");
            return err;
        }
        continuationToken.clear();

        // save results
        size_t prefixLen = strlen(params.getNamePrefixFilter());
        for (const auto& xblTourny : rsp.getValue())
        {
            // if specified, also filter by name prefix
            const char8_t* xblTournyName = xblTourny->getTournament().getName();
            if ((prefixLen != 0) && (blaze_strnistr(xblTournyName, params.getNamePrefixFilter(), prefixLen) != xblTournyName))
            {
                continue;
            }

            xblTourny->getTournament().copyInto(*resultToAppendTo.getTournamentDetailsList().pull_back());

            if ((params.getMaxResults()) != 0 && (resultToAppendTo.getTournamentDetailsList().size() >= params.getMaxResults()))
            {
                return ERR_OK;
            }
        }

        // setup next continuation token as needed
        if (rsp.getNextLink()[0] != '\0')
        {
            const char8_t* pStr = blaze_stristr(rsp.getNextLink(), "&continuationToken=");
            if (pStr == nullptr)
            {
                ERR_LOG("[ArsonExternalTournamentUtilXboxOne].getTournamentsInternal: internal err: unexpected nextLink URL(" <<
                    rsp.getNextLink() << ") in XBL response, missing expected continuation token format (has MS APIs changed?)");
                return ERR_SYSTEM;
            }
            continuationToken = pStr;
        }
        return ERR_OK;
    }

    /*! \brief  helper to attempt to retrieve MPS. returns ARSON_XBLSESSIONDIRECTORYARENA_NO_CONTENT if it does not exist */
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::getMps(const char8_t* sessionTemplateName, const char8_t* sessionName,
        ArsonXBLServices::ArenaMultiplayerSessionResponse& result) const
    {
        if (!mExternalSessionUtil->validateSessionTemplateName(sessionTemplateName))
        {
            return Blaze::GAMEMANAGER_ERR_SESSION_TEMPLATE_NOT_SUPPORTED;
        }

        XBLServices::GetMultiplayerSessionRequest req;
        req.setScid(getExternalSessionConfig().getScid());
        req.setSessionTemplateName(sessionTemplateName);
        req.setSessionName(sessionName);
        req.getHeader().setContractVersion(getExternalSessionConfig().getContractVersion());

        XBLServices::MultiplayerSessionErrorResponse errorTdf;

        const CommandInfo& cmd = ArsonXBLServices::ArsonXblSessionDirectoryArenaSlave::CMD_INFO_GETMULTIPLAYERSESSION;
        BlazeRpcError err = const_cast<ArsonExternalTournamentUtilXboxOne*>(this)->callClientMpsd(cmd, req, &result, nullptr, &errorTdf);

        if (err == ARSON_XBLSESSIONDIRECTORYARENA_NO_CONTENT)
        {
            TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].getMps: no such session(" << (sessionName != nullptr ? sessionName : "<nullptr>") <<
                "), Error(" << ErrorHelp::getErrorName(err) << ").");
        }
        else if (err != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].getMps: Failed to get(" << (sessionName != nullptr ? sessionName : "<nullptr>") <<
                "), Error(" << ErrorHelp::getErrorName(err) << "). Error: " << errorTdf << ", Request: " << req);
        }

        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief return whether call should be retried
    ***************************************************************************************************/
    bool ArsonExternalTournamentUtilXboxOne::isRetryError(BlazeRpcError xblCallErr)
    {
        return ((xblCallErr == ERR_TIMEOUT) ||
            (xblCallErr == ARSON_XBLSESSIONDIRECTORYARENA_SERVICE_UNAVAILABLE) ||
            (xblCallErr == ARSON_XBLSESSIONDIRECTORYARENA_SERVICE_INTERNAL_ERROR) ||
            (xblCallErr == ARSON_XBLSESSIONDIRECTORYARENA_BAD_GATEWAY));
    }

    /*! ************************************************************************************************/
    /*! \brief get the specified team's outcome from the MPS data. returns nullptr if n/a
        \param[out] ranking If the outcome was 'rank', the team's ranking is stored here
    ***************************************************************************************************/
    const char8_t* ArsonExternalTournamentUtilXboxOne::findTeamOutcomeInLastGameResults(const char8_t* teamName,
        const ArsonXBLServices::ArsonArenaServersArbitrationPropertiesSystem& lastGameData, uint64_t& ranking) const
    {
        for (const auto& itr : lastGameData.getResults())
        {
            if (EA_LIKELY(teamName != nullptr) && (blaze_strcmp(teamName, itr.first.c_str()) == 0))
            {
                if (blaze_stricmp(itr.second->getOutcome(), toOutcome(TournamentMatchResult::TOURNAMENT_MATCH_RESULT_RANK)) == 0)
                {
                    ranking = itr.second->getRanking();
                }
                return itr.second->getOutcome();
            }
        }
        WARN_LOG("[ArsonExternalTournamentUtilXboxOne].findTeamOutcomeInLastGameResults: possible internal error: couldn't find " <<
            " any team(" << (teamName != nullptr ? teamName : "<nullptr>") << ") in the game's outcome results.");
        return nullptr;
    }

    /*!************************************************************************************************/
    /*! \brief validate params and joining users aren't already in teams. MS requires at most 1 per
        tournament, and XBL does not automatically check
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::validateJoinTeamParams(
        const TournamentTeamIdentification& team, const GameManager::TournamentIdentification& tournament,
        const UserIdentificationList& joiners)
    {
        BlazeRpcError valErr = validateTeamIdentification(team);
        if (valErr != ERR_OK)
        {
            return valErr;
        }
        valErr = validateTournamentIdentification(tournament);
        if (valErr != ERR_OK)
        {
            return valErr;
        }

        // verify joining users not already registered with a team for the tournament
        XBLServices::ArenaTournamentTeamInfo teamInfo;
        ExternalUserAuthInfo authInfo;
        
        for (auto& itr : joiners)
        {
            BlazeRpcError authErr = getAuthInfo(itr, authInfo);
            if (authErr != ERR_OK)
            {
                return authErr;
            }

            TournamentTeamInfo result;
            GetTournamentTeamParameters getTeamParams;
            itr->copyInto(getTeamParams.getUser());
            tournament.copyInto(getTeamParams.getTournamentIdentification());

            BlazeRpcError getErr = getTeamForUser(getTeamParams, result);

            // mock testing may always return a joining team, just ignore it
            if ((ARSON_TO_ERR_INVALID_TOURNAMENT_TEAM == getErr) || (teamInfo.getId()[0] == '\0') || isMock())
            {
                TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].validateJoinTeamParams: user(" << itr->getPlatformInfo().getExternalIds().getXblAccountId() <<
                    ") didn't have existing teams for tournament(" << tournament.getTournamentId() <<
                    "), Error(" << ErrorHelp::getErrorName(getErr) << ").");

                continue;//passed
            }

            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].validateJoinTeamParams: user(" << itr->getPlatformInfo().getExternalIds().getXblAccountId() <<
                ") failed check for existing teams for tournament(" <<tournament.getTournamentId() <<
                "), Error(" << ErrorHelp::getErrorName(getErr) << ").");

            return getErr;
        }

        return ERR_OK;
    }

    /*!************************************************************************************************/
    /*! \brief validate params
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::validateTeamIdentification(
        const TournamentTeamIdentification& teamIdentification) const
    {
        if (teamIdentification.getUniqueName()[0] == '\0')
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].validateTournamentTeamidentification: identification(" <<
                teamIdentification.getUniqueName() << ") invalid.");

            return ARSON_TO_ERR_INVALID_TOURNAMENT_TEAM;
        }
        return ERR_OK;
    }

    BlazeRpcError ArsonExternalTournamentUtilXboxOne::validateTournamentIdentification(
        const GameManager::TournamentIdentification& identification) const
    {
        BlazeRpcError err = ERR_OK;
        if (identification.getTournamentId()[0] == '\0')
        {
            err = ARSON_TO_ERR_INVALID_TOURNAMENT_ID;
        }
        if (identification.getTournamentOrganizer()[0] == '\0')
        {
            err = ARSON_TO_ERR_INVALID_TOURNAMENT_ORGANIZER;
        }
        if (err != ERR_OK)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].validateTournamentIdentification: Tournament identification " <<
                "invalid/missing, error(" << ErrorHelp::getErrorName(err) << ").");
        }
        return err;
    }

    // check valid state for XBL tournament, if its specified.
    bool ArsonExternalTournamentUtilXboxOne::validateTournamentState(const char8_t* state) const
    {
        // MS spec: Tournaments are Active by default. Organizers can set the tournament to Canceled or Completed by
        // explicitly updating the tournament.
        if ((state != nullptr) && (state[0] != '\0') && (blaze_stricmp(state, "canceled") != 0) &&
            (blaze_stricmp(state, "active") != 0) && (blaze_stricmp(state, "completed") != 0))
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].validateTournamentState: requested state(" << (state != nullptr ? state : "<nullptr>") << ") is not supported on this platform.");
            return false;
        }
        return true;
    }

    bool ArsonExternalTournamentUtilXboxOne::validateTournamentSchedule(const Arson::ArsonExternalTournamentScheduleInfo& schedule, bool required) const
    {
        // at create tournament, MS requires schedule to be registrationStart or playingStart
        if (required && ((schedule.getRegistrationStart()[0] == '\0') || (schedule.getPlayingStart()[0] == '\0')))
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].validateTournamentSchedule: requested schedule missing the required " <<
                "registering start(" << schedule.getRegistrationStart() << ") or play start(" << schedule.getPlayingStart() << ").");
            return false;
        }
        return ((validateUtcTimeString(schedule.getRegistrationStart(), "registering start:")) &&
            (validateUtcTimeString(schedule.getPlayingStart(), "playing start:")) &&
            ((schedule.getRegistrationEnd()[0] == '\0') || validateUtcTimeString(schedule.getRegistrationEnd(), "register end:")) &&
            ((schedule.getCheckinStart()[0] == '\0') || validateUtcTimeString(schedule.getCheckinStart(), "check in start:")) &&
            ((schedule.getCheckinEnd()[0] == '\0') || validateUtcTimeString(schedule.getCheckinEnd(), "check in end:")) &&
            ((schedule.getPlayingEnd()[0] == '\0') || validateUtcTimeString(schedule.getPlayingEnd(), "playing end:")));
    }

    bool ArsonExternalTournamentUtilXboxOne::validateUtcTimeString(const char8_t* timeString, const char8_t* logContext) const
    {
        TimeValue startTime = ((timeString != nullptr) ? TimeValue::parseAccountTime(timeString) : 0);
        if (startTime.getMicroSeconds() == 0)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].validateUtcTimeString: " << (logContext != nullptr ? logContext : "") <<
                " time string(" << (timeString != nullptr ? timeString : "<nullptr>") << ") in request was invalid, not in UTC format.");
            return false;
        }
        return true;
    }

    // MS doesn't currently provide a PATCH request, so you have to specify a copy of entire team body, every time you update it
    // Note: we also need to omit certain parameters, if they're not present in the orig session response
    void ArsonExternalTournamentUtilXboxOne::initReqTeamBodyFromGetRsp(ArsonXBLServices::ArsonArenaTournamentTeamInfo& reqBody,
        const ArsonXBLServices::ArsonArenaTournamentTeamInfo& getRsp) const
    {
        // set required always-present fields:
        reqBody.setId(getRsp.getId());
        reqBody.setRanking(getRsp.getRanking());

        // set display name if specified
        if (getRsp.getName()[0] != '\0')
        {
            reqBody.setName(getRsp.getName());
        }
        if (getRsp.getRegistrationDate()[0] != '\0')
        {
            reqBody.setRegistrationDate(getRsp.getRegistrationDate());
        }
        if (getRsp.getRtaSubscription()[0] != '\0')
        {
            reqBody.setRtaSubscription(getRsp.getRtaSubscription());
        }
        if (getRsp.getState()[0] != '\0')
        {
            reqBody.setState(getRsp.getState());
        }
        if (getRsp.getCompletedReason()[0] != '\0')
        {
            reqBody.setCompletedReason(getRsp.getCompletedReason());
        }
        if (getRsp.getFormattedString().getFormatMoniker()[0] != '\0')
        {
            getRsp.getFormattedString().copyInto(reqBody.getFormattedString());
        }
        if (getRsp.getFormattedString().getFormatMoniker()[0] != '\0')
        {
            getRsp.getFormattedString().copyInto(reqBody.getFormattedString());
        }
        if (getRsp.getStanding().getFormatMoniker()[0] != '\0')
        {
            getRsp.getStanding().copyInto(reqBody.getStanding());
        }
        if (!getRsp.getMembers().empty())
        {
            getRsp.getMembers().copyInto(reqBody.getMembers());
        }

        // set current match
        initReqTeamMatchBodyFromGetRsp(reqBody.getCurrentMatch(), getRsp.getCurrentMatch());

        // set prev match
        initReqTeamMatchBodyFromGetRsp(reqBody.getPreviousMatch(), getRsp.getPreviousMatch());
    }

    // init the match info from the orig multiplayer session response.
    // Note: we also need to omit certain parameters, if they're not present in the orig session response
    void ArsonExternalTournamentUtilXboxOne::initReqTeamMatchBodyFromGetRsp(ArsonXBLServices::ArsonArenaCommonMatchInfo& reqBody,
        const ArsonXBLServices::ArsonArenaCommonMatchInfo& getRsp) const
    {
        reqBody.clearIsSetRecursive();
        if (getRsp.getGameSessionRef().getName()[0] != '\0')
        {
            getRsp.getGameSessionRef().copyInto(reqBody.getGameSessionRef());
        }
        if (getRsp.getLabel().getFormatMoniker()[0] != '\0')
        {
            getRsp.getLabel().copyInto(reqBody.getLabel());
        }
        if (getRsp.getOpposingTeamIds().empty())
        {
            getRsp.getOpposingTeamIds().copyInto(reqBody.getOpposingTeamIds());
        }
        if (getRsp.getResult().getOutcome()[0] != '\0')
        {
            reqBody.getResult().setOutcome(getRsp.getResult().getOutcome());
            if (blaze_stricmp(getRsp.getResult().getOutcome(), toOutcome(TournamentMatchResult::TOURNAMENT_MATCH_RESULT_RANK)) == 0)
            {
                reqBody.getResult().setRanking(getRsp.getResult().getRanking());
            }
        }
        if (getRsp.getStartTime()[0] != '\0')
        {
            reqBody.setStartTime(getRsp.getStartTime());
        }
        if (getRsp.getEndTime()[0] != '\0')
        {
            reqBody.setEndTime(getRsp.getEndTime());
        }

        if (getRsp.getBye())
        {
            reqBody.setBye(getRsp.getBye());
        }
    }

    void ArsonExternalTournamentUtilXboxOne::initDefaultFormatString(Arson::ArsonArenaFormattedString& output) const
    {
        // note: valid values are specified in XDP. See MS docs for details
        output.clearIsSetRecursive();
        output.setFormatMoniker("HardCodedWinLossFormat");
        output.getParams().push_back("123");
    }

    // transfer XBL response to Blaze tdf
    void ArsonExternalTournamentUtilXboxOne::toTournamentTeamInfo(const ArsonXBLServices::ArsonArenaTournamentTeamInfo& xblTeam,
        const GameManager::TournamentIdentification& tournamentIdentification, Arson::TournamentTeamInfo& result) const
    {
        tournamentIdentification.copyInto(result.getTournamentIdentification());

        result.getTeamIdentification().setDisplayName(xblTeam.getName());
        result.setRegistrationState(parseXblRegistrationState(xblTeam.getState()));
        result.setRegistrationStateReason(xblTeam.getCompletedReason());
        result.getFinalResult().setRank(xblTeam.getRanking());
        result.setBye(xblTeam.getCurrentMatch().getBye());
        result.getStanding().setFormatMoniker(xblTeam.getStanding().getFormatMoniker());
        xblTeam.getStanding().getParams().copyInto(result.getStanding().getParams());

        for (auto& itr : xblTeam.getMembers())
        {
            convertToPlatformInfo(result.getMembers().pull_back()->getPlatformInfo(), EA::StdC::AtoU64((itr)->getId()), nullptr, INVALID_ACCOUNT_ID, xone);
        }
        result.getTeamIdentification().setUniqueName(xblTeam.getId());
        result.getTeamIdentification().setDisplayName(xblTeam.getName());

        result.getMatchIdentification().getXone().setSessionName(xblTeam.getCurrentMatch().getGameSessionRef().getName());
        result.getMatchIdentification().getXone().setTemplateName(xblTeam.getCurrentMatch().getGameSessionRef().getTemplateName());
    }
    // get XBL value from Blaze value
    const char8_t* ArsonExternalTournamentUtilXboxOne::toOutcome(TournamentMatchResult blazeResult) const
    {
        switch (blazeResult)
        {
        case TOURNAMENT_MATCH_RESULT_WIN:
            return "win";
        case TOURNAMENT_MATCH_RESULT_LOSS:
            return "loss";
        case TOURNAMENT_MATCH_RESULT_DRAW:
            return "draw";
        case TOURNAMENT_MATCH_RESULT_RANK:
            return "rank";
        case TOURNAMENT_MATCH_RESULT_NOSHOW:
            return "noshow";
        default:
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].toOutcome: unhandled input type(" <<
                TournamentMatchResultToString(blazeResult) << "), returning default 'draw'.");
            return "draw";
        };
    }
    // get XBL value from Blaze value
    const char8_t* ArsonExternalTournamentUtilXboxOne::toRegistrationState(TeamRegistrationState blazeRegistrationState) const
    {
        switch (blazeRegistrationState)
        {
        case TEAM_REGISTRATION_WAITLISTED:
            return "waitlisted";
        case TEAM_REGISTRATION_STANDBY:
            return "standby";
        case TEAM_REGISTRATION_REGISTERED:
            return "registered";
        case TEAM_REGISTRATION_CHECKEDIN:
            return "checkedIn";
        case TEAM_REGISTRATION_PLAYING:
            return "playing";
        case TEAM_REGISTRATION_COMPLETED:
            return "completed";
        default:
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].toRegistrationState: unhandled input type(" <<
                TeamRegistrationStateToString(blazeRegistrationState) << "), returning default 'registered'.");
            return "registered";
        };
    }
    // get Blaze value from XBL value
    TeamRegistrationState ArsonExternalTournamentUtilXboxOne::parseXblRegistrationState(const char8_t* xblRegistrationState) const
    {
        if (xblRegistrationState != nullptr)
        {
            if (blaze_stricmp("waitlisted", xblRegistrationState) == 0)
                return TEAM_REGISTRATION_WAITLISTED;
            else if (blaze_stricmp("standby", xblRegistrationState) == 0)
                return TEAM_REGISTRATION_STANDBY;
            else if (blaze_stricmp("registered", xblRegistrationState) == 0)
                return TEAM_REGISTRATION_REGISTERED;
            else if (blaze_stricmp("checkedIn", xblRegistrationState) == 0)
                return TEAM_REGISTRATION_CHECKEDIN;
            else if (blaze_stricmp("playing", xblRegistrationState) == 0)
                return TEAM_REGISTRATION_PLAYING;
            else if (blaze_stricmp("completed", xblRegistrationState) == 0)
                return TEAM_REGISTRATION_COMPLETED;
        }
        ERR_LOG("[ArsonExternalTournamentUtilXboxOne].parseXblRegistrationState: unhandled input(" <<
            (xblRegistrationState != nullptr ? xblRegistrationState : "<nullptr>") << "), returning default(" <<
            TeamRegistrationStateToString(TEAM_REGISTRATION_REGISTERED) << ").");

        return TEAM_REGISTRATION_REGISTERED;
    }

    const char8_t* ArsonExternalTournamentUtilXboxOne::toArbitrationStatusLogStr(const ArsonXBLServices::ArenaMultiplayerSessionResponse& mpsRsp) const
    {
        mArbitrationStatusLogBuf.sprintf("Overall:%s,resultState:%s,resultConfidenceLevel:%u)", mpsRsp.getArbitration().getStatus(),
            mpsRsp.getServers().getArbitration().getProperties().getSystem().getResultState(),
            mpsRsp.getServers().getArbitration().getProperties().getSystem().getResultConfidenceLevel());

        for (auto& itr : mpsRsp.getMembers())
        {
            mArbitrationStatusLogBuf.append_sprintf(", member{%s,arbStatus:%s,team:%s}",
                itr.second->getConstants().getSystem().getXuid(),
                itr.second->getArbitrationStatus(),
                itr.second->getConstants().getSystem().getTeam());
        }
        return mArbitrationStatusLogBuf.c_str();
    }

    // setup common header for calls to tournament hub. Fetches auth token as needed
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::getTokenAndHeaders(XBLServices::TournamentsHubRequestHeader& header,
        const UserIdentification* caller)
    {
        ExternalUserAuthInfo authInfo;
        BlazeRpcError err = getAuthInfo(caller, authInfo);
        if (err != ERR_OK)
        {
            return err;//logged
        }
        header.setAuthToken(authInfo.getCachedExternalSessionToken());
        header.setOnBehalfOfTitle(mTitleId.c_str());
        header.setContractVersion(getExternalSessionConfig().getExternalTournamentsContractVersion());
        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief Call to the XBL Client/XDK based Tournament Hub service
        \param[in] caller If not nullptr use this as the caller. Otherwise uses the current local user as needed
        \param[in,out] reqHeader Pre: this is from the req. The input req's header member. If its token has expired, this method auto-refreshes.
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::callTournamentHub(const UserIdentification* caller,
        const CommandInfo& cmdInfo,
        XBLServices::TournamentsHubRequestHeader& reqHeader, const EA::TDF::Tdf& req,
        EA::TDF::Tdf* rsp, RawBuffer* rspRaw,
        XBLServices::TournamentsHubErrorResponse* rspErr /*= nullptr*/, bool addEncodedPayload /*= true*/,
        const char8_t* contentType /*= (Encoder::JSON)*/)
    {
        BlazeRpcError err = getTokenAndHeaders(reqHeader, caller);
        if (err != ERR_OK)
        {
            return err;//logged
        }

        err = callTournamentHubInternal(cmdInfo, req, rsp, rspRaw, rspErr, contentType, addEncodedPayload);
        if (err == ARSON_XBLORGANIZERTOURNAMENTSHUB_AUTHENTICATION_REQUIRED)
        {
            // refresh header's token
            err = getTokenAndHeaders(reqHeader, caller);
            if (err == ERR_OK)
                err = callTournamentHubInternal(cmdInfo, req, rsp, rspRaw, rspErr, contentType, addEncodedPayload);
        }
        for (size_t i = 0; (isRetryError(err) && (i <= MAX_SERVICE_UNAVAILABLE_RETRIES)); ++i)
        {
            if (ERR_OK != waitSeconds(SERVICE_RETRY_WAIT_SECONDS, cmdInfo.context, i + 1))
                return ERR_SYSTEM;
            err = callTournamentHubInternal(cmdInfo, req, rsp, rspRaw, rspErr, contentType, addEncodedPayload);
        }

        TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].callTournamentHub: " << ((cmdInfo.context != nullptr) ? cmdInfo.context : "XBL") << " call returned error(" << ErrorHelp::getErrorName(err) << ").");
        return err;
    }

    BlazeRpcError ArsonExternalTournamentUtilXboxOne::callTournamentHubInternal(const CommandInfo& cmdInfo, const EA::TDF::Tdf& req,
        EA::TDF::Tdf* rsp, RawBuffer* rspRaw, XBLServices::TournamentsHubErrorResponse* rspErr, const char8_t* contentType,
        bool addEncodedPayload)
    {
        if ((mTournamentHubConnManagerPtr == nullptr) || EA_UNLIKELY(cmdInfo.componentInfo == nullptr))
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].callTournamentHubInternal: internal error: null mTournamentHubConnManagerPtr or invalid input command info. No op.");
            return ERR_SYSTEM;
        }
        HttpStatusCode statusCode = 0;
        BlazeRpcError err = RestProtocolUtil::sendHttpRequest(mTournamentHubConnManagerPtr, cmdInfo.componentInfo->id, cmdInfo.commandId, &req, contentType, addEncodedPayload,
            rsp, rspErr, nullptr, nullptr, &statusCode, cmdInfo.componentInfo->baseInfo.index, RpcCallOptions(), rspRaw);
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief Call to XBL Tournament Organizer Hub service (APIs not exposed in client XDK)
        \param[in] caller If specified w/valid ExternalId, use this as the caller. Otherwise uses current local user as needed.
        \param[in,out] reqHeader Pre: this is from the req. The input req's header member. If its token has expired, this method auto-refreshes.
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::callTournamentOrganizerHub(const UserIdentification* caller,
        const CommandInfo& cmdInfo,
        XBLServices::TournamentsHubRequestHeader& reqHeader, const EA::TDF::Tdf& req,
        EA::TDF::Tdf* rsp, RawBuffer* rspRaw,
        XBLServices::TournamentsHubErrorResponse* rspErr /*= nullptr*/, bool addEncodedPayload /*= true*/,
        const char8_t* contentType /*= (Encoder::JSON)*/)
    {
        BlazeRpcError err = getTokenAndHeaders(reqHeader, caller);
        if (err != ERR_OK)
        {
            return err;//logged
        }

        err = callTournamentOrganizerHubInternal(cmdInfo, req, rsp, rspRaw, rspErr, contentType, addEncodedPayload);
        if (err == ARSON_XBLORGANIZERTOURNAMENTSHUB_AUTHENTICATION_REQUIRED)
        {
            // refresh header's token
            err = getTokenAndHeaders(reqHeader, caller);
            if (err == ERR_OK)
                err = callTournamentHubInternal(cmdInfo, req, rsp, rspRaw, rspErr, contentType, addEncodedPayload);
        }
        for (size_t i = 0; (isRetryError(err) && (i <= MAX_SERVICE_UNAVAILABLE_RETRIES)); ++i)
        {
            if (ERR_OK != waitSeconds(SERVICE_RETRY_WAIT_SECONDS, cmdInfo.context, i + 1))
                return ERR_SYSTEM;
            err = callTournamentHubInternal(cmdInfo, req, rsp, rspRaw, rspErr, contentType, addEncodedPayload);
        }

        TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].callTournamentOrganizerHub: " <<
            ((cmdInfo.context != nullptr) ? cmdInfo.context : "XBL") << " call returned error(" << ErrorHelp::getErrorName(err) << ").");

        return err;
    }

    BlazeRpcError ArsonExternalTournamentUtilXboxOne::callTournamentOrganizerHubInternal(const CommandInfo& cmdInfo,
        const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, RawBuffer* rspRaw, XBLServices::TournamentsHubErrorResponse* rspErr,
        const char8_t* contentType, bool addEncodedPayload)
    {
        if ((mTournamentOrganizerHubConnManagerPtr == nullptr) || EA_UNLIKELY(cmdInfo.componentInfo == nullptr))
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].callTournamentOrganizerHubInternal: internal error: null mTournamentOrganizerHubConnManagerPtr or invalid input command info. No op.");
            return ERR_SYSTEM;
        }

        HttpStatusCode statusCode = 0;
        BlazeRpcError err = RestProtocolUtil::sendHttpRequest(mTournamentOrganizerHubConnManagerPtr, cmdInfo.componentInfo->id,
            cmdInfo.commandId, &req, contentType, addEncodedPayload,
            rsp, rspErr, nullptr, nullptr, &statusCode, cmdInfo.componentInfo->baseInfo.index, RpcCallOptions(), rspRaw);
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief call to XBL MPSD with user claim tokens based service (for Arena)
        \param[in] caller If not nullptr use this as the caller. Otherwise uses the current local user as needed
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::callMpsd(const UserIdentification& caller, const CommandInfo& cmdInfo, ExternalUserAuthInfo& authInfo,
        XBLServices::MultiplayerSessionRequestHeader& reqHeader, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, RawBuffer* rspRaw,
        XBLServices::MultiplayerSessionErrorResponse* rspErr /*= nullptr*/, bool addEncodedPayload /*= true*/, const char8_t* contentType /*= Encoder::JSON*/)
    {
        BlazeRpcError err = callMpsdInternal(cmdInfo, req, rsp, rspRaw, rspErr, contentType, addEncodedPayload);
        if ((err == ARSON_XBLORGANIZERTOURNAMENTSHUB_AUTHENTICATION_REQUIRED) && (ERR_OK == getAuthInfo(&caller, authInfo)))
        {
            eastl::string tokenBuf;
            reqHeader.setAuthToken(tokenBuf.c_str());
            err = callMpsdInternal(cmdInfo, req, rsp, rspRaw, rspErr, contentType, addEncodedPayload);
        }
        for (size_t i = 0; (isRetryError(err) && (i <= MAX_SERVICE_UNAVAILABLE_RETRIES)); ++i)
        {
            if (ERR_OK != waitSeconds(SERVICE_RETRY_WAIT_SECONDS, cmdInfo.context, i + 1))
                return ERR_SYSTEM;
            err = callMpsdInternal(cmdInfo, req, rsp, rspRaw, rspErr, contentType, addEncodedPayload);
        }

        TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].callMpsd: " << ((cmdInfo.context != nullptr) ? cmdInfo.context : "XBL") << " call returned error(" << ErrorHelp::getErrorName(err) << ").");
        return err;
    }

    BlazeRpcError ArsonExternalTournamentUtilXboxOne::callMpsdInternal(const CommandInfo& cmdInfo, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp,
        RawBuffer* rspRaw, XBLServices::MultiplayerSessionErrorResponse* rspErr, const char8_t* contentType, bool addEncodedPayload)
    {
        if ((mSessDirConnManagerPtr == nullptr) || EA_UNLIKELY(cmdInfo.componentInfo == nullptr))
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].callMpsdInternal: internal error: null mSessDirConnManagerPtr or invalid input command info. No op.");
            return ERR_SYSTEM;
        }
        HttpStatusCode statusCode = 0;
        BlazeRpcError err = RestProtocolUtil::sendHttpRequest(mSessDirConnManagerPtr, cmdInfo.componentInfo->id, cmdInfo.commandId, &req, contentType, addEncodedPayload,
            rsp, rspErr, nullptr, nullptr, &statusCode, cmdInfo.componentInfo->baseInfo.index, RpcCallOptions(), rspRaw);
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief call to XBL MPSD Business Partner Cert based service (for Arena)
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtilXboxOne::callClientMpsd(const CommandInfo& cmdInfo,
        const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, RawBuffer* rspRaw, XBLServices::MultiplayerSessionErrorResponse* rspErr /*= nullptr*/,
        bool addEncodedPayload /*= true*/, const char8_t* contentType /*= (Encoder::JSON)*/)
    {
        BlazeRpcError err = callClientMpsdInternal(cmdInfo, req, rsp, rspRaw, rspErr, contentType, addEncodedPayload);
        for (size_t i = 0; (isRetryError(err) && (i <= MAX_SERVICE_UNAVAILABLE_RETRIES)); ++i)
        {
            if (ERR_OK != waitSeconds(SERVICE_RETRY_WAIT_SECONDS, cmdInfo.context, i + 1))
                return ERR_SYSTEM;
            err = callClientMpsdInternal(cmdInfo, req, rsp, rspRaw, rspErr, contentType, addEncodedPayload);
        }

        TRACE_LOG("[ArsonExternalTournamentUtilXboxOne].callClientMpsd: " << ((cmdInfo.context != nullptr) ? cmdInfo.context : "XBL") << " call returned error(" << ErrorHelp::getErrorName(err) << ").");
        return err;
    }

    BlazeRpcError ArsonExternalTournamentUtilXboxOne::callClientMpsdInternal(const CommandInfo& cmdInfo, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp,
        RawBuffer* rspRaw, XBLServices::MultiplayerSessionErrorResponse* rspErr, const char8_t* contentType, bool addEncodedPayload)
    {
        ArsonXBLServices::ArsonXblSessionDirectoryArenaSlave* xblSessionDirectorySlave = (ArsonXBLServices::ArsonXblSessionDirectoryArenaSlave *)Blaze::gOutboundHttpService->getService(ArsonXBLServices::ArsonXblSessionDirectoryArenaSlave::COMPONENT_INFO.name);
        if (xblSessionDirectorySlave == nullptr)
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].callClientMpsdInternal: Failed to instantiate XblSessionDirectoryArenaSlave object.");
            return ERR_SYSTEM;
        }
        BlazeRpcError err = xblSessionDirectorySlave->sendRequest(cmdInfo, (const EA::TDF::Tdf*)&req, (EA::TDF::Tdf*) rsp, (EA::TDF::Tdf*) rspErr, RpcCallOptions());
        // side: we don't cache off a local 'mClientSessDirConnManagerPtr' (see initializeConnManagerPtrs() for why)
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief set up connection to the external service
    ***************************************************************************************************/
    void ArsonExternalTournamentUtilXboxOne::initializeConnManager(const char8_t* serverAddress, InetAddress** inetAddress,
        OutboundHttpConnectionManager** connManager, int32_t poolSize, const char8_t* serverDesc) const
    {
        const char8_t* serviceHostname = nullptr;
        bool serviceSecure = false;
        HttpProtocolUtil::getHostnameFromConfig(serverAddress, serviceHostname, serviceSecure);

        if (*inetAddress != nullptr)
            delete *inetAddress;
        *inetAddress = BLAZE_NEW InetAddress(serviceHostname);

        if (*connManager != nullptr)
            delete *connManager;
        *connManager = BLAZE_NEW OutboundHttpConnectionManager(serviceHostname);
        (*connManager)->initialize(**inetAddress, poolSize, serviceSecure);
    }

    void ArsonExternalTournamentUtilXboxOne::initializeConnManagerPtrs()
    {
        int32_t poolSize = 10;
        if (mConfig.getExternalTournamentHubServiceUrl()[0] == '\0')
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].initializeConnManagerPtrs: External tournament hub service url was invalid/empty.");
        }
        else if (mTournamentHubConnManagerPtr == nullptr)
        {
            OutboundHttpConnectionManager* tournamentHubConnManager = nullptr;
            initializeConnManager(mConfig.getExternalTournamentHubServiceUrl(), &mTournamentHubAddr, &tournamentHubConnManager, poolSize, "XBL tournament hub service");
            mTournamentHubConnManagerPtr = HttpConnectionManagerPtr(tournamentHubConnManager);
        }

        if (mConfig.getExternalTournamentOrganizerHubServiceUrl()[0] == '\0')
        {
            ERR_LOG("[ArsonExternalTournamentUtilXboxOne].initializeConnManagerPtrs: tournament organizer hub url invalid/empty.");
        }
        else if (mTournamentOrganizerHubConnManagerPtr == nullptr)
        {
            OutboundHttpConnectionManager* tournamentOrganizerHubConnManager = nullptr;
            initializeConnManager(mConfig.getExternalTournamentOrganizerHubServiceUrl(), &mTournamentOrganizerHubAddr,
                &tournamentOrganizerHubConnManager, poolSize, "XBL tournament organizer hub service");
            mTournamentOrganizerHubConnManagerPtr = HttpConnectionManagerPtr(tournamentOrganizerHubConnManager);
        }

        ASSERT_COND_LOG((gController != nullptr), "[ArsonExternalTournamentUtilXboxOne].initializeConnManagerPtrs: Internal error: cannot init http service, gController nullptr!");
        if (gController != nullptr && mSessDirConnManagerPtr == nullptr)
        {
            const char8_t* xblServiceCompName = "xblserviceconfigs";
            HttpServiceConnectionMap::const_iterator found = gController->getFrameworkConfigTdf().getHttpServiceConfig().getHttpServices().find(xblServiceCompName);
            ASSERT_COND_LOG(found != gController->getFrameworkConfigTdf().getHttpServiceConfig().getHttpServices().end(), "[ArsonExternalTournamentUtilXboxOne] MPSD session directory proxy component(" << xblServiceCompName << ") not configured. Cannot initialize connection mgr for it!");
            if (found != gController->getFrameworkConfigTdf().getHttpServiceConfig().getHttpServices().end())
            {
                OutboundHttpConnectionManager* sessDir = nullptr;
                initializeConnManager(found->second->getUrl(), &mSessDirAddr, &sessDir, poolSize, "XBL session directory (MPSD) service");
                mSessDirConnManagerPtr = HttpConnectionManagerPtr(sessDir);
            }
        }
        // side: we don't cache off a local 'mClientSessDirConnManagerPtr', because the decryption of pw keyphrase is a private method to outboundhttpservice, and it won't compile etc
    }

}
}
