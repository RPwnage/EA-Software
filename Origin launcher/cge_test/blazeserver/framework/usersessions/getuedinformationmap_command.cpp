/*************************************************************************************************/
/*!
    \file   getuedinformationmap_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetUedInformationMapCommand

    Get the set of defined UED (and their defaults) for this Blaze instance, or the UED values for a particular user.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/getuedinformationmap_stub.h"

namespace Blaze
{

static const char8_t* UNNAMED_UED_OUTPUT_NAME = "<unnamed UED>";
static const char8_t* CLIENT_UED_COMPNENT_NAME = "Client UED";

class GetUedInformationMapCommand : public GetUedInformationMapCommandStub
{
public:

    GetUedInformationMapCommand(Message* message, Blaze::GetUedInformationMapRequest* request, UserSessionsSlave* componentImpl)
        :GetUedInformationMapCommandStub(message, request),
            mComponent(componentImpl)
    {
    }

    ~GetUedInformationMapCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:

    GetUedInformationMapCommandStub::Errors execute() override
    {
        BLAZE_TRACE_LOG(Log::USER, "[GetUedInformationMapCommand].execute()");

        // check if the current user has the permission to get the UED info map
        if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_GET_UED_INFORMATION_MAP))
        {
            BLAZE_WARN_LOG(Log::USER, "[GetUedInformationMapCommand].execute: User [" << ((gCurrentUserSession != nullptr) ? gCurrentUserSession->getBlazeId() : INVALID_BLAZE_ID) << "] " 
                << " attempted to retrieve the UED info map without permission!" );

            return ERR_AUTHORIZATION_REQUIRED;
        }

        BlazeRpcError err = ERR_OK;
        UserSessionExtendedData extendedData;
        convertToPlatformInfo(mRequest.getTargetUserIdentification());
        if (UserInfo::isUserIdentificationValid(mRequest.getTargetUserIdentification()))
        {
            // look up the user
            UserInfoPtr userInfo;
            BlazeRpcError userError = gUserSessionManager->lookupUserInfoByIdentification(mRequest.getTargetUserIdentification(), userInfo);

            if (userError == ERR_OK)
            {
                // We're only keeping the UED, so pass platform 'common' to filloutUserData to skip the obfuscation of 1st-party ids
                UserData userData;
                userError = gUserSessionManager->filloutUserData(*userInfo, userData,
                    (gCurrentUserSession != nullptr && userInfo->getId() == gCurrentUserSession->getBlazeId()) || UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_OTHER_NETWORK_ADDRESS, true), common);

                userData.getExtendedData().copyInto(extendedData);
            }

            if (userError != ERR_OK)
                return commandErrorFromBlazeError(userError);

        }
        else
        {
            // There's no need to set these to INVALID, since that's the default. 
            UserInfoData userInfo;
            err = gUserSessionManager->requestUserExtendedData(userInfo, extendedData, false);
            if (err != ERR_OK)//if err, logged
            {
                return ERR_SYSTEM;
            }
        }

        // lookup was a success, now build out our UED info
        UserExtendedDataMap::const_iterator dataIter = extendedData.getDataMap().begin();
        UserExtendedDataMap::const_iterator dataEnd = extendedData.getDataMap().end();
        for(; dataIter != dataEnd; ++dataIter)
        {
            UserExtendedDataKey currentKey = dataIter->first;
            ComponentId componentId = COMPONENT_ID_FROM_UED_KEY(currentKey);
            const char8_t* componentName;
            if (componentId != Component::INVALID_COMPONENT_ID)
            {
                componentName = BlazeRpcComponentDb::getComponentNameById(componentId);
            }
            else
            {
                componentName = CLIENT_UED_COMPNENT_NAME;
            }

            const char8_t* uedName = gUserSessionManager->getUserExtendedDataName(currentKey);

            if (uedName == nullptr)
            {
                // something is really wrong- UED key exists, but isn't in the name to key map!
                BLAZE_TRACE_LOG(Log::USER, "[GetUedInformationMapCommand].execute: UED contained key [" << currentKey 
                    << "] for component \"" << componentName << "\", but no name is defined for this key!" );

                uedName = UNNAMED_UED_OUTPUT_NAME;
            }

            ComponentUedDefinitions* uedDefinitions;
            ComponentUedDefinitionsMap::iterator uedDefinitionsIter = mResponse.getComponentUedDefinitionsMap().find(componentName);
            if (uedDefinitionsIter == mResponse.getComponentUedDefinitionsMap().end())
            {
                // new component, allocate and add to the map
                uedDefinitions = mResponse.getComponentUedDefinitionsMap().allocate_element();
                mResponse.getComponentUedDefinitionsMap()[componentName] = uedDefinitions;
            }
            else
            {
                // already encountered this component, add to the existing definition's list
                uedDefinitions = uedDefinitionsIter->second;
            }

            // each UED key is unique, so we don't need to test for uniqueness, and can just add to our list.
            uedDefinitions->setComponentId(componentId);
            UedDefinition* currentUedDefinition = uedDefinitions->getComponentUedList().pull_back();
            currentUedDefinition->setDataKey(currentKey);
            currentUedDefinition->setDefaultValue(dataIter->second); // this is the default value because the user we "looked up" above might not exist
            currentUedDefinition->setUserExtendedDataName(uedName);
        }

        return ERR_OK;
    }

private:
    UserSessionsSlave* mComponent;  // memory owned by creator, don't free
};

DEFINE_GETUEDINFORMATIONMAP_CREATE_COMPNAME(UserSessionManager)

} // Blaze
