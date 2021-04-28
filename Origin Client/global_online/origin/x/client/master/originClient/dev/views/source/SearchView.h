///////////////////////////////////////////////////////////////////////////////
// SearchView.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef SEARCHVIEW_H
#define SEARCHVIEW_H

#include <QWebView.h>
#include "OriginWebPage.h"
#include "UIScope.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{

class OriginWebController;

class ORIGIN_PLUGIN_API SearchWebPage : public OriginWebPage
{
    Q_OBJECT
public:
    SearchWebPage(const UIScope& scope, QObject* parent);
    QWebPage* createWindow(QWebPage::WebWindowType);

private slots:
    void onUrlChanged(const QUrl& url);
private:
    SearchWebPage(const SearchWebPage&);
    SearchWebPage& operator=(const SearchWebPage&);
    const UIScope mScope;
};



class ORIGIN_PLUGIN_API SearchView : public QWebView
{
public:
    SearchView(const UIScope& scope, QWidget* parent = 0);
    ~SearchView();
    void loadTrustedUrl(const QString&);
    void showError(const QString& title, const QString& error);

private:
    SearchView(const SearchView&);
    SearchView& operator=(const SearchView&);

    OriginWebController* mWebController;
    const UIScope mScope;
};

} // Client
} // Origin

#endif // STOREVIEW_H