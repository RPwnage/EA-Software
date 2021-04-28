//
//  IGOCustomerSupportViewController.h
//  originClient
//
//  Created by Frederic Meraud on 3/5/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#ifndef __originClient__IGOCustomerSupportViewController__
#define __originClient__IGOCustomerSupportViewController__

#include <QObject>

#include "originwindow.h"
#include "engine/igo/IGOController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
    
    class ORIGIN_PLUGIN_API IGOCustomerSupportViewController : public QObject, public Origin::Engine::ICustomerSupportViewController
    {
        Q_OBJECT
        
    public:
        IGOCustomerSupportViewController(const QString& url);
        ~IGOCustomerSupportViewController();
        
        // ICustomerSupportViewController impl
        virtual QWidget* ivcView() { return mWindow; }
        virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior);
        virtual void ivcPostSetup() {}
        
    private:
        UIToolkit::OriginWindow* mWindow;
    };
    
} // Client
} // Origin

#endif /* defined(__originClient__IGOCustomerSupportViewController__) */
