/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class MessagingModule


*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/rpc/usersessions_defines.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "messaging/rpc/messagingslave.h"
#include "messagingmodule.h"

using namespace Blaze::Messaging;

namespace Blaze
{
namespace Stress
{

/*
 * List of available RPCs that can be called by the module.
 */
static const char8_t* ACTION_STR_SENDMESSAGES       = "sendMessages";
static const char8_t* ACTION_STR_SENDGLOBALMESSAGES = "sendGlobalMessages";
static const char8_t* ACTION_STR_FETCHMESSAGES      = "fetchMessages";
static const char8_t* ACTION_STR_PURGEMESSAGES      = "purgeMessages";
static const char8_t* ACTION_STR_TOUCHMESSAGES      = "touchMessages";
static const char8_t* ACTION_STR_SIMULATEPROD       = "simulateProd";

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

MessagingInstance::MessagingInstance(MessagingModule* owner, StressConnection* connection, Login* login)
:   StressInstance(owner, connection, login, BlazeRpcLog::messaging), 
    mBlazeId(0), mInboxFull(false), mMessagesReceived(0),mPersistentMessages(0),
    mGamegroupId(GameManager::INVALID_GAME_ID),
    mOwner(owner),
    mGameManagerProxy(mOwner->mJoinGamegroup ? BLAZE_NEW GameManager::GameManagerSlave(*connection) : nullptr),
    mMessagingProxy(BLAZE_NEW MessagingSlave(*connection))
{
    connection->addAsyncHandler(MessagingSlave::COMPONENT_ID, MessagingSlave::NOTIFY_NOTIFYMESSAGE, this);
    mName = "";
}

MessagingInstance::~MessagingInstance()
{
    MessagingModule::Instances::remove(*this);
    getConnection()->removeAsyncHandler(MessagingSlave::COMPONENT_ID, MessagingSlave::NOTIFY_NOTIFYMESSAGE, this);
    delete mMessagingProxy;
    delete mGameManagerProxy;
}

void
MessagingInstance::onDisconnected()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].onDisconnected: User(" << mBlazeId << ") disconnected!");
}

void
MessagingInstance::onLogin(BlazeRpcError result)
{
    if (result == ERR_OK)
    {
        mBlazeId = getLogin()->getUserLoginInfo()->getBlazeId();
        BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].onLogin: User(" << mBlazeId << ") logged in!");
        mOwner->loginReceived(result);
        if(mGameManagerProxy)
        {
            if(mOwner->getNeedMoreGamegroups())
            {
                createGamegroup();
            }
            else
            {
                joinGamegroup();
            }
        }
    }
}

void
MessagingInstance::onAsync(uint16_t component, uint16_t type, RawBuffer* payload)
{
    if(component == MessagingSlave::COMPONENT_ID)
    {
        if(type == MessagingSlave::NOTIFY_NOTIFYMESSAGE)
        {
            Heat2Decoder decoder;
            if(Blaze::ERR_OK == decoder.decode(*payload, mServerMessage))
            {
                if(mServerMessage.getPayload().getFlags().getPersistent())
                {
                    ++mPersistentMessages;

                    if(mOwner->mPurge)
                    {
                        if(mOwner->mPurgeDelay)
                        {
                            TimeValue purgeTime(TimeValue::getTimeOfDay() + mOwner->mPurgeDelay*1000*1000);
                            Fiber::CreateParams params(Fiber::STACK_SMALL);
                            gSelector->scheduleFiberTimerCall(purgeTime, this, &MessagingInstance::purgeMessage, 
                                mServerMessage.getMessageId(), "MessagingInstance::purgeMessage", params);
                        }
                        else
                        {
                            Fiber::CreateParams params(Fiber::STACK_SMALL);
                            gSelector->scheduleFiberCall(this, &MessagingInstance::purgeMessage, 
                                mServerMessage.getMessageId(), "MessagingInstance::purgeMessage", params);
                        }
                    }
                }
                ++mMessagesReceived;
                BLAZE_TRACE_LOG(BlazeRpcLog::messaging, 
                    "[MessagingInstance].onAsync: received message: "
                    "{src:" << mServerMessage.getSourceIdent().getName() 
                    << ", id:" << mServerMessage.getMessageId() 
                    <<", type:" << mServerMessage.getPayload().getType() 
                    << ", tag:" << mServerMessage.getPayload().getTag() 
                    << ", cflags:" << mServerMessage.getPayload().getFlags().getBits() 
                    << ", sflags:" << mServerMessage.getFlags().getBits() 
                    << ", time:" << mServerMessage.getTimestamp() 
                    << ", attr_cnt:" << mServerMessage.getPayload().getAttrMap().size() << "}");
            }
            else
            {
                BLAZE_ERR_LOG(BlazeRpcLog::messaging, 
                    "[MessagingInstance].onAsync: Error when decoding received message!");
            }
        }
    }
}

BlazeRpcError
MessagingInstance::execute()
{
    BlazeRpcError result = ERR_OK;

    switch (mOwner->getAction())
    {
        case MessagingModule::ACTION_SENDMESSAGES:
        {
            mName = ACTION_STR_SENDMESSAGES;
            Fiber::setCurrentContext(mName);
            result = sendMessages();
            break;
        }
        case MessagingModule::ACTION_SENDGLOBALMESSAGES:
        {
            mName = ACTION_STR_SENDGLOBALMESSAGES;
            Fiber::setCurrentContext(mName);
            result = sendGlobalMessages();
            break;
        }
        case MessagingModule::ACTION_FETCHMESSAGES:
        {
            mName = ACTION_STR_FETCHMESSAGES;
            Fiber::setCurrentContext(mName);
            result = fetchMessages();
            break;
        }
        case MessagingModule::ACTION_PURGEMESSAGES:
        {
            mName = ACTION_STR_PURGEMESSAGES;
            Fiber::setCurrentContext(mName);
            result = purgeMessages();
            break;
        }
        case MessagingModule::ACTION_TOUCHMESSAGES:
        {
            mName = ACTION_STR_TOUCHMESSAGES;
            Fiber::setCurrentContext(mName);
            result = touchMessages();
            break;
        }
        case MessagingModule::ACTION_SIMULATEPROD:
        {
            mName = ACTION_STR_SIMULATEPROD;
            result = simulateProd();
            break;
        }
        default:
            break;
    }

    BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].execute: Action finished with result " << Blaze::ErrorHelp::getErrorName(result));

    return result;
}

BlazeRpcError
MessagingInstance::sendMessages()
{
    BlazeRpcError result = Blaze::ERR_OK;

    if (!mOwner->mWaitForLogins || mOwner->getAllLoginsReceived())
    {
        if (mOwner->mJoinGamegroup)
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].sendMessages: Sending a message to a game group");
            result = sendToGamegroup(this);
        }
        else
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].sendMessages: Sending a message to a user");
            result = sendToNeighbor(this);
        }
    }
    else
    {
        // If the not all instances have logged in, wait until they are before loading the server...
        // ALSO NOTE: If we returned without sleeping we'd end up burning a lot of CPU
        // while busy-waiting and would slow down the startup of other instances!
        BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].sendMessages: Waiting for all instances to login");
        idle();
    }

    return result;
}

BlazeRpcError
MessagingInstance::sendGlobalMessages()
{
    BlazeRpcError result = Blaze::ERR_OK;

    if (!mOwner->mWaitForLogins || mOwner->getAllLoginsReceived())
    {
        SendGlobalMessageRequest globalMessage;
        ClientMessage* message = mOwner->getRandomMessage()->clone();

        if (!message->getFlags().getLocalize())
        {
            globalMessage.getFlags().clearLocalize();
        }

        globalMessage.setTag(message->getTag());
        globalMessage.setType(message->getType());
        globalMessage.getAttrMap().copyInto(message->getAttrMap());

        SendGlobalMessageResponse resp;
        result = mMessagingProxy->sendGlobalMessage(globalMessage, resp);
        delete message;
    }
    else
    {
        // If the not all instances have logged in, wait until they are before loading the server...
        // ALSO NOTE: If we returned without sleeping we'd end up burning a lot of CPU
        // while busy-waiting and would slow down the startup of other instances!
        BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].sendGlobalMessages: Waiting for all instances to login");
        idle();
    }

    return result;
}

BlazeRpcError
MessagingInstance::fetchMessages()
{
    BlazeRpcError result = Blaze::ERR_OK;

    if (!mOwner->mWaitForLogins || mOwner->getAllLoginsReceived())
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].fetchMessages: Fetching messages");

        FetchMessageRequest *request = mOwner->getFetchMessageRequest();
        FetchMessageResponse response;

        result = mMessagingProxy->fetchMessages(*request, response);

        BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].fetchMessages: Expecting to receive "
                        << response.getCount() <<" messages");

        while (mMessagesReceived < response.getCount())
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].fetchMessages: Waiting for "
                            << response.getCount()-mMessagesReceived << " messages");
            idle();
        }

        BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].fetchMessages: Finished receiving "
                        << mMessagesReceived << " messages");

        mMessagesReceived = 0;
    }
    else
    {
        // If the not all instances have logged in, wait until they are before loading the server...
        // ALSO NOTE: If we returned without sleeping we'd end up burning a lot of CPU
        // while busy-waiting and would slow down the startup of other instances!
        BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].fetchMessages: Waiting for all instances to login");
        idle();
    }

    // no messages in inbox, consider as no problem
    if (result == MESSAGING_ERR_MATCH_NOT_FOUND)
    {
        result = ERR_OK;
    }

    return result;
}

BlazeRpcError
MessagingInstance::purgeMessages()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].purgeMessages: Purging messages");

    PurgeMessageRequest request;
    PurgeMessageResponse response;

    BlazeRpcError rc = mMessagingProxy->purgeMessages(request, response);
    if (rc == MESSAGING_ERR_MATCH_NOT_FOUND)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].purgeMessages: No messages to purge");
        rc = ERR_OK;
    }
    else if (ERR_OK == rc)
    {
        mPersistentMessages -= response.getCount();
    }
    return rc;
}

BlazeRpcError
MessagingInstance::touchMessages()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].touchMessages: Touching messages");

    TouchMessageRequest request;
    TouchMessageResponse response;

    return mMessagingProxy->touchMessages(request, response);
}

BlazeRpcError
MessagingInstance::simulateProd()
{
    BlazeRpcError result = ERR_OK;

    uint32_t rand = static_cast<uint32_t>(Blaze::Random::getRandomNumber(100)) + 1;

    uint32_t sendMessagesProb = mOwner->getProbabilities().sendMessages;
    uint32_t fetchMessagesProb = mOwner->getProbabilities().fetchMessages + sendMessagesProb;
    uint32_t purgeMessagesProb = mOwner->getProbabilities().purgeMessages + fetchMessagesProb;
    uint32_t touchMessagesProb = mOwner->getProbabilities().touchMessages + purgeMessagesProb;
    uint32_t sendGlobalMessagesProb = mOwner->getProbabilities().sendGlobalMessages + touchMessagesProb;

    if ((rand > 0) && (rand <= sendMessagesProb))
    {
        BLAZE_INFO_LOG(BlazeRpcLog::messaging, "[MessagingInstance].simulateProd: Sending messages");
        Fiber::setCurrentContext(ACTION_STR_SENDMESSAGES);
        result = sendMessages();
    }
    else if ((rand > sendMessagesProb) && (rand <= fetchMessagesProb))
    {
        if (mPersistentMessages > 0)
        {
            BLAZE_INFO_LOG(BlazeRpcLog::messaging, "[MessagingInstance].simulateProd: Fetching messages");
            Fiber::setCurrentContext(ACTION_STR_FETCHMESSAGES);
            result = fetchMessages();
        }
    }
    else if ((rand > fetchMessagesProb) && (rand <= purgeMessagesProb))
    {
        if (mPersistentMessages > 0)
        {
            BLAZE_INFO_LOG(BlazeRpcLog::messaging, "[MessagingInstance].simulateProd: Purging messages");
            Fiber::setCurrentContext(ACTION_STR_PURGEMESSAGES);
            result = purgeMessages();

            if (result == ERR_OK )
            {
                mPersistentMessages = 0;
            }
        }
    }
    else if ((rand > purgeMessagesProb) && (rand <= touchMessagesProb))
    {
        BLAZE_INFO_LOG(BlazeRpcLog::messaging, "[MessagingInstance].simulateProd: Touching messages");
        Fiber::setCurrentContext(ACTION_STR_TOUCHMESSAGES);
        result = touchMessages();
    }
    else if ((rand > touchMessagesProb) && (rand <= sendGlobalMessagesProb))
    {
        BLAZE_INFO_LOG(BlazeRpcLog::messaging, "[MessagingInstance].simulateProd: Sending global messages");
        Fiber::setCurrentContext(ACTION_STR_SENDGLOBALMESSAGES);
        result = sendGlobalMessages();
    }
    else
    {
        idle();
    }

    return result;
}

BlazeRpcError
MessagingInstance::sendToInstance(MessagingInstance* instance)
{
    BlazeRpcError rc;
    BlazeId blazeId = instance->getLogin()->getUserLoginInfo()->getBlazeId();
    if(blazeId && !instance->mInboxFull)
    {
        ClientMessage* message = mOwner->getRandomMessage()->clone();   
        if (Random::getRandomNumber(1000) > 20)
        {
            message->getFlags().clearPersistent();
        }
        else 
        {
            message->getFlags().setPersistent();    
        }

        if(this == instance)
        {
            message->getFlags().setEcho();
        }
        else
        {
            message->getFlags().clearEcho();
        }
        message->setTargetType(ENTITY_TYPE_USER);
        message->getTargetIds().clear();
        message->getTargetIds().push_back(blazeId);
        //message->setTag(mOwner->mTag++);
        SendMessageResponse resp;
        rc = mMessagingProxy->sendMessage(*message, resp);
        delete message;
        if(Blaze::MESSAGING_ERR_TARGET_INBOX_FULL == rc)
        {
            instance->mInboxFull = true;
        }
        return rc;
    }
    // If the blazeId is 0, then the instance is not yet logged in,
    // also if the inbox is full, we need to sleep a little, then return.
    // NOTE: If we returned without sleeping we'd end up burning a lot of CPU
    // while busy-waiting and would slow down the startup of other instances!
    idle();
    return Blaze::ERR_OK;
}

BlazeRpcError
MessagingInstance::sendToNeighbor(MessagingInstance* instance)
{
    // convert node to iterator O(1), unsafe, but we know the instance is part of the list...
    MessagingModule::Instances::iterator it(instance);
    // get next item in the list, if none, then wrap around
    if(++it == mOwner->mInstances.end())
    {
        it = mOwner->mInstances.begin();
    }
    return sendToInstance(&(*it));
}

BlazeRpcError
MessagingInstance::sendToGamegroup(MessagingInstance* instance)
{
    BlazeRpcError rc;
    if(instance->mGamegroupId != GameManager::INVALID_GAME_ID)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].sendToGamegroup: Sending message to game group id: " << instance->mGamegroupId);

        ClientMessage* message = mOwner->getRandomMessage()->clone();
        if (Random::getRandomNumber(1000) > 20)
        {
            message->getFlags().clearPersistent();
        }
        else
        {
            message->getFlags().setPersistent();
        }

        if(mGamegroupId == instance->mGamegroupId)
        {
            message->getFlags().setEcho();
        }
        else
        {
            message->getFlags().clearEcho();
        }
        message->setTargetType(GameManager::ENTITY_TYPE_GAME_GROUP);
        message->getTargetIds().clear();
        message->getTargetIds().push_back(instance->mGamegroupId);
        //message->setTag(mOwner->mTag++);
        SendMessageResponse resp;
        rc = mMessagingProxy->sendMessage(*message, resp);
        delete message;
        return rc;
    }

    BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].sendToGamegroup: Idling as the instance is not in a game group yet");

    // If the mGamegroupId is GameManager::INVALID_GAME_ID, then the instance is not yet joined the game group,
    // we need to sleep a little, then return.
    // NOTE: If we returned without sleeping we'd end up burning a lot of CPU
    // while busy-waiting and would slow down the startup of other instances!
    idle();
    return Blaze::ERR_OK;
}

BlazeRpcError
MessagingInstance::sendToAll()
{
    BlazeRpcError rc;
    MessagingModule::Instances::iterator it = mOwner->mInstances.begin();
    for(; it != mOwner->mInstances.end(); ++it)
    {
        rc = sendToInstance(&(*it));
        if(Blaze::ERR_OK != rc)
            return rc;
    }
    return Blaze::ERR_OK;
}

void
MessagingInstance::purgeMessage(Messaging::MessageId messageId)
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    BlazeRpcError rc;
    Blaze::Messaging::PurgeMessageRequest purgeReq;
    purgeReq.getFlags().setMatchId();
    purgeReq.setMessageId(messageId);
    PurgeMessageResponse resp;
    rc = mMessagingProxy->purgeMessages(purgeReq, resp);
    if(Blaze::ERR_OK != rc)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::messaging, "[MessagingInstance].purgeMessage: purgeMessage(" << messageId << ") failed, rc: " <<  ErrorHelp::getErrorName(rc));
    }
    else
    {
        --mPersistentMessages;
    }
}

void
MessagingInstance::createGamegroup()
{
    BlazeRpcError rc;
    char8_t buf[64];
    GameManager::CreateGameRequest req;
    req.getCommonGameData().setGameType(GameManager::GAME_TYPE_GROUP);
    req.getGameCreationData().setNetworkTopology(NETWORK_DISABLED);

    GameManager::GameSettings& gamegroupSettings = req.getGameCreationData().getGameSettings();
    gamegroupSettings.setHostMigratable();
    gamegroupSettings.setOpenToBrowsing();
    gamegroupSettings.setOpenToInvites();
    gamegroupSettings.setOpenToJoinByPlayer();
    gamegroupSettings.setOpenToMatchmaking();

    GameManager::CreateGameResponse resp;
    ++mOwner->mGamegroupPendingCount;
    blaze_snzprintf(buf, 64, "gamegroup%03u", mOwner->mGamegroupPendingCount);

    IpAddress ipAddr;
    ipAddr.setIp(getConnection()->getAddress().getIp());
    ipAddr.setPort(getConnection()->getAddress().getPort());
    req.getCommonGameData().getPlayerNetworkAddress().setIpAddress(&ipAddr);

    req.getGameCreationData().setGameName(buf);
    req.getGameCreationData().setMaxPlayerCapacity(mOwner->mGamegroupMemberLimit);
    req.getSlotCapacities()[GameManager::SLOT_PUBLIC_PARTICIPANT] = mOwner->mGamegroupMemberLimit;
    rc = mGameManagerProxy->createGame(req, resp);
    if(Blaze::ERR_OK == rc)
    {
        mGamegroupId = resp.getGameId();

        // We need to advance the game state of the newly-created game group
        // to allow other players to join.
        GameManager::AdvanceGameStateRequest agsReq;
        agsReq.setGameId(mGamegroupId);
        agsReq.setNewGameState(GameManager::GAME_GROUP_INITIALIZED);
        rc = mGameManagerProxy->advanceGameState(agsReq);
    }
    if (Blaze::ERR_OK != rc)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::messaging,"[MessagingInstance].createGamegroup: Failed to create game group("
            << buf <<"), reason: " << ErrorHelp::getErrorName(rc) << "!");
        mGamegroupId = GameManager::INVALID_GAME_ID;
        return;
    }

    BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].createGamegroup: Created & Joined game group("
                    << buf << ":)" << mGamegroupId );

    mOwner->mGamegroupList.push_back(mGamegroupId);
    if(mOwner->getAllGamegroupsCreated())
    {
        // we just created our last game group, therefore walk all the existing
        // instances and assign them evenly across game groups we've created.
        for(MessagingModule::GamegrouplessList::const_iterator 
            i = mOwner->mGamegrouplessList.begin(),
            e = mOwner->mGamegrouplessList.end(); i != e; ++i)
        {
            (*i)->joinGamegroup();
        }
        mOwner->mGamegrouplessList.clear();
    }
}

void MessagingInstance::joinGamegroup()
{
    BlazeRpcError rc;
    if(mOwner->getAllGamegroupsCreated())
    {
        GameManager::JoinGameRequest req;
        GameManager::JoinGameResponse resp;
        EntryCriteriaError joinError;

        mGamegroupId = mOwner->getNextGamegroupId();
        req.setGameId(mGamegroupId);
        rc = mGameManagerProxy->joinGame(req, resp, joinError);
        if(Blaze::ERR_OK == rc)
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::messaging, "[MessagingInstance].joinGamegroup: Joined game group(" << mGamegroupId << ")");
        }
        else
        {
            BLAZE_ERR_LOG(BlazeRpcLog::messaging, "[MessagingInstance].joinGamegroup: Failed to join game group("
                          << mGamegroupId << "), reason: " << ErrorHelp::getErrorName(rc) << "!");
        }
    }
    else
    {
        // add self to pending instances list
        mOwner->mGamegrouplessList.push_back(this);
    }
}

void MessagingInstance::idle()
{
    sleep(5);
}


/*** Public Methods ******************************************************************************/

// static
StressModule* MessagingModule::create()
{
    return BLAZE_NEW MessagingModule();
}

MessagingModule::MessagingModule()
:   mInstanceCount(0),
    mInstanceLoginCount(0),
    mWaitForLogins(false),
    mTag(1),
    mJoinGamegroup(false),
    mGamegroupPendingCount(0),
    mGamegroupIndex(0),
    mGamegroupLimit(0),
    mGamegroupMemberLimit(0),
    mPurge(false),
    mPurgeDelay(0)
{
}

MessagingModule::~MessagingModule()
{
    ClientMessageList::iterator cmIter, cmEnd;
    cmIter = mClientMessageList.begin();
    cmEnd = mClientMessageList.end();

    mClientMessageList.clear();
}

bool MessagingModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    if (!StressModule::initialize(config, bootUtil))
    {
        BLAZE_ERR_LOG(BlazeRpcLog::messaging, "[MessagingModule].initialize: Failed to initialize base module!");
        return false;
    }

    const char8_t* action = config.getString("action", ACTION_STR_SENDMESSAGES);
    if (blaze_stricmp(action, ACTION_STR_SENDMESSAGES) == 0)
    {
        mAction = ACTION_SENDMESSAGES;
    }
    else if (blaze_stricmp(action, ACTION_STR_SENDGLOBALMESSAGES) == 0)
    {
        mAction = ACTION_SENDGLOBALMESSAGES;
    }
    else if (blaze_stricmp(action, ACTION_STR_FETCHMESSAGES) == 0)
    {
        mAction = ACTION_FETCHMESSAGES;
    }
    else if (blaze_stricmp(action, ACTION_STR_PURGEMESSAGES) == 0)
    {
        mAction = ACTION_PURGEMESSAGES;
    }
    else if (blaze_stricmp(action, ACTION_STR_TOUCHMESSAGES) == 0)
    {
        mAction = ACTION_TOUCHMESSAGES;
    }
    else if (blaze_stricmp(action, ACTION_STR_SIMULATEPROD) == 0)
    {
        mAction = ACTION_SIMULATEPROD;

        const ConfigMap* simMap = config.getMap("sim");
        if (simMap != nullptr)
        {
            const ConfigMap* probabilityMap = simMap->getMap("probability");

            if (probabilityMap != nullptr)
            {
                mProbablities.sendMessages = probabilityMap->getUInt32("sendMessages", 0);
                mProbablities.sendGlobalMessages = probabilityMap->getUInt32("sendGlobalMessages", 0);
                mProbablities.fetchMessages = probabilityMap->getUInt32("fetchMessages", 0);
                mProbablities.purgeMessages = probabilityMap->getUInt32("purgeMessages", 0);
                mProbablities.touchMessages = probabilityMap->getUInt32("touchMessages", 0);
            }

            if (mProbablities.sendMessages + mProbablities.sendGlobalMessages + 
                mProbablities.fetchMessages + mProbablities.purgeMessages + 
                mProbablities.touchMessages > 100)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::messaging, "[MessagingModule].initialize: All probabilities combined are greater than 100");
                return false;
            }

            mMaxSleepTime = simMap->getUInt32("maxSleep", 5);
        }
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::messaging, "[MessagingModule].initialize: Unrecognized action: '" << action << "'");
        return false;
    }

    mWaitForLogins = config.getBool("waitForLogins", true);
    mJoinGamegroup = config.getBool("joinGamegroup", false);
    if(mJoinGamegroup)
    {
        mGamegroupLimit = config.getUInt16("gamegroupLimit", 1);
        mGamegroupMemberLimit = config.getUInt16("gamegroupMemberLimit", 100);
    }
    mPurge = config.getBool("purge", false);
    mPurgeDelay = config.getUInt32("purgeDelay", 0);

    const ConfigMap* fetchMessageMap = config.getMap("fetchMessage");
    if (fetchMessageMap != nullptr)
    {
        if (fetchMessageMap->getBool("matchId", false))
        {
            mFetchMessageReq.getFlags().setMatchId();
        }
        if (fetchMessageMap->getBool("matchSource", false))
        {
            mFetchMessageReq.getFlags().setMatchSource();
        }
        if (fetchMessageMap->getBool("matchType", false))
        {
            mFetchMessageReq.getFlags().setMatchType();
        }
        if (fetchMessageMap->getBool("matchTarget", false))
        {
            mFetchMessageReq.getFlags().setMatchTarget();
        }
        if (fetchMessageMap->getBool("matchStatus", false))
        {
            mFetchMessageReq.getFlags().setMatchStatus();
        }

        mFetchMessageReq.setType(fetchMessageMap->getUInt32("type", 0));
        mFetchMessageReq.setStatusMask(fetchMessageMap->getUInt32("statusMask", 0));
        mFetchMessageReq.setStatus(fetchMessageMap->getUInt32("status", 0));
    }

    const ConfigSequence* messagesSequence = config.getSequence("messages");
    if(!messagesSequence)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::messaging, "[MessagingModule].initialize: Message to send not found!");
        return false;
    }

    for (size_t i = 0; i < messagesSequence->getSize(); ++i)
    {
        const ConfigMap* mconfig = messagesSequence->nextMap();

        Messaging::ClientMessage message;

        message.setType(mconfig->getUInt32("type", 0));
        message.setTag(mconfig->getUInt32("tag", 0));
        if(mconfig->getBool("persistent", false))
        {
            message.getFlags().setPersistent();
        }
        if(mconfig->getBool("profanityFilter", false))
        {
            message.getFlags().setFilterProfanity();
        }
        if(mconfig->getBool("localize", false))
        {
            message.getFlags().setLocalize();
        }
        if(mconfig->getBool("echo", false))
        {
            message.getFlags().setEcho();
        }
        const ConfigSequence* aconfig = mconfig->getSequence("attrMap");
        if(aconfig)
        {
            int32_t key;
            const char8_t* str;
            while(aconfig->hasNext())
            {
                key = aconfig->nextInt32(0);
                str = aconfig->nextString("<error reading value!>");
                message.getAttrMap().insert(eastl::make_pair(key, str));
            }
        }

        mClientMessageList.push_back(message.clone());
    }

    BLAZE_INFO_LOG(BlazeRpcLog::messaging, "[MessagingModule].initialize: Config file parsed.");

    return true;
}

StressInstance* MessagingModule::createInstance(StressConnection* connection, Login* login)
{
    MessagingInstance* instance = BLAZE_NEW MessagingInstance(this, connection, login);
    mInstances.push_back(*instance);
    ++mInstanceCount;
    return instance;
}

void MessagingModule::loginReceived(BlazeRpcError result)
{
    ++mInstanceLoginCount;
}

GameManager::GameId MessagingModule::getNextGamegroupId()
{
    const GameManager::GameId gamegroupId = mGamegroupList[mGamegroupIndex];
    mGamegroupIndex = (mGamegroupIndex + 1) % mGamegroupLimit;
    return gamegroupId;
}

bool MessagingModule::getNeedMoreGamegroups()
{
    return mGamegroupPendingCount < mGamegroupLimit;
}

bool MessagingModule::getAllGamegroupsCreated()
{
    return mGamegroupList.size() == mGamegroupLimit;
}

bool MessagingModule::getAllLoginsReceived() const
{
    return mInstanceCount == mInstanceLoginCount;
}

} // Stress
} // Blaze

