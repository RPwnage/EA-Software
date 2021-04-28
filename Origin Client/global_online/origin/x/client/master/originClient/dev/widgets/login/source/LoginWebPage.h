///////////////////////////////////////////////////////////////////////////////
// LoginWebPage
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef LOGINWEBPAGE_H
#define LOGINWEBPAGE_H

#include <QWebView>
#include "OriginWebPage.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{

class OriginWebController;

class ORIGIN_PLUGIN_API LoginWebPage : public OriginWebPage
{
    Q_OBJECT
public:
    LoginWebPage(QObject* parent);
    QWebPage* createWindow(QWebPage::WebWindowType);
    //overridden virtual functions for extending the QWebPage
    bool supportsExtension(Extension extension) const;
    bool extension(Extension extension, const ExtensionOption * option, ExtensionReturn * output);

private slots:
    void onLoadStarted();

signals:
    void onRedirectionLoopDetected();

private:
};


} // Client
} // Origin

#endif // LOGINWEBPAGE_H
