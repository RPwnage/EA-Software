///////////////////////////////////////////////////////////////////////////////
// WebDispatcherRequestBuilder.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _WEBDISPATCHERREQUESTBUILDER_H
#define _WEBDISPATCHERREQUESTBUILDER_H

#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
	namespace Client
	{
		namespace WebDispatcherRequestBuilder
		{
			/// \brief Contains information about a web dispatcher request.
			struct WebDispatcherRequest
			{
				/// \brief The WebDispatcherRequest constructor.
				/// \param newNetworkRequest The request to copy from.
				/// \param newOperation The operation of the new request.
				/// \param newBody The body of the new request.
				WebDispatcherRequest(const QNetworkRequest &newNetworkRequest, QNetworkAccessManager::Operation newOperation, const QByteArray &newBody) :
			networkRequest(newNetworkRequest), operation(newOperation), body(newBody)
			{
			}

			QNetworkRequest networkRequest; ///< The Qt network request.
			QNetworkAccessManager::Operation operation; ///< The network operation.
			QByteArray body; ///< The body of the request.
			};

			/// \brief Builds a network request to be routed through the web dispatcher
			/// 
			/// This is intended to take SSO exchange tokens and turn them in to QNetworkRequest instances to be loaded
			/// by a QWebView/QWebPage
			/// \param exchangeTicket The exchange ticket to add to the request body.
			/// \param redirectUrl The redirect URL to add to the request body.
			/// \return The request that was built.
			ORIGIN_PLUGIN_API WebDispatcherRequest buildRequest(const QString &exchangeTicket, const QUrl redirectUrl);

			ORIGIN_PLUGIN_API QUrl buildGETUrl(const QString &exchangeTicket, const QUrl& redirectUrl);
		}
	}
}

#endif
