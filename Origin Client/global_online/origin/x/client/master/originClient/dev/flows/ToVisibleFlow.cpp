#include "ToVisibleFlow.h"

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
    
ToVisibleFlow::ToVisibleFlow(Engine::Social::SocialController *socialController)
: mSocialController(socialController)
{
}

void ToVisibleFlow::start()
{
    using namespace Origin::UIToolkit;

    if (mSocialController->chatConnection()->currentUser()->visibility() != Chat::ConnectedUser::Invisible)
    {
        // This requires no special handling
        emit finished(true);
        return;
    }

    // Warn the user that we'll close their conversations
    OriginWindow* warningDialog = OriginWindow::promptNonModal(OriginMsgBox::NoIcon, tr("ebisu_client_entering_chatroom_will_remove_invisibility_title"),
        tr("ebisu_client_if_you_enter_into_a_chatroom_your_status_will_text"), tr("ebisu_client_enter_room"), tr("ebisu_client_cancel"));
    ORIGIN_VERIFY_CONNECT(warningDialog, SIGNAL(accepted()), this, SLOT(continueFlow()));
    ORIGIN_VERIFY_CONNECT(warningDialog, SIGNAL(rejected()), this, SLOT(cancelFlow()));

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
    
void ToVisibleFlow::continueFlow()
{
    sender()->deleteLater();
    bool success = mSocialController->chatConnection()->currentUser()->requestVisibility(Chat::ConnectedUser::Visible);
    emit finished(success);
}

void ToVisibleFlow::cancelFlow()
{
    sender()->deleteLater();
    emit finished(false);
}

}
}
