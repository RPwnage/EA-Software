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
#include "fifastats/rpc/fifastatsslave/poke_stub.h"

// global includes

// fifastats includes
#include "fifastats/rpc/fifastatsmaster.h"
#include "fifastats/tdf/fifastatstypes.h"
#include "fifastatsslaveimpl.h"


namespace Blaze
{
    namespace FifaStats
    {

        class PokeCommand : public PokeCommandStub
        {
        public:
            PokeCommand(Message* message, FifaStatsRequest* request, FifaStatsSlaveImpl* componentImpl)
                : PokeCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~PokeCommand() { }

            /* Private methods *******************************************************************************/
        private:

            PokeCommandStub::Errors execute()
            {
                TRACE_LOG("[PokeCommand:" << this << "].execute() - Num(" << mRequest.getNum() << "), Text(" << mRequest.getText() << ")");

                TRACE_LOG("[PokeCommand:" << this << "].execute() - " << mRequest);

                FifaStatsMaster* master = mComponent->getMaster();
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
            FifaStatsSlaveImpl* mComponent;
        };


        // static factory method impl
        DEFINE_POKE_CREATE()

    } // FifaStats
} // Blaze
