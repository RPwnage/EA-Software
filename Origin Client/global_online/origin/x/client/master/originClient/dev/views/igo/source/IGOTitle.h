///////////////////////////////////////////////////////////////////////////////
// IGOTitle.h
// 
// Created by Mike Dorval
// Copyright (c) 2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOTITLE_H
#define IGOTITLE_H
#include "engine/igo/IGOQWidget.h"
#include "engine/igo/IGOController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
    class OriginLabel;
}
namespace Client
{
class ORIGIN_PLUGIN_API IGOTitle : public Origin::Engine::IGOQWidget, public Origin::Engine::IDesktopTitleViewController, public Origin::Engine::IIGOWindowManager::IScreenListener
{
    Q_OBJECT
public:
    IGOTitle(QWidget* parent = 0);
    virtual ~IGOTitle();

    virtual QWidget* ivcView() { return this; }
    virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior) { return true; }
    virtual void ivcPostSetup() {}

    virtual void paintEvent(QPaintEvent*);
    virtual bool canBringToFront() { return false; }


private slots:
    void onGameFocused(const uint32_t&);
    // IScreenListener impl
    virtual void onSizeChanged(uint32_t width, uint32_t height);

private:
    UIToolkit::OriginLabel* mLabel;
};
} // Client
} // Origin

#endif