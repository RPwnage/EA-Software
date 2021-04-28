//
//  IGOTelemetry.cpp
//  IGO
//
//  Created by Gamesim on 2/19/15.
//  Copyright (c) 2015 Electronic Arts. All rights reserved.
//

#include "IGOTelemetry.h"
#include "IGOLogger.h"
#include "IGOApplication.h"
#include "IGOIPC/IGOIPCClient.h"

#if defined(ORIGIN_PC)
#include <Tlhelp32.h>

extern HWND getProcMainWindow(DWORD PID);
extern bool GetOriginWindowInfo(PROCESSENTRY32* pPe);
#endif

namespace OriginIGO
{

static const int ThreadSleepInMS = 2000;


#if defined(ORIGIN_PC)
DWORD StartThread(void* dispatcher)
{
    reinterpret_cast<TelemetryDispatcher*>(dispatcher)->Listen();
    return 0;
}
#elif defined(ORIGIN_MAC)
void* StartThread(void* dispatcher)
{
    reinterpret_cast<TelemetryDispatcher*>(dispatcher)->Listen();
    return 0;
}
#endif
    
//////////////////////////////////////////////////////////////////////////////////////////
    
TelemetryDispatcher* TelemetryDispatcher::instance()
{
    static TelemetryDispatcher dispatcher;
    return &dispatcher;
}
    
TelemetryDispatcher::TelemetryDispatcher()
    : _timestamp(0), _productIdLen(0), _threadHandle(NULL)
#ifdef ORIGIN_MAC
    , _ipc(NULL)
#endif
    , _threadDone(false), _threadStarted(false)
{
    memset(_productId, 0, sizeof(_productId));
}
    

TelemetryDispatcher::~TelemetryDispatcher()
{
    
}

//////////////////////////////////////////////////////////////////////////////////////////
// API
void TelemetryDispatcher::BeginSession()
{
    // Grab unique identifiers from environment
#if defined(ORIGIN_PC)
    DWORD cCount = GetEnvironmentVariable(OIG_TELEMETRY_PRODUCTID_ENV_NAME, _productId, _countof(_productId));
    if (cCount > 0 && cCount != _countof(_productId))
        _productIdLen = wcsnlen(_productId, _countof(_productId)) * sizeof(wchar_t);
    
    else
    {
        IGOForceLogWarn("Unable to retrieve telemetry prod id");
        _productId[0] = 0;
    }
    
    wchar_t buffer[32];
    cCount = GetEnvironmentVariable(OIG_TELEMETRY_TIMESTAMP_ENV_NAME, buffer, _countof(buffer));
    if (cCount > 0 && cCount != _countof(buffer))
        _timestamp = _wtoi64(buffer);
    
    else
        IGOForceLogWarn("Unable to retrieve telemetry timestamp");
    
#elif defined(ORIGIN_MAC)
    
    const char* tmp = getenv(OIG_TELEMETRY_PRODUCTID_ENV_NAME_A);
    if (tmp)
    {
        strncpy(_productId, tmp, sizeof(_productId) - 1);
        _productIdLen = strlen(_productId);
    }
    else
    {
        IGOForceLogWarn("Unable to retrieve telemetry prod id");
        _productId[0] = 0;
    }
    
    tmp = getenv(OIG_TELEMETRY_TIMESTAMP_ENV_NAME_A);
    if (tmp)
        _timestamp = atoll(tmp);
    else
        IGOForceLogWarn("Unable to retrieve telemetry timestamp");
    
    // Setup IPC client
    _ipc = IGOIPC::instance()->createClient(IGOIPC_TELEMETRY_REGISTRATION_SERVICE, IGOIPC_TELEMETRY_MEMORY_BUFFER_SIZE);
    _ipc->setHandler(this);
    _ipc->start();
#endif
    
    TelemetryInfo info;
    memset(&info, 0, sizeof(info));
    PushMsg(TelemetryFcn_IGO_HOOKING_BEGIN, TelemetryContext_GENERIC, info);
}

void TelemetryDispatcher::RecordSession()
{
    _threadStarted = true; // notify we did try to start the thread (ie stop queuing up messages if this fails)
    
    // Start async loop to forward messages (and avoid deadlocks if Origin process is suspending or waiting on the injection to be done (for 64-bit games)
#if defined(ORIGIN_PC)
    _threadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartThread, this, 0, NULL);
    if (!_threadHandle)
        IGOForceLogWarn("Unable to create telemetry thread (err=%d)", GetLastError());
#elif defined(ORIGIN_MAC)
    int result = pthread_create(&_threadHandle, NULL, StartThread, this);
    if (result)
        IGOForceLogWarn("Unable to create telemetry thread (err=%d)", result);
#endif
}
    
void TelemetryDispatcher::EndSession()
{
    if (_threadHandle && !_threadDone)
    {
        TelemetryInfo info;
        memset(&info, 0, sizeof(info));
        PushMsg(TelemetryFcn_IGO_HOOKING_END, TelemetryContext_GENERIC, info);

        
#if defined(ORIGIN_PC)
        HANDLE h = _threadHandle;
        _threadDone = true; // signal our thread we are to flush the messages and stop
        ::WaitForSingleObject(h, ThreadSleepInMS);
#elif defined(ORIGIN_MAC)
        pthread_t h = _threadHandle;
        _threadDone = true;
        
        void* notUsed = NULL;
        pthread_join(h, &notUsed);
        
        Sleep(1000); // give it a tiny bit of time to flush the last messages
#endif
    }
}
        
void TelemetryDispatcher::Send(TelemetryFcn fcn, TelemetryContext ctxt, TelemetryRenderer renderer, const char* message, ...)
{
    static const char* TelemetryRenderers[] =
    {
        "UNKNOWN", // TelemetryRenderer_UNKNOWN
        "DX7",     // TelemetryRenderer_DX7
        "DX8",     // TelemetryRenderer_DX8
        "DX9",     // TelemetryRenderer_DX9
        "DX10",    // TelemetryRenderer_DX10
        "DX11",    // TelemetryRenderer_DX11
		"DX12",    // TelemetryRenderer_DX12
        "MANTLE",  // TelemetryRenderer_MANTLE
        "OPENGL"   // TelemetryRenderer_OPENGL
    };
    
    static_assert(_countof(TelemetryRenderers) == TelemetryRenderer_COUNT, OIG_TELEMETRY_STRUCTURE_SIZE_MISMATCH);

    if (renderer < 0 || renderer >= TelemetryRenderer_COUNT)
        return;
    
    int rendererCount = sizeof(TelemetryRenderers) / sizeof(TelemetryRenderers[0]);
    if (renderer >= rendererCount)
        return;
    
    if (!message)
        return;
    
    TelemetryInfo info;
    memset(&info, 0, sizeof(info));
    strncpy(info.msg.renderer, TelemetryRenderers[renderer], sizeof(info.msg.renderer) - 1);
    
#if defined(ORIGIN_PC)
    va_list args;
    va_start(args, message);
    HRESULT hr = StringCbVPrintfA(info.msg.message, sizeof(info.msg.message), message, args);
    va_end(args);
    
    if (SUCCEEDED(hr) || hr == STRSAFE_E_INSUFFICIENT_BUFFER)
    {
        IGOLogWarn("TelemetryMsg: fcn=%d, ctxt=%d, renderer:%s,  msg:%s", fcn, ctxt, info.msg.renderer, info.msg.message);
        PushMsg(fcn, ctxt, info);
    }
#elif defined(ORIGIN_MAC)
    va_list args;
    va_start(args, message);
    int charCount = vsnprintf(info.msg.message, sizeof(info.msg.message) - 1, message, args);
    va_end(args);
    
    if (charCount >= 0)
    {
        IGOLogWarn("TelemetryMsg: fcn=%d, ctxt=%d, renderer:%s,  msg:%s", fcn, ctxt, info.msg.renderer, info.msg.message);
        PushMsg(fcn, ctxt, info);
    }
    
#endif
}
        
int64_t TelemetryDispatcher::GetTimestamp() const
{
    return _timestamp;
}
    
#if defined(ORIGIN_PC)
const wchar_t* TelemetryDispatcher::GetProductId() const
{
    return _productId;
}
#elif defined(ORIGIN_MAC)
const char* TelemetryDispatcher::GetProductId() const
{
    return _productId;
}
#endif

TelemetryRenderer TelemetryDispatcher::GetRenderer() const
{
    TelemetryRenderer renderer = TelemetryRenderer_UNKNOWN;
#if defined(ORIGIN_PC)
    if (SAFE_CALL_LOCK_TRYREADLOCK_COND)
    {
        renderer = (TelemetryRenderer)SAFE_CALL(IGOApplication::instance(), &IGOApplication::rendererType);
        SAFE_CALL_LOCK_LEAVE
    }
#elif defined(ORIGIN_MAC)
    renderer = TelemetryRenderer_OPENGL;
#endif
    
    return renderer;
}

// IGOIPCHandler overrides
void TelemetryDispatcher::handleConnect(IGOIPCConnection *conn)
{
    IGOLogInfo("Telemetry connect: %d", conn->id());
}

void TelemetryDispatcher::handleDisconnect(IGOIPCConnection *conn, void *userData)
{
    IGOLogInfo("Telemetry disconnect: %d", conn->id());
}

bool TelemetryDispatcher::handleIPCMessage(IGOIPCConnection *conn, eastl::shared_ptr<IGOIPCMessage> msg, void *userData)
{
    IGOLogInfo("Telemetry msg type=%d", msg->type());
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// INTERNALS
void TelemetryDispatcher::Listen()
{
    TelemetryFcn fcn;
    TelemetryContext ctxt;
    TelemetryInfo info;
    
    while (!_threadDone)
    {
        // Flush pending messages
        while (PopMsg(&fcn, &ctxt, &info))
            DispatchMsg(fcn, ctxt, info);
        
        // let's take a breather
        Sleep(ThreadSleepInMS);
    }
    
    // Any final messages?
    while (PopMsg(&fcn, &ctxt, &info))
        DispatchMsg(fcn, ctxt, info);
    
#ifdef ORIGIN_PC
    CloseHandle(_threadHandle);
#endif
    
    _threadHandle = NULL;
}
    
void TelemetryDispatcher::DispatchMsg(TelemetryFcn fcn, TelemetryContext ctxt, const TelemetryInfo& info)
{
    static const char* ContextAcronyms[] =
    {
        "Generic",      // TelemetryContext_GENERIC
        "3rdParty",     // TelemetryContext_THIRD_PARTY
        "MHook",        // TelemetryContext_MHOOK
        "Resource",     // TelemetryContext_RESOURCE
        "Hardware"      // TelemetryContext_HARDWARE
    };
    
    static_assert(_countof(ContextAcronyms) == TelemetryContext_COUNT, OIG_TELEMETRY_STRUCTURE_SIZE_MISMATCH);

    // Validate the inputs
    if (fcn < 0 || fcn >= TelemetryFcn_COUNT)
        return;
    
    if (ctxt < 0 || ctxt >= TelemetryContext_COUNT)
        return;
    
    int ctxtCount = sizeof(ContextAcronyms) / sizeof(ContextAcronyms[0]);
    if (ctxt >= ctxtCount)
        return;
    
#if defined(ORIGIN_PC)
    
    // Make sure we can
    PROCESSENTRY32 pe;
    if (GetOriginWindowInfo(&pe))
    {
        HWND hWnd = getProcMainWindow(pe.th32ProcessID);
        if (hWnd)
        {
            TelemetryMsg msg;
            memset(&msg, 0, sizeof(msg));
            
            msg.version = TelemetryVersion;
            
            GameLaunchInfo* gi = gameInfo();
            msg.timestamp = _timestamp;
            msg.pid = GetCurrentProcessId();
            gi->gliMemcpy(msg.productId, sizeof(msg.productId), _productId, _productIdLen);
            
            msg.fcn = fcn;
            const char* context = ContextAcronyms[ctxt];
            strncpy(msg.context, context, sizeof(msg.context) - 1);
            msg.info = info;
            
            COPYDATASTRUCT copyData;
            static const int kGameTelemetry = 124542; // TODO: need to share this better with OriginApplication.cpp
            copyData.dwData = kGameTelemetry;
            copyData.cbData = sizeof(msg);
            copyData.lpData = &msg;
            if (IsWindowUnicode(hWnd))
                SendMessageW(hWnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&copyData));
            else
                SendMessageA(hWnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&copyData));
        }
        
        else
            IGOForceLogWarnOnce("Unable to find client window - losing telemetry call...");
    }
    
    else
        IGOForceLogWarnOnce("Unable to find client process - losing telemetry call...");
    
#elif defined(ORIGIN_MAC)
    
    TelemetryMsg msg;
    memset(&msg, 0, sizeof(msg));
    
    msg.version = TelemetryVersion;
    msg.timestamp = _timestamp;
    msg.pid = GetCurrentProcessId();
    strncpy(msg.productId, _productId, sizeof(msg.productId) - 1);
    
    msg.fcn = fcn;
    const char* context = ContextAcronyms[ctxt];
    strncpy(msg.context, context, sizeof(msg.context) - 1);
    msg.info = info;
    
    eastl::shared_ptr<IGOIPCMessage> ipcMsg(IGOIPC::instance()->createMessage(IGOIPC::MSG_TELEMETRY_EVENT, reinterpret_cast<const char*>(&msg), sizeof(msg)));
    _ipc->send(ipcMsg);
    
#else
#error "Telemetry not supported on this system"
#endif
}

void TelemetryDispatcher::PushMsg(TelemetryFcn fcn, TelemetryContext ctxt, const TelemetryInfo& info)
{
    if (!_threadHandle && _threadStarted)
        return;
    
    EA::Thread::AutoRWSpinLock lock(_queueLock, EA::Thread::AutoRWSpinLock::kLockTypeWrite);
    _queue.insert(_queue.begin(), QueueEntry(fcn, ctxt, info));
}
    
bool TelemetryDispatcher::PopMsg(TelemetryFcn* fcn, TelemetryContext* ctxt, TelemetryInfo* info)
{
    EA::Thread::AutoRWSpinLock lock(_queueLock, EA::Thread::AutoRWSpinLock::kLockTypeWrite);
    if (!_queue.empty())
    {
        const QueueEntry& entry = _queue.back();
        *fcn = entry.fcn;
        *ctxt = entry.ctxt;
        *info = entry.info;
        
        _queue.pop_back();
        
        return true;
    }
    
    return false;
}
    
} // OriginIGO
