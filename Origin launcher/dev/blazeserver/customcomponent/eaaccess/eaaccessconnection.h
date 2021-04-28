/*************************************************************************************************/
/*!
    \file   EaAccessConnection.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_EAACCESS_CONNECTION_H
#define BLAZE_EAACCESS_CONNECTION_H

/*** Include files *******************************************************************************/
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"

#include "framework/connection/outboundhttpservice.h"
#include "framework/protocol/outboundhttpresult.h"

#include "eaaccess/tdf/eaaccesstypes.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
	namespace EaAccess
	{

		class EaAccessConnection
		{
			public:
				EaAccessConnection(const ConfigMap* xmsHdData);
				~EaAccessConnection();

				HttpConnectionManagerPtr getConn();

				void addBaseUrl(char8_t* dstStr, size_t dstStrLen);
				void addPrefixUrl(char8_t* dstStr, size_t dstStrLen);
				void addPersonaIdUrl(char8_t* dstStr, size_t dstStrLen, uint64_t personaId);
				void addSuffixUrl(char8_t* dstStr, size_t dstStrLen);

				void addHttpParamAppendOp(char8_t* dstStr, size_t dstStrLen);

				void addHttpParam_ProductId(char8_t* dstStr, size_t dstStrLen);

			private:
				eastl::string mBaseUrl;
				eastl::string mPrefixUrl;
				eastl::string mSuffixUrl;
				eastl::string mProductId;
		};

	} // EaAccess
} // Blaze

#endif // BLAZE_EAACCESS_CONNECTION_H

