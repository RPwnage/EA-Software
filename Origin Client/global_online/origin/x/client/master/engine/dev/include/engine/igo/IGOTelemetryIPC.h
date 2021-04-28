//
//  IGOTelemetryIPC.h
//  engine
//
//  Created by Gamesim on 2/18/15.
//  Copyright (c) 2015 Electronic Arts. All rights reserved.
//

#ifndef IGOTELEMETRYIPC_H
#define IGOTELEMETRYIPC_H

#ifdef ORIGIN_MAC

#include "IGOIPC/IGOIPC.h"
#include "IGOIPC/IGOIPCMessage.h"


namespace Origin
{
namespace Engine
{
    class IGOTelemetryIPC : public IGOIPCHandler
    {
    public:
        IGOTelemetryIPC();
        virtual ~IGOTelemetryIPC();
        
    private:
        virtual void handleConnect(IGOIPCConnection* conn);
        virtual void handleDisconnect(IGOIPCConnection* conn, void* userData = NULL);
        virtual bool handleIPCMessage(IGOIPCConnection* conn, eastl::shared_ptr<IGOIPCMessage> msg, void* userData =  NULL);
        
        IGOIPCServer* mServer;
    };
}
}

#endif // ORIGIN_MAC
#endif // IGOTELEMETRYIPC_H
