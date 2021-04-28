/////////////////////////////////////////////////////////////////////////////
// AuthenticatedWebUIController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef _AUTHENTICATEDWEBUICONTROLLER_H
#define _AUTHENTICATEDWEBUICONTROLLER_H

#include <QUrl>
#include <QString>

#include "OriginWebController.h"
#include "services/session/AbstractSession.h"
#include "services/plugin/PluginAPI.h"

class QWebView;

namespace Origin
{
namespace Client
{

class ORIGIN_PLUGIN_API AuthenticatedWebUIController : public OriginWebController
{
	Q_OBJECT
public:
	AuthenticatedWebUIController(Services::Session::SessionRef session, QWebView *);

	///
	/// \brief Loads an authenticated Web UI hosted URL by transferring the authentication 
	///        of the passed session
	///
	void loadAuthenticatedUrl(const QUrl &);

    ///
    /// \brief allow for bypassing of retrieving exchange SSO token since it's not needed for EC2/customer portal verion 2
    ///
    void setSkipAuthenticaton (bool skip) { mSkipAuthentication = skip; }

protected:
    void beginAuthentication();

protected slots:
	void onExchangeTokenSuccess(QString);

	/// \brief Catches errors on the exchange token.
	void onExchangeTokenFailed(Origin::Services::restError);

    void handleNetworkReply(QNetworkReply *reply);

private:
    void init();

    Services::Session::SessionRef mSession;

	unsigned int mConsecutiveAuthenticationFailures;
    bool mAuthenticated;
    bool mSkipAuthentication;

	QUrl mRedirectUrl;
};

}
}

#endif
