/*************************************************************************************************/
/*!
    \file   userextendeddata_commands.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"

// framework includes
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getuserextendeddata_stub.h"
#include "arson/rpc/arsonslave/updateuserextendeddata_stub.h"

#include "framework/component/blazerpc.h"

namespace Blaze
{
namespace Arson
{

class UserExtendedDataUtil
{
public:
    static Blaze::BlazeRpcError getUEDKey(const char8_t& componentName, uint16_t dataId, const char8_t* dataName, UserExtendedDataKey& key)
    {
        uint16_t componentId = BlazeRpcComponentDb::getComponentIdByName(&componentName);
        key = UED_KEY_FROM_IDS(componentId, dataId);

        // If the name is given determine the componentId and dataId from the name.
        if (blaze_strcmp(dataName, "") != 0)
        {
            if (!gUserSessionManager->getUserExtendedDataKey(dataName, key))
            {
                ERR_LOG("[UserExtendedDataUtil].getUEDKey() key not found for name '" << dataName << "'");
                return Blaze::ARSON_ERR_KEY_NOT_FOUND;
            }
        }

        // invalid component or key
        if (key == 0)
        {
            ERR_LOG("[UserExtendedDataUtil].getUEDKey() invalid component '" << &componentName << "(" << componentId << ")' or key '" << key << "'");
            return Blaze::ARSON_ERR_INVALID_PARAMETER;
        }

        return Blaze::ERR_OK;
    }
};

/*************************************************************************************************/
/*!
    \class GetUserExtendedDataCommand

    Get the user extended data for this user given the request parameters.
*/
/*************************************************************************************************/
class GetUserExtendedDataCommand : public GetUserExtendedDataCommandStub
{
public:
    GetUserExtendedDataCommand(
        Message* message, GetUEDRequest* request, ArsonSlaveImpl* componentImpl)
        : GetUserExtendedDataCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GetUserExtendedDataCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetUserExtendedDataCommandStub::Errors execute() override
    {
        TRACE_LOG("[GetUserExtendedDataCommand].execute() UEDName '" << mRequest.getUserExtendedDataName() << "', componentName '" 
                  << mRequest.getComponentName() << "', dataId '" << mRequest.getUserExtendedDataId() << "'");
        UserExtendedDataKey key;
        UserExtendedDataValue value = 0;
        
        BlazeRpcError error = UserExtendedDataUtil::getUEDKey(*mRequest.getComponentName(), mRequest.getUserExtendedDataId(),
            mRequest.getUserExtendedDataName(), key);

        if (error != Blaze::ERR_OK)
        {
            return commandErrorFromBlazeError(error);
        }
        

        if (gCurrentLocalUserSession->getDataValue(key, value))
        {
            TRACE_LOG("[GetUserExtendedDataCommand].execute() value '" << value << "'");
            mResponse.setUserExtendedData(value);
        }
        else
        {
            ERR_LOG("[GetUserExtendedDataCommand].execute() user extended data not found for key(" << key << ")");
            return ARSON_ERR_DATA_NOT_FOUND;
        }
        
        return commandErrorFromBlazeError(Blaze::ERR_OK);
    }
};

DEFINE_GETUSEREXTENDEDDATA_CREATE()


/*************************************************************************************************/
/*!
\class UpdateUserExtendedDataCommand

Update the user extended data for this user given the request parameters.
*/
/*************************************************************************************************/
class UpdateUserExtendedDataCommand : public UpdateUserExtendedDataCommandStub
{
public:
    UpdateUserExtendedDataCommand(
        Message* message, UpdateUEDRequest* request, ArsonSlaveImpl* componentImpl)
        : UpdateUserExtendedDataCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~UpdateUserExtendedDataCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    UpdateUserExtendedDataCommandStub::Errors execute() override
    {
        TRACE_LOG("[UpdateUserExtendedDataCommand].execute()");
        UserExtendedDataKey key;

        BlazeRpcError error = UserExtendedDataUtil::getUEDKey(*mRequest.getComponentName(), mRequest.getUserExtendedDataId(),
            mRequest.getUserExtendedDataName(), key);

        if (error != Blaze::ERR_OK)
        {
            return commandErrorFromBlazeError(error);
        }

        error = gCurrentLocalUserSession->updateDataValue(key, mRequest.getUserExtendedData());

        return commandErrorFromBlazeError(error);
    }
};

DEFINE_UPDATEUSEREXTENDEDDATA_CREATE()


} // Arson
} // Blaze
