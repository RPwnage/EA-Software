/*************************************************************************************************/
/*!
    \file   FifaStatsConnection.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIFA_STATS_CONNECTION_H
#define BLAZE_FIFA_STATS_CONNECTION_H

/*** Include files *******************************************************************************/
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"

#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/protocol/outboundhttpresult.h"

#include "fifastats/tdf/fifastatstypes.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
	namespace FifaStats
	{
		class FifaStatsConnection
		{
			public:
				FifaStatsConnection(const ConfigMap* configRoot);
				~FifaStatsConnection();

				HttpConnectionManagerPtr getHttpConnManager();

				uint32_t getMaxAttempts() { return mMaxAttempts; }

				void getApiVersion(char8_t* dstStr, size_t dstStrLen);
				void getContextId(char8_t* dstStr, size_t dstStrLen);
				void getContextIdFUT(char8_t* dstStr, size_t dstStrLen);
				void getContextIdPlayerHealth(char8_t* dstStr, size_t dstStrLen);
				void getTokenScope(char8_t* dstStr, size_t dstStrLen);

				void addUpdateStatsAPI(char8_t* dstStr, size_t dstStrLen, const char8_t* categoryId, ContextOverrideType contextOverrideType);

			private:
				uint32_t mMaxAttempts;

				eastl::string mApiVersion;
				eastl::string mContextId;
				eastl::string mContextIdFUT;
				eastl::string mContextIdPlayerHealth;
				eastl::string mTokenScope;

				eastl::string mApiUpdateStats;
		};

	} // FifaStats
} // Blaze

#endif // BLAZE_FIFA_STATS_CONNECTION_H

