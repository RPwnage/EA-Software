/*************************************************************************************************/
/*!
    \file   coopseasonmasterimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class CoopSeasonMasterImpl

    Coop Season Master implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "coopseasonmasterimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

// coopseason includes
#include "coopseason/tdf/coopseasontypes.h"

namespace Blaze
{
namespace CoopSeason
{

// static
CoopSeasonMaster* CoopSeasonMaster::createImpl()
{
    return BLAZE_NEW_NAMED("CoopSeasonMasterImpl") CoopSeasonMasterImpl();
}

/*** Public Methods ******************************************************************************/
CoopSeasonMasterImpl::CoopSeasonMasterImpl():
	mDbId(DbScheduler::INVALID_DB_ID)
{
}

CoopSeasonMasterImpl::~CoopSeasonMasterImpl()
{
}

bool CoopSeasonMasterImpl::onConfigure()
{
    TRACE_LOG("[CoopSeasonMasterImpl:" << this << "].configure: configuring component.");

    return true;
}

PokeMasterError::Error CoopSeasonMasterImpl::processPokeMaster(
    const CoopSeasonRequest &request, CoopSeasonResponse &response, const Message *message)
{
	TRACE_LOG("[CoopSeasonMasterImpl:" << this << "].start() - Num(" << request.getNum() << "), Text(" << request.getText() << ")");

    // CoopSeason check for failure.
	if (request.getNum() >= 0)
	{
		INFO_LOG("[CoopSeasonMasterImpl:" << this << "].start() - Respond Success.");

        response.setRegularEnum(CoopSeasonResponse::COOPSEASON_ENUM_SUCCESS);
		char pokeMessage[1024] = "";
		blaze_snzprintf(pokeMessage, sizeof(pokeMessage), "Master got poked: %d, %s", request.getNum(), request.getText());
        response.setMessage(pokeMessage);
        CoopSeasonResponse::IntList* l = &response.getMyList();
        l->push_back(1);
        l->push_back(2);
        l->push_back(3);
        l->push_back(4);
		return PokeMasterError::ERR_OK;
	}
	else
	{
		WARN_LOG("[CoopSeasonMasterImpl:" << this << "].start() - Respond Failure");        
		return PokeMasterError::COOPSEASON_ERR_GENERAL;
	} // if
}

} // CoopSeason
} // Blaze
