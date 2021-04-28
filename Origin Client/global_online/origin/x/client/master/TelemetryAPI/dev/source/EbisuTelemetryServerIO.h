///////////////////////////////////////////////////////////////////////////////
// EbisuTelemetryServerIO.h
//
// Telemetry SDK control class.  Uses the passed in telemetry reader to get telemetry and feed it to the Telemetry SDK
//
// Owner Origin Dev Support
// Copyright (c) Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef EBISUTELEMETRYSERVERIO_H
#define EBISUTELEMETRYSERVERIO_H

#include "EABase/eabase.h"
#include "EASTL/shared_ptr.h"
#include "EASTL/string.h"
//Telemetry SDK interface
#include "TelemetrySDK/telemetryapi.h"

//Forward Declarations
namespace EA
{
    namespace Thread
    {
        class Thread;
        class Futex;
    }
}


namespace Telemetry
{
    class TelemetryApiRefT;
}

namespace OriginTelemetry
{
    //forward declare
    class ITelemetryReader;
    class TelemetryEvent;

    class EbisuTelemetryServerIO
    {
        friend class IOState;
    public:
        /// Constructor.
        EbisuTelemetryServerIO(eastl::shared_ptr<ITelemetryReader> telmReader);

        /// Destructor.
        virtual ~EbisuTelemetryServerIO();

        ///Signal from the manger to flush out all telemetry from the current reader and then notify the manager when done.
        void flushAndDestroyTelemetryReader();

        ///Signal from the manger to flush out all telemetry from the current reader and then notify the manager when done.
        void setTelemetryReader(eastl::shared_ptr<ITelemetryReader> telmReader);

    private:

        //Private class for encapsulating the states. (basically an internal private namespace)
        class IOState
        {
        public:
            // Add new enumeration members here please, don't forget the ending '\'
#define FOREACH_STATE(IOSTATE) \
    IOSTATE(SERVERIO_STATE_STARTUP)   \
    IOSTATE(SERVERIO_STATE_AUTHENTICATE)   \
    IOSTATE(SERVERIO_STATE_NOT_CONNECTED)   \
    IOSTATE(SERVERIO_STATE_COUNTRY_DISABLED)   \
    IOSTATE(SERVERIO_STATE_CONNECTED)   \
    IOSTATE(SERVERIO_STATE_SENDING)   \
    IOSTATE(SERVERIO_STATE_FLUSH)   \
    IOSTATE(SERVERIO_STATE_FLUSH_COMPLETE)   \
    IOSTATE(SERVERIO_STATE_SHUTDOWN_AND_RESTART)   \
    IOSTATE(SERVERIO_STATE_END)   \
    IOSTATE(SERVERIO_STATE_READY_TO_DESTRUCT)   \
    IOSTATE(SERVERIO_STATE_INVALID)   

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

            // Generate enumeration
            enum SERVERIO_STATE {
                FOREACH_STATE(GENERATE_ENUM)
            };

            ///Class to encapsulate the serverIO state.
            class State
            {
            public:
                /// Constructor
                State():m_serverioState(SERVERIO_STATE_STARTUP), m_previousState(SERVERIO_STATE_INVALID){}
                /// State Accessor
                IOState::SERVERIO_STATE ioState() const{return m_serverioState;}
                /// State Setter
                void setIoState(IOState::SERVERIO_STATE val);
                /// Get previous State
                IOState::SERVERIO_STATE previousIoState() const{return m_previousState;}
                /// State function for logging and managing m_previousState
                void stateProcessingStart();
                void stateProcessingEnd();
            private:
                IOState::SERVERIO_STATE m_serverioState; 
                //State the last time the state machine ran through the loop.
                IOState::SERVERIO_STATE m_previousState; 
                //Used for storing what the state was at the start of the state machine.
                IOState::SERVERIO_STATE m_stateAtStart;
            };
        };

        //Disable Default Constructor
        EbisuTelemetryServerIO();

        // Specifically disable things we don't want available
        EbisuTelemetryServerIO( const EbisuTelemetryServerIO&);
        EbisuTelemetryServerIO& operator=(const EbisuTelemetryServerIO &);
        EbisuTelemetryServerIO& operator&(const EbisuTelemetryServerIO &);

        //// Thread entry point for processing telemetry
        static intptr_t entryPoint( void* vServerIO = NULL);

        /// \brief authenticate credentials with telemetry server
        void authenticateTelemetrySDK();

        /// \brief process creation of telemetryApiRef
        void configureTelemetrySDK();

        /// Return if this serverio has finished shutting down.
        bool isShutdown() const;

        /// Get the futuex for the current object
        EA::Thread::Futex &getLock();

        /// Wrapper for calling TelemetryApiUpdate that handles all the errors.
        /// No Return. It will change mState if something went wrong.
        void telemetryApiUpdateHelper();

        /// \brief Returns status. Parameter depending.
        int32_t status(int32_t iSelect) const;

        /// Pumps the TelemetrySDK until it's Empty.
        void flushTelemetryReaderAndSDK();
        
        /// Destroy our TelemerySDK instance.
        void shutdownTelemetrySDK();

        /// Submits telemetry event to the SDK
        void sendTelemetryToSDK();

        /// \brief return true if we are in a unauthorized country otherwise false.
        bool isDisabledCountry();

        /// \brief returns true if telemetry can be sent to the server, otherwise false
        bool canSendTelemetry() const;

        /// \brief returns number of stored telemetry events stored
        size_t telemetrySDKEventsStored() const;

        /// \brief returns true if the buffer is empty
        inline bool isTelemetrySDKBufferEmpty() const;

        /// Background thread for sending telemetry to the Telemetry SDK and pumping it.
        void telemetryPoll();

        /// controls server's state.
        void stateMachine();

        /// \brief handles telemetry buffer full error. 
        /// Retries pumping the stored events to the server and 
        /// re queues the last event to try pumping again
        /// \param bytes written counter
        /// \param events counter
        /// \param event to re-queue/re-send
        int32_t handleSdkBufferFull(size_t& bytesWritten, size_t& numEvents, Telemetry::TelemetryApiEvent3T& event3);

        /// Helper function for handling connecting to the telemetry server.
        void connectToTelemetryServer();

        /// Helper for making sure the telemetry session is always unique and the same for a run of the client.
        void setSession();

        /// \brief iReader interface
        eastl::shared_ptr<ITelemetryReader> m_telmReader;

        /// \brief lock
        EA::Thread::Futex *m_lock;

        /// \brief pointer to telemetryAPI object.
        Telemetry::TelemetryApiRefT* m_telemetryApiRef;

        /// \brief Thread where all the process is performed.
        EA::Thread::Thread* m_telemetryUpdateThread;

        /// \brief state machine state
        IOState::State mState;

        /// \brief used to detect change of persona in order to perform authentication
        uint64_t mNucleusId;
        /// \brief used to detect change of persona in order to perform authentication
        uint64_t mHashedNucleusId;

        /// variable to store the local in the country disabled state, so we can detect a change and start sending telemetry.
        uint32_t m_locale;

        //Flag to indicate if we're waiting to flush a persona while initializing the system.
        bool m_waitingToFlush;

        //Number of events filtered by the telemetrySDK based on the filter rules that we passed in.
        uint32_t mFilteredEvents;

        eastl::string8 m_sessionID;

    };
}


#endif // EBISUTELEMETRYSERVERIO_H
