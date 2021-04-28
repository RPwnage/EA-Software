///////////////////////////////////////////////////////////////////////////////
// CustomerSupportJsHelper.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "CustomerSupportJsHelper.h"
#include "services/debug/DebugService.h"
#include "services/platform/PlatformService.h"
#include "engine/igo/IGOController.h"
#include <QWebView>
#include <QWebHistory>

namespace Origin
{
    namespace Client
    {
        CustomerSupportJsHelper::CustomerSupportJsHelper(Scope location, QObject* parent)
            : QObject(parent)
            , mScope(location)
        {
            mParentWebView = static_cast<QWebView*>(parent);
        }

        bool CustomerSupportJsHelper::canNavigatePrevious()
        {
            bool retFlag = false;
            if(mParentWebView)
            {
                retFlag = mParentWebView->history()->canGoBack();
            }

            return retFlag;
        }

        bool CustomerSupportJsHelper::canNavigateNext()
        {
            bool retFlag = false;

            if(mParentWebView)
            {
                retFlag = mParentWebView->history()->canGoForward();
            }

            return retFlag;
        }

        void CustomerSupportJsHelper::openLink(QString url)
        {
            if(mScope == IGO)
                emit Engine::IGOController::instance()->showBrowser(url, false);
            else
                Origin::Services::PlatformService::asyncOpenUrl(QUrl(url));
        }

    }
}