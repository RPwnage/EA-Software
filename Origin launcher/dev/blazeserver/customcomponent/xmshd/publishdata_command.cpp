/*************************************************************************************************/
/*!
    \file   publishdata_command.cpp

    $Header: //gosdev/games/FIFA/2014/Gen3/DEV/blazeserver/13.x/customcomponent/xmshd/publishdata_command.cpp#1 $

    \attention
    (c) Electronic Arts Inc. 2013
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class PublishDataCommand

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
#include "xmshdslaveimpl.h"
#include "xmshd/tdf/xmshdtypes.h"
#include "xmshd/rpc/xmshdslave/publishdata_stub.h"

#include "xmshdconnection.h"
#include "xmshdoperations.h"

namespace Blaze
{
    namespace XmsHd
    {

        class PublishDataCommand : public PublishDataCommandStub  
        {
        public:
            PublishDataCommand(Message* message, PublishDataRequest* request, XmsHdSlaveImpl* componentImpl)
                : PublishDataCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~PublishDataCommand() { }

        /* Private methods *******************************************************************************/
        private:

            PublishDataCommand::Errors execute()
            {
				TRACE_LOG("[PublishDataCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				// Get the XMS HD config and create an outbound HTTP connection.
				const ConfigMap* config = mComponent->getComponentConfig();
				XmsHdConnection connection(config);

				error = DoesFileExist(connection, mRequest);

				if ((error == Blaze::XMSHD_ERR_FILE_STATUS_FOUND) || (error == Blaze::XMSHD_ERR_FILE_STATUS_NOTFOUND))
				{
					if (error == Blaze::XMSHD_ERR_FILE_STATUS_FOUND)
					{
						TRACE_LOG("[PublishDataCommand] UpdateMedia");
						error = UpdateMedia(connection, mRequest);
					}
					else if (error == Blaze::XMSHD_ERR_FILE_STATUS_NOTFOUND)
					{
						TRACE_LOG("[PublishDataCommand] CreateMedia");
						error = CreateMedia(connection, mRequest);
					}
				}

                return PublishDataCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            XmsHdSlaveImpl* mComponent;
        };


        // static factory method impl
        PublishDataCommandStub* PublishDataCommandStub::create(Message* message, PublishDataRequest* request, XmsHdSlave* componentImpl)
        {
            return BLAZE_NEW PublishDataCommand(message, request, static_cast<XmsHdSlaveImpl*>(componentImpl));
        }

    } // XmsHd
} // Blaze
