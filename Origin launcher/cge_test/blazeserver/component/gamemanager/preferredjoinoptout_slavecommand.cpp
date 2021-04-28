/*! ************************************************************************************************/
/*!
    \file preferredjoinoptout_slavecommand.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/preferredjoinoptout_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"

#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief Opt out the player from its preferred join call (i.e. joinGameByUserlist), when
        the game is locked for preferred joins.
    ***************************************************************************************************/
    class PreferredJoinOptOutCommand : public PreferredJoinOptOutCommandStub
    {
    public:

        PreferredJoinOptOutCommand(Message* message, PreferredJoinOptOutRequest *request, GameManagerSlaveImpl* componentImpl)
            : PreferredJoinOptOutCommandStub(message, request), mComponent(*componentImpl)
        {
        }
        ~PreferredJoinOptOutCommand() override
        {
        }

    private:

        PreferredJoinOptOutCommandStub::Errors execute() override
        {
            PreferredJoinOptOutMasterRequest masterRequest;
            masterRequest.setGameId(mRequest.getGameId());

            // add additional members of its connection group as needed
            const UserGroupId groupId = gCurrentLocalUserSession->getConnectionGroupObjectId();
            if (groupId != EA::TDF::OBJECT_ID_INVALID)
            {
                BlazeRpcError err = gUserSetManager->getSessionIds(groupId, masterRequest.getSessionIdList());
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[PreferredJoinOptOutCommand] Failed to retrieve connection group(" << groupId.toString().c_str() << ") user ids for game(" << mRequest.getGameId() << ") for calling player(blaze id: " << UserSession::getCurrentUserBlazeId() << "), error " << ErrorHelp::getErrorName(err));
                    return PreferredJoinOptOutCommandStub::commandErrorFromBlazeError(err);
                }
            }
            else
            {
                masterRequest.getSessionIdList().push_back(UserSession::getCurrentUserSessionId());
            }

            return PreferredJoinOptOutCommandStub::commandErrorFromBlazeError(
                mComponent.getMaster()->preferredJoinOptOutMaster(masterRequest));
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //static creation factory method of command's stub class
    DEFINE_PREFERREDJOINOPTOUT_CREATE()

} // namespace GameManager
} // namespace Blaze
