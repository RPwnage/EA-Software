#include "Ignition/voip/VoipConfig.h"
#include "EAStdC/EASprintf.h"

#include "DirtySDK/voip/voipcodec.h"

#if defined(EA_PLATFORM_WINDOWS)
#include "voipaux/voipspeex.h"
#endif

#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_STADIA)
#include "voipaux/voipopus.h"
#endif

#if defined(EA_PLATFORM_WINDOWS)
enum VoipCodec
{
    VOIPCODEC_DVI,
    VOIPCODEC_PCM,
    VOIPCODEC_SPEEX,
    VOIPCODEC_OPUS
};

namespace Pyro
{
    template<>
    class PyroCustomEnumValues<VoipCodec> : public EnumNameValueCollectionT<VoipCodec>
    {
    public:
        static const PyroCustomEnumValues<VoipCodec> &getInstance()
        {
            static PyroCustomEnumValues<VoipCodec> instance;
            return instance;
        }

    private:
        PyroCustomEnumValues()
            : EnumNameValueCollectionT("VoipCodec")
        {
            add("DVI", VoipCodec::VOIPCODEC_DVI);
            add("PCM", VoipCodec::VOIPCODEC_PCM);
            add("Speex", VoipCodec::VOIPCODEC_SPEEX);
            add("Opus", VoipCodec::VOIPCODEC_OPUS);
        }
    };
}
#endif

static int32_t _VoipConfigDisplayTranscribedTextCallback(int32_t iConnId, int32_t iUserIndex, const char *pText, void *pUserData)
{
    Ignition::VoipConfig *pVoipConfig = (Ignition::VoipConfig *)pUserData;
    pVoipConfig->getUiDriver()->addLog_va("[STT] ", Pyro::ErrorLevel::NONE, " %s user[%d] \"%s\"", ((iConnId != -1) ? "remote" : "local"), iUserIndex, pText);
    return(0);
}

namespace Ignition
{
VoipConfig::VoipConfig():
    BlazeHubUiBuilder("Local Voip Config"),
    mShowVoipConfig(false),
    mVoipGroup(nullptr)
{
    int32_t iUserIndex, iChannelSlot;

    for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS; iUserIndex++)
    {
        for (iChannelSlot = 0; iChannelSlot < VoipGroupStatus(nullptr, 'chnc', 0, nullptr, 0) && iChannelSlot < VOIPCONFIG_MAX_SLOT; iChannelSlot++)
        {
            mChannelMode[iUserIndex][iChannelSlot] = VOIP_CHANNONE;
            mChannelId[iUserIndex][iChannelSlot] = 0;
        }
    }
    setVisibility(Pyro::ItemVisibility::ADVANCED);

    getActions().add(&getActionSelectChannel());
    getActions().add(&getActionResetChannels());

#if defined(EA_PLATFORM_PS4)
    getActions().add(&getActionAddMostRestrictiveVoipContributor());
    getActions().add(&getActionDelMostRestrictiveVoipContributor());
#endif

    getActions().add(&getActionSuspendPartyChat());
    getActions().add(&getActionResumePartyChat());

    getActions().add(&getActionToggleLoopback());
    getActions().add(&getActionTextChatNarrate());
    getActions().add(&getActionConvertTextToSpeech());
    getActions().add(&getActionConvertSpeechToText());

#if defined(EA_PLATFORM_WINDOWS)
    getActions().add(&getActionSelectCodec());
#endif

    memset(mLoopback, 0, sizeof(mLoopback));
    memset(mSpeechToTextConversion, 0, sizeof(mSpeechToTextConversion));

    NetConnIdleAdd(onIdle, this);
    mShowVoipConfig = false;
}

VoipConfig::~VoipConfig()
{
    NetConnIdleDel(onIdle, this);
}

void VoipConfig::onAuthenticatedPrimaryUser()
{
    setIsVisibleForAll(false);
    if (VoipGetRef())
    {
        VoipResetChannels(VoipGetRef(), static_cast<int32_t>(gBlazeHub->getPrimaryLocalUserIndex()));
    }
    else
    {
        // voip inactive
        return;
    }

    getActionSelectChannel().setIsVisible(true);
    getActionSelectChannel().getParameters().addInt32("channelId", 0, "Channel ID (integer between 0 and 63 inclusively)", "", "", Pyro::ItemVisibility::ADVANCED);

#if defined(EA_PLATFORM_PS4)
    getActionSelectChannel().getParameters().addInt32("userIndex", 0, "User Index", "0,1,2,3,4", "PS4 User Index (4 = shared user)", Pyro::ItemVisibility::ADVANCED);
    getActionSelectChannel().getParameters().addBool("useDefaultSharedConfig", true, "Use default shared channel config", "Use default shared channel config", Pyro::ItemVisibility::ADVANCED);
    getActionResetChannels().getParameters().addInt32("userIndex", 0, "User Index", "0,1,2,3,4", "PS4 User Index (4 = shared user)", Pyro::ItemVisibility::ADVANCED);
#elif defined (EA_PLATFORM_XBOXONE)
    getActionSelectChannel().getParameters().addInt32("userIndex", 0, "User Index", "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16", "Xbox One User Index (16 = shared user)", Pyro::ItemVisibility::ADVANCED);
    getActionSelectChannel().getParameters().addBool("useDefaultSharedConfig", true, "Use default shared channel config", "Use default shared channel config", Pyro::ItemVisibility::ADVANCED);
    getActionResetChannels().getParameters().addInt32("userIndex", 0, "User Index", "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16", "Xbox One User Index (16 = shared user)", Pyro::ItemVisibility::ADVANCED);
#else
    getActionSelectChannel().getParameters().addInt32("userIndex", 0, "User Index", "0,1,2,3", "User Index", Pyro::ItemVisibility::ADVANCED);
    getActionResetChannels().getParameters().addInt32("userIndex", 0, "User Index", "0,1,2,3", "User Index", Pyro::ItemVisibility::ADVANCED);
#endif
    
    getActionSelectChannel().getParameters().addString("channelMode", "TALK+LISTEN", "Operation mode for this channel", "TALK+LISTEN,TALK-ONLY,LISTEN-ONLY,UNSUBSCRIBE", "", Pyro::ItemVisibility::ADVANCED);
    getActionResetChannels().setIsVisible(true);

#if defined(EA_PLATFORM_PS4)
    getActionAddMostRestrictiveVoipContributor().setIsVisible(true);
    getActionAddMostRestrictiveVoipContributor().getParameters().addInt32("userIndex", 0, "User Index", "0,1,2,3", "PS4 User Index", Pyro::ItemVisibility::ADVANCED);
    getActionDelMostRestrictiveVoipContributor().setIsVisible(true);
    getActionDelMostRestrictiveVoipContributor().getParameters().addInt32("userIndex", 0, "User Index", "0,1,2,3", "PS4 User Index", Pyro::ItemVisibility::ADVANCED);
#endif

#if defined(EA_PLATFORM_XBOXONE)
    getActionSuspendPartyChat().setIsVisible(true);
    getActionResumePartyChat().setIsVisible(true);
#else
    getActionSuspendPartyChat().setIsVisible(false);
    getActionResumePartyChat().setIsVisible(false);
#endif

    getActionTextChatNarrate().setIsVisible(true);
    getActionTextChatNarrate().getParameters().addInt32("userIndex", 0, "User Index", "0,1,2,3", "User Index", Pyro::ItemVisibility::ADVANCED);
    getActionTextChatNarrate().getParameters().addString("text", "", "Text", "", "Text to Convert", Pyro::ItemVisibility::ADVANCED);

    getActionToggleLoopback().setIsVisible(true);
    getActionToggleLoopback().getParameters().addInt32("userIndex", 0, "User Index", "0,1,2,3", "User Index", Pyro::ItemVisibility::ADVANCED);

    getActionConvertTextToSpeech().setIsVisible(true);
    getActionConvertTextToSpeech().getParameters().addString("text", "", "Text", "", "Text to Convert", Pyro::ItemVisibility::ADVANCED);
    getActionConvertTextToSpeech().getParameters().addInt32("gender", 0, "Gender ID", "0,1", "Gender ID [0=MALE, 1=FEMALE]", Pyro::ItemVisibility::ADVANCED);
    getActionConvertTextToSpeech().getParameters().addInt32("userIndex", 0, "User Index", "0,1,2,3", "User Index", Pyro::ItemVisibility::ADVANCED);

    getActionConvertSpeechToText().setIsVisible(true);
    getActionConvertSpeechToText().getParameters().addInt32("userIndex", 0, "User Index", "0,1,2,3", "User Index", Pyro::ItemVisibility::ADVANCED);

#if defined(EA_PLATFORM_WINDOWS)
    getActionSelectCodec().setIsVisible(true);
    VoipCodecRegister('spex', &VoipSpeex_CodecDef);
    VoipControl(VoipGetRef(), 'svol', 90, nullptr);
#endif

#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_STADIA)
    VoipCodecRegister('opus', &VoipOpus_CodecDef);
    VoipControl(VoipGetRef(), 'cdec', 'opus', nullptr);
#endif

    mVoipGroup = VoipGroupCreate(16);
    VoipGroupSetDisplayTranscribedTextCallback(mVoipGroup, _VoipConfigDisplayTranscribedTextCallback, this);
}

void VoipConfig::onDeAuthenticatedPrimaryUser()
{
    if (mVoipGroup != NULL)
    {
        VoipGroupDestroy(mVoipGroup);
        mVoipGroup = NULL;
    }

    setIsVisibleForAll(false);
    getWindows().clear();
}


void VoipConfig::onIdle(void *pData, uint32_t uTick)
{
    VoipConfig *pVoipConfig = static_cast<VoipConfig *>(pData);

    if (pVoipConfig->mShowVoipConfig && VoipGetRef())
    {
        int32_t iChannelSlot;
        int32_t iUserIndex;

        for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iUserIndex++)
        {
            for (iChannelSlot = 0; iChannelSlot < VoipGroupStatus(nullptr, 'chnc', 0, nullptr, 0) && iChannelSlot < VOIPCONFIG_MAX_SLOT; iChannelSlot++)
            {
                const char *pChannelModeStr;
                int32_t iChannelId;
                VoipChanModeE eChannelMode;
                uint32_t uUserSlotPair;

                // initialize variable used with VoipStatus('chnl'). most-significant 16 bits = remote user index, least-significant 16 bits = conn index
                uUserSlotPair = (unsigned)iUserIndex;
                uUserSlotPair |= ((unsigned)iChannelSlot << 16);

                // get channel id and channel mode
                iChannelId = VoipGroupStatus(nullptr, 'chnl', (int32_t)uUserSlotPair, &eChannelMode, sizeof(eChannelMode));

                if (iChannelId != pVoipConfig->mChannelId[iUserIndex][iChannelSlot] || eChannelMode != pVoipConfig->mChannelMode[iUserIndex][iChannelSlot])
                {
                    pVoipConfig->mChannelId[iUserIndex][iChannelSlot] = iChannelId;
                    pVoipConfig->mChannelMode[iUserIndex][iChannelSlot] = eChannelMode;

                    // initialize channel mode string and channel id string
                    switch (eChannelMode)
                    {
                        case VOIP_CHANNONE:
                            pChannelModeStr = "UNSUBSCRIBED";
                            EA::StdC::Snprintf(pVoipConfig->mChannelIdString, sizeof(pVoipConfig->mChannelIdString), "n/a");
                            break;
                        case VOIP_CHANSEND:
                            pChannelModeStr = "TALK-ONLY";
                            EA::StdC::Snprintf(pVoipConfig->mChannelIdString, sizeof(pVoipConfig->mChannelIdString), "%03d", iChannelId);
                            break;
                        case VOIP_CHANRECV:
                            pChannelModeStr = "LISTEN-ONLY";
                            EA::StdC::Snprintf(pVoipConfig->mChannelIdString, sizeof(pVoipConfig->mChannelIdString), "%03d", iChannelId);
                            break;
                        case VOIP_CHANSENDRECV:
                            pChannelModeStr = "TALK+LISTEN";
                            EA::StdC::Snprintf(pVoipConfig->mChannelIdString, sizeof(pVoipConfig->mChannelIdString), "%03d", iChannelId);
                            break;
                        default:
                            pChannelModeStr = "UNKNOWN";
                            break;
                    }

                    // build channel slot string
                    EA::StdC::Snprintf(pVoipConfig->mChannelSlotString, sizeof(pVoipConfig->mChannelSlotString), "VOIP channel slot %d for user %d", iChannelSlot, iUserIndex);

                    // build channel config string
                    EA::StdC::Snprintf(pVoipConfig->mChannelCfgString, sizeof(pVoipConfig->mChannelCfgString), "id=%s  mode=%s", pVoipConfig->mChannelIdString, pChannelModeStr);
                    
                    Pyro::UiNodeValueContainer &sessionValues = pVoipConfig->getUiDriver()->getMainValues();
                    sessionValues[pVoipConfig->mChannelSlotString] = pVoipConfig->mChannelCfgString;
                }
            }
        }
    }
}

void VoipConfig::initActionSelectChannel(Pyro::UiNodeAction &action)
{
    action.setText("Select Channel");
    action.setDescription("Subscribe to a voip channel in one of 3 modes (talk, listen, both); or unsusbcribe");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);

    Pyro::UiNodeParameterStruct &parameters = getActionSelectChannel().getParameters();
    parameters.addInt32("channelId", 0, "Channel ID", "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63", "Channel Identifier [0,63]", Pyro::ItemVisibility::ADVANCED);
    parameters.addString("channelMode", "TALK+LISTEN", "Operation Mode", "TALK+LISTEN,TALK-ONLY,LISTEN-ONLY,UNSUBSCRIBE", "Operation mode for the selected channel", Pyro::ItemVisibility::ADVANCED);
}

void VoipConfig::actionSelectChannel(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    VoipChanModeE eMode = VOIP_CHANSENDRECV;
    int32_t iUserIndex;

    if (VoipGetRef())
    {
        if (!mShowVoipConfig)
        {
            mShowVoipConfig = true;    // only show voip config when it is being configured once

            // start with resetting the voip config
            for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iUserIndex++)
            {
                VoipResetChannels(VoipGetRef(), iUserIndex);
            }
        }

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS4)
        if (parameters["useDefaultSharedConfig"].getAsBool() == true)
        {
            VoipControl(VoipGetRef(), 'shch', TRUE, nullptr);
        }
        else
        {
            VoipControl(VoipGetRef(), 'shch', FALSE, nullptr);
        }
#endif

        iUserIndex = parameters["userIndex"];
        
        if (strcmp(parameters["channelMode"].getAsString(), "TALK+LISTEN") == 0)
        {
            eMode = VOIP_CHANSENDRECV;
        }
        if (strcmp(parameters["channelMode"].getAsString(), "TALK-ONLY") == 0)
        {
            eMode = VOIP_CHANSEND;
        }
        if (strcmp(parameters["channelMode"].getAsString(), "LISTEN-ONLY") == 0)
        {
            eMode = VOIP_CHANRECV;
        }
        if (strcmp(parameters["channelMode"].getAsString(), "UNSUBSCRIBE") == 0)
        {
            eMode = VOIP_CHANNONE;
        }

        // set channel
        VoipSelectChannel(VoipGetRef(), iUserIndex, parameters["channelId"], eMode);
    }
}

void VoipConfig::initActionResetChannels(Pyro::UiNodeAction &action)
{
    action.setText("Reset Channel");
    action.setDescription("Unsubscribe from all voip channels");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);
}

void VoipConfig::actionResetChannels(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (VoipGetRef())
    {
        int32_t iUserIndex = iUserIndex = parameters["userIndex"];

        // reset channel configuration
        VoipResetChannels(VoipGetRef(), iUserIndex);
    }
}

#if defined(EA_PLATFORM_PS4)
void VoipConfig::initActionAddMostRestrictiveVoipContributor(Pyro::UiNodeAction &action)
{
    action.setText("Add contributor to shared voip privilege");
    action.setDescription("Add a local user (not yet registered with DS voip) as a contributor to the most restrictive voip privilege.");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);
}

void VoipConfig::actionAddMostRestrictiveVoipContributor(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    int32_t iUserIndex = gBlazeHub->getPrimaryLocalUserIndex();

    iUserIndex = parameters["userIndex"];

    VoipControl(nullptr, '+pri', iUserIndex, nullptr);
}

void VoipConfig::initActionDelMostRestrictiveVoipContributor(Pyro::UiNodeAction &action)
{
    action.setText("Remove contributor from shared voip privilege");
    action.setDescription("Block a local user (not yet registered with DS voip) from contributing to the most restrictive voip privilege.");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);
}

void VoipConfig::actionDelMostRestrictiveVoipContributor(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    int32_t iUserIndex = gBlazeHub->getPrimaryLocalUserIndex();

    iUserIndex = parameters["userIndex"];

    VoipControl(nullptr, '-pri', iUserIndex, nullptr);
}
#endif

void VoipConfig::initActionSuspendPartyChat(Pyro::UiNodeAction &action)
{
    action.setText("Suspend Party Chat");
    action.setDescription("Removes mic focus from the party.");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);
}

void VoipConfig::actionSuspendPartyChat(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    VoipRefT *pVoipRefT = VoipGetRef();
    if (pVoipRefT)
    {
        //check to see if party chat is active
        if (VoipStatus(pVoipRefT, 'pcac', 0, nullptr, 0))
        {
            NetPrintf(("VoipConfig: Party Chat is active.\n"));
        }
        else
        {
            NetPrintf(("VoipConfig: Party Chat is not active.\n"));
        }

        if (VoipStatus(pVoipRefT, 'pcsu', 0, nullptr, 0))
        {
            NetPrintf(("VoipConfig: Party Chat was already suspended, ignoring action.\n"));
        }
        else
        {
            NetPrintf(("VoipConfig: Party Chat was not suspended, suspending now.\n"));
            VoipControl(pVoipRefT, 'pcsu', TRUE, nullptr);
        }
    }
}

void VoipConfig::initActionResumePartyChat(Pyro::UiNodeAction &action)
{
    action.setText("Resume Party Chat");
    action.setDescription("Allows the party to take mic focus.");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);
}

void VoipConfig::actionResumePartyChat(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    VoipRefT *pVoipRefT = VoipGetRef();
    if (pVoipRefT)
    {
        //check to see if party chat is active
        if (VoipStatus(pVoipRefT, 'pcac', 0, nullptr, 0))
        {
            NetPrintf(("VoipConfig: Party Chat is active.\n"));
        }
        else
        {
            NetPrintf(("VoipConfig: Party Chat is not active.\n"));
        }

        if (VoipStatus(pVoipRefT, 'pcsu', 0, nullptr, 0))
        {
            NetPrintf(("VoipConfig: Party Chat was suspended, attempting to resume now.\n"));
            VoipControl(pVoipRefT, 'pcsu', FALSE, nullptr);
        }
        else
        {
            NetPrintf(("VoipConfig: Party Chat was already not suspended, ignoring action.\n"));
        }
    }
}

void VoipConfig::initActionTextChatNarrate(Pyro::UiNodeAction& action)
{
    action.setText("Text Chat Narrate");
    action.setDescription("Text Chat Narrate (Loopback TTS)");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);
}

void VoipConfig::actionTextChatNarrate(Pyro::UiNodeAction* action, Pyro::UiNodeParameterStructPtr parameters)
{
    VoipRefT* pVoip;
    int32_t iUserIndex = parameters["userIndex"];
    const char* pText = parameters["text"];

    // make sure we have a valid ref
    if ((pVoip = VoipGetRef()) == nullptr)
    {
        return;
    }

    VoipControl(pVoip, 'txtn', iUserIndex, const_cast<char *>(pText));
}

void VoipConfig::initActionToggleLoopback(Pyro::UiNodeAction &action)
{
    action.setText("Toggle Loopback");
    action.setDescription("Allows for toggling of loopback in the voip subsystem");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);
}

void VoipConfig::actionToggleLoopback(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    VoipRefT *pVoip;
    int32_t iUserIndex = parameters["userIndex"];
    bool bValue = !mLoopback[iUserIndex];

    // make sure we have a valid ref
    if ((pVoip = VoipGetRef()) == nullptr)
    {
        return;
    }

    VoipControl(pVoip, 'loop', bValue, nullptr);
    VoipGroupActivateLocalUser(mVoipGroup, iUserIndex, bValue);
    mLoopback[iUserIndex] = bValue;
}

void VoipConfig::initActionConvertTextToSpeech(Pyro::UiNodeAction &action)
{
    action.setText("Convert Text to Speech");
    action.setDescription("Allows converting text to speech outside of a game (loopback)");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);
}

void VoipConfig::actionConvertTextToSpeech(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    VoipRefT *pVoip;
    VoipSynthesizedSpeechCfgT VoipSpeechCfg = {};

    const char *pText = parameters["text"];
    VoipSpeechCfg.iPersonaGender = parameters["gender"];
    int32_t iUserIndex = parameters["userIndex"];

    // make sure we have a valid ref
    if ((pVoip = VoipGetRef()) == nullptr)
    {
        return;
    }

    VoipControl(pVoip, 'voic', iUserIndex, &VoipSpeechCfg);
    VoipControl(pVoip, 'ttos', iUserIndex, (void *)pText);
}

void VoipConfig::initActionConvertSpeechToText(Pyro::UiNodeAction &action)
{
    action.setText("Toggle Speech To Text Conversion");
    action.setDescription("Allows toggling the conversion of speech to text outside of a game (loopback)");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);
}


void VoipConfig::actionConvertSpeechToText(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    VoipRefT *pVoip;
    int32_t iUserIndex = parameters["userIndex"];
    bool bValue = !mSpeechToTextConversion[iUserIndex];

    // make sure we have a valid ref
    if ((pVoip = VoipGetRef()) == nullptr)
    {
        return;
    }

    VoipControl(pVoip, 'stot', iUserIndex, &bValue);
    mSpeechToTextConversion[iUserIndex] = bValue;
}

#if defined(EA_PLATFORM_WINDOWS)
void VoipConfig::initActionSelectCodec(Pyro::UiNodeAction &action)
{
    action.setText("Select Codec");
    action.setDescription("Select which codec will be used for VoIP connection on Windows");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);

    Pyro::UiNodeParameter &codec = action.getParameters().addEnum("codec", VoipCodec::VOIPCODEC_DVI, "Name of Codec", "", Pyro::ItemVisibility::ADVANCED);
    codec.setIncludePreviousValues(false);
}

void VoipConfig::actionSelectCodec(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    VoipRefT *pVoip;

    // make sure we have a valid ref
    if ((pVoip = VoipGetRef()) == nullptr)
    {
        return;
    }

    // configure the codec
    switch (parameters["codec"].getAsEnum<VoipCodec>())
    {
    case VOIPCODEC_DVI:
        VoipControl(pVoip, 'cdec', 'dvid', nullptr);
        break;
    case VOIPCODEC_PCM:
        VoipControl(pVoip, 'cdec', 'lpcm', nullptr);
        break;
    case VOIPCODEC_SPEEX:
        VoipControl(pVoip, 'cdec', 'spex', nullptr);
        break;
    case VOIPCODEC_OPUS:
        VoipControl(pVoip, 'cdec', 'opus', nullptr);
        break;
    }
}
#endif

}
