/////////////////////////////////////////////////////////////////////////////
// EmailVerificationMessageAreaViewController.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "services/rest/SignInPortalServiceClient.h"
#include "services/rest/SignInPortalServiceResponses.h"
#include "EmailVerificationMessageAreaViewController.h"
#include "ClientMessageAreaView.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/debug/DebugService.h"
#include "services/rest/IdentityPortalServiceClient.h"
#include "engine/login/LoginController.h"
#include "MainFlow.h"
#include "RTPFlow.h"
#include "OriginApplication.h"
#include "TelemetryAPIDLL.h"
#include <QPushButton>
#include <QWidget>
#include "originwindow.h"
#include "originscrollablemsgbox.h"
#include "originpushbutton.h"


namespace Origin
{
namespace Client
{

const int NETWORK_TIMEOUT_MSECS = 30000;

EmailVerificationMessageAreaViewController::EmailVerificationMessageAreaViewController(QObject* parent)
: MessageAreaViewControllerBase(EmailVerification, parent)
, mClientMessageAreaView(NULL)
, mEmailVerified(false)
{
    queryInfo();
}


EmailVerificationMessageAreaViewController::~EmailVerificationMessageAreaViewController()
{
    if(mClientMessageAreaView)
    {
        delete mClientMessageAreaView;
        mClientMessageAreaView = NULL;
    }
}


QWidget* EmailVerificationMessageAreaViewController::view()
{
    if(mClientMessageAreaView == NULL)
    {
        mClientMessageAreaView = new ClientMessageAreaView();
        mClientMessageAreaView->setTitle(tr("ebisu_client_please_verify_your_email_allcaps"));
        mClientMessageAreaView->setText(tr("ebisu_client_know_its_you"));
        mClientMessageAreaView->firstButton()->setText(tr("ebisu_client_send_verification"));
        mClientMessageAreaView->secondButton()->setText(tr("ebisu_client_dismiss"));
        ORIGIN_VERIFY_CONNECT_EX(mClientMessageAreaView->firstButton(), SIGNAL(clicked()), this, SLOT(onSetQuestionNowSelected()), Qt::UniqueConnection);
        ORIGIN_VERIFY_CONNECT_EX(mClientMessageAreaView->secondButton(), SIGNAL(clicked()), this, SIGNAL(dismiss()), Qt::UniqueConnection);
        ORIGIN_VERIFY_CONNECT_EX(mClientMessageAreaView->secondButton(), SIGNAL(clicked()), this, SLOT(onDismissQuestionSelected()), Qt::UniqueConnection);
    }

    return mClientMessageAreaView;
}


void EmailVerificationMessageAreaViewController::queryInfo()
{
    QString accessToken = Services::Session::SessionService::accessToken(Engine::LoginController::currentUser()->getSession());
    Services::IdentityPortalUserResponse* response = Services::IdentityPortalServiceClient::retrieveUserInfoByToken(accessToken);
    if (response)
    {
        ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onInfoReceived()));
        QTimer::singleShot(NETWORK_TIMEOUT_MSECS, response, SLOT(abort()));
    }
}


bool EmailVerificationMessageAreaViewController::okToShowMessage()
{
    bool show = true;

    // Don't show under any of these cases:
    // Security question is set
    if(mEmailVerified
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


void EmailVerificationMessageAreaViewController::showMessageSentWindow()
{
    using namespace UIToolkit;
    UIToolkit::OriginWindow* window = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
        NULL, OriginWindow::ScrollableMsgBox, QDialogButtonBox::Close);
    window->scrollableMsgBox()->setup(OriginScrollableMsgBox::NoIcon, 
        tr("ebisu_client_verification_email_sent_allcaps"), 
        tr("ebisu_client_verification_email_sent_finish").arg(Origin::Engine::LoginController::currentUser()->email()) + "<br><br>" +tr("ebisu_client_verification_email_check_spam") + "<br><br>" + tr("ebisu_client_email_change_another_verification") + "<br><br>");
    ORIGIN_VERIFY_DISCONNECT(window, SIGNAL(closeWindow()), window, SIGNAL(rejected()));
    ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Close), SIGNAL(clicked()), window, SLOT(reject()));
    ORIGIN_VERIFY_CONNECT(window, SIGNAL(closeWindow()), window, SLOT(reject()));
    ORIGIN_VERIFY_CONNECT(Engine::LoginController::instance(), SIGNAL(userLoggedOut(Origin::Engine::UserRef)), window, SLOT(close()));
    window->showWindow();
}


int EmailVerificationMessageAreaViewController::showMessageErrorWindow()
{
    return UIToolkit::OriginWindow::prompt(UIToolkit::OriginMsgBox::Error, 
        tr("ebisu_client_unable_to_send_emailverification_caps"), 
        tr("ebisu_client_unable_to_send_emailverification_description"),
        tr("ebisu_client_retry"), tr("ebisu_client_close"), QDialogButtonBox::Yes);
}


void EmailVerificationMessageAreaViewController::onInfoReceived()
{
    Services::IdentityPortalUserResponse* response = dynamic_cast<Services::IdentityPortalUserResponse*>(sender());
    if(response)
    {
        ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(onInfoReceived()));
        mEmailVerified = response->emailStatus().compare("VERIFIED",Qt::CaseInsensitive) == 0;
        QString dateCreated = response->dateCreated();
        QString dateMod = response->dateModified();
        response->setDeleteOnFinish(true);
        emit responseReceived();
    }
}

void EmailVerificationMessageAreaViewController::onSendEmailRequestComplete()
{
    bool success = false;

    Services::SignInPortalEmailVerificationResponse* response = dynamic_cast<Services::SignInPortalEmailVerificationResponse*>(sender());

    if(response)
    {
        ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(onSendEmailRequestComplete()));
        int httpResponse = response->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if((httpResponse == Origin::Services::Http200Ok) && response->requestSent() )
        {
            success = true;
        }
    }

    if(success)
    {
        emit showAccountSettings();
        showMessageSentWindow();
    }       
    else
    {
        if(showMessageErrorWindow() == QDialog::Accepted)
        {
            sendVerificationEmailRequest();
        }
    }
}
void EmailVerificationMessageAreaViewController::sendVerificationEmailRequest()
{
    QString accessToken = Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession());

    Services::SignInPortalEmailVerificationResponse*response = Origin::Services::SignInPortalServiceClient::sendEmailVerificationRequest(Engine::LoginController::instance()->currentUser()->getSession(), accessToken);
    ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onSendEmailRequestComplete()));
}

void EmailVerificationMessageAreaViewController::onSetQuestionNowSelected()
{
    sendVerificationEmailRequest();
    GetTelemetryInterface()->Metric_EMAIL_VERIFICATION_STARTS_CLIENT();
}

void EmailVerificationMessageAreaViewController::onDismissQuestionSelected()
{
    GetTelemetryInterface()->Metric_EMAIL_VERIFICATION_DISMISSED();
}

}
}