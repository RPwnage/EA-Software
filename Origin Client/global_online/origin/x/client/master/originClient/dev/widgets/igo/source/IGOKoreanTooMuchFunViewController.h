//
//  IGOKoreanTooMuchFunViewController.h
//  originClient
//
//  Created by Frederic Meraud on 3/11/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#ifndef __originClient__IGOKoreanTooMuchFunViewController__
#define __originClient__IGOKoreanTooMuchFunViewController__

#include <QTimer>
#include <QDateTime>
#include <QWidget>

#include "engine/igo/IGOController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{

    // Korean Gaming Lay - We need to be able to show session time in the notice.
    class ORIGIN_PLUGIN_API IGOKoreanTooMuchFunViewController : public QObject, public Origin::Engine::IKoreanTooMuchFunViewController
    {
        Q_OBJECT
        
    public:
        IGOKoreanTooMuchFunViewController();
        ~IGOKoreanTooMuchFunViewController();
        
        // IAchievementsViewController impl
        virtual QWidget* ivcView() { return NULL; }
        virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior) { return mTimer != NULL; }
        virtual void ivcPostSetup() {}
        
    private slots:
        void showWarning();
        
    private:
        QTimer  *mTimer;
        QDateTime mTimerStart;
    };

} // Client
} // Origin

#endif /* defined(__originClient__IGOKoreanTooMuchFunViewController__) */
