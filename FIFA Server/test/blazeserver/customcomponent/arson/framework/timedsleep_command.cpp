/*************************************************************************************************/
/*!
    \file   echo_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class TimedSleepCommand

    Copies the request TDF directly into the Response.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/timedsleep_stub.h"
#include "framework/util/random.h"


namespace Blaze
{
namespace Arson
{

    class TimedSleepCommand : public TimedSleepCommandStub
    {
    public:
        TimedSleepCommand(Message* message, TimedSleepRequestResponse* request, ArsonSlaveImpl* componentImpl)
            : TimedSleepCommandStub(message, request), mComponent(componentImpl)
        {
        }

        ~TimedSleepCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        ArsonSlaveImpl* mComponent;  // memory owned by creator, don't free

        TimedSleepCommandStub::Errors execute() override
        {
            TRACE_LOG("[ArsonSlaveImpl::LongWaitCommand].execute() : Going to wait " <<  mRequest.getWaitSecs() << "seconds.");
            if (ERR_OK != Fiber::sleep(mRequest.getWaitSecs() * 1000 * 1000, "Sleeping TimedSleepCommand::execute"))
                return ERR_SYSTEM;
            
            mRequest.copyInto(mResponse);

             size_t blobSize = mRequest.getReturnedBlobSize();
            if (blobSize > 0)
            {
                uint8_t* mem = BLAZE_NEW_ARRAY(uint8_t, blobSize);
                for (size_t counter = 0; counter < blobSize; ++counter)
                {
                    mem[counter] = (uint8_t) Random::getRandomNumber(255);
                }                
                mResponse.getBlob().setData(mem, blobSize);               

                delete [] mem;
            }

            return ERR_OK;
        }
    };

    // static factory method impl
    DEFINE_TIMEDSLEEP_CREATE()

} // Arson
} // Blaze
