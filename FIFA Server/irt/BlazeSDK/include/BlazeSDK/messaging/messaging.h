/*************************************************************************************************/
/*!
    \file messaging.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_MESSAGING_H
#define BLAZE_MESSAGING_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/component/messaging/tdf/messagingtypes.h"

namespace Blaze
{
    
class UserGroup;

namespace UserManager
{
    class User;
}

namespace Messaging
{

class BLAZESDK_API SendMessageParameters : public ClientMessage
{
public:
    SendMessageParameters(MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP) : ClientMessage(memGroupId) {}
    void setTargetGroup(const UserGroup* group);
    void setTargetUser(const UserManager::User* user);
    void setTargetUser(BlazeId blazeId);
    bool setAttrParam(AttrKey attrKey, uint32_t paramIndex, const char8_t* param);
    const char8_t* getAttrParam(AttrKey attrKey, uint32_t paramIndex) const;
};

class BLAZESDK_API FetchMessageParameters : public FetchMessageRequest
{
public:
    FetchMessageParameters(MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP) : FetchMessageRequest(memGroupId) {}
};

class BLAZESDK_API PurgeMessageParameters : public PurgeMessageRequest
{
public:
    PurgeMessageParameters(MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP) : PurgeMessageRequest(memGroupId) {}
};

class BLAZESDK_API TouchMessageParameters : public TouchMessageRequest
{
public:
    TouchMessageParameters(MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP) : TouchMessageRequest(memGroupId) {}
};

} // namespace messaging

} // namespace Blaze
#endif // BLAZE_MESSAGING_H
