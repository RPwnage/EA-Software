///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Base.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_BASE_H
#define EAPATCHCLIENT_BASE_H


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Allocator.h>
#include <EASTL/fixed_string.h>
#include <EASTL/fixed_map.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/list.h>
#include <EAStdC/EAStopwatch.h>
#include <time.h>


namespace EA
{
    namespace Patch
    {

        /// EAPATCH_CLASS
        /// 
        /// These are standard values. The user can define additional values.
        /// Names can have spaces. Comparisons done with these are case-insensitive.
        ///
        #define EAPATCH_CLASS_PATCH_STR "patch"
        #define EAPATCH_CLASS_ADDON_STR "add-on"


        /// EAPATCH_OS
        ///
        /// These are standard values. The user can define additional values.
        /// Names can have spaces. Comparisons done with these are case-insensitive.
        ///
        #define EAPATCH_OS_ANY_STR        "*"
        #define EAPATCH_OS_WIN32_STR      "Win32"
        #define EAPATCH_OS_WIN64_STR      "Win64"
        #define EAPATCH_OS_XBOX_360_STR   "XBox 360"
        #define EAPATCH_OS_PS3_STR        "PS3"
        #define EAPATCH_OS_WII_STR        "Wii"
        #define EAPATCH_OS_WIIU_STR       "WiiU"


        /// File name extensions
        /// 
        /// See the technical documentation for a further description of the roles of these files.
        /// We use defines for these instead of hard-coded strings throughout this library because some
        /// platforms may require alternative versions of these due to limitation. For example, the 360
        /// platform has a short file name limitation that may not allow so many characters for a file
        /// name extension in practice.
        ///
        #define kPatchDirectoryFileNameExtension ".eaPatchDir"          /// Patch directory file, which is a file that lists patches (as URLs to .eaPatchInfo files).
        #define kPatchInfoFileNameExtension      ".eaPatchInfo"         /// Patch info file.
        #define kPatchImplFileNameExtension      ".eaPatchImpl"         /// Patch implementation file.
        #define kPatchDiffFileNameExtension      ".eaPatchDiff"         /// Patch diff file.
        #define kPatchBFIFileNameExtension       ".eaPatchBFI"          /// Patch built file info file.
        #define kPatchTempFileNameExtension      ".eaPatchTemp"         /// Patch temp file.
        #define kPatchStateFileNameExtension     ".eaPatchState"        /// Indicates the last PatchBuilder state, which is stored as a single value written to this file.
        #define kPatchOldFileNameExtension       ".eaPatchOld"          /// Old file. This is what an old version of a file that is being patched is renamed to just before deleting it. It's deleted only after the .eaPatchTemp file is successfully renamed to the final version.
        #define kPatchInstalledFileNameExtension ".eaPatchInstalled"    /// Presence of this empty file indicates that the given patch install is complete. It's useful for the application indicating to the user what patches are currently installed (one patch per such file).
        #define kEAPatchFileTypeCount            9
        #define kPatchFileNameExtensionPattern   ".eaPatch*"            /// 


        /// String
        /// 
        /// To consider: Give this string a name that identifies its length, directly or indirectly.
        /// To consider: Come up with naming that more consistently reflects the type of container (e.g. fixed-size, normal).
        ///
        typedef eastl::fixed_string<char8_t, 32, true, CoreAllocator> String;          // UTF8 Unicode
        typedef eastl::fixed_string<char8_t,  8, true, CoreAllocator> StringSmall;     // UTF8 Unicode
        typedef eastl::fixed_vector<String,   4, true, CoreAllocator> StringArray;     // Array of String objects.
        typedef eastl::list<String, CoreAllocator>                    StringList;      // Array of String objects.


        /// Endian-ness
        ///
        #define EAPATCH_ENDIAN EA::IO::kEndianLittle



        /// AsyncStatus
        ///
        /// Generic indicator of the status of a download or execution of something.
        /// An async operation that hasn't been started has state kAsyncStatusNone.
        /// An async operation that is done (successfully or not) has one of the 
        /// states kAsyncStatusCompleted, kAsyncStatusCancelled, kAsyncStatusAborted, kAsyncStatusCrashed.
        ///
        enum AsyncStatus
        {
            kAsyncStatusNone = 0,       /// The action (e.g. file retrieval) hasn't started. This is the initial state of a newly constructed instance of this class.
            kAsyncStatusStarted,    /// The action is occurring.
            kAsyncStatusCancelling, /// The action is in the process of being cancelled by the user. However, the next status may not be kAsyncStatusCancelled, for example in the case of an error before the cancel could execute.
            kAsyncStatusCompleted,  /// The action succeeded.
            kAsyncStatusDeferred,   /// The action was stopped, but can be re-started later.
            kAsyncStatusCancelled,  /// The action was explicitly cancelled, which is not the same as aborted by error.
            kAsyncStatusAborted,    /// The action failed and stopped due to the reason specified by the Error.
            kAsyncStatusCrashed,    /// The action failed and stopped due to a crash (exception) or due to having the process or thread killed.
            
            // kNumAsyncStatus must be the last entry
            kNumAsyncStatus         /// The number of Async Statuses. Used in kAsyncStatusStateTable
        };

        /// This is the canonical means of telling if the an async operation is busy (in progress).
        inline bool IsAsyncOperationInProgress(AsyncStatus status)
            { return (status == kAsyncStatusStarted) || (status == kAsyncStatusCancelling); }

        inline bool IsAsyncOperationEnded(AsyncStatus status)
            { return (status != kAsyncStatusNone) && (status >= kAsyncStatusCompleted); }

        const char8_t* GetAsyncStatusString(AsyncStatus status);


        ///////////////////////////////////////////////////////////////////////////////
        // Time functions
        ///////////////////////////////////////////////////////////////////////////////

        // GetTimeSeconds returns absolute elapsed time in seconds since 1970. It is unaffected 
        // by system clock changes and thus will always return a value >= the previous value.
        EAPATCHCLIENT_API uint64_t GetTimeSeconds();
        EAPATCHCLIENT_API uint64_t GetTimeMilliseconds();
        EAPATCHCLIENT_API uint64_t GetTimeMicroseconds();
        EAPATCHCLIENT_API uint64_t GetTimeNanoseconds();


        ///////////////////////////////////////////////////////////////////////////////
        // Utility functions
        ///////////////////////////////////////////////////////////////////////////////

        /// StringToInt64
        /// StringToUint64
        ///
        /// This is a wrapper for EA::StdC::StrtoI64 which makes calling and 
        /// error handling simpler for the user.
        ///
        EAPATCHCLIENT_API bool     StringToInt64(const char8_t* p, int64_t& i64, int base = 0);     // Error-checking version.
        EAPATCHCLIENT_API int64_t  StringToInt64(const char8_t* p, int base = 0);                   // Non-error-checking version. Returns 0 on error, but you shouldn't call if there's a possibility of error you need to handle.

        EAPATCHCLIENT_API bool     StringToUint64(const char8_t* p, uint64_t& u64, int base = 0);   // Error-checking version.
        EAPATCHCLIENT_API uint64_t StringToUint64(const char8_t* p, int base = 0);                  // Non-error-checking version. Returns 0 on error, but you shouldn't call if there's a possibility of error you need to handle.


        /// StringToDouble
        ///
        /// This is a wrapper for EA::StdC::Strtod which makes calling and 
        /// error handling simpler for the user.
        ///
        EAPATCHCLIENT_API bool   StringToDouble(const char8_t* p, double& d);       // Error-checking version.
        EAPATCHCLIENT_API double StringToDouble(const char8_t* p);                  // Non-error-checking version. Returns 0 on error, but you shouldn't call if there's a possibility of error you need to handle.


        /// DateStringToTimeT
        ///
        /// Converts ISO 8601 date strings to time_t and back.
        /// As of this writing, GMT dates are the only type expected.
        /// You can use libraries like EAStdC to convert between dates
        /// in more advanced ways.
        /// ISO 8601 format used here: "YYYY-MM-DD hh:mm:ss GMT"
        /// The GMT portion of the string may be omitted, but TimeTToDateString
        /// will always print it.
        /// pDateString must have a capacity of at least kMinDateStringCapacity chars.
        ///     Example usage:
        ///         time_t t;
        ///         DateStringToTimeT("2012-06-15 23:59:59 GMT", t);
        ///
        ///         char str[kMinDateStringCapacity];
        ///         TimeTToDateString(t, str);
        ///
        /// To consider: These functions might not be needed. We might instead need to 
        /// implement conversions between strings and EA::StdC::DateTime objects.
        /// It happens that time_t can also be passed to DateTime.
        ///
        const size_t kMinDateStringCapacity = 24;
        EAPATCHCLIENT_API bool DateStringToTimeT(const char8_t* pDateString, time_t& t);
        EAPATCHCLIENT_API bool TimeTToDateString(const time_t& t, char8_t* pDateString);



        /// ClientLimits
        ///
        /// Defines user-specified limits on resources the client is allowed to use.
        /// For example, the client typically has a memory usage limit so that it doesn't
        /// exhaust the memory available to the application.
        ///
        /// To consider: Move this to another location, as it currently looks like it's 
        /// tied to this high level Client class but it applies to the lower level 
        /// classes.
        ///
        struct ClientLimits
        {
            uint64_t mDiskUsageLimit;           /// Max amount of disk space to use at any given time.
            uint64_t mMemoryUsageLimit;         /// All memory is controlled by the allocator specified by EA::Patch::SetAllocator.
            uint32_t mFileLimit;                /// Max number of files to have simultaneously open. Some platforms have severe restrictions on the open file count.
            uint32_t mThreadCountLimit;         /// Max number of simultaneous threads to create.
            uint32_t mSocketCountLimit;         /// Max number of simultaneous sockets to create.
            uint32_t mCPUCountLimit;            /// Max number of simultaneous CPUs to use.
            uint32_t mCPUUsageLimit;            /// Max percent of CPU usage to use over time, in range of [0, 100]. This is a fuzzy value, despite being numerically specified.
            uint32_t mNetworkReadLimit;         /// Max amount of network data to read in bytes per second. Beyond that, network reads are throttled.
            uint32_t mNetworkRetrySeconds;      /// How long between network connection retries, when the network appears to be unavailable.
            uint32_t mHTTPMemorySize;           /// How memory the HTTP client can use.
            uint32_t mHTTPConcurrentLimit;      /// Max number of concurrent HTTP downloads to do.
            uint32_t mHTTPTimeoutSeconds;       /// Max amount of seconds to wait for HTTP activity. Not the same as waiting for an HTTP download to complete, but rather the time to wait for an HTTP read (or write) to succeed.
            uint32_t mHTTPRetryCount;           /// Max number of times we retry an HTTP transfer before giving up.
            bool     mHTTPPipeliningEnabled;    /// True if HTTP pipelining is enabled. Default is false.

            ClientLimits();
        };

        /// GetDefaultClientLimits
        /// Gets the default client limits used by an EAPatchClient and specified by a 
        /// call to SetDefaultClientLimits. If SetDefaultClientLimits has not been called
        /// then a suitable basic set of limits are returned.
        EAPATCHCLIENT_API ClientLimits& GetDefaultClientLimits();
        EAPATCHCLIENT_API void          SetDefaultClientLimits(const ClientLimits& clientLimits);



        /// DataRateMeasure
        ///
        /// Measures the speed at which some kind of data is processed, such as how fast it is downloaded.
        /// 
        /// Example usage:
        ///     DataRateMeasure drm;
        ///
        ///     drm.Start();
        ///     drm.ProcessData(12000);
        ///     Printf("Current data rate in bytes per millisecond: %I64u\n", drm.GetDataRate());
        ///     drm.ProcessData(1000);
        ///     drm.Stop();
        ///     drm.Start();
        ///     drm.ProcessData(1000);
        ///     Printf("Current data rate in bytes per millisecond: %I64u\n", drm.GetDataRate());
        ///
        class EAPATCHCLIENT_API DataRateMeasure
        {
        public:
            DataRateMeasure();

            /// Starts the data rate timing. It is expected that calls to ProcessData
            /// are made while the timing is Started.
            void Start();

            /// Returns true if Start has been called and neither Stop nor Reset has been
            /// called since the call to Start.
            bool IsStarted() const;

            /// Stops data rate timing, which may be subsequently resumed again with Start without
            /// losing the accumulated metrics. The Reset function causes loss of the metrics.
            void Stop();

            /// Resets the timer and accumulated data metrics, the same as a 
            /// newly constructed instance of this class.
            void Reset();

            /// Records that dataSize units (e.g. bytes) have been processed. 
            /// Must be called while Started. 
            /// This function doesn't do anything but add dataSize to the existing
            /// data size sum.
            void ProcessData(uint64_t dataSize);

            /// Returns the sum of data passed to ProcessData since the last Reset.
            uint64_t GetDataSize() const;

            /// Returns the current data rate in units per millisecond.
            double GetDataRate() const;

        protected:
            EA::StdC::Stopwatch mStopwatch;
            uint64_t            mDataSizeSum;
        };

    } // namespace Patch

} // namespace EA



#endif // Header include guard



 




