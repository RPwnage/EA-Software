//////////////////////////////////////////////////////////////////////////////
// PDLCJsHelper.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef PDLCJSHELPER_H
#define PDLCJSHELPER_H

#include <QObject>
#include <QUrl>

#include "MyGameJsHelper.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            class ContentController;
        }
    }

namespace Client
{
class PDLCView;
class ORIGIN_PLUGIN_API PDLCJsHelper : public MyGameJsHelper
{
    Q_OBJECT
public:
    /// \brief The PDLCJsHelper constructor.
    PDLCJsHelper(QObject* parent = NULL);

    /// \brief The PDLCJsHelper destructor.
	~PDLCJsHelper();

    /// \brief Stores the commerce profile for URL generation.
    /// \param profile The commerce profile.
	void setCommerceProfile(const QString& profile) {mCommerceProfile = profile;}
    
    /// \brief Stores the sku for URL generation.
    /// \param contentId The contentId.
	void setSku(const QString& sku) {mSku = sku;}
    
    /// \brief Stores the context for URL generation.
    /// \param context The context.
	void setContext(const QString& context) {mContext = context;}

    /// \brief Retrieves the commerce profile.
    /// \return The commerce profile.
    const QString &commerceProfile() const {return mCommerceProfile;}

public slots:
    /// \brief Leads to the Lockbox page to buy the product.
    void purchase(const QString& productId);

    /// \brief Refresh entitlements and let the SDK know the purchase succeeded.
    /// Used by Lockbox2.
    void purchaseSucceeded(const QString& productId = QString());
    
    /// \brief Refresh entitlements and let the SDK know the purchase succeeded.
    /// Used by Lockbox3.
    void refreshEntitlements();

    /// \brief Starts the download for the given entitlement.
    /// \param productId Product ID of the entitlement.
    void startDownload(const QString& productId);
    
    /// \brief Starts downloading all entitlements with matching entitlementTag and groupName.
    /// \param entitlementTags Comma-delimited list of entitlementTags.
    /// \param groupNames Comma-delimited list of groupNames.
	void startDownloadEx(const QString& entitlementTags, const QString& groupNames);

    /// \brief Opens the URL in an external browser, or in the IGO browser if we're accessed from in-game.
    /// \param urlstring The URL to redirect the user to.
    void launchExternalBrowser(QString urlstring);

    /// \brief Close the PDLC view. Called from java script.
    void close();

    /// \brief Allow the user to continue shopping.
    /// \param url The URL to redirect the user to, if any.
    void continueShopping(const QString& url = QString());
    
    /// \brief Opens a print dialog and prints the document if the user accepts.
    void print();

signals:
    /// \brief Closes the PDLC window.
    void closeStore();

    /// \brief Opens a print dialog and prints the document if the user accepts.
    void printRequested();

private:
	QString mSku;
    QString mCommerceProfile;
    QString mContext;
    PDLCView *mParent;
};
}
}
#endif