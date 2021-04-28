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
#include "coopseason/rpc/coopseasonslave/poke_stub.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"

// coopseason includes
#include "coopseasonslaveimpl.h"
#include "coopseason/tdf/coopseasontypes.h"

namespace Blaze
{
    namespace CoopSeason
    {

        class PokeCommand : public PokeCommandStub
        {
        public:
            PokeCommand(Message* message, CoopSeasonRequest* request, CoopSeasonSlaveImpl* componentImpl)
                : PokeCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~PokeCommand() { }

            /* Private methods *******************************************************************************/
        private:

            PokeCommandStub::Errors execute()
            {
                TRACE_LOG("[PokeCommand:" << this << "].execute() - Num(" << mRequest.getNum() << "), Text(" << mRequest.getText() << ")");

                char8_t buf[4096];
                mRequest.print(buf,sizeof(buf));
                TRACE_LOG("[PokeCommand:" << this << "].execute() - " << buf);

                CoopSeasonMaster* master = mComponent->getMaster();
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
            CoopSeasonSlaveImpl* mComponent;
        };


        // static factory method impl
        DEFINE_POKE_CREATE()

    } // CoopSeason
} // Blaze
