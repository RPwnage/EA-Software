/*************************************************************************************************/
/*!
    \file   clubsidentityprovider.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CLUBS_CLUBSIDENTITYPROVIDER_H
#define BLAZE_CLUBS_CLUBSIDENTITYPROVIDER_H

/*** Include files *******************************************************************************/
#include "framework/identity/identity.h"
#include "clubs/tdf/clubs.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Clubs
{

class ClubsSlaveImpl;

class ClubsIdentityProvider : public IdentityProvider
{
    NON_COPYABLE(ClubsIdentityProvider);

public:
    ClubsIdentityProvider(ClubsSlaveImpl* component);

    // IdentityProvider interface
    BlazeRpcError getIdentities(EA::TDF::ObjectType bobjType, const EntityIdList& keys, IdentityInfoByEntityIdMap& results) override;
    BlazeRpcError getIdentityByName(EA::TDF::ObjectType bobjType, const EntityName& entityName, IdentityInfo& result) override;

    void removeCacheEntry(EntityId key);

private:
    ClubsSlaveImpl* mComponent;
    static void identityCacheRemoveCb(const EntityId& id, ClubName* club);
    typedef LruCache<EntityId, ClubName*> IdentityCache;
    IdentityCache mCache;
};

} // Clubs
} // Blaze

#endif // BLAZE_CLUBS_CLUBSIDENTITYPROVIDER_H

