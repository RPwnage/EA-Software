///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Error.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_ERROR_H
#define EAPATCHCLIENT_ERROR_H


#include <EAPatchClient/Base.h>
#include <EAPatchClient/Debug.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
    #pragma warning(disable: 4275)  // non dll-interface class used as base for dll-interface class.
#endif


namespace EA
{
    namespace Patch
    {
        /// SystemErrorId
        ///
        /// Identifies an OS-specific system error id. 
        /// Its type may depend on the OS.
        ///
        typedef uint32_t SystemErrorId;

        const uint32_t kSystemErrorIdNone    = 0x00000000;
        const uint32_t kSystemErrorIdUnknown = 0xffffffff;
        // Other system error ids are defined by the system and platform SDK.


        /// GetLastSystemErrorId
        ///
        /// Returns the last relevant system error id. On Windows this maps to a call to GetLastError().
        /// On Unix this maps to a call to errno. On some platforms this function cannot be implemented
        /// because there is no global last error and instead error values are returned from API calls.
        ///
        EAPATCHCLIENT_API SystemErrorId GetLastSystemErrorId();



        /// EAPatchErrorId
        ///
        typedef uint32_t EAPatchErrorId;
                                                                            // To do: Document each of these. Possibly revise the naming to be more consistent.
        const uint32_t kEAPatchErrorIdNone                      = 0x00000000;
        const uint32_t kEAPatchErrorIdUnknown                   = 0xffffffff;

        // EAPatch-level errors
        const uint32_t kEAPatchErrorIdAlreadyActive             = 0x00001000;   /// A patching process is already occurring with the patch client instance and destination directory. This requires waiting for the other patch to complete or be cancelled.
        const uint32_t kEAPatchErrorIdTooManyFiles              = 0x00001001;   /// There are too many files in the patch. Tell the patch maker to allow more files.
        const uint32_t kEAPatchErrorIdInvalidSettings           = 0x00001002;   /// The settings are invalid.
        const uint32_t kEAPatchErrorIdExpectedFileMissing       = 0x00001003;
        const uint32_t kEAPatchErrorIdUnexpectedFilePresent     = 0x00001004;
        const uint32_t kEAPatchErrorIdExpectedDirMissing        = 0x00001005;
        const uint32_t kEAPatchErrorIdUnexpectedDirPresent      = 0x00001006;

        // OS-level file system errors
        const uint32_t kEAPatchErrorIdFileOpenFailure           = 0x00002000;   /// Unable to open a disk file. This is usually a fatal error.
        const uint32_t kEAPatchErrorIdFileReadFailure           = 0x00002001;   /// Unable to read a disk file. This is usually a fatal error.
        const uint32_t kEAPatchErrorIdFileReadTruncate          = 0x00002002;   /// The file's contents seem to not fully present. This is usually a fatal error as it indicates the patch is corrupt.
        const uint32_t kEAPatchErrorIdFileWriteFailure          = 0x00002003;   /// Unable to write a disk file. This is usually a fatal error.
        const uint32_t kEAPatchErrorIdFilePosFailure            = 0x00002004;   /// Unable to seek a disk file. This is usually a fatal error.
        const uint32_t kEAPatchErrorIdFileCloseFailure          = 0x00002005;   /// Failure to close a disk file.
        const uint32_t kEAPatchErrorIdFileCorrupt               = 0x00002006;   /// 
        const uint32_t kEAPatchErrorIdFileRemoveError           = 0x00002007;   /// Failure to delete a disk file.
        const uint32_t kEAPatchErrorIdFileNameLength            = 0x00002008;   /// 
        const uint32_t kEAPatchErrorIdFilePathLength            = 0x00002009;   /// 
        const uint32_t kEAPatchErrorIdFileNotFound              = 0x0000200a;   /// The specified file was not found.
        const uint32_t kEAPatchErrorIdFileNotSpecified          = 0x0000200b;   /// The file was not specified.
        const uint32_t kEAPatchErrorIdFileRenameError           = 0x0000200c;   /// The file could not be renamed.
        const uint32_t kEAPatchErrorIdFileCreateError           = 0x0000200d;   /// The file could not be created.
        const uint32_t kEAPatchErrorIdDirCreateError            = 0x00002020;   /// Failure to create a directory.
        const uint32_t kEAPatchErrorIdDirReadError              = 0x00002021;   /// Failure to read a directory during directory iteration.
        const uint32_t kEAPatchErrorIdDirRenameError            = 0x00002022;   /// Failure to rename a directory.
        const uint32_t kEAPatchErrorIdDirRemoveError            = 0x00002023;   /// Failure to delete a directory.

        // XML errors
        const uint32_t kEAPatchErrorIdXMLError                  = 0x00003000;   /// Received XML was invalid or corrupt. This is usually a fatal error because it means our patch implementation is bad.

        // Memory errors
        const uint32_t kEAPatchErrorIdMemoryFailure             = 0x00004000;   /// Failure to allocate memory. This requires at least restarting the application.

        // URL errors
        const uint32_t kEAPatchErrorIdURLError                  = 0x00005000;

        // Threading errors
        const uint32_t kEAPatchErrorIdThreadError               = 0x00006000;   /// Unable to create a thread. This requires at least restarting the application.

        // File server errors
        const uint32_t kEAPatchErrorIdServerError               = 0x00007000;   /// EA::Patch::Server class problem.

        // HTTP protocol errors
        const uint32_t kEAPatchErrorIdHttpInitFailure           = 0x00008000;   /// Unable to initialize the HTTP transport. This requires at least restarting the application.
        const uint32_t kEAPatchErrorIdHttpDNSFailure            = 0x00008001;   /// Unable to lookup the server DNS address.
        const uint32_t kEAPatchErrorIdHttpConnectFailure        = 0x00008002;   /// Unable to connect to the server.
        const uint32_t kEAPatchErrorIdHttpSSLSetupFailure       = 0x00008003;   /// Failure in secure setup.
        const uint32_t kEAPatchErrorIdHttpSSLCertInvalid        = 0x00008004;   /// Invalid SSL server certificate.
        const uint32_t kEAPatchErrorIdHttpSSLCertHostMismatch   = 0x00008005;   /// SSL Certificate is not issued to this host.
        const uint32_t kEAPatchErrorIdHttpSSLCertUntrusted      = 0x00008006;   /// SSL Certificate is not trusted/recognized.
        const uint32_t kEAPatchErrorIdHttpSSLCertRequestFailure = 0x00008007;   /// CA fetch request failed. 
        const uint32_t kEAPatchErrorIdHttpSSLConnectionFailure  = 0x00008008;   /// Failure in secure connection after setup 
        const uint32_t kEAPatchErrorIdHttpSSLUnknown            = 0x00008009;   /// Unknown SSL error.
        const uint32_t kEAPatchErrorIdHttpDisconnect            = 0x0000800a;   /// The network connection was lost. This requires re-trying later. No need to restart the application.
        const uint32_t kEAPatchErrorIdHttpUnknown               = 0x0000800b;   /// Unknown HTTP transport error. We should just treat this like a disconnect.
        const uint32_t kEAPatchErrorIdHttpBusyFailure           = 0x0000800c;   /// Trying to do two downloads with the same HTTP client.

        const uint32_t kEAPatchErrorIdHttpCodeBegin             = 0x00009000;   /// HTTP error code, such as 400 or 500 errors. The string begins with the error number.
        const uint32_t kEAPatchErrorIdHttpCode100               = 0x00009064;   /// Beginning of 100 series codes. These are not errors.
        const uint32_t kEAPatchErrorIdHttpCode200               = 0x000090C8;   /// Beginning of 200 series codes. These are not errors.
        const uint32_t kEAPatchErrorIdHttpCode300               = 0x0000912C;   /// Beginning of 300 series codes. These are not errors.
        const uint32_t kEAPatchErrorIdHttpCode400               = 0x00009190;   /// Beginning of 400 series codes.
        const uint32_t kEAPatchErrorIdHttpCode500               = 0x000091F4;   /// Beginning of 500 series codes.



        /// SystemErrorIdToString
        ///
        /// Converts a system error id to a readable string under the current system locale.
        /// Output is in the format of: 
        ///     prefixId = true:  "<id>: <string>"
        ///     prefixId = false: "<string>"
        /// Returns true if the id was a recognized system error id. The return value doesn't 
        /// indicate a failure of the function to print some result.
        /// There may be newlines (\n) in the result string.
        ///
        EAPATCHCLIENT_API bool SystemErrorIdToString(EAPatchErrorId id, String& sResult, bool prefixId);


        /// EAPatchErrorIdToString
        ///
        EAPATCHCLIENT_API bool EAPatchErrorIdToString(EAPatchErrorId id, String& sResult, bool prefixId);



        /// ErrorSource
        /// 
        /// Enumerates the entities that can cause an error
        ///
        enum ErrorSource
        {
            kESNone,
            kESPatchDirectoryRetriever,
            kESPatchInfoRetriever
        };

        /// ErrorSourceToString
        ///
        /// This is currently intended for internal use only and not localized user use.
        ///
        EAPATCHCLIENT_API bool ErrorSourceToString(uint32_t errorSource, String& sResult);


        /// ErrorClass
        /// 
        /// Categorizes the error as to how the PatchClient should respond to it.
        ///
        enum ErrorClass
        {
            kECNone,           /// Not an error, or belongs to no class. The user can always pause or cancel on their own during patch application, even when there is no error.
            kECBenign,         /// The error is something to log, but doesn't affect the ability of the patch process to continue. We log this but don't notify the user or pause the patching process. The user can always pause or cancel on their own.
            kECTemporary,      /// Requires us to delay or pause the patching process until a later. e.g. Network lost connectivity. We notify the user only passively (e.g. status string), pause the patching process, but retry it every N seconds. The user can always pause or cancel on their own.
            kECBlocking,       /// Prevents the patching process from completing, but isn't an error in the patching system itself. e.g. Out of disk space, out of memory. We pause the patching process and ask the user to retry now, pause until the user can fix something, or cancel the patch application. Maybe the user needs to clean up disk space, maybe needs to simply reboot the computer.
            kECFatal           /// Refers to an error in the patching system itself which appears to be the fault of the patching system. e.g. Patch files are corrupt, server says patch file doesn't exist. We still ask the user to retry, pause, or cancel, same as with kECBlocking.
        };


        ///////////////////////////////////////////////////////////////////////
        /// Error
        ///
        /// This class has copy and assignment semantics.
        /// To consider: Have this class inherit from a C++ std::exception or
        /// write an adapter class which inherits from std::exception.
        /// 
        class EAPATCHCLIENT_API Error
        {
        public:
            Error();
            Error(const Error& error);
            Error(ErrorClass errorClass, SystemErrorId systemErrorId, EAPatchErrorId eapatchErrorId);
            Error(ErrorClass errorClass, SystemErrorId systemErrorId, EAPatchErrorId eapatchErrorId, const char8_t* pSourceFilePath, int sourceFileLine);
            virtual ~Error();

            const Error& operator=(const Error& error);

            void Clear();

            // Reset acts like Clear but also frees any memory that's possibly been allocated.
            void Reset();

            void SetErrorId(ErrorClass errorClass, SystemErrorId systemErrorId, EAPatchErrorId eapatchErrorId, const char8_t* pErrorContext = NULL);
            void GetErrorId(ErrorClass& errorClass, SystemErrorId& systemErrorId, EAPatchErrorId& eapatchErrorId) const;

            /// sContext is a string that's relevant to the operation in error, such as a 
            /// file path that failed to open or URL that failed to download. It can also be 
            /// an arbitrary description string. It is only informational.
            void SetErrorContext(const char8_t* pErrorContext);
            void GetErrorContext(String& sErrorContext) const;

            /// Refers to the __FILE__/__LINE__ where the error was first detected.
            void SetErrorSource(const char8_t* pSourceFilePath, int sourceFileLine);
            void GetErrorSource(String& sourceFilePath, int& sourceFileLine) const;

            /// There may be newlines (\n) in the result string.
            void GetErrorString(String& sError) const;
            const char* GetErrorString() const;

            bool GetErrorReported() const { return mbErrorReported; }
            void SetErrorReported(bool reported) { mbErrorReported = reported; }

        protected:
            ErrorClass     mErrorClass;         /// 
            ErrorSource    mErrorSource;        /// To consider: Do we want to make this a string instead of an enum?
            SystemErrorId  mSystemErrorId;      /// 
            EAPatchErrorId mEAPatchErrorId;     /// 
            String         mErrorContext;       /// This might be a file path that failed to open, a URL that failed to download.
            String         mSourceFilePath;     /// The source code file path where the error was first detected (e.g. __FILE__).
            int            mSourceFileLine;     /// The source code line number where the error was first detected (e.g. __LINE__).
            mutable String mErrorString;        /// The full error string description.
            bool           mbErrorReported;     /// True if the error has been reported to the application's registered user error handler. See RegisterUserErrorHandler. 
        };



        ///////////////////////////////////////////////////////////////////////
        /// ErrorBase
        ///
        /// Implements a base class for having an Error variable and functions 
        /// for maintaining its state.
        ///
        /// Classes that implement error tracking can inherit from this class
        /// and thus gain the variables for tracking whether they are in an 
        /// error state and what the Error is if so.
        ///
        /// It's important that subclasses of this class make sure that the class 
        /// stops executing upon the first error encountered, and the GetError
        /// function thus returns that initial encountered error. The primary 
        /// way to accomplish this is for subclasses to check mbSuccess before 
        /// doing any operations of significance.
        ///
        /// In this class' destructor, it asserts that the user has called HasError.
        /// This is to help ensure that there are no missed errors. Granted, it 
        /// still depends on the subclass of this class reporting errors properly.
        ///
        /// Example usage:
        ///     class PatchInfoSerializer : public ErrorBase {
        ///       public:
        ///         bool Serialize(EA::IO::IStream* pStream);
        ///         bool Deserialize(EA::IO::IStream* pStream);
        ///     };
        ///
        ///     bool PatchInfo::Serialize(EA::IO::IStream* pStream)
        ///     {
        ///         if(mbSuccess) // If we aren't already in an error state...
        ///         {
        ///             if(!pStream->Write(blah))
        ///                 HandleError(kECBlocking, errno, kEAPatchErrorFileWriteFailure, &String(GetFilePathFromStream(pStream))); // Will set mbSuccess to false and set mError.
        ///         }
        ///     
        ///         return mbSuccess;
        ///     }
        ///
        class ErrorBase
        {
        public:
            ErrorBase();
            ErrorBase(const ErrorBase& errorBase);
           ~ErrorBase();

            ErrorBase& operator=(const ErrorBase& errorBase);

            /// Returns true if in an error state.
            bool HasError() const;

            /// Returns the last error object. 
            Error GetError() const;

            /// Sets the error state to its initial non-error state.
            /// Don't call this arbitrarily to shut up an error unless you know what you're doing.
            void ClearError();

            // Reset acts like ClearError but also frees any memory that's possibly been allocated.
            void Reset();

            /// Enables or disables reports that HasError/GetError haven't been called.
            /// This is needed for cases when you know there can never be an error and 
            /// can't check for one because the class is auto-destructing, for example.
            void EnableErrorAssertions(bool enable);

            /// Implements a basic error handler. Sets the error to the given error and does a 
            /// debug trace of the error information. 
            /// The arguments to this function match the member data of the Error class.
            /// pContext may be NULL, to indicate no known context. Recall that pContext is a string
            /// that's relevant to the operation in error, such as a file path that failed to open or 
            /// URL that failed to download. It can also be an arbitrary description string. It is 
            /// only informational.
            void HandleError(ErrorClass errorClass, SystemErrorId systemErrorId, EAPatchErrorId eaPatchErrorId, const char8_t* pErrorContext = NULL, bool reportError = true);
            void HandleError(const Error& error, bool reportError = true);

            /// Moves the error state from errorBaseSource to this.
            /// This is like saying static_cast<ErrorBase>(*this) = errorBaseSource except avoiding casting 
            /// or unusual code needed for that. But more importantly it also clears the error checking
            /// of the errorBaseSource.
            void TransferError(const ErrorBase& errorBaseSource);

        protected:
            volatile bool mbSuccess;                /// Set to true on construction, false on any subsequent error. If true then mError will indicate the error.
            mutable bool  mbHasErrorUsed;           /// True if HasError has been called. Used to assert that the user is using error checking.
            mutable bool  mbGetErrorUsed;           /// True if GetError has been called. Used to assert that the user is using error checking.
            bool          mbEnableErrorAssertions;  /// True if we assert on destruction that GetError/HasError hasn't been called. By default this is true.
            Error         mError;                   /// The Error information.
        };



        ///////////////////////////////////////////////////////////////////////
        /// RegisterUserErrorHandler
        ///
        /// This is used by the application to detect any errors when they occur.
        /// The ErrorHandler may be called from a thread different than the thread
        /// which registered the handler, though the ErrorHandler will not be called 
        /// during a time when it is unsafe to execute arbitrary application code, 
        /// such as might be the case within a Unix signal handler, for example.
        /// However, the application should not block during handling of the error.
        /// 
        /// The registered error handler should usually not respond by cancelling
        /// the operation or requesting user feedback, but rather it should use
        /// the callback for logging and passive display purposes. Actions such as
        /// cancelling a patch or requesting user feedback about an error should be
        /// handled after the given patching operation completes. However, unilateral
        /// patch operation cancellations can in fact be done from the callback if necessary.
        ///
        /// To do: Change this from being global to being associated with a class,
        /// such as PatchBuilder, etc. The problem with it being global is that it
        /// makes it hard to associate context with the error callback. Note that 
        /// we are already taking steps in this direction by having the PatchBuilder
        /// class implement an Error callback as part of it's event callback system,
        /// and implementing that for the other high level classes (e.g. DiectoryRetriever)
        /// would possibly allow us to just delete the code below.
        ///
        /// Example usage:
        ///     class SomeClass {
        ///         void Init()
        ///             { EA::Patch::RegisterUserErrorHandler(StaticErrorHandler, (intptr_t)this); }
        ///         
        ///         static void StaticErrorHandler(EA::Patch::Error& error, intptr_t pThis)
        ///             { return ErrorHandler(error); }
        ///         
        ///         void ErrorHandler(EA::Patch::Error& error)
        ///             { printf("EAPatch error: %s\n, error.GetErrorString()); }
        ///     };
        ///
        typedef void (*ErrorHandler)(const Error& error, intptr_t userContext);

        EAPATCHCLIENT_API void         RegisterUserErrorHandler(ErrorHandler pErrorHandler, intptr_t userContext);
        EAPATCHCLIENT_API ErrorHandler GetUserErrorHandler(intptr_t* pUserContext = NULL);
        EAPATCHCLIENT_API void         ReportError(Error& error);


    } // namespace Patch

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard



 