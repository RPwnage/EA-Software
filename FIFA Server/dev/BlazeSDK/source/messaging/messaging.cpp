/*************************************************************************************************/
/*!
    \file messaging.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/usermanager/user.h"
#include "BlazeSDK/usergroup.h"
#include "BlazeSDK/component/messagingcomponent.h"
#include "BlazeSDK/messaging/messaging.h"

namespace Blaze
{
namespace Messaging
{


void SendMessageParameters::setTargetGroup(const UserGroup* userGroup)
{
    setTargetType((userGroup->getBlazeObjectId()).type);
    getTargetIds().clear();
    getTargetIds().push_back((userGroup->getBlazeObjectId()).id);
}

void SendMessageParameters::setTargetUser(const UserManager::User* user)
{
    setTargetType((user->getBlazeObjectId()).type);
    getTargetIds().clear();
    getTargetIds().push_back((user->getBlazeObjectId()).id);
}

void SendMessageParameters::setTargetUser(BlazeId blazeId)
{
    setTargetType(ENTITY_TYPE_USER);
    getTargetIds().clear();
    getTargetIds().push_back(blazeId);
}

bool SendMessageParameters::setAttrParam(AttrKey attrKey, uint32_t paramIndex, const char8_t* param)
{
    if(paramIndex < PARAM_COUNT_LIMIT)
    {
        getAttrMap().insert(eastl::make_pair(PARAM_ATTR_MIN + attrKey*PARAM_COUNT_LIMIT + paramIndex, param));
        return true;
    }
    return false;
}

const char8_t* SendMessageParameters::getAttrParam(AttrKey attrKey, uint32_t paramIndex) const
{
    if(paramIndex < PARAM_COUNT_LIMIT)
    {
        MessageAttrMap::const_iterator it = getAttrMap().find(PARAM_ATTR_MIN + attrKey*PARAM_COUNT_LIMIT + paramIndex);
        if(it != getAttrMap().end())
        {
            return it->second.c_str();
        }
    }
    return nullptr;
}


}
}
