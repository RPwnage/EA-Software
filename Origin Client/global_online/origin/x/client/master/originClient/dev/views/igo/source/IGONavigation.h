///////////////////////////////////////////////////////////////////////////////
// IGONavigation.h
// 
// Created by Mike Dorval
// Copyright (c) 2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGONAVIGATION_H
#define IGONAVIGATION_H
#include "engine/igo/IGOQWidget.h"
#include "engine/igo/IGOController.h"
#include "services/plugin/PluginAPI.h"

namespace WebWidget
{
    class WidgetView;
}

namespace Origin
{
namespace Client
{
class ORIGIN_PLUGIN_API IGONavigation : public Origin::Engine::IGOQWidget, public Origin::Engine::IDesktopNavigationViewController
{
    Q_OBJECT
public:
    IGONavigation(QWidget* parent = 0);
    virtual ~IGONavigation();

    virtual QWidget* ivcView() { return this; }
    virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior) { return true; }
    virtual void ivcPostSetup() {}

    virtual void paintEvent(QPaintEvent *);
    virtual bool canBringToFront() { return false; }


private slots:
    void onProxiesLoaded();
    void updateSize();

private:
    WebWidget::WidgetView* mWebView;
};
} // Client
} // Origin

#endif