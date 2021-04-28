///////////////////////////////////////////////////////////////////////////////
// IGOIPCClient.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOIPCCLIENT_H
#define IGOIPCCLIENT_H

#include "IGOIPCConnection.h"
#include "eathread/eathread_futex.h"

class IGOIPCClient : public IGOIPCConnection
{
public:
#if defined(ORIGIN_MAC)
    IGOIPCClient(const char* serviceName, size_t messageQueueSize);
#elif defined(ORIGIN_PC)
    IGOIPCClient();
#endif
    virtual ~IGOIPCClient();

	virtual bool start();
	virtual void stop(void *userData = NULL);

	void setHandler(IGOIPCHandler *handler) { mHandler = handler; }
	void processMessages(void *userData = NULL);

private:
    IGOIPCHandler *mHandler;
    uint32_t mConnectionCreationTime;
    EA::Thread::Futex mPipeMutex;
};

#endif