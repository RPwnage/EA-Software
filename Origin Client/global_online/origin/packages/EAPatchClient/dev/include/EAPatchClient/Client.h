///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Client.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Note
//
// This class is incomplete and may either be implemented in the future or 
// removed altogether. This is intended to implement a high level Client class
// which hides the lower level patching classes, though it's not clear if this
// class will be necessary. In the meantime, you can use the functionality
// in the PatchDirectoryRetriever and PatchBuilder classes to implement the 
// two phases of patching (collecting list of patches, and implementing one
// of the patches).
/////////////////////////////////////////////////////////////////////////////














#ifndef EAPATCHCLIENT_CLIENT_H
#define EAPATCHCLIENT_CLIENT_H


#include <EAPatchClient/Base.h>
#include <EAPatchClient/Error.h>
#include <EAPatchClient/Locale.h>
#include <EAPatchClient/Server.h>
#include <EAPatchClient/Telemetry.h>
#include <EAPatchClient/PatchInfo.h>
#include <EAPatchClient/PatchInfo.h>
#include <EAPatchClient/PatchDirectory.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
    #pragma warning(disable: 4275)  // non dll-interface class used as base for dll-interface class.
#endif


namespace EA
{
    namespace Patch
    {
        /*
        /// PatchState
        ///
        /// Enumerates the states of the patching process, primarily for the purpose of 
        /// implementing a patching state machine and for providing user GUI feedback as 
        /// to the progress of the patching. 
        ///
        /// Patching is done in two steps, with application user choice occurring between them.
        ///
        enum PatchState
        {
            kPSNone,

            // Patch info retrieval
            kPSDownloadingPatchDirectory,       /// The directory (.eaPatchDir file) is a list of patch infos. There is typically only a single patch directory for a title. However, the game team may choose to separate patches and purchasable downloads into separate directories.
            kPSDownloadingPatchInfo,            /// There are zero or more patch infos (.eaPatchInfo files) in a directory listing. 

            kPSDownloadingPatchImpl,            /// There are one of these (.eaPatchImpl) per patch, and this state occurs when the user has chosen to proceed with installing a patch from the directory of patch infos.
            kPSDownloadingPatchFiles,           /// This encompasss downloading .eaPatchDiff and the files themselves.
            kPSApplyingPatch,                   /// This refers to applying building the destination files.
            kPSFinalizing                       /// After destination files have been built, they must replace the original files and the original files need to be deleted, etc.
        };


        /// PatchProgress
        ///
        /// Provides information about the status of a patching operation, primarily for 
        /// the purpose of providing (optional) user GUI feedback.
        /// 
        struct PatchProgress
        {
            //     bytes written, %complete, current file, file throughput, network throughput.
            //     How to measure progress: Assume that the entire patch image is being downloaded. Its size is known.
            //     The primary things a GUI needs to display progress.
        };
        */


        /// Client
        ///
        /// High level class for implementing patching.
        /// Instead of manually instantiating and managing the individual patch-related classes,
        /// You can just create an instances of this, fill it in, and use it.
        /// 
        /// You do the following things, in order, with this class:
        ///    1 Init.
        ///    2 Download patch directory.
        //     3 Present UI to user to select a patch from the directory.
        ///    4 Install a selected patch.
        ///    5 Shutdown.
        /// You may cycle between steps 3 and 4.
        /// See the EAPatchDemo and the documentation for example application code.
        ///
        /// If you want to understand how this class and patches in general work, you want to 
        /// read the EAPatchTechnicalDesign.html doc and some of the key references. Once you 
        /// understand them then this class and the auxiliary classes in this library and 
        /// most of what they do will be self-evident.
        ///
        class EAPATCHCLIENT_API Client
        {
        public:
            Client();
            virtual ~Client();

            /// SetClientLimits
            ///
            /// Specifies runtime settings related to resource usage for the patching process.
            /// By default a conservative group of limits is use per platform,
            /// but SetClientLimits can override them.
            /// 
            void          SetClientLimits(const ClientLimits&);
            ClientLimits& GetClientLimits() const;

            /// Set the locale or locales that the patch applies to.
            /// Can be more than one, which will result in installing patch support for all
            /// present patch locales. Locale matching is done as per the LocaleMatch function.
            /// This implementation is a pass-through to the equivalent Locale function.
            void SetLocale(const StringArray& localeArray);
            void GetLocale(StringArray& localeArray) const;

            /// Patch directory location.
            /// Example: https://patch.ea.com:1234/FIFA/2014/PS3/PatchDirectory.html
            /// The actual URL may be something a bit more cryptic than above.
            /// The patch URL is somthing that must be built into the game somehow retrievable
            /// by the game after it ships. The patch directory URL is the one thing that 
            /// bootstraps the entire patching process.
            /// This implementation is a pass-through to the equivalent Server function.
            const char8_t* GetPatchDirectoryURL() const;
            void           SetPatchDirectoryURL(const char8_t* pURL);

            // Get patch directory.


            // Install patch.


            // Pause, cancel, resume install.


            // Read progress.
            //     bytes written, %complete, current file, file throughput, network throughput.
            //     How to measure progress: Assume that the entire patch image is being downloaded. Its size is known.
            //     The primary things a GUI needs to display progress.

        protected:
            Client(const Client&);
            Client& operator=(const Client&);

        protected:
            Server                  mServer;
            PatchDirectoryRetriever mPatchDirectoryRetriever;
            ClientLimits            mClientLimits;
        };

    } // namespace Patch

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard



 
