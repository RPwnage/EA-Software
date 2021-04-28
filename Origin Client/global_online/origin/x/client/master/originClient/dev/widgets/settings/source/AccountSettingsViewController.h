/////////////////////////////////////////////////////////////////////////////
// AccountSettingsViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef ACCOUNT_SETTINGS_VIEW_CONTROLLER_H
#define ACCOUNT_SETTINGS_VIEW_CONTROLLER_H

#include "services/network/AuthenticationMonitor.h"
#include "services/plugin/PluginAPI.h"

#include <QObject>
#include <QUrl>

namespace Origin
{
namespace UIToolkit
{
class OriginWebView;
}
namespace Client
{
class OriginWebController;

class ORIGIN_PLUGIN_API AccountSettingsViewController : public QObject
{
	Q_OBJECT

public:
    /// \brief List of valid URLs to navigate to.
    enum Path
    {
        Home = 0, ///< Home page of account portal
        Privacy, ///< Privacy settings page
        Security, ///< Security settings page
        Subscription, ///< Subscription page
        OrderHistory, ///< Order history page
        PaymentShipping, ///< Payment & Shipping page
        Redemption, ///< Redemption page
        Unset = -1  ///< Special value to indicate no URL has been navigated to
    };

    /// \brief Constructor
    /// \param parent The parent of the AccountSettingsViewController
    AccountSettingsViewController(QWidget* parent = 0);

    /// \brief Destructor
    ~AccountSettingsViewController();

    /// \brief Returns the widget to be shown in the client view
    QWidget* view() const;

    /// \brief Loads the account web page if the current web page isn't already loaded
    /// \brief path Path to navigate to, if any.
    void loadWebPage(AccountSettingsViewController::Path path = AccountSettingsViewController::Home, const QString& status = "");
    
    /// \brief Returns the URL of the currently loaded web page.
    AccountSettingsViewController::Path currentPath() const { return mCurrentPath; }

public slots:
    /// \brief Refreshes the account settings page if the path is the current. If it's unset it will always refresh
    void refreshPage(const AccountSettingsViewController::Path& path = AccountSettingsViewController::Unset);

signals:

    // \brief Emitted when the accounts page receives a 403 error
    void lostAuthentication();

protected slots:
    /// \brief Called when the web page is changed. Saves the new url to our global navigation
    void onUrlChanged(const QUrl& url);

    /// \brief Called when the authentication monitor encounters a 403 error
    void onLostAuthentication();

    /// \brief Called when the verify email link is clicked
    void onEmailClicked();


private:
    OriginWebController* mWebController;
    UIToolkit::OriginWebView* mWebViewContainer;
    AccountSettingsViewController::Path mCurrentPath;
    QString mHomePage;
    QString mOrderHistoryPage;
    QString mPrivacyPage;
    QString mSecurityPage;
    QString mPaymentShippingPage;
    QString mSubscriptionPage;
    QString mRedemptionPage;

    Services::AuthenticationMonitor mAuthenticationMonitor;
};

}
}
#endif