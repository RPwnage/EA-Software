//
//  IGOTelemetryServer.cpp
//  engine
//
//  Created by Gamesim on 2/18/15.
//  Copyright (c) 2015 Electronic Arts. All rights reserved.
//

#ifdef ORIGIN_MAC

#include "IGOTelemetryIPC.h"
#include "IGOSharedStructs.h"
#include "IGOIPC/IGOIPCServer.h"
#include "IGOIPC/IGOIPCConnection.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/session/SessionService.h"
#include "services/settings/SettingsManager.h"

#include "TelemetryAPIDLL.h"

namespace Origin
{
namespace Engine
{
    IGOTelemetryIPC::IGOTelemetryIPC()
    {
        IGOIPC* ipc = IGOIPC::instance();
        mServer = ipc->createServer(IGOIPC_TELEMETRY_REGISTRATION_SERVICE, IGOIPC_TELEMETRY_MEMORY_BUFFER_SIZE);
        mServer->setHandler(this);
        mServer->start();
    }
    
    IGOTelemetryIPC::~IGOTelemetryIPC()
    {
        mServer->stop();
        delete mServer;
    }
        
    void IGOTelemetryIPC::handleConnect(IGOIPCConnection* conn)
    {
        ORIGIN_LOG_EVENT << "TelemetryIPC: connect to " << conn->id();
    }
    
    void IGOTelemetryIPC::handleDisconnect(IGOIPCConnection* conn, void* userData)
    {
        ORIGIN_LOG_EVENT << "TelemetryIPC: disconnect from " << conn->id();
    }
    
    bool IGOTelemetryIPC::handleIPCMessage(IGOIPCConnection* conn, eastl::shared_ptr<IGOIPCMessage> msg, void* userData)
    {
        switch (msg->type())
        {
            case IGOIPC::MSG_TELEMETRY_EVENT:
            {
                size_t msgSize = msg->size();
                if (msgSize != sizeof(OriginIGO::TelemetryMsg))
                {
                    ORIGIN_LOG_ERROR << "TelemetryIPC: invalid message size (" << msgSize << "/" << sizeof(OriginIGO::TelemetryMsg) << ")";
                    return false;
                }
                
                const OriginIGO::TelemetryMsg* tMsg = reinterpret_cast<const OriginIGO::TelemetryMsg*>(msg->data());
                if (tMsg)
                {
                    if (tMsg->version != OriginIGO::TelemetryVersion)
                    {
                        ORIGIN_LOG_ERROR << "TelemetryIPC: invalid message version (" << tMsg->version << "/" << OriginIGO::TelemetryVersion << ")";
                        return false;
                    }
                    
                    QString productId = QString::fromUtf8(tMsg->productId);
                    
                    switch (tMsg->fcn)
                    {
                        case OriginIGO::TelemetryFcn_IGO_HOOKING_BEGIN:
                        {
#ifdef SHOW_RECEIVED_EVENTS
                            ORIGIN_LOG_EVENT << "TelemetryIPC: TelemetryFcn_IGO_HOOKING_BEGIN - ProductId:" << productId << ", Timestamp:" << tMsg->timestamp << ", Pid:" << tMsg->pid;
#endif

                            Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();
                            bool showHardwareSpecs = !Origin::Services::readSetting(Origin::Services::SETTING_HW_SURVEY_OPTOUT, session);
                            GetTelemetryInterface()->Metric_IGO_HOOKING_BEGIN(productId.toLatin1().constData(), tMsg->timestamp, tMsg->pid, showHardwareSpecs);
                        }
                        break;
                            
                        case OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL:
                        {
#ifdef SHOW_RECEIVED_EVENTS
                            ORIGIN_LOG_EVENT << "TelemetryIPC: TelemetryFcn_IGO_HOOKING_FAIL - ProductId:" << productId << ", Timestamp:" << tMsg->timestamp << ", Pid:" << tMsg->pid << ", Context:" << tMsg->context << ", Renderer:" << tMsg->info.msg.renderer << ", Message:" << tMsg->info.msg.message;
#endif
                            GetTelemetryInterface()->Metric_IGO_HOOKING_FAIL(productId.toLatin1().constData(), tMsg->timestamp, tMsg->pid, tMsg->context, tMsg->info.msg.renderer, tMsg->info.msg.message);
                        }
                        break;
                            
                        case OriginIGO::TelemetryFcn_IGO_HOOKING_INFO:
                        {
#ifdef SHOW_RECEIVED_EVENTS
                            ORIGIN_LOG_EVENT << "TelemetryIPC: TelemetryFcn_IGO_HOOKING_FAIL - ProductId:" << productId << ", Timestamp:" << tMsg->timestamp << ", Pid:" << tMsg->pid << ", Context:" << tMsg->context << ", Renderer:" << tMsg->info.msg.renderer << ", Message:" << tMsg->info.msg.message;
#endif
                            GetTelemetryInterface()->Metric_IGO_HOOKING_INFO(productId.toLatin1().constData(), tMsg->timestamp, tMsg->pid, tMsg->context, tMsg->info.msg.renderer, tMsg->info.msg.message);
                            // keep old telemetry event for now as well
                            GetTelemetryInterface()->Metric_IGO_OVERLAY_3RDPARTY_DLL(false, true, tMsg->info.msg.message);
                        }
                        break;
                            
                        case OriginIGO::TelemetryFcn_IGO_HOOKING_END:
                        {
#ifdef SHOW_RECEIVED_EVENTS
                            ORIGIN_LOG_EVENT << "TelemetryIPC: TelemetryFcn_IGO_HOOKING_END - ProductId:" << productId << ", Timestamp:" << tMsg->timestamp << ", Pid:" << tMsg->pid;
#endif
                            GetTelemetryInterface()->Metric_IGO_HOOKING_END(productId.toLatin1().constData(), tMsg->timestamp, tMsg->pid);
                        }
                        break;
                            
                        default:
                            ORIGIN_LOG_ERROR << "TelemetryIPC: invalid IGO telemetry msg fcn = " << tMsg->fcn;
                            break;
                    }
                }
            }
            break;
            
            default:
                ORIGIN_LOG_ERROR << "Telemetry server: unsupported message type:" << msg->type();
            break;
        }
        
        return true;
    }
    
}
}

#endif // ORIGIN_MAC
