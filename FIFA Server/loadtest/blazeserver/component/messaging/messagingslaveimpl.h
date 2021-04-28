/*************************************************************************************************/
/*!
    \file   messagingslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MESSAGING_SLAVEIMPL_H
#define BLAZE_MESSAGING_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "messaging/rpc/messagingslave_stub.h"
#include "messaging/tdf/messagingtypes.h"

#include "framework/database/query.h"
#include "framework/taskscheduler/taskschedulerslaveimpl.h"
#include "framework/metrics/metrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class UserSession;

namespace Messaging
{

class MessageInfo;

class MessagingSlaveImpl 
    : public MessagingSlaveStub, TaskEventHandler
{
public:
    MessagingSlaveImpl();
    ~MessagingSlaveImpl() override;
    
    BlazeRpcError sendMessage(const EA::TDF::ObjectId &sender, ClientMessage& message, MessageIdList* messageIds = nullptr, bool isGlobalMessage = false);
    BlazeRpcError fetchMessages(UserSessionId userSessionId, const FetchMessageRequest& request, uint32_t* count = nullptr, uint32_t seqNo = 0);
    BlazeRpcError purgeMessages(UserSessionId userSessionId, const PurgeMessageRequest& request, uint32_t* count = nullptr);
    BlazeRpcError touchMessages(UserSessionId userSessionId, const TouchMessageRequest& request, uint32_t* count = nullptr);
    BlazeRpcError getMessages(UserSessionId userSessionId, const FetchMessageRequest& request, ServerMessageList* messages);
    BlazeRpcError sendGlobalMessage(const EA::TDF::ObjectId &sender, SendGlobalMessageRequest& message, MessageIdList* messageIds = nullptr);
    
    static bool setAttrParam(ClientMessage& message, AttrKey attrKey, uint32_t paramIndex, const char8_t* param);
    static const char8_t* getAttrParam(const ClientMessage& message, AttrKey attrKey, uint32_t paramIndex);
    
    /*! \brief Populate the given status object with the component's current status.*/
    void getStatusInfo(ComponentStatus& status) const override;
    uint16_t getDbSchemaVersion() const override { return 2; }
    const DbNameByPlatformTypeMap* getDbNameByPlatformTypeMap() const override { return &getConfig().getDbNamesByPlatform(); }
    
private:
    bool onValidateConfig(MessagingConfig& config, const MessagingConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
    bool onConfigure() override;
    bool onReconfigure() override;

    void sendMessageToUserSessions(ServerMessage& message, const UserSessionIdVectorSet& userSessionIds);
    void broadcastMessage(ServerMessage& serverMessage, const UserSessionIdList& targetUserSessionIdList, const UserSessionIdList& sourceSessionIds);

    bool onPrepareForReconfigure(const MessagingConfig& config) override;
    bool configure(bool reconfigure);

    BlazeRpcError dbConfigure();

    void genMessagingSentEvent(ServerMessage& message, bool isGlobalMessage = false) const;
    BlazeRpcError retrieveMessagesFromDb(UserSessionId userSessionId, const FetchMessageRequest& request, ServerMessageList* messages);

    void onScheduledTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName) override; 
    void onExecutedTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName) override { doSweep(); } 
    void onCanceledTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName) override { mSweepTaskId = INVALID_TASK_ID; scheduleSweep(); }

    void scheduleSweep();
    void cancelSweep();
    void doSweep();

private:
    BlazeRpcError buildInsertQuery(
        const ServerMessage* message, 
        QueryPtr& query, 
        bool messageOverwrite,
        uint32_t messageLimit, 
        uint32_t attrLimit,
        uint32_t* numQueries,
        uint32_t* insertQuery
    ) const;
    BlazeRpcError writeMessageToDb(
        uint32_t dbId, 
        ServerMessage* message, 
        bool messageOverwrite, 
        uint32_t messageLimit, 
        uint32_t attrLimit
    ) const;
    BlazeRpcError fillSourceName(ServerMessage&);
    BlazeRpcError sendLocalizedMessageToSessions(ServerMessage& serverMessage, const UserSessionIdVectorSet& userSessionIds);
    BlazeRpcError getNextUniqueMessageId(MessageId& messageId);

    static bool isMessageSourceOmitted(const EA::TDF::ObjectId &messageSource) { return (messageSource.type == EA::TDF::OBJECT_TYPE_INVALID); }

    uint32_t mPersistentMessageAttributeLimit;
    char8_t mProfanityBuffer[MSG_ATTR_STR_MAX+1];

    // temp data used in onPrepareForReconfigure()
    uint32_t mTempPersistentMessageAttributeLimit;
    TaskId mSweepTaskId;
    MessageTypeList mSkipSweepTypeList;

    Metrics::Counter mForwardedRegularMessages;
    Metrics::Counter mForwardedPersistentMessages;
    Metrics::Counter mDeliveredRegularMessages;
    Metrics::Counter mDeliveredPersistentMessages;
    Metrics::Counter mProfanityFilteredMessages;
    Metrics::Counter mLocalizedMessages;
    Metrics::Counter mDroppedInboundMessages;
    Metrics::Counter mPurgedMessages;
    Metrics::Counter mFetchedMessages;
    Metrics::Counter mTouchedMessages;
    Metrics::Counter mInvalidSenderIdentityWarnings;
    Metrics::Counter mInvalidTargetWarnings;
    Metrics::Counter mTargetNotFoundWarnings;
    Metrics::Counter mTargetInboxFullWarnings;
    Metrics::Counter mSweptStaleMessages;
    Metrics::Counter mMessageDbSweeps;
    Metrics::Counter mEmptyMessageDbSweeps;
    Metrics::Counter mFailedMessageDbSweeps;

};

} // Messaging
} // Blaze

#endif // BLAZE_MESSAGING_SLAVEIMPL_H

