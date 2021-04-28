/////////////////////////////////////////////////////////////////////////////
// OfflineMessageAreaViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef OFFLINEMESSAGEAREAVIEWCONTROLLER_H
#define OFFLINEMESSAGEAREAVIEWCONTROLLER_H

#include "MessageAreaViewControllerBase.h"
#include "services/plugin/PluginAPI.h"

class QWidget;

namespace Origin
{
namespace Client
{
class ClientMessageAreaView;
/// \brief Message that lets the user know they are offline.
class ORIGIN_PLUGIN_API OfflineMessageAreaViewController : public MessageAreaViewControllerBase
{
	Q_OBJECT

public:
	OfflineMessageAreaViewController(QObject* parent);
	~OfflineMessageAreaViewController();
    QWidget* view();

public slots:
    /// \brief Emitted when the user's connect settings changed.
    void onConnectionChanged(bool isOnline);

signals:
    /// \brief Emitted when the Go Online button is pressed.
    void goOnline();


private:
	ClientMessageAreaView* mClientMessageAreaView;
};
}
}
#endif