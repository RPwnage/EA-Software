/*************************************************************************************************/
/*!
\file   fileonlinereport_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class FileOnlineReportCommand

Pokes the slave.

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "contentreporter/rpc/contentreporterslave/fileonlinereport_stub.h"

// global includes


// contentreporter includes
#include "contentreporterslaveimpl.h"
#include "contentreporter/tdf/contentreportertypes.h"

namespace Blaze
{
namespace ContentReporter
{

class FileOnlineReportCommand : public FileOnlineReportCommandStub
{
public:
    FileOnlineReportCommand(Message* message, FileOnlineReportRequest* request, ContentReporterSlaveImpl* componentImpl)
        : FileOnlineReportCommandStub(message, request), mComponent(componentImpl)
    { }

    ~FileOnlineReportCommand() override { }

/* Private methods *******************************************************************************/
private:	
    FileOnlineReportCommandStub::Errors execute() override
    {
        TRACE_LOG("[FileOnlineReportCommand].execute()");

        return commandErrorFromBlazeError(mComponent->fileOnlineReportHttpRequest(mRequest, mResponse));
    }

private:
    // Not owned memory.
    ContentReporterSlaveImpl* mComponent;

};
// static factory method impl
DEFINE_FILEONLINEREPORT_CREATE()

} // ContentReporter
} // Blaze
