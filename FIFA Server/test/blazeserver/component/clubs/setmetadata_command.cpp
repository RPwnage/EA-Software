/*************************************************************************************************/
/*!
    \file   setmetadata_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SetMetadataCommand

    set metadata for a club or club member

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/setmetadata_stub.h"
#include "setmetadata_commandbase.h"

namespace Blaze
{
namespace Clubs
{

    class SetMetadataCommand : public SetMetadataCommandStub, private SetMetadataCommandBase
    {
    public:
        SetMetadataCommand(Message* message, SetMetadataRequest* request, ClubsSlaveImpl* componentImpl)
            : SetMetadataCommandStub(message, request),
            SetMetadataCommandBase(componentImpl, "SetMetadataCommand")
        {
        }

        ~SetMetadataCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        SetMetadataCommandStub::Errors execute() override
        {
            BlazeRpcError result = SetMetadataCommandBase::setMetadata(mRequest, CLUBS_METADATA_UPDATE);
            return commandErrorFromBlazeError(result);
        }
    };

    // static factory method impl
    DEFINE_SETMETADATA_CREATE()

} // Clubs
} // Blaze
