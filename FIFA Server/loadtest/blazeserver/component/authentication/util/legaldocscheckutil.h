#ifndef BLAZE_AUTHENTICATION_LEGALDOCSCHECKUTIL_H
#define BLAZE_AUTHENTICATION_LEGALDOCSCHECKUTIL_H


namespace Blaze
{
namespace Authentication
{

class AuthenticationSlaveImpl;

class LegalDocsCheckUtil
{
public:
    LegalDocsCheckUtil(const PeerInfo* peerInfo)
      : mPeerInfo(peerInfo)
    {
    }

    BlazeRpcError DEFINE_ASYNC_RET(checkForLegalDoc(AccountId id, const char8_t* tosString, bool& accepted));

private:
    // memory owned by creator, don't free
    const PeerInfo* mPeerInfo;
};

}
}

#endif
