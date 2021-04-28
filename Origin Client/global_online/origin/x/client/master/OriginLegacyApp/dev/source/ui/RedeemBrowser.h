#ifndef REDEEMBROWSER_H
#define REDEEMBROWSER_H


#include "Qt/originwebview.h"
#include "PageErrorDetector.h"
#include "engine/login/LoginController.h"
#include "services/plugin/PluginAPI.h"

class QUrl;

namespace Origin
{
namespace Client
{
    class OriginWebController;
}
}

class ORIGIN_PLUGIN_API RedeemBrowser : public Origin::UIToolkit::OriginWebView
{
    Q_OBJECT

public:
    enum SourceType{Origin, ITE, RTP, NUM_SOURCE_TYPES};
    enum RequestorID{OriginCodeClient, OriginCodeITE, OriginCodeRTP, OriginCodeWeb};
    RedeemBrowser(QWidget *parent = NULL, SourceType sourceType = Origin, RequestorID requestorID = OriginCodeWeb, QFrame *otherFrame = NULL, const QString &code = QString());
    void loadRedeemURL();
    ~RedeemBrowser();

signals:
    void closeWindow();

protected:
    void changeEvent( QEvent* event );

    /// \brief URL used for activation/redemption UI
    ///
    /// \warning This method constructs the activation URL and percent-encodes
    ///          the "gameName" attribute specifically to avoid formatting 
    ///          issues with some game titles. Therefore, this method should never 
    ///          return a string representation of the URL, as QUrl::toString 
    ///          will remove any such encoding.
    QUrl activationUrl() const;

protected slots:
    void requestFinished(QNetworkReply *reply);

public slots:
    void onCloseWindow();

private:
    SourceType mSourceType;
    RequestorID mRequestorID;
    Origin::Client::OriginWebController *mController;
    QString mRedemptionCode;
    ///
    /// returns sourceType
    QString srcType() const;

    ///
    /// adds returnURL
    QString returnUrl() const;
};

#endif
