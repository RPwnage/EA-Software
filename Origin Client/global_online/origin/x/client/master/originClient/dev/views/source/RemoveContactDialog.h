///////////////////////////////////////////////////////////////////////////////
// RemoveContactDialog.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _REMOVECONTACTDIALOG_H
#define _REMOVECONTACTDIALOG_H

#include <QWidget>

#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Chat
{
class Roster;
class RemoteUser;
}

namespace Client
{

class ORIGIN_PLUGIN_API RemoveContactDialog : public QWidget
{
	Q_OBJECT

public:
	RemoveContactDialog(Chat::Roster *, const Chat::RemoteUser *contact);

public slots:
    void onDeleteRequested();

private:
    Chat::Roster *mRoster;
    const Chat::RemoteUser *mContact;
};

}
}

#endif
