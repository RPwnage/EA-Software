/////////////////////////////////////////////////////////////////////////////
// CreateGroupView.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "CreateGroupView.h"
#include "Services/debug/DebugService.h"
#include "ui_CreateGroupView.h"

namespace Origin
{
	namespace Client
	{
		CreateGroupView::CreateGroupView(QWidget *parent)	: 
            QWidget(parent)
           ,ui(new Ui::CreateGroupView())
		{
			ui->setupUi(this);
            init();
		}

		CreateGroupView::~CreateGroupView()
		{

		}

        void CreateGroupView::init()
        {
            ORIGIN_VERIFY_CONNECT(ui->leGroupName, SIGNAL(textChanged(const QString&)), 
                this, SLOT(onGroupNameTextChanged(const QString&)));
        }

        QString CreateGroupView::getGroupName()
        {
            return ui->leGroupName->displayText();
        }

        void CreateGroupView::setGroupName(const QString& groupName)
        {
            ui->leGroupName->setText(groupName);
        }

        void CreateGroupView::onGroupNameTextChanged(const QString& text)
        {
            if (text.trimmed().isEmpty())
            {
                emit (acceptButtonEnabled(false));
            }
            else
            {
                emit (acceptButtonEnabled(true));
            }
        }

        void CreateGroupView::setFocus()
        {
            ui->leGroupName->setFocus();
        }

	}
}
