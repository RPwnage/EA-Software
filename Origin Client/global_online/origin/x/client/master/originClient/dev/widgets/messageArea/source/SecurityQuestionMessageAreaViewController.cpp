/////////////////////////////////////////////////////////////////////////////
// SecurityQuestionMessageAreaViewController.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "SecurityQuestionMessageAreaViewController.h"
#include "ClientMessageAreaView.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/debug/DebugService.h"
#include "services/rest/IdentityPortalServiceClient.h"
#include "MainFlow.h"
#include "RTPFlow.h"
#include "OriginApplication.h"
#include "TelemetryAPIDLL.h"
#include <QPushButton>
#include <QWidget>

namespace Origin
{
namespace Client
{

SecurityQuestionMessageAreaViewController::SecurityQuestionMessageAreaViewController(QObject* parent)
: MessageAreaViewControllerBase(SecurityQuestion, parent)
, mClientMessageAreaView(NULL)
, mSecurityQASet(false)
{
    querySecurityQuestionInfo();
}


SecurityQuestionMessageAreaViewController::~SecurityQuestionMessageAreaViewController()
{
    if(mClientMessageAreaView)
    {
        delete mClientMessageAreaView;
        mClientMessageAreaView = NULL;
    }
}


QWidget* SecurityQuestionMessageAreaViewController::view()
{
    if(mClientMessageAreaView == NULL)
    {
        mClientMessageAreaView = new ClientMessageAreaView();
        mClientMessageAreaView->setTitle(tr("ebisu_client_protect_your_account_caps"));
        mClientMessageAreaView->setText(tr("ebisu_client_security_question_description").arg(tr("application_name")));
        mClientMessageAreaView->firstButton()->setText(tr("ebisu_client_set_your_question"));
        mClientMessageAreaView->secondButton()->setText(tr("ebisu_client_dismiss"));
        ORIGIN_VERIFY_CONNECT_EX(mClientMessageAreaView->firstButton(), SIGNAL(clicked()), this, SIGNAL(showAccountSettings()), Qt::UniqueConnection);
        ORIGIN_VERIFY_CONNECT_EX(mClientMessageAreaView->firstButton(), SIGNAL(clicked()), this, SLOT(onSetQuestionNowSelected()), Qt::UniqueConnection);
        ORIGIN_VERIFY_CONNECT_EX(mClientMessageAreaView->secondButton(), SIGNAL(clicked()), this, SIGNAL(dismiss()), Qt::UniqueConnection);
        ORIGIN_VERIFY_CONNECT_EX(mClientMessageAreaView->secondButton(), SIGNAL(clicked()), this, SLOT(onDismissQuestionSelected()), Qt::UniqueConnection);
    }

    return mClientMessageAreaView;
}


void SecurityQuestionMessageAreaViewController::querySecurityQuestionInfo()
{
    Services::IdentityPortalUserProfileUrlsResponse* response = NULL;
    Engine::UserRef user = Engine::LoginController::currentUser();
    QString accessToken = Services::Session::SessionService::accessToken(user->getSession());
    qint64 nucleusUser = user->userId();
    response = Services::IdentityPortalServiceClient::retrieveUserProfileUrls(accessToken, nucleusUser, Services::IdentityPortalUserProfileUrlsResponse::SecurityQA);
    ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onSecurityQAReceived()));
}


bool SecurityQuestionMessageAreaViewController::okToShowMessage()
{
    bool show = true;

    // Don't show under any of these cases:
    // Security question is set
    if(mSecurityQASet
    // User underage
       || Engine::LoginController::instance()->currentUser()->isUnderAge()
    // User not authenticated
       || Services::Connection::ConnectionStatesService::instance()->isAuthenticated(Engine::LoginController::instance()->currentUser()->getSession()) == false
    // ITO Active
       || OriginApplication::instance().isITE()
    // RTP Active 
       || MainFlow::instance()->rtpFlow()->getRtpLaunchActive() || MainFlow::instance()->rtpFlow()->isPending())
    {
        show = false;
    }

    return show;
}


void SecurityQuestionMessageAreaViewController::onSecurityQAReceived()
{
    Services::IdentityPortalUserProfileUrlsResponse* response = dynamic_cast<Services::IdentityPortalUserProfileUrlsResponse*>(sender());
    if(response)
    {
        mSecurityQASet = response->profileInfo()[Services::IdentityPortalUserProfileUrlsResponse::SecurityQA].isEmpty() == false;
        response->setDeleteOnFinish(true);
        emit responseReceived();
    }
}

void SecurityQuestionMessageAreaViewController::onSetQuestionNowSelected()
{
    GetTelemetryInterface()->Metric_SECURITY_QUESTION_SET();
}

void SecurityQuestionMessageAreaViewController::onDismissQuestionSelected()
{
    GetTelemetryInterface()->Metric_SECURITY_QUESTION_CANCEL();
}

}
}