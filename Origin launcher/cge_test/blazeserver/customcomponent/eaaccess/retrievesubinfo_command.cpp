/*************************************************************************************************/
/*!
    \file   retrievesubinfo_command.cpp

    $Header: //gosdev/games/FIFA/2015/Gen4/DEV/blazeserver/13.x/customcomponent/eaaccess/retrievesubinfo_command.cpp#1 $

    \attention
    (c) Electronic Arts Inc. 2014
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RetrieveSubInfoCommand

    Publish an asset to XMS HD

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"

// easfc includes
#include "eaaccessslaveimpl.h"
#include "eaaccess/tdf/eaaccesstypes.h"
#include "eaaccess/rpc/eaaccessslave/retrievesubinfo_stub.h"

#include "eaaccessconnection.h"

namespace Blaze
{
    namespace EaAccess
    {

        class RetrieveSubInfoCommand : public RetrieveSubInfoCommandStub  
        {
        public:
            RetrieveSubInfoCommand(Message* message, EaAccessSubInfoRequest* request, EaAccessSlaveImpl* componentImpl)
                : RetrieveSubInfoCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~RetrieveSubInfoCommand() { }

        /* Private methods *******************************************************************************/
        private:

            RetrieveSubInfoCommand::Errors execute()
            {
				TRACE_LOG("[RetrieveSubInfoCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				// Get the XMS HD config and create an outbound HTTP connection.
				const ConfigMap* config = mComponent->getComponentConfig();
				EaAccessConnection connection(config);

				TRACE_LOG("[RetrieveSubInfoCommand] XBL Inventory");
				error = XblInventory(connection, mRequest, mResponse);

                return RetrieveSubInfoCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            EaAccessSlaveImpl* mComponent;
        };


        // static factory method impl
        RetrieveSubInfoCommandStub* RetrieveSubInfoCommandStub::create(Message* message, EaAccessSubInfoRequest* request, EaAccessSlave* componentImpl)
        {
            return BLAZE_NEW RetrieveSubInfoCommand(message, request, static_cast<EaAccessSlaveImpl*>(componentImpl));
        }

    } // XmsHd
} // Blaze
