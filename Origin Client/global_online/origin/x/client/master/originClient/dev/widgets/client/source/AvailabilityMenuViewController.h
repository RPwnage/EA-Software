#ifndef _AVAILABILITYMENUVIEWCONTROLLER_H
#define _AVAILABILITYMENUVIEWCONTROLLER_H

#include <QObject>
#include <QHash>

#include <chat/Presence.h>
#include "services/plugin/PluginAPI.h"

class QMenu;
class QAction;
class QActionGroup;
class QWidget;

namespace Origin
{

namespace Engine
{
namespace Social
{
    class SocialController;
}
}

namespace Client
{
    /// \brief Controller for a QMenu that allows the user to select their current chat presence
    class ORIGIN_PLUGIN_API AvailabilityMenuViewController : public QObject
    {
        Q_OBJECT
    public:
        AvailabilityMenuViewController(Engine::Social::SocialController *socialController, QWidget *parent = NULL);

        QMenu *menu() { return mMenu; }

    private slots:
        void onAboutToShow();
        void onTriggered();

        void onChatConnected();
        void onChatDisconnected();

    private:
        typedef QHash<QAction *, Chat::Presence::Availability> ActionAvailabilityHash;

        QAction* addAction(const QString& iconFilename, const QString &text);

        Engine::Social::SocialController *mSocialController;

        QMenu *mMenu;
        QActionGroup *mActionGroup;
        ActionAvailabilityHash mVisibleActions;
        QAction *mInvisibleAction;
    };

}
}

#endif

