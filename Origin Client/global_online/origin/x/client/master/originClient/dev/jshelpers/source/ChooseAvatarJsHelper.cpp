#include "ChooseAvatarJsHelper.h"

#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"
#include "engine/social/AvatarManager.h"

namespace Origin
{
    namespace Client
    {

        ChooseAvatarJsHelper::ChooseAvatarJsHelper(QObject *parent) : QObject(parent)
        {
        }

        void ChooseAvatarJsHelper::updateAvatar(QString avatarID)
        {
            Engine::UserRef user = Engine::LoginController::currentUser();
            Engine::Social::SocialController *socialController = user->socialControllerInstance();

            // Request an avatar refresh for both our avatars
            socialController->smallAvatarManager()->refresh(user->userId());
            socialController->mediumAvatarManager()->refresh(user->userId());

            emit(updateAvatarRequested(avatarID));
        }

        void ChooseAvatarJsHelper::cancel()
        {
            emit(cancelled());
        }


    }
}
