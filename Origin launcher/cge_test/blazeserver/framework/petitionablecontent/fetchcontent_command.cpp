/*************************************************************************************************/
/*!
    \file   fetchcontent_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FetchContentCommand

    fetch petitionable content.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/petitionablecontent/petitionablecontent.h"
#include "framework/rpc/petitionablecontentslave/fetchcontent_stub.h"
namespace Blaze
{
    class FetchContentCommand : public FetchContentCommandStub
    {
    public:

        FetchContentCommand(Message* message, Blaze::FetchContentRequest* request, PetitionableContentManager* componentImpl)
            : FetchContentCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~FetchContentCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        FetchContentCommandStub::Errors execute() override
        {
            eastl::string url;
            BlazeRpcError err = mComponent->fetchLocalContent(mRequest.getBlazeObjectId(), mResponse.getAttributeMap(), url);
            mResponse.setUrl(url.c_str());
            return commandErrorFromBlazeError(err);
        }

    private:
        PetitionableContentManager* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_FETCHCONTENT_CREATE_COMPNAME(PetitionableContentManager)

} // Blaze
