/*H********************************************************************************/
/*!
    \File voipheadsetxboxone.cpp

    \Description
        VoIP headset manager. Wraps Xbox One Game Chat api.

    \Copyright
        Copyright (c) Electronic Arts 2013. ALL RIGHTS RESERVED.

    \Version 05/24/2013 (mclouatre) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

// system includes
#include <collection.h>
#include <wrl.h>        // required for robuffer.h
#include <robuffer.h>   // required for byte access to IBuffers
#include <xdk.h>        // _XDK_VER

// dirtysock includes
#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/voip/voipdef.h"

// voip includes
#include "voippriv.h"
#include "voippacket.h"
#include "voipheadset.h"
#include "voipserializationxboxone.h"

using namespace Microsoft::WRL;
using namespace Windows::Storage::Streams;
using namespace Windows::Xbox::Chat;

/*** Defines **********************************************************************/
#define VOIPHEADSET_DEBUG                               (DIRTYCODE_DEBUG && FALSE)
#define VOIPHEADSET_GAMECHAT_DEFAULT_ENCODING_QUALITY   (EncodingQuality::Normal)
#define VOIPHEADSET_INBOUND_QUEUE_MAX_CAPACITY          (16)  // max number of sub-packets in an inbound queue
#define VOIPHEADSET_INBOUND_QUEUE_LOW_WATERMARK         (4)   // number of sub-packets required in an inbound queue before we start submitting the data (homemade pre-buffering)


/*** Macros ***********************************************************************/


/*** Type Definitions *************************************************************/

/*
For the xboxone to be able to apply the most restrictive user-specific privileges when kinect/tv combo is used as a "shared voip device", all local users
need to be added to the chat session as soon as they exist on the console... regardless of those users being "participating users" or not. A user that is not
participating is however added to the chat session as a "bystander".

In the local user data space defined below, bystander users have a valid refChatParticipant field but a NULL user field. Registered users have both a valid
refChatParticipant field and user field. When a registered user is promoted to the "participating" state, the corresponding chat participant of type
"ChatParticipantTypes::Bystander" is replaced with a chat participant of type "ChatParticipantTypes::Listener|ChatParticipantTypes::Talker".

pHeadsetRef->aLocalUsers[] is internally populated with bystanders in _VoipHeadsetUpdateBystanders(), and it is externally populated with registered users
when VoipSetLocalUser() is called.
*/

//!< local user data space
typedef struct LocalVoipUserT
{
    ChatParticipant ^refChatParticipant;                                    // used with both registered users and bystanders

    VoipUserT user;                                                         // used only with registered users

    /*
    we need to maintain this cache of audio devs for local users because accessing the real
    collection via refLocalUser->AudioDevices proved to be consuming several ms.. and we may be
    polling this at each frame
    */
    //! collection of devices associated with this user
    Platform::Collections::Vector<AudioDevInfo ^> ^refAudioDevsVector;      // used only with registered users
} LocalVoipUserT;

//!< remote shared user
typedef struct RemoteSharedUserT
{
    VoipUserT user;
    String^ CaptureSoureId;
    uint32_t uEnablePlaybackFlag;
} RemoteSharedUserT;

//! voipheadset module state data
struct VoipHeadsetRefT
{
    //! critical section
    NetCritT crit;

    //! headset status
    uint32_t uLastHeadsetStatus; //!< bit field capturing last reported headset status (on/off) for each local player

    //! kinect last capture tick
    uint32_t uKinectLastCapture;

    //! channel info
    int32_t iMaxChannels;

    //! user callback data
    VoipHeadsetMicDataCbT *pMicDataCb;
    VoipHeadsetStatusCbT *pStatusCb;
    void *pCbUserData;

    //! speaker callback data
    VoipSpkrCallbackT *pSpkrDataCb;
    void *pSpkrCbUserData;

    ChatSession ^refChatSession;
    Windows::Foundation::EventRegistrationToken ChatSessionStateChangedEventToken;
    Windows::Foundation::EventRegistrationToken KinectAvailabilityChangedEventToken;
    Windows::Foundation::EventRegistrationToken AudioDeviceAddedToken;
    Windows::Foundation::EventRegistrationToken AudioDeviceChangedToken;
    Windows::Foundation::EventRegistrationToken AudioDeviceRemovedToken;
    Platform::Collections::Map<String ^, IChatEncoder ^> ^refEncodersMap;   //!< one encoder per local capture source (capture source id/encoder pairs)
    Platform::Collections::Map<String ^, IChatDecoder ^> ^refDecodersMap;   //!< one decoder per local or remote capture source (capture source id/decoder pairs)
    Platform::Collections::Map<String ^, Object ^> ^refInboundQueuesMap;    //!< one inbound queue per local or remote capture source (capture source id/inbound queue pairs)  $$TODO consider a single map for refInboundQueuesMap and refPreBufStateMap
    Platform::Collections::Map<String ^, bool> ^refPreBufStateMap;          //!< one pre-buffering state per local or remote capture source (capture source id/state pairs)
    IChatSessionState ^refCurrentChatSessionState;
    IChatSessionState ^refNewChatSessionState;
    IAsyncOperation<IChatSessionState ^> ^refAsyncOp;
    IBuffer ^refDecodedBuffer;

    //! encoding / decoding
    Windows::Xbox::Chat::EncodingQuality EncodingQual;

    uint32_t uLastBystanderUpdate;

    #if DIRTYCODE_LOGGING
    int32_t iDebugLevel;            //!< module debuglevel
    uint32_t uLastTick;
    #endif

    LocalVoipUserT aLocalUsers[VOIP_MAXLOCALUSERS];
    RemoteSharedUserT aRemoteSharedUsers[VOIP_MAX_LOW_LEVEL_CONNS];

    uint8_t uSendSeq;
    uint8_t bLoopback;
    uint8_t bHeadsetChanged;
    uint8_t bSessionStateChanged;
    uint8_t bSessionStateRefreshInProgress;
    uint8_t bAsyncOpInProgress;
    uint8_t bIsKinectAvailable;
    uint8_t bKinectAvailabilityChanged;
    uint8_t bKinectIsCapturing;
    uint8_t bMicFocus;
    uint8_t _pad[2];
};


/*** Function Prototypes **********************************************************/

static void _VoipHeadsetUpdateStatusEx(VoipHeadsetRefT *pHeadset, bool bTriggerCallback);

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function   _VoipHeadsetDumpChatParticipants

    \Description
        Dump all participants currently registered with the default channel.

    \Input *pHeadset        - module reference
    \Version 11/06/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetDumpChatParticipants(VoipHeadsetRefT *pHeadset)
{
    uint32_t uParticipantIndex;

    try
    {
        Windows::Foundation::Collections::IVectorView<IChatParticipant ^> ^refChatParticipantImmutableVector;
        refChatParticipantImmutableVector = pHeadset->refChatSession->Channels->First()->Current->Participants->GetView();

        NetPrintf(("voipheadsetxboxone: dumping chat participant(s)    (total nb of participants = %d)\n", refChatParticipantImmutableVector->Size));

        for (uParticipantIndex = 0; uParticipantIndex < refChatParticipantImmutableVector->Size; uParticipantIndex++)
        {
            NetPrintf(("   chat participant #%02d ==> %S  (%s - %s)\n", uParticipantIndex, 
                refChatParticipantImmutableVector->GetAt(uParticipantIndex)->User->XboxUserId->ToString()->Data(),
                (refChatParticipantImmutableVector->GetAt(uParticipantIndex)->ParticipantType == Windows::Xbox::Chat::ChatParticipantTypes::Bystander ? "bystander    " : "participating"),
                (refChatParticipantImmutableVector->GetAt(uParticipantIndex)->User->Location == Windows::Xbox::System::UserLocation::Local ? "local" : "remote")));
        }
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to dump session participants (%S/0x%08x)\n", e->Message->Data(), e->HResult));
    }
 }
#endif

/*F********************************************************************************/
/*!
    \Function   _VoipHeadsetChatSessionStateChangedHandler

    \Description
        Event handler (delegate) registered for session state changed events.

    \Input *pHeadset        - module reference
    \Input ^refChatSession  - The session the event applies to.

    \Version 05/31/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetChatSessionStateChangedHandler(VoipHeadsetRefT *pHeadset, IChatSession ^ refChatSession)
{
    pHeadset->bSessionStateChanged = TRUE;

    NetPrintf(("voipheadsetxboxone: game chat session state changed event notified - updating internal session state snapshot\n"));
}

/*F********************************************************************************/
/*!
    \Function   _VoipHeadsetKinectAvailabilityChangedHandler

    \Description
        Event handler (delegate) registered for kinect availability changed events.

    \Input *pHeadset    - module reference
    \Input bIsAvailable - TRUE if kinect is available, FALSE otherwise

    \Version 05/31/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetKinectAvailabilityChangedHandler(VoipHeadsetRefT *pHeadset, uint8_t bIsAvailable)
{
    pHeadset->bKinectAvailabilityChanged = TRUE; 
    pHeadset->bIsKinectAvailable = bIsAvailable;

    NetPrintf(("voipheadsetxboxone: kinect availability changed event notified - kinect is %s\n", (bIsAvailable?"available":"not available")));
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetStartChatSessionRefresh

    \Description
        Initiates query of new game chat session state. Lamdda expression
        use to define delegate that deals with completion.

    \Input *pHeadset    - module reference

    \Output
        int32_t         - negative=error, zero=success

    \Version 05/31/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetStartChatSessionRefresh(VoipHeadsetRefT *pHeadset)
{
    if (pHeadset->bAsyncOpInProgress == FALSE)
    {
        pHeadset->bAsyncOpInProgress = TRUE;

        // release previous operation ref if necessary
        if (pHeadset->refAsyncOp != nullptr)
        {
            delete pHeadset->refAsyncOp;
            pHeadset->refAsyncOp = nullptr;
        }

        try
        {
            pHeadset->refAsyncOp = pHeadset->refChatSession->GetStateAsync();
        }
        catch (Exception ^ e)
        {
            NetPrintf(("voipheadsetxboxone: exception caught while trying to initiate async get session state (%S/0x%08x)\n", e->Message->Data(), e->HResult));
            return(-1);
        }

        pHeadset->refNewChatSessionState = nullptr;

        pHeadset->refAsyncOp->Completed = ref new AsyncOperationCompletedHandler<IChatSessionState ^>(
            [pHeadset](IAsyncOperation<IChatSessionState ^>^ refAsyncOp, Windows::Foundation::AsyncStatus asyncStatus)
        {
            switch (asyncStatus)
            {
            case AsyncStatus::Completed:
                try
                {
                    pHeadset->refNewChatSessionState = refAsyncOp->GetResults();
                    NetPrintf(( "voipheadsetxboxone: async session state update succeeded\n"));
                }
                catch (Exception ^ e)
                {
                    NetPrintf(("voipheadsetxboxone: async get session state threw an exception (%S/0x%08x)\n", e->Message->Data(), e->HResult));
                }
                break;
            case AsyncStatus::Error:
                NetPrintf(( "voipheadsetxboxone: async session state update failed ErrorCode=%S (0x%08x)\n", refAsyncOp->ErrorCode.ToString()->Data(), refAsyncOp->ErrorCode.Value));
                break;
            case AsyncStatus::Canceled:
                NetPrintf(( "voipheadsetxboxone: async session state update canceled\n"));
                break;
            default:
                NetPrintf(( "voipheadsetxboxone: unknown AsyncStatus code (%d) when updating session state\n", asyncStatus));
                break;
            }

            pHeadset->bAsyncOpInProgress = FALSE;
        });
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetRegisterSessionStateChangedHandler

    \Description
        Register the session state changed handler.

    \Input *pHeadset    - module reference

    \Output
        int32_t         - negative=error, zero=success

    \Version 05/31/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetRegisterSessionStateChangedHandler(VoipHeadsetRefT *pHeadset)
{
    int32_t iRetCode = 0; // default to success

    try
    {
        pHeadset->ChatSessionStateChangedEventToken = pHeadset->refChatSession->StateChangedEvent += ref new ChatSessionStateChangedHandler(
            [ pHeadset ](IChatSession^ chatSession, Windows::Xbox::Chat::ChatSessionStateChangeReason eChatChangeReason)
        {
            _VoipHeadsetChatSessionStateChangedHandler(pHeadset, chatSession);
        });

        NetPrintf(("voipheadsetxboxone: successfully registered the game chat session state changed handler\n"));
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to register for game chat session state changed events (err=%s: %S)\n", DirtyErrGetName(e->HResult), e->Message->Data()));
        iRetCode = -1;
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetUnregisterSessionStateChangedHandler

    \Description
        Unregister the session state changed handler.

    \Input *pHeadset    - module reference

    \Output
        int32_t         - negative=error, zero=success

    \Version 07/22/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetUnregisterSessionStateChangedHandler(VoipHeadsetRefT *pHeadset)
{
    int32_t iRetCode = 0; // default to success

    try
    {
        pHeadset->refChatSession->StateChangedEvent -= pHeadset->ChatSessionStateChangedEventToken;
        NetPrintf(("voipheadsetxboxone: successfully unregistered the game chat session state changed handler\n"));
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to unregister for game chat session state changed events (err=%s: %S)\n", DirtyErrGetName(e->HResult), e->Message->Data()));
        iRetCode = -1;
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetRegisterKinectAvailabilityChangedHandler

    \Description
        Register the kinect availability change handler.

    \Input *pHeadset    - module reference

    \Output
        int32_t         - negative=error, zero=success

    \Version 05/31/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetRegisterKinectAvailabilityChangedHandler(VoipHeadsetRefT *pHeadset)
{
    int32_t iRetCode = 0; // default to success

    try
    {
        Windows::Kinect::KinectSensor ^refKinect = Windows::Kinect::KinectSensor::GetDefault();

        pHeadset->KinectAvailabilityChangedEventToken = refKinect->IsAvailableChanged += ref new TypedEventHandler<Windows::Kinect::KinectSensor ^, Windows::Kinect::IsAvailableChangedEventArgs ^> (
            [ pHeadset ](Windows::Kinect::KinectSensor ^refKinect, Windows::Kinect::IsAvailableChangedEventArgs ^refArgs)
        {
            _VoipHeadsetKinectAvailabilityChangedHandler(pHeadset, (refArgs->IsAvailable?TRUE:FALSE));
        });

        refKinect->Open();

#if _XDK_VER < 10951   // anything prior to May 2014 XDK
        // refKinect->IsAvailable is not reliable at this point
        // implemented workaround suggested by Microsoft here: https://forums.xboxlive.com/AnswerPage.aspx?qid=1d8bbe86-bdff-422a-bf4e-9dd6f2b10843&tgt=1
        Windows::Kinect::BodyFrameReader ^ refBodyFrameReader = refKinect->BodyFrameSource->OpenReader();
        for (int32_t iIndex = 0; iIndex < 100; iIndex++)
        {
            Windows::Kinect::BodyFrame ^ refBodyFrame = refBodyFrameReader->AcquireLatestFrame();
            if (iIndex > 50 && (refBodyFrame != nullptr))
            {
                 NetPrintf(("voipheadsetxboxone: successfully detected kinect as available (iIndex = %d)\n", iIndex));
                pHeadset->bIsKinectAvailable = TRUE;
                break;
            }

            NetConnSleep(10);
        }
#else
        pHeadset->bIsKinectAvailable = refKinect->IsAvailable;
#endif

        NetPrintf(("voipheadsetxboxone: successfully registered the kinect availability changed handler - kinect is %s\n",
            (pHeadset->bIsKinectAvailable?"available":"not available")));
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to register for kinect availability changed events (err=%s: %S)\n", DirtyErrGetName(e->HResult), e->Message->Data()));
        iRetCode = -1;
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetUnregisterKinectAvailabilityChangedHandler

    \Description
        Unregister the kinect availability change handler.

    \Input *pHeadset    - module reference

    \Output
        int32_t         - negative=error, zero=success

    \Version 07/22/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetUnregisterKinectAvailabilityChangedHandler(VoipHeadsetRefT *pHeadset)
{
    int32_t iRetCode = 0; // default to success

    try
    {
        Windows::Kinect::KinectSensor ^refKinect = Windows::Kinect::KinectSensor::GetDefault();
        refKinect->IsAvailableChanged -= pHeadset->KinectAvailabilityChangedEventToken;
        refKinect->Close();
        NetPrintf(("voipheadsetxboxone: successfully unregistered the kinect availability handler\n"));
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to unregister for kinect availability changed events (err=%s: %S)\n", DirtyErrGetName(e->HResult), e->Message->Data()));
        iRetCode = -1;
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetCreateDefaultChatChannel

    \Description
        Create the default game chat channel used by the game chat session.

    \Input *pHeadset    - module reference

    \Output
        int32_t         - negative=error, zero=success

    \Version 07/26/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetCreateDefaultChatChannel(VoipHeadsetRefT *pHeadset)
{
    int32_t iRetCode = 0; // default to success

    try
    {
        pHeadset->refChatSession->Channels->Append(ref new ChatChannel());
        NetPrintf(("voipheadsetxboxone: successfully created default chat channel\n"));
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to create default chat channel err=%s: %S)\n",
            DirtyErrGetName(e->HResult), e->Message->Data()));
        iRetCode = -1;
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetDestroyDefaultChatChannel

    \Description
        Destroy the default game chat channel used by the game chat session.

    \Input *pHeadset    - module reference

    \Output
        int32_t         - negative=error, zero=success

    \Version 07/26/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetDestroyDefaultChatChannel(VoipHeadsetRefT *pHeadset)
{
    int32_t iRetCode = 0;

    try
    {
        pHeadset->refChatSession->Channels->Clear();
        NetPrintf(("voipheadsetxboxone: successfully destroyed the default chat channel\n"));
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to clear the channel vector (err=%s: %S)\n", DirtyErrGetName(e->HResult), e->Message->Data()));
        iRetCode = -1;
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetCreateChatSession

    \Description
        Create the game chat session.

    \Input *pHeadset    - module reference

    \Output
        int32_t         - negative=error, zero=success

    \Version 05/31/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetCreateChatSession(VoipHeadsetRefT *pHeadset)
{
    int32_t iRetCode = 0; // default to success

    try
    {
        /*
        From MS Doc: "The recommendation is to create one ChatSession object per application life cycle, and to
        set the period of the session to one of 20ms, 40ms or 80ms. These three periods are the only ones
        currently supported by the Durango XDK. If too small a period is requested, the E_CHAT_PERIOD_TOO_SMALL
        error will be returned."
        */
        TimeSpan timeSpan;          // A time period expressed in 100-nanosecond units. 
        timeSpan.Duration = 200000; // 20 ms 
        pHeadset->refChatSession = ref new ChatSession(timeSpan, Windows::Xbox::Chat::ChatFeatures::VoiceActivityDetector);
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to create chat session (err=%s: %S)\n", DirtyErrGetName(e->HResult), e->Message->Data()));
        pHeadset->refChatSession = nullptr;
        iRetCode = -1;
    }

    // if session creation succeeded, create the default chat channel used with this session
    if (iRetCode == 0)
    {
        iRetCode = _VoipHeadsetCreateDefaultChatChannel(pHeadset);

        pHeadset->refDecodedBuffer = ref new Buffer(5 * 1024);
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetDestroyChatSession

    \Description
        Destroy the game chat session.

    \Input *pHeadset    - module reference

    \Version 05/31/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetDestroyChatSession(VoipHeadsetRefT *pHeadset)
{
    if (pHeadset->refDecodedBuffer != nullptr)
    {
        delete pHeadset->refDecodedBuffer;
        pHeadset->refDecodedBuffer = nullptr;
    }

    _VoipHeadsetDestroyDefaultChatChannel(pHeadset);

    delete pHeadset->refChatSession;
    pHeadset->refChatSession = nullptr;
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetCreateChatEncoder

    \Description
        Create a game chat encoder for the specified local capture source

    \Input *pHeadset                - module reference
    \Input ^refChatCaptureSourceId  - id of local capture source the encoder is associated with
    \Input ^refFormat               - the format for encoding data

    \Output
        int32_t                     - negative=error, zero=success

    \Version 05/31/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetCreateChatEncoder(VoipHeadsetRefT *pHeadset, String ^refChatCaptureSourceId, IFormat ^refFormat)
{
    int32_t iRetCode = 0; // default to success

    try
    {
        ChatEncoder ^refChatEncoder = ref new ChatEncoder(refFormat, pHeadset->EncodingQual);
        pHeadset->refEncodersMap->Insert(refChatCaptureSourceId, refChatEncoder);
        NetPrintf(("voipheadsetxboxone: successfully created chat encoder for capture source %S\n", refChatCaptureSourceId->Data()));
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to create chat encoder (err=%s: %S)\n",
            DirtyErrGetName(e->HResult), e->Message->Data()));
        iRetCode = -1;
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetDestroyChatEncoder

    \Description
        Destroy the game chat encoder associated with the specified capture source

    \Input *pHeadset                - module reference
    \Input ^refChatCaptureSourceId  - id of local capture source the encoder is associated with

    \Version 05/31/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetDestroyChatEncoder(VoipHeadsetRefT *pHeadset, String ^refChatCaptureSourceId)
{
    try
    {
        pHeadset->refEncodersMap->Remove(refChatCaptureSourceId);
        NetPrintf(("voipheadsetxboxone: successfully destroyed chat encoder for capture source %S\n", refChatCaptureSourceId->Data()));
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to remove chat encoder (err=%s: %S)\n",
            DirtyErrGetName(e->HResult), e->Message->Data()));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetCreateChatDecoder

    \Description
        Create a game chat decoder for the specified originating capture source
        (which can be local or remote)

    \Input *pHeadset                - module reference
    \Input ^refChatCaptureSourceId  - id of local/remote capture source the decoder is associated with

    \Output
        int32_t                     - negative=error, zero=success

    \Version 05/31/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetCreateChatDecoder(VoipHeadsetRefT *pHeadset, String ^refChatCaptureSourceId)
{
    int32_t iRetCode = 0; // default to success

    try
    {
        ChatDecoder ^refChatDecoder = ref new ChatDecoder();
        pHeadset->refDecodersMap->Insert(refChatCaptureSourceId, refChatDecoder);
        NetPrintf(("voipheadsetxboxone: successfully created chat decoder for capture source %S\n", refChatCaptureSourceId->Data()));
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to create chat decoder (err=%s: %S)\n",
            DirtyErrGetName(e->HResult), e->Message->Data()));
        iRetCode = -1;
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetDestroyChatDecoder

    \Description
        Destroy the game chat decoder for the specified originating capture source
        (which can be local or remote)

    \Input *pHeadset                - module reference
    \Input ^refChatCaptureSourceId  - id of local/remote capture source the decoder is associated with

    \Version 05/31/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetDestroyChatDecoder(VoipHeadsetRefT *pHeadset, String ^refChatCaptureSourceId)
{
    try
    {
        pHeadset->refDecodersMap->Remove(refChatCaptureSourceId);
        NetPrintf(("voipheadsetxboxone: successfully destroyed chat decoder for capture source %S\n", refChatCaptureSourceId->Data()));
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to remove chat encoder(err=%s: %S)\n",
            DirtyErrGetName(e->HResult), e->Message->Data()));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetCreateInboundQueue

    \Description
        Create an inbound queue for the specified originating capture source
        (which can be local or remote)

    \Input *pHeadset                - module reference
    \Input ^refChatCaptureSourceId  - id of local/remote capture source the decoder is associated with

    \Output
        int32_t                     - negative=error, zero=success

    \Version 05/31/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetCreateInboundQueue(VoipHeadsetRefT *pHeadset, String ^refChatCaptureSourceId)
{
    int32_t iRetCode = 0; // default to success

    try
    {
        Platform::Collections::Vector<IBuffer ^> ^refInboundQueue = ref new Platform::Collections::Vector<IBuffer ^>(VOIPHEADSET_INBOUND_QUEUE_MAX_CAPACITY);
        refInboundQueue->Clear();
        pHeadset->refInboundQueuesMap->Insert(refChatCaptureSourceId, refInboundQueue);
        NetPrintf(("voipheadsetxboxone: successfully created inbound queue for capture source %S\n", refChatCaptureSourceId->Data()));
        pHeadset->refPreBufStateMap->Insert(refChatCaptureSourceId, true);  // signal that prebuffering is on
        NetPrintfVerbose((pHeadset->iDebugLevel, 2, "voipheadsetxboxone: pre-buffering for %S\n", refChatCaptureSourceId->Data()));
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to create inbound queue (err=%s: %S)\n",
            DirtyErrGetName(e->HResult), e->Message->Data()));
        iRetCode = -1;
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetDestroyInboundQueue

    \Description
        Destroy the inbound queue for the specified originating capture source
        (which can be local or remote)

    \Input *pHeadset                - module reference
    \Input ^refChatCaptureSourceId  - id of local/remote capture source the decoder is associated with

    \Version 05/31/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetDestroyInboundQueue(VoipHeadsetRefT *pHeadset, String ^refChatCaptureSourceId)
{
    try
    {
        pHeadset->refInboundQueuesMap->Remove(refChatCaptureSourceId);
        pHeadset->refPreBufStateMap->Remove(refChatCaptureSourceId); 
        NetPrintf(("voipheadsetxboxone: successfully destroyed inbound queue for capture source %S\n", refChatCaptureSourceId->Data()));
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to remove inbound queue (err=%s: %S)\n",
            refChatCaptureSourceId->Data(), DirtyErrGetName(e->HResult), e->Message->Data()));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetUpdateAudioDevicesForUser

    \Description
        Make sure we work with the updated vector of audio devices for the
        specified user

    \Input *pHeadset        - module reference
    \Input iLocalUserIndex  - local user index

    \Output                 - 0 for success; negative for failure
    \Version 03/11/2014 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetUpdateAudioDevicesForUser(VoipHeadsetRefT *pHeadset, int32_t iLocalUserIndex)
{
    int32_t iRetCode = 0;
    uint32_t uDeviceIndex;
    User ^refUser = nullptr;

    NetCritEnter(&pHeadset->crit);
    // find the system local user (winrt ref)
    if (NetConnStatus('xusr', iLocalUserIndex, &refUser, sizeof(refUser)) >= 0)
    {
        if (pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector != nullptr)
        {
            pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector->Clear();
            pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector = nullptr;
        }

        pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector = ref new Platform::Collections::Vector<AudioDevInfo ^>();
        for (uDeviceIndex = 0; uDeviceIndex < refUser->AudioDevices->Size; uDeviceIndex++)
        {
            uint8_t uDeviceId;
            VoipSerializedDeviceT SerializedDevice;
            AudioDevInfo ^refRemoteAudioDeviceInfo = ref new AudioDevInfo;
            VoipSerializeAudioDevice(refUser->AudioDevices->GetAt(uDeviceIndex), &SerializedDevice);
            VoipDeserializeAudioDevice(&SerializedDevice, NetConnMachineId(), refRemoteAudioDeviceInfo, &uDeviceId);
            // fill in some of the fields hardcoded during deserialization
            refRemoteAudioDeviceInfo->Id = refUser->AudioDevices->GetAt(uDeviceIndex)->Id;
            refRemoteAudioDeviceInfo->DeviceType = refUser->AudioDevices->GetAt(uDeviceIndex)->DeviceType;
            pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector->Append(refRemoteAudioDeviceInfo);
        }

        _VoipHeadsetUpdateStatusEx(pHeadset, true);
    }
    else
    {
        NetPrintf(("voipheadsetxboxone: _VoipHeadsetUpdateAudioDevicesForUser() - no local user at index %d\n", iLocalUserIndex));
        iRetCode = -1;
    }

    NetCritLeave(&pHeadset->crit);

    return (iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetUpdateAudioDevices

    \Description
        Make sure we work with the updated vector of audio devices for each local
        user

    \Input *pHeadset    - module reference

    \Version 03/11/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetUpdateAudioDevices(VoipHeadsetRefT *pHeadset)
{
    int32_t iIndex;
    
    // make sure there is no other local talker
    for (iIndex = 0; iIndex < VOIP_MAXLOCALUSERS; iIndex++)
    {
        if (!VOIP_NullUser(&pHeadset->aLocalUsers[iIndex].user))
        {
            _VoipHeadsetUpdateAudioDevicesForUser(pHeadset, iIndex);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetUpdateEncodersDecodersForLocalCaptureSources

    \Description
        Make sure we have an encoder and a decoder per local capture source.

    \Input *pHeadset    - module reference

    \Version 07/10/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetUpdateEncodersDecodersForLocalCaptureSources(VoipHeadsetRefT *pHeadset)
{
    uint8_t bFound, bNew;
    uint32_t uOldCaptureSourceIndex, uNewCaptureSourceIndex;

    // is this the very first pass?
    if (pHeadset->refCurrentChatSessionState == nullptr)
    {
        // create encoders/decoders associated with local capture sources that were newly added
        for (uNewCaptureSourceIndex = 0; uNewCaptureSourceIndex < pHeadset->refNewChatSessionState->CaptureSources->Size; uNewCaptureSourceIndex++)
        {
            IChatCaptureSource ^refNewChatCaptureSource = pHeadset->refNewChatSessionState->CaptureSources->GetAt(uNewCaptureSourceIndex);

            // capture source was newly added, create associated encoder and decoder
            _VoipHeadsetCreateChatEncoder(pHeadset, refNewChatCaptureSource->Id, refNewChatCaptureSource->Format);
            _VoipHeadsetCreateChatDecoder(pHeadset, refNewChatCaptureSource->Id);
        }
    }
    else
    {
        // delete encoders/decoders associated with local capture sources that no longer exist
        for (uOldCaptureSourceIndex = 0; uOldCaptureSourceIndex < pHeadset->refCurrentChatSessionState->CaptureSources->Size; uOldCaptureSourceIndex++)
        {
            IChatCaptureSource ^refOldChatCaptureSource = pHeadset->refCurrentChatSessionState->CaptureSources->GetAt(uOldCaptureSourceIndex);

            bFound = FALSE;

            for (uNewCaptureSourceIndex = 0; uNewCaptureSourceIndex < pHeadset->refNewChatSessionState->CaptureSources->Size; uNewCaptureSourceIndex++)
            {
                IChatCaptureSource ^refNewChatCaptureSource = pHeadset->refNewChatSessionState->CaptureSources->GetAt(uNewCaptureSourceIndex);

                if (refOldChatCaptureSource->Id == refNewChatCaptureSource->Id)
                {
                    bFound = TRUE;
                }
            }

            if (bFound == FALSE)
            {
                // capture source no longer exists, clean associated encoder and decoder
                _VoipHeadsetDestroyChatEncoder(pHeadset, refOldChatCaptureSource->Id);
                _VoipHeadsetDestroyChatDecoder(pHeadset, refOldChatCaptureSource->Id);
            }
        }

        // create encoders/decoders associated with local capture sources that were newly added
        for (uNewCaptureSourceIndex = 0; uNewCaptureSourceIndex < pHeadset->refNewChatSessionState->CaptureSources->Size; uNewCaptureSourceIndex++)
        {
            IChatCaptureSource ^refNewChatCaptureSource = pHeadset->refNewChatSessionState->CaptureSources->GetAt(uNewCaptureSourceIndex);

            if (pHeadset->refCurrentChatSessionState->CaptureSources->Size == 0)
            {
                bNew = TRUE;
            }
            else
            {
                bNew = FALSE;
                for (uOldCaptureSourceIndex = 0; uOldCaptureSourceIndex < pHeadset->refCurrentChatSessionState->CaptureSources->Size; uOldCaptureSourceIndex++)
                {
                    IChatCaptureSource ^refOldChatCaptureSource = pHeadset->refCurrentChatSessionState->CaptureSources->GetAt(uOldCaptureSourceIndex);

                    if (refNewChatCaptureSource->Id != refOldChatCaptureSource->Id)
                    {
                        bNew = TRUE;
                    }
                }
            }

            if (bNew == TRUE)
            {
                // capture source was newly added, create associated encoder and decoder
                _VoipHeadsetCreateChatEncoder(pHeadset, refNewChatCaptureSource->Id, refNewChatCaptureSource->Format);
                _VoipHeadsetCreateChatDecoder(pHeadset, refNewChatCaptureSource->Id);
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetUpdateStatus

    \Description
        Headset update function.

    \Input *pHeadset    - pointer to headset state

    \Version 05/24/2013 (mclouatre)
    \Version 05/24/2013 (amakoukji) Version for _VoipHeadsetUpdateStatusEx
*/
/********************************************************************************F*/
static void _VoipHeadsetUpdateStatus(VoipHeadsetRefT *pHeadset)
{
    _VoipHeadsetUpdateStatusEx(pHeadset, true);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetUpdateStatusEx

    \Description
        Headset update function.

    \Input *pHeadset         - pointer to headset state
    \Input  bTriggerCallback - flag whether to trigger callback 

    \Version 05/24/2013 (mclouatre)
    \Version 05/24/2013 (amakoukji) Changed to  _VoipHeadsetUpdateStatusEx
*/
/********************************************************************************F*/
static void _VoipHeadsetUpdateStatusEx(VoipHeadsetRefT *pHeadset, bool bTriggerCallback)
{
    int32_t iLocalUserIndex;

    NetCritEnter(&pHeadset->crit);

    for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; iLocalUserIndex++)
    {
        if (!VOIP_NullUser(&pHeadset->aLocalUsers[iLocalUserIndex].user))
        {
            uint8_t bHeadsetDetected = FALSE;
            uint32_t uUserHeadsetStatus = 0;
            uint32_t uLastUserHeadsetStatus = pHeadset->uLastHeadsetStatus & (1 << iLocalUserIndex);
            IAudioDeviceInfo ^refAudioDeviceInfo;

            // look for a capture audio device associated with this user
            if (pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector->Size > 0)
            {
                uint32_t uIndex;

                for (uIndex = 0; uIndex < pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector->Size; uIndex++)
                {
                    refAudioDeviceInfo = pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector->GetAt(uIndex);
                    if (refAudioDeviceInfo->DeviceType == Windows::Xbox::System::AudioDeviceType::Capture)
                    {
                        // ignore kinect because MS confirmed it is always present
                        if (wcsstr(refAudioDeviceInfo->Id->Data(), L"postmec") == NULL)
                        {
                            uUserHeadsetStatus = 1 << iLocalUserIndex;
                            bHeadsetDetected = TRUE;
                            break;
                        }
                    }
                    refAudioDeviceInfo = nullptr;
                }
            }

            // before concluding that there is no headset, check whether kinect is connected or not
            if (!bHeadsetDetected)
            {
                if (pHeadset->bIsKinectAvailable)
                {
                    uUserHeadsetStatus = 1 << iLocalUserIndex;
                    bHeadsetDetected = TRUE;
                }
            }

            if (uUserHeadsetStatus != uLastUserHeadsetStatus)
            {
                #if DIRTYCODE_LOGGING
                NetPrintf(("voipheadsetxboxone: headset %s for local user index %d\n", (bHeadsetDetected ? "inserted" : "removed"), iLocalUserIndex));
                if (bHeadsetDetected)
                {
                    if (refAudioDeviceInfo == nullptr)
                    {
                        Windows::Kinect::KinectSensor ^refKinect = Windows::Kinect::KinectSensor::GetDefault();
                        NetPrintf(("voipheadsetxboxone:   unique kinect ID -> %S\n", refKinect->UniqueKinectId->Data()));
                    }
                    else
                    {
                        NetPrintf(("voipheadsetxboxone:   audio device ID -> %S\n", refAudioDeviceInfo->Id->Data()));
                        NetPrintf(("voipheadsetxboxone:   audio device category: %S\n", refAudioDeviceInfo->DeviceCategory.ToString()->Data()));
                        NetPrintf(("voipheadsetxboxone:   audio device type: %S\n", refAudioDeviceInfo->DeviceType.ToString()->Data()));
                    }
                }
                #endif

                // trigger callback
                if (bTriggerCallback)
                {
                    pHeadset->pStatusCb(iLocalUserIndex, bHeadsetDetected, VOIP_HEADSET_STATUS_INOUT, pHeadset->pCbUserData);
                }

                // update last headset status
                pHeadset->uLastHeadsetStatus &= ~(1 << iLocalUserIndex);  // clear bit first
                pHeadset->uLastHeadsetStatus |= uUserHeadsetStatus;       // now set bit status if applicable
            }
        }
    }

    NetCritLeave(&pHeadset->crit);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetUpdateChatSessionState

    \Description
        Game chat session state update function.

    \Input *pHeadset    - pointer to headset state

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetUpdateChatSessionState(VoipHeadsetRefT *pHeadset)
{
    int32_t iResult;

    // did we get a session state changed event from the system?
    if (pHeadset->bSessionStateChanged)
    {
        // postpone if we are already in the middle of refreshing the session state?
        if (!pHeadset->bSessionStateRefreshInProgress)
        {
            NetPrintf(("voipheadsetxboxone: initiating a game chat session state refresh\n"));

            pHeadset->bSessionStateRefreshInProgress = TRUE;
            pHeadset->bSessionStateChanged = FALSE;
            if ((iResult = _VoipHeadsetStartChatSessionRefresh(pHeadset)) != 0)
            {
                pHeadset->bSessionStateRefreshInProgress = FALSE;
            }
        }
    }

    // is a refresh in progress?
    if (pHeadset->bSessionStateRefreshInProgress)
    {
        // is the refresh complete?
        if (pHeadset->bAsyncOpInProgress == FALSE)
        {
            // did the refresh succeed?
            if (pHeadset->refNewChatSessionState != nullptr)
            {
                // update encoders and decoders associated with local capture sources
                _VoipHeadsetUpdateEncodersDecodersForLocalCaptureSources(pHeadset);

                // refresh is complete, update current session state with new session state
                pHeadset->refCurrentChatSessionState = pHeadset->refNewChatSessionState;

                #if DIRTYCODE_LOGGING
                // display new set of render targets
                NetPrintf(("voipheadsetxboxone: new collection of local render targets is:\n"));
                for (uint32_t uRenderTargetIndex = 0; uRenderTargetIndex < pHeadset->refCurrentChatSessionState->RenderTargets->Size; uRenderTargetIndex++)
                {
                    IChatRenderTarget ^refChatRenderTarget = pHeadset->refCurrentChatSessionState->RenderTargets->GetAt(uRenderTargetIndex);
                    NetPrintf(("voipheadsetxboxone:    %S\n", refChatRenderTarget->Id->ToString()->Data()));
                }

                // display new set of capture sources
                NetPrintf(("voipheadsetxboxone: new collection of local capture sources is:\n"));
                for (uint32_t uCaptureSourceIndex = 0; uCaptureSourceIndex < pHeadset->refCurrentChatSessionState->CaptureSources->Size; uCaptureSourceIndex++)
                {
                    IChatCaptureSource ^refChatCaptureSource = pHeadset->refCurrentChatSessionState->CaptureSources->GetAt(uCaptureSourceIndex);
                    NetPrintf(("voipheadsetxboxone:    %S\n", refChatCaptureSource->Id->ToString()->Data()));
                }
                #endif
            }

            pHeadset->bSessionStateRefreshInProgress = FALSE;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetFindParticipant

    \Description
        Find the specified participant in the default chat channel.

    \Input *pHeadset            - pointer to headset state
    \Input ^refChatParticipant  - winrt ref for the participant
    \Input *pParticipantIndex   - [out] filled with participant index when function succeeds

    \Output
        int32_t                 - 1 if success, 0 if not found, negative if failure

    \Version 07/26/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetFindParticipant(VoipHeadsetRefT *pHeadset, IChatParticipant ^refChatParticipant, uint32_t *pParticipantIndex)
{
    uint32_t uParticipantIndex;

    try
    {
        Windows::Foundation::Collections::IVectorView<IChatParticipant ^> ^refChatParticipantImmutableVector;
        refChatParticipantImmutableVector = pHeadset->refChatSession->Channels->First()->Current->Participants->GetView();

        for (uParticipantIndex = 0; uParticipantIndex < refChatParticipantImmutableVector->Size; uParticipantIndex++)
        {
            IChatParticipant ^refTempChatParticipant = refChatParticipantImmutableVector->GetAt(uParticipantIndex);

            if (refTempChatParticipant->User->Id == refChatParticipant->User->Id)
            {
                *pParticipantIndex = uParticipantIndex;
                return(1);
            }
        }
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to find user in default channel (err=%s: %S)\n", 
            DirtyErrGetName(e->HResult), e->Message->Data()));
        return(-2);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetCleanParticipant

    \Description
        Clean up resources associated with the participant (local or remote) and
        remove it from the default chat channel

    \Input *pHeadset            - pointer to headset state
    \Input ^refChatParticipant  - winrt ref for the participant
    \Input bRemote              - TRUE if participant is remote, FALSE  if local

    \Version 07/26/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetCleanParticipant(VoipHeadsetRefT *pHeadset, IChatParticipant ^refChatParticipant, uint32_t bRemote)
{
    uint32_t uIndex;
    uint32_t uParticipantIndex;

    NetPrintf(("voipheadsetxboxone: cleaning talker %S\n", refChatParticipant->User->XboxUserId->Data()));

    // for remote users, destroy decoder and inbound queue associated with the user's capture source ids
    // (for local users, this is done in _VoipHeadsetUpdateEncodersDecodersForLocalCaptureSources())
    if (bRemote)
    {
        try
        {
            Windows::Foundation::Collections::IVectorView<IAudioDeviceInfo ^> ^refAudioDeviceImmutableVector;
            refAudioDeviceImmutableVector = refChatParticipant->User->AudioDevices;

            for (uIndex = 0; uIndex < refAudioDeviceImmutableVector->Size; uIndex++)
            {
                IAudioDeviceInfo ^refAudioDevice = refAudioDeviceImmutableVector->GetAt(uIndex);

                if (refAudioDevice->DeviceType == Windows::Xbox::System::AudioDeviceType::Capture)
                {
                    String ^refChatCaptureSourceId = refAudioDevice->Id;

                    NetPrintf(("voipheadsetxboxone: attempting to cleanup resources for capture source %S\n", refChatCaptureSourceId->Data()));

                    if (pHeadset->refDecodersMap->HasKey(refChatCaptureSourceId))
                    {
                        _VoipHeadsetDestroyChatDecoder(pHeadset, refChatCaptureSourceId);
                    }

                    // destroy inbound queue associated with the user's capture source id
                    if (pHeadset->refInboundQueuesMap->HasKey(refChatCaptureSourceId))
                    {
                        _VoipHeadsetDestroyInboundQueue(pHeadset, refChatCaptureSourceId);
                    }
                }
            }
        }
        catch (Exception ^ e)
        {
            NetPrintf(("voipheadsetxboxone: exception caught while trying to cleanup resources associated with talker %S (err=%s: %S)\n",
                refChatParticipant->User->XboxUserId->Data(), DirtyErrGetName(e->HResult), e->Message->Data()));
        }
    }

    if (_VoipHeadsetFindParticipant(pHeadset, refChatParticipant, &uParticipantIndex) == 1)
    {
        try
        {
            pHeadset->refChatSession->Channels->First()->Current->Participants->RemoveAt(uParticipantIndex);

            // when a user is removed from the session, the system does not notify the implicit session change, just fake it here
            pHeadset->bSessionStateChanged = TRUE;
        }
        catch (Exception ^ e)
        {
            NetPrintf(("voipheadsetxboxone: exception caught while trying to remove talker %S from default channel (err=%s: %S)\n",
                refChatParticipant->User->XboxUserId->Data(), DirtyErrGetName(e->HResult), e->Message->Data()));
        }
    }
    else
    {
        NetPrintf(("voipheadsetxboxone: can't delete talker %S because not found in default channel\n", refChatParticipant->User->XboxUserId->Data()));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetDeleteLocalTalker

    \Description
        Destroy the game chat participant for this local user and remove the user 
        from the aLocalUser array.

    \Input *pHeadset        - headset module
    \Input iLocalUserIndex  - local index
    \Input *pLocalUser      - local user

    \Output
        int32_t             - negative=error, zero=success

    \Version 01/22/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetDeleteLocalTalker(VoipHeadsetRefT *pHeadset, int32_t iLocalUserIndex, VoipUserT *pLocalUser)
{
    int32_t iRetCode = 0;

    if (pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant == nullptr)
    {
        NetPrintf(("voipheadsetxboxone: deleting local talker %s at local user index %d\n", pLocalUser->strUserId, iLocalUserIndex));

        NetCritEnter(&pHeadset->crit);
        try
        {
            pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector->Clear();
            pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector = nullptr;
        }
        catch (Exception ^ e)
        {
            NetPrintf(("voipheadsetxboxon exception caught while trying to remove local talker %s at local user index %d from default chat channel (err=%s: %S)\n",
                pLocalUser->strUserId, iLocalUserIndex, DirtyErrGetName(e->HResult), e->Message->Data()));
            iRetCode = -2;
        }
        VOIP_ClearUser(&pHeadset->aLocalUsers[iLocalUserIndex].user);
        NetCritLeave(&pHeadset->crit);
    }
    else
    {
        NetPrintf(("voipheadsetxboxone: cannot delete local talker %s at local user index %d because the user must first be deactivated\n", pHeadset->aLocalUsers[iLocalUserIndex].user.strUserId, iLocalUserIndex));
        iRetCode = -1;
    }

    // clear last headset status
    pHeadset->uLastHeadsetStatus &= ~(1 << iLocalUserIndex);
    
    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetAddLocalTalker

    \Description
        Create a game chat participant for this local user and populate the
        aLocalUser array.

    \Input *pHeadset        - headset module
    \Input iLocalUserIndex  - local user index
    \Input *pLocalUser      - local user

    \Output
        int32_t             - negative=error, zero=success

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetAddLocalTalker(VoipHeadsetRefT *pHeadset, int32_t iLocalUserIndex, VoipUserT *pLocalUser)
{
    User ^refUser = nullptr;

    NetPrintf(("voipheadsetxboxone: adding local talker %s at local user index %d\n", pLocalUser->strUserId, iLocalUserIndex));

    if (VOIP_NullUser(pLocalUser))
    {
        NetPrintf(("voipheadsetxboxone: can't add NULL local talker at local user index %d\n", iLocalUserIndex));
        return(-1);
    }

    // make sure there is no other local talker
    if (!VOIP_NullUser(&pHeadset->aLocalUsers[iLocalUserIndex].user))
    {
        NetPrintf(("voipheadsetxboxone: can't add multiple local talkers at the same index\n"));
        return(-2);
    }

    if (NetConnStatus('xusr', iLocalUserIndex, &refUser, sizeof(refUser)) < 0)
    {
        NetPrintf(("voipheadsetxboxone: no local user at index %d\n", iLocalUserIndex));
        return(-3);
    }

    VOIP_CopyUser(&pHeadset->aLocalUsers[iLocalUserIndex].user, pLocalUser);
    _VoipHeadsetUpdateAudioDevicesForUser(pHeadset, iLocalUserIndex);

    NetPrintf(("voipheadsetxboxone: local talker at index %d is %S\n", iLocalUserIndex, refUser->DisplayInfo->Gamertag->Data()));

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetActivateLocalTalker

    \Description
        Create a game chat participant for this local user and populate the
        aLocalUser array.

    \Input *pHeadset        - headset module
    \Input iLocalUserIndex  - local user index
    \Input bParticipate     - register flag

    \Output
        int32_t             - negative=error, zero=success

    \Version 05/14/2014 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetActivateLocalTalker(VoipHeadsetRefT *pHeadset, int32_t iLocalUserIndex, bool bParticipate)
{
    User ^refUser = nullptr;

    // find the system local user (winrt ref)
    if (NetConnStatus('xusr', iLocalUserIndex, &refUser, sizeof(refUser)) < 0)
    {
        NetPrintf(("voipheadsetxboxone: no local user at index %d\n", iLocalUserIndex));
        return(-1);
    }

    if ((pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant == nullptr) && (bParticipate == FALSE))
    {
        NetPrintf(("voipheadsetxboxone: cannot demote a user %d that is not already a participating user. Ignoring.\n", iLocalUserIndex));
        return(-1);
    }

    NetCritEnter(&pHeadset->crit);    // to prevent against conccurrent execution of _VoipHeadsetUpdateBystanders() from the voip thread

    if (bParticipate)
    {
        // remove previously registered bystander
        if (pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant != nullptr)
        {
            if (pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant->ParticipantType == Windows::Xbox::Chat::ChatParticipantTypes::Bystander)
            {
                _VoipHeadsetCleanParticipant(pHeadset, pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant, FALSE);
                pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant = nullptr;
            }
            else
            {
                NetPrintf(("voipheadsetxboxone: fatal conflict when trying to promote local user %s at local user index %d to participating state\n",
                    pHeadset->aLocalUsers[iLocalUserIndex].user.strUserId, iLocalUserIndex));


                NetCritLeave(&pHeadset->crit);

                return(-2);
            }
        }

        NetPrintf(("voipheadsetxboxone: local user %s at local user index %d is now participating\n",
            pHeadset->aLocalUsers[iLocalUserIndex].user.strUserId, iLocalUserIndex));

        try
        {
            pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant = ref new ChatParticipant(refUser);    // defaul participant type is ChatParticipantTypes::Listener|ChatParticipantTypes::Talker
            pHeadset->refChatSession->Channels->First()->Current->Participants->Append(pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant);
        }
        catch (Exception ^ e)
        {
            NetPrintf(("voipheadsetxboxone: exception caught while trying to add local talker %s at local user index %d (err=%s: %S)\n",
                pHeadset->aLocalUsers[iLocalUserIndex].user.strUserId, iLocalUserIndex, DirtyErrGetName(e->HResult), e->Message->Data()));

            NetCritLeave(&pHeadset->crit);

            return(-3);
        }

        // when a new local user is added to the session, the system does not notify the implicit session change, just fake it here
        pHeadset->bSessionStateChanged = TRUE;
    }
    else
    {
        NetPrintf(("voipheadsetxboxone: local user %s at local user index %d is no longer participating\n",
                    pHeadset->aLocalUsers[iLocalUserIndex].user.strUserId, iLocalUserIndex));

        _VoipHeadsetCleanParticipant(pHeadset, pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant, FALSE);
        pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant = nullptr;
    }

    NetCritLeave(&pHeadset->crit);

    #if DIRTYCODE_LOGGING
    _VoipHeadsetDumpChatParticipants(pHeadset);
    #endif

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetDeleteRemoteTalker

    \Description
        Destroy the game chat participant for this remote user.

    \Input *pHeadset        - headset module
    \Input ^refRemoteUser   - remote user

    \Version 07/12/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetDeleteRemoteTalker(VoipHeadsetRefT *pHeadset, RemoteUser ^refRemoteUser)
{
    NetPrintf(("voipheadsetxboxone: deleting remote talker %S\n", refRemoteUser->XboxUserId->Data()));

    _VoipHeadsetCleanParticipant(pHeadset, ref new ChatParticipant(refRemoteUser), TRUE);

    #if DIRTYCODE_LOGGING
    _VoipHeadsetDumpChatParticipants(pHeadset);
    #endif
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetAddRemoteTalker

    \Description
        Create the game chat participant for this remote user.

    \Input *pHeadset        - headset module
    \Input ^refRemoteUser   - remote user

    \Version 07/12/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetAddRemoteTalker(VoipHeadsetRefT *pHeadset, RemoteUser ^refRemoteUser)
{
    NetPrintf(("voipheadsetxboxone: adding remote talker %S\n", refRemoteUser->XboxUserId->Data()));

    try
    {
        ChatParticipant ^refChatParticipant = ref new ChatParticipant(refRemoteUser);    // default participant type is ChatParticipantTypes::Listener|ChatParticipantTypes::Talker
        pHeadset->refChatSession->Channels->First()->Current->Participants->Append(refChatParticipant);
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to add remote talker %S (err=%s: %S)\n",
            refRemoteUser->XboxUserId->Data(), DirtyErrGetName(e->HResult), e->Message->Data()));
    }

    // when a new remote user is added to the session, the system does not notify the implicit session change, just fake it here
    pHeadset->bSessionStateChanged = TRUE;

    #if DIRTYCODE_LOGGING
    _VoipHeadsetDumpChatParticipants(pHeadset);
    #endif
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetGetAudioDeviceWithId

    \Description
        Returns local user index associated with the Capture Source

    \Input *pHeadset                - pointer to headset state
    \Input ^strAudioDeviceId        - capture source or render target id
    \Input pLocalUserIndex          - [out] user index

    \Output
        IAudioDeviceInfo ^          - audio device associated with capture source

    \Notes
        $$todo - Review for MLU - this function returns in *pLocalUserIndex the index of
        the user associated with the audio device. This is problematic for the kinect
        as it can appear in the collection of devices of multiple local users.

    \Version 11/19/2014 (tcho)
*/
/********************************************************************************F*/
static IAudioDeviceInfo ^ _VoipHeadsetGetAudioDeviceWithId(VoipHeadsetRefT *pHeadset, String ^strAudioDeviceId, int32_t *pLocalUserIndex)
{
    int32_t iLocalUserIndex;
    uint32_t uAudioDeviceIndex;
    IAudioDeviceInfo ^refAudioDeviceInfo = nullptr;

    NetCritEnter(&pHeadset->crit);

    for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; ++iLocalUserIndex)
    {
        // find a audio device own by a local user that has a matching render target id
        if ((!VOIP_NullUser(&pHeadset->aLocalUsers[iLocalUserIndex].user)) && (pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector != nullptr))
        {
            for (uAudioDeviceIndex = 0; uAudioDeviceIndex < pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector->Size; ++uAudioDeviceIndex)
            {
                refAudioDeviceInfo = pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector->GetAt(uAudioDeviceIndex);

                if (strAudioDeviceId == refAudioDeviceInfo->Id)
                {
                    *pLocalUserIndex = iLocalUserIndex;
                    break;
                }
                else
                {
                    refAudioDeviceInfo = nullptr;
                }
            }
        }

        if (refAudioDeviceInfo != nullptr)
        {
            break;
        }
    }

    NetCritLeave(&pHeadset->crit);

    return(refAudioDeviceInfo);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetBeginMix

    \Description
        Begin chat mix for the specified render target

    \Input *pHeadset            - pointer to headset state
    \Input ^refChatRenderTarget - winrt ref for the render target

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetBeginMix(VoipHeadsetRefT *pHeadset, IChatRenderTarget ^refChatRenderTarget)
{
    try
    {
        refChatRenderTarget->BeginMix();
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to begin mix (err=%s: %S)\n",
            DirtyErrGetName(e->HResult), e->Message->Data()));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetAddMixBuffer

    \Description
        Add voice frame as a mix contributor.

    \Input *pHeadset            - pointer to headset state
    \Input ^refBuffer           - winrt ref for voice data buffer
    \Input ^refSourceId         - winrt ref for source id
    \Input ^refSourceFormat     - winrt ref for source format
    \Input ^refChatRenderTarget - winrt ref for the render target

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetAddMixBuffer(VoipHeadsetRefT *pHeadset, IBuffer ^refBuffer, String ^refSourceId, IFormat ^refSourceFormat, IChatRenderTarget ^refChatRenderTarget)
{
    try
    {
        refChatRenderTarget->AddMixBuffer(refSourceId, refSourceFormat, refBuffer);
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to add mix buffer (err=%s: %S)\n",
            DirtyErrGetName(e->HResult), e->Message->Data()));
        refChatRenderTarget->ResetMix();
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSubmitMix

    \Description
        Submit the chat mix buffers (previously submitted with calls to
        _VoipHeadsetAddMixBuffer()) to be rendered on the specified render target.

    \Input *pHeadset            - pointer to headset state
    \Input ^refChatRenderTarget - winrt ref for the render target

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetSubmitMix(VoipHeadsetRefT *pHeadset, IChatRenderTarget ^refChatRenderTarget)
{
    uint8_t bOperationFailed = FALSE;

    try
    {
        refChatRenderTarget->SubmitMix();
    }
    catch (Exception ^ e)
    {
        NetPrintf(("voipheadsetxboxone: exception caught while trying to submit mix (err=%s: %S)\n", DirtyErrGetName(e->HResult), e->Message->Data()));
        bOperationFailed = TRUE;
    }

    if (bOperationFailed)
    {
        try
        {
            refChatRenderTarget->ResetMix();
        }
        catch (Exception ^ e)
        {
            NetPrintf(("voipheadsetxboxone: exception caught while trying to reset mix (err=%s: %S)\n", DirtyErrGetName(e->HResult), e->Message->Data()));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetEnqueueInboundSubPacket

    \Description
        Store inbound packet into inbound queue.

    \Input *pHeadset                - pointer to headset state
    \Input *pRemoteUser             - remote user from which the voice data originated from (NULL when loopback is used)
    \Input iRemoteUserIndex         - remote index of user
    \Input *pData                   - data pointer
    \Input iDataSize                - buffer size
    \Input ^refChatCaptureSourceId  - originating capture source id
    \Input *pSerializedDevice       - originating capture source (serialized blob)

    \Version 16/07/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetEnqueueInboundSubPacket(VoipHeadsetRefT *pHeadset, VoipUserT *pRemoteUser, int32_t iRemoteUserIndex, const uint8_t *pData, int32_t iDataSize, String ^refChatCaptureSourceId, VoipSerializedDeviceT *pSerializedDevice)
{
    // special handling for inbound traffic from a remote capture source
    // if it is a new source for the remote user, the associated chat participant needs to
    // have his collection of audio devices updated, consequently it is removed from and
    // re-added to the default game chat channel
    if (pRemoteUser != NULL)
    {
        uint32_t bCollectionChanged = VoipUpdateSerializedUser((VoipUserExT *)pRemoteUser, pSerializedDevice);
        if (bCollectionChanged)
        {
            // unregister user to make sure it is removed from the default channel
            VoipHeadsetRegisterUserCb(pRemoteUser, FALSE, pHeadset);

            // register user back
            VoipHeadsetRegisterUserCb(pRemoteUser, TRUE, pHeadset);
        }
    }

    // create an inbound queue for this capture source if none exists yet
    if (!pHeadset->refInboundQueuesMap->HasKey(refChatCaptureSourceId))
    {
        _VoipHeadsetCreateInboundQueue(pHeadset, refChatCaptureSourceId);

        // create a decoder for this capture source if none exists yet
        if (!pHeadset->refDecodersMap->HasKey(refChatCaptureSourceId))
        {
            _VoipHeadsetCreateChatDecoder(pHeadset, refChatCaptureSourceId);
        }
    }

    Platform::Collections::Vector<IBuffer ^> ^refInboundQueue =
        safe_cast<Platform::Collections::Vector<IBuffer ^>^>(pHeadset->refInboundQueuesMap->Lookup(refChatCaptureSourceId));

    if (refInboundQueue->Size < VOIPHEADSET_INBOUND_QUEUE_MAX_CAPACITY)
    {
        // $todo - can we optimize this? pre-allocate?

        // allocate and fill data buffer
        IBuffer ^refBuffer = ref new Buffer(iDataSize);
        ComPtr<IBufferByteAccess> bufferByteAccess;
        reinterpret_cast<IInspectable*>(refBuffer)->QueryInterface(IID_PPV_ARGS(&bufferByteAccess));
        byte *pBufferBytes = nullptr;
        bufferByteAccess->Buffer(&pBufferBytes);
        ds_memcpy(pBufferBytes, pData, iDataSize);
        refBuffer->Length = iDataSize;

        refInboundQueue->Append(refBuffer);
    }
    else
    {
        NetPrintf(("voipheadsetxboxone: dropping voice frame because inbound queue is full (source = %S)\n", refChatCaptureSourceId->Data()));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetPollForVoiceData

    \Description
        Poll to see if voice data is available.

    \Input *pHeadset    - pointer to headset state

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetPollForVoiceData(VoipHeadsetRefT *pHeadset)
{
    uint8_t aBuffer[128];
    uint32_t uSourceIndex;
    int32_t iLocalUserIndex;
    Windows::Xbox::Chat::CaptureBufferStatus eCaptureBufferStatus;
    IBuffer ^refBuffer = nullptr;
    uint32_t uTick = NetTick();

    if (pHeadset->bKinectIsCapturing && (NetTickDiff(uTick, pHeadset->uKinectLastCapture) > 500))
    {
        #if DIRTYCODE_LOGGING
        if (pHeadset->bKinectIsCapturing == TRUE)
        {
            NetPrintf(("voipheadsetxboxone: stopping to submit voice captured from kinect for network transmission\n"));
        }
        #endif
        pHeadset->bKinectIsCapturing = FALSE;
    }

    for (uSourceIndex = 0; uSourceIndex < pHeadset->refCurrentChatSessionState->CaptureSources->Size; uSourceIndex++)
    {
        IChatCaptureSource ^refChatCaptureSource = pHeadset->refCurrentChatSessionState->CaptureSources->GetAt(uSourceIndex);
        IAudioDeviceInfo ^refAudioDeviceInfo = nullptr;
        uint8_t bIsKinect = FALSE;
        uint8_t aMetaData[32];
        int32_t iMetaDataSize;
        uint8_t * pWrite = aMetaData;
        uint32_t uConsoleId = NetConnMachineId();
        VoipSerializedDeviceT *pSerializedDevice;

        // check if capture source is kinect
        if (wcsstr(refChatCaptureSource->Id->Data(), L"postmec") != NULL)
        {
            bIsKinect = TRUE;
        }

        // find audio device matching capture source id
        refAudioDeviceInfo = _VoipHeadsetGetAudioDeviceWithId(pHeadset, refChatCaptureSource->Id, &iLocalUserIndex);
        if (refAudioDeviceInfo == nullptr)
        {
          continue;
        }

        // serialize the console identifier
        *pWrite++  = (uint8_t)(uConsoleId >> 24);
        *pWrite++  = (uint8_t)(uConsoleId >> 16);
        *pWrite++  = (uint8_t)(uConsoleId >> 8);
        *pWrite++  = (uint8_t)uConsoleId;

        // serialize the capture source
        VoipSerializeAudioDevice(refAudioDeviceInfo, (VoipSerializedDeviceT *)pWrite);
        pSerializedDevice = (VoipSerializedDeviceT *)pWrite;
        iMetaDataSize = sizeof(uConsoleId) + sizeof (VoipSerializedDeviceT);

        while(1)
        {
            eCaptureBufferStatus = refChatCaptureSource->GetNextBuffer(&refBuffer);

            if (eCaptureBufferStatus == Windows::Xbox::Chat::CaptureBufferStatus::NoMicrophoneFocus)
            {
                if (pHeadset->bMicFocus == TRUE)
                {
                    NetPrintf(("voipheadsetxboxone: mic focus lost - voice rendering is interrupted\n"));
                    pHeadset->bMicFocus = FALSE;
                }
                break;
            }
            else
            {
                if (pHeadset->bMicFocus == FALSE)
                {
                    NetPrintf(("voipheadsetxboxone: mic focus regained - voice rendering resumes\n"));
                    pHeadset->bMicFocus = TRUE;
                }
            }

            if (eCaptureBufferStatus != Windows::Xbox::Chat::CaptureBufferStatus::Filled && eCaptureBufferStatus != Windows::Xbox::Chat::CaptureBufferStatus::Incomplete)
            {
               break;  // no more voice data for the moment
            }

            // if data from the kinect is about to be sent over the network, flag it
            if (bIsKinect)
            {
                #if DIRTYCODE_LOGGING
                if (pHeadset->bKinectIsCapturing == FALSE)
                {
                    NetPrintf(("voipheadsetxboxone: starting to submit voice captured from kinect for network transmission\n"));
                }
                #endif
                pHeadset->bKinectIsCapturing = TRUE;
                pHeadset->uKinectLastCapture = uTick;
            }


            #if DIRTYCODE_LOGGING
            if (eCaptureBufferStatus == Windows::Xbox::Chat::CaptureBufferStatus::Incomplete)
            {
                NetPrintf(("voipheadsetxboxone: incomplete voice frame acquired from mic\n"));
            }
            #endif

            // encode the data
            IBuffer ^refEncodedBuffer = nullptr;
            IChatEncoder ^refChatEncoder = pHeadset->refEncodersMap->Lookup(refChatCaptureSource->Id);
            refChatEncoder->Encode(refBuffer, &refEncodedBuffer);

            if (refEncodedBuffer->Length <= sizeof(aBuffer))
            { 
                DataReader ^refDataReader = DataReader::FromBuffer(refEncodedBuffer);
                refDataReader->ReadBytes(ArrayReference<uint8_t>(aBuffer, refEncodedBuffer->Length));

                if (pHeadset->bLoopback)
                {
                    // loop data back: virtually exercise voip packet reception from network
                    _VoipHeadsetEnqueueInboundSubPacket(pHeadset, NULL, 0, aBuffer, refEncodedBuffer->Length, refChatCaptureSource->Id, pSerializedDevice);
                }
                else
                {
                    // send to mic data callback
                    if (pHeadset->bKinectIsCapturing == TRUE)
                    {
                        // the capture source is a kinect send the voice data as the local shared user
                        pHeadset->pMicDataCb(aBuffer, refEncodedBuffer->Length, aMetaData, iMetaDataSize, VOIP_SHARED_USER_INDEX, pHeadset->uSendSeq++, pHeadset->pCbUserData);
                    }
                    else
                    {
                        // the capture is not a kinect send the voice data normally
                        pHeadset->pMicDataCb(aBuffer, refEncodedBuffer->Length, aMetaData, iMetaDataSize, iLocalUserIndex, pHeadset->uSendSeq++, pHeadset->pCbUserData);
                    }

                    /*
                    $todo also submit to all local render targets as instructed in MS doc (probably only required for MLU support, and probably
                    excluding the render target that loops back to the talking player ?!)
                    */
                }
            }
            else
            {
                NetPrintf(("voipheadsetxboxone: static buffer not big enough (%d vs %d) to store encoded buffer for local user index %d\n",
                    refBuffer->Length, sizeof(aBuffer), iLocalUserIndex));
                break;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetVoicePlaybackCheck

    \Description
        Check to see if the capture source can playback voice on the given
        render target.

    \Input *pHeadset            - pointer to headset state
    \Input refRenderTargetId    - render target id
    \Input refCaptureSourceId   - capture source id


    \Version 11/20/2014 (tcho)
*/
/********************************************************************************F*/
static uint8_t _VoipHeadsetVoicePlaybackCheck(VoipHeadsetRefT *pHeadset, String^ refRenderTargetId, String^ refCaptureSourceId)
{
    uint8_t bVoiceEnabled = FALSE;
    uint8_t bRemoteCaptureSourceOwnerFound = FALSE;
    uint32_t uParticipantIndex;
    uint32_t uAudioDevIndex;
    int32_t iRenderTargetOwnerIndex;
    int32_t iRemoteSharedUserIndex;
    RemoteUser^ refRemoteUser;

    // if the render target is the TV, we assume the render target onwer is the local shared user
    if (wcsstr(refRenderTargetId->Data(), L"hdmi") != NULL)
    {
        iRenderTargetOwnerIndex = VOIP_SHARED_USER_INDEX;
    }
    else
    {
        // get the local user index of the render target owner
        _VoipHeadsetGetAudioDeviceWithId(pHeadset, refRenderTargetId, &iRenderTargetOwnerIndex);
    }

    // if the remote capture source is shared (kinect), find the the remote user that owns it
    if (wcsstr(refCaptureSourceId->Data(), L"postmec") != NULL)
    {
        for (iRemoteSharedUserIndex = 0; iRemoteSharedUserIndex < VOIP_MAX_LOW_LEVEL_CONNS; ++iRemoteSharedUserIndex)
        {
            if (pHeadset->aRemoteSharedUsers[iRemoteSharedUserIndex].CaptureSoureId == refCaptureSourceId)
            {
                if (pHeadset->aRemoteSharedUsers[iRemoteSharedUserIndex].uEnablePlaybackFlag & (1 << iRenderTargetOwnerIndex))
                {
                    // enable playback
                    bVoiceEnabled = TRUE;
                }
                else
                {
                    // disable playback
                    bVoiceEnabled = FALSE;
                }

                bRemoteCaptureSourceOwnerFound = TRUE;
                break;
            } // if
        } // for
    }
    // if the remote capture source is not shared look up the owner by asking the chat session
    else
    {
        // get a immutable version of the channel vector
        IVectorView<IChatChannel ^> ^refChannelImmutableVector;
        refChannelImmutableVector = pHeadset->refChatSession->Channels->GetView();
            
        // get a immutable version of the participant vector
        IVectorView<IChatParticipant ^> ^refParticipantImmutableVector;
        refParticipantImmutableVector = refChannelImmutableVector->First()->Current->Participants->GetView();

        // loop through all the participants in the channel
        for (uParticipantIndex = 0; uParticipantIndex < refParticipantImmutableVector->Size; ++uParticipantIndex)
        {
            IChatParticipant ^refParticipant = refParticipantImmutableVector->GetAt(uParticipantIndex);

            // check if the Participant is a Remote User
            refRemoteUser = dynamic_cast<RemoteUser^>(refParticipant->User);
            
            if (refRemoteUser != nullptr)
            {
                // we found a remote user
                // now check to see if he owns the capture source
                for (uAudioDevIndex = 0; uAudioDevIndex < refRemoteUser->AudioDevices->Size; ++uAudioDevIndex)
                {
                    if (refRemoteUser->AudioDevices->GetAt(uAudioDevIndex)->Id == refCaptureSourceId)
                    {
                        if (refRemoteUser->EnablePlaybackFlag & (1 << iRenderTargetOwnerIndex))
                        {
                            // enable playback
                            bVoiceEnabled = TRUE;
                        }
                        else
                        {
                            // disable playback
                            bVoiceEnabled = FALSE;
                        }

                        bRemoteCaptureSourceOwnerFound = TRUE;
                        break;
                    } // if 
                } // for 
            }// if

            if (bRemoteCaptureSourceOwnerFound == TRUE)
            {
                break;
            }
        } // for
    } 

    // if a capture source is not own by a remote user just return true (for loopback mode)
    if (bRemoteCaptureSourceOwnerFound == TRUE)
    {
        return(bVoiceEnabled);
    }
    else
    {
        return(TRUE);
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetProcessInboundQueues

    \Description
        Fill the game chat render targets with voice data from inbound queues.

    \Input *pHeadset    - pointer to headset state

    \Notes
        $$todo - Review for Voice Playback Enable. Currently for the Kinect Disabling 
        voice playback is not supported. We might need to revisit this later.

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetProcessInboundQueues(VoipHeadsetRefT *pHeadset)
{
    uint32_t uRenderTargetIndex;
    int32_t bMixing = FALSE;

    // loop through all inbound queues
    for (IIterator<IKeyValuePair<String ^, Object ^> ^> ^it = pHeadset->refInboundQueuesMap->First(); it->HasCurrent; it->MoveNext())
    {
        Platform::Collections::Vector<IBuffer ^> ^refInboundQueue = safe_cast<Platform::Collections::Vector<IBuffer ^>^>(it->Current->Value);
        String ^refRemoteChatCaptureSourceId = it->Current->Key;
        uint8_t bPreBuffering = pHeadset->refPreBufStateMap->Lookup(refRemoteChatCaptureSourceId);   // find out if prebuffering is on or off on that queue

        // is the queue full?
        if (refInboundQueue->Size < VOIPHEADSET_INBOUND_QUEUE_MAX_CAPACITY)
        {
            // is the queue empty? 
            if (refInboundQueue->Size == 0)
            {
                // re-enter pre-buffering state if necessary
                if (!bPreBuffering)
                {
                    NetPrintfVerbose((pHeadset->iDebugLevel, 2, "voipheadsetxboxone: re-enter pre-buffering state for %S\n", refRemoteChatCaptureSourceId->Data()));
                    pHeadset->refPreBufStateMap->Remove(refRemoteChatCaptureSourceId);
                    pHeadset->refPreBufStateMap->Insert(refRemoteChatCaptureSourceId, true);
                }
            }
            else
            {
                // is it time to extract an entry from the queue?
                // if prebuffering is on: wait until the low watermark is reached
                // if prebuffering is off: consume even if we are under the low watermark
                if ((bPreBuffering && (refInboundQueue->Size > VOIPHEADSET_INBOUND_QUEUE_LOW_WATERMARK)) || !bPreBuffering)
                {
                    // mark prebuffering as completed
                    if (bPreBuffering)
                    {
                        pHeadset->refPreBufStateMap->Remove(refRemoteChatCaptureSourceId);
                        pHeadset->refPreBufStateMap->Insert(refRemoteChatCaptureSourceId, false);
                    }

                    // begin chat mix on all render targets if not done already
                    if (bMixing == FALSE)
                    {
                        for (uRenderTargetIndex = 0; uRenderTargetIndex < pHeadset->refCurrentChatSessionState->RenderTargets->Size; uRenderTargetIndex++)
                        {
                            _VoipHeadsetBeginMix(pHeadset, pHeadset->refCurrentChatSessionState->RenderTargets->GetAt(uRenderTargetIndex));
                        }

                        bMixing = TRUE;
                    }

                    // add inbound voice frame for mixing on all render targets
                    IBuffer ^refBuffer = refInboundQueue->GetAt(0);
                    IChatDecoder ^refChatDecoder = pHeadset->refDecodersMap->Lookup(refRemoteChatCaptureSourceId);
                    refChatDecoder->Decode(refBuffer, &pHeadset->refDecodedBuffer);
                    
                    for (uRenderTargetIndex = 0; uRenderTargetIndex < pHeadset->refCurrentChatSessionState->RenderTargets->Size; uRenderTargetIndex++)
                    {
                        IChatRenderTarget ^refChatRenderTarget = pHeadset->refCurrentChatSessionState->RenderTargets->GetAt(uRenderTargetIndex);

                        if (pHeadset->bMicFocus == TRUE)
                        {
                            // only submit in bound queue buffer for mixing if voice playback is enabled for our capture source 
                            if (_VoipHeadsetVoicePlaybackCheck(pHeadset, refChatRenderTarget->Id, refRemoteChatCaptureSourceId) == TRUE)
                            {
                                // add the decoded data in the mix queue of the specified render target
                                _VoipHeadsetAddMixBuffer(pHeadset, pHeadset->refDecodedBuffer, refRemoteChatCaptureSourceId, refChatDecoder->Format, refChatRenderTarget);
                            }
                        }
                    }

                    refInboundQueue->RemoveAt(0);
                    NetPrintfVerbose((pHeadset->iDebugLevel, 3, "voipheadsetxboxone: consumed voice frame, new queue size is %d\n", refInboundQueue->Size));
                }
            }
        }
        else
        {
            NetPrintf(("voipheadsetxboxone: clearing inbound queue because it is full\n"));
            refInboundQueue->Clear();
            break;
        }
    }

    // submit chat mix on all render targets if mixing is in progress
    if (bMixing)
    {
        for (uRenderTargetIndex = 0; uRenderTargetIndex < pHeadset->refCurrentChatSessionState->RenderTargets->Size; uRenderTargetIndex++)
        {
            _VoipHeadsetSubmitMix(pHeadset, pHeadset->refCurrentChatSessionState->RenderTargets->GetAt(uRenderTargetIndex));
        }
    }

    #if DIRTYCODE_LOGGING
    uint32_t uCurrentTick, uTickDiff;
    uCurrentTick = NetTick();
    uTickDiff = NetTickDiff(uCurrentTick, pHeadset->uLastTick);
    if ((uTickDiff > 20) && (pHeadset->uLastTick != 0))
    {
        NetPrintfVerbose((pHeadset->iDebugLevel, 2, "voipheadsetxboxone: voice submission is not fast enough (%d ms since last attempt)\n", uTickDiff));
    }
    pHeadset->uLastTick = uCurrentTick;
    #endif
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetUpdateBystanders

    \Description
        Make sure that any locally signed-in user not actively participating in 
        the chat session is also added to the chat session as "bystander" for the
        game chat system to be able to restrict "shared" voip via kinect accordingly.

    \Input *pHeadset    - pointer to headset state

    \Version 02/06/2015 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetUpdateBystanders(VoipHeadsetRefT *pHeadset)
{
    int32_t iLocalUserIndex;
    User ^refUser = nullptr;

    NetCritEnter(&pHeadset->crit);  // to prevent about concurrent execution of _VoipHeadsetActivateLocalTalker() from the main thread

    for (iLocalUserIndex = 0; iLocalUserIndex < NETCONN_MAXLOCALUSERS; ++iLocalUserIndex)
    {
        if (pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant == nullptr)
        {
            if (NetConnStatus('xusr', iLocalUserIndex, &refUser, sizeof(refUser)) >= 0)
            {
                // deal with newly added bystander

                try
                {
                    pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant = ref new ChatParticipant(refUser);    // defaul participant type is ChatParticipantTypes::Listener|ChatParticipantTypes::Talker
                    pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant->ParticipantType = Windows::Xbox::Chat::ChatParticipantTypes::Bystander;
                    pHeadset->refChatSession->Channels->First()->Current->Participants->Append(pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant);

                    NetPrintf(("voipheadsetxboxone: local user %S at local user index %d is now a bystander\n", refUser->XboxUserId->Data(), iLocalUserIndex));
                }
                catch (Exception ^ e)
                {
                    NetPrintf(("voipheadsetxboxone: exception caught while trying to add bystander %s at local user index %d (err=%s: %S)\n",
                        pHeadset->aLocalUsers[iLocalUserIndex].user.strUserId, iLocalUserIndex, DirtyErrGetName(e->HResult), e->Message->Data()));
                }

                // when a new local user is added to the session, the system does not notify the implicit session change, just fake it here
                pHeadset->bSessionStateChanged = TRUE;
            }
        }
        else
        {
            if (pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant->ParticipantType == Windows::Xbox::Chat::ChatParticipantTypes::Bystander)
            {
                uint32_t bBystanderRemoved = FALSE;

                if (NetConnStatus('xusr', iLocalUserIndex, &refUser, sizeof(refUser)) >= 0)
                {
                    if (pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant->User->Id != refUser->Id)
                    {
                        bBystanderRemoved = TRUE;
                    }
                }
                else
                {
                    bBystanderRemoved = TRUE;
                }

                if (bBystanderRemoved)
                {
                    // deal with newly removed bystander

                    NetPrintf(("voipheadsetxboxone: local user %S at local user index %d is no longer a bystander\n",
                        pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant->User->XboxUserId->Data(), iLocalUserIndex));

                    _VoipHeadsetCleanParticipant(pHeadset, pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant, FALSE);
                    pHeadset->aLocalUsers[iLocalUserIndex].refChatParticipant = nullptr;
                }
            }
        }
    }

    NetCritLeave(&pHeadset->crit);

    #if DIRTYCODE_LOGGING
    if (pHeadset->bSessionStateChanged)
    {
        _VoipHeadsetDumpChatParticipants(pHeadset);
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSetRemoteUserVoicePlayback

    \Description
        Helper function to enable or disable voice playback for a non shared remote user
        for a given local user or local shared user

    \Input *pHeadset            - pointer to headset state
    \Input *pRemoteUser         - Remote User
    \Input iLocalUserIndex      - local or local shared user index
    \Input bEnablePlayback      - TRUE to enable voice playback. FALSE to disable voice playback

    \Output
        int32_t                 - negative=error, zero=success 

    \Version 12/10/2014 (tcho)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetSetRemoteUserVoicePlayback(VoipHeadsetRefT *pHeadset, VoipUserT *pRemoteUser, int32_t iLocalUserIndex, uint8_t bEnablePlayback)
{
    uint8_t bRemoteUserFound = FALSE;
    uint32_t uParticipantIndex;
    int32_t iRet = 0;

    // get a immutable version of the channel vector
    IVectorView<IChatChannel ^> ^refChannelImmutableVector;
    refChannelImmutableVector = pHeadset->refChatSession->Channels->GetView();

    // get a immutable version of the participant vector
    IVectorView<IChatParticipant ^> ^refParticipantImmutableVector;
    refParticipantImmutableVector = refChannelImmutableVector->First()->Current->Participants->GetView();

    for (uParticipantIndex = 0; uParticipantIndex < refParticipantImmutableVector->Size; ++uParticipantIndex)
    {
        VoipUserT voipUser;
        uint64_t uXuid;
        RemoteUser ^refRemoteUser;
        IChatParticipant ^refParticipant = refParticipantImmutableVector->GetAt(uParticipantIndex);

        // check to see if the xuid match our 
        uXuid = _wtoi64(refParticipant->User->XboxUserId->Data());
        memset(&voipUser, 0, sizeof(voipUser));
        ds_snzprintf(voipUser.strUserId, (int32_t)sizeof(voipUser.strUserId), "%llX", uXuid); 

        if (VOIP_SameUser(&voipUser, pRemoteUser))
        {
            bRemoteUserFound = TRUE;
            refRemoteUser = static_cast<RemoteUser ^>(refParticipant->User); 
                        
            // set playback flags for the remote user
            if (bEnablePlayback)
            {
                refRemoteUser->EnablePlaybackFlag |= (1 << iLocalUserIndex);
                NetPrintf(("voipheadsetxboxone: voice playback enabled for remote user %s at local index %d, uEnablePlaybackFlag bit field 0x%08X\n", voipUser.strUserId, iLocalUserIndex, refRemoteUser->EnablePlaybackFlag));
            }
            else
            {
                refRemoteUser->EnablePlaybackFlag &= ~(1 << iLocalUserIndex);
                NetPrintf(("voipheadsetxboxone: voice playback disabled for remote shared user %s at local index %d, uEnablePlaybackFlag bit field 0x%08X\n", voipUser.strUserId, iLocalUserIndex, refRemoteUser->EnablePlaybackFlag));                           
            }
                        
            // break out of the participant loop
            break;
        }
    }

    if (bRemoteUserFound  == FALSE)
    {
        iRet = -1;
        NetPrintf(("voipheadsetxboxone:_VoipHeadsetSetRemoteUserVoicePlayback() failed no remote user was found"));  
    }

    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSetRemoteSharedUserVoicePlayback

    \Description
        Helper function to enable or disable voice playback for a shared remote user
        for a given local user or local shared user

    \Input *pHeadset            - pointer to headset state
    \Input *pRemoteSharedUser   - Remote Shared User
    \Input iLocalUserIndex      - local or local shared user index
    \Input bEnablePlayback      - TRUE to enable voice playback. FALSE to disable voice playback

    \Output
        int32_t                 - negative=error, zero=success 

    \Version 12/10/2014 (tcho)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetSetRemoteSharedUserVoicePlayback(VoipHeadsetRefT *pHeadset, VoipUserT *pRemoteSharedUser, int32_t iLocalUserIndex, uint8_t bEnablePlayback)
{
    uint8_t bRemoteSharedUserFound = FALSE;
    int32_t iRemoteSharedUserIndex;
    int32_t iRet = 0;
    
    // find the remote shared user and set the playback flag for it
    for (iRemoteSharedUserIndex = 0; iRemoteSharedUserIndex < VOIP_MAX_LOW_LEVEL_CONNS; ++iRemoteSharedUserIndex)
    {
        if (VOIP_SameUser(&pHeadset->aRemoteSharedUsers[iRemoteSharedUserIndex].user, pRemoteSharedUser))
        {
            if (bEnablePlayback == TRUE)
            {
                 pHeadset->aRemoteSharedUsers[iRemoteSharedUserIndex].uEnablePlaybackFlag |= (1 << iLocalUserIndex);
                 NetPrintf(("voipheadsetxboxone: voice playback enabled for remote shared user %s at local index %d, uEnablePlaybackFlag bit field 0x%08X\n", pRemoteSharedUser->strUserId, iLocalUserIndex, pHeadset->aRemoteSharedUsers[iRemoteSharedUserIndex].uEnablePlaybackFlag));
            }
            else
            {
                pHeadset->aRemoteSharedUsers[iRemoteSharedUserIndex].uEnablePlaybackFlag  &= ~(1 << iLocalUserIndex);
                NetPrintf(("voipheadsetxboxone: voice playback disabled for remote shared user %s at local index %d, uEnablePlaybackFlag bit field 0x%08X\n", pRemoteSharedUser->strUserId, iLocalUserIndex, pHeadset->aRemoteSharedUsers[iRemoteSharedUserIndex].uEnablePlaybackFlag));  
            }

            bRemoteSharedUserFound = TRUE;
            break;
        }
    }

    if (bRemoteSharedUserFound == FALSE)
    {
        iRet = -1;
        NetPrintf(("voipheadsetxboxone:_VoipHeadsetSetRemoteSharedUserVoicePlayback() failed no remote shared user was found"));  
    }

    return(iRet);
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function   VoipHeadsetCreate

    \Description
        Create the headset manager.

    \Input iMaxChannels - max number of remote users
    \Input iMaxLocal    - max number of local users
    \Input *pMicDataCb  - pointer to user callback to trigger when mic data is ready
    \Input *pStatusCb   - pointer to user callback to trigger when headset status changes
    \Input *pCbUserData - pointer to user callback data
    \Input iData        - platform-specific - unused for xboxone

    \Output
        VoipHeadsetRefT *   - pointer to module state, or NULL if an error occured

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
VoipHeadsetRefT *VoipHeadsetCreate(int32_t iMaxChannels, int32_t iMaxLocal, VoipHeadsetMicDataCbT *pMicDataCb, VoipHeadsetStatusCbT *pStatusCb, void *pCbUserData, int32_t iData)
{
    VoipHeadsetRefT *pHeadset;
    int32_t iResult;
    int32_t iMemGroup;
    int32_t iIndex;
    void *pMemGroupUserData;

    // query current mem group data
    VoipMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // make sure we don't exceed maxchannels
    if (iMaxChannels > VOIP_MAXCONNECT)
    {
        NetPrintf(("voipheadsetxboxone: request for %d channels exceeds max\n", iMaxChannels));
        return(NULL);
    }

    // allocate and clear module state
    if ((pHeadset = (VoipHeadsetRefT *)DirtyMemAlloc(sizeof(*pHeadset), VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipheadsetxboxone: not enough memory to allocate module state\n"));
        return(NULL);
    }
    memset(pHeadset, 0, sizeof(*pHeadset));

    pHeadset->refChatSession = nullptr;         // assumption: this line implicitly releases all participants and channels associated with the session
    pHeadset->refCurrentChatSessionState = nullptr;
    pHeadset->refAsyncOp = nullptr;
    pHeadset->refNewChatSessionState = nullptr;
    pHeadset->EncodingQual = VOIPHEADSET_GAMECHAT_DEFAULT_ENCODING_QUALITY;
    pHeadset->uLastBystanderUpdate = NetTick();

    // set default debuglevel
    #if DIRTYCODE_LOGGING
    pHeadset->iDebugLevel = 1;
    #endif

    // save callback info
    pHeadset->pMicDataCb = pMicDataCb;
    pHeadset->pStatusCb = pStatusCb;
    pHeadset->pCbUserData = pCbUserData;
    pHeadset->iMaxChannels = iMaxChannels;

    // initialize array of local users
    for (iIndex = 0; iIndex < VOIP_MAXLOCALUSERS; iIndex++)
    {
        pHeadset->aLocalUsers[iIndex].refAudioDevsVector = nullptr;
        pHeadset->aLocalUsers[iIndex].refChatParticipant = nullptr;
    }

    // create maps used to track encoders, decoders and  inbound queues associated with local and remote capture source
    pHeadset->refEncodersMap = ref new Platform::Collections::Map<String ^, IChatEncoder ^>();
    pHeadset->refDecodersMap = ref new Platform::Collections::Map<String ^, IChatDecoder ^>();
    pHeadset->refInboundQueuesMap = ref new Platform::Collections::Map<String ^, Object ^>();  // here we use object to work around compile errors with a map of vectors
    pHeadset->refPreBufStateMap = ref new Platform::Collections::Map<String ^, bool>();

    // init the critical section
    NetCritInit(&pHeadset->crit, "voipheadset");

    if ((iResult = _VoipHeadsetCreateChatSession(pHeadset)) != 0)
    {
        NetPrintf(("voipheadsetxboxone: failed to create chat session\n"));
        VoipHeadsetDestroy(pHeadset);
        return(NULL);
    }

    if ((iResult = _VoipHeadsetRegisterSessionStateChangedHandler(pHeadset)) != 0)
    {
        NetPrintf(("voipheadsetxboxone: failed to register game chat session state changed event handler\n"));
        VoipHeadsetDestroy(pHeadset);
        return(NULL);
    }

    if ((iResult = _VoipHeadsetRegisterKinectAvailabilityChangedHandler(pHeadset)) != 0)
    {
        NetPrintf(("voipheadsetxboxone: failed to register kinect availability changed event handler\n"));
        VoipHeadsetDestroy(pHeadset);
        return(NULL);
    }

    // listen for audio device update events
    if (!pHeadset->AudioDeviceAddedToken.Value)
    {
        pHeadset->AudioDeviceAddedToken = User::AudioDeviceAdded += ref new EventHandler<AudioDeviceAddedEventArgs^>(
            [pHeadset] (Platform::Object^, AudioDeviceAddedEventArgs^ eventArgs)
        {
            User ^refUser = nullptr;
            int32_t iLocalUserIndex = -1;

            // find the correct user index
            for (int32_t i = 0; i < NETCONN_MAXLOCALUSERS; ++i)
            {
                if (NetConnStatus('xusr', i, &refUser, sizeof(refUser)) >= 0)
                {
                    if (refUser->Id == eventArgs->User->Id)
                    {
                        iLocalUserIndex = i;
                        break;
                    }
                }
            }

            // append to the device list for this user
            if (iLocalUserIndex >= 0 && !VOIP_NullUser(&pHeadset->aLocalUsers[iLocalUserIndex].user))
            {
                NetCritEnter(&pHeadset->crit);

                uint8_t uDeviceId;
                VoipSerializedDeviceT SerializedDevice;
                AudioDevInfo ^refRemoteAudioDeviceInfo = ref new AudioDevInfo;
                VoipSerializeAudioDevice(eventArgs->AudioDevice, &SerializedDevice);
                VoipDeserializeAudioDevice(&SerializedDevice, NetConnMachineId(), refRemoteAudioDeviceInfo, &uDeviceId);
                // fill in some of the fields hardcoded during deserialization
                refRemoteAudioDeviceInfo->Id = eventArgs->AudioDevice->Id;
                refRemoteAudioDeviceInfo->DeviceType = eventArgs->AudioDevice->DeviceType;
                pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector->Append(refRemoteAudioDeviceInfo);

                pHeadset->bHeadsetChanged = TRUE;

                NetCritLeave(&pHeadset->crit);
            }
        });
    }

    if (!pHeadset->AudioDeviceChangedToken.Value)
    {
        pHeadset->AudioDeviceChangedToken = User::AudioDeviceChanged += ref new EventHandler<AudioDeviceChangedEventArgs^>(
            [pHeadset] (Platform::Object^, AudioDeviceChangedEventArgs^ eventArgs)
        {
            NetPrintf(("voipheadsetxboxone: audio device change detected\n"));
        });
    }

    if (!pHeadset->AudioDeviceRemovedToken.Value)
    {
        pHeadset->AudioDeviceRemovedToken = User::AudioDeviceRemoved += ref new EventHandler<AudioDeviceRemovedEventArgs^>(
            [pHeadset] (Platform::Object^, AudioDeviceRemovedEventArgs^ eventArgs)
        {
            User ^refUser = nullptr;
            int32_t iLocalUserIndex = -1;

            // find the correct user index
            for (int32_t i = 0; i < NETCONN_MAXLOCALUSERS; ++i)
            {
                if (NetConnStatus('xusr', i, &refUser, sizeof(refUser)) >= 0)
                {
                    if (refUser->Id == eventArgs->User->Id)
                    {
                        iLocalUserIndex = i;
                        break;
                    }
                }
            }

            // remove the device from list
            if (iLocalUserIndex >= 0 && !VOIP_NullUser(&pHeadset->aLocalUsers[iLocalUserIndex].user))
            {
                NetCritEnter(&pHeadset->crit);

                // search for and remove the device from the user's list
                if (pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector != nullptr)
                {
                    for (uint32_t i = 0; i < pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector->Size; ++i)
                    {
                        if (pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector->GetAt(i)->Id == eventArgs->AudioDevice->Id)
                        {
                            pHeadset->aLocalUsers[iLocalUserIndex].refAudioDevsVector->RemoveAt(i);
                            pHeadset->bHeadsetChanged = TRUE;
                            break;
                        }
                    }
                }

                NetCritLeave(&pHeadset->crit);
            }
        });
    }

    // return module ref to caller
    return(pHeadset);
}

/*F********************************************************************************/
/*!
    \Function   VoipHeadsetDestroy

    \Description
        Destroy the headset manager.

    \Input *pHeadset    - pointer to headset state

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetDestroy(VoipHeadsetRefT *pHeadset)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    _VoipHeadsetUnregisterKinectAvailabilityChangedHandler(pHeadset);

    NetCritKill(&pHeadset->crit);

    // unlisten for audio device update events
    if (pHeadset->AudioDeviceAddedToken.Value)
    {
        User::AudioDeviceAdded -= pHeadset->AudioDeviceAddedToken;
        pHeadset->AudioDeviceAddedToken.Value = 0;
    }

    if (pHeadset->AudioDeviceChangedToken.Value)
    {
        User::AudioDeviceChanged -= pHeadset->AudioDeviceChangedToken;
        pHeadset->AudioDeviceChangedToken.Value = 0;
    }

    if (pHeadset->AudioDeviceRemovedToken.Value)
    {
        User::AudioDeviceRemoved -= pHeadset->AudioDeviceRemovedToken;
        pHeadset->AudioDeviceRemovedToken.Value = 0;
    }

    _VoipHeadsetUnregisterSessionStateChangedHandler(pHeadset);

    if (pHeadset->bAsyncOpInProgress)
    {
        pHeadset->refAsyncOp->Cancel();
    }

    if (pHeadset->refCurrentChatSessionState != nullptr)
    {
        delete pHeadset->refCurrentChatSessionState;
        pHeadset->refCurrentChatSessionState = nullptr;
    }

    if (pHeadset->refNewChatSessionState != nullptr)
    {
        delete pHeadset->refNewChatSessionState;
        pHeadset->refNewChatSessionState = nullptr;
    }

    if (pHeadset->refAsyncOp != nullptr)
    {
        delete pHeadset->refAsyncOp;
        pHeadset->refAsyncOp = nullptr;
    }

    if (pHeadset->refEncodersMap != nullptr)
    {
        pHeadset->refEncodersMap->Clear();
        pHeadset->refEncodersMap = nullptr;
    }

    if (pHeadset->refDecodersMap != nullptr)
    {
        pHeadset->refDecodersMap->Clear();
        pHeadset->refDecodersMap = nullptr;
    }

    if (pHeadset->refInboundQueuesMap != nullptr)
    {
        pHeadset->refInboundQueuesMap->Clear();
        pHeadset->refInboundQueuesMap = nullptr;
    }

    if (pHeadset->refPreBufStateMap != nullptr)
    {
        pHeadset->refPreBufStateMap->Clear();
        pHeadset->refPreBufStateMap = nullptr;
    }

    _VoipHeadsetDestroyChatSession(pHeadset);

    // dispose of module memory
    VoipMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    DirtyMemFree(pHeadset, VOIP_MEMID, iMemGroup, pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function   VoipReceiveVoiceDataCb

    \Description
        Connectionlist callback to handle receiving a voice packet from a remote peer.

    \Input *pRemoteUsers - user we're receiving the voice data from
    \Input *pMicrInfo    - micr info from inbound packet
    \Input *pPacketData  - pointer to beginning of data in packet payload
    \Input *pUserData    - VoipHeadsetT ref

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetReceiveVoiceDataCb(VoipUserT *pRemoteUsers, VoipMicrInfoT *pMicrInfo, uint8_t *pPacketData, void *pUserData)
{
    VoipUserExT *pRemoteUserEx =  (VoipUserExT *)pRemoteUsers;
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;
    uint32_t uSubPktLen;
    uint32_t uMicrPkt;
    uint32_t uConsoleId;
    int32_t iRemoteSharedUserIndex;
    uint8_t *pRead;
    uint8_t uDeviceId;
    VoipSerializedDeviceT *pSerializedDevice;
    AudioDevInfo ^refRemoteAudioDeviceInfo = ref new AudioDevInfo();

    pRead = pPacketData;
    pRead++; // skip metadata size

    // get console identifier
    uConsoleId = *pRead << 24;
    pRead++;
    uConsoleId |= *pRead << 16;
    pRead++;
    uConsoleId |= *pRead << 8;
    pRead++;
    uConsoleId |= *pRead;
    pRead++;

    pSerializedDevice = (VoipSerializedDeviceT *)pRead;
    VoipDeserializeAudioDevice(pSerializedDevice, uConsoleId, refRemoteAudioDeviceInfo, &uDeviceId);
    pRead += sizeof(VoipSerializedDeviceT);

    // check to see if we have a remote shared user, if yes update its capture source id
    if (pMicrInfo->uUserIndex == VOIP_SHARED_USER_INDEX)
    {
        for (iRemoteSharedUserIndex = 0; iRemoteSharedUserIndex < VOIP_MAX_LOW_LEVEL_CONNS; ++iRemoteSharedUserIndex)
        {
            if (VOIP_SameUser(&pHeadset->aRemoteSharedUsers[iRemoteSharedUserIndex].user, &pRemoteUserEx[VOIP_SHARED_USER_INDEX].user))
            {
                pHeadset->aRemoteSharedUsers[iRemoteSharedUserIndex].CaptureSoureId = refRemoteAudioDeviceInfo->Id;
            }
        }
    }

    #if DIRTYCODE_LOGGING
    if (pMicrInfo->uNumSubPackets > 5)
    {
        NetPrintfVerbose((pHeadset->iDebugLevel, 2, "voipheadsetxboxone: inbound voip packet has more than 5 sub-packets (%d)\n", pMicrInfo->uNumSubPackets));
    }
    #endif

    // save variable-length sub-packets in inbound queue associated with the remote capture source
    for (uMicrPkt = 0; uMicrPkt < pMicrInfo->uNumSubPackets; uMicrPkt++)
    {
        uSubPktLen = *pRead;
        pRead++;  // skip sub-pkt length

        #if VOIPHEADSET_DEBUG
        NetPrintf(("voipheadsetxboxone: enqueueing chat data for remote user %s at index %d\n", &pRemoteUserEx[pMicrInfo->uUserIndex].user.strUserId, pMicrInfo->uUserIndex));
        #endif

        _VoipHeadsetEnqueueInboundSubPacket(pHeadset, (VoipUserT *) &pRemoteUserEx[pMicrInfo->uUserIndex], pMicrInfo->uUserIndex, pRead, uSubPktLen, refRemoteAudioDeviceInfo->Id, pSerializedDevice);
        pRead += uSubPktLen;
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipRegisterUserCb

    \Description
        Connectionlist callback to register a new remote user with the 1st party voice system
        It will also register remote shared users with voipheadset internally

    \Input *pRemoteUser     - user to register
    \Input bRegister        - true=register, false=unregister
    \Input *pUserData       - voipheadset module ref

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetRegisterUserCb(VoipUserT *pRemoteUser, uint32_t bRegister, void *pUserData)
{
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;
    int32_t iRemoteSharedUserIndex;

    // early exit if we have a invalid remote user
    if (VOIP_NullUser(pRemoteUser))
    {
        NetPrintf(("voipheadsetxboxone: can't %s NULL remote talker\n", (bRegister?"register":"unregister")));
        return;
    }

    // check to see if this is a shared remote user
    if (ds_stristr(pRemoteUser->strUserId, "shared") != NULL)
    {
        // if this is a shared remote user add him to the next free spot in the remote shared user array
        for (iRemoteSharedUserIndex = 0; iRemoteSharedUserIndex < VOIP_MAX_LOW_LEVEL_CONNS; ++iRemoteSharedUserIndex)
        {
            if (bRegister)
            {
                if (VOIP_NullUser(&pHeadset->aRemoteSharedUsers[iRemoteSharedUserIndex].user))
                {
                    VOIP_CopyUser(&pHeadset->aRemoteSharedUsers[iRemoteSharedUserIndex].user, pRemoteUser);
                    pHeadset->aRemoteSharedUsers[iRemoteSharedUserIndex].uEnablePlaybackFlag = 0xFFFFFFFF; // enable playback by default
                    pHeadset->aRemoteSharedUsers[iRemoteSharedUserIndex].CaptureSoureId = L"";
                    NetPrintf(("voipheadsetxboxone: registered remote shared talker at remote shared user index %i\n", iRemoteSharedUserIndex));
                    break;
                }
            }
            else
            {
                if (VOIP_SameUser(&pHeadset->aRemoteSharedUsers[iRemoteSharedUserIndex].user, pRemoteUser))\
                {
                    memset(&pHeadset->aRemoteSharedUsers[iRemoteSharedUserIndex], 0, sizeof(pHeadset->aRemoteSharedUsers[iRemoteSharedUserIndex]));
                    NetPrintf(("voipheadsetxboxone: unregistered remote shared talker at remote shared user index %i\n", iRemoteSharedUserIndex));
                    break;
                }
            }
        }
    }
    else
    {
        RemoteUser ^refRemoteUser = ref new RemoteUser(NetHash(pRemoteUser->strUserId));

        // populate remote user object from the serialized remote user data embedded in the VoipUserT
        VoipDeserializeUser(((VoipUserExT *)pRemoteUser)->aSerializedUserBlob, refRemoteUser);

        if (bRegister)
        {
            _VoipHeadsetAddRemoteTalker(pHeadset, refRemoteUser);
        }
        else
        {
            _VoipHeadsetDeleteRemoteTalker(pHeadset, refRemoteUser);
        }
    }
}


/*F********************************************************************************/
/*!
    \Function   VoipHeadsetProcess

    \Description
        Headset process function.

    \Input *pHeadset    - pointer to headset state
    \Input uFrameCount  - current frame count

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetProcess(VoipHeadsetRefT *pHeadset, uint32_t uFrameCount)
{
    if (pHeadset->bKinectAvailabilityChanged || pHeadset->bHeadsetChanged)
    {
        _VoipHeadsetUpdateStatus(pHeadset);
        pHeadset->bKinectAvailabilityChanged = FALSE;
        pHeadset->bHeadsetChanged = FALSE;
    }

    _VoipHeadsetUpdateChatSessionState(pHeadset);

    if ((pHeadset->refCurrentChatSessionState != nullptr) && !pHeadset->bSessionStateRefreshInProgress)
    {
        _VoipHeadsetPollForVoiceData(pHeadset);

        _VoipHeadsetProcessInboundQueues(pHeadset);

        // every 2 seconds, check for new or deleted bystanders
        if (NetTickDiff(NetTick(), pHeadset->uLastBystanderUpdate) > 2000)
        {
            _VoipHeadsetUpdateBystanders(pHeadset);
            pHeadset->uLastBystanderUpdate = NetTick();
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipHeadsetSetVolume

    \Description
        Sets play and record volume.

    \Input *pHeadset    - pointer to headset state
    \Input iPlayVol     - play volume to set
    \Input iRecVol      - record volume to set

    \Notes
        To not set a value, specify it as -1.

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetSetVolume(VoipHeadsetRefT *pHeadset, int32_t iPlayVol, uint32_t iRecVol)
{
    return; // not implemented
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetControl

    \Description
        Control function.

    \Input *pHeadset    - headset module state
    \Input iControl     - control selector
    \Input iValue       - control value
    \Input iValue2      - control value
    \Input *pValue      - control value

    \Output
        int32_t         - selector specific, or -1 if no such selector

    \Notes
        iControl can be one of the following:

        \verbatim
            '+pbk' - enable voice playback for a given remote user (pValue is the remote user VoipUserT).
            '-pbk' - disable voice playback for a given remote user (pValue is the remote user VoipUserT).
            'aloc' - set local user as participating / not participating
            'loop' - enable/disable loopback
            'qual' - set encoding quality
            'rloc' - register/unregister local talker
            'spam' - debug verbosity level
        \endverbatim

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipHeadsetControl(VoipHeadsetRefT *pHeadset, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if ((iControl == '-pbk') || (iControl == '+pbk'))
    {
        int8_t bVoiceEnable = (iControl == '+pbk')? TRUE : FALSE;

        VoipUserT *pRemoteUser;

        if ((pValue != NULL) && (iValue < VOIP_MAXLOCALUSERS_EXTENDED))
        {
            pRemoteUser = (VoipUserT *)pValue;

            // check to see if the remote user is shared
            if (ds_stristr(pRemoteUser->strUserId, "shared") != NULL)
            {
                return (_VoipHeadsetSetRemoteSharedUserVoicePlayback(pHeadset, pRemoteUser, iValue, bVoiceEnable));
            }
            else
            {
               return (_VoipHeadsetSetRemoteUserVoicePlayback(pHeadset, pRemoteUser, iValue, bVoiceEnable));
            }
        }
        else
        {
            NetPrintf(("voipheadsetxboxone: VoipHeadset('%s') invalid arguments\n", bVoiceEnable? "+pbk" : "-pbk"));
            return(-1);
        }
    }
    if (iControl == 'loop')
    {
        pHeadset->bLoopback = iValue;
        return(0);
    }
    if (iControl == 'qual')
    {
        pHeadset->EncodingQual = (Windows::Xbox::Chat::EncodingQuality) iValue;
        return(0);
    }
    if (iControl == 'aloc')
    {
        return(_VoipHeadsetActivateLocalTalker(pHeadset, iValue, iValue2 > 0));
    }
    if (iControl == 'rloc')
    {
        if (iValue2)
        {
            return(_VoipHeadsetAddLocalTalker(pHeadset, iValue, (VoipUserT *)pValue));
        }
        else
        {
            return(_VoipHeadsetDeleteLocalTalker(pHeadset, iValue, (VoipUserT *)pValue));
        }
    }
    #if DIRTYCODE_LOGGING
    if (iControl == 'spam')
    {
        NetPrintf(("voipheadsetxboxone: setting debuglevel=%d\n", iValue));
        pHeadset->iDebugLevel = iValue;
        return(0);
    }
    #endif
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function    VoipHeadsetStatus

    \Description
        Status function.

    \Input *pHeadset    - headset module state
    \Input iSelect      - control selector
    \Input iValue       - selector specific
    \Input *pBuf        - buffer pointer
    \Input iBufSize     - buffer size

    \Output
        int32_t         - selector specific, or -1 if no such selector

    \Notes
        iSelect can be one of the following:

        \verbatim
            'kine' - TRUE if kinect is in set of active capture source, FALSE otherwise
            'ruid' - return TRUE if the given remote user (pBuf) is registered with voipheadset, FALSE if not.
            'supd' - TRUE if chat session update is in progress
            'vlen' - TRUE if voice sub-packet have variable length, FALSE otherwise
        \endverbatim

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipHeadsetStatus(VoipHeadsetRefT *pHeadset, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'kine')
    {
        return(pHeadset->bKinectIsCapturing);
    }
    if (iSelect == 'ruid')
    {
        // check the shared user list first
        int32_t iRemoteUserIndex = 0;
        uint32_t uParticipantIndex = 0;

        for (iRemoteUserIndex = 0; iRemoteUserIndex < VOIP_MAX_LOW_LEVEL_CONNS; ++iRemoteUserIndex)
        {
            if (VOIP_SameUser(&pHeadset->aRemoteSharedUsers[iRemoteUserIndex].user, (VoipUserT *)pBuf))
            {
                return (TRUE);
            }
        }

        // check the regular remote user
        IVectorView<IChatChannel ^> ^refChannelImmutableVector;
        refChannelImmutableVector = pHeadset->refChatSession->Channels->GetView();

        IVectorView<IChatParticipant ^> ^refParticipantImmutableVector;
        refParticipantImmutableVector = refChannelImmutableVector->First()->Current->Participants->GetView();

        for (uParticipantIndex = 0; uParticipantIndex < refParticipantImmutableVector->Size; ++uParticipantIndex)
        {
            VoipUserT voipUser;
            IChatParticipant ^refParticipant = refParticipantImmutableVector->GetAt(uParticipantIndex);
            uint64_t uXuid = _wtoi64(refParticipant->User->XboxUserId->Data());
            RemoteUser ^refRemoteUser = dynamic_cast<RemoteUser ^>(refParticipant->User); 

            ds_snzprintf(voipUser.strUserId, (int32_t)sizeof(voipUser.strUserId), "%llX", uXuid); 
            
            if (refRemoteUser != nullptr)
            {
               // check to see if the xuid match our 
               uXuid = _wtoi64(refParticipant->User->XboxUserId->Data());
               memset(&voipUser, 0, sizeof(voipUser));
               ds_snzprintf(voipUser.strUserId, (int32_t)sizeof(voipUser.strUserId), "%llX", uXuid);

               if (VOIP_SameUser(&voipUser, (VoipUserT *)pBuf))
               {
                   return (TRUE);
               }
            }
        }

        return (FALSE);
    }
    if (iSelect == 'supd')
    {
        return(pHeadset->bSessionStateRefreshInProgress);
    }
    if (iSelect == 'vlen')
    {
        *(uint8_t *)pBuf = TRUE;
        return(0);
    }

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetSpkrCallback

    \Description
        Set speaker output callback.

    \Input *pHeadset    - headset module state
    \Input *pCallback   - what to call when output data is available
    \Input *pUserData   - user data for callback

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetSpkrCallback(VoipHeadsetRefT *pHeadset, VoipSpkrCallbackT *pCallback, void *pUserData)
{
    pHeadset->pSpkrDataCb = pCallback;
    pHeadset->pSpkrCbUserData = pUserData;
}
