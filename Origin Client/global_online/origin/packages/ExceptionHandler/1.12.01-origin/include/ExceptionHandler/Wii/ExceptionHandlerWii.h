///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerWii.h
// 
// Copyright (c) 2005, Electronic Arts. All Rights Reserved.
// Created by Paul Pedriana
//
// Exception handling and reporting facilities for Wii (Revolution) and GameCube.
///////////////////////////////////////////////////////////////////////////////


#ifndef DEBUG_EXCEPTIONHANDLERWII_H
#define DEBUG_EXCEPTIONHANDLERWII_H


#include <ExceptionHandler/ExceptionHandler.h>
#include <revolution/os/OSContext.h>


// Forward declarations
struct OSContext;


namespace EA
{
    namespace Debug
    {

        /// \class ExceptionHandlerWii
        /// \brief Wii implementation of ExceptionHandler
        class ExceptionHandlerWii
        {
        public:
            ExceptionHandlerWii(ExceptionHandler* pOwner);
           ~ExceptionHandlerWii();

            bool SetEnabled(bool state);
            bool IsEnabled() const;

            bool SetMode(ExceptionHandler::Mode) { return false; /* To do */ }

            void SetAction(ExceptionHandler::Action action, int returnValue = ExceptionHandler::kDefaultTerminationReturnValue);
            ExceptionHandler::Action GetAction(int* pReturnValue = NULL) const;

            intptr_t RunTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught);

            // Provides detailed information about the exception. This struct is platform-specific.
            struct ExceptionInfoWii : public ExceptionHandler::ExceptionInfo
            {
                uint64_t mExceptionCause;
            };

            const ExceptionInfoWii* GetExceptionInfo() const
                { return &mExceptionInfoWii; }

            void SimulateExceptionHandling(int exceptionType); // exceptionType is one of enum ExceptionType or some non-enumerated custom type.

            // Deprecated functions. use GetExceptionInfo instead.
                size_t                        GetExceptionCallstack(void* pCallstackArray, size_t capacity) const;
                const EA::Callstack::Context* GetExceptionContext() const;
                EA::Thread::ThreadId          GetExceptionThreadId(EA::Thread::SysThreadId& sysThreadId, char* threadName, size_t threadNameCapacity) const;
                size_t                        GetExceptionDescription(char* buffer, size_t capacity);
            // End deprecated functions.

        protected:
            friend class ExceptionHandler;

            static void ExceptionCallbackStatic(__OSException exceptionCause, OSContext* pContext);
            void        ExceptionCallback      (__OSException exceptionCause, OSContext* pContext);

        protected:
            ExceptionHandler*           mpOwner;
            __OSExceptionHandler        mSavedOSExceptionHandler;
            bool                        mbEnabled;
            bool                        mbExceptionOccurred;
            ExceptionHandler::Mode      mMode;
            ExceptionHandler::Action    mAction;
            int                         mnTerminateReturnValue;
            ExceptionInfoWii            mExceptionInfoWii;
        };


    } // namespace Debug

} // namespace EA


#endif // Header include guard


















