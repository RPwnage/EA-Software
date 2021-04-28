/*************************************************************************************************/
/*!
\file   filereport_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class FileReportCommand

Pokes the slave.

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "contentreporter/rpc/contentreporterslave/filereport_stub.h"

// global includes


// contentreporter includes
#include "contentreporterslaveimpl.h"
#include "contentreporter/tdf/contentreportertypes.h"

namespace Blaze
{
namespace ContentReporter
{

class FileReportCommand : public FileReportCommandStub
{
public:
    FileReportCommand(Message* message, FileReportRequest* request, ContentReporterSlaveImpl* componentImpl)
        : FileReportCommandStub(message, request), mComponent(componentImpl)
    { }

    ~FileReportCommand() override { }

/* Private methods *******************************************************************************/
private:	
    FileReportCommandStub::Errors execute() override
    {
        TRACE_LOG("[FileReportCommand].execute()");

        return commandErrorFromBlazeError(mComponent->fileReportHttpRequest(mRequest, mResponse));
    }

private:
    // Not owned memory.
    ContentReporterSlaveImpl* mComponent;

};
// static factory method impl
DEFINE_FILEREPORT_CREATE()

} // ContentReporter
} // Blaze
