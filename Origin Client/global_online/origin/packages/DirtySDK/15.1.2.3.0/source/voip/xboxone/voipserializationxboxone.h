/*H********************************************************************************/
/*!
    \File voipserializationtxboxone.h

    \Description
        XBoxOne user and device serialization/deserialization code.

    \Copyright
        Copyright (c) Electronic Arts 2014. ALL RIGHTS RESERVED.

    \Version 03/04/2014 (mclouatre) First Version
*/
/********************************************************************************H*/

#ifndef _voipserializationxboxone_h
#define _voipserializationxboxone_h

/*** Include files ****************************************************************/
#include "voippriv.h"

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Xbox::Input;
using namespace Windows::Xbox::System;

using namespace Microsoft::Xbox::Services;


/*** Defines **********************************************************************/

#define VOIP_SERIALIZED_DEVICE_FLAG_KINECT   (1)
#define VOIP_SERIALIZED_DEVICE_FLAG_MUTED    (2)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! VoipSerializedDeviceT
typedef struct VoipSerializedDeviceT
{
    uint8_t uFlags;
    uint8_t uDeviceCategory;
    uint8_t uSharing;
    uint8_t uDeviceId;
} VoipSerializedDeviceT;

/*F********************************************************************************/
/*!
    \class     RemoteUser

    \Description
        winrt class required for feeding remote users into the game chat system

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
ref class RemoteUser sealed : IUser
{
public:
    RemoteUser(uint32_t uUserId);
    virtual ~RemoteUser();

    property IVectorView<IAudioDeviceInfo ^> ^AudioDevices {
           virtual IVectorView<IAudioDeviceInfo ^> ^get();
           void set(IVectorView<IAudioDeviceInfo ^> ^);
        }

    // not needed by the voip subsystem, so no need to provide a setter method
    property IVectorView<IController ^> ^Controllers {
           virtual IVectorView<IController ^> ^get();
        }

    // not needed by the voip subsystem, so no need to provide a setter method
    property UserDisplayInfo ^DisplayInfo {
           virtual UserDisplayInfo ^get();
        };

    property uint32 Id {
           virtual uint32 get();
           void set(uint32);
        };

    // not needed by the voip subsystem, so no need to provide a setter method
    property bool IsAdult {
           virtual bool get();
        };

    property bool IsGuest {
           virtual bool get();
           void set(bool);
        };

    // remote users will always be signed in, so no need to provide a setter method
    property bool IsSignedIn {
           virtual bool get();
        };

    property User ^ Sponsor {
        virtual User ^ get();
        };

    // remote users will always have Location = UserLocation::Remote, so no need to provide a setter method
    property Windows::Xbox::System::UserLocation Location {
           virtual Windows::Xbox::System::UserLocation get();
        };

    // not needed by the voip subsystem, so no need to provide a setter method
    property IVectorView<uint32> ^Privileges {
           virtual IVectorView<uint32> ^get();
        }

    // not needed by the voip subsystem, so no need to provide a setter method
    property String ^XboxUserHash {
           virtual String ^get();
        }

    property String ^XboxUserId {
           virtual String ^get();
           void set(String ^);
        }

    // needed for disabling and enabling voice playback for local users
    property uint32_t EnablePlaybackFlag {
        virtual uint32_t get();
        void set(uint32_t);
    }

    // needed for the bad rep auto muting feature
    property uint8_t IsFriend {
        virtual uint8_t get();
        void set(uint8_t);
    }

    // needed for the bad rep auto muting feature
    property uint8_t IsBadRepMute {
        virtual uint8_t get();
        void set(uint8_t);
    }

    // needed to keep track of the reputation op per remote user
    property IAsyncOperation<UserStatistics::UserStatisticsResult^> ^BadReqOp {
        virtual IAsyncOperation<UserStatistics::UserStatisticsResult^> ^get();
        void set(IAsyncOperation<UserStatistics::UserStatisticsResult^> ^);
    }

    // needed to timneout the bad rep check async op
    property uint32_t BadRepTimer {
        virtual uint32_t get();
        void set(uint32_t);
    }

    virtual IAsyncOperation<GetTokenAndSignatureResult ^> ^GetTokenAndSignatureAsync(String ^, String ^, String ^);
    virtual IAsyncOperation<GetTokenAndSignatureResult ^> ^GetTokenAndSignatureAsync(String ^, String ^, String ^, const Array<unsigned char, 1> ^);
    virtual IAsyncOperation<GetTokenAndSignatureResult ^> ^GetTokenAndSignatureAsync(String ^, String ^, String ^, String ^);

private:
    IVectorView<IAudioDeviceInfo ^> ^ m_AudioDevices;
    IAsyncOperation<UserStatistics::UserStatisticsResult^> ^ m_refBadReqOp;
    uint32_t m_uBadRepTimer;
    String ^ m_XboxUserId;
    uint32 m_Id;
    uint32_t m_uEnablePlayback; //!< bit field use to disable or enable playback of voice for a specific remote user (1 local user per bit)
    uint8_t m_IsFriend;
    uint8_t m_IsBadRepMute;
    bool m_IsGuest;
};

/*F********************************************************************************/
/*!
    \class     AudioDevInfo

    \Description
        winrt class required for:
            1- caching audio device info (both capture and render) of local users (to avoid accessing system api which blocks for long)
            2- feeding remote users' capture audio devices into the game chat system

    \Version 07/11/2013 (mclouatre)
*/
/********************************************************************************F*/
ref class AudioDevInfo sealed : IAudioDeviceInfo
{
public:
    AudioDevInfo();
    virtual ~AudioDevInfo();

    property AudioDeviceCategory DeviceCategory {
           virtual AudioDeviceCategory get();
           void set(AudioDeviceCategory);
        };

    property AudioDeviceType DeviceType {
           virtual AudioDeviceType get();
           void set(AudioDeviceType);
        };

    property String ^Id {
           virtual String ^get();
           void set(String ^);
        }

    property bool IsMicrophoneMuted {
           virtual bool get();
           void set(bool);
        };

    property bool IsKinect {
           virtual bool get();
           void set(bool);
        };

    property AudioDeviceSharing Sharing {
           virtual AudioDeviceSharing get();
           void set(AudioDeviceSharing);
        };

private:
    Windows::Xbox::System::AudioDeviceCategory m_DeviceCategory;
    Windows::Xbox::System::AudioDeviceType m_DeviceType;
    String ^ m_Id;
    bool m_Muted;
    bool m_IsKinect;
    Windows::Xbox::System::AudioDeviceSharing m_Sharing;
};


/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

void VoipSerializeInit();

void VoipSerializeTerm();

void VoipSerializeAudioDevice(Windows::Xbox::System::IAudioDeviceInfo ^refAudioDeviceInfo, VoipSerializedDeviceT *pBuffer);

void VoipDeserializeAudioDevice(VoipSerializedDeviceT *pSerializedDevice, uint32_t uConsoleId, AudioDevInfo ^refRemoteAudioDeviceInfo, uint8_t *pDeviceId);

int32_t VoipSerializeUser(IUser ^refLocalUser, VoipUserExT *pRemoteUser);

void VoipDeserializeUser(uint8 *pRemoteUser, RemoteUser ^refRemoteUser);

void VoipDumpUser(IUser ^refUser);

uint32_t VoipUpdateSerializedUser(VoipUserExT *pRemoteUser, VoipSerializedDeviceT *pSerializedDevice);

#endif // _voipserialization_h
