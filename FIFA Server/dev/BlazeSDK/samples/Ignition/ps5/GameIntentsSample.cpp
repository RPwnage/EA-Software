#if defined(EA_PLATFORM_PS5) || (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
#include "Ignition/ps5/GameIntentsSample.h"
#include "Ignition/GameManagement.h"

#include "BlazeSDK/shared/gamemanager/externalsessionjsondatashared.h"
#include "DirtySDK/dirtysock/ps5/dirtywebapi2ps5.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#if defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4)
#include "DirtySDK/dirtysock/ps4/dirtyeventmanagerps4.h"
#endif
#include <EAJson/Json.h> //For EA Json in getPlayerSessionCallback
#include <EAJson/JsonDomReader.h>
#include <system_service.h> //for SceSystemServiceStatus in idle


namespace Ignition
{
Ignition::GameIntentsSample GameIntentsSample::mGameIntentsSample;

GameIntentsSample::~GameIntentsSample()
{    
    clearCachedOnlineJoinParams(mParamsForOnlineJoins);
    // Do not call DirtyWebApiDestroy(mWebApi) here. To ensure that can be done
    // before NetConnShutdown(), its handled via GameIntentsSample::shutdown()
}

void GameIntentsSample::init()
{
    SceNpGameIntentInitParam intentInitParam;
    memset(&intentInitParam, 0, sizeof(intentInitParam));
    sceNpGameIntentInitParamInit(&intentInitParam);
    auto iSceResult = sceNpGameIntentInitialize(&intentInitParam);
    if (iSceResult != SCE_OK)
    {
        NetPrintf(("[GameIntentsSample].init: Ignition: sceNpGameIntentInitialize (err=%s).\n", DirtyErrGetName(iSceResult)));
    }    

#if defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4)
    // On PS4, system events centrally consumed via DirtyEventManager
    DirtyEventManagerRegisterSystemService(&handleGameIntentEvent, nullptr);
#endif
}
void GameIntentsSample::shutdown()
{
    sceNpGameIntentTerminate();
    if (mGameIntentsSample.mWebApi != nullptr)
    {
        DirtyWebApiDestroy(mGameIntentsSample.mWebApi);
    }
}


void GameIntentsSample::doIdle()
{
    int32_t iSceResult = SCE_OK;
    if (!mWebApi)
    {
        if ((mWebApi = DirtyWebApiCreate(nullptr)) == nullptr)
        {
            NetPrintf(("[GameIntentsSample].doIdle: Ignition: DirtyWebApiCreate failed, see logs. NOOP.\n"));
            return;
        }    
	}

    // handle game intents
    SceSystemServiceStatus status;
    memset(&status, 0, sizeof(status));
    iSceResult = sceSystemServiceGetStatus(&status);
    if (iSceResult != SCE_OK)
    {
        NetPrintf(("[GameIntentsSample].doIdle: Ignition: sceSystemServiceGetStatus failed (err=%s).\n", DirtyErrGetName(iSceResult)));
        return;
    }
    for (int32_t i = 0; i < status.eventNum; ++i)
    {
        SceSystemServiceEvent event;
        iSceResult = sceSystemServiceReceiveEvent(&event);
        if (iSceResult != SCE_OK)
        {
            continue; //logged
        }
        NetPrintf(("[GameIntentsSample].doIdle: Ignition: received event #(%d)/(%d), eventType(%" PRIi64 ").\n", i, status.eventNum, event.eventType));        
        handleGameIntentEvent(nullptr, &event);
    }
}

//handle system event if its a GameIntent
void GameIntentsSample::handleGameIntentEvent(void *pUserData, const SceSystemServiceEvent *event)
{
    if ((event != nullptr) && (event->eventType == SCE_SYSTEM_SERVICE_EVENT_GAME_INTENT))
    {
        NetPrintf(("[GameIntentsSample].handleGameIntentEvent: Ignition: Handle eventType(%" PRIi64 ").\n", event->eventType));
        int32_t iSceResult = SCE_OK;
        SceNpGameIntentInfo intentInfo;
        sceNpGameIntentInfoInit(&intentInfo);
        iSceResult = sceNpGameIntentReceiveIntent(&intentInfo);
        if (iSceResult != SCE_OK)
        {
            NetPrintf(("[GameIntentsSample].doIdle: Ignition: sceNpGameIntentReceiveIntent failed (err=%s).\n", DirtyErrGetName(iSceResult)));
            return;
        }

        if (strncmp(intentInfo.intentType, "joinSession", sizeof(intentInfo.intentType)) == 0)
        {
            // get the PlayerSessionId
            char playerSessionId[Blaze::MAX_PSNPLAYERSESSIONID_LEN];
            iSceResult = sceNpGameIntentGetPropertyValueString(&intentInfo.intentData, "playerSessionId", playerSessionId, sizeof(playerSessionId));
            if (iSceResult != SCE_OK)
            {
                NetPrintf(("[GameIntentsSample].doIdle: Ignition: sceNpGameIntentGetPropertyValueString(playerSessionId) failed (err=%s).\n", DirtyErrGetName(iSceResult)));
                return;
            }
            // get the memberType
            char memberType[17];//(size as documented in 0.95 SDK docs)
            iSceResult = sceNpGameIntentGetPropertyValueString(&intentInfo.intentData, "memberType", memberType, sizeof(memberType));
            if (iSceResult != SCE_OK)
            {
                NetPrintf(("[GameIntentsSample].doIdle: Ignition: sceNpGameIntentGetPropertyValueString(playerSessionId) failed (err=%s).\n", DirtyErrGetName(iSceResult)));
                return;
            }

            SampleJoinParams sampleJoinParams;
            sampleJoinParams.mPlayerSessionId = playerSessionId;
            sampleJoinParams.mSlotType = ((strncmp(memberType, "spectator", sizeof(memberType)) == 0) ? Blaze::GameManager::SLOT_PUBLIC_SPECTATOR : Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT);
            mGameIntentsSample.joinBlazeGame(sampleJoinParams);
        }
        else if (strncmp(intentInfo.intentType, "launchMultiplayerActivity", sizeof(intentInfo.intentType)) == 0)
        {
            char playerSessionId[Blaze::MAX_PSNPLAYERSESSIONID_LEN];
            iSceResult = sceNpGameIntentGetPropertyValueString(&intentInfo.intentData, "playerSessionId", playerSessionId, sizeof(playerSessionId));
            if (iSceResult != SCE_OK)
            {
                NetPrintf(("[GameIntentsSample].doIdle: Ignition: sceNpGameIntentGetPropertyValueString(playerSessionId) failed (err=%s).\n", DirtyErrGetName(iSceResult)));
                return;
            }
            char activityId[Blaze::MAX_PSNUDSOBJECTID_LEN];
            iSceResult = sceNpGameIntentGetPropertyValueString(&intentInfo.intentData, "activityId", activityId, sizeof(activityId));
            if (iSceResult != SCE_OK)
            {
                NetPrintf(("[GameIntentsSample].doIdle: Ignition: sceNpGameIntentGetPropertyValueString(activityId) failed (err=%s).\n", DirtyErrGetName(iSceResult)));
                return;
            }

            SampleJoinParams sampleJoinParams;
            sampleJoinParams.mPlayerSessionId = playerSessionId;
            sampleJoinParams.mActivityId = activityId;
            sampleJoinParams.mSlotType = Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT;
            mGameIntentsSample.joinBlazeGame(sampleJoinParams);
        }
    }
}

void GameIntentsSample::joinBlazeGame(const SampleJoinParams& sampleJoinParams)
{
    // cache off the partially complete join game params before fetching the PlayerSession, to fill remaining params
    auto pendingJoinIt = mParamsForOnlineJoins.insert(sampleJoinParams.mPlayerSessionId);
    if (!pendingJoinIt.second)
    {
        return;//already joining
    }
    pendingJoinIt.first->second = sampleJoinParams;
    
    auto iSceResult = getPlayerSession(sampleJoinParams.mPlayerSessionId, &getPlayerSessionCallback);
    if (iSceResult < 0)
    {
        NetPrintf(("[GameIntentsSample].joinBlazeGame: GET playerSession(%s) failed (err=%s).\n", sampleJoinParams.mPlayerSessionId.c_str(), DirtyErrGetName(iSceResult)));
        clearCachedOnlineJoinParams(mParamsForOnlineJoins, sampleJoinParams.mPlayerSessionId.c_str());
    }
}

int64_t GameIntentsSample::getPlayerSession(const eastl::string& playerSessionId, DirtyWebApiCallbackT *pCallback)
{
    eastl::string header(eastl::string::CtorSprintf(), "X-PSN-SESSION-MANAGER-SESSION-IDS:%s", playerSessionId.c_str());
    eastl::string path(eastl::string::CtorSprintf(), "/v1/playerSessions?fields=sessionId,customData1,customData2");

    NetPrintf(("[GameIntentsSample].getPlayerSession sending GET request(%s), header(%s).\n", path.c_str(), header.c_str()));

    return DirtyWebApiRequestEx(mWebApi, 0, "sessionManager", SCE_NP_WEBAPI2_HTTP_METHOD_GET, path.c_str(), NULL, 0, NULL, header.c_str(), pCallback, &mParamsForOnlineJoins);
}

/*! ************************************************************************************************/
/*! \brief Parse the PlayerSession response. Ref Sony API doc: https://p.siedev.net/resources/documents/WebAPI/1/Session_Manager_WebAPI-Reference/0003.html
***************************************************************************************************/
bool GameIntentsSample::parsePlayerSessionResponse(eastl::string& parsedPlayerSessionId, Blaze::ExternalSessionBlazeSpecifiedCustomDataPs5& parsedBlazeCustData,
    const char *respBody, int32_t respBodyLen)
{
    if ((respBody == nullptr) || (respBodyLen == 0))
    {
        NetPrintf(("[GameIntentsSample].parsePlayerSessionResponse: failed, or missing rsp body.\n"));
        return false;
    }
    // Setup JSON parser
    EA::Json::JsonDomReader reader;
    reader.SetString(respBody, respBodyLen, false);
    EA::Json::JsonDomDocument doc;
    auto result = reader.Build(doc);
    if (result != EA::Json::kSuccess)
    {
        NetPrintf(("[GameIntentsSample].parsePlayerSessionResponse: Could not parse json response. error: 0x%x at index %d (%d:%d), length: %d, data:\n%s", result, (int32_t)reader.GetByteIndex(), reader.GetLineNumber(), reader.GetColumnNumber(), respBodyLen, respBody));
        return false;
    }

    // get the PlayerSessionId
    auto* playerSessionList = doc.GetArray("/playerSessions");
    if (!playerSessionList || (playerSessionList->mJsonDomNodeArray.size() != 1))
    {
        NetPrintf(("[GameIntentsSample].parsePlayerSessionResponse: Expecting 1 PlayerSession info, but got(%d) from response.\n", (playerSessionList ? playerSessionList->mJsonDomNodeArray.size() : 0)));
        return false;
    }
    auto* sessionId = doc.GetString("/playerSessions/0/sessionId");
    if (!sessionId || sessionId->mValue.empty())
    {
        NetPrintf(("[GameIntentsSample].parsePlayerSessionResponse: PlayerSession rsp body missing its sessionId.\n"));
        return false;
    }
    parsedPlayerSessionId = sessionId->mValue;

    // get the customData1. It can be missing/null if this is a boot-to-group/create-or-join GameIntent
    auto* customData1 = doc.GetString("/playerSessions/0/customData1");
    if (customData1 != nullptr)
    {
        eastl::string custData1Str(customData1->mValue.c_str());
        if (!Blaze::GameManager::psnPlayerSessionCustomDataToBlazeData(parsedBlazeCustData, custData1Str))
        {
            NetPrintf(("[GameIntentsSample].parsePlayerSessionResponse: failed to parse rsp body customData1(%s).\n", custData1Str.c_str()));
            return false;
        }
    }
    return true;
}



/*! ************************************************************************************************/
/*! \brief On fetch of PlayerSession, use the GameId Blaze embedded into its customdata1 to join the Blaze Game.
***************************************************************************************************/
void GameIntentsSample::getPlayerSessionCallback(int32_t iSceResult, int32_t iUserIndex, int32_t iStatusCode, const char *respBody, int32_t respBodyLen, void *pUserData)
{
    auto pendingJoinInfos = (CachedJoinParamsMap*)pUserData;
    Blaze::ExternalSessionBlazeSpecifiedCustomDataPs5 parsedCustData(gsAllocator, "GameIntentsSample::custData");
    eastl::string parsedPlayerSessionId;

    // Parse PlayerSession rsp customData1 to get Blaze GameId to join
    if ((iSceResult != SCE_OK) ||
        !parsePlayerSessionResponse(parsedPlayerSessionId, parsedCustData, respBody, respBodyLen))
    {
        NetPrintf(("GameIntentsSample: GET PlayerSession (err=%s). Unable to parse rsp.\n", DirtyErrGetName(iSceResult)));
        clearCachedOnlineJoinParams(*pendingJoinInfos, (parsedPlayerSessionId.empty() ? nullptr : parsedPlayerSessionId.c_str()));
        return;
    }
    auto itr = pendingJoinInfos->find(parsedPlayerSessionId);
    if (itr == pendingJoinInfos->end())
    {
        NetPrintf(("GameIntentsSample: join session(%s) was canceled.\n", parsedPlayerSessionId.c_str()));
        return;
    }
    SampleJoinParams sampleJoinParams(itr->second);
    clearCachedOnlineJoinParams(*pendingJoinInfos, parsedPlayerSessionId.c_str());//(ungating next req, in case there is another, for this PlayerSession)


    sampleJoinParams.mGameId = parsedCustData.getGameId();

    // If user not fully logged in, keep the info cached off for joining game once player is.
    if (!isTitleActivated())
    {
        mGameIntentsSample.mParamsForTitleActivationJoin = sampleJoinParams;
        return;
    }
    doJoinGame(sampleJoinParams);
}

void GameIntentsSample::doJoinGame(const SampleJoinParams& sampleJoinParams)
{
    // If GameId available, join the Blaze Game. Else, create-or-join Game
    Blaze::GameManager::GameId gameId = sampleJoinParams.mGameId;
    if (gameId != Blaze::GameManager::INVALID_GAME_ID)
    {
        gBlazeHubUiDriver->getGameManagement().runJoinGameByIdWithInvite(
            gameId,
            nullptr,
            Blaze::GameManager::UNSPECIFIED_TEAM_INDEX,
            Blaze::GameManager::GAME_ENTRY_TYPE_DIRECT, nullptr,
            sampleJoinParams.mSlotType, nullptr);
    }
    else
    {
        gBlazeHubUiDriver->getGameManagement().runCreateOrJoinGameByPlayerSessionId(
            sampleJoinParams.mPlayerSessionId.c_str(), 
            sampleJoinParams.mActivityId.c_str());
    }
}


void GameIntentsSample::clearCachedOnlineJoinParams(CachedJoinParamsMap& map, const char8_t* onlyPlayerSessionId)
{
    if (!onlyPlayerSessionId)
    {
        map.clear();
    }
    else
    {
        auto it = map.find(eastl::string(onlyPlayerSessionId));
        if (it != map.end())
        {
            map.erase(eastl::string(onlyPlayerSessionId));
        }
    }
}

/*! ************************************************************************************************/
/*! \brief Handle delayed join from title launch as neededHandle delayed join from title launch as needed
***************************************************************************************************/
void GameIntentsSample::checkTitleActivation()
{
    if (!mGameIntentsSample.mParamsForTitleActivationJoin.mPlayerSessionId.empty() && isTitleActivated())
    {
        // cache off copy to use, and clear the orig
        const auto params = mGameIntentsSample.mParamsForTitleActivationJoin;
        mGameIntentsSample.mParamsForTitleActivationJoin.clear();

        doJoinGame(params);
    }
}

bool GameIntentsSample::isTitleActivated()
{
    if ((gBlazeHubUiDriver == nullptr) || (gBlazeHub == nullptr) || (gBlazeHub->getGameManagerAPI() == nullptr) || (gBlazeHub->getConnectionManager() == nullptr))
    {
        NetPrintf(("[GameIntentsSample].isTitleActivated: cannot run game manager operations yet as game manager api is not yet initialized."));
        return false;
    }

    // If Blaze ping site data not ready (needed before game joins) wait for onQosPingSiteLatencyRetrieved first.
    if (!gBlazeHub->getConnectionManager()->initialQosPingSiteLatencyRetrieved())
        return false;

    // If network adapter not yet initialized init it so we can join (i.e. we beat GMAPI's onAuthenticated to it here).
    if (!gBlazeHub->getGameManagerAPI()->getNetworkAdapter()->isInitialized())
    {
        gBlazeHub->getGameManagerAPI()->getNetworkAdapter()->initialize(gBlazeHub);
    }
    return true;
}


}//ns

#endif
