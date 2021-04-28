///////////////////////////////////////////////////////////////////////////////
// IGOLogo.h
// 
// Copyright (c) 2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef IGOLOGO_H
#define IGOLOGO_H
#include "engine/igo/IGOQWidget.h"
#include "engine/igo/IGOController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
class ORIGIN_PLUGIN_API IGOLogo : public Origin::Engine::IGOQWidget, public Origin::Engine::IDesktopLogoViewController
{
    Q_OBJECT
public:
    IGOLogo(QWidget* parent = 0);
    virtual ~IGOLogo();

    virtual QWidget* ivcView() { return this; }
    virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior) { return true; }
    virtual void ivcPostSetup() {}

    virtual void paintEvent(QPaintEvent*);
    virtual bool canBringToFront() { return false; }
};
} // Client
} // Origin

#endif