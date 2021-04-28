/*H********************************************************************************/
/*!
    \File voipheadsetxboxone.cpp

    \Description
        VoIP headset manager. Wraps Xbox One Game Chat 2 api.

    \Copyright
        Copyright (c) Electronic Arts 2018. ALL RIGHTS RESERVED.

    \Version 05/24/2013 (mclouatre) First Version
    \Version 11/19/2018 (mclouatre) Rebased from Windows::XBox:Chat to MS game chat 2.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include "DirtySDK/platform.h"

// system includes
#include <windows.h>
#include <stdio.h>
#include <wrl.h>
#include <GameChat2.h>
#include <GameChat2Impl.h>
#include <xaudio2.h>

// dirtysock includes
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/voip/voipdef.h"
#include "DirtySDK/voip/voipnarrate.h"
#include "DirtySDK/misc/userlistapi.h"

// voip includes
#include "voippriv.h"
#include "voippacket.h"
#include "voipheadset.h"

using namespace xbox::services::game_chat_2;
using namespace Windows::Xbox::System;

/*** Defines **********************************************************************/
#define VOIPHEADSET_DEBUG           (DIRTYCODE_DEBUG && FALSE)
#define VOIPHEADSET_XUID_SAFE_LEN   (64) 

/*** Macros ***********************************************************************/


/*** Type Definitions *************************************************************/

//!< local user data space
typedef struct
{
    chat_user *pChatUser;       //!< chat user associated with this local user
    uint8_t bParticipating;     //!< TRUE if this user is a participating user FALSE if not
    uint8_t bHeadset;           //!< TRUE if this user has a headset, FALSE if not
    uint8_t uSendSeq;
    uint8_t bStopping;          //!< are we stopping the playing audio?
    VoipNarrateGenderE eGender; //!< gender setting for text to speech, used outside the context of gamechat

    int32_t iAudioLen;          //!< amount of data written to the audio buffer
    uint8_t aAudioData[8*1024]; //!< buffer up to 4k samples (256ms at 16kbps)

    VoipNarrateRefT *pNarrateRef;//!< narrate ref for text to speech, used outside the context of gamechat
    Microsoft::WRL::ComPtr<IXAudio2> pXaudio2;  //!< pointer to the xaudio2 interface
    IXAudio2MasteringVoice *pMasteringVoice;    //!< pointer to the master voice
    IXAudio2SourceVoice *pSourceVoice;          //!< pointer to the source voice
} XONELocalVoipUserT;

//!< remote user data space
typedef struct
{
    game_chat_communication_relationship_flags aRelFromChannels[VOIP_MAXLOCALUSERS];
    game_chat_communication_relationship_flags aRelFromMuting[VOIP_MAXLOCALUSERS];
} XONERemoteVoipUserT;

//! voipheadset module state data
struct VoipHeadsetRefT
{
    int32_t iMemGroup;                    //!< mem group
    void *pMemGroupUserData;              //!< mem group user data

    //! channel info
    int32_t iMaxChannels;

    //! user callback data
    VoipHeadsetMicDataCbT *pMicDataCb;          //!< callback invoked when locally acquired voice data is ready to be transmitted over the network
    VoipHeadsetTextDataCbT *pTextDataCb;        //!< callback invoked when transcribed text from a local orginiator is ready to be transmitted over the network
    VoipHeadsetOpaqueDataCbT *pOpaqueDataCb;    //!< callback invoked when locally generated gamechat2 control packet is ready to be transmitted over the network
    VoipHeadsetStatusCbT *pStatusCb;
    void *pCbUserData;

    VoipHeadsetTranscribedTextReceivedCbT *pTranscribedTextReceivedCb; //!< callback invoked when transcribed text from a remote orginator needs to be surfaced up to voipcommon to get displayed locall
    void *pTranscribedTextReceivedCbUserData;

    //! speaker callback data
    VoipSpkrCallbackT *pSpkrDataCb;
    void *pSpkrCbUserData;

#if DIRTYCODE_LOGGING
    int32_t iDebugLevel;            //!< module debuglevel
    uint32_t uLastChatUsersDump;    //!< last time users where dumped to the log file
#endif

    XONELocalVoipUserT aLocalUsers[VOIP_MAXLOCALUSERS];

    wchar_t *pTTSInputText;         //!< TTS input text
    int32_t iTTSInputLen;
    VoipTextToSpeechMetricsT aTTSMetrics[VOIP_MAXLOCALUSERS];

    /** Not working on XBoxOne **: Because transcribed text in xboxone is an "opaque data frame",
        it is not routed through pTextDataCb(), but through pOpaqueDataCb().
        Because we have no visibility on whether the opaque data is text, or voice or control,
        we cannot implement STTMetrics at this time */
    VoipSpeechToTextMetricsT STTMetrics;

    uint8_t uSendSeq;
    uint8_t bLoopback;
    uint8_t pad[2];
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/
static int32_t VoipHeadsetXboxone_iMemGroup;
static void *VoipHeadsetXboxone_pMemGroupUserData;


/*** Private Functions ************************************************************/

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _VoipHeadsetGetChatIndicatorString

    \Description
        Return debug string for specified chat indicator value.

    \Input ChatIndicator    - chat indicator value

    \Output
        const char *        - matching chat indicator string

    \Version 11/27/2018 (mclouatre)
*/
/********************************************************************************F*/
static const char * _VoipHeadsetGetChatIndicatorString(game_chat_user_chat_indicator ChatIndicator)
{
    switch (ChatIndicator)
    {
        case game_chat_user_chat_indicator::incoming_communications_muted:
            return("COMM_MUTED");

        case game_chat_user_chat_indicator::local_microphone_muted:
            return("MIC_MUTED ");

        case game_chat_user_chat_indicator::no_chat_focus:
            return("NO_FOCUS  ");

        case game_chat_user_chat_indicator::no_microphone:
            return("NO_MIC    ");

        case game_chat_user_chat_indicator::platform_restricted:
            return("PLAT_RESTR");

        case game_chat_user_chat_indicator::reputation_restricted:
            return("REP_RESTR ");

        case game_chat_user_chat_indicator::silent:
            return("SILENT    ");

        case game_chat_user_chat_indicator::talking:
            return("TALKING   ");

        default:
            break;
    }

    return("INVALID   ");
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetGetCommRelAdjusterString

    \Description
        Return debug string for specified communication relationship adjuster

    \Input CommRelAdjuster  - communication relationship adjuster

    \Output
        const char *        - matching adjuster string

    \Version 11/27/2018 (mclouatre)
*/
/********************************************************************************F*/
static const char * _VoipHeadsetGetCommRelAdjusterString(game_chat_communication_relationship_adjuster CommRelAdjuster)
{
    switch (CommRelAdjuster)
    {
        case game_chat_communication_relationship_adjuster::conflict_with_other_user:
            return("CONFLICT ");

        case game_chat_communication_relationship_adjuster::initializing:
            return("INIT     ");

        case game_chat_communication_relationship_adjuster::none:
            return("NONE     ");

        case game_chat_communication_relationship_adjuster::privacy:
            return("PRIVACY  ");

        case game_chat_communication_relationship_adjuster::privilege:
            return("PRIVILEGE");

        case game_chat_communication_relationship_adjuster::reputation:
            return("REPUTATION");

        case game_chat_communication_relationship_adjuster::uninitialized:
            return("UNINIT    ");

        default:
            break;
    }

    return("INVALID   ");
}
#endif

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetDumpChatUsers

    \Description
        Dump user info for all users registered with the chat manager.

    \Version 11/27/2018 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetDumpChatUsers(void)
{
    uint32_t uChatUserIndex;
    uint32_t uChatUserCount;
    chat_user_array aChatUsers;
    chat_user *pChatUser = NULL;

    // get collection of chat users from the chat manager
    chat_manager::singleton_instance().get_chat_users(&uChatUserCount, &aChatUsers);

    NetPrintf(("voipheadsetxboxone: CHAT USERS DUMP  --  %d chat user(s) currently registered with the chat manager:\n", uChatUserCount));

    // look for a chat user with the matching XBoxUserId
    for (uChatUserIndex = 0; uChatUserIndex < uChatUserCount; ++uChatUserIndex)
    {
        pChatUser = aChatUsers[uChatUserIndex];

        NetPrintf(("voipheadsetxboxone: chat user #%d --> local:%s   xuid:%S   chat_indicator:%s\n",
            uChatUserIndex, (pChatUser->local() == NULL) ? "n" : "y", pChatUser->xbox_user_id(),
            _VoipHeadsetGetChatIndicatorString(pChatUser->chat_indicator())));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetGetXboxUserIdForVoipUser

    \Description
        Fills the caller's buffer with the XBoxUserId (wchar_t string) for the 
        specified user.

    \Input *pUser           - user for which the XBoxUserId is queried
    \Input *pXboxUserId     - [out] pointer to be filled in with the XBoxUserId (wchar_t string)
    \Input iXboxUserIdLen   - size of caller's buffer
     
    \Output
        int32_t             - <0:error, >=0: number of bytes written in caller's buffer

    \Version 11/22/2018 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetGetXboxUserIdForVoipUser(VoipUserT *pUser, wchar_t *pXboxUserId, int32_t iXboxUserIdLen)
{
    uint64_t uXuid;

    // read hexadecimal numeric string into temp var
    sscanf(pUser->strUserId, "%I64X", &uXuid);

    // write base-10 numeric string from temp var
    return(_snwprintf(pXboxUserId, iXboxUserIdLen, L"%lld", uXuid)); 
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetGetChatUserForXBoxUserId

    \Description
        Returns the chat_user object that matches the specified XBoxUserID.

    \Input *pXboxUserId     - user for which the chat_user is queried
     
    \Output
       chat_user *          - chat_user pointer, NULL if not found

    \Version 11/22/2018 (mclouatre)
*/
/********************************************************************************F*/
static chat_user * _VoipHeadsetGetChatUserForXBoxUserId(const wchar_t *pXboxUserId)
{
    uint32_t uChatUserIndex;
    uint32_t uChatUserCount;
    chat_user_array aChatUsers;
    chat_user *pChatUser = NULL;

    // get collection of chat users from the chat manager
    chat_manager::singleton_instance().get_chat_users(&uChatUserCount, &aChatUsers);

    // look for a chat user with the matching XBoxUserId
    for (uChatUserIndex = 0; uChatUserIndex < uChatUserCount; ++uChatUserIndex)
    {
        pChatUser = aChatUsers[uChatUserIndex];

        if (wcscmp(pChatUser->xbox_user_id(), pXboxUserId) == 0)
        {
            // found!
            break;
        }
        else
        {
            pChatUser = NULL;
        }
    }

    return(pChatUser);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetGetChatUserForVoipUser

    \Description
        Returns the chat_user object that matches the specifief VoipUser.

    \Input *pUser           - user for which the chat_user is queried
     
    \Output
       chat_user *          - chat_user pointer, NULL if not found

    \Version 11/22/2018 (mclouatre)
*/
/********************************************************************************F*/
static chat_user * _VoipHeadsetGetChatUserForVoipUser(VoipUserT *pUser)
{
    wchar_t wstrXuid[VOIPHEADSET_XUID_SAFE_LEN];

    // get XBoxUserId wchar string for the specified user
    _VoipHeadsetGetXboxUserIdForVoipUser(pUser, wstrXuid, sizeof(wstrXuid));

    return(_VoipHeadsetGetChatUserForXBoxUserId(wstrXuid));
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetGetVoipUserForChatUser

    \Description
        Fills in the voipuser buffer with the xuid of specified chat user.

    \Input *pChatUser       - chat user
    \Input *pVoipUser       - [OUT] VoipUserT to be initialized

    \Version 11/22/2018 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetGetVoipUserForChatUser(chat_user *pChatUser, VoipUserT *pVoipUser)
{
    uint64_t uXuid;

    // read hexadecimal numeric string into temp var
    swscanf(pChatUser->xbox_user_id(), L"%I64d", &uXuid);

    // write base-10 numeric string from temp var
    _snprintf(&pVoipUser->strUserId[0], sizeof(*pVoipUser), "%llX", uXuid);
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
    User ^refXboxUser = nullptr;

    if (VOIP_NullUser(pLocalUser))
    {
        NetPrintf(("voipheadsetxboxone: can't add NULL local talker at local user index %d\n", iLocalUserIndex));
        return(-1);
    }

    // make sure there is no other local talker
    if (pHeadset->aLocalUsers[iLocalUserIndex].pChatUser != NULL)
    {
        NetPrintf(("voipheadsetxboxone: can't add multiple local talkers at the same index (%d)\n", iLocalUserIndex));
        return(-2);
    }

    // find the system local user (winrt ref)
    if (NetConnStatus('xusr', iLocalUserIndex, &refXboxUser, sizeof(refXboxUser)) < 0)
    {
        NetPrintf(("voipheadsetxboxone: no local user at index %d\n", iLocalUserIndex));
        return(-3);
    }

    // see if there is already a matching user registered with the chat manager
    if (_VoipHeadsetGetChatUserForXBoxUserId(refXboxUser->XboxUserId->Data()) == NULL)
    {
        pHeadset->aLocalUsers[iLocalUserIndex].pChatUser = chat_manager::singleton_instance().add_local_user(refXboxUser->XboxUserId->Data());
        NetPrintf(("voipheadsetxboxone: adding local talker %s at index %d (gamertag: %S, xuid: %S)\n", 
            pLocalUser->strUserId, iLocalUserIndex, refXboxUser->DisplayInfo->Gamertag->Data(), refXboxUser->XboxUserId->Data()));
    }
    else
    {
        NetPrintf(("voipheadsetxboxone: local talker %S at local user index %d is already known by the chat manager\n",
            refXboxUser->XboxUserId->Data(), iLocalUserIndex));
        return(-4);
    }

    _VoipHeadsetDumpChatUsers();

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetRemoveLocalTalker

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
static int32_t _VoipHeadsetRemoveLocalTalker(VoipHeadsetRefT *pHeadset, int32_t iLocalUserIndex, VoipUserT *pLocalUser)
{
    int32_t iRetCode = 0;

    if (pHeadset->aLocalUsers[iLocalUserIndex].bParticipating == FALSE)
    {
        User ^refXboxUser = nullptr;

        if (pHeadset->aLocalUsers[iLocalUserIndex].pChatUser != NULL)
        {
            NetPrintf(("voipheadsetxboxone: deleting local talker (xuid: %S) at index %d\n",
                pHeadset->aLocalUsers[iLocalUserIndex].pChatUser->xbox_user_id(), iLocalUserIndex));

            chat_manager::singleton_instance().remove_user(pHeadset->aLocalUsers[iLocalUserIndex].pChatUser);
            pHeadset->aLocalUsers[iLocalUserIndex].pChatUser = FALSE;
        }
        else
        {
            NetPrintf(("voipheadsetxboxone: local talker at local user index %d is already deleted\n", iLocalUserIndex));
            iRetCode = -1;
        }
    }
    else
    {
        NetPrintf(("voipheadsetxboxone: cannot delete local talker (xuid: %S) at local user index %d because the user must first be deactivated\n",
            pHeadset->aLocalUsers[iLocalUserIndex].pChatUser->xbox_user_id(), iLocalUserIndex));
        iRetCode = -2;
    }

    _VoipHeadsetDumpChatUsers();

    return(iRetCode);
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
    User ^refXboxUser = nullptr;

    // find the system local user (winrt ref)
    if (NetConnStatus('xusr', iLocalUserIndex, &refXboxUser, sizeof(refXboxUser)) < 0)
    {
        NetPrintf(("voipheadsetxboxone: no local user at index %d\n", iLocalUserIndex));
        return(-1);
    }

    if ((pHeadset->aLocalUsers[iLocalUserIndex].bParticipating == FALSE) && (bParticipate == FALSE))
    {
        NetPrintf(("voipheadsetxboxone: cannot demote user at local index %d because not already a participating user. Ignoring.\n", iLocalUserIndex));
        return(-2);
    }

    pHeadset->aLocalUsers[iLocalUserIndex].bParticipating = bParticipate;

    NetPrintf(("voipheadsetxboxone: local user (xuid: %S) at index %d is %s participating\n",
        pHeadset->aLocalUsers[iLocalUserIndex].pChatUser->xbox_user_id(), iLocalUserIndex, (bParticipate ? "now" : "no longer")));

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetAddRemoteTalker

    \Description
        Create the game chat participant for this remote user.

    \Input *pHeadset    - headset module
    \Input *pRemoteUser - remote user
    \Input uConsoleId   - unique console identifier (local scope only, does not need to be the same on all hosts)

    \Output
        int32_t         - negative=error, zero=success

    \Version 07/12/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetAddRemoteTalker(VoipHeadsetRefT *pHeadset, VoipUserT *pRemoteUser, uint64_t uConsoleId)
{
    int iRetCode = 0;
    chat_user *pChatUser;

    pChatUser = _VoipHeadsetGetChatUserForVoipUser(pRemoteUser);

    if (pChatUser == NULL)
    {
        wchar_t wstrXuid[VOIPHEADSET_XUID_SAFE_LEN];
        _VoipHeadsetGetXboxUserIdForVoipUser(pRemoteUser, wstrXuid, sizeof(wstrXuid));
        XONERemoteVoipUserT *pXoneRemoteUser;

        if ((pXoneRemoteUser = (XONERemoteVoipUserT *)DirtyMemAlloc(sizeof(XONERemoteVoipUserT), VOIP_MEMID, pHeadset->iMemGroup, pHeadset->pMemGroupUserData)) != NULL)
        {
            NetPrintf(("voipheadsetxboxone: adding remote talker %s (xuid: %S, console id: 0x%llx)\n",
                pRemoteUser->strUserId, wstrXuid, uConsoleId));

            pChatUser = chat_manager::singleton_instance().add_remote_user(wstrXuid, uConsoleId);

            if (pChatUser != NULL)
            {
                pChatUser->set_custom_user_context((void *)pXoneRemoteUser);
            }
            else
            {
                NetPrintf(("voipheadsetxboxone: adding remote talker %s failed because it chat_matanger::add_remote_user() returned NULL\n"));
                DirtyMemFree(pXoneRemoteUser, VOIP_MEMID, pHeadset->iMemGroup, pHeadset->pMemGroupUserData);
                iRetCode = -1;
            }
        }
        else
        {
            NetPrintf(("voipheadsetxboxone: adding remote talker %s failed because out of memory\n", pRemoteUser->strUserId));
            iRetCode = -2;
        }
    }
    else
    {
        NetPrintf(("voipheadsetxboxone: adding remote talker %s failed because it already exists\n", pRemoteUser->strUserId));
        iRetCode = -3;
    }

    _VoipHeadsetDumpChatUsers();

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetRemoveRemoteTalker

    \Description
        Remove the specified remote user from the collection of users known by the chat manager.

    \Input *pHeadset    - headset module
    \Input *pUser       - user to be removed

    \Output
        int32_t         - negative=error, zero=success

    \Version 11/22/2018 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetRemoveRemoteTalker(VoipHeadsetRefT *pHeadset, VoipUserT *pUser)
{
#if DIRTYCODE_LOGGING
    wchar_t wstrXuid[VOIPHEADSET_XUID_SAFE_LEN];
#endif
    chat_user *pChatUser;
    int iRetCode = 0;

    pChatUser = _VoipHeadsetGetChatUserForVoipUser(pUser);

    if (pChatUser != NULL)
    {
        NetPrintf(("voipheadsetxboxone: deleting remote talker (xuid: %S)\n", wstrXuid));

        DirtyMemFree(pChatUser->custom_user_context(), VOIP_MEMID, pHeadset->iMemGroup, pHeadset->pMemGroupUserData);

        chat_manager::singleton_instance().remove_user(pChatUser);
    }
    else
    {
        NetPrintf(("voipheadsetxboxone: deleting remote talker %s failed because it does not exist\n", pUser->strUserId));
        iRetCode = -1;
    }

    _VoipHeadsetDumpChatUsers();

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSynthesizeSpeech

    \Description
        Text-to-speech for the specified user.

    \Input *pHeadset        - headset module
    \Input iLocalUserIndex  - local user index (text originator)
    \Input *pText           - text to by synthesized

    \Output
        int32_t             - 0 if success, negative otherwise

    \Version 04/25/2017 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetSynthesizeSpeech(VoipHeadsetRefT *pHeadset, int32_t iLocalUserIndex, const wchar_t *pText)
{
    int32_t iRetCode = 0; // default to success

    if (pHeadset->aLocalUsers[iLocalUserIndex].pChatUser != NULL)
    {
        if (pHeadset->aLocalUsers[iLocalUserIndex].pChatUser->local()->speech_to_text_conversion_preference_enabled() == false)
        {
            NetPrintf(("voipheadsetxboxone: game chat 2 will not synthesize speech for local talker at local user index %d because it has STT disabled\n", iLocalUserIndex));
        }
        pHeadset->aLocalUsers[iLocalUserIndex].pChatUser->local()->synthesize_text_to_speech(pText);
    }
    else
    {
        NetPrintf(("voipheadsetxboxone: can't synthesize speeck for local talker at local user index %d because no matching chat user found\n", iLocalUserIndex));
        iRetCode = -2;
    }
    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetProcessLoopback

    \Description
        Submit the loopback audio to the output device

    \Input *pHeadset    - pointer to headset state
    \Input *pLocaLuser  - user for which the audio belongs

    \Version 3/15/2019 (eesponda)
*/
/********************************************************************************F*/
static void _VoipHeadsetProcessLoopback(VoipHeadsetRefT *pHeadset, XONELocalVoipUserT *pLocalUser)
{
    XAUDIO2_VOICE_STATE State;
    pLocalUser->pSourceVoice->GetState(&State);
    if (State.BuffersQueued == 0)
    {
        if (pLocalUser->iAudioLen > 0)
        {
            XAUDIO2_BUFFER Buffer = {};
            Buffer.pAudioData = pLocalUser->aAudioData;
            Buffer.AudioBytes = pLocalUser->iAudioLen;
            pLocalUser->pSourceVoice->SubmitSourceBuffer(&Buffer);
            pLocalUser->iAudioLen = 0;
        }
        else if (pLocalUser->bStopping)
        {
            pLocalUser->pSourceVoice->Stop();
            pLocalUser->bStopping = FALSE;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetUpdateStatus

    \Description
        Headset update function.

    \Input *pHeadset    - pointer to headset state

    \Version 11/23/2018 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetUpdateStatus(VoipHeadsetRefT *pHeadset)
{
    int32_t iLocalUserIndex;

    for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; iLocalUserIndex++)
    {
        XONELocalVoipUserT *pLocalUser = &pHeadset->aLocalUsers[iLocalUserIndex];

        if (pLocalUser->pChatUser != NULL)
        {
            uint8_t bHeadsetDetected = (pHeadset->aLocalUsers[iLocalUserIndex].pChatUser->chat_indicator() == game_chat_user_chat_indicator::no_microphone) ? FALSE : TRUE;
            
            if (pHeadset->aLocalUsers[iLocalUserIndex].bHeadset != bHeadsetDetected)
            {
                pHeadset->pStatusCb(iLocalUserIndex, bHeadsetDetected, VOIP_HEADSET_STATUS_INOUT, pHeadset->pCbUserData);
                pHeadset->aLocalUsers[iLocalUserIndex].bHeadset = bHeadsetDetected;
            }
        }
        if (pLocalUser->pNarrateRef != NULL)
        {
            VoipNarrateUpdate(pLocalUser->pNarrateRef);
        }
        if (pLocalUser->pSourceVoice != NULL)
        {
            _VoipHeadsetProcessLoopback(pHeadset, pLocalUser);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetProcessDataFrames

    \Description
        Give a chance to game chat 2 to send 'unreliable' and 'relialbe' data
        over the network to the game chat 2 instances at remote end of voip
        connections.

    \Input *pHeadset    - headset module

    \Version 11/22/2018 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetProcessDataFrames(VoipHeadsetRefT *pHeadset)
{
    uint32_t uDataFrameIndex;
    uint32_t uDataFrameCount;
    game_chat_data_frame_array aDataFrames;

    chat_manager::singleton_instance().start_processing_data_frames(&uDataFrameCount, &aDataFrames);

    for (uDataFrameIndex = 0; uDataFrameIndex < uDataFrameCount; ++uDataFrameIndex)
    {
        uint32_t uTargetEndpointIndex;
        uint32_t uSendMask = 0;
        game_chat_data_frame const* pDataFrame = aDataFrames[uDataFrameIndex];
        uint8_t bReliable = (pDataFrame->transport_requirement == game_chat_data_transport_requirement::guaranteed) ? TRUE : FALSE;

        // build send mask from collection of endpoint identifiers
        for (uTargetEndpointIndex = 0; uTargetEndpointIndex < pDataFrame->target_endpoint_identifier_count; ++uTargetEndpointIndex)
        {
            uSendMask |= (1 << pDataFrame->target_endpoint_identifiers[uTargetEndpointIndex]);
        }

        pHeadset->pOpaqueDataCb(pDataFrame->packet_buffer, pDataFrame->packet_byte_count, uSendMask, bReliable, pHeadset->uSendSeq++, pHeadset->pCbUserData);
    }

    chat_manager::singleton_instance().finish_processing_data_frames(aDataFrames);
}


/*F********************************************************************************/
/*!
    \Function _VoipHeadsetProcessStateChanges

    \Description
        Process notifications from game chat 2's chat manager

    \Input *pHeadset    - headset module

    \Version 11/22/2018 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetProcessStateChanges(VoipHeadsetRefT *pHeadset)
{
    uint32_t uStateChangeIndex;
    uint32_t uStateChangeCount;
    game_chat_state_change_array aGameChatStateChanges;
    chat_manager::singleton_instance().start_processing_state_changes(&uStateChangeCount, &aGameChatStateChanges);

    for (uStateChangeIndex = 0; uStateChangeIndex < uStateChangeCount; ++uStateChangeIndex)
    {
        switch (aGameChatStateChanges[uStateChangeIndex]->state_change_type)
        {
            case game_chat_state_change_type::text_chat_received:
            {   
                #if DIRTYCODE_LOGGING
                const game_chat_text_chat_received_state_change* pStateChange = static_cast<const game_chat_text_chat_received_state_change*>(aGameChatStateChanges[uStateChangeIndex]);
                #endif
                NetPrintf(("voipheadsetxboxone: ignoring player-to-player text communication message from user with xuid %S\n", pStateChange->sender->xbox_user_id()));
                break;
            }

            case game_chat_state_change_type::transcribed_chat_received:
            {
                const game_chat_transcribed_chat_received_state_change* pStateChange = static_cast<const game_chat_transcribed_chat_received_state_change*>(aGameChatStateChanges[uStateChangeIndex]);
                VoipUserT VoipUser;
                char strTranscribedTextUtf8[c_maxChatTextMessageLength * sizeof(wchar_t) + 1];
                int32_t iMbStrLenNeeded;

                iMbStrLenNeeded = WideCharToMultiByte(CP_UTF8, 0, pStateChange->message, -1, NULL, 0, NULL, 0);
                if (iMbStrLenNeeded <= sizeof(strTranscribedTextUtf8))
                {
                    WideCharToMultiByte(CP_UTF8, 0, pStateChange->message, -1, strTranscribedTextUtf8, sizeof(strTranscribedTextUtf8), NULL, 0);
                    _VoipHeadsetGetVoipUserForChatUser(pStateChange->speaker, &VoipUser);
                    pHeadset->pTranscribedTextReceivedCb(&VoipUser, strTranscribedTextUtf8, pHeadset->pTranscribedTextReceivedCbUserData);
                }
                else
                {
                    NetPrintf(("voipheadsetxboxone: transcribed text unexpectedly does not fit in statically allocated buffer during wcstring mbstring converstion\n"));
                }

                break;
            }

            case game_chat_state_change_type::text_conversion_preference_changed:
            {
                #if DIRTYCODE_LOGGING
                const game_chat_text_conversion_preference_changed_state_change* pStateChange = static_cast<const game_chat_text_conversion_preference_changed_state_change*>(aGameChatStateChanges[uStateChangeIndex]);
                #endif
                NetPrintf(("voipheadsetxboxone: game chat 2 notifies a 'text conversion preference' change for user with xuid %S\n", pStateChange->chatUser->xbox_user_id()));
                break;
            }

            case game_chat_state_change_type::communication_relationship_adjuster_changed:
            {
                game_chat_communication_relationship_adjuster CommRelAdj;
                game_chat_communication_relationship_flags EffCommRelFlags;
                const game_chat_communication_relationship_adjuster_changed_state_change* pStateChange = static_cast<const game_chat_communication_relationship_adjuster_changed_state_change*>(aGameChatStateChanges[uStateChangeIndex]);

                pStateChange->local_user->local()->get_effective_communication_relationship(pStateChange->target_user, &EffCommRelFlags, &CommRelAdj);
                NetPrintf(("voipheadsetxboxone: 'comm rel adjuster' event for localXUID %S target XUID %S --> RCV_AUD:%s, RECV_TXT:%s, SND_MIC_AUD:%s, SEND_TEXT:%s, SEND_TTS_AUD:%s | commRelAdj = %s\n",
                    pStateChange->local_user->xbox_user_id(), pStateChange->target_user->xbox_user_id(),
                    ((EffCommRelFlags & game_chat_communication_relationship_flags::receive_audio) == game_chat_communication_relationship_flags::none) ? "n" : "y",
                    ((EffCommRelFlags & game_chat_communication_relationship_flags::receive_text) == game_chat_communication_relationship_flags::none) ? "n" : "y",
                    ((EffCommRelFlags & game_chat_communication_relationship_flags::send_microphone_audio) == game_chat_communication_relationship_flags::none) ? "n" : "y",
                    ((EffCommRelFlags & game_chat_communication_relationship_flags::send_text) == game_chat_communication_relationship_flags::none) ? "n" : "y",
                    ((EffCommRelFlags & game_chat_communication_relationship_flags::send_text_to_speech_audio) == game_chat_communication_relationship_flags::none) ? "n" : "y",
                    _VoipHeadsetGetCommRelAdjusterString(CommRelAdj)));
                break;
            }

            default:
                break;
        }

    }

    chat_manager::singleton_instance().finish_processing_state_changes(aGameChatStateChanges);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetApplyEffectiveRelationship

    \Description
        Set local player to remote player relationship taking into account the 
        per-connection muting (aRelFromMuting) and the channel config (aRelFromChannels).

    \Input *pHeadset            - pointer to headset state
    \Input iLocalUserIndex      - index of local user
    \Input *pRemoteChatUser     - remote user

    \Version 12/18/2018 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetApplyEffectiveRelationship(VoipHeadsetRefT *pHeadset, int32_t iLocalUserIndex, chat_user *pRemoteChatUser)
{
    game_chat_communication_relationship_flags EffCommRelFlags;
    XONERemoteVoipUserT *pXoneRemoteUser = (XONERemoteVoipUserT *)pRemoteChatUser->custom_user_context();

    EffCommRelFlags = pXoneRemoteUser->aRelFromMuting[iLocalUserIndex] & pXoneRemoteUser->aRelFromChannels[iLocalUserIndex];

    pHeadset->aLocalUsers[iLocalUserIndex].pChatUser->local()->set_communication_relationship(pRemoteChatUser, EffCommRelFlags);

    NetPrintf(("voipheadsetxboxone: applying new comm rel for localXUID %S target XUID %S --> RCV_AUD:%s, RECV_TXT:%s, SND_MIC_AUD:%s, SEND_TEXT:%s, SEND_TTS_AUD:%s\n",
        pHeadset->aLocalUsers[iLocalUserIndex].pChatUser->xbox_user_id(), pRemoteChatUser->xbox_user_id(),
        ((EffCommRelFlags & game_chat_communication_relationship_flags::receive_audio) == game_chat_communication_relationship_flags::none) ? "n" : "y",
        ((EffCommRelFlags & game_chat_communication_relationship_flags::receive_text) == game_chat_communication_relationship_flags::none) ? "n" : "y",
        ((EffCommRelFlags & game_chat_communication_relationship_flags::send_microphone_audio) == game_chat_communication_relationship_flags::none) ? "n" : "y",
        ((EffCommRelFlags & game_chat_communication_relationship_flags::send_text) == game_chat_communication_relationship_flags::none) ? "n" : "y",
        ((EffCommRelFlags & game_chat_communication_relationship_flags::send_text_to_speech_audio) == game_chat_communication_relationship_flags::none) ? "n" : "y"));

    return;
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSetRemoteUserVoicePlayback

    \Description
        Helper function to enable or disable playback of voice from a remote user
        for a given local user.

    \Input *pHeadset            - pointer to headset state
    \Input *pRemoteUser         - remote user
    \Input iLocalUserIndex      - local user index
    \Input bEnablePlayback      - TRUE to enable voice playback. FALSE to disable voice playback.

    \Output
        int32_t                 - negative=error, zero=success 

    \Version 12/10/2014 (tcho)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetSetRemoteUserVoicePlayback(VoipHeadsetRefT *pHeadset, VoipUserT *pRemoteUser, int32_t iLocalUserIndex, uint8_t bEnablePlayback)
{
    chat_user *pLocalChatUser = pHeadset->aLocalUsers[iLocalUserIndex].pChatUser;
    chat_user *pRemoteChatUser = _VoipHeadsetGetChatUserForVoipUser(pRemoteUser);

    if ((pLocalChatUser != NULL) && (pRemoteChatUser != NULL))
    {
        XONERemoteVoipUserT *pXoneRemoteUser = (XONERemoteVoipUserT *)pRemoteChatUser->custom_user_context();

        NetPrintf(("voipheadsetxboxone: %s localXUID %S remoteXUID %S\n", (bEnablePlayback ? "+pbk" : "-pbk"), pLocalChatUser->xbox_user_id(), pRemoteChatUser->xbox_user_id()));

        pXoneRemoteUser->aRelFromChannels[iLocalUserIndex] = (bEnablePlayback ? c_communicationRelationshipSendAndReceiveAll : c_communicationRelationshipSendAll);

        _VoipHeadsetApplyEffectiveRelationship(pHeadset, iLocalUserIndex, pRemoteChatUser);
    }
    else
    {
        NetPrintf(("voipheadsetxboxone: failed to change voice playback for local user index %d because chat users can't be found (pLocalChatUser = %p, pRemoteChatUser = %p)\n",
            pLocalChatUser, pRemoteChatUser));
        return(-1);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSetRemoteUserOutboundMute

    \Description
        Helper function to enable or disable outbound mute for a pair of local
        user and remote user.

    \Input *pHeadset            - pointer to headset state
    \Input *pRemoteUser         - remote user
    \Input iLocalUserIndex      - local user index
    \Input bSend                - TRUE to disable outbound mute. FALSE to enable outbound mute.

    \Output
        int32_t                 - negative=error, zero=success 

    \Version 12/18/2018 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetSetRemoteUserOutboundMute(VoipHeadsetRefT *pHeadset, VoipUserT *pRemoteUser, int32_t iLocalUserIndex, uint8_t bSend)
{
    chat_user *pLocalChatUser = pHeadset->aLocalUsers[iLocalUserIndex].pChatUser;
    chat_user *pRemoteChatUser = _VoipHeadsetGetChatUserForVoipUser(pRemoteUser);

    if ((pLocalChatUser != NULL) && (pRemoteChatUser != NULL))
    {
        XONERemoteVoipUserT *pXoneRemoteUser = (XONERemoteVoipUserT *)pRemoteChatUser->custom_user_context();

        NetPrintf(("voipheadsetxboxone: %s localXUID %S remoteXUID %S\n", (bSend ? "+snd" : "-snd"), pLocalChatUser->xbox_user_id(), pRemoteChatUser->xbox_user_id()));

        if (bSend)
        {
            pXoneRemoteUser->aRelFromMuting[iLocalUserIndex] = pXoneRemoteUser->aRelFromMuting[iLocalUserIndex] | c_communicationRelationshipSendAll;
        }
        else
        {
            pXoneRemoteUser->aRelFromMuting[iLocalUserIndex] = pXoneRemoteUser->aRelFromMuting[iLocalUserIndex] & (game_chat_communication_relationship_flags)(~(uint32_t)c_communicationRelationshipSendAll);
        }

        _VoipHeadsetApplyEffectiveRelationship(pHeadset, iLocalUserIndex, pRemoteChatUser);
    }
    else
    {
        NetPrintf(("voipheadsetxboxone: failed to change outbound mute for local user index %d because chat users can't be found (pLocalChatUser = %p, pRemoteChatUser = %p)\n",
            pLocalChatUser, pRemoteChatUser));
        return(-1);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSetRemoteUserInboundMute

    \Description
        Helper function to enable or disable inbound mute for a pair of local
        user and remote user.

    \Input *pHeadset            - pointer to headset state
    \Input *pRemoteUser         - remote user
    \Input iLocalUserIndex      - local user index
    \Input bRecv                - TRUE to disable inbound mute. FALSE to enable inbbound mute.

    \Output
        int32_t                 - negative=error, zero=success 

    \Version 12/18/2018 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetSetRemoteUserInboundMute(VoipHeadsetRefT *pHeadset, VoipUserT *pRemoteUser, int32_t iLocalUserIndex, uint8_t bRecv)
{
    chat_user *pLocalChatUser = pHeadset->aLocalUsers[iLocalUserIndex].pChatUser;
    chat_user *pRemoteChatUser = _VoipHeadsetGetChatUserForVoipUser(pRemoteUser);

    if ((pLocalChatUser != NULL) && (pRemoteChatUser != NULL))
    {
        XONERemoteVoipUserT *pXoneRemoteUser = (XONERemoteVoipUserT *)pRemoteChatUser->custom_user_context();

        NetPrintf(("voipheadsetxboxone: %s localXUID %S remoteXUID %S\n", (bRecv ? "+rcv" : "-rcv"), pLocalChatUser->xbox_user_id(), pRemoteChatUser->xbox_user_id()));

        if (bRecv)
        {
            pXoneRemoteUser->aRelFromMuting[iLocalUserIndex] = pXoneRemoteUser->aRelFromMuting[iLocalUserIndex] | c_communicationRelationshipReceiveAll;
        }
        else
        {
            pXoneRemoteUser->aRelFromMuting[iLocalUserIndex] = pXoneRemoteUser->aRelFromMuting[iLocalUserIndex] & (game_chat_communication_relationship_flags)(~(uint32_t)c_communicationRelationshipReceiveAll);
        }

        _VoipHeadsetApplyEffectiveRelationship(pHeadset, iLocalUserIndex, pRemoteChatUser);
    }
    else
    {
        NetPrintf(("voipheadsetxboxone: failed to change inbound mute for local user index %d because chat users can't be found (pLocalChatUser = %p, pRemoteChatUser = %p)\n",
            pLocalChatUser, pRemoteChatUser));
        return(-1);
    }

    return(0);
}


/*F********************************************************************************/
/*!
    \Function _VoipHeadsetAllocateMemory

    \Description
        Memory allocator registered with game chat 2.

    \Input Size             - the size of the allocation to be made. This value will never be zero.
    \Input uMemoryTypeId    - opaque identifier representing the Game Chat 2 internal category of memory being allocated.

    \Output
        void *              - NULL if failure, pointer to newly allocated memory if success

    \Version 29/11/2018 (mclouatre)
*/
/********************************************************************************F*/
static void * _VoipHeadsetAllocateMemory(size_t Size, uint32_t uMemoryTypeId)
{
    void *pAllocatedMemory;

    // allocate requested memory
    if ((pAllocatedMemory = DirtyMemAlloc((int32_t)Size, VOIP_PLATFORM_MEMID, VoipHeadsetXboxone_iMemGroup, VoipHeadsetXboxone_pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipheadsetxboxone: failed to allocate a buffer of size %s with gamechat2 memory type %s\n", Size, uMemoryTypeId));
    }

    // return the pointer passed the spot reserved to store the memory type id.
    return(pAllocatedMemory);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetFreeMemory

    \Description
        Memory allocator registered with game chat 2.

    \Input *pMemory         - pointer to the memory buffer previously allocated. This value will never be a null pointer
    \Input uMemoryTypeId    - opaque identifier representing the Game chat internal category of memory being freed

    \Version 29/11/2018 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetFreeMemory(void *pMemory, uint32_t uMemoryTypeId)
{
    DirtyMemFree(pMemory, VOIP_PLATFORM_MEMID, VoipHeadsetXboxone_iMemGroup, VoipHeadsetXboxone_pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetNarrateWriteAudio

    \Description
        Write to narrate audio to our audio buffer

    \Input *pLocalUser  - the user that the audio belongs to
    \Input *pBuffer     - audio data we are writing
    \Input iBufLen      - amount of audio data

    \Output
        int32_t         - amount of data written

    \Version 03/15/2019 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetNarrateWriteAudio(XONELocalVoipUserT *pLocalUser, const uint8_t *pBuffer, int32_t iBufLen)
{
    if ((iBufLen = DS_MIN((signed)sizeof(pLocalUser->aAudioData) - pLocalUser->iAudioLen, iBufLen)) > 0)
    {
        ds_memcpy(pLocalUser->aAudioData+pLocalUser->iAudioLen, pBuffer, iBufLen);
        pLocalUser->iAudioLen += iBufLen;
    }
    return(iBufLen);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetNarrateCb

    \Description
        Voice callback from the narrate module

    \Input *pNarrateRef - narrate module
    \Input iUserIndex   - index this voice data belongs to
    \Input *pSamples    - audio data samples
    \Input iSize        - amount of audio data
    \Input *pUserData   - callback user data

    \Output
        int32_t         - amount of data consumed

    \Version 03/15/2019 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetNarrateCb(VoipNarrateRefT *pNarrateRef, int32_t iUserIndex, const int16_t *pSamples, int32_t iSize, void *pUserData)
{
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;
    XONELocalVoipUserT *pLocalUser = &pHeadset->aLocalUsers[iUserIndex];

    // start the playback
    if (iSize == VOIPNARRATE_STREAM_START)
    {
        pLocalUser->pSourceVoice->Start(0, 0);
    }
    // queue the end of the playback
    else if (iSize == VOIPNARRATE_STREAM_END)
    {
        pLocalUser->bStopping = TRUE;
    }
    // if we have no buffers queue start writing data again
    else
    {
        XAUDIO2_VOICE_STATE State;
        pLocalUser->pSourceVoice->GetState(&State);
        iSize = (State.BuffersQueued == 0) ? _VoipHeadsetNarrateWriteAudio(pLocalUser, (const uint8_t *)pSamples, iSize) : 0;
    }
    return(iSize);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetConvertRenderString

    \Description
        Converts the render (output) device id string to one that can be used by xaudio

    \Input *pInput      - the string we are converting
    \Input *pOutput     - [out] the result of the conversion
    \Input iOutputLen   - size of the output

    \Output
        wchar_t *       - result of conversion or NULL if failure

    \Notes
        There are no official docs on this. The knowledge of how this is done was
        taken from research on the Xbox Developer Forums.
        For more information see my post:
            https://forums.xboxlive.com/questions/88438/converting-iaudiodeviceinfo-id-to-device-identifie.html

    \Version 03/15/2019 (eesponda)
*/
/********************************************************************************F*/
static wchar_t *_VoipHeadsetConvertRenderString(const wchar_t *pInput, wchar_t *pOutput, int32_t iOutputLen)
{
    const char *pBegin, *pEnd;
    char strSection[64], strInput[512], strOutput[512] = {0};
    int32_t iOffset = 0, iInputLen;

    // convert to multi-byte for ease of parsing
    iInputLen = (signed)wcstombs(strInput, pInput, sizeof(strInput));

    // attempt to find our controller render device
    if (strstr(strInput, "xboxcontrollerrender") == NULL)
    {
        return(NULL);
    }
    // convert the prefix to the expected format
    iOffset += ds_snzprintf(strOutput+iOffset, sizeof(strOutput)-iOffset, "\\??\\XboxControllerRender#");

    // look for the first delimiter, append to output if found
    if ((pBegin = strchr(strInput, '#')) == NULL)
    {
        return(NULL);
    }
    pEnd = strInput+iInputLen;

    // iterate over each section and process if needed then append to output
    while (ds_strsplit(pBegin, '#', strSection, sizeof(strSection), &pBegin) != 0)
    {
        if ((strstr(strSection, "vid") != NULL) || (strstr(strSection, "pid") != NULL))
        {
            ds_strtoupper(strSection);
        }
        iOffset += ds_snzprintf(strOutput+iOffset, sizeof(strOutput)-iOffset, (pBegin != pEnd) ? "%s#" : "%s", strSection);
    }

    // convert the output to wchar_t and return
    mbstowcs(pOutput, strOutput, iOutputLen);
    return(pOutput);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetCreateNarrate

    \Description
        Create the narrate module for a user

    \Input *pHeadset        - module state
    \Input iLocalUserIndex  - the index of the user we are creating for

    \Output
        int32_t             - zero=success, negative=failure

    \Version 03/15/2019 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetCreateNarrate(VoipHeadsetRefT *pHeadset, int32_t iLocalUserIndex)
{
    User ^refUser = nullptr;
    wchar_t wstrDeviceId[512], *pResult = NULL;
    HRESULT hResult;
    WAVEFORMATEX WaveFormat;
    uint32_t uDeviceIndex, uDeviceMax;
    XONELocalVoipUserT *pLocalUser = &pHeadset->aLocalUsers[iLocalUserIndex];

    // check that we have a user logged in to this index and get its reference
    if (NetConnStatus('xusr', iLocalUserIndex, &refUser, sizeof(refUser)) < 0)
    {
        NetPrintf(("voipheadsetxboxone: no valid user at index %d to create narration for\n", iLocalUserIndex));
        return(-1);
    }
    // if narration already created then nothing left to do
    if (pLocalUser->pNarrateRef != NULL)
    {
        return(0);
    }

    // create the narrate module
    if ((pLocalUser->pNarrateRef = VoipNarrateCreate(_VoipHeadsetNarrateCb, pHeadset)) != NULL)
    {
        // since this is being created late, set the debug level to the current setting
        #if DIRTYCODE_LOGGING
        VoipNarrateControl(pLocalUser->pNarrateRef, 'spam', pHeadset->iDebugLevel, 0, NULL);
        #endif
    }
    else
    {
        NetPrintf(("voipheadsetxboxone: failed to start narration module\n"));
        return(-1);
    }
    VoipNarrateStatus(pLocalUser->pNarrateRef, 'wave', 0, &WaveFormat, sizeof(WaveFormat));

    // create xaudio2 for this user
    if (FAILED(hResult = XAudio2Create(pLocalUser->pXaudio2.GetAddressOf(), 0)))
    {
        NetPrintf(("voipheadsetxboxone: failed to create xaudio2 (%s)", DirtyErrGetName(hResult)));
        return(-1);
    }

    // find the controller headset if we have one. if not found we will use default render device (speakers usually)
    for (uDeviceIndex = 0, uDeviceMax = refUser->AudioDevices->Size; uDeviceIndex < uDeviceMax; uDeviceIndex += 1)
    {
        IAudioDeviceInfo ^refAudioDevice = refUser->AudioDevices->GetAt(uDeviceIndex);
        if ((refAudioDevice->DeviceCategory == AudioDeviceCategory::Communications) &&
            (refAudioDevice->DeviceType == AudioDeviceType::Render) &&
            (refAudioDevice->Sharing == AudioDeviceSharing::Exclusive))
        {
            pResult = _VoipHeadsetConvertRenderString(refAudioDevice->Id->Data(), wstrDeviceId, sizeof(wstrDeviceId) / sizeof(*wstrDeviceId));
            break;
        }
    }

    // attempt to create mastering voice
    if (FAILED(hResult = pLocalUser->pXaudio2->CreateMasteringVoice(&pLocalUser->pMasteringVoice, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, pResult)))
    {
        pLocalUser->pXaudio2 = nullptr;
        NetPrintf(("voipheadsetxboxone: failed to create mastering voice (%s)\n", DirtyErrGetName(hResult)));
        return(-1);
    }
    // attempt to create sourcce voice
    if (FAILED(hResult = pLocalUser->pXaudio2->CreateSourceVoice(&pLocalUser->pSourceVoice, &WaveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO)))
    {
        pLocalUser->pMasteringVoice->DestroyVoice();
        pLocalUser->pMasteringVoice = NULL;
        pLocalUser->pXaudio2 = nullptr;
        NetPrintf(("voipheadsetxboxone: failed to create source voice (%s)\n", DirtyErrGetName(hResult)));
        return(-1);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSynthesizeSpeechLoopback

    \Description
        Text-to-speech for the specified user to be played back locally

    \Input *pHeadset        - headset module
    \Input iLocalUserIndex  - local user index (text originator)
    \Input *pText           - text to by synthesized

    \Output
        int32_t             - 0 if success, negative otherwise

    \Version 03/15/2019 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetSynthesizeSpeechLoopback(VoipHeadsetRefT *pHeadset, int32_t iLocalUserIndex, const char *pText)
{
    XONELocalVoipUserT *pLocalUser;

    // validate index
    if ((iLocalUserIndex < 0) || (iLocalUserIndex >= VOIP_MAXLOCALUSERS))
    {
        return(-1);
    }
    pLocalUser = &pHeadset->aLocalUsers[iLocalUserIndex];

    // create narration module if needed and submit text to be synthesized
    if (_VoipHeadsetCreateNarrate(pHeadset, iLocalUserIndex) == 0)
    {
        return(VoipNarrateInput(pLocalUser->pNarrateRef, iLocalUserIndex, pLocalUser->eGender, pText));
    }
    else
    {
        NetPrintf(("voipheadsetxboxone: failed to start narration module\n"));
        return(-1);
    }
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function   VoipHeadsetCreate

    \Description
        Create the headset manager.

    \Input iMaxChannels     - max number of remote users
    \Input *pMicDataCb      - pointer to user callback to trigger when mic data is ready
    \Input *pTextDataCb     - pointer to user callback to trigger when transcribed text is ready
    \Input *pOpaqueDataCb   - pointer to user callback to trigger when opaque data is ready
    \Input *pStatusCb       - pointer to user callback to trigger when headset status changes
    \Input *pCbUserData     - pointer to user callback data
    \Input iData            - platform-specific - use to specify the game chat 2 threads affinity on XBoxONe

    \Output
        VoipHeadsetRefT *   - pointer to module state, or NULL if an error occured

    \Notes
        \verbatim
            Valid value for iData:
            The higher 16 bit is the CPU affinity value for the network processing. The lower 16 bits is 
            the CPU affinity value for the audio processing.
            For example iDate = (iNetworkCPU << 16) | iAudioCPU.
        \endverbatim

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
VoipHeadsetRefT *VoipHeadsetCreate(int32_t iMaxChannels, VoipHeadsetMicDataCbT *pMicDataCb, VoipHeadsetTextDataCbT *pTextDataCb, VoipHeadsetOpaqueDataCbT *pOpaqueDataCb, VoipHeadsetStatusCbT *pStatusCb, void *pCbUserData, int32_t iData)
{
    VoipHeadsetRefT *pHeadset;
    int32_t iMemGroup;
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
    ds_memclr(pHeadset, sizeof(*pHeadset));

    // set default debuglevel
    #if DIRTYCODE_LOGGING
    pHeadset->iDebugLevel = 1;
    #endif

    // save callback info
    pHeadset->pMicDataCb = pMicDataCb;
    pHeadset->pTextDataCb = pTextDataCb;
    pHeadset->pOpaqueDataCb = pOpaqueDataCb;
    pHeadset->pStatusCb = pStatusCb;
    pHeadset->pCbUserData = pCbUserData;
    pHeadset->iMaxChannels = iMaxChannels;
    pHeadset->iMemGroup = VoipHeadsetXboxone_iMemGroup = iMemGroup;
    pHeadset->pMemGroupUserData = VoipHeadsetXboxone_pMemGroupUserData = pMemGroupUserData;


    // applying custom game chat 2 thread affinity if specified
    if (iData != 0)
    {
        chat_manager::set_thread_processor(game_chat_thread_id::audio, iData & 0xFFFF);
        chat_manager::set_thread_processor(game_chat_thread_id::networking, iData >> 16);
    }

    chat_manager::set_memory_callbacks(
        (game_chat_allocate_memory_callback)_VoipHeadsetAllocateMemory,
        (game_chat_free_memory_callback)_VoipHeadsetFreeMemory);

    chat_manager::singleton_instance().initialize(
        iMaxChannels,
        1.0F, 
        c_communicationRelationshipSendAndReceiveAll,
        game_chat_shared_device_communication_relationship_resolution_mode::restrictive,    // aligns with DS default shared channel config behavior
        game_chat_speech_to_text_conversion_mode::automatic,
        game_chat_audio_manipulation_mode_flags::none);

    // set the default bitrate to 'medium/normal' quality
    chat_manager::singleton_instance().set_audio_encoding_type_and_bitrate(game_chat_audio_encoding_type_and_bitrate::silk_16_kilobits_per_second);

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
    int32_t iLocalUserIndex;

    // release all user resources
    for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; iLocalUserIndex += 1)
    {
        XONELocalVoipUserT *pLocalUser = &pHeadset->aLocalUsers[iLocalUserIndex];

        // narrate module
        if (pLocalUser->pNarrateRef != NULL)
        {
            VoipNarrateDestroy(pLocalUser->pNarrateRef);
            pLocalUser->pNarrateRef = NULL;
        }
        // xaudio modules
        if (pLocalUser->pSourceVoice != NULL)
        {
            pLocalUser->pSourceVoice->DestroyVoice();
            pLocalUser->pSourceVoice = NULL;
        }
        if (pLocalUser->pMasteringVoice != NULL)
        {
            pLocalUser->pMasteringVoice->DestroyVoice();
            pLocalUser->pMasteringVoice = NULL;
        }
        pLocalUser->pXaudio2 = nullptr;
    }
    
    if (pHeadset->pTTSInputText)
    {
        DirtyMemFree(pHeadset->pTTSInputText, VOIP_MEMID, pHeadset->iMemGroup, pHeadset->pMemGroupUserData);
    }

    chat_manager::singleton_instance().cleanup();

    // dispose of module memory
    NetPrintf(("voipheadsetxboxone: [%p] destroy complete\n", pHeadset));
    DirtyMemFree(pHeadset, VOIP_MEMID, pHeadset->iMemGroup, pHeadset->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function   VoipHeadsetReceiveVoiceDataCb

    \Description
        Connectionlist callback to handle receiving an unreliable opaque packet from a remote peer.

    \Input *pRemoteUsers - user we're receiving the voice data from
    \Input iConsoleId    - generic identifier for the console to which the users belong
    \Input *pMicrInfo    - micr info from inbound packet
    \Input *pPacketData  - pointer to beginning of data in packet payload
    \Input *pUserData    - VoipHeadsetT ref

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetReceiveVoiceDataCb(VoipUserT *pRemoteUsers, int32_t iConsoleId, VoipMicrInfoT *pMicrInfo, uint8_t *pPacketData, void *pUserData)
{
    uint32_t uSubPktLen;
    uint32_t uMicrPkt;
    uint8_t *pRead = pPacketData;

    #if DIRTYCODE_LOGGING
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;
    #endif

    // each sub-packet is an unreliabe gamechat2 data frame to be delivered to the local instance of gamechat2
    for (uMicrPkt = 0; uMicrPkt < pMicrInfo->uNumSubPackets; uMicrPkt++)
    {
        uSubPktLen = *pRead;
        pRead++;  // skip sub-pkt length

        NetPrintfVerbose((pHeadset->iDebugLevel, 2, "voipheadsetxboxone: chat_manager::process_incoming_data(%d) - unreliable data\n", uSubPktLen));

        chat_manager::singleton_instance().process_incoming_data((uint64_t)iConsoleId, uSubPktLen, pRead);

        pRead += uSubPktLen;
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipHeadsetReceiveOpaqueDataCb

    \Description
        Connectionlist callback to handle receiving a reliable opaque packet from a remote peer.

    \Input iConsoleId       - generic identifier for the console to which the users belong
    \Input *pOpaqueData     - pointer to opaque data
    \Input iOpaqueDataSize  - opaque data size
    \Input *pUserData       - VoipHeadsetT ref

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetReceiveOpaqueDataCb(int32_t iConsoleId, const uint8_t *pOpaqueData, int32_t iOpaqueDataSize, void *pUserData)
{
    #if DIRTYCODE_LOGGING
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;

    NetPrintfVerbose((pHeadset->iDebugLevel, 2, "voipheadsetxboxone: chat_manager::process_incoming_data(%d) - reliable data\n", iOpaqueDataSize));
    #endif

    chat_manager::singleton_instance().process_incoming_data((uint64_t)iConsoleId, iOpaqueDataSize, pOpaqueData);
}

/*F********************************************************************************/
/*!
    \Function   VoipHeadsetRegisterUserCb

    \Description
        Connectionlist callback to register a new remote user with the 1st party voice system
        It will also register remote shared users with voipheadset internally

    \Input *pRemoteUser     - user to register
    \Input iConsoleId       - generic identifier for the console to which the user belongs
    \Input bRegister        - true=register, false=unregister
    \Input *pUserData       - voipheadset module ref

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetRegisterUserCb(VoipUserT *pRemoteUser, int32_t iConsoleId, uint32_t bRegister, void *pUserData)
{
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;

    // ignore shared users... this is all taken care of by game chat 2 now
    if (ds_stristr(pRemoteUser->strUserId, "shared") != NULL)
    {
        return;
    }

    if (bRegister)
    {
        _VoipHeadsetAddRemoteTalker(pHeadset, pRemoteUser, (uint64_t)iConsoleId);
    }
    else
    {
        _VoipHeadsetRemoveRemoteTalker(pHeadset, pRemoteUser);
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
    _VoipHeadsetUpdateStatus(pHeadset);

    _VoipHeadsetProcessDataFrames(pHeadset);

    _VoipHeadsetProcessStateChanges(pHeadset);

    #if DIRTYCODE_LOGGING
    if (pHeadset->iDebugLevel > 3)
    {
        if ((pHeadset->uLastChatUsersDump == 0) || (NetTickDiff(NetTick(), pHeadset->uLastChatUsersDump) > 5000))
        {
            _VoipHeadsetDumpChatUsers();
            pHeadset->uLastChatUsersDump = NetTick();
        }
    }
    #endif
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
            '+pbk' - enable voice playback for a given remote user (pValue is the remote user VoipUserT).  - maps to voip per-user channel feature
            '-pbk' - disable voice playback for a given remote user (pValue is the remote user VoipUserT).  - maps to voip per-user channel feature
            '+rcv' - enable sending voice to a remote user (pValue is the remote user VoipUserT).  - maps to voip per-connection muting feature
            '-rcv' - disable sending voice to a remote user (pValue is the remote user VoipUserT).  - maps to voip per-connection muting feature
            '+snd' - enable receiving voice from a remote user (pValue is the remote user VoipUserT).  - maps to voip per-connection muting feature
            '-snd' - disable receiving voice from a remote user (pValue is the remote user VoipUserT).  - maps to voip per-connection muting feature
            'aloc' - set local user as participating / not participating
            'loop' - enable/disable loopback mode (NOT SUPPORTED WITH MS GAME CHAT 2)
            'qual' - set encoding quality (iValue must map to one of the enum value of game_chat_audio_encoding_type_and_bitrate)
            'rloc' - register/unregister local talker
            'spam' - debug verbosity level
            'ttos' - text-to-speech: text buffer (wchar_t) received as input is passed to the synthesizer for the specified speech impaired user
        \endverbatim

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipHeadsetControl(VoipHeadsetRefT *pHeadset, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    XONELocalVoipUserT *pLocalUser;

    if ((iControl == '-pbk') || (iControl == '+pbk'))
    {
        int8_t bVoiceEnable = (iControl == '+pbk') ? TRUE : FALSE;
        int32_t iRetCode = 0; // default to success

        VoipUserT *pRemoteUser;

        if ((pValue != NULL) && (iValue < VOIP_MAXLOCALUSERS_EXTENDED))
        {
            pRemoteUser = (VoipUserT *)pValue;

            // make sure the local user and the remote user are not a shared user (shared user concept not supported on xboxone)
            if ((iValue != VOIP_SHARED_USER_INDEX) && (ds_stristr(pRemoteUser->strUserId, "shared") == NULL))
            {
                _VoipHeadsetSetRemoteUserVoicePlayback(pHeadset, pRemoteUser, iValue, bVoiceEnable);
            }
        }
        else
        {
            NetPrintf(("voipheadsetxboxone: VoipHeadsetControl('%C', %d) invalid arguments\n", iControl, iValue));
            iRetCode = -2;
        }
        return(iRetCode);
    }
    if ((iControl == '-snd') || (iControl == '+snd'))
    {
        int8_t bSend = (iControl == '+snd') ? TRUE : FALSE;
        int32_t iRetCode = 0; // default to success

        VoipUserT *pRemoteUser;

        if ((pValue != NULL) && (iValue < VOIP_MAXLOCALUSERS_EXTENDED))
        {
            pRemoteUser = (VoipUserT *)pValue;

            // make sure the local user and the remote user are not a shared user (shared user concept not supported on xboxone)
            if ((iValue != VOIP_SHARED_USER_INDEX) && (ds_stristr(pRemoteUser->strUserId, "shared") == NULL))
            {
                _VoipHeadsetSetRemoteUserOutboundMute(pHeadset, pRemoteUser, iValue, bSend);
            }
        }
        else
        {
            NetPrintf(("voipheadsetxboxone: VoipHeadsetControl('%C', %d) invalid arguments\n", iControl, iValue));
            iRetCode = -2;
        }
        return(iRetCode);
    }
    if ((iControl == '-rcv') || (iControl == '+rcv'))
    {
        int8_t bRcv = (iControl == '+rcv') ? TRUE : FALSE;
        int32_t iRetCode = 0; // default to success

        VoipUserT *pRemoteUser;

        if ((pValue != NULL) && (iValue < VOIP_MAXLOCALUSERS_EXTENDED))
        {
            pRemoteUser = (VoipUserT *)pValue;

            // make sure the local user and the remote user are not a shared user (shared user concept not supported on xboxone)
            if ((iValue != VOIP_SHARED_USER_INDEX) && (ds_stristr(pRemoteUser->strUserId, "shared") == NULL))
            {
                _VoipHeadsetSetRemoteUserInboundMute(pHeadset, pRemoteUser, iValue, bRcv);
            }
        }
        else
        {
            NetPrintf(("voipheadsetxboxone: VoipHeadsetControl('%C', %d) invalid arguments\n", iControl, iValue));
            iRetCode = -2;
        }
        return(iRetCode);
    }
    if (iControl == 'aloc')
    {
        return(_VoipHeadsetActivateLocalTalker(pHeadset, iValue, iValue2 > 0));
    }
    if (iControl == 'loop')
    {
        /* mclouatre dec 2018
           I tried exercising loopback by setting up a comm relationship between a local user and himself.
           Game Chat 2 complained with:
                GC2: Fatal Error: ChatUserSetCommunicationRelationship failed
                Game Chat Fatal Error in xbox::services::game_chat_2::chat_user::chat_user_local::set_communication_relationship - Error code: 0x80070057
                - The setting cannot be configured or retrieved for a chat user with itself as the target input.
        */

        // enable loopback mode which can only be used for local tts (not with gamechat2)
        pHeadset->bLoopback = iValue;
        NetPrintf(("voipheadsetxboxone: loopback mode %s\n", (iValue ? "enabled" : "disabled")));
        return(0);
    }
    if (iControl == 'qual')
    {
        chat_manager::singleton_instance().set_audio_encoding_type_and_bitrate((game_chat_audio_encoding_type_and_bitrate)iValue);
        return(0);
    }

    if (iControl == 'rloc')
    {
        if (iValue2)
        {
            return(_VoipHeadsetAddLocalTalker(pHeadset, iValue, (VoipUserT *)pValue));
        }
        else
        {
            return(_VoipHeadsetRemoveLocalTalker(pHeadset, iValue, (VoipUserT *)pValue));
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
    if (iControl == 'ttos')
    {
        if (!pHeadset->bLoopback)
        {
            int32_t iStringLength;
            int32_t iResult = 0;
            int32_t iSizeNeeded = MultiByteToWideChar(CP_UTF8, 0, (char *)pValue, -1, NULL, 0);

            if (pHeadset->iTTSInputLen < iSizeNeeded)
            {
                if (pHeadset->pTTSInputText != NULL)
                {
                    DirtyMemFree(pHeadset->pTTSInputText, VOIP_MEMID, pHeadset->iMemGroup, pHeadset->pMemGroupUserData);
                    pHeadset->pTTSInputText = NULL;
                }

                if ((pHeadset->pTTSInputText = (wchar_t *)DirtyMemAlloc((int32_t)(sizeof(wchar_t) * iSizeNeeded), VOIP_MEMID, pHeadset->iMemGroup, pHeadset->pMemGroupUserData)) == NULL)
                {
                    NetPrintf(("voipheadsetxboxone: VoipHeadsetControl('ttos' cannot allocate ttos text!\n"));
                    return(-1);
                }

                pHeadset->iTTSInputLen = iSizeNeeded;
            }

            iStringLength = MultiByteToWideChar(CP_UTF8, 0, (char *)pValue, -1, pHeadset->pTTSInputText, iSizeNeeded);
            iResult = _VoipHeadsetSynthesizeSpeech(pHeadset, iValue, (const wchar_t *)pHeadset->pTTSInputText);

            // gather metrics
            // note on xone we have no way of getting; uDurationMsRecv, uEmptyResultCount
            pHeadset->aTTSMetrics[iValue].uEventCount++;
            pHeadset->aTTSMetrics[iValue].uCharCountSent += iStringLength;
            if (iResult != 0)
            {
                pHeadset->aTTSMetrics[iValue].uErrorCount++;
            }

            return(iResult);
        }
        else
        {
            return(_VoipHeadsetSynthesizeSpeechLoopback(pHeadset, iValue, (const char *)pValue));
        }
    }
    if ((iControl == 'voic') && (iValue < VOIP_MAXLOCALUSERS) && (pValue != NULL))
    {
        const VoipSynthesizedSpeechCfgT *pCfg = (const VoipSynthesizedSpeechCfgT *)pValue;
        pLocalUser = &pHeadset->aLocalUsers[iValue];
        pLocalUser->eGender = (pCfg->iPersonaGender == 1) ? VOIPNARRATE_GENDER_FEMALE : VOIPNARRATE_GENDER_MALE;

        // create the narrate module
        return(_VoipHeadsetCreateNarrate(pHeadset, iValue));
    }
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
            'ruid' - return TRUE if the given remote user (pBuf) is registered with voipheadset, FALSE if not.
            'talk' - return TRUE if the specified local talker (iValue) is talking or the specified remote talker (pBuf) is talking, FALSE otherwise
            'sttm' - get the VoipSpeechToTextMetricsT via pBuf, user index is iValue
            'ttsm' - get the VoipTextToSpeechMetricsT via pBuf, user index is iValue 
            'vlen' - TRUE if voice sub-packets have variable length, FALSE otherwise
        \endverbatim

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipHeadsetStatus(VoipHeadsetRefT *pHeadset, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'ruid')
    {
        VoipUserT *pRemoteUser = (VoipUserT *)pBuf;

        // ignore shared users... this is all taken care of by game chat 2 now
        if (ds_stristr(pRemoteUser->strUserId, "shared") != NULL)
        {
            return(FALSE);
        }

        chat_user * pChatUser = _VoipHeadsetGetChatUserForVoipUser(pRemoteUser);

        // if the chat user is valid and is a remote user, then return TRUE
        if ((pChatUser != NULL) && (pChatUser->local() == NULL))
        {
            return(TRUE);
        }
        return(FALSE);
    }
    if (iSelect == 'talk')
    {
        uint32_t bTalking = FALSE;  // default to FALSE
        chat_user *pChatUser = NULL;

        // local user or remote user ?
        if (pBuf == NULL)
        {
            pChatUser = pHeadset->aLocalUsers[iValue].pChatUser;
        }
        else
        {
            VoipUserT *pVoipUser = (VoipUserT *)pBuf;
            pChatUser = _VoipHeadsetGetChatUserForVoipUser(pVoipUser);
        }

        if (pChatUser)
        {
            bTalking = (pChatUser->chat_indicator() == game_chat_user_chat_indicator::talking) ? TRUE : FALSE;
        }

        return(bTalking);
    }
    if (iSelect == 'sttm')
    {
        if ((pBuf != NULL) && (iBufSize >= sizeof(VoipSpeechToTextMetricsT)))
        {
            ds_memcpy_s(pBuf, iBufSize, &pHeadset->STTMetrics, sizeof(VoipSpeechToTextMetricsT));
            return(0);
        }
        return(-1);
    }
    if (iSelect == 'ttsm')
    {
        if ((pBuf != NULL) && (iBufSize >= sizeof(VoipTextToSpeechMetricsT)))
        {
            ds_memcpy_s(pBuf, iBufSize, &pHeadset->aTTSMetrics[iValue], sizeof(VoipTextToSpeechMetricsT));
            return(0);
        }
        return(-1);
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

/*F********************************************************************************/
/*!
    \Function VoipHeadsetSetTranscribedTextReceivedCallback

    \Description
        Set speaker output callback.

    \Input *pHeadset    - headset module state
    \Input *pCallback   - what to call when transcribed text is received from remote player
    \Input *pUserData   - user data for callback

    \Version 11/28/2018 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetSetTranscribedTextReceivedCallback(VoipHeadsetRefT *pHeadset, VoipHeadsetTranscribedTextReceivedCbT *pCallback, void *pUserData)
{
    pHeadset->pTranscribedTextReceivedCb = pCallback;
    pHeadset->pTranscribedTextReceivedCbUserData = pUserData;
}

