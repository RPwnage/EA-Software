/*************************************************************************************************/
/*!
\file   filepetition_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class FilePetitionCommand

Pokes the slave.

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "gpscontentcontroller/rpc/gpscontentcontrollerslave/filepetition_stub.h"

// global includes


// gpscontentcontroller includes
#include "gpscontentcontrollerslaveimpl.h"
#include "gpscontentcontroller/tdf/gpscontentcontrollertypes.h"

namespace Blaze
{
namespace GpsContentController
{

class FilePetitionCommand : public FilePetitionCommandStub
{
public:
    FilePetitionCommand(Message* message, FilePetitionRequest* request, GpsContentControllerSlaveImpl* componentImpl)
        : FilePetitionCommandStub(message, request), mComponent(componentImpl)
    { }

    ~FilePetitionCommand() override { }

/* Private methods *******************************************************************************/
private:
    FilePetitionCommandStub::Errors execute() override
    {
        TRACE_LOG("[FilePetitionCommand].execute()");

        return commandErrorFromBlazeError(mComponent->filePetitionHttpRequest(mRequest, mResponse)); 
    }

private:
    // Not owned memory.
    GpsContentControllerSlaveImpl* mComponent;

};
// static factory method impl
DEFINE_FILEPETITION_CREATE()

} // GpsContentController
} // Blaze
