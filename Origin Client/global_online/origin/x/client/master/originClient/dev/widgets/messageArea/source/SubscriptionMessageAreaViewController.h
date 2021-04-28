/////////////////////////////////////////////////////////////////////////////
// SubscriptionMessageAreaViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef SUBSCRIPTIONMESSAGEAREAHANDLER_H
#define SUBSCRIPTIONMESSAGEAREAHANDLER_H

#include "MessageAreaViewControllerBase.h"

namespace Origin
{
namespace Client
{
class ClientMessageAreaView;
/// \brief Message that lets the user know there is an issue with their subscription.
class SubscriptionMessageAreaViewController : public MessageAreaViewControllerBase
{
    Q_OBJECT

public:
    SubscriptionMessageAreaViewController(QObject* parent);
    ~SubscriptionMessageAreaViewController();
    QWidget* view();

    /// \brief Checks to see if a subscription message can be shown. This is 
    /// dependent on the subscription status and the user's status.
    /// (e.g. user saw this message recently, user is subscriber, user is online...)
    static bool okToShow();

public slots:
    /// \brief If the subscription information changes, it shows the Subscription 
    /// Message if the user's subscription is about to expire.
    void onSubscriptionInfoChanged();

signals:
    /// \brief Emitted when the Subscribe button is pressed.
    void subscribe();

    /// \brief Emitted when the Dismiss button is pressed.
    void dismiss();


protected:
    /// \brief Updates the timer.
    void timerEvent(QTimerEvent *event);

protected slots:
    /// \brief Called when the message first button is clicked. Saves when user saw message.
    /// Emits subscribe.
    void onSubscribeClicked();

    /// \brief Called when the message disable button is clicked. Saves when user saw message.
    /// Emits dismiss.
    void onDismissMessage();


private:
    void stopSubscriptionTimer();
    void updateTitle();
    static bool isTimeToShow(const int& lastJulianDay, const int& daysToWait);

    ClientMessageAreaView* mClientMessageAreaView;
    int mSubscriptionTimer;
};
}
}
#endif