///////////////////////////////////////////////////////////////////////////////
// RequestClientBase.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _REQUESTCLIENTBASE_H
#define _REQUESTCLIENTBASE_H

#include "OriginAuthServiceClient.h"
#include <QDomDocument>
#include "services/common/XmlUtil.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API RequestClientBase : public OriginAuthServiceClient
        {

        public:
            virtual ~RequestClientBase() = 0;

        protected:

            /// \brief Creates a new Atom service client
            /// \param nam           QNetworkAccessManager instance to send requests through
            explicit RequestClientBase(NetworkAccessManager *nam = NULL);

            /// \brief helper GET request generator
            template <typename T>
            T* userRequest(Session::SessionRef session, quint64 nucleusId, const QString& api)
            {
                ORIGIN_ASSERT(session);
                QString servicePath = QString("/users/%1/other/%2/%3").arg(Session::SessionService::nucleusUser(session)).arg(nucleusId).arg(api);
                return getRequest<T>(session, servicePath);
            }

            /// \brief The HTTP GET request method.
            /// \param session The accessToken.
            /// \param servicePath The service path.
            /// \param queryItems TBD.
            /// \todo Please fill in the TBD item for this method.
            template <typename T>
            T* getRequest(Session::SessionRef session, const QString& servicePath, const QMap<QString, QString>& queryItems = ( QMap<QString, QString>() ))
            {
                QNetworkRequest request = buildRequest(session, servicePath, queryItems);
                return new T(getAuth(session, request));
            }

            /// \brief The HTTP PUT request method.
            /// \param session The accessToken.
            /// \param servicePath The service path.
            /// \param mySearchOptions The search options.
            template <typename T>
            T* putRequest(Session::SessionRef session, const QString& servicePath, QVector<QString>& mySearchOptions)
            {
                QNetworkRequest request = buildRequest(session, servicePath);

                QDomDocument doc;
                QDomProcessingInstruction instr = doc.createProcessingInstruction("xml", "version='1.0' encoding='UTF-8'");
                doc.appendChild(instr);
                QDomElement elem = doc.createElement("options");

                foreach(QString myOption, mySearchOptions)
                {
                    Origin::Util::XmlUtil::setString(elem, "option", myOption);
                }

                doc.appendChild(elem);

                QByteArray postBody(doc.toByteArray());

                return new T(putAuth(session, request, postBody));
            }

            /// \brief The common function that builds the request for the clients.
            /// \param session The access token.
            /// \param servicePath The common service path.
            /// \param queryItems TBD.
            /// \todo Please fill in the TBD item for this method.
            QNetworkRequest buildRequest(Session::SessionRef session, const QString& servicePath, const QMap<QString, QString>& queryItems = ( QMap<QString, QString>() ));

        };
    }
}

#endif
