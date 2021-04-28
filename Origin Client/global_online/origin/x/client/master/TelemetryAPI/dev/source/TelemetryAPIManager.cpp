///////////////////////////////////////////////////////////////////////////////
// TelemetryAPIManager.cpp
//
// Implements the manager for TelemetryAPI
//
// Created by Origin DevSupport
// Copyright (c) Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TelemetryAPIManager.h"
#include "TelemetryWriterInterface.h"
#include "TelemetryReaderInterface.h"
#include "EbisuTelemetryServerIO.h"
#include "EbisuTelemetryAPIConcrete.h"
#include "TelemetryPersona.h"
#include "TelemetryLogger.h"
#include "TelemetryConfig.h"

#include "SystemInformation.h"

#include "netconn.h"
#include "DirtySDK/dirtysock/dirtycert.h"

#include "EAAssert/eaassert.h"
#include "eathread/eathread_futex.h"

bool g_dirtynetStarted = false;

namespace OriginTelemetry
{
    static const uint32_t WAIT_SHUTDOWN_SLEEP_TIME = 200;
    static const uint32_t   SHUTDOWN_TIMEOUT = 10000;

    TelemetryAPIManager::TelemetryAPIManager():
        m_TelemetryAPIConcrete(NULL)
        ,m_TelemetrySendingState(TELEMETRY_SENDING_STATE_WAIT_FOR_CONFIG)
        ,m_lock(new EA::Thread::Futex())
        , m_WasShutdownCalled(false)
    {
        NonQTOriginServices::SystemInformation::instance(); // init instance, starts DirtyNet.
       
        //Initialize any settings in the telemetry config
        TelemetryConfig::processEacoreiniSettings();
        // Set TelemetryConfig values
        TelemetryConfig::setUniqueMachineHash(NonQTOriginServices::SystemInformation::instance().uniqueMachineHashFNV1A());

        m_TelemetryPersona = new TelemetryPersona();

        //The API must startup before ServerIO as it initializes DirtySDK.  
        m_TelemetryAPIConcrete = new EbisuTelemetryAPIConcrete(m_TelemetryPersona);

        TELEMETRY_DEBUG("TelemetryAPIManager::TelemetryAPIManager() Constructor complete");
    }

    TelemetryAPIManager::~TelemetryAPIManager()
    {
        if( m_TelemetrySendingState != TELEMETRY_SENDING_DESTROY)
        {
            //Shutdown wasn't called so let's call it
            eastl::string8 errMsg = "~TelemetryAPIManager() called without shutdown being called first.";
            TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
            EA_FAIL_MSG( errMsg.c_str());
            shutdown();
        }

        m_TelemetryPersona.reset();

        if(m_lock && m_lock->HasLock())
        {
            m_lock->Unlock();
        }
        if(m_lock)
        {
            delete m_lock;
            m_lock = NULL;
        }

        // Get rid of the TelemetryAPI interface.
        if( m_TelemetryAPIConcrete != NULL)
        {
            delete m_TelemetryAPIConcrete;
            m_TelemetryAPIConcrete = NULL;
        }
    }


    void TelemetryAPIManager::shutdown()
    {
        //Guard so we don't call shutdown more than once.
        if (m_WasShutdownCalled)
            return; 

        m_WasShutdownCalled = true;

        //Give the API a null persona so that it will log any events that come in late.
        m_TelemetryAPIConcrete->setTelemetryWriter(PersonaSharedPointer());

        {   //Block to scope the lock to just the final flush setup.
            EA::Thread::AutoFutex lock(getLock());

            TELEMETRY_DEBUG("TelemetryAPIManager::~TelemetryAPIManager() Setting m_TelemetrySendingState to TELEMETRY_SENDING_SHUTDOWN");
            TelemetrySendingState currentState = m_TelemetrySendingState;
            m_TelemetrySendingState = TELEMETRY_SENDING_SHUTDOWN;

            if( TELEMETRY_SENDING_STATE_FLUSHING == currentState)
            {
                m_PersonaShutdownList.push_back(m_TelemetryPersona);
            }
            else
            {
                if(m_TelemetryServerIO)
                {
                    m_TelemetryServerIO->flushAndDestroyTelemetryReader();
                }
                else
                {
                    eastl::string8 errMsg = "m_TelemetryServerIO was null in the TelemetryAPIManager destructor. We probably never constructed it.";
                    TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
                    EA_FAIL_MSG(errMsg.c_str());
                }
            }
        }

        uint32_t timer = 0;

        while( TELEMETRY_SENDING_SHUTDOWN == m_TelemetrySendingState && timer < SHUTDOWN_TIMEOUT)
        {
            EA::Thread::ThreadSleep(WAIT_SHUTDOWN_SLEEP_TIME);
            timer += WAIT_SHUTDOWN_SLEEP_TIME;
        }

        {   //Block to scope the lock.
            EA::Thread::AutoFutex lock(getLock());
            if( TELEMETRY_SENDING_DESTROY != m_TelemetrySendingState)
            {
                TelemetryLogger::GetTelemetryLogger().logTelemetryWarning("Flushing the TelemtrySDK on exit timed out.");
            }
            //Force the state to shutdown. If we didn't finish at this point we're relying 
            m_TelemetrySendingState = TELEMETRY_SENDING_DESTROY;
            TELEMETRY_DEBUG("TelemetryAPIManager::~TelemetryAPIManager() Setting m_TelemetrySendingState to TELEMETRY_SENDING_DESTROY");

            while( !m_PersonaShutdownList.empty())
            {
                m_PersonaShutdownList.pop_front();
            }
        }

        m_TelemetryServerIO.reset();

        if (g_dirtynetStarted)
        {
            NetConnShutdown(0);     // if we don't do this, the installer crashes!!!
            g_dirtynetStarted = false;
        }
    }

    EbisuTelemetryAPI* TelemetryAPIManager::getTelemetryAPI()
    { 
        return static_cast<EbisuTelemetryAPI*>(m_TelemetryAPIConcrete);
    }

    void TelemetryAPIManager::notify( TelemetryManagerEvent event)
    {
        switch( event )
        {
        case TELEMETRY_MANAGER_EVENT_LOGIN:
            TELEMETRY_DEBUG("TelemetryManager::notify() Login detected.");
            break;

        case TELEMETRY_MANAGER_EVENT_LOGOUT:
            handleLogout();
            break;

        case TELEMETRY_MANAGER_EVENT_TELEMETRY_READER_FLUSH_COMPLETE:
            handleTelemetryReaderFlushComplete();
            break;

        case TELEMETRY_MANAGER_EVENT_READY_TO_SEND:
            handleStartSending();
            break;

        default:
            eastl::string8 err("TelemetryAPIManager::notify() Invalid TelemetryManagerEvent recieved.");
            TelemetryLogger::GetTelemetryLogger().logTelemetryError(err);
            EA_FAIL_MSG(err.c_str());
            break;
        }
    }

    void TelemetryAPIManager::handleStartSending()
    {
        if( TELEMETRY_SENDING_STATE_WAIT_FOR_CONFIG != m_TelemetrySendingState)
        {
            eastl::string8 errMsg;
            errMsg.append_sprintf("TelemetryAPIManager::handleConfigLoadComplete() called while not in state TELEMETRY_SENDING_STATE_WAIT_FOR_CONFIG. State was %d", m_TelemetrySendingState);
            TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
            EA_FAIL_MSG(errMsg.c_str());
            return;
        }
        m_TelemetrySendingState = TELEMETRY_SENDING_STATE_SENDING;
        TELEMETRY_DEBUG("TelemetryAPIManager::handleConfigLoadComplete() Setting state from TELEMETRY_SENDING_STATE_WAIT_FOR_CONFIG to TELEMETRY_SENDING_STATE_SENDING.");
        m_TelemetryServerIO = new EbisuTelemetryServerIO(m_TelemetryPersona);
    }

    void TelemetryAPIManager::handleTelemetryReaderFlushComplete()
    {
        EA::Thread::AutoFutex lock(getLock());

        if( !(TELEMETRY_SENDING_SHUTDOWN == m_TelemetrySendingState || TELEMETRY_SENDING_STATE_FLUSHING == m_TelemetrySendingState) )
        {
            eastl::string8 badStateMsg;
            badStateMsg.append_sprintf("TelemetryAPIManager::handleTelemetryReaderFlushComplete() function called while m_TelemetrySendingState is in an invalid state  m_TelemetrySendingState = %d", m_TelemetrySendingState);
            TelemetryLogger::GetTelemetryLogger().logTelemetryError(badStateMsg);
            EA_FAIL_MESSAGE(badStateMsg.c_str());
            return;
        }

        if(m_PersonaShutdownList.empty())
        {
            switch(m_TelemetrySendingState)
            {
                case TELEMETRY_SENDING_SHUTDOWN:
                    m_TelemetrySendingState = TELEMETRY_SENDING_DESTROY;
                    TELEMETRY_DEBUG("TelemetryAPIManager::handleTelemetryReaderFlushComplete() Setting state from TELEMETRY_SENDING_SHUTDOWN to TELEMETRY_SENDING_DESTROY");
                    break;
                case TELEMETRY_SENDING_STATE_FLUSHING:
                    m_TelemetrySendingState = TELEMETRY_SENDING_STATE_SENDING;
                    TELEMETRY_DEBUG("TelemetryAPIManager::handleTelemetryReaderFlushComplete() Setting state from TELEMETRY_SENDING_STATE_FLUSHING to TELEMETRY_SENDING_STATE_SENDING");
                    //Nothing else to flush so set the current persona as the current reader.
                    m_TelemetryServerIO->setTelemetryReader(m_TelemetryPersona);
                    break;
                default:
                    eastl::string8 err("TelemetryAPIManager::handleTelemetryReaderFlushComplete() : Invalid stated detected. This should be unreachable code.");
                    TelemetryLogger::GetTelemetryLogger().logTelemetryError(err);
                    EA_FAIL_MSG(err.c_str());
                    break;
            }
        }
        else 
        {
            TELEMETRY_DEBUG("TelemetryAPIManager::handleTelemetryReaderFlushComplete() flushing next Persona from m_PersonaShutdownList.");
            PersonaSharedPointer nextPersonaToFlush = m_PersonaShutdownList.front();
            m_PersonaShutdownList.pop_front();
            m_TelemetryServerIO->setTelemetryReader(nextPersonaToFlush);
            m_TelemetryServerIO->flushAndDestroyTelemetryReader();
        }
    }

    void TelemetryAPIManager::handleLogout()
    {
        EA::Thread::AutoFutex lock(getLock());
        PersonaSharedPointer personaToFlush = m_TelemetryPersona;

        //Create a new persona since the user has logged out and redirect all telemetry there.
        m_TelemetryPersona.reset( new TelemetryPersona() );
        m_TelemetryAPIConcrete->setTelemetryWriter(m_TelemetryPersona);

        switch(m_TelemetrySendingState)
        {
            case TELEMETRY_SENDING_STATE_SENDING:
                //If we were in the sending state then personaToFlush is already owned by the serverio
                // so we just tell it to flush it out and don't need to hang onto the persona reference.
                m_TelemetrySendingState = TELEMETRY_SENDING_STATE_FLUSHING;
                TELEMETRY_DEBUG("TelemetryAPIManager::handleLogout() Setting state from TELEMETRY_SENDING_STATE_SENDING to TELEMETRY_SENDING_STATE_FLUSHING.");
                m_TelemetryServerIO->flushAndDestroyTelemetryReader();
                break;

            case TELEMETRY_SENDING_STATE_FLUSHING:
                TELEMETRY_DEBUG("TelemetryAPIManager::handleLogout() Already flushing. Adding Persona to m_PersonaShutdownList.");
                //serverIO is busy flushing some other persona. So store this one in the list of persona's to flush.
                m_PersonaShutdownList.push_back(personaToFlush);
                break;

            default:
                eastl::string8 err("TelemetryAPIManager::handleLogout invalid state encountered.  This should be unreachable code.");
                TelemetryLogger::GetTelemetryLogger().logTelemetryError(err);
                EA_FAIL_MSG(err.c_str());
                break;
        }
    }

    EA::Thread::Futex& TelemetryAPIManager::getLock()
    {
        return *m_lock; 
    } 
    

}
