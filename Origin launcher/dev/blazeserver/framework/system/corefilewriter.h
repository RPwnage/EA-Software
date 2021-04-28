/*************************************************************************************************/
/*!
    \file corefilerwriter.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_COREFILEWRITER_H
#define BLAZE_COREFILEWRITER_H

/*** Include files *******************************************************************************/

#include "framework/system/jobqueue.h"
#include "framework/system/blazethread.h"

#include "EASTL/string.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

//  CoreFileWriter allows outputting a core file while server threads are running.
//      - There should only be one CoreFileWriter thread running per process.
//      - 
class CoreFileWriter : public BlazeThread
{
    
public:
    CoreFileWriter(const char *name, uint32_t coreBufferSize);
    ~CoreFileWriter() override;

    //  Interface
    BlazeRpcError coreDump(const char8_t* fileNamePrefix, const char8_t* fileNameDetail, uint64_t coreSizeLimit, void* context=nullptr, bool alwaysSignalMonitor=false);

    //  BlazeThread overrides
    void stop() override;   
    
private:
    JobQueue mQueue;
    volatile bool mShutdown;
   
    void dumpCoreFile(const char8_t* fileNamePrefix, const char8_t* fileNameDetail, const char8_t* instanceName, uint64_t coreSizeLimit, bool alwaysSignalMonitor);
    void dumpCoreFile(bool alwaysSignalMonitor);

    void outputLog(Logging::Level level,const StringBuilder& logMessage);

    void *mCoreBuffer;
    uint32_t mCoreBufferSize;
    void* mLastContext;

    // Used to capture the file-descriptor of the main server log to support writing log entries after forking
    int mMainLogFd;

    //  Blaze Thread override
    void run() override;

    EA::Thread::Semaphore mCoreWrittenSignal;
    int32_t mDumpNum;
};

extern EA_THREAD_LOCAL CoreFileWriter* gCoreFileWriter;

} // namespace Blaze

#endif // BLAZE_COREFILEWRITER_H

