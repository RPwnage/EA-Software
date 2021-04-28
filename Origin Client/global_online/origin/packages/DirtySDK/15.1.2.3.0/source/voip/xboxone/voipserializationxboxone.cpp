/*H********************************************************************************/
/*!
    \File voipserializationxboxone.cpp

    \Description
        Xbox user and device serialization/deserialization code.

    \Copyright
        Copyright (c) Electronic Arts 2014. ALL RIGHTS RESERVED.

    \Version 03/04/2014 (mclouatre) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include "DirtySDK/platform.h"

#include <collection.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "voipserializationxboxone.h"


/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

Platform::Collections::Map<String ^, int> ^ g_refDeviceIdMap = nullptr;      // one numerical id per local capture source (capture source id/numerical id [0-255])

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _VoipMapDeviceId

    \Description
        Add device id to local map.

    \Input ^refDeviceId     - device id (winrt ref)

    \Output
        int32               - numerical device id

    \Version 02/28/2014 (mclouatre)
*/
/********************************************************************************F*/
static uint8_t _VoipMapDeviceId(String ^refDeviceId)
{
    uint8_t uDeviceId;
    static uint8_t uNextDeviceId = 0;

    if (g_refDeviceIdMap->HasKey(refDeviceId))
    {
        uDeviceId = g_refDeviceIdMap->Lookup(refDeviceId);
    }
    else
    {
        uDeviceId = uNextDeviceId++;
        g_refDeviceIdMap->Insert(refDeviceId, uDeviceId);
    }

    return(uDeviceId);
}


/*** Public functions *************************************************************/

#ifndef DOXYGEN_SHOULD_SKIP_THIS   // defined in .dxy file
/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    RemoteUser

    \Description
        Constructor

    \Input uUserId  - value used to uniquely identify the user locally on this machine

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
RemoteUser::RemoteUser(uint32_t uUserId)
{
    m_Id = uUserId;
    m_uEnablePlayback = 0xFFFFFFFF; // enable playback by default
    m_IsBadRepMute = FALSE;
    m_IsFriend = FALSE;
    m_refBadReqOp = nullptr;
    m_uBadRepTimer = 0;
};

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    RemoteUser

    \Description
        Destructor

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
RemoteUser::~RemoteUser()
{
};

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    EnablePlaybackFlag::get

    \Description
        Getter for EnablePlaybackFlag

    \Output
        uint32_t EnablePlaybackFlag

    \Version 20/11/2014 (tcho)
*/
/********************************************************************************F*/
uint32_t RemoteUser::EnablePlaybackFlag::get()
{
    return(m_uEnablePlayback);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    EnablePlaybackFlag::set

    \Description
        Setter for EnablePlaybackFlag

    \Input 
        uEnablePlaybackFlag - Enable Voice Playback flags

    \Version 11/20/2014 (tcho)
*/
/********************************************************************************F*/
void RemoteUser::EnablePlaybackFlag::set(uint32_t uEnablePlaybackFlag)
{
    m_uEnablePlayback = uEnablePlaybackFlag;
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    IsBadRepMute::get

    \Description
        Getter for IsBadRepMute

    \Output
        uint8_t IsBadRepMute

    \Version 26/10/2015 (tcho)
*/
/********************************************************************************F*/
uint8_t RemoteUser::IsBadRepMute::get()
{
    return(m_IsBadRepMute);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    IsBadRepMute::set

    \Description
        Setter for IsBadRepMute

    \Input 
       uint8_t isBadRepMute - should the user be muted because of bad rep

    \Version 10/26/2015 (tcho)
*/
/********************************************************************************F*/
void RemoteUser::IsBadRepMute::set(uint8_t isBadRepMute)
{
    m_IsBadRepMute = isBadRepMute;
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    BadReqOp::get

    \Description
        Getter for BadRepOp

    \Output
        IAsyncOperation<UserStatistics::UserStatisticsResult^> m_refBadReqOp

    \Version 04/18/2016 (tcho)
*/
/********************************************************************************F*/
IAsyncOperation<UserStatistics::UserStatisticsResult^> ^RemoteUser::BadReqOp::get()
{
    return (m_refBadReqOp);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    BadReqOp::set

    \Description
        Setter for BadRepOp

    \Input 
        IAsyncOperation<UserStatistics::UserStatisticsResult^> ^refBadRepOp - the bad rep async op

    \Version 04/18/2016 (tcho)
*/
/********************************************************************************F*/
void RemoteUser::BadReqOp::set(IAsyncOperation<UserStatistics::UserStatisticsResult^> ^ refBadRepOp)
{
    m_refBadReqOp = refBadRepOp;
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    BadReqTimer::get

    \Description
        Getter for Bad Rep Timer

    \Output
        uint32_t m_uBadReqTimer

    \Version 04/18/2016 (tcho)
*/
/********************************************************************************F*/
uint32_t RemoteUser::BadRepTimer::get()
{
    return (m_uBadRepTimer);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    BadReqTimer::set

    \Description
        Setter for Bad Rep Timer

    \Input
        int32_t uBadReqTimer

    \Version 04/18/2016 (tcho)
*/
/********************************************************************************F*/
void RemoteUser::BadRepTimer::set(uint32_t uBadRepTimer)
{
    m_uBadRepTimer = uBadRepTimer;
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    IsFriend::get

    \Description
        Getter for IsFriend

    \Output
        uint32_t IsFriend

    \Version 10/26/2015 (tcho)
*/
/********************************************************************************F*/
uint8_t RemoteUser::IsFriend::get()
{
    return(m_IsFriend);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    IsFriend::set

    \Description
        Setter for IsFriend

    \Input 
        IsFriend - Is the bad reputation muting flag valid

    \Version 26/10/2014 (tcho)
*/
/********************************************************************************F*/
void RemoteUser::IsFriend::set(uint8_t IsFriend)
{
    m_IsFriend = IsFriend;
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    AudioDevices::get

    \Description
        Getter for AudioDevices property

    \Output
        IVectorView<IAudioDeviceInfo ^> ^ - immutable collection of audio devices

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
IVectorView<IAudioDeviceInfo ^> ^RemoteUser::AudioDevices::get()
{
    return(m_AudioDevices);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    AudioDevices::set

    \Description
        Setter for AudioDevices property

    \Input
        ^refAudioDeviceInfoVectorView   - immutable collection of audio devices

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
void RemoteUser::AudioDevices::set(IVectorView<IAudioDeviceInfo ^> ^ refAudioDeviceInfoVectorView)
{
    m_AudioDevices = refAudioDeviceInfoVectorView;
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    Controllers::get

    \Description
        Getter for Controllers property
        
    \Output
        IVectorView<IController ^> ^ - immutable collection of controllers

    \Notes
        MS confirmed that this property is not used by the game chat sub-system.
        So we always return nullptr.

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
IVectorView<IController ^> ^RemoteUser::Controllers::get()
{
    return(nullptr);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    DisplayInfo::get

    \Description
        Getter for DisplayInfo property

    \Output
        UserDisplayInfo ^ - display info

    \Notes
        MS confirmed that this property is not used by the game chat sub-system.
        So we always return nullptr.

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
UserDisplayInfo ^RemoteUser::DisplayInfo::get()
{
    return(nullptr);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    Id::get

    \Description
        Getter for Id property

    \Output
        uint32  - id

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
uint32 RemoteUser::Id::get()
{
    return(m_Id);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    Id::set

    \Description
        Setter for Id property

    \Input uId - id

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
void RemoteUser::Id::set(uint32 uId)
{
    m_Id = uId;
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    IsAdult::get

    \Description
        Getter for IsAdult property

    \Output
        bool  - true or false

    \Notes
        MS confirmed that this property is not used by the game chat sub-system.
        So we always return true.

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
bool RemoteUser::IsAdult::get()
{
    return(true);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    IsGuest::get

    \Description
        Getter for IsGuest property

    \Output
        bool  - true or false

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
bool RemoteUser::IsGuest::get()
{
    return(m_IsGuest);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    IsGuest::set

    \Description
        Setter for IsGuest property

    \Input bIsGuest - TRUE if remote user is guest, FALSE otherwise

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
void RemoteUser::IsGuest::set(bool bIsGuest)
{
    m_IsGuest = bIsGuest;
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    IsSignedIn::get

    \Description
        Getter for IsSignedIn property

    \Output
        bool  - true or false

    \Notes
        MS confirmed that this property is not used by the game chat sub-system.
        So we always return true.

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
bool RemoteUser::IsSignedIn::get()
{
    return(true);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    Sponsor::get

    \Description
        Getter for Sponsor property

    \Output
        User ^ - ref to sponsoring user

    \Notes
        Required in August XDK

        $$TODO - Verify we can return nullptr here

    \Version 08/12/2013 (jbrookes)
*/
/********************************************************************************F*/
User ^ RemoteUser::Sponsor::get()
{
    return(nullptr);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    Location::get

    \Description
        Getter for Location property

    \Output
        UserLocation - location

    \Notes
        The RemoteUser class always has the Location property set to "Remote".

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
Windows::Xbox::System::UserLocation RemoteUser::Location::get()
{
    return(Windows::Xbox::System::UserLocation::Remote);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    Privileges::get

    \Description
        Getter for Privileges property

    \Output
        IVectorView<uint32> ^ - immutable collection of privileges

    \Notes
        MS confirmed that this property is not used by the game chat sub-system.
        So we always return nullptr.

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
IVectorView<uint32> ^RemoteUser::Privileges::get()
{
    return(nullptr);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    XboxUserHash::get

    \Description
        Getter for XboxUserHash property

    \Output
        String ^ - hashcode identifying the user

     \Notes
        MS confirmed that this property is not used by the game chat sub-system.
        So we always return nullptr.

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
String ^RemoteUser::XboxUserHash::get()
{
    return(nullptr);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    XboxUserId::get

    \Description
        Getter for XboxUserId property

    \Output
        String ^    - Xbox User ID

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
String ^RemoteUser::XboxUserId::get()
{
    return(m_XboxUserId);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    XboxUserId::set

    \Description
        Setter for XboxUserId property

    \Input ^refXboxUserId   - Xbox User ID

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
void RemoteUser::XboxUserId::set(String ^refXboxUserId)
{
    m_XboxUserId = refXboxUserId;
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    GetTokenAndSignatureAsync

    \Description
        Retrieves an authorization token and digital signature for an HTTP request by this user.

    \Input ^refMethod   - method
    \Input ^refUrl      - The URL for which to retrieve the authorization token and digital signature.
    \Input ^refHeaders  - The headers to be included in the HTTP request

    \Output
        IAsyncOperation<GetTokenAndSignatureResult ^> ^ - An interface for tracking the progress of the asynchronous call.
                                                          The result is an object indicating the token and the digital signature
                                                          of the entire request, including the token.

    \Notes
        This function is assumed to not be needed/used by the game chat sub-system.
        So we leave the implementation empty.

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
IAsyncOperation<GetTokenAndSignatureResult ^> ^ RemoteUser::GetTokenAndSignatureAsync(String ^refMethod, String ^refUrl, String ^refHeaders)
{
    return(nullptr);
}


/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    GetTokenAndSignatureAsync

    \Description
        Retrieves an authorization token and digital signature for an HTTP request by this user.

    \Input ^refMethod   - method
    \Input ^refUrl      - The URL for which to retrieve the authorization token and digital signature.
    \Input ^refHeaders  - The headers to be included in the HTTP request
    \Input ^refBody     - The body of the HTTP request

    \Output
        IAsyncOperation<GetTokenAndSignatureResult ^> ^ - An interface for tracking the progress of the asynchronous call.
                                                          The result is an object indicating the token and the digital signature
                                                          of the entire request, including the token.

    \Notes
        This function is assumed to not be needed/used by the game chat sub-system.
        So we leave the implementation empty.

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
IAsyncOperation<GetTokenAndSignatureResult ^> ^ RemoteUser::GetTokenAndSignatureAsync(String ^refMethod, String ^refUrl, String ^refHeaders, const Array<unsigned char, 1> ^refBody)
{
    return(nullptr);
}

/*F********************************************************************************/
/*!
    \relates     RemoteUser
    \Function    GetTokenAndSignatureAsync

    \Description
        Retrieves an authorization token and digital signature for an HTTP request by this user.

    \Input ^refMethod   - method
    \Input ^refUrl      - The URL for which to retrieve the authorization token and digital signature.
    \Input ^refHeaders  - The headers to be included in the HTTP request
    \Input ^refBody     - The body of the HTTP request

    \Output
        IAsyncOperation<GetTokenAndSignatureResult ^> ^ - An interface for tracking the progress of the asynchronous call.
                                                          The result is an object indicating the token and the digital signature
                                                          of the entire request, including the token.

    \Notes
        This function is assumed to not be needed/used by the game chat sub-system.
        So we leave the implementation empty.

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
IAsyncOperation<GetTokenAndSignatureResult ^> ^ RemoteUser::GetTokenAndSignatureAsync(String ^refMethod, String ^refUrl, String ^refHeaders, String ^refBody)
{
    return(nullptr);
}

/*F********************************************************************************/
/*!
    \relates     AudioDevInfo
    \Function    AudioDevInfo

    \Description
        Constructor

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
AudioDevInfo::AudioDevInfo()
{

};

/*F********************************************************************************/
/*!
    \relates     AudioDevInfo
    \Function    AudioDevInfo

    \Description
        Destructor

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
AudioDevInfo::~AudioDevInfo()
{

};

/*F********************************************************************************/
/*!
    \relates     AudioDevInfo
    \Function    DeviceCategory::get

    \Description
        Getter for DeviceCategory property

    \Output
        AudioDeviceCategory  - device category

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
Windows::Xbox::System::AudioDeviceCategory AudioDevInfo::DeviceCategory::get()
{
    return(m_DeviceCategory);
}

/*F********************************************************************************/
/*!
    \relates     AudioDevInfo
    \Function    DeviceCategory::set

    \Description
        Setter for DeviceCategory property

    \Input category - device category

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
void AudioDevInfo::DeviceCategory::set(Windows::Xbox::System::AudioDeviceCategory category)
{
    m_DeviceCategory = category;
}

/*F********************************************************************************/
/*!
    \relates     AudioDevInfo
    \Function    DeviceType::get

    \Description
        Getter for DeviceType property

    \Output
        DeviceType  - device type

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
Windows::Xbox::System::AudioDeviceType AudioDevInfo::DeviceType::get()
{
    return(m_DeviceType);
}

/*F********************************************************************************/
/*!
    \relates     AudioDevInfo
    \Function    DeviceCategory::set

    \Description
        Setter for DeviceType property

    \Input category - device category

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
void AudioDevInfo::DeviceType::set(Windows::Xbox::System::AudioDeviceType type)
{
    m_DeviceType = type;
}

/*F********************************************************************************/
/*!
    \relates     AudioDevInfo
    \Function    Id::get

    \Description
        Getter for Id property

    \Output
        String ^    - ID

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
String ^AudioDevInfo::Id::get()
{
    return(m_Id);
}

/*F********************************************************************************/
/*!
    \relates     AudioDevInfo
    \Function    Id::set

    \Description
        Setter for Id property

    \Input ^refId  - ID

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
void AudioDevInfo::Id::set(String ^refId)
{
    m_Id = refId;
}

/*F********************************************************************************/
/*!
    \relates     AudioDevInfo
    \Function    IsMicrophoneMuted::get

    \Description
        Getter for IsMicrophoneMuted property

    \Output
        bool  - true or false

    \Notes
        $todo - This function always returns false at the moment because we currently
        fail to properly maintain local devices muting state consistent with real value.
        MS sample code does the same.
        See implementation of ComStyleRemoteAudioDeviceInfo in 
        C:\Program Files (x86)\Microsoft Durango XDK\xdk\Extensions\Xbox GameChat API\8.0\SourceDist\Xbox.GameChat\ChatUserSerialization\ComStyle\AudioDeviceInfo.cpp

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
bool AudioDevInfo::IsMicrophoneMuted::get()
{
    return(false);
    //return(m_Muted);
}

/*F********************************************************************************/
/*!
    \relates     AudioDevInfo
    \Function    IsMicrophoneMuted::set

    \Description
        Setter for IsMicrophoneMuted property

    \Input bMuted - TRUE if remote audio device is muted, FALSE otherwise

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
void AudioDevInfo::IsMicrophoneMuted::set(bool bMuted)
{
    m_Muted = bMuted;
}

/*F********************************************************************************/
/*!
    \relates     AudioDevInfo
    \Function    IsKinect::get

    \Description
        Getter for IsKinect property

    \Output
        bool  - true or false

    \Version 02/28/2014 (mclouatre)
*/
/********************************************************************************F*/
bool AudioDevInfo::IsKinect::get()
{
    return(m_IsKinect);
}

/*F********************************************************************************/
/*!
    \relates     AudioDevInfo
    \Function    IsKinect::set

    \Description
        Setter for IsKinect property

    \Input bIsKinect - TRUE if remote audio device is kinect, FALSE otherwise

    \Version 02/28/2014 (mclouatre)
*/
/********************************************************************************F*/
void AudioDevInfo::IsKinect::set(bool bIsKinect)
{
    m_IsKinect = bIsKinect;
}

/*F********************************************************************************/
/*!
    \relates     AudioDevInfo
    \Function    Sharing::get

    \Description
        Getter for Sharing property

    \Output
        AudioDeviceSharing  - sharing value

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
Windows::Xbox::System::AudioDeviceSharing AudioDevInfo::Sharing::get()
{
    return(m_Sharing);
}

/*F********************************************************************************/
/*!
    \relates     AudioDevInfo
    \Function    Sharing::set

    \Description
        Setter for Sharing property

    \Input sharing - device sharing value

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
void AudioDevInfo::Sharing::set(Windows::Xbox::System::AudioDeviceSharing sharing)
{
    m_Sharing = sharing;
}
#endif // DOXYGEN_SHOULD_SKIP_THIS



/*F********************************************************************************/
/*!
    \Function VoipSerializeAudioDevice

    \Description
        Populate specified buffer with a serialized blob (ready for transmission over
        the network) of the local audio device.

    \Input ^refAudioDeviceInfo  - local device winrt ref
    \Input *pSerializedDevice   - [out] pointer to serialized device buffer

    \Version 07/12/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipSerializeAudioDevice(IAudioDeviceInfo ^refAudioDeviceInfo, VoipSerializedDeviceT *pSerializedDevice)
{
    memset(pSerializedDevice, 0, sizeof(VoipSerializedDeviceT));

    // serialize the DeviceCategory property of the current audio device
    pSerializedDevice->uDeviceCategory = (uint8_t)refAudioDeviceInfo->DeviceCategory;

    // serialize the Muted property of the current audio device
    if (refAudioDeviceInfo->IsMicrophoneMuted)
    {
        pSerializedDevice->uFlags |= VOIP_SERIALIZED_DEVICE_FLAG_MUTED;
    }

    // serialize the IsKinect property of the current audio device
    if (wcsstr(refAudioDeviceInfo->Id->Data(), L"postmec") != nullptr)
    {
        pSerializedDevice->uFlags |= VOIP_SERIALIZED_DEVICE_FLAG_KINECT;
    }

    // serialize the Sharing property of the current audio device
    pSerializedDevice->uSharing = (uint8_t)refAudioDeviceInfo->Sharing;

    // serialize the ID property of the current audio device
    pSerializedDevice->uDeviceId = _VoipMapDeviceId(refAudioDeviceInfo->Id);
}

/*F********************************************************************************/
/*!
    \Function VoipDeserializeAudioDevice

    \Description
        Initialized the RemoteAudioDevice object from the serialized remote audio device data blob.

    \Input *pSerializedDevice       - remote audio device pointer
    \Input uConsoleId               - id of the console to which the device belongs
    \Input ^refRemoteAudioDeviceInfo - [in, out] remote audio device winrt ref
    \Input *pDeviceId               - [out] to be filled with numerical device id associate with device

    \Version 07/12/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipDeserializeAudioDevice(VoipSerializedDeviceT *pSerializedDevice, uint32_t uConsoleId, AudioDevInfo ^refRemoteAudioDeviceInfo, uint8_t *pDeviceId)
{
    // de-serialize the DeviceCategory property of the current audio device
    refRemoteAudioDeviceInfo->DeviceCategory =  (Windows::Xbox::System::AudioDeviceCategory)(pSerializedDevice->uDeviceCategory);

    // de-serialize the DeviceType property of the current audio device
    refRemoteAudioDeviceInfo->DeviceType =  Windows::Xbox::System::AudioDeviceType::Capture;

    // de-serialize Muted property
    refRemoteAudioDeviceInfo->IsMicrophoneMuted = (pSerializedDevice->uFlags & VOIP_SERIALIZED_DEVICE_FLAG_MUTED) ? true : false;

    // de-serialize IsKinect property
    refRemoteAudioDeviceInfo->IsKinect = (pSerializedDevice->uFlags & VOIP_SERIALIZED_DEVICE_FLAG_KINECT) ? true : false;

    // de-serialize the Sharing property of the current audio device
    refRemoteAudioDeviceInfo->Sharing = (Windows::Xbox::System::AudioDeviceSharing)(pSerializedDevice->uSharing);

    // build audio device Id property
    refRemoteAudioDeviceInfo->Id = uConsoleId.ToString();
    refRemoteAudioDeviceInfo->Id += L"-";
    refRemoteAudioDeviceInfo->Id += pSerializedDevice->uDeviceId.ToString(); 
    if (refRemoteAudioDeviceInfo->IsKinect)
    {
        refRemoteAudioDeviceInfo->Id += L"-postmec";             // append postmec to id if device is kinect
    }

    // fill out param
    *pDeviceId = pSerializedDevice->uDeviceId;
}

/*F********************************************************************************/
/*!
    \Function VoipSerializeuser

    \Description
        Populate a VoipUserT with a serialized blob (ready for transmission over
        the network) 

    \Input ^refLocalUser    - local user winrt ref
    \Input *pRemoteUser     - [out] remote user pointer

    \Output
        int32_t             - size of serialized data

    \Version 07/12/2013 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipSerializeUser(IUser ^refLocalUser, VoipUserExT *pRemoteUser)
{
    uint32_t uIndex;
    int32_t iCharCount;
    uint8_t *pWrite = &pRemoteUser->aSerializedUserBlob[0];
    uint8_t *pEnd = pWrite + sizeof(pRemoteUser->aSerializedUserBlob);
    uint8_t *pCaptureSourceCount;
    uint32_t uConsoleId = NetConnMachineId();

    // serialize IsGuest property
    *pWrite++ = (uint8_t)refLocalUser->IsGuest;

    // serialize the XboxUserId property (convert wchar to char to save space)
    iCharCount = (int32_t) wcstombs((char *)pWrite, refLocalUser->XboxUserId->Data(), (sizeof(pRemoteUser->aSerializedUserBlob) - (pWrite - &pRemoteUser->aSerializedUserBlob[0])));
    pWrite += iCharCount;
    *pWrite++ = '\0'; // null terminate

    // serialize the console identifier
    *pWrite++  = (uint8_t)(uConsoleId >> 24);
    *pWrite++  = (uint8_t)(uConsoleId >> 16);
    *pWrite++  = (uint8_t)(uConsoleId >> 8);
    *pWrite++  = (uint8_t)uConsoleId;

    // save location of capture source count and initialize the count to 0
    pCaptureSourceCount = pWrite++;
    *pCaptureSourceCount = 0;

    // serialize the device info
    for (uIndex = 0; uIndex < refLocalUser->AudioDevices->Size; uIndex++)
    {
        // skip if no more room in buffer
        if ((pWrite + sizeof(VoipSerializedDeviceT)) >= pEnd)
        {
            NetPrintf(("voipserializationxboxone: failed to serialize device #%d", uIndex));
            continue;
        }

        IAudioDeviceInfo ^refAudioDeviceInfo = refLocalUser->AudioDevices->GetAt(uIndex);

        if (refAudioDeviceInfo->DeviceType == Windows::Xbox::System::AudioDeviceType::Capture)
        {
            VoipSerializeAudioDevice(refAudioDeviceInfo, (VoipSerializedDeviceT *)pWrite);
            pWrite += sizeof(VoipSerializedDeviceT);

            // increment number of encoded serialized audio devices
            *pCaptureSourceCount = *pCaptureSourceCount + 1;
        }
    }

    return(pWrite - &pRemoteUser->aSerializedUserBlob[0]);
}


/*F********************************************************************************/
/*!
    \Function VoipDeserializeUser

    \Description
        Initialized the RemoteUser object from the serialized remote user data blob.

    \Input *pRemoteUser     - remote user pointer
    \Input ^refRemoteUser   - [in, out] remote user winrt ref

    \Version 07/12/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipDeserializeUser(uint8 *pRemoteUser, RemoteUser ^refRemoteUser)
{
    int32_t iIndex, iCount;
    wchar_t wstrString[256];
    uint32_t uConsoleId;
    uint8 *pRead = pRemoteUser;

    // de-serialize IsGuest property
    if (*pRead++ != 0)
    {
        refRemoteUser->IsGuest = true;
    }
    else
    {
        refRemoteUser->IsGuest = false;
    }

    // de-serialize XboxUserId property
    memset(wstrString, 0, sizeof(wstrString));
    mbstowcs(wstrString, (char *)pRead, sizeof(wstrString)-1);
    refRemoteUser->XboxUserId = ref new String(wstrString);
    pRead += (strlen((char *)pRead) + 1);

    // de-serialize console identifier
    uConsoleId = *pRead << 24;
    pRead++;
    uConsoleId |= *pRead << 16;
    pRead++;
    uConsoleId |= *pRead << 8;
    pRead++;
    uConsoleId |= *pRead;
    pRead++;

    // de-serialize count of serialized AudioDevices
    iCount = *pRead++;

    // de-serialize each AudioDevice property
    Platform::Collections::Vector<IAudioDeviceInfo ^> ^ refAudioDevicesVector = ref new Platform::Collections::Vector<IAudioDeviceInfo ^>(iCount);
    for (iIndex = 0; iIndex < iCount; iIndex++)
    {
        uint8_t uDeviceId;
        AudioDevInfo ^refRemoteAudioDeviceInfo = ref new AudioDevInfo();

        VoipDeserializeAudioDevice((VoipSerializedDeviceT *)pRead, uConsoleId, refRemoteAudioDeviceInfo, &uDeviceId);
        pRead += sizeof(VoipSerializedDeviceT);

        refAudioDevicesVector->SetAt(iIndex, refRemoteAudioDeviceInfo);
    }

    refRemoteUser->AudioDevices = refAudioDevicesVector->GetView();
}

/*F********************************************************************************/
/*!
    \Function VoipUpdateSerializedUser

    \Description
        Update the remote user blob with a new capture source if necessary.

    \Input *pRemoteUser         - remote user pointer
    \Input *pSerializedDevice   - originating capture source (serialized blob)

    \Output
        uint32_t                - TRUE if remote user's collection of devices changed, FALSE otherwise

    \Version 07/12/2013 (mclouatre)
*/
/********************************************************************************F*/
uint32_t VoipUpdateSerializedUser(VoipUserExT *pRemoteUser, VoipSerializedDeviceT *pSerializedDevice)
{
    uint8_t *pWrite = &pRemoteUser->aSerializedUserBlob[0];;
    uint8_t *pAudioDeviceCount;
    int32_t iIndex;
    uint32_t bCollectionChanged = FALSE;

    // skip IsGuest property
    pWrite++;

    // skip XboxUserId property
    pWrite += (strlen((char *)pWrite) + 1);

    // skip console identifier
    pWrite += 4;

    // pWrite is now pointing to audio device count, save it, then jump over it
    pAudioDeviceCount = pWrite;
    pWrite++;

    // loop through all device and append new device at the end if not already in the set
    for (iIndex = 0; iIndex < *pAudioDeviceCount; iIndex++)
    {
        VoipSerializedDeviceT *pCurrent = (VoipSerializedDeviceT *)pWrite;

        // check if id match
        if (pCurrent->uDeviceId == pSerializedDevice->uDeviceId)
        {
            if (memcmp(pCurrent, pSerializedDevice, sizeof (*pCurrent)) != 0)
            {
                *pCurrent = *pSerializedDevice;  // update 
                bCollectionChanged = TRUE;
                NetPrintf(("voipserializationxboxone: capture source device %d was changed for remote user %s\n", pCurrent->uDeviceId, pRemoteUser->user.strUserId));
            }

            break;
        }

        pWrite += sizeof(VoipSerializedDeviceT);
    }

    if (iIndex == *pAudioDeviceCount)
    {
        VoipSerializedDeviceT *pCurrent = (VoipSerializedDeviceT *)pWrite;

        *pCurrent = *pSerializedDevice;  // append

        // increment the device count
        *pAudioDeviceCount = *pAudioDeviceCount + 1;
        bCollectionChanged = TRUE;
        NetPrintf(("voipserializationxboxone: capture source device %d was added for remote user %s\n", pCurrent->uDeviceId, pRemoteUser->user.strUserId));
    }

    return(bCollectionChanged);
}

/*F********************************************************************************/
/*!
    \Function VoipDumpUser

    \Description
        Display user data.

    \Input ^refUser - user winrt ref

    \Version 07/12/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipDumpUser(IUser ^refUser)
{
    uint32_t uIndex;

    NetPrintf(("voipserializationxboxone: dumping user info\n"));

    NetPrintf(("  ID = 0x%08x\n", refUser->Id));

    if (refUser->XboxUserId != nullptr)
    {
        NetPrintf(("  XboxUserId = %S\n", refUser->XboxUserId->Data()));
    }
    else
    {
        NetPrintf(("  XboxUserId = nullptr\n"));
    }

    if (refUser->XboxUserHash != nullptr)
    {
        NetPrintf(("  XboxUserHash = %S\n", refUser->XboxUserHash->Data()));
    }
    else
    {
        NetPrintf(("  XboxUserHash = nullptr\n"));
    }

    if (refUser->DisplayInfo != nullptr)
    {
        NetPrintf(("  GamerTag = %S\n", refUser->DisplayInfo->Gamertag->Data()));
    }
    else
    {
        NetPrintf(("  DisplayInfo = nullptr\n"));
    }

    NetPrintf(("  IsGuest = %s\n", (refUser->IsGuest?"true":"false")));

    NetPrintf(("  IsSignedIn = %s\n", (refUser->IsSignedIn?"true":"false")));

    NetPrintf(("  Location = %d\n", refUser->Location));

    if (refUser->AudioDevices != nullptr)
    {
        NetPrintf(("  %d AudioDevices :\n", refUser->AudioDevices->Size));
        for(uIndex = 0; uIndex < refUser->AudioDevices->Size; uIndex++)
        {
            IAudioDeviceInfo ^refAudioDeviceInfo = refUser->AudioDevices->GetAt(uIndex);
            NetPrintf(("    AudioDev%d - Category = %d\n", uIndex, refAudioDeviceInfo->DeviceCategory));
            NetPrintf(("    AudioDev%d - Type = %d\n", uIndex, refAudioDeviceInfo->DeviceType));
            NetPrintf(("    AudioDev%d - Sharing = %d\n", uIndex, refAudioDeviceInfo->Sharing));
            NetPrintf(("    AudioDev%d - Id = %S\n", uIndex, refAudioDeviceInfo->Id->Data()));
            NetPrintf(("    AudioDev%d - Muted = %s\n", uIndex, (refAudioDeviceInfo->IsMicrophoneMuted?"true":"false")));
        }
    }
    else
    {
        NetPrintf(("  AudioDevices = nullptr\n"));
    }
}

void VoipSerializeInit()
{
    g_refDeviceIdMap = ref new Platform::Collections::Map<String ^, int>();
}

void VoipSerializeTerm()
{
    g_refDeviceIdMap = nullptr;
}
