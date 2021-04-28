/////////////////////////////////////////////////////////////////////////////
// LoginSystemMenu.h
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef LOGINSYSTEMMENU_H
#define LOGINSYSTEMMENU_H

#include "CustomQMenu.h"

namespace Origin
{
namespace Client
{

class ORIGIN_PLUGIN_API LoginSystemMenu : public CustomQMenu
{
    Q_OBJECT
public:
    LoginSystemMenu(QWidget* parent = NULL);
    ~LoginSystemMenu();

signals:
    void openTriggered();
    void quitTriggered();


private slots:
    void onActionTriggered();

private:
    enum Action {
        Action_Open,
        Action_Quit
    };
    QAction* addSystemTrayOption(const QString& text, const Action& actionType, const bool& enabled = true);
};
}
}
#endif