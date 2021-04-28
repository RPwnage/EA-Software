/////////////////////////////////////////////////////////////////////////////
// AuthenticatedWebUIController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebFrame>

#include "AuthenticatedWebUIController.h"
#include "WebDispatcherRequestBuilder.h"

#include "services/rest/SSOTicketServiceClient.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/rest/HttpStatusCodes.h"

namespace
{
	const unsigned int MaximumConsecutiveAuthenticationFailures = 3;
}

using Origin::Services::Session::SessionRef;
using Origin::Services::SSOTicketServiceClient;
using Origin::Services::SSOTicketServiceResponse;

namespace Origin
{
namespace Client
{
	
AuthenticatedWebUIController::AuthenticatedWebUIController(SessionRef session, QWebView *view) :
	OriginWebController(view),
    mSession(session),
	mConsecutiveAuthenticationFailures(0),
    mAuthenticated(false),
    mSkipAuthentication (false)
{
    init();
}

void AuthenticatedWebUIController::init()
{
    ORIGIN_VERIFY_CONNECT(page()->networkAccessManager(), SIGNAL(finished(QNetworkReply *)),
        this, SLOT(handleNetworkReply(QNetworkReply *)));
    
	// Disable detecting mainFrame load errors as we handle them ourselves
    errorHandler()->errorDetector()->disableErrorCheck(PageErrorDetector::MainFrameRequestFailed);
}
	
void AuthenticatedWebUIController::loadAuthenticatedUrl(const QUrl &url)
{
    // This is the URL we redirect to after we authenticate
    // We also return to this URL upon authentication failure
	mRedirectUrl = url;

    if (mSkipAuthentication)    //for EC2 and customer portal 2, we don't need to be SSO exchange token, it just uses session cookie
        mAuthenticated = true;

    if (!mAuthenticated)
    {
        // Start trying from the beginning
        mConsecutiveAuthenticationFailures = 0;
        beginAuthentication();
    }
    else
    {
        // We already have a Web UI session
        // Skip the dispatcher
        loadTrustedUrl(url);
    }
}

void AuthenticatedWebUIController::beginAuthentication()
{
    // Request a fresh exchange token
    SSOTicketServiceResponse *response = SSOTicketServiceClient::exchangeTicket(mSession);
    ORIGIN_VERIFY_CONNECT(response, SIGNAL(success(QString)), this, SLOT(onExchangeTokenSuccess(QString)));
    ORIGIN_VERIFY_CONNECT(response, SIGNAL(error(Origin::Services::restError)), this, SLOT(onExchangeTokenFailed(Origin::Services::restError)));

    response->setDeleteOnFinish(true);
}
	
void AuthenticatedWebUIController::onExchangeTokenSuccess(QString exchangeToken)
{
    // Build a request to the dispatcher
    WebDispatcherRequestBuilder::WebDispatcherRequest dispatchRequest = WebDispatcherRequestBuilder::buildRequest(exchangeToken, mRedirectUrl);

    // Load it in the web view
    loadTrustedRequest(dispatchRequest.networkRequest, dispatchRequest.operation, dispatchRequest.body);
}

void AuthenticatedWebUIController::onExchangeTokenFailed(Origin::Services::restError)
{
	SSOTicketServiceResponse* response = dynamic_cast<SSOTicketServiceResponse*>(sender());
	ORIGIN_LOG_ERROR << "SSOTicketServiceResponse REST error [" << response->error() << "] errorString [" << response->errorString() << "] HTTP Status [" << response->httpStatus() << "]";
    // Not much we can do
    errorHandler()->showPageError(PageErrorDetector::FinishedWithFailure);
}

void AuthenticatedWebUIController::handleNetworkReply(QNetworkReply *reply)
{
    const QUrl replyUrl = reply->url();

    if ((replyUrl != page()->mainFrame()->url()) &&
        (replyUrl != page()->mainFrame()->requestedUrl()))
    {
        // This is for another resource on the page, AJAX request etc
        // Only assume base pages are checking authentication
        return;
    }

    // Only care about requests to the redirect URL or .origin.com
    if ((replyUrl.host().compare(mRedirectUrl.host(), Qt::CaseInsensitive) == 0) ||
        (replyUrl.host().endsWith(".origin.com", Qt::CaseInsensitive)))
    {
        if (reply->error() == QNetworkReply::NoError)
        {
            // 200-like response; assume we're authenticated
            mConsecutiveAuthenticationFailures = 0;
            mAuthenticated = true;
        }
        else if (reply->error() == QNetworkReply::OperationCanceledError)
        {
            // Qt uses this for 304s - ignore
            return;
        }
        else
        {
            const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            if (statusCode == Services::Http403ClientErrorForbidden && !mSkipAuthentication)
            {
                mConsecutiveAuthenticationFailures++;
                mAuthenticated = false;

                if (mConsecutiveAuthenticationFailures <= MaximumConsecutiveAuthenticationFailures)
                {
                    // Attempt to reauthenticate
                    beginAuthentication();
                    // Don't show an error page
                    return;
                }
            }
            
            errorHandler()->showPageError(PageErrorDetector::MainFrameRequestFailed, reply);
        }
    }
}


}
}
