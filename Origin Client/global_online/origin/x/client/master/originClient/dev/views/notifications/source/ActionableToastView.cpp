/////////////////////////////////////////////////////////////////////////////
// ActionableToastView.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ActionableToastView.h"
#include "ui_ActionableToastView.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "originlabel.h"
#include "ToastView.h"

namespace Origin
{
    namespace Client
    {

        ActionableToastView::ActionableToastView(QWidget *parent)
        : QWidget(parent)
        , ui(new Ui::ActionableToastView)
        {
	        ui->setupUi(this);
            // TODO: Move to origin.qss
	        QPixmap acceptIcon = QPixmap(":/notifications/answer_icon.png");
            ui->accept->setIcon(acceptIcon);
	        ui->accept->setIconSize(acceptIcon.rect().size());
	        QPixmap declineIcon(":/notifications/decline_icon.png");
	        ui->decline->setIcon(declineIcon);
	        ui->decline->setIconSize(declineIcon.rect().size());

            ORIGIN_VERIFY_CONNECT(ui->accept, SIGNAL(clicked()), this, SIGNAL(acceptClicked()));
            ORIGIN_VERIFY_CONNECT(ui->decline, SIGNAL(clicked()), this, SIGNAL(declineClicked()));
        }

        ActionableToastView::~ActionableToastView()
        {
            if(ui)
            {
                delete ui;
                ui = NULL;
            }
        }

    }// namespace Client
}// namespace Origin
