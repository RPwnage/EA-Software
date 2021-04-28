//  NetworkCookieJar.h
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.

#ifndef NETWORKCOOKIEJAR_H
#define NETWORKCOOKIEJAR_H

#include <QNetworkCookieJar>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API NetworkCookieJar : public QNetworkCookieJar
        {
            Q_OBJECT

            public:
                enum cookieElementType
                {
                    cookieName = 0,
                    cookieValue,
                    cookieExpiration
                };

                NetworkCookieJar(QObject *parent = NULL);

                /// \brief override of the base class method so we can detect when cookies are added/updated
                bool setCookiesFromUrl ( const QList<QNetworkCookie> & cookieList, const QUrl & url ); 

                void initialize();

                /// \brief returns how many cookies Assigns cookie to the first cookie found in the jar with the specified name
                int cookieCountByName (const QString &name);

                /// \brief Assigns cookie to the first cookie found in the jar with the specified name
                bool findFirstCookieByName(const QString& name, QNetworkCookie& cookie);

                /// \brief Looks for the cookie that matches name that has the expiration date furthest out in the future
                bool findCookieWithLatestExpirationByName (const QString &name, QNetworkCookie& cookie);

                /// \brief If it exists, removes a cookie from the jar with a specified value (see QNetworkCookie::value())
                void removeCookieByValue(const QString& cookieValue);

                /// \brief If it exists, remove the cookie from jar with specified value and expiration
                void removeCookieByValueAndExpiration(const QString& cookieValue, const QDateTime expiration );

                /// \brief Transfer cookies from this cookie jar into the passed cookie jar, preserving any cookies in the 
                /// passed jar.
                void transferCookies(NetworkCookieJar* jar);

            signals:
                void setCookie (QString cookieName);

            public slots:
                void clear();

            private:
        };
    }
}

#endif // NETWORKCOOKIEJAR_H
