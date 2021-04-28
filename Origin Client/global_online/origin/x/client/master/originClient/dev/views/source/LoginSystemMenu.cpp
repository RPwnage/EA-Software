/////////////////////////////////////////////////////////////////////////////
// LoginSystemMenu.cpp
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "LoginSystemMenu.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{
namespace Client
{
LoginSystemMenu::LoginSystemMenu(QWidget* parent)
: CustomQMenu(ClientScope, parent)
{
    // We don't want to show the Open Origin option on Mac
    addSystemTrayOption(tr("ebisu_client_open_app").arg(tr("application_name")), Action_Open);
#if defined(ORIGIN_PC)
    addSeparator();
    // We don't want to show "Exit" because Mac has a default "Quit" option
    addSystemTrayOption(tr("ebisu_mainmenuitem_quit_app").arg(tr("application_name")), Action_Quit);
#endif
}


LoginSystemMenu::~LoginSystemMenu()
{
}


QAction* LoginSystemMenu::addSystemTrayOption(const QString& text, const Action& actionType, const bool& enabled)
{
    QAction* action = new QAction(this);
    action->setObjectName(QString::number(actionType));
    action->setEnabled(enabled);
    action->setText(text);
    addAction(action);
    ORIGIN_VERIFY_CONNECT(action, SIGNAL(triggered()), this, SLOT(onActionTriggered()));
    return action;
}


void LoginSystemMenu::onActionTriggered()
{
    if (sender() == NULL || sender()->objectName().isEmpty())
        return;

    switch (sender()->objectName().toInt())
    {
    case Action_Open:
        GetTelemetryInterface()->Metric_JUMPLIST_ACTION("open", "SystemTray", "");
        emit openTriggered();
        break;
    case Action_Quit:
        GetTelemetryInterface()->Metric_JUMPLIST_ACTION("quit", "SystemTray", "");
        ORIGIN_LOG_EVENT << "User requested application exit via system tray, but no user has logged in yet so simply exiting.";
        emit quitTriggered();
        break;
    default:
        break;
    }
}

}
}