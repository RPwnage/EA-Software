/////////////////////////////////////////////////////////////////////////////
// ClientMessageAreaManager.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CLIENTMESSAGEAREAMANAGER_H
#define CLIENTMESSAGEAREAMANAGER_H

#include <QObject>
#include "MessageAreaViewControllerBase.h"
#include "services/plugin/PluginAPI.h"

class QWidget;

namespace Origin
{
namespace Client
{
/// \brief Manages messages that will be displayed in the client message area. (Just under titlebar)
class ORIGIN_PLUGIN_API ClientMessageAreaManager : public QObject
{
	Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent The parent of the ClientMessageAreaManager. Is the widget the message
    /// views will be displayed in.
	ClientMessageAreaManager(QWidget* parent);
	~ClientMessageAreaManager();

    /// \brief Sets the initial list of what messages should show and in what order.
    void init();

    /// \brief Removes all messages of message type from list. Calls removeCurrentMessage if it's the current message being displayed.
    void removeMessage(const MessageAreaViewControllerBase::MessageType& messageType);


signals:
    /// \brief Emitted when the Go Online button is pressed. (OfflineMessageAreaViewController)
    void goOnline();

    /// \brief Emitted when the subscription button is pressed. (SubscriptionMessageAreaViewController)
    void subscribe();

    /// \brief Emitted when the a message asks the client to show the account settings page.
    void showAccountSettings();

    /// \brief Emitted when the message area should be shown.
    void changeMessageAreaVisibility(const bool& showMessageArea);


protected slots:
    /// \brief Removes the current message from the list and shows the next message.
    void removeCurrentMessage();

    /// \brief If Origin goes offline, it creates and shows the Offline Message. (OfflineMessageAreaViewController)
    void onConnectionChanged(bool isOnline);

    /// \brief If a user's subscription changes we might show a expiring message (SubscriptionMessageAreaViewController)
    void onSubscriptionInfoChanged();

    /// \brief When we get the security response we query if we should should show the set security QA.
    void onSecurityResponseReceived();
    
    /// \brief When we get the email verification response we query if we should should show the message.
    void onEmailVerificationResponseReceived();


private:
    /// \brief Creates the view controller for the corresponding message type.
    /// \param messageType The message type that is to be added.
    /// \return The view controller of the message that was created
    MessageAreaViewControllerBase* pushMessage(const MessageAreaViewControllerBase::MessageType& messageType);

    /// \brief Adds the view controller message to the list of messages to show.
    /// \param message The message that will be added to the message list.
    void pushMessage(MessageAreaViewControllerBase* message);

    /// \brief Removes message from list. Calls removeCurrentMessage if it's the current message being displayed.
    void removeMessage(MessageAreaViewControllerBase* message);

    /// \brief Adds the object to the message list. If the message was pushed to the front, it shows the message right away.
    /// If it's pushed to the back, it's put into the "queue" of messages to be displayed.
    enum PushTo { Front, Back };
    void addToListAndLayout(MessageAreaViewControllerBase* message, const PushTo& push);

    /// \brief Checks to see if the list has a instance of the message type. Useful for if we want to only have one message
    /// of a particular type.
    QList<MessageAreaViewControllerBase*> instancesOfMessageType(const MessageAreaViewControllerBase::MessageType& messageType);

    QList<MessageAreaViewControllerBase*> mMessageAreaViewControllerList;
    QWidget* mCurrentMessageView;
    QWidget* mMessageParentContainer;
};
}
}
#endif