/////////////////////////////////////////////////////////////////////////////
// FriendsListView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef FRIENDS_LIST_VIEW_H
#define FRIENDS_LIST_VIEW_H

#include <QtCore/QScopedPointer>
#include <QWidget>

#include "services/plugin/PluginAPI.h"

class QFrame;
class QLayout;
class QEvent;

namespace Ui
{
    class FriendsListView;
}

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API FriendsListView : public QWidget
        {
            Q_OBJECT

        public:

            FriendsListView(QWidget *parent);
            ~FriendsListView();

            void init();

            QFrame* buddyListFrame();
            QLayout* buddyListLayout();

        protected:
            bool event(QEvent* event);

        private:
            QScopedPointer<Ui::FriendsListView> ui;
        };
    }
}

#endif
