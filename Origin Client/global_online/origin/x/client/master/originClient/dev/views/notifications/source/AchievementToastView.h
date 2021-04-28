/////////////////////////////////////////////////////////////////////////////
// AchievementToastView.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef ACHIEVEMENTTOASTVIEW_H
#define ACHIEVEMENTTOASTVIEW_H

#include <QWidget>
#include "UIScope.h"

#include "services/plugin/PluginAPI.h"

namespace Ui
{
    class AchievementToastView;
}

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API AchievementToastView : public QWidget
        {
            Q_OBJECT

        public:
            enum Alignment {ALIGN_LEFT, ALIGN_RIGHT};

            AchievementToastView(int xp, const UIScope& scope, QWidget* parent = 0);
            ~AchievementToastView();

        signals:
            /// \brief Emitted when the XP label or icon is clicked
            void clicked();


        private:
            Ui::AchievementToastView* ui;
        };
    }
}

#endif