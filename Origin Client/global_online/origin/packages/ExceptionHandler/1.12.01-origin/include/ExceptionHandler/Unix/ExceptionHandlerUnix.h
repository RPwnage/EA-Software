///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerUnix.h
// 
// Copyright (c) 2011, Electronic Arts. All Rights Reserved.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#ifndef EXCEPTIONHANDLER_UNIX_EXCEPTIONHANDLERUNIX_H
#define EXCEPTIONHANDLER_UNIX_EXCEPTIONHANDLERUNIX_H


#include <ExceptionHandler/ExceptionHandler.h>
#include <EACallstack/Context.h>
#include <EASTL/fixed_string.h>
#include <time.h>
#if defined(EA_PLATFORM_BSD)
    #include <sys/signal.h>
#else
    #include <signal.h>
#endif


namespace EA
{
    namespace Debug
    {
        class ReportWriter;
        class ExceptionHandler;

        /// ExceptionHandlerUnix
        /// 
        /// Implements an exception handler for Unix PPU (the primary PowerPC) exceptions.
        /// Exception handling for the Unix SPU processors is not supported by Sony 
        /// as of this writing.
        ///
        class ExceptionHandlerUnix
        {
        public:
            ExceptionHandlerUnix(ExceptionHandler* pOwner);
           ~ExceptionHandlerUnix();

            bool SetEnabled(bool state);
            bool IsEnabled() const;

            bool SetMode(ExceptionHandler::Mode);

            void SetAction(ExceptionHandler::Action action, int returnValue = ExceptionHandler::kDefaultTerminationReturnValue);
            ExceptionHandler::Action GetAction(int* pReturnValue = NULL) const;

            intptr_t RunTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught);

            // Provides detailed information about the exception. This struct is platform-specific.
            struct ExceptionInfoUnix : public ExceptionHandler::ExceptionInfo
            {
                uint32_t   mLWPId;          /// The lightweight process ID that the exception occurred in.
                int        mSigNumber;      /// The signal number (e.g. SIGSEGV).
                sigcontext mSigContext;     /// kernel signal context information, which includes some CPU register context information.
                #if defined(EA_PLATFORM_BSD)
                    __siginfo mSigInfo;     /// Kernel signal info.
                #else
                    siginfo_t mSigInfo;
                #endif

                ExceptionInfoUnix();
            };

            const ExceptionInfoUnix* GetExceptionInfo() const
                { return &mExceptionInfoUnix; }

            void SimulateExceptionHandling(int exceptionType); // exceptionType is one of enum ExceptionType or some non-enumerated custom type.

            // Deprecated functions. use GetExceptionInfo instead.
                size_t                        GetExceptionCallstack(void* pCallstackArray, size_t capacity) const;
                const EA::Callstack::Context* GetExceptionContext() const;
                EA::Thread::ThreadId          GetExceptionThreadId(EA::Thread::SysThreadId& sysThreadId, char* threadName, size_t threadNameCapacity) const;
                size_t                        GetExceptionDescription(char* buffer, size_t capacity);
            // End deprecated functions.

        protected:
            friend class ExceptionHandler;

            void GetSignalExceptionDescription(String& sDescription);

            static void ExceptionCallbackStatic(uint64_t exceptionCause/*, uint64_t ppuThreadId, uint64_t dataAccessRegisterValue*/);
            void        ExceptionCallback      (uint64_t exceptionCause/*, uint64_t ppuThreadId, uint64_t dataAccessRegisterValue*/);

        protected:
            ExceptionHandler*           mpOwner;
            bool                        mbEnabled;
            bool                        mbExceptionOccurred;
            EA::Thread::AtomicInt32     mbHandlingInProgress;
            ExceptionHandler::Mode      mMode;
            ExceptionHandler::Action    mAction;
            int                         mnTerminateReturnValue;
            ExceptionInfoUnix           mExceptionInfoUnix;
        };

    } // namespace Debug

} // namespace EA


#endif // Header include guard





