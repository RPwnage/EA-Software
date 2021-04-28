//
//  IGOTelemetry.h
//  IGO
//
//  Created by Gamesim on 2/19/15.
//  Copyright (c) 2015 Electronic Arts. All rights reserved.
//

#ifndef IGOTELEMETRY_H
#define IGOTELEMETRY_H

#include "IGO.h"
#include "IGOSharedStructs.h"
#include "IGOIPC/IGOIPC.h"
#include "eathread/eathread_rwspinlock.h"

namespace OriginIGO
{
    
#if defined(ORIGIN_MAC)
class TelemetryDispatcher : public IGOIPCHandler
#elif defined(ORIGIN_PC)
class TelemetryDispatcher
#endif
{
public:
    static TelemetryDispatcher* instance();
    
    // API
    void BeginSession();    // Prepare for recording of telemetry
    void RecordSession();   // Actually listen/send the telemetry over to the client
    void EndSession();      // End telemetry recording
    
    void Send(TelemetryFcn fcn, TelemetryContext ctxt, TelemetryRenderer renderer, const char* message, ...);
    
    // Accessors for InjectHook.cpp when forwarding info to other processes
    int64_t GetTimestamp() const;
#if defined(ORIGIN_PC)
    const wchar_t* GetProductId() const;
#elif defined(ORIGIN_MAC)
    const char* GetProductId() const;
#endif
    TelemetryRenderer GetRenderer() const;

    // IGOIPCHandler overrides
    virtual void handleConnect(IGOIPCConnection *);
    virtual void handleDisconnect(IGOIPCConnection *, void *userData = NULL);
    virtual bool handleIPCMessage(IGOIPCConnection *, eastl::shared_ptr<IGOIPCMessage> msg, void *userData = NULL);

    
public: // BUT REALLY PRIVATE
    void Listen();
    
private:
    TelemetryDispatcher();
     virtual ~TelemetryDispatcher();


    void DispatchMsg(TelemetryFcn fcn, TelemetryContext ctxt, const TelemetryInfo& info);
    void PushMsg(TelemetryFcn fcn, TelemetryContext ctxt, const TelemetryInfo& info);
    bool PopMsg(TelemetryFcn* fcn, TelemetryContext* ctxt, TelemetryInfo* info);
    
    
    int64_t _timestamp;
    size_t _productIdLen;
    
#if defined(ORIGIN_PC)
    wchar_t _productId[64];
    HANDLE _threadHandle;
#elif defined(ORIGIN_MAC)
    char _productId[64];
    pthread_t _threadHandle = NULL;
    IGOIPCClient* _ipc = NULL;
#endif
    volatile bool _threadDone;
    volatile bool _threadStarted;

    
    struct QueueEntry
    {
        QueueEntry(TelemetryFcn f, TelemetryContext c, const TelemetryInfo& i) : fcn(f), ctxt(c), info(i) {}
        TelemetryFcn fcn;
        TelemetryContext ctxt;
        TelemetryInfo info;
    };
    
    eastl::vector<QueueEntry> _queue;
    EA::Thread::RWSpinLock _queueLock;
};
    
} // OriginIGO

#endif