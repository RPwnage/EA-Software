///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerPSP2.h
// 
// Copyright (c) 2012, Electronic Arts. All Rights Reserved.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Currently (as of SDK 1.8) PSP2 (PSVita) exception handling is limited. 
// You get called back when the exception occurs, but you can't see anything 
// about the exception and can only write 16K of data to the exception report
// that the OS generates. You supposedly cannot safely call system functions.
///////////////////////////////////////////////////////////////////////////////


#ifndef EXCEPTIONHANDLER_PSP2_EXCEPTIONHANDLERPSP2_H
#define EXCEPTIONHANDLER_PSP2_EXCEPTIONHANDLERPSP2_H


#include <ExceptionHandler/ExceptionHandler.h>
#include <EACallstack/Context.h>
#include <time.h>


namespace EA
{
    namespace Debug
    {
        class ReportWriter;
        class ExceptionHandler;


        /// ExceptionHandlerPSP2
        /// 
        /// Implements an exception handler for PSVita exceptions.
        ///
        class ExceptionHandlerPSP2
        {
        public:
            ExceptionHandlerPSP2(ExceptionHandler* pOwner);
           ~ExceptionHandlerPSP2();

            bool SetEnabled(bool state);
            bool IsEnabled() const;

            bool SetMode(ExceptionHandler::Mode) { return false; /* To do */ }

            void SetAction(ExceptionHandler::Action action, int returnValue = ExceptionHandler::kDefaultTerminationReturnValue);
            ExceptionHandler::Action GetAction(int* pReturnValue = NULL) const;

            intptr_t RunTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught);

            // Provides detailed information about the exception. This struct is platform-specific.
            struct ExceptionInfoPSP2 : public ExceptionHandler::ExceptionInfo
            {
                uint64_t mExceptionCause;
            };

            const ExceptionInfoPSP2* GetExceptionInfo() const
                { return &mExceptionInfoPSP2; }

            // Deprecated functions. use GetExceptionInfo instead.
                size_t                        GetExceptionCallstack(void* pCallstackArray, size_t capacity) const;
                const EA::Callstack::Context* GetExceptionContext() const;
                EA::Thread::ThreadId          GetExceptionThreadId(EA::Thread::SysThreadId& sysThreadId, char* threadName, size_t threadNameCapacity) const;
                size_t                        GetExceptionDescription(char* buffer, size_t capacity);
            // End deprecated functions.

            void SimulateExceptionHandling(int exceptionType); // exceptionType is one of enum ExceptionType or some non-enumerated custom type.

        protected:
            friend class ExceptionHandler;

            static int  ExceptionCallbackStatic(void* pContext);
            void        ExceptionCallback();

        protected:
            ExceptionHandler*           mpOwner;
            bool                        mbEnabled;
            bool                        mbExceptionOccurred;
            ExceptionHandler::Mode      mMode;
            ExceptionHandler::Action    mAction;
            int                         mnTerminateReturnValue;
            ExceptionInfoPSP2           mExceptionInfoPSP2;
        };

    } // namespace Debug

} // namespace EA


#endif // Header include guard








