/*************************************************************************************************/
/*!
    \file   updateprofaneclubname_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetClubName

    Get members for a specify club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/updateprofaneclubname_stub.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"
#include "clubsslaveimpl.h"

namespace Blaze
{
namespace Clubs
{

class UpdateProfaneClubNameCommand : public UpdateProfaneClubNameCommandStub, private ClubsCommandBase
{
public:
    UpdateProfaneClubNameCommand(Message* message, UpdateProfaneClubNameRequest* request, ClubsSlaveImpl* componentImpl)
        : UpdateProfaneClubNameCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~UpdateProfaneClubNameCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    UpdateProfaneClubNameCommandStub::Errors execute() override
    {
        TRACE_LOG("[UpdateProfaneClubName].execute");

        BlazeRpcError error = Blaze::ERR_OK;

        if (mComponent->getConfig().getDisableAbuseReporting())
        {
            return CLUBS_ERR_NO_PRIVILEGE;
        }

        ClubId clubId = mRequest.getClubId();

        ClubsDbConnector dbc(mComponent);
        if (dbc.acquireDbConnection(false) == nullptr)
            return ERR_SYSTEM;

        if (!dbc.startTransaction())
            return ERR_SYSTEM;

        if ((clubId == INVALID_CLUB_ID) || !mComponent->checkClubId(dbc.getDbConn(), clubId))
            return CLUBS_ERR_INVALID_CLUB_ID;

        int numRenameTries = 5;
        while (numRenameTries > 0)
        {
            //pick a new club name
            eastl::string newName;
            int randomNumber = Blaze::Random::getRandomNumber(1000000);
            newName.sprintf("FIFA00%d", randomNumber);

            //profanity check
            error = mComponent->checkClubStrings(newName.c_str(), nullptr, nullptr, nullptr);
            if (error == Blaze::ERR_OK)
            {
                //submit name
                uint64_t version = 0;
                error = dbc.getDb().changeClubStrings(clubId, newName.c_str(), nullptr, nullptr, version);

                if ((error == Blaze::ERR_OK) || (error != CLUBS_ERR_CLUB_NAME_IN_USE))
                {
                    //success or error we don't know how to handle
                    break;
                }
            }

            //we havent't succeeded for some reason (profanity or club name in use) so keep going
            --numRenameTries;
        }

		if (error == Blaze::ERR_OK)
		{
			int32_t clubNameResetCount = 0;
			error = dbc.getDb().getClubNameResetCount(clubId, clubNameResetCount);

			if (error == Blaze::ERR_OK)
			{
				if (clubNameResetCount < mComponent->getConfig().getMaxClubNameResets())
				{
					dbc.getDb().changeIsNameProfaneFlag(clubId, 1);
					mComponent->logEvent(&dbc.getDb(), clubId, mComponent->getEventString(LOG_EVENT_FORCED_CLUB_NAME_CHANGE));
				}
				else
				{
					dbc.getDb().changeIsNameProfaneFlag(clubId, 0);
					mComponent->logEvent(&dbc.getDb(), clubId, mComponent->getEventString(LOG_EVENT_MAX_CLUB_NAME_CHANGES));
				}
			}
		}

        dbc.completeTransaction((error == Blaze::ERR_OK));

        return commandErrorFromBlazeError(error);
    }

};
// static factory method impl
DEFINE_UPDATEPROFANECLUBNAME_CREATE()

} // Clubs
} // Blaze
