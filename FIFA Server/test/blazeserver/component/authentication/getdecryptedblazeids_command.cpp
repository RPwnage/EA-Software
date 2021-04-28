/*************************************************************************************************/
/*!
    \file   getdecryptedblazeids_command.cpp

        Decrypt encrypted BlazeIds

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "authentication/authenticationimpl.h"
#include "authentication/rpc/authenticationslave/getdecryptedblazeids_stub.h"
#include "proxycomponent/nucleusidentity/rpc/nucleusidentityslave.h"
#include "framework/connection/outboundhttpservice.h" //for gOutboundHttpService in execute()
#include "framework/oauth/accesstokenutil.h"
#include "framework/oauth/oauthslaveimpl.h"
#include "authentication/util/authenticationutil.h" // for AuthenticationUtil in execute()

#include "nucleusidentity/tdf/nucleusidentity.h"

namespace Blaze
{
namespace Authentication
{

class GetDecryptedBlazeIdsCommand : public GetDecryptedBlazeIdsCommandStub
{
public:
    GetDecryptedBlazeIdsCommand(Message* message, GetDecryptedBlazeIdsRequest* request, AuthenticationSlaveImpl* componentImpl) :
        GetDecryptedBlazeIdsCommandStub(message, request), mComponent(componentImpl)
    {
    }

    ~GetDecryptedBlazeIdsCommand() override {};

private:
    AuthenticationSlaveImpl* mComponent;

    GetDecryptedBlazeIdsCommandStub::Errors execute() override
    {
        OAuth::AccessTokenUtil tokenUtil;
        BlazeRpcError rc = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
        if (rc != Blaze::ERR_OK)
        {
            ERR_LOG("[GetDecryptedBlazeIdsCommand].execute : Failed to retrieve access token, error(" << ErrorHelp::getErrorName(rc) << ").");
            return commandErrorFromBlazeError(rc);
        }

        NucleusIdentity::NucleusIdentitySlave* comp = (NucleusIdentity::NucleusIdentitySlave*)gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name);
        if (comp == nullptr)
        {
            ERR_LOG("[GetDecryptedBlazeIdsCommand].execute: nucleusidentity is nullptr");
            return ERR_SYSTEM;
        }

        NucleusIdentity::DecryptBlazeIdsRequest request;
        NucleusIdentity::DecryptBlazeIdsResponse response;
        NucleusIdentity::IdentityError error;
        request.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());
        request.getAuthCredentials().setClientId(tokenUtil.getClientId());
        request.getAuthCredentials().setIpAddress(AuthenticationUtil::getRealEndUserAddr(this).getIpAsString());
        request.getBody().setEncryptionType("tournamentId");
        mRequest.getEncryptedBlazeIds().copyInto(request.getBody().getEncryptedValues());

        BlazeRpcError err = comp->decryptBlazeIds(request, response, error);
        if (err != ERR_OK)
        {
            if (mComponent->oAuthSlaveImpl()->isMock())
            {
                //To avoid blocking mock testing, ignore real auth / nucleus errors. Mock encrypted ids are strings of the non mock BlazeId
                for (auto eItr : mRequest.getEncryptedBlazeIds())
                {
                    BlazeId id = EA::StdC::AtoI64(eItr);
                    mResponse.getDecryptedIds().insert(eastl::make_pair(eItr, id));
                    TRACE_LOG("[GetDecryptedBlazeIdsCommand].execute: decrypted mock id(" << eItr << ") to BlazeId(" << id << ").");
                }
                return ERR_OK;
            }

            ERR_LOG("[GetDecryptedBlazeIdsCommand].execute: Nucleus failed to decrypt blazeids, error(" << ErrorHelp::getErrorName(err) << "): " << error);
            return commandErrorFromBlazeError(err);
        }

        for (auto itr : response.getCrypto().getCryptoResult())
        {
            BlazeId id = EA::StdC::AtoI64(itr->getDecryptedValue());
            mResponse.getDecryptedIds().insert(eastl::make_pair(itr->getEncryptedValue(), id));
        }

        // log if failed to decrypt all requested ids
        if (mRequest.getEncryptedBlazeIds().size() != response.getCrypto().getCryptoResult().size())
        {
            for (const auto& encryptedId : mRequest.getEncryptedBlazeIds())
            {
                auto found = mResponse.getDecryptedIds().find(encryptedId);
                if (found == mResponse.getDecryptedIds().end())
                {
                    mErrorResponse.getFailedEncryptedBlazeIds().push_back(encryptedId);
                }
            }
            ERR_LOG("[GetDecryptedBlazeIdsCommand].execute: failed to decrypt possibly invalid encryptedBlazeIds: " << mErrorResponse);
            return commandErrorFromBlazeError(AUTH_ERR_INVALID_USER);
        }

        return commandErrorFromBlazeError(err);
    }
};

DEFINE_GETDECRYPTEDBLAZEIDS_CREATE()


} // namespace Authentication
} // namespace Blaze
