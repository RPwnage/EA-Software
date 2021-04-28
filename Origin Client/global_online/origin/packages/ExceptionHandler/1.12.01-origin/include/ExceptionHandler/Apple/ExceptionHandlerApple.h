///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerApple.h
// 
// Copyright (c) 2011, Electronic Arts. All Rights Reserved.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#ifndef EXCEPTIONHANDLER_APPLE_EXCEPTIONHANDLERAPPLE_H
#define EXCEPTIONHANDLER_APPLE_EXCEPTIONHANDLERAPPLE_H


#include <ExceptionHandler/ExceptionHandler.h>
#include <EACallstack/Context.h>
#include <EASTL/fixed_string.h>
#include <eathread/eathread_atomic.h>
#include <time.h>
#include <mach/mach_types.h>



extern "C" void* MachThreadFunctionStatic(void*);
extern "C" int catch_mach_exception_raise_state_identity_ea(mach_port_t, mach_port_t, mach_port_t, exception_type_t, mach_exception_data_type_t*, mach_msg_type_number_t, int*, thread_state_t, mach_msg_type_number_t, thread_state_t, mach_msg_type_number_t*);



namespace EA
{
    namespace Debug
    {
        class ReportWriter;
        class ExceptionHandler;

        /// ExceptionHandlerApple
        /// 
        /// Implements an exception handler for Unix PPU (the primary PowerPC) exceptions.
        /// Exception handling for the Unix SPU processors is not supported by Sony 
        /// as of this writing.
        ///
        class ExceptionHandlerApple
        {
        public:
            ExceptionHandlerApple(ExceptionHandler* pOwner);
           ~ExceptionHandlerApple();

            bool SetEnabled(bool state);
            bool IsEnabled() const;

            bool SetMode(ExceptionHandler::Mode mode);

            void SetAction(ExceptionHandler::Action action, int returnValue = ExceptionHandler::kDefaultTerminationReturnValue);
            ExceptionHandler::Action GetAction(int* pReturnValue = NULL) const;

            intptr_t RunTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught);

            // Provides detailed information about the exception. This struct is platform-specific.
            struct ExceptionInfoApple : public ExceptionHandler::ExceptionInfo
            {
                uint32_t mCPUExceptionId;                // CPU-level exception ID.
                uint32_t mCPUExceptionDetail;            // Fault Status Register on ARM; CPU exception Id subtype on x86.
                int64_t  mMachExceptionDetail[4];        // The kernel exception code information, which is an array
                int      mMachExceptionDetailCount;      //    of integers because that's how it's given to us.
                uint64_t mExceptionCause;                // For Mach exceptions this is EXC_BAD_ACCESS, EXC_BAD_INSTRUCTION, etc.

                ExceptionInfoApple();
            };

            const ExceptionInfoApple* GetExceptionInfo() const
                { return &mExceptionInfoApple; }

            void SimulateExceptionHandling(int exceptionType); // exceptionType is one of enum ExceptionType or some non-enumerated custom type.

            // Deprecated functions. use GetExceptionInfo instead.
                size_t                        GetExceptionCallstack(void* pCallstackArray, size_t capacity) const;
                const EA::Callstack::Context* GetExceptionContext() const;
                EA::Thread::ThreadId          GetExceptionThreadId(EA::Thread::SysThreadId& sysThreadId, char* threadName, size_t threadNameCapacity) const;
                size_t                        GetExceptionDescription(char* buffer, size_t capacity);
            // End deprecated functions.

        protected:
            friend class ExceptionHandler;
            
        protected:
            ExceptionHandler*               mpOwner;
            bool                            mbEnabled;
            bool                            mbExceptionOccurred;
            ExceptionHandler::Mode          mMode;
            ExceptionHandler::Action        mAction;
            int                             mnTerminateReturnValue;
            EA::Thread::AtomicInt32         mbHandlingInProgress;
            ExceptionInfoApple              mExceptionInfoApple;
            static ExceptionHandlerApple*   mpThis;
            
        protected: // kModeCPP-specific
            void HandleCaughtCPPException(std::exception& e);
    
        protected: // kModeVectored-specific (Mach-specific)
            friend void* ::MachThreadFunctionStatic(void*);
            friend int ::catch_mach_exception_raise_state_identity_ea(mach_port_t, mach_port_t, mach_port_t, exception_type_t, 
                                                                      mach_exception_data_type_t*, mach_msg_type_number_t, int*, thread_state_t,
                                                                      mach_msg_type_number_t, thread_state_t, mach_msg_type_number_t*);
            // Structs
            struct OldExceptionPorts
            {
                enum { kOldExceptionPortsCount = 8 };

                OldExceptionPorts();
                
                mach_msg_type_number_t mCount;
                exception_mask_t       mMasks[kOldExceptionPortsCount];
                exception_handler_t    mPorts[kOldExceptionPortsCount];
                exception_behavior_t   mBehaviors[kOldExceptionPortsCount];
                thread_state_flavor_t  mFlavors[kOldExceptionPortsCount];
            };
            
            // Data
            bool              mbMachHandlerInitialized;
            mach_port_t       mExceptionPort;
            OldExceptionPorts mOldExceptionPorts;
            volatile bool     mbMachThreadShouldContinue;
            pthread_t         mMachThread;

            // Functions
            bool          InitMachHandler();         // MachHandler can be initialized only once, until it is shutdown.
            void          ShutdownMachHandler();
            void*         MachThreadFunction();
            kern_return_t HandleMachException(mach_port_t exceptionPort, mach_port_t thread, mach_port_t task, exception_type_t exceptionType, 
                                              mach_exception_data_type_t* pExceptionDetail, mach_msg_type_number_t exceptionDetailCount, 
                                              int* pFlavor, thread_state_t pOldState, mach_msg_type_number_t oldStateCount, thread_state_t pNewState,
                                              mach_msg_type_number_t* pNewStateCount);
            kern_return_t ForwardMachException(mach_port_t thread, mach_port_t task, exception_type_t exceptionType,
                                               mach_exception_data_t pExceptionDetail, mach_msg_type_number_t exceptionDetailCount);
            void ReadThreadExceptionContext(mach_port_t thread, thread_state_t pOldState);
            void GetMachExceptionDescription(String& sDescription);

        protected: // kModeSignalHandling-specific
            bool mbSignalHandlerInitialized;
            
            bool InitSignalHandler();         // SignalHandler can be initialized only once, until it is shutdown.
            void ShutdownSignalHandler();
        };


    } // namespace Debug

} // namespace EA


#endif // Header include guard













