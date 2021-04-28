#include "CustomerSupportDialog.h" 
#include "jshelpers/source/CommonJsHelper.h"
#include "jshelpers/source/CustomerSupportJsHelper.h"
#include "OriginApplication.h"

#include "OriginWebController.h"

#include "Qt/originspinner.h"

#include "engine/content/ContentController.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/login/LoginController.h"
#include "engine/igo/IGOController.h"
#include  "UIScope.h"

#include "services/log/LogService.h"

#include <limits>
#include <QDateTime>
#include <QDesktopServices>
#include <QtWebKitWidgets/QWebView>

CustomerSupportDialog::CustomerSupportDialog(const QString& baseCustomerSupportUrl, QWidget* parent, Scope scope) 
: Origin::UIToolkit::OriginWebView(parent)
, mHelper(NULL)
, mWebController(NULL)
, mScope(scope)
{
    using namespace Origin::Client;

    setHasSpinner(true);
    setIsContentSize(false);
    if(scope == DEFAULT && spinner())
        spinner()->setSpinnerType(Origin::UIToolkit::OriginSpinner::SpinnerLarge_Dark);
    
    mWebController = new OriginWebController(webview(), DefaultErrorPage::centerAlignedPage());

    mWebController->jsBridge()->addHelper("commonJsHelper", new CommonJsHelper(webview()));
    mHelper = new Origin::Client::CustomerSupportJsHelper((Origin::Client::CustomerSupportJsHelper::Scope)mScope, webview());
    mWebController->jsBridge()->addHelper("customerSupportHelper", mHelper);

    QStringList hosts;
    hosts << ".origin.com";
    hosts << ".ea.com";
    hosts << QUrl(baseCustomerSupportUrl).host();
    mAuthenticationMonitor.setRelevantHosts(hosts);
    mAuthenticationMonitor.setWebPage(mWebController->page());
    ORIGIN_VERIFY_CONNECT(&mAuthenticationMonitor, SIGNAL(lostAuthentication()), this, SLOT(onLostAuthentication()));
    mAuthenticationMonitor.start();

    mWebController->loadTrustedUrl(addCustomerUrlParameters(baseCustomerSupportUrl));

    setVScrollPolicy(Qt::ScrollBarAsNeeded);
    setHScrollPolicy(Qt::ScrollBarAsNeeded);
    if (scope == IGO)
    {
        if (Origin::Engine::IGOController::instance()->showWebInspectors())
            Origin::Engine::IGOController::instance()->showWebInspector(webview()->page());
    }
}


CustomerSupportDialog::~CustomerSupportDialog()
{
    mAuthenticationMonitor.shutdown();
    mWebController = NULL;
}

bool CustomerSupportDialog::checkShowCustomerSupportInBrowser(QString& customerSupportUrl, const QString& authCode)
{
    // This should be fixed - somebody decided that the install flow should also use the GetEbisuCustomerSupportInBrowser
    // function to determine the locales supported by the client for gray market support or something.  It's a bit of work to unwind
    // and should be fixed properly when OriginLegacyApp is finally completely retired.
    return OriginApplication::instance().GetEbisuCustomerSupportInBrowser(customerSupportUrl, authCode);
}

QString CustomerSupportDialog::addCustomerUrlParameters(const QString& baseCustomerSupportUrl) const
{
    Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();
    QString accessToken = Origin::Services::Session::SessionService::accessToken(session);	

    QString context("ORIGIN-CLIENT");
    if(mScope == IGO)
        context = "IGO";

    using namespace Origin;
    QUrl customerSupportUrl(baseCustomerSupportUrl);
    QUrlQuery customerSupportUrlQuery(customerSupportUrl);
    Engine::Content::ContentController * c = Engine::Content::ContentController::currentUserContentController();
    QList<Engine::Content::EntitlementRef> playing = c->entitlementByState(0, Engine::Content::LocalContent::Playing);

    if (playing.size())
    {
        const Engine::Content::ContentConfigurationRef c = playing.first()->contentConfiguration();
        customerSupportUrlQuery.addQueryItem("c_contentID", c->contentId());
        customerSupportUrlQuery.addQueryItem("o_offerID", c->productId());
        customerSupportUrlQuery.addQueryItem("o_gameTitle", c->displayName());
    }

    customerSupportUrlQuery.addQueryItem("context", context);

    QString locale = Origin::Services::readSetting(Origin::Services::SETTING_LOCALE);

    //CE expects a different code for norway
    if(locale.compare("no_no", Qt::CaseInsensitive) == 0)
        locale = "nb_NO";

    customerSupportUrlQuery.addQueryItem("locale", locale);
    customerSupportUrl.setQuery(customerSupportUrlQuery);
    return customerSupportUrl.toString(QUrl::FullyEncoded);
}

void CustomerSupportDialog::onLostAuthentication()
{
    ORIGIN_LOG_ERROR << "Customer support lost authentication.";
    mWebController->page()->triggerAction(QWebPage::Stop);
    emit lostAuthentication();
}
