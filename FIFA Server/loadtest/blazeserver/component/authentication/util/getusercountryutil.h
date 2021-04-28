#ifndef BLAZE_AUTHENTICATION_GETUSERCOUNTRY_H
#define BLAZE_AUTHENTICATION_GETUSERCOUNTRY_H

namespace Blaze
{
namespace Authentication
{

class AuthenticationSlaveImpl;

class GetUserCountryUtil
{
public:
    GetUserCountryUtil(AuthenticationSlaveImpl &componentImpl, const PeerInfo *peerInfo)
      : mComponent(componentImpl),
        mPeerInfo(peerInfo)
    {}

    const char8_t* getUserCountry(PersonaId personaId);
	
private:
    // memory owned by creator, don't free
    AuthenticationSlaveImpl &mComponent;
    const PeerInfo* mPeerInfo;
};

}
}
#endif //BLAZE_AUTHENTICATION_GETUSERCOUNTRY_H
