///////////////////////////////////////////////////////////////////////////////
// OriginAuthServiceClient.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/rest/OriginAuthServiceClient.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/network/OfflineModeReply.h"
#include <QNetworkReply>

namespace Origin
{
    namespace Services
    {

        OriginAuthServiceClient::OriginAuthServiceClient(NetworkAccessManager *nam)
            : OriginServiceClient(nam)
        {
        }

        OriginAuthServiceClient::~OriginAuthServiceClient(void)
        {
        }

        AuthNetworkRequest OriginAuthServiceClient::headAuth(Session::SessionRef session, const QNetworkRequest &request) const
        {
            // TODO need to implement similar throttling in HttpChannel::SendRequest() and 
            // FileChannel::SendRequest()
            if ( Connection::ConnectionStatesService::isUserOnline(session) )
                return AuthNetworkRequest(networkAccessManager()->head(request), session);
            else
                return AuthNetworkRequest(new OfflineModeReply(QNetworkAccessManager::HeadOperation, request), session);
        }

        AuthNetworkRequest OriginAuthServiceClient::getAuth(Session::SessionRef session, const QNetworkRequest &request) const
        {
            if ( Connection::ConnectionStatesService::isUserOnline(session) )
                return AuthNetworkRequest(networkAccessManager()->get(request), session);
            else
                return AuthNetworkRequest(new OfflineModeReply(QNetworkAccessManager::GetOperation, request), session);
        }

        AuthNetworkRequest OriginAuthServiceClient::postAuth(Session::SessionRef session, const QNetworkRequest &request, QIODevice *data) const
        {
            if ( Connection::ConnectionStatesService::isUserOnline(session) )
                return AuthNetworkRequest(networkAccessManager()->post(request, data), session);
            else
                return AuthNetworkRequest(new OfflineModeReply(QNetworkAccessManager::PostOperation, request), session);
        }

        AuthNetworkRequest OriginAuthServiceClient::postAuth(Session::SessionRef session, const QNetworkRequest &request, const QByteArray &data) const
        {
            if ( Connection::ConnectionStatesService::isUserOnline(session) )
                return AuthNetworkRequest(networkAccessManager()->post(request, data), session);
            else
                return AuthNetworkRequest(new OfflineModeReply(QNetworkAccessManager::PostOperation, request), session);
        }

        AuthNetworkRequest OriginAuthServiceClient::postAuth(Session::SessionRef session, const QNetworkRequest &request, QHttpMultiPart *multiPart) const
        {
            if ( Connection::ConnectionStatesService::isUserOnline(session) )
                return AuthNetworkRequest(networkAccessManager()->post(request, multiPart), session);
            else
                return AuthNetworkRequest(new OfflineModeReply(QNetworkAccessManager::PostOperation, request), session);
        }

        AuthNetworkRequest OriginAuthServiceClient::putAuth(Session::SessionRef session, const QNetworkRequest &request, QIODevice *data) const
        {
            if ( Connection::ConnectionStatesService::isUserOnline(session) )
                return AuthNetworkRequest(networkAccessManager()->put(request, data), session);
            else
                return AuthNetworkRequest(new OfflineModeReply(QNetworkAccessManager::PutOperation, request), session);
        }

        AuthNetworkRequest OriginAuthServiceClient::putAuth(Session::SessionRef session, const QNetworkRequest &request, const QByteArray &data) const
        {
            if ( Connection::ConnectionStatesService::isUserOnline(session) )
                return AuthNetworkRequest(networkAccessManager()->put(request, data), session);
            else
                return AuthNetworkRequest(new OfflineModeReply(QNetworkAccessManager::PutOperation, request), session);
        }

        AuthNetworkRequest OriginAuthServiceClient::putAuth(Session::SessionRef session, const QNetworkRequest &request, QHttpMultiPart *multiPart) const
        {
            if ( Connection::ConnectionStatesService::isUserOnline(session) )
                return AuthNetworkRequest(networkAccessManager()->put(request, multiPart), session);
            else
                return AuthNetworkRequest(new OfflineModeReply(QNetworkAccessManager::PutOperation, request), session);
        }

        AuthNetworkRequest OriginAuthServiceClient::deleteResourceAuth(Session::SessionRef session, const QNetworkRequest &request) const
        {
            if ( Connection::ConnectionStatesService::isUserOnline(session) )
                return AuthNetworkRequest(networkAccessManager()->deleteResource(request), session);
            else
                return AuthNetworkRequest(new OfflineModeReply(QNetworkAccessManager::DeleteOperation, request), session);
        }

        AuthNetworkRequest OriginAuthServiceClient::sendCustomRequestAuth(Session::SessionRef session, const QNetworkRequest &request, const QByteArray &verb, QIODevice *data /*= NULL*/) const
        {
            if ( Connection::ConnectionStatesService::isUserOnline(session) )
                return AuthNetworkRequest(networkAccessManager()->sendCustomRequest(request, verb, data), session);
            else
                return AuthNetworkRequest(new OfflineModeReply(QNetworkAccessManager::CustomOperation, request), session);
        }

        AuthNetworkRequest OriginAuthServiceClient::getAuthOffline(Session::SessionRef session, const QNetworkRequest &request) const
        {
            return AuthNetworkRequest(networkAccessManager()->get(request), session);
        }
    }
}
