//
//  IGOSharedViewController.h
//  originClient
//
//  Created by Richard Spitler on 6/8/15.
//  Copyright (c) 2015 Electronic Arts. All rights reserved.
//

#ifndef __originClient__IGOSharedViewController__
#define __originClient__IGOSharedViewController__

#include <QObject>

#include "originwindow.h"
#include "engine/igo/IGOController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
    class OriginWebController;

    class ORIGIN_PLUGIN_API IGOSharedViewController : public QObject, public Origin::Engine::ISharedSPAViewController, public Origin::Engine::IASPViewController, public Origin::Engine::IIGOWindowManager::IScreenListener
    {
        Q_OBJECT
        
    public:
        IGOSharedViewController(const QString& id);
        ~IGOSharedViewController();
        
        // ISocialHubViewController impl
        virtual QWidget* ivcView() { return mWindow; }
        virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior);
        virtual void ivcPostSetup();

        // IASPViewController impl
        virtual QWebPage* createJSWindow(const QUrl& url);
        virtual void navigate(QString id);
        virtual void setProfileId(QString id){ mProfileId = id; };

    public slots:
        // IScreenListener impl
        virtual void onSizeChanged(uint32_t width, uint32_t height);
        
        
    private:
        QPoint defaultPosition(uint32_t width, uint32_t height);
        
        UIToolkit::OriginWindow* mWindow;
        QString mProfileId;
        OriginWebController* mWebController;
        bool mIntialized;
    };
    
} // Client
} // Origin

#endif /* defined(__originClient__IGOSharedViewController__) */
