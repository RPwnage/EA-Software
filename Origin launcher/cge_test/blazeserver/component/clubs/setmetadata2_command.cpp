/*************************************************************************************************/
/*!
    \file   setmetadata2_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SetMetadata2Command

    set metadata2 for a club or club member

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/setmetadata2_stub.h"
#include "setmetadata_commandbase.h"

namespace Blaze
{
namespace Clubs
{

    class SetMetadata2Command : public SetMetadata2CommandStub, private SetMetadataCommandBase
    {
    public:
        SetMetadata2Command(Message* message, SetMetadataRequest* request, ClubsSlaveImpl* componentImpl)
            : SetMetadata2CommandStub(message, request),
            SetMetadataCommandBase(componentImpl, "SetMetadata2Command")
        {
        }

        ~SetMetadata2Command() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        SetMetadata2CommandStub::Errors execute() override
        {
            BlazeRpcError result = SetMetadataCommandBase::setMetadata(mRequest, CLUBS_METADATA_UPDATE_2);
            return commandErrorFromBlazeError(result);
        }
    };

    // static factory method impl
    DEFINE_SETMETADATA2_CREATE()

} // Clubs
} // Blaze
