
#include "Ignition/GamePlay.h"
#include "GameManagement.h" // for GameManagement in refreshUi()

#include "Leaderboards.h"
#include "Transmission.h"

#include "BlazeSDK/util/utilapi.h"
#include "BlazeSDK/gamemanager/player.h"
#include "BlazeSDK/gamemanager/playerjoindatahelper.h"

#include "BlazeSDK/component/gamereporting/custom/integratedsample/tdf/integratedsample.h"

#if defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)
#include <collection.h> // for Platform::Collections in actionSendSessionInvite
using namespace Microsoft::Xbox::Services; // for XboxLiveContext in actionSendSessionInvite
using namespace Windows::Foundation; // for IAsyncOperation in actionSendSessionInvite
using namespace Windows::Xbox::Multiplayer;
using namespace Windows::Xbox::System;
using namespace Microsoft::Xbox::Services::Multiplayer;
using namespace Microsoft::Xbox::Services;
#endif
#if defined(EA_PLATFORM_XBOX_GDK)
#include "Ignition/xboxgdk/GdkInvitesSample.h" //for sendInvite in actionSendSessionInvite
#endif

namespace Ignition
{
    static const int32_t TRANSMISSION_MAX_CLIENTS = 32;   // one client = a console

GamePlayLocalUserActionGroup::GamePlayLocalUserActionGroup(const char8_t *id, uint32_t userIndex, Blaze::GameManager::Game *game, GamePlay *gameWindow)
    : LocalUserActionGroup(id, userIndex),
    mGame(game),
    mGameWindow(gameWindow)
{
    setIconImage(Pyro::IconImage::USER_YELLOW);

    getActions().add(&getActionJoinGame());
    getActions().add(&getActionChangePlayerSlotType());
    getActions().add(&getActionLeaveGame());
    getActions().add(&getActionSendMessage());
    getActions().add(&getActionSetPlayerAttribute());
#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
    getActions().add(&getActionSendSessionInvite());
#endif
}

void GamePlayLocalUserActionGroup::initActionJoinGame(Pyro::UiNodeAction &action)
{
    action.setText("Join Game Local User");
    action.setDescription("Joins a specific game by id for the local user");

    action.getParameters().addEnum("gameEntry", Blaze::GameManager::GAME_ENTRY_TYPE_DIRECT, "Game Entry", "", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("playerRole", "", "Joining Role", ",forward,midfielder,defender,goalie", "", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addUInt16("teamIndex", Blaze::GameManager::UNSPECIFIED_TEAM_INDEX, "Joining Team Index", "", "", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addEnum("slotType", Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT, "Slot Type", "The slot type to request for this join.", Pyro::ItemVisibility::ADVANCED);
}

void GamePlayLocalUserActionGroup::actionJoinGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::GameManager::GameEntryType gameEntryType;
    Blaze::GameManager::RoleName joiningRole;
    Blaze::GameManager::TeamIndex teamIndex = parameters["teamIndex"];
    joiningRole.set(parameters["playerRole"]);

    ParseGameEntryType(parameters["gameEntry"], gameEntryType);

    vLOG(gBlazeHub->getGameManagerAPI()->joinGameLocalUser(
        getUserIndex(), mGame->getId(),
        Blaze::GameManager::GameManagerAPI::JoinGameCb(this, &GamePlayLocalUserActionGroup::JoinGameCb), parameters["slotType"],
        nullptr, teamIndex, gameEntryType, joiningRole));
}

void GamePlayLocalUserActionGroup::JoinGameCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::GameManager::Game *game, const char8_t *errorMsg)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    if (blazeError == Blaze::ERR_OK)
    {
        mGameWindow->setupGame(game->getLocalPlayer(getUserIndex()));
        if (!game->getLocalPlayer(getUserIndex())->isReserved())
        {
            getActionJoinGame().setIsVisible(false);
        }

        getActionLeaveGame().setIsVisible(true);
        getActionSendMessage().setIsVisible(true);
        getActionSetPlayerAttribute().setIsVisible(true);
    }

    REPORT_BLAZE_ERROR(blazeError);
}


void GamePlayLocalUserActionGroup::initActionChangePlayerSlotType(Pyro::UiNodeAction &action)
{
    action.setText("Change Player Slot Type");
    action.setDescription("Sets the slot type. For choosing participant, spectator, public, private, etc.");

    action.getParameters().addEnum("slotType", Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT, "Slot Type Choice", ""); 
    action.getParameters().addString("roleName", "forward", "Role Name", "goalie,forward,midfielder,defender", "The role name to use when going from spectator to participant. (Ignored otherwise).");
    action.getParameters().addUInt16("teamIndex", 101, "Team Index", "101,102,0,1,2,3,4", "The team index to use when going from spectator to participant. (Ignored otherwise).");
}

void GamePlayLocalUserActionGroup::actionChangePlayerSlotType(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;
        player = mGame->getLocalPlayer(getUserIndex());

        if (player != nullptr)
        {
            Blaze::GameManager::Player::ChangePlayerTeamAndRoleCb cb(this, &GamePlayLocalUserActionGroup::ChangePlayerTeamAndRoleCb);
            Blaze::GameManager::SlotType slotType = parameters["slotType"];
            if (player->isSpectator() && slotType < Blaze::GameManager::MAX_PARTICIPANT_SLOT_TYPE)
            {
                const char* name = parameters["roleName"];
                Blaze::GameManager::RoleName roleName = name;
                vLOG(player->setPlayerSlotTypeTeamAndRole(slotType, parameters["teamIndex"], roleName, cb));
            }
            else
            {
                vLOG(player->setPlayerSlotType(slotType, cb));
            }
        }
    }
}

#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
void GamePlayLocalUserActionGroup::initActionSendSessionInvite(Pyro::UiNodeAction &action)
{
    action.setText("Send Invite Via Dialog");
    action.setDescription("Opens the session invitation dialog.");
}

void GamePlayLocalUserActionGroup::actionSendSessionInvite(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        #if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
        DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'imus', 16, 0, nullptr);
        DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'imsg', 0, 0, "Play Ignition with me!");
        DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'iued', 1, 0, nullptr);
        DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'osid', getUserIndex(), 0, nullptr);
        #elif defined(EA_PLATFORM_PS5) || (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
        ScePlayerInvitationDialogParam param;
        ScePlayerInvitationDialogSendParam sendParam;
        int32_t iSceResult = 0;

        if ((iSceResult = sceCommonDialogInitialize()) != SCE_OK)
        {
            if ((iSceResult != SCE_COMMON_DIALOG_ERROR_ALREADY_INITIALIZED) && (iSceResult != SCE_COMMON_DIALOG_ERROR_ALREADY_SYSTEM_INITIALIZED))
            {
                getUiDriver()->showMessage_va("sceCommonDialogInitialize failed (err=%s)", DirtyErrGetName(iSceResult));
                return;
            }
        }
        if ((iSceResult = scePlayerInvitationDialogInitialize()) != SCE_OK)
        {
            if ((iSceResult != SCE_COMMON_DIALOG_ERROR_ALREADY_INITIALIZED) && (iSceResult != SCE_COMMON_DIALOG_ERROR_ALREADY_SYSTEM_INITIALIZED))
            {
                getUiDriver()->showMessage_va("scePlayerInvitationDialogInitialize failed (err=%s)", DirtyErrGetName(iSceResult));
                return;
            }
        }

        scePlayerInvitationDialogParamInitialize(&param);
        ds_memclr(&sendParam, sizeof(sendParam));

        // setup the params
        param.userId = NetConnStatus('suid', getUserIndex(), NULL, 0);
        param.mode = SCE_PLAYER_INVITATION_DIALOG_MODE_SEND;
        param.sendParam = &sendParam;
        sendParam.sessionId = mGame->getExternalSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId();

        if ((iSceResult = scePlayerInvitationDialogOpen(&param)) == SCE_OK)
        {
            if (scePlayerInvitationDialogUpdateStatus() != SCE_COMMON_DIALOG_STATUS_RUNNING)
            {
                getUiDriver()->showMessage_va("player dialog returned unexpected status (not running)");
            }
        }
        else
        {
            if (getUiDriver() != nullptr)
            {
                getUiDriver()->showMessage_va("scePlayerInvitationDialogOpen failed (err=%s)", DirtyErrGetName(iSceResult));
            }
        }
        #endif
    }
}

#endif

void GamePlayLocalUserActionGroup::ChangePlayerTeamAndRoleCb(Blaze::BlazeError blazeError, Blaze::GameManager::Player *player)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);
}

void GamePlayLocalUserActionGroup::initActionLeaveGame(Pyro::UiNodeAction &action)
{
    action.setText("Leave Game Local User");
    action.setDescription("Leave this game for this local user");
}

void GamePlayLocalUserActionGroup::actionLeaveGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Game::LeaveGameCb cb(this, &GamePlayLocalUserActionGroup::LeaveGameCb);

        vLOG(mGame->leaveGameLocalUser(getUserIndex(), cb));
    }
}

void GamePlayLocalUserActionGroup::LeaveGameCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);

    if (blazeError == Blaze::ERR_OK)
    {
        getActionJoinGame().setIsVisible(true);
        getActionLeaveGame().setIsVisible(false);
        getActionChangePlayerSlotType().setIsVisible(true);
        getActionSendMessage().setIsVisible(false);
        getActionSetPlayerAttribute().setIsVisible(false);
    }
}

void GamePlayLocalUserActionGroup::initActionSendMessage(Pyro::UiNodeAction &action)
{
    action.setText("Send Message");
    action.setDescription("Send a message to other players in this game");

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_STADIA)
    action.getParameters().addBool("TextToSpeech", false, "Synthesize text and send as voip.", "Message to be synthesized");
#endif
    action.getParameters().addString("messageText", "", "Message");
}

void GamePlayLocalUserActionGroup::actionSendMessage(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_STADIA)
        if (parameters["TextToSpeech"])
        {
            const char *pText = parameters["messageText"].getAsString();
            VoipControl(VoipGetRef(), 'ttos', getUserIndex(), const_cast<char *>(pText));
        }
        else
#endif
        {
            ((BlazeHubUiDriver *)getUiDriver())->runSendMessageToGroup(getUserIndex(), mGame, parameters["messageText"]);
        }
    }
}

void GamePlayLocalUserActionGroup::initActionSetPlayerAttribute(Pyro::UiNodeAction &action)
{
    action.setText("Set Player Attribute");
    action.getParameters().addString("attribKey", "", "Player Attribute Name", "attr1,attr2,attr3", "The name of the player attribute.");
    action.getParameters().addString("attribValue", "", "Player Attribute Value", "X,Y,Z", "Player attribute value.");
}

void GamePlayLocalUserActionGroup::actionSetPlayerAttribute(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        const char* attrKey = parameters["attribKey"];
        const char* attrVal = parameters["attribValue"];
        Blaze::GameManager::Player *player = mGame->getLocalPlayer();
        if ((player != nullptr) && (attrKey != nullptr) && (attrKey[0] != '\0') && (attrVal != nullptr))
        {
            vLOG(player->setPlayerAttributeValue(attrKey, attrVal, Blaze::GameManager::Player::ChangePlayerAttributeCb(this, &GamePlayLocalUserActionGroup::SetPlayerAttributeValueCb)));
        }
    }
}

void GamePlayLocalUserActionGroup::SetPlayerAttributeValueCb(Blaze::BlazeError blazeError, Blaze::GameManager::Player* player)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError)
}


void GamePlayLocalUserActionGroup::onAuthenticated()
{
    LocalUserActionGroup::onAuthenticated();

    if (mGame == nullptr)
    {
        mGameWindow->getLocalUserActionGroup(getUserIndex());
    }
    getActionJoinGame().setIsVisible(true);
    getActionChangePlayerSlotType().setIsVisible(true);
    getActionLeaveGame().setIsVisible(false);
    getActionSendMessage().setIsVisible(false);
    getActionSetPlayerAttribute().setIsVisible(false);
}

void GamePlayLocalUserActionGroup::onDeAuthenticated()
{
    LocalUserActionGroup::onDeAuthenticated();
    getActionJoinGame().setIsVisible(false);
    getActionChangePlayerSlotType().setIsVisible(false);
    getActionLeaveGame().setIsVisible(false);
    getActionSendMessage().setIsVisible(false);
    getActionSetPlayerAttribute().setIsVisible(false);
}


//
// Game Play
//

GamePlay::GamePlay(const char8_t *id, Blaze::GameManager::Game *game, bool bUseNetGameDist)
    : Pyro::UiNodeWindow(id),
      mGame(game),
      mGameGroup(nullptr),
      mActivePlayerId(Blaze::INVALID_BLAZE_ID),
      mIdlerRefCount(0),
      mUseNetGameDist(bUseNetGameDist)
#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
      ,mExternalSessionImgSize(0)
#endif
{
    mLastStatsUpdate = NetTick();

    game->addListener(this);
    if (game->isGameTypeGroup())
    {
        mGameGroup = game;
    }

    Pyro::UiNodeActionPanel &actionPanel = getActionPanel("GamePlay Actions");

    Pyro::UiNodeActionGroup &actionGroup = actionPanel.getGroup("GamePlay Actions");

    actionGroup.getActions().add(&getActionLeaveGame());
    actionGroup.getActions().add(&getActionLeaveGameGroup());
    actionGroup.getActions().add(&getActionLeaveGameLUGroup());
    actionGroup.getActions().add(&getActionStartGame());
    actionGroup.getActions().add(&getActionEndGame());
    actionGroup.getActions().add(&getActionReplayGame());
    actionGroup.getActions().add(&getActionUpdateGameName());
    actionGroup.getActions().add(&getActionSetGameMods());
    actionGroup.getActions().add(&getActionSetGameSettings());
    actionGroup.getActions().add(&getActionSetGameAttribute());
    actionGroup.getActions().add(&getActionSetDedicatedServerAttribute());
    actionGroup.getActions().add(&getActionSetGameEntryCriteria());
    actionGroup.getActions().add(&getActionClearGameEntryCriteria());

    actionGroup.getActions().add(&getActionSetPlayerCapacity());
    actionGroup.getActions().add(&getActionSwapPlayers());
    actionGroup.getActions().add(&getActionDirectReset());
    actionGroup.getActions().add(&getActionReturnDSToPool());
    actionGroup.getActions().add(&getActionDestroyGame());
    actionGroup.getActions().add(&getActionEjectHost());

    actionGroup.getActions().add(&getActionGameGroupCreateGame());
    actionGroup.getActions().add(&getActionGameGroupCreateGamePJD());
    actionGroup.getActions().add(&getActionGameGroupJoinGame());
    actionGroup.getActions().add(&getActionGameGroupJoinGamePJD());
    actionGroup.getActions().add(&getActionGameGroupCreateDedicatedServerGame());
    actionGroup.getActions().add(&getActionGameGroupLeaveGame());

    actionGroup.getActions().add(&getActionSetGameLockedAsBusy());
    actionGroup.getActions().add(&getActionSetAllowAnyReputation());
    actionGroup.getActions().add(&getActionSetDynamicReputationRequirement());
    actionGroup.getActions().add(&getActionSetOpenToJoinByPlayer());
    actionGroup.getActions().add(&getActionSetOpenToJoinByInvite());

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
    actionGroup.getActions().add(&getActionSetFriendsBypassClosedToJoinByPlayer());
    actionGroup.getActions().add(&getActionSubmitTournamentMatchResult());
#endif
#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
    actionGroup.getActions().add(&getActionSendSessionInvite());
#endif
#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
    actionGroup.getActions().add(&getActionUpdateExternalSession());
    memset(mExternalSessionImgBuf, 0, sizeof(mExternalSessionImgBuf));
#endif

    // We should have this scenario information by now (if not, no scenarios actions will be added).
    if (game->isGameTypeGroup())
    {
        GameManagement::addScenarioActions(actionGroup.getActions(), Pyro::UiNodeAction::ExecuteEventHandler(this, &GamePlay::actionGameGroupScenarioMatchmake));
        GameManagement::addTemplateActions(actionGroup.getActions(), Pyro::UiNodeAction::ExecuteEventHandler(this, &GamePlay::actionGameGroupCreateGameTemplate));
    }

    for (uint32_t userIndex = 0; userIndex < gBlazeHub->getInitParams().UserCount; ++userIndex)
    {
        GamePlayLocalUserActionGroup &localUserActionGroup = getLocalUserActionGroup(userIndex);

        Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(userIndex);
        if (localUser != nullptr)
            localUserActionGroup.onAuthenticated();
        else
            localUserActionGroup.onDeAuthenticated();
    }

    const EA::TDF::ObjectType gameType = game->getBlazeObjectId().type;//for game group, game session etc
    gBlazeHub->getMessagingAPI(gBlazeHub->getPrimaryLocalUserIndex())->registerCallback(
        Blaze::Messaging::MessagingAPI::GameMessageCb(this, &GamePlay::GameMessageCb), gameType);
    
    mPrivateGameData.mGameRegisteredIdler = false;
    registerIdler();
}

GamePlay::~GamePlay()
{
    unregisterIdler();
    gameDestroyed();
}

GamePlayLocalUserActionGroup &GamePlay::getLocalUserActionGroup(uint32_t userIndex)
{
    Pyro::UiNodeActionPanel &actionPanel = getActionPanel("GamePlay Actions");

    eastl::string actionGroupId(eastl::string::CtorSprintf(), "localUserActionGroup:%" PRIu32, userIndex);
    GamePlayLocalUserActionGroup *actionGroup = (GamePlayLocalUserActionGroup *)actionPanel.getGroups().findById(actionGroupId.c_str());

    if (actionGroup == nullptr)
    {
        actionGroup = new GamePlayLocalUserActionGroup(actionGroupId.c_str(), userIndex, mGame, this);
        actionPanel.getGroups().add(actionGroup);
    }
    else
    {
        actionGroup->setGame(mGame);
    }

    return *actionGroup;
}

void GamePlay::idle(const uint32_t currentTime, const uint32_t elapsedTime)
{
    if (mGame != nullptr)
    {
        if (gConnApiAdapter->getConnApiRefT(mGame, true) != nullptr)
        {
            ConnApiUpdate(gConnApiAdapter->getConnApiRefT(mGame));
        }

        gTransmission2.Idle();
#if !defined(EA_PLATFORM_NX)
        voipIdle();
#endif
        refreshConnections();

        gTransmission2.GetTransmission()->ReceiveFromAll(mGame, Transmission::ReceiveHandlerCb(this, &GamePlay::ReceiveHandlerCb));
    }
}

#if !defined(EA_PLATFORM_NX)
void GamePlay::voipIdle()
{
    if (mGame != nullptr)
    {
        bool refreshPlayersTable = false;
        Blaze::GameManager::Player *player;
        for (uint16_t a = 0;
            a < mGame->getPlayerCount();
            a++)
        {
            player = mGame->getPlayerByIndex(a);
            InternalPlayerData* internalPlayerData = (InternalPlayerData *)player->getTitleContextData();

            if (internalPlayerData == nullptr)
                continue;
            
            //Copy old data to verify if refresh is required
            InternalPlayerData oldInternalPlayerData;
            if (!refreshPlayersTable)
            {
                oldInternalPlayerData.mVoipAvailable = internalPlayerData->mVoipAvailable;
                oldInternalPlayerData.mHeadset = internalPlayerData->mHeadset;
                oldInternalPlayerData.mVoipAvailableDS = internalPlayerData->mVoipAvailableDS;
                oldInternalPlayerData.mHeadsetDS = internalPlayerData->mHeadsetDS;
                oldInternalPlayerData.mTalking = internalPlayerData->mTalking;
            }

            if (player->getUser()->hasExtendedData())
            {
                internalPlayerData->mHeadset = player->getUser()->getExtendedData()->getHardwareFlags().getVoipHeadsetStatus();
                internalPlayerData->mVoipAvailable = true;
            }
            else
            {
                internalPlayerData->mHeadset = false;
                internalPlayerData->mVoipAvailable = false;
            }


            
            if (((mGame->getLocalMeshEndpoint() != nullptr) && mGame->getLocalMeshEndpoint()->isDedicatedServerHost()) 
                || ((mGame->getLocalPlayer() != nullptr) && (mGame->getLocalPlayer()->getPlayerState() == Blaze::GameManager::ACTIVE_CONNECTED)))
            {
                VoipGroupRefT *voipGroupRef = nullptr;
                ConnApiRefT *gameConnApiRefT = gConnApiAdapter->getConnApiRefT(mGame, true);

                if (gameConnApiRefT != nullptr && ((mGame->getVoipTopology() != Blaze::VOIP_DISABLED && mGame->isTopologyHost()) || ((mGame->getLocalPlayer() != nullptr) && 
                    (mGame->getLocalPlayer()->getPlayerState() == Blaze::GameManager::ACTIVE_CONNECTED))))
                {
                    ConnApiStatus(gameConnApiRefT, 'vgrp', &voipGroupRef, sizeof(voipGroupRef));
                }

                if (VoipGetRef() != nullptr && voipGroupRef != nullptr && mGame->getVoipTopology() != Blaze::VOIP_DISABLED && mGame->getLocalPlayer() != nullptr &&
                    mGame->getLocalPlayer()->getPlayerState() == Blaze::GameManager::ACTIVE_CONNECTED)
                {
                    int32_t iStatus = 0;
                    if (player->isLocalPlayer() == true)
                    {
                        Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUserById(player->getId());
                        if (localUser != nullptr)
                        {
                            iStatus = VoipGroupLocalUserStatus(voipGroupRef, localUser->getUserIndex());
                            internalPlayerData->mTalking = (iStatus & VOIP_LOCAL_USER_SENDVOICE) == 0; 
                            internalPlayerData->mHeadsetDS = (iStatus & VOIP_LOCAL_USER_HEADSETOK) == 0; 
                        }
                    }
                    else
                    {
                        iStatus = VoipGroupRemoteUserStatus(voipGroupRef, getVoipConnIndexForPlayer(player), player->getLocalUserIndex());
                        internalPlayerData->mTalking = (iStatus & VOIP_REMOTE_USER_RECVVOICE) == 0; 
                        internalPlayerData->mHeadsetDS = (iStatus & VOIP_REMOTE_USER_HEADSETOK) == 0;
                    }

                    internalPlayerData->mVoipAvailableDS = true;
                }
                else 
                {
                    internalPlayerData->mVoipAvailableDS = false;
                    internalPlayerData->mTalking = false; 
                    internalPlayerData->mHeadsetDS = false;
                }
            }
            else 
            {
                internalPlayerData->mVoipAvailableDS = false;
                internalPlayerData->mTalking = false; 
                internalPlayerData->mHeadsetDS = false;
            }

            if (!refreshPlayersTable && (
                oldInternalPlayerData.mVoipAvailable != internalPlayerData->mVoipAvailable ||
                oldInternalPlayerData.mHeadset != internalPlayerData->mHeadset ||
                oldInternalPlayerData.mVoipAvailableDS != internalPlayerData->mVoipAvailableDS ||
                oldInternalPlayerData.mHeadsetDS != internalPlayerData->mHeadsetDS ||
                oldInternalPlayerData.mTalking != internalPlayerData->mTalking))
            {
                refreshPlayersTable = true;
            }
        }

        if (refreshPlayersTable)
        {
            refreshPlayers();
        }
    }
}
#endif

void GamePlay::ReceiveHandlerCb(Blaze::GameManager::Game *game, char8_t *buffer, int bufferSize)
{
    IngameMessage message;

    message.readMessagePacket(buffer);
    Blaze::GameManager::Player* shooter = game->getPlayerById(message.getShooter());
    if (shooter != nullptr) 
    {
        if (shooter->getTitleContextData() != nullptr)
        {
            switch (message.getIngameMessageType())
            {
                case INGAME_MESSAGE_TYPE_HIT:
                {
                    ((InternalPlayerData *)shooter->getTitleContextData())->mHits++;
                    break;
                }
                case INGAME_MESSAGE_TYPE_MISS:
                {
                    ((InternalPlayerData *)shooter->getTitleContextData())->mMisses++;
                    break;
                }
                case INGAME_MESSAGE_TYPE_TORCHE:
                {
                    ((InternalPlayerData *)shooter->getTitleContextData())->mTorchings++;
                    break;
                }
            }
        }

        // if this console is the topology host , it needs to rebroadcast the data
        if (game->isTopologyHost() && 
            game->getNetworkTopology() != Blaze::PEER_TO_PEER_FULL_MESH)
        {
            // broadcast to other players
            gTransmission2.GetTransmission()->SendToAll(game, buffer, bufferSize, TRUE, shooter->getConnectionGroupId());
        }
    }
    refreshPlayers();
}

#if !defined(EA_PLATFORM_NX)
int16_t GamePlay::getVoipConnIndexForPlayer(Blaze::GameManager::Player *player)
{
    const ConnApiClientT* client = gConnApiAdapter->getClientHandleForEndpoint(player->getMeshEndpoint());
    if (client)
    {
        return client->iVoipConnId;
    }
    return -1;
}
#endif

uint8_t GamePlay::getConnApiIndexForPlayer(Blaze::GameManager::Game *game, Blaze::GameManager::Player *player)
{
    return(player->getConnectionSlotId());
}

void GamePlay::ConnApiCb(ConnApiRefT *pConnApi, ConnApiCbInfoT *pCbInfo, void *pUserData)
{
    GamePlay *gamePlayInstance = (GamePlay *)pUserData;

    if (pCbInfo->eType == CONNAPI_CBTYPE_GAMEEVENT)
    {
        if (pCbInfo->eNewStatus == CONNAPI_STATUS_ACTV)
        {
            //game server using NetGameDist
            if ((pCbInfo->iClientIndex == gamePlayInstance->getGame()->getTopologyHostConnectionSlotId()) && 
                (gamePlayInstance->getGame()->getNetworkTopology() == Blaze::GameNetworkTopology::CLIENT_SERVER_DEDICATED) &&
                (gamePlayInstance->UseNetGameDist()))
            {
                NetGameLinkRefT *gameServerLink = ConnApiGetClientList(pConnApi)->Clients[pCbInfo->iClientIndex].pGameLinkRef;
                if (gameServerLink != nullptr)
                {
                    gTransmission2.AddGameServer(pCbInfo->iClientIndex, gameServerLink, gamePlayInstance->mGame);
                }
            }
            //client
            else
            {
                gTransmission2.AddClient(pCbInfo->iClientIndex, ConnApiGetClientList(pConnApi)->Clients[pCbInfo->iClientIndex].pGameLinkRef, gamePlayInstance->mGame);
            }
        }
    }
    else if (pCbInfo->eType == CONNAPI_CBTYPE_DESTEVENT)
    {
        //game server are also clients, so just remove them the same way
        NetGameLinkRefT *pLinkRef = ConnApiGetClientList(pConnApi)->Clients[pCbInfo->iClientIndex].pGameLinkRef;
        gTransmission2.RemoveClient(pCbInfo->iClientIndex, pLinkRef, gamePlayInstance->mGame);
    }
    
    if (pCbInfo->eType == CONNAPI_CBTYPE_DESTEVENT)
    {

    }
}

#if !defined(EA_PLATFORM_NX)
void GamePlay::gameVoipCallback(VoipRefT *pVoip, VoipCbTypeE eCbType, int32_t iValue, void *pUserData)
{
}
#endif

int32_t GamePlay::gameTranscribedTextCallback(int32_t iConnId, int32_t iUserIndex, const char *pStrUtf8, void *pUserData)
{
#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_STADIA)
    uint16_t iPlayerIndex = 0;
    GamePlay *pGamePlayObject = (GamePlay *)pUserData;
    Blaze::GameManager::Player* pCurrentPlayer = nullptr;

    // loop through all the active players in the game
    for (iPlayerIndex = 0; iPlayerIndex < pGamePlayObject->mGame->getActivePlayerCount(); ++iPlayerIndex)
    {
        Blaze::GameManager::Player* pPlayer = pGamePlayObject->mGame->getActivePlayerByIndex(iPlayerIndex);

        if (pPlayer != nullptr)
        {
            // get the ConnApiClient for a particular mesh endpoint (a mesh endpoint is a console)
            const ConnApiClientT *pConnApiClient = gConnApiAdapter->getClientHandleForEndpoint(pPlayer->getMeshEndpoint());

            if (pConnApiClient != nullptr)
            {
                /* iConndId represents the voip connection id to a ConnApiClient (a remote console)
                   iUserIndex is the player's local index on its own console
                   if both the VoipConnId of the ConnApiClient of the player and the player's local user index match, we have found the player that sent the text
                */
                if ((!pPlayer->isLocal()) && (iConnId != -1))
                {

                    if ((pConnApiClient->iVoipConnId == (int16_t)iConnId) && (((int32_t)pPlayer->getLocalUserIndex() == iUserIndex)))
                    {
                        pCurrentPlayer = pPlayer;
                        break;
                    }
                }
                /* for local players there is no voip connections so the iConnId is -1
                   therefore we just need to find a player with a matching local user index
                */
                else if (pPlayer->isLocal() && (iConnId == -1))
                {
                    if ((int32_t)pPlayer->getLocalUserIndex() == iUserIndex)
                    {
                        pCurrentPlayer = pPlayer;
                        break;
                    }
                }
            }
        }
    }

    // if we found a matching player display the text and player name
    if (pCurrentPlayer != nullptr)
    {
        pGamePlayObject->getMemo("chat").appendText_va("transcribed text [Player: %s] > %s", pCurrentPlayer->getName(), pStrUtf8);
        return(TRUE);
    }
    return(FALSE);
#else
    return(FALSE);
#endif
}

void GamePlay::setupConnApi()
{
    bool isDedicatedServerAndTopologyHost = (mGame->isDedicatedServer() && mGame->isTopologyHost());

    Blaze::GameManager::Player *player = mGame->getLocalPlayer(gBlazeHub->getPrimaryLocalUserIndex());

    if ( isDedicatedServerAndTopologyHost || ((player != nullptr) && player->isActive()) )
    {
#if !defined(EA_PLATFORM_NX)
        if ((VoipGetRef() != nullptr) && (gConnApiAdapter->getConnApiRefT(mGame) != nullptr) 
            && ((gBlazeHub->getInitParams().Client == Blaze::CLIENT_TYPE_GAMEPLAY_USER) || (gBlazeHub->getInitParams().Client == Blaze::CLIENT_TYPE_LIMITED_GAMEPLAY_USER)))
        {
            VoipGroupRefT *voipGroupRef; 
            ConnApiStatus(gConnApiAdapter->getConnApiRefT(mGame), 'vgrp', &voipGroupRef, sizeof(voipGroupRef));
            VoipGroupSetEventCallback(voipGroupRef, &GamePlay::gameVoipCallback, this);
            VoipGroupSetDisplayTranscribedTextCallback(voipGroupRef, &GamePlay::gameTranscribedTextCallback, this);
        }
#endif
        if (gConnApiAdapter->getConnApiRefT(mGame) != nullptr)
        {
            ConnApiAddCallback(gConnApiAdapter->getConnApiRefT(mGame), ConnApiCb, this);
            ConnApiControl(gConnApiAdapter->getConnApiRefT(mGame), 'auto', FALSE, 0, NULL);
        }
    }
}

void GamePlay::setupGame(Blaze::GameManager::Player *player)
{
    mPrivateGameData.mMuteMask = 0xFFFFFFFF;
    mPrivateGameData.mGameNeedsReset = false;

    for (uint16_t a = 0; a < mGame->getPlayerCount(); a++)
    {
        setupPlayer(mGame->getPlayerByIndex(a));
    }
    
    bool isDedicatedServerAndTopologyHost = (mGame->isDedicatedServer() && mGame->isTopologyHost());

    // setup a default active player
    if (!isDedicatedServerAndTopologyHost && !mGame->isExternalOwner())
    {
        mActivePlayerId = mGame->getLocalPlayer()->getId();

    }

    refreshProperties();
    refreshPlayers();

    getActionDirectReset().setIsVisible(isDedicatedServerAndTopologyHost);
    getActionReturnDSToPool().setIsVisible(isDedicatedServerAndTopologyHost);
    getActionDestroyGame().setIsVisible(isDedicatedServerAndTopologyHost || mGame->isExternalOwner());
    getActionEjectHost().setIsVisible(isDedicatedServerAndTopologyHost && mGame->getGameSettings().getVirtualized());

    getActionStartGame().setIsVisible(!mGame->isGameTypeGroup());
    getActionLeaveGame().setIsVisible(!mGame->isGameTypeGroup());
    getActionEndGame().setIsVisible(!mGame->isGameTypeGroup());
    getActionReplayGame().setIsVisible(!mGame->isGameTypeGroup());
    getActionEjectHost().setIsVisible(!mGame->isGameTypeGroup());

    getActionLeaveGameGroup().setIsVisible(mGame->isGameTypeGroup());
    getActionLeaveGameLUGroup().setIsVisible(!gBlazeHub->getUserManager()->getLocalUserGroup().isEmpty());

    const bool showAdminOptions = (isDedicatedServerAndTopologyHost || ((player != nullptr) && player->isAdmin()) || mGame->isExternalOwner());
    getActionSetGameLockedAsBusy().setIsVisible(showAdminOptions);
    getActionSetDynamicReputationRequirement().setIsVisible(showAdminOptions);
    getActionSetAllowAnyReputation().setIsVisible(showAdminOptions);
    getActionSetOpenToJoinByPlayer().setIsVisible(showAdminOptions);
    getActionSetOpenToJoinByInvite().setIsVisible(showAdminOptions);

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
    getActionSetFriendsBypassClosedToJoinByPlayer().setIsVisible(showAdminOptions);
#endif
#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
    getActionSubmitTournamentMatchResult().setIsVisible(true);
#endif
#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
    getActionUpdateExternalSession().setIsVisible(true);
#endif

    getActionGameGroupCreateGame().setIsVisible(mGame->isGameTypeGroup());
    getActionGameGroupCreateGamePJD().setIsVisible(mGame->isGameTypeGroup());
    getActionGameGroupJoinGame().setIsVisible(mGame->isGameTypeGroup());
    getActionGameGroupJoinGamePJD().setIsVisible(mGame->isGameTypeGroup());
    getActionGameGroupCreateDedicatedServerGame().setIsVisible(mGame->isGameTypeGroup());
    getActionGameGroupLeaveGame().setIsVisible(mGame->isGameTypeGroup());

    if ((mGame->getGameState() == Blaze::GameManager::INITIALIZING || mGame->getGameState() == Blaze::GameManager::CONNECTION_VERIFICATION) 
        && (mGame->isPlatformHost() || mGame->isExternalOwner()))
    {
        if ((mGame->isTopologyHost() && !mGame->isDedicatedServer())
            || (mGame->isPlatformHost() && (mGame->getNetworkTopology() == Blaze::GameNetworkTopology::NETWORK_DISABLED)))
        {
            if ((player != nullptr) && (player->isHost() || player->isPlatformHost()))
            {
                mGame->initGameComplete(Blaze::GameManager::Game::ChangeGameStateCb(this, &GamePlay::ChangeGameStateCb));
            }
        }
        else if (mGame->isExternalOwner())
        {
            mGame->initGameComplete(Blaze::GameManager::Game::ChangeGameStateCb(this, &GamePlay::ChangeGameStateCb));
        }
    }
}

void GamePlay::setupPlayer(Blaze::GameManager::Player *player)
{
    InternalPlayerData *playerData;

    playerData = new InternalPlayerData;

    player->setTitleContextData(playerData);

     // when a player joins, it always defaults to unmuted
    mPrivateGameData.mMuteMask |= (1 << player->getConnectionSlotId());

    Blaze::UserManager::LocalUser* localUser = gBlazeHub->getUserManager()->getLocalUserById(player->getId());
    if (localUser != nullptr)
    {
        GamePlayLocalUserActionGroup& actionGroup = getLocalUserActionGroup(localUser->getUserIndex());
        if (!player->isReserved())
        {
            actionGroup.getActionJoinGame().setIsVisible(false);
        }
        actionGroup.getActionLeaveGame().setIsVisible(true);
        actionGroup.getActionSendMessage().setIsVisible(true);
        actionGroup.getActionSetPlayerAttribute().setIsVisible(true);
    }
}

void GamePlay::gameDestroyed()
{
    if ((mGame != nullptr) && (gConnApiAdapter->getConnApiRefT(mGame) != nullptr))
        ConnApiRemoveCallback(gConnApiAdapter->getConnApiRefT(mGame), ConnApiCb, this);

    if (mGame != nullptr)
    {
        const EA::TDF::ObjectType gameType = mGame->getBlazeObjectId().type;
        gBlazeHub->getMessagingAPI(gBlazeHub->getPrimaryLocalUserIndex())->removeCallback(
            Blaze::Messaging::MessagingAPI::GameMessageCb(this, &GamePlay::GameMessageCb), gameType);
        mGame->removeListener(this);
    }
    mGame = nullptr;
}

void GamePlay::ChangeGameStateCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    if (blazeError == Blaze::ERR_OK)
    {
        refreshProperties();
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void GamePlay::UpdateGameNameCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    if (blazeError == Blaze::ERR_OK)
    {
        refreshProperties();
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void GamePlay::DestroyGameCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game)
{
    if (blazeError == Blaze::ERR_OK)
    {
        refreshProperties();
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void GamePlay::AddQueuedPlayerCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game, const Blaze::GameManager::Player *player)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    if (blazeError == Blaze::ERR_OK)
    {
        refreshProperties();
    }
    else
    {
        // on error boot the player who errord and pump again.
        if ((blazeError != Blaze::GAMEMANAGER_ERR_PLAYER_NOT_FOUND) && (player != nullptr))
        {
            vLOG(game->kickPlayer(player,
                Blaze::GameManager::Game::KickPlayerCb(this, &GamePlay::KickPlayerCb)));

            pumpGameQueue(game);
        }
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void GamePlay::SetPlayerCapacityCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game, const Blaze::GameManager::SwapPlayersErrorInfo *errorInfo)
{
    if (blazeError == Blaze::ERR_OK)
    {
        refreshProperties();
    }

    REPORT_BLAZE_ERROR(blazeError);

    if (blazeError != Blaze::ERR_OK)
    {
        if ((getUiDriver() != nullptr) && (errorInfo != nullptr))
        { 
            getUiDriver()->showMessage_va(Pyro::ErrorLevel::ERR, "SetPlayerCapacity Error", "Error: %s \n\n Details:\nPlayer: (%" PRIi64 ") TeamIndex: (%" PRIi16 ") RoleName: (%s) SlotType: (%s)",  errorInfo->getErrMessage(), 
                errorInfo->getPlayerId(), errorInfo->getTeamIndex(), errorInfo->getRoleName(), Blaze::GameManager::SlotTypeToString(errorInfo->getSlotType()));
        }
    }
}

void GamePlay::refreshProperties()
{
    eastl::string adminList;

    Pyro::UiNodeValueContainer &valueGroupNode = getValuePanel("Settings and Attributes").getValues();
    valueGroupNode.clear();

    for (Blaze::GameManager::PlayerIdList::const_iterator i = mGame->getAdminList().begin();
        i != mGame->getAdminList().end();
        i++)
    {
        if (mGame->getPlayerById(*i) != nullptr)
        {
            adminList += mGame->getPlayerById(*i)->getName();
            adminList += ", ";
        }
        else if (mGame->getExternalOwnerInfo().getPlayerId() == *i)
        {
            adminList.append_sprintf("% " PRIi64 "", mGame->getExternalOwnerInfo().getPlayerId());
            adminList += ", ";
        }
    }
    if (adminList.length() >= 2)
    {
        adminList.force_size(adminList.length() - 2);
    }

    valueGroupNode["Admins"].setDescription("Name of a player who has admin rights for this game");
    valueGroupNode["Admins"] = adminList.c_str();
    
    valueGroupNode["Name"].setDescription("The name of this game");
    valueGroupNode["Name"] = mGame->getName();

    valueGroupNode["Game Id"].setDescription("The id of this game");
    valueGroupNode["Game Id"] = mGame->getId();

    valueGroupNode["Game Object Id"].setDescription("The id of this game, including component and entity type.");
    valueGroupNode["Game Object Id"] = mGame->getBlazeObjectId().toString().c_str();


    valueGroupNode["State"].setDescription("The current state of this game");
    valueGroupNode["State"] = mGame->getGameState();
        
    valueGroupNode["Protocol Version"].setDescription("The game protocol version specified for this game");
    valueGroupNode["Protocol Version"] = mGame->getGameProtocolVersionString();

    if (mGame->getNetworkTopology() == Blaze::CLIENT_SERVER_DEDICATED)
    {
        valueGroupNode["Ping Site"].setDescription("The game ping site alias");
        valueGroupNode["Ping Site"] = mGame->getPingSite().c_str();
    }

    valueGroupNode["Game Mods"].setDescription("The Game Mod Register");
    valueGroupNode["Game Mods"] = mGame->getGameModRegister();

    eastl::string entryCriteriaStr;
    for( Blaze::EntryCriteriaMap::const_iterator iter = mGame->getEntryCriteriaMap().begin(); iter != mGame->getEntryCriteriaMap().end(); ++iter )
    {
        entryCriteriaStr += iter->first;
        entryCriteriaStr += ": ";
        entryCriteriaStr += iter->second;
        entryCriteriaStr += ", ";
    }

    valueGroupNode["Game Entry Criteria"].setDescription("The Entry Criteria");
    valueGroupNode["Game Entry Criteria"] = entryCriteriaStr.c_str();

    valueGroupNode["Max Player Capacity"].setDescription("The maximum number of users that can join this game & the max the capacity can be resized to");
    valueGroupNode["Max Player Capacity"] = mGame->getMaxPlayerCapacity();

    valueGroupNode["Min Player Capacity"].setDescription("The minimum player capacity that this game can be resized to support");
    valueGroupNode["Min Player Capacity"] = mGame->getMinPlayerCapacity();


    valueGroupNode["Player Capacity"].setDescription("The current capacity of this game. (Spectators + Participants)");
    valueGroupNode["Player Capacity"] = mGame->getPlayerCapacityTotal();

    valueGroupNode["Participant Capacity"].setDescription("The current participant capacity of this game.");
    valueGroupNode["Participant Capacity"] = mGame->getParticipantCapacityTotal();

    valueGroupNode["Spectator Capacity"].setDescription("The current spectator capacity of this game.");
    valueGroupNode["Spectator Capacity"] = mGame->getSpectatorCapacityTotal();

    valueGroupNode["Public Participant Slots"].setDescription("The number of slots available in this game for public participants.");
    valueGroupNode["Public Participant Slots"] = mGame->getPlayerCapacity(Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT);
        
    valueGroupNode["Private Participant Slots"].setDescription("The number of slots available in this game for private participants.");
    valueGroupNode["Private Participant Slots"] = mGame->getPlayerCapacity(Blaze::GameManager::SLOT_PRIVATE_PARTICIPANT);

    valueGroupNode["Public Spectator Slots"].setDescription("The number of slots available in this game for public spectators.");
    valueGroupNode["Public Spectator Slots"] = mGame->getPlayerCapacity(Blaze::GameManager::SLOT_PUBLIC_SPECTATOR);

    valueGroupNode["Private Spectator Slots"].setDescription("The number of slots available in this game for private spectators.");
    valueGroupNode["Private Spectator Slots"] = mGame->getPlayerCapacity(Blaze::GameManager::SLOT_PRIVATE_SPECTATOR);
        
    valueGroupNode["Queue Capacity"].setDescription("The current queue capacity of this game");
    valueGroupNode["Queue Capacity"] = mGame->getQueueCapacity();
        
    valueGroupNode["Open to Browsing"].setDescription("Indicates if players can join this game by browsing for it");
    valueGroupNode["Open to Browsing"] = mGame->getGameSettings().getOpenToBrowsing();
        
    valueGroupNode["Open to Invites"].setDescription("Indicates if players can join this game by invitation");
    valueGroupNode["Open to Invites"] = mGame->getGameSettings().getOpenToInvites();
        
    valueGroupNode["Open to Join By Player"].setDescription("Indicates if players can join this game by searching for a player already in this game");
    valueGroupNode["Open to Join By Player"] = mGame->getGameSettings().getOpenToJoinByPlayer();
#if (defined(EA_PLATFORM_XBOXONE) && (_XDK_VER >= __XDK_VER_2015_MP)) || defined(EA_PLATFORM_XBSX)
    valueGroupNode["Friends Bypass Join By Player"].setDescription("Indicates if allow friends to bypass closed to join by player.");
    valueGroupNode["Friends Bypass Join By Player"] = mGame->getGameSettings().getFriendsBypassClosedToJoinByPlayer();
#endif 

    valueGroupNode["Open to Matchmaking"].setDescription("Indicates if players can join this game by matchmaking");
    valueGroupNode["Open to Matchmaking"] = mGame->getGameSettings().getOpenToMatchmaking();

    valueGroupNode["Enable Persisted Game Id"].setDescription("Indicates if players can join this game by Persisted Game Id");
    valueGroupNode["Enable Persisted Game Id"] = mGame->getGameSettings().getEnablePersistedGameId();

    valueGroupNode["Is Open to All Joins"].setDescription("Indicates if this game is open to all applicable join methods");
    valueGroupNode["Is Open to All Joins"] = mGame->isOpenToAllJoins();

    valueGroupNode["Is Closed to All Joins"].setDescription("Indicates if this game is closed to all applicable join methods");
    valueGroupNode["Is Closed to All Joins"] = mGame->isClosedToAllJoins();

    valueGroupNode["Is Ranked"].setDescription("Indicates if this game is a ranked match");
    valueGroupNode["Is Ranked"] = mGame->getGameSettings().getRanked();
        
    valueGroupNode["Is Host Migratable"].setDescription("Indicates if the host of this game can migrate to a new host if the current host leaves");
    valueGroupNode["Is Host Migratable"] = mGame->getGameSettings().getHostMigratable();
        
    valueGroupNode["Admin Only Invites"].setDescription("Indicates if the administrator of this game is the only player allowed to invite other players");
    valueGroupNode["Admin Only Invites"] = mGame->getGameSettings().getAdminOnlyInvites();
        
    valueGroupNode["Allow Join In Progress"].setDescription("If Join in Progress is supported, other users are allowed to join this game while it is in the IN_GAME state");
    valueGroupNode["Allow Join In Progress"] = mGame->getGameSettings().getJoinInProgressSupported();

    valueGroupNode["Allow Any Reputation"].setDescription("If Allow Any Reputation is enabled, users joining the game bypass the reputation check.");
    valueGroupNode["Allow Any Reputation"] = mGame->getGameSettings().getAllowAnyReputation();
    
    valueGroupNode["Dynamic Reputation Requirement"].setDescription("If Dynamic Reputation Requirement is enabled allowAnyReputation is dynamically controlled/updated by GameManager and join by invites bypass reputation checks.");
    valueGroupNode["Dynamic Reputation Requirement"] = mGame->getGameSettings().getDynamicReputationRequirement();

    valueGroupNode["Locked As Busy"].setDescription("If Locked As Busy is enabled, join game calls to get into the game will have their join's execution internally suspended/delayed until game is no longer Locked As Busy, for up to the server configured max wait time, or, fail the join if no wait time is configured.");
    valueGroupNode["Locked As Busy"] = mGame->getGameSettings().getLockedAsBusy();

    valueGroupNode["Crossplay Enabled"].setDescription("If Crossplay Enabled is enabled, then other platforms can join this game (as configured on the server).  Cannot be reconfigured.");
    valueGroupNode["Crossplay Enabled"] = mGame->isCrossplayEnabled();

    {
        eastl::string platsString;
        for (Blaze::ClientPlatformType curPlat : mGame->getClientPlatformOverrideList())
        {
            platsString.append(Blaze::ClientPlatformTypeToString(curPlat));
            platsString.append(" ");
        }
        if (platsString.empty())
        {
            platsString = "(Crossplay Disabled)";
        }

        valueGroupNode["Platform Restrictions"].setDescription("Which platforms can play on in this Game.");
        valueGroupNode["Platform Restrictions"] = platsString.c_str();
    }



    for (Blaze::Collections::AttributeMap::const_iterator i = mGame->getGameAttributeMap().begin();
        i != mGame->getGameAttributeMap().end();
        i++)
    {
        valueGroupNode[(*i).first.c_str()].setDescription("An item from the game's AttributeMap");
        valueGroupNode[(*i).first.c_str()] = (*i).second.c_str();
    }

    for (Blaze::Collections::AttributeMap::const_iterator i = mGame->getDedicatedServerAttributeMap().begin();
        i != mGame->getDedicatedServerAttributeMap().end();
        i++)
    {
        valueGroupNode[(*i).first.c_str()].setDescription("An item from the dedicated server's AttributeMap");
        valueGroupNode[(*i).first.c_str()] = (*i).second.c_str();
    }

    for (uint16_t i = 0; i < mGame->getTeamCount(); ++i)
    {
        eastl::string teamIdentity;
        teamIdentity.sprintf("TeamIndex %d [Id=%d]", i, mGame->getTeamIdByIndex(i));

        eastl::string teamData;
        teamData.sprintf("%d/%d players/max", mGame->getTeamSizeByIndex(i), mGame->getTeamCapacity());

        valueGroupNode[teamIdentity.c_str()].setDescription("Team IDs with current size and capacity");
        valueGroupNode[teamIdentity.c_str()] = teamData.c_str();
    }

    Blaze::GameManager::RoleCriteriaMap::const_iterator roleCriteriaIter = mGame->getRoleInformation().getRoleCriteriaMap().begin();
    Blaze::GameManager::RoleCriteriaMap::const_iterator roleCriteriaEnd = mGame->getRoleInformation().getRoleCriteriaMap().end();
    for (; roleCriteriaIter != roleCriteriaEnd; ++roleCriteriaIter)
    {
        eastl::string roleInformation;
        roleInformation.sprintf("RoleName '%s' [Capacity=%d]", roleCriteriaIter->first.c_str(), roleCriteriaIter->second->getRoleCapacity());

        eastl::string perTeamRoleData;
        perTeamRoleData.sprintf("");
        for (uint16_t i = 0; i < mGame->getTeamCount(); ++i)
        {
            perTeamRoleData.append_sprintf("TeamIndex[%d]:%d/%d players, ", i, mGame->getRoleSizeForTeamAtIndex(i, roleCriteriaIter->first), roleCriteriaIter->second->getRoleCapacity());
        }
        valueGroupNode[roleInformation.c_str()].setDescription("RoleNames with current size and capacity");
        valueGroupNode[roleInformation.c_str()] = perTeamRoleData.c_str();
    }

    eastl::string multiRoleEntryCriteriaStr;
    for( Blaze::EntryCriteriaMap::const_iterator iter = mGame->getRoleInformation().getMultiRoleCriteria().begin(); iter != mGame->getRoleInformation().getMultiRoleCriteria().end(); ++iter )
    {
        multiRoleEntryCriteriaStr += iter->first;
        multiRoleEntryCriteriaStr += ": ";
        multiRoleEntryCriteriaStr += iter->second;
        multiRoleEntryCriteriaStr += ", ";
    }

    valueGroupNode["MultiRoleCriteria"].setDescription("MultiRole Game Entry Criteria");
    valueGroupNode["MultiRoleCriteria"] = multiRoleEntryCriteriaStr.c_str();

    valueGroupNode["Active Presence"] = ((mGame->getPresenceMode() == Blaze::PRESENCE_MODE_NONE) ? "n/a" : mPresenceStatus.c_str());
    valueGroupNode["PresenceMode"] = Blaze::PresenceModeToString(mGame->getPresenceMode());
    valueGroupNode["PersistedGameId"] = mGame->getPersistedGameId().c_str();

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
    if (mGame->getLocalPlayer() && (mGame->getLocalPlayer()->getPlayerClientPlatform() == Blaze::xone || mGame->getLocalPlayer()->getPlayerClientPlatform() == Blaze::xbsx))
    {
        valueGroupNode["Scid"].setDescription("Service Config Identifier.");
        valueGroupNode["Scid"] = mGame->getScid();
        valueGroupNode["External Session Name"].setDescription("External Session Name.");
        valueGroupNode["External Session Name"] = mGame->getExternalSessionName();
        valueGroupNode["External Session Template Name"].setDescription("External Session Template Name.");
        valueGroupNode["External Session Template Name"] = mGame->getExternalSessionTemplateName();
        valueGroupNode["External Session Correlation Id"] = mGame->getExternalSessionCorrelationId();
    }
#endif
#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
    if (mGame->getLocalPlayer() && (mGame->getLocalPlayer()->getPlayerClientPlatform() == Blaze::ps4))
    {
        valueGroupNode["NpSessionId"].setDescription("NP session id.");
        valueGroupNode["NpSessionId"] = mGame->getExternalSessionIdentification().getPs4().getNpSessionId();
    }
#endif
#if defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_PS4_CROSSGEN) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
    if (mGame->getLocalPlayer() && (mGame->getLocalPlayer()->getPlayerClientPlatform() == Blaze::ps5 || mGame->getLocalPlayer()->getPlayerClientPlatform() == Blaze::ps4))
    {
        valueGroupNode["MatchId"].setDescription("Match Id.");
        valueGroupNode["MatchId"] = mGame->getExternalSessionIdentification().getPs5().getMatch().getMatchId();
        valueGroupNode["MatchActivityObjectId"].setDescription("PSN UDS Activity Object Id for the Match.");
        valueGroupNode["MatchActivityObjectId"] = mGame->getExternalSessionIdentification().getPs5().getMatch().getActivityObjectId();
        valueGroupNode["PlayerSessionId"].setDescription("PlayerSession Id.");
        valueGroupNode["PlayerSessionId"] = mGame->getExternalSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId();
    }
#endif

    valueGroupNode["Is External Owner"].setDescription("Indicates if the local user is game's external owner.");
    valueGroupNode["Is External Owner"] = mGame->isExternalOwner();
}

void GamePlay::refreshPlayers()
{
    Pyro::UiNodeTable *playersTable = getTables().findById("Players");
    if (playersTable == nullptr)
    {
        playersTable = new Pyro::UiNodeTable("Players");
        playersTable->setPosition(Pyro::ControlPosition::TOP_MIDDLE);

        playersTable->ActionsShowing += Pyro::UiNodeTable::ActionsShowingEventHandler(this, &GamePlay::PlayerActionsShowing);

        playersTable->getActions().insert(&getActionSetActivePlayer());
        playersTable->getActions().insert(&getActionHitPlayer());
        playersTable->getActions().insert(&getActionMissPlayer());
        playersTable->getActions().insert(&getActionTorchePlayer());
        playersTable->getActions().insert(&getActionKickPlayer());
        playersTable->getActions().insert(&getActionKickPlayerPoorConnection());
        playersTable->getActions().insert(&getActionKickPlayerUnresponsive());
        playersTable->getActions().insert(&getActionBanPlayer());
        playersTable->getActions().insert(&getActionKickPlayerWithBan());
        playersTable->getActions().insert(&getActionKickPlayerPoorConnectionWithBan());
        playersTable->getActions().insert(&getActionKickPlayerUnresponsiveWithBan());
        playersTable->getActions().insert(&getActionAddPlayerToAdminList());
        playersTable->getActions().insert(&getActionRemovePlayerFromAdminList());
#if !defined(EA_PLATFORM_NX)
        playersTable->getActions().insert(&getActionUnmuteThisPlayer());
        playersTable->getActions().insert(&getActionMuteThisPlayer());
#endif
        playersTable->getActions().insert(&getActionChangeTeamId());
        playersTable->getActions().insert(&getActionChangePlayerTeamIndex());
        playersTable->getActions().insert(&getActionChangePlayerTeamIndexToUnspecified());
        playersTable->getActions().insert(&getActionChangePlayerRole());
        playersTable->getActions().insert(&getActionBecomeSpectator());
        playersTable->getActions().insert(&getActionPromoteFromQueue());
        playersTable->getActions().insert(&getActionDemoteToQueue());

        playersTable->getColumn("name").setText("Name");
        playersTable->getColumn("blazeId").setText("Blaze Id");
        playersTable->getColumn("nucleusId").setText("Nucleus Account Id");
        playersTable->getColumn("originName").setText("Origin Persona Name");
        playersTable->getColumn("platform").setText("Player Platform");
        playersTable->getColumn("xblexternalid").setText("Xbox Id");
        playersTable->getColumn("psnexternalid").setText("Playstation Id");
        playersTable->getColumn("nxexternalId").setText("Switch Id");
        playersTable->getColumn("steamexternalid").setText("Steam Id");
        playersTable->getColumn("stadiaexternalid").setText("Stadia Id");
#if defined(EA_PLATFORM_PS4)
        playersTable->getColumn("sonyId").setText("Sony Id");
#endif
        playersTable->getColumn("teamId").setText("Team ID");
        playersTable->getColumn("spectatorParticipant").setText("Participant/Spectator");
        playersTable->getColumn("publicPrivateSlot").setText("Public/Private");
        playersTable->getColumn("teamIndex").setText("Team Index");
        playersTable->getColumn("playerRole").setText("Player Role");
        playersTable->getColumn("localRemote").setText("Local/Remote");
        playersTable->getColumn("hits").setText("Hits");
        playersTable->getColumn("misses").setText("Misses");
        playersTable->getColumn("torchings").setText("Torchings");
        playersTable->getColumn("state").setText("State");
        playersTable->getColumn("ping").setText("Ping");
        playersTable->getColumn("headset").setText("Headset");
        playersTable->getColumn("headsetDS").setText("HeadsetDS");
        playersTable->getColumn("talking").setText("Talking");
        playersTable->getColumn("natType").setText("NatType");
        playersTable->getColumn("downBPS").setText("DownBPS");
        playersTable->getColumn("upBPS").setText("UpBPS");
        playersTable->getColumn("queueIdx").setText("QueueIdx");
        playersTable->getColumn("gamingPort").setText("port");

        getTables().add(playersTable);
    }
    // build list of all player attribs avail on all players, refresh always since its possible attributes get added.
    for (uint16_t a = 0; a < mGame->getPlayerCount(); ++a)
    {
        Blaze::GameManager::Player *player = mGame->getPlayerByIndex(a);
        for (Blaze::Collections::AttributeMap::const_iterator i = player->getPlayerAttributeMap().begin(); i != player->getPlayerAttributeMap().end(); ++i)
        {
            playersTable->getColumn((*i).first.c_str()).setText((*i).first.c_str());
        }
    }

    const char8_t *bestPingSiteAlias = gBlazeHub->getUserManager()->getLocalUser(gBlazeHub->getPrimaryLocalUserIndex())->getExtendedData()->getBestPingSiteAlias();

    Blaze::GameManager::Player *player;
    for (uint16_t a = 0;
        a < mGame->getPlayerCount();
        a++)
    {
        Blaze::PingSiteLatencyByAliasMap pingSiteLatencyMap(Blaze::MEM_GROUP_TITLE, "GamePlay::PingSiteLatencyByAliasMap");
        Blaze::PingSiteLatencyByAliasMap::const_iterator pingSiteLatency;

        player = mGame->getPlayerByIndex(a);

        Pyro::UiNodeTableRow &row = playersTable->getRow_va("%" PRId64, player->getId());
        
        if (player->getId() == mActivePlayerId)
        {
            row.setIconImage(Pyro::IconImage::USER_YELLOW);
        }
        else
        {
            row.setIconImage(Pyro::IconImage::USER);
        }

        Pyro::UiNodeTableRowFieldContainer &fields = row.getFields();
        fields["blazeId"] = player->getId();
        fields["nucleusId"] = player->getUser()->getNucleusAccountId();
        fields["externalId"] = player->getUser()->getExternalId();
        fields["originName"] = player->getUser()->getOriginPersonaName();
        fields["platform"] = ClientPlatformTypeToString(player->getUser()->getClientPlatform());
        fields["xblexternalid"] = player->getUser()->getXblAccountId();
        fields["psnexternalid"] = player->getUser()->getPsnAccountId();
        fields["nxexternalId"] = player->getUser()->getSwitchId();
        fields["steamexternalid"] = player->getUser()->getSteamAccountId();
        fields["stadiaexternalid"] = player->getUser()->getStadiaAccountId();
#if defined(EA_PLATFORM_PS4)
        eastl::string sonyIdStrBuf;
        fields["sonyId"] = sonyIdToStr(*player, sonyIdStrBuf).c_str();
#endif
        fields["name"] = player->getName();
        fields["spectatorParticipant"] = (player->isSpectator() ? "SPECTATOR" : "PARTICIPANT");
        fields["publicPrivateSlot"] = (((player->getSlotType() == Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT) || (player->getSlotType() == Blaze::GameManager::SLOT_PUBLIC_SPECTATOR)) ? "Public" : "Private");

        fields["teamId"] = mGame->getTeamIdByIndex(player->getTeamIndex());
        fields["teamIndex"] = player->getTeamIndex();
        fields["playerRole"] = player->getRoleName().c_str();
        fields["localRemote"] = (player->isLocalPlayer() ? "LOCAL" : "REMOTE");
        if (player->getQueueIndex() != Blaze::GameManager::INVALID_QUEUE_INDEX)
        {
            fields["queueIdx"] = player->getQueueIndex();
        }
        else
        {
            fields["queueIdx"] = "N/A";
        }

        InternalPlayerData* internalPlayerData = (InternalPlayerData *)player->getTitleContextData();

        for (Blaze::Collections::AttributeMap::const_iterator i = player->getPlayerAttributeMap().begin(); i != player->getPlayerAttributeMap().end(); ++i)
        {
            const char8_t* attribKey = (*i).first.c_str();
            const char8_t* attribVal = (*i).second.c_str();
            fields[attribKey] = attribVal;
        }

        if (internalPlayerData != nullptr)
        {
            fields["hits"].setAsInt32(internalPlayerData->mHits);
            fields["misses"].setAsInt32(internalPlayerData->mMisses);
            fields["torchings"].setAsInt32(internalPlayerData->mTorchings);
        }
        else
        {
            fields["hits"] = "";
            fields["misses"] = "";
            fields["torchings"] = "";
        }

        fields["state"] = player->getPlayerState();

        if (player->getPlayerState() == Blaze::GameManager::ACTIVE_CONNECTED)
        {
            gBlazeHub->getUserManager()->getQosPingSitesLatency(player->getId(), pingSiteLatencyMap);

            pingSiteLatency = pingSiteLatencyMap.find(bestPingSiteAlias);
            if (pingSiteLatency != pingSiteLatencyMap.end())
            {
                if (pingSiteLatency->second == (int32_t)Blaze::MAX_QOS_LATENCY)
                {
                    fields["ping"] = "UNKNOWN";
                }
                else
                {
                    fields["ping"] = pingSiteLatency->second;
                }
            }
            else
            {
                fields["ping"] = "UNAVAILABLE";
            }
        }
        else
        {
            fields["ping"] = "UNAVAILABLE";
        }

#if defined(EA_PLATFORM_NX)
        fields["headset"] = "UNAVAILABLE";
        fields["headsetDS"] = "N/A";
        fields["talking"] = "N/A";
#else
        if (internalPlayerData != nullptr && internalPlayerData->mVoipAvailable)
        {
            fields["headset"] = (internalPlayerData->mHeadset ? "Ready" : "No");
        }
        else
        {
            fields["headset"] = "UNAVAILABLE";
        }

        if (internalPlayerData != nullptr && internalPlayerData->mVoipAvailableDS)
        {
            fields["talking"] = (internalPlayerData->mTalking ? "Quiet" : "Talking");
            fields["headsetDS"] = (internalPlayerData->mHeadsetDS ? "No" : "Ready");
        }
        else 
        {
            fields["headsetDS"] = "N/A";
            fields["talking"] = "N/A";
        }
#endif

        // Nat Type
        if (player->getUser()->hasExtendedData())
        {
            fields["natType"] = player->getUser()->getExtendedData()->getQosData().getNatType();
            fields["downBPS"] = player->getUser()->getExtendedData()->getQosData().getDownstreamBitsPerSecond();
            fields["upBPS"] = player->getUser()->getExtendedData()->getQosData().getUpstreamBitsPerSecond();
        }
        else
        {
            fields["natType"] = "UNAVAILABLE";
            fields["downBPS"] = "UNAVAILABLE";
            fields["upBPS"] = "UNAVAILABLE";
        }

        if (player->getNetworkAddress() && player->getNetworkAddress()->getIpPairAddress())
        {
            fields["gamingPort"] = player->getNetworkAddress()->getIpPairAddress()->getInternalAddress().getPort();
        }

        if (((mGame->getLocalMeshEndpoint() != nullptr) && mGame->getLocalMeshEndpoint()->isDedicatedServerHost()) 
            || ((mGame->getLocalPlayer() != nullptr) && (mGame->getLocalPlayer()->getPlayerState() == Blaze::GameManager::ACTIVE_CONNECTED)))
        {
            ConnApiRefT *playerConnApiRefT = gConnApiAdapter->getConnApiRefT(player->getGame());
            if ((playerConnApiRefT != nullptr) && (player->isActive()) && (mGame->getLocalPlayer() != nullptr) &&
                (mGame->getLocalPlayer()->getPlayerState() == Blaze::GameManager::ACTIVE_CONNECTED))
            {
#if !defined(EA_PLATFORM_NX)
                VoipGroupRefT *voipGroupRef;
                ConnApiStatus(playerConnApiRefT, 'vgrp', &voipGroupRef, sizeof(voipGroupRef));
#endif

                int32_t iSelf = ConnApiStatus(playerConnApiRefT, 'self', nullptr, 0);

                gTransmission2.SetUseNetGameDist(UseNetGameDist());
                gTransmission2.SetHostIndex(mGame->getTopologyHostConnectionSlotId());

                // need to -1 for iSelf because iSelf includes the server at index 0 (NetGameDist)
                gTransmission2.SetupClients(mGame, iSelf - 1, TRANSMISSION_MAX_CLIENTS);
            }
        }
    }
    // need to refresh the player list string in the swap players UI
    initActionSwapPlayers(getActionSwapPlayers());
}

void GamePlay::updateConnectionStats(Pyro::UiNodeValueContainer &perConnectionStatsGroupNode, NetGameLinkStatT * pNetGameLinkStats)
{
    perConnectionStatsGroupNode["transmission indicator"].setDescription("show send-data indicator");
    perConnectionStatsGroupNode["transmission indicator"] = (pNetGameLinkStats->sentshow?"on":"off");

    perConnectionStatsGroupNode["reception indicator"].setDescription("show recv-data indicator");
    perConnectionStatsGroupNode["reception indicator"] = (pNetGameLinkStats->rcvdshow?"on":"off");

    perConnectionStatsGroupNode["ping deviation (ms)"].setDescription("ping deviation");
    perConnectionStatsGroupNode["ping deviation (ms)"] = pNetGameLinkStats->pingdev;

    perConnectionStatsGroupNode["ping average (ms)"].setDescription("ping average");
    perConnectionStatsGroupNode["ping average (ms)"] = pNetGameLinkStats->pingavg;

    perConnectionStatsGroupNode["p2p ping = avg + dev (ms)"].setDescription("peer to peer ping");
    perConnectionStatsGroupNode["p2p ping = avg + dev (ms)"] = pNetGameLinkStats->ping;

    perConnectionStatsGroupNode["latency (ms)"].setDescription("overall latency of game (show this to user)");
    perConnectionStatsGroupNode["latency (ms)"] = pNetGameLinkStats->late;

    perConnectionStatsGroupNode["outbound bytes"].setDescription("number of bytes sent");
    perConnectionStatsGroupNode["outbound bytes"] = pNetGameLinkStats->sent;

    perConnectionStatsGroupNode["outbound bytes/sec"].setDescription("[outbound] bytes per second (user data)");
    perConnectionStatsGroupNode["outbound bytes/sec"] = pNetGameLinkStats->outbps;

    perConnectionStatsGroupNode["outbound raw bytes/sec"].setDescription("[outbound] raw bytes per second (data sent to network)");
    perConnectionStatsGroupNode["outbound raw bytes/sec"] = pNetGameLinkStats->outrps;

    perConnectionStatsGroupNode["outbound network bytes/sec"].setDescription("[outbound] network bytes per second (rps + estimated UDP/Ethernet frame overhead)");
    perConnectionStatsGroupNode["outbound network bytes/sec"] = pNetGameLinkStats->outnps;

    perConnectionStatsGroupNode["inbound bytes"].setDescription("number of bytes received");
    perConnectionStatsGroupNode["inbound bytes"] = pNetGameLinkStats->rcvd;

    perConnectionStatsGroupNode["inbound bytes/sec"].setDescription("[inbound] received bytes per second");
    perConnectionStatsGroupNode["inbound bytes/sec"] = pNetGameLinkStats->inbps;

    perConnectionStatsGroupNode["inbound raw bytes/sec"].setDescription("[inbound] received raw bytes per second");
    perConnectionStatsGroupNode["inbound raw bytes/sec"] = pNetGameLinkStats->inrps;

    perConnectionStatsGroupNode["inbound network bytes/sec"].setDescription("[inbound] received network bytes per second");
    perConnectionStatsGroupNode["inbound network bytes/sec"] = pNetGameLinkStats->innps;

    perConnectionStatsGroupNode["outbound pkts (at last inbound counters update)"].setDescription("number of packets sent to peer since start (at time = last inbound sync packet)");
    perConnectionStatsGroupNode["outbound pkts (at last inbound counters update)"] = pNetGameLinkStats->lpacksent;

    perConnectionStatsGroupNode["outbound pkts"].setDescription("number of packets sent to peer since start (at time = now)");
    perConnectionStatsGroupNode["outbound pkts"] = pNetGameLinkStats->lpacksent_now;

    perConnectionStatsGroupNode["outbound pkts/sec"].setDescription("[outbound] packets per second (user packets)");
    perConnectionStatsGroupNode["outbound pkts/sec"] = pNetGameLinkStats->outpps;

    perConnectionStatsGroupNode["outbound raw pkts/sec"].setDescription("[outbound] raw packets per second (packets sent to network)");
    perConnectionStatsGroupNode["outbound raw pkts/sec"] = pNetGameLinkStats->outrpps;

    perConnectionStatsGroupNode["inbound pkts"].setDescription("number of packets received from peer since start");
    perConnectionStatsGroupNode["inbound pkts"] = pNetGameLinkStats->lpackrcvd;

    perConnectionStatsGroupNode["inbound pkts lost"].setDescription("remote->local packets lost: number of packets (from peer) lost since start - not always equal to (rpacksent - lpackrcvd)");
    perConnectionStatsGroupNode["inbound pkts lost"] = pNetGameLinkStats->lpacklost;

    perConnectionStatsGroupNode["inbound pkts/sec"].setDescription("[inbound] received packets per second");
    perConnectionStatsGroupNode["inbound pkts/sec"] = pNetGameLinkStats->inpps;

    perConnectionStatsGroupNode["inbound raw pkts/sec"].setDescription("[inbound] received raw packets per second (rps + estimated UDP/Ethernet frame overhead)");
    perConnectionStatsGroupNode["inbound raw pkts/sec"] = pNetGameLinkStats->inrpps;

    perConnectionStatsGroupNode["peer counter - outbound naks"].setDescription("number of NAKs sent by peer (to us) since start");
    perConnectionStatsGroupNode["peer counter - outbound naks"] = pNetGameLinkStats->rnaksent;

    perConnectionStatsGroupNode["peer counter - outbound pkts"].setDescription("number of packets sent by peer (to us) since start");
    perConnectionStatsGroupNode["peer counter - outbound pkts"] = pNetGameLinkStats->rpacksent;

    perConnectionStatsGroupNode["peer counter - inbound pkts"].setDescription("number of packets received by peer (from us) since start");
    perConnectionStatsGroupNode["peer counter - inbound pkts"] = pNetGameLinkStats->rpackrcvd;

    perConnectionStatsGroupNode["peer counter - inbound pkts lost"].setDescription("local->remote packets lost: number of packets (from us) lost by peer since start - not always equal to (lpacksent - rpackrcvd)");
    perConnectionStatsGroupNode["peer counter - inbound pkts lost"] = pNetGameLinkStats->rpacklost;
}


void GamePlay::refreshConnections()
{
    if (mGame->getNetworkTopology() == Blaze::NETWORK_DISABLED)
    {
        return;
    }

    if (((mGame->getLocalMeshEndpoint() != nullptr) && mGame->getLocalMeshEndpoint()->isDedicatedServerHost()) 
        || ((mGame->getLocalPlayer() != nullptr) && (mGame->getLocalPlayer()->getPlayerState() == Blaze::GameManager::ACTIVE_CONNECTED)))
    {
        // every 2 seconds update connection statistics
        if (NetTickDiff(NetTick(), mLastStatsUpdate) > 2000)
        {
            ConnApiRefT *pConnApi = gConnApiAdapter->getConnApiRefT(mGame);
            if (pConnApi == nullptr)
                return;
            mLastStatsUpdate = NetTick();

            // is this an otp game (dirtycast-based, not failover)
            if (mGame->getNetworkTopology() == Blaze::CLIENT_SERVER_DEDICATED)
            {
                NetGameLinkRefT *pNetGameLink;
                NetGameLinkStatT Stat;

                pNetGameLink = ConnApiGetClientList(pConnApi)->Clients[mGame->getTopologyHostConnectionSlotId()].pGameLinkRef;

                if (pNetGameLink != nullptr)
                {
                    NetGameLinkStatus(pNetGameLink, 'stat', 0, &Stat, sizeof(NetGameLinkStatT));
                    updateConnectionStats(getValuePanel("GameServer").getValues(), &Stat);
                }
            }
            else
            {
                for (uint16_t uPlayerIndex = 0; uPlayerIndex < mGame->getPlayerCount(); uPlayerIndex++)
                {
                    Blaze::GameManager::Player *player  = mGame->getPlayerByIndex(uPlayerIndex);

                    if (!player->isLocalPlayer())
                    {
                        NetGameLinkRefT * pNetGameLink = ConnApiGetClientList(pConnApi)->Clients[player->getConnectionSlotId()].pGameLinkRef;

                        if (player->getPlayerState() == Blaze::GameManager::ACTIVE_CONNECTED && pNetGameLink)
                        {
                            NetGameLinkStatT Stat;
                            NetGameLinkStatus(pNetGameLink, 'stat', 0, &Stat, sizeof(NetGameLinkStatT));
                            updateConnectionStats(getValuePanel(player->getName()).getValues(), &Stat);
                        }
                    }
                }
            }
        }
    }
}

void GamePlay::PlayerActionsShowing(Pyro::UiNodeTable *sender, Pyro::UiNodeTableRow *row)
{
    Blaze::GameManager::Player *player = mGame->getActivePlayerById(row->getField("blazeId"));
    if (player == nullptr)
    {
        // if player non-active/queued we'll disable its in-game actions.
        return;
    }
    Blaze::GameManager::Game *game = player->getGame();

    if ( ! ( 
        ((game->getNetworkTopology() == Blaze::CLIENT_SERVER_DEDICATED)) 
        && (game->isTopologyHost()) 
        ) && (game->getGameState() == Blaze::GameManager::IN_GAME))
    {
        getActionSetActivePlayer().setIsVisible(true);
        getActionHitPlayer().setIsVisible(true);
        getActionMissPlayer().setIsVisible(true);
        getActionTorchePlayer().setIsVisible(true);
    }
    else
    {
        getActionSetActivePlayer().setIsVisible(false);
        getActionHitPlayer().setIsVisible(false);
        getActionMissPlayer().setIsVisible(false);
        getActionTorchePlayer().setIsVisible(false);
    }

    getActionKickPlayer().setIsVisible(true);
    getActionKickPlayerUnresponsive().setIsVisible(true);
    getActionKickPlayerPoorConnection().setIsVisible(true);
    getActionKickPlayerWithBan().setIsVisible(true);
    getActionKickPlayerPoorConnectionWithBan().setIsVisible(true);
    getActionKickPlayerUnresponsiveWithBan().setIsVisible(true);
    getActionBanPlayer().setIsVisible(true);
    getActionAddPlayerToAdminList().setIsVisible(true);
    getActionRemovePlayerFromAdminList().setIsVisible(true);
    getActionPromoteFromQueue().setIsVisible(true);
    getActionDemoteToQueue().setIsVisible(true);

    const bool isLocalPlayerAdmin = (game->getLocalPlayer() && game->getLocalPlayer()->isAdmin());
    getActionChangeTeamId().setIsVisible(isLocalPlayerAdmin);

#if !defined(EA_PLATFORM_NX)
    if ((((mGame->getLocalMeshEndpoint() != nullptr) && mGame->getLocalMeshEndpoint()->isDedicatedServerHost()) 
        || ((mGame->getLocalPlayer() != nullptr) && (mGame->getLocalPlayer()->getPlayerState() == Blaze::GameManager::ACTIVE_CONNECTED)))
        && (game->getVoipTopology() != Blaze::VOIP_DISABLED))
    {
        ConnApiRefT *playerConnApiRefT = gConnApiAdapter->getConnApiRefT(player->getGame());
        if ((playerConnApiRefT != nullptr) && (player->isActive()) && (game->getLocalPlayer() != nullptr) &&
            (game->getLocalPlayer()->getPlayerState() == Blaze::GameManager::ACTIVE_CONNECTED))
        {
            VoipGroupRefT *voipGroupRef;
            ConnApiStatus(playerConnApiRefT, 'vgrp', &voipGroupRef, sizeof(voipGroupRef));

            if (VoipGroupIsMutedByConnId(voipGroupRef, getVoipConnIndexForPlayer(player)))
            {
                getActionUnmuteThisPlayer().setIsVisible(true);
                getActionMuteThisPlayer().setIsVisible(false);
            }
            else
            {
                getActionUnmuteThisPlayer().setIsVisible(false);
                getActionMuteThisPlayer().setIsVisible(true);
            }
        }
    }
    else
    {
        getActionUnmuteThisPlayer().setIsVisible(false);
        getActionMuteThisPlayer().setIsVisible(false);
    }
#endif
}

void GamePlay::initActionSetActivePlayer(Pyro::UiNodeAction &action)
{
    action.setText("Set Active Player");
}

void GamePlay::actionSetActivePlayer(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        if (mGame->isDedicatedServer() && mUseNetGameDist)
        {
            getUiDriver()->showMessage("This version of the Blaze sample does not "
                "support sending the custom Hit/Miss/Torche game packets over the CLIENT_SERVER_DEDICATED network "
                "topology using NetGameDist.  This feature is supported for other network topologies, such as CLIENT_SERVER_PEER_HOSTED");

            return;
        }

        player = mGame->getActivePlayerById(parameters["blazeId"]);

        if (player == nullptr)
        {
            getUiDriver()->showMessage("Error player not found in game!");
            refreshPlayers();
            return;
        }

        if (player->isLocalPlayer())
        {
            mActivePlayerId = player->getId();
        }
        else
        {
            getUiDriver()->showMessage("Error cannot set a remote player as a active user!");
            
        }

        refreshPlayers();
    }
}

void GamePlay::initActionHitPlayer(Pyro::UiNodeAction &action)
{
    action.setText("Hit Player");
}

void GamePlay::actionHitPlayer(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        if (mGame->isDedicatedServer() && mUseNetGameDist)
        {
            getUiDriver()->showMessage("This version of the Blaze sample does not "
                "support sending the custom Hit/Miss/Torche game packets over the CLIENT_SERVER_DEDICATED network "
                "topology using NetGameDist.  This feature is supported for other network topologies, such as CLIENT_SERVER_PEER_HOSTED");

            return;
        }

        player = mGame->getActivePlayerById(parameters["blazeId"]);
        if (player == nullptr)
        {
            getUiDriver()->showMessage("Error targeted player not found in game!");
            refreshPlayers();
            return;
        }
        if (player->getId() != mActivePlayerId)
        {
            IngameMessage message;

            ((InternalPlayerData *)mGame->getPlayerById(mActivePlayerId)->getTitleContextData())->mHits++;

            message.setIngameMessageType(INGAME_MESSAGE_TYPE_HIT);
            message.setShooter(mActivePlayerId);
            message.setVictim(player->getId());

            gTransmission2.GetTransmission()->SendToAll(mGame, message.getMessagePacket(), message.messageSize(), TRUE, mGame->getLocalPlayer()->getConnectionGroupId());

            refreshPlayers();
        }
        else
        {
            getUiDriver()->showMessage("You cannot perform this action on yourself");
        }
    }
}

void GamePlay::initActionMissPlayer(Pyro::UiNodeAction &action)
{
    action.setText("Miss Player");
}

void GamePlay::actionMissPlayer(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        if (mGame->isDedicatedServer() && mUseNetGameDist)
        {
            getUiDriver()->showMessage("This version of the Blaze sample does not "
                "support sending the custom Hit/Miss/Torche game packets over the CLIENT_SERVER_DEDICATED network "
                "topology using NetGameDist.  This feature is supported for other network topologies, such as CLIENT_SERVER_PEER_HOSTED");

            return;
        }

        player = mGame->getActivePlayerById(parameters["blazeId"]);
        if (player->getId() != mActivePlayerId)
        {
            IngameMessage message;

            ((InternalPlayerData *)mGame->getPlayerById(mActivePlayerId)->getTitleContextData())->mMisses++;

            message.setIngameMessageType(INGAME_MESSAGE_TYPE_MISS);
            message.setShooter(mActivePlayerId);
            message.setVictim(player->getId());

            gTransmission2.GetTransmission()->SendToAll(mGame, message.getMessagePacket(), message.messageSize(), TRUE, mGame->getLocalPlayer()->getConnectionGroupId());

            refreshPlayers();
        }
        else
        {
            getUiDriver()->showMessage("You cannot perform this action on yourself");
        }
    }
}

void GamePlay::initActionTorchePlayer(Pyro::UiNodeAction &action)
{
    action.setText("Torche Player");
}

void GamePlay::actionTorchePlayer(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        if (mGame->isDedicatedServer() && mUseNetGameDist)
        {
            getUiDriver()->showMessage("This version of the Blaze sample does not "
                "support sending the custom Hit/Miss/Torche game packets over the CLIENT_SERVER_DEDICATED network "
                "topology using NetGameDist.  This feature is supported for other network topologies, such as CLIENT_SERVER_PEER_HOSTED");

            return;
        }

        player = mGame->getActivePlayerById(parameters["blazeId"]);
        if (player->getId() != mActivePlayerId)
        {
            IngameMessage message;

            ((InternalPlayerData *)mGame->getPlayerById(mActivePlayerId)->getTitleContextData())->mTorchings++;

            message.setIngameMessageType(INGAME_MESSAGE_TYPE_TORCHE);
            message.setShooter(mActivePlayerId);
            message.setVictim(player->getId());

            gTransmission2.GetTransmission()->SendToAll(mGame, message.getMessagePacket(), message.messageSize(), TRUE, mGame->getLocalPlayer()->getConnectionGroupId());

            refreshPlayers();
        }
        else
        {
            getUiDriver()->showMessage("You cannot perform this action on yourself");
        }
    }
}

void GamePlay::initActionPromoteFromQueue(Pyro::UiNodeAction &action)
{
    action.setText("Promote From Queue");
}
void GamePlay::initActionDemoteToQueue(Pyro::UiNodeAction &action)
{
    action.setText("Demote To Queue");
}

void GamePlay::actionPromoteFromQueue(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;
        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            vLOG(mGame->addQueuedPlayer(player, Blaze::GameManager::Game::AddQueuedPlayerCb(this, &GamePlay::QueuePlayerCb)));
        }
    }
}
void GamePlay::actionDemoteToQueue(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;
        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            vLOG(mGame->demotePlayerToQueue(player, Blaze::GameManager::Game::AddQueuedPlayerCb(this, &GamePlay::QueuePlayerCb)));
        }
    }
}

void GamePlay::QueuePlayerCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game, const Blaze::GameManager::Player *player)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    if (blazeError == Blaze::ERR_OK)
    {
        refreshPlayers();
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void GamePlay::initActionKickPlayer(Pyro::UiNodeAction &action)
{
    action.setText("Kick Player (NORMAL)");
}

void GamePlay::initActionKickPlayerUnresponsive(Pyro::UiNodeAction &action)
{
    action.setText("Kick Player (UNRESPONSIVE_PEER)");
}

void GamePlay::initActionKickPlayerPoorConnection(Pyro::UiNodeAction &action)
{
    action.setText("Kick Player (POOR_CONNECTION)");
}

void GamePlay::actionKickPlayer(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            vLOG(mGame->kickPlayer(player,
                Blaze::GameManager::Game::KickPlayerCb(this, &GamePlay::KickPlayerCb)));
        }
    }
}

void GamePlay::actionKickPlayerUnresponsive(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            vLOG(mGame->kickPlayer(player,
                Blaze::GameManager::Game::KickPlayerCb(this, &GamePlay::KickPlayerCb), 
                false, 
                Blaze::GameManager::Game::KICK_UNRESPONSIVE_PEER, 
                "KICK_UNRESPONSIVE_PEER", 
                Blaze::GameManager::Game::KICK_UNRESPONSIVE_PEER));
        }
    }
}

void GamePlay::actionKickPlayerPoorConnection(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            vLOG(mGame->kickPlayer(player,
                Blaze::GameManager::Game::KickPlayerCb(this, &GamePlay::KickPlayerCb), 
                false, 
                Blaze::GameManager::Game::KICK_POOR_CONNECTION, 
                "KICK_POOR_CONNECTION", 
                Blaze::GameManager::Game::KICK_POOR_CONNECTION));
        }
    }
}

void GamePlay::initActionKickPlayerWithBan(Pyro::UiNodeAction &action)
{
    action.setText("Kick Player With Ban (NORMAL)");
}

void GamePlay::initActionKickPlayerUnresponsiveWithBan(Pyro::UiNodeAction &action)
{
    action.setText("Kick Player With Ban (UNRESPONSIVE_PEER)");
}

void GamePlay::initActionKickPlayerPoorConnectionWithBan(Pyro::UiNodeAction &action)
{
    action.setText("Kick Player With Ban (POOR_CONNECTION)");
}

void GamePlay::actionKickPlayerWithBan(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            vLOG(mGame->kickPlayer(player,
                Blaze::GameManager::Game::KickPlayerCb(this, &GamePlay::KickPlayerCb), true));
        }
    }
}

void GamePlay::actionKickPlayerUnresponsiveWithBan(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            vLOG(mGame->kickPlayer(player,
                Blaze::GameManager::Game::KickPlayerCb(this, &GamePlay::KickPlayerCb), 
                true, 
                Blaze::GameManager::Game::KICK_UNRESPONSIVE_PEER, 
                "KICK_UNRESPONSIVE_PEER", 
                Blaze::GameManager::Game::KICK_UNRESPONSIVE_PEER));
        }
    }
}

void GamePlay::actionKickPlayerPoorConnectionWithBan(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            vLOG(mGame->kickPlayer(player,
                Blaze::GameManager::Game::KickPlayerCb(this, &GamePlay::KickPlayerCb), 
                true, 
                Blaze::GameManager::Game::KICK_POOR_CONNECTION, 
                "KICK_POOR_CONNECTION", 
                Blaze::GameManager::Game::KICK_POOR_CONNECTION));
        }
    }
}

void GamePlay::KickPlayerCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game, const Blaze::GameManager::Player *player)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    if (blazeError == Blaze::ERR_OK)
    {
        refreshPlayers();
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void GamePlay::initActionBanPlayer(Pyro::UiNodeAction &action)
{
    action.setText("Ban Player");
}

void GamePlay::actionBanPlayer(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        vLOG(mGame->banUser(parameters["blazeId"],
            Blaze::GameManager::Game::BanPlayerCb(this, &GamePlay::BanPlayerCb)));
    }
}

void GamePlay::BanPlayerCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game, Blaze::BlazeId blazeId)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    if (blazeError == Blaze::ERR_OK)
    {
        refreshPlayers();
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void GamePlay::initActionAddPlayerToAdminList(Pyro::UiNodeAction &action)
{
    action.setText("Add Player To Admin List");
}

void GamePlay::actionAddPlayerToAdminList(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            vLOG(mGame->addPlayerToAdminList(player,
                Blaze::GameManager::Game::UpdateAdminListCb(this, &GamePlay::UpdateAdminListCb)));
        }
    }
}

void GamePlay::initActionRemovePlayerFromAdminList(Pyro::UiNodeAction &action)
{
    action.setText("Remove Player From Admin List");
}

void GamePlay::actionRemovePlayerFromAdminList(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            vLOG(mGame->removePlayerFromAdminList(player,
                Blaze::GameManager::Game::UpdateAdminListCb(this, &GamePlay::UpdateAdminListCb)));
        }
    }
}

void GamePlay::UpdateAdminListCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game, Blaze::GameManager::Player *player)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);
}

#if !defined(EA_PLATFORM_NX)
void GamePlay::initActionMuteThisPlayer(Pyro::UiNodeAction &action)
{
    action.setText("Mute Player");
}

void GamePlay::actionMuteThisPlayer(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            VoipGroupRefT *voipGroupRef;
            ConnApiStatus(gConnApiAdapter->getConnApiRefT(mGame), 'vgrp', &voipGroupRef, sizeof(voipGroupRef));
            VoipGroupMuteByConnId(voipGroupRef, getVoipConnIndexForPlayer(player), TRUE);

            refreshPlayers();
        }
    }
}

void GamePlay::initActionUnmuteThisPlayer(Pyro::UiNodeAction &action)
{
    action.setText("Unmute Player");
}

void GamePlay::actionUnmuteThisPlayer(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            VoipGroupRefT *voipGroupRef;
            ConnApiStatus(gConnApiAdapter->getConnApiRefT(mGame), 'vgrp', &voipGroupRef, sizeof(voipGroupRef));
            VoipGroupMuteByConnId(voipGroupRef, getVoipConnIndexForPlayer(player), FALSE);

            refreshPlayers();
        }
    }
}
#endif

void GamePlay::initActionChangeTeamId(Pyro::UiNodeAction &action)
{
    action.setText("Change Team Id");
}

void GamePlay::actionChangeTeamId(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            // gamesession.cfg would need a configured TeamId mapping for any value set here for a non-system-default PS5 team name shown in the UX
            vLOG(mGame->changeTeamIdAtIndex(player->getTeamIndex(), (Blaze::GameManager::TeamId)((mGame->getTeamIdByIndex(player->getTeamIndex()) + 1) % 6),
                Blaze::GameManager::Game::ChangeTeamIdCb(this, &GamePlay::ChangeGameTeamIdCb)));
        }
    }
}

void GamePlay::ChangeGameTeamIdCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);
}

void GamePlay::initActionChangePlayerTeamIndex(Pyro::UiNodeAction &action)
{
    action.setText("Change Team Index");
}

void GamePlay::actionChangePlayerTeamIndex(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            Blaze::GameManager::TeamIndex teamIndex = player->getTeamIndex();
            if (teamIndex != Blaze::GameManager::UNSPECIFIED_TEAM_INDEX)
            {
                if (++teamIndex >= mGame->getTeamCount())
                {
                    // if user is in last team index, move to team index 0
                    teamIndex = 0;
                }
            }
            vLOG(player->setPlayerTeamIndex(teamIndex, Blaze::GameManager::Player::ChangePlayerTeamAndRoleCb(this, &GamePlay::ChangePlayerTeamAndRoleCb)));
        }
    }
}

void GamePlay::initActionChangePlayerTeamIndexToUnspecified(Pyro::UiNodeAction &action)
{
    action.setText("Change Team Index to Unspecified");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);
}

void GamePlay::actionChangePlayerTeamIndexToUnspecified(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            vLOG(player->setPlayerTeamIndex(Blaze::GameManager::UNSPECIFIED_TEAM_INDEX, Blaze::GameManager::Player::ChangePlayerTeamAndRoleCb(this, &GamePlay::ChangePlayerTeamAndRoleCb)));
        }
    }
}

void GamePlay::ChangePlayerTeamAndRoleCb(Blaze::BlazeError blazeError, Blaze::GameManager::Player *player)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);
}

void GamePlay::initActionChangePlayerRole(Pyro::UiNodeAction &action)
{
    action.setText("Change Player Role");
}

void GamePlay::actionChangePlayerRole(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            Blaze::GameManager::RoleName roleName = player->getRoleName();
            Blaze::GameManager::RoleCriteriaMap::const_iterator roleIter = mGame->getRoleInformation().getRoleCriteriaMap().find(roleName);

            if (roleIter != mGame->getRoleInformation().getRoleCriteriaMap().end())
                ++roleIter; // advance to next role

            if (roleIter != mGame->getRoleInformation().getRoleCriteriaMap().end() )
            {
                roleName = roleIter->first;
            }
            else
            {
                // set to first role in the map
                roleName = mGame->getRoleInformation().getRoleCriteriaMap().begin()->first;
            }
            vLOG(player->setPlayerRole(roleName, Blaze::GameManager::Player::ChangePlayerTeamAndRoleCb(this, &GamePlay::ChangePlayerTeamAndRoleCb)));
        }
    }
}


void GamePlay::initActionBecomeSpectator(Pyro::UiNodeAction &action)
{
    action.setText("Become Spectator");
}

void GamePlay::actionBecomeSpectator(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Player *player;

        player = mGame->getPlayerById(parameters["blazeId"]);

        if (player != nullptr)
        {
            Blaze::GameManager::Player::ChangePlayerTeamAndRoleCb cb(this, &GamePlay::ChangePlayerTeamAndRoleCb);
            vLOG(player->becomeSpectator(cb));
        }
    }
}


void GamePlay::setupSwapPlayersParameters(Pyro::UiNodeParameterStruct &parameters, const char* tabName/* = ""*/, const char* playerIds/* = ""*/)
{
    Pyro::UiNodeParameterArrayPtr teamSetupArray = parameters.addArray("PlayerTeamData", "Swap Players Data", "Specify player team / role / slot.", Pyro::ItemVisibility::ADVANCED, tabName).getAsArray();
    Pyro::UiNodeParameter *attrStruct = new Pyro::UiNodeParameter("playerTeamData");
    attrStruct->getValue().setAsDataNodePtr(new Pyro::UiNodeParameterStruct());
    Pyro::UiNodeParameterStructPtr structPtr = attrStruct->getAsStruct();
    structPtr->addInt64("playerId", 0, "PlayerId", playerIds, "Player for this entry.");
    structPtr->addEnum("slotType", Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT, "Slot Type", "New SlotType for this player.");
    structPtr->addInt16("teamIndex", 0, "Team Index", "0,1,2,3", "New TeamIndex for this player.");
    structPtr->addString("role", "", "Role Name", ",forward,defender,goalie", "New RoleName for this player.");

    teamSetupArray->setNewItemTemplate(attrStruct);
}

void GamePlay::onGameGroupInitialized(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    refreshProperties();
}

void GamePlay::onPreGame(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    for (uint16_t a = 0; a < mGame->getActivePlayerCount(); a++)
    {
        Blaze::GameManager::Player *player = mGame->getPlayerByIndex(a);

        if (player->getTitleContextData() != nullptr)
        {
            InternalPlayerData *playerData = (InternalPlayerData*)player->getTitleContextData();
            playerData->mHits = 0;
            playerData->mMisses = 0;
            playerData->mTorchings = 0;
        }
    }

    refreshProperties();
}

void GamePlay::onPostGame(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);
}

void GamePlay::onGameStarted(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    gTransmission2.FlowControlEnable(game);

    refreshProperties();
}

void GamePlay::onGameEnded(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    gTransmission2.FlowControlDisable(game);

    if ( ((mGame->getNetworkTopology() != Blaze::CLIENT_SERVER_DEDICATED) || mGame->isTopologyHost()) && (blaze_strcmp(mGame->getGameModeAttributeName(), "mode") == 0) )
    {
        //  set game data.
        Blaze::GameReporting::SubmitGameReportRequest reportReq;

        //  Step: Set finished status if applicable to your title.
        reportReq.setFinishedStatus(Blaze::GameReporting::GAMEREPORT_FINISHED_STATUS_DEFAULT);
             
        //  Step: TODO add private data if you wish to the submit request (see the TDF for information.)
 
        //  Step: define game-type specific report.
        Blaze::GameReporting::GameReport& gameReport = reportReq.getGameReport();
        gameReport.setGameReportingId(mGame->getGameReportingId());
        gameReport.setGameReportName(mGame->getGameReportName().empty() ? "integratedSample" : mGame->getGameReportName().c_str());

        Blaze::GameReporting::IntegratedSample::Report report;

        //  Step: set game attributes that are used for keyscopes (this will be done by gamereporting implicitly for non-trusted reports, so do nothing.)
        //Blaze::GameReporting::IntegratedSample::GameAttributes &gameAttributes = report.getGameAttrs();

        // Set Player Reports
        Blaze::GameReporting::IntegratedSample::Report::PlayerReportsMap& playerMap = report.getPlayerReports();

        // Prepare player reports
        for (uint16_t a = 0; a < mGame->getActivePlayerCount(); a++)
        {
            Blaze::GameManager::Player *player = mGame->getPlayerByIndex(a);

            playerMap[player->getId()] = playerMap.allocate_element();            

            if (player->getTitleContextData() != nullptr)
            {
                InternalPlayerData *playerData = (InternalPlayerData*)player->getTitleContextData();
                playerMap[player->getId()]->setHits((uint16_t)playerData->mHits);
                playerMap[player->getId()]->setMisses((uint16_t)playerData->mMisses);
                // Walkthrough 1: Step 18 - Add a new Torchings attribute to the game report, and set the value as desired
                //playerMap[player->getId()]->setTorchings((uint16_t)playerData->mTorchings);
            }
            else
            {
                playerMap[player->getId()]->setHits(0);
                playerMap[player->getId()]->setMisses(0);
                // Walkthrough 1: Step 18 - Add a new Torchings attribute to the game report, and set the value as desired
                //playerMap[player->getId()]->setTorchings(0);
            }                      
        }

        gameReport.setReport(report, Blaze::MEM_GROUP_DEFAULT);

        // request completed, send it off!
        if (gBlazeHub->getInitParams().Client == Blaze::CLIENT_TYPE_DEDICATED_SERVER)
        {
            vLOG(gBlazeHub->getComponentManager(gBlazeHub->getUserManager()->getPrimaryLocalUserIndex())->getGameReportingComponent()->submitTrustedEndGameReport(
                reportReq, Blaze::GameReporting::GameReportingComponent::SubmitGameReportCb(this, &GamePlay::SubmitGameReportCb)));
        }
        else
        {
            vLOG(gBlazeHub->getComponentManager(gBlazeHub->getUserManager()->getPrimaryLocalUserIndex())->getGameReportingComponent()->submitGameReport(
                reportReq, Blaze::GameReporting::GameReportingComponent::SubmitGameReportCb(this, &GamePlay::SubmitGameReportCb)));
        }
    }

    if (mPrivateGameData.mGameNeedsReset == true)
    {
        mGame->returnDedicatedServerToPool(
            Blaze::GameManager::Game::ChangeGameStateCb(this, &GamePlay::ChangeGameStateCb));
    }

    refreshProperties();
}

void GamePlay::onGameReset(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    refreshProperties();

    mPrivateGameData.mGameNeedsReset = false;

    game->initGameComplete(
        Blaze::GameManager::Game::ChangeGameStateCb(this, &GamePlay::ChangeGameStateCb));
}

void GamePlay::onReturnDServerToPool(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    refreshProperties();
}

void GamePlay::onGameAttributeUpdated(Blaze::GameManager::Game *game, const Blaze::Collections::AttributeMap *changedAttributeMap)
{
    REPORT_LISTENER_CALLBACK(nullptr);
    refreshProperties();
}

void GamePlay::onDedicatedServerAttributeUpdated(Blaze::GameManager::Game *game, const Blaze::Collections::AttributeMap *changedAttributeMap)
{
    REPORT_LISTENER_CALLBACK(nullptr);
    refreshProperties();
}

void GamePlay::onGameSettingUpdated(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);
    refreshProperties();
}

void GamePlay::onGameModRegisterUpdated(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    refreshProperties();
}

void GamePlay::onGameEntryCriteriaUpdated(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    refreshProperties();
}

void GamePlay::onPlayerCapacityUpdated(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    refreshProperties();
    refreshPlayers();
}

void GamePlay::onPlayerJoining(Blaze::GameManager::Player *player)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    setupPlayer(player);

    refreshProperties();
    refreshPlayers();

    // if adding new platform host to non resetable dedicated server, advance it to pre game as needed
    if ((mGame != nullptr) && mGame->isDedicatedServer() && mGame->isServerNotResetable() && mGame->isTopologyHost() &&
        (mGame->getPlatformHostPlayer() == player) && (mGame->getGameState() == Blaze::GameManager::INITIALIZING))
    {
        mGame->initGameComplete(Blaze::GameManager::Game::ChangeGameStateCb(this, &GamePlay::ChangeGameStateCb));
    }
}

void GamePlay::onPlayerJoiningQueue(Blaze::GameManager::Player *player)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    setupPlayer(player);

    refreshPlayers();
}

void GamePlay::onPlayerJoinedFromQueue(Blaze::GameManager::Player *player)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    setupPlayer(player);

    refreshPlayers();
}

void GamePlay::onPlayerJoinedFromReservation(Blaze::GameManager::Player *player)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    setupPlayer(player);

    refreshPlayers();
}

void GamePlay::onProcessGameQueue(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    // Our default pyro implementation will let only the dedicated host client pump the queue if there is
    // a dedicated host.
    if ( (game->isDedicatedServer()) && !(game->isTopologyHost()) )
    {
        return;
    }

    pumpGameQueue(game);
}

void GamePlay::pumpGameQueue(Blaze::GameManager::Game *game)
{
    for (uint16_t i=0; i < game->getQueuedPlayerCount(); ++i)
    {
        Blaze::GameManager::Player *queuedPlayer = game->getQueuedPlayerByIndex(i);

        // game has teams
        if (game->getTeamCount() > 0)
        {
            // requested team for this player is full.
            if ((queuedPlayer->getTeamIndex() != Blaze::GameManager::UNSPECIFIED_TEAM_INDEX) &&
                (game->getTeamSizeByIndex(queuedPlayer->getTeamIndex()) >= game->getTeamCapacity()))
            {
                continue;
            }
        }

        // attempt to add this player.
        vLOG(game->addQueuedPlayer(queuedPlayer, Blaze::GameManager::Game::AddQueuedPlayerCb(this, &GamePlay::AddQueuedPlayerCb)));
        // we only attempt a single player per notification
        // Our callback handles any errors.
        return;
    }
}

void GamePlay::onGameNameChanged(Blaze::GameManager::Game *game, const Blaze::GameManager::GameName newName)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    refreshProperties();
}

void GamePlay::onPlayerJoinComplete(Blaze::GameManager::Player *player)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    // required for JiP --> When the game is already started, we will never receive the onGameStarted notification
    if (player->isLocalPlayer() && player->getGame() && (player->getGame()->getGameState() == Blaze::GameManager::IN_GAME))
    {
        refreshPlayers();
        onGameStarted(player->getGame());
    }
    else
    {
        refreshPlayers();
        refreshProperties();
    }
}

void GamePlay::onQueueChanged(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    refreshPlayers();
}

void GamePlay::onGameRecreateRequested(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);
}

void GamePlay::onPlayerRemoved(Blaze::GameManager::Game *game,
    const Blaze::GameManager::Player *removedPlayer,
    Blaze::GameManager::PlayerRemovedReason playerRemovedReason,
    Blaze::GameManager::PlayerRemovedTitleContext titleContext)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    if (removedPlayer->isLocalPlayer())
    {
        // is the removed player alone on the local console
        bool bLastLocalPlayer = true;
        for (uint16_t a = 0; a < mGame->getActivePlayerCount(); a++)
        {
            Blaze::GameManager::Player *player = mGame->getPlayerByIndex(a);

            if ((player != removedPlayer) && (player->getConnectionGroupId() == removedPlayer->getConnectionGroupId()))
            {
                bLastLocalPlayer = false;
                break;
            }
        }

        if (bLastLocalPlayer)
        {
            gTransmission2.RemoveGame(game);
        }
    }

    delete (InternalPlayerData *)removedPlayer->getTitleContextData();

    if ((game->getPlayerCount() == 0) &&
        (game->isDedicatedServer()) && (game->isTopologyHost()))
    {
        endAndReturnDedicatedServerToPool(game);
    }

    getTable("Players").getRows().removeById_va("%" PRId64, removedPlayer->getId());
    refreshProperties();

    // remove value panel used to diplay netgamelink stats for this player
    getValuePanels().removeById(removedPlayer->getName());
}

// Helper called by topology host after detecting game emptied. If IN_GAME, end the game. Return ded server to pool.
void GamePlay::endAndReturnDedicatedServerToPool(Blaze::GameManager::Game *game)
{
    if (!game->isTopologyHost())
        return;

    if ((game->getGameState() == Blaze::GameManager::PRE_GAME) || (game->getGameState() == Blaze::GameManager::POST_GAME) || (game->getGameState() == Blaze::GameManager::INITIALIZING))
    {
        if (!(game->isServerNotResetable()) && (!game->getGameSettings().getVirtualized()))
        {
            game->returnDedicatedServerToPool(
                Blaze::GameManager::Game::ChangeGameStateCb(this, &GamePlay::ChangeGameStateCb));
        }
    }
    else if (game->getGameState() == Blaze::GameManager::IN_GAME)
    {
        if (!game->getGameSettings().getVirtualized())
        {
            mPrivateGameData.mGameNeedsReset = true;
        }

        game->endGame(
            Blaze::GameManager::Game::ChangeGameStateCb(this, &GamePlay::ChangeGameStateCb));
    }
    else if (game->getGameState() == Blaze::GameManager::UNRESPONSIVE)
    {
        // we can be here if we actually became responsive again but just got the (queued) notification of the
        // state change to UNRESPONSIVE. The state change out of UNRESPONSIVE should come soon, wait for it.
        getMemo("Event log").appendText_va("[%s] empty game detected but we are still recovering from unresponsive. awaiting onBackFromUnresponsive before returning to pool", game->getName());
        if (!game->getGameSettings().getVirtualized())
        {
            mPrivateGameData.mGameNeedsReset = true;
        }
    }
}

void GamePlay::onAdminListChanged(Blaze::GameManager::Game *game,
    const Blaze::GameManager::Player *adminPlayer,
    Blaze::GameManager::UpdateAdminListOperation  operation,
    Blaze::GameManager::PlayerId updaterId)
{
    REPORT_LISTENER_CALLBACK(nullptr);
    refreshProperties();
}

void GamePlay::onPlayerAttributeUpdated(Blaze::GameManager::Player *game, const Blaze::Collections::AttributeMap *changedAttributeMap)
{
    REPORT_LISTENER_CALLBACK(nullptr);
    if (game != nullptr)
    {
        eastl::string logStr;
        attributeMapToLogStr(changedAttributeMap, logStr);
        getMemo("Event log").appendText_va("[%s] onPlayerAttributeUpdated: changed attribs: %s", game->getName(), logStr.c_str());
    }
    refreshPlayers();
}

void GamePlay::attributeMapToLogStr(const Blaze::Collections::AttributeMap *changedAttributeMap, eastl::string &logStr)
{
    if (changedAttributeMap != nullptr)
    {
        for (Blaze::Collections::AttributeMap::const_iterator itr = changedAttributeMap->begin(); itr != changedAttributeMap->end(); ++itr)
        {
            logStr.append_sprintf("\n\t['%s'] = '%s'", (*itr).first.c_str(), (*itr).second.c_str());
        }
    }
}

void GamePlay::onPlayerTeamUpdated(Blaze::GameManager::Player *game, Blaze::GameManager::TeamIndex teamIndex)
{
    REPORT_LISTENER_CALLBACK(nullptr);
    refreshProperties();
    refreshPlayers();
}

void GamePlay::onPlayerRoleUpdated(Blaze::GameManager::Player *player, Blaze::GameManager::RoleName playerRole)
{
    REPORT_LISTENER_CALLBACK(nullptr);
    refreshProperties();
    refreshPlayers();
}

void GamePlay::onPlayerSlotUpdated(Blaze::GameManager::Player *player, Blaze::GameManager::SlotType slotType)
{
    REPORT_LISTENER_CALLBACK(nullptr);
    refreshProperties();
    refreshPlayers();
}

void GamePlay::onGameTeamIdUpdated(Blaze::GameManager::Game *game, Blaze::GameManager::TeamId oldTeamId, Blaze::GameManager::TeamId newTeamId)
{
    REPORT_LISTENER_CALLBACK(nullptr);
    refreshProperties();
    refreshPlayers();
}

void GamePlay::onPlayerCustomDataUpdated(Blaze::GameManager::Player *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);
}

void GamePlay::onPlayerStateChanged(Blaze::GameManager::Player *player, Blaze::GameManager::PlayerState playerState)
{
    REPORT_LISTENER_CALLBACK(nullptr);
    refreshPlayers();
}

void GamePlay::onHostMigrationStarted(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    getMemo("chat").appendText_va("SERVER> Host migration has started. Current host is [%s]",
        (game->getPlatformHostPlayer() != nullptr ? game->getPlatformHostPlayer()->getName() : "UNKNOWN"));
}

void GamePlay::onHostMigrationEnded(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    getMemo("chat").appendText_va("SERVER> Host migration has ended. New host is [%s]",
        (game->getPlatformHostPlayer() != nullptr ? game->getPlatformHostPlayer()->getName() : "UNKNOWN"));

    Blaze::GameManager::Player* player = game->getLocalPlayer(gBlazeHub->getPrimaryLocalUserIndex());

    if ( (game->isPlatformHost() && game->isTopologyHost()) && !game->isDedicatedServer())
    {
        if ( (player != nullptr) && player->isAdmin())
        {
            getActionGameGroupCreateGame().setIsVisible(mGame->isGameTypeGroup());
            getActionGameGroupCreateGamePJD().setIsVisible(mGame->isGameTypeGroup());
            getActionGameGroupJoinGame().setIsVisible(mGame->isGameTypeGroup());
            getActionGameGroupJoinGamePJD().setIsVisible(mGame->isGameTypeGroup());
            getActionLeaveGameGroup().setIsVisible(mGame->isGameTypeGroup());
            getActionGameGroupCreateDedicatedServerGame().setIsVisible(mGame->isGameTypeGroup());
            getActionGameGroupLeaveGame().setIsVisible(mGame->isGameTypeGroup());
        }
    }

    const bool showAdminOptions = ((mGame->isDedicatedServer() && mGame->isTopologyHost()) || ((player != nullptr) && player->isAdmin()));
    getActionSetGameLockedAsBusy().setIsVisible(showAdminOptions);
    getActionSetDynamicReputationRequirement().setIsVisible(showAdminOptions);
    getActionSetAllowAnyReputation().setIsVisible(showAdminOptions);
    getActionSetOpenToJoinByPlayer().setIsVisible(showAdminOptions);
    getActionSetOpenToJoinByInvite().setIsVisible(showAdminOptions);
#if (defined(EA_PLATFORM_XBOXONE) && (_XDK_VER >= __XDK_VER_2015_MP)) || defined(EA_PLATFORM_XBSX)
    getActionSetFriendsBypassClosedToJoinByPlayer().setIsVisible(showAdminOptions);
#endif
#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
    getActionUpdateExternalSession().setIsVisible(showAdminOptions);
#endif

    refreshProperties();
}

void GamePlay::onNetworkCreated(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    setupConnApi();
}


/* TO BE DEPRECATED - will never be notified if you don't use GameManagerAPI::receivedMessageFromEndpoint() */
void GamePlay::onMessageReceived(Blaze::GameManager::Game *game, const void *buf, size_t bufSize, Blaze::BlazeId senderId, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::MessageFlagsBitset messageFlags)
{

}


void GamePlay::onUnresponsive(Blaze::GameManager::Game *game, Blaze::GameManager::GameState previousGameState)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    getMemo("chat").appendText_va("SERVER> Game became unresponsive. Awaiting reconnection..");
    refreshProperties();
}

void GamePlay::onBackFromUnresponsive(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    getMemo("chat").appendText_va("SERVER> Game came back from being unresponsive. Restored state is [%s]. Current player count is %u (%u active). %s",
        Blaze::GameManager::GameStateToString(game->getGameState()), game->getPlayerCount(), game->getActivePlayerCount(), (mPrivateGameData.mGameNeedsReset? "Returning game to pool. " : ""));
    refreshProperties();

    if (mPrivateGameData.mGameNeedsReset == true)
    {
        // ensure endGame if needed before returning to pool
        endAndReturnDedicatedServerToPool(game);
    }
}

#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
/*! DEPRECATED - in favor of using onActivePresenceUpdated(). */
void GamePlay::onNpSessionIdAvailable(Blaze::GameManager::Game *game, const char8_t* npSessionId)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    refreshProperties();
}
#endif

#if defined(EA_PLATFORM_XBOXONE)|| defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
void GamePlay::onActivePresenceUpdated(Blaze::GameManager::Game *game, Blaze::GameManager::PresenceActivityStatus status, uint32_t userIndex)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    mPresenceStatus.sprintf("%s", (status == Blaze::GameManager::PRESENCE_ACTIVE ? "yes" : "no"));
#if defined(EA_PLATFORM_XBOXONE)|| defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
    getActionSendSessionInvite().setIsVisible(status == Blaze::GameManager::PRESENCE_ACTIVE);
#endif
    refreshProperties();
}
#endif

void GamePlay::initActionLeaveGame(Pyro::UiNodeAction &action)
{
    action.setText("Leave Game");
    action.setDescription("Leave this game");

    action.getParameters().addBool("discReserv", false, "Make disconnect reservation", "Leave Game with disconnect Reservation");
}

void GamePlay::actionLeaveGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Game::LeaveGameCb cb(this, &GamePlay::LeaveGameCb);
        vLOG(mGame->leaveGame(cb, nullptr, parameters["discReserv"]));
    }
}

void GamePlay::LeaveGameCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);
}

void GamePlay::initActionLeaveGameLUGroup(Pyro::UiNodeAction &action)
{
    action.setText("Local User Group - Leave Game");
    action.setDescription("A subset of local users leaves this game");
}

void GamePlay::actionLeaveGameLUGroup(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Game::LeaveGameCb cb(this, &GamePlay::LeaveGameCb);
        vLOG(mGame->leaveGame(cb, &gBlazeHub->getUserManager()->getLocalUserGroup()));
    }
}

void GamePlay::initActionDestroyGame(Pyro::UiNodeAction &action)
{
    action.setText("Destroy Game");
    action.setDescription("Topolgy host destroys game");
}

void GamePlay::actionDestroyGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Game::DestroyGameCb cb(this, &GamePlay::DestroyGameCb);

        vLOG(mGame->destroyGame(Blaze::GameManager::TITLE_REASON_BASE_GAME_DESTRUCTION_REASON, cb));
    }
}

void GamePlay::initActionSetGameMods(Pyro::UiNodeAction &action)
{
    action.setText("Set Game Mods");
    action.setDescription("Sets Game Mod Register for the selected game");

    action.getParameters().addUInt64("newGameModReg", 0, "Game Mods", "", "Game Mod Register value to replace the current value");
}

void GamePlay::actionSetGameMods(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::Game::ChangeGameModRegisterCb cb(this, &GamePlay::ChangeGameModRegisterCb);
        vLOG(mGame->setGameModRegister(parameters["newGameModReg"], cb));
    }
}

void GamePlay::ChangeGameModRegisterCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);

    refreshProperties();
}


void GamePlay::initActionSetGameEntryCriteria(Pyro::UiNodeAction &action)
{
    action.setText("Set Game Entry Criteria");
    action.setDescription("Sets Game Entry Criteria for the selected game");

    action.getParameters().addString("gecName", "rule1", "Game Entry Criteria Name", "rule1,rule2,rule3", "The name of the GEC rule.");
    action.getParameters().addString("gecString", "", "Game Entry Criteria", ",skill>100,playerStats<50", "An evaluated string that will prevent game entry. Using UED values.");
    action.getParameters().addBool("ignoreGEC", false, "Ignore Entry Criteria for Invites", "Ignore Entry Criteria for Invites");


    action.getParameters().addString("roleName", "goalie", "Role Name", "goalie,forward,midfielder,defender", "The role associated with this GEC.");
    action.getParameters().addString("roleGecName", "roleRule1", "Role-Specific Game Entry Criteria Name", "roleRule1,roleRule2,roleRule3", "The name of the GEC rule.");
    action.getParameters().addString("roleGecString", "", "Role-Specific Game Entry Criteria", ",skill>150,playerStats<25", "An evaluated string that will prevent game entry. Using UED values.");
}

void GamePlay::actionSetGameEntryCriteria(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::EntryCriteriaMap map;
        const char* criteria =  parameters["gecString"];
        const char* name =  parameters["gecName"];
        if (criteria[0] != '\0')
        {
            map[ name ] = criteria;
        }

        Blaze::GameManager::RoleEntryCriteriaMap roleSpecificEntryCriteriaMap;
        const char* roleName = parameters["roleName"];
        const char* roleGecName = parameters["roleGecName"];
        const char* roleCriteria = parameters["roleGecString"];

        if (roleCriteria[0] != '\0')
        {   
            Blaze::EntryCriteriaMap *roleCriteriaMap = roleSpecificEntryCriteriaMap.allocate_element();
            (*roleCriteriaMap)[roleGecName] = roleCriteria;
            roleSpecificEntryCriteriaMap[roleName] = roleCriteriaMap;
        }

        Blaze::GameManager::Game::ChangeGameEntryCriteriaCb cb(this, &GamePlay::ChangeGameEntryCriteriaCb);
        vLOG(mGame->setGameEntryCriteria(map, roleSpecificEntryCriteriaMap, cb));

        // update IgnoreEntryCriteriaWithInvite
        Blaze::GameManager::GameSettings newSettings, settingsMask;
        settingsMask.setIgnoreEntryCriteriaWithInvite();
        newSettings.clearIgnoreEntryCriteriaWithInvite();
        if (parameters["ignoreGEC"])
            newSettings.setIgnoreEntryCriteriaWithInvite();
        Blaze::GameManager::Game::ChangeGameSettingsCb gsettCb(this, &GamePlay::ChangeGameSettingsCb);
        vLOG(mGame->setGameSettings(newSettings, settingsMask, gsettCb));
    }
}

void GamePlay::ChangeGameEntryCriteriaCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);

    refreshProperties();
}


void GamePlay::initActionClearGameEntryCriteria(Pyro::UiNodeAction &action)
{
    action.setText("Clear Game Entry Criteria");
    action.setDescription("Clears Game Entry Criteria for the selected game");

    action.getParameters().addBool("ignoreGEC", false, "Ignore Entry Criteria for Invites", "Ignore Entry Criteria for Invites");
}

void GamePlay::actionClearGameEntryCriteria(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::EntryCriteriaMap map;
        Blaze::GameManager::RoleEntryCriteriaMap roleCriteriaMap;
        Blaze::GameManager::Game::ChangeGameEntryCriteriaCb cb(this, &GamePlay::ChangeGameEntryCriteriaCb);
        vLOG(mGame->setGameEntryCriteria(map, roleCriteriaMap, cb));

        // update IgnoreEntryCriteriaWithInvite
        Blaze::GameManager::GameSettings newSettings, settingsMask;
        settingsMask.setIgnoreEntryCriteriaWithInvite();
        newSettings.clearIgnoreEntryCriteriaWithInvite();
        if (parameters["ignoreGEC"])
            newSettings.setIgnoreEntryCriteriaWithInvite();
        Blaze::GameManager::Game::ChangeGameSettingsCb gsettCb(this, &GamePlay::ChangeGameSettingsCb);
        vLOG(mGame->setGameSettings(newSettings, settingsMask, gsettCb));
    }
}

void GamePlay::initActionStartGame(Pyro::UiNodeAction &action)
{
    action.setText("Start Game");
    action.setDescription("Start this game");
}

void GamePlay::actionStartGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        // this code is commented out until Player::getPlayerState() is reliable - see GOS-5513
        /*
        for (uint16_t a = 0;
            a < game->getActivePlayerCount();
            a++)
        {
            Blaze::GameManager::Player *player;

            player = game->getActivePlayerByIndex(a);
            if (player->getPlayerState() != Blaze::GameManager::ACTIVE_CONNECTED)
            {
                UI->reportMessage(Pyro::ErrorLevel::NONE, "Some players are not yet fully connected.  You cannot start a game until all players in the game are connected.");
                return;
            }
        }
        */

        vLOG(mGame->startGame(
            Blaze::GameManager::Game::ChangeGameStateCb(this, &GamePlay::ChangeGameStateCb)));
    }
}

void GamePlay::initActionDirectReset(Pyro::UiNodeAction &action)
{
    action.setText("Direct Reset");
    action.setDescription("Resetable Game Server resets itself");
}

void GamePlay::actionDirectReset(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        mGame->resetDedicatedServer(Blaze::GameManager::Game::ChangeGameStateCb( this, &GamePlay::ChangeGameStateCb ));
    }
}

void GamePlay::initActionReturnDSToPool(Pyro::UiNodeAction &action)
{
    action.setText("Return DS to Pool");
    action.setDescription("Return a dedicated server game to the pool");
}

void GamePlay::actionReturnDSToPool(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        mGame->returnDedicatedServerToPool(Blaze::GameManager::Game::ChangeGameStateCb( this, &GamePlay::ChangeGameStateCb ));
    }
}

void GamePlay::initActionEjectHost(Pyro::UiNodeAction &action)
{
    action.setText("Eject Host");
    action.setDescription("Ejects the topology host from this virtual game");

    action.getParameters().addBool("replaceHost", false, "Replace Host", "Replace the host machine with a new one. Only for dedicated servers.");
}

void GamePlay::actionEjectHost(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        vLOG(mGame->ejectHost(Blaze::GameManager::Game::EjectHostCb(this, &GamePlay::EjectHostCb), parameters["replaceHost"]));
    }
}

void GamePlay::EjectHostCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    if (blazeError == Blaze::ERR_OK)
    {
        refreshProperties();
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void GamePlay::initActionEndGame(Pyro::UiNodeAction &action)
{
    action.setText("End Game");
    action.setDescription("End this game");
}

void GamePlay::actionEndGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        vLOG(mGame->endGame(
            Blaze::GameManager::Game::ChangeGameStateCb(this, &GamePlay::ChangeGameStateCb)));
    }
}

void GamePlay::initActionReplayGame(Pyro::UiNodeAction &action)
{
    action.setText("Replay Game");
    action.setDescription("Replay this game");
}

void GamePlay::actionReplayGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        vLOG(mGame->replayGame(
            Blaze::GameManager::Game::ChangeGameStateCb(this, &GamePlay::ChangeGameStateCb)));
    }
}

void GamePlay::initActionSetGameAttribute(Pyro::UiNodeAction &action)
{
    action.setText("Set Game Attribute");
    action.getParameters().addString("attribKey", "", "Game Attribute Name", "HardcoreMode,ISmap,mode,Torchings", "The name of the game attribute.");
    action.getParameters().addString("attribValue", "", "Game Attribute Value", "0,1,2,3", "Game attribute value.");
}

void GamePlay::actionSetGameAttribute(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        const char* attrKey = parameters["attribKey"];
        const char* attrVal = parameters["attribValue"];
        if ((attrKey != nullptr) && (attrKey[0] != '\0') && (attrVal != nullptr))
        {
            vLOG(mGame->setGameAttributeValue(attrKey, attrVal, Blaze::GameManager::Game::ChangeGameAttributeJobCb(this, &GamePlay::SetGameAttributeValueCb)));
        }
    }
}


void GamePlay::SetGameAttributeValueCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game* game, Blaze::JobId jobId)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError)
}

void GamePlay::initActionSetDedicatedServerAttribute(Pyro::UiNodeAction &action)
{
    action.setText("Set Dedicated Server Attribute");
    action.getParameters().addString("attribKey", "", "DS Attribute Name", "serverType,bonusFactor", "The name of the DedicatedServer attribute.");
    action.getParameters().addString("attribValue", "", "DS Attribute Value", "DLC,nonDLC,Bonus,noBonus,antiBonus", "Game attribute value.");
}

void GamePlay::actionSetDedicatedServerAttribute(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        const char* attrKey = parameters["attribKey"];
        const char* attrVal = parameters["attribValue"];
        if ((attrKey != nullptr) && (attrKey[0] != '\0') && (attrVal != nullptr))
        {
            vLOG(mGame->setDedicatedServerAttributeValue(attrKey, attrVal, Blaze::GameManager::Game::ChangeGameAttributeJobCb(this, &GamePlay::SetDedicatedServerAttributeValueCb)));
        }
    }
}


void GamePlay::SetDedicatedServerAttributeValueCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game* game, Blaze::JobId jobId)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError)
}

void GamePlay::initActionUpdateGameName(Pyro::UiNodeAction &action)
{
    action.setText("Update Game Name");
    action.setDescription("Update the name of this game");
    action.getParameters().addString("newGameName", "", "New Game Name", "", "The new name of this game");
}

void GamePlay::actionUpdateGameName(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        vLOG(mGame->updateGameName(
            parameters["newGameName"].getAsString(), Blaze::GameManager::Game::UpdateGameNameCb(this, &GamePlay::UpdateGameNameCb)));
    }
}

void GamePlay::initActionSetPlayerCapacity(Pyro::UiNodeAction &action)
{
    action.setText("Set Game Capacity");
    action.setDescription("Change this games player capacity settings");

    action.getParameters().addUInt16("publicParticipantSlots", 30, "Public Participant Slot Count", "", "");
    action.getParameters().addUInt16("privateParticipantSlots", 0, "Private Participant Slot Count", "", "");
    action.getParameters().addUInt16("publicSpectatorSlots", 0, "Public Spectator Slot Count", "", "", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addUInt16("privateSpectatorSlots", 0, "Private Spectator Slot Count", "", "", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addUInt16("teamCount", 0, "Teams Count", "0,1,2,4,8", "", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addEnum("teamAssignmentBehavior", NO_TEAM_ASSIGNMENTS, "Team Assignment Behavior", "", Pyro::ItemVisibility::ADVANCED); 
    action.getParameters().addBool("setAutoTeams", false, "Auto Setup TeamIds", "If true, team ids will be (team index + 100). If false, all will be ANY_TEAM_ID.", Pyro::ItemVisibility::ADVANCED);
    
    action.getParameters().addString("multiroleRule", "", "Multirole Rule", ",forward+defender+midfielder<10", "Changes the default multirole rule. (Other role information is unchanged)", Pyro::ItemVisibility::ADVANCED);

    // allow team / role assignment
}

void GamePlay::actionSetPlayerCapacity(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::SlotCapacities slotCapacities;
        slotCapacities[Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT] = parameters["publicParticipantSlots"];
        slotCapacities[Blaze::GameManager::SLOT_PRIVATE_PARTICIPANT] = parameters["privateParticipantSlots"];
        slotCapacities[Blaze::GameManager::SLOT_PUBLIC_SPECTATOR] = parameters["publicSpectatorSlots"];
        slotCapacities[Blaze::GameManager::SLOT_PRIVATE_SPECTATOR] = parameters["privateSpectatorSlots"];
        uint16_t teamCount = parameters["teamCount"];
        Ignition::PlayerCapacityChangeTeamSetup teamAssignmentBehavior = parameters["teamAssignmentBehavior"];

        uint32_t userIndex = 0;
        Blaze::GameManager::TeamDetailsList teamDetailsList;
        for(uint16_t i = 0; i < teamCount; ++i)
        {
            // insert a team into the game
            teamDetailsList.pull_back();
            if (parameters["setAutoTeams"])
                teamDetailsList.back()->setTeamId(i+100);
            else
                teamDetailsList.back()->setTeamId(65534); // ANY_TEAM_ID

            if (teamAssignmentBehavior != SEPERATE_LOCAL_CONNECTION_GROUP)
            {
                teamDetailsList.back()->getTeamRoster().clear();
            }
            else if ((teamCount != 0) && (userIndex < gBlazeHub->getInitParams().UserCount))
            {
                // assign users in local conn group to different teams
                Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(userIndex);
                if (localUser != nullptr)
                {
                    Blaze::GameManager::TeamMemberInfo* memberInfo = teamDetailsList.back()->getTeamRoster().pull_back();
                    memberInfo->setPlayerId(localUser->getId());
                    memberInfo->setPlayerRole(GameManagement::getInitialPlayerRole(mGame->getGameMode(), mGame->getGameType()));
                }
                ++userIndex;
            }
        }

        if ((teamCount != 0) && (teamAssignmentBehavior == ASSIGN_LOCAL_CONNECTION_GROUP_TO_SAME_TEAM))
        {
            for (userIndex = 0; userIndex < gBlazeHub->getInitParams().UserCount; ++userIndex)
            {
                Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(userIndex);
                if (localUser != nullptr)
                {
                    Blaze::GameManager::TeamMemberInfo* memberInfo = teamDetailsList.back()->getTeamRoster().pull_back();
                    memberInfo->setPlayerId(localUser->getId());
                    memberInfo->setPlayerRole(GameManagement::getInitialPlayerRole(mGame->getGameMode(), mGame->getGameType()));
                }
            }
        }

        // Role Information (not currently providing a way to set/update the roles themselves)
        Blaze::GameManager::RoleInformation roleInfo;
        bool isDefaultRoles = ((mGame->getRoleInformation().getRoleCriteriaMap().size() == 1) && (blaze_strcmp(mGame->getRoleInformation().getRoleCriteriaMap().begin()->first.c_str(), Blaze::GameManager::PLAYER_ROLE_NAME_DEFAULT) == 0));
        if (!isDefaultRoles)
        {
            mGame->getRoleInformation().copyInto(roleInfo);
        }
        const char8_t* entryCriteria = parameters["multiroleRule"];
        if ((entryCriteria != nullptr) && (entryCriteria[0] != '\0'))
        {
            roleInfo.getMultiRoleCriteria()["overall"].set(parameters["multiroleRule"]);  
        }

        vLOG(mGame->setPlayerCapacity(slotCapacities, teamDetailsList, roleInfo, Blaze::GameManager::Game::ChangePlayerCapacityCb(this, &GamePlay::SetPlayerCapacityCb)));
    }
}

void GamePlay::initActionSwapPlayers(Pyro::UiNodeAction &action)
{
    action.setText("Swap Players");
    action.setDescription("Swap player teams, roles and slots atomically");


    const char8_t *playerDetailsTab = "Player Details";
    eastl::string playerIds;

    buildPlayerList(playerIds);


    setupSwapPlayersParameters(action.getParameters(), playerDetailsTab, playerIds.c_str());
}

void GamePlay::actionSwapPlayers(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::SwapPlayerDataList swapPlayersDataList;

        Pyro::UiNodeParameterArrayPtr playerDataArray = parameters["PlayerTeamData"].getAsArray();
        if (playerDataArray != nullptr)
        {
            for (int32_t i = 0; i < (*playerDataArray).getCount(); ++i)
            {
                Pyro::UiNodeParameterStructPtr playerDetails = playerDataArray->at(i)->getAsStruct();

                swapPlayersDataList.pull_back();
                Blaze::GameManager::SwapPlayerData *playerData = swapPlayersDataList.back();

                playerData->setPlayerId(playerDetails["playerId"]);
                playerData->setSlotType(playerDetails["slotType"]);
                playerData->setTeamIndex(playerDetails["teamIndex"]);
                playerData->setRoleName(playerDetails["role"]);
                
            }
        }

        vLOG(mGame->swapPlayerTeamsAndRoles(swapPlayersDataList, Blaze::GameManager::Game::ChangePlayerCapacityCb(this, &GamePlay::SetPlayerCapacityCb)));
    }
}

void GamePlay::initActionGameGroupCreateGame(Pyro::UiNodeAction &action)
{
    action.setText("Game Group - Create Game");
    action.setDescription("Create a new game with this game group");

    GameManagement::setupCreateGameParameters(action.getParameters());
}

void GamePlay::actionGameGroupCreateGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    gBlazeHubUiDriver->getGameManagement().runCreateNewGame(*parameters, mGame);
}

void GamePlay::initActionGameGroupCreateGamePJD(Pyro::UiNodeAction &action)
{
    action.setText("Game Group - Create Game (PJD)");
    action.setDescription("Create a new game with this game group (Player Join Data can be used to set Per-Player data)");

    GameManagement::setupCreateGameParametersPJD(action.getParameters());
}

void GamePlay::actionGameGroupCreateGamePJD(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    gBlazeHubUiDriver->getGameManagement().runCreateNewGamePJD(*parameters, mGame);
}

void GamePlay::initActionGameGroupJoinGame(Pyro::UiNodeAction &action)
{
    action.setText("Game Group - Join Game");
    action.setDescription("Join this game group into a game");

    action.getParameters().addUInt64("gameId", 0, "Game Id", "", "The Id of the game you would like to join.");
    action.getParameters().addEnum("gameEntryType", Blaze::GameManager::GAME_ENTRY_TYPE_DIRECT, "Game Entry", "", Pyro::ItemVisibility::ADVANCED);
    GameManagement::setupReservedExternalPlayersParameters(action.getParameters());
}

void GamePlay::actionGameGroupJoinGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserIdentificationList reservedExternalIds;
    GameManagement::parseReservedExternalPlayersParameters(*parameters, reservedExternalIds);

    gBlazeHubUiDriver->getGameManagement().runJoinGameById(
        parameters["gameId"],
        mGame,
        Blaze::GameManager::UNSPECIFIED_TEAM_INDEX,
        parameters["gameEntryType"], nullptr, Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT, &reservedExternalIds);
}

void GamePlay::initActionGameGroupJoinGamePJD(Pyro::UiNodeAction &action)
{
    action.setText("Game Group - Join Game (PJD)");
    action.setDescription("Join this game group into a game. (Player Join Data can be used to set Per-Player data)");

    action.getParameters().addUInt64("gameId", 0, "Game Id", "", "The Id of the game you would like to join.");
    gBlazeHubUiDriver->getGameManagement().setupPlayerJoinDataParameters(action.getParameters());
}

void GamePlay::actionGameGroupJoinGamePJD(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::GameManager::PlayerJoinDataHelper playerJoinData;

    gBlazeHubUiDriver->getGameManagement().parsePlayerJoinDataParameters(*parameters, playerJoinData.getPlayerJoinData());
    gBlazeHubUiDriver->getGameManagement().runJoinGameByIdPJD(parameters["gameId"], mGame, playerJoinData);
}

void GamePlay::actionGameGroupScenarioMatchmake(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    gBlazeHubUiDriver->getGameManagement().runScenarioMatchmake(parameters, mGame);
}

void GamePlay::actionGameGroupCreateGameTemplate(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    gBlazeHubUiDriver->getGameManagement().runCreateGameTemplate(parameters, mGame);
}

void GamePlay::initActionGameGroupCreateDedicatedServerGame(Pyro::UiNodeAction &action)
{
    action.setText("Game Group - Find/Reset Dedicated Server Game");
    action.setDescription("Finds an available dedicated server for this game group, and resets it so that it can be initialized.");

    GameManagement::setupCreateGameParameters(action.getParameters(), true);
}

void GamePlay::actionGameGroupCreateDedicatedServerGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    gBlazeHubUiDriver->getGameManagement().runCreateDedicatedServerGame(*parameters, mGame);
}


void GamePlay::initActionGameGroupLeaveGame(Pyro::UiNodeAction &action)
{
    action.setText("Game Group - Leave Game");
    action.setDescription("Leave this game group from a game");

    action.getParameters().addUInt64("gameId", 0, "Game Id", "", "The Id of the game you would like to leave.");
}

void GamePlay::actionGameGroupLeaveGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::GameManager::Game *gameToLeave = gBlazeHub->getGameManagerAPI()->getGameById(parameters["gameId"]);
    if (gameToLeave != nullptr)
    {
        Blaze::GameManager::Game::LeaveGameCb cb(this, &GamePlay::LeaveGameCb);
        vLOG(gameToLeave->leaveGame(cb, mGame));
    }
    else
    {
        getUiDriver()->showMessage_va("You are not currently a member of the Game ID [%016" PRIX64 "].", parameters["gameId"].getAsUInt64());
    }
}

void GamePlay::initActionLeaveGameGroup(Pyro::UiNodeAction &action)
{
    action.setText("Leave Game Group");
    action.setDescription("Leave this game group");

    action.getParameters().addBool("discReserv", false, "Make disconnect reservation", "Leave Game Group with disconnect Reservation");
}

void GamePlay::actionLeaveGameGroup(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGameGroup != nullptr)
    {
        Blaze::GameManager::Game::LeaveGameCb cb(this, &GamePlay::LeaveGameCb);
        vLOG(mGameGroup->leaveGame(cb, nullptr, parameters["discReserv"]));
    }
}

void GamePlay::initActionSetGameLockedAsBusy(Pyro::UiNodeAction &action)
{
    action.setText("Set Lock As Busy");
    action.setDescription("Set Lock Game As Busy");
    action.getParameters().addBool("lockGameAsBusy", false, "Lock game as busy?", "Set to false to unlock a busy Game.", Pyro::ItemVisibility::ADVANCED);
}

void GamePlay::actionSetGameLockedAsBusy(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::GameSettings gameSettings, settingsMask;
        settingsMask.setLockedAsBusy();
        if (parameters["lockGameAsBusy"] == true)
            gameSettings.setLockedAsBusy();
        else
            gameSettings.clearLockedAsBusy();

        Blaze::GameManager::Game::ChangeGameSettingsCb cb(this, &GamePlay::ChangeGameSettingsCb);
        vLOG(mGame->setGameSettings(gameSettings, settingsMask, cb));
    }
}

void GamePlay::initActionSetAllowAnyReputation(Pyro::UiNodeAction &action)
{
    action.setText("Allow Any Reputation");
    action.setDescription("Toggle whether game will allow users with any reputation to join");
    action.getParameters().addBool("enableAllowAnyReputation", false, "whether to allow any reputation", "", Pyro::ItemVisibility::ADVANCED);
}

void GamePlay::actionSetAllowAnyReputation(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::GameSettings gameSettings, settingsMask;
        settingsMask.setAllowAnyReputation();
        if (parameters["enableAllowAnyReputation"] == true)
            gameSettings.setAllowAnyReputation();
        else
            gameSettings.clearAllowAnyReputation();

        Blaze::GameManager::Game::ChangeGameSettingsCb cb(this, &GamePlay::ChangeGameSettingsCb);
        vLOG(mGame->setGameSettings(gameSettings, settingsMask, cb));
    }
}

void GamePlay::initActionSetDynamicReputationRequirement(Pyro::UiNodeAction &action)
{
    action.setText("Dynamic Reputation Requirement");
    action.setDescription("Toggle whether game will use Dynamic Reputation Requirement");
    action.getParameters().addBool("enableDynamicReputation", false, "whether to use dynamic reputation requirement", "", Pyro::ItemVisibility::ADVANCED);
}

void GamePlay::actionSetDynamicReputationRequirement(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::GameSettings gameSettings, settingsMask;
        settingsMask.setDynamicReputationRequirement();
        if (parameters["enableDynamicReputation"] == true)
            gameSettings.setDynamicReputationRequirement();
        else
            gameSettings.clearDynamicReputationRequirement();

        Blaze::GameManager::Game::ChangeGameSettingsCb cb(this, &GamePlay::ChangeGameSettingsCb);
        vLOG(mGame->setGameSettings(gameSettings, settingsMask, cb));
    }
}


void GamePlay::initActionSetOpenToJoinByPlayer(Pyro::UiNodeAction &action)
{
    action.setText("Open to Join By Player");
    action.setDescription("Toggle whether game will be open to joins by player");
    action.getParameters().addBool("enableOpenToJoinByPlayer", false, "whether to open game to joins by player", "", Pyro::ItemVisibility::ADVANCED);
}

void GamePlay::actionSetOpenToJoinByPlayer(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::GameSettings gameSettings, settingsMask;
        settingsMask.setOpenToJoinByPlayer();
        if (parameters["enableOpenToJoinByPlayer"] == true)
            gameSettings.setOpenToJoinByPlayer();
        else
            gameSettings.clearOpenToJoinByPlayer();

        Blaze::GameManager::Game::ChangeGameSettingsCb cb(this, &GamePlay::ChangeGameSettingsCb);
        vLOG(mGame->setGameSettings(gameSettings, settingsMask, cb));
    }
}

void GamePlay::initActionSetFriendsBypassClosedToJoinByPlayer(Pyro::UiNodeAction &action)
{
    action.setText("Friends Bypass Join By Player");
    action.setDescription("Toggle whether game will allow friends to bypass closed to join by player");
    action.getParameters().addBool("enableFriendsBypassClosedToJoinByPlayer", false, "whether to allow friends to bypass closed to join by player", "", Pyro::ItemVisibility::ADVANCED);
}

void GamePlay::actionSetFriendsBypassClosedToJoinByPlayer(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::GameSettings gameSettings, settingsMask;
        settingsMask.setFriendsBypassClosedToJoinByPlayer();
        if (parameters["enableFriendsBypassClosedToJoinByPlayer"] == true)
            gameSettings.setFriendsBypassClosedToJoinByPlayer();
        else
            gameSettings.clearFriendsBypassClosedToJoinByPlayer();

        Blaze::GameManager::Game::ChangeGameSettingsCb cb(this, &GamePlay::ChangeGameSettingsCb);
        vLOG(mGame->setGameSettings(gameSettings, settingsMask, cb));
    }
}

void GamePlay::initActionSetOpenToJoinByInvite(Pyro::UiNodeAction &action)
{
    action.setText("Open to Join By Invite");
    action.setDescription("Toggle whether game will be open to joins by invite");
    action.getParameters().addBool("enableOpenToJoinByInvite", false, "whether to open game to joins by invite", "", Pyro::ItemVisibility::ADVANCED);
}

void GamePlay::actionSetOpenToJoinByInvite(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::GameSettings gameSettings, settingsMask;
        settingsMask.setOpenToInvites();
        if (parameters["enableOpenToJoinByInvite"] == true)
            gameSettings.setOpenToInvites();
        else
            gameSettings.clearOpenToInvites();

        Blaze::GameManager::Game::ChangeGameSettingsCb cb(this, &GamePlay::ChangeGameSettingsCb);
        vLOG(mGame->setGameSettings(gameSettings, settingsMask, cb));
    }
}

void GamePlay::initActionSetGameSettings(Pyro::UiNodeAction &action)
{
    action.setText("Set Game Settings Bitfield");
    action.setDescription("Set the Game Settings bitfield values");

    Blaze::GameManager::GameSettings gameSettings;
    const EA::TDF::TypeDescription& typeDesc = gameSettings.getTypeDescription();

    Pyro::EnumNameValueCollection bitValues;
    EA::TDF::TypeDescriptionBitfield::TdfBitfieldMembersMap::const_iterator iter = typeDesc.asBitfieldDescription()->mBitfieldMembersMap.begin();
    EA::TDF::TypeDescriptionBitfield::TdfBitfieldMembersMap::const_iterator end = typeDesc.asBitfieldDescription()->mBitfieldMembersMap.end();
    for (; iter != end; ++iter)
    {
        const EA::TDF::TypeDescriptionBitfieldMember& bitfieldInfo = static_cast<const EA::TDF::TypeDescriptionBitfieldMember &>(*iter);
        bitValues.add(bitfieldInfo.mName, (int32_t)bitfieldInfo.mBitMask);
    }
    action.getParameters().addBitSet("gameSettings", 0, bitValues, "Game Settings", "", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addBitSet("settingsMask", 0, bitValues, "Game Settings Mask", "Only values set in the mask will be updated. Default 0 updates all values.", Pyro::ItemVisibility::ADVANCED);

}

void GamePlay::actionSetGameSettings(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::GameManager::GameSettings gameSettings, settingsMask;
        gameSettings.setBits((uint32_t)parameters["gameSettings"].getAsInt32());
        settingsMask.setBits((uint32_t)parameters["settingsMask"].getAsInt32());
        Blaze::GameManager::Game::ChangeGameSettingsCb cb(this, &GamePlay::ChangeGameSettingsCb);
        vLOG(mGame->setGameSettings(gameSettings, settingsMask, cb));
    }
}

void GamePlay::ChangeGameSettingsCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);
}

void GamePlay::SubmitGameReportCb(Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);
}

void GamePlay::GameMessageCb(const Blaze::Messaging::ServerMessage *serverMessage, Blaze::GameManager::Game *game)
{
    Blaze::Messaging::MessageAttrMap::const_iterator i;

    if ((i = serverMessage->getPayload().getAttrMap().find(Blaze::Messaging::MSG_ATTR_SUBJECT)) != serverMessage->getPayload().getAttrMap().end())
    {
        //getMemo("chat").setPosition(Pyro::ControlPosition::BOTTOM_MIDDLE);
        getMemo("chat").appendText_va("%s> %s", serverMessage->getSourceIdent().getName(), (*i).second.c_str());
    }
}


#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
// for cases where titles send invites programmatically
void GamePlay::initActionSendSessionInvite(Pyro::UiNodeAction &action)
{
    eastl::string possRoles;
    for (Blaze::GameManager::RoleCriteriaMap::const_iterator itr = mGame->getRoleInformation().getRoleCriteriaMap().begin(),
        end = mGame->getRoleInformation().getRoleCriteriaMap().end(); itr != end; ++itr)
    {
        possRoles.append_sprintf(",%s", itr->first.c_str());//side: include empty
    }

    action.setText("Send Invite 1st-Party API");
    action.setDescription("Opens the session invitation dialog.");
    action.getParameters().addUInt64("inviteeExternalId", Blaze::INVALID_EXTERNAL_ID, "Invitee External Id");
}

void GamePlay::actionSendSessionInvite(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        // get invitee xuid
        Blaze::ExternalXblAccountId extId = parameters["inviteeExternalId"];
        wchar_t wstrXuid[256];
        MultiByteToWideChar(CP_UTF8, 0, eastl::string().sprintf("%" PRIu64 "", extId).c_str(), -1, wstrXuid, sizeof(wstrXuid));

#if defined(EA_PLATFORM_XBOX_GDK)
        GDKInvitesSample::sendInvite(*mGame, extId);
#else
        // Client built with old XDK
        try
        {
            //translation of data types from char -> wchar -> string
            if ((mGame->getScid() == nullptr) || (mGame->getScid()[0] == '\0') || (mGame->getExternalSessionName() == nullptr) || (mGame->getExternalSessionName()[0] == '\0') || (mGame->getExternalSessionTemplateName() == nullptr) || (mGame->getExternalSessionTemplateName()[0] == '\0'))
            {
                getUiDriver()->showMessage_va(Pyro::ErrorLevel::ERR, "Error", "actionSendSessionInvite: game missing external session parameters,  MPS calls disabled");
                return;
            }
            Microsoft::Xbox::Services::XboxLiveContext^ xblContext = ref new XboxLiveContext(Windows::Xbox::System::User::Users->GetAt(0));

            // get mps reference
            wchar_t wstrScid [256];
            wchar_t wstrSessionTemplateName[256];
            wchar_t wstrSessionName[256];
            MultiByteToWideChar(CP_UTF8, 0, mGame->getScid(), -1, wstrScid, sizeof(wstrScid));
            MultiByteToWideChar(CP_UTF8, 0, mGame->getExternalSessionTemplateName(), -1, wstrSessionTemplateName, sizeof(wstrSessionTemplateName));
            MultiByteToWideChar(CP_UTF8, 0, mGame->getExternalSessionName(), -1, wstrSessionName, sizeof(wstrSessionName));
            Platform::String^ serviceConfigurationId = ref new Platform::String(wstrScid);
            Platform::String^ sessionTemplateName = ref new Platform::String(wstrSessionTemplateName);
            Platform::String^ sessionName = ref new Platform::String(wstrSessionName);
            Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionReference^ msSessionRef = ref new Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionReference(serviceConfigurationId, sessionTemplateName, sessionName);

            Platform::Collections::Vector<Platform::String^ >^ xboxUserIdsVector = ref new Platform::Collections::Vector<Platform::String^ >();
            xboxUserIdsVector->Append(ref new Platform::String(wstrXuid));

            unsigned int titleId = (unsigned int)_wcstoui64(Windows::Xbox::Services::XboxLiveConfiguration::TitleId->Data(), nullptr, 10);

            IAsyncOperation<Windows::Foundation::Collections::IVectorView<Platform::String^ >^ >^ op =
                xblContext->MultiplayerService->SendInvitesAsync(msSessionRef, xboxUserIdsVector->GetView(), titleId);
            op->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<Collections::IVectorView<Platform::String^ >^ >(
                [](IAsyncOperation<Collections::IVectorView<Platform::String^ >^ >^ op, Windows::Foundation::AsyncStatus status)
            {
                switch(status)
                {
                case Windows::Foundation::AsyncStatus::Error:
                    NetPrintf(("actionSendSessionInvite: error: %S/0x%x\n", op->ErrorCode.ToString()->Data(), op->ErrorCode.Value));
                    break;
                default:
                    break;
                };
                op->Close();
            });
        }
        catch(Platform::Exception^ ex)
        {
            NetPrintf(("actionSendSessionInvite: failed: %S\n", ProtocolActivationInfo::getErrorStringForException(ex)->Data()));
        }
#endif
    }
}
#endif

#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
// for cases where titles send invites programmatically
void GamePlay::initActionSendSessionInvite(Pyro::UiNodeAction &action)
{
    eastl::string possRoles;
    for (Blaze::GameManager::RoleCriteriaMap::const_iterator itr = mGame->getRoleInformation().getRoleCriteriaMap().begin(),
        end = mGame->getRoleInformation().getRoleCriteriaMap().end(); itr != end; ++itr)
    {
        possRoles.append_sprintf(",%s", itr->first.c_str());//side: include empty
    }

    action.setText("Send Invite Via SDK");
    action.setDescription("Sends invite to player directly.");
    action.getParameters().addString("inviteeOnlineId", "", "Invitee onlineId/persona", "", "", Pyro::ItemVisibility::ADVANCED);
}

void GamePlay::actionSendSessionInvite(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
        // Note: PS4/DirtySessionManager requires the session to be the local user's 'primary active' session, to invite to it.
        // You may check if this game is the one by using 'gcre' (no async waiting needed), as described here: https://developer.ea.com/display/blaze/PS4+First+Party+Sessions+Integration+-+References
        char activeSessionId[SCE_NP_SESSION_ID_MAX_SIZE+1];
        uint32_t status = DirtySessionManagerStatus(gConnApiAdapter->getDirtySessionManagerRefT(), 'gcre', activeSessionId, sizeof(activeSessionId));
        if ((status == 1) && (mGame->getNpSessionId() != nullptr) && (blaze_strcmp(mGame->getNpSessionId(), activeSessionId) == 0))
        {
            eastl::string msg;
            msg.sprintf("Play Ignition game mode(%s) with me! Join my session(%s).", mGame->getGameMode(), mGame->getName());
            DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'imus', 16, 0, nullptr);
            DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'imsg', 0, 0, msg.c_str());
            DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'iued', 0, 0, nullptr);
            int64_t invitee = parameters["inviteeOnlineId"].getAsInt64();
            DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'icnp', 0, 0, nullptr);
            DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'ianp', 0, 0, &invitee);
            DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'sind', 0, 0, nullptr);   
        }
        else
        {
            getUiDriver()->showMessage_va( Pyro::ErrorLevel::ERR, "Error", "actionSendSessionInvite: game's NP session is not available for first party invites at this time");
        }
#elif defined(EA_PLATFORM_PS5) || (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
        // send invite via PS SDK WebApi2
        auto* webApi = Ignition::GameIntentsSample::getWebApi2();
        if (webApi == nullptr)
        {
            getUiDriver()->showMessage_va(Pyro::ErrorLevel::ERR, "Error", "actionSendSessionInvite: cannot invite, Web API null n/a. Check Ignition code.");
            return;
        }
        uint64_t invitee = parameters["inviteeOnlineId"].getAsUInt64();
        eastl::string body(eastl::string::CtorSprintf(), "{\"invitations\":[{\"to\":{\"accountId\":\"%" PRIu64 "\"}}]}", invitee);
        eastl::string path(eastl::string::CtorSprintf(), "/v1/playerSessions/%s/invitations", mGame->getExternalSessionIdentification().getPs5().getPlayerSession().getPlayerSessionId());
        int64_t iSceResult = DirtyWebApiRequestEx(webApi, 0, "sessionManager", SCE_NP_WEBAPI2_HTTP_METHOD_POST, path.c_str(), reinterpret_cast<const uint8_t*>(body.data()), (int32_t)body.length(), "application/json", NULL, &sendPlayerSessionInviteCb, NULL);
        if (iSceResult != SCE_OK)
        {
            NetPrintf(("[GamePlay].actionSendSessionInvite: POST(%s) sceResult(%s), body: %s\n", DirtyErrGetName(iSceResult), path.c_str(), body.c_str()));
        }
#endif
    }
}
#endif

#if defined(EA_PLATFORM_PS5) || (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
void GamePlay::sendPlayerSessionInviteCb(int32_t iSceResult, int32_t iUserIndex, int32_t iStatusCode, const char *pRespBody, int32_t iRespBodyLength, void *pUserData)
{
    NetPrintf(("[GamePlay].sendPlayerSessionInviteCb: PSN send invite sceResult(%s), statusCode(%i), rsp body: %s\n", DirtyErrGetName(iSceResult), iStatusCode, eastl::string((char8_t*)pRespBody, iRespBodyLength).c_str()));
}
#endif

#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
void GamePlay::initActionUpdateExternalSession(Pyro::UiNodeAction &action)
{
    action.setText("Update External Session");
    action.setDescription("Update the external session image, status of this game");
    GameManagement::setupExternalSessionUpdateParameters(action.getParameters());
}

void GamePlay::actionUpdateExternalSession(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
        Blaze::Ps4NpSessionImage image;
        Blaze::ExternalSessionStatus status;
        parseExternalSessionUpdateParams(*parameters, image, status);
        if ((status.getUnlocalizedValue()[0] != '\0') || !status.getLangMap().empty())
        {
            vLOG(mGame->updateExternalSessionStatus(status, Blaze::GameManager::Game::UpdateExternalSessionStatusCb(this, &GamePlay::UpdateExternalSessionStatusCb)));
        }
        if (image.getCount() > 0)
        {
            vLOG(mGame->updateExternalSessionImage(image, Blaze::GameManager::Game::UpdateExternalSessionImageCb(this, &GamePlay::UpdateExternalSessionImageCb)));
        }
    }
}

void GamePlay::parseExternalSessionUpdateParams(Pyro::UiNodeParameterStruct &parameters, Blaze::Ps4NpSessionImage& imageBlob, Blaze::ExternalSessionStatus& status)
{
    const char8_t* filePath = parameters["externalSessionImageFilePath"].getAsString();
    if ((filePath != nullptr) && (filePath[0] != '\0'))
    {
        FILE *fp = fopen(filePath, "rb");
        if (fp != nullptr)
        {
            memset(mExternalSessionImgBuf, 0, sizeof(mExternalSessionImgBuf));
            mExternalSessionImgSize = fread(mExternalSessionImgBuf, 1, sizeof(mExternalSessionImgBuf), fp);
            if (mExternalSessionImgSize > 0)
            {
                imageBlob.assignData(mExternalSessionImgBuf, mExternalSessionImgSize);
            }
            fclose(fp);
        }
        else
        {
            getMemo("Event log").appendText_va("Warning: possible sample tool error: invalid or missing external session image file. Default image will be loaded for the request.");
        }
    }

    gBlazeHubUiDriver->getGameManagement().parseExternalSessionStatus(parameters, status);
}

void GamePlay::UpdateExternalSessionImageCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game, const Blaze::ExternalSessionErrorInfo *errorInfo)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    if (blazeError != Blaze::ERR_OK)
    {
        memset(mExternalSessionImgBuf, 0, sizeof(mExternalSessionImgBuf));
    }
    REPORT_BLAZE_ERROR(blazeError);
}

void GamePlay::UpdateExternalSessionStatusCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);
}

#endif

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)

void GamePlay::initActionSubmitTournamentMatchResult(Pyro::UiNodeAction &action)
{
    action.setText("Post Tournament Match Results");
    action.setDescription("Post Tournament Match Results to External Session");
}

void GamePlay::actionSubmitTournamentMatchResult(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mGame != nullptr)
    {
#if !defined(EA_PLATFORM_XBOX_GDK)
        Microsoft::Xbox::Services::XboxLiveContext^ xblContext = ref new XboxLiveContext(Windows::Xbox::System::User::Users->GetAt(0));
        // get mps reference
        wchar_t wstrScid[256];
        wchar_t wstrSessionTemplateName[256];
        wchar_t wstrSessionName[256];
        MultiByteToWideChar(CP_UTF8, 0, mGame->getScid(), -1, wstrScid, sizeof(wstrScid));
        MultiByteToWideChar(CP_UTF8, 0, mGame->getExternalSessionTemplateName(), -1, wstrSessionTemplateName, sizeof(wstrSessionTemplateName));
        MultiByteToWideChar(CP_UTF8, 0, mGame->getExternalSessionName(), -1, wstrSessionName, sizeof(wstrSessionName));
        Platform::String^ serviceConfigurationId = ref new Platform::String(wstrScid);
        Platform::String^ sessionTemplateName = ref new Platform::String(wstrSessionTemplateName);
        Platform::String^ sessionName = ref new Platform::String(wstrSessionName);
        Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionReference^ msSessionRef = ref new Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionReference(serviceConfigurationId, sessionTemplateName, sessionName);

        IAsyncOperation<Microsoft::Xbox::Services::Multiplayer::MultiplayerSession ^>^ mpsOp = xblContext->MultiplayerService->GetCurrentSessionAsync(msSessionRef);
        mpsOp->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<MultiplayerSession ^>([xblContext](IAsyncOperation<MultiplayerSession ^>^ mpsOp, Windows::Foundation::AsyncStatus status)
        {
            if (status == Windows::Foundation::AsyncStatus::Completed)
            {
                Microsoft::Xbox::Services::Multiplayer::MultiplayerSession ^ mps = mpsOp->GetResults();

                // this sample assumes 2 teams, and at least 2 members
                Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionMember ^>^ members = mps->Members;
                if (mps->TournamentsServer == nullptr || mps->TournamentsServer->Teams == nullptr || mps->TournamentsServer->Teams->Size != 2 || members->Size < 2)
                {
                    NetPrintf(("actionSubmitTournamentMatchResult: error: invalid multiplayer session or teams"));
                    return;
                }
                Platform::Collections::Map<Platform::String^, Microsoft::Xbox::Services::Tournaments::TournamentTeamResult^>^ results = ref new Platform::Collections::Map<Platform::String ^, Microsoft::Xbox::Services::Tournaments::TournamentTeamResult ^>();
                
                Platform::String^ team1 = members->GetAt(0)->TeamId;
                Microsoft::Xbox::Services::Tournaments::TournamentTeamResult^ team1Result = ref new Microsoft::Xbox::Services::Tournaments::TournamentTeamResult();
                team1Result->State = Microsoft::Xbox::Services::Tournaments::TournamentGameResultState::Win;
                results->Insert(team1, team1Result);

                Platform::String^ team2 = members->GetAt(1)->TeamId;
                Microsoft::Xbox::Services::Tournaments::TournamentTeamResult^ team2Result = ref new Microsoft::Xbox::Services::Tournaments::TournamentTeamResult();
                team2Result->State = Microsoft::Xbox::Services::Tournaments::TournamentGameResultState::Loss;
                results->Insert(team2, team2Result);
                
                mps->SetCurrentUserArbitrationResults(results->GetView());
                // commit
                IAsyncOperation<Microsoft::Xbox::Services::Multiplayer::MultiplayerSession ^>^ mpsOp2 = xblContext->MultiplayerService->WriteSessionAsync(mps, MultiplayerSessionWriteMode::UpdateExisting);
                mpsOp2->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<MultiplayerSession ^>([xblContext](IAsyncOperation<MultiplayerSession ^>^ mpsOp2, Windows::Foundation::AsyncStatus status)
                {
                    if (status == Windows::Foundation::AsyncStatus::Error)
                    {
                        NetPrintf(("actionSubmitTournamentMatchResult: error updating arbitration results: %S/0x%x\n", mpsOp2->ErrorCode.ToString()->Data(), mpsOp2->ErrorCode.Value));
                    }
                    mpsOp2->Close();
                });
            }
            else if (status == Windows::Foundation::AsyncStatus::Error)
            {
                NetPrintf(("actionSubmitTournamentMatchResult: error fetching game mps: %S/0x%x\n", mpsOp->ErrorCode.ToString()->Data(), mpsOp->ErrorCode.Value));
            }
            mpsOp->Close();
        });
#endif
    }
}

#endif



void GamePlay::registerIdler()
{
    if (mPrivateGameData.mGameRegisteredIdler == false)
    {
       mPrivateGameData.mGameRegisteredIdler = true;

        if (mIdlerRefCount == 0)
        {
            vLOG(gBlazeHub->addIdler(this, "GamePlay"));
        }

        mIdlerRefCount++;
    }
}

void GamePlay::unregisterIdler()
{
    if (mPrivateGameData.mGameRegisteredIdler == true)
    {
        mPrivateGameData.mGameRegisteredIdler = false;

        mIdlerRefCount--;

        if (mIdlerRefCount == 0)
        {
            vLOG(gBlazeHub->removeIdler(this));
        }
    }
}


void GamePlay::buildPlayerList(eastl::string &playerIds)
{
    playerIds.sprintf("");
    bool addedPlayer = false;
    uint16_t playerCount = mGame->getPlayerCount();
    for ( uint16_t i = 0; i < playerCount; ++i)
    {

        Blaze::GameManager::Player *player = mGame->getPlayerByIndex(i);
        if ((player != nullptr) && !player->isQueued())
        {
            if (addedPlayer)
            {
                playerIds += ",";
            }
            addedPlayer = true;
            playerIds.append_sprintf("%" PRIi64, player->getId());
        }
    }
}


#if defined(EA_PLATFORM_PS4)
const eastl::string& GamePlay::sonyIdToStr(const Blaze::GameManager::Player& player, eastl::string& logStr) const
{
    PrimarySonyId sonyId;
    if (player.getUser()->getPrimarySonyId(sonyId))
    {
        logStr.sprintf("%s", sonyId.data);
    }
    return logStr;
}

#endif

void IngameMessage::setShooter(Blaze::BlazeId shooter)
{
    mShooter = shooter;
}

void IngameMessage::setVictim(Blaze::BlazeId victim)
{
    mVictim = victim;
}

void IngameMessage::setIngameMessageType(IngameMessageType messageType)
{
    mIngameMessageType = messageType;
}

Blaze::BlazeId IngameMessage::getShooter()
{
    return mShooter;
}

Blaze::BlazeId IngameMessage::getVictim()
{
    return mVictim;
}

IngameMessageType IngameMessage::getIngameMessageType()
{
    return mIngameMessageType;
}

uint64_t IngameMessage::Int64toNetwrokOrder(uint64_t iValue)
{
#ifdef EA_SYSTEM_LITTLE_ENDIAN
    
    unsigned char netw[8];
    unsigned char tempNum;
    uint64_t networkOrder = 0;

    memcpy((unsigned char *)netw, (unsigned char *)&iValue, sizeof(iValue));

    for (int32_t i = 0; i < 4; ++i) 
    {
        tempNum = netw[8-i-1];
        netw[8-i-1] = netw[i];
        netw[i] = tempNum;
    }

    for (int32_t i = 0; i < 8; ++i)
    {
        uint64_t shift = ((uint64_t)netw[i] << (8*i));
        networkOrder += shift;
    }

    return(networkOrder);

#else

    return(iValue);

#endif
}

uint64_t IngameMessage::NetworkOrderToInt64(unsigned char *pValue)
{
    uint64_t iValue = 0;
    
#ifdef EA_SYSTEM_LITTLE_ENDIAN
    
    unsigned char netw[8];
    unsigned char iTemp;

    memcpy((unsigned char *)netw, pValue, sizeof(netw));

    for (int32_t i = 0; i < 4; ++i) 
    {
        iTemp = netw[8-i-1];
        netw[8-i-1] = netw[i];
        netw[i] = iTemp;
    }

    for (int32_t i = 0; i < 8; ++i)
    {
        uint64_t iByte = ((uint64_t)netw[i] << (8*i));
        iValue += iByte;
    }

    return(iValue);

#else

    memcpy((unsigned char *)&iValue, pValue, sizeof(uint64_t));
    return(iValue);

#endif
}

char* IngameMessage::getMessagePacket()
{
    uint64_t iShooterNo = Int64toNetwrokOrder(mShooter);
    uint64_t iVictimNo = Int64toNetwrokOrder(mVictim);
    uint64_t iMessageTypeNo = Int64toNetwrokOrder((int64_t)mIngameMessageType);

    memcpy(mMessage, (unsigned char *)&iShooterNo, sizeof(uint64_t));
    memcpy(mMessage + sizeof(uint64_t), (unsigned char *)&iVictimNo, sizeof(uint64_t));
    memcpy(mMessage + (2 * sizeof(uint64_t)), (unsigned char *)&iMessageTypeNo, sizeof(uint64_t));

    return(mMessage);
}

void IngameMessage::readMessagePacket(void* buffer)
{
    unsigned char tempBuffer[sizeof(uint64_t)];

    memcpy(tempBuffer, (unsigned char *)buffer, sizeof(uint64_t));
    mShooter = NetworkOrderToInt64(tempBuffer);

    memcpy(tempBuffer, (unsigned char *)buffer + sizeof(uint64_t), sizeof(uint64_t));
    mVictim = NetworkOrderToInt64(tempBuffer);
    
    memcpy(tempBuffer, (unsigned char *)buffer + (2 * sizeof(uint64_t)), sizeof(uint64_t));
    mIngameMessageType = (IngameMessageType)NetworkOrderToInt64(tempBuffer);
}

uint32_t IngameMessage::messageSize()
{
    return(sizeof(mMessage));
}

}
