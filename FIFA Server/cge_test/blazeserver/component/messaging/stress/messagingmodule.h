/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_MESSAGINGMODULE_H
#define BLAZE_STRESS_MESSAGINGMODULE_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#include "blazerpcerrors.h"
#include "EASTL/intrusive_list.h"
#include "EASTL/vector.h"
#include "framework/util/random.h"
#include "messaging/tdf/messagingtypes.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "stressmodule.h"
#include "stressinstance.h"
#include "loginmanager.h"

namespace Blaze
{

namespace Messaging
{
    class MessagingSlave;
}

namespace Stress
{

class MessagingModule;

class MessagingInstance : public StressInstance, 
                          public AsyncHandler,
                          public eastl::intrusive_list_node
{
    NON_COPYABLE(MessagingInstance);

public:
    explicit MessagingInstance(MessagingModule* owner, StressConnection* connection, Login* login);
    ~MessagingInstance() override;

    void onDisconnected() override;
    void onAsync(uint16_t component, uint16_t type, RawBuffer* payload) override;

protected:
    void onLogin(BlazeRpcError result) override;

private:
    const char8_t* getName() const override { return mName; }

    BlazeRpcError execute() override;
    BlazeRpcError sendMessages();
    BlazeRpcError sendGlobalMessages();
    BlazeRpcError purgeMessages();
    BlazeRpcError fetchMessages();
    BlazeRpcError touchMessages();
    BlazeRpcError simulateProd();

    BlazeRpcError sendToInstance(MessagingInstance*);
    BlazeRpcError sendToNeighbor(MessagingInstance*);
    BlazeRpcError sendToGamegroup(MessagingInstance*);
    BlazeRpcError sendToAll();
    void purgeMessage(Messaging::MessageId messageId);
    void createGamegroup();
    void joinGamegroup();
    void idle();

private:
    const char8_t *mName;

    BlazeId mBlazeId;
    bool mInboxFull;
    uint32_t mMessagesReceived;
    uint32_t mPersistentMessages;
    Blaze::GameManager::GameId mGamegroupId;
    MessagingModule* mOwner;
    Blaze::GameManager::GameManagerSlave* mGameManagerProxy;
    Blaze::Messaging::MessagingSlave* mMessagingProxy;
    Blaze::Messaging::ServerMessage mServerMessage;
};

class MessagingModule : public StressModule
{
    NON_COPYABLE(MessagingModule);
    
    friend class MessagingInstance;
    
    typedef eastl::intrusive_list<MessagingInstance> Instances;
    typedef eastl::list<Blaze::Messaging::ClientMessagePtr> ClientMessageList;

    typedef enum
    {
        ACTION_SENDMESSAGES,
        ACTION_SENDGLOBALMESSAGES,
        ACTION_FETCHMESSAGES,
        ACTION_PURGEMESSAGES,
        ACTION_TOUCHMESSAGES,
        ACTION_SIMULATEPROD
    } Action;

    struct Probabilities
    {
        uint32_t sendMessages;
        uint32_t sendGlobalMessages;
        uint32_t fetchMessages;
        uint32_t purgeMessages;
        uint32_t touchMessages;
    };

public:
    ~MessagingModule() override;
    bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil) override;
    StressInstance* createInstance(StressConnection* connection, Login* login) override;
    const char8_t* getModuleName() override {return "messaging";}

    Action getAction() const { return mAction; }
    Probabilities getProbabilities() const { return mProbablities; }
    uint32_t getMaxSleepTime() const { return mMaxSleepTime; }
    Blaze::Messaging::FetchMessageRequest *getFetchMessageRequest() { return &mFetchMessageReq; }
    const Blaze::Messaging::ClientMessage *getRandomMessage() const
    {
        int index = Blaze::Random::getRandomNumber(static_cast<int>(mClientMessageList.size()));

        ClientMessageList::const_iterator cmIter, cmEnd;
        cmIter = mClientMessageList.begin();
        cmEnd = mClientMessageList.end();

        for (int i = 0; i < index && cmIter != cmEnd; ++cmIter, ++i)
        {
        }

        return *cmIter;
    }

    static StressModule* create();

private:
    MessagingModule();

    void loginReceived(BlazeRpcError result);
    GameManager::GameId getNextGamegroupId();
    bool getNeedMoreGamegroups();
    bool getAllGamegroupsCreated();

    bool getAllLoginsReceived() const;

private:
    typedef eastl::vector<GameManager::GameId> GamegroupList;
    typedef eastl::vector<MessagingInstance*> GamegrouplessList;

    Action mAction;  // Which command to issue
    Probabilities mProbablities;
    uint32_t mMaxSleepTime;

    Instances mInstances;
    uint32_t mInstanceCount;
    uint32_t mInstanceLoginCount;
    bool mWaitForLogins;
    ClientMessageList mClientMessageList;
    uint32_t mTag;
    bool mJoinGamegroup;
    uint32_t mGamegroupPendingCount;
    uint32_t mGamegroupIndex;
    uint16_t mGamegroupLimit;
    uint16_t mGamegroupMemberLimit;
    GamegroupList mGamegroupList;
    GamegrouplessList mGamegrouplessList;
    bool mPurge;
    uint32_t mPurgeDelay;
    Blaze::Messaging::FetchMessageRequest mFetchMessageReq;
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_MESSAGINGMODULE_H

