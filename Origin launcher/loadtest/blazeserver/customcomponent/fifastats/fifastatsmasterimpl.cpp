/*************************************************************************************************/
/*!
    \file   fifastatsmasterimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FifaStatsMasterImpl

    FifaStats Master implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifastatsmasterimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

// fifastats includes
#include "fifastats/tdf/fifastatstypes.h"

namespace Blaze
{
namespace FifaStats
{

// static
FifaStatsMaster* FifaStatsMaster::createImpl()
{
    return BLAZE_NEW_NAMED("FifaStatsMasterImpl") FifaStatsMasterImpl();
}

/*** Public Methods ******************************************************************************/
FifaStatsMasterImpl::FifaStatsMasterImpl()
{
}

FifaStatsMasterImpl::~FifaStatsMasterImpl()
{
}

bool FifaStatsMasterImpl::onConfigure()
{
    TRACE_LOG("[FifaStatsMasterImpl:" << this << "].configure: configuring component.");
	    return true;
}

PokeMasterError::Error FifaStatsMasterImpl::processPokeMaster(
    const FifaStatsRequest &request, FifaStatsResponse &response, const Message *message)
{
    TRACE_LOG("[FifaStatsMasterImpl:" << this << "].start() - Num(" << request.getNum() << "), Text(" << request.getText() << ")");

    // FifaStats check for failure.
    if (request.getNum() >= 0)
    {
        INFO_LOG("[FifaStatsMasterImpl:" << this << "].start() - Respond Success.");

        response.setRegularEnum(FifaStatsResponse::FIFASTATS_ENUM_SUCCESS);
        char pokeMessage[1024] = "";
        blaze_snzprintf(pokeMessage, sizeof(pokeMessage), "Master got poked: %d, %s", request.getNum(), request.getText());
        response.setMessage(pokeMessage);
        FifaStatsResponse::IntList* l = &response.getMyList();
        l->push_back(1);
        l->push_back(2);
        l->push_back(3);
        l->push_back(4);
        return PokeMasterError::ERR_OK;
    }
    else
    {
        WARN_LOG("[FifaStatsMasterImpl:" << this << "].start() - Respond Failure");        
        return PokeMasterError::FIFASTATS_ERR_UNKNOWN;
    } // if
}

} // FifaStats
} // Blaze
