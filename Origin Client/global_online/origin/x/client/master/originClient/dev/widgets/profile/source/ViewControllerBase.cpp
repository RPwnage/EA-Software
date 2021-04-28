//    Copyright © 2012 Electronic Arts
//    All rights reserved.

#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "engine/login/LoginController.h"
#include "engine/igo/IGOController.h"
#include "services/network/NetworkAccessManager.h"
#include "ViewControllerBase.h"
#include "NativeBehaviorManager.h"
#include "WebWidget/WidgetView.h"
#include "WebWidget/WidgetPage.h"
#include "WebWidgetController.h"
#include "engine/login/User.h"

using namespace WebWidget;

namespace Origin
{
    using namespace Engine::Content;

    namespace Client
    {
        ViewControllerBase::ViewControllerBase(UIScope context, NavigationItem naviItem, QWidget *parent)
            : QObject(parent)
            , mView(new WebWidget::WidgetView())
            , mNaviItem(naviItem)
            , mContext(context)
        {
            mView->widgetPage()->setExternalNetworkAccessManager(Services::NetworkAccessManager::threadDefaultInstance());

            // Act and look more native
            NativeBehaviorManager::applyNativeBehavior(mView);

            // Register our history
            if(mContext!=IGOScope)
                NavigationController::instance()->addWebHistory(mNaviItem, mView->page()->history());

            ORIGIN_VERIFY_CONNECT(mView->page()->currentFrame(), SIGNAL(urlChanged(const QUrl&)), this, SLOT(onUrlChanged(const QUrl&)));
            
            if (context == IGOScope)
            {
                if (Origin::Engine::IGOController::instance()->showWebInspectors())
                    Origin::Engine::IGOController::instance()->showWebInspector(mView->page());
            }
        }

        ViewControllerBase::~ViewControllerBase()
        {
            ORIGIN_LOG_EVENT << "[information] ViewControllerBase DESTROYED";
        }

        void ViewControllerBase::onUrlChanged(const QUrl& url)
        {
            if(mContext!=IGOScope)
                NavigationController::instance()->recordNavigation(mNaviItem, url.toString());
        }

        WebWidget::WidgetView* ViewControllerBase::view() const
        {
            return mView;
        }
    }
}
