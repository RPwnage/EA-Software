#ifndef ORIGINWEBPAGE_H
#define ORIGINWEBPAGE_H

#include <QtWebKitWidgets/QWebPage>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{

class ORIGIN_PLUGIN_API OriginWebPage : public QWebPage
{
public:
    OriginWebPage(QObject *parent = NULL);

    static const char* CreateWindowUrlKey() { return "CreateWindowUrl"; }

protected:
    QString userAgentForUrl(const QUrl &url) const;
    QString chooseFile(QWebFrame * parentFrame, const QString &suggestedFile);
    QWebPage* createJSWindow(QUrl url);

    virtual QWebPage* createWindow(WebWindowType type);
    virtual bool acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type);
};

}
}

#endif
