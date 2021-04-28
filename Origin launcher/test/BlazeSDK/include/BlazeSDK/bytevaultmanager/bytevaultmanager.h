/**************************************************************************************************/
/*! 
    \file bytevaultmanager.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef BYTEVAULTMANAGER_H
#define BYTEVAULTMANAGER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/component/framework/tdf/networkaddress.h"

namespace Blaze
{

namespace ByteVault
{
class ByteVaultComponent;
}

namespace ByteVaultManager
{

class BLAZESDK_API ByteVaultManager
{
public:
    static ByteVaultManager* create(BlazeHub* hub);
    BlazeError initialize(const char8_t* hostname = nullptr, uint16_t port = 0, bool secure = true, const char8_t* certData = nullptr, const char8_t* keyData = nullptr);
    ByteVault::ByteVaultComponent* getComponent(uint32_t userIndex) const;

    // Deprecated - use getComponent(uint32_t userIndex) instead
    ByteVault::ByteVaultComponent* getComponent() const { return getComponent(mHub->getPrimaryLocalUserIndex()); }

    ~ByteVaultManager();

    bool isInitialized() const { return (getComponent(mHub->getPrimaryLocalUserIndex()) != nullptr); }

private:
    BlazeHub *mHub;
    MemoryGroupId mMemGroup;

private:
    ByteVaultManager(BlazeHub *hub, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
    NON_COPYABLE(ByteVaultManager);
};

} // namespace ByteVaultManager
} // namespace Blaze

#endif // BYTEVAULTMANAGER_H
