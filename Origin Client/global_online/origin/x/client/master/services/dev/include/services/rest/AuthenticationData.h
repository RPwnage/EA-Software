///////////////////////////////////////////////////////////////////////////////
// AuthenticationData.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _AUTHENTICATIONDATA_H_INCLUDED_
#define _AUTHENTICATIONDATA_H_INCLUDED_

#include "ServerUserData.h"

#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        /// \struct TokensNucleus2
        /// \brief contains the Nucleus 2.0 token info
        struct TokensNucleus2 : public ServerUserData
        {
            QString mAccessToken;
            QString mTokenType;
            QString mExpiresIn;
            QString mRefreshToken;
            QString mIdToken;
            const quint64 expiryTimeSecs() const { return mExpiresIn.toULongLong(); }
            bool isValid() const {return mAccessToken.size() > 0 && mRefreshToken.size() > 0 && mExpiresIn.size() > 0;}
            void reset() 
            {
                mAccessToken.clear();
                mTokenType.clear();
                mExpiresIn.clear();
                // dont reset mRefreshToken
                mIdToken.clear();
            }
        };

        ///
        /// struct User
        /// \brief contains all the user's information
        struct User : public ServerUserData
        {
            quint64 userId; //	The UserId for this user.
            quint64 personaId; //	The PersonaId for this user.
            QString originId; // The user's EAID.
            QString firstName; // The user's first name.
            QString lastName; // The user's last name.
            QString country; // The user's country.
            QString email; // The user's email.
            QString dob; //The user's dob
            bool isUnderage; // The user is Underage or false.
            User() : userId(0LL), personaId(0LL), isUnderage(true) {}
            bool isValid() const { return userId > 0LL && personaId > 0LL && originId.size() > 0 && email.size() > 0;}
            void reset() {userId=0LL;personaId=0LL;originId.clear();firstName.clear();lastName.clear();country.clear();email.clear();isUnderage=true;}
            
        };

        struct AppSettings : public ServerUserData
        {
            QString dt;
            bool overrideTab;
            AppSettings() : dt("mygames") // If we don't get any response from the server we want to default to my games
            , overrideTab(false) {}
            bool isValid() const { return !dt.isEmpty(); };
            void reset() { overrideTab = false; dt.clear(); }
        };

        struct Authentication2 : public ServerUserData
        {
            TokensNucleus2 token;
            User user;
            AppSettings appSettings;
            QString mIsLoginInvisible;

            bool isValid() const { return token.isValid() && user.isValid() && appSettings.isValid();}
            void reset() { token.reset(); user.reset(); appSettings.reset(); mIsLoginInvisible.clear();}
        };
    }
}





#endif


