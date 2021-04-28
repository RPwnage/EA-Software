/*************************************************************************************************/
/*!
    \file   pokeslave_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class PokeSlaveCommand

    Pokes the slave.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/event/eventmanager.h"
#include "framework/util/random.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/pokeslave_stub.h"


namespace Blaze
{
namespace Arson
{

    class PokeSlaveCommand : public PokeSlaveCommandStub
    {
    public:
        PokeSlaveCommand(Message* message, ArsonRequest* request, ArsonSlaveImpl* componentImpl)
            : PokeSlaveCommandStub(message, request), mComponent(componentImpl)
        {
        }

        ~PokeSlaveCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        PokeSlaveCommandStub::Errors execute() override
        {
            TRACE_LOG("[PokeSlaveCommand].start() - Num(" << mRequest.getNum() << "), Text(" << mRequest.getText() 
                      << "), Sleep(" << mRequest.getSleepMs() << " milli-seconds)");

            BlazeRpcError errorCode;

            // IF a sleep was requested
            if (mRequest.getSleepMs() > 0)
            {
                errorCode = gSelector->sleep(mRequest.getSleepMs() * 1000);
                if (errorCode != Blaze::ERR_OK)
                {
                    ERR_LOG("[PokeSlaveCommand].execute() Got sleep error " << (ErrorHelp::getErrorName(errorCode)) << "!");
                    return commandErrorFromBlazeError(errorCode);
                }
            }
            
            // Response
            if ((errorCode = mComponent->getMaster()->pokeMaster(mRequest, mMasterResponse, mMasterErrorResponse)) == Blaze::ERR_OK)
            {
                // Return what the master sent.
                mResponse.setMessage(mMasterResponse.getMessage());
                mResponse.getBitfield().setField(3);
                mResponse.getBitfield().setFlag();
                mResponse.setRegularEnum(ArsonResponse::ARSON_ENUM_ARG_2);

                size_t blobSize = mRequest.getReturnedBlobSize();
                if (blobSize > 0)
                {
                    uint8_t* mem = BLAZE_NEW_ARRAY(uint8_t, blobSize);
                    for (size_t counter = 0; counter < blobSize; ++counter)
                    {
                        mem[counter] = (uint8_t) Random::getRandomNumber(255);
                    }                
                    mResponse.getBlob().setData(mem, blobSize);

                    for(int i = 0; i <3; i++)
                    {
                        mComponent->sendResponseNotificationToUserSession(gCurrentLocalUserSession, &mResponse);
                    }

                    delete [] mem;
                }

                if (mRequest.getGenerateEvent())
                {
                    gEventManager->submitEvent(ArsonSlave::EVENT_ARSONEVENT, mRequest);
                }
                return ERR_OK;
            }
            // Error.
            else
            {
                WARN_LOG("[PokeSlaveCommand].gotResponse() - Master replied with error message " << mMasterErrorResponse.getMessage());
                mErrorResponse.setMessage(mMasterErrorResponse.getMessage());
                return commandErrorFromBlazeError(errorCode);
            }
        }

    private:
        ArsonSlaveImpl* mComponent;  // memory owned by creator, don't free
        ArsonResponse mMasterResponse;
        ArsonError mMasterErrorResponse;

    };

    // static factory method impl
    DEFINE_POKESLAVE_CREATE()



} // Arson
} // Blaze
