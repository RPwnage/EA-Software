/////////////////////////////////////////////////////////////////////////////
// SettingsJsHelper.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "SettingsJsHelper.h"
#include "services/debug/DebugService.h"
#include "ClientFlow.h"

namespace Origin
{
    namespace Client
    {
        SettingsJsHelper::SettingsJsHelper(QObject* parent) 
        : QObject(parent)
        {
        }

        SettingsJsHelper::~SettingsJsHelper()
        {
        }

        void SettingsJsHelper::showSettings()
        {
            emit(openSettings());
        }

        void SettingsJsHelper::verifyEmailClicked()
        {
            emit emailClicked();
        }

        void SettingsJsHelper::uploadAvatar()
        {
            ClientFlow::instance()->showSelectAvatar();
        }
    }
}

