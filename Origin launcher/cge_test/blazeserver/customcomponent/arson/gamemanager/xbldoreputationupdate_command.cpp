/*************************************************************************************************/
/*!
    \file   xbldoreputationupdate_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/xbldoreputationupdate_stub.h"

#include "externalreputationservicefactorytesthook.h"


namespace Blaze
{
namespace Arson
{
class XblDoReputationUpdateCommand : public XblDoReputationUpdateCommandStub, public Blaze::ReputationService::TestHook
{
public:
    XblDoReputationUpdateCommand(
        Message* message, Blaze::Arson::DoReputationUpdateRequest* request, ArsonSlaveImpl* componentImpl)
        : XblDoReputationUpdateCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~XblDoReputationUpdateCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    XblDoReputationUpdateCommandStub::Errors execute() override
    {
        bool bFoundPoorReputation = false;
        Blaze::ReputationService::ReputationServiceUtilPtr externalReputationServiceUtilXboxOne = getReputationServiceUtil();

        if(!getReputationUtil)
        {
            return ERR_SYSTEM;
        }

        UserSession::SuperUserPermissionAutoPtr autoPtr(true);
        UserIdentificationList tempExternalUserList;
        for (ExternalIdList::const_iterator iter = mRequest.getExternalIds().begin(); iter != mRequest.getExternalIds().end(); ++iter)
        {
            UserIdentification newId;
            convertToPlatformInfo(newId.getPlatformInfo(), *iter, nullptr, INVALID_ACCOUNT_ID, xone);
            newId.copyInto(*tempExternalUserList.pull_back());
        }

        bFoundPoorReputation = externalReputationServiceUtilXboxOne->doReputationUpdate(mRequest.getUserSessionIdList(), tempExternalUserList);

       // Blaze::ReputationService::ReputationServiceUtil &externalReputationServiceUtilXboxOne =  gUserSessionManager->getReputationServiceUtil();
       // bFoundPoorReputation = externalReputationServiceUtilXboxOne.doReputationUpdate(mRequest.getUserSessionIdList(), mRequest.getExternalIds());

        mResponse.setFoundPoorReputation(bFoundPoorReputation);

        return ERR_OK;
    }

};

DEFINE_XBLDOREPUTATIONUPDATE_CREATE()

} //Arson
} //Blaze
