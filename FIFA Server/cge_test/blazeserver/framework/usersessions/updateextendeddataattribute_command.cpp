/*************************************************************************************************/
/*!
    \file   updateextendeddataattribute_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateExtendedDataAttributeCommand 

    process user session extended data retrieval requests from the client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/updateextendeddataattribute_stub.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/util/connectutil.h"

namespace Blaze
{
    class UpdateExtendedDataAttributeCommand : public UpdateExtendedDataAttributeCommandStub
    {
    public:

        UpdateExtendedDataAttributeCommand(Message* message, Blaze::UpdateExtendedDataAttributeRequest* request, UserSessionsSlave* componentImpl)
            : UpdateExtendedDataAttributeCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~UpdateExtendedDataAttributeCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        UpdateExtendedDataAttributeCommandStub::Errors execute() override
        {
            ComponentId componentId = mRequest.getComponent();
            uint16_t dataId = mRequest.getKey();
            UserExtendedDataKey uedKey = UED_KEY_FROM_IDS(componentId, dataId);

            if (componentId != Component::INVALID_COMPONENT_ID)
            {
                return commandErrorFromBlazeError(Blaze::USER_ERR_KEY_NOT_FOUND);
            }

            UserExtendedDataValue value = mRequest.getValue();
            BlazeRpcError err = Blaze::ERR_OK;
            bool doRemove = false;

            // Check if the key is whitelisted:
            const UserSessionManager::UEDClientConfigMap& uedConfigMap = gUserSessionManager->getUEDClientConfigMap();
            UserSessionManager::UEDClientConfigMap::const_iterator uedIter = uedConfigMap.find( uedKey );
            if (uedIter != uedConfigMap.end()) 
            {
                const UserSessionClientUEDConfig& config = *uedIter->second;
                if (mRequest.getRemove())
                    value = config.getDefault();

                // Clamp value:
                if (value < config.getMin())
                    value = config.getMin();
                if (value > config.getMax())
                    value = config.getMax();
            }
            else
            {
                const UserExtendedDataMap& uedDataMap = gCurrentLocalUserSession->getExtendedData().getDataMap();
                bool foundData = (uedDataMap.find( uedKey ) != uedDataMap.end());
                if (foundData)
                {
                    if (mRequest.getRemove())
                    {
                        doRemove = true;
                    }
                }
                else
                {
                    if (mRequest.getRemove())
                    {   // Error when attempting to remove a non-set client value
                        err = Blaze::USER_ERR_KEY_NOT_FOUND;
                    }
                    else
                    {
                        // Tally up the total number of client values, and make sure this one fits.
                        uint32_t numClientKeys = 0;
                        for (UserExtendedDataMap::const_iterator iter = uedDataMap.begin(); iter != uedDataMap.end(); ++iter)
                        {
                            uint16_t curComponentId = COMPONENT_ID_FROM_UED_KEY( iter->first );
                            if( curComponentId == Component::INVALID_COMPONENT_ID )
                                ++numClientKeys;
                        }
                    
                        int numWhitelistedKeys = uedConfigMap.size();    // These will always be present, and don't count towards the limit.
                        if (numClientKeys > gUserSessionManager->getConfig().getMaxClientUEDAttributes() + numWhitelistedKeys)
                        {
                            err = Blaze::USER_ERR_MAX_DATA_REACHED;
                        }
                    }
                }
            }

            // If we didn't hit an error case, remove or update the data:
            if (err == ERR_OK)
            {
                if (!doRemove)
                    err = gUserSessionManager->updateExtendedData(gCurrentUserSession->getSessionId(), componentId, dataId, value );
                else
                    err = gUserSessionManager->removeExtendedData(gCurrentUserSession->getSessionId(), componentId, dataId);
            }

            return commandErrorFromBlazeError(err);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_UPDATEEXTENDEDDATAATTRIBUTE_CREATE_COMPNAME(UserSessionManager)

} // Blaze
