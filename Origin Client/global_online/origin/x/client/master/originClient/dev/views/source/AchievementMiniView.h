/////////////////////////////////////////////////////////////////////////////
// AchievementMiniView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef ACHIEVEMENTMINIVIEW_H
#define ACHIEVEMENTMINIVIEW_H

#include <QWidget>

#include "services/plugin/PluginAPI.h"

namespace Ui
{
    class AchievementMiniView;
}
namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API AchievementMiniView : public QWidget
        {
            Q_OBJECT
    
        public:
            enum Alignment {ALIGN_LEFT, ALIGN_RIGHT};
            
            AchievementMiniView(QWidget* parent = 0);
            ~AchievementMiniView();
    
            /// \brief Set the alignment of the label and icon
            void setAlignment(const Alignment& alignment);

            /// \brief Set the value of the label
            void setXP(const int& xp);

            /// \brief Set the style property of the widget. If there is a 
            /// particular area that needs a style: in the qss call 
            /// #AchievementMiniView[style="STYLEPROPERTY"]
            /// If argument is empty it will clear the style.
            void setStyleProperty(const QString& styleProperty);
    
            /// \brief Set if a tooltip should show for the icon or text. 
            /// True by default.
            void setToolTipEnabled(const bool& iconEnabled, const bool& textEnabled);

        signals:
            /// \brief Emitted when the XP label or icon is clicked
            void clicked();
    

        private:
            Ui::AchievementMiniView* ui;
        };
    }
}
#endif