/*************************************************************************************************/
/*!
    \file   echotdf_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/echotdf_stub.h"

namespace Blaze
{
namespace Arson
{
class EchoTdfCommand : public EchoTdfCommandStub
{
public:
    EchoTdfCommand(
        Message* message, EchoTdfRequestResponse* request, ArsonSlaveImpl* componentImpl)
        : EchoTdfCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~EchoTdfCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    EchoTdfCommandStub::Errors execute() override
    {
        EA::TDF::Tdf* tdf = mRequest.getTdf();
        if (tdf != nullptr)
        {
            EA::TDF::Tdf* newTdf = tdf->clone();
            mResponse.setTdf(*newTdf);
        }

        mRequest.getEchoList().copyInto(mResponse.getEchoList());
        mRequest.getVariableList().copyInto(mResponse.getVariableList());
        mRequest.getVariableMap().copyInto(mResponse.getVariableMap());

        return ERR_OK;
    }
};

DEFINE_ECHOTDF_CREATE()

} //Arson
} //Blaze
