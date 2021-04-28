///////////////////////////////////////////////////////////////////////////////
// TelemetryAPIManager.h
//
// Implements the manager for TelemetryAPI
//
// Created by OriginDevSupport
// Copyright (c) Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef TelemetryAPIManager_h__
#define TelemetryAPIManager_h__


#include "EASTL/shared_ptr.h"
#include "EASTL/list.h"

//Forward Declarations
class EbisuTelemetryAPI;

namespace EA
{
    namespace Thread
    {
        class Futex;
    }
}

namespace NonQTOriginServices
{
    class SystemInformation;
}

namespace OriginTelemetry
{

    class EbisuTelemetryAPIConcrete;
    class EbisuTelemetryServerIO;
    class TelemetryPersona;

    enum TelemetryManagerEvent
    {
        TELEMETRY_MANAGER_EVENT_LOGIN,
        TELEMETRY_MANAGER_EVENT_LOGOUT,
        TELEMETRY_MANAGER_EVENT_TELEMETRY_READER_FLUSH_COMPLETE,
        TELEMETRY_MANAGER_EVENT_READY_TO_SEND
    };

    enum TelemetrySendingState
    {
        TELEMETRY_SENDING_STATE_WAIT_FOR_CONFIG,
        TELEMETRY_SENDING_STATE_SENDING,
        TELEMETRY_SENDING_STATE_FLUSHING,
        TELEMETRY_SENDING_SHUTDOWN,
        TELEMETRY_SENDING_DESTROY
    };

    typedef eastl::shared_ptr<EbisuTelemetryServerIO> SeverIOSharedPointer;
    typedef eastl::shared_ptr<TelemetryPersona> PersonaSharedPointer;

    class TelemetryAPIManager
    {
    public:
        virtual ~TelemetryAPIManager();

        ///Singleton Instance accessor.
        static TelemetryAPIManager& getInstance()
        {
            static TelemetryAPIManager instance;

            return instance;
        }

        EbisuTelemetryAPI* getTelemetryAPI();

        void shutdown();

        void notify( TelemetryManagerEvent event);

    private:
        //private constructor for singleton
        TelemetryAPIManager();

        //Disable copy and dereference
        TelemetryAPIManager(TelemetryAPIManager&);
        TelemetryAPIManager& operator=(const TelemetryAPIManager&);
        TelemetryAPIManager& operator&(const TelemetryAPIManager&);

        /// Get the futex for this object
        EA::Thread::Futex &getLock();

        /// Helper function for constructing the serverIO after the dynamic config settings have been processed.
        void handleStartSending();
        /// Helper function for handling when the serverIO has finished flushing a persona.
        void handleTelemetryReaderFlushComplete();
        /// Helper function for handling logout.
        void handleLogout();

        SeverIOSharedPointer m_TelemetryServerIO;
        eastl::list<PersonaSharedPointer> m_PersonaShutdownList;
        eastl::shared_ptr<TelemetryPersona> m_TelemetryPersona;
        EbisuTelemetryAPIConcrete* m_TelemetryAPIConcrete;
        TelemetrySendingState m_TelemetrySendingState;
        EA::Thread::Futex *m_lock;
        bool m_WasShutdownCalled;
        
    };

    
}




#endif // TelemetryAPIManager_h__
