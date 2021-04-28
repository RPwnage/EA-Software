/////////////////////////////////////////////////////////////////////////////
// AchievementMiniView.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "AchievementMiniView.h"
#include "ui_AchievementMiniView.h"
#include "Services/debug/DebugService.h"

namespace Origin
{
	namespace Client
	{
		AchievementMiniView::AchievementMiniView(QWidget *parent)
        : QWidget(parent)
        , ui(NULL)
		{
            ui = new Ui::AchievementMiniView();
			ui->setupUi(this);
            ORIGIN_VERIFY_CONNECT(ui->btnXP, SIGNAL(clicked()), this, SIGNAL(clicked()));
            ORIGIN_VERIFY_CONNECT(ui->btnIcon, SIGNAL(clicked()), this, SIGNAL(clicked()));

#ifdef ORIGIN_MAC
            // Mac sometimes requires unique spacing.
            ui->layout->setSpacing(15);
#endif
		}
		
        AchievementMiniView::~AchievementMiniView()
        {
             delete ui;
             ui = NULL;
        }
        
        void AchievementMiniView::setAlignment(const Alignment& alignment)
        {
            switch(alignment)
            {
                case ALIGN_LEFT:
#ifdef ORIGIN_MAC
                    // Mac sometimes requires unique spacing.
                    ui->leftSpacer->changeSize(7, 1, QSizePolicy::Fixed);
#else
                    ui->leftSpacer->changeSize(0, 0, QSizePolicy::Fixed);
#endif
                    ui->rightSpacer->changeSize(1, 1, QSizePolicy::Expanding);
                    break;

                case ALIGN_RIGHT:
                    ui->leftSpacer->changeSize(1, 1, QSizePolicy::Expanding);
#ifdef ORIGIN_MAC
                    // Mac sometimes requires unique spacing.
                    ui->rightSpacer->changeSize(6, 1, QSizePolicy::Fixed);
#else
                    ui->rightSpacer->changeSize(0, 0, QSizePolicy::Fixed);
#endif
                    break;

                default:
                    break;
            }
        }

        void AchievementMiniView::setXP(const int& xp)
        {
            // Hide the UI if no achievements
            ui->btnXP->setHidden(xp == 0);
            ui->btnIcon->setHidden(xp == 0);
            ui->btnXP->setText(QString::number(xp));
        }

        void AchievementMiniView::setStyleProperty(const QString& styleProperty)
        {
            setProperty("style", styleProperty.isEmpty() ? QVariant::Invalid : QVariant(styleProperty));
            setStyle(QApplication::style());
        }

        void AchievementMiniView::setToolTipEnabled(const bool& iconEnabled, const bool& textEnabled)
        {
            ui->btnIcon->setToolTip(iconEnabled ? tr("ebisu_client_achievement_tooltip") : "");
            ui->btnXP->setToolTip(textEnabled ? tr("ebisu_client_achievement_tooltip") : "");
        }
    }
}