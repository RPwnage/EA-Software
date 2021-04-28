/////////////////////////////////////////////////////////////////////////////
// FriendsListView.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "FriendsListView.h"
#include "Services/debug/DebugService.h"
#include "NavigationController.h"
#include "engine/login/LoginController.h"
#include <QMouseEvent>
#include "ui_FriendsListView.h"

namespace Origin
{
	namespace Client
	{
		FriendsListView::FriendsListView(QWidget *parent)	: 
            QWidget(parent)
           ,ui(new Ui::FriendsListView())
		{
			ui->setupUi(this);
			init();
		}

		FriendsListView::~FriendsListView()
		{

		}

		void FriendsListView::init()
		{
		}

        bool FriendsListView::event(QEvent* event)
        {
            switch(event->type())
            {
            case QEvent::KeyPress:
                // Eat the Enter key - it brings up the profile tab. Don't want.
                if(((QKeyEvent*)event)->key() == Qt::Key_Return)
                    return true;
                break;
                
            default:
                break;
            }

            return QWidget::event(event);
        }

        QFrame* FriendsListView::buddyListFrame() 
        { 
            return ui->buddyList; 
        }
        
        QLayout* FriendsListView::buddyListLayout() 
        { 
            return ui->buddyListLayout; 
        }
	}
}
