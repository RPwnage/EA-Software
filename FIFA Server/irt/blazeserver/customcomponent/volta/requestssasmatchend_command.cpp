/*************************************************************************************************/
/*!
    \file   requestssasmatchend_command.cpp

    \attention
    (c) Electronic Arts Inc. 2019
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"

// volta includes
#include "voltaslaveimpl.h"
#include "volta/tdf/voltatypes.h"
#include "volta/rpc/voltaslave/requestssasmatchend_stub.h"

#include "ssasconnection.h"

namespace Blaze
{
    namespace Volta
    {

        class RequestSSASMatchEndCommand
			: public RequestSSASMatchEndCommandStub
        {
        public:
			RequestSSASMatchEndCommand(Message* message, SSASMatchEndRequest* request, VoltaSlaveImpl* componentImpl)
                : RequestSSASMatchEndCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~RequestSSASMatchEndCommand() { }

        /* Private methods *******************************************************************************/
        private:

			RequestSSASMatchEndCommand::Errors execute()
            {
				TRACE_LOG("[RequestSSASMatchEndCommand].execute()");

				// Get the Volta config and create an outbound HTTP connection.
				const VoltaConfig& voltaConfig = mComponent->getConfig();
				const SSASConfig& ssasConfig = voltaConfig.getSSASConfig();
				SSASConnection ssasConnection(ssasConfig);

				Blaze::BlazeRpcError error = ssasConnection.RequestMatchEnd(mRequest);

                return RequestSSASMatchEndCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VoltaSlaveImpl* mComponent;
        };


        // static factory method impl
        RequestSSASMatchEndCommandStub* RequestSSASMatchEndCommandStub::create(Message* message, SSASMatchEndRequest* request, VoltaSlave* componentImpl)
        {
            return BLAZE_NEW RequestSSASMatchEndCommand(message, request, static_cast<VoltaSlaveImpl*>(componentImpl));
        }

    } // Volta
} // Blaze
