/*************************************************************************************************/
/*!
    \file   XmsHdOperations.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_XMSHD_OPERATIONS_H
#define BLAZE_XMSHD_OPERATIONS_H

/*** Include files *******************************************************************************/
/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
	namespace XmsHd
	{
		class XmsHdConnection;
		class PublishDataRequest;

		// Create Media
		BlazeRpcError CreateMedia(XmsHdConnection& connection, PublishDataRequest& request);

		// Update Media
		BlazeRpcError UpdateMedia(XmsHdConnection& connection, PublishDataRequest& request);

		// File Status
		BlazeRpcError DoesFileExist(XmsHdConnection& connection, PublishDataRequest& request);

	} // XmsHd
} // Blaze

#endif // BLAZE_XMSHD_OPERATIONS_H

