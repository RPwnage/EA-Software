///////////////////////////////////////////////////////////////////////////////
// IGOClock.h
// 
// Created by Mike Dorval
// Copyright (c) 2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOCLOCK_H
#define IGOCLOCK_H
#include "engine/content/ContentTypes.h"
#include "engine/igo/IGOQWidget.h"
#include "engine/igo/IGOController.h"
#include "services/plugin/PluginAPI.h"

namespace Ui
{
    class IGOClock;
}

namespace Origin
{
namespace Client
{
class ORIGIN_PLUGIN_API IGOClock : public Origin::Engine::IGOQWidget, public Origin::Engine::IDesktopClockViewController
{
    Q_OBJECT
public:
    IGOClock(QWidget* parent = 0);
    virtual ~IGOClock();

    virtual QWidget* ivcView() { return this; }
    virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior) { return true; }
    virtual void ivcPostSetup() {}
    virtual void paintEvent(QPaintEvent*);
    virtual bool canBringToFront() { return false; }
    void timerEvent(QTimerEvent* event);


private slots:
    void onGameFocused(const uint32_t& gameId);
    void onGameAdded(const uint32_t& gameId);
    void onGameFreeTrialInfoUpdated(const uint32_t& gameId, const qint64&, QElapsedTimer*);

private:
    void startClock();
    void updateClock();
    void startBatteryTimer();
    void updateBattery();
    void updatePlaySession();
    void startPlayTimeLeft();
    void updatePlayTimeLeft();
    void stopTimer(int& timerId);
    void updateSize(const int& previousWidth);

    Ui::IGOClock* ui;
    int mClockTimerId;
    int mBatteryTimerId;
    int mPlayTimeLeftTimerId;
    qint64 mTrialTimeRemaining;
    QElapsedTimer* mTrialElapsedTimer;
    Engine::Content::EntitlementRef mEntitlement;
};
} // Client
} // Origin

#endif