#include "ToInvisibleFlow.h"

#include <QDebug>
#include <chat/OriginConnection.h>
#include <engine/social/SocialController.h>
#include <engine/social/ConversationManager.h>
#include <engine/igo/IGOController.h>
#include <services/debug/DebugService.h>

#include "Qt/originwindow.h"                                                                                            
#include "Qt/originwindowmanager.h"  
    

namespace Origin
{
namespace Client
{
    
ToInvisibleFlow::ToInvisibleFlow(Engine::Social::SocialController *socialController)
: mSocialController(socialController)
{
}

void ToInvisibleFlow::start(Chat::ConnectedUser::Visibility visibilty)
{
    using namespace Origin::UIToolkit;

    if ((visibilty == Chat::ConnectedUser::Visible) ||
        (!mSocialController->conversations()->hasMultiUserDependentConversations()))
    {
        // This requires no special handling
        mSocialController->chatConnection()->currentUser()->requestVisibility(visibilty);
        emit finished(true);

        return;
    }

    // Warn the user that we'll close their conversations
    OriginWindow* warningDialog = OriginWindow::promptNonModal(OriginMsgBox::NoIcon, tr("ebisu_client_you_may_not_set_your_status_to_invisible_title"),
        tr("ebisu_client_if_you_change_your_status_to_invisible_text"), tr("ebisu_client_enable_invisibility"), tr("ebisu_client_cancel"));
    ORIGIN_VERIFY_CONNECT(warningDialog, SIGNAL(accepted()), this, SLOT(continueInvisible()));
    ORIGIN_VERIFY_CONNECT(warningDialog, SIGNAL(rejected()), this, SLOT(cancelInvisible()));

    if (Engine::IGOController::instance()->igowm()->visible())
    {
        warningDialog->configForOIG();
        Engine::IGOController::instance()->igowm()->addPopupWindow(warningDialog, Engine::IIGOWindowManager::WindowProperties());
    }
    else
    {
        warningDialog->showWindow();
    }
}
    
void ToInvisibleFlow::continueInvisible()
{
    mSocialController->conversations()->finishMultiUserDependentConversations(Engine::Social::Conversation::UserFinished_Invisible);
    mSocialController->chatConnection()->currentUser()->requestVisibility(Chat::ConnectedUser::Invisible);
    emit finished(true);
}

void ToInvisibleFlow::cancelInvisible()
{
    emit finished(false);
}

}
}
