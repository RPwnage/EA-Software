/*************************************************************************************************/
/*!
    \file   FifaGroupsConnection.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FifaStatsConnection

    Creates an HTTP connection manager to the Fifa Stats system.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/util/xmlbuffer.h"
#include "framework/connection/outboundhttpservice.h"
#include "fifastatsconnection.h"

#include "fifastats/rpc/fifastatsmaster.h"
#include "fifastats/tdf/fifastatstypes.h"
          
namespace Blaze
{
	namespace FifaStats
	{
		FifaStatsConnection::FifaStatsConnection(const ConfigMap *configRoot)
		{
			const ConfigMap* configData = configRoot->getMap("fifastats");

			mMaxAttempts = configData->getUInt32("maxAttempts", 3);

			mContextId				= configData->getString("contextId", "");
			mContextIdFUT			= configData->getString("contextIdFUT", "");
			mContextIdPlayerHealth	= configData->getString("contextIdPlayerHealth", "");
			mApiVersion				= configData->getString("apiVersion", "");
			mTokenScope				= configData->getString("tokenScope", "");

			mApiUpdateStats			= configData->getString("apiUpdateStats", "");
		}

		//=======================================================================================
		FifaStatsConnection::~FifaStatsConnection()
		{
		}

		//=======================================================================================
		HttpConnectionManagerPtr FifaStatsConnection::getHttpConnManager()
		{
			return gOutboundHttpService->getConnection("fifastats");
		}

		void FifaStatsConnection::getApiVersion(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_snzprintf(dstStr, dstStrLen, "%s", mApiVersion.c_str());
		}

		void FifaStatsConnection::getContextId(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_snzprintf(dstStr, dstStrLen, "%s", mContextId.c_str());
		}

		void FifaStatsConnection::getContextIdFUT(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_snzprintf(dstStr, dstStrLen, "%s", mContextIdFUT.c_str());
		}

		void FifaStatsConnection::getContextIdPlayerHealth(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_snzprintf(dstStr, dstStrLen, "%s", mContextIdPlayerHealth.c_str());
		}

		void FifaStatsConnection::getTokenScope(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_snzprintf(dstStr, dstStrLen, "%s", mTokenScope.c_str());
		}

		void FifaStatsConnection::addUpdateStatsAPI(char8_t* dstStr, size_t dstStrLen, const char8_t* categoryId, ContextOverrideType contextOverrideType)
		{
			eastl::string temp;

			eastl::string& contextId = mContextId;  // default
			switch (contextOverrideType)
			{
				case CONTEXT_OVERRIDE_TYPE_FUT:
					contextId = mContextIdFUT;
					break;
				case CONTEXT_OVERRIDE_TYPE_PLAYER_HEALTH:
					contextId = mContextIdPlayerHealth;
					break;
				default:
					break;
			}
			
			temp.sprintf(mApiUpdateStats.c_str(), mApiVersion.c_str(), contextId.c_str(), categoryId);

			blaze_strnzcat(dstStr, temp.c_str(), dstStrLen);
		}


		//=======================================================================================

	} // FifaStats
} // Blaze
