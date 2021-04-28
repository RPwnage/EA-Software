//
//  IGOErrorViewController.h
//  originClient
//
//  Created by Frederic Meraud on 3/6/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#ifndef __originClient__IGOErrorViewController__
#define __originClient__IGOErrorViewController__

#include <QWidget>

#include "originwindow.h"
#include "engine/igo/IGOController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
    
    class ORIGIN_PLUGIN_API IGOErrorViewController : public QObject, public Origin::Engine::IErrorViewController
    {
        Q_OBJECT
        
    public:
        IGOErrorViewController();
        ~IGOErrorViewController();
        
        // IAchievementsViewController impl
        virtual QWidget* ivcView() { return mWindow; }
        bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior);
        virtual void ivcPostSetup();

        virtual void setView(UIToolkit::OriginWindow* view);
        virtual void setView(const QString& title, const QString& text, const QString& okBtnText, QObject* okBtnObj, const QString& okBtnSlot);
        
    private slots:
        void windowDestroyed();
        
    private:
        UIToolkit::OriginWindow* mWindow;
    };
    
} // Client
} // Origin


#endif /* defined(__originClient__IGOErrorViewController__) */
