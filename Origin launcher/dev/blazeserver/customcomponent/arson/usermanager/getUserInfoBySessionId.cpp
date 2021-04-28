
/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getuserinfobysessionid_stub.h"

namespace Blaze
{
    namespace Arson
    {
        class GetUserInfoBySessionIdCommand : public GetUserInfoBySessionIdCommandStub
        {
        public:
            GetUserInfoBySessionIdCommand(Message* message, GetUserInfoBySessionIdReq *mRequest, ArsonSlaveImpl* componentImpl)
                : GetUserInfoBySessionIdCommandStub(message, mRequest),
                mComponent(componentImpl)
            {
            }
            ~GetUserInfoBySessionIdCommand() override { }

        private:
            // Not owned memory
            ArsonSlaveImpl* mComponent;

            GetUserInfoBySessionIdCommandStub::Errors execute() override
            {
                BlazeRpcError err = Blaze::ERR_OK;
                GetUserInfoDataRequest req;
                GetUserInfoDataResponse rsp;
                req.setUserSessionId(mRequest.getUserSessionId());
                Blaze::RpcCallOptions opts;
                opts.routeTo.setSliverIdentityFromKey(Blaze::UserSessionsMaster::COMPONENT_ID, getUserSessionId());
                err = gUserSessionManager->getMaster()->getUserInfoData(req, rsp);

                ExternalId extId = getExternalIdFromPlatformInfo(rsp.getUserInfo().getPlatformInfo());

                mResponse.setId(rsp.getUserInfo().getId());
                // rsp.getUserInfo().getPlatformInfo().copyInto(mResponse.getPlatformInfo());  // CROSSPLAY_TODO - Add this param to the Arson command. Reviewed by Arpit. Not sure about it. No integrator impact
                mResponse.setAccountId(rsp.getUserInfo().getPlatformInfo().getEaIds().getNucleusAccountId());
                mResponse.setPersonaName(rsp.getUserInfo().getPersonaName());
                mResponse.setPersonaNamespace(rsp.getUserInfo().getPersonaNamespace());
                mResponse.setExternalId(extId);
                mResponse.setClientPlatform(rsp.getUserInfo().getPlatformInfo().getClientPlatform());
                mResponse.setAccountLocale(rsp.getUserInfo().getAccountLocale());
                mResponse.setAccountCountry(rsp.getUserInfo().getAccountCountry());
                mResponse.setUserInfoAttribute(rsp.getUserInfo().getUserInfoAttribute());
                mResponse.setStatus(rsp.getUserInfo().getStatus());
                mResponse.setFirstLoginDateTime(rsp.getUserInfo().getFirstLoginDateTime());
                mResponse.setLastLoginDateTime(rsp.getUserInfo().getLastLoginDateTime());
                mResponse.setPreviousLoginDateTime(rsp.getUserInfo().getPreviousLoginDateTime());
                mResponse.setLastLogoutDateTime(rsp.getUserInfo().getLastLogoutDateTime());
                mResponse.setLastLocaleUsed(rsp.getUserInfo().getLastLocaleUsed());
                mResponse.setLastAuthErrorCode(rsp.getUserInfo().getLastAuthErrorCode());
                mResponse.setIsChildAccount(rsp.getUserInfo().getIsChildAccount());
                mResponse.setOriginPersonaId(rsp.getUserInfo().getOriginPersonaId());
                mResponse.setPidId(rsp.getUserInfo().getPidId());

                if (err == Blaze::ERR_OK)
                {
                    return ERR_OK;
                }
                else
                {
                    return ERR_SYSTEM;
                }

            }
        };

        DEFINE_GETUSERINFOBYSESSIONID_CREATE()

    } //Arson
} //Blaze
