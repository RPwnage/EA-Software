#ifndef _TOINVISIBLEFLOW_H
#define _TOINVISIBLEFLOW_H

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
class ORIGIN_PLUGIN_API ToInvisibleFlow : public QObject
{
    Q_OBJECT
public:
    explicit ToInvisibleFlow(Engine::Social::SocialController *socialController);

    ///
    /// Starts the flow
    ///
    // \param  socialController Social controller to use for the visibility change operation
    /// \param visibilty TBD.
    ///
    void start(Chat::ConnectedUser::Visibility visibilty);

signals:
    /// \brief Emitted once this flow finished
    void finished(bool success);

private slots:
    void continueInvisible();
    void cancelInvisible();

private:
    Engine::Social::SocialController *mSocialController;
};

}
}

#endif

