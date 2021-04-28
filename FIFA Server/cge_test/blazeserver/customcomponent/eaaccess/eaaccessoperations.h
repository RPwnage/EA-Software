/*************************************************************************************************/
/*!
    \file   EaAccessOperations.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_EAACCESS_OPERATIONS_H
#define BLAZE_EAACCESS_OPERATIONS_H

/*** Include files *******************************************************************************/
/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
	namespace EaAccess
	{
		class EaAccessConnection;
		class EaAccessSubInfoRequest;

		// Create Media
		BlazeRpcError XblInventory(XmsHdConnection& connection, EaAccessSubInfoRequest& request);

	} // EaAccess
} // Blaze

#endif // BLAZE_EAACCESS_OPERATIONS_H

