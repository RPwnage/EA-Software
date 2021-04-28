///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Error.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Error.h>
#include <EAPatchClient/Locale.h>
#include <EAPatchClient/Debug.h>

#if defined(EA_PLATFORM_XENON)
    #pragma warning(push, 0)
    #include <XTL.h>
    #pragma warning(pop)
#elif defined(EA_PLATFORM_MICROSOFT)
    #pragma warning(push, 0)
    #include <Windows.h>
    #pragma warning(pop)
#elif defined(EA_PLATFORM_UNIX)
    #include <errno.h>
#endif



namespace EA
{
namespace Patch
{


EAPATCHCLIENT_API SystemErrorId GetLastSystemErrorId()
{
    #if defined(EA_PLATFORM_MICROSOFT)
        return (SystemErrorId)GetLastError();
    #elif defined(EA_PLATFORM_UNIX)
        return (SystemErrorId)errno;
    #else
        // To do: Add other platforms which have the concept of errno or equivalent.
        // Some platforms (e.g. Nintendo and Sony platforms) return error codes directly
        // from API functions and there isn't a way to get those values except by reading
        // those return values.
        return kSystemErrorIdNone;
    #endif
}



EAPATCHCLIENT_API bool SystemErrorIdToString(SystemErrorId id, String& sResult, bool prefixId)
{
    bool errorRecognized = false;

    #if defined(EA_PLATFORM_WINDOWS)
        // It's important to use the Windows wchar API, as otherwise the  
        // output won't be localized, at least not in a useful way.
        eastl::fixed_string<wchar_t, 256, true> sError;
        WCHAR* pError   = NULL;
        DWORD  dwResult = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, id, 
                                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (WCHAR*)&pError, 0, NULL);
        if(dwResult)
        {
            errorRecognized = true;
            sError = pError;
            LocalFree(pError);
        }
        else
            sError = L"unknown error";

        if(prefixId)
            sResult.sprintf("0x%I64x: %ls", (uint64_t)id, sError.c_str());
        else
            sResult.sprintf("%ls", sError.c_str());
    #else
        // To do: Consult the current locale 
        // To do: Complete this per platform.
        if(prefixId)
            sResult.sprintf("0x%I64x: unknown error", (uint64_t)id);
        else
            sResult.sprintf("unknown error");
    #endif

    return errorRecognized;
}


// These are English strings and are not translated. If you want to display these errors to 
// end users then you'll/we'll need to implement a localized version of this function, though 
// it doesn't necessarily need to be part of this library.
//
EAPATCHCLIENT_API bool EAPatchErrorIdToString(EAPatchErrorId id, String& sResult, bool /*prefixId*/)
{
    bool           errorRecognized = true;
    const char8_t* pError = NULL;

    sResult.clear();

    switch (id)
    {
        case kEAPatchErrorIdNone:
            pError = "None"; break;
        case kEAPatchErrorIdUnknown:
            pError = "Unknown"; break;

        // EAPatch-level errors
        case kEAPatchErrorIdAlreadyActive:
            pError = "Patch already active"; break;
        case kEAPatchErrorIdTooManyFiles:
            pError = "Too many files"; break;
        case kEAPatchErrorIdInvalidSettings:
            pError = "Invalid settings"; break;
        case kEAPatchErrorIdExpectedFileMissing:
            pError = "Expected file missing"; break;
        case kEAPatchErrorIdUnexpectedFilePresent:
            pError = "Unexpected file present"; break;
        case kEAPatchErrorIdExpectedDirMissing:
            pError = "Expected directory missing"; break;
        case kEAPatchErrorIdUnexpectedDirPresent:
            pError = "Unexpected directory present"; break;

        // OS-level file system errors
        case kEAPatchErrorIdFileOpenFailure:
            pError = "File open failure"; break;
        case kEAPatchErrorIdFileReadFailure:
            pError = "File read failure"; break;
        case kEAPatchErrorIdFileReadTruncate:
            pError = "File read truncated"; break;
        case kEAPatchErrorIdFileWriteFailure:
            pError = "File write failure"; break;
        case kEAPatchErrorIdFilePosFailure:
            pError = "File position read failure"; break;
        case kEAPatchErrorIdFileCloseFailure:
            pError = "File close failure"; break;
        case kEAPatchErrorIdFileCorrupt:
            pError = "File corrupt"; break;
        case kEAPatchErrorIdFileRemoveError:
            pError = "File remove error"; break;
        case kEAPatchErrorIdFileNameLength:
            pError = "File name length invalid"; break;
        case kEAPatchErrorIdFilePathLength:
            pError = "File path length invalid"; break;
        case kEAPatchErrorIdFileNotFound:
            pError = "File not found"; break;
        case kEAPatchErrorIdFileNotSpecified:
            pError = "File not specified"; break;
        case kEAPatchErrorIdFileRenameError:
            pError = "File rename error"; break;
        case kEAPatchErrorIdFileCreateError:
            pError = "File create error"; break;
        case kEAPatchErrorIdDirCreateError:
            pError = "Directory create error"; break;
        case kEAPatchErrorIdDirReadError:
            pError = "Directory read error"; break;
        case kEAPatchErrorIdDirRenameError:
            pError = "Directory rename error"; break;
        case kEAPatchErrorIdDirRemoveError:
            pError = "Directory remove error"; break;

        // XML errors
        case kEAPatchErrorIdXMLError:
            pError = "XML content error"; break;

        // Memory errors
        case kEAPatchErrorIdMemoryFailure:
            pError = "Memory failure"; break;

        // URL errors
        case kEAPatchErrorIdURLError:
            pError = "URL error"; break;

        // Threading errors
        case kEAPatchErrorIdThreadError:
            pError = "Thread error"; break;

        // File server errors
        case kEAPatchErrorIdServerError:
            pError = "Server error"; break;

        // HTTP protocol errors
        case kEAPatchErrorIdHttpInitFailure:
            pError = "HTTP init failure"; break;
        case kEAPatchErrorIdHttpDNSFailure:
            pError = "HTTP name lookup failure"; break;
        case kEAPatchErrorIdHttpConnectFailure:
            pError = "HTTP connect failure"; break;
        case kEAPatchErrorIdHttpSSLSetupFailure:
            pError = "HTTP SSL init failure"; break;
        case kEAPatchErrorIdHttpSSLCertInvalid:
            pError = "HTTP SSL certificate invalid"; break;
        case kEAPatchErrorIdHttpSSLCertHostMismatch:
            pError = "HTTP SSL certificate host mismatch"; break;
        case kEAPatchErrorIdHttpSSLCertUntrusted:
            pError = "HTTP SSL certificate untrusted"; break;
        case kEAPatchErrorIdHttpSSLCertRequestFailure:
            pError = "HTTP SSL certificate request failure"; break;
        case kEAPatchErrorIdHttpSSLConnectionFailure:
            pError = "HTTP connection failure"; break; // May or may not actually be SSL-based.
        case kEAPatchErrorIdHttpSSLUnknown:
            pError = "HTTP unknown SSL failure"; break;
        case kEAPatchErrorIdHttpDisconnect:
            pError = "HTTP disconnect error"; break;
        case kEAPatchErrorIdHttpUnknown:
            pError = "HTTP unknown failure"; break;
        case kEAPatchErrorIdHttpBusyFailure:
            pError = "HTTP busy failure"; break;

        case kEAPatchErrorIdHttpCodeBegin:
        case kEAPatchErrorIdHttpCode100:
        case kEAPatchErrorIdHttpCode200:
        case kEAPatchErrorIdHttpCode300:
        case kEAPatchErrorIdHttpCode400:
        case kEAPatchErrorIdHttpCode500:
        default:
        {
            if((id >= kEAPatchErrorIdHttpCodeBegin) && (id < kEAPatchErrorIdHttpCodeBegin + 600))
                sResult.sprintf("0x%08x: HTTP code %u", (unsigned)((unsigned)id - kEAPatchErrorIdHttpCodeBegin));
            else
            {
                EAPATCH_FAIL_MESSAGE("Unknown EAPatchErrorId. Need to update EAPatchErrorIdToString.");
                pError = "Unknown"; break;
            }
            break;
        }
    }

    if(sResult.empty()) // If it wasn't already written above...
        sResult.sprintf("0x%08x: %s", (uint32_t)id, pError);

    return errorRecognized;
}



EAPATCHCLIENT_API bool ErrorSourceToString(uint32_t errorSource, String& sResult)
{
    sResult.clear();

    switch (errorSource)
    {
        case kESNone:
            sResult = "None";
            break;

        case kESPatchDirectoryRetriever:
            sResult = "PatchDirectoryRetriever";
            break;

        case kESPatchInfoRetriever:
            sResult = "PatchInfoRetriever";
            break;
    }

    return !sResult.empty();
}




///////////////////////////////////////////////////////////////////////////////
// Error
///////////////////////////////////////////////////////////////////////////////

Error::Error()
  : mErrorClass(kECNone),
    mErrorSource(kESNone),
    mSystemErrorId(kSystemErrorIdNone),
    mEAPatchErrorId(kEAPatchErrorIdNone),
    mErrorContext(),
    mSourceFilePath(),
    mSourceFileLine(),
    mErrorString(),
    mbErrorReported(false)
{
}


Error::Error(const Error& error)
  : mErrorClass(error.mErrorClass),
    mErrorSource(error.mErrorSource),
    mSystemErrorId(error.mSystemErrorId),
    mEAPatchErrorId(error.mEAPatchErrorId),
    mErrorContext(error.mErrorContext),
    mSourceFilePath(error.mSourceFilePath),
    mSourceFileLine(error.mSourceFileLine),
    mErrorString(error.mErrorString),
    mbErrorReported(error.mbErrorReported)
{
}


Error::Error(ErrorClass errorClass, SystemErrorId systemErrorId, EAPatchErrorId eapatchErrorId)
  : mErrorClass(errorClass),
    mErrorSource(),
    mSystemErrorId(systemErrorId),
    mEAPatchErrorId(eapatchErrorId),
    mErrorContext(),
    mSourceFilePath(),
    mSourceFileLine(),
    mErrorString(),
    mbErrorReported(false)
{
    //SetSource(const char8_t* pSourceFilePath, int sourceFileLine);
}


Error::Error(ErrorClass errorClass, SystemErrorId systemErrorId, EAPatchErrorId eapatchErrorId, const char8_t* pSourceFilePath, int sourceFileLine)
  : mErrorClass(errorClass),
    mErrorSource(),
    mSystemErrorId(systemErrorId),
    mEAPatchErrorId(eapatchErrorId),
    mErrorContext(),
    mSourceFilePath(pSourceFilePath ? pSourceFilePath : ""),
    mSourceFileLine(sourceFileLine),
    mErrorString(),
    mbErrorReported(false)
{
}


Error::~Error()
{
}


const Error& Error::operator=(const Error& error)
{
    if(this != &error)
    {
        mErrorClass     = error.mErrorClass;
        mErrorSource    = error.mErrorSource;
        mSystemErrorId  = error.mSystemErrorId;
        mEAPatchErrorId = error.mEAPatchErrorId;
        mErrorContext   = error.mErrorContext;
        mSourceFilePath = error.mSourceFilePath;
        mSourceFileLine = error.mSourceFileLine;
        mErrorString    = error.mErrorString;
        mbErrorReported = error.mbErrorReported;
    }

    return *this;
}


void Error::Clear()
{
    mErrorClass     = kECNone;
    mErrorSource    = kESNone;
    mSystemErrorId  = kSystemErrorIdNone;
    mEAPatchErrorId = kEAPatchErrorIdNone;
    mErrorContext.clear();
    mSourceFilePath.clear();
    mSourceFileLine = 0;
    mErrorString.clear();
    mbErrorReported  = false;
}


void Error::Reset()
{
    Clear();

    mErrorContext.set_capacity(0); // This causes the memory to be freed while also clearing the container.
    mErrorString.set_capacity(0);
}


void Error::SetErrorId(ErrorClass errorClass, SystemErrorId systemErrorId, EAPatchErrorId eapatchErrorId, const char8_t* pErrorContext)
{
    mErrorClass     = errorClass;
  //mErrorSource    = errorSource;      // Doesn't currently exist as an argument.
    mSystemErrorId  = systemErrorId;
    mEAPatchErrorId = eapatchErrorId;
    if(pErrorContext)
        mErrorContext = pErrorContext;
    mErrorString.clear();               // Clearing this triggers it to be re-evaluated later if it's requested.
}


void Error::GetErrorId(ErrorClass& errorClass, SystemErrorId& systemErrorId, EAPatchErrorId& eapatchErrorId) const
{
    errorClass     = mErrorClass;
  //errorSource    = mErrorSource;
    systemErrorId  = mSystemErrorId;
    eapatchErrorId = mEAPatchErrorId;
  //errorContext   = mErrorContext; // Doesn't currently exist as an argument.
  //errorString    = mErrorString;  // "
}


void Error::SetErrorContext(const char8_t* pErrorContext)
{
    EAPATCH_ASSERT(pErrorContext != NULL);
    mErrorContext = pErrorContext;
}


void Error::GetErrorContext(String& sContext) const
{
    sContext = mErrorContext;
}


void Error::SetErrorSource(const char8_t* pSourceFilePath, int sourceFileLine)
{
    mSourceFilePath = pSourceFilePath ? pSourceFilePath : "";
    mSourceFileLine = sourceFileLine;

    mErrorString.clear();               // Clearing this triggers it to be re-evaluated later if it's requested.
}


void Error::GetErrorSource(String& sourceFilePath, int& sourceFileLine) const
{
    sourceFilePath = mSourceFilePath;
    sourceFileLine = mSourceFileLine;
}


void Error::GetErrorString(String& sError) const
{
    if(mErrorString.empty()) // If the error string needs to be updated...
    {
        // To consider: We may need to handle the case of kSystemErrorIdNone, as we don't want to 
        // do that dumb thing that applications do when then print "Error happend: no error".
        // Maybe that's really a problem for higher level code, as it's a mistake to be
        // calling this function if there's no real error.
        EAPATCH_ASSERT_MESSAGE((mSystemErrorId  != kSystemErrorIdNone) || 
                               (mEAPatchErrorId != kEAPatchErrorIdNone), "Error::GetErrorString: No error.");

        // To do: Make the output more formalized in terms of its string formatting and organization.
        //        Currently it just writes a few lines without being obvious to the reader what they are.
        // To do: Handle localization if necessary here.

        // Add mErrorClass info
        //String sErrorClass;
        //if(mErrorClass != kECNone)
        //    sErrorClass;
        //if(!mErrorString.empty() && !sEAPatchError.empty())
        //    mErrorString += "\n";
        //mErrorString += sSystemError;

        // Add mErrorSource info
        String sErrorSource;
        if(mErrorSource != kESNone)
            ErrorSourceToString(mErrorSource, sErrorSource);
        if(!mErrorString.empty() && !sErrorSource.empty())
            mErrorString += "\n";
        mErrorString += sErrorSource;

        // Add mSystemErrorId info
        String sSystemError;
        if(mSystemErrorId != kSystemErrorIdNone)
            SystemErrorIdToString(mSystemErrorId, sSystemError, false);
        if(!mErrorString.empty() && !sSystemError.empty())
            mErrorString += "\n";
        mErrorString += sSystemError;

        // Add EAPatchErrorId info
        String sEAPatchError;
        if(mEAPatchErrorId != kEAPatchErrorIdNone)
            EAPatchErrorIdToString(mEAPatchErrorId, sEAPatchError, false);
        if(!mErrorString.empty() && !sEAPatchError.empty())
            mErrorString += "\n";
        mErrorString += sEAPatchError;

        // Add mErrorContext info
        if(!mErrorString.empty() && !mErrorContext.empty())
            mErrorString += "\n";
        mErrorString += mErrorContext;

        // Add __FILE__/__LINE__ info
        String sFileLine;
        if(!mSourceFilePath.empty())
            sFileLine.sprintf("At %s (%d)", mSourceFilePath.c_str(), mSourceFileLine);
        if(!mErrorString.empty() && !sFileLine.empty())
            mErrorString += "\n";
        mErrorString += sFileLine;

        // Something should have been written.
        EAPATCH_ASSERT_MESSAGE(!mErrorString.empty(), "Error::GetErrorString: Empty error string result.");
    }

    sError = mErrorString;
}


const char* Error::GetErrorString() const
{
    // We do the following to trigger the writing of the string if it hasn't been written already.
    if(mErrorString.empty())
        GetErrorString(mErrorString);

    return mErrorString.c_str();
}



///////////////////////////////////////////////////////////////////////////////
// ErrorBase
///////////////////////////////////////////////////////////////////////////////

ErrorBase::ErrorBase()
 : mbSuccess(true)
 , mbHasErrorUsed(false)
 , mbGetErrorUsed(false)
 , mbEnableErrorAssertions(true)
 , mError()
{
}


ErrorBase::ErrorBase(const ErrorBase& errorBase)
 : mbSuccess(errorBase.mbSuccess)
 , mbHasErrorUsed(errorBase.mbHasErrorUsed)
 , mbGetErrorUsed(errorBase.mbGetErrorUsed)
 , mbEnableErrorAssertions(errorBase.mbEnableErrorAssertions)
 , mError(errorBase.mError)
{   
}


ErrorBase::~ErrorBase()
{
    // Assert that the user of this class (or really, a subclass of this class) has called HasLastError. 
    // And assert that if the class has been used and there was an error, that GetError was called and 
    // the error was reported to the application.
    #if EAPATCH_ENABLE_ERROR_ASSERTIONS
        if(mbEnableErrorAssertions)
            EAPATCH_ASSERT(mbHasErrorUsed && (mbSuccess || (mbGetErrorUsed && mError.GetErrorReported())));
    #endif
}


ErrorBase& ErrorBase::operator=(const ErrorBase& errorBase)
{
    mbSuccess      = errorBase.mbSuccess;
    mbHasErrorUsed = errorBase.mbHasErrorUsed;
    mbGetErrorUsed = errorBase.mbGetErrorUsed;
    mError         = errorBase.mError;

    return *this;
}


bool ErrorBase::HasError() const
{
    mbHasErrorUsed = true;
    return !mbSuccess;
}


Error ErrorBase::GetError() const
{
    mbGetErrorUsed = true;
    return mError;
}


void ErrorBase::ClearError()
{
    mbSuccess      = true;
    mbHasErrorUsed = false;
    mbGetErrorUsed = false;
    mError.Clear();
}


void ErrorBase::Reset()
{
    ClearError();
    mError.Reset();
}


void ErrorBase::EnableErrorAssertions(bool enable)
{
    mbEnableErrorAssertions = enable;
}


void ErrorBase::HandleError(ErrorClass errorClass, SystemErrorId systemErrorId, EAPatchErrorId eaPatchErrorId, const char8_t* pErrorContext, bool reportError)
{
    mError.SetErrorId(errorClass, systemErrorId, eaPatchErrorId);

    if(pErrorContext)
        mError.SetErrorContext(pErrorContext);

    HandleError(mError, reportError);
}


void ErrorBase::HandleError(const Error& error, bool reportError)
{
    mbSuccess = false;
    mError    = error;

    if(reportError)
    {
        #if defined(EAPATCH_TRACE_ENABLED) && EAPATCH_TRACE_ENABLED
            String sErrorText;
            mError.GetErrorString(sErrorText);
            EAPATCH_TRACE_MESSAGE(sErrorText.c_str());
        #endif

        ReportError(mError);
    }
}


void ErrorBase::TransferError(const ErrorBase& errorBaseSource)
{
    // We call HasError and GetError instead of directly copying the variables because
    // we want to quell the error checking assertions that fire if HasError isn't called.
    // Don't transfer mbEnableErrorAssertions.
    mbSuccess      = errorBaseSource.HasError() == false;
    mbHasErrorUsed = errorBaseSource.mbHasErrorUsed;
    mbGetErrorUsed = errorBaseSource.mbGetErrorUsed;
    mError         = errorBaseSource.GetError();
}



///////////////////////////////////////////////////////////////////////////////
// RegisterUserErrorHandler
///////////////////////////////////////////////////////////////////////////////

ErrorHandler gErrorHandler = NULL;
intptr_t     gErrorHandlerContext = 0;


EAPATCHCLIENT_API void RegisterUserErrorHandler(ErrorHandler pErrorHandler, intptr_t userContext)
{
    gErrorHandler        = pErrorHandler;
    gErrorHandlerContext = userContext;
}


EAPATCHCLIENT_API ErrorHandler GetUserErrorHandler(intptr_t* pUserContext)
{
    if(pUserContext)
        *pUserContext = gErrorHandlerContext;
    return gErrorHandler;
}


EAPATCHCLIENT_API void ReportError(Error& error)
{
    if(!error.GetErrorReported()) // Avoid double-reporting errors...
    {
        error.SetErrorReported(true);

        intptr_t     userContext;
        ErrorHandler pErrorHandler = GetUserErrorHandler(&userContext);

        if(pErrorHandler)
            pErrorHandler(error, userContext);
    }
}



} // namespace Patch
} // namespace EA







