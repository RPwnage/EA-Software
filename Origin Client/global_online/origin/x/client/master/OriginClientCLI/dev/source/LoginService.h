//
//  LoginService.h
//  
//
//  Created by Kiss, Bela on 12-06-05.
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
//

#ifndef _LoginService_h
#define _LoginService_h

#include "Command.h"
#include "CommandDispatcher.h"
#include "services/session/SessionService.h"
#include <QObject>
#include <QStringList>

#ifndef WIN32
#include <termios.h>
#endif


class LoginService : public QObject
{
    Q_OBJECT
    
public:

    static void init();

    static LoginService* instance();
    static Origin::Services::Session::SessionRef currentSession();
    
    static void registerCommands(CommandDispatcher& dispatcher);
    
    static void login(QStringList const& options);
    static void logout(QStringList const& options);
    
protected slots:
    
    void usernameResponse(const QStringList&);
    void passwordResponse(const QStringList&);
    
    void onLoginComplete(
        Origin::Services::Session::SessionError sessionError, 
        Origin::Services::Session::SessionRef sessionRef,
        QSharedPointer<Origin::Services::Session::AbstractSessionConfiguration> sessionConfiguration);
        
    void onLogoutComplete(
        Origin::Services::Session::SessionError sessionError);
        
private:

    LoginService();

    static LoginService* sInstance;
    Origin::Services::Session::SessionRef mCurrentSession;
    
    CommandDispatcher* mCli;
    
    QString mUsername;
#ifndef WIN32
    termios mTermios;
#endif
};

inline LoginService* LoginService::instance()
{
    return sInstance;
}

inline Origin::Services::Session::SessionRef LoginService::currentSession()
{
    return instance()->mCurrentSession;
}

#endif
