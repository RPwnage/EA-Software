/*************************************************************************************************/
/*!
    \file

    Common header file used by the Blaze framework and all Blaze components and Blaze SDK.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_BLAZEOBJECTUTIL_H
#define BLAZE_BLAZEOBJECTUTIL_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#include "BlazeSDK/blazeobject.h"

namespace Blaze
{

/*************************************************************************************************/
/* DEPRECATED
 *
 * The following functions remain only for backward compatiblity.  With the ObjectType and
 * ObjectId types now being first-class structure there should be no need to use these
 * functions.
 *
 * These functions are compiled out on the server code as all the usages of them have been
 * changed to use the ObjectId and ObjectType classes directly.
 */
/*************************************************************************************************/

inline EA::TDF::ObjectId getBlazeObjectId(ComponentId componentId, EntityType entityType, EntityId entityId)
{
    return EA::TDF::ObjectId(componentId, entityType, entityId);
}

inline EA::TDF::ObjectId getBlazeObjectId(EA::TDF::ObjectType bobjType, EntityId entityId)
{
    return EA::TDF::ObjectId(bobjType, entityId);
}

inline EA::TDF::ObjectType getBlazeObjectType(ComponentId componentId, EntityType entityType)
{
    return EA::TDF::ObjectType(componentId, entityType);
}

inline EA::TDF::ObjectType getBlazeObjectType(EA::TDF::ObjectId bobjId)
{
    return bobjId.type;
}

inline ComponentId getComponentId(EA::TDF::ObjectId bobjId)
{
    return bobjId.type.component;
}

inline EntityType getEntityType(EA::TDF::ObjectId bobjId)
{
    return bobjId.type.type;
}

inline EntityId getEntityId(EA::TDF::ObjectId bobjId)
{
    return bobjId.id;
}

inline ComponentId getComponentId(EA::TDF::ObjectType bobjType)
{
    return bobjType.component;
}

inline EntityType getEntityType(EA::TDF::ObjectType bobjType)
{
    return bobjType.type;
}

} // Blaze

#endif // BLAZE_BLAZEOBJECTUTIL_H
