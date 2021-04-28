///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/PatchDirectory.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/PatchDirectory.h>
#include <EAPatchClient/Debug.h>
#include <EAPatchClient/XML.h>
#include <EAPatchClient/URL.h>
#include <EAPatchClient/Telemetry.h>
#include <EAIO/EAStreamMemory.h>
#include <EAIO/EAStreamAdapter.h>
#include <coreallocator/icoreallocatormacros.h>
#include <eathread/eathread_sync.h>


namespace EA
{
namespace Patch
{


/////////////////////////////////////////////////////////////////////////////
// PatchDirectoryEventCallback
/////////////////////////////////////////////////////////////////////////////

void PatchDirectoryEventCallback::EAPatchStart              (PatchDirectoryRetriever*, intptr_t){}
void PatchDirectoryEventCallback::EAPatchProgress           (PatchDirectoryRetriever*, intptr_t, double){}
void PatchDirectoryEventCallback::EAPatchNetworkAvailability(PatchDirectoryRetriever*, intptr_t, bool){}
void PatchDirectoryEventCallback::EAPatchError              (PatchDirectoryRetriever*, intptr_t, Error&){}
void PatchDirectoryEventCallback::EAPatchNewState           (PatchDirectoryRetriever*, intptr_t, int){}
void PatchDirectoryEventCallback::EAPatchBeginFileDownload  (PatchDirectoryRetriever*, intptr_t, const char8_t*, const char8_t*){}
void PatchDirectoryEventCallback::EAPatchEndFileDownload    (PatchDirectoryRetriever*, intptr_t, const char8_t*, const char8_t*){}
void PatchDirectoryEventCallback::EAPatchStop               (PatchDirectoryRetriever*, intptr_t, AsyncStatus){}




/////////////////////////////////////////////////////////////////////////////
// PatchDirectory
/////////////////////////////////////////////////////////////////////////////

PatchDirectory::PatchDirectory()
  : mPatchInfoArray()
{
}

PatchDirectory::~PatchDirectory()
{
}


const PatchInfoArray& PatchDirectory::GetPatchInfoArray() const
{
    return mPatchInfoArray;
}


PatchInfoArray& PatchDirectory::GetPatchInfoArray()
{
    return mPatchInfoArray;
}


void PatchDirectory::Reset()
{
    mPatchInfoArray.set_capacity(0); // This causes the memory to be freed while also clearing the container.
}


bool PatchDirectory::AddPatchInfo(const PatchInfo& patchInfo)
{
    // To consider: Validate that patchInfo isn't already present.
    mPatchInfoArray.push_back(patchInfo);
    return true;
}


bool PatchDirectory::RemovePatchInfo(const char8_t* pPatchId)
{
    for(PatchInfoArray::iterator it = mPatchInfoArray.begin(); it != mPatchInfoArray.end(); ++it)
    {
        PatchInfo& patchInfo = *it;

        if(patchInfo.mPatchId == pPatchId)
        {
            mPatchInfoArray.erase(it);
            return true;
        }
    }

    return false;
}




/////////////////////////////////////////////////////////////////////////////
// PatchDirectoryRetriever
/////////////////////////////////////////////////////////////////////////////

PatchDirectoryRetriever::PatchDirectoryRetriever()
  : mpServer(NULL)
  , mServerURLBase()
  , mPatchDirectoryURL()
  , mPatchDirectory()
  , mState(kStateNone)
  , mAsyncStatus(kAsyncStatusNone)
  , mThread()
  , mFutex()
  , mpCurrentJob(NULL)
  , mHTTP()
  , mPatchDirDownloadVolume(0)
  , mPatchInfoDownloadVolume(0)
  , mpPatchDirectoryEventCallback(NULL)
  , mPatchDirectoryEventUserContext(0)
  , mpTelemetryEventCallback(NULL)
  , mTelemetryEventUserContext(0)
{
}


PatchDirectoryRetriever::~PatchDirectoryRetriever()
{
    // If the following assertion fails then the user is quitting the app or destroying the 
    // PatchDirectoryRetriever while it is busy doing something. The user should have 
    // cancelled the operation. 
    EAPATCH_ASSERT(!IsAsyncOperationInProgress(mAsyncStatus));

    CancelRetrieval(true);
}


Server* PatchDirectoryRetriever::GetServer()
{
    return mpServer;
}


void PatchDirectoryRetriever::SetServer(Server* pServer)
{
    mpServer = pServer;
}


void PatchDirectoryRetriever::Reset()
{
    // We clear all state that's associated with a directory retrieval.
    ErrorBase::Reset();

  //mpServer                            // Unchanged.
    mServerURLBase.set_capacity(0);     //             This causes the memory to be freed while also clearing the container.
    mPatchDirectory.Reset();
    mState = kStateNone;
    mAsyncStatus = kAsyncStatusNone;
  //mThread                             // Unchanged.
  //mFutex;                             // Unchanged.
    mpCurrentJob = NULL;
    mHTTP.Reset();
    mPatchDirDownloadVolume = 0;
    mPatchInfoDownloadVolume = 0;
  //mpPatchDirectoryEventCallback       // Unchanged.
  //mPatchDirectoryEventUserContext     // Unchanged.
  //mpTelemetryEventCallback;           // Unchanged.
  //mTelemetryEventUserContext;         // Unchanged.
}


const char8_t* PatchDirectoryRetriever::GetPatchDirectoryURL() const
{
    return mPatchDirectoryURL.c_str();
}


void PatchDirectoryRetriever::SetPatchDirectoryURL(const char8_t* pURL)
{
    if(pURL)
        mPatchDirectoryURL = pURL;
}


void PatchDirectoryRetriever::RegisterPatchDirectoryEventCallback(PatchDirectoryEventCallback* pCallback, intptr_t userContext)
{
    mpPatchDirectoryEventCallback   = pCallback;
    mPatchDirectoryEventUserContext = userContext;
}


void PatchDirectoryRetriever::RegisterTelemetryEventCallback(TelemetryEventCallback* pCallback, intptr_t userContext)
{
    mpTelemetryEventCallback   = pCallback;
    mTelemetryEventUserContext = userContext;
}


bool PatchDirectoryRetriever::ShouldContinue()
{
    return mbSuccess && ((mAsyncStatus <= kAsyncStatusStarted) || (mAsyncStatus == kAsyncStatusCompleted));
}


void PatchDirectoryRetriever::NotifyStart()
{
    if(mpPatchDirectoryEventCallback)  
        mpPatchDirectoryEventCallback->EAPatchStart(this, mPatchDirectoryEventUserContext);
}

void PatchDirectoryRetriever::NotifyProgress(double progress)
{
    if(mpPatchDirectoryEventCallback)  
        mpPatchDirectoryEventCallback->EAPatchProgress(this, mPatchDirectoryEventUserContext, progress);
}

void PatchDirectoryRetriever::NotifyNetworkAvailability(bool available)
{
    if(mpPatchDirectoryEventCallback)  
        mpPatchDirectoryEventCallback->EAPatchNetworkAvailability(this, mPatchDirectoryEventUserContext, available);
}

void PatchDirectoryRetriever::NotifyError(Error& error)
{
    if(mpPatchDirectoryEventCallback)  
        mpPatchDirectoryEventCallback->EAPatchError(this, mPatchDirectoryEventUserContext, error);
}

void PatchDirectoryRetriever::NotifyNewState(int newState)
{
    if(mpPatchDirectoryEventCallback)  
        mpPatchDirectoryEventCallback->EAPatchNewState(this, mPatchDirectoryEventUserContext, newState);
}

void PatchDirectoryRetriever::NotifyBeginFileDownload(const char8_t* pFilePath, const char8_t* pFileURL)
{
    if(mpPatchDirectoryEventCallback)  
        mpPatchDirectoryEventCallback->EAPatchBeginFileDownload(this, mPatchDirectoryEventUserContext, pFilePath, pFileURL);
}

void PatchDirectoryRetriever::NotifyEndFileDownload(const char8_t* pFilePath, const char8_t* pFileURL)
{
    if(mpPatchDirectoryEventCallback)  
        mpPatchDirectoryEventCallback->EAPatchEndFileDownload(this, mPatchDirectoryEventUserContext, pFilePath, pFileURL);
}

void PatchDirectoryRetriever::NotifyStop()
{
    if(mpPatchDirectoryEventCallback)  
        mpPatchDirectoryEventCallback->EAPatchStop(this, mPatchDirectoryEventUserContext, mAsyncStatus);
}

void PatchDirectoryRetriever::TelemetryNotifyDirectoryRetrievalBegin()
{
    if(mpTelemetryEventCallback)
    {
        TelemetryDirectoryRetrievalBegin tdrb(*this);

        tdrb.Init();
        tdrb.mPatchDirectoryURL = mPatchDirectoryURL;

        mpTelemetryEventCallback->TelemetryEvent(mTelemetryEventUserContext, tdrb);
    }
}


void PatchDirectoryRetriever::TelemetryNotifyDirectoryRetrievalEnd()
{
    if(mpTelemetryEventCallback)
    {
        TelemetryDirectoryRetrievalEnd tdre(*this);

        tdre.Init();
        tdre.mPatchDirectoryURL = mPatchDirectoryURL;
        tdre.mStatus = GetAsyncStatusString(mAsyncStatus);
        tdre.mDirDownloadVolume.sprintf("%I64u", mPatchDirDownloadVolume);
        tdre.mInfoDownloadVolume.sprintf("%I64u", mPatchInfoDownloadVolume);

        mpTelemetryEventCallback->TelemetryEvent(mTelemetryEventUserContext, tdre);
    }
}


bool PatchDirectoryRetriever::GetPatchDirectory(PatchDirectory& patchDirectory)
{
    // In order to help prevent user errors, we don't copy our PatchDirectory unless
    // the result was successful. Otherwise, if the user forgets to check the return
    // value of this function then this function might return valid values 90% of the 
    // time but occasionally return an incomplete PatchDirectory that the user doesn't
    // detect.

    if(mAsyncStatus == kAsyncStatusCompleted)
    {
        patchDirectory = mPatchDirectory;
        return true;
    }
    else
    {
        patchDirectory = PatchDirectory();
        return false;
    }
}


void PatchDirectoryRetriever::EnsureURLIsAbsolute(String& sURL)
{
    // We currently require that absolute URLs begin with http or https (case-sensitive).
    if(sURL.find("http") != 0) // If this appears to be a relative URL...
    {
        EAPATCH_ASSERT(!mServerURLBase.empty()); // It should have been set on init.
        sURL.insert(0, mServerURLBase); // Prepend the http server (e.g. http://www.some.server.com:80/) to the relative path.
    }
}


bool PatchDirectoryRetriever::RetrievePatchDirectoryBlocking()
{
    if(!mpServer)
        mpServer = EA::Patch::GetServer();
    EAPATCH_ASSERT(mpServer != NULL);

    Job job(this, false);

    return RetrievePatchDirectoryBlocking(&job);
}


// This is the actual patch retrieval function. All other functions in this class exist
// to manage the execution and outcome of this function, whether the download is 
// synchronous (blocking) or asynchronous.

// If the job is cancelled at any point during our execution, we don't set the 
// class Error state, even if we encountered an error. The reason is that the
// operation has been cancelled and thus we are being thrown away and ignored.
// If we encounter an error, don't set the Error state if we are cancelled.
//
// Patch directory retrieval doesn't write intermediate files to disk for the purpose
// of recovering from an exception or disruption of some sort. Either we download 
// and read the entire patch directory without interruption, or we just the process
// over again upon restarting the app. This is OK because patch directories are small
// and don't benefit much from such recovery. On the other hand, patch implementations
// need to have such recovery as they can be large downloads (up to gigabytes).
//
bool PatchDirectoryRetriever::RetrievePatchDirectoryBlocking(Job* pJob)
{
    NotifyStart();

    if(ShouldContinue())
        SetState(kStateBegin);

    ///////////////////////////////////////
    // Verify the Server.
    ///////////////////////////////////////

    if(ShouldContinue())
    {
        EAPATCH_ASSERT(mpServer);

        EAReadBarrier(); // Make sure any mbCancelJob assignment becomes visible from other threads.

        if(!mpServer && !pJob->mbCancelJob) // Don't set or report errors if we've been cancelled.
            HandleError(kECBlocking, kSystemErrorIdNone, kEAPatchErrorIdServerError, "null");
    }

    ///////////////////////////////////////
    // Get the PatchDirectory URL.
    ///////////////////////////////////////

    if(ShouldContinue())
    {
        if(mPatchDirectoryURL.empty()) // If the user didn't call SetPatchDirectoryURL...
        {
            // Fall back to calling  Server::GetPatchDirectoryURL. This use of Server is deprecated
            // and the user should always instead call this class' SetPatchDirectoryURL.
            mPatchDirectoryURL = mpServer->GetPatchDirectoryURL();
            EAPATCH_ASSERT(!mPatchDirectoryURL.empty()); // If this fails then you need to call PatchDirectory

            EAReadBarrier(); // Make sure any mbCancelJob assignment becomes visible from other threads.

            if(mPatchDirectoryURL.empty() && !pJob->mbCancelJob) // Don't set or report errors if we've been cancelled.
                HandleError(kECBlocking, kSystemErrorIdNone, kEAPatchErrorIdURLError, "null");
        }
    }

    ///////////////////////////////////////
    // Setup mServerURLBase
    ///////////////////////////////////////

    if(ShouldContinue())
    {
        // Setup mServerURLBase, which should be something like: https://patch.ea.com:443/ or http://patch.ea.com/
        // Question: Which should have precedence? mPatchDirectoryURL or mpServer->GetURLBase()?
        if(mPatchDirectoryURL.find("http") == 0) // This should normally be true.
        {
            mServerURLBase = mPatchDirectoryURL;

            if(mServerURLBase.find('/', 8) == String::npos) // If somehow the reference URL doesn't have / char after (e.g.) https://patch.ea.com:443 ...
                mServerURLBase.push_back('/');
            else
                mServerURLBase.resize(mServerURLBase.find('/', 8) + 1);
        }
        else if(mpServer) // This should normally be true.
        {
            mServerURLBase = mpServer->GetURLBase();
            EAPATCH_ASSERT(!mServerURLBase.empty()); // If the user set a Server, and we need to use that Server, then the user should have set a Server URL.

            if(!mServerURLBase.empty() && (mServerURLBase.find('/', 8) == String::npos)) // If somehow the reference URL doesn't have / char after (e.g.) https://patch.ea.com:443 ...
                mServerURLBase.push_back('/');
        }
        // Else mServerURLBase will be empty, so hopefully we won't need it during execution, which would 
        // occur if URLs are relative, as we use mServerURLBase to make relative URLs be full URLs.
    }

    TelemetryNotifyDirectoryRetrievalBegin();

    ///////////////////////////////////////
    // Download the .eaPatchDir file.
    ///////////////////////////////////////

    EA::IO::MemoryStream eaPatchDirStream;
    eaPatchDirStream.AddRef();
    eaPatchDirStream.SetAllocator(EA::Patch::GetAllocator());
    eaPatchDirStream.SetOption(EA::IO::MemoryStream::kOptionResizeEnabled, 1);

    if(ShouldContinue())
    {
        SetState(kStateDownloadDirectory);

        NotifyBeginFileDownload(mPatchDirectoryURL.c_str(), mPatchDirectoryURL.c_str());

        mHTTP.SetServer(mpServer);

        // We'll read the .eaPatchDir file (mPatchDirectoryURL) into a MemoryStream.
        if(mHTTP.GetFileBlocking(mPatchDirectoryURL.c_str(), &eaPatchDirStream, 0, 0))
        {
            if(mHTTP.GetAsyncStatus() == kAsyncStatusCompleted) // If the download proceeded complete to conclusion...
                NotifyEndFileDownload(mPatchDirectoryURL.c_str(), mPatchDirectoryURL.c_str());
            // Else the download was cancelled or deferred and we simply stop the processing.
        }
        else
        {
            EAReadBarrier(); // Make sure any mbCancelJob assignment becomes visible from other threads.

            if(!pJob->mbCancelJob)  // Don't set or report errors if we've been cancelled.
                TransferError(mHTTP); // This will set mbSuccess to false.
        }
    }

    ///////////////////////////////////////
    // Read the .eaPatchDir file entries (URLs).
    ///////////////////////////////////////

    PatchDirectory patchDirectory;

    EAReadBarrier(); // Make sure any mbCancelJob assignment becomes visible from other threads.

    if(ShouldContinue() && !pJob->mbCancelJob)
    {
        SetState(kStateProcessDirectory);

        PatchInfo     patchInfo;
        char8_t       buffer[1024]; // To do: Make this size at least configurable. EAIO really should have a templated string version of ReadLine (not to be confused with ReadString).
        IO::size_type size;

        mPatchDirDownloadVolume = eaPatchDirStream.GetSize();
        eaPatchDirStream.SetPosition(0);

        while((size = IO::ReadLine(&eaPatchDirStream, buffer, EAArrayCount(buffer))) < IO::kSizeTypeDone)
        {
            EA_UNUSED(size);
            EAPATCH_ASSERT(size < (EAArrayCount(buffer) - 1)); // Assert that we didn't get near the limit.

            patchInfo.mPatchInfoURL = buffer;
            patchInfo.mPatchInfoURL.trim();

            if(!patchInfo.mPatchInfoURL.empty() && 
               (patchInfo.mPatchInfoURL[0] != '#')) // To do: Find a good place to define this # char as a constant.
            {
                // URL will look like one of the following:
                //    http://127.0.0.1:24564/patches/patch1.eaPatchInfo
                //    patches/patch1.eaPatchInfo
                //    some patches/patch 1.eaPatchInfo
                EnsureURLIsAbsolute(patchInfo.mPatchInfoURL); // Convert patches/patch1.eaPatchInfo to http://127.0.0.1:24564/patches/patch1.eaPatchInfo if needed.

                URL::EncodeSpacesInURL(patchInfo.mPatchInfoURL.c_str(), patchInfo.mPatchInfoURL);

                patchDirectory.AddPatchInfo(patchInfo);

                if(patchDirectory.GetPatchInfoArray().back().HasError())
                {
                    mbSuccess = false;
                    TransferError(patchInfo);
                }
            }
            else
                patchInfo.mPatchInfoURL.clear();
        }

        // Explicitly free the stream memory.
        eaPatchDirStream.SetCapacity(0); 

        if(patchInfo.HasError())
        {
            mbSuccess = false;
            TransferError(patchInfo);
        }
    }

    ///////////////////////////////////////
    // Download each of the .eaPatchInfo files.
    ///////////////////////////////////////
    if(ShouldContinue() && !pJob->mbCancelJob)
    {
        SetState(kStateDownloadDirectoryEntries);

        EA::IO::MemoryStream eaPatchInfoStream;
        eaPatchInfoStream.AddRef();
        eaPatchInfoStream.SetAllocator(EA::Patch::GetAllocator());
        eaPatchInfoStream.SetOption(EA::IO::MemoryStream::kOptionResizeEnabled, 1);

        // To consider: Parallelize this by giving the HTTP job multiple downloads to execute
        // in parallel. These aren't big files and so it might not matter much, but downloading
        // multiple files simultaneously can possibly half the total download time.
        PatchInfoArray& patchInfoArray = patchDirectory.GetPatchInfoArray();

        for(eastl_size_t i = 0, iEnd = patchInfoArray.size(); (i != iEnd) && !pJob->mbCancelJob && ShouldContinue(); i++)
        {
            PatchInfo& patchInfo = patchInfoArray[i];

            NotifyBeginFileDownload(patchInfo.mPatchInfoURL.c_str(), patchInfo.mPatchInfoURL.c_str());

            // Read the .eaPatchInfo file into a MemoryStream.
            if(mHTTP.GetFileBlocking(patchInfo.mPatchInfoURL.c_str(), &eaPatchInfoStream, 0, 0))
            {
                if(mHTTP.GetAsyncStatus() == kAsyncStatusCompleted) // If the download proceeded complete to conclusion...
                {
                    mPatchInfoDownloadVolume += eaPatchInfoStream.GetSize();
                    eaPatchInfoStream.SetPosition(0);

                    PatchInfoSerializer patchInfoSerializer;

                    if(patchInfoSerializer.Deserialize(patchInfo, &eaPatchInfoStream, true))
                    {
                        // If the patchInfo.mPatchInfoURL field uses a relative directory, convert it 
                        // to an absolute one which uses the same server root as we retrieved the file from. 
                        EnsureURLIsAbsolute(patchInfo.mPatchInfoURL); // Convert patches/patch1.eaPatchInfo to http://127.0.0.1:24564/patches/patch1.eaPatchInfo if needed.

                        // Note that there is also a URL specified in the patchInfo.mPatchImplURL field.
                        // However, we leave this relative and let the PatchBuilder (which is what uses 
                        // these fields) interpret the relative path when it uses them. The reason we do 
                        // it then is that perhaps the URL will change before the PatchBuilder uses it, 
                        // whereas the PatchInfoURL above was already retrieved.

                        NotifyEndFileDownload(patchInfo.mPatchInfoURL.c_str(), patchInfo.mPatchInfoURL.c_str());
                    }
                    else
                    {
                        mbSuccess = false;
                        EAReadBarrier(); // Make sure any mbCancelJob assignment becomes visible from other threads.

                        if(pJob->mbCancelJob)
                            patchInfoSerializer.EnableErrorAssertions(false);
                        else
                            TransferError(patchInfoSerializer); // This will set mbSuccess to false.
                    }
                }
                // Else the download was cancelled or deferred and we simply stop the processing.
            }
            else
            {
                mbSuccess = false;
                EAReadBarrier(); // Make sure any mbCancelJob assignment becomes visible from other threads.

                if(!pJob->mbCancelJob)    // Don't set or report errors if we've been cancelled.
                    TransferError(mHTTP); // This will set mbSuccess to false.

                // Take a look at the actual error.
                ErrorClass     errorClass;
                SystemErrorId  sytemErrorId;
                EAPatchErrorId eapatchErrorId;

                mError.GetErrorId(errorClass, sytemErrorId, eapatchErrorId);

                if(mHTTP.ErrorIsNetworkAvailability(eapatchErrorId))
                    NotifyNetworkAvailability(false);
            }
            // We need to reset the stream or else we risk having xml from a new file running into
            // data left over from the last file, if the new file is smaller than the last file read
            EA::StdC::Memset8(eaPatchInfoStream.GetData(), 0, eaPatchInfoStream.GetSize());
            
            // Move back to the beginning of the stream so we can re-use it.
            eaPatchInfoStream.SetPosition(0);

            EAReadBarrier(); // Make sure any mbCancelJob assignment becomes visible from other threads.
        }
    }

    EAPATCH_ASSERT(!mbSuccess || !mHTTP.HasError()); // Verify that bSuccess is consistent with mHTTP::HasError.
    if(mHTTP.HasError())
    {
        mbSuccess = false;
        TransferError(mHTTP);
    }

    ///////////////////////////////////////
    // Finalize the operation
    ///////////////////////////////////////

    EAReadBarrier(); // Make sure any mbCancelJob assignment becomes visible from other threads.

    if(!pJob->mbCancelJob)
    {
        EA::Thread::AutoFutex autoFutex(mFutex);
 
        if(mbSuccess)
        {
            mAsyncStatus    = kAsyncStatusCompleted;
            mPatchDirectory = patchDirectory;

            SetState(kStateEnd);
        }
        else
        {
            mAsyncStatus    = kAsyncStatusAborted;
            mPatchDirectory = PatchDirectory();
        }
    }
    // Else do nothing; just throw away any results we have.

    if(mAsyncStatus == kAsyncStatusCancelling)
        mAsyncStatus = kAsyncStatusCancelled;

    if(!mbSuccess)
        NotifyError(mError);

    TelemetryNotifyDirectoryRetrievalEnd();

    NotifyStop();

    return mbSuccess;  // Note that even if mbSuccess is true, mAsyncStatus may be something other than kAsyncStatusCompleted (e.g. kAsyncStatusCancelled).
}


bool PatchDirectoryRetriever::RetrievePatchDirectoryAsync()
{
    if(!mpServer)
        mpServer = EA::Patch::GetServer();
    EAPATCH_ASSERT(mpServer != NULL);

    // End any existing job, which generally shouldn't be executing, but we do this just to make sure.
    EndJob();

    if(mbSuccess)
    {
        mAsyncStatus = kAsyncStatusStarted;

        if(!StartThreadJob())
        {
            mAsyncStatus = kAsyncStatusAborted;
            HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdThreadError, NULL);
        }
    }

    return mbSuccess;
}


AsyncStatus PatchDirectoryRetriever::GetAsyncStatus() const
{
    EA::Thread::AutoFutex autoFutex(mFutex);

    return mAsyncStatus;
}


void PatchDirectoryRetriever::CancelRetrieval(bool waitForCompletion)
{
    mFutex.Lock();

    if(IsAsyncOperationInProgress(mAsyncStatus))
    {
        if(mAsyncStatus != kAsyncStatusCancelled)   // If we didn't reached cancelled state already due to a previous call to this function...
            mAsyncStatus = kAsyncStatusCancelling;  // Will later be converted to kAsyncStatusCancelled. 
    }

    if(mpCurrentJob)
    {
        Job* pJob = mpCurrentJob; // Save this off, as it will be NULLed (but not freed) in EndJob.

        EndJob();

        if(pJob->mbThreaded && waitForCompletion)
        {
            mFutex.Unlock();
            mThread.WaitForEnd(EA::Thread::GetThreadTime() + EA::Thread::ThreadTime(1000));
            return; // Don't just fall through, as there's a mutex unlock below.
        }
    }

    mFutex.Unlock();
}


PatchDirectoryRetriever::State PatchDirectoryRetriever::GetState()
{
    return mState;
}


void PatchDirectoryRetriever::SetState(State state)
{
    mState = state;
    NotifyNewState(state);
}


const char8_t* kDirectoryRetrieverStateTable[] = 
{
    "None",
    "Begin",
    "Downloading directory",
    "Processing directory",
    "Downloading directory entries",
    "End"
};

const char8_t* PatchDirectoryRetriever::GetStateString(State state)
{
    EA_COMPILETIME_ASSERT(kStateNone == 0);

    if((state < 0) || (state >= (int)EAArrayCount(kDirectoryRetrieverStateTable)))
        state = kStateNone;

    return kDirectoryRetrieverStateTable[state];
}


intptr_t PatchDirectoryRetriever::ThreadFunctionStatic(void* pContext)
{
    PatchDirectoryRetriever* pThis = static_cast<PatchDirectoryRetriever*>(pContext);
    return pThis->ThreadFunction();
}


intptr_t PatchDirectoryRetriever::ThreadFunction()
{
    EAPATCH_ASSERT(mpCurrentJob != NULL);
    const bool bResult = RetrievePatchDirectoryBlocking(mpCurrentJob);

    mFutex.Lock();
    CORE_DELETE(EA::Patch::GetAllocator(), mpCurrentJob); // We assume the job was created for us by StartThreadJob and was allocated by CORE_NEW.
    mpCurrentJob = NULL;
    mFutex.Unlock();

    return bResult ? kThreadSuccess : kThreadError;
}


bool PatchDirectoryRetriever::StartThreadJob()
{
    EA::Thread::AutoFutex autoFutex(mFutex);

    // The Job we allocate here will be freed in the thread function before it exits.
    EAPATCH_ASSERT(mpCurrentJob == NULL);
    mpCurrentJob = CORE_NEW(EA::Patch::GetAllocator(), "PatchDirectoryRetriever/Job", EA::Allocator::MEM_TEMP) Job(this, true);

    EA::Thread::ThreadParameters params;
    #if defined(EAPATCH_DEBUG)
        params.mpName = "EAPatchDir";
    #endif
    EA::Thread::ThreadId threadId = mThread.Begin(ThreadFunctionStatic, static_cast<PatchDirectoryRetriever*>(this), NULL, NULL);

    return (threadId != EA::Thread::kThreadIdInvalid);
}


void PatchDirectoryRetriever::EndJob()
{
    mFutex.Lock();

    if(mpCurrentJob)
    {
        // We don't attempt to destroy the job here. We let the owner of it do so.
        // If the job is being done in a thread, we let the thread exit on its own.
        mpCurrentJob->mbCancelJob = true;
        mFutex.Unlock(); // Must unlock before calling external code, otherwise some deadlock could possibly occur.
      
        mHTTP.CancelRetrieval(true); // We call this after setting mbCancelJob = true, because otherwise the code might see the HTTP cancel but not realize it was due to this cancel here.
        return; // Don't just fall through, as there's a mutex unlock below.
    }

    mFutex.Unlock();
}


} // namespace Patch
} // namespace EA





 




