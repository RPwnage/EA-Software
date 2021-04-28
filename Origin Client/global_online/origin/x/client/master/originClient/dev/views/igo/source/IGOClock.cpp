///////////////////////////////////////////////////////////////////////////////
// IGOClock.cpp
// 
// Created by Mike Dorval
// Copyright (c) 2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGOClock.h"
#include "ui_IGOClock.h"
#include "engine/content/ContentController.h"
#include "engine/cloudsaves/CloudDateFormat.h"
#include "services/platform/PlatformService.h"
#include "services/platform/TrustedClock.h"
#include <QPainter>
#include <QTime>
#include <qstyleoption.h>
#include "LocalizedDateFormatter.h"

namespace Origin
{
namespace Client
{

IGOClock::IGOClock(QWidget* parent)
: Origin::Engine::IGOQWidget(parent)
, ui(new Ui::IGOClock)
, mClockTimerId(0)
, mBatteryTimerId(0)
, mPlayTimeLeftTimerId(0)
, mTrialTimeRemaining(0)
, mTrialElapsedTimer(NULL)
{
    ui->setupUi(this);
    startClock();
    startPlayTimeLeft();
    startBatteryTimer();
    ORIGIN_VERIFY_CONNECT(
        Origin::Engine::IGOController::instance()->gameTracker(), SIGNAL(gameFocused(uint32_t)),
        this, SLOT(onGameFocused(const uint32_t&)));
    ORIGIN_VERIFY_CONNECT(Origin::Engine::IGOController::instance()->gameTracker(), SIGNAL(gameAdded(uint32_t)), this, SLOT(onGameAdded(const uint32_t&)));
    ORIGIN_VERIFY_CONNECT(Origin::Engine::IGOController::instance()->gameTracker(), SIGNAL(gameFreeTrialInfoUpdated(uint32_t, qint64, QElapsedTimer*)), this, SLOT(onGameFreeTrialInfoUpdated(const uint32_t&, const qint64&, QElapsedTimer*)));
}


IGOClock::~IGOClock()
{
    delete ui;
    stopTimer(mClockTimerId);
    stopTimer(mBatteryTimerId);
    stopTimer(mPlayTimeLeftTimerId);
    mTrialElapsedTimer = NULL;
}


void IGOClock::timerEvent(QTimerEvent* event)
{
#ifdef ORIGIN_MAC
    if (!isIGOVisible()) // tmp to help with performance for SimCity
        return;
#endif
    if (event->timerId() == mClockTimerId)
    {
        updateClock();
        // Piggy-backing on the clock timer
        updatePlaySession();
    }
    else if(event->timerId() == mBatteryTimerId)
    {
        updateBattery();
    }
    else if (event->timerId() == mPlayTimeLeftTimerId)
    {
        updatePlayTimeLeft();
    }
}


void IGOClock::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}


void IGOClock::startClock()
{
    mClockTimerId = startTimer(30000);
    updateClock();
}


void IGOClock::updateClock()
{
    const int initalWidth = width();
    ui->lblTime->setText(QTime::currentTime().toString(Qt::SystemLocaleShortDate));
    updateSize(initalWidth);
}


void IGOClock::startBatteryTimer()
{
    mBatteryTimerId = startTimer(5000);
    updateBattery();
}


void IGOClock::startPlayTimeLeft()
{
    mPlayTimeLeftTimerId = startTimer(500);
    updatePlayTimeLeft();
}


void IGOClock::stopTimer(int& timerId)
{
    if (timerId)
    {
        killTimer(timerId);
        timerId = 0;
    }
}


void IGOClock::updateBattery()
{
    Services::PlatformService::BatteryFlag batteryStatus = Services::PlatformService::NoBattery;
    int batteryPercent = 0;
    Origin::Services::PlatformService::powerStatus(batteryStatus, batteryPercent);

    switch (batteryStatus)
    {
    case Services::PlatformService::PluggedIn:
        ui->lblBattery->setProperty("style", "pluggedIn");
        ui->lblBattery->setStyle(QApplication::style());
        ui->lblBattery->setVisible(true);
        break;
    case Services::PlatformService::UsingBattery:
        if (batteryPercent <= 10) // 0 - 10%
            ui->lblBattery->setProperty("style", "1");
        else if (batteryPercent <= 32) // 11 - 32% : 25%
            ui->lblBattery->setProperty("style", "25");
        else if (batteryPercent <= 54) // 33 - 54% : 50%
            ui->lblBattery->setProperty("style", "50");
        else if (batteryPercent <= 76) // 55 - 76% : 75%
            ui->lblBattery->setProperty("style", "75");
        else if (batteryPercent <= 100) // 77%+ : 100
            ui->lblBattery->setProperty("style", "100");
        ui->lblBattery->setStyle(QApplication::style());
        ui->lblBattery->setVisible(true);
        break;
    case Services::PlatformService::NoBattery:
    default:
        stopTimer(mBatteryTimerId);
        ui->lblBattery->setVisible(false);
        break;
    }
    adjustSize();
}


void IGOClock::updatePlaySession()
{
    if (mEntitlement.isNull() || mEntitlement->localContent() == NULL)
        return;
    const int seconds = mEntitlement->localContent()->playSessionStartTime().secsTo(Services::TrustedClock::instance()->nowUTC());
    const int hour = seconds / 3600;
    const int minute = (seconds % 3600) / 60;
    const int initalWidth = width();
    ui->lblSession->setText(tr("ebisu_client_this_session_length").arg(hour).arg(minute));
    updateSize(initalWidth);
}


void IGOClock::updatePlayTimeLeft()
{
    const int initalWidth = width();
    if (mEntitlement.isNull() || mEntitlement->localContent() == NULL)
    {
        ui->lblPlayTimeLeft->setText("");
        return;
    }
    if ((mTrialTimeRemaining > 0) && mTrialElapsedTimer)
    {
        const qint64 secondsTilExpire = (mTrialTimeRemaining - mTrialElapsedTimer->elapsed()) * 0.001;
        if (secondsTilExpire > 0)
        {
            ui->lblPlayTimeLeft->setText(tr("ebisu_client_free_trial_ending_timer_message").arg(LocalizedDateFormatter().formatShortInterval_hs(secondsTilExpire)));
        }
        else
        {
            ui->lblPlayTimeLeft->setText(tr("ebisu_client_trial_expired"));
        }
    }
    else
    {
        ui->lblPlayTimeLeft->setText("");
    }
    updateSize(initalWidth);
}


void IGOClock::onGameFocused(const uint32_t& gameId)
{
    const Engine::IGOGameTracker::GameInfo info = Engine::IGOController::instance()->gameTracker()->gameInfo(gameId);
    if (info.isValid())
    {
        mEntitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(info.mBase.mProductID);
        updatePlaySession();
        updatePlayTimeLeft();
    }
}


void IGOClock::onGameAdded(const uint32_t& gameId)
{
    Origin::Engine::IGOGameTracker::GameInfo info = Origin::Engine::IGOController::instance()->gameTracker()->gameInfo(gameId);
    if (info.isValid())
    {
        mEntitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(info.mBase.mProductID);
#if 0 //TODO FIXUP_ORIGINX
        onGameFreeTrialInfoUpdated(gameId, info.mTimeRemaining, info.mElapsedTimer);
#endif
    }
    else
    {
        // Clear Free Trial info that may be left over from previously launched Free Trial
        onGameFreeTrialInfoUpdated(0, 0, 0);
    }
}


void IGOClock::onGameFreeTrialInfoUpdated(const uint32_t& gameId, const qint64& timeRemaining, QElapsedTimer* elapsedTimer)
{
    // TODO: the trial code needs to be updated - passing a timer around - not cool!
    mTrialTimeRemaining = timeRemaining;
    mTrialElapsedTimer = elapsedTimer;
    updatePlayTimeLeft();
}


void IGOClock::updateSize(const int& previousWidth)
{
    adjustSize();
    // If we grew our width with the text change, update the size.
    if (previousWidth < width())
    {
        const int MARGIN_DEFAULT = 80;
        Engine::IIGOWindowManager::WindowProperties settings;
        settings.setPosition(QPoint(MARGIN_DEFAULT + width(), MARGIN_DEFAULT), Engine::IIGOWindowManager::WindowDockingOrigin_X_RIGHT, true);
        Engine::IGOController::instance()->igowm()->setWindowProperties(ivcView(), settings);
        emit igoResize(size());
    }
}

} // Client
} // Origin