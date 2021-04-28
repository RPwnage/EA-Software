/*************************************************************************************************/
/*!
    \file   showcontent_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ShowContentCommand

    show petitionable content.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/petitionablecontent/petitionablecontent.h"
#include "framework/rpc/petitionablecontentslave/showcontent_stub.h"
namespace Blaze
{
    class ShowContentCommand : public ShowContentCommandStub
    {
    public:

        ShowContentCommand(Message* message, Blaze::ShowContentRequest* request, PetitionableContentManager* componentImpl)
            : ShowContentCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~ShowContentCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        ShowContentCommandStub::Errors execute() override
        {
            return commandErrorFromBlazeError(mComponent->showLocalContent(mRequest.getBlazeObjectId(), mRequest.getVisible()));
        }

    private:
        PetitionableContentManager* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_SHOWCONTENT_CREATE_COMPNAME(PetitionableContentManager)

} // Blaze
