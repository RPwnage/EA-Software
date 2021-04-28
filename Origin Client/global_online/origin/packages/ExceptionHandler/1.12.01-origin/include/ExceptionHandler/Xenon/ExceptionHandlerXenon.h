///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerXenon.h
// 
// Copyright (c) 2005, Electronic Arts. All Rights Reserved.
// Written and maintained by Vasyl Tsvirkunov and Paul Pedriana.
//
// Exception handling and reporting facilities for Xenon and XBox.
///////////////////////////////////////////////////////////////////////////////


#ifndef EXCEPTIONHANDLER_XENON_EXCEPTIONHANDLERXENON_H
#define EXCEPTIONHANDLER_XENON_EXCEPTIONHANDLERXENON_H


#include <ExceptionHandler/ExceptionHandler.h>
#include <eathread/eathread.h>
#include <eathread/eathread_atomic.h>
#include <EASTL/fixed_string.h>


#pragma warning(push, 0)
#include <xtl.h>
#include <excpt.h>
#include <time.h>
#pragma warning(pop)


namespace EA
{
    namespace Debug
    {
        class ReportWriter;
        class ExceptionHandler;

        /// \class ExceptionHandlerXenon
        /// \brief Xenon implementation of ExceptionHandler
        class ExceptionHandlerXenon
        {
        public:
            ExceptionHandlerXenon(ExceptionHandler* pOwner);
           ~ExceptionHandlerXenon();

            bool SetEnabled(bool state);
            bool IsEnabled() const;

            bool SetMode(ExceptionHandler::Mode);

            ExceptionHandler::Action GetAction(int* pReturnValue = NULL) const;
            void SetAction(ExceptionHandler::Action action, int returnValue = ExceptionHandler::kDefaultTerminationReturnValue);

            intptr_t RunTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught);

            // Provides detailed information about the exception. This struct is platform-specific.
            struct ExceptionInfoXenon : public ExceptionHandler::ExceptionInfo
            {
                LPEXCEPTION_POINTERS mpExceptionPointers; // To consider: Don't store the pointer here but rather store a concrete struct.
            };

            const ExceptionInfoXenon* GetExceptionInfo() const
                { return &mExceptionInfoXenon; }

            void SimulateExceptionHandling(int exceptionType); // exceptionType is one of enum ExceptionType or some non-enumerated custom type.

            // This is public so that a global function can call it. But we don't want to declare it a friend
            // because the global function uses windows.h identifiers and we can't include windows.h in headers.
            int ExceptionFilter(LPEXCEPTION_POINTERS pExceptionPointers);

            // Deprecated functions. use GetExceptionInfo instead.
                size_t                        GetExceptionCallstack(void* pCallstackArray, size_t capacity) const;
                const EA::Callstack::Context* GetExceptionContext() const;
                EA::Thread::ThreadId          GetExceptionThreadId(EA::Thread::SysThreadId& sysThreadId, char* threadName, size_t threadNameCapacity) const;
                size_t                        GetExceptionDescription(char* buffer, size_t capacity);
            // End deprecated functions.

        protected: // kModeCPP-specific
            intptr_t RunModeCPPTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught);
            void     HandleCaughtCPPException(std::exception& e);

        protected:
            friend class ExceptionHandler;

            void WriteMiniDump();
            void GenerateExceptionDescription();

        protected:
            ExceptionHandler*               mpOwner;                        // High-level proxy class
            bool                            mbEnabled;
            bool                            mbExceptionOccurred;
            EA::Thread::AtomicInt32         mbHandlingInProgress;
            ExceptionHandler::Mode          mMode;
            ExceptionHandler::Action        mAction;
            int                             mnTerminateReturnValue;
            LPTOP_LEVEL_EXCEPTION_FILTER    mPreviousFilter;
            ExceptionInfoXenon              mExceptionInfoXenon;
        };

    } // namespace Debug

} // namespace EA



#endif // Header include guard










