/////////////////////////////////////////////////////////////////////////////
// OfflineMessageAreaViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "OfflineMessageAreaViewController.h"
#include "ClientMessageAreaView.h"
#include "Services/debug/DebugService.h"
#include <QPushButton>
#include <QWidget>

namespace Origin
{
namespace Client
{

OfflineMessageAreaViewController::OfflineMessageAreaViewController(QObject* parent)
: MessageAreaViewControllerBase(Offline, parent)
, mClientMessageAreaView(NULL)
{
    mClientMessageAreaView = new ClientMessageAreaView();
}


OfflineMessageAreaViewController::~OfflineMessageAreaViewController()
{
    if(mClientMessageAreaView)
    {
        delete mClientMessageAreaView;
        mClientMessageAreaView = NULL;
    }
}


void OfflineMessageAreaViewController::onConnectionChanged(bool isOnline)
{
    if(isOnline == false)
    {
        mClientMessageAreaView->setTitle(tr("ebisu_client_offline_mode_message_title"));
        mClientMessageAreaView->setText(tr("ebisu_client_offline_mode_message_text").arg(tr("application_name")));
        mClientMessageAreaView->firstButton()->setText(tr("ebisu_client_single_login_go_online"));
        mClientMessageAreaView->secondButton()->setVisible(false);
        //this can get called multiple times but we only want to connect it once
        ORIGIN_VERIFY_CONNECT_EX(mClientMessageAreaView->firstButton(), SIGNAL(clicked()), this, SIGNAL(goOnline()), Qt::UniqueConnection);
    }
	else
	{
	    ORIGIN_VERIFY_DISCONNECT(mClientMessageAreaView->firstButton(), SIGNAL(clicked()), this, SIGNAL(goOnline()));
	}
}


QWidget* OfflineMessageAreaViewController::view()
{
    return mClientMessageAreaView;
}

}
}
