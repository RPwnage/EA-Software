/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class NameResolver

    This class provides the ability to resolve hostnames in either a blocking or non-blocking
    fashion.  It is a singleton because the underlying system calls (gethostbyname) are not
    thread safe so there must only ever be one call being made in a given process at one time.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"

#include "framework/system/corefilewriter.h"
#include "framework/util/shared/blazestring.h"
#include "framework/system/threadlocal.h"

#ifdef GOOGLE_COREDUMPER_LIB
#include "google/coredumper.h"
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"
#endif

#ifdef EA_PLATFORM_LINUX
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#endif

namespace Blaze
{

/*** Public Methods ******************************************************************************/

CoreFileWriter::CoreFileWriter(const char *name, uint32_t coreBufferSize)
    : BlazeThread(name, EXCEPTION_HANDLER, 4096*1024), mQueue("CoreFileWriter"), mLastContext(nullptr)
{
    mShutdown = false;
    mCoreBuffer = malloc(coreBufferSize);
    mCoreBufferSize = coreBufferSize;
    mDumpNum = 0;
}

CoreFileWriter::~CoreFileWriter()
{
    // make sure thread is stopped
    // before everything is deleted
    stop();
    free(mCoreBuffer);
}

//  Signal thread to begin writing core file
//  If thread is already locked, fail (queuing core dump requests is not practical since core dumps are 
//      a snapshot of the current process.  Also only one google-coredumper request can run at a time.
//  
BlazeRpcError CoreFileWriter::coreDump(const char8_t* fileNamePrefix, const char8_t* fileNameDetail, uint64_t coreSizeLimit, void* context, bool alwaysSignalMonitor)
{
#ifdef GOOGLE_COREDUMPER_LIB    

    mLastContext = context;

    mQueue.push(BLAZE_NEW MethodCall5Job<CoreFileWriter, const char8_t*, const char8_t*, const char8_t*, uint64_t, bool>
        (this, &CoreFileWriter::dumpCoreFile, fileNamePrefix, fileNameDetail, gController->getInstanceName(), coreSizeLimit, alwaysSignalMonitor));

    //don't do anything until the process has forked w/ our thread state
    mCoreWrittenSignal.Wait();

    mLastContext = nullptr;

    return ERR_OK;
#elif defined(EA_PLATFORM_LINUX)
    // call directly the function as we want the context of the current stack when not using the coredumper library
    dumpCoreFile(alwaysSignalMonitor);
    return ERR_OK;
#else
    BLAZE_WARN_LOG(Log::SYSTEM, "[CoreFileWriter].coreDump() : Core dump request not queued.  No operation on this platform.");
    return ERR_SYSTEM;    
#endif
}


void CoreFileWriter::dumpCoreFile(const char8_t* fileNamePrefix, const char8_t* fileNameDetail, const char8_t* instanceName, uint64_t coreSizeLimit, bool alwaysSignalMonitor)
{
#ifdef GOOGLE_COREDUMPER_LIB
    BLAZE_INFO_LOG(Log::SYSTEM, "[CoreFileWriter].dumpCoreFile() : Beginning core dump.");

    int coreFileHandle = -1;

    //  generate coredump using this thread as the crash context.   the core writer thread will write to a file using the
    //  returned read handle for the core dump.
    CoredumperCompressor *compressor;
    coreFileHandle = ::GetCompressedCoreDump(COREDUMPER_GZIP_COMPRESSED, &compressor);

    if (coreFileHandle < 0)
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[CoreFileWriter].dumpCoreFile() : Failed to retrieve core dump (errno=" << errno << ")");
        if (alwaysSignalMonitor && gProcessController->getMonitorPid() != 0)
            kill(gProcessController->getMonitorPid(), SIGUSR1);
        return;
    }

    eastl::string coreFileName;
    coreFileName.sprintf("%s-%s-%s.%" PRId64 ".%d.%d%s", fileNamePrefix, fileNameDetail, instanceName, TimeValue::getTimeOfDay().getSec(), getpid(), mDumpNum, compressor->suffix);
    ++mDumpNum;

    BLAZE_INFO_LOG(Log::SYSTEM, "[CoreFileWriter].dumpCoreFile() : Writing core file " << coreFileName.c_str() << ", compression=" << compressor->compressor);

    //Here we do another fork.  The core writer library has already forked, but because we don't want to block on writing the core file (possibly enough time
    //to end up erroring out transactions) AND because any potential crash could kill the server before the writing thread is finished, we instead 
    //fork again to ensure that the core file will complete no matter what happens after this.
    //After the fork, the parent will close the pipe returned from the first child process.  The second child process will actually make use of the 
    //pipe to do the core writing.
    pid_t pid = syscall(SYS_fork); //Use the sys fork here to avoid an issue with libc/malloc and fork()
    
    if (pid == -1)
    {
        if (alwaysSignalMonitor && gProcessController->getMonitorPid() != 0)
            kill(gProcessController->getMonitorPid(), SIGUSR1);

        //The fork failed.  Signal the main thread to continue. 
        mCoreWrittenSignal.Post();
        return;
    }

    if (pid != 0)
    {
        //close the pipe, we don't want it
        close(coreFileHandle);

        //At this point the processes have been forked and the child core writer process has a COW version of our process memory
        //and the thread state.  We can unlock the main cored thread and continue on our way before we wait on writing to avoid
        //blocking further calls
        mCoreWrittenSignal.Post();

        //The core file writer thread on the main process will wait for the writer child to die before accepting another core.  
        //This will limit the number of active core writers
        waitpid(pid, nullptr, 0);
        return;
    }
    
    //We're the child dump the core and exit
    const uint64_t MAX_CORE_SIZE_LIMIT = 4LL*1024LL*1024LL*1024LL;    // 4GB
    if (coreSizeLimit == 0)
        coreSizeLimit = MAX_CORE_SIZE_LIMIT;

    bool signalMonitor = alwaysSignalMonitor;

    //  retrieve compressed core file handle
    if (coreFileHandle >= 0)
    {
        int outfile = open(coreFileName.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0600);
        if (outfile < 0)
        {
            BLAZE_WARN_LOG(Log::SYSTEM, "[CoreFileWriter].dumpCoreFile() : Failed to open file '" << coreFileName.c_str() << "' for core dump (errno=" << errno << ")");
        }
        else
        {
            uint64_t coreSizeTotal = 0;
            //  read a chunk at a time and write out to output file.
            int coresize = read(coreFileHandle, mCoreBuffer, mCoreBufferSize);
            while (coresize > 0)
            {   
                if (coresize > 0)
                {                
                    if (coreSizeTotal+coresize > coreSizeLimit)
                    {
                        BLAZE_WARN_LOG(Log::SYSTEM, "[CoreFileWriter].dumpCoreFile() : Failed to write full core dump information (size limit reached " << coreSizeLimit);
                        break;
                    }
                    if (write(outfile, mCoreBuffer, coresize) < 0)
                    {
                        BLAZE_WARN_LOG(Log::SYSTEM, "[CoreFileWriter].dumpCoreFile() : Failed to write core dump information");
                        break;
                    }
                    coreSizeTotal += coresize;
                    coresize = read(coreFileHandle, mCoreBuffer, mCoreBufferSize);    
                }                    
                
                if (coresize < 0)
                {
                    BLAZE_WARN_LOG(Log::SYSTEM, "[CoreFileWriter].dumpCoreFile() : Failed to read core dump information");
                }
                else
                {
                    signalMonitor = true;
                }
            }
            close(outfile);       
        }
        close(coreFileHandle);
    }

    // signal the monitor process
    if (signalMonitor && gProcessController->getMonitorPid() != 0)
        kill(gProcessController->getMonitorPid(), SIGUSR1);
    
    //Child dies here
    syscall(SYS_exit);
 #endif
}

// version of the function that doesn't use the thread which isn't required if not using the coredumper lib
void CoreFileWriter::dumpCoreFile(bool alwaysSignalMonitor)
{
#if defined(EA_PLATFORM_LINUX) // we are on linux but without using the coredumper library
    BLAZE_INFO_LOG(Log::SYSTEM, "[CoreFileWriter].dumpCoreFile() : Beginning core dump.");

    // when not using the coredumper library we will fork off to allow us to abort at the end of the function
    // it is similar to what is done with the coredumper library but for a different reason
    pid_t pid = syscall(SYS_fork); //Use the sys fork here to avoid an issue with libc/malloc and fork()
 
    // check for an error in the call   
    if (pid == -1)
    {
        if (alwaysSignalMonitor && gProcessController->getMonitorPid() != 0)
        {
            kill(gProcessController->getMonitorPid(), SIGUSR1);
        }
        return;
    }
    
    // make sure that this is a child process off the main process
    if (pid != 0)
    {
        // wait for pid before trying to write out another core file
        waitpid(pid, nullptr, 0);
        return;
    }

    // signal the monitor process
    if (alwaysSignalMonitor && gProcessController->getMonitorPid() != 0)
    {
        kill(gProcessController->getMonitorPid(), SIGUSR1);
    }

    // child dies here. we are aborting to let the system write out the core file for us
    abort();
#endif
}

void CoreFileWriter::run()
{
    while (!mShutdown)
    {
        Job* job = mQueue.popOne(-1);
        if (!mShutdown && job != nullptr)
        {
            job->execute();
            delete job;
        }
    }
}
 
void CoreFileWriter::stop()
{
    // thread will terminate eventually - 
    //  if in the middle of a core file write, the write will conclude before exiting (the signal will be a no-op in that case.)
    if (!mShutdown)
    {
        mShutdown = true;
        mQueue.wakeAll();
    }
    // always block the caller until this thread exits
    waitForEnd();
}

} // namespace Blaze

