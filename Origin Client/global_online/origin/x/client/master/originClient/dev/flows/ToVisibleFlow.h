#ifndef _TOVISIBLEFLOW_H
#define _TOVISIBLEFLOW_H

#include <QObject>

#include "chat/ConnectedUser.h" 
#include "services/plugin/PluginAPI.h"

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

///
/// \brief Handles high-level logic and user feedback for setting the connected user's chat visibility
///
/// This will ask for user confirmation to close all multi-user chats when becoming invisible. This is required to 
/// work around the lack of support for multi-user chatting while invisible on our XMPP server.
///
class ORIGIN_PLUGIN_API ToVisibleFlow : public QObject
{
    Q_OBJECT
public:
    explicit ToVisibleFlow(Engine::Social::SocialController* socialController);

    ///
    /// Starts the flow
    ///
    // \param  socialController Social controller to use for the visibility change operation
    /// \param visibilty TBD.
    ///
    void start();

signals:
    /// \brief Emitted once this flow finished
    void finished(const bool& success);

private slots:
    void continueFlow();
    void cancelFlow();

private:
    Engine::Social::SocialController* mSocialController;
};

}
}

#endif

