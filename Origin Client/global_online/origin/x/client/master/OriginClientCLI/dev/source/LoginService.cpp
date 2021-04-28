//
//  LoginService.cpp
//  
//
//  Created by Kiss, Bela on 12-06-05.
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
//

#include "LoginService.h"
#include "Console.h"
#include "services/debug/DebugService.h"
#include "services/session/LoginRegistrationSession.h"

#include <iostream>

#include <unistd.h>


using namespace Origin::Services;

LoginService* LoginService::sInstance = NULL;

void LoginService::init()
{
    if (sInstance == NULL)
    {
        sInstance = new LoginService();
        ORIGIN_ASSERT(sInstance);
    }
}

LoginService::LoginService()
{
    ORIGIN_VERIFY_CONNECT(
        Session::SessionService::instance(), SIGNAL(beginSessionComplete(Origin::Services::Session::SessionError, Origin::Services::Session::SessionRef, QSharedPointer<Origin::Services::Session::AbstractSessionConfiguration>)),
        this, SLOT(onLoginComplete(Origin::Services::Session::SessionError, Origin::Services::Session::SessionRef, QSharedPointer<Origin::Services::Session::AbstractSessionConfiguration>)));
    ORIGIN_VERIFY_CONNECT(
        Session::SessionService::instance(), SIGNAL(endSessionComplete(Origin::Services::Session::SessionError, Origin::Services::Session::SessionRef)),
        this, SLOT(onLogoutComplete(Origin::Services::Session::SessionError)));
}

void LoginService::registerCommands(CommandDispatcher& dispatcher)
{
    instance()->mCli = &dispatcher;
    dispatcher.add(Command("login", "", LoginService::login));
    dispatcher.add(Command("logout", "", LoginService::logout));
}


void LoginService::login(QStringList const& options)
{
    // Bail if we are already have a current session.
    if ( Session::SessionService::currentSession().data() )
    {
        CONSOLE_WRITE << "Already logged in.";
        return;
    }
    
    // Prompt the user for a username.
    CONSOLE_WRITE << "        Username: ";
    instance()->mCli->takeInputFromNextLine(instance(), "usernameResponse");

}

void LoginService::usernameResponse(QStringList const& input)
{
    // Save the username.
    mUsername = input[0];
    
    // Prompt the user for a password.
    CONSOLE_WRITE << "        Password: ";
    
#ifndef WIN32
    // Turn off terminal echo.
    tcgetattr(0, &mTermios);
    termios newt = mTermios;
    newt.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &newt);
#endif    
    mCli->takeInputFromNextLine(this, "passwordResponse");
}

void LoginService::passwordResponse(QStringList const& input)
{
#ifndef WIN32
    // Turn on terminal echo.
    tcsetattr(0, TCSANOW, &mTermios);
    CONSOLE_WRITE << "";
#endif
    
    QString password;
    if (mUsername.isNull())
    {
        mUsername = "";
        password = "";
    }
    else
        password = input[0];
    
    CONSOLE_WRITE << "    Logging in as " << qPrintable(mUsername);

    Session::LoginRegistrationCredentials loginCredentials(mUsername, password);
    QString oemTag("");
    QString encryptedMacAddress("");
    Session::LoginOptions loginOptions = 0;
    Session::LoginRegistrationConfiguration loginConfig(loginCredentials, oemTag, encryptedMacAddress, loginOptions);
    Session::SessionService::beginSessionAsync(Session::LoginRegistrationSession::create, loginConfig);
}

void LoginService::logout(QStringList const& options)
{
    CONSOLE_WRITE << "handling logout";

    Session::SessionRef session = Session::SessionService::currentSession();
    Session::SessionService::endSessionAsync(session);
}

void LoginService::onLoginComplete(
    Origin::Services::Session::SessionError sessionError, 
    Origin::Services::Session::SessionRef sessionRef,
    QSharedPointer<Origin::Services::Session::AbstractSessionConfiguration> sessionConfiguration)
{
    if (sessionError == Session::SESSION_NO_ERROR)
    {
        CONSOLE_WRITE << "logged in, auth = " << qPrintable(Session::SessionService::authToken(sessionRef));
        mCurrentSession = sessionRef;
    }
    else
    {
        CONSOLE_WRITE << "login failed, err = " << int(sessionError.errorCode());
    }
}

void LoginService::onLogoutComplete(
    Origin::Services::Session::SessionError sessionError)
{
    CONSOLE_WRITE << "logged out, err = " << int(sessionError.errorCode());
    mCurrentSession = Session::SessionRef();
}
