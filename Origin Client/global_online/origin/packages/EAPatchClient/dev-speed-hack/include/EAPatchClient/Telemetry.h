///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Telemetry.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_TELEMETRY_H
#define EAPATCHCLIENT_TELEMETRY_H


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Base.h>
#include <EAPatchClient/SystemInfo.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
    #pragma warning(disable: 4275)  // non dll-interface class used as base for dll-interface class.
#endif


namespace EA
{
    namespace Patch
    {
        class PatchBuilder;
        class PatchDirectoryRetriever;



        struct TelemetryBase
        {
            String     mDateNumber;     // Event date in time_t format: GMT seconds since Jan 1, 1970.
            String     mDate;           // Event date in an ISO 8601 format: "yyyy-MM-dd hh:mm:ss GMT", such as "1994-11-06 08:49:37 GMT" for November 6, 1994.
            SystemInfo mSystemInfo;     // 

            void Init();
        };
 
        // This corresponds the GOS Telemetry begin session/end session.
        struct TelemetryPatchSystemInit : public TelemetryBase
        {
            // Nothing
        };

        struct TelemetryPatchSystemShutdown : public TelemetryBase
        {
            // Nothing
        };

        struct TelemetryDirectoryRetrievalBegin : public TelemetryBase
        {   
            PatchDirectoryRetriever& mPatchDirectoryRetriever;
            String                   mPatchDirectoryURL;     /// Where the .eaPatchDirectory came from.

            TelemetryDirectoryRetrievalBegin(PatchDirectoryRetriever& pdr) : mPatchDirectoryRetriever(pdr){}

        protected:
            // Hide as assignment is unsupported by PatchDirectoryRetriever
            void operator=(const TelemetryDirectoryRetrievalBegin& /*rhs*/){}
        };

        struct TelemetryDirectoryRetrievalEnd : public TelemetryBase
        {
            PatchDirectoryRetriever& mPatchDirectoryRetriever;   
            String                   mPatchDirectoryURL;     /// Where the .eaPatchDirectory came from.
            String                   mStatus;                /// See enum AsyncStatus. One of "Completed", "Deferred, "Cancelled, "Aborted", "Crashed"
            String                   mDirDownloadVolume;     /// uint64_t sum of bytes downloaded as the .eaPatchDir file. 0 if the operation never made it that far.
            String                   mInfoDownloadVolume;    /// uint64_t sum of bytes downloaded as .eaPatchInfo files. 0 if the operation never made it that far.

            TelemetryDirectoryRetrievalEnd(PatchDirectoryRetriever& pdr) : mPatchDirectoryRetriever(pdr){}

        protected:
            // Hide as assignment is unsupported by PatchDirectoryRetriever
            void operator=(const TelemetryDirectoryRetrievalEnd& /*rhs*/){}
        };

        struct TelemetryPatchBuildBegin : public TelemetryBase
        {
            PatchBuilder&   mPatchBuilder;                   /// 
            String          mPatchImplURL;                   /// Where the .eaPatchImpl came from.
            String          mPatchId;                        /// The PatchInfo::mPatchId value.
            String          mPatchCourse;                    /// "New" or "Resume" or "None". Indicates if this patch execution was a new execution, a resumed execution, or none (patch cancellation).

            TelemetryPatchBuildBegin(PatchBuilder& pb) : mPatchBuilder(pb){}

        protected:
            // Hide as assignment is unsupported by PatchDirectoryRetriever
            void operator=(const TelemetryPatchBuildBegin& /*rhs*/){}
        };

        struct TelemetryPatchBuildEnd : public TelemetryBase
        {
            PatchBuilder&   mPatchBuilder;                   /// The instance that generated this telemetry.
            String          mPatchImplURL;                   /// Where the .eaPatchImpl came from.
            String          mPatchId;                        /// The PatchInfo::mPatchId value.
            String          mPatchCourse;                    /// "New" or "Resume" or "None". Indicates if this patch execution was a new execution, a resumed execution, or none (patch cancellation).
            String          mPatchDirection;                 /// "Build" or "Cancel" (always build in this case). Indicates if this identifies a patch build or a patch cancellation (rollback).
            String          mStatus;                         /// See enum AsyncStatus. One of "Completed", "Deferred, "Cancelled, "Aborted", "Crashed"
            String          mDownloadSpeedBytesPerSecond;    /// Floating point value.
            String          mDiskSpeedBytesPerSecond;        /// Floating point value.
            String          mImplDownloadVolume;             /// uint64_t sum of bytes downloaded as the .eaPatchImpl file. 0 if the build never made it that far.
            String          mDiffDownloadVolume;             /// uint64_t sum of bytes downloaded as .eaPatchDiff files. 0 if the build never made it that far.
            String          mFileDownloadVolume;             /// uint64_t sum of bytes downloaded as patch files. 0 if the build never made it that far.
            String          mFileDownloadVolumeFinal;        /// uint64_t sum of all bytes needed to complete the build. 0 if the build never made it far enough to calculate this.
            String          mFileCopyVolume;                 /// uint64_t sum of bytes copied. 0 if the build never made it that far.
            String          mFileCopyVolumeFinal;            /// uint64_t sum of bytes needed to complete the build. 0 if the build never made it far enough to calculate this.
            String          mPatchTimeEstimate;              /// uint64_t seconds to complete a patch that wasn't completed in a single shot.
            String          mPatchTime;                      /// uint64_t seconds to complete an uninterrupted patch.

            TelemetryPatchBuildEnd(PatchBuilder& pb) : mPatchBuilder(pb){}

        protected:
            // Hide as assignment is unsupported by PatchBuilder
            void operator=(const TelemetryPatchBuildEnd& /*rhs*/){}
        };

        struct TelemetryPatchCancelBegin : public TelemetryBase
        {
            PatchBuilder&   mPatchBuilder;                   /// 
            String          mPatchImplURL;                   /// Where the .eaPatchImpl came from.
            String          mPatchId;                        /// The PatchInfo::mPatchId value.

            TelemetryPatchCancelBegin(PatchBuilder& pb) : mPatchBuilder(pb){}

        protected:
            // Hide as assignment is unsupported by PatchBuilder
            void operator=(const TelemetryPatchCancelBegin& /*rhs*/){}
        };

        struct TelemetryPatchCancelEnd : public TelemetryBase
        {
            PatchBuilder&   mPatchBuilder;                   /// 
            String          mPatchImplURL;                   /// Where the .eaPatchImpl came from.
            String          mPatchId;                        /// The PatchInfo::mPatchId value.
            String          mPatchDirection;                 /// "Build" or "Cancel". Indicates if this identifies a patch build or a patch cancellation (rollback).
            String          mStatus;                         /// See enum AsyncStatus. One of "Completed", "Deferred, "Cancelled, "Aborted", "Crashed"
            String          mCancelTimeEstimate;             /// uint64_t seconds to complete a patch that wasn't completed in a single shot.
            String          mCancelTime;                     /// uint64_t seconds to complete an uninterrupted patch.

            TelemetryPatchCancelEnd(PatchBuilder& pb) : mPatchBuilder(pb){}

        protected:
            // Hide as assignment is unsupported by PatchBuilder
            void operator=(const TelemetryPatchCancelEnd& /*rhs*/){}
        };



        /// TelemetryEventCallback
        ///
        /// Telemetry events are sent to the registered TelemetryEventCallback.
        ///
        class TelemetryEventCallback
        {
        public:
            virtual void TelemetryEvent(intptr_t userContext, TelemetryPatchSystemInit&);
            virtual void TelemetryEvent(intptr_t userContext, TelemetryPatchSystemShutdown&);
            virtual void TelemetryEvent(intptr_t userContext, TelemetryDirectoryRetrievalBegin&);
            virtual void TelemetryEvent(intptr_t userContext, TelemetryDirectoryRetrievalEnd&);
            virtual void TelemetryEvent(intptr_t userContext, TelemetryPatchBuildBegin&);
            virtual void TelemetryEvent(intptr_t userContext, TelemetryPatchBuildEnd&);
            virtual void TelemetryEvent(intptr_t userContext, TelemetryPatchCancelBegin&);
            virtual void TelemetryEvent(intptr_t userContext, TelemetryPatchCancelEnd&);

            virtual ~TelemetryEventCallback(){}
       };

    } // namespace Patch

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard



 




