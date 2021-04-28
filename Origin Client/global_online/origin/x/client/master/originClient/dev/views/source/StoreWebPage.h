///////////////////////////////////////////////////////////////////////////////
// StoreWebPage.h
//
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef STOREWEBPAGE_H
#define STOREWEBPAGE_H

#include "services/plugin/PluginAPI.h"
#include "OriginWebPage.h"

class QWebPage;

namespace Origin
{
namespace Client
{

class ORIGIN_PLUGIN_API StoreWebPage : public OriginWebPage
{
    Q_OBJECT

public:
    StoreWebPage(QObject* parent = 0);
    QWebPage* createWindow(QWebPage::WebWindowType);
    void setUseWhiteList(bool use) { mUseWhiteList = use; }

protected:
    bool eventFilter(QObject* obj, QEvent* e);

protected slots:
    void onUrlChanged(const QUrl& url);


private:
    bool urlInWhiteList(const QString& url);
    bool urlInBlackList(const QString& url);
    QStringList mUrlBlackList;
    bool mUseWhiteList;
};

}
}

#endif