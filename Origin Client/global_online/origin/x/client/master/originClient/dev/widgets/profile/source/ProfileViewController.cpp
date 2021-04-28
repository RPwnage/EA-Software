#include <QWebFrame>
#include "ProfileViewController.h"

#include "WebWidget/WidgetView.h"
#include "WebWidget/WidgetPage.h"
#include "chat/OriginConnection.h"
#include "chat/JabberID.h"

#include "engine/social/SocialController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/network/NetworkAccessManager.h"

#include "NativeBehaviorManager.h"
#include "WebWidgetController.h"
#include "OriginSocialProxy.h"
#include "SocialUserProxy.h"
#include "NavigationController.h"
#include "engine/login/LoginController.h"

namespace Origin
{
    namespace Client
    {

        ProfileViewController::ProfileViewController(UIScope context, const quint64& initialNucleusId, QWidget* parent)
            : ViewControllerBase(context, SOCIAL_TAB, parent)
            ,mNucleusId(initialNucleusId)
        {

            ORIGIN_VERIFY_CONNECT(mView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(populatePageJavaScriptWindowObject()));

            bool isWidgetLoaded = 
            WebWidgetController::instance()->loadUserSpecificWidget(
                mView->widgetPage(),
                "profile",
                Engine::LoginController::currentUser(), 
                widgetLinkForNucleusId(initialNucleusId)
                );

            Q_UNUSED(isWidgetLoaded);
            ORIGIN_ASSERT(isWidgetLoaded);
        }

        QUrl ProfileViewController::startUrl() const
        {
            WebWidget::ForwardedWidgetPageView* fw = static_cast<WebWidget::ForwardedWidgetPageView*>(mView);
            return fw->widgetPage()->startUrl();
        }

        QWebHistory* ProfileViewController::history() const
        {
            WebWidget::ForwardedWidgetPageView* fw = static_cast<WebWidget::ForwardedWidgetPageView*>(mView);
            return fw->widgetPage()->history();
        }

        quint64 ProfileViewController::nucleusIdByUserId(const QString& id)
        {
            quint64 nucleusId = -1;
#if 0 //disable for Origin X
            JsInterface::OriginSocialProxy* social = JsInterface::OriginSocialProxy::instance();
            if (social)
            {
                nucleusId = social->nucleusIdByUserId(id);
            }
#endif
            return nucleusId;
        }



        bool ProfileViewController::showProfileForNucleusId(quint64 nucleusId)
        {
            // If our NucleusId is -1 return false
            if (nucleusId == -1 )
                return false;
            // Always reload the page, even if we are looking at the same user, as there may be some "interesting" javascript that may have 
            // entirely changed the content (ie when doing a search from inside the profile page)
            mNucleusId = nucleusId;
            return mView->loadLink(widgetLinkForNucleusId(mNucleusId), true);
        }

        void ProfileViewController::populatePageJavaScriptWindowObject()
        {
            mView->page()->mainFrame()->addToJavaScriptWindowObject("helper", this);
        }

        WebWidget::WidgetLink ProfileViewController::widgetLinkForNucleusId(quint64 nucleusId)
        {

            using namespace WebWidget;
            UriTemplate::VariableMap profileVars;
#if 0 //disable for Origin X
            JsInterface::OriginSocialProxy* social = JsInterface::OriginSocialProxy::instance();
            // If we do not have an OriginSocialProxy object return an empty WidgetLink
            if (!social)
            {
                return WidgetLink();
            }
            profileVars["id"] = social->userIdByNucleusId(nucleusId);
#endif
            return WidgetLink("http://widgets.dm.origin.com/linkroles/profiledetails", profileVars);
        }

        quint64 ProfileViewController::nucleusId() const
        {
            return mNucleusId;
        }

        void ProfileViewController::resetNucleusId()
        {
            mNucleusId = 0;
        }

    }
}
