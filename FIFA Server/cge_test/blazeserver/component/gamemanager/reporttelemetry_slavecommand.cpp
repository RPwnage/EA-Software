/*! ************************************************************************************************/
/*!
    \file reporttelemetry_slavecommands.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/reporttelemetry_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"

#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief Process telemetry report
    ***************************************************************************************************/
    class ReportTelemetryCommand : public ReportTelemetryCommandStub
    {
    public:
        ReportTelemetryCommand(Message* message, TelemetryReportRequest* request, GameManagerSlaveImpl* componentImpl)
            : ReportTelemetryCommandStub(message, request),
              mComponent(*componentImpl)
        {
        }

        ~ReportTelemetryCommand() override {}

    private:
        
        ReportTelemetryCommandStub::Errors execute() override
        {
            if (mRequest.getLocalConnGroupId() == 0)
            {
                // something went wrong, early out
                ERR_LOG("[ReportTelemetryCommand].execute: incoming telemetry report for game (" << mRequest.getGameId() << ") with topology (" << mRequest.getNetworkTopology() << ") had a localConnGroupId of (0).");
                return commandErrorFromBlazeError(ERR_SYSTEM);
            }

            ProcessTelemetryReportRequest req;
            req.setGameId(mRequest.getGameId());
            req.setBestPingSiteAlias(gCurrentLocalUserSession->getExtendedData().getBestPingSiteAlias());
            mRequest.copyInto(req.getReport());
            BlazeRpcError rc = mComponent.getMaster()->processTelemetryReport(req);
            return commandErrorFromBlazeError(rc);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    //static creation factory method of command's stub class
    DEFINE_REPORTTELEMETRY_CREATE()

} // namespace GameManager
} // namespace Blaze
