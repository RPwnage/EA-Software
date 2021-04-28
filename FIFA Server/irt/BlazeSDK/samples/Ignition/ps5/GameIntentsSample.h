/*! ************************************************************************************************/
/*!
    \file GameIntentsSample.h

    \brief Sample util for PS5 GameIntents handling

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef IGNITION_SAMPLE_GAME_INTENTS_SAMPLE_H
#define IGNITION_SAMPLE_GAME_INTENTS_SAMPLE_H

#if defined(EA_PLATFORM_PS5) || (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
#include "Ignition/Ignition.h"
#include <np/np_game_intent.h>
#include <system_service.h> //for SceSystemServiceEvent in handleGameIntentEvent
#include "BlazeSDK/component/gamemanager/tdf/gamemanager.h"
struct DirtyWebApiRefT;
typedef void (DirtyWebApiCallbackT)(int32_t iSceError, int32_t iUserIndex, int32_t iStatusCode, const char *pRespBody, int32_t iRespBodyLength, void *pUserData);

namespace Ignition
{

class GameIntentsSample
{
public:
    GameIntentsSample() : mWebApi(nullptr) {}
    ~GameIntentsSample();

    static void init();
    static void shutdown();
    static void idle() { mGameIntentsSample.doIdle(); }
    static void checkTitleActivation();
    static DirtyWebApiRefT* getWebApi2() { return mGameIntentsSample.mWebApi; }

private:
    static Ignition::GameIntentsSample mGameIntentsSample;

    struct SampleJoinParams
    {
        SampleJoinParams() : mGameId(Blaze::GameManager::INVALID_GAME_ID), mSlotType(Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT) {}
        void clear()
        {
            mActivityId.clear();
            mPlayerSessionId.clear();
            mGameId = Blaze::GameManager::INVALID_GAME_ID;
            mSlotType = Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT;
        }
        eastl::string mActivityId;//for launchMultiplayerActivity GameIntent
        eastl::string mPlayerSessionId;
        Blaze::GameManager::GameId mGameId;//for joinSession GameIntent
        Blaze::GameManager::SlotType mSlotType;
    };
    typedef eastl::hash_map<eastl::string, SampleJoinParams> CachedJoinParamsMap;

    void doIdle();

    static void handleGameIntentEvent(void *pUserData, const SceSystemServiceEvent *event); //pUserData is just so this can be a DirtyEventManagerSystemServiceCallbackT on PS4
    int64_t getPlayerSession(const eastl::string& playerSessionId, DirtyWebApiCallbackT *pCallback);
    static void getPlayerSessionCallback(int32_t iSceResult, int32_t iUserIndex, int32_t iStatusCode, const char *pRespBody, int32_t iRespBodyLength, void *pUserData);
    void joinBlazeGame(const SampleJoinParams& sampleJoinParams);
    static void doJoinGame(const SampleJoinParams& sampleJoinParams);
    static bool parsePlayerSessionResponse(eastl::string& parsedPlayerSessionId, Blaze::ExternalSessionBlazeSpecifiedCustomDataPs5& parsedBlazeCustData, const char *respBody, int32_t respBodyLength);
    
    static void clearCachedOnlineJoinParams(CachedJoinParamsMap& map, const char8_t* onlyPlayerSessionId = nullptr);
    static bool isTitleActivated();

    DirtyWebApiRefT* mWebApi;
    CachedJoinParamsMap mParamsForOnlineJoins;
    SampleJoinParams mParamsForTitleActivationJoin;
};

}//ns Ignition

#endif
#endif //IGNITION_SAMPLE_GAME_INTENTS_SAMPLE_H
