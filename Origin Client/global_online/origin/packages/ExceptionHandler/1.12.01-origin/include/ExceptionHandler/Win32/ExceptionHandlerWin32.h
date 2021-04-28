///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerWin32.h
// 
// Copyright (c) 2005, Electronic Arts. All Rights Reserved.
// Written and maintained by Vasyl Tsvirkunov and Paul Pedriana.
//
// Exception handling and reporting facilities for Win32.
///////////////////////////////////////////////////////////////////////////////


#ifndef EXCEPTIONHANDLER_WIN32_EXCEPTIONHANDLERWIN32_H
#define EXCEPTIONHANDLER_WIN32_EXCEPTIONHANDLERWIN32_H


#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0400)
    #undef  _WIN32_WINNT
    #define _WIN32_WINNT 0x0400
#endif


#include <ExceptionHandler/ExceptionHandler.h>
#include <eathread/eathread.h>
#include <eathread/eathread_atomic.h>
#include <EASTL/fixed_string.h>
#include <excpt.h>
#include <time.h>
#include <stdio.h>


///////////////////////////////////////////////////////////////////////////////
// Windows.h avoidance
//
struct  _EXCEPTION_POINTERS;
typedef _EXCEPTION_POINTERS * LPEXCEPTION_POINTERS;
typedef long (__stdcall * PTOP_LEVEL_EXCEPTION_FILTER)(struct _EXCEPTION_POINTERS * ExceptionInfo);
typedef PTOP_LEVEL_EXCEPTION_FILTER LPTOP_LEVEL_EXCEPTION_FILTER;
///////////////////////////////////////////////////////////////////////////////


namespace EA
{
    namespace Debug
    {
        class ReportWriter;
        class ExceptionHandler;


        /// \class ExceptionHandlerWin32
        /// \brief Win32 implementation of ExceptionHandler
        class ExceptionHandlerWin32
        {
        public:
            ExceptionHandlerWin32(ExceptionHandler* pOwner);
           ~ExceptionHandlerWin32();

            bool SetEnabled(bool state);
            bool IsEnabled() const;

            bool SetMode(ExceptionHandler::Mode mode);

            ExceptionHandler::Action GetAction(int* pReturnValue = NULL) const;
            void SetAction(ExceptionHandler::Action action, int returnValue = ExceptionHandler::kDefaultTerminationReturnValue);

            intptr_t RunTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught);

            // Provides detailed information about the exception. This struct is platform-specific.
            struct ExceptionInfoWin32 : public ExceptionHandler::ExceptionInfo
            {
                LPEXCEPTION_POINTERS mpExceptionPointers; // To consider: Don't store the pointer here but rather store a concrete struct.
            };

            const ExceptionInfoWin32* GetExceptionInfo() const
                { return &mExceptionInfoWin32; }

            // This is public so that a global function can call it. But we don't want to declare it a friend
            // because the global function uses windows.h identifiers and we can't include windows.h in headers.
            int ExceptionFilter(LPEXCEPTION_POINTERS pExceptionPointers);

            const char* GetMiniDumpPath() const 
                { return mMiniDumpPath; }

            void SimulateExceptionHandling(int exceptionType); // exceptionType is one of enum ExceptionType or some non-enumerated custom type.

            // Deprecated functions. use GetExceptionInfo instead.
                size_t                        GetExceptionCallstack(void* pCallstackArray, size_t capacity) const;
                const EA::Callstack::Context* GetExceptionContext() const;
                EA::Thread::ThreadId          GetExceptionThreadId(EA::Thread::SysThreadId& sysThreadId, char* threadName, size_t threadNameCapacity) const;
                size_t                        GetExceptionDescription(char* buffer, size_t capacity);
            // End deprecated functions.

        protected: // kModeCPP-specific
            intptr_t RunModeCPPTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught);
            void     HandleCaughtCPPException(std::exception& e);
            void     GenerateExceptionDescription();

        protected:
            friend class ExceptionHandler;

            // miniDumpType is the same as Microsoft MINIDUMP_TYPE, though we don't use MINIDUMP_TYPE because doing so would require #including a Windows header from this header.
            void WriteMiniDump(int miniDumpType);

        protected:
            ExceptionHandler*            mpOwner;                       // High-level proxy class
            bool                         mbEnabled;
            bool                         mbExceptionOccurred;
            EA::Thread::AtomicInt32      mbHandlingInProgress;
            ExceptionHandler::Mode       mMode;
            ExceptionHandler::Action     mAction;
            void*                        mpVectoredHandle;               // Used for vectored exception handling.
            int                          mnTerminateReturnValue;
            char                         mMiniDumpPath[EA::IO::kMaxPathLength];
            LPTOP_LEVEL_EXCEPTION_FILTER mPreviousFilter;
            ExceptionInfoWin32           mExceptionInfoWin32;
        };

    } // namespace Debug

} // namespace EA


#endif // Header include guard








