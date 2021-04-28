/*************************************************************************************************/
/*!
\file   filessfreport_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class FileSSFReportCommand

Pokes the slave.

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "contentreporter/rpc/contentreporterslave/filessfreport_stub.h"

// global includes


// contentreporter includes
#include "contentreporterslaveimpl.h"
#include "contentreporter/tdf/contentreportertypes.h"

namespace Blaze
{
namespace ContentReporter
{

class FileSSFReportCommand : public FileSSFReportCommandStub
{
public:
    FileSSFReportCommand(Message* message, FileSSFReportRequest* request, ContentReporterSlaveImpl* componentImpl)
        : FileSSFReportCommandStub(message, request), mComponent(componentImpl)
    { }

    ~FileSSFReportCommand() override { }

/* Private methods *******************************************************************************/
private:	
    FileSSFReportCommandStub::Errors execute() override
    {
        TRACE_LOG("[FileSSFReportCommand].execute()");

        return commandErrorFromBlazeError(mComponent->fileSSFReportHttpRequest(mRequest, mResponse));
    }

private:
    // Not owned memory.
    ContentReporterSlaveImpl* mComponent;

};
// static factory method impl
DEFINE_FILESSFREPORT_CREATE()

} // ContentReporter
} // Blaze
