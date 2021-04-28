/////////////////////////////////////////////////////////////////////////////
// ChangeAvatarViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CHANGE_AVATAR_VIEW_CONTROLLER_H
#define CHANGE_AVATAR_VIEW_CONTROLLER_H

#include <QObject>
#include "UIScope.h"

#include "engine/igo/IIGOCommandController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace UIToolkit
{
class OriginWindow;
}
namespace Client
{
/// \brief Controller for the avatar window. Helps load avatar web page,
/// display, and log debug info.
class ORIGIN_PLUGIN_API ChangeAvatarViewController : public QObject
{
    Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent - The parent of the ChangeAvatarViewController.
    ChangeAvatarViewController(QObject* parent = 0);

    /// \brief Destructor - calls closePromoDialog()
    ~ChangeAvatarViewController();

    /// \brief Shows the change avatar window.
    void show(UIScope scope, Engine::IIGOCommandController::CallOrigin callOrigin);

signals:
    /// \brief Emitted when the window is closed and deleted.
    void windowClosed();


protected slots:
    /// \brief Closes the avatar window.
    void closeAvatarWindow();

    void closeIGOAvatarWindow();

    void onLoadFinishedInIGO(bool ok);
    
private:
    UIToolkit::OriginWindow* mAvatarWindow;
    UIToolkit::OriginWindow* mIGOAvatarWindow;
};
}
}
#endif