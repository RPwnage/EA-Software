/*************************************************************************************************/
/*!
\file   filefutreport_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class FileFUTReportCommand

Pokes the slave.

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "contentreporter/rpc/contentreporterslave/filefutreport_stub.h"

// global includes


// contentreporter includes
#include "contentreporterslaveimpl.h"
#include "contentreporter/tdf/contentreportertypes.h"

namespace Blaze
{
namespace ContentReporter
{

class FileFUTReportCommand : public FileFUTReportCommandStub
{
public:
    FileFUTReportCommand(Message* message, FileFUTReportRequest* request, ContentReporterSlaveImpl* componentImpl)
        : FileFUTReportCommandStub(message, request), mComponent(componentImpl)
    { }

    ~FileFUTReportCommand() override { }

/* Private methods *******************************************************************************/
private:	
    FileFUTReportCommandStub::Errors execute() override
    {
        TRACE_LOG("[FileFUTReportCommand].execute()");

        return commandErrorFromBlazeError(mComponent->fileFUTReportHttpRequest(mRequest, mResponse));
    }

private:
    // Not owned memory.
    ContentReporterSlaveImpl* mComponent;

};
// static factory method impl
DEFINE_FILEFUTREPORT_CREATE()

} // ContentReporter
} // Blaze
