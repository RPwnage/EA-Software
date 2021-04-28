#ifndef CUSTOMERSUPPORTDIALOG_H
#define CUSTOMERSUPPORTDIALOG_H

#include "Qt/originwebview.h"
#include "engine/login/LoginController.h"
#include "services/network/AuthenticationMonitor.h"
#include "services/plugin/PluginAPI.h"

namespace Origin 
{
    namespace UIToolkit
    {
        class OriginWindow;
    }
    namespace Client
    {
        class OriginWebController;
        class CustomerSupportJsHelper;
    }
}

class ORIGIN_PLUGIN_API CustomerSupportDialog : public Origin::UIToolkit::OriginWebView
{	
    Q_OBJECT 

public:
    enum Scope { DEFAULT, IGO };
    CustomerSupportDialog(const QString& baseCustomerSupportUrl, QWidget* parent = 0, Scope scope = DEFAULT);
    ~CustomerSupportDialog();
    Origin::Client::CustomerSupportJsHelper* helper() const {return mHelper;}

    static bool checkShowCustomerSupportInBrowser(QString& customerSupportUrl, const QString& authCode = QString());

protected slots:
    void onLostAuthentication();

private:
    QString addCustomerUrlParameters(const QString& baseCustomerSupportUrl) const;
    Origin::Client::CustomerSupportJsHelper* mHelper;
    Origin::Client::OriginWebController* mWebController;
    const Scope	mScope;
    Origin::Services::AuthenticationMonitor mAuthenticationMonitor;

signals:
    void openInIGOBrowser(const QString &url);
    void lostAuthentication();
};

#endif // CustomerSupport
