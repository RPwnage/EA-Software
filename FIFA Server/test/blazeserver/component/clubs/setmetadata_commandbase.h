/*************************************************************************************************/
/*!
    \file   setmetadata_commandbase.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SetMetadataCommandBase

    Abstract class that defines the bulk of functionality for SetMetadataCommand/SetMetadata2Command classes.

    \note
        Implements the common methods. Subclasses(SetMetadataCommand/SetMetadata2Command) need
        to simply call the functions.
*/
/*************************************************************************************************/

#ifndef BLAZE_SETMETADATA_COMMANDBASE_H
#define BLAZE_SETMETADATA_COMMANDBASE_H

/*** Include Files *******************************************************************************/

// clubs includes
#include "clubscommandbase.h"
#include "clubs/clubsdb.h"

namespace Blaze
{
namespace Clubs
{

    class SetMetadataCommandBase : public ClubsCommandBase
    {
    public:
        ~SetMetadataCommandBase() override
        {
        }

    protected:

        // This class is supposed to be used only as a base for subclasses.
        SetMetadataCommandBase(ClubsSlaveImpl* componentImpl, const char8_t* cmdName);

        // set metadata or metadata2 for a club.
        BlazeRpcError setMetadata(const SetMetadataRequest& request, ClubMetadataUpdateType updateType);

    private:
        const char8_t*       mCmdName;
    };

} // Clubs
} // Blaze

#endif // BLAZE_SETMETADATA_COMMANDBASE_H
