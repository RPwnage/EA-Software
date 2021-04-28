/*************************************************************************************************/
/*!
    \file   osdktournamentsutil.cpp
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/database/dbscheduler.h"
#include "osdktournaments/tdf/osdktournamentstypes_custom.h"
#include "osdktournaments/rpc/osdktournamentsslave.h"
#include "osdktournamentsutil.h"

namespace Blaze
{
namespace GameReporting
{

BlazeRpcError OSDKTournamentsUtil::reportMatchResult(
    OSDKTournaments::TournamentId tournId,
    OSDKTournaments::TournamentMemberId memberOneId, OSDKTournaments::TournamentMemberId memberTwoId,
    OSDKTournaments::TeamId memberOneTeam, OSDKTournaments::TeamId memberTwoTeam,
    uint32_t memberOneScore, uint32_t memberTwoScore,
    const char8_t* memberOneMetaData, const char8_t* memberTwoMetaData) const
{
    OSDKTournaments::OSDKTournamentsSlave* component = static_cast<OSDKTournaments::OSDKTournamentsSlave*>(gController->getComponent(OSDKTournaments::OSDKTournamentsSlave::COMPONENT_ID, false));    
    if (component == NULL)
    {
        WARN_LOG("[OSDKTournamentsUtil].reportMatchResult() - osdktournaments component is not available");
        return Blaze::ERR_SYSTEM;
    }

    OSDKTournaments::ReportMatchResultRequest req;
    req.setTournId(tournId);
    req.setUserOneId(memberOneId);
    req.setUserTwoId(memberTwoId);
    req.setUserOneTeam(memberOneTeam);
    req.setUserTwoTeam(memberTwoTeam);
    req.setUserOneScore(memberOneScore);
    req.setUserTwoScore(memberTwoScore);
    req.setUserOneMetaData(memberOneMetaData);
    req.setUserTwoMetaData(memberTwoMetaData);

    // to invoke this RPC will require authentication
    UserSession::pushSuperUserPrivilege();
    BlazeRpcError error =  component->reportMatchResult(req);
    UserSession::popSuperUserPrivilege();

    return error;
}

DbError OSDKTournamentsUtil::getMemberStatus(OSDKTournaments::TournamentMemberId memberId, OSDKTournaments::TournamentMemberData& data) const
{
    DbError err = getMemberStatus(memberId, OSDKTournaments::MODE_USER, data);
    return err;
}

DbError OSDKTournamentsUtil::getMemberStatus(OSDKTournaments::TournamentMemberId memberId, OSDKTournaments::TournamentMode mode, OSDKTournaments::TournamentMemberData& data) const
{
    OSDKTournaments::OSDKTournamentsSlave* component = static_cast<OSDKTournaments::OSDKTournamentsSlave*>(gController->getComponent(OSDKTournaments::OSDKTournamentsSlave::COMPONENT_ID, false));    
    if (component == NULL)
    {
        WARN_LOG("[OSDKTournamentsUtil].getMemberStatus() - osdktournaments component is not available");
        return Blaze::ERR_SYSTEM;
    }

    OSDKTournaments::GetMemberStatusRequest req;
    req.setMemberId(memberId);
    req.setMode(mode);

    // to invoke this RPC will require authentication
    UserSession::pushSuperUserPrivilege();
    BlazeRpcError error = component->getMemberStatus(req,data);
    UserSession::popSuperUserPrivilege();

    return error;
}

} //namespace GameReporting
} //namespace Blaze


