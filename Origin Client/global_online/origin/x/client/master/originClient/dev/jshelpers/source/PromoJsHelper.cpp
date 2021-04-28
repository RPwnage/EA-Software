/////////////////////////////////////////////////////////////////////////////
// PromoJsHelper.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "PromoJsHelper.h"
#include <QUrl>
#include "flows/ClientFlow.h"
#include "services/session/SessionService.h"
#include "services/platform/PlatformService.h"
#include "engine/login/LoginController.h"
#include "engine/content//ContentController.h"

// Legacy
#include "EbisuSystemTray.h"

namespace Origin
{
	namespace Client
	{
		PromoJsHelper::PromoJsHelper(QObject *parent) : QObject(parent){ }

		void PromoJsHelper::viewProductOnStoreTab( const QString& productId )
		{
            ClientFlow::instance()->showProductIDInStore(productId);
            EbisuSystemTray::instance()->showPrimaryWindow();
		}

		void PromoJsHelper::viewUrlOnStoreTab( const QString& path )
		{
            ClientFlow::instance()->showUrlInStore(QUrl(path));
            EbisuSystemTray::instance()->showPrimaryWindow();
		}

		void PromoJsHelper::launchBrowserWithSingleSignOn(QString url)
		{
            QString urlToUse = url.arg( Services::Session::SessionService::accessToken(Engine::LoginController::instance()->currentUser()->getSession()) );
			Origin::Services::PlatformService::asyncOpenUrl(urlToUse);
		}

        void PromoJsHelper::viewGamesDetailPage(const QString& ids)
        {
            QStringList idList = ids.split(',', QString::SkipEmptyParts);
            Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();
            if(contentController)
            {
                Origin::Engine::Content::EntitlementRef entitlement = contentController->bestMatchedEntitlementByIds(idList, Engine::Content::ContentController::EntById, true);
                if(entitlement)
                {
                    if(ClientFlow::instance())
                    {
                        ClientFlow::instance()->showGameDetails(entitlement);
                    }

                    if(EbisuSystemTray::instance())
                    {
                        EbisuSystemTray::instance()->showPrimaryWindow();
                    }
                }
            }
        }

        void PromoJsHelper::close()
        {
            emit closeWindow();
        }

	}
}
