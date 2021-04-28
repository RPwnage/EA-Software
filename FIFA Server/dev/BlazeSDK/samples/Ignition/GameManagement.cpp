#if defined(EA_PLATFORM_XBOXONE)
#define NOMINMAX   // make <Windows.h> not define the min and max macros such that conflicts with eastl::min() are resolved.
#endif

#include "Ignition/GameManagement.h"

#include "Ignition/BlazeInitialization.h"
#include "Ignition/Leaderboards.h"
#include "Ignition/GamePlay.h"
#include "Ignition/MatchmakingManagement.h"
#include "Ignition/Transmission.h"

#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/gamemanager/playerjoindatahelper.h"
#include "BlazeSDK/component/gamereporting/custom/integratedsample/tdf/integratedsample.h"
#if defined(EA_PLATFORM_PS4)
#include <EAIO/EAFileDirectory.h>// for DirectoryIterator in populatePossImages
#endif
#include <time.h>


#if defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)

#include <collection.h> // required for Platform::Collections::Vector (must be included after dirtysock.h)
using namespace Windows::Foundation;
using namespace Windows::Xbox::Multiplayer;
using namespace Windows::Xbox::System;

#endif

namespace Ignition
{

GameManagement::GameManagement()
    : BlazeHubUiBuilder("Game Management"),
    mUseNetGameDist(false)
{
    getActions().add(&getActionBrowseGames());
    getActions().add(&getActionCreateNewGame());
    getActions().add(&getActionCreateNewGamePJD());
    getActions().add(&getActionJoinGameById());
    getActions().add(&getActionJoinGameByUserName());
    getActions().add(&getActionJoinGameByUserNamePJD());
    getActions().add(&getActionJoinGameByUserList());
    getActions().add(&getActionCreateOrJoinGame());

    getActions().add(&getActionCreateDedicatedServerGame());
    getActions().add(&getActionListJoinedGames());
    getActions().add(&getActionDisplayMyDisconnectReservations());
    getActions().add(&getActionOverrideGeoip());
}

GameManagement::~GameManagement()
{
    gBlazeHub->getGameManagerAPI()->removeListener(this);
    gBlazeHub->getConnectionManager()->removeListener(this);
}

void GameManagement::onAuthenticatedPrimaryUser()
{
    gBlazeHub->getGameManagerAPI()->addListener(this);
    gBlazeHub->getConnectionManager()->addListener(this);

    refreshTemplateConfigs();

    gBlazeHub->getComponentManager(gBlazeHub->getPrimaryLocalUserIndex())->getGameReportingComponent()->setResultNotificationHandler(
        Blaze::GameReporting::GameReportingComponent::ResultNotificationCb(this, &GameManagement::ResultNotificationCb));

    setIsVisibleForAll(true);

#if defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)
    // handle cases where logged-out user launched title via Xbox UX invite accept or 'Join' button click.
    Ignition::ProtocolActivationInfo::checkProtocolActivation();
#endif
#if defined(EA_PLATFORM_PS5) || (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
    Ignition::GameIntentsSample::checkTitleActivation();
#endif
}

void GameManagement::refreshTemplateConfigs()
{
    // Load the RSP: 
    gBlazeHub->getGameManagerAPI()->getGameManagerComponent()->getScenariosAndAttributes(
        Blaze::GameManager::GameManagerComponent::GetScenariosAndAttributesCb(this, &GameManagement::GetScenariosAndAttributesCb));
    gBlazeHub->getGameManagerAPI()->getGameManagerComponent()->getGameBrowserAttributesConfig(
        Blaze::GameManager::GameManagerComponent::GetGameBrowserAttributesConfigCb(this, &GameManagement::GetGameBrowserScenarioAttributesConfigCb));
    gBlazeHub->getGameManagerAPI()->getGameManagerComponent()->getTemplatesAndAttributes(
        Blaze::GameManager::GameManagerComponent::GetTemplatesAndAttributesCb(this, &GameManagement::GetTemplatesAndAttributesCb));
}

void GameManagement::onDeAuthenticatedPrimaryUser()
{
    setIsVisibleForAll(false);

    gTransmission2.RemoveAllGamesOnDeauthentication();

    gBlazeHub->getComponentManager(gBlazeHub->getPrimaryLocalUserIndex())->getGameReportingComponent()->clearResultNotificationHandler();

    gBlazeHub->getGameManagerAPI()->removeListener(this);

    getWindows().clear();
}

GamePlay &GameManagement::getGamePlayWindow(Blaze::GameManager::Game *game)
{
    const char8_t* gameTypeStr = nullptr;
    switch (game->getGameType())
    {
    case Blaze::GameManager::GAME_TYPE_GAMESESSION:
        gameTypeStr = "GameSession";
        break;
    case Blaze::GameManager::GAME_TYPE_GROUP:
        gameTypeStr = "GameGroup";
        break;
    default:
        gameTypeStr = "GamePlay";
        break;
    }
    eastl::string windowStr(eastl::string::CtorSprintf(), "%s:0x%016" PRIX64, gameTypeStr, game->getId());

    GamePlay *gamePlayWindow = static_cast<GamePlay *>(getWindows().findById_va(windowStr.c_str(), game->getId()));
    if (gamePlayWindow == nullptr)
    {
        switch (game->getGameType())
        {
            case Blaze::GameManager::GAME_TYPE_GAMESESSION:
                gameTypeStr = "GameSession";
                break;
            case Blaze::GameManager::GAME_TYPE_GROUP:
                gameTypeStr = "GameGroup";
                break;
            default:
                gameTypeStr = "GamePlay";
                break;
        }

        gamePlayWindow = new GamePlay(
            eastl::string(eastl::string::CtorSprintf(), "%s:0x%016" PRIX64, gameTypeStr, game->getId()).c_str(), game, mUseNetGameDist);

        gamePlayWindow->setAllowUserClosing(true);
        gamePlayWindow->UserClosing += Pyro::UiNodeWindow::UserClosingEventHandler(this, &GameManagement::GameWindowClosing);

        getWindows().add(gamePlayWindow);
    }

    return *gamePlayWindow;
}


MatchmakingManagement &GameManagement::getMatchmakingWindow(const Blaze::GameManager::GameId gameId)
{
    MatchmakingManagement *matchmakingWindow = static_cast<MatchmakingManagement *>(getWindows().findById_va("matchmakingMatch:0x%016" PRIX64, gameId));
    if (matchmakingWindow == nullptr)
    {
        matchmakingWindow = new MatchmakingManagement(eastl::string(eastl::string::CtorSprintf(), "matchmakingMatch:0x%016" PRIX64, gameId).c_str());

        matchmakingWindow->setAllowUserClosing(true);
        matchmakingWindow->UserClosing += Pyro::UiNodeWindow::UserClosingEventHandler(this, &GameManagement::MatchmakingWindowClosing);

        getWindows().add(matchmakingWindow);
    }

    return *matchmakingWindow;
}


void GameManagement::GameWindowClosing(Pyro::UiNodeWindow *sender)
{
    GamePlay *gamePlayWindow = (GamePlay *)sender;

    Blaze::GameManager::Game::LeaveGameCb cb(this, &GameManagement::LeaveGameCb);
    gamePlayWindow->getGame()->leaveGame(cb);
}

void GameManagement::MatchmakingWindowClosing(Pyro::UiNodeWindow *sender)
{
    getWindows().remove(sender);
}

void GameManagement::setupCreateGameParametersPJD(Pyro::UiNodeParameterStruct &parameters, bool resetGameParams)
{
    setupCreateGameBaseParameters(parameters, resetGameParams);

    setupPlayerJoinDataParameters(parameters);
}

void GameManagement::setupCreateGameParameters(Pyro::UiNodeParameterStruct &parameters, bool resetGameParams)
{
    setupCreateGameBaseParameters(parameters, resetGameParams);

    parameters.addEnum("joinSlotType", Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT, "Join Slot Type", "", Pyro::ItemVisibility::ADVANCED);
    parameters.addEnum("gameEntry", Blaze::GameManager::GAME_ENTRY_TYPE_DIRECT, "Game Entry", "", Pyro::ItemVisibility::ADVANCED);

    // reservedExternalPlayers
    setupReservedExternalPlayersParameters(parameters);

    // Teams/Roles setup
    const char8_t *tabTeamSettings = "Team Settings";
    setupTeamsCreateParameters(parameters, tabTeamSettings);

}

void GameManagement::setupCreateGameBaseParameters(Pyro::UiNodeParameterStruct &parameters, bool resetGameParams)
{
    parameters.addString("gameName", "", "Name", "", "", Pyro::ItemVisibility::ADVANCED);
    parameters.addUInt32("gameMods", 0, "Game Mod Register", "", "", Pyro::ItemVisibility::ADVANCED);
    parameters.addEnum("networkTopology", resetGameParams ? Blaze::CLIENT_SERVER_DEDICATED : Blaze::CLIENT_SERVER_PEER_HOSTED, "Network Topology", "", Pyro::ItemVisibility::ADVANCED);
#if !defined(EA_PLATFORM_NX)
    parameters.addEnum("voipTopology", resetGameParams ? Blaze::VOIP_DEDICATED_SERVER : Blaze::VOIP_PEER_TO_PEER, "Voip Topology", "", Pyro::ItemVisibility::ADVANCED);
#endif
    if (!resetGameParams)
    {
        parameters.addEnum("gameType", Blaze::GameManager::GAME_TYPE_GAMESESSION, "Game Type", "", Pyro::ItemVisibility::ADVANCED);
    }

    if (!resetGameParams)
    {
        parameters.addInt32("maxPlayerCapacity", resetGameParams ? 20 : 8, "Max Player Capacity", "1,2,4,8,10,20,30", "", Pyro::ItemVisibility::SIMPLE);
        parameters.addInt32("minPlayerCapacity", 1, "Min Player Capacity", "1,2,4,8,16", "", Pyro::ItemVisibility::SIMPLE);
    }
    parameters.addInt32("publicParticipantSlots", 8, "Public Participant Slots", "0,1,2,3,4,5,6,7,8", "", Pyro::ItemVisibility::GAME);
    parameters.addInt32("privateParticipantSlots", 0, "Private Participant Slots Slots", "0,1,2,3,4,5,6,7,8", "", Pyro::ItemVisibility::ADVANCED);
    parameters.addInt32("publicSpectatorSlots", 0, "Public Spectator Slots", "0,1,2,3,4,5,6,7,8", "", Pyro::ItemVisibility::ADVANCED);
    parameters.addInt32("privateSpectatorSlots", 0, "Private Spectator Slots", "0,1,2,3,4,5,6,7,8", "", Pyro::ItemVisibility::ADVANCED);

    parameters.addInt32("maxQueueCapacity", 0, "Max Queue Capacity", "1,2,4,8,10,20,30", "", Pyro::ItemVisibility::ADVANCED);
    if (!resetGameParams)
    {
        parameters.addBool("serverNotResetable", false, "NotResetable", "", Pyro::ItemVisibility::ADVANCED);
    }
    parameters.addEnum("presenceMode", Blaze::PRESENCE_MODE_STANDARD, "Set Presence Mode", "", Pyro::ItemVisibility::ADVANCED);
    parameters.addString("persistedGameId", "", "Persisted Game Id", "", "", Pyro::ItemVisibility::ADVANCED);
    parameters.addString("persistedGameIdSecret", "", "Persisted Game Id Secret", "", "", Pyro::ItemVisibility::ADVANCED);
    if (!resetGameParams)
    {
        parameters.addString("gamePingSiteAlias", "bio-sjc", "Ping Site Override", "sjc,iad,gva,bio-sjc,bio-iad", "", Pyro::ItemVisibility::ADVANCED);
    }

    // DirtySDK mode
    parameters.addBool("enableNetGameDist", false, "Enable NetGameDist", "Configure DirtySDK and Blaze to use NetGameDist", Pyro::ItemVisibility::ADVANCED);

    const char8_t *gameSettingsTab = "Game Settings";
    setupGameSettings(parameters, gameSettingsTab);

    const char8_t *gameAttributesTab = "Game Attributes";

    // Game Attribute Rule Mode
    parameters.addString("gameAttributeMode", "tdeathmatch", "Mode", "deathmatch,tdeathmatch", "", Pyro::ItemVisibility::ADVANCED, gameAttributesTab);

    // Game Attribute Rule Map
    parameters.addString("gameAttributeMap", "map_1", "Map", "map_1,map_2,map_3", "", Pyro::ItemVisibility::ADVANCED, gameAttributesTab);

    setupGameAttributeParameters(parameters, false, gameAttributesTab);

    parameters.addBool("createByLUGroup", false, "Create By Local User Group", "", Pyro::ItemVisibility::SIMPLE);

    setupExternalSessionCreateParameters(parameters);

    const char8_t* roleInformationTab = "Role Information";
    setupRoleInformationParameters(parameters, roleInformationTab);

    // Team Ids: 
    Pyro::UiNodeParameterArrayPtr teamIdsArray = parameters.addArray("teamIds", "Team Ids", "List of Team Ids used in the game.").getAsArray();

    Pyro::UiNodeParameter* teamId = new Pyro::UiNodeParameter("TeamId");
    teamId->setText("Team Id");
    teamId->setAsUInt16(Blaze::GameManager::ANY_TEAM_ID);
    teamId->getAvailableValues().setValues("101,102,103,104,65534,65535");

    // Associate it with the underlying data type: 
    teamIdsArray->setNewItemTemplate(teamId);
}

void GameManagement::setupRoleInformationParameters(Pyro::UiNodeParameterStruct &parameters, const char8_t* tabName)
{
    {
        // Role Criteria entries:
        Pyro::UiNodeParameterMapPtr roleCriteriaMap = parameters.addMap("roleCriteria", "Role Size Criteria", "Role sizes and Criteria.", Pyro::ItemVisibility::ADVANCED, tabName).getAsMap();

        Pyro::UiNodeParameter* roleName = new Pyro::UiNodeParameter("Role");
        roleName->setText("Role");
        roleName->setAsString("");
        roleName->getAvailableValues().setValues(",forward,midfielder,defender,goalie") ;

        Pyro::UiNodeParameter* roleCriteria = new Pyro::UiNodeParameter("RoleCriteria");
        roleCriteria->setText("Role Criteria");

        roleCriteria->getValue().setAsDataNodePtr(new Pyro::UiNodeParameterStruct());
        Pyro::UiNodeParameterStructPtr roleCriteriaPtr = roleCriteria->getAsStruct();

        roleCriteriaPtr->addUInt16("roleCapacity", 0, "Role Capacity", "0,1,2,4,8", "How many players can be in this role?");
        Pyro::UiNodeParameterMapPtr roleEntryCriteriaMap = roleCriteriaPtr->addMap("roleEntryCriteria", "Role Entry Criteria", "Rules that define the entry criteria for the role.").getAsMap();

        // Create the key value pair:
        Pyro::UiNodeParameter* entryName = new Pyro::UiNodeParameter("Name");
        entryName->setText("Name");
        entryName->setAsString("");

        Pyro::UiNodeParameter* entryValue = new Pyro::UiNodeParameter("EntryCriteria");
        entryValue->setText("Entry Criteria");
        entryValue->setAsString("");

        Pyro::UiNodeParameterMapKeyValuePair* criteriaEntry = new Pyro::UiNodeParameterMapKeyValuePair();

        // Associate it with the underlying data type: 
        criteriaEntry->setMapKey(entryName);
        criteriaEntry->setMapValue(entryValue);
        roleEntryCriteriaMap->setKeyValueType(criteriaEntry);

        Pyro::UiNodeParameterMapKeyValuePair* roleCriteriaMapEntry = new Pyro::UiNodeParameterMapKeyValuePair();
        roleCriteriaMapEntry->setMapKey(roleName);
        roleCriteriaMapEntry->setMapValue(roleCriteria);
        roleCriteriaMap->setKeyValueType(roleCriteriaMapEntry);
    }

    {
        // Multi Role entry criteria:
        Pyro::UiNodeParameterMapPtr multiRoleCriteriaMap = parameters.addMap("multiRoleCriteria", "Multi-Role Criteria", "Criteria that applies to multiple roles.", Pyro::ItemVisibility::ADVANCED, tabName).getAsMap();

        // Create the key value pair:
        Pyro::UiNodeParameter* entryName = new Pyro::UiNodeParameter("Name");
        entryName->setText("Name");
        entryName->setAsString("");

        Pyro::UiNodeParameter* entryValue = new Pyro::UiNodeParameter("EntryCriteria");
        entryValue->setText("Entry Criteria");
        entryValue->setAsString("");

        Pyro::UiNodeParameterMapKeyValuePair* criteriaEntry = new Pyro::UiNodeParameterMapKeyValuePair();

        // Associate it with the underlying data type: 
        criteriaEntry->setMapKey(entryName);
        criteriaEntry->setMapValue(entryValue);
        multiRoleCriteriaMap->setKeyValueType(criteriaEntry);
    }
}

void GameManagement::parseRoleInformationParameters(Pyro::UiNodeParameterStruct &parameters, Blaze::GameManager::RoleInformation &roleInformation)
{
    Pyro::UiNodeParameterMapPtr roleCriteriaMapPtr = parameters["roleCriteria"].getAsMap();   
    if (roleCriteriaMapPtr != nullptr)
    {
        for (int32_t i = 0; i < (*roleCriteriaMapPtr).getCount(); ++i)
        {
            Pyro::UiNodeParameterMapKeyValuePair* uiMapKeyValuePair = roleCriteriaMapPtr->at(i);
            const char8_t* roleName = uiMapKeyValuePair->getMapKey()->getAsString();
            Pyro::UiNodeParameterStructPtr roleCriteriaPtr = uiMapKeyValuePair->getMapValue()->getAsStruct();

            Blaze::GameManager::RoleCriteria *roleCriteria = roleInformation.getRoleCriteriaMap().allocate_element();
            roleCriteria->setRoleCapacity((*roleCriteriaPtr)["roleCapacity"].getAsUInt16());
            roleInformation.getRoleCriteriaMap()[roleName] = roleCriteria;

            Pyro::UiNodeParameterMapPtr roleEntryCriteriaMapPtr = (*roleCriteriaPtr)["roleEntryCriteria"].getAsMap();
            if (roleEntryCriteriaMapPtr != nullptr)
            {
                for (int32_t j = 0; j < (*roleEntryCriteriaMapPtr).getCount(); ++j)
                {
                    Pyro::UiNodeParameterMapKeyValuePair* roleEntryCriteriaMapKeyValuePair = roleEntryCriteriaMapPtr->at(j);
                    const char8_t* ruleName = roleEntryCriteriaMapKeyValuePair->getMapKey()->getAsString();
                    const char8_t* entryCriteria = roleEntryCriteriaMapKeyValuePair->getMapValue()->getAsString();

                    roleCriteria->getRoleEntryCriteriaMap()[ruleName] = entryCriteria;
                }
            }
        }
    }

    Pyro::UiNodeParameterMapPtr multiRoleCriteriaMapPtr = parameters["multiRoleCriteria"].getAsMap();   
    if (roleCriteriaMapPtr != nullptr)
    {
        for (int32_t i = 0; i < (*multiRoleCriteriaMapPtr).getCount(); ++i)
        {
            Pyro::UiNodeParameterMapKeyValuePair* roleEntryCriteriaMapKeyValuePair = multiRoleCriteriaMapPtr->at(i);
            const char8_t* ruleName = roleEntryCriteriaMapKeyValuePair->getMapKey()->getAsString();
            const char8_t* entryCriteria = roleEntryCriteriaMapKeyValuePair->getMapValue()->getAsString();

            roleInformation.getMultiRoleCriteria()[ruleName] = entryCriteria;
        }
    }
}


void GameManagement::setupBasicGameBrowserParameters(Pyro::UiNodeParameterStruct &parameters)
{
    parameters.addString("listConfigName", "default", "List Config", "default,defaultUnsorted,fpsUnsorted", "", Pyro::ItemVisibility::ADVANCED);
    parameters.addInt32("maxGamesInList", 100, "Max Games To Show", "1,2,4,8,10,20,30,50,100,200,4294967295", "", Pyro::ItemVisibility::SIMPLE);
    parameters.addEnum("listType", Blaze::GameManager::GameBrowserList::LIST_TYPE_SNAPSHOT, "List Type", "", Pyro::ItemVisibility::SIMPLE);
    
    parameters.addInt32("upstreamBPS", 0, "Upstream Bits Per Second", "0,100,200,300,400,500", "", Pyro::ItemVisibility::ADVANCED);
    
    parameters.addBool("enableModRule", false, "Use Game Mod Rule", "", Pyro::ItemVisibility::ADVANCED);
    parameters.addUInt32("desiredMods", 0, "Desired Mods", "", "", Pyro::ItemVisibility::ADVANCED);
    // Virtual Game Rule
    parameters.addBool("virtualGames", false, "Virtual Games", "", Pyro::ItemVisibility::ADVANCED);
    // Reputation Rule
    parameters.addBool("anyReputationGames", false, "Poor Reputation Match Opt-In", "", Pyro::ItemVisibility::ADVANCED);

    // New Game Slot Rules (not used with legacy Game Size Rule)
    // Player Count
    parameters.addInt32("playerCountRuleMin", 1, "Player Slots Filled: Min Player Count", "1,2,3,4,8,10,20,30", "", Pyro::ItemVisibility::ADVANCED);
    parameters.addInt32("playerCountRuleDes", 2, "Player Slots Filled: Desired Player Count", "1,2,3,4,8,10,20,30", "", Pyro::ItemVisibility::ADVANCED);
    parameters.addInt32("playerCountRuleMax", 30, "Player Slots Filled: Max Player Count", "1,2,3,4,8,10,20,30", "", Pyro::ItemVisibility::ADVANCED);

    const char8_t *playerSlotsTab = "Player Slots Advanced Criteria";
    // Total Player Slots
    parameters.addInt32("minTotalPlayerSlots", 2, "Total Player Slots: Minimum", "1,2,4,8,10,16,20,32,64", "", Pyro::ItemVisibility::ADVANCED, playerSlotsTab);
    parameters.addInt32("desiredTotalPlayerSlots", 4, "Total Player Slots: Desired", "1,2,4,8,10,16,20,32,64", "", Pyro::ItemVisibility::ADVANCED, playerSlotsTab);
    parameters.addInt32("maxTotalPlayerSlots", 64, "Total Player Slots: Maximum", "1,2,4,8,10,16,20,32,64", "", Pyro::ItemVisibility::ADVANCED, playerSlotsTab);
    // Player Slot Utilization
    parameters.addInt32("minPercentFull", 0, "Percent Player Slots Filled: Minimum", "0,1,10,20,50,75,80,90,99,100", "", Pyro::ItemVisibility::ADVANCED, playerSlotsTab);
    parameters.addInt32("desiredPercentFull", 50, "Percent Player Slots Filled: Desired", "0,1,10,20,50,75,80,90,99,100", "", Pyro::ItemVisibility::ADVANCED, playerSlotsTab);
    parameters.addInt32("maxPercentFull", 100, "Percent Player Slots Filled: Maximum", "0,1,10,20,50,75,80,90,99,100", "", Pyro::ItemVisibility::ADVANCED, playerSlotsTab);
    // Free Player Slots
    parameters.addInt32("minFreePlayerSlots", 0, "Free Player Slots: Minimum", "0,1,2,3,4,5,6,7,8,9,10,20,30", "", Pyro::ItemVisibility::ADVANCED, playerSlotsTab);
    parameters.addInt32("maxFreePlayerSlots", 64, "Free Player Slots: Maximum", "0,1,2,3,4,5,6,7,8,9,10,20,30", "", Pyro::ItemVisibility::ADVANCED, playerSlotsTab);


    parameters.addString("gameNameSearchString", "", "Game Name Substring (optional)", "game,server,com,nexus", "", Pyro::ItemVisibility::ADVANCED);
    //quicksprintf("%s,%s", gBlazeHub->getUserManager()->getLocalUser(0)->getName(),);
}

void GameManagement::setupCommonJoinGameParameters(Pyro::UiNodeParameterStruct &parameters)
{
    parameters.addEnum("gameType", Blaze::GameManager::GAME_TYPE_GAMESESSION, "Game Type", "", Pyro::ItemVisibility::ADVANCED);
    parameters.addEnum("gameEntry", Blaze::GameManager::GAME_ENTRY_TYPE_DIRECT, "Game Entry", "", Pyro::ItemVisibility::ADVANCED);
    parameters.addInt32("teamIndex", 65535, "Team Index", "65535,65534,0,1,2,3,4", "", Pyro::ItemVisibility::ADVANCED);
    setupReservedExternalPlayersParameters(parameters);

    parameters.addString("playerRoles", "", "Joining Role", ",forward,midfielder,defender,goalie", "", Pyro::ItemVisibility::ADVANCED);
    parameters.addEnum("slotType", Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT, "Slot Type", "The slot type to request for this join.", Pyro::ItemVisibility::ADVANCED);
    parameters.addBool("joinByLUGroup", false, "Join by Local User Group", "", Pyro::ItemVisibility::SIMPLE);

    // DirtySDK mode
    parameters.addBool("enableNetGameDist", false, "Enable NetGameDist", "Configure DirtySDK and Blaze to use NetGameDist", Pyro::ItemVisibility::ADVANCED);
}

void GameManagement::setupJoinGameByIdParameters(Pyro::UiNodeParameterStruct &parameters)
{
    parameters.addUInt64("gameId", 0, "Game Id", "", "The GameId of the game you would like to join.", Pyro::ItemVisibility::SIMPLE);
    setupCommonJoinGameParameters(parameters);
}

void GameManagement::setupJoinGameByUserNameParameters(Pyro::UiNodeParameterStruct &parameters)
{
    parameters.addString("userName", "", "User name", "", "The name of a user in a game you would like to join.", Pyro::ItemVisibility::ADVANCED);
    setupCommonJoinGameParameters(parameters);
}

void GameManagement::setupTeamsCreateParameters(Pyro::UiNodeParameterStruct &parameters, const char* tabName /*= ""*/)
{
    parameters.addUInt16("joiningTeamIndex", 0, "My Team Index", "0,1,2,3,4, 65535", "The index of the team for the joining player(s) to join", Pyro::ItemVisibility::ADVANCED, tabName);
    parameters.addUInt16("numberOfTeams", 1, "Number of Teams", "1,2,3,4", "The number of teams in the game.  Team Capacity will be Total / number of teams", Pyro::ItemVisibility::ADVANCED, tabName);
    parameters.addEnum("sampleRoles", DEFAULT_ROLES, "Sample Role Setup", "Select a sample roles setup, which mimics a football game.", Pyro::ItemVisibility::ADVANCED, tabName);
}

template <typename T>
void parseTeamsCreateParameters(Pyro::UiNodeParameterStruct &parameters, T &createGameParams, GameManagement& gameManagement)
{
    uint16_t teamId = 101;
    for (uint16_t i = 0; i < parameters["numberOfTeams"].getAsUInt16(); ++i)
    {
        createGameParams.mTeamIds.push_back(teamId);
        ++teamId;
    }
    createGameParams.mJoiningTeamIndex = parameters["joiningTeamIndex"].getAsUInt16();
}

#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
static void populatePossImages(eastl::string& possImagesStr, const wchar_t* defaultDir)
{
    EA::IO::DirectoryIterator di;
    EA::IO::DirectoryIterator::EntryList entryList;
    const bool fileWasFound = (di.Read(defaultDir, entryList, L"*", EA::IO::kDirectoryEntryFile, 1000, false) > 0);
    if (fileWasFound)
    {
        for (EA::IO::DirectoryIterator::EntryList::const_iterator itr = entryList.begin(), end = entryList.end(); itr != end; ++itr)
        {
            // validate extension
            eastl::string fileName;
            fileName.sprintf("%ls", itr->msName.c_str());
            eastl::string::size_type pos = fileName.find_last_of(".");
            eastl::string ext = ((pos == eastl::string::npos)? "" : fileName.substr(pos + 1));
            if ((ext.comparei("jpg") != 0) && (ext.comparei("jpeg") != 0) && (ext.comparei("jpe") != 0) &&
                (ext.comparei("jif") != 0) && (ext.comparei("jfif") != 0) && (ext.comparei("jfi") != 0))
            {
                continue;
            }

            if (itr != entryList.begin())
                possImagesStr.append_sprintf(",");
            possImagesStr.append_sprintf("%ls%ls", defaultDir, itr->msName.c_str());
        }
    }
}

static void populatePossDataFiles(eastl::string& possDataFilesStr, const wchar_t* defaultDir)
{
    EA::IO::DirectoryIterator di;
    EA::IO::DirectoryIterator::EntryList entryList;
    const bool fileWasFound = (di.Read(defaultDir, entryList, L"*", EA::IO::kDirectoryEntryFile, 1000, false) > 0);
    if (fileWasFound)
    {
        for (EA::IO::DirectoryIterator::EntryList::const_iterator itr = entryList.begin(), end = entryList.end(); itr != end; ++itr)
        {
            // validate extension
            eastl::string fileName;
            fileName.sprintf("%ls", itr->msName.c_str());
            eastl::string::size_type pos = fileName.find_last_of(".");
            eastl::string ext = ((pos == eastl::string::npos)? "" : fileName.substr(pos + 1));
            if (ext.comparei("dat") != 0)
            {
                continue;
            }

            if (itr != entryList.begin())
                possDataFilesStr.append_sprintf(",");
            possDataFilesStr.append_sprintf("%ls%ls", defaultDir, itr->msName.c_str());
        }
    }
}

void GameManagement::setupExternalSessionUpdateParameters(Pyro::UiNodeParameterStruct &parameters, const char8_t * tabName /*= nullptr*/)
{
    // image
    eastl::string possImagesStr;
    populatePossImages(possImagesStr, L"/data/");
    parameters.addString("externalSessionImageFilePath", "", "External session Image File Path", possImagesStr.c_str(), "Optional client specified image for game's external session (JPEG file). If unspecified the server configured default image is used.", Pyro::ItemVisibility::ADVANCED, tabName);
    // status
    setupExternalSessionStatusParameters(parameters, tabName);
}

void GameManagement::setupExternalSessionStatusParameters(Pyro::UiNodeParameterStruct &parameters, const char8_t * tabName /*= nullptr*/)
{
    // status
    parameters.addString("externalSessionStatusDefault", "Playing Ignition", "External session status default value", "", "", Pyro::ItemVisibility::ADVANCED, tabName);
    Pyro::UiNodeParameterArrayPtr localizationsArray = parameters.addArray("externalSessionStatusLocalizations", "External Session Status Localizations", "Specify language code / localized value.", Pyro::ItemVisibility::ADVANCED, tabName).getAsArray();
    Pyro::UiNodeParameter *attrStruct = new Pyro::UiNodeParameter("externalSessionStatusLocalizations");
    attrStruct->getValue().setAsDataNodePtr(new Pyro::UiNodeParameterStruct());
    Pyro::UiNodeParameterStructPtr structPtr = attrStruct->getAsStruct();
    structPtr->addString("langCode", "", "Language Code", "en,fr,es,de,it,ja,nl,pt,ru,ko,zh-TW,zh-CN,fi,sv,da,no,pl,pt-BR,en-GB,tr,es-MX,ar,fr-CA", "ISO language code.");   
    structPtr->addString("localizedText", "", "Localized Text", "", "Localized Text.");
    localizationsArray->setNewItemTemplate(attrStruct);
}
#endif

void GameManagement::setupExternalSessionCreateParameters(Pyro::UiNodeParameterStruct &parameters, const char8_t * tabName /*= nullptr*/)
{
#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
    parameters.addString("sessionTemplateName", "", "External Session Template Name", "", "", Pyro::ItemVisibility::SIMPLE, tabName);
    parameters.addString("externalSessionCustomData", "", "External Session Custom Data", "", "", Pyro::ItemVisibility::ADVANCED, tabName);
#endif
#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
    // status
    setupExternalSessionStatusParameters(parameters, tabName);
    // custom data
    eastl::string possDataFilesStr;
    populatePossDataFiles(possDataFilesStr, L"/data/");
    parameters.addString("externalSessionCustomDataFilePath", "", "External session Custom Data File Path", possDataFilesStr.c_str(), "Optional client specified custom data for game's external session (.dat file).", Pyro::ItemVisibility::ADVANCED, tabName);
#endif
}

void GameManagement::setupReservedExternalPlayersParameters(Pyro::UiNodeParameterStruct &parameters)
{
    Pyro::UiNodeParameterArrayPtr playersArray = parameters.addArray("reservedExternalPlayers", "Reserved External Players", "External Ids of additional party members to try making reservations for, if there's spare room.").getAsArray();
    Pyro::UiNodeParameter *attrStruct = new Pyro::UiNodeParameter("userIdentificaton");
    attrStruct->getValue().setAsDataNodePtr(new Pyro::UiNodeParameterStruct());
    Pyro::UiNodeParameterStructPtr userInfo = attrStruct->getAsStruct();
    userInfo->addUInt64("blazeId", 0, "Blaze Id", "", "The Player's Blaze Id of the optional player to try reserving a spot for");
    userInfo->addUInt64("externalId", 0, "External Id", "", "External Id of the optional player to try reserving a spot for");
    userInfo->addString("externalString", "", "External String", "", "External String of the optional player to try reserving a spot for (use instead of external id on the Nintendo Switch)");
    playersArray->setNewItemTemplate(attrStruct);   
}

void GameManagement::setupPlayerJoinDataParameters(Pyro::UiNodeParameterStruct &parameters)
{
    // Join Data List: 
    Pyro::UiNodeParameterStructPtr playerJoinData = parameters.addStruct("playerJoinData", "Player Join Data", "Who is joining and how.", Pyro::ItemVisibility::REQUIRED, "").getAsStruct();

    // Per Player Join Data: 
    {
        Pyro::UiNodeParameterArrayPtr playerDataList = playerJoinData->addArray("perPlayerJoinData", "Player Join Data", "Add player join data, player attributes, and role information.", Pyro::ItemVisibility::ADVANCED).getAsArray();
        Pyro::UiNodeParameter *attrStruct = new Pyro::UiNodeParameter("playerJoinInformation");
        attrStruct->getValue().setAsDataNodePtr(new Pyro::UiNodeParameterStruct());
        Pyro::UiNodeParameterStructPtr structPtr = attrStruct->getAsStruct();

        Pyro::UiNodeParameterStructPtr userInfo = structPtr->addStruct("userInfo", "User Information", "The Id of the Joining User").getAsStruct();
        userInfo->addUInt64("blazeId", 0, "Blaze Id", "", "The Player's Blaze Id");
        userInfo->addUInt64("externalId", 0, "External Id", "", "External XBL or PSN Id");
        userInfo->addString("externalString", "", "External String", "", "External NSA id with environment prefix (e.g. dd1:123456)");

        Pyro::UiNodeParameter* roleName = new Pyro::UiNodeParameter("Role");
        roleName->setText("Role");
        roleName->setAsString("");
        roleName->getAvailableValues().setValues(",forward,midfielder,defender,goalie");

        Pyro::UiNodeParameterArrayPtr roleList = structPtr->addArray("roleNames", "Role Names", "The user's desired roles", Pyro::ItemVisibility::ADVANCED).getAsArray();
        roleList->setNewItemTemplate(roleName);

        structPtr->addUInt16("teamId", Blaze::GameManager::INVALID_TEAM_ID, "Team Id", "0,1,2,3,65534,65535", "Team id that will be used in-game. 65535 == Use Default, 65534 == ANY TEAM ID");
        structPtr->addUInt16("teamIndex", Blaze::GameManager::UNSPECIFIED_TEAM_INDEX, "Team Index", "0,1,2,3,65535", "Team index that will be used by the player. NOT USED BY MATCHMAKING. 65535 == ANY TEAM INDEX");
        structPtr->addEnum("slotType", Blaze::GameManager::INVALID_SLOT_TYPE, "Slot Type", "Type of Slot to join into. NOT USED BY MATCHMAKING.");

        // Player Attributes:
        {
            Pyro::UiNodeParameterArrayPtr playerAttributesArray = structPtr->addArray("playerAttributes", "Player Attributes", "Add player attributes.", Pyro::ItemVisibility::ADVANCED).getAsArray();
            Pyro::UiNodeParameter *playerAttrStruct = new Pyro::UiNodeParameter("playerAttribute");
            playerAttrStruct->getValue().setAsDataNodePtr(new Pyro::UiNodeParameterStruct());
            Pyro::UiNodeParameterStructPtr playerAttrStructPtr = playerAttrStruct->getAsStruct();
            playerAttrStructPtr->addString("playerAttrName", "", "Name", "Class", "Player Attribute Name");
            playerAttrStructPtr->addString("playerAttrValue", "", "Value", "Fighter,Mage,Rogue", "Player Attribute Value");
            playerAttributesArray->setNewItemTemplate(playerAttrStruct);
        }

        structPtr->addBool("isOptional", false, "Is Optional", "Is the player not required to successfully join the game? (Typically an external user)");
        playerDataList->setNewItemTemplate(attrStruct);   
    }

    // Group Id:
    Pyro::UiNodeParameterStructPtr userInfo = playerJoinData->addStruct("userGroupId", "User Group Id", "The Id of the Joining User Group.").getAsStruct();
    userInfo->addUInt16("componentId", 0, "Component Id");
    userInfo->addUInt16("entityType", 0, "Entity Type");
    userInfo->addInt64("entityId", 0, "Entity Id");


    playerJoinData->addEnum("slotType", Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT, "Slot Type", "Type of Slot to join into. NOT USED BY MATCHMAKING.");
    playerJoinData->addEnum("gameEntryType", Blaze::GameManager::GAME_ENTRY_TYPE_DIRECT, "Game Entry Type", "Is the group joining, or reserving a slot?");
    playerJoinData->addUInt16("teamId", Blaze::GameManager::ANY_TEAM_ID, "Team Id", "0,1,2,3,65534", "Team id that will be used in-game. 65534 == ANY TEAM ID");
    playerJoinData->addUInt16("teamIndex", Blaze::GameManager::UNSPECIFIED_TEAM_INDEX, "Team Index", "0,1,2,3,65535", "Team index that will be used. NOT USED BY MATCHMAKING. 65535 == ANY TEAM INDEX");
    playerJoinData->addString("roleName", "", "Role");
}

void GameManagement::parsePlayerJoinDataParameters(Pyro::UiNodeParameterStruct &parameters, Blaze::GameManager::PlayerJoinData &playerJoinData)
{
    Pyro::UiNodeParameterStructPtr playerJoinDataParam = parameters["playerJoinData"].getAsStruct();

    // Per Player Join Data:
    Pyro::UiNodeParameterArrayPtr playerDataListParam = playerJoinDataParam["perPlayerJoinData"].getAsArray();
    if (playerDataListParam != nullptr)
    {    
        for (int32_t i = 0; i < (*playerDataListParam).getCount(); ++i)
        {
            Pyro::UiNodeParameterStructPtr perPlayerJoinDataParam = playerDataListParam->at(i)->getAsStruct();
            Blaze::GameManager::PerPlayerJoinData* perPlayerJoinData = playerJoinData.getPlayerDataList().pull_back();

            Pyro::UiNodeParameterStructPtr userInfoParam = perPlayerJoinDataParam["userInfo"].getAsStruct();
            Blaze::UserIdentification& userInfo = perPlayerJoinData->getUser();
            userInfo.setBlazeId(userInfoParam["blazeId"]);
            gBlazeHub->setExternalStringOrIdIntoPlatformInfo(userInfo.getPlatformInfo(), userInfoParam["externalId"], userInfoParam["externalString"]);

            // Player Attributes:
            Pyro::UiNodeParameterArrayPtr playerAttrParam = perPlayerJoinDataParam["playerAttributes"].getAsArray();
            if (playerAttrParam != nullptr)
            {    
                for (int32_t j = 0; j < (*playerAttrParam).getCount(); ++j)
                {
                    Pyro::UiNodeParameterStructPtr playerAttr = playerAttrParam->at(j)->getAsStruct();
                    const char8_t* name = playerAttr["playerAttrName"];
                    const char8_t* value = playerAttr["playerAttrValue"];
                    perPlayerJoinData->getPlayerAttributes()[name] = value;
                }
            }

            // Player desired roles:
            Pyro::UiNodeParameterArrayPtr roleNames = perPlayerJoinDataParam["roleNames"].getAsArray();
            if (roleNames != nullptr)
            {
                for (int32_t j = 0; j < (*roleNames).getCount(); ++j)
                {
                    const char8_t* roleName = roleNames->at(j)->getAsString();
                    perPlayerJoinData->getRoles().push_back(roleName);
                }
            }

            perPlayerJoinData->setTeamIndex(perPlayerJoinDataParam["teamIndex"]);
            perPlayerJoinData->setTeamId(perPlayerJoinDataParam["teamId"]);
            perPlayerJoinData->setSlotType(perPlayerJoinDataParam["slotType"]);
            perPlayerJoinData->setIsOptionalPlayer(perPlayerJoinDataParam["isOptional"]);
        }
    }

    Pyro::UiNodeParameterStructPtr userGroupIdParam = playerJoinDataParam["userGroupId"].getAsStruct();
    Blaze::GameManager::UserGroupId userGroupId(userGroupIdParam["componentId"], userGroupIdParam["entityType"], userGroupIdParam["entityId"]);
    playerJoinData.setGroupId(userGroupId);

    playerJoinData.setDefaultSlotType(playerJoinDataParam["slotType"]);
    playerJoinData.setGameEntryType(playerJoinDataParam["gameEntryType"]);
    playerJoinData.setDefaultTeamId(playerJoinDataParam["teamId"]);
    playerJoinData.setDefaultTeamIndex(playerJoinDataParam["teamIndex"]);
    playerJoinData.setDefaultRole(playerJoinDataParam["roleName"]);
}

void GameManagement::setupGameAttributeParameters(Pyro::UiNodeParameterStruct &parameters, bool addSearchParams, const char* tabName)
{
    Pyro::UiNodeParameterArrayPtr gameAttributesArray = parameters.addArray("GameAttributes", "Additional Game Attributes", "Add additional game attributes.", Pyro::ItemVisibility::ADVANCED, tabName).getAsArray();
    Pyro::UiNodeParameter *attrStruct = new Pyro::UiNodeParameter("gameAttribute");
    attrStruct->getValue().setAsDataNodePtr(new Pyro::UiNodeParameterStruct());
    Pyro::UiNodeParameterStructPtr structPtr = attrStruct->getAsStruct();
    structPtr->addString("gameAttrName", "", "Name", "HardcoreMode", "Game Attribute Name");
    structPtr->addString("gameAttrValue", "", "Value", "0,1,2", "Game Attribute we're searching for");
    if (addSearchParams)
    {
        structPtr->addString("gameAttrRuleName", "", "Rule Name", "gameHardcoreModeMatchRule", "Game Attribute Rule we're searching for");
        structPtr->addString("gameAttrPref", "", "Fit Threshold", "requireExactMatch,quickMatch,gbPreferred", "Game Attribute we're searching for (empty == match any)");
    }
    gameAttributesArray->setNewItemTemplate(attrStruct);
}

void GameManagement::setupPreferredGamesParameters(Pyro::UiNodeParameterStruct &parameters)
{
    const char8_t *preferredGamesRuleTab = "Preferred Games Rule Criteria";
    parameters.addBool("requireGames", false, "Require Games", "", Pyro::ItemVisibility::ADVANCED, preferredGamesRuleTab);
    Pyro::UiNodeParameterArrayPtr gamesArray = parameters.addArray("preferredGames", "Preferred Games", "Game Id's of any games to prefer to matchmake into. If require is not checked, the game only gets a match boost.", Pyro::ItemVisibility::ADVANCED, preferredGamesRuleTab).getAsArray();
    Pyro::UiNodeParameter *p = new Pyro::UiNodeParameter("gameId");
    p->setText("Game Id");
    p->setDescription("Game Id to prefer");
    p->setAsUInt64(0);
    gamesArray->setNewItemTemplate(p);
}

void GameManagement::setupPreferredPlayersParameters(Pyro::UiNodeParameterStruct &parameters)
{
    const char8_t *preferredPlayersRuleTab = "Preferred Players Rule Criteria";
    parameters.addBool("requirePlayers", false, "Require Players", "", Pyro::ItemVisibility::ADVANCED, preferredPlayersRuleTab);
    Pyro::UiNodeParameterArrayPtr playersArray = parameters.addArray("preferredPlayers", "Preferred Players", "Player Id's of players to prefer to matchmake into their games. If require is not checked, the player being in game only gets a match boost.", Pyro::ItemVisibility::ADVANCED, preferredPlayersRuleTab).getAsArray();
    Pyro::UiNodeParameter *p = new Pyro::UiNodeParameter("playerId");
    p->setText("Player Id");
    p->setDescription("Player Id to prefer");
    p->setAsUInt64(0);
    playersArray->setNewItemTemplate(p);
}

void GameManagement::setupGameSettings(Pyro::UiNodeParameterStruct &parameters, const char* tabName)
{
    parameters.addBool("enableJoinInProgress", false, "Join In Progress", "", Pyro::ItemVisibility::SIMPLE, tabName);
    parameters.addBool("enableOpenToBrowsing", true, "Open To Browsing", "", Pyro::ItemVisibility::SIMPLE, tabName);
    parameters.addBool("enableOpenToInvites", true, "Open To Invites", "", Pyro::ItemVisibility::SIMPLE, tabName);
    parameters.addBool("enableAdminOnlyInvites", false, "Enable Admin Only Invites", "", Pyro::ItemVisibility::ADVANCED, tabName);
    parameters.addBool("enableOpenToMatchmaking", true, "Open To Matchmaking", "", Pyro::ItemVisibility::SIMPLE, tabName);
    parameters.addBool("enableOpenToJoinByPlayer", true, "Open To Join By Player", "", Pyro::ItemVisibility::ADVANCED, tabName);

    parameters.addBool("enableFriendsBypassClosedToJoinByPlayer", false, "Friends Bypass Join By Player", "", Pyro::ItemVisibility::ADVANCED, tabName);

    parameters.addBool("enableHostMigratable", true, "Host Migratable", "", Pyro::ItemVisibility::ADVANCED, tabName);
    parameters.addBool("isAllowSameTeamId", false, "Allow Duplicate TeamIds", "", Pyro::ItemVisibility::ADVANCED, tabName);
    parameters.addBool("enableDynamicReputation", false, "Enable Dynamic Reputation Requirement", "", Pyro::ItemVisibility::ADVANCED, tabName);
    parameters.addBool("enableDisconnectReservation", false, "Enable Disconnect Reservation", "", Pyro::ItemVisibility::ADVANCED, tabName);
    parameters.addBool("isRanked", false, "Is Ranked", "", Pyro::ItemVisibility::SIMPLE, tabName);
    parameters.addBool("enableAllowAnyReputation", false, "Allow Any Reputation", "", Pyro::ItemVisibility::ADVANCED, tabName);
    parameters.addBool("enablePersistedId", false, "Enable Persisted Id", "", Pyro::ItemVisibility::ADVANCED, tabName);
    parameters.addBool("isVirtualGame", false, "Is Game Virtual", "", Pyro::ItemVisibility::ADVANCED, tabName);
    parameters.addBool("allowMemberGameAttributeEdit", false, "Allow Members to Edit Game Attribute", "", Pyro::ItemVisibility::SIMPLE, tabName);
    parameters.addBool("autoDemoteReservedPlayers", false, "Auto Demote Reserved", "Automatically demote reserved players to the queue, if a new player joins the queue.", Pyro::ItemVisibility::SIMPLE, tabName);
}

void GameManagement::fillGameBrowserListParameters(Pyro::UiNodeParameterStructPtr parameters, Blaze::GameManager::GameBrowserList::GameBrowserListParameters& gameListParams)
{
    gameListParams.mListConfigName = parameters["listConfigName"].getAsString();

    gameListParams.mListCapacity =  parameters["maxGamesInList"];

    // Default to Snapshot list unless otherwise specified
    gameListParams.mListType = Blaze::GameManager::GameBrowserList::LIST_TYPE_SNAPSHOT;
    if (ds_stricmp(parameters["listType"], "LIST_TYPE_SUBSCRIPTION") == 0)
    {
        gameListParams.mListType = Blaze::GameManager::GameBrowserList::LIST_TYPE_SUBSCRIPTION;
    }

    if (parameters["enableModRule"])
    {
        gameListParams.mListCriteria.getModRuleCriteria().setDesiredModRegister(parameters["desiredMods"]);
        gameListParams.mListCriteria.getModRuleCriteria().setIsEnabled(0x1);
    }
    if (!parameters["virtualGames"].getAsBool())
    {
        gameListParams.mListCriteria.getVirtualGameRulePrefs().setDesiredVirtualGameValue(Blaze::GameManager::VirtualGameRulePrefs::STANDARD);
    }
    else
    {
        gameListParams.mListCriteria.getVirtualGameRulePrefs().setDesiredVirtualGameValue(Blaze::GameManager::VirtualGameRulePrefs::ABSTAIN);
    }
    gameListParams.mListCriteria.getVirtualGameRulePrefs().setMinFitThresholdName("testDecay");

    if (parameters["anyReputationGames"].getAsBool())
    {
        gameListParams.mListCriteria.getReputationRulePrefs().setReputationRequirement(Blaze::GameManager::ACCEPT_ANY_REPUTATION);
    }
    else
    {
        gameListParams.mListCriteria.getReputationRulePrefs().setReputationRequirement(Blaze::GameManager::REJECT_POOR_REPUTATION);
    }

    // Select the appropriate game size rule (can't have all)
    {
        gameListParams.mListCriteria.getPlayerCountRuleCriteria().setMinPlayerCount(parameters["playerCountRuleMin"]);
        gameListParams.mListCriteria.getPlayerCountRuleCriteria().setDesiredPlayerCount(parameters["playerCountRuleDes"]);
        gameListParams.mListCriteria.getPlayerCountRuleCriteria().setMaxPlayerCount(parameters["playerCountRuleMax"]);
        gameListParams.mListCriteria.getPlayerCountRuleCriteria().setRangeOffsetListName("matchAny");

        gameListParams.mListCriteria.getTotalPlayerSlotsRuleCriteria().setMinTotalPlayerSlots(parameters["minTotalPlayerSlots"]);
        gameListParams.mListCriteria.getTotalPlayerSlotsRuleCriteria().setDesiredTotalPlayerSlots(parameters["desiredTotalPlayerSlots"]);
        gameListParams.mListCriteria.getTotalPlayerSlotsRuleCriteria().setMaxTotalPlayerSlots(parameters["maxTotalPlayerSlots"]);
        gameListParams.mListCriteria.getTotalPlayerSlotsRuleCriteria().setRangeOffsetListName("matchAny");

        gameListParams.mListCriteria.getFreePlayerSlotsRuleCriteria().setMinFreePlayerSlots(parameters["minFreePlayerSlots"]);
        gameListParams.mListCriteria.getFreePlayerSlotsRuleCriteria().setMaxFreePlayerSlots(parameters["maxFreePlayerSlots"]);

        gameListParams.mListCriteria.getPlayerSlotUtilizationRuleCriteria().setMinPercentFull(parameters["minPercentFull"]);
        gameListParams.mListCriteria.getPlayerSlotUtilizationRuleCriteria().setDesiredPercentFull(parameters["desiredPercentFull"]);
        gameListParams.mListCriteria.getPlayerSlotUtilizationRuleCriteria().setMaxPercentFull(parameters["maxPercentFull"]);
        gameListParams.mListCriteria.getPlayerSlotUtilizationRuleCriteria().setRangeOffsetListName("matchAny");
    }

    // MM_CUSTOM_CODE - BEGIN exampleUpstreamBPSRule
    //     gameListParams.mListCriteria.getCustomRulePrefs()->getExampleUpstreamBPSRuleCriteria()->setDesiredUpstreamBps(parameters["upstreamBPS"]);
    //     gameListParams.mListCriteria.getCustomRulePrefs()->getExampleUpstreamBPSRuleCriteria()->setMinFitThresholdName("quickMatch");
    // MM_CUSTOM_CODE - END exampleUpstreamBPSRule
    //gameListParams.mListCriteria.getHostBalancingRulePrefs()->setMinFitThresholdName("matchAny");
    //gameListParams.mListCriteria.getHostViabilityRulePrefs()->setMinFitThresholdName("hostViability");

    // GameNameRule
    gameListParams.mListCriteria.getGameNameRuleCriteria().setSearchString(parameters["gameNameSearchString"]);
}

void GameManagement::setupTeamIdsAndCapacityParameters(Pyro::UiNodeParameterStruct &parameters, Blaze::GameManager::TeamIdVector& teamIdVectorParam, uint16_t &teamCapacityParam)
{
    //uint16_t numEnabled = 0;
    //teamCapacityParam = parameters["teamsCapacityCommon"];

    //// set up parameter for create game, reset dedicated server etc
    //const int maxNumTeams = 4; // max teams in TeamIdVector
    //const int BUF_SIZE = 64;
    //char8_t param[BUF_SIZE];
    //const char8_t *expandableListName = "Team";
    //for (int index = 0; index < maxNumTeams; ++index)

    //{
    //    if (ExpandableListParameter::ExpandableList::isListItemAtIndexEnabled(parameters, expandableListName, index))
    //    {
    //        sprintf(param, "team%dId", index);
    //        teamIdVectorParam.push_back(parameters[param]);
    //        numEnabled++;
    //    }
    //}
    //return numEnabled;
}

void GameManagement::refreshMatchmakingConfig()
{
    gBlazeHub->getGameManagerAPI()->getMatchmakingConfig(
        Blaze::GameManager::GameManagerAPI::GetMatchmakingConfigCb(this, &GameManagement::GetMatchmakingConfigCb));
}

void GameManagement::GetMatchmakingConfigCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, const Blaze::GameManager::GetMatchmakingConfigResponse *matchmakingConfigResponse)
{
    if (blazeError == Blaze::ERR_OK)
    {
        matchmakingConfigResponse->copyInto(mMatchmakingConfigResponse);
    }
}

const Blaze::GameManager::GetMatchmakingConfigResponse *GameManagement::getMatchmakingConfig()
{
    return &mMatchmakingConfigResponse;
}

void GameManagement::matchmakingScenarioStarted(Blaze::GameManager::MatchmakingScenarioId matchmakingScenarioId)
{
    getActionCancelScenarioMatchmake().setIsVisible(true);

    eastl::pair<MatchmakingScenarioTimeMap::iterator, bool> mapEntry;
    mapEntry = mMatchmakingScenarioTimeMap.insert(MatchmakingScenarioTimeMapPair(matchmakingScenarioId, time(nullptr)));
    if (mapEntry.second == false)
    {
        (mapEntry.first)->second = time(nullptr);
    }
}


void GameManagement::onGameCreated(Blaze::GameManager::Game *createdGame)
{
    REPORT_LISTENER_CALLBACK(
        "Called whenever a game is created, but before the network has been established.  "
        "You can use this listener method to start listening to the game's listner methods.");

    // creates the GamePlay window, attaches itself as listener to game notifications
    getGamePlayWindow(createdGame);
}

void GameManagement::removeWindow(GamePlayPtr gamePlayWindow)
{
    getWindows().remove(gamePlayWindow);
}

void GameManagement::onGameDestructing(Blaze::GameManager::GameManagerAPI *gameManagerApi, Blaze::GameManager::Game *dyingGame, Blaze::GameManager::GameDestructionReason reason)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    GamePlay &gamePlayWindow = getGamePlayWindow(dyingGame);
    gamePlayWindow.UserClosing -= Pyro::UiNodeWindow::UserClosingEventHandler(this, &GameManagement::GameWindowClosing);
    
    gamePlayWindow.gameDestroyed();

    // Delay the removal of the window to avoid adding and removing the window on the same logic. 
    gBlazeHub->getScheduler()->scheduleMethod("removeWindow", this, &GameManagement::removeWindow, GamePlayPtr(&gamePlayWindow), nullptr, 10);
    if (shouldRecreate(*dyingGame, reason))
    {
        // create a new game session
        Blaze::GameManager::Game::CreateGameParameters createGameParams;
        createGameParams.mMaxPlayerCapacity = dyingGame->getMaxPlayerCapacity();
        createGameParams.mMinPlayerCapacity = dyingGame->getMinPlayerCapacity();
        for (uint16_t i = Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT; i < Blaze::GameManager::MAX_SLOT_TYPE; ++i)
        {
            createGameParams.mPlayerCapacity[i] = dyingGame->getPlayerCapacity((Blaze::GameManager::SlotType)i);
        }

        createGameParams.mQueueCapacity = dyingGame->getQueueCapacity();
        createGameParams.mNetworkTopology = dyingGame->getNetworkTopology();
        createGameParams.mVoipTopology = dyingGame->getVoipTopology();
        createGameParams.mGameType = dyingGame->getGameType();

        vLOG(gameManagerApi->createGame(createGameParams,
            Blaze::GameManager::GameManagerAPI::CreateGameCb(this, &GameManagement::CreateGameCb)));
    }

}

bool GameManagement::shouldRecreate(const Blaze::GameManager::Game &dyingGame, Blaze::GameManager::GameDestructionReason destructionReason) const
{
    // Based off Frostbite/BF title implementation
    if ((destructionReason == Blaze::GameManager::SYS_GAME_RECREATE) || (
        (destructionReason == Blaze::GameManager::SYS_GAME_ENDING) && (dyingGame.getNetworkTopology() == Blaze::CLIENT_SERVER_DEDICATED) &&
        dyingGame.getGameSettings().getVirtualized() && (dyingGame.getGameState() != Blaze::GameManager::INACTIVE_VIRTUAL)))
    {
        return true;
    }
    return false;
}

void GameManagement::onMatchmakingScenarioFinished(Blaze::GameManager::MatchmakingResult matchmakingResult,
    const Blaze::GameManager::MatchmakingScenario* matchmakingScenario,
    Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    getActionCancelScenarioMatchmake().setIsVisible(false);
    mMatchmakingScenarioTimeMap.erase(matchmakingScenario->getScenarioId());
    getWindows().removeById_va("matchmakingScenario:0x%016" PRIX64, matchmakingScenario->getScenarioId());

    updateMatchmakingResult(matchmakingResult, matchmakingScenario->getDebugFindGameResults(), matchmakingScenario->getDebugCreateGameResults(), game);
}

void GameManagement::updateMatchmakingResult(Blaze::GameManager::MatchmakingResult matchmakingResult, 
                                             const Blaze::GameManager::DebugFindGameResults& mmFindGameResults, 
                                             const Blaze::GameManager::DebugCreateGameResults& mmCreateGameResults,
                                             Blaze::GameManager::Game *game)
{

    switch (matchmakingResult)
    {
        case Blaze::GameManager::SUCCESS_CREATED_GAME:
        case Blaze::GameManager::SUCCESS_JOINED_EXISTING_GAME:
        case Blaze::GameManager::SUCCESS_JOINED_NEW_GAME:
        {
            getGamePlayWindow(game).setupGame(game->getLocalPlayer(gBlazeHub->getPrimaryLocalUserIndex()));

            break;
        }
        case Blaze::GameManager::SUCCESS_PSEUDO_FIND_GAME:
        {
            getUiDriver()->showMessage("The pseudo matchmaking session returned results for an existing game.");

            Blaze::GameManager::DebugTopResultList::const_iterator iter = mmFindGameResults.getTopResultList().begin();
            Blaze::GameManager::DebugTopResultList::const_iterator end = mmFindGameResults.getTopResultList().end();
            for (; iter != end; ++iter)
            {
                getMatchmakingWindow((*iter)->getGameId()).showDebugInfo(*iter, mmFindGameResults);
            }
            
            break;
        }
        case Blaze::GameManager::SUCCESS_PSEUDO_CREATE_GAME:
        {
            getUiDriver()->showMessage("The pseudo matchmaking session was successful, and matched to create a new game.");

            Blaze::GameManager::DebugSessionResultList::const_iterator iter = mmCreateGameResults.getSessionResultList().begin();
            Blaze::GameManager::DebugSessionResultList::const_iterator end = mmCreateGameResults.getSessionResultList().end();
            for (; iter != end; ++iter)
            {
                getMatchmakingWindow((*iter)->getMatchedSessionId()).showDebugInfo(*iter, mmCreateGameResults, mmCreateGameResults.getMaxFitScore());
            }

            break;
        }
        case Blaze::GameManager::SESSION_TIMED_OUT:
        {
            getUiDriver()->showMessage("The matchmaking session timed out.  An appropriate match was not found.");

            break;
        }
        case Blaze::GameManager::SESSION_CANCELED:
        {
            getUiDriver()->showMessage("The matchmaking session was canceled by the user.");

            break;
        }
        case Blaze::GameManager::SESSION_TERMINATED:
        {
            getUiDriver()->showMessage("The matchmaking session was terminated.");

            break;
        }
        case Blaze::GameManager::SESSION_QOS_VALIDATION_FAILED:
        {
            getUiDriver()->showMessage("The matchmaking session failed QoS validation.");

            break;
        }
        case Blaze::GameManager::SESSION_ERROR_GAME_SETUP_FAILED:
        {
            getUiDriver()->showMessage("Game setup failed.");

            break;
        }
    }
}

void GameManagement::onMatchmakingScenarioAsyncStatus(const Blaze::GameManager::MatchmakingScenario* matchmakingScenario,
    const Blaze::GameManager::MatchmakingAsyncStatusList* matchmakingAsyncStatusList)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    Pyro::UiNodeWindow &progressWindow = getWindow_va("matchmakingScenario:0x%016" PRIX64, matchmakingScenario->getScenarioId());
    progressWindow.setDockingState(Pyro::DockingState::DOCK_TOP);

    Pyro::UiNodeValuePanel &valuePanel = progressWindow.getValuePanel("Matchmaking Progress");
    
    for (auto& status : *matchmakingAsyncStatusList)
    {
        Pyro::UiNodeValue *value = &valuePanel.getValue("Host Viability");
        value->setDescription("The likelyhood of finding a match");
        value->setAsEnum(status->getHostViabilityRuleStatus().getMatchedHostViabilityValue());

        // display ETTM
        value = &valuePanel.getValue("Estimated Time To Match (seconds)");
        value->setDescription("The amount of time this matchmaking scenario is expected to run");
        value->setAsInt64(matchmakingScenario->getEstimatedTimeToMatch().getSec());

        // display instance
        value = &valuePanel.getValue("Assigned to instance");
        value->setDescription("The instance id");
        value->setAsUInt16(matchmakingScenario->getAssignedInstanceId());

        value = &valuePanel.getValue("Fit Score (%)");
        value->setDescription("The aggregate quality score corresponding to the currently assigned provisional game");
        eastl::string theFloat; // necessary for now because currently setAsFloat() has a bug that prevents correct output
        theFloat.sprintf("%f", matchmakingScenario->getFitScorePercent());
        value->setAsString(theFloat.c_str());

        //////////////////////////////////////////////////////////////////////////
        // Example Upstream BPS Rule
        //////////////////////////////////////////////////////////////////////////

        // MM_CUSTOM_CODE - BEGIN exampleUpstreamBPSRule
        //         UI->
        //             UI->addValue(valueListNode, "ExampleRule:bpsmin", "Current bps min allowed",
        //             static_cast<int>((*i)->getCustomAsynStatus()->getExampleUpstreamBPSRuleStatus()->getUpstreamBpsMin()));
        // 
        //         UI->addValue(valueListNode, "ExampleRule:bpsmax", "Current bps max allowed",
        //             static_cast<int>((*i)->getCustomAsynStatus()->getExampleUpstreamBPSRuleStatus()->getUpstreamBpsMax()));
        // MM_CUSTOM_CODE - END exampleUpstreamBPSRule

        MatchmakingScenarioTimeMap::iterator mapEntry;
        if ((mapEntry = mMatchmakingScenarioTimeMap.find(matchmakingScenario->getScenarioId())) != mMatchmakingScenarioTimeMap.end())
        {
            value = &valuePanel.getValue("Time Elapsed (seconds)");
            value->setDescription("The amount of time this matchmaking scenario has been running");
            value->setAsInt64(time(nullptr) - (*mapEntry).second);
        }
    }
}

void GameManagement::onGameBrowserListUpdated(Blaze::GameManager::GameBrowserList *gameBrowserList,
    const Blaze::GameManager::GameBrowserList::GameBrowserGameVector *removedGames,
    const Blaze::GameManager::GameBrowserList::GameBrowserGameVector *updatedGames)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    Pyro::UiNodeWindow &window = getWindow("Game Browser");
    window.setDockingState(Pyro::DockingState::DOCK_TOP);

    Pyro::UiNodeTable &table = window.getTable("Game Search Results");

    if (table.getActions().findById("JoinBrowsedGame") == nullptr)
        table.getActions().insert(&getActionJoinBrowsedGame());
    if (table.getActions().findById("JoinTeamGame") == nullptr)
        table.getActions().insert(&getActionJoinTeamGame());
    if (table.getActions().findById("SpectateBrowsedGame") == nullptr)
        table.getActions().insert(&getActionSpectateBrowsedGame());

    table.getColumn("gameId").setText("Id");
    table.getColumn("name").setText("Name");
    table.getColumn("state").setText("State");
    table.getColumn("numPlayers").setText("Number of Players");
    table.getColumn("numSpectators").setText("Number of Spectators");
    table.getColumn("gameMods").setText("Game Mods");
    table.getColumn("gameType").setText("Game Type");


    Blaze::GameManager::GameBrowserList::GameBrowserGameVector::const_iterator it;
    Blaze::GameManager::GameBrowserList::GameBrowserGameVector::const_iterator end;

    table.getRows().clear();

    it = gameBrowserList->getGameVector().begin();
    end = gameBrowserList->getGameVector().end();
    for (; it != end; ++it)
    {
        Blaze::GameManager::GameBrowserGame *gameBrowserGame = *it;

        Pyro::UiNodeTableRowFieldContainer &fields = table.getRow_va("0x%016" PRIX64, gameBrowserGame->getId()).getFields();

        fields["gameId"].setAsString_va("0x%016" PRIX64, gameBrowserGame->getId());
        fields["name"] = gameBrowserGame->getName();
        fields["state"] = gameBrowserGame->getGameState();
        fields["numPlayers"] = gameBrowserGame->getPlayerCount();
        fields["numSpectators"] = gameBrowserGame->getSpectatorCount();
        fields["gameMods"] = gameBrowserGame->getGameModRegister();
        fields["gameType"] = gameBrowserGame->getGameType();

        
        //for (uint16_t i = 0; i < gameBrowserGame->getTeamCount(); ++i)
        //{
        //    eastl::string teamData;
        //    eastl::string teamIdentity = "TeamIndex ";
        //    teamIdentity += int2str(i);
        //    teamIdentity += " [Id=";
        //    teamIdentity += int2str(gameBrowserGame->getTeamIdByIndex(i));
        //    teamIdentity += "]";

        //    teamData += int2str(gameBrowserGame->getTeamSizeByIndex(i));
        //    teamData += "/";
        //    teamData += int2str(gameBrowserGame->getTeamCapacityByIndex(i));
        //    teamData += " players/max";
        //    gameTable.getColumn(teamIdentity.c_str()).setText(teamIdentity.c_str());
        //    UI->addTableRowField(gameTableRow, teamData.c_str());
        //}
    }

    if (removedGames != nullptr)
    {
        it = removedGames->begin();
        end = removedGames->end();
        for (; it != end; ++it)
        {
            Blaze::GameManager::GameBrowserGame *gameBrowserGame = *it;

            table.getRows().removeById_va("0x%016" PRIX64, gameBrowserGame->getId());
        }
    }
}

void GameManagement::onGameBrowserListUpdateTimeout(Blaze::GameManager::GameBrowserList *gameBrowserList)
{
    REPORT_LISTENER_CALLBACK(nullptr);
    getUiDriver()->showMessage(Pyro::ErrorLevel::ERR, "Timeout fetching game snapshot list");
}

void GameManagement::onGameBrowserListDestroy(Blaze::GameManager::GameBrowserList *gameBrowserList)
{
    REPORT_LISTENER_CALLBACK(nullptr);
}

void GameManagement::onUserGroupJoinGame(Blaze::GameManager::Game *game, Blaze::GameManager::Player* player, Blaze::GameManager::UserGroupId groupId)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    getGamePlayWindow(game).setupGame(player);
}

void GameManagement::onTopologyHostInjected(Blaze::GameManager::Game *game)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    getGamePlayWindow(game).setupGame(game->getLocalPlayer(gBlazeHub->getPrimaryLocalUserIndex()));
}

void GameManagement::onRemoteMatchmakingStarted(const Blaze::GameManager::NotifyRemoteMatchmakingStarted *matchmakingStartedNotification, uint32_t userIndex)
{
    BLAZE_SDK_DEBUGF("User [%016" PRIX64 "] pulled user index [%" PRIX32 "] into matchmaking session [%016" PRIX64 "], scenario [%016" PRIX64 "].",
        matchmakingStartedNotification->getRemoteUserInfo().getInitiatorId(), userIndex, matchmakingStartedNotification->getMMSessionId(), matchmakingStartedNotification->getMMScenarioId());

    //getUiDriver()->showMessage_va("User [%016" PRIX64 "] pulled user index [%" PRIX32 "] into matchmaking session [%016" PRIX64 "], scenario [%016" PRIX64 "].",
    //   matchmakingStartedNotification->getRemoteUserInfo().getInitiatorId(), userIndex, matchmakingStartedNotification->getMMSessionId(), matchmakingStartedNotification->getMMScenarioId());
}

void GameManagement::onRemoteMatchmakingEnded(const Blaze::GameManager::NotifyRemoteMatchmakingEnded *matchmakingEndedNotification, uint32_t userIndex)
{
    BLAZE_SDK_DEBUGF("User [%016" PRIX64 "] pulled user index [%" PRIX32 "] into matchmaking session [%016" PRIX64 "], scenario [%016" PRIX64 "], matchmaking result '%s'.",
        matchmakingEndedNotification->getRemoteUserInfo().getInitiatorId(), userIndex, matchmakingEndedNotification->getMMSessionId(), matchmakingEndedNotification->getMMScenarioId(),
        Blaze::GameManager::MatchmakingResultToString(matchmakingEndedNotification->getMatchmakingResult()));

    //getUiDriver()->showMessage_va("User [%016" PRIX64 "] pulled user index [%" PRIX32 "] into matchmaking session [%016" PRIX64 "], scenario [%016" PRIX64 "], matchmaking result '%s'.",
    //    matchmakingEndedNotification->getRemoteUserInfo().getInitiatorId(), userIndex, matchmakingEndedNotification->getMMSessionId(), matchmakingEndedNotification->getMMScenarioId(),
    //    Blaze::GameManager::MatchmakingResultToString(matchmakingEndedNotification->getMatchmakingResult()));
}

void GameManagement::onRemoteJoinFailed(const Blaze::GameManager::NotifyRemoteJoinFailed *remoteJoinFailedNotification, uint32_t userIndex)
{
    BLAZE_SDK_DEBUGF("User [%016" PRIX64 "] tried to pull user index [%" PRIX32 "] into matchmaking game [%016" PRIX64 "], but join failed with error '%s'.",
        remoteJoinFailedNotification->getRemoteUserInfo().getInitiatorId(), userIndex, remoteJoinFailedNotification->getGameId(),
        (!gBlazeHub ? "" : gBlazeHub->getErrorName(remoteJoinFailedNotification->getJoinError())));

    //getUiDriver()->showMessage_va("User [%016" PRIX64 "] tried to pull user index [%" PRIX32 "] into matchmaking game [%016" PRIX64 "], but join failed with error '%s'.",
    //    remoteJoinFailedNotification->getRemoteUserInfo().getInitiatorId(), userIndex, remoteJoinFailedNotification->getGameId(), 
    //    (!gBlazeHub ? "" : gBlazeHub->getErrorName(remoteJoinFailedNotification->getJoinError())));
}

void GameManagement::onReservedPlayerIdentifications(Blaze::GameManager::Game *game, uint32_t userIndex, const Blaze::UserIdentificationList *reservedPlayerIdentifications)
{
    REPORT_LISTENER_CALLBACK(nullptr);


}


void GameManagement::initActionListJoinedGames(Pyro::UiNodeAction &action)
{
    action.setText("List Joined Games");
    action.setDescription("List the games that you are currently joined into");
}

void GameManagement::actionListJoinedGames(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Pyro::UiNodeTable &table = getWindow("joinedgames").getTable("Currently Joined Games");

    table.getActions().insert(&getActionLeaveGame());
    table.getActions().insert(&getActionLeaveGameByLUGroup());

    table.getColumn("gameId").setText("Id");
    table.getColumn("gameType").setText("Type");
    table.getColumn("name").setText("Name");
    table.getColumn("state").setText("State");
    table.getColumn("numPlayers").setText("Number of Players");
#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
    table.getColumn("extSessTemplate").setText("External Session Template");
#endif

    table.getRows().clear();

    for (uint32_t a = 0;
        a < gBlazeHub->getGameManagerAPI()->getGameCount();
        a++)
    {
        Blaze::GameManager::Game *game;

        game = gBlazeHub->getGameManagerAPI()->getGameByIndex(a);

        Pyro::UiNodeTableRowFieldContainer &fields = table.getRow_va("0x%016" PRIX64, game->getId()).getFields();

        fields["gameId"].setAsString_va("0x%016" PRIX64, game->getId());
        fields["gameType"] = game->getGameType();
        fields["name"] = game->getName();
        fields["state"] = game->getGameState();
        fields["numPlayers"] = game->getPlayerCount();
#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
        fields["extSessTemplate"] = game->getExternalSessionTemplateName();
#endif
    }
}

void GameManagement::initActionOverrideGeoip(Pyro::UiNodeAction &action)
{
    action.setText("Override GeoIP");
    action.setDescription("Overrides the users geoip location.");

    action.getParameters().addFloat("latitude", 37.526890f, "GeoIP Latitude", "37.526890", "Default is Redwood Shores", Pyro::ItemVisibility::SIMPLE);
    action.getParameters().addFloat("longitude", -122.252383f, "GeoIP Longitude", "-122.252383", "Default is Redwood Shores", Pyro::ItemVisibility::SIMPLE);
}

void GameManagement::actionOverrideGeoip(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::GeoLocationData geoData;
    geoData.setBlazeId(gBlazeHub->getUserManager()->getPrimaryLocalUser()->getId());
    geoData.setLatitude(parameters["latitude"]);
    geoData.setLongitude(parameters["longitude"]);

    gBlazeHub->getUserManager()->overrideUsersGeoIPData(&geoData, Blaze::UserManager::UserManager::OverrideGeoCb(this, &GameManagement::OverrideGeoIpCb));
}

void GameManagement::OverrideGeoIpCb(Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
}

void GameManagement::initActionDisplayMyDisconnectReservations(Pyro::UiNodeAction &action)
{
    action.setText("Display My Disconnect Reservations");
    action.setDescription("Display the disconnect reservations made for the games that you were members of");
}

void GameManagement::actionDisplayMyDisconnectReservations(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(gBlazeHub->getGameManagerAPI()->lookupReservationsByUser(gBlazeHub->getUserManager()->getPrimaryLocalUser()->getUser(), Blaze::GameManager::GameBrowserListConfigName("ReservList"), 
        Blaze::GameManager::GameManagerAPI::LookupGamesByUserCb(this, &GameManagement::LookupDisconnectReservationsCb)));
}


void GameManagement::LookupDisconnectReservationsCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, const Blaze::GameManager::GameBrowserDataList* gameBrowserDataList)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    Pyro::UiNodeTable &table = getWindow("disconnectReserv").getTable("My Disconnect Reservations");

    table.getColumn("gameId").setText("Id");
    table.getColumn("gameType").setText("Type");
    table.getColumn("name").setText("Name");
    table.getColumn("state").setText("State");
    table.getColumn("numPlayers").setText("Number of Players");
    table.getColumn("reservctime").setText("Reservation Creation Time");
    table.getColumn("jointime").setText("Joined Game Time");
    table.getRows().clear();

    if (gameBrowserDataList != nullptr)
    {
        table.getActions().insert(&getActionClaimReservation());

        Blaze::GameManager::GameBrowserDataList::GameBrowserGameDataList::const_iterator itr = gameBrowserDataList->getGameData().begin();
        Blaze::GameManager::GameBrowserDataList::GameBrowserGameDataList::const_iterator end = gameBrowserDataList->getGameData().end();

        for (; itr != end; ++itr)
        {
            const Blaze::GameManager::GameBrowserGameData* gameData = *itr;

            Pyro::UiNodeTableRowFieldContainer &fields = table.getRow_va("0x%016" PRIX64, gameData->getGameId()).getFields();

            fields["gameId"].setAsString_va("0x%016" PRIX64, gameData->getGameId());
            fields["gameType"] = gameData->getGameType();
            fields["name"] = gameData->getGameName();
            fields["state"] = gameData->getGameState();
            fields["numPlayers"] = (uint32_t) gameData->getGameRoster().size();

            Blaze::GameManager::GameBrowserGameData::GameBrowserPlayerDataList::const_iterator pdItr, pdEnd;
            for (pdItr = gameData->getGameRoster().begin(), pdEnd = gameData->getGameRoster().end(); pdItr != pdEnd; ++pdItr)
            {
                const Blaze::GameManager::GameBrowserPlayerData* playerData = *pdItr;
                if ((playerData->getPlayerId() == gBlazeHub->getUserManager()->getPrimaryLocalUser()->getUser()->getId()) && playerData->getReservationCreationTimestamp() != 0)
                {
                    Blaze::TimeValue reservationCreationTimestamp(playerData->getReservationCreationTimestamp());
                    char8_t timeValueStr[64];
                    fields["reservctime"] = reservationCreationTimestamp.toString(timeValueStr, sizeof(timeValueStr));
                    Blaze::TimeValue joinedGameTimestamp(playerData->getJoinedGameTimestamp());
                    fields["jointime"] = joinedGameTimestamp.toString(timeValueStr, sizeof(timeValueStr));
                    break;
                }
            }
        }
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void GameManagement::initActionClaimReservation(Pyro::UiNodeAction &action)
{
    action.setText("Claim Reservation");
    action.setDescription("Claim the reservation made in this disconnected game");
    action.getParameters().addBool("joinByLUGroup", false, "Join by Local User Group", "", Pyro::ItemVisibility::SIMPLE);
}


void GameManagement::actionClaimReservation(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    runJoinGameById(
        parameters["gameId"],
        (parameters["joinByLUGroup"]) ? &gBlazeHub->getUserManager()->getLocalUserGroup() : nullptr,
        Blaze::GameManager::UNSPECIFIED_TEAM_INDEX,
        Blaze::GameManager::GAME_ENTRY_TYPE_CLAIM_RESERVATION, nullptr);
}

void GameManagement::initActionBrowseGames(Pyro::UiNodeAction &action)
{
    action.setText("Browse Games");
    action.setDescription("Browse available games");

    setupBasicGameBrowserParameters(action.getParameters());

    const char8_t *roleTab = "Role Search";
    // Role Rule Mode
    action.getParameters().addBool("useRoles", false, "Search For Roles", "Search for specific roles if true.", Pyro::ItemVisibility::ADVANCED, roleTab);
    action.getParameters().addEnum("roleScenario", DEFAULT_ROLES, "Role Scenario", "", Pyro::ItemVisibility::ADVANCED, roleTab);
   
    const char8_t *devTab = "Advanced";
    // Game Attribute Rule Mode
    action.getParameters().addString("gameAttributeMode", "tdeathmatch", "Mode", "deathmatch,tdeathmatch", "", Pyro::ItemVisibility::ADVANCED, devTab);
    action.getParameters().addString("gameAttributeMode_MatchPreference", "", "Mode - Match preference", "requireExactMatch,quickMatch,gbPreferred", "", Pyro::ItemVisibility::ADVANCED, devTab);

    // Game Attribute Rule Map
    action.getParameters().addString("gameAttributeMap", "map_1", "Map", "map_1,map_2,map_3", "", Pyro::ItemVisibility::ADVANCED, devTab);
    action.getParameters().addString("gameAttributeMap_MatchPreference", "", "Map - Match preference", "requireExactMatch,quickMatch,gbPreferred", "", Pyro::ItemVisibility::ADVANCED, devTab);

    setupGameAttributeParameters(action.getParameters(), true);

}

void GameManagement::actionBrowseGames(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::GameManager::GameBrowserList::GameBrowserListParameters gameListParams;

    fillGameBrowserListParameters(parameters, gameListParams);

    if(parameters["useRoles"])
    {
        Ignition::CreateGameTeamAndRoleSetup roleSearch = parameters["roleScenario"];
        switch(roleSearch)
        {
        case DEFAULT_ROLES:
            {
                gameListParams.mRoleMap[""] = 0;
                break;
            }
        case SIMPLE_ROLES:
            {
                gameListParams.mRoleMap["forward"] = 0;
                gameListParams.mRoleMap["defender"] = 0;
                break;
            }
        case FIFA_ROLES:
            {
                gameListParams.mRoleMap["forward"] = 0;
                gameListParams.mRoleMap["defender"] = 0;
                gameListParams.mRoleMap["goalie"] = 0;
                break;
            }
        }

    }

    addGameAttributeSearchCriteria(
        gameListParams.mListCriteria.getGameAttributeRuleCriteriaMap(),
        "gameModeMatchRule",
        parameters["gameAttributeMode_MatchPreference"],
        parameters["gameAttributeMode"], true);

    addGameAttributeSearchCriteria(
        gameListParams.mListCriteria.getGameAttributeRuleCriteriaMap(),
        "gameMapMatchRule",
        parameters["gameAttributeMap_MatchPreference"],
        parameters["gameAttributeMap"], true);

    Pyro::UiNodeParameterArrayPtr gameAttrArray = parameters["GameAttributes"].getAsArray();
    if (gameAttrArray != nullptr)
    {
        for (int32_t i = 0; i < (*gameAttrArray).getCount(); ++i)
        {
            Pyro::UiNodeParameterStructPtr attr = gameAttrArray->at(i)->getAsStruct();

            addGameAttributeSearchCriteria(
                gameListParams.mListCriteria.getGameAttributeRuleCriteriaMap(),
                attr["gameAttrRuleName"],
                attr["gameAttrPref"],
                attr["gameAttrValue"], true);
        }
    }

    vLOG(gBlazeHub->getGameManagerAPI()->createGameBrowserList(gameListParams, 
        Blaze::GameManager::GameManagerAPI::CreateGameBrowserListCb(this, &GameManagement::CreateGameBrowserListCb)));
}

void GameManagement::CreateGameBrowserListScenarioCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::GameManager::GameBrowserList* gameBrowserList, const char8_t* errDetails)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    if (blazeError == Blaze::ERR_OK)
        onGameBrowserListUpdated(gameBrowserList, nullptr, nullptr);

    REPORT_BLAZE_ERROR(blazeError);
}

void GameManagement::CreateGameBrowserListCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::GameManager::GameBrowserList* gameBrowserList, const char8_t* errDetails)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    if (blazeError == Blaze::ERR_OK)
        onGameBrowserListUpdated(gameBrowserList, nullptr, nullptr);

    REPORT_BLAZE_ERROR(blazeError);
}

void GameManagement::addGameAttributeSearchCriteria(Blaze::GameManager::GameAttributeRuleCriteriaMap& gameAttrCriteriaMap, const char8_t* ruleName, const char8_t* matchPref, const char8_t* desiredValue, bool gamebrowser /* = false */)
{
    if (ruleName != nullptr && ruleName[0] != '\0' &&
        matchPref != nullptr && matchPref[0] != '\0' && ds_stricmp(matchPref, "None") != 0)
    {

        Blaze::GameManager::GameAttributeRuleCriteriaPtr rulePrefsPtr = gameAttrCriteriaMap.allocate_element();
        gameAttrCriteriaMap.insert(eastl::make_pair(ruleName, rulePrefsPtr));

        rulePrefsPtr->getDesiredValues().push_back(desiredValue);

        if (ds_stricmp(matchPref, "Required") == 0)
        {
            rulePrefsPtr->setMinFitThresholdName("requireExactMatch");
        }
        else if (!gamebrowser && ds_stricmp(matchPref, "Preferred") == 0)
        {
            rulePrefsPtr->setMinFitThresholdName("quickMatch");
        }
        else if (gamebrowser && ds_stricmp(matchPref, "Preferred") == 0)
        {
            rulePrefsPtr->setMinFitThresholdName("gbPreference");
        }
        else
        {
            rulePrefsPtr->setMinFitThresholdName(matchPref);
        }
    }
}

void GameManagement::initActionCreateNewGamePJD(Pyro::UiNodeAction &action)
{
    action.setText("Create New Game (PJD)");
    action.setDescription("Create a new game where you are the leader using player join data");

    setupCreateGameParametersPJD(action.getParameters());
}

void GameManagement::actionCreateNewGamePJD(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    runCreateNewGamePJD(*parameters, (parameters["createByLUGroup"]) ? &gBlazeHub->getUserManager()->getLocalUserGroup() : nullptr);

    //SAMPLECORE->recallBlazeStateEventHandler(this);
}

void GameManagement::initActionCreateNewGame(Pyro::UiNodeAction &action)
{
    action.setText("Create New Game");
    action.setDescription("Create a new game where you are the leader");

    setupCreateGameParameters(action.getParameters());
}

void GameManagement::actionCreateNewGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    runCreateNewGame(*parameters, (parameters["createByLUGroup"]) ? &gBlazeHub->getUserManager()->getLocalUserGroup() : nullptr);

    //SAMPLECORE->recallBlazeStateEventHandler(this);
}

void GameManagement::initActionCreateDedicatedServerGame(Pyro::UiNodeAction &action)
{
    action.setText("Find/Reset Dedicated Server Game");
    action.setDescription("Finds an available dedicated server, and resets it so that it can be initialized");

    setupCreateGameParameters(action.getParameters(), true);
}

void GameManagement::actionCreateDedicatedServerGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    runCreateDedicatedServerGame(*parameters, (parameters["joinByLUGroup"]) ? &gBlazeHub->getUserManager()->getLocalUserGroup() : nullptr);
}

void GameManagement::initActionJoinGameById(Pyro::UiNodeAction &action)
{
    action.setText("Join Game");
    action.setDescription("Joins a specific game by id");

    setupJoinGameByIdParameters(action.getParameters());
}

void GameManagement::actionJoinGameById(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::GameManager::RoleName roleName;
    roleName.set(parameters["playerRoles"]);
    Blaze::GameManager::RoleNameToPlayerMap joiningRoles;
    Blaze::GameManager::PlayerIdList *playerIdList = joiningRoles.allocate_element();
    playerIdList->push_back(Blaze::INVALID_BLAZE_ID);
    joiningRoles[roleName] = playerIdList;
    Blaze::GameManager::GameEntryType gameEntryType;
    ParseGameEntryType(parameters["gameEntry"], gameEntryType);

    Blaze::UserIdentificationList reservedExternalIds;
    parseReservedExternalPlayersParameters(*parameters, reservedExternalIds);
    Blaze::GameManager::RoleNameToUserIdentificationMap userIdentificationJoiningRoles;
    if (!reservedExternalIds.empty())
    {
        Blaze::UserIdentificationList *rolesExternalIds = userIdentificationJoiningRoles.allocate_element();
        gBlazeHub->setExternalStringOrIdIntoPlatformInfo(rolesExternalIds->pull_back()->getPlatformInfo(), Blaze::INVALID_EXTERNAL_ID, nullptr);
        userIdentificationJoiningRoles[roleName] = rolesExternalIds;
    }

    mUseNetGameDist = parameters["enableNetGameDist"].getAsBool();

    runJoinGameById(
        parameters["gameId"],
            (parameters["joinByLUGroup"]) ? &gBlazeHub->getUserManager()->getLocalUserGroup() : nullptr,
        parameters["teamIndex"],
        gameEntryType, &joiningRoles, parameters["slotType"], &reservedExternalIds, &userIdentificationJoiningRoles);
}

void GameManagement::initActionJoinGameByUserNamePJD(Pyro::UiNodeAction &action)
{
    action.setText("Join Game By User Name (PJD)");
    action.setDescription("Joins a specific game by name of user in game with Player Join Data");

    action.getParameters().addString("userName", "", "User name", "", "The name of a user in a game you would like to join.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addEnum("gameType", Blaze::GameManager::GAME_TYPE_GAMESESSION, "Game Type", "", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addBool("joinByLUGroup", false, "Join by Local User Group", "", Pyro::ItemVisibility::SIMPLE);

    setupPlayerJoinDataParameters(action.getParameters());
}

void GameManagement::actionJoinGameByUserNamePJD(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::GameManager::PlayerJoinDataHelper playerJoinData;

    parsePlayerJoinDataParameters(*parameters, playerJoinData.getPlayerJoinData());
    if (parameters["joinByLUGroup"]) 
    {
        playerJoinData.setGroupId(gBlazeHub->getUserManager()->getLocalUserGroup().getBlazeObjectId());
    }

    mUseNetGameDist = parameters["enableNetGameDist"].getAsBool();

    vLOG(gBlazeHub->getGameManagerAPI()->joinGameByUsername(
        parameters["userName"],
        Blaze::GameManager::JOIN_BY_PLAYER, 
        playerJoinData,
        Blaze::GameManager::GameManagerAPI::JoinGameCb(this, &GameManagement::JoinGameCb),
        parameters["gameType"]));
}

void GameManagement::initActionJoinGameByUserName(Pyro::UiNodeAction &action)
{
    action.setText("Join Game By User Name");
    action.setDescription("Joins a specific game by name of user in game");

    setupJoinGameByUserNameParameters(action.getParameters());
}

void GameManagement::actionJoinGameByUserName(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::GameManager::RoleName roleName;
    roleName.set(parameters["playerRoles"]);
    Blaze::GameManager::RoleNameToPlayerMap joiningRoles;
    Blaze::GameManager::PlayerIdList *playerIdList = joiningRoles.allocate_element();
    playerIdList->push_back(Blaze::INVALID_BLAZE_ID);
    joiningRoles[roleName] = playerIdList;
    Blaze::GameManager::GameEntryType gameEntryType;
    ParseGameEntryType(parameters["gameEntry"], gameEntryType);

    Blaze::UserIdentificationList reservedExternalIds;
    parseReservedExternalPlayersParameters(*parameters, reservedExternalIds);

    Blaze::GameManager::RoleNameToUserIdentificationMap userIdentificationJoiningRoles;
    if (!reservedExternalIds.empty())
    {
        Blaze::UserIdentificationList *rolesExternalIds = userIdentificationJoiningRoles.allocate_element();
        gBlazeHub->setExternalStringOrIdIntoPlatformInfo(rolesExternalIds->pull_back()->getPlatformInfo(), Blaze::INVALID_EXTERNAL_ID, nullptr);
        userIdentificationJoiningRoles[roleName] = rolesExternalIds;
    }

    mUseNetGameDist = parameters["enableNetGameDist"].getAsBool();

    runJoinGameByUserName(
        parameters["userName"],
        (parameters["joinByLUGroup"]) ? &gBlazeHub->getUserManager()->getLocalUserGroup() : nullptr,
        parameters["teamIndex"],
        gameEntryType, &joiningRoles, parameters["slotType"], &reservedExternalIds, &userIdentificationJoiningRoles, parameters["gameType"]);
}

void GameManagement::initActionJoinBrowsedGame(Pyro::UiNodeAction &action)
{
    action.setText("Join Game");
    action.setDescription("Joins a specific game by id");
    action.getParameters().addBool("joinByLUGroup", false, "Join by Local User Group", "", Pyro::ItemVisibility::SIMPLE);
}

void GameManagement::actionJoinBrowsedGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    runJoinGameById(
        parameters["gameId"],
        (parameters["joinByLUGroup"]) ? &gBlazeHub->getUserManager()->getLocalUserGroup() : nullptr,
        Blaze::GameManager::UNSPECIFIED_TEAM_INDEX,
        Blaze::GameManager::GAME_ENTRY_TYPE_DIRECT, nullptr);
}

void GameManagement::initActionSpectateBrowsedGame(Pyro::UiNodeAction &action)
{
    action.setText("Spectate Game");
    action.setDescription("Joins a specific game by id as a spectator.");
}

void GameManagement::actionSpectateBrowsedGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    runJoinGameById(
        parameters["gameId"], nullptr, Blaze::GameManager::UNSPECIFIED_TEAM_INDEX,
        Blaze::GameManager::GAME_ENTRY_TYPE_DIRECT, nullptr, Blaze::GameManager::SLOT_PUBLIC_SPECTATOR);
}

void GameManagement::initActionJoinTeamGame(Pyro::UiNodeAction &action)
{
    action.setText("Join Game With Teams");
    action.setDescription("Join the selected game with teams");
}

void GameManagement::actionJoinTeamGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{

    vLOG(gBlazeHub->getGameManagerAPI()->joinGameById(
        parameters["gameId"],
        Blaze::GameManager::JOIN_BY_BROWSING,
        Blaze::GameManager::GameManagerAPI::JoinGameCb(this, &GameManagement::JoinGameCb), parameters["slotType"],
        nullptr, nullptr, parameters["teamIndex"], Blaze::GameManager::GAME_ENTRY_TYPE_DIRECT, nullptr, nullptr));
}


void GameManagement::initActionJoinGameByUserList(Pyro::UiNodeAction &action)
{
    action.setText("Join Game By User List");
    action.setDescription("Put party members in the game, making their reservations.");
    setupJoinGameByUserListParameters(action.getParameters());
}
void GameManagement::setupJoinGameByUserListParameters(Pyro::UiNodeParameterStruct &parameters)
{
    parameters.addUInt64("gameId", 0, "Game Id", "", "The GameId of the game you would like party members to join by reservation claim.", Pyro::ItemVisibility::SIMPLE);
    parameters.addEnum("joinMethod", Blaze::GameManager::JOIN_BY_INVITES, "Join Method", "", Pyro::ItemVisibility::ADVANCED);

    // add the user list
    setupReservedExternalPlayersParameters(parameters);

    parameters.addString("playerRoles", "", "Joining Role", "forward,midfielder,defender,goalie", "", Pyro::ItemVisibility::ADVANCED);
}

void GameManagement::actionJoinGameByUserList(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::GameManager::RoleName roleName;
    roleName.set(parameters["playerRoles"]);

    Blaze::UserIdentificationList reservedExternalIds;
    parseReservedExternalPlayersParameters(*parameters, reservedExternalIds);

    Blaze::GameManager::RoleNameToUserIdentificationMap userIdentificationJoiningRoles;
    if (!reservedExternalIds.empty())
    {
        Blaze::UserIdentificationList *rolesExternalIds = userIdentificationJoiningRoles.allocate_element();
        gBlazeHub->setExternalStringOrIdIntoPlatformInfo(rolesExternalIds->pull_back()->getPlatformInfo(), Blaze::INVALID_EXTERNAL_ID, nullptr);
        userIdentificationJoiningRoles[roleName] = rolesExternalIds;
    }
    Blaze::GameManager::JoinMethod joinMethod;
    ParseJoinMethod(parameters["joinMethod"], joinMethod);

    mUseNetGameDist = parameters["enableNetGameDist"].getAsBool();

    runJoinGameByUserList(
        parameters["gameId"],
        Blaze::GameManager::UNSPECIFIED_TEAM_INDEX,
        reservedExternalIds,
        &userIdentificationJoiningRoles,
        joinMethod);
}

void GameManagement::initActionCreateOrJoinGame(Pyro::UiNodeAction &action)
{
    action.setText("Create New Game or Join Game");
    action.setDescription("Create a new game or join if the persisted game id already exists");

    setupCreateGameParameters(action.getParameters());
}

void GameManagement::actionCreateOrJoinGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    runCreateOrJoinGame(*parameters, (parameters["createLUGroup"]) ? &gBlazeHub->getUserManager()->getLocalUserGroup() : nullptr);
}

void GameManagement::initActionLeaveGame(Pyro::UiNodeAction &action)
{
    action.setText("Leave Game");
    action.setDescription("Leave the selected game");
}

void GameManagement::actionLeaveGame(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::GameManager::Game *game;

    game = gBlazeHub->getGameManagerAPI()->getGameById(parameters["gameId"]);

    if (game != nullptr)
    {
        Blaze::GameManager::Game::LeaveGameCb cb(this, &GameManagement::LeaveGameCb);
        vLOG(game->leaveGame(cb));
    }
    else
    {
        getUiDriver()->showMessage_va("The Game ID [%016" PRIX64 "] no longer exists.", parameters["gameId"].getAsUInt64());
    }
}

void GameManagement::LeaveGameCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);
}

void GameManagement::initActionLeaveGameByLUGroup(Pyro::UiNodeAction &action)
{
    action.setText("Leave Game with Local User Group");
    action.setDescription("Leave the selected game with a subset of local users");
}

void GameManagement::actionLeaveGameByLUGroup(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::GameManager::Game *game;

    game = gBlazeHub->getGameManagerAPI()->getGameById(parameters["gameId"]);

    if (game != nullptr)
    {
        Blaze::GameManager::Game::LeaveGameCb cb(this, &GameManagement::LeaveGameByLUGroupCb);
        vLOG(game->leaveGame(cb, &gBlazeHub->getUserManager()->getLocalUserGroup()));
    }
    else
    {
        getUiDriver()->showMessage_va("The Game ID [%016" PRIX64 "] no longer exists.", parameters["gameId"].getAsUInt64());
    }
}

void GameManagement::LeaveGameByLUGroupCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    const Blaze::ConnectionUserIndexList& list = gBlazeHub->getUserManager()->getLocalUserGroup().getConnUserIndexList();

    Blaze::ConnectionUserIndexList::const_iterator itr, end;
    for (itr = list.begin(), end = list.end(); itr != end; ++itr)
    {
        getGamePlayWindow(game).getLocalUserActionGroup(*itr).LeaveGameCb(blazeError, game);
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void GameManagement::JoinGameCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::GameManager::Game *game, const char8_t *errorMsg)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    if (blazeError == Blaze::ERR_OK && game != nullptr)
    {
        getGamePlayWindow(game).setupGame(game->getLocalPlayer(gBlazeHub->getPrimaryLocalUserIndex()));
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void GameManagement::JoinGameByUserListCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::GameManager::Game *game, const char8_t *errorMsg)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);
}

// Create the param struct from the members:
Pyro::UiNodeParameter* buildUiFromType(const EA::TDF::TypeDescription* typeDesc, const char8_t* name, EA::TDF::TdfGenericReference defaultValue = EA::TDF::TdfGenericReference(), const EA::TDF::TypeDescriptionBitfieldMember* bfMember = nullptr)
{
    if (typeDesc == nullptr || name == nullptr)
    {
        return nullptr;
    }

    bool hasDefault = (defaultValue.getType() == typeDesc->type);

    Pyro::UiNodeParameter* newParam = new Pyro::UiNodeParameter(name);
    newParam->setVisibility(Pyro::ItemVisibility::REQUIRED);
    newParam->setText(name);

    // Build the type based on the type used: (Simple Types)
    switch (typeDesc->type)
    {
    case EA::TDF::TDF_ACTUAL_TYPE_STRING:           newParam->setAsString(hasDefault ? defaultValue.asString() : "");       break;
    case EA::TDF::TDF_ACTUAL_TYPE_BOOL:             newParam->setAsBool(hasDefault   ? defaultValue.asBool()   : false);    break;
    case EA::TDF::TDF_ACTUAL_TYPE_INT8:             newParam->setAsInt8(hasDefault   ? defaultValue.asInt8()   : 0);        break;
    case EA::TDF::TDF_ACTUAL_TYPE_UINT8:            newParam->setAsUInt8(hasDefault  ? defaultValue.asUInt8()  : 0);        break;
    case EA::TDF::TDF_ACTUAL_TYPE_INT16:            newParam->setAsInt16(hasDefault  ? defaultValue.asInt16()  : 0);        break;
    case EA::TDF::TDF_ACTUAL_TYPE_UINT16:           newParam->setAsUInt16(hasDefault ? defaultValue.asUInt16() : 0);        break;
    case EA::TDF::TDF_ACTUAL_TYPE_INT32:            newParam->setAsInt32(hasDefault  ? defaultValue.asInt32()  : 0);        break;
    case EA::TDF::TDF_ACTUAL_TYPE_UINT32:           newParam->setAsUInt32(hasDefault ? defaultValue.asUInt32() : 0);        break;
    case EA::TDF::TDF_ACTUAL_TYPE_INT64:            newParam->setAsInt64(hasDefault  ? defaultValue.asInt64()  : 0);        break;
    case EA::TDF::TDF_ACTUAL_TYPE_UINT64:           newParam->setAsUInt64(hasDefault ? defaultValue.asUInt64() : 0);        break;
    case EA::TDF::TDF_ACTUAL_TYPE_FLOAT:            newParam->setAsFloat(hasDefault  ? defaultValue.asFloat()  : 0);        break;
    case EA::TDF::TDF_ACTUAL_TYPE_BLOB:             
    {
        if (hasDefault)
        {
            newParam->setAsBlob(defaultValue.asBlob().getData(), defaultValue.asBlob().getCount());
        }
        else
        {
            newParam->setAsBlob(nullptr, 0);   
        }
        break;
    }
    case EA::TDF::TDF_ACTUAL_TYPE_ENUM:             
    {
        Pyro::EnumNameValueCollection enumValues;
        
        EA::TDF::TypeDescriptionEnum::NamesByValue::const_iterator iter = typeDesc->asEnumMap()->getNamesByValue().begin();
        EA::TDF::TypeDescriptionEnum::NamesByValue::const_iterator end = typeDesc->asEnumMap()->getNamesByValue().end();
        for (; iter != end; ++iter)
        {
            const EA::TDF::TdfEnumInfo& enumInfo = static_cast<const EA::TDF::TdfEnumInfo &>(*iter);
            enumValues.add(enumInfo.mName, enumInfo.mValue);
        }
        newParam->setAsEnum(hasDefault ? defaultValue.asEnum() : enumValues.getAt(0).Value, enumValues);        
        break;
    }
    case EA::TDF::TDF_ACTUAL_TYPE_BITFIELD:
    {
        if (bfMember == nullptr)
        {
            Pyro::EnumNameValueCollection bitValues;
        
            EA::TDF::TypeDescriptionBitfield::TdfBitfieldMembersMap::const_iterator iter = typeDesc->asBitfieldDescription()->mBitfieldMembersMap.begin();
            EA::TDF::TypeDescriptionBitfield::TdfBitfieldMembersMap::const_iterator end = typeDesc->asBitfieldDescription()->mBitfieldMembersMap.end();
            for (; iter != end; ++iter)
            {
                const EA::TDF::TypeDescriptionBitfieldMember& bitfieldInfo = static_cast<const EA::TDF::TypeDescriptionBitfieldMember &>(*iter);
                bitValues.add(bitfieldInfo.mName, (int32_t)bitfieldInfo.mBitMask);
            }
            newParam->setAsBitSet(hasDefault ? defaultValue.asBitfield().getBits() : 0, bitValues);        
        }
        else
        {
            if (bfMember->mIsBool)
                newParam->setAsBool(hasDefault   ? defaultValue.asBool()   : false);
            else
                newParam->setAsUInt32(hasDefault ? defaultValue.asUInt32() : 0);
        }
        break;
    }

    case EA::TDF::TDF_ACTUAL_TYPE_TIMEVALUE:
        newParam->setAsInt64(hasDefault ? defaultValue.asTimeValue().getMicroSeconds() : 0);  
        newParam->setDescription("TimeValues are set as MICROSECONDS. 1000us = 1ms, 1000000 = 1s.");
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_TYPE:
        newParam->setAsString(hasDefault ? defaultValue.asObjectType().toString().c_str() : "");  
        newParam->setDescription("ObjectType format: 'componentId/entityType'  (ex. 101/332 ) ");
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_ID:
        newParam->setAsString(hasDefault ? defaultValue.asObjectId().toString().c_str() : "");  
        newParam->setDescription("ObjectType format: 'componentId/entityType/entityId'  (ex. 101/332/150 ) ");
        break;


    case EA::TDF::TDF_ACTUAL_TYPE_LIST:
    {
        // Create the array:
        newParam->setAsDataNodePtr(new Pyro::UiNodeParameterArray());
        Pyro::UiNodeParameterArrayPtr newArray = newParam->getAsArray();

        // Create the list's type:
        const EA::TDF::TypeDescription* valueTypeDesc = &(typeDesc->asListDescription()->valueType);
        Pyro::UiNodeParameter* newTypeParam = buildUiFromType(valueTypeDesc, valueTypeDesc->fullName);
        
        // Associate it with the underlying data type: 
        newArray->setNewItemTemplate(newTypeParam);

/*
        // No adding list elements by default, since it has bad interactions with the loading of previous list data.
        if (hasDefault)
        {
            for (uint32_t i = 0; i < defaultValue.asList().vectorSize(); ++i)
            {            
                EA::TDF::TdfGenericReference listRef;
                defaultValue.asList().getReferenceByIndex(i, listRef);

                // Not sure if this is the right thing to do...
                Pyro::UiNodeParameter* newParam = buildUiFromType(valueTypeDesc, valueTypeDesc->fullName, listRef);
                newArray->add(newParam);
            }
        }
*/
        break;
    }
    case EA::TDF::TDF_ACTUAL_TYPE_MAP:
    {
        // Create the array:
        newParam->setAsDataNodePtr(new Pyro::UiNodeParameterMap());
        Pyro::UiNodeParameterMapPtr newMap = newParam->getAsMap();

        // Create the map's key & value types:
        const EA::TDF::TypeDescription* keyTypeDesc = &(typeDesc->asMapDescription()->keyType);
        Pyro::UiNodeParameter* keyTypeParam = buildUiFromType(keyTypeDesc, keyTypeDesc->fullName);
        const EA::TDF::TypeDescription* valueTypeDesc = &(typeDesc->asMapDescription()->valueType);
        Pyro::UiNodeParameter* valueTypeParam = buildUiFromType(valueTypeDesc, valueTypeDesc->fullName);
        
        // Create the key value pair:
        Pyro::UiNodeParameterMapKeyValuePair* newKeyValueType = new Pyro::UiNodeParameterMapKeyValuePair();

        // Associate it with the underlying data type: 
        newKeyValueType->setMapKey(keyTypeParam);
        newKeyValueType->setMapValue(valueTypeParam);
        newMap->setKeyValueType(newKeyValueType);

        // Maps don't load previous data, so adding the defaults should be fine.
        if (hasDefault)
        {
            for (uint32_t i = 0; i < defaultValue.asMap().mapSize(); ++i)
            {            
                EA::TDF::TdfGenericReference keyRef;
                EA::TDF::TdfGenericReference valueRef;
                defaultValue.asMap().getReferenceByIndex(i, keyRef, valueRef);

                // Not sure if this is the right thing to do...
                Pyro::UiNodeParameter* newKeyParam = buildUiFromType(keyTypeDesc, keyTypeDesc->fullName, keyRef);
                Pyro::UiNodeParameter* newValueParam = buildUiFromType(valueTypeDesc, valueTypeDesc->fullName, valueRef);

                Pyro::UiNodeParameterMapKeyValuePair* newPair = new Pyro::UiNodeParameterMapKeyValuePair();
                newPair->setMapKey(newKeyParam);
                newPair->setMapKey(newValueParam);
                newMap->add(newPair);
            }
        }
        break;
    }

    case EA::TDF::TDF_ACTUAL_TYPE_TDF:
    {
        newParam->getValue().setAsDataNodePtr(new Pyro::UiNodeParameterStruct());
        Pyro::UiNodeParameterStructPtr structPtr = newParam->getAsStruct();

        const EA::TDF::TypeDescriptionClass* tdfTypeDesc = typeDesc->asClassInfo();
        for (uint16_t i = 0; i < tdfTypeDesc->memberCount; ++i)
        {
            EA::TDF::TdfGenericReference valueRef;
            if (hasDefault)
            {
                defaultValue.asTdf().getValueByName(tdfTypeDesc->memberInfo[i].getMemberName(), valueRef);
            }

            Pyro::UiNodeParameter* newTdfParam = buildUiFromType(tdfTypeDesc->memberInfo[i].getTypeDescription(), tdfTypeDesc->memberInfo[i].getMemberName(), valueRef);
            structPtr->add(newTdfParam);
        }
        break;
    }
    case EA::TDF::TDF_ACTUAL_TYPE_UNION:            newParam->setAsString("Unsupported type: TDF_ACTUAL_TYPE_UNION (Not sure how to indicate param choice)");  break;


    case EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE:     newParam->setAsString("Unsupported type: TDF_ACTUAL_TYPE_GENERIC_TYPE");  break;
    case EA::TDF::TDF_ACTUAL_TYPE_VARIABLE:         newParam->setAsString("Unsupported type: TDF_ACTUAL_TYPE_VARIABLE");  break;
    case EA::TDF::TDF_ACTUAL_TYPE_UNKNOWN:          newParam->setAsString("Unsupported type: TDF_ACTUAL_TYPE_UNKNOWN");  break;
    default:  newParam->setAsString("Using an unsupported type");      break; 
    }

    return newParam;
}

void buildGenericDataFromParams(Pyro::UiNodeParameter* param, EA::TDF::TdfGenericReference outValue)
{
    if (param == nullptr)
    {
        return;
    }

    // Build the type based on the type used: (Simple Types)
    switch (outValue.getType())
    {
    case EA::TDF::TDF_ACTUAL_TYPE_STRING:           outValue.asString() = param->getAsString();       break;
    case EA::TDF::TDF_ACTUAL_TYPE_BOOL:             outValue.asBool() = param->getAsBool();           break;
    case EA::TDF::TDF_ACTUAL_TYPE_INT8:             outValue.asInt8() = param->getAsInt8();           break;
    case EA::TDF::TDF_ACTUAL_TYPE_UINT8:            outValue.asUInt8() = param->getAsUInt8();         break;
    case EA::TDF::TDF_ACTUAL_TYPE_INT16:            outValue.asInt16() = param->getAsInt16();         break;
    case EA::TDF::TDF_ACTUAL_TYPE_UINT16:           outValue.asUInt16() = param->getAsUInt16();       break;
    case EA::TDF::TDF_ACTUAL_TYPE_INT32:            outValue.asInt32() = param->getAsInt32();         break;
    case EA::TDF::TDF_ACTUAL_TYPE_UINT32:           outValue.asUInt32() = param->getAsUInt32();       break;
    case EA::TDF::TDF_ACTUAL_TYPE_INT64:            outValue.asInt64() = param->getAsInt64();         break;
    case EA::TDF::TDF_ACTUAL_TYPE_UINT64:           outValue.asUInt64() = param->getAsUInt64();       break;
    case EA::TDF::TDF_ACTUAL_TYPE_FLOAT:            outValue.asFloat() = param->getAsFloat();         break;
    case EA::TDF::TDF_ACTUAL_TYPE_BLOB:             outValue.asBlob().setData(param->getAsBlob()->begin(), static_cast<uint32_t>(param->getAsBlob()->size()));     break;
    case EA::TDF::TDF_ACTUAL_TYPE_ENUM:             outValue.asEnum() = param->getAsInt32();          break;
    case EA::TDF::TDF_ACTUAL_TYPE_BITFIELD:         outValue.asBitfield().setBits((uint32_t)param->getAsInt32());                           break;
    case EA::TDF::TDF_ACTUAL_TYPE_TIMEVALUE:        outValue.asTimeValue() = EA::TDF::TimeValue(param->getAsInt64());                       break;
    case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_TYPE:      outValue.asObjectType() = EA::TDF::ObjectType::parseString(param->getAsString());       break;
    case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_ID:        outValue.asObjectId() = EA::TDF::ObjectId::parseString(param->getAsString());           break;

    case EA::TDF::TDF_ACTUAL_TYPE_LIST:
    {
        // iterate over all the elements of the array, and add them in:
        Pyro::UiNodeParameterArrayPtr arrayPtr = param->getAsArray();   
        if (arrayPtr != nullptr)
        {
            for (int32_t i = 0; i < (*arrayPtr).getCount(); ++i)
            {
                Pyro::UiNodeParameter* uiArrayValue = arrayPtr->at(i);
                EA::TDF::TdfGenericReference outListValue;
                outValue.asList().pullBackRef(outListValue);
                
                buildGenericDataFromParams(uiArrayValue, outListValue);
            }
        }        
        break;
    }

    case EA::TDF::TDF_ACTUAL_TYPE_MAP:
    {
        // iterate over all the elements of the array, and add them in:
        Pyro::UiNodeParameterMapPtr mapPtr = param->getAsMap();   
        if (mapPtr != nullptr)
        {
            for (int32_t i = 0; i < (*mapPtr).getCount(); ++i)
            {
                Pyro::UiNodeParameterMapKeyValuePair* uiMapKeyValuePair = mapPtr->at(i);

                // These three lines should be a function on the Map (createNewKey?)
                EA::TDF::TdfGenericValue key;
                key.setType(outValue.asMap().getKeyTypeDesc());
                EA::TDF::TdfGenericReference keyRef(key);
                buildGenericDataFromParams(uiMapKeyValuePair->getMapKey(), keyRef); // Build the key:

                // Add the key (get the value reference)
                EA::TDF::TdfGenericReference outMapValue;
                outValue.asMap().insertKeyGetValue(keyRef, outMapValue);

                // Build the value
                buildGenericDataFromParams(uiMapKeyValuePair->getMapValue(), outMapValue);
            }
        }        
        break;
    }

    case EA::TDF::TDF_ACTUAL_TYPE_TDF:
    {
        Pyro::UiNodeParameterStructPtr structPtr = param->getAsStruct();  
        if (structPtr != nullptr)
        {
            for (int32_t i = 0; i < (*structPtr).getCount(); ++i)
            {
                Pyro::UiNodeParameter* uiStructValue = structPtr->at(i);

                EA::TDF::TdfGenericReference outMemberValue;
                outValue.asTdf().getValueByName(uiStructValue->getText(), outMemberValue);

                buildGenericDataFromParams(uiStructValue, outMemberValue);
            }
        }
        break;
    }

    default: break;
    }
}

void GameManagement::GetGameBrowserScenarioAttributesConfigCb(const Blaze::GameManager::GetGameBrowserScenariosAttributesResponse* scenarioInfo, Blaze::BlazeError err, Blaze::JobId jobId)
{
    if (err != Blaze::ERR_OK)
    {
        return;
    }

    scenarioInfo->copyInto(getGameBrowserScenarioAttributesResponse());
    addGameBrowserScenarioActions(getActions(), Pyro::UiNodeAction::ExecuteEventHandler(this, &GameManagement::actionGameBrowserScenarios));
}

void GameManagement::actionGameBrowserScenarios(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::GameManager::GameBrowserList::GameBrowserListParameters gameListParams;
    const char8_t* scenarioName = parameters["_scenarioName"];
    Blaze::GameManager::ScenarioAttributes scenarioAttributes;

    if (parameters["useRoles"])
    {
        Ignition::CreateGameTeamAndRoleSetup roleSearch = parameters["roleScenario"];
        switch (roleSearch)
        {
        case DEFAULT_ROLES:
        {
            gameListParams.mRoleMap[""] = 0;
            break;
        }
        case SIMPLE_ROLES:
        {
            gameListParams.mRoleMap["forward"] = 0;
            gameListParams.mRoleMap["defender"] = 0;
            break;
        }
        case FIFA_ROLES:
        {
            gameListParams.mRoleMap["forward"] = 0;
            gameListParams.mRoleMap["defender"] = 0;
            gameListParams.mRoleMap["goalie"] = 0;
            break;
        }
        }

    }

    Blaze::GameManager::GetGameBrowserScenariosAttributesResponse::GameBrowserScenarioAttributesMap::const_iterator it = getGameBrowserScenarioAttributesResponse().getGameBrowserScenarioAttributes().find(scenarioName);
    if (it != getGameBrowserScenarioAttributesResponse().getGameBrowserScenarioAttributes().end())
    {
        Blaze::GameManager::ScenarioAttributeMapping::iterator curAttr = it->second->begin();
        Blaze::GameManager::ScenarioAttributeMapping::iterator endAttr = it->second->end();
        for (; curAttr != endAttr; ++curAttr)
        {
            const char8_t* attrName = curAttr->second->getAttrName();

            // Add all the configurable attributes: (nothing else)
            if (attrName[0] != '\0')
            {
                // Grab the attribute data from the params: 
                if (scenarioAttributes.find(attrName) == scenarioAttributes.end())
                {
                    scenarioAttributes[attrName] = scenarioAttributes.allocate_element();
                }

                EA::TDF::TdfGenericValue genericVal;
                if (EA::TDF::TdfFactory::get().createGenericValue(curAttr->first.c_str(), genericVal))
                {
                    EA::TDF::TdfGenericReference genericRef(genericVal);
                    buildGenericDataFromParams(&parameters[attrName], genericRef);
                    scenarioAttributes[attrName]->set(genericVal);
                }
            }
        }
    }

    Blaze::GameManager::PlayerJoinDataHelper playerJoinData;
    playerJoinData.setTeamId(gameListParams.mTeamId);

    parsePlayerJoinDataParameters(*parameters, playerJoinData.getPlayerJoinData());

    Blaze::GameManager::RoleMap::const_iterator curIter = gameListParams.mRoleMap.begin();
    Blaze::GameManager::RoleMap::const_iterator endIter = gameListParams.mRoleMap.end();
    for (; curIter != endIter; ++curIter)
    {
        for (uint16_t i = 0; i < curIter->second; ++i)
        {
            playerJoinData.getPlayerDataList().pull_back()->setRole(curIter->first);
        }
    }

    vLOG(gBlazeHub->getGameManagerAPI()->createGameBrowserListByScenario(scenarioName, scenarioAttributes, playerJoinData,
        Blaze::GameManager::GameManagerAPI::CreateGameBrowserListCb(this, &GameManagement::CreateGameBrowserListCb)));
}

void GameManagement::addGameBrowserScenarioActions(Pyro::UiNodeActionContainer& outActions, Pyro::UiNodeAction::ExecuteEventHandler executeAction)
{
    // Get the scenarios, add a new action for each one:
    Blaze::GameManager::GetScenariosAttributesResponse::ScenarioAttributesMap::const_iterator it = getGameBrowserScenarioAttributesResponse().getGameBrowserScenarioAttributes().begin();
    Blaze::GameManager::GetScenariosAttributesResponse::ScenarioAttributesMap::const_iterator end = getGameBrowserScenarioAttributesResponse().getGameBrowserScenarioAttributes().end();
    for (; it != end; ++it)
    {
        eastl::string actionName;
        actionName.sprintf("Game Browser Scenario: %s", it->first.c_str());

        // Replace any existing action with this name: 
        outActions.removeById(actionName.c_str());
        Pyro::UiNodeAction* action = new Pyro::UiNodeAction(actionName.c_str());

        Blaze::GameManager::ScenarioAttributeMapping::iterator curAttr = it->second->begin();
        Blaze::GameManager::ScenarioAttributeMapping::iterator endAttr = it->second->end();
        for (; curAttr != endAttr; ++curAttr)
        {
            const char8_t* attrName = curAttr->second->getAttrName();

            // Add all the configurable attributes: (nothing else)
            if (attrName[0] != '\0')
            {
                // Each attribute needs to add a new value, based on the type description:
                const EA::TDF::TypeDescriptionBitfieldMember* bfMember = nullptr;
                const EA::TDF::TypeDescription* typeDesc = EA::TDF::TdfFactory::get().getTypeDesc(curAttr->first.c_str(), &bfMember);
                if (typeDesc)
                {
                    EA::TDF::TdfGenericReference defaultValue(curAttr->second->getDefault().get());
                    Pyro::UiNodeParameter* newParam = buildUiFromType(typeDesc, attrName, defaultValue, bfMember);
                    if (action->getParameters().findById(newParam->getText()) == nullptr)
                    {
                        action->getParameters().add(newParam);
                    }
                    else
                    {
                        // This can occur if we have a GameBrowserSetting attribute and a RuleAttribute at the same time. (The should be the same type, though)
                        delete newParam;
                    }
                }
            }
        }

        const char8_t *roleTab = "Role Search";
        // Role Rule Mode
        action->getParameters().addBool("useRoles", false, "Search For Roles", "Search for specific roles if true.", Pyro::ItemVisibility::ADVANCED, roleTab);
        action->getParameters().addEnum("roleScenario", DEFAULT_ROLES, "Role Scenario", "", Pyro::ItemVisibility::ADVANCED, roleTab);

        // This hidden param is used as a way to pass along a value to the function. 
        action->getParameters().addString("_scenarioName", it->first.c_str(), "", "", "", Pyro::ItemVisibility::HIDDEN);
        setupPlayerJoinDataParameters(action->getParameters());

        action->Execute += executeAction;

        outActions.add(action);
    }
}


void GameManagement::GetScenariosAndAttributesCb(const Blaze::GameManager::GetScenariosAndAttributesResponse* scenarioInfo, Blaze::BlazeError err, Blaze::JobId jobId)
{
    if (err != Blaze::ERR_OK)
    {
        return;
    }

    scenarioInfo->copyInto(getScenariosAndAttributesResponse());

    addScenarioActions(getActions(), Pyro::UiNodeAction::ExecuteEventHandler(this, &GameManagement::actionScenarioMatchmake));

    getActions().remove(&getActionCancelScenarioMatchmake());
    getActions().add(&getActionCancelScenarioMatchmake());
}

void GameManagement::addScenarioActions(Pyro::UiNodeActionContainer& outActions, Pyro::UiNodeAction::ExecuteEventHandler executeAction)
{
    // Get the scenarios, add a new action for each one:
    for (auto& scenarioAttributesMapItr : getScenariosAndAttributesResponse().getScenarioAttributeDescriptions())
    {
        eastl::string actionName;
        actionName.sprintf("Run Scenario: %s", scenarioAttributesMapItr.first.c_str());

        // Replace any existing action with this name: 
        outActions.removeById(actionName.c_str());
        Pyro::UiNodeAction* action = new Pyro::UiNodeAction(actionName.c_str());

        Blaze::GameManager::TemplateAttributeDescriptionMapping::iterator curAttr = scenarioAttributesMapItr.second->begin();
        Blaze::GameManager::TemplateAttributeDescriptionMapping::iterator endAttr = scenarioAttributesMapItr.second->end();
        for (; curAttr != endAttr; ++curAttr)
        {
            const char8_t* attrName = curAttr->first;

            // Add all the configurable attributes: (nothing else)
            if (attrName[0] != '\0')
            {
                // Each attribute needs to add a new value, based on the type description:
                //const EA::TDF::TypeDescriptionBitfieldMember* bfMember = nullptr;
                const EA::TDF::TypeDescription* typeDesc = EA::TDF::TdfFactory::get().getTypeDesc(curAttr->second->getAttrTdfId());
                if (typeDesc)
                {
                    EA::TDF::TdfGenericReference defaultValue(curAttr->second->getDefault().get());
                    Pyro::UiNodeParameter* newParam = buildUiFromType(typeDesc, attrName, defaultValue);//, bfMember);
                    if (action->getParameters().findById(newParam->getText()) == nullptr)
                    {
                        action->getParameters().add(newParam);
                    }
                    else
                    {
                        // This can occur if we have a CreateGame attribute and a RuleAttribute at the same time. (The should be the same type, though)
                        delete newParam;
                    }
                }
            }
        }

        // This hidden param is used as a way to pass along a value to the function. 
        action->getParameters().addString("_scenarioName", scenarioAttributesMapItr.first.c_str(), "", "", "", Pyro::ItemVisibility::HIDDEN);
        action->getParameters().addBool("indirectMatchmaking", false, "Indirect Matchmaking", "If set, the local user will not be added to matchmaking");

        setupPlayerJoinDataParameters(action->getParameters());

        action->Execute += executeAction;

        outActions.add(action);
    }
}


void GameManagement::actionScenarioMatchmake(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    runScenarioMatchmake(parameters, nullptr);
}
void GameManagement::runScenarioMatchmake(Pyro::UiNodeParameterStructPtr parameters, const Blaze::UserGroup *userGroup)
{
    // Scenario Name:
    const char8_t* scenarioName = parameters["_scenarioName"];
    bool indirectMatchmaking = parameters["indirectMatchmaking"];

    // Player Attributes: 
    Blaze::GameManager::PlayerJoinDataHelper playerJoinData;
    parsePlayerJoinDataParameters(*parameters, playerJoinData.getPlayerJoinData());
    
    // Scenario Attributes: 
    Blaze::GameManager::ScenarioAttributes scenarioAttributes;
    Blaze::GameManager::GetScenariosAndAttributesResponse::ScenarioAttributeDescriptionsMap::const_iterator it = getScenariosAndAttributesResponse().getScenarioAttributeDescriptions().find(scenarioName);
    if (it != getScenariosAndAttributesResponse().getScenarioAttributeDescriptions().end())
    {
        Blaze::GameManager::TemplateAttributeDescriptionMapping::iterator currAttr = it->second->begin();
        Blaze::GameManager::TemplateAttributeDescriptionMapping::iterator endAttr = it->second->end();
        for (; currAttr != endAttr; ++currAttr)
        {
            const char8_t* attrName = currAttr->first;
            if (attrName[0] != '\0')
            {
                if (scenarioAttributes.find(attrName) == scenarioAttributes.end())
                {
                    scenarioAttributes[attrName] = scenarioAttributes.allocate_element();
                }

                EA::TDF::TdfGenericValue genericVal;
                if (EA::TDF::TdfFactory::get().createGenericValue(currAttr->second->getAttrTdfId(), genericVal))
                {
                    EA::TDF::TdfGenericReference genericRef(genericVal);
                    buildGenericDataFromParams(&parameters[attrName], genericRef);
                    scenarioAttributes[attrName]->set(genericVal);
                }
            }
        }
    }

    // Override group id, if one is provided:
    if (userGroup)
    {
        playerJoinData.setGroupId(userGroup->getBlazeObjectId());
    }

    vLOG(gBlazeHub->getGameManagerAPI()->startMatchmakingScenario(scenarioName, scenarioAttributes, playerJoinData, 
        Blaze::GameManager::GameManagerAPI::StartMatchmakingScenarioCb(this, &GameManagement::StartMatchmakingScenarioCb), !indirectMatchmaking));
}

void GameManagement::StartMatchmakingScenarioCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::GameManager::MatchmakingScenario* matchmakingScenario, const char8_t* err)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    if (blazeError == Blaze::ERR_OK)
    {
        matchmakingScenarioStarted(matchmakingScenario->getScenarioId());
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void GameManagement::initActionCancelScenarioMatchmake(Pyro::UiNodeAction &action)
{
    action.setText("Cancel Scenario");
    action.setDescription("Cancels current matchmaking scenario");
}

void GameManagement::actionCancelScenarioMatchmake(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    uint32_t mmCount = gBlazeHub->getGameManagerAPI()->getMatchmakingScenarioCount();
    for (uint32_t i = 0; i < mmCount; ++i)
    {
        Blaze::GameManager::MatchmakingScenario* scenario = gBlazeHub->getGameManagerAPI()->getMatchmakingScenarioByIndex(i);
        if (scenario != nullptr)
        {
            scenario->cancelScenario();
        }
    }
}

void GameManagement::parseGameAttributes(const char* attrValue, char* result)
{
    if (ds_stricmp(attrValue, "map_2") == 0 || ds_stricmp(attrValue, "tdeathmatch") == 0)
    {
        strcpy(result,"2");
    }
    else if (ds_stricmp(attrValue, "map_3") == 0)
    {
        strcpy(result,"3");
    }
    else if (ds_stricmp(attrValue, "map_1") == 0 || ds_stricmp(attrValue, "deathmatch") == 0)
    {
        strcpy(result,"1");
    }
    else
    {
        strcpy(result, attrValue);
    }
}

void GameManagement::parseGameSettings(Pyro::UiNodeParameterStruct &parameters, Blaze::GameManager::GameSettings &gameSettings)
{
    gameSettings.clearOpenToBrowsing();
    if (parameters["enableOpenToBrowsing"].getAsBool())
        gameSettings.setOpenToBrowsing();

    gameSettings.clearOpenToMatchmaking();
    if (parameters["enableOpenToMatchmaking"].getAsBool())
        gameSettings.setOpenToMatchmaking();
    
    gameSettings.clearOpenToJoinByPlayer();
    if (parameters["enableOpenToJoinByPlayer"].getAsBool())
        gameSettings.setOpenToJoinByPlayer();

    gameSettings.clearAllowSameTeamId();
    if (parameters["isAllowSameTeamId"].getAsBool())
        gameSettings.setAllowSameTeamId();

    gameSettings.clearHostMigratable();
    if (parameters["enableHostMigratable"].getAsBool())
        gameSettings.setHostMigratable();

    gameSettings.clearOpenToInvites();
    if (parameters["enableOpenToInvites"].getAsBool())
        gameSettings.setOpenToInvites();

    gameSettings.clearAdminOnlyInvites();
    if (parameters["enableAdminOnlyInvites"].getAsBool())
        gameSettings.setAdminOnlyInvites();

    gameSettings.clearJoinInProgressSupported();
    if (parameters["enableJoinInProgress"].getAsBool())
        gameSettings.setJoinInProgressSupported();

    gameSettings.clearEnablePersistedGameId();
    if (parameters["enablePersistedId"].getAsBool())
        gameSettings.setEnablePersistedGameId();

    if (parameters["isVirtualGame"].getAsBool())
        gameSettings.setVirtualized();

    gameSettings.clearRanked();
    if (parameters["isRanked"].getAsBool())
        gameSettings.setRanked();

    gameSettings.clearAllowAnyReputation();
    if (parameters["enableAllowAnyReputation"].getAsBool())
        gameSettings.setAllowAnyReputation();

    gameSettings.clearDisconnectReservation();
    if (parameters["enableDisconnectReservation"].getAsBool())
        gameSettings.setDisconnectReservation();

    gameSettings.clearDynamicReputationRequirement();
    if (parameters["enableDynamicReputation"].getAsBool())
        gameSettings.setDynamicReputationRequirement();

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
    gameSettings.clearFriendsBypassClosedToJoinByPlayer();
    if (parameters["enableFriendsBypassClosedToJoinByPlayer"].getAsBool())
        gameSettings.setFriendsBypassClosedToJoinByPlayer();
#endif

    gameSettings.clearAllowMemberGameAttributeEdit();
    if (parameters["allowMemberGameAttributeEdit"].getAsBool())
        gameSettings.setAllowMemberGameAttributeEdit();

    gameSettings.clearAutoDemoteReservedPlayers();
    if (parameters["autoDemoteReservedPlayers"].getAsBool())
        gameSettings.setAutoDemoteReservedPlayers();
}

void GameManagement::parseCreateGameOnlyParams(Blaze::GameManager::Game::CreateGameParametersBase& createGameParams, Pyro::UiNodeParameterStruct &parameters)
{
    createGameParams.mGameType = parameters["gameType"];

    createGameParams.mMaxPlayerCapacity = parameters["maxPlayerCapacity"];
    createGameParams.mMinPlayerCapacity = parameters["minPlayerCapacity"];

    createGameParams.mGamePingSiteAlias = parameters["gamePingSiteAlias"].getAsString();

    createGameParams.mGameReportName.set("integratedSample");
    createGameParams.mServerNotResetable = parameters["serverNotResetable"];
}

template <typename T>
void parseCreateResetGameParams(T& createGameParams, Pyro::UiNodeParameterStruct &parameters, GameManagement& gameManagement)
{
    parseBaseCreateResetGameParams(createGameParams, parameters, gameManagement);

    createGameParams.mJoiningSlotType = parameters["joinSlotType"];
    createGameParams.mJoiningEntryType = parameters["gameEntry"];

    // setup team info
    parseTeamsCreateParameters(parameters, createGameParams, gameManagement);
}

template <typename T>
void parseBaseCreateResetGameParams(T& createGameParams, Pyro::UiNodeParameterStruct &parameters, GameManagement& gameManagement)
{
    createGameParams.mGameName = parameters["gameName"].getAsString();
    createGameParams.mPlayerCapacity[Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT] = parameters["publicParticipantSlots"];
    createGameParams.mPlayerCapacity[Blaze::GameManager::SLOT_PRIVATE_PARTICIPANT] = parameters["privateParticipantSlots"];
    createGameParams.mPlayerCapacity[Blaze::GameManager::SLOT_PUBLIC_SPECTATOR] = parameters["publicSpectatorSlots"];
    createGameParams.mPlayerCapacity[Blaze::GameManager::SLOT_PRIVATE_SPECTATOR] = parameters["privateSpectatorSlots"];

    
    createGameParams.mGameModRegister = parameters["gameMods"];

    createGameParams.mQueueCapacity = parameters["maxQueueCapacity"];
    createGameParams.mNetworkTopology = parameters["networkTopology"];
#if defined(EA_PLATFORM_NX)
    createGameParams.mVoipTopology = Blaze::VoipTopology::VOIP_DISABLED;
#else
    createGameParams.mVoipTopology = parameters["voipTopology"];
#endif

    gameManagement.parseGameSettings(parameters, createGameParams.mGameSettings);

    if (createGameParams.mGameSettings.getEnablePersistedGameId())
    {
        createGameParams.mPersistedGameId = parameters["persistedGameId"].getAsString();
        const char8_t* persistedIdSecretStr = parameters["persistedGameIdSecret"].getAsString();
        createGameParams.mPersistedGameIdSecret.setData((const uint8_t*) persistedIdSecretStr, Blaze::SECRET_MAX_LENGTH);
    }


    createGameParams.mPresenceMode = parameters["presenceMode"];

    const char* attrValueMode;
    const char* attrValueMap;
    char result[Blaze::Collections::MAX_ATTRIBUTENAMEVALUE_LEN] = "";
    attrValueMode = parameters["gameAttributeMode"];
    gameManagement.parseGameAttributes(attrValueMode, result);
    createGameParams.mGameAttributes["mode"] = result;

    attrValueMap = parameters["gameAttributeMap"];
    gameManagement.parseGameAttributes(attrValueMap, result);
    createGameParams.mGameAttributes["ISmap"] = result;

    Pyro::UiNodeParameterArrayPtr gameAttrArray = parameters["GameAttributes"].getAsArray();
    if (gameAttrArray != nullptr)
    {
        for (int32_t i = 0; i < (*gameAttrArray).getCount(); ++i)
        {
            Pyro::UiNodeParameterStructPtr attr = gameAttrArray->at(i)->getAsStruct();
            
            const char* attrName = attr["gameAttrName"];
            const char* attrValue = attr["gameAttrValue"];

            if (attrName != nullptr && attrName[0] != '\0')
            {
                createGameParams.mGameAttributes[attrName] = attrValue;
            }
        }
    }



    // Walkthrough 2: Step 8 - Add the Torchings attribute to the games we create
    
    createGameParams.mGameAttributes["Torchings"] = parameters["gameAttributeTorchings"].getAsString();
    

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
    gameManagement.parseExternalSessionCreateParams(parameters, createGameParams.mSessionTemplateName, createGameParams.mExternalSessionCustomDataString);
#endif
#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
    gameManagement.parseExternalSessionCreateParams(parameters, createGameParams.mExternalSessionCustomData, createGameParams.mExternalSessionStatus);
#endif

    gameManagement.parseRoleInformationParameters(parameters, createGameParams.mRoleInformation);

    // Team Ids: 
    Pyro::UiNodeParameterArrayPtr teamIdsArray = parameters["teamIds"].getAsArray();
    if (teamIdsArray != nullptr)
    {
        for (int32_t i = 0; i < (*teamIdsArray).getCount(); ++i)
        {
            createGameParams.mTeamIds.push_back(teamIdsArray->at(i)->getAsUInt16());
        }
    }
}


void GameManagement::runCreateNewGamePJD(Pyro::UiNodeParameterStruct &parameters, const Blaze::UserGroup *userGroup)
{
    Blaze::GameManager::Game::CreateGameParametersBase createGameParams;
    Blaze::GameManager::PlayerJoinDataHelper playerJoinData;

    parseCreateGameOnlyParams(createGameParams, parameters);

    parseBaseCreateResetGameParams(createGameParams, parameters, *this);
    parsePlayerJoinDataParameters(parameters, playerJoinData.getPlayerJoinData());

    if (userGroup != nullptr)
    {
        playerJoinData.setGroupId(userGroup->getBlazeObjectId());
    }

    mUseNetGameDist = parameters["enableNetGameDist"].getAsBool();

    vLOG(gBlazeHub->getGameManagerAPI()->createGame(createGameParams, playerJoinData, Blaze::GameManager::GameManagerAPI::CreateGameCb(this, &GameManagement::CreateGameCb)));
}

void GameManagement::runCreateNewGame(Pyro::UiNodeParameterStruct &parameters, const Blaze::UserGroup *userGroup)
{
    Blaze::GameManager::Game::CreateGameParameters createGameParams;

    parseCreateGameOnlyParams(createGameParams, parameters);
    parseCreateResetGameParams(createGameParams, parameters, *this);

    mUseNetGameDist = parameters["enableNetGameDist"].getAsBool();

    Blaze::UserIdentificationList reservedExternalIds;
    parseReservedExternalPlayersParameters(parameters, reservedExternalIds);

    vLOG(gBlazeHub->getGameManagerAPI()->createGame(createGameParams,
        Blaze::GameManager::GameManagerAPI::CreateGameCb(this, &GameManagement::CreateGameCb), userGroup, &reservedExternalIds));

    //SAMPLECORE->recallBlazeStateEventHandler(this);
}

void GameManagement::GetTemplatesAndAttributesCb(const Blaze::GameManager::GetTemplatesAndAttributesResponse * templateInfo, Blaze::BlazeError err, Blaze::JobId jobId)
{
    if (err != Blaze::ERR_OK)
    {
        return;
    }

    templateInfo->copyInto(getTemplatesAndAttributesResponse());

    addTemplateActions(getActions(), Pyro::UiNodeAction::ExecuteEventHandler(this, &GameManagement::actionCreateGameTemplate));
}

void GameManagement::addTemplateActions(Pyro::UiNodeActionContainer & outActions, Pyro::UiNodeAction::ExecuteEventHandler executeAction)
{
    // Get the scenarios, add a new action for each one:
    for (auto& templateAttributesMapItr : getTemplatesAndAttributesResponse().getTemplateAttributeDescriptions())
    {
        eastl::string actionName;
        actionName.sprintf("Create Game: %s", templateAttributesMapItr.first.c_str());

        // Replace any existing action with this name: 
        outActions.removeById(actionName.c_str());
        Pyro::UiNodeAction* action = new Pyro::UiNodeAction(actionName.c_str());

        Blaze::GameManager::TemplateAttributeDescriptionMapping::iterator curAttr = templateAttributesMapItr.second->begin();
        Blaze::GameManager::TemplateAttributeDescriptionMapping::iterator endAttr = templateAttributesMapItr.second->end();
        for (; curAttr != endAttr; ++curAttr)
        {
            const char8_t* attrName = curAttr->first;

            // Add all the configurable attributes: (nothing else)
            if (attrName[0] != '\0')
            {
                // Each attribute needs to add a new value, based on the type description:
                //const EA::TDF::TypeDescriptionBitfieldMember* bfMember = nullptr;
                const EA::TDF::TypeDescription* typeDesc = EA::TDF::TdfFactory::get().getTypeDesc(curAttr->second->getAttrTdfId());
                if (typeDesc)
                {
                    EA::TDF::TdfGenericReference defaultValue(curAttr->second->getDefault().get());
                    Pyro::UiNodeParameter* newParam = buildUiFromType(typeDesc, attrName, defaultValue);//, bfMember);
                    if (action->getParameters().findById(newParam->getText()) == nullptr)
                    {
                        action->getParameters().add(newParam);
                    }
                    else
                    {
                        // This can occur if we have a CreateGame attribute and a RuleAttribute at the same time. (The should be the same type, though)
                        delete newParam;
                    }
                }
            }
        }

        setupPlayerJoinDataParameters(action->getParameters());

        // This hidden param is used as a way to pass along a value to the function. 
        action->getParameters().addString("_templateName", templateAttributesMapItr.first.c_str(), "", "", "", Pyro::ItemVisibility::HIDDEN);
        action->Execute += executeAction;

        outActions.add(action);
    }
}


void GameManagement::actionCreateGameTemplate(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    runCreateGameTemplate(parameters, nullptr);
}
void GameManagement::runCreateGameTemplate(Pyro::UiNodeParameterStructPtr parameters, const Blaze::UserGroup *userGroup)
{
    // Template Name:
    const char8_t* templateName = parameters["_templateName"];

    // Player Attributes: 
    Blaze::GameManager::PlayerJoinDataHelper playerJoinData;
    parsePlayerJoinDataParameters(*parameters, playerJoinData.getPlayerJoinData());
    
    // Scenario Attributes: 
    Blaze::GameManager::TemplateAttributes templateAttributes;
    Blaze::GameManager::GetTemplatesAndAttributesResponse::TemplateAttributeDescriptionsMap::const_iterator it = getTemplatesAndAttributesResponse().getTemplateAttributeDescriptions().find(templateName);
    if (it != getScenariosAndAttributesResponse().getScenarioAttributeDescriptions().end())
    {
        Blaze::GameManager::TemplateAttributeDescriptionMapping::iterator currAttr = it->second->begin();
        Blaze::GameManager::TemplateAttributeDescriptionMapping::iterator endAttr = it->second->end();
        for (; currAttr != endAttr; ++currAttr)
        {
            const char8_t* attrName = currAttr->first;
            if (attrName[0] != '\0')
            {
                if (templateAttributes.find(attrName) == templateAttributes.end())
                {
                    templateAttributes[attrName] = templateAttributes.allocate_element();
                }

                EA::TDF::TdfGenericValue genericVal;
                if (EA::TDF::TdfFactory::get().createGenericValue(currAttr->second->getAttrTdfId(), genericVal))
                {
                    EA::TDF::TdfGenericReference genericRef(genericVal);
                    buildGenericDataFromParams(&parameters[attrName], genericRef);
                    templateAttributes[attrName]->set(genericVal);
                }
            }
        }
    }
    
    // Override group id, if one is provided:
    if (userGroup)
    {
        playerJoinData.setGroupId(userGroup->getBlazeObjectId());
    }

    vLOG(gBlazeHub->getGameManagerAPI()->createGameFromTemplate(templateName, templateAttributes, playerJoinData, 
        Blaze::GameManager::GameManagerAPI::JoinGameCb(this, &GameManagement::JoinGameCb)));
}

void GameManagement::setupTeamCriteria(Pyro::UiNodeParameterStruct& parameters, uint16_t teamCount, uint16_t gameCapacity,
    Blaze::GameManager::RoleCriteriaMap& roleCriteria)
{
    if (teamCount == 0)
    {
        // all games have 1 team, but it's valid for a CG param request to not specify any teams
        teamCount = 1;
    }

    uint32_t totalCapacitySet = 0;
    Pyro::UiNodeParameterMapPtr rolesMap = parameters["teamRoleInfo"].getAsMap();
    for (int32_t i = 0; i < rolesMap->getCount(); ++i)
    {
        Pyro::UiNodeParameterMapKeyValuePair* uiMapKeyValuePair = rolesMap->at(i);
        const char8_t* role = uiMapKeyValuePair->getMapKey()->getAsString();
        uint16_t roleCount = uiMapKeyValuePair->getMapValue()->getAsUInt16() * teamCount;

        Blaze::GameManager::RoleCriteria *criteria = roleCriteria.allocate_element();
        roleCriteria[role] = criteria;
        criteria->setRoleCapacity(roleCount);

        totalCapacitySet += roleCount;
    }

    if (totalCapacitySet > gameCapacity)
    {
        // Log a warning and clear the map
        roleCriteria.clear();
        return;
    }
}

void GameManagement::setUpRoles(Pyro::UiNodeParameterStruct& parameters, Blaze::GameManager::RoleInformation& roleInformation, Blaze::GameManager::RoleNameToPlayerMap& desiredRoles)
{
    parseRoleInformationParameters(parameters, roleInformation);

    Pyro::UiNodeParameterMapPtr rolesMap = parameters["desiredRoles"].getAsMap();
    for (int32_t i = 0; i < rolesMap->getCount(); ++i)
    {
        Pyro::UiNodeParameterMapKeyValuePair* uiMapKeyValuePair = rolesMap->at(i);
        const char8_t* role = uiMapKeyValuePair->getMapKey()->getAsString();
        Pyro::UiNodeParameterArrayPtr players = uiMapKeyValuePair->getMapValue()->getAsArray();

        Blaze::GameManager::RoleNameToPlayerMap::iterator it = desiredRoles.find(role);
        Blaze::GameManager::PlayerIdList* playerIds = nullptr;
        if (it == desiredRoles.end())
        {
            playerIds = desiredRoles.allocate_element();
            desiredRoles[role] = playerIds;
        }
        else
        {
            playerIds = it->second;
        }

        for (int32_t j = 0; j < players->getCount(); ++j)
        {
            playerIds->push_back(players->at(j)->getAsUInt64());
        }
    }

}

const char8_t* GameManagement::getInitialPlayerRole(const char8_t* strGameMode, Blaze::GameManager::GameType gameType)
{
    if ((strcmp(strGameMode, "SIMPLE") == 0) || (strcmp(strGameMode, "FIFA") == 0))
    {
        return "defender";
    }
    return Blaze::GameManager::PLAYER_ROLE_NAME_ANY;
}

void GameManagement::CreateGameCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::GameManager::Game *game)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    if (blazeError == Blaze::ERR_OK)
    {
        getGamePlayWindow(game).setupGame(game->getLocalPlayer(gBlazeHub->getPrimaryLocalUserIndex()));

    }

    REPORT_BLAZE_ERROR(blazeError);
}

void GameManagement::runCreateDedicatedServerGame(Pyro::UiNodeParameterStruct &parameters, const Blaze::UserGroup *userGroup)
{
    Blaze::GameManager::Game::ResetGameParameters resetGameParameters;

    parseCreateResetGameParams(resetGameParameters, parameters, *this);

    mUseNetGameDist = parameters["enableNetGameDist"].getAsBool();

    Blaze::UserIdentificationList reservedExternalIds;
    parseReservedExternalPlayersParameters(parameters, reservedExternalIds);

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
    parseExternalSessionCreateParams(parameters, resetGameParameters.mSessionTemplateName, resetGameParameters.mExternalSessionCustomDataString);
#endif
#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
    parseExternalSessionCreateParams(parameters, resetGameParameters.mExternalSessionCustomData, resetGameParameters.mExternalSessionStatus);
#endif

    vLOG(gBlazeHub->getGameManagerAPI()->resetDedicatedServer(resetGameParameters,
        Blaze::GameManager::GameManagerAPI::JoinGameCb(this, &GameManagement::JoinGameCb), nullptr, userGroup, &reservedExternalIds));
}

void GameManagement::runCreateOrJoinGame(Pyro::UiNodeParameterStruct &parameters, const Blaze::UserGroup *userGroup)
{
    Blaze::GameManager::Game::CreateGameParameters createGameParams;

    parseCreateGameOnlyParams(createGameParams, parameters);
    parseCreateResetGameParams(createGameParams, parameters, *this);

    mUseNetGameDist = parameters["enableNetGameDist"].getAsBool();

    Blaze::UserIdentificationList reservedExternalIds;
    parseReservedExternalPlayersParameters(parameters, reservedExternalIds);

    vLOG(gBlazeHub->getGameManagerAPI()->createOrJoinGame(createGameParams,
        Blaze::GameManager::GameManagerAPI::JoinGameCb(this, &GameManagement::JoinGameCb), nullptr, userGroup, &reservedExternalIds));
}

void GameManagement::runJoinGameById(Blaze::GameManager::GameId gameId, const Blaze::UserGroup* userGroup, Blaze::GameManager::TeamIndex teamIndex, Blaze::GameManager::GameEntryType gameEntryType,
                                     Blaze::GameManager::RoleNameToPlayerMap *joiningRoles, Blaze::GameManager::SlotType slotType /* = Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT*/,
                                     const Blaze::UserIdentificationList* reservedExternalPlayerIdentities/* = nullptr*/, const Blaze::GameManager::RoleNameToUserIdentificationMap* externalPlayerIdentityJoiningRoles /*= nullptr*/,
                                     Blaze::GameManager::JoinMethod joinMethod /*= Blaze::GameManager::JOIN_BY_BROWSING*/)
{
    vLOG(gBlazeHub->getGameManagerAPI()->joinGameById(
        gameId,
        joinMethod,
        Blaze::GameManager::GameManagerAPI::JoinGameCb(this, &GameManagement::JoinGameCb), slotType,
        nullptr, userGroup, teamIndex, gameEntryType, nullptr, joiningRoles, 
        reservedExternalPlayerIdentities, externalPlayerIdentityJoiningRoles));
}

void GameManagement::runJoinGameByIdPJD(Blaze::GameManager::GameId gameId, const Blaze::UserGroup* userGroup, Blaze::GameManager::PlayerJoinDataHelper& pjd,
                                        Blaze::GameManager::JoinMethod joinMethod /*= Blaze::GameManager::JOIN_BY_BROWSING*/)
{
    if (userGroup != nullptr)
        pjd.setGroupId(userGroup->getBlazeObjectId());

    vLOG(gBlazeHub->getGameManagerAPI()->joinGameById(
        gameId,
        joinMethod,
        pjd, Blaze::GameManager::GameManagerAPI::JoinGameCb(this, &GameManagement::JoinGameCb)));
}

#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
void GameManagement::runJoinGameByIdWithInvite(Blaze::GameManager::GameId gameId, const Blaze::UserGroup* userGroup, 
                                     Blaze::GameManager::TeamIndex teamIndex, Blaze::GameManager::GameEntryType gameEntryType, Blaze::GameManager::RoleNameToPlayerMap *joiningRoles, Blaze::GameManager::SlotType slotType /* = Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT*/, const Blaze::Collections::AttributeMap* playerAttribs /*= nullptr*/,
                                     const Blaze::UserIdentificationList* reservedExternalIds/* = nullptr*/, const Blaze::GameManager::RoleNameToUserIdentificationMap* userIdentificationJoiningRoles /*= nullptr*/)
{
    vLOG(gBlazeHub->getGameManagerAPI()->joinGameById(
        gameId,
        Blaze::GameManager::JOIN_BY_INVITES,
        Blaze::GameManager::GameManagerAPI::JoinGameCb(this, &GameManagement::JoinGameCb), slotType,
        playerAttribs, userGroup, teamIndex, gameEntryType, nullptr, joiningRoles, reservedExternalIds, userIdentificationJoiningRoles));
}
#endif
#if  defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_PS4_CROSSGEN)
void GameManagement::runCreateOrJoinGameByPlayerSessionId(const char8_t* playerSessionId, const char8_t* activityId)
{
    //sample template and attributes configured on server:
    const Blaze::GameManager::TemplateName createGameTemplateName("createOrJoinByPlayerSessionId");
    const EA::TDF::TdfString playerSessionIdAttrName("PS5_PLAYER_SESSION_ID");
    const EA::TDF::TdfString activityIdAttrName("PS5_ACTIVITY_ID");

    auto it = getTemplatesAndAttributesResponse().getTemplateAttributeDescriptions().find(createGameTemplateName);
    if (it == getTemplatesAndAttributesResponse().getTemplateAttributeDescriptions().end())
    {
        gBlazeHubUiDriver->showMessage_va("runCreateOrJoinGameByPlayerSessionId: Sample create game template(%s) not configured on server. Check your create_game_templates.cfg", createGameTemplateName.c_str());
        return;
    }
    if (it->second->find(playerSessionIdAttrName) == it->second->end())
    {
        gBlazeHubUiDriver->showMessage_va("runCreateOrJoinGameByPlayerSessionId: Sample create game template(%s) not configured with attrName(%s) needed for this sample. Check your create_game_templates.cfg", createGameTemplateName.c_str(), playerSessionIdAttrName.c_str());
        return;
    }
    if (activityId && (activityId[0] != '\0') && (it->second->find(activityIdAttrName) == it->second->end()))
    {
        gBlazeHubUiDriver->showMessage_va("runCreateOrJoinGameByPlayerSessionId: Sample create game template(%s) not configured with attrName(%s) needed for this sample. Check your create_game_templates.cfg", createGameTemplateName.c_str(), activityIdAttrName.c_str());
        return;
    }

    Blaze::GameManager::TemplateAttributes templateAttrs;
    templateAttrs[playerSessionIdAttrName] = templateAttrs.allocate_element();
    templateAttrs[playerSessionIdAttrName]->set(EA::TDF::TdfString(playerSessionId));

    templateAttrs[activityIdAttrName] = templateAttrs.allocate_element();
    templateAttrs[activityIdAttrName]->set(EA::TDF::TdfString(activityId));

    Blaze::GameManager::PlayerJoinDataHelper playerJoinData;

    vLOG(gBlazeHub->getGameManagerAPI()->createGameFromTemplate(createGameTemplateName, templateAttrs, playerJoinData,
        Blaze::GameManager::GameManagerAPI::JoinGameCb(this, &GameManagement::JoinGameCb)));
}

#endif

void GameManagement::runJoinGameByUserName(const char8_t* userName, const Blaze::UserGroup* userGroup, 
    Blaze::GameManager::TeamIndex teamIndex, Blaze::GameManager::GameEntryType gameEntryType, Blaze::GameManager::RoleNameToPlayerMap *joiningRoles, Blaze::GameManager::SlotType slotType /* = Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT*/,
    const Blaze::UserIdentificationList* reservedExternalIds/* = nullptr*/, const Blaze::GameManager::RoleNameToUserIdentificationMap* userIdentificationJoiningRoles /*= nullptr*/, Blaze::GameManager::GameType gameType /*Blaze::GameManager::GAME_TYPE_GAMESESSION*/)
{
    vLOG(gBlazeHub->getGameManagerAPI()->joinGameByUsername(
        userName,
        Blaze::GameManager::JOIN_BY_PLAYER,
        Blaze::GameManager::GameManagerAPI::JoinGameCb(this, &GameManagement::JoinGameCb), slotType,
        nullptr, userGroup, teamIndex, gameEntryType, nullptr, joiningRoles, gameType, reservedExternalIds, userIdentificationJoiningRoles));
}


void GameManagement::runJoinGameByUserList(Blaze::GameManager::GameId gameId, Blaze::GameManager::TeamIndex teamIndex, 
                                           const Blaze::UserIdentificationList &reservedExternalIds,
                                           const Blaze::GameManager::RoleNameToUserIdentificationMap *userIdentificationJoiningRoles,
                                           Blaze::GameManager::JoinMethod joinMethod /*= Blaze::GameManager::JOIN_BY_INVITES*/)
{
    vLOG(gBlazeHub->getGameManagerAPI()->joinGameByUserList(
        gameId,
        joinMethod,
        Blaze::GameManager::GameManagerAPI::JoinGameCb(this, &GameManagement::JoinGameByUserListCb),
        reservedExternalIds,
        Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT, teamIndex, Blaze::GameManager::GAME_TYPE_GAMESESSION,
        userIdentificationJoiningRoles));
}

void GameManagement::ResultNotificationCb(const Blaze::GameReporting::ResultNotification *resultNotification, uint32_t userIndex)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    if (static_cast<Blaze::BlazeError>(resultNotification->getBlazeError()) == Blaze::ERR_OK)
    {
        Leaderboards &leaderboards = (Leaderboards &)getUiDriver()->getUiBuilder("Leaderboards");
        leaderboards.GetLeaderboardDataCenteredAroundCurrentPlayer("IS_LB_allmodes");
    }

    REPORT_BLAZE_ERROR(static_cast<Blaze::BlazeError>(resultNotification->getBlazeError()));
}


void GameManagement::parseExternalSessionCreateParams(Pyro::UiNodeParameterStruct &parameters,
    Blaze::XblSessionTemplateName& sessionTemplateName, Blaze::ExternalSessionCustomDataString& customData)
{
    // X1's custom data uses strings
    sessionTemplateName = parameters["sessionTemplateName"].getAsString();
    customData = parameters["externalSessionCustomData"].getAsString();
}

void GameManagement::parseExternalSessionCreateParams(Pyro::UiNodeParameterStruct &parameters,
    Blaze::ExternalSessionCustomData& customData, Blaze::ExternalSessionStatus& status)
{    
    // status
    parseExternalSessionStatus(parameters, status);

    // custom data
    const char8_t* filePath = parameters["externalSessionCustomDataFilePath"].getAsString();
    if ((filePath != nullptr) && (filePath[0] != '\0'))
    {
        FILE *fp = fopen(filePath, "rb");
        if (fp != nullptr)
        {
            uint8_t dataBuf[1024];
            memset(dataBuf, 0, sizeof(dataBuf));
            size_t dataSize = fread(dataBuf, 1, sizeof(dataBuf), fp);
            if (dataSize > 0)
            {
                customData.setData(dataBuf, (EA::TDF::TdfSizeVal)dataSize);
            }
            fclose(fp);
        }
    }
}

void GameManagement::parseExternalSessionStatus(Pyro::UiNodeParameterStruct &parameters, Blaze::ExternalSessionStatus& status)
{
    // status default
    status.setUnlocalizedValue(parameters["externalSessionStatusDefault"].getAsString());

    // status localizations
    Pyro::UiNodeParameterArrayPtr langArray = parameters["externalSessionStatusLocalizations"].getAsArray();
    if (langArray != nullptr)
    {
        for (int32_t i = 0; i < (*langArray).getCount(); ++i)
        {
            Pyro::UiNodeParameterStructPtr langItem = langArray->at(i)->getAsStruct();
            const char* langCode = langItem["langCode"];
            const char* locText = langItem["localizedText"];
            if (langCode != nullptr && langCode[0] != '\0')
            {
                status.getLangMap()[langCode] = locText;
            }
        }
    }
}

void GameManagement::parseReservedExternalPlayersParameters(Pyro::UiNodeParameterStruct &parameters, Blaze::UserIdentificationList &reservedPlayerIdentifications)
{
    Pyro::UiNodeParameterArrayPtr playerArray = parameters["reservedExternalPlayers"].getAsArray();
    if (playerArray == nullptr)
        return;

    for (int32_t i = 0; i < (*playerArray).getCount(); ++i)
    {
        Pyro::UiNodeParameterStructPtr userInfoParam = playerArray->at(i)->getAsStruct();
        Blaze::ExternalId externalId = userInfoParam["externalId"];
        Blaze::BlazeId blazeId = userInfoParam["blazeId"];
        Blaze::UserIdentification* ident = reservedPlayerIdentifications.pull_back();
        ident->setBlazeId(blazeId);
        gBlazeHub->setExternalStringOrIdIntoPlatformInfo(ident->getPlatformInfo(), externalId, userInfoParam["externalString"]);
    }
}

void GameManagement::parsePreferredGamesParameters(Pyro::UiNodeParameterStruct &parameters, Blaze::GameManager::MatchmakingCriteriaData& mCriteria)
{
    mCriteria.getPreferredGamesRuleCriteria().setRequirePreferredGame(parameters["requireGames"].getAsBool());
    Pyro::UiNodeParameterArrayPtr gameArray = parameters["preferredGames"].getAsArray();
    if (gameArray == nullptr)
        return;

    for (int32_t i = 0; i < (*gameArray).getCount(); ++i)
    {
        Blaze::GameManager::GameId gameId = gameArray->at(i)->getAsUInt64();
        mCriteria.getPreferredGamesRuleCriteria().getPreferredList().push_back(gameId);
    }
}

void GameManagement::parsePreferredPlayersParameters(Pyro::UiNodeParameterStruct &parameters, Blaze::GameManager::MatchmakingCriteriaData& mCriteria)
{
    mCriteria.getPreferredPlayersRuleCriteria().setRequirePreferredPlayer(parameters["requirePlayers"].getAsBool());
    Pyro::UiNodeParameterArrayPtr playerArray = parameters["preferredPlayers"].getAsArray();
    if (playerArray == nullptr)
        return;

    for (int32_t i = 0; i < (*playerArray).getCount(); ++i)
    {
        Blaze::BlazeId playerId = playerArray->at(i)->getAsUInt64();
        mCriteria.getPreferredPlayersRuleCriteria().getPreferredList().push_back(playerId);
    }
}

void GameManagement::setDefaultRole(Blaze::GameManager::RoleNameToPlayerMap& joiningRoles, const char8_t* roleNameStr)
{
    Blaze::GameManager::RoleName roleName;
    roleName.set((roleNameStr != nullptr)? roleNameStr : "");
    Blaze::GameManager::PlayerIdList *playerIdList = joiningRoles.allocate_element();
    playerIdList->push_back(Blaze::INVALID_BLAZE_ID);
    joiningRoles[roleName] = playerIdList;
}


/*! ************************************************************************************************/
/*! \brief ConnectionManagerListener process delayed handling of protocol activation for Xbox One
***************************************************************************************************/
void GameManagement::onQosPingSiteLatencyRetrieved(bool success)
{
#if defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)
    Ignition::ProtocolActivationInfo::checkProtocolActivation();
#endif
}

#if defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)
Blaze::GameManager::GameId GameManagement::getGameIdFromMps(Microsoft::Xbox::Services::Multiplayer::MultiplayerSession^ const mps)
{
    Platform::String^ jsonConstants = mps->SessionConstants->CustomConstantsJson;
    Windows::Data::Json::JsonValue^ jsonValue = Windows::Data::Json::JsonValue::Parse(jsonConstants);
    Platform::String^ gameIdStr = jsonValue->GetObject()->GetNamedString("externalSessionId");
    Blaze::GameManager::GameId gameId = _wcstoui64(gameIdStr->Data(), nullptr, 10);
    return gameId;
}

Blaze::GameManager::GameType GameManagement::getGameTypeFromMps(Microsoft::Xbox::Services::Multiplayer::MultiplayerSession^ const mps)
{
    Platform::String^ jsonConstants = mps->SessionConstants->CustomConstantsJson;
    Windows::Data::Json::JsonValue^ jsonValue = Windows::Data::Json::JsonValue::Parse(jsonConstants);
    Blaze::GameManager::GameType gameType = (Blaze::GameManager::GameType)((int64_t)
        jsonValue->GetObject()->GetNamedNumber("externalSessionType"));
    return gameType;
}
Platform::String^ GameManagement::getCustomDataStringFromMps(Microsoft::Xbox::Services::Multiplayer::MultiplayerSession^ const mps)
{
    Platform::String^ jsonConstants = mps->SessionConstants->CustomConstantsJson;
    Windows::Data::Json::JsonValue^ jsonValue = Windows::Data::Json::JsonValue::Parse(jsonConstants);
    Platform::String^ customDataStr = jsonValue->GetObject()->GetNamedString("externalSessionCustomDataString");
    return customDataStr;
}
#endif


}// BlazeSample
