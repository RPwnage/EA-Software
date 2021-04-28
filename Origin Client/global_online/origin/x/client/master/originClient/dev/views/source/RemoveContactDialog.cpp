///////////////////////////////////////////////////////////////////////////////
// RemoveContactDialog.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "RemoveContactDialog.h"
#include "TelemetryAPIDLL.h"
#include "services/debug/DebugService.h"
#include "chat/Roster.h"
#include "chat/RemoteUser.h"

namespace Origin
{
namespace Client
{

RemoveContactDialog::RemoveContactDialog(Chat::Roster *roster, const Chat::RemoteUser *contact)
: QWidget(NULL)
, mRoster(roster)
, mContact(contact)
{
}

void RemoveContactDialog::onDeleteRequested()
{
	GetTelemetryInterface()->Metric_FRIEND_REMOVAL_SENT();
    mRoster->removeContact(mContact);
}

}
}
