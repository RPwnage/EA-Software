/////////////////////////////////////////////////////////////////////////////
// SubscriptionMessageAreaViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "SubscriptionMessageAreaViewController.h"
#include "ClientMessageAreaView.h"
#include "engine/login/LoginController.h"
#include "engine/subscription/SubscriptionManager.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "utilities/LocalizedContentString.h"
#include <QPushButton>
#include "LocalizedDateFormatter.h"
#include <serverEnumStrings.h>
#include "TelemetryAPIDLL.h"

namespace
{
    const int SECONDS_IN_DAY = 86400;
    const int SECONDS_IN_HOUR = 3600;
    const int SUBSCRIPTIONMESSAGE_SUPPRESS_DAYS = 1;
}

namespace Origin
{
namespace Client
{

SubscriptionMessageAreaViewController::SubscriptionMessageAreaViewController(QObject* parent)
: MessageAreaViewControllerBase(Subscription, parent)
, mClientMessageAreaView(NULL)
, mSubscriptionTimer(0)
{
    mClientMessageAreaView = new ClientMessageAreaView();
}


SubscriptionMessageAreaViewController::~SubscriptionMessageAreaViewController()
{
    if(mClientMessageAreaView)
    {
        delete mClientMessageAreaView;
        mClientMessageAreaView = NULL;
    }
}


void SubscriptionMessageAreaViewController::onSubscriptionInfoChanged()
{
    ORIGIN_VERIFY_DISCONNECT(mClientMessageAreaView->firstButton(), SIGNAL(clicked()), 0, 0);
    ORIGIN_VERIFY_DISCONNECT(mClientMessageAreaView->secondButton(), SIGNAL(clicked()), 0, 0);

    updateTitle();

    mClientMessageAreaView->secondButton()->setText(tr("ebisu_client_dismiss"));
    // Both buttons dismiss the message
    ORIGIN_VERIFY_CONNECT(mClientMessageAreaView->firstButton(), SIGNAL(clicked()), this, SLOT(onDismissMessage()));
    ORIGIN_VERIFY_CONNECT(mClientMessageAreaView->secondButton(), SIGNAL(clicked()), this, SLOT(onDismissMessage()));

    switch(Engine::Subscription::SubscriptionManager::instance()->status())
    {
        // TODO: Handle the other status values.

    case server::SUBSCRIPTIONSTATUS_EXPIRED:
    case server::SUBSCRIPTIONSTATUS_DISABLED:
        mClientMessageAreaView->setText(tr("Subs_Library_Banner_Expires_SubHeader_Payment"));
        mClientMessageAreaView->firstButton()->setText(tr("Subs_Library_Banner_Expires_Payment_Action"));
        ORIGIN_VERIFY_CONNECT(mClientMessageAreaView->firstButton(), SIGNAL(clicked()), this, SLOT(onSubscribeClicked()));
        break;

    default:
        mClientMessageAreaView->setText(tr("Subs_Library_Banner_Status_SubHeader_Generic"));
        mClientMessageAreaView->firstButton()->setText(tr("Subs_Library_Banner_Expires_Subscribe_Action"));
        ORIGIN_VERIFY_CONNECT(mClientMessageAreaView->firstButton(), SIGNAL(clicked()), this, SLOT(onSubscribeClicked()));
        break;
    }
}


void SubscriptionMessageAreaViewController::onSubscribeClicked()
{
    switch(Engine::Subscription::SubscriptionManager::instance()->status())
    {
    case server::SUBSCRIPTIONSTATUS_EXPIRED:
    case server::SUBSCRIPTIONSTATUS_DISABLED:
        GetTelemetryInterface()->Metric_SUBSCRIPTION_UPSELL(QString("message-area: expired").toStdString().c_str(), Engine::Subscription::SubscriptionManager::instance()->isActive());
        break;

    default:
        GetTelemetryInterface()->Metric_SUBSCRIPTION_UPSELL(QString("message-area: other").toStdString().c_str(), Engine::Subscription::SubscriptionManager::instance()->isActive());
        break;
    }

    emit subscribe();
}


void SubscriptionMessageAreaViewController::onDismissMessage()
{
    stopSubscriptionTimer();
    Services::writeSetting(Services::SETTING_SUBSCRIPTION_ERROR_MESSAGE_LASTSHOWN, QDate::currentDate().toJulianDay(), Services::Session::SessionService::currentSession());
    emit dismiss();
}


bool SubscriptionMessageAreaViewController::okToShow()
{
    Engine::Subscription::SubscriptionManager* subsManager = Engine::Subscription::SubscriptionManager::instance();

    if(Services::Connection::ConnectionStatesService::isUserOnline(Engine::LoginController::instance()->currentUser()->getSession()) == false)
    {
        ORIGIN_LOG_ACTION << "Subscription Message Area: Not showing - User offline";
        return false;
    }
    
    if(subsManager->timeRemaining() < 0)
    {
        ORIGIN_LOG_ACTION << "Subscription Message Area: Not showing - Time remaining not valid";
        return false;
    }
    
    if (subsManager->timeRemaining() > 432000)
    {
        ORIGIN_LOG_ACTION << "Subscription Message Area: Not showing - Time remaining more than 5 days";
        return false;
    }

    const server::SubscriptionStatusT subStatus = subsManager->status();
    switch(subStatus)
    {
    case server::SUBSCRIPTIONSTATUS_EXPIRED:
    case server::SUBSCRIPTIONSTATUS_DISABLED:
        break;
    default:
        ORIGIN_LOG_ACTION << "Subscription Message Area: Not showing - status = " + QString(server::SubscriptionStatusStrings[subStatus]);
        return false;
    }

    if(isTimeToShow(Services::readSetting(Services::SETTING_SUBSCRIPTION_ERROR_MESSAGE_LASTSHOWN, Services::Session::SessionService::currentSession()), SUBSCRIPTIONMESSAGE_SUPPRESS_DAYS) == false)
    {
        ORIGIN_LOG_ACTION << "Subscription Message Area: Not showing - Message shown recently";
        return false;
    }

    return true;
}


void SubscriptionMessageAreaViewController::stopSubscriptionTimer()
{
    if(mSubscriptionTimer)
    {
        killTimer(mSubscriptionTimer);
        mSubscriptionTimer = 0;
    }
}


void SubscriptionMessageAreaViewController::updateTitle()
{
    Engine::Subscription::SubscriptionManager* subsManager = Engine::Subscription::SubscriptionManager::instance();
    const int secondsRemaining = subsManager->timeRemaining();
    // Stop the timer because we might want to update it with a new interval
    stopSubscriptionTimer();

    QString interval = "";
    int suggestedRefreshTimer = 0;
    const int daysLeft = (secondsRemaining / SECONDS_IN_DAY);
    const int hoursLeft = (secondsRemaining / SECONDS_IN_HOUR);

    // Day(s):
    if(daysLeft > 0)
        suggestedRefreshTimer = SECONDS_IN_DAY * 1000;
    // Hour(s):
    else if(hoursLeft > 0)
        suggestedRefreshTimer = SECONDS_IN_HOUR * 1000;
    else
        suggestedRefreshTimer = 30000;

    switch(subsManager->status())
    {
    case server::SUBSCRIPTIONSTATUS_EXPIRED:
    case server::SUBSCRIPTIONSTATUS_DISABLED:
        {
            mClientMessageAreaView->setTitle(tr("Subs_Library_Banner_Expired_Header"));
        }
        break;
    default:
        {
            const QString interval = LocalizedDateFormatter().formatInterval(secondsRemaining, LocalizedDateFormatter::Minutes, LocalizedDateFormatter::Days, true);
            mClientMessageAreaView->setTitle(tr("Subs_Library_Banner_Expires_Header_Days").arg(interval));
            mSubscriptionTimer = startTimer(suggestedRefreshTimer);
        }
        break;
    }
}


void SubscriptionMessageAreaViewController::timerEvent(QTimerEvent* event)
{
    if(event->timerId() == mSubscriptionTimer)
    {
        updateTitle();
    }
}


QWidget* SubscriptionMessageAreaViewController::view()
{
    return mClientMessageAreaView;
}

bool SubscriptionMessageAreaViewController::isTimeToShow(const int& lastJulianDay, const int& daysToWait)
{
    QDate lastShowedPromo;
    if(lastJulianDay)
    {
        lastShowedPromo = QDate::fromJulianDay(lastJulianDay); 
    }

    return (!lastShowedPromo.isValid() || lastShowedPromo.daysTo(QDate::currentDate()) >= daysToWait);
}

}
}
