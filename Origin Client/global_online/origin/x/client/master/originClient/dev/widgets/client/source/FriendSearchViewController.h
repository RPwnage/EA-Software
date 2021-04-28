/////////////////////////////////////////////////////////////////////////////
// ClientViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef FRIEND_SEARCH_VIEW_CONTROLLER_H
#define FRIEND_SEARCH_VIEW_CONTROLLER_H

#include <QObject>
#include "UIScope.h"

#include "engine/igo/IIGOCommandController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
    }
    namespace Client
    {
        class SearchView;

        class ORIGIN_PLUGIN_API FriendSearchViewController : public QObject
        {
            Q_OBJECT

        public:
            FriendSearchViewController(QObject* parent = 0);
            ~FriendSearchViewController();

            void show(const UIScope& scope, Engine::IIGOCommandController::CallOrigin callOrigin);

        signals:
            void windowClosed();

        public slots:
             void closeWindow();
             void closeIGOWindow();

        private:

            UIToolkit::OriginWindow* createWindow(const UIScope& scope, SearchView* const wv);

            UIToolkit::OriginWindow* mWindow;
            UIToolkit::OriginWindow* mIGOWindow;

            SearchView* mSearchView;
            SearchView* mSearchViewOIG;
        };
    }
}
#endif //FRIEND_SEARCH_VIEW_CONTROLLER_H