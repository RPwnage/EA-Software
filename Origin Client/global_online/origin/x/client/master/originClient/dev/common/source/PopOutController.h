/////////////////////////////////////////////////////////////////////////////
// PopOutController.h
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef POPOUT_CONTROLLER_H
#define POPOUT_CONTROLLER_H
#include <QObject>
#include <QMap>
#include "services/plugin/PluginAPI.h"
#include <QJsonObject>
#include <QtWebKitWidgets/QWebPage>
#include "originwindow.h"

namespace Origin
{
namespace UIToolkit {
class OriginWindow;
}
namespace Client
{
class OriginWebController;

class ORIGIN_PLUGIN_API PopOutController : public QObject
{
    Q_OBJECT
public:
    enum DialogMode
    {
        SPA = 0,
        WebWidget
    };

    static void create();
    static PopOutController *instance();
    static void destroy();
    static const char* popOutWindowUrlKey() {return "PopOutWindowUrl";}
    QWebPage* createJSWindow(const QUrl& url);
    UIToolkit::OriginWindow* chatWindow();
    void moveWindowToFront(const QString& url);

signals:
    void closed(QString);
public slots:
    void onClosed();
    void onPopInWindow(QString);
    void onZoomed();
private:
    QMap<QString, QSize> mSizeMap;
    QMap<QString, QSize> mMinSizeMap;
    QMap<QString, UIToolkit::OriginWindow*> mWindows;
    PopOutController();
    ~PopOutController();
};

}
}
#endif //POPOUT_CONTROLLER_H