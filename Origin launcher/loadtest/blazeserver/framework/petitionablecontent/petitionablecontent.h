/*************************************************************************************************/
/*!
    \file
        petitionablecontent.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_PETITIONABLECONTENT_H
#define BLAZE_PETITIONABLECONTENT_H

/*** Include files *******************************************************************************/
#include "framework/tdf/petitionablecontenttypes_server.h"
#include "framework/rpc/petitionablecontentslave_stub.h"

#include "framework/tdf/attributes.h"

#include "EASTL/string.h"

namespace Blaze
{

class PetitionableContentProvider
{
public:
    virtual ~PetitionableContentProvider() {}

    // required methods

    /* \brief Fetch the content for an id (fiber) */
    virtual BlazeRpcError fetchContent(const EA::TDF::ObjectId& bobjId, Collections::AttributeMap& attributeMap, eastl::string& url) = 0;
    
    /* \brief Find the identity info for multiple keys (fiber) */
    virtual BlazeRpcError showContent(const EA::TDF::ObjectId& bobjId, bool visible) = 0;
};

class PetitionableContentManager : public PetitionableContentSlaveStub
{
    friend class FetchContentCommand;
    friend class ShowContentCommand;

NON_COPYABLE(PetitionableContentManager);

public:

    PetitionableContentManager();
    ~PetitionableContentManager() override;

    // \brief Find the identity info for one key.
    BlazeRpcError DEFINE_ASYNC_RET(fetchContent(const EA::TDF::ObjectId& bobjId, Collections::AttributeMap& attributeMap, eastl::string& url));

    // \brief Find the identity info for multiple keys.
    BlazeRpcError DEFINE_ASYNC_RET(showContent(const EA::TDF::ObjectId& bobjId, bool visible));

    // \brief Register an PetitionableContent provider for the given *local* component. Memory is owned by the caller.
    bool registerProvider(ComponentId compId, PetitionableContentProvider& provider);

    // \brief Deregister an PetitionableContent provider for the given *local* component.
    bool deregisterProvider(ComponentId compId);

private:

    // \brief Find all UserSessionIds matching EA::TDF::ObjectId.
    BlazeRpcError fetchLocalContent(const EA::TDF::ObjectId& bobjId, Collections::AttributeMap& attributeMap, eastl::string& url);
    BlazeRpcError showLocalContent(const EA::TDF::ObjectId& bobjId, bool visible);

    InstanceId getRemotePetitionableContentSlaveInstanceId(ComponentId componentId) const;

    typedef eastl::hash_map<ComponentId, PetitionableContentProvider*> ProvidersByComponent;
    ProvidersByComponent mProviders;
};

extern EA_THREAD_LOCAL PetitionableContentManager* gPetitionableContentManager;

}

#endif //BLAZE_PETITIONABLECONTENT_H
