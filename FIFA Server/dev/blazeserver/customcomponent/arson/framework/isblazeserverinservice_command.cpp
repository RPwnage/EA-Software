/*************************************************************************************************/
/*!
    \file   waituntilblazeserverinservice_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class IsBlazeServerInServiceCommand

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
#include "arson/rpc/arsonslave/isblazeserverinservice_stub.h"


namespace Blaze
{
namespace Arson
{

    class IsBlazeServerInServiceCommand : public IsBlazeServerInServiceCommandStub
    {
    public:
        IsBlazeServerInServiceCommand(Message* message, ArsonSlaveImpl* componentImpl)
            : IsBlazeServerInServiceCommandStub(message), mComponent(componentImpl)
        {
        }

        ~IsBlazeServerInServiceCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        ArsonSlaveImpl* mComponent;  // memory owned by creator, don't free

        IsBlazeServerInServiceCommandStub::Errors execute() override
        {
            TRACE_LOG("[IsBlazeServerInServiceCommand].start()");
            mResponse.setIsInService(gController->isInService());
            return ERR_OK;            
        }
    };

    // static factory method impl
    DEFINE_ISBLAZESERVERINSERVICE_CREATE()


} // Arson
} // Blaze
