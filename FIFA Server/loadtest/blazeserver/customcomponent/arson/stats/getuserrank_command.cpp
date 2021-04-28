/*************************************************************************************************/
/*!
    \file   reconfigure.cpp

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
#include "arson/rpc/arsonslave/getuserrank_stub.h"
#include "component/stats/rpc/statsslave_stub.h"
#include "stats/statsslaveimpl.h"

namespace Blaze
{
namespace Arson
{
class GetUserRankCommand : public GetUserRankCommandStub
{
public:
    GetUserRankCommand(Message* message, GetUserRankRequest* request, ArsonSlaveImpl* componentImpl)
        : GetUserRankCommandStub(message, request),
        mComponent(componentImpl)
    {
        mStatsComponent = static_cast<Blaze::Stats::StatsSlaveImpl*>(
            gController->getComponent(Stats::StatsSlave::COMPONENT_ID));
    }

    ~GetUserRankCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl *mComponent;
    Blaze::Stats::StatsSlaveImpl *mStatsComponent;
    
    GetUserRankCommandStub::Errors execute() override
    {
        int32_t rank = mStatsComponent->getUserRank(mRequest.getUserId(), mRequest.getName());
        
        if (rank >= 0 )
        {
            mResponse.setUserRank(rank);
        }
        return ERR_OK;
    }
};
    
DEFINE_GETUSERRANK_CREATE()


} //Arson
} //Blaze
