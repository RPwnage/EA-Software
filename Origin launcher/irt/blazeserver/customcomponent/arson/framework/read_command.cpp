/*************************************************************************************************/
/*!
    \file   read_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/read_stub.h"
#include "framework/connection/outboundhttpservice.h"

#include "secretvault/tdf/secretvault.h"
#include "secretvault/rpc/secretvaultslave.h"
#include "EAStdC/EAString.h"

namespace Blaze
{
    namespace Arson
    {
        class ReadCommand : public ReadCommandStub
        {
        public:
            ReadCommand(
                Message* message, Blaze::Arson::SecretVaultReadRequest* request, ArsonSlaveImpl* componentImpl)
                : ReadCommandStub(message, request),
                mComponent(componentImpl)
            {
            }

            ~ReadCommand() override { }

        private:
            // Not owned memory
            ArsonSlaveImpl* mComponent;

            ReadCommandStub::Errors execute() override
            {

                Blaze::SecretVault::SecretVaultSlave* secretVaultSlave = (Blaze::SecretVault::SecretVaultSlave *)Blaze::gOutboundHttpService->getService(Blaze::SecretVault::SecretVaultSlave::COMPONENT_INFO.name);

                if (secretVaultSlave == nullptr)
                {
                    ERR_LOG("[ArsonComponent] Failed to instantiate secretVaultSlave object.");
                    return ERR_SYSTEM;
                }

                Blaze::SecretVault::SecretVaultReadRequest req;
                Blaze::SecretVault::SecretVaultSecret rsp;
                Blaze::SecretVault::SecretVaultErrorResponse err;

                req.setPath(mRequest.getPath());
                req.setVaultNamespace(mRequest.getVaultNamespace());
                req.setVaultToken(mRequest.getVaultToken());

                BlazeRpcError ret = secretVaultSlave->read(req, rsp, err);

                for (auto& errString : err.getErrors())
                {
                    mErrorResponse.getErrors().push_back(errString);
                }

                if (ret == Blaze::ERR_OK)
                {
                    secretVaultResponseToArsonResponse(rsp, mResponse);
                    return ERR_OK;
                }
                else
                {
                    ERR_LOG("[SecretVault].execute: returns error code " << ret << " (" << SbFormats::HexLower(ret) << ") instead of ERR_OK");
                    return arsonErrorFromSecretVaultError(ret);
                }
            }

            ReadCommandStub::Errors arsonErrorFromSecretVaultError(BlazeRpcError error)
            {
                switch (error)
                {
                case ::Blaze::ERR_OK: return ERR_OK;
                case ::Blaze::ERR_SYSTEM: return ERR_SYSTEM;
                case ::Blaze::ERR_TIMEOUT: return ERR_TIMEOUT;
                case ::Blaze::ERR_MOVED: return ERR_MOVED;
                case ::Blaze::ERR_GUEST_SESSION_NOT_ALLOWED: return ERR_GUEST_SESSION_NOT_ALLOWED;
                case ::Blaze::ERR_AUTHORIZATION_REQUIRED: return ERR_AUTHORIZATION_REQUIRED;
                case ::Blaze::SECRETVAULT_ERR_INVALID_REQUEST: return ARSON_SECRETVAULT_ERR_INVALID_REQUEST;
                case ::Blaze::SECRETVAULT_ERR_FORBIDDEN: return ARSON_SECRETVAULT_ERR_FORBIDDEN;
                case ::Blaze::SECRETVAULT_ERR_INVALID_PATH: return ARSON_SECRETVAULT_ERR_INVALID_PATH;
                case ::Blaze::SECRETVAULT_ERR_RATE_LIMIT_EXCEEDED: return ARSON_SECRETVAULT_ERR_RATE_LIMIT_EXCEEDED;
                case ::Blaze::SECRETVAULT_ERR_INTERNAL: return ARSON_SECRETVAULT_ERR_INTERNAL;
                case ::Blaze::SECRETVAULT_ERR_SEALED: return ARSON_SECRETVAULT_ERR_SEALED;
                default:
                {
                    //got an error not defined in rpc definition, log it
                    TRACE_LOG("[SecretVault].arsonErrorFromSecretVaultError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
                    return ERR_SYSTEM;;
                }
                }
            }
            
            //TODO: this method is common in read_command.cpp, renewlease_command.cpp, renewtoken_command.cpp
            //TODO: find a commonplace for it
            void secretVaultResponseToArsonResponse(Blaze::SecretVault::SecretVaultSecret& rsp, Blaze::Arson::SecretVaultSecret& arsonRsp)
            {
                //arsonRsp.setRequestId(rsp.getRequestId());
                arsonRsp.setLeaseId(rsp.getLeaseId());
                arsonRsp.setRenewable(rsp.getRenewable());
                arsonRsp.setLeaseDuration(rsp.getLeaseDuration());

                rsp.getData().copyInto(arsonRsp.getData());

                //arsonRsp.getWrapInfo().setCreationTime(rsp.getWrapInfo().getCreationTime());
                //arsonRsp.getWrapInfo().setToken(rsp.getWrapInfo().getToken());
                //arsonRsp.getWrapInfo().setTTL(rsp.getWrapInfo().getTTL());

                //rsp.getWarnings().copyInto(arsonRsp.getWarnings());

                //arsonRsp.getAuth().setAccessor(rsp.getAuth().getAccessor());
                //arsonRsp.getAuth().setClientToken(rsp.getAuth().getClientToken());
                //arsonRsp.getAuth().setLeaseDuration(rsp.getAuth().getLeaseDuration());
                //arsonRsp.getAuth().setRenewable(rsp.getAuth().getRenewable());
            }

        };
        DEFINE_READ_CREATE()
    } //Arson
} //Blaze
