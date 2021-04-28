//    Copyright © 2012 Electronic Arts
//    All rights reserved.

#include "services/debug/DebugService.h"
#include "engine/login/LoginController.h"
#include "services/network/NetworkAccessManager.h"
#include "SearchViewController.h"
#include "NavigationController.h"
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
        namespace
        {
            const QString SearchDetailsLinkRole("http://widgets.dm.origin.com/linkroles/searchdetails");
        }

        SearchViewController::SearchViewController(UIScope context, QWidget *parent)
            : ViewControllerBase(context, SEARCH_CONTENT, parent)
        {

            bool isWidgetLoaded = 
                WebWidgetController::instance()->loadUserSpecificWidget(
                mView->widgetPage(),"profile",
                Engine::LoginController::instance()->currentUser());

            Q_UNUSED(isWidgetLoaded);
            ORIGIN_ASSERT(isWidgetLoaded);
        }

        WebWidget::WidgetLink SearchViewController::widgetLinkForSearch(const QString& keyword)
        {
            using namespace WebWidget;
            UriTemplate::VariableMap searchVars;

            searchVars["keyword"] = keyword;

            return WidgetLink(SearchDetailsLinkRole, searchVars);
        }

        bool SearchViewController::isLoadSearchPage( const QString keyword )
        {
            return mView->loadLink(widgetLinkForSearch(keyword));
        }

    }
}
