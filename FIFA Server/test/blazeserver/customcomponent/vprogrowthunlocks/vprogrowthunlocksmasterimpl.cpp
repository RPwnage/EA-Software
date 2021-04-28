/*************************************************************************************************/
/*!
    \file   vprogrowthunlocksmasterimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class VProGrowthUnlocksMasterImpl

    VProGrowthUnlocks Master implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocksmasterimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

// vprogrowthunlocks includes
#include "vprogrowthunlocks/tdf/vprogrowthunlockstypes.h"

namespace Blaze
{
namespace VProGrowthUnlocks
{

// static
VProGrowthUnlocksMaster* VProGrowthUnlocksMaster::createImpl()
{
    return BLAZE_NEW_NAMED("VProGrowthUnlocksMasterImpl") VProGrowthUnlocksMasterImpl();
}

/*** Public Methods ******************************************************************************/
VProGrowthUnlocksMasterImpl::VProGrowthUnlocksMasterImpl():
	mDbId(DbScheduler::INVALID_DB_ID)
{
}

VProGrowthUnlocksMasterImpl::~VProGrowthUnlocksMasterImpl()
{
}

bool VProGrowthUnlocksMasterImpl::onConfigure()
{
    TRACE_LOG("[VProGrowthUnlocksMasterImpl:" << this << "].configure: configuring component.");

    return true;
}

PokeMasterError::Error VProGrowthUnlocksMasterImpl::processPokeMaster(
    const VProGrowthUnlocksRequest &request, VProGrowthUnlocksResponse &response, const Message *message)
{
    TRACE_LOG("[VProGrowthUnlocksMasterImpl:" << this << "].start() - Num(" << request.getNum() << "), Text(" << request.getText() << ")");

    // VProGrowthUnlocks check for failure.
    if (request.getNum() >= 0)
    {
        INFO_LOG("[VProGrowthUnlocksMasterImpl:" << this << "].start() - Respond Success.");

        response.setRegularEnum(VProGrowthUnlocksResponse::VPROGROWTHUNLOCKS_ENUM_SUCCESS);
        char pokeMessage[1024] = "";
        blaze_snzprintf(pokeMessage, sizeof(pokeMessage), "Master got poked: %d, %s", request.getNum(), request.getText());
        response.setMessage(pokeMessage);
        VProGrowthUnlocksResponse::IntList* l = &response.getMyList();
        l->push_back(1);
        l->push_back(2);
        l->push_back(3);
        l->push_back(4);
        return PokeMasterError::ERR_OK;
    }
    else
    {
        WARN_LOG("[VProGrowthUnlocksMasterImpl:" << this << "].start() - Respond Failure");        
        return PokeMasterError::VPROGROWTHUNLOCKS_ERR_UNKNOWN;
    } // if
}

} // VProGrowthUnlocks
} // Blaze
