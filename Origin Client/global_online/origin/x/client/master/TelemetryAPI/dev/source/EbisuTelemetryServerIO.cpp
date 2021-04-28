///////////////////////////////////////////////////////////////////////////////
// EbisuTelemetryServerIO.cpp
//
// Telemetry SDK control class.  Uses the passed in telemetry reader to get telemetry and feed it to the Telemetry SDK
//
// Owner Origin Dev Support
// Copyright (c) Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "EbisuTelemetryServerIO.h"
#include "TelemetryConfig.h"
#include "TelemetryReaderInterface.h"
#include "TelemetryLogger.h"
#include "TelemetryAPIManager.h"
#include "TelemetryEvent.h"

//Origin Version include to get the current version.
#include "version/version.h"
#include "Utilities.h"


#include "EAStdC/EADateTime.h"
#include "eathread/eathread_futex.h"
#include "eathread/eathread_mutex.h"
#include "eathread/eathread_thread.h"
#include "EAAssert/eaassert.h"

namespace OriginTelemetry
{

//*************************************** CONSTANTS ***************************************

    // values for filling up telemetry buffer in TelemetryAPI
    static const uint32_t BUFFER_PERCENTAGE_FULL = 20; // percent of the buffer 100 = full
    static const uint32_t BUFFER_SEND_DELAY = 15000; // milliseconds before sending data

    // Size of buffer telemetry SDK should use
    static const uint32_t TELMETRYSDK_BUFFER_SIZE = 640 * 1024;
    // The SubmitEven3Ex function can submit additional attributes
    // with the event being sent, however, we're not using that
    // functionality.
    static const uint32_t NUM_ADDITIONAL_ATTRIBUTES = 0;
    // Number of times to retry pumping the telemetry SDK before failing when it is full.
    static const uint32_t NUM_TRIES = 10;
    //Time to sleep between loops of our state machine.
    //TODO tmobbs tune this setting (Do we want it closer to 30 FPS like a game?)
    static const uint32_t TIME_TO_SLEEP = 200;

    // Generate string for enumeration. Excellent for debugging
    static const char *STATE_STRING[] = {
        FOREACH_STATE(GENERATE_STRING)
    };


//*************************************** CALLBACKS ***************************************

    /// \brief function to recover from telemetry sending failure
    /// \param pointer to serverIO
    void dataRecoveryCallback(TelemetryApiRefT *pRef)
    {
        // TODO We could hook this up to capture the snapshot and save it to send it next time the client runs.
        TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] [CALLBACK] dataRecoveryCallback called.", __FUNCTION__, __FILE__, __LINE__));

        int32_t numEventsRemaining = TelemetryApiStatus(pRef, 'num3', NULL, 0); // number of v3 events
        int32_t backbufferState = TelemetryApiStatus(pRef, 'bbuf', NULL, 0);

        if(numEventsRemaining > 0 || backbufferState != 0)
        {
            //we will be losing events
            eastl::string8 errMsg;
            errMsg.append_sprintf("dataRecoveryCallback called when there are unsent events. TelemterySDK will lose events.events=[%d], backbuffer=[%d]", numEventsRemaining, backbufferState);
            TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
            EA_FAIL_MSG(errMsg.c_str());
        }
    }

    /// \brief callback called when the transaction is finished.
    /// \param telemetry reference
    /// \param error code when successful or http code when in error
    void loggerCallback(TelemetryApiRefT *pRef, int32_t iErrorCode)
    {
        eastl::string8 msgWithInfo;
        if (1==iErrorCode)
        {
            msgWithInfo.append_sprintf("[%s %s %d] [loggerCallback] Server request finished successfully. HTTP code = [200].", __FUNCTION__, __FILE__, __LINE__);
            TELEMETRY_DEBUG(msgWithInfo);
        }
        else
        {
            msgWithInfo.append_sprintf("TelemetrySDK loggerCallback returned a non 200 http code. . HTTP code = [%d].", iErrorCode);
            TelemetryLogger::GetTelemetryLogger().logTelemetryWarning(msgWithInfo);
        }
    }

    void setSnapshotFreeCallback(TelemetryApiRefT *pRef, int32_t iErrorCode)
    {
        TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] [setSnapshotFreeCallback] code  [%d].", __FUNCTION__, __FILE__, __LINE__, iErrorCode));
    }


//*************************************** CONSTRUCTORS/DESTRUCTORS ***************************************

    EbisuTelemetryServerIO::EbisuTelemetryServerIO( eastl::shared_ptr<ITelemetryReader> telmReader) 
        :m_telmReader(telmReader)
        ,mNucleusId(0)
        ,mHashedNucleusId(0)
        ,m_locale(0)
        ,m_waitingToFlush(false)
        ,mFilteredEvents(0)
        , m_sessionID("")
    {
        m_lock = new EA::Thread::Futex();
        mState.setIoState(IOState::SERVERIO_STATE_STARTUP);
        EA::Thread::ThreadParameters param;
        param.mpName = "EbisuTelemetryServerIOThread";
        m_telemetryUpdateThread = new EA::Thread::Thread;
        TELEMETRY_DEBUG( eastl::string8().append_sprintf( "[%s %s %d] EbisuTelemetryServerIO Constructor STATE [%s].", __FUNCTION__, __FILE__, __LINE__, STATE_STRING[mState.ioState()]) );
        m_telemetryUpdateThread->Begin(&EbisuTelemetryServerIO::entryPoint, this, &param );
    }

    EbisuTelemetryServerIO::~EbisuTelemetryServerIO()
    {
        TELEMETRY_DEBUG( eastl::string8().append_sprintf( "[%s %s %d] EbisuTelemetryServerIO Destructor.", __FUNCTION__, __FILE__, __LINE__) );

        if( !(mState.ioState() == IOState::SERVERIO_STATE_FLUSH_COMPLETE || mState.ioState() == IOState::SERVERIO_STATE_FLUSH || mState.ioState() == IOState::SERVERIO_STATE_SENDING))
        {
            eastl::string8 errMsg;
            errMsg.append_sprintf("EbisuTelemetryServerIO Destructor. We're in an unexpected state [%s] on destruction. We will have no idea how many events we're losing in the TelemetrySDK", 
                STATE_STRING[mState.ioState()]);
            TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);

        }

        //At this point if we haven't pumped everything out then it's too late. Just shutdown.
        // Any lost events should get logged directly or at least a count of lost events should.
        { // Scoping for the lock
            EA::Thread::AutoFutex lock(getLock());
            //Pump once to make sure our Events Stored is up to date.
            telemetryApiUpdateHelper();
            mState.setIoState(IOState::SERVERIO_STATE_READY_TO_DESTRUCT);
        } 

        //Release the lock and then sleep for enough time for the pump to get called so it will exit.
        EA::Thread::ThreadSleep(TIME_TO_SLEEP*2);

        { //Scoping for the lock
            EA::Thread::AutoFutex lock(getLock());

            int32_t  res = telemetrySDKEventsStored();
            if (0 < res)
            {
                // we are missing some telemetry
                eastl::string8 errMsg;
                errMsg.append_sprintf("[%s %s %d] Telemetry Server DTOR: Events not sent [%d]", __FUNCTION__, __FILE__, __LINE__ , res);
                TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
                EA_FAIL_MSG(errMsg.c_str());
            }

            //Destroy the telemetry reader.
            m_telmReader.reset();

            if( mFilteredEvents > 0)
                TelemetryLogger::GetTelemetryLogger().logTelemetryWarning( eastl::string8().append_sprintf("[%s %s %d] TelemeterySDK filtered [%u] events.", __FUNCTION__, __FILE__, __LINE__, mFilteredEvents) );


            // Ask telemetry to shut down
            shutdownTelemetrySDK();
            
            if (m_telemetryUpdateThread)
            {
                delete m_telemetryUpdateThread;
                m_telemetryUpdateThread = NULL;
            }
        } // let go of the lock.

        //delete the lock.
        if(m_lock && m_lock->HasLock())
        {
            m_lock->Unlock();
        }
        if(m_lock)
        {
            delete m_lock;
            m_lock = NULL;
        }
        // The last persona we were hanging onto should get destructed as we go out of scope.
    }


//*************************************** PUBLIC API ***************************************
    
    void EbisuTelemetryServerIO::flushAndDestroyTelemetryReader()
    {
        TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] flushAndDestroyTelemetryReader called.", __FUNCTION__, __FILE__, __LINE__));

        // Public function that is called from the TelemetryAPIManager. So set the lock. The manager could also be called from 
        // the flush complete state. So we might already have the lock.
        EA::Thread::AutoFutex lock(getLock());

        EA_ASSERT_MSG( !m_waitingToFlush, "flushAndDestroyTelemetryReader() called while waiting to flush. This shoudl not happen.");

        switch(mState.ioState())
        {
        case IOState::SERVERIO_STATE_END:
        case IOState::SERVERIO_STATE_COUNTRY_DISABLED:
                //Is the users country is disabled or we're in the end sate then we don't need to flush. 
                //we just toss out the persona and go into FLUSH COMPLETE that will signal the manager
                // to give us a new persona.
                mState.setIoState( IOState::SERVERIO_STATE_FLUSH_COMPLETE);
                break;
        case IOState::SERVERIO_STATE_SENDING:
                mState.setIoState(IOState::SERVERIO_STATE_FLUSH);
                break;
        case IOState::SERVERIO_STATE_AUTHENTICATE:
        case IOState::SERVERIO_STATE_CONNECTED:
        case IOState::SERVERIO_STATE_NOT_CONNECTED:
        case IOState::SERVERIO_STATE_SHUTDOWN_AND_RESTART:
        case IOState::SERVERIO_STATE_STARTUP:
            {
                eastl::string8 wrnMsg;
                TELEMETRY_DEBUG( eastl::string8().append_sprintf("[%s %s %d] flushAndDestroyTelemetryReader() waiting to flush because we're in state [%s].", __FUNCTION__, __FILE__, __LINE__, STATE_STRING[mState.ioState()]).c_str());
                m_waitingToFlush = true;
                break;
            }
        case IOState::SERVERIO_STATE_FLUSH_COMPLETE:
        case IOState::SERVERIO_STATE_FLUSH:
        case IOState::SERVERIO_STATE_READY_TO_DESTRUCT:
            {
                eastl::string8 errMsg;
                errMsg.append_sprintf("[%s %s %d] flushAndDestroyTelemetryReader called in state %s this should not happen.",__FUNCTION__, __FILE__, __LINE__, STATE_STRING[mState.ioState()]);
                TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
                EA_FAIL_MSG(errMsg.c_str());
                //Do nothing. We're already in the flush or flush complete state. or we've destructed.
                //None of these should actually happen.
                break;
            }
        default:
            {
                eastl::string8 errMsg2;
                errMsg2.append_sprintf("[%s %s %d] flushAndDestroyTelemetryReader() mState.ioState() is an unknown state. Did someoen modify the enum? State was [%s].",__FUNCTION__, __FILE__, __LINE__, STATE_STRING[mState.ioState()]);
                TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg2);
                EA_FAIL_MSG(errMsg2.c_str());
                break;
            }
        }
    }

    void EbisuTelemetryServerIO::setTelemetryReader(eastl::shared_ptr<ITelemetryReader> telmReader)
    {
        TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] setTelemetryReader called.", __FUNCTION__, __FILE__, __LINE__));

        //This method can get called by the manger and then immediately after the manager calls flushAndDestroyTelemetryReader. 
        //We'll rely on flushAndDestroyTelemetryReader setting the m_waitingToFlush flag in that scenario.

        // Public function that is called from the TelemetryAPIManager. So set the lock. The manager could also be called from 
        // the flush complete state. So we might already have the lock.
        EA::Thread::AutoFutex lock(getLock());

        EA_ASSERT_MSG( !m_waitingToFlush, "setTelemetryReader() called while waiting to flush. This should not happen.");

        //This will release our shared pointer reference to the current reader which should cause it to destruct.
        m_telmReader = telmReader;

        if(IOState::SERVERIO_STATE_FLUSH_COMPLETE != mState.ioState())
        {
            eastl::string8 errMsg;
            errMsg.append_sprintf("[%s %s %d] setTelemetryReader called when not in state SERVERIO_STATE_FLUSH_COMPLETE. This shouldn't happen. State was [%s] ", __FUNCTION__, __FILE__, __LINE__, STATE_STRING[mState.ioState()]);
            TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
            EA_FAIL_MSG(errMsg.c_str());
            //Reset the whole system.  This might lose telemetry but is the safest way to get us back into a known state.
            mState.setIoState(IOState::SERVERIO_STATE_SHUTDOWN_AND_RESTART);
            return;
        }

        if( m_telemetryApiRef == NULL)
        {
            TelemetryLogger::GetTelemetryLogger().logTelemetryWarning(eastl::string8().append_sprintf("[%s %s %d] setTelemetryReader was called when m_telemetryApiRef was NULL..", __FUNCTION__, __FILE__, __LINE__));
            mState.setIoState(IOState::SERVERIO_STATE_SHUTDOWN_AND_RESTART);
        }

        //If we get here we're in the Flush state and we have a valid telemetry reference so we can just re authenticate with the new user.
        mState.setIoState(IOState::SERVERIO_STATE_AUTHENTICATE);
    }


//*************************************** PRIVATE STATIC(Thread entry) ***************************************

    intptr_t EbisuTelemetryServerIO::entryPoint( void* vServerIO )
    {
        EbisuTelemetryServerIO* const serverIO = static_cast<EbisuTelemetryServerIO*>(vServerIO);
        TELEMETRY_DEBUG( eastl::string8().append_sprintf( "[%s %s %d] [THREAD %d] Entry point.", __FUNCTION__, __FILE__, __LINE__, serverIO->m_telemetryUpdateThread->GetId()) );
        serverIO->telemetryPoll();
        return 0;
    }

//*************************************** PRIVATE MEMBER FUNCTIONS ***************************************

void EbisuTelemetryServerIO::stateMachine()
{
    // Ensure we don't get interrupted when processing our states.
    EA::Thread::AutoFutex lock(getLock());

    mState.stateProcessingStart();

    switch (mState.ioState())
    {
    case IOState::SERVERIO_STATE_STARTUP:
        configureTelemetrySDK();
        break;
    case IOState::SERVERIO_STATE_AUTHENTICATE:
        authenticateTelemetrySDK();
        break;
    case IOState::SERVERIO_STATE_COUNTRY_DISABLED:
        //If the users local changes then let's reset the system to see if the new local is disabled. 
        if( m_telmReader->locale() != m_locale)
        {
            m_locale = m_telmReader->locale();
            mState.setIoState(IOState::SERVERIO_STATE_AUTHENTICATE);
        }
        break;
    case IOState::SERVERIO_STATE_NOT_CONNECTED:
        connectToTelemetryServer();
        break;
    case IOState::SERVERIO_STATE_CONNECTED:
        // Don't process telemetry
        EA_ASSERT_MSG(m_telemetryApiRef,"Invalid telemetryAPI reference.");
        if(Telemetry::TelemetryApiControl(m_telemetryApiRef, 'halt', 0, NULL) >= TC_OKAY )
        {
            mState.setIoState( IOState::SERVERIO_STATE_SENDING);
            // Process telemetry
        }
        else
        {
            TelemetryLogger::GetTelemetryLogger().logTelemetryError(eastl::string8().append_sprintf("[%s %s %d] Telemetry Control: starting failed.", __FUNCTION__, __FILE__, __LINE__));
            mState.setIoState(IOState::SERVERIO_STATE_END);
            EA_FAIL_MSG("Unable to un-halt telemetry.");
        }
        break;
    case IOState::SERVERIO_STATE_SENDING:
        // Process telemetry
        EA_ASSERT_MSG(m_telemetryApiRef,"Invalid telemetryAPI reference."); // Should never happen

        // if nucleusid changes, re-authenticate
        if(mHashedNucleusId != m_telmReader->hashedNucleusId() || mNucleusId != m_telmReader->nucleusId())
        {
            TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] ServerIO Detected nucleus id change. oldnucid [%I64u], newnucid [%I64u], oldhash[%I64u], newhash [%I64u]."
                , __FUNCTION__, __FILE__, __LINE__ , mNucleusId, m_telmReader->nucleusId(), mHashedNucleusId, m_telmReader->hashedNucleusId()));
            mState.setIoState(IOState::SERVERIO_STATE_AUTHENTICATE);
        }
        else
        {
            if(m_waitingToFlush)
            {
                mState.setIoState(IOState::SERVERIO_STATE_FLUSH);
                m_waitingToFlush = false;
            }
            else
            {
                // ...and pump messages out if we can.
                sendTelemetryToSDK();
            }
        }
        break;
    case IOState::SERVERIO_STATE_FLUSH:
        //This will keep looping until the flush is complete, there was an error, or we get destroyed.
        flushTelemetryReaderAndSDK();
        break;
    case IOState::SERVERIO_STATE_FLUSH_COMPLETE:
        //Only call notify the first time we enter this state.
        if( mState.ioState() != mState.previousIoState())
        {
            //This will normally cause notify to call setTelemetryReader where we will set the new state.
            TelemetryAPIManager::getInstance().notify(TELEMETRY_MANAGER_EVENT_TELEMETRY_READER_FLUSH_COMPLETE);
        }
        break;
    case IOState::SERVERIO_STATE_SHUTDOWN_AND_RESTART:
        //Cleanup the old instance before we create a new one.
        // Any flushing should have already been done because we'll just destroy it all here.
        shutdownTelemetrySDK();
        // Don't process telemetry
        EA_ASSERT_MSG(!m_telemetryApiRef,"telemetryAPI reference shouldnt be valid here.");
        mState.setIoState( IOState::SERVERIO_STATE_STARTUP);
        break;
    case IOState::SERVERIO_STATE_END:
        // Shut the telemetrySDK down. We're in a bad state. We will restart if we get a login or logout
        // We also use the fact that this set's the m_telemetryApiRef to NULL to know if we shoudl just reauth or restart when we go to state flush complete.
        shutdownTelemetrySDK();
        if(m_waitingToFlush)
        {
            //If we get here and we're waiting to flush then something went wrong.
            // So we set the state to flush complete to send the notification back to the manager.
            // The we should trigger a re-authentication with the new persona.
            mState.setIoState(IOState::SERVERIO_STATE_FLUSH_COMPLETE);
            m_waitingToFlush = false;
        }
        EA_ASSERT_MSG(!m_telemetryApiRef,"telemetryAPI reference shouldnt be valid here.");
        break;
    case IOState::SERVERIO_STATE_READY_TO_DESTRUCT:
        //Don't do anything We're destructing. Should only be called once and only if this function was waiting on the lock when the destructor had it.
        TELEMETRY_DEBUG("StateMachine called while in ready to destruct state.");
        break;
    default:
        eastl::string8 errMsg;
        errMsg.append_sprintf("[%s %s %d] Invalid mState.ioState() %s. Did someone modify the enum?", __FUNCTION__, __FILE__, __LINE__, STATE_STRING[mState.ioState()]);
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
        EA_FAIL_MSG(errMsg.c_str());
        //Go to state end so we'll try to recover on the next login.
        mState.setIoState(IOState::SERVERIO_STATE_END);
        break;
    }

    //Don't use early returns.
    mState.stateProcessingEnd();
}

void EbisuTelemetryServerIO::connectToTelemetryServer()
{
    //This has a bit of a wacky return system.  It can return TELEMETRY3_ERROR_NULLPARAM, TRUE, or FALSE.
    int32_t res = Telemetry::TelemetryApiConnect(m_telemetryApiRef);

    switch( res)
    {
    case TELEMETRY3_ERROR_NULLPARAM:
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(eastl::string8().append_sprintf("[%s %s %d] m_telemetryApiRef was null when calling TelemetryApiConnect(). This should not happen. ", __FUNCTION__, __FILE__, __LINE__));
        EA_FAIL_MSG("m_telemetryApiRef was null when calling TelemetryApiConnect(). This should not happen. ");
        mState.setIoState(IOState::SERVERIO_STATE_SHUTDOWN_AND_RESTART);
    case TRUE: 
        //Successful connection.
        TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] Successfully called TelemetryApiConnect(). ", __FUNCTION__, __FILE__, __LINE__));
        mState.setIoState(IOState::SERVERIO_STATE_CONNECTED);
        break;
    case FALSE:
        //Could not create the http session.
        //TODO tmobbs we need to figure this out. Is this an actual connection fail as in no network connection?  If so we should probably keep retrying.
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(eastl::string8().append_sprintf("[%s %s %d] TelemetryApiConnect() returned FALSE. Shutting down.", __FUNCTION__, __FILE__, __LINE__));
        EA_FAIL_MSG("TelemetryApiConnect() returned FALSE. Shutting down.");
        mState.setIoState(IOState::SERVERIO_STATE_END);
        break;
    default:
        eastl::string8 errMsg;
        errMsg.append_sprintf("[%s %s %d] Unexpected return value from TelemetryApiConnect(). Return value was [%d].", __FUNCTION__, __FILE__, __LINE__, res);
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
        EA_FAIL_MSG(errMsg.c_str());
        break;
    }
}

void EbisuTelemetryServerIO::telemetryPoll()
{

    // While our state is not shutting down...
    while(!isShutdown())
    {
        // ...process state machine...
        stateMachine();

        // Nap for a while to give time to the queues to empty.
        EA::Thread::ThreadSleep(TIME_TO_SLEEP);
    } // poll loop

    TELEMETRY_DEBUG( eastl::string8().append_sprintf( "[%s %s %d] Telemetry poll FINISHED [%s].", __FUNCTION__, __FILE__, __LINE__, STATE_STRING[mState.ioState()]));
}

void EbisuTelemetryServerIO::shutdownTelemetrySDK()
{
    if (m_telemetryApiRef != NULL)
    {
        //This will cause any unsent telemetry to be lost.
        TelemetryApiDisconnect(m_telemetryApiRef);
        TelemetryApiDestroy(m_telemetryApiRef);
        m_telemetryApiRef = NULL;
    }
    TELEMETRY_DEBUG( eastl::string8().append_sprintf( "Telemetry Server: Shutdown.") );
}

void EbisuTelemetryServerIO::flushTelemetryReaderAndSDK()
{
    //This should clean out the telemetry reader.
    sendTelemetryToSDK();

    if( mState.ioState() != IOState::SERVERIO_STATE_FLUSH)
    {
        //Something went wrong in sendTelemetryToSDK we'll need a reset after letting the manager know we're done so destroy the m_telemetryApiRef.
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(eastl::string8().append_sprintf("[%s %s %d] There was an error when flushing telemetry from the persona.  Aborting flush. Telemetry will be lost.", __FUNCTION__, __FILE__, __LINE__));
        EA_FAIL_MSG("There was an error when flushing telemetry from the persona.  Aborting flush. Telemetry will be lost");
        shutdownTelemetrySDK();
        mState.setIoState(IOState::SERVERIO_STATE_FLUSH_COMPLETE);
        return;
    }

    //telemetryApiUpdateHelper was already called by sendTelemetryToSDK but we set the ApiCiontrol flush value to signal to Telemtery SDK to flush everything out. 
    //This will also cause a double pump which should help us flush events out faster.

    //We don't really care about the result.  It should just work. If it didn't it might not actually send anything on this loop.
    //Reset the flush every time beause it get's reset in the update.
    Telemetry::TelemetryApiControl(m_telemetryApiRef, 'flsh', 1, NULL);
    telemetryApiUpdateHelper();

    if( mState.ioState() != IOState::SERVERIO_STATE_FLUSH)
    {
        //Something went wrong in telemetryApiUpdateHelper we'll need a reset after letting the manager know we're done so destroy the m_telemetryApiRef.
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(eastl::string8().append_sprintf("[%s %s %d] There was an error when flushing telemetry from the TelemetrySDK.  Aborting flush. Telemetry will be lost.", __FUNCTION__, __FILE__, __LINE__));
        EA_FAIL_MSG("There was an error when flushing telemetry from the TelemetrySDK.  Aborting flush. Telemetry will be lost");

        shutdownTelemetrySDK();
        mState.setIoState(IOState::SERVERIO_STATE_FLUSH_COMPLETE);
        return;
    }

    int32_t numEventsRemaining = telemetrySDKEventsStored();
    int32_t v2Events = TelemetryApiStatus(m_telemetryApiRef, 'nume', NULL, 0);
    int32_t backbufferState = TelemetryApiStatus(m_telemetryApiRef, 'bbuf', NULL, 0);

    if(numEventsRemaining == 0 && v2Events == 0 && backbufferState ==0)
    {
        TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] Telemetry Flushing complete", __FUNCTION__, __FILE__, __LINE__));
        mState.setIoState(IOState::SERVERIO_STATE_FLUSH_COMPLETE);
    }
    else
    {
        TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] flushTelemetryReaderAndSDK() Still flushing.  v3[%d] v2[%d] backbuffer[%d].", __FUNCTION__, __FILE__, __LINE__
            , numEventsRemaining, v2Events, backbufferState));
    }
    //TODO Tmobbs currently we never give up on the flush unless we get destroyed. This is what we want in most cases. If we can't connect just keep trying until we shutdown.
    // We probably want to add some logic to control the number of persona's we have and how full they are to avoid using crazy amounts of memory.
}

void EbisuTelemetryServerIO::telemetryApiUpdateHelper()
{
    if( mState.ioState() != IOState::SERVERIO_STATE_SENDING && mState.ioState() != IOState::SERVERIO_STATE_FLUSH )
    {
        TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] telemetryApiUpdateHelper called in state [%s]. Skipping update.", __FUNCTION__, __FILE__, __LINE__ , STATE_STRING[mState.ioState()]).c_str());
        return;
    }

    int32_t res = TelemetryApiUpdate(m_telemetryApiRef);
    switch(res)
    {
    case TC_OKAY:
        break;
    case TELEMETRY_ERROR_NOTCONNECTED:
        TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] TelemetryApiUpdate returned TELEMETRY_ERROR_NOTCONNECTED. Must have lost connection. reconnecting.", __FUNCTION__, __FILE__, __LINE__ ));
        mState.setIoState( IOState::SERVERIO_STATE_NOT_CONNECTED); // Enter the CONNECT flow
        break;
    case TELEMETRY3_ERROR_NULLPARAM:
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(eastl::string8().append_sprintf("[%s %s %d] m_telemetryApiRef was null. This should not happen", __FUNCTION__, __FILE__, __LINE__));
        EA_FAIL_MSG("m_telemetryAPIRef was null. This should not happen");
        mState.setIoState( IOState::SERVERIO_STATE_SHUTDOWN_AND_RESTART);
        break;
    case TELEMETRY_ERROR_THREAD_UNSAFE:
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(eastl::string8().append_sprintf("[%s %s %d] TelemetryAPIupdate called from competing threads. This should not happen.", __FUNCTION__, __FILE__, __LINE__));
        EA_FAIL_MSG("TelemetryAPIupdate called from competing threads. This should not happen.");
        mState.setIoState( IOState::SERVERIO_STATE_END);
    default:
        eastl::string8 errMsg;
        errMsg.append_sprintf("[%s %s %d] Unknown error returned from TelemetryApiUpdate().  Error code is %d.", __FUNCTION__, __FILE__, __LINE__, res);
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
        EA_FAIL_MSG(errMsg.c_str());
        // A new failure case was added. Not sure what to do so go to state end unless it was a positive return code.
        if( res < TC_OKAY)
        {
            mState.setIoState(IOState::SERVERIO_STATE_END);
        }
        break;
    }

#if DEBUG
    if (!isTelemetrySDKBufferEmpty())
    {
        TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] Telemetry is not empty after calling TelmetryAPIUpdate. Number of stored events [%d]", __FUNCTION__, __FILE__, __LINE__, telemetrySDKEventsStored()));
    }
#endif
}

void EbisuTelemetryServerIO::sendTelemetryToSDK()
{
    if (!m_telmReader)
    {
        // Shouldn't happen.  m_telemetryApiRef is null.
        eastl::string8 errMsg;
        errMsg.append_sprintf("[%s %s %d] m_telemetryApiRef was NULL when calling sendTelemetryToSDK. Reseting the system.", __FUNCTION__, __FILE__, __LINE__);
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
        EA_FAIL_MSG(errMsg.c_str());
        mState.setIoState(IOState::SERVERIO_STATE_END);
        return;
    }

    if (!canSendTelemetry())
    {
        // Can't send telemetry. End the server.
        eastl::string8 errMsg;
        errMsg.append_sprintf("[%s %s %d] Can't send telemetry. Going to state SERVERIO_STATE_END.", __FUNCTION__, __FILE__, __LINE__);
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
        EA_FAIL_MSG(errMsg.c_str());
        mState.setIoState(IOState::SERVERIO_STATE_END);
        return;
    }

    TelemetryEvent nextEvent = m_telmReader->readTelemetry();

    int32_t numRetries = 0;

    //TODO tmobbs add a max number of events to insert at once.
    while((mState.ioState() == IOState::SERVERIO_STATE_SENDING || mState.ioState() == IOState::SERVERIO_STATE_FLUSH) && canSendTelemetry() && !nextEvent.isEmpty() )
    {
        if( numRetries > NUM_TRIES )
        {
            //We've been trying to flush out the telemetry SDK and it's still full. Somethings wrong. Let's just shutdown until the next login/out.
            eastl::string8 errMsg;
            errMsg.append_sprintf("[%s %s %d] sendTelemetryToSDK() gave up after %d tries trying to pump out a full telemetrySDK buffer. Going to state SERVERIO_STATE_END.", __FUNCTION__, __FILE__, __LINE__, numRetries);
            TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
            EA_FAIL_MSG(errMsg.c_str());
            mState.setIoState(IOState::SERVERIO_STATE_END);
            break;
        }

        TelemetryApiEvent3T event3;

        //This is a bit bad. If we loop the same event we deserialize it each time.
        nextEvent.deserializeToEvent3(event3);

        // res will contain either:
        // - the number of bytes written or
        // - a negative value (ERROR)
        // queue the event.
        int32_t res = TelemetryApiSubmitEvent3Ex(m_telemetryApiRef, &event3, NUM_ADDITIONAL_ATTRIBUTES);
        if( res > 0 )
        {
            //We're successful. Grab the next event.
            nextEvent = m_telmReader->readTelemetry();
            numRetries = 0;
        }
        else
        {
            // error messages buffer
            eastl::string8 errMsg;
            switch( res )
            {
                case TELEMETRY3_ERROR_FULL:
                {
                    //Pump the sdk to clear some events outs.
                    telemetryApiUpdateHelper();
                    TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d]  Buffer full res [%d], trying to process telemetry [%d of %d] times.", __FUNCTION__, __FILE__, __LINE__, res, numRetries, NUM_TRIES));
                    ++numRetries;
                }
                //Don't grab a new event. We'll loop around and process this one again.
                break;
                case TELEMETRY3_ERROR_NULLPARAM:
                {
                    // Shouldn't happen.  m_telemetryApiRef is null.
                    errMsg.append_sprintf("[%s %s %d] m_telemetryApiRef was NULL when calling TelemetryApiSubmitEvent3Ex(). Resting the system.", __FUNCTION__, __FILE__, __LINE__);
                    TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
                    EA_FAIL_MSG(errMsg.c_str());
                    mState.setIoState(IOState::SERVERIO_STATE_SHUTDOWN_AND_RESTART);
                }
                break;
                case TELEMETRY3_ERROR_NOTVER3:
                case TELEMETRY3_ERROR_INVALIDTOKEN:
                case TELEMETRY3_ERROR_TOOBIG:
                {
                    //Event was bad. Log an error and log the event as lost.
                    errMsg.append_sprintf("[%s %s %d] Telemetry Event can't be processed by the TelemetrySDK. Error [%d]", __FUNCTION__, __FILE__, __LINE__, res);

                    TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
                    TelemetryLogger::GetTelemetryLogger().logLostTelemetryEvent(nextEvent);
                    EA_FAIL_MSG(errMsg.c_str());
                    //Grab the next event.
                    nextEvent = m_telmReader->readTelemetry();
                    numRetries = 0;
                }
                break;
                case TELEMETRY3_ERROR_FILTERED:
                {
                    TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] Event was filtered by the TelemetrySDK", __FUNCTION__, __FILE__, __LINE__));
                    ++mFilteredEvents;
                    nextEvent = m_telmReader->readTelemetry();
                    numRetries = 0;
                }
                break;
                case TELEMETRY_ERROR_UNKNOWN:
                {
                    //This is really bad. It means the telemetry SDK ran out of memory.
                    eastl::string8;
                    errMsg.append_sprintf("[%s %s %d] TelemetryApiSubmitEvent3Ex() returned TELEMETRY_ERROR_UNKNOWN. This indicates that it ran out of memory. Resetting the system.", __FUNCTION__, __FILE__, __LINE__);
                    TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
                    EA_FAIL_MSG(errMsg.c_str());
                    //Guess we'll try a restart
                    mState.setIoState(IOState::SERVERIO_STATE_SHUTDOWN_AND_RESTART);
                }
                break;
                default:
                {
                    // Should not happen. Unexpected return value.
                    errMsg.append_sprintf("[%s %s %d] TelemetryApiSubmitEvent3Ex() returned an unexpected error [%d]. Did the TelemetrySDK get upgraded? Going to SERVERIO_STATE_END", __FUNCTION__, __FILE__, __LINE__, res);
                    TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
                    EA_FAIL_MSG(errMsg.c_str());
                    //Shutdown till next login/out
                    mState.setIoState(IOState::SERVERIO_STATE_END);
                }
                break;
            }
        }
    } // while

    if( !nextEvent.isEmpty() )
    {
        // re-add event to its queue
        TelemetryLogger::GetTelemetryLogger().logTelemetryError("We're re-adding a telemetry event to its queue owed to an exceptional condition during processing of this event.");
        m_telmReader->reInsertTelemetryEvent(nextEvent);
    }

    // pump the telemetry SDK. (will skip if the state has changed)
    telemetryApiUpdateHelper();

}

void EbisuTelemetryServerIO::configureTelemetrySDK()
{
    // error message buffer
    eastl::string8 errMsg;

    // Create a m_telemetryApiRef 
    m_telemetryApiRef = Telemetry::TelemetryApiCreate(TELMETRYSDK_BUFFER_SIZE);

    if (m_telemetryApiRef == NULL)
    {
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(eastl::string8().append_sprintf(" [FATAL][%s %s %d] Telemetry SDK instance could not be created. TelemetryApiCreateEx returned NULL", __FUNCTION__, __FILE__, __LINE__));
        EA_FAIL_MESSAGE("[FATAL] Telemetry SDK instance could not be created. TelemetryApiCreateEx returned NULL");
        // This state will retry if we get a login or logout.
        mState.setIoState(IOState::SERVERIO_STATE_END);
        return;
    }
    int32_t res = 0;
    // Synchronize anonymous telemetry with server's time
    if( (res = Telemetry::TelemetryApiControl(m_telemetryApiRef, 'stim', 1, NULL)) < TC_OKAY)
    {
        errMsg.append_sprintf("[%s %s %d] Telemetry Control: Synchronize anonymous telemetry with server's time failed. [%d]", __FUNCTION__, __FILE__, __LINE__, res);
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
        EA_FAIL_MSG(errMsg.c_str());
        // Just continue.  Should just mean the timestamps are messed up.
    }

    // Truncate the IP on the server
    if( (res = TelemetryApiControl(m_telemetryApiRef, 'tcip', 1, NULL)) < TC_OKAY )
    {
        errMsg.append_sprintf("[%s %s %d] Telemetry Control:'tcip' returned error [%d].", __FUNCTION__, __FILE__, __LINE__, res);
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
        EA_FAIL_MSG(errMsg.c_str());
        // Just continue.  We'll be sending full ip addresses.
    }

    // Setting Spam level for protohttp
    if( (res = Telemetry::TelemetryApiControl(m_telemetryApiRef, 'spam', TelemetryConfig::spamLevel(), NULL)) < TC_OKAY)
    {
        errMsg.append_sprintf("[%s %s %d] Telemetry Control:'spam' returned error [%d].", __FUNCTION__, __FILE__, __LINE__, res);
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
        EA_FAIL_MSG(errMsg.c_str());
        // Just continue. Spam should almost almost always be off which is the default.
    }

    // Setup telemetry filtering by country. This always returns 0 so no point in error handling.
    TelemetryApiControl(m_telemetryApiRef, 'cdbl', 0, reinterpret_cast<void*>(const_cast<char*>(TelemetryConfig::GetCountryFilter())));

    //Setup telemetry filtering by hook. No return value. 
    TelemetryApiFilter(m_telemetryApiRef, TelemetryConfig::getHooksFilter());

    Telemetry::TelemetryReleaseTypeE releaseType =  (TelemetryConfig::isBOI())?TELEMETRY_RELEASE_BETA:TELEMETRY_RELEASE_PRODUCTION;
    //TODO should we put in a real mdm Project ID for the Origin Client?
    // Setup additional info required for the Aquarius data format
    Telemetry::TelemetryApiCoreParameters( m_telemetryApiRef, 0/*MDM Project ID*/, releaseType/*Release version*/, EALS_VERSION_P_DELIMITED/*Build Version*/, 0/*Aquarius player ID*/);

    if( (res = Telemetry::TelemetryApiControl(m_telemetryApiRef, 'thrs', BUFFER_PERCENTAGE_FULL, NULL)) < TC_OKAY)
    {
        eastl::string8 errMsg;
        errMsg.append_sprintf("[%s %s %d] Telemetry Control:'thrs' returned error [%d].", __FUNCTION__, __FILE__, __LINE__, res);
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
        EA_FAIL_MSG(errMsg.c_str());
        // Just continue. Hopefully the default is fine.
    }

    if( (res = Telemetry::TelemetryApiControl(m_telemetryApiRef, 'time', BUFFER_SEND_DELAY, NULL)) < TC_OKAY )
    {
        eastl::string8 errMsg;
        errMsg.append_sprintf("[%s %s %d] Telemetry Control:'time' returned error [%d].", __FUNCTION__, __FILE__, __LINE__, res);
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
        EA_FAIL_MSG(errMsg.c_str());
        // Just continue. Hopefully the default is fine.
    }

    // set callback functions (All callbacks return void)
    //TODO tmobbs future ticket to figure out how we will handle the recovery callback.
    Telemetry::TelemetryApiSetDataRecoveryCallback(m_telemetryApiRef, dataRecoveryCallback);
    Telemetry::TelemetryApiSetLoggerCallback(m_telemetryApiRef, loggerCallback);
    Telemetry::TelemetryApiSetSnapshotFreeCallback(m_telemetryApiRef,setSnapshotFreeCallback);


    mState.setIoState(IOState::SERVERIO_STATE_AUTHENTICATE);
}


EA::Thread::Futex& EbisuTelemetryServerIO::getLock()
{
    return *m_lock; 
} 

bool EbisuTelemetryServerIO::canSendTelemetry() const
{
    // is the server EnablED?
    return (TelemetryApiStatus(m_telemetryApiRef,'nabl',NULL,0) > 0); 
}

size_t EbisuTelemetryServerIO::telemetrySDKEventsStored() const
{
    //We only check V3 events because we don't use V2 events.
    int32_t num3 = TelemetryApiStatus(m_telemetryApiRef, 'num3', NULL, 0); // number of v3 events
    return num3;
}

bool EbisuTelemetryServerIO::isTelemetrySDKBufferEmpty() const
{
    // 'mpty' - return 1 if empty, 0 otherwise
    return 1==TelemetryApiStatus(m_telemetryApiRef, 'mpty', NULL, 0); // is the buffer Empty?
}

bool EbisuTelemetryServerIO::isDisabledCountry()
{
    bool res = 0 == TelemetryApiStatus(m_telemetryApiRef, 'ctry', NULL, 0); // is the current country disabled?
    if(res)
    {
        char locl[3] = "";
        uint32_t res = TelemetryApiStatus(m_telemetryApiRef, 'locl', locl, 3); // retrieve the locale for logging
        if( 0 == res)
        {
            TelemetryLogger::GetTelemetryLogger().logTelemetryWarning(eastl::string8().append_sprintf("[%s %s %d] Not sending telemetry because we're from a banned country locale: [%s].", __FUNCTION__, __FILE__, __LINE__, locl));
        }
        else
        {
            TelemetryLogger::GetTelemetryLogger().logTelemetryError(eastl::string8().append_sprintf("[%s %s %d] Not sending telemetry because we're from a banned country, couldn't get country [%d].", __FUNCTION__, __FILE__, __LINE__, res));
        }
    }
    return res;
}

bool EbisuTelemetryServerIO::isShutdown() const
{
    return mState.ioState() == IOState::SERVERIO_STATE_READY_TO_DESTRUCT;
}

void EbisuTelemetryServerIO::authenticateTelemetrySDK()
{
    TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] EbisuTelemetryServerIO::authenticateTelemetrySDK. persona [%I64u].", __FUNCTION__, __FILE__, __LINE__, m_telmReader->nucleusId()));

    eastl::string8 hashedNucleusIdAsString;
    hashedNucleusIdAsString.append_sprintf( "%I64u", m_telmReader->hashedNucleusId()); 
    eastl::string8 hashedMacString = OriginTelemetry::TelemetryConfig::uniqueMachineHash();

    int32_t res = Telemetry::TelemetryApiAuthentAnonymousEx(
        m_telemetryApiRef
        , TelemetryConfig::telemetryServer() //pDestIP
        , TelemetryConfig::telemeteryPort() //iDestPort 
        , m_telmReader->locale() //uLocale
        , OriginTelemetry::TelemetryConfig::telemetryDomain() //pDomain
        , static_cast<uint32_t>(EA::StdC::GetTime() / UINT64_C(1000000000)) //uTime This is used to generate the sessionID as uTime combined with pPlayerID.  We override this anyway.
        , hashedNucleusIdAsString.c_str() //pPlayerID 
        , hashedMacString.c_str() //pMAC
        , m_telmReader->nucleusId() /*uNucleusPersonaID*/);

    switch(res)
    {
    case TC_OKAY:
        TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] Successfully authenticated the TelemetrySDK with nucleusID %I64u.", __FUNCTION__, __FILE__, __LINE__, m_telmReader->nucleusId()));
        mNucleusId = m_telmReader->nucleusId();
        mHashedNucleusId = m_telmReader->hashedNucleusId();
        setSession();
        break;
    case TELEMETRY_ERROR_UNKNOWN:
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(eastl::string8().append_sprintf("[%s %s %d] TelemetryApiAuthentAnonymousEx() returned TELEMETRY_ERROR_UNKNOWN. This indicates that an invalid Telemetry Server Address was used.", __FUNCTION__, __FILE__, __LINE__));
        EA_FAIL_MSG("TelemetryApiAuthentAnonymousEx() returned TELEMETRY_ERROR_UNKNOWN. This indicates that an invalid Telemetry Server Address was used.");
        //This should never happen. If it does we just go into a shutdown state.
        mState.setIoState(IOState::SERVERIO_STATE_END);
        break;
    case TC_MISSING_PARAM:
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(eastl::string8().append_sprintf("[%s %s %d] TelemetryApiAuthentAnonymousEx() returned TC_MISSING_PARAM. This means that m_telemetryApiRef was NULL.", __FUNCTION__, __FILE__, __LINE__));
        EA_FAIL_MSG("TelemetryApiAuthentAnonymousEx() returned TC_MISSING_PARAM. This means that m_telemetryApiRef was NULL.");
        //This should never happen. If it does let's set the state back to startup to recreate m_telemetryAPIRef.
        mState.setIoState(IOState::SERVERIO_STATE_SHUTDOWN_AND_RESTART);
        break;
    default:
        eastl::string8 errMsg;
        errMsg.append_sprintf("[%s %s %d] Unknown error returned from TelemetryApiAuthentAnonymousEx().  Error code is %d.", __FUNCTION__, __FILE__, __LINE__, res);
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
        EA_FAIL_MSG(errMsg.c_str());
        // This should never happen. If it does TelemetryApiAuthentAnonymousEx was update to return a new state.
        //If a new success state was added we'll change the state again below the switch.
        mState.setIoState(IOState::SERVERIO_STATE_END);
        break;
    }

    if(res >= TC_OKAY)
    {
        if (isDisabledCountry())
        {
            m_locale = m_telmReader->locale();
            mState.setIoState(IOState::SERVERIO_STATE_COUNTRY_DISABLED);
        }
        else
        {
            mState.setIoState(IOState::SERVERIO_STATE_NOT_CONNECTED);
        }
    }
}


void EbisuTelemetryServerIO::setSession()
{
    if (m_sessionID == "")
    {
        const size_t INDEX = 8;
        auto myTime = NonQTOriginServices::Utilities::printUint64AsHex(EA::StdC::GetTime());
        auto myMachineHash = OriginTelemetry::TelemetryConfig::uniqueMachineHash();
        m_sessionID = myTime.substr(0, INDEX) + myMachineHash.substr(INDEX);
        TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] Setting SessionID to [%s].", __FUNCTION__, __FILE__, __LINE__, m_sessionID.c_str()));
    }
    //Set the session id to always be the same for the same execution of the client.
    Telemetry::TelemetryApiSetSessionID(m_telemetryApiRef, m_sessionID.c_str());
}

void EbisuTelemetryServerIO::IOState::State::setIoState( IOState::SERVERIO_STATE val )
{
    m_serverioState = val;
    TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] Setting telemetryServerIO state to [%s].", __FUNCTION__, __FILE__, __LINE__, STATE_STRING[m_serverioState]));
}

void EbisuTelemetryServerIO::IOState::State::stateProcessingStart()
{
    m_stateAtStart = m_serverioState;
    if(m_serverioState != m_previousState)
    {
        // only print debugging state when state changes - to avoid spamming the output window.
        TELEMETRY_DEBUG(eastl::string8().append_sprintf("[%s %s %d] Processing State: [%s].", __FUNCTION__, __FILE__, __LINE__,STATE_STRING[m_serverioState]));
    }
}

void EbisuTelemetryServerIO::IOState::State::stateProcessingEnd()
{
    m_previousState = m_stateAtStart;
}

} // namespace


