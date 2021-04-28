/*************************************************************************************************/
/*!
    \file   checktokenretrievalandrefreshment_command.cpp


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
#include "arson/rpc/arsonslave/checktokenretrievalandrefreshment_stub.h"

#include "framework/usersessions/usersession.h"
#include "framework/oauth/accesstokenutil.h"

namespace Blaze
{
    namespace Arson
    {
        class CheckTokenRetrievalAndRefreshmentCommand : public CheckTokenRetrievalAndRefreshmentCommandStub
        {
        public:
            CheckTokenRetrievalAndRefreshmentCommand(Message* message, Blaze::Arson::CheckTokenRetrievalAndRefreshmentRequest* request, ArsonSlaveImpl* componentImpl)
                : CheckTokenRetrievalAndRefreshmentCommandStub(message, request),
                mComponent(componentImpl)
            {
            }

            ~CheckTokenRetrievalAndRefreshmentCommand() override { }

        private:
            // Not owned memory
            ArsonSlaveImpl* mComponent;

            CheckTokenRetrievalAndRefreshmentCommandStub::Errors execute() override
            {
                INFO_LOG("[CheckTokenRetrievalAndRefreshmentCommand].execute:" << mRequest);

                BlazeRpcError err = Blaze::ERR_SYSTEM;

                if (!mRequest.getIsNucleusAvailable() && mRequest.getIsTokenRefreshedAfterInterval())
                {
                    ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].execute: Cannot expect tokens get refreshed while nucleus is not available. Invalid request.");
                    return commandErrorFromBlazeError(err);
                }

                if (gCurrentUserSession == nullptr)
                {
                    ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].execute: gCurrentUserSession is nullptr");
                    return commandErrorFromBlazeError(err);
                }

                if (mRequest.getCheckServerToken())
                {
                    err = checkServerToken();
                    if (err != Blaze::ERR_OK)
                    {
                        ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].execute: checkServerToken failed");
                        return commandErrorFromBlazeError(err);
                    }
                }
                else
                {
                    err = checkUserToken();
                    if (err != Blaze::ERR_OK)
                    {
                        ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].execute: checkUserToken failed");
                        return commandErrorFromBlazeError(err);
                    }
                }
                
                return commandErrorFromBlazeError(err);
            }

            BlazeRpcError checkUserToken()
            {
                BlazeRpcError err = Blaze::ERR_SYSTEM;

                OAuth::AccessTokenUtil userTokenUtil;
                err = userTokenUtil.retrieveCurrentUserAccessToken(OAuth::TOKEN_TYPE_NONE);
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].checkUserToken: Failed to retrieve user access token with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                OAuth::AccessTokenUtil restrictedScopeServerTokenUtil;
                err = restrictedScopeServerTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_NONE);
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].checkUserToken: Failed to retrieve restricted scope server access token with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                OAuth::AccessTokenUtil fullScopeServerTokenUtil;
                {
                    //retrieve full scope server access token in a block to avoid changing function context permission
                    UserSession::SuperUserPermissionAutoPtr autoPtr(true);
                    err = fullScopeServerTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_NONE);
                    if (err != Blaze::ERR_OK)
                    {
                        ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].checkUserToken: Failed to retrieve full scope server access token with error(" << ErrorHelp::getErrorName(err) << ")");
                        return err;
                    }
                }

                uint16_t failedCount = 0;
                failedCount += compareTokens(false, "token", userTokenUtil.getAccessToken(), restrictedScopeServerTokenUtil.getAccessToken());
                failedCount += compareTokens(false, "token", userTokenUtil.getAccessToken(), fullScopeServerTokenUtil.getAccessToken());

                if (failedCount != 0)
                {
                    err = Blaze::ERR_SYSTEM;
                    ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].checkUserToken: " << failedCount << " checks failed before sleep");
                    return err;
                }

                //sleep longer than the task internal to make sure the task is run
                int64_t sleepTime = mRequest.getCachedTokenUpdateInterval().getMicroSeconds() * 6 / 5;
                err = gSelector->sleep(sleepTime);
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].checkUserToken: sleep failed");
                    return err;
                }

                OAuth::AccessTokenUtil newUserTokenUtil;
                err = newUserTokenUtil.retrieveCurrentUserAccessToken(OAuth::TOKEN_TYPE_NONE);
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].checkUserToken: Failed to retrieve user access token with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                if (mRequest.getIsTokenRefreshedAfterInterval())
                    failedCount += compareTokens(false, "userToken", userTokenUtil.getAccessToken(), newUserTokenUtil.getAccessToken());
                else
                    failedCount += compareTokens(true, "userToken", userTokenUtil.getAccessToken(), newUserTokenUtil.getAccessToken());

                if (failedCount != 0)
                {
                    err = Blaze::ERR_SYSTEM;
                    ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].checkUserToken: " << failedCount << " checks failed after sleep");
                    return err;
                }

                return err;
            }

            BlazeRpcError checkServerToken()
            {
                BlazeRpcError err = Blaze::ERR_SYSTEM;
                const char8_t* specificScope = "basic.identity basic.persona basic.entitlement";

                OAuth::AccessTokenUtil specificScopeServerTokenUtil;
                err = specificScopeServerTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_NONE, specificScope);
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].checkServerToken: Failed to retrieve specific scope server access token with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                OAuth::AccessTokenUtil restrictedScopeServerTokenUtil;
                err = restrictedScopeServerTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_NONE);
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].checkServerToken: Failed to retrieve restricted scope server access token with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                OAuth::AccessTokenUtil fullScopeServerTokenUtil;
                {
                    //retrieve full scope server access token in a block to avoid changing function context permission
                    UserSession::SuperUserPermissionAutoPtr autoPtr(true);
                    err = fullScopeServerTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_NONE);
                    if (err != Blaze::ERR_OK)
                    {
                        ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].checkServerToken: Failed to retrieve full scope server access token with error(" << ErrorHelp::getErrorName(err) << ")");
                        return err;
                    }
                }

                uint16_t failedCount = 0;
                failedCount += compareTokens(false, "server token", restrictedScopeServerTokenUtil.getAccessToken(), specificScopeServerTokenUtil.getAccessToken());
                failedCount += compareTokens(false, "server token", restrictedScopeServerTokenUtil.getAccessToken(), fullScopeServerTokenUtil.getAccessToken());

                if (mRequest.getIsNucleusAvailable())
                    failedCount += compareTokens(false, "server token", specificScopeServerTokenUtil.getAccessToken(), fullScopeServerTokenUtil.getAccessToken());
                else
                    failedCount += compareTokens(true, "server token", specificScopeServerTokenUtil.getAccessToken(), fullScopeServerTokenUtil.getAccessToken());

                if (failedCount != 0)
                {
                    err = Blaze::ERR_SYSTEM;
                    ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].checkServerToken: " << failedCount << " checks failed before sleep");
                    return err;
                }

                //sleep longer than the task internal to make sure the task is run
                int64_t sleepTime = mRequest.getCachedTokenUpdateInterval().getMicroSeconds() * 6 / 5;
                err = gSelector->sleep(sleepTime);
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].checkServerToken: sleep failed");
                    return err;
                }

                OAuth::AccessTokenUtil newSpecificScopeServerTokenUtil;
                err = newSpecificScopeServerTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_NONE, specificScope);
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].checkServerToken: Failed to retrieve specific scope server access token with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                OAuth::AccessTokenUtil newRestrictedScopeServerTokenUtil;
                err = newRestrictedScopeServerTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_NONE);
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].checkServerToken: Failed to retrieve restricted scope server access token with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                OAuth::AccessTokenUtil newFullScopeServerTokenUtil;
                {
                    //retrieve full scope server access token in a block to avoid changing function context permission
                    UserSession::SuperUserPermissionAutoPtr autoPtr(true);
                    err = newFullScopeServerTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_NONE);
                    if (err != Blaze::ERR_OK)
                    {
                        ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].checkServerToken: Failed to retrieve full scope server access token with error(" << ErrorHelp::getErrorName(err) << ")");
                        return err;
                    }
                }

                bool expectEqual = !mRequest.getIsTokenRefreshedAfterInterval();
                failedCount += compareTokens(expectEqual, "restrictedScopeServerToken", restrictedScopeServerTokenUtil.getAccessToken(), newRestrictedScopeServerTokenUtil.getAccessToken());
                failedCount += compareTokens(expectEqual, "fullScopeServerToken", fullScopeServerTokenUtil.getAccessToken(), newFullScopeServerTokenUtil.getAccessToken());

                if (mRequest.getIsNucleusAvailable())
                    failedCount += compareTokens(false, "specificScopeServerToken", specificScopeServerTokenUtil.getAccessToken(), newSpecificScopeServerTokenUtil.getAccessToken());
                else
                    failedCount += compareTokens(true, "specificScopeServerToken", specificScopeServerTokenUtil.getAccessToken(), newSpecificScopeServerTokenUtil.getAccessToken());

                if (failedCount != 0)
                {
                    err = Blaze::ERR_SYSTEM;
                    ERR_LOG("[CheckTokenRetrievalAndRefreshmentCommand].checkServerToken: " << failedCount << " checks failed after sleep");
                    return err;
                }

                return err;
            }

            uint16_t compareTokens(const bool expectEqual, const char8_t* tokenName, const char8_t* token1, const char8_t* token2)
            {
                int32_t compareResult = blaze_strcmp(token1, token2);

                if (expectEqual)
                {
                    if (compareResult == 0)
                    {
                        return 0;
                    }
                    else
                    {
                        ERR_LOG("[CompareNucleusApiResponsesCommand].compareTokens: " << tokenName << "(" << token1 << ") is different from " << tokenName << "(" << token2 << ")");
                        return 1;
                    }
                }
                else
                {
                    if (compareResult == 0)
                    {
                        ERR_LOG("[CompareNucleusApiResponsesCommand].compareTokens: " << tokenName << "(" << token1 << ") is the same as " << tokenName << "(" << token2 << ")");
                        return 1;
                    }
                    else
                    {
                        return 0;
                    }
                }
            }

        };

        DEFINE_CHECKTOKENRETRIEVALANDREFRESHMENT_CREATE()

    } //Arson
} //Blaze
