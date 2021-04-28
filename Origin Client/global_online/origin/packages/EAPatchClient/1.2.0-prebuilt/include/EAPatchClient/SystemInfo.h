///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/SystemInfo.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_SYSTEMINFO_H
#define EAPATCHCLIENT_SYSTEMINFO_H


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Base.h>
#include <EAPatchClient/Serialization.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
    #pragma warning(disable: 4275)  // non dll-interface class used as base for dll-interface class.
#endif


namespace EA
{
    namespace Patch
    {

        /// SystemInfo
        ///
        /// Used to retrieve system information, such as OS version, memory etc.
        /// Used for telemetry (metrics reporting, though possibly has some use 
        /// during patch application.
        /// 
        /// We do not attempt to record all possible system info, as that's already
        /// handled by other functionality in most applications. 
        ///
        class EAPATCHCLIENT_API SystemInfo
        {
        public:
            String mEAUserId;               // EA Nucleus user id.
            String mMachineName;            // This may be privacy-sensitive.
            String mMachineNetworkName;     // This may be privacy-sensitive.
            String mMachineType;            // 
            String mMachineProcessor;       // 
            String mMachineMemory;          // 
            String mMachineCPUCount;        // 
            String mLanguage;               // 
            String mUserName;               // This may be privacy-sensitive.
            String mOSVersion;              // 
            String mSDKVersion;             // 
            String mCompilerVersion;        // 
            String mCompilerPlatformTarget; // 
            String mAppPath;                // 
            String mAppVersion;             // 
            String mEAPatchVersion;         // 

        public:
            SystemInfo(bool bSet = false, bool bEnableUserInfo = false);

            void Reset();
            void Set();

            bool operator==(const SystemInfo& x);

            bool mbEnableUserInfo;        // If true then enable privacy-sensitive user info. Disabled by default.
        };


        /* Disabled until deemed useful.

        /// SystemInfoSerializer
        ///
        /// Serializes SystemInfo to and from a stream/file.
        ///
        class EAPATCHCLIENT_API SystemInfoSerializer : public SerializationBase
        {
        public:
            /// File version of Serialize. This is essentially a serialization function.
            /// Writes the SystemInfo as an XML file to the given file path.
            /// If the return value is false, consult GetError to determine the error cause.
            bool Serialize(const SystemInfo& systemInfo, const char8_t* pFilePath);

            /// Stream version of Serialize. This is essentially a serialization function.
            /// Writes the SystemInfo as an XML file to the given IO stream.
            /// If writeDoc is true then we assume we are writing the entire XML doc and not just   
            /// our SystemInfo at the current location in the stream.
            bool Serialize(const SystemInfo& systemInfo, EA::IO::IStream* pStream, bool writeDoc);

            /// File path version of Deserialize.
            /// Reads the SystemInfo as an XML file from the given IO stream.
            bool Deserialize(SystemInfo& systemInfo, const char8_t* pFilePath);

            /// Stream version of Deserialize.
            /// If readDoc is true then we assume we are reading the entire XML doc and not just   
            /// our SystemInfo at the current location in the stream.
            bool Deserialize(SystemInfo& systemInfo, EA::IO::IStream* pStream, bool readDoc);
        };
        */

    } // namespace Patch

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard



 




