/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REMOTE_INSTANCE_LISTENER_H
#define BLAZE_REMOTE_INSTANCE_LISTENER_H

#include "framework/component/componentstub.h"

namespace Blaze
{

class RemoteServerInstance;

class RemoteServerInstanceListener
{
public:
    virtual void onRemoteInstanceChanged(const RemoteServerInstance& instance) = 0;
    virtual void onRemoteInstanceDrainingStateChanged(const RemoteServerInstance& instance) {}
    virtual ~RemoteServerInstanceListener() { }
};
    
} // Blaze

#endif // BLAZE_REMOTE_INSTANCE_LISTENER_H

