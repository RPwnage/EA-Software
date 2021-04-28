/*************************************************************************************************/
/*!
    \file   getmytournamentdetails_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"

// osdktournaments includes
#include "osdktournaments/osdktournamentsslaveimpl.h"
#include "osdktournaments/rpc/osdktournamentsslave/getmytournamentdetails_stub.h"
#include "osdktournamentsdatabase.h"

namespace Blaze
{
namespace OSDKTournaments
{

class GetMyTournamentDetailsCommand : public GetMyTournamentDetailsCommandStub
{
public:
    GetMyTournamentDetailsCommand(Message* message, GetMyTournamentDetailsRequest* request, OSDKTournamentsSlaveImpl* compImpl)
    :   GetMyTournamentDetailsCommandStub(message, request),
        mComponent(compImpl)
    { 
    }

    virtual ~GetMyTournamentDetailsCommand() {}

private:
    OSDKTournamentsSlaveImpl* mComponent;
    GetMyTournamentDetailsCommandStub::Errors execute();

};
// static factory method impl
DEFINE_GETMYTOURNAMENTDETAILS_CREATE()

GetMyTournamentDetailsCommandStub::Errors GetMyTournamentDetailsCommand::execute()
{
    TRACE_LOG("[GetMyTournamentDetailsCommand:" << this << "] start");
    GetMyTournamentDetailsCommandStub::Errors result = GetMyTournamentDetailsCommandStub::ERR_OK;

    DbConnPtr conn = gDbScheduler->getReadConnPtr(mComponent->getDbId());
    if (conn == NULL)
    {
        // early return here when the scheduler is unable to find a connection
        ERR_LOG("[GetMyTournamentDetailsCommandStub:" << this << "].execute() - Failed to obtain connection.");
        return GetMyTournamentDetailsCommandStub::ERR_SYSTEM;
    }
    OSDKTournamentsDatabase tournDb(conn);

    TournamentMemberId memberId = mRequest.getMemberIdOverride();
    if (INVALID_TOURNAMENT_MEMBER_ID == memberId)
    {
        // if the request's member id is "invalid", calculate which member id should be used
        memberId = mComponent->getCurrentUserMemberIdForMode(mRequest.getMode());
    }
    
    TournamentId tournId = mRequest.getTournamentId();

    TournamentMemberData memberData;
    if (DB_ERR_OK == tournDb.getMemberStatus(memberId, mRequest.getMode(), tournId, memberData))
    {
        // ensure that we're using the id from the db
        tournId = memberData.getTournamentId();

		const TournamentData* tournData = mComponent->getTournamentDataById(tournId);
        if ((INVALID_TOURNAMENT_ID == tournId) || (NULL == tournData))
        {
            result = GetMyTournamentDetailsCommandStub::OSDKTOURN_ERR_MEMBER_NOT_IN_TOURNAMENT;
        }
        else
        {
            //set the values after we make sure the tournament is valid and enabled.
            mResponse.setTournamentId(tournId);
            mResponse.setLevel(memberData.getLevel());
            mResponse.setIsActive(memberData.getIsActive());
            mResponse.setTournAttribute(memberData.getTournAttribute());
            mResponse.setTeam(memberData.getTeamId());

            TournamentNodeMap nodeMap;
            uint32_t lastMatchId = 0;
            if (DB_ERR_OK == tournDb.getTreeForMember(memberId, &memberData, tournData, lastMatchId, nodeMap))
            {
                TournamentNodeMap::const_iterator itr = nodeMap.begin();
                TournamentNodeMap::const_iterator end = nodeMap.end();
                for (; itr != end; ++itr)
                {
                    mResponse.getTournamentTree().push_back(itr->second);
                    //this block of code is only kept because the team id never used to be cached in the
					// members table.  users upgrading to v6 of the db would have retrieved -1 for the team
					// of users that were already in the tournament at time of upgrade
                    if (itr->first == lastMatchId)
                    {
                        TournamentNode* node = itr->second;
                        if (node->getMemberOneId() == memberId)
                        {
                            mResponse.setTeam(node->getMemberOneTeam());
                        }
                        else
                        {
                            mResponse.setTeam(node->getMemberTwoTeam());
                        }
                    }
                }
                ++(mComponent->mMetrics.mTotalTreeRetrievals);
            }
            else
            {
                result = GetMyTournamentDetailsCommandStub::OSDKTOURN_ERR_TOURNAMENT_GENERIC;
            }
        }
    }
    else
    {
        result = GetMyTournamentDetailsCommandStub::OSDKTOURN_ERR_TOURNAMENT_GENERIC;
    }

    return result;
}

} // OSDKTournaments
} // Blaze
