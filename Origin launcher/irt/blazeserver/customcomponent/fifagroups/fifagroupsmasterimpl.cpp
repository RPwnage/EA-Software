/*************************************************************************************************/
/*!
    \file   fifagroupsmasterimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FifaGroupsMasterImpl

    FifaGroups Master implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifagroupsmasterimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

// fifagroups includes
#include "fifagroups/tdf/fifagroupstypes.h"

namespace Blaze
{
namespace FifaGroups
{

// static
FifaGroupsMaster* FifaGroupsMaster::createImpl()
{
    return BLAZE_NEW_NAMED("FifaGroupsMasterImpl") FifaGroupsMasterImpl();
}

/*** Public Methods ******************************************************************************/
FifaGroupsMasterImpl::FifaGroupsMasterImpl()
{
}

FifaGroupsMasterImpl::~FifaGroupsMasterImpl()
{
}

bool FifaGroupsMasterImpl::onConfigure()
{
    TRACE_LOG("[FifaGroupsMasterImpl:" << this << "].configure: configuring component.");

    return true;
}

PokeMasterError::Error FifaGroupsMasterImpl::processPokeMaster(
    const FifaGroupsRequest &request, FifaGroupsResponse &response, const Message *message)
{
    TRACE_LOG("[FifaGroupsMasterImpl:" << this << "].start() - Num(" << request.getNum() << "), Text(" << request.getText() << ")");

    // FifaGroups check for failure.
    if (request.getNum() >= 0)
    {
        INFO_LOG("[FifaGroupsMasterImpl:" << this << "].start() - Respond Success.");

        response.setRegularEnum(FifaGroupsResponse::FIFAGROUPS_ENUM_SUCCESS);
        char pokeMessage[1024] = "";
        blaze_snzprintf(pokeMessage, sizeof(pokeMessage), "Master got poked: %d, %s", request.getNum(), request.getText());
        response.setMessage(pokeMessage);
        FifaGroupsResponse::IntList* l = &response.getMyList();
        l->push_back(1);
        l->push_back(2);
        l->push_back(3);
        l->push_back(4);
        return PokeMasterError::ERR_OK;
    }
    else
    {
        WARN_LOG("[FifaGroupsMasterImpl:" << this << "].start() - Respond Failure");        
        return PokeMasterError::FIFAGROUPS_ERR_UNKNOWN;
    } // if
}

} // FifaGroups
} // Blaze
