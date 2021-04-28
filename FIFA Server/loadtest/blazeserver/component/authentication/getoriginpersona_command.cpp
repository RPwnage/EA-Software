/*************************************************************************************************/
/*!
    \file   getoriginpersona_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetOriginPersonaCommand

    Fetch origin persona.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/authenticationimpl.h"
#include "authentication/rpc/authenticationslave/getoriginpersona_stub.h"
#include "authentication/util//getpersonautil.h"
#include "authentication/util/authenticationutil.h"

#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"

namespace Blaze
{
namespace Authentication
{

class GetOriginPersonaCommand : public GetOriginPersonaCommandStub
{
public:
    GetOriginPersonaCommand(Message* message, GetOriginPersonaRequest* request, AuthenticationSlaveImpl* componentImpl)
        : GetOriginPersonaCommandStub(message, request),
        mComponent(componentImpl),
        mGetOriginPersonaUtil(getPeerInfo())
    {
    }

    ~GetOriginPersonaCommand() override {};

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

    GetPersonaUtil mGetOriginPersonaUtil;

    /* Private methods *******************************************************************************/
    GetOriginPersonaCommandStub::Errors execute() override
    {
        BlazeRpcError error = Blaze::ERR_OK;
        if (mRequest.getBlazeIdOrPersonaName().getActiveMember() == Blaze::Authentication::OriginPersonaIdentifier::MEMBER_PERSONANAME)
        {
            AccountId accountId = INVALID_ACCOUNT_ID;
            error = mGetOriginPersonaUtil.getPersona(mRequest.getBlazeIdOrPersonaName().getPersonaName(),
                NAMESPACE_ORIGIN, &accountId, &mResponse.getPersonaInfo());
            mResponse.setAccountId(accountId);
        }
        else
        {
            BlazeId blazeId = INVALID_BLAZE_ID;
            if (mRequest.getBlazeIdOrPersonaName().getActiveMember() == Blaze::Authentication::OriginPersonaIdentifier::MEMBER_BLAZEID)
                blazeId = mRequest.getBlazeIdOrPersonaName().getBlazeId();

            AccountId accountId = INVALID_ACCOUNT_ID;
            if (blazeId == INVALID_BLAZE_ID)
            {
                accountId = gCurrentUserSession->getAccountId();
                if (accountId == INVALID_ACCOUNT_ID)
                {
                    WARN_LOG("[GetOriginPersonaCommand].execute(): No Blaze ID was provided in the request and no user is logged in. "
                        << "Failed with error [AUTH_ERR_INVALID_USER]");
                    return AUTH_ERR_INVALID_USER;
                }
            }
            else
            {
                PersonaInfo personaInfo;
                error = mGetOriginPersonaUtil.getPersona(blazeId, &accountId, &personaInfo);
                if (error != Blaze::ERR_OK)
                {    
                    ERR_LOG("[GetOriginPersonaCommand].execute(): getPersona for personaId[" << blazeId << "], failed with error [" 
                        << ErrorHelp::getErrorName(error) << "]");
                    return (commandErrorFromBlazeError(error));
                }
            }

            PersonaInfoList personaInfoList;
            error = mGetOriginPersonaUtil.getPersonaList(accountId, NAMESPACE_ORIGIN, personaInfoList, Blaze::Nucleus::PersonaStatus::UNKNOWN);
            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[GetOriginPersonaCommand].execute(): getPersonaList for nucleus id[" << accountId << "], namespace[" << NAMESPACE_ORIGIN
                    << "], failed with error [" << ErrorHelp::getErrorName(error) << "]");
                return (commandErrorFromBlazeError(error));
            }

            if (personaInfoList.empty())
            {
                TRACE_LOG("[GetOriginPersonaCommand].execute(): personaInfoList is empty for nucleus id[" << accountId << "], "
                    "failed with error [AUTH_ERR_PERSONA_NOT_FOUND]");
                return AUTH_ERR_PERSONA_NOT_FOUND;
            }

            personaInfoList[0]->copyInto(mResponse.getPersonaInfo());

            mResponse.setAccountId(accountId);
        }

        // Return results.
        return (commandErrorFromBlazeError(error));
    }

};
DEFINE_GETORIGINPERSONA_CREATE();

/* Private methods *******************************************************************************/


} // Authentication
} // Blaze
