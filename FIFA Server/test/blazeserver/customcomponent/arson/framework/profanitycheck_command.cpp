/*************************************************************************************************/
/*!
    \file   profanitycheck.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/util/profanityfilter.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/profanitycheck_stub.h"

namespace Blaze
{
namespace Arson
{
class ProfanityCheckCommand : public ProfanityCheckCommandStub
{
public:
    ProfanityCheckCommand(Message* message, ProfanityCheckRequest* request, ArsonSlaveImpl* componentImpl)
        : ProfanityCheckCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~ProfanityCheckCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    ProfanityCheckCommandStub::Errors execute() override
    {
        // Call to internal command
        if (gProfanityFilter->isProfane(mRequest.getText()) != 0)
        {
            // Text is offensive return TRUE
            mResponse.setIsProfanity(true);
        }
        else
        {
            // Text is not offensive return FALSE
            mResponse.setIsProfanity(false);
        }

        return commandErrorFromBlazeError(Blaze::ERR_OK);
    }
};

DEFINE_PROFANITYCHECK_CREATE()


} //Arson
} //Blaze
