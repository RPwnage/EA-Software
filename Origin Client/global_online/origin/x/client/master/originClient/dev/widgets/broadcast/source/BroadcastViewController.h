/////////////////////////////////////////////////////////////////////////////
// BroadcastViewController.h
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef BROADCAST_VIEW_CONTROLLER_H
#define BROADCAST_VIEW_CONTROLLER_H

#include <QObject>
#include <QPoint>
#include "services/plugin/PluginAPI.h"
#include "engine/igo/IGOController.h"
#include "UIScope.h"

class QKeyEvent;
class QMouseEvent;

namespace WebWidget
{
    class WidgetView;
}

namespace Origin
{
namespace UIToolkit
{
class OriginWindow;
}
namespace Client
{
class TwitchJsHelper : public QObject
{
    Q_OBJECT
signals:
    void close(const QString& token);
};

#ifdef ORIGIN_PC
class ORIGIN_PLUGIN_API BroadcastViewController : public QObject, public Origin::Engine::ITwitchViewController, public Origin::Engine::IIGOWindowManager::IScreenListener
{
	Q_OBJECT

public:
    enum BroadcastWindows
    {
        BROADCAST_NONE,
        BROADCAST_INTRO,
        BROADCAST_LOGIN,
        BROADCAST_SETTINGS,
        BROADCAST_ERROR
    };
    // ITwitchViewController impl
    virtual QWidget* ivcView() { return mWindow; }
    virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior);
    virtual void ivcPostSetup() {}
    virtual QPoint defaultPosition(uint32_t width, uint32_t height);
    virtual Origin::Engine::IIGOCommandController::CallOrigin startedFrom() const;

    /// \brief Constructor
    /// \param parent The parent of the BroadcastViewController
    /// \param context TBD.
    BroadcastViewController(QObject* parent = 0, const UIScope& context = IGOScope, const bool& inDebug = false);

    /// \brief Destructor
	~BroadcastViewController();

    UIToolkit::OriginWindow* window() const {return mWindow;}
    QPoint point() const {return mOIGPrevPosition;}

public slots:
    // IScreenListener impl
    virtual void onSizeChanged(uint32_t width, uint32_t height);
    void showIntro(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
    void showSettings(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
    void showTwitchLogin(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
    void showNotSupportedError(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
    void showNetworkError(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
    void showBroadcastStoppedError();


private slots:
    void closeWindow();
    void populatePageJavaScriptWindowObject();
    void onLoginWindowDone(const QString& token);

private:
    void showWindow(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
    bool resetCloseIGOOnClosePropertyIfStartedFromSDK() const;

    const UIScope mContext;
    const bool mInDebug;
    UIToolkit::OriginWindow* mWindow;
    WebWidget::WidgetView* mWebView;
    TwitchJsHelper* mTwitchHelper;
    QPoint mOIGPrevPosition;
    BroadcastWindows mCurrentWindowOption;
};
#endif
}
}
#endif
