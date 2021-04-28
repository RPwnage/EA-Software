// *********************************************************************
//  IGOBroadcastWidget.cpp
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
// **********************************************************************

#ifndef IGO_BROADCASTING_WIDGET_H
#define IGO_BROADCASTING_WIDGET_H

#include "engine/igo/IGOQWidget.h"
#include "engine/igo/IGOController.h"
#include "services/plugin/PluginAPI.h"
#include <QWidget>

namespace Ui
{
    class IGOBroadcastingWidget;
}

namespace Origin
{
namespace Engine
{
    class IGOTwitch;
}
namespace Client
{

class ORIGIN_PLUGIN_API IGOBroadcastingWidget : public Origin::Engine::IGOQWidget, public Origin::Engine::ITwitchBroadcastIndicator
{
    Q_OBJECT

public: 
    IGOBroadcastingWidget(QWidget* parent = NULL);
    virtual ~IGOBroadcastingWidget();

	// ITwitchBroadcastIndicator impl
	virtual QWidget* ivcView() { return this; }
    virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior) { return true; }
    virtual void ivcPostSetup() {}
	virtual bool canBringToFront() { return false; }
    virtual void paintEvent(QPaintEvent*);
    virtual void resizeEvent(QResizeEvent*);


private slots:
    void onBroadcastStreamInfoChanged(const int& num);
    void onBroadcastDurationChanged(const int& duration);
    void onBroadcastIndicatorChanged(const bool& visible);

private:
    bool event(QEvent* event);

    Ui::IGOBroadcastingWidget* ui;
};

} // Client
} // Origin

#endif