///////////////////////////////////////////////////////////////////////////////
// StoreView.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef STOREVIEW_H
#define STOREVIEW_H

#include <QString>
#include <QUrl>
#include <QTime>
#include "QWebView.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{

class OriginWebController;
class ExposedWidgetDetector;

class ORIGIN_PLUGIN_API StoreView : public QWebView
{
    Q_OBJECT

public:
    StoreView(QWidget *parent = 0);
    
    void loadStoreUrl(const QUrl &);

    // Uses the default initial page
    void loadEntryUrlAndExpose(const QUrl &url);

    void showError(const QString& title, const QString& error);

protected slots:
    void onStoreViewLoadStarted();
    void onStoreViewLoadFinished(bool success);
    
    void exposed();
    void stopAnimations();
    void onPrintRequested();

protected:
    void load(const QUrl& url);

    // Used to defer loading until we're shown
    bool mHasBeenExposed;
    QUrl mLoadOnExpose;

    OriginWebController *mWebController;
    ExposedWidgetDetector *mExposureDetector;
    
    /// \brief Used to measure how long it takes to load the store
    /// \sa loadStarted(), loadFinished()
    QTime mStorePageLoadTimer;

    void init();
};

}
}

#endif // STOREVIEW_H
