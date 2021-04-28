///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/HTTP.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/HTTP.h>
#include <EAPatchClient/File.h>
#include <EAPatchClient/Debug.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>

// DirtySDK
#include <dirtylib.h>
#include <netconn.h>
#include <protohttpmanager.h>
#include <protossl.h>
#include <protohttp.h>
#include <dirtyvers.h>

#ifndef MULTICHAR_CONST
    #define MULTICHAR_CONST(a,b,c,d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d)) 
#endif

#ifndef PROTOSSL_ERROR_CERT_REQUEST // Older versions of DirtySDK don't have this error.
    #define PROTOSSL_ERROR_CERT_REQUEST (-9)
#endif



///////////////////////////////////////////////////////////////////////////////
// DirtySDK requirements
///////////////////////////////////////////////////////////////////////////////

// Your app or DLL must provide the following functions:
// extern "C" void* DirtyMemAlloc(int32_t iSize, int32_t /*iMemModule*/, int32_t /*iMemGroup*/);
// extern "C" void  DirtyMemFree(void* p, int32_t /*iMemModule*/, int32_t /*iMemGroup*/);
//
// On startup your app must call NetConnStartup(const char* pParams) and on 
// app shutdown you must call NetConnShutdown(uint32_t uShutdownFlags). Possibly also NetConnConnect().


namespace EA
{
namespace Patch
{


HTTP::HTTP()
  : mFutex()
  , mpServer(NULL)
  , mpHttpManager(NULL)
  , mClientLimits(EA::Patch::GetDefaultClientLimits())
  , mHandleEventFunction()
  , mHandleEventContext(0)
  , mAsyncStatus(kAsyncStatusNone)
  , mPaused(false)
  , mTimeOfNextTrySeconds(0)            // Some time in the past
  , mTimeNetworkUnavailable(UINT64_MAX) // Some time in the future
  //mBuffer()
  , mNetworkReadDataRateMeasure()
  , mThread()
  , mpAsyncDownloadJobPtrArray(NULL)
  , mAsyncDownloadJobArrayCount(0)
  , mDJIWaitingList()
  , mDJICompletedList()
  , mDJIActiveList()
  , mDJIFailureList()
  , mDownloadStats()
{
    #if (EAPATCH_DEBUG >= 2)
        memset(mBuffer, 0, sizeof(mBuffer));
    #endif
}

HTTP::~HTTP()
{
    // If the following assertion fails then the user is quitting the app or destroying the 
    // PatchDirectoryRetriever while it is busy doing something. The user should have 
    // cancelled the operation. 
    EAPATCH_ASSERT(!IsAsyncOperationInProgress(mAsyncStatus));
    CancelRetrieval(true);
}


void HTTP::Reset()
{
    CancelRetrieval(true);

    ErrorBase::Reset();

  //mFutex                                  // Unchanged.
  //mpServer                                // Unchanged.
  //mpHttpManager                           // Unchanged.
  //mClientLimits                           // Unchanged.
  //mHandleEventFunction = NULL;
  //mHandleEventContext = NULL;
    mAsyncStatus = kAsyncStatusNone;
    mPaused = false;
    mTimeOfNextTrySeconds = 0;
    mTimeNetworkUnavailable = UINT64_MAX;
    #if (EAPATCH_DEBUG >= 2)
        memset(mBuffer, 0, sizeof(mBuffer));
    #endif // Else unchanged.
    mNetworkReadDataRateMeasure.Reset();
  //mThread                                 // Nothing to do.
    mpAsyncDownloadJobPtrArray  = NULL;
    mAsyncDownloadJobArrayCount = 0;
    mDJIWaitingList.clear();
    mDJICompletedList.clear();
    mDJIActiveList.clear();
    mDJIFailureList.clear();
    mDownloadStats = DownloadStats(); // Clear by assigning a newly constructed instance.
}


void HTTP::GetJobDebugDescription(DownloadJobInternal& dji, HttpManagerRefT* pHttpManager, int32_t httpManagerHandle, const char8_t* pStringPrefix, const char8_t* pURL, String& sDescription)
{
    // URL
    String sURL = pURL;

    // IP Address
    String   sAddrDottedDecimal;
    uint32_t ipAddress = (uint32_t)HttpManagerStatus(pHttpManager, httpManagerHandle, MULTICHAR_CONST('a','d','d','r'), NULL, 0); 

    if(ipAddress != 0xffffffff)
        sAddrDottedDecimal.sprintf("%I32u.%I32u.%I32u.%I32u", (ipAddress >> 24) & 0xff, (ipAddress >> 16) & 0xff, (ipAddress >> 8) & 0xff, ipAddress & 0xff);
    else
        sAddrDottedDecimal = "(NA)";

    // Port
    String  sPort;
    int32_t ipPort = HttpManagerStatus(pHttpManager, httpManagerHandle, MULTICHAR_CONST('p','o','r','t'), NULL, 0); 

    if(ipPort > 0)
        sPort.sprintf("%I32d", ipPort);
    else
        sPort = "(NA)";

    // Host
    String  sHost;
    char8_t hostBuffer[256];
    int32_t hostResult = HttpManagerStatus(pHttpManager, httpManagerHandle, MULTICHAR_CONST('h','o','s','t'), hostBuffer, EAArrayCount(hostBuffer)); 

    if(hostResult > 0)
        sHost = hostBuffer;
    else
        sHost = "(NA)";

    // HTTP result
    String  sHTTPResult;
    int32_t httpCode = HttpManagerStatus(pHttpManager, httpManagerHandle, MULTICHAR_CONST('c','o','d','e'), NULL, 0); 

    sHTTPResult.sprintf("%I32d", httpCode);

    // To consider: Print some additional information.
    // 'rtxt' most recent http request header text copied to output buffer
    // 'htxt' current received http header text copied to output buffer

    sDescription.sprintf("%sURL: %s\nIP Address: %s\nPort: %s\nHost: %s\nTransferred size: %I64u, Expected size: %I64u, HTTP result: %s\n", 
                          pStringPrefix ? pStringPrefix : "", sURL.c_str(), sAddrDottedDecimal.c_str(), sPort.c_str(), sHost.c_str(), dji.mDownloadedSize, dji.mExpectedSize, sHTTPResult.c_str());
}


bool HTTP::ErrorIsNetworkAvailability(EAPatchErrorId errorId)
{
    return (errorId == kEAPatchErrorIdHttpConnectFailure) || 
           (errorId == kEAPatchErrorIdHttpSSLUnknown)     || 
           (errorId == kEAPatchErrorIdHttpDisconnect)     || 
           (errorId == kEAPatchErrorIdHttpUnknown);

}


void HTTP::GetHTTPFailureInfo(DownloadJobInternal& dji, HttpManagerRefT* pHttpManager, int32_t httpManagerHandle, const char8_t* pURL, int32_t dirtySDKResult, 
                                ErrorClass& errorClass, SystemErrorId& systemErrorId, EAPatchErrorId& eaPatchErrorId, String& sErrorDescription)
{
    errorClass     = kECNone;
    systemErrorId  = kSystemErrorIdNone;
    eaPatchErrorId = kEAPatchErrorIdNone;
    sErrorDescription.clear();

    GetJobDebugDescription(dji, pHttpManager, httpManagerHandle, "Error job description: ", pURL, sErrorDescription);

    // To consider: see if it's possible to get a system error ID, as some errors may in fact be derived from system errors.

    if(eaPatchErrorId == kEAPatchErrorIdNone)
    {
        // 'essl' returns protossl error state. 
        // However, it also refers to regular non-SSL state.
        int32_t sslError = HttpManagerStatus(pHttpManager, httpManagerHandle, MULTICHAR_CONST('e','s','s','l'), NULL, 0);

        switch(sslError)
        {
            case PROTOSSL_ERROR_NONE:
                // If this happens then the error is probably an HTTP error or network disconnect.
                break;

            case PROTOSSL_ERROR_DNS:
                eaPatchErrorId = kEAPatchErrorIdHttpDNSFailure;
                errorClass = kECBlocking;
                break;

            case PROTOSSL_ERROR_CONN:
                eaPatchErrorId = kEAPatchErrorIdHttpConnectFailure;
                errorClass = kECBlocking;
                break;

            case PROTOSSL_ERROR_CERT_INVALID:
                eaPatchErrorId = kEAPatchErrorIdHttpSSLCertInvalid;
                errorClass = kECBlocking;
                break;

            case PROTOSSL_ERROR_CERT_HOST:
                eaPatchErrorId = kEAPatchErrorIdHttpSSLCertHostMismatch;
                errorClass = kECBlocking;
                break;

            case PROTOSSL_ERROR_CERT_NOTRUST:
                eaPatchErrorId = kEAPatchErrorIdHttpSSLCertUntrusted;
                errorClass = kECBlocking;
                break;

            case PROTOSSL_ERROR_SETUP:
                eaPatchErrorId = kEAPatchErrorIdHttpSSLSetupFailure;
                errorClass = kECBlocking;
                break;

            case PROTOSSL_ERROR_SECURE:
                eaPatchErrorId = kEAPatchErrorIdHttpSSLConnectionFailure;
                errorClass = kECTemporary;
                break;

            case PROTOSSL_ERROR_CERT_REQUEST:
                eaPatchErrorId = kEAPatchErrorIdHttpSSLCertRequestFailure;
                errorClass = kECBlocking;
                break;

            case PROTOSSL_ERROR_UNKNOWN:
            default:
                eaPatchErrorId = kEAPatchErrorIdHttpSSLUnknown;
                errorClass = kECBlocking;
                break;
        }

        if((sslError == PROTOSSL_ERROR_CERT_INVALID) || 
           (sslError == PROTOSSL_ERROR_CERT_HOST)    || 
           (sslError == PROTOSSL_ERROR_CERT_NOTRUST))
        {
            ProtoSSLCertInfoT certInfo;

            if(HttpManagerStatus(pHttpManager, httpManagerHandle, MULTICHAR_CONST('c','e','r','t'), &certInfo, sizeof(certInfo)) == 0)
            {
                // To consider: We may need to support localization for these strings.
                sErrorDescription.append_sprintf("TLS/SSL certificate failure (%d) for %s: (country=%s, state=%s, city=%s, organization=%s, unit=%s, common=%s)\n", 
                                            sslError, pURL, certInfo.Ident.strCountry, certInfo.Ident.strState, certInfo.Ident.strCity, 
                                            certInfo.Ident.strOrg, certInfo.Ident.strUnit, certInfo.Ident.strCommon);
            }
            else
                sErrorDescription.append_sprintf("Could not get TLS/SSL certificate info for %s\n", pURL);
        }
    }

    if(eaPatchErrorId == kEAPatchErrorIdNone)
    {
        // 'code' negative=no response, else server response code (ProtoHttpResponseE)
        // If httpCode < 0 then that means we never got to the point of receiving an 
        // HTTP status code from the server. If that's the case then we couldn't get 
        // the server's DNS address, couldn't connect to the address, or could connect
        // to it but couldn't get far enough during protocol communication to get a 
        // code from the server. 
        int32_t httpCode      = HttpManagerStatus(pHttpManager, httpManagerHandle, MULTICHAR_CONST('c','o','d','e'), NULL, 0);
        int32_t httpCodeClass = PROTOHTTP_GetResponseClass(httpCode); EA_UNUSED(httpCodeClass);

        if(dirtySDKResult != PROTOHTTP_RECVFAIL) // It turns out we can't make the assert below if dirtySDKResult == PROTOHTTP_RECVFAIL, since DirtySDK can set httpCodeClass to 200 even if there's a failure.
        {
            EAPATCH_ASSERT((httpCodeClass != 100) && // httpCode should be -1, 4xx, or 5xx.
                           (httpCodeClass != 200) && // 1xx, 2xx, and 3xx should be impossible unless
                           (httpCodeClass != 300));  // DirtySDK is broken.
        }

        if(httpCode >= 400) // If it's an error class...
        {
            eaPatchErrorId = (kEAPatchErrorIdHttpCodeBegin + httpCode);
            errorClass = kECBlocking;
        }
    }

    if(eaPatchErrorId == kEAPatchErrorIdNone)
    {
        // 'plst' return whether any piped requests were lost due to a premature close.
        // Note by Paul Pedriana: I don't understand what is meant by "piped requests were lost." Is this error simply saying the connection was closed?
        int32_t pipedLost = HttpManagerStatus(pHttpManager, httpManagerHandle, MULTICHAR_CONST('p','l','s','t'), NULL, 0); 

        // 'time' TRUE if the client timed out the connection, else FALSE.
        int32_t timeoutOccurred = HttpManagerStatus(pHttpManager, httpManagerHandle, MULTICHAR_CONST('t','i','m','e'), NULL, 0); 

        if((pipedLost != 0) || (timeoutOccurred != 0))
        {
            eaPatchErrorId = kEAPatchErrorIdHttpDisconnect;
            errorClass = kECTemporary;
        }
    }

    if(eaPatchErrorId == kEAPatchErrorIdNone)
    {
        eaPatchErrorId = kEAPatchErrorIdHttpUnknown;
        errorClass = kECBlocking;
    }
}


bool HTTP::ShouldContinue()
{
    return mbSuccess && ((mAsyncStatus <= kAsyncStatusStarted) || (mAsyncStatus == kAsyncStatusCompleted));
}


bool HTTP::GetFilesAsync(DownloadJob* pDownloadJobPtrArray[], size_t downloadJobArrayCount)
{
    EAPATCH_ASSERT(!IsAsyncOperationInProgress(mAsyncStatus)); // If this fails then you forgot to stop and Reset this class before beginning a new download.

    if(!IsAsyncOperationInProgress(mAsyncStatus))
    {
        Reset();

        EA::Thread::ThreadParameters params;
        #if defined(EAPATCH_DEBUG)
            params.mpName = "EAPatchHTTP";
        #endif

        mpAsyncDownloadJobPtrArray  = pDownloadJobPtrArray;
        mAsyncDownloadJobArrayCount = downloadJobArrayCount;

        EA::Thread::ThreadId threadId = mThread.Begin(ThreadFunctionStatic, static_cast<HTTP*>(this), &params, NULL);

        return (threadId != EA::Thread::kThreadIdInvalid);
    }

    return false;
}


intptr_t HTTP::ThreadFunctionStatic(void* pContext)
{
    HTTP* pHTTP = static_cast<HTTP*>(pContext);
    return pHTTP->ThreadFunction();
}


intptr_t HTTP::ThreadFunction()
{
    const bool bResult = GetFilesBlocking(mpAsyncDownloadJobPtrArray, mAsyncDownloadJobArrayCount);

    return bResult ? kThreadSuccess : kThreadError;
}


bool HTTP::GetFileBlocking(const char8_t* pURL, const char8_t* pDestFilePath, uint64_t rangeBegin, uint64_t rangeCount)
{
    ClearError();

    File file;

    if(!file.Open(pDestFilePath, EA::IO::kAccessFlagReadWrite, EA::IO::kCDCreateAlways, 
                    EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential))
    {
        TransferError(file); // Will set mbSuccess to false.
        return false;
    }

    GetFileBlocking(pURL, file.GetStream(), rangeBegin, rangeCount); // Any error will have been set appropriately by GetFileBlocking.

    file.Close();

    if(file.HasError()) // If the Close failed or it was somehow else in an error state from before...
    {
        if(mbSuccess)   // If the Get operation was otherwise successful... TransferError from the file. Otherwise let's use the Get operation error take precedence.
            TransferError(file); // Will set mbSuccess to false.
    }

    return mbSuccess;  // Note that even if mbSuccess is true, mAsyncStatus may be something other than kAsyncStatusCompleted (e.g. kAsyncStatusCancelled).
}


bool HTTP::GetFileBlocking(const char8_t* pURL, EA::IO::IStream* pStream, uint64_t rangeBegin, uint64_t rangeCount)
{
    DownloadJob downloadJob;
    downloadJob.mSourceURL  = pURL;
    downloadJob.mpStream    = pStream;
    downloadJob.mRangeBegin = rangeBegin;
    downloadJob.mRangeCount = rangeCount;

    DownloadJob* pDownloadJobPtrArray[1] = { &downloadJob };

    return GetFilesBlocking(pDownloadJobPtrArray, 1); // This will set mbSuccess upon return.
}


// We are expected to possibly rewrite pHeader, though we can't use more than uHeaderCapacity space.
// pHeader is 0-terminated the raw header text received by ProtoHttp.
// uHeaderCapacity is the capacity of the buffer pointed to by pHeader, which will usually be > than the strlen of pHeader.
// pData is the immutable body data that will be sent. It is data and so not necessarily 0-terminated.
// uDataLen is the length of the body data.
// pDownloadJobInternalVoid is set by a call to HttpManagerControl(pHttpManager, dji.mHttpMangerHandle, MULTICHAR_CONST('c','b','u','p'), 0, 0, (void*)&dji) further below.
int32_t ProtoHttpCustomHeaderCbT(ProtoHttpRefT* /*pHttpManager*/, char* pHeader, uint32_t uHeaderSize, 
                                  const char* /*pData*/, DirtySDKDataSizeType /*uDataLen*/, void* pDownloadJobInternalVoid)
{
    using namespace EA::Patch;

    HTTP::DownloadJobInternal& dji = *static_cast<HTTP::DownloadJobInternal*>(pDownloadJobInternalVoid);

    if(dji.mpDownloadJob->mRangeCount != 0) // If the request is for a range of bytes and not the entire file...
    {
        char8_t  buffer[80];
        uint64_t rangeBegin = dji.mpDownloadJob->mRangeBegin;
        uint64_t rangeCount = dji.mpDownloadJob->mRangeCount;

        if(rangeCount == UINT64_MAX) // If the user want to read from rangeBegin to the end of the file...
            EA::StdC::Sprintf(buffer, "Range: bytes=%I64u-\r\n", rangeBegin);
        else
            EA::StdC::Sprintf(buffer, "Range: bytes=%I64u-%I64u\r\n", rangeBegin, rangeBegin + rangeCount - 1);

        // We will have a deadly problem if there isn't enough space. However, we can guarantee that 
        // there won't be a problem by making uHeaderSize be large enough. Also our headers are rather
        // small in in practice could never run out of uHeaderSize space, even with a default uHeaderSize.
        size_t requiredStrlen = EA::StdC::Strlcat(pHeader, buffer, (size_t)uHeaderSize);
        EAPATCH_ASSERT(requiredStrlen < uHeaderSize); EA_UNUSED(requiredStrlen);

        return (int32_t)EA::StdC::Strlen(pHeader);
    }

    return 0; // Unchanged
}

// pDownloadJobInternalVoid is set by a call to HttpManagerControl(pHttpManager, dji.mHttpMangerHandle, MULTICHAR_CONST('c','b','u','p'), 0, 0, (void*)&dji) further below.
int32_t ProtoHttpReceiveHeaderCbT(ProtoHttpRefT*, const char* pHeader, uint32_t /*uHeaderSize*/, void* pDownloadJobInternalVoid)
{
    using namespace EA::Patch;

    // Retrieve the file size, so our metrics can be updated.
    HTTP::DownloadJobInternal& dji = *static_cast<HTTP::DownloadJobInternal*>(pDownloadJobInternalVoid);

    if(dji.mExpectedSize == UINT64_MAX)
    {
        const char8_t* kHeader = "Content-Length:";
        const char8_t* pContentLengthHeader = EA::StdC::Stristr(pHeader, kHeader);

        if(pContentLengthHeader)
        {
            // pValue might begin with whitespace, but AtoU64 skips leading whitespace.
            const char8_t* pValue = pContentLengthHeader + EA::StdC::Strlen(kHeader);
            dji.mExpectedSize = EA::StdC::AtoU64(pValue);
            dji.mpHTTP->mDownloadStats.mBytesExpected += dji.mExpectedSize;
        }
    }

    return 0;
}


bool HTTP::GetFilesBlocking(DownloadJob* pDownloadJobPtrArray[], size_t downloadJobArrayCount)
{
    // To do: Implememt mFutex lock appropriately. Must not remain locked during the blocking download.
    // To consider: Have a callback that this function repeatedly calls to see if we should cancel this blocking operation.
    bool bUnusedArgument = false;

    ClearError();

    // There must be no active jobs in order for this to start.
    EAPATCH_ASSERT(mAsyncStatus != kAsyncStatusStarted);

    if(mAsyncStatus == kAsyncStatusStarted)
    {
        // This error is a bug in the EAPatchClient code, as it's trying to use this class for two simultaneous downloads.
        HandleError(kECBlocking, kSystemErrorIdNone, kEAPatchErrorIdHttpBusyFailure, NULL); // This will set mbSuccess to false.
        return false;
    }

    Reset();
    mAsyncStatus = kAsyncStatusStarted;

    // We have downloadJobArrayCount jobs to process. But we can process only mHTTPConcurrentLimit
    // of them at a time. Below we have a loop which starts jobs and when jobs complete it adds 
    // additional jobs from the array. 
    const size_t            maxConcurrentDownloadCount = mClientLimits.mHTTPConcurrentLimit;   // This is a number like 6 or 8.
    const size_t            maxHTTPRetryCount = mClientLimits.mHTTPRetryCount;

    // We start by adding all the jobs to mDJIWaitingList.
    for(size_t i = 0; i < downloadJobArrayCount; i++)
    {
        DownloadJobInternal& dji = mDJIWaitingList.push_back();
        dji.mpHTTP        = this;
        dji.mpDownloadJob = pDownloadJobPtrArray[i];
        mDownloadStats.mTotalFileCount++;

        // To do: Support DownloadJob::mDestPath, which isn't supported as of this writing. If we add code to support it, beware that the user of this class may be handling file opens themselves in kEventPreDownload. So we don't want to open the file path if the user already is doing so.
        // We don't yet support mDestPath, and so require mpStream to be non-NULL. 
        EAPATCH_ASSERT(dji.mpDownloadJob->mpStream);
    }

    // On startup your app must call NetConnStartup(const char* pParams) and on 
    // app shutdown you must call NetConnShutdown(uint32_t uShutdownFlags). 
    EAPATCH_ASSERT_MESSAGE(NetConnStatus(MULTICHAR_CONST('o','p','e','n'), 0, NULL, 0) != 0, "EA::Patch::HTTP: DirtySDK is not initialized. Need to call DirtySDK NetConnStartup(const char* pParams) before using DirtySDK.");

    int32_t          dirtySDKResult = 0;
    HttpManagerRefT* pHttpManager;
    bool             bDestroyHttpManager;

    if(mpHttpManager)
    {
        pHttpManager = mpHttpManager;
        bDestroyHttpManager = false;
    }
    else
    {
        pHttpManager = HttpManagerCreate((int32_t)mClientLimits.mHTTPMemorySize, static_cast<int32_t>(maxConcurrentDownloadCount));
        bDestroyHttpManager = true;
    }

    EAPATCH_ASSERT(pHttpManager);
    if(!pHttpManager)
        HandleError(kECBlocking, kSystemErrorIdNone, kEAPatchErrorIdHttpInitFailure, NULL);

    if(mbSuccess)
    {
        HttpManagerControl(pHttpManager, -1, MULTICHAR_CONST('t','i','m','e'), (int32_t)mClientLimits.mHTTPTimeoutSeconds * 1000, 0, NULL);
	    HttpManagerControl(pHttpManager, -1, MULTICHAR_CONST('p','i','p','e'), mClientLimits.mHTTPPipeliningEnabled ? 1 : 0, 0, NULL); 
        HttpManagerControl(pHttpManager, -1, MULTICHAR_CONST('r','m','a','x'), 10, 0, NULL);  // We set this only for sanity reasons.

        #if (DIRTYVERS > 0x08090000) // Enable all the cipher suites including the ones added recently
		    HttpManagerControl(pHttpManager, -1, MULTICHAR_CONST('c','i','p','h'), PROTOSSL_CIPHER_ALL, 0, NULL);
        #endif

        #if (DIRTYVERS > 0x08120000) // Don't tick http from NetConnIdle. We do it ourselves.
		    HttpManagerControl(pHttpManager, -1, MULTICHAR_CONST('a','u','t','o'), 0, 0, NULL); 
        #endif

        #if defined(EA_DEBUG) || defined(_DEBUG)
		    HttpManagerControl(pHttpManager, -1, MULTICHAR_CONST('s','p','a','m'), 1, 0, NULL);    // Enable DirtySDK logging.
		    HttpManagerControl(pHttpManager, -1, MULTICHAR_CONST('n','c','r','t'), 1, 0, NULL);    // Disable SSL certificate validation.
        #endif

        // DirtySDK has a bug which currently requires the user to call this, even if it does nothing.
        HttpManagerCallback(pHttpManager, ProtoHttpCustomHeaderCbT, ProtoHttpReceiveHeaderCbT);
    }

    #if (EAPATCH_DEBUG >= 2)
        memset(mBuffer, 0, sizeof(mBuffer));
    #endif

    EAPATCH_ASSERT(!mNetworkReadDataRateMeasure.IsStarted());
    mNetworkReadDataRateMeasure.Start();

    uint32_t retryCount = 0;
    
    while(ShouldContinue() &&                                 // While we haven't had an error or were cancelled,
          (mDJICompletedList.size() < downloadJobArrayCount)) // and while there are uncompleted jobs to do...
    {
        if(mPaused)
        {
            if(retryCount >= mClientLimits.mHTTPRetryCount)
            {
                HandleError(kECBlocking, kSystemErrorIdNone, kEAPatchErrorIdHttpConnectFailure, NULL);
            }

            retryCount++;

            if(GetTimeSeconds() >= mTimeOfNextTrySeconds) // If it's time to un-pause...
            {
                // To consider: Instead of unilaterally un-pausing, perhaps we should do a ping to tell
                // to the HTTP server to see if we are on the Internet. Otherwise, every N seconds we 
                // will trigger a bunch of downloads which will fail every time until the user's Internet
                // connection comes back alive.
                mPaused = false;
            }
        }

        // Don't queue additional downloads while we are paused.
        if(!mPaused)
        {
            // Queue any additional downloads if there are more slots.
            while((mDJIActiveList.size() < maxConcurrentDownloadCount) &&    // While there are more active slots to fill,
                  !mDJIWaitingList.empty() &&                                // and while we have more jobs to start,
                  ShouldContinue())                                          // and while we weren't explicitly cancelled...
            {
                // Set up this job which was just added to the acive list.
                DownloadJobInternal& dji = mDJIWaitingList.front();

                if(mHandleEventFunction && !mHandleEventFunction(kEventPreDownload, this, dji.mpDownloadJob, mHandleEventContext, NULL, 0, bUnusedArgument))
                {
                    dji.mpDownloadJob->mStatus = kAsyncStatusAborted;
                    HandleError(dji.mpDownloadJob->mError);
                    mDJIFailureList.splice(mDJIFailureList.end(), mDJIWaitingList, mDJIWaitingList.begin());
                    mDownloadStats.mFailureFileCount++; // Don't set it to mDJIFailureList.size() because list::size is O(n).
                    // To do: What more to do here? Probably the callback attempted to open a file but failed.
                }
                else
                {
                    dji.mHttpMangerHandle = HttpManagerAlloc(pHttpManager);

                    EAPATCH_ASSERT(dji.mHttpMangerHandle >= 0);
                    if(dji.mHttpMangerHandle >= 0)
                    {
                        // Move the first item in the waiting list to the end of the active list.
                        mDJIActiveList.splice(mDJIActiveList.end(), mDJIWaitingList, mDJIWaitingList.begin());
                        mDownloadStats.mActiveFileCount++;

                        // Start this job
                        // The following doesn't work as expected with DirtySDK. You need to use the HttpManagerCallback 
                        // functionality to append to the headers. However, it is expected to be supported in a future DirtySDK version.
                        //if(dji.mpDownloadJob->mRangeCount) // If the request is for a range of bytes and not the entire file...
                        //{
                        //    uint64_t rangeBegin = dji.mpDownloadJob->mRangeBegin;
                        //    uint64_t rangeCount = dji.mpDownloadJob->mRangeCount;
                        //    String   sRangeHeader;
                        //    if(rangeCount == UINT64_MAX) // If the user want to read from rangeBegin to the end of the file...
                        //        sRangeHeader.sprintf("Range: bytes=%I64u-\r\n", rangeBegin);
                        //    else
                        //        sRangeHeader.sprintf("Range: bytes=%I64u-%I64u\r\n", rangeBegin, rangeBegin + rangeCount - 1);
                        //    HttpManagerControl(pHttpManager, dji.mHttpMangerHandle, MULTICHAR_CONST('a','p','n','d'), 0, 0, const_cast<char8_t*>(sRangeHeader.c_str()));
                        //}

                        // Set the 'callback user pointer' so that the header callback has dji, and we can use it to write the range info.
                        HttpManagerControl(pHttpManager, dji.mHttpMangerHandle, MULTICHAR_CONST('c','b','u','p'), 0, 0, (void*)&dji);

                        // Initiate an HTTP GET. The actual reading of bytes from HttpManager will happen below.
                        dirtySDKResult = HttpManagerGet(pHttpManager, dji.mHttpMangerHandle, dji.mpDownloadJob->mSourceURL.c_str(), 0);
                        EAPATCH_ASSERT(dirtySDKResult >= 0);
                        if(dirtySDKResult < 0)  // To do: We need to be able to tell if this error is due to lack of connectivity (kECTemporary) or a serious problem (kECBlocking).
                        {
                            dji.mpDownloadJob->mStatus = kAsyncStatusAborted;
                            HandleError(kECBlocking, kSystemErrorIdNone, kEAPatchErrorIdHttpConnectFailure, NULL);
                            // To do: What to do here?
                        }
                        else
                            dji.mpDownloadJob->mStatus = kAsyncStatusStarted;
                    }
                    else
                    {
                        dji.mpDownloadJob->mStatus = kAsyncStatusAborted;
                        HandleError(kECBlocking, kSystemErrorIdNone, kEAPatchErrorIdHttpInitFailure, NULL);
                        mDJIFailureList.splice(mDJIFailureList.end(), mDJIWaitingList, mDJIWaitingList.begin());
                        mDownloadStats.mFailureFileCount++;
                        // To do: What to do here?
                    }
                }
            } // while
        }

        EAPATCH_ASSERT(mDJIActiveList.size() <= maxConcurrentDownloadCount);

        HttpManagerUpdate(pHttpManager);

        // Process active jobs.
        for(DownloadJobInternalList::iterator it = mDJIActiveList.begin(); 
            (it != mDJIActiveList.end()) && ShouldContinue(); /*update 'it' below*/)
        {
            DownloadJobInternal& dji = *it;

            bool bEvidenceOfNetworkAvailable = false; // Identifies if our processing has shown that the network is available.

            dirtySDKResult = HttpManagerRecv(pHttpManager, dji.mHttpMangerHandle, (char*)mBuffer, 1, sizeof(mBuffer));

            int32_t httpCodeFamily = (HttpManagerStatus(pHttpManager, dji.mHttpMangerHandle, MULTICHAR_CONST('c','o','d','e'), NULL, 0) / 200 * 200); 

            if(dirtySDKResult >= 0) // If we have a successful read of bytes from the HTTP server...
            {
                bEvidenceOfNetworkAvailable = true;

                if(mNetworkReadDataRateMeasure.IsStarted()) // It's possible that we stopped measurements due to encountering an error.
                    mNetworkReadDataRateMeasure.ProcessData((uint64_t)dirtySDKResult);

                // Write to the stream.
                bool bResult       = true;
                bool bWriteHandled = false; // Indicates if the user callback handled the write instead of us.

                if(mHandleEventFunction) // If the user has provided a hook for us to call...
                {
                    bResult = mHandleEventFunction(kEventWrite, this, dji.mpDownloadJob, mHandleEventContext, mBuffer, (EA::IO::size_type)dirtySDKResult, bWriteHandled);
                    EAPATCH_ASSERT_FORMATTED(bResult, ("HTTP user-provided write of %d bytes failed. %s", (int)dirtySDKResult, dji.mpDownloadJob->mSourceURL.c_str()));
                }

                if(bResult && !bWriteHandled) // If there wasn't any error above, and the user didn't already handle the write in their hook...
                {
                    bResult = dji.mpDownloadJob->mpStream->Write(mBuffer, (EA::IO::size_type)dirtySDKResult);

                    #if EAPATCH_ASSERT_ENABLED // Temporary code while we are trying to debug an auto-build failure.
                        if(!bResult)
                        {
                            String   sName;
                            uint32_t systemErrorId;
                            EA::Patch::GetStreamInfo(dji.mpDownloadJob->mpStream, systemErrorId, sName);
                            EAPATCH_FAIL_FORMATTED(("HTTP received data write of %I32d bytes failed with system error %I32u. URL: %s. Stream name: %s.", dirtySDKResult, systemErrorId, dji.mpDownloadJob->mSourceURL.c_str(), sName.c_str()));
                        }
                    #endif
                }

                if(bResult) // If the write succeeded...
                {
                    if(dji.mExpectedSize == UINT64_MAX) // If somehow we missed setting mBytesExpected in our ProtoHttpReceiveHeaderCbT callback...
                        dji.mExpectedSize = 0;

                    dji.mDownloadedSize             += (uint32_t)dirtySDKResult;
                    mDownloadStats.mBytesDownloaded += (uint32_t)dirtySDKResult;

                    ++it;
                }
                else
                {
                    uint32_t systemErrorId;
                    String   sName;

                    EA::Patch::GetStreamInfo(dji.mpDownloadJob->mpStream, systemErrorId, sName);
                    HandleError(kECBlocking, systemErrorId, kEAPatchErrorIdFileWriteFailure, sName.c_str());

                    dji.mpDownloadJob->mStatus = kAsyncStatusAborted;
                    dji.mpDownloadJob->mError  = mError;

                    HttpManagerFree(pHttpManager, dji.mHttpMangerHandle);

                    DownloadJobInternalList::iterator itToMove = it++;
                    mDJIFailureList.splice(mDJIFailureList.end(), mDJIActiveList, itToMove);
                    mDownloadStats.mActiveFileCount--;
                    mDownloadStats.mFailureFileCount++;

                    if(mHandleEventFunction && !mHandleEventFunction(kEventPostDownload, this, dji.mpDownloadJob, mHandleEventContext, NULL, 0, bUnusedArgument))
                    {
                        // To consider: Anything to do here? We've already failed as it is.
                    }
                }
            }
            else if((dirtySDKResult == PROTOHTTP_RECVDONE) && (httpCodeFamily == 200))
            {
                HttpManagerFree(pHttpManager, dji.mHttpMangerHandle);

                DownloadJobInternalList::iterator itToMove = it++;
                dji.mpDownloadJob->mStatus = kAsyncStatusCompleted;

                if(dji.mpDownloadJob->mpStream) // Rewind the stream to the beginning. To consider: on start save the initial position (which might be non-zero) and rewind to there.
                    dji.mpDownloadJob->mpStream->SetPosition(0);

                if(mHandleEventFunction && !mHandleEventFunction(kEventPostDownload, this, dji.mpDownloadJob, mHandleEventContext, NULL, 0, bUnusedArgument))
                {
                    dji.mpDownloadJob->mStatus = kAsyncStatusAborted;
                    HandleError(dji.mpDownloadJob->mError);
                }

                if(dji.mpDownloadJob->mStatus == kAsyncStatusCompleted)
                {
                    mDJICompletedList.splice(mDJICompletedList.end(), mDJIActiveList, itToMove);
                    mDownloadStats.mActiveFileCount--;
                    mDownloadStats.mCompletedFileCount++;
                }
                else
                {
                    mDJIFailureList.splice(mDJIFailureList.end(), mDJIActiveList, itToMove);
                    mDownloadStats.mActiveFileCount--;
                    mDownloadStats.mFailureFileCount++;
                }
            }
            else if((dirtySDKResult == PROTOHTTP_RECVFAIL) ||  // Question: Does HttpManager automatically implement redirections for you? We hope that it does.
                    ((dirtySDKResult == PROTOHTTP_RECVDONE) && (httpCodeFamily != 200)))
            {
                // There are three basic classes of errors we can encounter:
                //    - network connectivity errors, which cause our app to pause and retry later.
                //    - TLS/SSL or authorization-related connection errors, which may be fatal but we should probably treat them as something that will later recover once we fix something on the server.
                //    - missing resource errors (e.g. 404), which are generally fatal, because it means the patch is malformed. The user should be given the chance to later retry or to cancel the patch.
                //
                // You probably have to understand a bit about how HTTP works in order to understand how the    
                // error analysis below is working. We check a number of error types below and one of them 
                // should be the one that caused our problem.

                ErrorClass     errorClass;
                SystemErrorId  systemErrorId;
                EAPatchErrorId eaPatchErrorId;
                String         sErrorDescription;

                GetHTTPFailureInfo(dji, pHttpManager, dji.mHttpMangerHandle, dji.mpDownloadJob->mSourceURL.c_str(), 
                                    dirtySDKResult, errorClass, systemErrorId, eaPatchErrorId, sErrorDescription);

                mNetworkReadDataRateMeasure.Stop(); // It will be restarted later if we resume downloads.

                // If we get a 404 error, do we stop or do we re-try? We can be pretty sure 
                // that a re-try of a 404 error will result in another 404 error.
                const bool bHTTPResourceError = ((eaPatchErrorId >= kEAPatchErrorIdHttpCode400) && (eaPatchErrorId <( kEAPatchErrorIdHttpCode400 + 100))) ||
                                                ((eaPatchErrorId >= kEAPatchErrorIdHttpCode500) && (eaPatchErrorId <( kEAPatchErrorIdHttpCode500 + 100)));

                if(!bHTTPResourceError && (dji.mTryCount++ < maxHTTPRetryCount)) // If we have more retries to do...
                {
                    // Queue the job to retry later by adding it back to the waiting list.
                    HttpManagerFree(pHttpManager, dji.mHttpMangerHandle);

                    DownloadJobInternalList::iterator itToMove = it++;
                    mDJIWaitingList.splice(mDJIWaitingList.end(), mDJIActiveList, itToMove); // Move from the active list back to the waiting list.
                    mDownloadStats.mActiveFileCount--;

                    // We expect that if one file fails then others will too. So the code here will execute 
                    // multiple times in succession, re-setting mPaused and mTimeOfNextTrySeconds. That's OK.
                    const uint64_t timeNow = GetTimeSeconds();

                    mPaused = true;
                    mTimeOfNextTrySeconds = timeNow + 30; // To consider: Have this re-try time implement 'back-off' instead of unilaterally waiting N seconds every time.

                    if(mHandleEventFunction)
                        mHandleEventFunction(kEventRetryableDownloadError, this, dji.mpDownloadJob, mHandleEventContext, NULL, 0, bUnusedArgument);

                    if(ErrorIsNetworkAvailability(eaPatchErrorId))
                    {
                        // In this case we don't consider the error to be fatal, but rather we notify the app of the disconnect and keep retrying.
                        if(mTimeNetworkUnavailable != UINT64_MAX) // If this is the first time we are seeing this since it was last available...
                        {
                            mTimeNetworkUnavailable = timeNow;

                            if(mHandleEventFunction)
                                mHandleEventFunction(kEventNetworkUnavailable, this, dji.mpDownloadJob, mHandleEventContext, NULL, 0, bUnusedArgument);
                        }
                    }
                }
                else
                {
                    HandleError(errorClass, kSystemErrorIdNone, eaPatchErrorId, sErrorDescription.c_str());

                    HttpManagerFree(pHttpManager, dji.mHttpMangerHandle);

                    DownloadJobInternalList::iterator itToMove = it++;
                    dji.mpDownloadJob->mStatus = kAsyncStatusAborted;
                    mDJIFailureList.splice(mDJIFailureList.end(), mDJIActiveList, itToMove);
                    mDownloadStats.mActiveFileCount--;
                    mDownloadStats.mFailureFileCount++;

                    if(mHandleEventFunction)
                    {
                        if(!mHandleEventFunction(kEventNonretryableDownloadError, this, dji.mpDownloadJob, mHandleEventContext, NULL, 0, bUnusedArgument))
                        {
                            // To consider: Anything to do here? We've already failed as it is.
                        }

                        if(!mHandleEventFunction(kEventPostDownload, this, dji.mpDownloadJob, mHandleEventContext, NULL, 0, bUnusedArgument))
                        {
                            // To consider: Anything to do here? We've already failed as it is.
                        }
                    }
                }
            }
            else
            {
                // Assert that the only other possible return value is PROTOHTTP_RECVWAIT. 
                // PROTOHTTP_RECVHEAD and PROTOHTTP_RECVBUFF should never occur for the download
                // we are doing, nor should there be any other value we aren't aware of.
                EAPATCH_ASSERT(dirtySDKResult == PROTOHTTP_RECVWAIT);
                ++it;

                if(mHandleEventFunction && !mHandleEventFunction(kEventIdle, this, dji.mpDownloadJob, mHandleEventContext, NULL, 0, bUnusedArgument))
                {
                    // To consider: Anything to do here? We've already failed as it is.
                }
            }

            // We test mPaused and mTimeNetworkUnavailable below because there's a tricky situation we need to be wary of.
            // This loop is processing multiple connections in parallel. It's possible that one of them could fail due to 
            // a network disconnect, but another connection tested after it in this loop could succeed, making us think that
            // we very briefly lost and then regained the network connection. But that would usually be wrong, and instead 
            // what happened was there was a socket race condition which caused the second one to test as connected after
            // the first one was found disconnected. The second one will usually become disconnected itself on the next time
            // through this loop. So we need to guard against mistakenly thinking we re-gained the connection. The way we 
            // do that is that after the first disconnect, we ignore any evidence of reconnect until after our (e.g. 30 second)
            // waiting period. mPaused re-becomes true only after that period, so we test it below.

            if(bEvidenceOfNetworkAvailable &&           // If it appears we are currently connected to the network during this loop,
               !mPaused &&                              // and we aren't paused because of another connection having just failed,
               (mTimeNetworkUnavailable != UINT64_MAX)) // and we were previously disconnected from the network...
            {
                mTimeNetworkUnavailable = UINT64_MAX;   // Mark it as being available.

                if(mHandleEventFunction)
                    mHandleEventFunction(kEventNetworkAvailable, this, dji.mpDownloadJob, mHandleEventContext, NULL, 0, bUnusedArgument);

                mNetworkReadDataRateMeasure.Start(); // This was previously Stopped during error handling above.
            }

        } // for ...

    } // while ...

    mNetworkReadDataRateMeasure.Stop();

    if(mAsyncStatus == kAsyncStatusCancelling)
        mAsyncStatus = kAsyncStatusCancelled;

    if((mAsyncStatus == kAsyncStatusCancelled) || 
       (mAsyncStatus == kAsyncStatusDeferred))    // If we stopped because we were cancelled or deferred...
    {
        // To consider: Do we report a final progress state?
        // Currently we don't consider cancellation to be an error, and so we don't call HandleError, EAPATCH_ASSERT, etc.
    }
    else
    {
        // Assert that we either failed or we downloaded everything.
        if(mbSuccess)
        {
            mAsyncStatus = kAsyncStatusCompleted;
            EAPATCH_ASSERT(mDJIWaitingList.empty() && (mDJICompletedList.size() == downloadJobArrayCount) && mDJIActiveList.empty() && mDJIFailureList.empty());
        }
        else
        {
            mAsyncStatus = kAsyncStatusAborted;
            // EAPATCH_ASSERT(!mDJIFailureList.empty()); // We can't assert this because we may have not gotten that far above.
        }
    }

    if(pHttpManager && bDestroyHttpManager)
        HttpManagerDestroy(pHttpManager);

    return mbSuccess; // Note that even if mbSuccess is true, mAsyncStatus may be something other than kAsyncStatusCompleted (e.g. kAsyncStatusCancelled).
}


void HTTP::CancelRetrieval(bool waitForCompletion)
{
    // To do: Use mFutex appropriately.
    
    if(IsAsyncOperationInProgress(mAsyncStatus))
    {
        // To do: Allow for the user to specify this as kAsyncStatusDeferred instead of kAsyncStatusCancelled. 
        //        This would require adding a state called kAsyncStatusDeferring.

        if(mAsyncStatus != kAsyncStatusCancelled)   // If we didn't reached cancelled state already due to a previous call to this function...
            mAsyncStatus = kAsyncStatusCancelling;  // Will later be converted to kAsyncStatusCancelled. 
        EAWriteBarrier(); // Until we get mFutex involved, at least implement a write barrier.

        if(waitForCompletion)
        {
            // This loop won't exit until the operation is finished. Is there a pathological 
            // condition in which the operation won't finish?
            while(!IsAsyncOperationInProgress(mAsyncStatus))
            {
                EA::Thread::ThreadSleep(100);
                EAReadBarrier();
            }
        }
    }
}


void HTTP::GetDownloadStats(DownloadStats& stats)
{
    using namespace EA::Thread;

    // We have a small problem: The download code above isn't currently using mFutex, at least not in a way
    // that would allow us to simply copy mDownloadStats to stats. To do: Implement the mFutex usage as-needed.
    EA::Thread::AutoFutex autoFutex(mFutex);

    stats = mDownloadStats;

    if(stats.mBytesDownloaded > stats.mBytesExpected) // Sanity check due to (probably never occurring) race.
        stats.mBytesExpected = stats.mBytesDownloaded;
}


} // namespace Patch
} // namespace EA

