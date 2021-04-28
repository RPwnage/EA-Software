/////////////////////////////////////////////////////////////////////////////
// MOTDJsHelper.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "MOTDJsHelper.h"

#include <QString>
#include "services/platform/PlatformService.h"
#include "services/session/SessionService.h"
#include "engine/login/LoginController.h"

namespace Origin
{
	namespace Client
	{
		MOTDJsHelper::MOTDJsHelper(QObject* parent) : QObject(parent){ }

		void MOTDJsHelper::launchSystemBrowser(QString url)
		{
            QString urlToUse = url.arg( Origin::Services::Session::SessionService::accessToken(Origin::Engine::LoginController::instance()->currentUser()->getSession()));
			Origin::Services::PlatformService::asyncOpenUrl(urlToUse);
		}
	}
}
