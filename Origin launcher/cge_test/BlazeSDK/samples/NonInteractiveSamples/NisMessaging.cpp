
#include "NisMessaging.h"

#include "EASTL/string.h"
#include "EAStdC/EAString.h"

#include "BlazeSDK/messaging/messagingapi.h"
#include "BlazeSDK/util/utilapi.h"

using namespace Blaze::UserManager;
using namespace Blaze::Messaging;

namespace NonInteractiveSamples
{

// Custom message attributes
enum MessageAttrEnum
{
    MSG_ATTR_CUSTOM_ATTACHMENT = Blaze::Messaging::CLEAN_ATTR_MIN,
};

// Toggle these flags to experiment with the functionality
static bool gIsPersistentMode   = true;
static bool gIsAutoPurge        = true;

NisMessaging::NisMessaging(NisBase &nisBase)
    : NisSample(nisBase)
{
}

NisMessaging::~NisMessaging()
{
}

void NisMessaging::onCreateApis()
{
}

void NisMessaging::onCleanup()
{
}

void NisMessaging::onRun()
{
    getUiDriver().addDiagnosticFunction();

    // Register to receive messages
    getUiDriver().addDiagnosticCallFunc(getBlazeHub()->getMessagingAPI(getBlazeHub()->getPrimaryLocalUserIndex())->registerCallback(MessagingAPI::UserMessageCb(this, &NisMessaging::UserMessageCb)));

    if (gIsPersistentMode)
    {
        // Lets fetch any existing persistent messages
        //FetchMessageRequest request;
        FetchMessageParameters request;
        MessagingAPI::FetchMessagesCb fetchCb(this, &NisMessaging::FetchMessagesCb); 
        getUiDriver().addDiagnosticCallFunc(getBlazeHub()->getMessagingAPI(getBlazeHub()->getPrimaryLocalUserIndex())->fetchMessages(request, fetchCb));
    }

    // We're going to send a message to ourself. So sad!
    // The normal process would be to lookup the User object from the recipient persona name, so we'll demonstrate that.
    const char * recipientPersonaName = getBlazeHub()->getUserManager()->getLocalUser(getBlazeHub()->getPrimaryLocalUserIndex())->getName();
    Blaze::UserManager::UserManager::LookupUserCb cb(this, &NisMessaging::LookupUserCb);
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "lookupUserByName %s", recipientPersonaName);
    getBlazeHub()->getUserManager()->lookupUserByName(recipientPersonaName, cb);
}

//------------------------------------------------------------------------------
// This callback will fire whenever we receive a message (either because we registered to receive messages, 
// or we did an explicit fetchMessages request
void NisMessaging::UserMessageCb(const ServerMessage* msg, const Blaze::UserManager::User* user)
{
    getUiDriver().addDiagnosticCallback();

    // Extract the various components of the message

    // Sender name
    const char8_t* msgSender = msg->getSourceIdent().getName();

    // Timestamp
    int64_t msgTime = msg->getTimestamp();
    char8_t strTime[1024] = "";
    EA::StdC::I64toa(msgTime, strTime, 10);

    // The attributes
    const MessageAttrMap& attrMap = msg->getPayload().getAttrMap();
    MessageAttrMap::const_iterator attrIt;

    // Subject
    const char8_t* msgSubject = "";
    attrIt = attrMap.find(MSG_ATTR_SUBJECT);
    if (attrIt != attrMap.end())
    {
        msgSubject = (*attrIt).second.c_str();
    }

    // Body
    const char8_t* msgBody = "";
    attrIt = attrMap.find(MSG_ATTR_BODY);
    if (attrIt != attrMap.end())
    {
        msgBody = (*attrIt).second.c_str();
    }

    // Our custom attribute (an attachment)
    const char8_t* msgAttch = "";
    attrIt = attrMap.find(MSG_ATTR_CUSTOM_ATTACHMENT);
    if (attrIt != attrMap.end())
    {
        msgAttch = (*attrIt).second.c_str();
    }

    // Here's how to distinguish between persistent and non-persistent messages
    const bool isPersistentMessage = msg->getPayload().getFlags().getPersistent();

    // Display the message
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  Message id:  %d", msg->getMessageId());
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  Sender:      %s", msgSender);
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  Timestamp:   %s", strTime);
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  Subject:     %s", msgSubject);
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  Body:        %s", msgBody);
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  Attachment:  %s", msgAttch);
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  Persistent?: %d", isPersistentMessage);

    // If gIsAutoPurge is set, delete it 
    // Non-persistent messages don't need to be deleted.
    if (isPersistentMessage && gIsAutoPurge)
    {
        PurgeMessageParameters request;

        // Tell Blaze to match via message Id
        // Without the next two lines we would delete all messages
        request.getFlags().setMatchId(); 
        request.setMessageId(msg->getMessageId());

        MessagingAPI::PurgeMessagesCb cb(this, &NisMessaging::PurgeMessagesCb);
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "purgeMessages - for id %d", msg->getMessageId());
        getBlazeHub()->getMessagingAPI(getBlazeHub()->getPrimaryLocalUserIndex())->purgeMessages(request, cb);
    }
}

//------------------------------------------------------------------------------
// The result of purgeMessages
void NisMessaging::PurgeMessagesCb(const Blaze::Messaging::PurgeMessageResponse* response, Blaze::BlazeError error, Blaze::JobId id)
{
    getUiDriver().addDiagnosticCallback();

    if (error == Blaze::ERR_OK)
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  %d messages found.", response->getCount());
        done();
    }
    else
    {
        reportBlazeError(error);
    }
}

//------------------------------------------------------------------------------
// The result of fetchMessages
void NisMessaging::FetchMessagesCb(const Blaze::Messaging::FetchMessageResponse* response, Blaze::BlazeError error, Blaze::JobId id)
{
    getUiDriver().addDiagnosticCallback();

    if (error == Blaze::ERR_OK)
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  %d messages found.", response->getCount());
    }
    else
    {
        if (error == Blaze::MESSAGING_ERR_MATCH_NOT_FOUND)
        {
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  No messages found.");
        }
        else
        {
            reportBlazeError(error);
        }
    }
}

//------------------------------------------------------------------------------
// The result of lookupUserByName. If successful, we'll send the user (in this case, ourself) a message
void NisMessaging::LookupUserCb(Blaze::BlazeError error, Blaze::JobId id, const Blaze::UserManager::User* user)
{
    getUiDriver().addDiagnosticCallback();

    if (error == Blaze::ERR_OK)
    {
        SendMessageParameters message;

        // Message can be persistent or non-persistent.
        // Persistent messages must be deleted, like email
        // Non-persistent messages would be suitable for IM
        if (gIsPersistentMode)
        {
            message.getFlags().setPersistent();
        }
    
        // In order to send a message to ourself, we are required to set the echo flag
        message.getFlags().setEcho();

        // setFilterProfanity will cause subject and body to be filtered for profanity
        // Note: requires Util API
        message.getFlags().setFilterProfanity(); 
        
        // Set recipient
        message.setTargetUser(user);

        // Now set the message subject and body
        message.getAttrMap().insert(eastl::make_pair(MSG_ATTR_SUBJECT, (const char *)"Some message subject")); //casting to avoid compiler error from EASTL
        message.getAttrMap().insert(eastl::make_pair(MSG_ATTR_BODY, (const char *)"Some message body"));

        // We can also add custom fields to the message. Let's add an attachment
        message.getAttrMap().insert(eastl::make_pair(MSG_ATTR_CUSTOM_ATTACHMENT, (const char *)"Some message attachment"));
         
        // Now do the actual send
        MessagingAPI::SendMessageCb cb(this, &NisMessaging::SendMessageCb);
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "sendMessage to %s", user->getName());
        getBlazeHub()->getMessagingAPI(getBlazeHub()->getPrimaryLocalUserIndex())->sendMessage(message, cb);
    }
    else
    {
        reportBlazeError(error);
    }
}

//------------------------------------------------------------------------------
// The result of sendMessage
void NisMessaging::SendMessageCb(const Blaze::Messaging::SendMessageResponse* response, Blaze::BlazeError error, Blaze::JobId id)
{
    getUiDriver().addDiagnosticCallback();

    if (error == Blaze::ERR_OK)
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  Sent message id %u", response->getMessageId());
    }
    else
    {
        reportBlazeError(error);
    }
}

}

