/////////////////////////////////////////////////////////////////////////////
// CreateRoomView.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "CreateRoomView.h"
#include "Services/debug/DebugService.h"
#include "ui_CreateRoomView.h"

namespace Origin
{
	namespace Client
	{
		CreateRoomView::CreateRoomView(QWidget *parent)	: 
            QWidget(parent)
           ,ui(new Ui::CreateRoomView())
		{
			ui->setupUi(this);
            init();
		}

		CreateRoomView::~CreateRoomView()
		{

		}

        void CreateRoomView::init()
        {
            ORIGIN_VERIFY_CONNECT(ui->chkPassword, SIGNAL(stateChanged(int)), 
                this, SLOT(onPasswordChecked(int)));
            ORIGIN_VERIFY_CONNECT(ui->lePassword, SIGNAL(textChanged(const QString&)), 
                this, SLOT(onPasswordTextChanged(const QString&)));
            ORIGIN_VERIFY_CONNECT(ui->leChannelName, SIGNAL(textChanged(const QString&)), 
                this, SLOT(onRoomNameTextChanged(const QString&)));

            ui->lePassword->setEnabled(false);
        }

        void CreateRoomView::onPasswordChecked(int state)
        {
            if (state == Qt::Unchecked)
            {
                ui->lePassword->setEnabled(false);
                if (!getRoomName().isEmpty())
                {
                    emit (acceptButtonEnabled(true));
                }                
            }
            else if (state == Qt::Checked)
            {
                ui->lePassword->setEnabled(true);
                if (ui->lePassword->displayText().isEmpty())
                {
                    emit (acceptButtonEnabled(false));
                }
            }
            else
            {
                ORIGIN_ASSERT(0);
            }
        }

        void CreateRoomView::onPasswordTextChanged(const QString& text)
        {
            if (text.isEmpty())
            {
                emit (acceptButtonEnabled(false));
            }
            else
            {
                if (!getRoomName().isEmpty())
                {
                    emit (acceptButtonEnabled(true));
                }
            }
        }

        void CreateRoomView::onRoomNameTextChanged(const QString& text)
        {
            if (text.isEmpty())
            {
                emit (acceptButtonEnabled(false));
            }
            else
            {
                if (ui->lePassword->isEnabled() && getPassword().isEmpty())
                {
                    emit (acceptButtonEnabled(false));
                }
                else
                {
                    emit (acceptButtonEnabled(true));
                }
            }
        }

        QString CreateRoomView::getRoomName()
        {
            return ui->leChannelName->displayText();
        }

        QString CreateRoomView::getPassword()
        {
            return ui->lePassword->displayText();
        }

        bool CreateRoomView::isRoomLocked()
        {
            if (ui->chkPassword->checkState() == Qt::Unchecked)
            {
                return false;
            }
            else if (ui->chkPassword->checkState() == Qt::Checked)
            {
                return true;
            }
            else
            {
                ORIGIN_ASSERT(0);
            }
            return false;
        }
	}
}
