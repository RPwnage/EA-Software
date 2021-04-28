/*************************************************************************************************/
/*!
\file


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_COLLECTIONS_H
#define BLAZE_COLLECTIONS_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/component/framework/tdf/attributes.h"
#else
#include "framework/tdf/attributes.h"
#endif

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace Collections
{

    /*! ****************************************************************************/
    /*! \brief Performs an upsert on the destination AttributeMap from
        source AttributeMap.

        \details For every item in the source AttributeMap this function inserts those items
        into the dest AttributeMap.  For any item that already exists that value is updated.

        \param[in] dest    The AttributeMap to be updated.
        \param[in] source  The source AttributeMap to update from.
    ********************************************************************************/
    BLAZESDK_API void upsertAttributeMap(AttributeMap& dest, const AttributeMap& source);

} // Collections

} // Blaze

#endif // BLAZE_COLLECTIONS_H

