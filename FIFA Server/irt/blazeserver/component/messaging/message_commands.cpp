/*************************************************************************************************/
/*!
    \file   pokeslave_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Message Commands

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/usersessionmaster.h"

// messaging includes
#include "messaging/rpc/messagingslave/sendmessage_stub.h"
#include "messaging/rpc/messagingslave/fetchmessages_stub.h"
#include "messaging/rpc/messagingslave/getmessages_stub.h"
#include "messaging/rpc/messagingslave/purgemessages_stub.h"
#include "messaging/rpc/messagingslave/touchmessages_stub.h"
#include "messaging/rpc/messagingslave/sendsourcemessage_stub.h"
#include "messaging/rpc/messagingslave/sendglobalmessage_stub.h"

#include "messagingslaveimpl.h"

namespace Blaze
{
namespace Messaging
{

////////////////////////////////////////////////////////
class SendMessageCommand : public SendMessageCommandStub
{
public:
    SendMessageCommand(Message* message, ClientMessage* request, MessagingSlaveImpl* componentImpl)
        : SendMessageCommandStub(message, request), mComponent(componentImpl)
    { }

    /* Private methods *******************************************************************************/
private:
    SendMessageCommandStub::Errors execute() override;
    
private:
    // Not owned memory.
    MessagingSlaveImpl* mComponent;
};

SendMessageCommandStub::Errors SendMessageCommand::execute()
{
    EA::TDF::ObjectId source;
    if (gCurrentUserSession)
        source = EA::TDF::ObjectId(ENTITY_TYPE_USER, gCurrentUserSession->getBlazeId());

    BlazeRpcError rc = mComponent->sendMessage(source, mRequest, &mResponse.getMessageIds());
    if(mResponse.getMessageIds().size() == 1)
        mResponse.setMessageId(mResponse.getMessageIds().at(0));
    else
        mResponse.setMessageId(0);
    return commandErrorFromBlazeError(rc);
}

// static factory method impl
DEFINE_SENDMESSAGE_CREATE()

////////////////////////////////////////////////////////
class FetchMessagesCommand : public FetchMessagesCommandStub
{
public:
    FetchMessagesCommand(Message* message, FetchMessageRequest* request, MessagingSlaveImpl* componentImpl)
        : FetchMessagesCommandStub(message, request), mComponent(componentImpl)
    { }

    /* Private methods *******************************************************************************/
private:
    FetchMessagesCommandStub::Errors execute() override;

private:
    // Not owned memory.
    MessagingSlaveImpl* mComponent;
};

FetchMessagesCommandStub::Errors FetchMessagesCommand::execute()
{
    uint32_t count = 0;
    BlazeRpcError rc = mComponent->fetchMessages(UserSession::getCurrentUserSessionId(), mRequest, &count, getMsgNum());
    mResponse.setCount(count);
    return commandErrorFromBlazeError(rc);
}

// static factory method impl
DEFINE_FETCHMESSAGES_CREATE()

////////////////////////////////////////////////////////
class GetMessagesCommand : public GetMessagesCommandStub
{
public:
    GetMessagesCommand(Message* message, FetchMessageRequest* request, MessagingSlaveImpl* componentImpl)
        : GetMessagesCommandStub(message, request), mComponent(componentImpl)
    { }

    /* Private methods *******************************************************************************/
private:
    GetMessagesCommandStub::Errors execute() override;

private:
    // Not owned memory.
    MessagingSlaveImpl* mComponent;
};

GetMessagesCommandStub::Errors GetMessagesCommand::execute()
{
    BlazeRpcError rc = mComponent->getMessages(UserSession::getCurrentUserSessionId(), mRequest, &mResponse.getMessages());
    return commandErrorFromBlazeError(rc);
}
// static factory method impl
DEFINE_GETMESSAGES_CREATE()

////////////////////////////////////////////////////////
class PurgeMessagesCommand : public PurgeMessagesCommandStub
{
public:
    PurgeMessagesCommand(Message* message, PurgeMessageRequest* request, MessagingSlaveImpl* componentImpl)
        : PurgeMessagesCommandStub(message, request), mComponent(componentImpl)
    { }

    /* Private methods *******************************************************************************/
private:
    PurgeMessagesCommandStub::Errors execute() override;

private:
    // Not owned memory.
    MessagingSlaveImpl* mComponent;
};

PurgeMessagesCommandStub::Errors PurgeMessagesCommand::execute()
{
    uint32_t count = 0;
    BlazeRpcError rc = mComponent->purgeMessages(UserSession::getCurrentUserSessionId(), mRequest, &count);
    mResponse.setCount(count);
    return commandErrorFromBlazeError(rc);
}

// static factory method impl
DEFINE_PURGEMESSAGES_CREATE()

////////////////////////////////////////////////////////
class TouchMessagesCommand : public TouchMessagesCommandStub
{
public:
    TouchMessagesCommand(Message* message, TouchMessageRequest* request, MessagingSlaveImpl* componentImpl)
        : TouchMessagesCommandStub(message, request), mComponent(componentImpl)
    { }

    /* Private methods *******************************************************************************/
private:
    TouchMessagesCommandStub::Errors execute() override;

private:
    // Not owned memory.
    MessagingSlaveImpl* mComponent;
};

TouchMessagesCommandStub::Errors TouchMessagesCommand::execute()
{
    uint32_t count = 0;
    BlazeRpcError rc = mComponent->touchMessages(UserSession::getCurrentUserSessionId(), mRequest, &count);
    mResponse.setCount(count);
    return commandErrorFromBlazeError(rc);
}

// static factory method impl
DEFINE_TOUCHMESSAGES_CREATE()

////////////////////////////////////////////////////////
class SendSourceMessageCommand : public SendSourceMessageCommandStub
{
public:
    SendSourceMessageCommand(Message* message, SendSourceMessageRequest* request, MessagingSlaveImpl* componentImpl)
        : SendSourceMessageCommandStub(message, request), mComponent(componentImpl)
    { }

    /* Private methods *******************************************************************************/
private:
    SendSourceMessageCommandStub::Errors execute() override;
    
private:
    // Not owned memory.
    MessagingSlaveImpl* mComponent;
};

SendSourceMessageCommandStub::Errors SendSourceMessageCommand::execute()
{          
    BlazeRpcError rc = mComponent->sendMessage(mRequest.getSource(), mRequest.getPayload(), &mResponse.getMessageIds());
    if(mResponse.getMessageIds().size() == 1)
        mResponse.setMessageId(mResponse.getMessageIds().at(0));
    else
        mResponse.setMessageId(0);
    return commandErrorFromBlazeError(rc);
}

// static factory method impl
DEFINE_SENDSOURCEMESSAGE_CREATE()

////////////////////////////////////////////////////////
class SendGlobalMessageCommand : public SendGlobalMessageCommandStub
{
public:
    SendGlobalMessageCommand(Message* message, SendGlobalMessageRequest* request, MessagingSlaveImpl* componentImpl)
        : SendGlobalMessageCommandStub(message, request), mComponent(componentImpl)
    { }

    /* Private methods *******************************************************************************/
private:
    SendGlobalMessageCommandStub::Errors execute() override;
    
private:
    // Not owned memory.
    MessagingSlaveImpl* mComponent;
};

SendGlobalMessageCommandStub::Errors SendGlobalMessageCommand::execute()
{
    EA::TDF::ObjectId source;
    if (gCurrentUserSession)
        source = EA::TDF::ObjectId(ENTITY_TYPE_USER, gCurrentUserSession->getBlazeId());
    BlazeRpcError rc = mComponent->sendGlobalMessage(source, mRequest, &mResponse.getMessageIds());
    
    return commandErrorFromBlazeError(rc);
}

// static factory method impl
DEFINE_SENDGLOBALMESSAGE_CREATE()

} // Messaging
} // Blaze
