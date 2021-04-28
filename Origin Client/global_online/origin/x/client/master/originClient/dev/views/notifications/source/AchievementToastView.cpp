/////////////////////////////////////////////////////////////////////////////
// AchievementToastView.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "AchievementToastView.h"
#include "ui_AchievementToastView.h"
#include "Services/debug/DebugService.h"
#include "engine/igo/IGOController.h"

namespace Origin
{
    namespace Client
    {
        AchievementToastView::AchievementToastView(int xp, const UIScope& scope, QWidget *parent)
            : QWidget(parent)
            , ui(NULL)
        {
            bool oigActive = Origin::Engine::IGOController::instance()->isActive();
            ui = new Ui::AchievementToastView();
            ui->setupUi(this);
            ORIGIN_VERIFY_CONNECT(ui->AchievementMiniView, SIGNAL(clicked()), this, SIGNAL(clicked()));
            ORIGIN_VERIFY_CONNECT(ui->AchievementMiniView, SIGNAL(clicked()), this, SIGNAL(clicked()));

#ifdef ORIGIN_MAC
            // Mac sometimes requires unique spacing.
            this->layout()->setSpacing(15);
            ui->AchievementMiniView->layout()->setSpacing(15);
#endif
            // don't want any indentation
            ui->AchievementMiniView->layout()->setContentsMargins(0,0,0,0);
            ui->AchievementMiniView->setXP(xp);
            ui->AchievementMiniView->setToolTipEnabled(false, false);
            
            if (scope == IGOScope && !oigActive)
            {
                ui->AchievementMiniView->setAlignment(AchievementMiniView::ALIGN_LEFT);
                ui->AchievementMiniView->setStyleProperty("inOIG");
            }
            else
            {
                ui->AchievementMiniView->setAlignment(AchievementMiniView::ALIGN_RIGHT);
                ui->AchievementMiniView->setStyleProperty("notification");
            }
        }

        AchievementToastView::~AchievementToastView()
        {
            delete ui;
            ui = NULL;
        }

    }// namespace Client
}// namespace Origin
