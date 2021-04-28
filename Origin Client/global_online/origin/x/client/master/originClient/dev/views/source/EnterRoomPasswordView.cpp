/////////////////////////////////////////////////////////////////////////////
// EnterRoomPasswordView.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "EnterRoomPasswordView.h"
#include "Services/debug/DebugService.h"
#include "ui_EnterRoomPasswordView.h"

namespace Origin
{
	namespace Client
	{
		EnterRoomPasswordView::EnterRoomPasswordView(QWidget *parent)	: 
            QWidget(parent)
           ,ui(new Ui::EnterRoomPasswordView())
		{
			ui->setupUi(this);
		}

		EnterRoomPasswordView::~EnterRoomPasswordView()
		{

		}

        QString EnterRoomPasswordView::getPassword()
        {
            return ui->lePassword->displayText();
        }

        void EnterRoomPasswordView::setNormal()
        {
            ui->originValidationContainer->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Normal);
        }

        void EnterRoomPasswordView::setValidating()
        {
            ui->originValidationContainer->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Validating);
        }

        void EnterRoomPasswordView::setError()
        {
            ui->originValidationContainer->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Invalid);
        }
	}
}
