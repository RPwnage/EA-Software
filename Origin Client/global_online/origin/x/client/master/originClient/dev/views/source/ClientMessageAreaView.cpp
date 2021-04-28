/////////////////////////////////////////////////////////////////////////////
// ClientMessageAreaView.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ClientMessageAreaView.h"
#include "Services/debug/DebugService.h"
#include "ui_ClientMessageAreaView.h"

namespace Origin
{
	namespace Client
	{
		ClientMessageAreaView::ClientMessageAreaView(QWidget* parent)
		: QWidget(parent)
        , ui(NULL)
		{
            ui = new Ui::ClientMessageAreaView();
			ui->setupUi(this);
		}

		ClientMessageAreaView::~ClientMessageAreaView()
		{

		}

        void ClientMessageAreaView::setTitle(const QString& title)
        {
            ui->lblTitle->setText(title);
        }

        QString ClientMessageAreaView::title() const
        {
            return ui->lblTitle->text();
        }
		
		void ClientMessageAreaView::setText(const QString& text)
		{
			ui->lblText->setText(text);
		}

		QString ClientMessageAreaView::text() const
		{
			return ui->lblText->text();
		}

        QPushButton* ClientMessageAreaView::firstButton()
        {
            return ui->btn1;
        }

        QPushButton* ClientMessageAreaView::secondButton()
        {
            return ui->btn2;
        }
    }
}