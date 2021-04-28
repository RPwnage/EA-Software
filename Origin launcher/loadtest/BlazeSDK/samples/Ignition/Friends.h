#ifndef FRIENDS_H
#define FRIENDS_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"

#include "BlazeSDK/component/friendscomponent.h"
#include "BlazeSDK/component/friends/tdf/friends.h"

namespace Ignition
{

class Friends :
    public LocalUserUiBuilder
{
    public:

        Friends(uint32_t userIndex);
        virtual ~Friends();
        eastl::string getAccessToken() { return mAccessToken; }
        void setAccessToken(eastl::string token) { mAccessToken = token; }

        virtual void onAuthenticated();
        virtual void onDeAuthenticated();

    private:

        PYRO_ACTION(Friends, MuteUser);
        PYRO_ACTION(Friends, UnmuteUser);
        PYRO_ACTION(Friends, CheckMuteStatus);
        PYRO_ACTION(Friends, SetAccessToken);
        
        void CheckMuteStatusCb(const Blaze::Social::Friends::PagedListUser *response, Blaze::BlazeError error, Blaze::JobId jobId);
        void MuteUserActionCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, int64_t friendId);
        void UnMuteUserActionCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, int64_t friendId);

        static const char8_t* DEFAULT_FRIENDS_APPLICATION_KEY;
        static const char8_t* DEFAULT_FRIENDS_API_VERSION;
        static const char8_t* DEFAULT_FRIENDS_HOSTNAME;
        static const int      DEFAULT_FRIENDS_HOST_PORT;
        static const bool     DEFAULT_FRIENDS_SECURE;
        
        //Temporary placeholder for access token until Nucleus SDK is integrated.
        eastl::string mAccessToken;
};

}

#endif //FRIENDS_H