/////////////////////////////////////////////////////////////////////////////
// IGOFlow.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef IGO_FLOW_H
#define IGO_FLOW_H

#include <QStringList>
#include "flows/AbstractFlow.h"
#include "ClientViewController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API IGOFlow : public AbstractFlow
        {
            Q_OBJECT

        public:
            IGOFlow() {};
            virtual ~IGOFlow() {};

            void start();

        protected slots:

            void showMyProfile(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void showFriendsProfile(qint64 user, Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void showAvatarSelect(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void showFindFriends(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void showChat(QString from, QStringList to, QString msg, Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void onRequestGoOnline();
            void showToast(const QString& toasteeSetting, const QString& toast, const QString& message);
            void showOpenOIGToast();
            void showTrialWelcomeToast(const QString& title, const qint64& timeRemaining);
        };
    }
}

#endif //IGO_FLOW_H
