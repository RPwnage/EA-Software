///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerPS3.h
// 
// Copyright (c) 2006, Electronic Arts. All Rights Reserved.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Exception handling on the PS3 doesn't work like it does with other platforms.
// Instead of using try/catch, the PS3 works by registering a callback handler.
// The application is crashed and is terminated upon completion of the callback
// handler. The handler cannot retry the operation or continue execution from
// a safe point; it is merely a notification mechanism that allows the user 
// to record that the exception occurred.
//
// As a result of how the PS3 exception handling works, the RunTrapped function
// can't do what it does for other platforms, which is to provide a try/catch
// mechanism. We provide RunTrapped for compatibility with other, though the 
// kActionContinue option is not available.
//
///////////////////////////////////////////////////////////////////////////////


#ifndef EXCEPTIONHANDLER_PS3_EXCEPTIONHANDLERPS3_H
#define EXCEPTIONHANDLER_PS3_EXCEPTIONHANDLERPS3_H


#include <ExceptionHandler/ExceptionHandler.h>
#include <EACallstack/Context.h>
#include <time.h>


namespace EA
{
    namespace Debug
    {
        class ReportWriter;
        class ExceptionHandler;

        /// ExceptionHandlerPS3
        /// 
        /// Implements an exception handler for PS3 PPU (the primary PowerPC) exceptions.
        /// Exception handling for the PS3 SPU processors is not supported by Sony 
        /// as of this writing.
        ///
        class ExceptionHandlerPS3
        {
        public:
            ExceptionHandlerPS3(ExceptionHandler* pOwner);
           ~ExceptionHandlerPS3();

            bool SetEnabled(bool state);
            bool IsEnabled() const;

            bool SetMode(ExceptionHandler::Mode);

            void SetAction(ExceptionHandler::Action action, int returnValue = ExceptionHandler::kDefaultTerminationReturnValue);
            ExceptionHandler::Action GetAction(int* pReturnValue = NULL) const;

            intptr_t RunTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught);

            // Provides detailed information about the exception. This struct is platform-specific.
            struct ExceptionInfoPS3 : public ExceptionHandler::ExceptionInfo
            {
                uint64_t mExceptionCause;
            };

            const ExceptionInfoPS3* GetExceptionInfo() const
                { return &mExceptionInfoPS3; }

            void SimulateExceptionHandling(int exceptionType); // exceptionType is one of enum ExceptionType or some non-enumerated custom type.

            // Deprecated functions. use GetExceptionInfo instead.
                size_t                        GetExceptionCallstack(void* pCallstackArray, size_t capacity) const;
                const EA::Callstack::Context* GetExceptionContext() const;
                EA::Thread::ThreadId          GetExceptionThreadId(EA::Thread::SysThreadId& sysThreadId, char* threadName, size_t threadNameCapacity) const;
                size_t                        GetExceptionDescription(char* buffer, size_t capacity);
            // End deprecated functions.

        protected: // kModeCPP-specific
            void HandleCaughtCPPException(std::exception& e);
    
        protected: // kModeVectored-specific
            static void ExceptionCallbackStatic(uint64_t exceptionCause, uint64_t ppuThreadId, uint64_t dataAccessRegisterValue);
            void        ExceptionCallback      (uint64_t exceptionCause, uint64_t ppuThreadId, uint64_t dataAccessRegisterValue);

        protected:
            void GenerateExceptionDescription();

            friend class ExceptionHandler;

            ExceptionHandler*           mpOwner;
            bool                        mbEnabled;
            bool                        mbExceptionOccurred;
            EA::Thread::AtomicInt32     mbHandlingInProgress;
            ExceptionHandler::Mode      mMode;
            ExceptionHandler::Action    mAction;
            int                         mnTerminateReturnValue;
            ExceptionInfoPS3            mExceptionInfoPS3;
        };

    } // namespace Debug

} // namespace EA


#endif // Header include guard








