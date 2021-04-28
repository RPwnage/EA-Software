///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/PatchInfo.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_PATCHINFO_H
#define EAPATCHCLIENT_PATCHINFO_H


#include <EAPatchClient/Base.h>
#include <EAPatchClient/Error.h>
#include <EAPatchClient/Locale.h>
#include <EAPatchClient/Serialization.h>
#include <EASTL/fixed_vector.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
    #pragma warning(disable: 4275)  // non dll-interface class used as base for dll-interface class.
#endif


namespace EA
{
    namespace IO
    {
        class IStream;
    }

    namespace Patch
    {
        class XMLWriter;
        class XMLReader;


        /// PatchInfo
        ///
        /// Describes a patch. Doesn't provide the patch itself.
        /// The application typically would download a PatchInfo and 
        /// based on its contents and the users actions decide to download
        /// the PatchImplementation, which bigger and not a descriptive thing.
        /// PatchInfo instances are downloaded by PatchDirectoryRetriever, as 
        /// opposed to having a dedicated PatchInfoRetriever class.
        ///
        /// This struct can be cached to disk as an XML .eaPatchInfo (or .eaPI) file, though 
        /// it's not usually necessary because PatchInfo is something that's 
        /// only shown to the user and is re-downloaded every time and thus
        /// doesn't need caching to disk.
        ///
        class EAPATCHCLIENT_API PatchInfo : public ErrorBase
        {
        public:
            PatchInfo();
           ~PatchInfo();

            void Reset();

        public:
            // Const data published by the patch server         /// Generated?     | Required? | Description
            //-------------------------------------------------------------------------------------------------------------------------------------------------------------------
            String              mPatchInfoURL;                  /// User-specified | Required  | The URL for this PatchInfo. 
            String              mPatchId;                       /// User-specified | Required  | Unique string, like a GUID. mPatchName is the user-readable name of the patch.
            String              mPatchClass;                    /// User-specified |           | Indicates whether the patch is a fix, add-on, etc. Case-insensitive.
            bool                mRequired;                      /// User-specified |           | True if the patch is required in order to further run the application.
            LocalizedString     mPatchImplURL;                  /// User-specified | Required  | URL for the patch implementation (.eaPatchImpl file) itself. May get redirected by the server.
            StringArray         mContentIdArray;                /// User-specified |           | Entitlement ids, defined by the people who tie SKUs and users to downloadable entitlements.
            StringArray         mSKUIdArray;                    /// User-specified |           | List of SKUs which this patch is valid for.
            LocalizedString     mPatchName;                     /// User-specified |           | Human readable patch name. Must be text only, and not HTML, RTF, etc.
            LocalizedString     mPatchDescription;              /// User-specified |           | Human readable patch name. Must be text, HTML text, or a URL to an HTML page suitable for displaying a nice looking display of the patch from within the application (e.g. via EAWebKit).
            LocalizedString     mPatchEULA;                     /// User-specified |           | EULA text. May be HTML text.
            String              mAppId;                         /// User-specified |           | Name of App which this patch applies to.
            StringArray         mOSNameArray;                   /// User-specified |           | OSs which this patch applies to. See PATCH_OS.
            StringArray         mMinOSVersionArray;             /// User-specified |           | Minimum OS versions needed to be able to download and use the patch. Synchronized with mOSNameArray.
            String              mReleaseDate;                   /// User-specified |           | Date in an ISO 8601 format: "yyyy-MM-dd hh:mm:ss GMT", such as "1994-11-06 08:49:37 GMT" for November 6, 1994.
            LocalizedString     mLocaleSupport;                 /// User-specified |           | List of supported locales. GetString returns NULL if the locale isn't supported; returns a non-NULL empty string if supported. 
            StringArray         mSupercedentPatchIdArray;       /// User-specified |           | List of patches which this patch supercedes, and thus aren't necessary if you install this patch.
            StringArray         mSupercedingPatchIdArray;       /// User-specified |           | List of patches which supercede this patch.
            StringArray         mDependentPatchIdArray;         /// User-specified |           | List of patches which this patch depends on being installed before this one can be installed.
            String              mPatchBaseDirectory;            /// User-specified |           | Where the patch goes. The format of this string is platform and possibly application-specific. The application may need to fix this up at runtime before starting the patching process, for example if it's a relative directory or a user-specific directory.
            String              mUserInfo;                      /// User-specified |           | Optional arbitrary information the user might want to associate with the patch.
            String              mUserChecksum;                  /// User-specified |           | Optional checksum/hash the user might want to associate with the patch. May have a different meaning than any kind of hash we use elsewhere within this library.
            uint64_t            mFinalFileSizeUsage;            /// Generated      |           | The sum of the file sizes that an installed version of this patch uses. Doesn't include ignored directories; includes just the files in the patch.
            uint64_t            mFinalDiskSpaceUsage;           /// Generated      |           | The amount of disk space an installed version of this patch uses. Doesn't include ignored directories; includes just the files in the patch. This is usually a value a little greater than the sum of the file sizes, due to things like file system sector sizes.
            uint64_t            mIntermediateDiskSpaceUsage;    /// Generated      |           | The max amount of disk space the patch requires to have as part of its download and install. This is > mFinalDiskSpaceUsage, as in the simplest case it could need a full patch image plus intermediate files created by the build process.

        public:
            /// Returns the content pointed to by mPatchDescription if it's HTML (directly or via URL).
            /// This string is an HTML page for describing the patch. It may contain anything a conventional
            /// HTML 5 page might have, and is expected to be rendered with a compliant HTML engine such as EAWebKit.
            /// If the return value is false then there is no HTML present in mPatchDescription, but strHTML will
            /// still contain mPatchDescription's contents.
            bool GetPatchDescriptionHtml(String& strHTML) const;  

        protected:
            String mPatchDescriptionHTMLContent;    // Content of mPatchDescription, if HTML.
        };

        typedef eastl::vector<PatchInfo, CoreAllocator> PatchInfoArray;


        /// PatchInfoSerializer
        ///
        /// Serializes PatchInfo to and from a stream.
        /// To consider: Move this and other Serializer classes to a seperate .h/.cpp file.
        ///
        class EAPATCHCLIENT_API PatchInfoSerializer : public SerializationBase
        {
        public:
            /// File version of Serialize. This is essentially a serialization function.
            /// Writes the PatchInfo as an XML file to the given file path.
            /// If the return value is false, consult GetError to determine the error cause.
            bool Serialize(const PatchInfo& patchInfo, const char8_t* pFilePath, bool bAddHelpComments);

            /// Stream version of Serialize. This is essentially a serialization function.
            /// Writes the PatchInfo as an XML file to the given IO stream.
            bool Serialize(const PatchInfo& patchInfo, EA::IO::IStream* pStream, bool bAddHelpComments);

            /// File path version of Deserialize.
            /// Reads the PatchInfo as an XML file from the given IO stream.
            /// If bConvertPathsToLocalForm is true, file paths (not URL paths) are converted
            /// to local form (e.g. convert / to \ if running on Microsoft platforms).
            bool Deserialize(PatchInfo& patchInfo, const char8_t* pFilePath, bool bConvertPathsToLocalForm);

            /// Stream version of Deserialize.
            bool Deserialize(PatchInfo& patchInfo, EA::IO::IStream* pStream, bool bConvertPathsToLocalForm);

            // Helper functions.
            // These are separate functions because they are shared with other code.
            static void SerializePatchInfoContents(const PatchInfo& patchInfo, XMLWriter& writer, bool bAddHelpComments);
            static void DeserializePatchInfoContents(PatchInfo& patchInfo, XMLReader& reader, bool& bFound, bool bConvertPathsToLocalForm);
        };

    } // namespace Patch

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard



 




