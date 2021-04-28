//
//  IGOInviteFriendsToGameViewController.h
//  originClient
//
//  Created by Frederic Meraud on 3/10/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#ifndef __originClient__IGOInviteFriendsToGame__
#define __originClient__IGOInviteFriendsToGame__

#include <QWidget>

#include "engine/igo/IGOController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
    class InviteFriendsWindowView;

    class ORIGIN_PLUGIN_API IGOInviteFriendsToGameViewController : public QObject, public Origin::Engine::IInviteFriendsToGameViewController
    {
        Q_OBJECT
        
    public:
        IGOInviteFriendsToGameViewController(const QString& gameName);
        ~IGOInviteFriendsToGameViewController();
        
        // IInviteFriendsToGameViewController impl
        virtual QWidget* ivcView() { return mWindow; }
        virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior);
        virtual void ivcPostSetup();
    private slots:
        void closeWindow();
        void onInviteUsersToGame(const QObjectList& users);
    private:
        void showUnavailableWarning(const bool& underAge, const bool& invisible, const bool& offline);
        
        QString mGameName;
        UIToolkit::OriginWindow* mWindow;
        InviteFriendsWindowView* mInviteFriendsWindowView;
    };
    
} // Client
} // Origin


#endif /* defined(__originClient__IGOInviteFriendsToGameViewController__) */
