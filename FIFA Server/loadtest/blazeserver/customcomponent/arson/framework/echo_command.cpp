/*************************************************************************************************/
/*!
    \file   echo_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class EchoCommand

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
#include "arson/rpc/arsonslave/echo_stub.h"


namespace Blaze
{
namespace Arson
{

    class EchoCommand : public EchoCommandStub
    {
    public:
        EchoCommand(Message* message, MyTest* request, ArsonSlaveImpl* componentImpl)
            : EchoCommandStub(message, request), mComponent(componentImpl)
        {
        }

        ~EchoCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        ArsonSlaveImpl* mComponent;  // memory owned by creator, don't free

        EchoCommandStub::Errors execute() override
        {
            TRACE_LOG("[EchoCommand].start()");
            
            mRequest.copyInto(mResponse);

            return ERR_OK;
        }
    };

    // static factory method impl
    DEFINE_ECHO_CREATE()


} // Arson
} // Blaze
