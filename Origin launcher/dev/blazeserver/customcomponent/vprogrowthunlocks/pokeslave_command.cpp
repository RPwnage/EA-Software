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
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/poke_stub.h"

// global includes

// vprogrowthunlocks includes
#include "vprogrowthunlocks/rpc/vprogrowthunlocksmaster.h"
#include "vprogrowthunlocksslaveimpl.h"
#include "vprogrowthunlocks/tdf/vprogrowthunlockstypes.h"

namespace Blaze
{
    namespace VProGrowthUnlocks
    {

        class PokeCommand : public PokeCommandStub
        {
        public:
            PokeCommand(Message* message, VProGrowthUnlocksRequest* request, VProGrowthUnlocksSlaveImpl* componentImpl)
                : PokeCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~PokeCommand() { }

            /* Private methods *******************************************************************************/
        private:

            PokeCommandStub::Errors execute()
            {
                TRACE_LOG("[PokeCommand:" << this << "].execute() - Num(" << mRequest.getNum() << "), Text(" << mRequest.getText() << ")");

                TRACE_LOG("[PokeCommand:" << this << "].execute() - " << mRequest);

                VProGrowthUnlocksMaster* master = mComponent->getMaster();
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
            VProGrowthUnlocksSlaveImpl* mComponent;
        };


        // static factory method impl
        DEFINE_POKE_CREATE()

    } // VProGrowthUnlocks
} // Blaze
