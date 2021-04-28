/*************************************************************************************************/
/*!
    \file   FifaGroupsOperations.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIFA_STATS_OPERATIONS_H
#define BLAZE_FIFA_STATS_OPERATIONS_H

/*** Include files *******************************************************************************/
/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
	namespace FifaStats
	{
		class FifaStatsConnection;
		class UpdateStatsRequest;
		class UpdateStatsResponse;

		BlazeRpcError UpdateStatsOperation(FifaStatsConnection& connection, UpdateStatsRequest& request, UpdateStatsResponse& response);
		char8_t* ConstructUpdateStatsBody(UpdateStatsRequest& request, size_t& outBodySize);

		const char* const JSON_UPDATE_ID_TAG	= "updateId";
		const char* const JSON_ENTITIES_TAG		= "entities";
		const char* const JSON_ENTITY_ID_TAG	= "entityId";
		const char* const JSON_STATS_TAG		= "stats";
		const char* const JSON_STATS_VALUE_TAG	= "value";
		const char* const JSON_STATS_OP_TAG		= "operator";

		const char* const JSON_STATS_OP_SUM_TAG			= "OPERATION_SUM";
		const char* const JSON_STATS_OP_DECREMENT_TAG	= "OPERATION_DECREMENT";
		const char* const JSON_STATS_OP_MIN_TAG			= "OPERATION_MIN";
		const char* const JSON_STATS_OP_MAX_TAG			= "OPERATION_MAX";
		const char* const JSON_STATS_OP_REPLACE_TAG		= "OPERATION_REPLACE";

	} // FifaStats
} // Blaze

#endif // BLAZE_FIFA_STATS_OPERATIONS_H

