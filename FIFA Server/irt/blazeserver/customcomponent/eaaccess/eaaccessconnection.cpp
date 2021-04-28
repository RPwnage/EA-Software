/*************************************************************************************************/
/*!
    \file   EaAccessConnection.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class EaAccessConnection

    Creates an HTTP connection manager to the EA Access system.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/util/xmlbuffer.h"

#include "eaaccessconnection.h"

// xms includes
#include "eaaccess/rpc/eaaccessmaster.h"
#include "eaaccess/tdf/eaaccesstypes.h"

namespace Blaze
{
	namespace EaAccess
	{
		/*** Public Methods ******************************************************************************/
		//=======================================================================================
		EaAccessConnection::EaAccessConnection(const ConfigMap *configRoot)
		{
			const ConfigMap* eaAccessData = configRoot->getMap("eaaccess");

			mBaseUrl = eaAccessData->getString("eaaccess_urlbase", "/proxy/identity/personas");

			mProductId = eaAccessData->getString("eaaccess_productid", "f4fd26b2-6f93-4ff4-b811-b04804866acb");
			mPrefixUrl = eaAccessData->getString("eaaccess_urlprefix", "/proxy/identity/personas");
			mSuffixUrl = eaAccessData->getString("eaaccess_urlsuffix", "/xblinventory");
		}

		//=======================================================================================
		EaAccessConnection::~EaAccessConnection()
		{
		}

		//=======================================================================================
		HttpConnectionManagerPtr EaAccessConnection::getConn()
		{
			// The string passed to getConnection must match the key of the entry you add to the httpServiceConfig.httpServices map in framework.cfg
			return gOutboundHttpService->getConnection("eaaccess");
		}

		//=======================================================================================
		void EaAccessConnection::addBaseUrl(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_strnzcat(dstStr, mBaseUrl.c_str(), dstStrLen);
		}

		//=======================================================================================
		void EaAccessConnection::addSuffixUrl(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_strnzcat(dstStr, mSuffixUrl.c_str(), dstStrLen);
		}

		//=======================================================================================
		void EaAccessConnection::addPersonaIdUrl(char8_t* dstStr, size_t dstStrLen, uint64_t personaId)
		{
			const int personaIdParamSize = 64;
			char8_t personaIdParam[personaIdParamSize];
			blaze_snzprintf(personaIdParam, personaIdParamSize, "/%" PRId64 "", personaId);

			blaze_strnzcat(dstStr, personaIdParam, dstStrLen);
		}

		//=======================================================================================
		void EaAccessConnection::addHttpParamAppendOp(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_strnzcat(dstStr, "&", dstStrLen);
		}

		//=======================================================================================
		void EaAccessConnection::addHttpParam_ProductId(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_strnzcat(dstStr, "productId=", dstStrLen);
			blaze_strnzcat(dstStr, mProductId.c_str(), dstStrLen);
		}

		//=======================================================================================

	} // EaAccess
} // Blaze
