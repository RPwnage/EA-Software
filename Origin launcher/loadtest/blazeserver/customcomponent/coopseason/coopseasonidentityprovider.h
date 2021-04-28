/*************************************************************************************************/
/*!
    \file   coopseasonidentityprovider.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_COOPSEASON_COOPSEASONIDENTITYPROVIDER_H
#define BLAZE_COOPSEASON_COOPSEASONIDENTITYPROVIDER_H

/*** Include files *******************************************************************************/
#include "framework/identity/identity.h"
#include "coopseason/tdf/coopseasontypes.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace CoopSeason
{

class CoopSeasonSlaveImpl;

class CoopSeasonIdentityProvider : public IdentityProvider
{
    NON_COPYABLE(CoopSeasonIdentityProvider);

public:
    CoopSeasonIdentityProvider(CoopSeasonSlaveImpl* component);

    // IdentityProvider interface
    virtual BlazeRpcError getIdentities(BlazeObjectType bobjType, const EntityIdList& keys, IdentityInfoByEntityIdMap& results);
    virtual BlazeRpcError getIdentityByName(BlazeObjectType bobjType, const EntityName& entityName, IdentityInfo& result);

    void removeCacheEntry(EntityId key);
    
private:
    CoopSeasonSlaveImpl* mComponent;
	static void identityCacheRemoveCb(const EntityId& id, CoopIdName* name);
    typedef LruCache<EntityId, CoopIdName*> IdentityCache;
    IdentityCache mCache;
};

} // CoopSeason
} // Blaze

#endif // BLAZE_COOPSEASON_COOPSEASONIDENTITYPROVIDER_H

