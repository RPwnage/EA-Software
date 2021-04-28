#include "framework/blaze.h"
#include "arson/rpc/arsonslave/updatestats_stub.h"
#include "arson/rpc/arsonslave/deletestats_stub.h"
#include "arson/tdf/arson.h"
#include "framework/controller/controller.h"
#include "framework/connection/selector.h"
#include "framework/usersessions/usersession.h"

#include "stats/statsconfig.h"
#include "stats/statsslaveimpl.h"

#include "arsonslaveimpl.h"

namespace Blaze
{

namespace Arson
{

using namespace Stats;

class DeleteStatsCommand : public DeleteStatsCommandStub
{
public:
    DeleteStatsCommand(Message *message,  Blaze::Arson::DeleteStatsRequest *request, ArsonSlaveImpl* componentImp) : DeleteStatsCommandStub(message, request) {}

    DeleteStatsCommandStub::Errors execute() override
    {
        Stats::StatsSlaveImpl *component = (Stats::StatsSlaveImpl *) gController->getComponent(Stats::StatsSlave::COMPONENT_ID);

        Blaze::Stats::DeleteStatsRequest request;
        mRequest.getStatDeletes().copyInto(request.getStatDeletes());

        return (commandErrorFromBlazeError(component->deleteStats(request)));
    }
};

DEFINE_DELETESTATS_CREATE()

} // namespace Arson
} // namespace Blaze

