//
//  IGOAchievementViewController.h
//  originClient
//
//  Created by Frederic Meraud on 2/12/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#ifndef __originClient__IGOAchievementViewController__
#define __originClient__IGOAchievementViewController__

#include <QWidget>

#include "originwindow.h"
#include "WebWidget/WidgetView.h"
#include "engine/igo/IGOController.h"
#include "engine/login/LoginController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{    
namespace Client
{
    
    class ORIGIN_PLUGIN_API IGOAchievementsViewController : public QObject, public Origin::Engine::IAchievementsViewController
    {
        Q_OBJECT
        
    public:
        IGOAchievementsViewController();
        ~IGOAchievementsViewController();
        
        // IAchievementsViewController impl
        virtual QWidget* ivcView() { return mWindow; }
        bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior);
        virtual void ivcPostSetup() {}
        
        virtual void show(Origin::Engine::UserRef user, uint64_t userId, const QString& achievementSet);

    private:
        WebWidget::WidgetView* mWidgetView;
        UIToolkit::OriginWindow* mWindow;
    };
    
} // Client
} // Origin

#endif /* defined(__originClient__IGOAchievementViewController__) */
