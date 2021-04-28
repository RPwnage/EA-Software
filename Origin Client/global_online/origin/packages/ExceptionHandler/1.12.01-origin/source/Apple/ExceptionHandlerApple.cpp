///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerApple.cpp
// 
// Copyright (c) 2012, Electronic Arts. All Rights Reserved.
// Written and maintained by Paul Pedriana.
//
// Exception handling and reporting facilities.
///////////////////////////////////////////////////////////////////////////////

/*

-- Mach-level exception handling notes --

Many of the runtime errors that trigger signals on Unix cause exceptions under Mach. 
Common examples are accessing an unmapped memory address, violating page permissions 
on mapped memory, or dividing by zero. When one of these events happens, the thread 
performing the invalid action (referred to as the victim thread) generates an exception. 
Every thread has a special exception port that may be set to allow another thread 
(referred to as the handler thread) to handle exceptions generated in the victim. 
If there is no thread-exception port set or if the exception handler does not handle 
the exception, the kernel delivers the exception to the task. Similar to the thread, 
every task has an exception port that allows another task to handle exceptions within it. 
If the task-exception handler does not handle the exception or the task-exception port 
is not set, the exception is converted into a Unix signal and delivered to the BSD process.

The kernel handles the communication with the exception handlers on behalf of the victim 
thread or task. This communication is performed through Mach IPC. An exception handler 
thread allocates a new port and sets it as the excep¬tion port for another thread or task. 
The handler thread can then block in a call to mach_msg_receive() waiting for a message 
from the kernel if and when the victim thread or task generates an exception. The handler 
thread is given send rights to the thread and task where the exception occurred and may 
manipulate both to handle the exception. The exception handler then sends a message back 
to the kernel indicating whether the exception was handled (and the kernel should resume 
execution of the victim thread) or not handled (in which case the kernel should continue 
searching for an exception handler).

A downside to the way that Apple mach-level exception handling works is that the kernel
callback function names are fixed and a given application can have only one set of them.
Thus it's not possible to link an app that has two such handlers, as they both would
need to have functions of the same name (catch_exception_raise, catch_exception_raise_state,
catch_exception_raise_state_identity). I -think- this can be avoided if you don't use
the built-in exc_server function and instead use a manually generated one as we need to
do with 64 bit exception handling.

There are three callback functions (catch_exception_raise, catch_exception_raise_state,
catch_exception_raise_state_identity) but I believe you need only implement one, the one
you specify in task_set_exception_ports or thread_set_exception_ports. Each of them is 
simply like the previous one but with a few more available arguments.

API documentation:
   http://web.mit.edu/darwin/src/modules/xnu/osfmk/man/
   
Useful article:
   http://blogs.embarcadero.com/eboling/2009/11/10/5628
     and http://www.brianweb.net/misc/mach_exceptions_demo.c
     
Thread:
   http://lists.apple.com/archives/darwin-dev/2006/Oct/msg00122.html
   
Thread:
   http://lists.apple.com/archives/darwin-dev/2008/Jun/msg00052.html
   http://lists.apple.com/archives/darwin-dev/2008/Jun/msg00061.html
   http://lists.apple.com/archives/darwin-dev/2008/Jun/msg00062.html
   http://lists.apple.com/archives/darwin-dev/2008/Jun/msg00067.html
   
Wisdom:
   http://stackoverflow.com/questions/2824105/handling-mach-exceptions-in-64bit-os-x-application
     and http://opensource.apple.com/source/gdb/gdb-1708/src/gdb/macosx/macosx-nat-excthread.c
   http://technosloth.blogspot.com/2010/08/thread-based-thread-level-mach.html
   
Apple test code for mach exception handling:
   http://paimei.googlecode.com/svn/trunk/MacOSX/macdll/Exception.c

iOS crash handling article:
    http://landonf.bikemonkey.org/code/objc/Reliable_Crash_Reporting.20110912.html

Google Breakpad:
    http://code.google.com/p/google-breakpad/source/browse/trunk/src/client/mac/handler/exception_handler.cc
   
It seems that exc_server (exception handling server) usage isn't allowed on iOS for
shipping applications. Can we use C signals in that case?:
   http://permalink.gmane.org/gmane.comp.gnome.mono.bugs/33102
   
Signal-based exception handling:
   http://cocoawithlove.com/2010/05/handling-unhandled-exceptions-and.html
   
Stack trace during an exception:
   http://rel.me/2008/12/30/getting-a-useful-stack-trace-from-nsexception-callstackreturnaddresses/
   
Exception types and codes:
   http://stackoverflow.com/questions/1282428/whats-the-difference-between-kern-invalid-address-and-kern-protection-failure
   
GCC stack traces from a signal handler:
    http://stackoverflow.com/questions/77005/how-to-generate-a-stacktrace-when-my-gcc-c-app-crashes
    http://www.linuxjournal.com/article/6391?page=0,1
    http://www.khmere.com/freebsd_book/html/ch04.html
    http://www.samba.org/ftp/unpacked/junkcode/segv_handler/

Get detailed current task information:
    http://flylib.com/books/en/3.126.1.79/1/
    
Core dump example code:
    http://osxbook.com/book/bonus/chapter8/core/download/gcore.c

*/


#include <ExceptionHandler/Apple/ExceptionHandlerApple.h>


// Apple-specific implemenation.
#if defined(EA_PLATFORM_APPLE)

#include <EACallstack/EAAddressRep.h>
#include <EACallstack/EACallstack.h>
#include <EACallstack/EADasm.h>
#include <EACallstack/Context.h>
#include <eathread/eathread.h>
#include <EAStdC/EAString.h>
#include EA_ASSERT_HEADER

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <mach/thread_status.h>
#include <mach/exception.h>
#include <mach/task.h>
#include <mach/thread_act.h>
#include "internal/mach_exc.h"
#include <pthread.h>
#include <exception>
#include <stdexcept>




// The following two functions should never be called, because they are merely alternatives to 
// the catch_exception_raise_state_identity function. By specifying EXCEPTION_STATE_IDENTITY
// to task_set_exception_ports, we tell it to use catch_mach_exception_raise_state.
// However, the exc_mac_server code is still dependent on these existing.

extern "C" kern_return_t catch_mach_exception_raise_ea(mach_port_t /*exception_port*/, mach_port_t /*thread*/,
                                                       mach_port_t /*task*/, exception_type_t /*exceptionType*/, 
                                                       mach_exception_data_t /*pExceptionDetail*/, mach_msg_type_number_t /*exceptionDetailCount*/)
{    
    return KERN_FAILURE;
}

extern "C" kern_return_t catch_mach_exception_raise_state_ea(mach_port_t /*exception_port*/, exception_type_t /*exceptionType*/,
                                                             const mach_exception_data_t /*pExceptionDetail*/, mach_msg_type_number_t /*exceptionDetailCount*/, int* /*flavor*/,
                                                             const thread_state_t /*pOldState*/, mach_msg_type_number_t /*oldStateCount*/,
                                                             thread_state_t /*pNewState*/, mach_msg_type_number_t* /*pNewStateCount*/)
{    
    return KERN_FAILURE;
}

// This is the mac_exc_server callback function we use. The EXCEPTION_STATE_IDENTITY flag in the setup 
// code below is what tells the system to call this function instead of either of the above two.
extern "C" kern_return_t catch_mach_exception_raise_state_identity_ea(
                            mach_port_t exception_port, mach_port_t thread, mach_port_t task, exception_type_t exceptionType, 
                            mach_exception_data_type_t*  pExceptionDetail, mach_msg_type_number_t exceptionDetailCount, int* flavor, thread_state_t  pOldState,
                            mach_msg_type_number_t oldStateCount, thread_state_t pNewState, mach_msg_type_number_t* pNewStateCount)      
{
    // To do: Revise the mach_exc_server_ea function to accept a user void* pointer so that 
    //        we don't need to rely on this mpThis pointer, which is thread-unsafe.
    return EA::Debug::ExceptionHandlerApple::mpThis->HandleMachException(exception_port, thread, task, exceptionType, pExceptionDetail, exceptionDetailCount, flavor, pOldState, oldStateCount, pNewState, pNewStateCount);
}

extern "C" void* MachThreadFunctionStatic(void* pExceptionHandlerAppleVoid)
{
    EA::Debug::ExceptionHandlerApple* p = static_cast<EA::Debug::ExceptionHandlerApple*>(pExceptionHandlerAppleVoid);
    return p->MachThreadFunction();
}




namespace EA {
namespace Debug {


ExceptionHandlerApple::ExceptionInfoApple::ExceptionInfoApple()
  : mCPUExceptionId(0),
    mCPUExceptionDetail(0),
  //mMachExceptionDetail[]
    mMachExceptionDetailCount(0),
    mExceptionCause(0)
{
    memset(mMachExceptionDetail, 0, sizeof(mMachExceptionDetail));
}



// To consider: Make this not be a single global but instead some kind of 
// thread-specific stack of pointers. That way we could have more than one
// exception handler at a time.
ExceptionHandlerApple* ExceptionHandlerApple::mpThis = NULL;


///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerApple
//
ExceptionHandlerApple::ExceptionHandlerApple(ExceptionHandler* pOwner) :
    mpOwner(pOwner),
    mbEnabled(false),
    mbExceptionOccurred(false),
    mMode(ExceptionHandler::kModeVectored),
    mAction(ExceptionHandler::kActionDefault),
    mnTerminateReturnValue(ExceptionHandler::kDefaultTerminationReturnValue),
    mbHandlingInProgress(0),
    
    // Mach-specific
    mbMachHandlerInitialized(false),
    mExceptionPort(MACH_PORT_NULL),
    mOldExceptionPorts(),
    mbMachThreadShouldContinue(true),
  //mMachThread()

    // Signal-specific
    mbSignalHandlerInitialized(false)
{
    memset(&mMachThread, 0, sizeof(mMachThread));

    EA_ASSERT(mpThis == NULL);
    mpThis = this;
    
    SetMode(ExceptionHandler::kModeDefault);
}


///////////////////////////////////////////////////////////////////////////////
// ~ExceptionHandlerApple
//
ExceptionHandlerApple::~ExceptionHandlerApple()
{
    SetEnabled(false);
    mpThis = NULL;
}


///////////////////////////////////////////////////////////////////////////////
// SetEnabled
//
bool ExceptionHandlerApple::SetEnabled(bool state)
{
    switch (mMode)
    {
        case ExceptionHandler::kModeSignalHandling:
        {
            if(state && !mbEnabled)
            {
                mbEnabled = InitSignalHandler(); // This will set up a signal handler if not done already, and activate it.
                return mbEnabled;
            }
            else if(!state && mbEnabled)
            {
                ShutdownSignalHandler();
                mbEnabled = false;
                return true;
            }

            break;
        }

        case ExceptionHandler::kModeVectored:
        {
            if(state && !mbEnabled)
            {
                mbEnabled = InitMachHandler(); // This will set up a Mach exception handler if not done already, and activate it.
                return mbEnabled;
            }
            else if(!state && mbEnabled)
            {
                ShutdownMachHandler();
                mbEnabled = false;
                return true;
            }

            break;
        }

        case ExceptionHandler::kModeCPP:
        {
            // C++ try/catch exception handling is executed only via a user call to 
            // our RunTrapped functionand we don't need to do anything here.
            #if defined(EA_COMPILER_NO_EXCEPTIONS)
                EA_FAIL_MESSAGE("ExceptionHandlerApple::SetEnabled: kModeCPP selected, but the exception handling is disabled. Need to build with -fexceptions to successfully use C++ exception handling.");
            #endif
            
            mbEnabled = state;
            return true;
        }

        default:
            // This should never execute in practice. kModeStackBased is always disabled
            // and kModeDefault has no actual meaning.
            return false;
    }

    return (mbEnabled == state);
}


///////////////////////////////////////////////////////////////////////////////
// IsEnabled
//
bool ExceptionHandlerApple::IsEnabled() const
{
    return mbEnabled;
}


///////////////////////////////////////////////////////////////////////////////
// SetMode
//
bool ExceptionHandlerApple::SetMode(ExceptionHandler::Mode mode)
{
    // We support the following:
    //     - C++ exception handling (to consider: Also support objective C).
    //     - Posix sigint exception handling.
    //     - Mach kernel exception handling (using the Microsoft "vectored" term here).
    
    bool originallyEnabled = mbEnabled;

    // If the mode is being changed, disable the current mode first.
    if(mbEnabled && (mode != mMode))
        SetEnabled(false);
    
    if(!mbEnabled) // If we were already disabled or were able to disable it above...
    {
        // Apple doesn't provide a stack-based exception handling system like Microsoft
        // does, and so kModeStackBased isn't supported.
        if(mode == ExceptionHandler::kModeStackBased)
            mode = ExceptionHandler::kModeDefault;

        if(mode == ExceptionHandler::kModeDefault)
        {
            // The mach API is more developer-friendly and of a lower level than others and 
            // is thus preferred when possible. However, it is largely undocumented.
            // It is supported by both OS X and iOS Apple platforms.
            mode = ExceptionHandler::kModeVectored;
        }
        
        // The user needs to call SetEnabled(true/false) in order to enable/disable handling in this mode.
        mMode = mode;

        if(originallyEnabled)
            SetEnabled(true); // What if this fails?

        return true;
    }
    
    return false;
}


ExceptionHandlerApple::OldExceptionPorts::OldExceptionPorts()
  : mCount(0)
{
    memset(mMasks,     0, sizeof(mMasks));
    memset(mPorts,     0, sizeof(mPorts));
    memset(mBehaviors, 0, sizeof(mBehaviors));
    memset(mFlavors,   0, sizeof(mFlavors));
}


bool ExceptionHandlerApple::InitMachHandler()
{
    bool bSuccess = false;
    
    if(!mbMachHandlerInitialized)
    {
        // EXC_SOFTWARE is not part of the mask below. From mach/exception_types.h and mach/i386/exception.h, 
        // it seems that it is primarily used for nonfatal UNIX signals (e.g. SIGCHLD). Including it here 
        // treats those normally-ignored signals as fatal exceptions, so we do not request or handle EXC_SOFTWARE exceptions.

        kern_return_t     result = MACH_MSG_SUCCESS;
        mach_port_t       machTaskSelf = mach_task_self();
        exception_mask_t  mask = EXC_MASK_BAD_ACCESS      |     // Disabled: EXC_MASK_EMULATION, EXC_MASK_BREAKPOINT, 
                                 EXC_MASK_BAD_INSTRUCTION |     //           EXC_MASK_SYSCALL, EXC_MASK_MACH_SYSCALL, 
                                 EXC_MASK_ARITHMETIC      |     //           EXC_MASK_RPC_ALERT, EXC_MASK_SOFTWARE
                                 EXC_MASK_CRASH;
        
        // Create a port by allocating a receive right, and then create a send right accessible under the same name.
        if(mExceptionPort == MACH_PORT_NULL)
        {
            result = mach_port_allocate(machTaskSelf, MACH_PORT_RIGHT_RECEIVE, &mExceptionPort);
            
            if(result == MACH_MSG_SUCCESS)
                result = mach_port_insert_right(machTaskSelf, mExceptionPort, mExceptionPort, MACH_MSG_TYPE_MAKE_SEND);

            if(result == MACH_MSG_SUCCESS)
                result = task_get_exception_ports(machTaskSelf, mask, mOldExceptionPorts.mMasks, &mOldExceptionPorts.mCount, 
                                                  mOldExceptionPorts.mPorts, mOldExceptionPorts.mBehaviors, mOldExceptionPorts.mFlavors);
        }
        
        if(result == MACH_MSG_SUCCESS)
        {
            // There's a thread_set_exception_ports function which sets the exception handler just for a 
            // given thread. For the time being we don't support that, but it's feasible.
            // EXCEPTION_STATE_IDENTITY means to send a __Request__mach_exception_raise_state_identity_t message. 
            // It's important that we specify MACH_EXCEPTION_CODES, as otherwise we'll get some slightly different message 
            // message sent and things will go inexplicably wrong in the mach_exc_server_ea function. This is undocumented black magic.
            result = task_set_exception_ports(machTaskSelf, mask, mExceptionPort, EXCEPTION_STATE_IDENTITY | MACH_EXCEPTION_CODES, MACHINE_THREAD_STATE);
        }
        
        if(result == MACH_MSG_SUCCESS)
        {
            pthread_attr_t attr;
            int            err;

            mbMachThreadShouldContinue = true;
            pthread_attr_init(&attr);
            err = pthread_create(&mMachThread, &attr, MachThreadFunctionStatic, this);
            pthread_attr_destroy(&attr);
            
            mbMachHandlerInitialized = (err == 0);
            bSuccess = mbMachHandlerInitialized;
        }
        
        if(!bSuccess)
            ShutdownMachHandler();
    }
    
    return bSuccess;
}


void ExceptionHandlerApple::ShutdownMachHandler()
{
    mach_port_t machTaskSelf = mach_task_self();

    if(mExceptionPort != MACH_PORT_NULL)
    {
        kern_return_t result = KERN_SUCCESS;
        
        // Restore the previous ports
        for(size_t i = 0; (i < (size_t)mOldExceptionPorts.mCount) && (result == KERN_SUCCESS); ++i)
        {
            result = task_set_exception_ports(machTaskSelf, mOldExceptionPorts.mMasks[i], mOldExceptionPorts.mPorts[i], 
                                                            mOldExceptionPorts.mBehaviors[i], mOldExceptionPorts.mFlavors[i]);
        }

        // Does the following trigger the thread function's call to mach_msg to return an error? If so then that makes
        // it easy for us to break out of that blocking call. Otherwise we'll have to call pthread_cancel or maybe 
        // trigger an exception and let it handle it gracefully.
        mach_port_deallocate(machTaskSelf, mExceptionPort);
        mExceptionPort = MACH_PORT_NULL;
    }
    
    if(mbMachHandlerInitialized)
    {
        mbMachThreadShouldContinue = false;
        
        void* threadResult = NULL;
        pthread_join(mMachThread, &threadResult);
        // Or do we need to call pthread_cancel(mMachThread); ?
        
        mbMachHandlerInitialized = false;
    }
}


// The timeout in milliseconds for polling Mach exceptions. Use 0 to wait indefinitely.
static const int kMachThreadFunctionTimeoutMs = 2000; // We want to make this 0, but if we do so then we need to come up with a means to signal for it to end on app shutdown.

void* ExceptionHandlerApple::MachThreadFunction()
{
    __Request__mach_exception_raise_state_identity_t msg;
    __Reply__mach_exception_raise_state_identity_t   reply;
    
    mach_msg_return_t result;

    while(mbMachThreadShouldContinue)
    {
        // If a timeout is specified, use it.
        natural_t         timeout = kMachThreadFunctionTimeoutMs;
        mach_msg_option_t options = MACH_RCV_MSG | MACH_RCV_LARGE;

        if(timeout)
            options |= MACH_RCV_TIMEOUT;
        
        result = mach_msg(&msg.Head, options, 0, sizeof(msg), mExceptionPort, timeout, MACH_PORT_NULL);

        if(result == MACH_MSG_SUCCESS)
        {
            // Pass the message onto mach_exc_server_ea. It in turn will call our catch_mach_exception_raise_state_identity function.
            if(!mach_exc_server_ea(&msg.Head, &reply.Head))
            {
                result = ~MACH_MSG_SUCCESS;
            }
        }

        // Send the reply
        if(result == MACH_MSG_SUCCESS)
        {
            result = mach_msg(&reply.Head, MACH_SEND_MSG, reply.Head.msgh_size, 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
            
            if(result != MACH_MSG_SUCCESS)
            {
                // printf("MachThreadFunction: mach_msg reply failed.\n");
                // Not much we can do here.
            }
        }
    }

    return NULL;
}


/////////////////////////////////////////////////////////////////////////////////////
// HandleMachException
//
// Kernel callback function which handles an exception.
//
// Beware that documentation on the Internet for these functions is usually at
// least partially wrong, as they are describing the original Carnegie Mellon 
// Mach kernel from the 1990s. 
//
// On entry, flavor identifies the type of thread state information supplied in pOldState.  
// For example, x86_THREAD_STATE32 or PPC_THREAD_STATE. This is the type you 
// requested when you installed the exception port. If this function returns KERN_FAILURE
// then flavor is ignored by the caller (the kernel). If returning KERN_SUCCESS and 
// you want to restart the thread, flavor identifies the type of thread state information 
// returned in pNewState.
//
// On entry, pOldState is a pointer to the thread state information. Its type 
// is specified by flavor. For example, if flavor is x86_THREAD_STATE, you
// can cast this to x86_thread_state_t and access the fields of that structure.
// In practice we usually know ahead of time what this is going to be, because 
// we are the ones that asked for it.
//
// oldStateCount is the size, in units of natural_t, of the thread state 
// information supplied in pOldState. This size is determined by the flavour.  
// For example, if flavor is x86_THREAD_STATE, pOldState is a pointer to a 
// x86_thread_state_t and this value is x86_THREAD_STATE_COUNT.
//
// On entry, pNewState is a pointer to a buffer of size determined by pNewStateCount. 
// The contents of the buffer are unspecified. If returning KERN_FAILURE, the contents 
// of the buffer are ignored.  On KERN_SUCCESS, the contents of this buffer, along 
// with the final values of flavor and pNewStateCount, are used to correct the state 
// of the thread that took the exception. For example, if, on KERN_SUCCESS, flavor 
// is x86_THREAD_STATE then pNewStateCount should be x86_THREAD_STATE_COUNT 
// and the buffer pointed to be pNewState must be set up as a x86_thread_state_t 
// structure containing the new state of the thread.
//
// On entry, pNewStateCount is the size of the buffer pointed to by pNewState, 
// in units of natural_t. This will always be at least THREAD_STATE_MAX 
// for the architecture for which you are compiled. On error, pNewStateCount 
// is ignored. On success, pNewStateCount is the size, again in units 
// of natural_t, of the new thread state information placed in pNewState buffer.
//
kern_return_t ExceptionHandlerApple::HandleMachException(
    mach_port_t                 /*exception_port*/,   // The port (thread) to which the exception notification was sent. 
    mach_port_t                 thread,               // The thread self port for the thread taking the exception. 
    mach_port_t                 task,                 // The task self port for the task containing the thread taking the exception. 
    exception_type_t            exceptionType,        // The type of the exception. EXC_BAD_ACCESS, EXC_BAD_INSTRUCTION, etc. 
    mach_exception_data_type_t* pExceptionDetail,     // A machine dependent array of integers indicating a particular subclass of exceptionType. In theory the kernel could assign multiple values, though usually it's just one, like KERN_PROTECTION_FAILURE or EXC_I386_DIV. I think that EXC_BAD_ACCESS uses the KERN_XXX codes, while other EXC_XXX values use platform-specific codes like EXC_I386_DIV.
    mach_msg_type_number_t      exceptionDetailCount, // The size of pExceptionDetail in natural_t (uint32_t for all Apple machine types). 
    int*                        /*pFlavor*/,          // On input, the type of state included as selected when the exception port was set. On output, the type of state being returned. 
    thread_state_t              pOldState,            // State information of the thread at the time of the exception. You cast this to a platform-specific type, such as x86_thread_state_t. 
    mach_msg_type_number_t      /*oldStateCount*/,    // The size of pOldState in natural_t (uint32_t for all Apple machine types). 
    thread_state_t              pNewState,            // The state the thread will have if continued from the point of the exception. The maximum size of this array is THREAD_STATE_MAX. 
    mach_msg_type_number_t*     pNewStateCount)       // The size of pNewState in natural_t (uint32_t for all Apple machine types). 
{
    // Because task-level exception ports are inherited during fork(), exceptions from  
    // other tasks are forwarded to the original handlers, presumably ux_handler().
    if(task != mach_task_self())
        return ForwardMachException(thread, task, exceptionType, pExceptionDetail, exceptionDetailCount);

    // KERN_FAILURE means to tell the kernel to try another handler (including possibly the debugger). 
    // KERN_SUCCESS means that the kernel should retry the instruction and continue (assumes we fixed the problem).
    kern_return_t result = KERN_FAILURE;
    
    switch (exceptionType)
    {
        // We shouldn't be getting the following set, as we didn't ask for them in our task_set_exception_ports call.
        case EXC_EMULATION:
        case EXC_SYSCALL:
        case EXC_MACH_SYSCALL:
        case EXC_RPC_ALERT:
        case EXC_BREAKPOINT:
        case EXC_SOFTWARE:
            return KERN_FAILURE; // Let the debugger or somebody else handle it. These are not crashes and probably don't matter to us. We can add support for them if needed via a SetOption function specific to ExceptionHandlerApple.
            
        case EXC_BAD_ACCESS:
        case EXC_BAD_INSTRUCTION:
        case EXC_ARITHMETIC:
        case EXC_CRASH:
            // We'll handle these further below
            break;
    }

    // Save the thread id. The registered clients may want to use this.
    mExceptionInfoApple.mThreadId    = pthread_from_mach_thread_np(thread); 
    mExceptionInfoApple.mSysThreadId = thread;
    pthread_getname_np(mExceptionInfoApple.mThreadId, mExceptionInfoApple.mThreadName, sizeof(mExceptionInfoApple.mThreadName));
    
    // Record exception time.
    const time_t timeValue = time(NULL);
    mExceptionInfoApple.mTime = *localtime(&timeValue);

    // Read the CPU context information for the exception generating thread.
    // This will also fill in the CPU-level exception id and fault memory location (if applicable).
    // However I believe that information can be different from the pExceptionDetail that
    // the kernel sent us here.
    ReadThreadExceptionContext(thread, pOldState);
    
    // Record the exception callstack
    EA::Callstack::CallstackContext callstackContext;
    EA::Callstack::GetCallstackContext(callstackContext, GetExceptionContext());
    mExceptionInfoApple.mCallstackEntryCount = EA::Thread::GetCallstack(mExceptionInfoApple.mCallstack, EAArrayCount(mExceptionInfoApple.mCallstack), &callstackContext);

    // A single int64_t is insufficient.
    mExceptionInfoApple.mExceptionCause = exceptionType; 

    mExceptionInfoApple.mMachExceptionDetailCount = (exceptionDetailCount < EAArrayCount(mExceptionInfoApple.mMachExceptionDetail)) ? exceptionDetailCount : EAArrayCount(mExceptionInfoApple.mMachExceptionDetail);
    for(int i = 0; i < mExceptionInfoApple.mMachExceptionDetailCount; i++)
        mExceptionInfoApple.mMachExceptionDetail[i] = pExceptionDetail[i];

    GetMachExceptionDescription(mExceptionInfoApple.mExceptionDescription);

    if(mpOwner)
    {
        // To do: Make it so the user can fix the exception cause and retry the execution, and return a different result.

        mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingBegin);

        if(mpOwner->GetReportTypes() & ExceptionHandler::kReportTypeException)
            mpOwner->WriteExceptionReport();

        mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingEnd);
    }
    

    // Do post-handling.
    if(mAction == ExceptionHandler::kActionTerminate)
        ::exit(mnTerminateReturnValue);

    if(mAction == ExceptionHandler::kActionThrow)
    {
        // Let another handler deal with the exception. One of those handlers might be the debugger, 
        // but could also be another user-installed handler.
        ForwardMachException(thread, task, exceptionType, pExceptionDetail, exceptionDetailCount);
    }

    // Unless the user has fixed the exception cause (e.g. via a call to Apple's thread_set_state) and
    // set the action to kActionContinue, we have no choice but to exit the app, as the exception 
    // cause remains and we'd sit in an infinite loop fielding the same exception repeatedly.
    if(mAction == ExceptionHandler::kActionDefault)
        ::exit(mnTerminateReturnValue);

    // Else let the execution continue from the catch function.
    // If the user fixed the cause of the crash (e.g. fixed a stack page fault), then we need to 
    // set result to KERN_SUCCESS, make sure pNewState/pNewStateCount is set to the new thread context,
    // and return. The kernel will retry the operation. If instead we just return KERN_FAILURE then
    // the kernel will end the process. If a debugger is present, it should catch it first.
    EA_UNUSED(pNewState); // To do.
    EA_UNUSED(pNewStateCount);

    return result;
}


void ExceptionHandlerApple::ReadThreadExceptionContext(mach_port_t thread, thread_state_t pOldState)
{
    #if defined(EA_PROCESSOR_ARM)
        // /Developer3.2/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS4.3.sdk/usr/include/mach/arm/_structs.h
        // /Developer3.2/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS4.3.sdk/usr/include/mach/arm/thread_status.h
        
        arm_thread_state_t threadState = *(arm_thread_state_t*)pOldState;

        // Read the exception state
        arm_exception_state_t  exceptionState;
        mach_msg_type_number_t stateCount  = ARM_EXCEPTION_STATE_COUNT;
        thread_get_state(thread, ARM_EXCEPTION_STATE, (natural_t*)&exceptionState, &stateCount);
        
        // Read the floating point state
        arm_vfp_state_t vfpState;
        stateCount  = ARM_VFP_STATE_COUNT;
        thread_get_state(thread, ARM_VFP_STATE, (natural_t*)&vfpState, &stateCount);

        // Copy the state to our mExceptionInfoApple.mContext platform-generic version.
        memset(&mExceptionInfoApple.mContext, 0, sizeof(mExceptionInfoApple.mContext));

        for(int i = 0; i < 13; i++)
            mExceptionInfoApple.mContext.mGpr[i] = threadState.__r[i];
        mExceptionInfoApple.mContext.mGpr[13] = threadState.__sp;
        mExceptionInfoApple.mContext.mGpr[14] = threadState.__lr;
        mExceptionInfoApple.mContext.mGpr[15] = threadState.__pc;
        mExceptionInfoApple.mContext.mCpsr    = threadState.__cpsr;
      //mExceptionInfoApple.mContext.mSpsr;                         // Not exposed by Apple.
        #if EATHREAD_VERSION_N >= 11903         // mFpscr was missing from earlier EAThread versions.
        mExceptionInfoApple.mContext.mFpscr   = vfpState.__fpscr;
        #endif
        for(int i = 0; i < 64; i++)
            mExceptionInfoApple.mContext.mDoubleFloat[i/2].u32[i%2] = vfpState.__r[i];

        mExceptionInfoApple.mpExceptionInstructionAddress = (void*)threadState.__r[15];
        mExceptionInfoApple.mCPUExceptionId               = exceptionState.__exception;
        mExceptionInfoApple.mCPUExceptionDetail           = exceptionState.__fsr;
        mExceptionInfoApple.mpExceptionMemoryAddress      = (void*)exceptionState.__far;
        
    #elif defined(EA_PROCESSOR_X86_64)
    
        x86_thread_state_t threadState = *(x86_thread_state_t*)pOldState;

        // Read the exception state
        x86_exception_state    exceptionState;
        mach_msg_type_number_t stateCount  = x86_EXCEPTION_STATE_COUNT;
        thread_get_state(thread, x86_EXCEPTION_STATE, (natural_t*)&exceptionState, &stateCount);
        
        // Read the floating point state
        x86_float_state floatState;
        stateCount  = x86_FLOAT_STATE_COUNT;
        thread_get_state(thread, x86_FLOAT_STATE, (natural_t*)&floatState, &stateCount);

        // Copy the state to our mExceptionInfoApple.mContext platform-generic version.
        memset(&mExceptionInfoApple.mContext, 0, sizeof(mExceptionInfoApple.mContext));
        mExceptionInfoApple.mContext.ContextFlags = 0xffffffff;

        mExceptionInfoApple.mContext.SegGs  = (uint16_t)threadState.uts.ts64.__gs;
        mExceptionInfoApple.mContext.SegFs  = (uint16_t)threadState.uts.ts64.__fs;
        mExceptionInfoApple.mContext.Rdi    = threadState.uts.ts64.__rdi;
        mExceptionInfoApple.mContext.Rsi    = threadState.uts.ts64.__rsi;
        mExceptionInfoApple.mContext.Rbx    = threadState.uts.ts64.__rbx;
        mExceptionInfoApple.mContext.Rdx    = threadState.uts.ts64.__rdx;
        mExceptionInfoApple.mContext.Rcx    = threadState.uts.ts64.__rcx;
        mExceptionInfoApple.mContext.Rax    = threadState.uts.ts64.__rax;
        mExceptionInfoApple.mContext.R8     = threadState.uts.ts64.__r8;
        mExceptionInfoApple.mContext.R9     = threadState.uts.ts64.__r9;
        mExceptionInfoApple.mContext.R10    = threadState.uts.ts64.__r10;
        mExceptionInfoApple.mContext.R11    = threadState.uts.ts64.__r11;
        mExceptionInfoApple.mContext.R12    = threadState.uts.ts64.__r12;
        mExceptionInfoApple.mContext.R13    = threadState.uts.ts64.__r13;
        mExceptionInfoApple.mContext.R14    = threadState.uts.ts64.__r14;
        mExceptionInfoApple.mContext.R15    = threadState.uts.ts64.__r15;
        mExceptionInfoApple.mContext.Rbp    = threadState.uts.ts64.__rbp;
        mExceptionInfoApple.mContext.Rip    = threadState.uts.ts64.__rip;
        mExceptionInfoApple.mContext.SegCs  = (uint16_t)threadState.uts.ts64.__cs;
        mExceptionInfoApple.mContext.EFlags = (uint32_t)threadState.uts.ts64.__rflags;
        mExceptionInfoApple.mContext.Rsp    = threadState.uts.ts64.__rsp;

        // To-do: Other registers
        
        mExceptionInfoApple.mpExceptionInstructionAddress = (void*)threadState.uts.ts64.__rip;
        mExceptionInfoApple.mCPUExceptionId               = exceptionState.ues.es32.__cpu;
        mExceptionInfoApple.mCPUExceptionDetail           = exceptionState.ues.es64.__err;
        mExceptionInfoApple.mpExceptionMemoryAddress      = (void*)exceptionState.ues.es64.__trapno;

    #elif defined(EA_PROCESSOR_X86)
    
        x86_thread_state_t threadState = *(x86_thread_state_t*)pOldState;

        // Read the exception state
        x86_exception_state    exceptionState;
        mach_msg_type_number_t stateCount  = x86_EXCEPTION_STATE_COUNT;
        thread_get_state(thread, x86_EXCEPTION_STATE, (natural_t*)&exceptionState, &stateCount);
        
        // Read the floating point state
        x86_float_state floatState;
        stateCount  = x86_FLOAT_STATE_COUNT;
        thread_get_state(thread, x86_FLOAT_STATE, (natural_t*)&floatState, &stateCount);

        // Copy the state to our mExceptionInfoApple.mContext platform-generic version.
        memset(&mExceptionInfoApple.mContext, 0, sizeof(mExceptionInfoApple.mContext));
        mExceptionInfoApple.mContext.ContextFlags = 0xffffffff;

        mExceptionInfoApple.mContext.SegGs  = threadState.uts.ts32.__gs;
        mExceptionInfoApple.mContext.SegFs  = threadState.uts.ts32.__fs;
        mExceptionInfoApple.mContext.SegEs  = threadState.uts.ts32.__es;
        mExceptionInfoApple.mContext.SegDs  = threadState.uts.ts32.__ds;
        mExceptionInfoApple.mContext.Edi    = threadState.uts.ts32.__edi;
        mExceptionInfoApple.mContext.Esi    = threadState.uts.ts32.__esi;
        mExceptionInfoApple.mContext.Ebx    = threadState.uts.ts32.__ebx;
        mExceptionInfoApple.mContext.Edx    = threadState.uts.ts32.__edx;
        mExceptionInfoApple.mContext.Ecx    = threadState.uts.ts32.__ecx;
        mExceptionInfoApple.mContext.Eax    = threadState.uts.ts32.__eax;
        mExceptionInfoApple.mContext.Ebp    = threadState.uts.ts32.__ebp;
        mExceptionInfoApple.mContext.Eip    = threadState.uts.ts32.__eip;
        mExceptionInfoApple.mContext.SegCs  = threadState.uts.ts32.__cs;
        mExceptionInfoApple.mContext.EFlags = threadState.uts.ts32.__eflags;
        mExceptionInfoApple.mContext.Esp    = threadState.uts.ts32.__esp;
        mExceptionInfoApple.mContext.SegSs  = threadState.uts.ts32.__ss;

        // To-do: Other registers
        
        mExceptionInfoApple.mpExceptionInstructionAddress = (void*)threadState.uts.ts32.__eip;
        mExceptionInfoApple.mCPUExceptionId               = exceptionState.ues.es32.__trapno;
        mExceptionInfoApple.mCPUExceptionDetail           = exceptionState.ues.es32.__err;
        mExceptionInfoApple.mpExceptionMemoryAddress      = (void*)exceptionState.ues.es32.__faultvaddr;

    #else
        mExceptionInfoApple.mpExceptionInstructionAddress = NULL;
        mExceptionInfoApple.mCPUExceptionId               = 0;
        mExceptionInfoApple.mCPUExceptionDetail           = 0;
        mExceptionInfoApple.mpExceptionMemoryAddress      = NULL;
    #endif
}


static const char* GetMachExceptionTypeString(uint64_t machExceptionCause)
{
    switch (machExceptionCause)
    {
        case EXC_EMULATION:       return "EXC_EMULATION";
        case EXC_SYSCALL:         return "EXC_SYSCALL";
        case EXC_MACH_SYSCALL:    return "EXC_MACH_SYSCALL";
        case EXC_RPC_ALERT:       return "EXC_RPC_ALERT";
        case EXC_BREAKPOINT:      return "EXC_BREAKPOINT";
        case EXC_BAD_ACCESS:      return "EXC_BAD_ACCESS";
        case EXC_BAD_INSTRUCTION: return "EXC_BAD_INSTRUCTION";
        case EXC_ARITHMETIC:      return "EXC_ARITHMETIC";
        case EXC_SOFTWARE:        return "EXC_SOFTWARE";
        case EXC_CRASH:           return "EXC_CRASH";
    };

    return "EXC_???";
}


static const char* GetMachExceptionCodeString(uint64_t machExceptionCause, uint64_t code0)
{
    #if defined(EA_PROCESSOR_ARM)
    
        // To do.
    
    #elif defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64)
        switch (machExceptionCause)
        {
            case EXC_BREAKPOINT:
                if(code0 == EXC_I386_SGL)
                    return "EXC_I386_SGL";
                if(code0 == EXC_I386_BPT)
                    return "EXC_I386_BPT";
                break;
                                
            case EXC_BAD_INSTRUCTION:
                if(code0 == EXC_I386_INVOP)
                    return "EXC_I386_INVOP";
                break;
                
            case EXC_ARITHMETIC:
                switch (code0)
                {
                    case EXC_I386_DIV:
                        return "EXC_I386_DIV";
                    case EXC_I386_INTO:
                        return "EXC_I386_INTO";
                    case EXC_I386_NOEXT:
                        return "EXC_I386_NOEXT";
                    case EXC_I386_EXTOVR:
                        return "EXC_I386_EXTOVR";
                    case EXC_I386_EXTERR:
                        return "EXC_I386_EXTERR";
                    case EXC_I386_EMERR:
                        return "EXC_I386_EMERR";
                    case EXC_I386_BOUND:
                        return "EXC_I386_BOUND";
                    case EXC_I386_SSEEXTERR:
                        return "EXC_I386_SSEEXTERR";
                }
                break;
                
            case EXC_SOFTWARE:
                break; // Note: 0x10000-0x10003 in use for unix signal. How do we best return this?
        };

        // The header defines these but doesn't say what they are for.
        // #define EXC_I386_DIVERR		0	/* divide by 0 eprror		*/
        // #define EXC_I386_SGLSTP		1	/* single step			*/
        // #define EXC_I386_NMIFLT		2	/* NMI				*/
        // #define EXC_I386_BPTFLT		3	/* breakpoint fault		*/
        // #define EXC_I386_INTOFLT	    4	/* INTO overflow fault		*/
        // #define EXC_I386_BOUNDFLT	5	/* BOUND instruction fault	*/
        // #define EXC_I386_INVOPFLT	6	/* invalid opcode fault		*/
        // #define EXC_I386_NOEXTFLT	7	/* extension not available fault*/
        // #define EXC_I386_DBLFLT		8	/* double fault			*/
        // #define EXC_I386_EXTOVRFLT	9	/* extension overrun fault	*/
        // #define EXC_I386_INVTSSFLT	10	/* invalid TSS fault		*/
        // #define EXC_I386_SEGNPFLT	11	/* segment not present fault	*/
        // #define EXC_I386_STKFLT		12	/* stack fault			*/
        // #define EXC_I386_GPFLT		13	/* general protection fault	*/
        // #define EXC_I386_PGFLT		14	/* page fault			*/
        // #define EXC_I386_EXTERRFLT	16	/* extension error fault	*/
        // #define EXC_I386_ALIGNFLT   	17	/* Alignment fault */
        // #define EXC_I386_ENDPERR	    33	/* emulated extension error flt	*/
        // #define EXC_I386_ENOEXTFLT	32	/* emulated ext not present	*/
    #endif
    
    return "unknown";
}
        


static const char* GetCPUExceptionCodeString(uint32_t cpuExceptionCode)
{
    #if defined(EA_PROCESSOR_ARM)
        switch (cpuExceptionCode)
        {
            // These are the exception vector addresses, which probably aren't what the exception handler 
            // passes us. To do: Find out what the cpu code actually is.
            case 0x00:
                return "reset";
            case 0x04:
                return "invalid instruction";
            case 0x08:
                return "software interrupt";
            case 0x0c:
                return "prefetch abort";
            case 0x10:
                return "data abort";
            case 0x18:
                return "interrupt (IRQ)";
            case 0x1c:
                return "fast interrupt (FIQ)";
        }
        
        return "unknown";

    #elif defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64)
        switch (cpuExceptionCode)
        {
            case 0:
                return "integer divide by zero";
            case 1:
                return "breakpoint fault";
            case 2:
                return "non-maskable hardware interrupt";
            case 3:
                return "int 3 breakpoint";
            case 4:
                return "overflow";
            case 5:
                return "bounds check failure";
            case 6:
                return "invalid instruction";
            case 7:
                return "coprocessor unavailable";
            case 8:
                return "exception within exception";
            case 9:
                return "coprocessor segment overrun";
            case 10:
                return "invalid task switch";
            case 11:
                return "segment not present";
            case 12:
                return "stack exception";
            case 13:
                return "general protection fault";
            case 14:
                return "page fault";
            case 16:
                return "coprocessor error";
        }
        
        return "unknown";
    #endif
}



///////////////////////////////////////////////////////////////////////////////
// GetMachExceptionDescription
//
void ExceptionHandlerApple::GetMachExceptionDescription(String& sDescription)
{
    sDescription.sprintf("Mach exception type: %llu (%s)\r\n", mExceptionInfoApple.mExceptionCause, GetMachExceptionTypeString(mExceptionInfoApple.mExceptionCause));
    
    sDescription.append_sprintf("Mach exception codes: %I64u (%s), 0x%I64x (%I64u).\r\n", mExceptionInfoApple.mMachExceptionDetail[0], 
                                GetMachExceptionCodeString(mExceptionInfoApple.mExceptionCause, mExceptionInfoApple.mMachExceptionDetail[0]), 
                                mExceptionInfoApple.mMachExceptionDetail[1], mExceptionInfoApple.mMachExceptionDetail[1]);
    
    // To do: Make a string version of the exception id, which will be CPU-specific.
    sDescription.append_sprintf("CPU codes: exception id: %u (%s), exception info: %u, fault memory address: %p", mExceptionInfoApple.mCPUExceptionId, GetCPUExceptionCodeString(mExceptionInfoApple.mCPUExceptionId), mExceptionInfoApple.mCPUExceptionDetail, mExceptionInfoApple.mpExceptionMemoryAddress);
}



///////////////////////////////////////////////////////////////////////////////
// ForwardMachException
//
kern_return_t ExceptionHandlerApple::ForwardMachException(mach_port_t thread, mach_port_t task, exception_type_t exceptionType,
                                                          mach_exception_data_t pExceptionDetail, mach_msg_type_number_t exceptionDetailCount) 
{
    mach_msg_type_number_t i;
    kern_return_t          result;
    thread_state_data_t    threadState;
    mach_msg_type_number_t threadStateCount = THREAD_STATE_MAX;
        
    for(i = 0; i < mOldExceptionPorts.mCount; i++) // Find the first of the previously installed exception handlers which are set to hand this exception.
    {
        if(mOldExceptionPorts.mMasks[i] & (1 << exceptionType))
            break;
    }
    
    if(i == mOldExceptionPorts.mCount) // If none match...
        return KERN_FAILURE;
    
    mach_port_t           port     = mOldExceptionPorts.mPorts[i];
    exception_behavior_t  behavior = mOldExceptionPorts.mBehaviors[i];
    thread_state_flavor_t flavor   = mOldExceptionPorts.mFlavors[i];

    if(behavior != EXCEPTION_DEFAULT)
        thread_get_state(thread, flavor, threadState, &threadStateCount);
    
    switch(behavior)
    {
        case EXCEPTION_DEFAULT:
            result = mach_exception_raise_ea(port, thread, task, exceptionType, pExceptionDetail, exceptionDetailCount);
            break;
            
        case EXCEPTION_STATE:
            // Question: Why does this function lack thread and task arguments? We may need to fix up mach_exc.h and mach_excServer.c
            result = mach_exception_raise_state_ea(port, /*thread, task,*/ exceptionType, pExceptionDetail, 
                                                   exceptionDetailCount, &flavor, threadState, threadStateCount,
                                                   threadState, &threadStateCount);
            break;

        case EXCEPTION_STATE_IDENTITY:
            result = mach_exception_raise_state_identity_ea(port, thread, task, exceptionType, pExceptionDetail,
                                                            exceptionDetailCount, &flavor, threadState, threadStateCount,
                                                            threadState, &threadStateCount);
            break;
            
        default:
            result = KERN_FAILURE;
            break;
    }
    
    if(behavior != EXCEPTION_DEFAULT)
        result = thread_set_state(thread, flavor, threadState, threadStateCount);
    
    return result;
}



///////////////////////////////////////////////////////////////////////////////
// InitSignalHandler
//
bool ExceptionHandlerApple::InitSignalHandler()
{
    // Not yet implemented. The vectored (mach) exception handler is mostly better anyway.
    EA_FAIL();
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// ShutdownSignalHandler
//
void ExceptionHandlerApple::ShutdownSignalHandler()
{
    // Not yet implemented.
}



///////////////////////////////////////////////////////////////////////////////
// SetAction
//
void ExceptionHandlerApple::SetAction(ExceptionHandler::Action action, int returnValue)
{
    // Due to the way that Unix exception handling works, kActionTerminate is the 
    // only supported action. It is impossible to continue/recover from an exception
    // on the Unix and there is no way to re-throw an exception at the system level.

    mAction = action;
    mnTerminateReturnValue = returnValue;
}


///////////////////////////////////////////////////////////////////////////////
// GetAction
//
ExceptionHandler::Action ExceptionHandlerApple::GetAction(int* pReturnValue) const
{
    if(pReturnValue)
        *pReturnValue = mnTerminateReturnValue;
    return mAction;
}


///////////////////////////////////////////////////////////////////////////////
// SimulateExceptionHandling
//
void ExceptionHandlerApple::SimulateExceptionHandling(int exceptionType)
{
    memset(&mExceptionInfoApple.mContext, 0, sizeof(mExceptionInfoApple.mContext));

    // It turns out that the the following code to get the state of the current thread will
    // always fail or give the wrong info, as the kernel thread_get_state function can't return the  
    // state of the current thread. Instead we will need to spawn a second thread which pauses this 
    // thread and reads its state then returns to us what it got. Let's just implement that within
    // the EACallstack package and use it from there.
    //
    //thread_act_t           threadSelf = mach_thread_self(); // pthread_mach_thread_np(pthread_self());
    //thread_state_flavor_t  flavor = MACHINE_THREAD_STATE;   // Get default type of state info for the current CPU environment.
    //thread_state_data_t    oldStateArray;                   // This happens to be an array.
    //mach_msg_type_number_t oldStateCount = THREAD_STATE_MAX;
    //
    //kern_return_t stateResult = thread_get_state(threadSelf, flavor, oldStateArray, &oldStateCount);
    //
    //if(stateResult == KERN_SUCCESS)
    //{
    //    ReadThreadExceptionContext(threadSelf, oldStateArray);
    //}
    //else
    {
        EA::Callstack::CallstackContext callstackContext;
        EA::Callstack::GetCallstackContext(callstackContext, (intptr_t)EA::Thread::kThreadIdInvalid);
    
        #if defined(EA_PROCESSOR_ARM) 
            mExceptionInfoApple.mContext.mGpr[11] = callstackContext.mFP;   /// Frame pointer; register 11 for ARM instructions, register 7 for Thumb instructions.
            mExceptionInfoApple.mContext.mGpr[13] = callstackContext.mSP;   /// Stack pointer; register 13
            mExceptionInfoApple.mContext.mGpr[14] = callstackContext.mLR;   /// Link register; register 14
            mExceptionInfoApple.mContext.mGpr[15] = callstackContext.mPC;   /// Program counter; register 15

        #elif defined(EA_PROCESSOR_X86) 
            mExceptionInfoApple.mContext.Eip = callstackContext.mEIP;      /// Instruction pointer.
            mExceptionInfoApple.mContext.Esp = callstackContext.mESP;      /// Stack pointer.
            mExceptionInfoApple.mContext.Ebp = callstackContext.mEBP;      /// Base pointer.

        #elif defined(EA_PROCESSOR_X86_64)
            mExceptionInfoApple.mContext.Rip = callstackContext.mRIP;      /// Instruction pointer.
            mExceptionInfoApple.mContext.Rsp = callstackContext.mRSP;      /// Stack pointer.
            mExceptionInfoApple.mContext.Rbp = callstackContext.mRBP;      /// Base pointer.

        #elif defined(EA_PROCESSOR_POWERPC)
            mExceptionInfoApple.mContext.mGpr[1] = callstackContext.mGPR1; /// General purpose register 1.
            mExceptionInfoApple.mContext.mIar    = callstackContext.mIAR;  /// Instruction address pseudo-register.
        #endif    
    }

    mExceptionInfoApple.mThreadId    = pthread_self();
    mExceptionInfoApple.mSysThreadId = pthread_mach_thread_np(mExceptionInfoApple.mThreadId);  // Get self as a mach kernel thread type.
    pthread_getname_np(mExceptionInfoApple.mThreadId, mExceptionInfoApple.mThreadName, sizeof(mExceptionInfoApple.mThreadName));

    // To do: Fix the following to really use one of our platform-specific exception types.
    // We may need to have different handling based on what mode we are in.
    mExceptionInfoApple.mExceptionCause = (uint64_t)exceptionType; 

    const time_t timeValue = time(NULL);
    mExceptionInfoApple.mTime = *localtime(&timeValue);

    #if defined(EA_PROCESSOR_ARM) 
        mExceptionInfoApple.mpExceptionInstructionAddress = (void*)(uintptr_t)mExceptionInfoApple.mContext.mGpr[15];
    #elif defined(EA_PROCESSOR_X86) 
        mExceptionInfoApple.mpExceptionInstructionAddress = (void*)(uintptr_t)mExceptionInfoApple.mContext.Eip;
    #elif defined(EA_PROCESSOR_X86_64)
        mExceptionInfoApple.mpExceptionInstructionAddress = (void*)(uintptr_t)mExceptionInfoApple.mContext.Rip;
    #elif defined(EA_PROCESSOR_POWERPC)
        mExceptionInfoApple.mpExceptionInstructionAddress = (void*)(uintptr_t)mExceptionInfoApple.mContext.mIar;
    #endif    

    // To do: Fill out the following with appropriate fake values. This is not a high-priority to-do.
    mExceptionInfoApple.mCPUExceptionId          = 0;
    mExceptionInfoApple.mCPUExceptionDetail      = 0;
    mExceptionInfoApple.mpExceptionMemoryAddress = NULL;
            
    // We do nothing more than make an admittedly fake description.
    mExceptionInfoApple.mExceptionDescription.sprintf("Simulated exception %d", exceptionType);

    if(mpOwner)
    {
        // In the case of other platforms, we disable the exception handler here.
        // That would do no good on the Unix because exceptions are unilaterally
        // terminating events and if another exception occurred we would just be 
        // shut down by the system.

        mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingBegin);

        if(mpOwner->GetReportTypes() & ExceptionHandler::kReportTypeException)
            mpOwner->WriteExceptionReport();

        mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingEnd);
    }
    
    if(mAction == ExceptionHandler::kActionTerminate)
        ::exit(mnTerminateReturnValue);

    if(mAction == ExceptionHandler::kActionThrow)
    {
        // To do: To propertly implement this depends on mMode. In practice this simulation
        // functionality is more meant for exercizing the above report writing and so it 
        // may not be important to implement this throw.
    }

    // kActionContinjue, kActionDefault
    // Else let the execution continue from the catch function. This is an exception
    // simulation function so there's no need to do anything drastic.
}


///////////////////////////////////////////////////////////////////////////////
// RunTrapped
//
intptr_t ExceptionHandlerApple::RunTrapped(ExceptionHandler::UserFunctionUnion userFunctionUnion, ExceptionHandler::UserFunctionType userFunctionType, void* pContext, bool& exceptionCaught)
{
    intptr_t returnValue = 0;

    exceptionCaught = false;

    if(mbEnabled)
    {
        mbExceptionOccurred = false;

        if(mMode == ExceptionHandler::kModeCPP)
        {
            // The user has compiler exceptions disabled, but is using kModeCPP. This is a contradiction
            // and there is no choice but to skip using C++ exception handling. We could possibly assert
            // here but it might be annoying to the user.
            
            #if defined(EA_COMPILER_NO_EXCEPTIONS)
                returnValue = ExceptionHandler::ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext);
            #else
                try
                {
                    returnValue = ExceptionHandler::ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext);
                }
                catch(std::exception& e)
                {
                    HandleCaughtCPPException(e);
                    exceptionCaught = true;
                }
                // The following catch(...) will catch all C++ language exceptions (i.e. C++ throw), 
                // and with some compilers (e.g. VC++) it can catch processor or system exceptions 
                // as well, depending on the compile/link settings.
                catch(...)
                {
                    std::runtime_error e(std::string("generic"));
                    HandleCaughtCPPException(e);
                    exceptionCaught = true;
                }
            #endif
        }
        else // kModeSignalHandling, kModeVectored
        {
            // If there's an exception then we'll catch it asynchronously (kModeSignalHandling) or in another thread (kModeVectored).
            returnValue = ExceptionHandler::ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext);
        }
    }
    else
        returnValue = ExceptionHandler::ExecuteUserFunction(userFunctionUnion, userFunctionType, pContext);

    mbExceptionOccurred = exceptionCaught;
    return returnValue;
}


void ExceptionHandlerApple::HandleCaughtCPPException(std::exception& e)
{    
    if(mbHandlingInProgress.SetValueConditional(1, 0)) // Set it true (1) from false (0) atomically, evaluating as true if somebody else didn't do it first.
    {
        mbExceptionOccurred = true;
            
        // It's not possible to tell where the exception occurred with C++ exceptions, unless the caller provides info via 
        // a custom exception class or at least via the exception::what() function. To consider: Make a standardized 
        // Exception C++ class for this package which users can use and provides some basic info, including location 
        // and possibly callstack.
        mExceptionInfoApple.mpExceptionInstructionAddress = NULL;
        mExceptionInfoApple.mCPUExceptionId               = 0;
        mExceptionInfoApple.mCPUExceptionDetail           = 0;
        mExceptionInfoApple.mpExceptionMemoryAddress      = NULL;
        
        mExceptionInfoApple.mExceptionDescription.assign(e.what());

        // There is currently no CPU context information for a C++ exception.
        memset(&mExceptionInfoApple.mContext, 0, sizeof(mExceptionInfoApple.mContext));

        // Save the thread id.
        mExceptionInfoApple.mThreadId    = EA::Thread::GetThreadId(); // C++ exception handling occurs in the same thread as the exception occurred.
        mExceptionInfoApple.mSysThreadId = pthread_mach_thread_np(mExceptionInfoApple.mThreadId);  // Get self as a mach kernel thread type.
        pthread_getname_np(mExceptionInfoApple.mThreadId, mExceptionInfoApple.mThreadName, sizeof(mExceptionInfoApple.mThreadName));

        // Record exception time
        const time_t timeValue = time(NULL);
        mExceptionInfoApple.mTime = *localtime(&timeValue);

        // Record the exception callstack
        // Unfortunately, this will return the callstack where we are now and not where the throw was.
        mExceptionInfoApple.mCallstackEntryCount = EA::Thread::GetCallstack(mExceptionInfoApple.mCallstack, EAArrayCount(mExceptionInfoApple.mCallstack), NULL); // C++ exception handling occurs in the same thread as the exception occurred.

        if(mpOwner)
        {
            // Notify clients about exception handling start.
            mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingBegin);

            // Write exception report
            if(mpOwner->GetReportTypes() & ExceptionHandler::kReportTypeException)
                mpOwner->WriteExceptionReport();

            // Notify clients about exception handling end.
            mpOwner->NotifyClients(ExceptionHandler::kExceptionHandlingEnd);
        }

        mbHandlingInProgress = 0;
    }

    if(mAction == ExceptionHandler::kActionTerminate)
        ::exit(mnTerminateReturnValue);

    if(mAction == ExceptionHandler::kActionThrow)
    {
        #if defined(EA_COMPILER_NO_EXCEPTIONS)
            EA_FAIL_MESSAGE("ExceptionHandlerApple::HandleCaughtCPPException: Unable to throw, as compiler C++ exception support is disabled.");
        #else
            throw(e);
        #endif
    }

    // Else let the execution continue from the catch function.
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionCallstack
//
size_t ExceptionHandlerApple::GetExceptionCallstack(void* pCallstackArray, size_t capacity) const
{
    if(capacity > mExceptionInfoApple.mCallstackEntryCount)
        capacity = mExceptionInfoApple.mCallstackEntryCount;
    
    memmove(pCallstackArray, mExceptionInfoApple.mCallstack, capacity * sizeof(void*));

    return capacity;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionContext
//
const EA::Callstack::Context* ExceptionHandlerApple::GetExceptionContext() const
{
    return &mExceptionInfoApple.mContext;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionThreadId
//
EA::Thread::ThreadId ExceptionHandlerApple::GetExceptionThreadId(EA::Thread::SysThreadId& sysThreadId, char* threadName, size_t threadNameCapacity) const
{
    sysThreadId = mExceptionInfoApple.mSysThreadId;
    EA::StdC::Strlcpy(threadName, mExceptionInfoApple.mThreadName, threadNameCapacity);
    
    return mExceptionInfoApple.mThreadId;
}


///////////////////////////////////////////////////////////////////////////////
// GetExceptionDescription
//
size_t ExceptionHandlerApple::GetExceptionDescription(char* buffer, size_t capacity)
{
    return EA::StdC::Strlcpy(buffer, mExceptionInfoApple.mExceptionDescription.c_str(), capacity);
}




} // namespace Debug

} // namespace EA





#endif  // #if defined(EA_PLATFORM_APPLE)












