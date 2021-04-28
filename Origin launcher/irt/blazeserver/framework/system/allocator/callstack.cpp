/*************************************************************************************************/
/*! \file


    This file has functions for handling the per thread allocation system.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/system/allocator/callstack.h"
#include "EASTL/fixed_string.h"
#include "framework/tdf/controllertypes_server.h"

#if defined(EA_PLATFORM_LINUX)
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <cxxabi.h> // for abi::__cxa_demangle
#elif defined(EA_PLATFORM_WINDOWS)
#include <DbgHelp.h> // symbol resolution
#endif


#if defined(EA_PLATFORM_WINDOWS)
/**
 * Used to initialize sybmol database on process startup(before any threads are started).
 * Necessary for printing backtraces on Windows.
 */
struct WindowsDebugSymbolLoader
{
    WindowsDebugSymbolLoader()
    {
        SymSetOptions(SYMOPT_UNDNAME|SYMOPT_LOAD_LINES);
        SymInitialize(GetCurrentProcess(), nullptr, TRUE);
    }
} gWindowsDebugSymbolLoader;
#endif

/**
 * ignoreFrames > 0 is useful for omitting the top frames of the stack
 * snapshot especially when the snapshot is taken from within several
 * levels of unchanging wrapper code (e.g.: EA_ASSERT).

 * keepFrames are useful if you want a shorter callstack.
 */
void CallStack::getStack(uint32_t ignoreFrames, uint32_t keepFrames)
{
    void* stack[MEM_TRACK_CALLSTACK_DEPTH+MAX_FRAMES_TO_IGNORE];

    if (ignoreFrames > MAX_FRAMES_TO_IGNORE)
        ignoreFrames = MAX_FRAMES_TO_IGNORE;

    if (keepFrames > MEM_TRACK_CALLSTACK_DEPTH)
        keepFrames = MEM_TRACK_CALLSTACK_DEPTH;

    size_t callstackSize;

#if defined(EA_PLATFORM_LINUX)
    callstackSize = (size_t) backtrace(stack, EAArrayCount(stack));
#elif defined(EA_PLATFORM_WINDOWS)
    callstackSize = (size_t) CaptureStackBackTrace(0, EAArrayCount(stack), stack, nullptr);
#endif
    
    if (callstackSize > ignoreFrames)
        callstackSize -= ignoreFrames;
    else
        ignoreFrames = 0; // only ignore frames if the captured stack was big enough

    if (callstackSize > keepFrames)
        callstackSize = keepFrames;// clamp to what we need

    if (callstackSize > EAArrayCount(mData)) 
        callstackSize = EAArrayCount(mData); //clamp to what we can hold
    
    mCallStackSize = callstackSize;
    memcpy(mData, stack + ignoreFrames, callstackSize * sizeof(mData[0]));
}

/**
 *  NOTE: On Linux getSymbols() does not return line numbers but its easy to get them by performing the following operation:
 *        addr2line -e /path/to/non-stripped/.../my-buggy-app 0x4a6889 0x4a8b43 0x4e8765
 *
 *        The address values in the backtrace should have 5(size of the CALL instruction on x86_64 is 5 bytes) subtracted to give the actual instruction address
 *        Example:
 *            [ 0] Blaze::GamePacker::GamePackerSlaveImpl::cleanupSession(unsigned long, char const*) [0x72e61c0]\n
 *            [ 1] Blaze::GamePacker::GamePackerSlaveImpl::completeJob(Blaze::BlazeError, unsigned long, Blaze::GameManager::MatchmakingResult, Blaze::GameManager::PlayerRemovedReason) [0x72f2591]\n
 *            [ 2] Blaze::GamePacker::GamePackerSlaveImpl::finalizePackedGame(unsigned long) [0x72eea15]\n
 *            [ 3] Blaze::MethodCall1Job<Blaze::GamePacker::GamePackerSlaveImpl, unsigned long, Blaze::FiberJobQueue::InternalJob>::execute() [0x76ff5f2]\n
 *            [ 4] Blaze::MethodCall0Job<Blaze::FiberJobQueue::InternalJob, Blaze::Job>::execute() [0x6985266]\n
 *            [ 5] Blaze::Fiber::run() [0x677049d]\n[ 6] Blaze::Fiber::runFuncBootstrap(void*, void*) [0x676e696]\n
 *            [ 7] switch_fiber [0x65bbca9]
 * 
 *            subtract 5 (size of CALL instruction) from the stack address (0x72f2591) - 5 = 0x72f258c
 *
 *            user@eadpl0041:bin$ addr2line -e blazeserver 0x72f258c
 *            /opt/local/user/dev/gs3/blazeserver/component/gamemanager/gamepacker/gamepackerslaveimpl.cpp:1560 <--- This is the real line number
 */
void CallStack::getSymbols(Blaze::MemTrackerStatus::StackTraceList& list) const
{
    if (mCallStackSize == 0)
        return;

    list.reserve(mCallStackSize);
#if defined(EA_PLATFORM_LINUX)
    char8_t** result = backtrace_symbols(mData, (int32_t) mCallStackSize);
    if (result != nullptr)
    {
        eastl::fixed_string<char8_t, 1024> stackTraceLine;
        for (uint32_t i = 0; i < mCallStackSize; ++i)
        {
            stackTraceLine.sprintf("[%2u] ", i);
            // find parentheses and +address offset surrounding the mangled name: ./module(function+0x15c) [0x8048a6d]
            char8_t* beginName = strrchr(result[i], '(');
            bool symbolFound = false;
            if (beginName != nullptr)
            {
                ++beginName;
                char8_t* endName = strpbrk(beginName, "+)");
                if (endName != nullptr)
                {
                    *endName = '\0'; // 0 term the name
                    int32_t status = 0;
                    char8_t* demangled = abi::__cxa_demangle(beginName, (char8_t*)nullptr, (size_t*)nullptr, &status);
                    if (status == 0)
                    {
                        // demangle succeeded, copy demangled name
                        stackTraceLine += demangled;
                    }
                    else
                    {
                        // demangling failed, copy mangled name
                        stackTraceLine.append(beginName, endName-beginName);
                    }
                    free(demangled);
                    stackTraceLine.append_sprintf(" [0x%p]", mData[i]);
                    symbolFound = true;
                }
            }
            if (!symbolFound)
            {
                // failed to extract symbol name, store full symbol entry
                stackTraceLine += result[i];
            }
            list.push_back(stackTraceLine.c_str());
        }

        // need to free result array, strings themselves do not need to be freed
        free(result);
        return;
    }
#elif defined(EA_PLATFORM_WINDOWS)
    
    SYMBOL_INFO* symbol = (SYMBOL_INFO *) BLAZE_ALLOC(sizeof(SYMBOL_INFO) + 256 * sizeof(char8_t));
    symbol->MaxNameLen  = 256;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    DWORD disp = 0;
    IMAGEHLP_LINE64 lineInfo;
    memset(&lineInfo, 0, sizeof(lineInfo));
    lineInfo.SizeOfStruct = sizeof(lineInfo);

    HANDLE process = GetCurrentProcess();

    eastl::fixed_string<char8_t, 1024> stackTraceLine;
    for (uint32_t i = 0; i < mCallStackSize; i++)
    {
        bool symbolOk = false;
        stackTraceLine.append_sprintf("[%2u] ", i);
        if (SymFromAddr(process, (DWORD64)(mData[i]), 0, symbol))
        {
            stackTraceLine.append(symbol->Name, symbol->NameLen);
            stackTraceLine.append("() ! ");
            symbolOk = true;
        }
        bool linenumOk = false;
        if (SymGetLineFromAddr64(process, (DWORD64)(mData[i]), &disp, &lineInfo))
        {
            stackTraceLine.append(lineInfo.FileName);
            stackTraceLine.append_sprintf(":%d", lineInfo.LineNumber);
            linenumOk = true;
        }
        if (symbolOk || linenumOk)
        {
            list.push_back(stackTraceLine.c_str());
        }
        stackTraceLine.clear();
    }

    BLAZE_FREE(symbol);

    if (!list.empty())
        return;

    list.clear();
#endif
    // if we get here that means the text symbols were not found,
    // print out the hex addresses of the stack trace.
    for (size_t i = 0; i < mCallStackSize; ++i)
    {
        char8_t buf[32];
        blaze_snzprintf(buf, sizeof(buf), "0x%p", mData[i]);
        list.push_back(buf);
    }
}

void CallStack::getSymbols(eastl::string& callstack) const
{
    Blaze::MemTrackerStatus::StackTraceList list;
    getSymbols(list);

    for (Blaze::MemTrackerStatus::StackTraceList::const_iterator it = list.begin(), end = list.end(); it != end; ++it)
    {
        callstack.append_sprintf("%s\n", it->c_str());
    }
}

void CallStack::getCurrentStackSymbols(Blaze::MemTrackerStatus::StackTraceList& list)
{
    CallStack stack;
    stack.getStack();
    stack.getSymbols(list);
}

void CallStack::getCurrentStackSymbols(eastl::string& callstack, uint32_t ignoreFrames, uint32_t keepFrames)
{
    CallStack stack;
    stack.getStack(ignoreFrames, keepFrames);
    stack.getSymbols(callstack);
}
