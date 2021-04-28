/*************************************************************************************************/
/*!
    \file
        identity.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_IDENTITY_H
#define BLAZE_IDENTITY_H

/*** Include files *******************************************************************************/
#include "framework/tdf/identitytypes_server.h"
#include "framework/rpc/identityslave_stub.h"

#include "framework/util/lrucache.h"

namespace Blaze
{

typedef eastl::vector<EntityId> EntityIdList;

class IdentityProvider
{
public:
    virtual ~IdentityProvider() {}

    // required methods

    /* \brief Find the identity info for multiple keys (fiber) */
    virtual BlazeRpcError getIdentities(EA::TDF::ObjectType bobjType, const EntityIdList& keys, IdentityInfoByEntityIdMap& results) = 0;

    /* \brief Find the identity info for one name (fiber) */
    virtual BlazeRpcError getIdentityByName(EA::TDF::ObjectType bobjType, const EntityName& entityName, IdentityInfo& result) = 0;
};

class IdentityManager : public IdentitySlaveStub
{
    friend class GetIdentityCommand;
    friend class GetIdentitiesCommand;
    friend class GetIdentityByNameCommand;

    NON_COPYABLE(IdentityManager);

public:

    IdentityManager();
    ~IdentityManager() override;

    // \brief Find the identity info for one key.
    BlazeRpcError DEFINE_ASYNC_RET(getIdentity(EA::TDF::ObjectType bobjType, EntityId key, IdentityInfo& result));

    // \brief Find the identity info for multiple keys.
    BlazeRpcError DEFINE_ASYNC_RET(getIdentities(EA::TDF::ObjectType bobjType, const EntityIdList& keys, IdentityInfoByEntityIdMap& results));

    // \brief Find the identity info for one name.
    BlazeRpcError DEFINE_ASYNC_RET(getIdentityByName(EA::TDF::ObjectType bobjType, const EntityName& name, IdentityInfo& result));

    // \brief Register an Identity provider for the given *local* component. Memory is owned by the caller.
    bool registerProvider(ComponentId compId, IdentityProvider& provider);

    // \brief Deregister an Identity provider for the given *local* component.
    bool deregisterProvider(ComponentId compId);

private:

    // \brief Find all UserSessionIds matching EA::TDF::ObjectId.
    BlazeRpcError getLocalIdentity(EA::TDF::ObjectType bobjType, EntityId key, IdentityInfo& result);
    BlazeRpcError getLocalIdentities(EA::TDF::ObjectType bobjType, const EntityIdList& keys, IdentityInfoByEntityIdMap& results);
    BlazeRpcError getLocalIdentityByName(EA::TDF::ObjectType bobjType, const EntityName& name, IdentityInfo& result);

    InstanceId getRemoteIdentitySlaveInstanceId(ComponentId componentId) const;

    typedef eastl::hash_map<ComponentId, IdentityProvider*> ProvidersByComponent;
    ProvidersByComponent mProviders;
};

extern EA_THREAD_LOCAL IdentityManager* gIdentityManager;

}

#endif //BLAZE_IDENTITY_H
