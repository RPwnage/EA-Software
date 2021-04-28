/*************************************************************************************************/
/*!
    \file   osdktournamentmatchnode.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_OSDKTOURNAMENTMATCHNODE_H
#define BLAZE_OSDKTOURNAMENTMATCHNODE_H

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#include "framework/tdf/userdefines.h"
#include "osdktournaments/tdf/osdktournamentstypes.h"

namespace Blaze
{
namespace OSDKTournaments
{

class TournamentMatchNode : public TournamentNode
{
public:
    TournamentMatchNode()
    : mLeftNodeId(0),
    mRightNodeId(0)
    {
    }

    TournamentMatchNode(TournamentId tournId, TournamentMemberId memberOneId, TournamentMemberId memberTwoId, uint32_t memberOneTeam, uint32_t memberTwoTeam, uint32_t memberOneScore, 
        uint32_t memberTwoScore, uint32_t leftMatchId, uint32_t rightMatchId, const char8_t* memberOneAttribute, 
        const char8_t* memberTwoAttribute, const char8_t* memberOneMetaData, const char8_t* memberTwoMetaData)
    :   mLeftNodeId(leftMatchId),
        mRightNodeId(rightMatchId)
    {
        setTournamentId(tournId);
        setMemberOneId(memberOneId);
        setMemberTwoId(memberTwoId);
        setMemberOneTeam(memberOneTeam);
        setMemberTwoTeam(memberTwoTeam);
        setMemberOneScore(memberOneScore);
        setMemberTwoScore(memberTwoScore);
        setMemberOneAttribute(memberOneAttribute);
        setMemberTwoAttribute(memberTwoAttribute);
        setMemberOneMetaData(memberOneMetaData);
        setMemberTwoMetaData(memberTwoMetaData);
    }

    uint32_t getLeftNodeId() const { return mLeftNodeId; }
    uint32_t getRightNodeId() const { return mRightNodeId; }
    void setLeftNodeId(uint32_t value) { mLeftNodeId = value; }
    void setRightNodeId(uint32_t value) { mRightNodeId = value; }

private:
    uint32_t mLeftNodeId;
    uint32_t mRightNodeId;
};

} // OSDKTournaments
} // Blaze

#endif //BLAZE_TOURNAMENTMATCHNODE_H
