/**************************************************************************************************/
/*! 
    \file bytevaultmanager.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

// Include files
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/bytevaultmanager/bytevaultmanager.h"
#include "BlazeSDK/component/bytevaultcomponent.h"

#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/blazeerrortype.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"

#include "BlazeSDK/blazesdkdefs.h"
#include "shared/framework/protocol/shared/heat2encoder.h"
#include "shared/framework/protocol/shared/heat2decoder.h"

#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/dirtysock/netconn.h"

namespace Blaze
{

namespace ByteVaultManager
{

ByteVaultManager* ByteVaultManager::create(BlazeHub* hub)
{
    return BLAZE_NEW(MEM_GROUP_FRAMEWORK, "ByteVaultManager") ByteVaultManager(hub, MEM_GROUP_FRAMEWORK);
}

ByteVaultManager::ByteVaultManager(BlazeHub *hub, MemoryGroupId memGroupId/* = MEM_GROUP_FRAMEWORK_TEMP */)
    : mHub(hub),
      mMemGroup(memGroupId)
{
}

ByteVaultManager::~ByteVaultManager()
{
}

BlazeError ByteVaultManager::initialize(const char8_t* hostname/* = nullptr*/, uint16_t port/* = 0*/, bool secure/* = true*/, const char8_t* certData/* = nullptr*/, const char8_t* keyData/* = nullptr*/)
{
    if (isInitialized())
        return BYTEVAULT_ALREADY_INITIALIZED;

    if (hostname == nullptr || hostname[0] == '\0')
    {
        const char8_t* tempHost = hostname;
        const char8_t* tempSecure = secure ? "true" : "false";

        if (mHub->getConnectionManager()->getServerConfigString(BLAZESDK_CONFIG_KEY_BYTEVAULT_HOSTNAME, &tempHost))
        {
            hostname = tempHost;
        }

        if (mHub->getConnectionManager()->getServerConfigString(BLAZESDK_CONFIG_KEY_BYTEVAULT_SECURE, &tempSecure))
        {
            if (blaze_stricmp(tempSecure, "true") != 0)
            {
                secure = false;
            }
        }
    }

    if (port == 0)
    {
        int32_t tempPort = 0;
        if (mHub->getConnectionManager()->getServerConfigInt(BLAZESDK_CONFIG_KEY_BYTEVAULT_PORT, &tempPort))
        {
            port = (uint16_t) tempPort;
        }
    }

    if (hostname == nullptr || hostname[0] == '\0' || port == 0)
    {
        BLAZE_SDK_DEBUGF("[ByteVaultManager:%p].initialize : No bytevault connection parameters found.", this);
        return BYTEVAULT_NOT_INITIALIZED;
    }

    BlazeSender::ServerConnectionInfo serverInfo(hostname, port, secure);

    ByteVault::ByteVaultComponent::createComponent(mHub, serverInfo, Encoder::JSON, certData, keyData);

    return ERR_OK;
}

ByteVault::ByteVaultComponent* ByteVaultManager::getComponent(uint32_t userIndex) const
{
    if (mHub->getComponentManager(userIndex) != nullptr)
    {
        return mHub->getComponentManager(userIndex)->getByteVaultComponent();
    }
    else
    {
        return nullptr;
    }
}

} // namespace ByteVaultManager
} // namespace Blaze
