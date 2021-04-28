/*************************************************************************************************/
/*! \file


    This file has functions for platform specific shell execution and output capture.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


#ifndef BLAZE_SHELL_EXEC_H
#define BLAZE_SHELL_EXEC_H

#include "EASTL/string.h"
#include "EASTL/vector.h"

#if defined(EA_PLATFORM_WINDOWS)
#define WIFEXITED(ret) (1) // on linux this macro describes if the process terminated on it's own
#define WEXITSTATUS(ret) (ret) // on linux this macro can only be used if WIFEXITED(ret) != 0
#endif

namespace Blaze
{
    /*! \brief allowed command types. command lines are validated within shellExec. */
    enum ShellExecCommandType
    {
        SHELLEXEC_PYTHON_DBMIG = 0x0,
        SHELLEXEC_PYTHON_SENDMAIL = 0x1
    };

    class ShellExecArg
    {
    public:
        ShellExecArg(const char8_t* arg, const char8_t* asCensored = "") :
            mArg((arg!= nullptr)? arg : ""), mAsCensoredString((asCensored != nullptr)? asCensored : "") {}
        
        const eastl::string& getValue() const { return mArg; }
        const eastl::string& getCensoredValue() const { return (mAsCensoredString.empty()? mArg : mAsCensoredString); }
    private:
        eastl::string mArg;
        eastl::string mAsCensoredString;
    };
    typedef eastl::vector<ShellExecArg> ShellExecArgList;


class ShellRunner : public BlazeThread
{
public:
    ShellRunner(const char8_t* baseName);
    ~ShellRunner() override;

    typedef uint32_t ShellJobId;
    static const ShellJobId INVALID_JOB_ID = 0xFFFFFFFF;

    void stop() override;

    BlazeRpcError DEFINE_ASYNC_RET(shellExec(
        ShellExecCommandType commandType, const eastl::string& mainExecutablePath, const ShellExecArgList& commandArgs, // Inputs
        int* status, eastl::string& output, eastl::string& builtCommandLine, eastl::string& censoredCommandLine,    // Output from command
        ShellJobId& jobId, const TimeValue& absoluteTimeout = TimeValue()));                                            // Job id and timeout
    void cancelShellExec(ShellJobId jobId);

private:

    struct ShellJob
    {
        ShellJobId mJobId;

        const char8_t* mCommand;

        int* mStatus;
        eastl::string* mOutput;

        Fiber::EventHandle mEventHandle;

        ShellJob(ShellJobId jobId, const char8_t* command, int* status, eastl::string* output, Fiber::EventHandle handle)
            : mJobId(jobId), mCommand(command), mStatus(status), mOutput(output), mEventHandle(handle) { }
                 
    };

    typedef eastl::deque<ShellJob> Queue;
    Queue mQueue;

    EA::Thread::Mutex mMutex;
    EA::Thread::Condition mCond;
    volatile bool mShutdown;
    EA::Thread::Thread mThread;
    ShellJobId mNextJobId;

    void run() override;
};

extern EA_THREAD_LOCAL ShellRunner* gShellRunner;

}

#endif // BLAZE_SHELL_EXEC_H
