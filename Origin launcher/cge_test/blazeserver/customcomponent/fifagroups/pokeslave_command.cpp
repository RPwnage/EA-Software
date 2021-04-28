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
// main include
#include "framework/blaze.h"
#include "fifagroups/rpc/fifagroupsslave/poke_stub.h"

// global includes

// fifagroups includes
#include "fifagroups/rpc/fifagroupsmaster.h"
#include "fifagroupsslaveimpl.h"
#include "fifagroups/tdf/fifagroupstypes.h"

namespace Blaze
{
    namespace FifaGroups
    {

        class PokeCommand : public PokeCommandStub
        {
        public:
            PokeCommand(Message* message, FifaGroupsRequest* request, FifaGroupsSlaveImpl* componentImpl)
                : PokeCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~PokeCommand() { }

            /* Private methods *******************************************************************************/
        private:

            PokeCommandStub::Errors execute()
            {
                TRACE_LOG("[PokeCommand:" << this << "].execute() - Num(" << mRequest.getNum() << "), Text(" << mRequest.getText() << ")");

                TRACE_LOG("[PokeCommand:" << this << "].execute() - " << mRequest);

                FifaGroupsMaster* master = mComponent->getMaster();
                BlazeRpcError errorCode = master->pokeMaster(mRequest, mResponse);

                TRACE_LOG("[PokeCommand:" << this << "].gotResponse()");

                // Response
                if (errorCode == Blaze::ERR_OK)
                {
                    // Return what the master sent.
                    return ERR_OK;
                }
                // Error.
                else
                {
                    WARN_LOG("[PokeCommand:" << this << "].gotResponse() - Master replied with error (" << (ErrorHelp::getErrorName(errorCode)) 
                             << "): " << (ErrorHelp::getErrorDescription(errorCode)));
                    mErrorResponse.setMessage(ErrorHelp::getErrorDescription(errorCode));
                    return commandErrorFromBlazeError(errorCode);
                }
            }

        private:
            // Not owned memory.
            FifaGroupsSlaveImpl* mComponent;
        };


        // static factory method impl
        DEFINE_POKE_CREATE()

    } // FifaGroups
} // Blaze
