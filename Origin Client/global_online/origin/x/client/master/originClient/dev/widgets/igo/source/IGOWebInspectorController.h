//
//  IGOWebInspectorController.h
//  originClient
//
//  Created by Frederic Meraud on 6/4/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#ifndef __originClient__IGOWebInspectorController__
#define __originClient__IGOWebInspectorController__

#include "engine/igo/IGOController.h"

namespace Origin
{
namespace Client
{

    class IGOWebInspectorController : public QObject, public Origin::Engine::IWebInspectorController
    {
        Q_OBJECT
        
    public:
        IGOWebInspectorController(QWebPage* page);
        ~IGOWebInspectorController();
        
        // IWebInspectorController impl
        virtual QWidget* ivcView() { return mWindow; }
        virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior) { return true; }
        virtual void ivcPostSetup() {}
        
    private:
        UIToolkit::OriginWindow* mWindow;
    };

} // Client
} // Origin

#endif /* defined(__originClient__IGOWebInspectorController__) */
