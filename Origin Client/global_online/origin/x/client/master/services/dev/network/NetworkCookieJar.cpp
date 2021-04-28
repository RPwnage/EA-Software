//  NetworkCookieJar.cpp
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.

#include "services/network/NetworkCookieJar.h"

#include <limits>
#include <QDateTime>
#include <QNetworkCookie>
#include "services/log/LogService.h"

namespace Origin
{
    namespace Services
    {
        NetworkCookieJar::NetworkCookieJar(QObject *parent) : QNetworkCookieJar(parent)
        {
        }

        bool NetworkCookieJar::setCookiesFromUrl ( const QList<QNetworkCookie> & cookieList, const QUrl & url )
        {
            //first have the base class make sure that the cookie is "valid", have it replace any old ones
            bool addedCookies = (QNetworkCookieJar::setCookiesFromUrl (cookieList, url));

            if (addedCookies)
            {
                foreach (QNetworkCookie c, cookieList)
                {
                    QNetworkCookie foundCookie;
                    if (findFirstCookieByName (c.name(), foundCookie))
                    {
                        QString cookiename = c.name();
                        emit (setCookie (cookiename));
                    }
                }
            }
            return addedCookies;
        }

        void NetworkCookieJar::clear()
        {
            QList<QNetworkCookie> emptyList;
            setAllCookies(emptyList);
        }

        /// \brief returns the count of how many cookies of "name" are in the jar
        int NetworkCookieJar::cookieCountByName(const QString& name)
        {
            int count = 0;
            QList<QNetworkCookie> currentCookies(allCookies());
            foreach (QNetworkCookie c, currentCookies)
            {
                if (QString(c.name()).compare(name, Qt::CaseInsensitive) == 0)
                {
                    count++;
                }
            }
            return count;
        }

        /// \brief Assigns the parameter cookie to the first cookie found in the jar with the specified name
        bool NetworkCookieJar::findFirstCookieByName(const QString& name, QNetworkCookie& cookie)
        {
            QList<QNetworkCookie> currentCookies(allCookies());
            foreach (QNetworkCookie c, currentCookies)
            {
                if (QString(c.name()).compare(name, Qt::CaseInsensitive) == 0)
                {
                    cookie = c;
                    return true;
                }
            }

            return false;
        }

        bool NetworkCookieJar::findCookieWithLatestExpirationByName (const QString &name, QNetworkCookie& cookie)
        {
            bool found = false;
            QList<QNetworkCookie> currentCookies(allCookies());
            foreach (QNetworkCookie c, currentCookies)
            {
                if (QString(c.name()).compare(name, Qt::CaseInsensitive) == 0)
                {
                    if (!found) //first one
                    {
                        cookie = c;
                        found = true;
                    }
                    else if (c.expirationDate() > cookie.expirationDate())
                    {
                        cookie = c;
                    }
                }
            }

            return found;
        }


        void NetworkCookieJar::removeCookieByValue(const QString& cookieValue)
        {
            QList<QNetworkCookie> currentCookies(allCookies());
            QList<QNetworkCookie> newCookieList;
            foreach (QNetworkCookie c, currentCookies)
            {
                QString value(c.value());
                if (value != cookieValue)
                {
                    newCookieList.push_back(c);
                }
            }

            setAllCookies(newCookieList);
        }


        void NetworkCookieJar::removeCookieByValueAndExpiration(const QString& cookieValue, const QDateTime cookieExpiration )
        {
            QList<QNetworkCookie> currentCookies(allCookies());
            QList<QNetworkCookie> newCookieList;
            foreach (QNetworkCookie c, currentCookies)
            {
                QString value(c.value());
                QDateTime expiration(c.expirationDate());
                if (value != cookieValue)
                {
                    newCookieList.push_back(c);
                }
                else if (expiration != cookieExpiration)
                {
                    newCookieList.push_back(c);
                }
            }

            setAllCookies(newCookieList);
        }


        void NetworkCookieJar::transferCookies(NetworkCookieJar* jar)
        {
            if(jar)
            {
                QList<QNetworkCookie> cookieList = jar->allCookies();
                cookieList.append(allCookies());
                jar->setAllCookies(cookieList);
            }
        }
    }
}
