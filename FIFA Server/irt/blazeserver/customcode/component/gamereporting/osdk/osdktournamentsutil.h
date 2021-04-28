/*************************************************************************************************/
/*!
    \file   osdktournamentsutil.h
    \attention
        (c) Electronic Arts. All Rights Reserved
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEREPORTING_OSDKTOURNAMENTSUTIL
#define BLAZE_GAMEREPORTING_OSDKTOURNAMENTSUTIL

#include "osdktournaments/tdf/osdktournamentstypes_server.h"

namespace Blaze
{
namespace GameReporting
{

class OSDKTournamentsUtil
{
public:
    OSDKTournamentsUtil() {}

    ~OSDKTournamentsUtil() {}

    /*************************************************************************************************/
    /*!
        \brief reportMatchResult

        helper function for other components to report match results to osdktournaments

        \param[in] tournId - The Id of the tournament that the match was played in
        \param[in] memberOneId - member one's blazeId
        \param[in] memberTwoId - member two's blazeId
        \param[in] memberOneTeam - member one's team
        \param[in] memberTwoTeam - member two's team
        \param[in] memberOneScore - member one's score
        \param[in] memberTwoScore - member two's score
        \param[in] memberOneMetaData - arbitrary data associated with member one for the match
        \param[in] memberTwoMetaData - arbitrary data associated with member two for the match

        \return - the result of the operation.
    */
    /*************************************************************************************************/
    BlazeRpcError reportMatchResult(
        OSDKTournaments::TournamentId tournId,
        OSDKTournaments::TournamentMemberId memberOneId, OSDKTournaments::TournamentMemberId memberTwoId,
        OSDKTournaments::TeamId memberOneTeam, OSDKTournaments::TeamId memberTwoTeam,
        uint32_t memberOneScore, uint32_t memberTwoScore,
        const char8_t* memberOneMetaData, const char8_t* memberTwoMetaData) const;

    DbError getMemberStatus(OSDKTournaments::TournamentMemberId memberId, OSDKTournaments::TournamentMemberData& data) const;

    DbError getMemberStatus(OSDKTournaments::TournamentMemberId memberId, OSDKTournaments::TournamentMode mode, OSDKTournaments::TournamentMemberData& data) const;
};
} //namespace GameReporting
} //namespace Blaze

#endif //BLAZE_GAMEREPORTING_OSDKTOURNAMENTSUTIL
