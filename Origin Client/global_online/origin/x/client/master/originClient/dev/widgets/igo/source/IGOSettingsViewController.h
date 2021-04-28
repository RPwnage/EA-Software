//
//  IGOSettingsViewController.h
//  originClient
//
//  Created by Frederic Meraud on 3/5/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#ifndef __originClient__IGOSettingsViewController__
#define __originClient__IGOSettingsViewController__

#include <QObject>

#include "originwindow.h"
#include "engine/igo/IGOController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{

    class ORIGIN_PLUGIN_API IGOSettingsViewController : public QObject, public Origin::Engine::ISettingsViewController, public Origin::Engine::IASPViewController, public Origin::Engine::IIGOWindowManager::IScreenListener
    {
        Q_OBJECT
        
    public:
        IGOSettingsViewController(ISettingsViewController::Tab displayTab);
        ~IGOSettingsViewController();
        
        // ISettingsViewController impl
        virtual QWidget* ivcView() { return mWindow; }
        virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior);
        virtual void ivcPostSetup();

        // IASPViewController impl
        virtual QWebPage* createJSWindow(const QUrl& url);

    public slots:
        // IScreenListener impl
        virtual void onSizeChanged(uint32_t width, uint32_t height);
        
        
    private:
        QPoint defaultPosition(uint32_t width, uint32_t height);
        
        UIToolkit::OriginWindow* mWindow;
        ISettingsViewController::Tab mDisplayTab;
    };
    
} // Client
} // Origin

#endif /* defined(__originClient__IGOSettingsViewController__) */
