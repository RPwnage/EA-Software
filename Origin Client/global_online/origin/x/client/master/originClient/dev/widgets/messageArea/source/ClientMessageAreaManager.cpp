/////////////////////////////////////////////////////////////////////////////
// ClientMessageAreaManager.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ClientMessageAreaManager.h"
#include "MessageAreaViewControllerBase.h"
#include "OfflineMessageAreaViewController.h"
#include "SubscriptionMessageAreaViewController.h"
#include "SecurityQuestionMessageAreaViewController.h"
#include "EmailVerificationMessageAreaViewController.h"
#include "engine/login/LoginController.h"
#include "engine/subscription/SubscriptionManager.h"
#include "Services/debug/DebugService.h"
#include "TelemetryAPIDLL.h"
#include <QWidget>
#include <QLayout>

namespace Origin
{
namespace Client
{
ClientMessageAreaManager::ClientMessageAreaManager(QWidget* parent)
: QObject(parent)
, mCurrentMessageView(NULL)
, mMessageParentContainer(parent)
{

}


ClientMessageAreaManager::~ClientMessageAreaManager()
{

}


void ClientMessageAreaManager::init()
{
    Engine::UserRef user = Engine::LoginController::instance()->currentUser();
    if(user)
    {
        onConnectionChanged(Services::Connection::ConnectionStatesService::isUserOnline(user->getSession()));
        ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)), 
            this, SLOT(onConnectionChanged(bool)));

        onSubscriptionInfoChanged();
        ORIGIN_VERIFY_CONNECT(Engine::Subscription::SubscriptionManager::instance(), SIGNAL(updated()), this, SLOT(onSubscriptionInfoChanged()));

        SecurityQuestionMessageAreaViewController* securityQA = new SecurityQuestionMessageAreaViewController(this);
        ORIGIN_VERIFY_CONNECT(securityQA, SIGNAL(responseReceived()), this, SLOT(onSecurityResponseReceived()));

        EmailVerificationMessageAreaViewController* emailVerification = new EmailVerificationMessageAreaViewController(this);
        ORIGIN_VERIFY_CONNECT(emailVerification, SIGNAL(responseReceived()), this, SLOT(onEmailVerificationResponseReceived()));
    }
}


void ClientMessageAreaManager::onConnectionChanged(bool isOnline)
{
    QList<MessageAreaViewControllerBase*> instanceOfMessageType = instancesOfMessageType(MessageAreaViewControllerBase::Offline);
    if(isOnline == false)
    {
        // Only allow one instance of the offline message
        if(instanceOfMessageType.count() == 0)
        {
            OfflineMessageAreaViewController* message = dynamic_cast<OfflineMessageAreaViewController*>(pushMessage(MessageAreaViewControllerBase::Offline));
            if(message)
                message->onConnectionChanged(isOnline);
        }
    }
    else
    {
        if(instanceOfMessageType.count())
            removeMessage(instanceOfMessageType[0]);
    }
}


void ClientMessageAreaManager::onSubscriptionInfoChanged()
{
    QList<MessageAreaViewControllerBase*> instanceOfMessageType = instancesOfMessageType(MessageAreaViewControllerBase::Subscription);
    if(SubscriptionMessageAreaViewController::okToShow())
    {
        SubscriptionMessageAreaViewController* message = NULL;

        // Will we need to show multiple messages of subscription?
        // For now let's only allow one.
        if(instanceOfMessageType.count() == 0)
        {
            message = dynamic_cast<SubscriptionMessageAreaViewController*>(pushMessage(MessageAreaViewControllerBase::Subscription));
        }
        else
        {
            message = dynamic_cast<SubscriptionMessageAreaViewController*>(instanceOfMessageType[0]);
        }

        if(message)
            message->onSubscriptionInfoChanged();
    }
    else
    {
        if(instanceOfMessageType.count())
            removeMessage(instanceOfMessageType[0]);
    }
}


void ClientMessageAreaManager::onSecurityResponseReceived()
{
    QList<MessageAreaViewControllerBase*> instanceOfMessageType = instancesOfMessageType(MessageAreaViewControllerBase::SecurityQuestion);
    SecurityQuestionMessageAreaViewController* sender = dynamic_cast<SecurityQuestionMessageAreaViewController*>(this->sender());
    if(sender && sender->okToShowMessage())
    {
        // Only allow one instance of the security message
        if(instanceOfMessageType.count() == 0)
            pushMessage(sender);
    }
    else
    {
        if(instanceOfMessageType.count())
            removeMessage(instanceOfMessageType[0]);
        // If the security question wasn't pushed to the message list 
        // removeMessage won't delete it. We have to manual delete it here.
        else if(sender)
            sender->deleteLater();
    }
}


void ClientMessageAreaManager::onEmailVerificationResponseReceived()
{
    QList<MessageAreaViewControllerBase*> instanceOfMessageType = instancesOfMessageType(MessageAreaViewControllerBase::EmailVerification);
    EmailVerificationMessageAreaViewController* sender = dynamic_cast<EmailVerificationMessageAreaViewController*>(this->sender());
    if(sender && sender->okToShowMessage())
    {
        // Only allow one instance of the security message
        if(instanceOfMessageType.count() == 0)
            pushMessage(sender);
    }
    else
    {
        if(instanceOfMessageType.count())
            removeMessage(instanceOfMessageType[0]);
        // If the message wasn't pushed to the message list 
        // removeMessage won't delete it. We have to manual delete it here.
        else if(sender)
            sender->deleteLater();
    }
}


MessageAreaViewControllerBase* ClientMessageAreaManager::pushMessage(const MessageAreaViewControllerBase::MessageType& messageType)
{
    MessageAreaViewControllerBase* message = NULL;

    switch(messageType)
    {
    case MessageAreaViewControllerBase::Offline:
        message = new OfflineMessageAreaViewController(this);
        break;

    case MessageAreaViewControllerBase::SecurityQuestion:
        message = new SecurityQuestionMessageAreaViewController(this);
        break;

    case MessageAreaViewControllerBase::EmailVerification:
        message = new EmailVerificationMessageAreaViewController(this);
        break;

    case MessageAreaViewControllerBase::Subscription:
        message = new SubscriptionMessageAreaViewController(this);
        break;

    default:
        break;
    }

    pushMessage(message);

    return message;
}


void ClientMessageAreaManager::pushMessage(MessageAreaViewControllerBase* message)
{
    ORIGIN_ASSERT(message);
    switch(message->messageType())
    {
    case MessageAreaViewControllerBase::Offline:
        {
            OfflineMessageAreaViewController* offlineMessage = dynamic_cast<OfflineMessageAreaViewController*>(message);
            ORIGIN_VERIFY_CONNECT(offlineMessage, SIGNAL(goOnline()), this, SIGNAL(goOnline()));
            // Offline always takes priority - so always put it on top
            addToListAndLayout(message, Front);
        }
        break;

    case MessageAreaViewControllerBase::SecurityQuestion:
        {
            SecurityQuestionMessageAreaViewController* securityMessage = dynamic_cast<SecurityQuestionMessageAreaViewController*>(message);
            ORIGIN_VERIFY_CONNECT(securityMessage, SIGNAL(showAccountSettings()), this, SIGNAL(showAccountSettings()));
            ORIGIN_VERIFY_CONNECT(securityMessage, SIGNAL(showAccountSettings()), this, SLOT(removeCurrentMessage()));
            ORIGIN_VERIFY_CONNECT(securityMessage, SIGNAL(dismiss()), this, SLOT(removeCurrentMessage()));
            addToListAndLayout(securityMessage, Back);

            GetTelemetryInterface()->Metric_SECURITY_QUESTION_SHOW();
        }
        break;

    case MessageAreaViewControllerBase::Subscription:
        {
            SubscriptionMessageAreaViewController* subscriptionMessage = dynamic_cast<SubscriptionMessageAreaViewController*>(message);
            ORIGIN_VERIFY_CONNECT(subscriptionMessage, SIGNAL(subscribe()), this, SIGNAL(subscribe()));
            ORIGIN_VERIFY_CONNECT(subscriptionMessage, SIGNAL(dismiss()), this, SLOT(removeCurrentMessage()));
            addToListAndLayout(subscriptionMessage, Back);
            message = subscriptionMessage;
        }
        break;

    case MessageAreaViewControllerBase::EmailVerification:
        {
            EmailVerificationMessageAreaViewController* emailMessage = dynamic_cast<EmailVerificationMessageAreaViewController*>(message);
            ORIGIN_VERIFY_CONNECT(emailMessage, SIGNAL(showAccountSettings()), this, SIGNAL(showAccountSettings()));
            ORIGIN_VERIFY_CONNECT(emailMessage, SIGNAL(showAccountSettings()), this, SLOT(removeCurrentMessage()));
            ORIGIN_VERIFY_CONNECT(emailMessage, SIGNAL(dismiss()), this, SLOT(removeCurrentMessage()));
            addToListAndLayout(emailMessage, Back);
        }
        break;

    default:
        break;
    }

    emit changeMessageAreaVisibility(true);
}


void ClientMessageAreaManager::removeMessage(const MessageAreaViewControllerBase::MessageType& messageType)
{
    MessageAreaViewControllerBase* message = NULL;
    for(int iMessage = 0; iMessage < mMessageAreaViewControllerList.count(); iMessage++)
    {
        message = mMessageAreaViewControllerList[iMessage];
        // Is the message the same type?
        if(message && message->messageType() == messageType)
        {
            removeMessage(message);
        }
    }
}


void ClientMessageAreaManager::removeMessage(MessageAreaViewControllerBase* message)
{
    if(message->view() == mCurrentMessageView)
    {
        removeCurrentMessage();
    }
    else
    {
        mMessageAreaViewControllerList.removeOne(message);
        message->deleteLater();
    }
}


void ClientMessageAreaManager::addToListAndLayout(MessageAreaViewControllerBase* message, const PushTo& push)
{
    if(push == Front)
    {
        // Push to the front of the list.
        mMessageAreaViewControllerList.push_front(message);

        // Remove the current view if there is one.
        if(mCurrentMessageView)
        {
            mMessageParentContainer->layout()->removeWidget(mCurrentMessageView);
            mCurrentMessageView->setParent(NULL);
        }

        // Display the new message.
        mCurrentMessageView = message->view();
        mMessageParentContainer->layout()->addWidget(mCurrentMessageView);
    }
    else
    {
        // Push to the back of the list
        mMessageAreaViewControllerList.push_back(message);
        // If there isn't a current view, display this message.
        if(mCurrentMessageView == NULL)
        {
            mCurrentMessageView = message->view();
            mMessageParentContainer->layout()->addWidget(mCurrentMessageView);
        }
    }
}


QList<MessageAreaViewControllerBase*> ClientMessageAreaManager::instancesOfMessageType(const MessageAreaViewControllerBase::MessageType& messageType)
{
    QList<MessageAreaViewControllerBase*> instanceOfMessageType;
    MessageAreaViewControllerBase* message = NULL;
    for(int iMessage = 0; iMessage < mMessageAreaViewControllerList.count(); iMessage++)
    {
        message = mMessageAreaViewControllerList[iMessage];
        // Is the message the same type?
        if(message && message->messageType() == messageType)
        {
            instanceOfMessageType.append(message);
        }
    }
    return instanceOfMessageType;
}


void ClientMessageAreaManager::removeCurrentMessage()
{
    if(mMessageAreaViewControllerList.empty() == false)
    {
        MessageAreaViewControllerBase* message = mMessageAreaViewControllerList[0];
        mMessageAreaViewControllerList.removeAt(0);
        mMessageParentContainer->layout()->removeWidget(message->view());
        message->deleteLater();
        mCurrentMessageView = NULL;
    }
    
    // If the list is still not empty - show the next message
    if(mMessageAreaViewControllerList.empty() == false)
    {
        mCurrentMessageView = mMessageAreaViewControllerList[0]->view();
        mMessageParentContainer->layout()->addWidget(mCurrentMessageView);
    }
    else
    {
        emit changeMessageAreaVisibility(false);
    }
}

}
}
