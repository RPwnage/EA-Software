/*************************************************************************************************/
/*! \file


    This file has functions for handling the per thread allocation system.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/system/shellexec.h"
#include <EAIO/PathString.h> // for EA::IO::Path methods in validateCommandLine

#if defined(EA_PLATFORM_LINUX)
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#elif defined(EA_PLATFORM_WINDOWS)
#include <stdio.h>
#define popen _popen
#define pclose _pclose
#endif


namespace Blaze
{
int shellExecInternal(const char8_t* command, int* status, eastl::string& output);

bool validateCommandLine(ShellExecCommandType commandType, const eastl::string& mainExecutablePath, const ShellExecArgList &arguments, const eastl::string& builtCommandLine);
void buildCommandLine(const eastl::string& mainExecutablePath, const ShellExecArgList &arguments, eastl::string& builtCommandLine, eastl::string& censoredCommandLine);

ShellRunner::ShellRunner(const char8_t* baseName)
    : BlazeThread("shell_runner", BlazeThread::SHELL_RUNNER, 256 * 1024),
      mShutdown(false),
      mNextJobId(0)
{
    char8_t name[256];
    blaze_snzprintf(name, sizeof(name), "%sSR", baseName);
    BlazeThread::setName(name);
}

ShellRunner::~ShellRunner()
{
}

BlazeRpcError ShellRunner::shellExec(ShellExecCommandType commandType, const eastl::string& mainExecutablePath,
              const ShellExecArgList& arguments, int* status, eastl::string& output,
              eastl::string& builtCommandLine, eastl::string& censoredCommandLine,
              ShellJobId& jobId, const TimeValue& absoluteTimeout)
{
    BlazeRpcError rc = ERR_OK;

    buildCommandLine(mainExecutablePath, arguments, builtCommandLine, censoredCommandLine);

    if (!validateCommandLine(commandType, mainExecutablePath, arguments, builtCommandLine))
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[shellExec] Error locating python script. Failed to validate command line '" << builtCommandLine.c_str() << "'.");
        return ERR_SYSTEM;
    }

    Fiber::EventHandle eventHandle = Fiber::getNextEventHandle();

    mMutex.Lock();

    jobId = mNextJobId++;
    mQueue.push_back(ShellJob(jobId, builtCommandLine.c_str(), status, &output, eventHandle));
    mMutex.Unlock();

    mCond.Signal(true);

    rc = Fiber::wait(eventHandle, "ShellRunner::shellExec", absoluteTimeout);

    if (rc != ERR_OK)
    {
        cancelShellExec(jobId);
    }

    return rc;
}

void ShellRunner::cancelShellExec(ShellJobId jobId)
{
    mMutex.Lock();
    for (Queue::iterator itr = mQueue.begin(), end = mQueue.end(); itr != end; ++itr)
    {
        if (itr->mJobId == jobId)
        {
            Fiber::EventHandle eventHandle = itr->mEventHandle;
            mQueue.erase(itr);
            mMutex.Unlock();
            Fiber::signal(eventHandle, Blaze::ERR_OK);
            return;
        }
    }

    mMutex.Unlock();
}

void ShellRunner::run()
{
    while (!mShutdown)
    {
        mMutex.Lock();
        if (mQueue.empty())
            mCond.Wait(&mMutex);
        if (mShutdown)
        {
            mMutex.Unlock();
            break;
        }

        if (mQueue.empty())
        {
            mMutex.Unlock();
            continue;
        }

        ShellJob& job = mQueue.front();
        ShellJobId id = job.mJobId;
        eastl::string command(job.mCommand);
        mMutex.Unlock();

        int status;
        eastl::string output;

        int returnCode = shellExecInternal(command.c_str(), &status, output);

        mMutex.Lock();
        if (!mQueue.empty())
        {
            ShellJob& item = mQueue.front();
            if (item.mJobId == id)
            {
                *(item.mOutput) = output;
                *(item.mStatus) = status;

                Fiber::signal(item.mEventHandle, returnCode < 0 ? Blaze::ERR_SYSTEM : Blaze::ERR_OK);
                mQueue.pop_front();
            }
        }
        mMutex.Unlock();

    }
}

void ShellRunner::stop()
{
    if (!mShutdown)
    {
        mShutdown = true;
        mCond.Signal(true);
        waitForEnd();
    }
}

#if defined(EA_PLATFORM_LINUX)

int shellExecInternal(const char8_t* command, int* status, eastl::string& output)
{
    enum BlazeExecIoTypes
    {
        BLAZE_EXEC_IO_IN = 0,
        BLAZE_EXEC_IO_OUT,
        BLAZE_EXEC_IO_MAX
    };
    
    int fd[BLAZE_EXEC_IO_MAX];
    
    *status = 0;
    if (pipe(fd) < 0)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[shellExec] pipe() failed with errno " << errno << ". Could not execute command line '" << command << "'.");
        return -1;
    }

    const pid_t pid = syscall(SYS_fork); //Use the sys fork here to avoid an issue with libc/malloc and fork()

    int ret = 0;
    if(pid == 0)
    {
        // Child process closes up input side of pipe
        close(fd[BLAZE_EXEC_IO_IN]);

        // Duplicate the output part of the pipe into stdout
        dup2(fd[BLAZE_EXEC_IO_OUT], BLAZE_EXEC_IO_OUT);
        
        // Close output side of pipe, since it's now bound to stdout
        close(fd[BLAZE_EXEC_IO_OUT]);

        ret = execl("/bin/sh", "sh", "-c", command, nullptr);
        if(ret < 0)
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, "[shellExec] execl() failed with errno " << errno << ". Could not execute command line '" << command << "'.");
            exit(ret);
        }
    }
    else
    {
        // Parent process closes up output side of pipe
        close(fd[BLAZE_EXEC_IO_OUT]);

        if (pid > 0)
        {
            // Open a buffered file(using raw pipe reads does not work)
            FILE* fp = fdopen(fd[BLAZE_EXEC_IO_IN], "r");

            if (fp != nullptr)
            {
                char buf[1024];
                output.reserve(sizeof(buf));
                output.append(4, ' ');
                while (fgets(buf, sizeof(buf), fp) != nullptr)
                {
                    output += buf;
                    if (output.back() == '\n')
                        output.append(4, ' ');
                }

                fclose(fp);
            }
            else
            {
                ret = -3;
                BLAZE_ERR_LOG(Log::CONTROLLER, "[shellExec] fdopen() failed with errno " << errno << ". Could not execute command line '" << command << "'.");
            }

            // wait for child to exit
            waitpid(pid, status, 0);            
        }
        else
        {
            ret = -4;
            BLAZE_ERR_LOG(Log::CONTROLLER, "[shellExec] call to SYS_fork failed with errno " << errno << ". Could not execute command line '" << command << "'.");
        }
        
        // Parent process closes up input side of pipe
        close(fd[BLAZE_EXEC_IO_IN]);
    }

    return ret;
}

#elif defined(EA_PLATFORM_WINDOWS)

int shellExecInternal(const char8_t* command, int* status, eastl::string& output)
{
    int ret = 0;
    *status = 0;
    FILE * fp = popen(command, "r");
    if(fp != nullptr)
    {
        char8_t buf[1024];
        output.reserve(sizeof(buf));
        output.append(4, ' ');
        while (fgets(buf, sizeof(buf), fp) != nullptr)
        {
            output += buf;
            if (output.back() == '\n')
                output.append(4, ' ');
        }
        *status = pclose(fp);
    }
    else
    {
        // fail to run shell command
        BLAZE_ERR_LOG(Log::CONTROLLER, "[shellExec] popen() failed with errno " << errno << ". Could not execute command line '" << command << "'.");
        ret = -1;
    }
    return ret;
}

#endif


/*! \brief for security validate command line follows an expected format. */
bool validateCommandLine(ShellExecCommandType commandType, const eastl::string& mainExecutablePath,
                         const ShellExecArgList &arguments, const eastl::string& builtCommandLine)
{
    switch (commandType)
    {
    case SHELLEXEC_PYTHON_DBMIG:
        if (EA::IO::Path::GetFileName(mainExecutablePath.c_str()) != EA::IO::Path::PathString8("python"))
            return false;
        // 1st arg must be dbmig script.
        if (EA::IO::Path::GetFileName(arguments.begin()->getValue().c_str()) != EA::IO::Path::PathString8("dbmig.py"))
            return false;

        return true;
    case SHELLEXEC_PYTHON_SENDMAIL:
        if (EA::IO::Path::GetFileName(mainExecutablePath.c_str()) != EA::IO::Path::PathString8("python"))
            return false;
        // 1st arg must be the sendmail script.
        if (EA::IO::Path::GetFileName(arguments.begin()->getValue().c_str()) != EA::IO::Path::PathString8("sendmail.py"))
            return false;
        return true;
    default:
        BLAZE_ERR_LOG(Log::CONTROLLER, "[shellExec] internal validation error, unhandled command type '" << commandType << "'. Failed to validate command line '" << builtCommandLine.c_str() << "'.");
        return false;
    };
}

/*! \brief builds the full command line to run. */
void buildCommandLine(const eastl::string& mainExecutablePath, const ShellExecArgList &arguments, eastl::string& builtCommandLine, eastl::string& censoredCommandLine)
{
    builtCommandLine.assign(mainExecutablePath);
    censoredCommandLine.assign(builtCommandLine);
    ShellExecArgList::const_iterator iter = arguments.begin();
    ShellExecArgList::const_iterator end = arguments.end();
    for (; iter != end; ++iter)
    {
        // space delimited
        builtCommandLine.append_sprintf(" %s", iter->getValue().c_str());
        censoredCommandLine.append_sprintf(" %s", iter->getCensoredValue().c_str());
    }
    BLAZE_INFO_LOG(Log::CONTROLLER, "[shellExec] '" << censoredCommandLine.c_str() << "' ...");
}

} // namespace Blaze

