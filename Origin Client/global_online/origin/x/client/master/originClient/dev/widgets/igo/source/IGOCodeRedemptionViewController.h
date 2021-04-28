//
//  IGOCodeRedemptionViewController.h
//  originClient
//
//  Created by Frederic Meraud on 3/26/15.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#ifndef __originClient__IGOCodeRedemptionViewController__
#define __originClient__IGOCodeRedemptionViewController__

#include <QWidget>

#include "originwindow.h"
#include "engine/igo/IGOController.h"
#include "services/plugin/PluginAPI.h"

// Fwd decls
class RedeemBrowser;

namespace Origin
{    
namespace Client
{
    
    class ORIGIN_PLUGIN_API IGOCodeRedemptionViewController : public QObject, public Origin::Engine::ICodeRedemptionViewController
    {
        Q_OBJECT
        
    public:
        IGOCodeRedemptionViewController();
        ~IGOCodeRedemptionViewController();
        
        // ICodeRedemptionViewController impl
        virtual QWidget* ivcView() { return mWindow; }
        bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior);
        virtual void ivcPostSetup() {}
        
    private:
        RedeemBrowser* mRedeemBrowser;
        UIToolkit::OriginWindow* mWindow;
    };
    
} // Client
} // Origin

#endif /* defined(__originClient__IGOCodeRedemptionViewController__) */
