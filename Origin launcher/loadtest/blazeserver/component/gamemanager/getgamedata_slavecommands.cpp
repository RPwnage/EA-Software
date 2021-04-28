/*! ************************************************************************************************/
/*!
    \file banplayer_slavecommand.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/getfullgamedata_stub.h"
#include "gamemanager/rpc/gamemanagerslave/getgamedatafromid_stub.h"
#include "gamemanager/rpc/gamemanagerslave/getgamedatabyuser_stub.h"
#include "gamemanagerslaveimpl.h"
#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
namespace GameManager
{

    class GetFullGameDataCommand : public GetFullGameDataCommandStub
    {
    public:

        GetFullGameDataCommand(Message* message, GetFullGameDataRequest* request, GameManagerSlaveImpl* componentImpl)
            : GetFullGameDataCommandStub(message, request),
              mComponent(*componentImpl) {}
    private:

        GetFullGameDataCommandStub::Errors execute() override
        {
            Blaze::Search::GetGamesRequest ggReq;            
            ggReq.setPersistedGameIdList(mRequest.getPersistedGameIdList());
            ggReq.setGameIds(mRequest.getGameIdList());
            ggReq.setResponseType(Search::GET_GAMES_RESPONSE_TYPE_FULL);

            Blaze::Search::GetGamesResponse ggResp;
            ggResp.setFullGameData(mResponse.getGames());

            return commandErrorFromBlazeError(mComponent.fetchGamesFromSearchSlaves(ggReq, ggResp));
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    DEFINE_GETFULLGAMEDATA_CREATE();

    class GetGameDataByUserCommand : public GetGameDataByUserCommandStub
    {
    public:

        GetGameDataByUserCommand(Message* message, GetGameDataByUserRequest* request, GameManagerSlaveImpl* componentImpl)
            : GetGameDataByUserCommandStub(message, request),
            mComponent(*componentImpl) {}
    private:

        GetGameDataByUserCommandStub::Errors execute() override
        {
            convertToPlatformInfo(mRequest.getUser());


            Blaze::Search::GetGamesRequest ggReq;            
            ggReq.setListConfigName(mRequest.getListConfigName());
            ggReq.setResponseType(Search::GET_GAMES_RESPONSE_TYPE_GAMEBROWSER);

            if (mRequest.getUserSetId().isSet()) 
            {
                BlazeRpcError userSetLookupError = gUserSetManager->getUserBlazeIds(mRequest.getUserSetId(), ggReq.getBlazeIds());
                if (userSetLookupError != ERR_OK)
                {
                    return commandErrorFromBlazeError(userSetLookupError);
                }
            }

            const bool hasValidUserId = UserInfo::isUserIdentificationValid(mRequest.getUser());
            const int32_t singleUserCount = hasValidUserId ? 1 : 0;
            const uint32_t maxSyncSize = mComponent.getConfig().getGameBrowser().getMaxGameListSyncSize();
            const uint32_t maxRequestSize = (maxSyncSize == 0) ? maxSyncSize : (maxSyncSize - singleUserCount);

            // Remove offline users and limit the number of users to the configured MaxGameListSyncSize
            // This is because the UserSetManager could return a large number of users (e.g., large association lists)
            // that the client is not necessarily aware of. Rather than returning an error, make an effort to fetch
            // some games.
            size_t numUsers = ggReq.getBlazeIds().size();
            if (numUsers > maxRequestSize)
            {
                WARN_LOG("[GetGameDataByUser].execute: UserSetManager returned (" << numUsers << ") users, greater than maxGameListSyncSize ("
                    << maxSyncSize << "). Limiting number of users to maxGameListSyncSize.");

                Blaze::BlazeIdList tempIds;
                uint32_t i = 0;
                for (BlazeId it : ggReq.getBlazeIds())
                {
                    if (gUserSessionManager->isUserOnline(it))
                    {
                        tempIds.push_back(it);
                        ++i;
                    }

                    if (i >= maxRequestSize)
                        break;
                }
                tempIds.swap(ggReq.getBlazeIds());
            }

            if (hasValidUserId)
            {
                UserInfoPtr userInfo;
                BlazeRpcError userError = gUserSessionManager->lookupUserInfoByIdentification(mRequest.getUser(), userInfo, true /*restrictCrossPlatformResults*/);
                if (userError != ERR_OK)
                {
                    if (userError == USER_ERR_DISALLOWED_PLATFORM)
                        return GetGameDataByUserCommandStub::GAMEBROWSER_ERR_DISALLOWED_PLATFORM;
                    if (userError == USER_ERR_CROSS_PLATFORM_OPTOUT)
                        return GetGameDataByUserCommandStub::GAMEBROWSER_ERR_CROSS_PLATFORM_OPTOUT;
                    return commandErrorFromBlazeError(userError);
                }
                
                ggReq.getBlazeIds().push_back(userInfo->getId());
            }
            
            Blaze::Search::GetGamesResponse ggResp;
            ggResp.setGameBrowserData(mResponse.getGameData());

            return commandErrorFromBlazeError(mComponent.fetchGamesFromSearchSlaves(ggReq, ggResp));
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    DEFINE_GETGAMEDATABYUSER_CREATE();


    class GetGameDataFromIdCommand : public GetGameDataFromIdCommandStub
    {
    public:

        GetGameDataFromIdCommand(Message* message, GetGameDataFromIdRequest* request, GameManagerSlaveImpl* componentImpl)
            : GetGameDataFromIdCommandStub(message, request),
            mComponent(*componentImpl) {}
    private:

        GetGameDataFromIdCommandStub::Errors execute() override
        {
            Blaze::Search::GetGamesRequest ggReq;            
            ggReq.setGameIds(mRequest.getGameIds());
            ggReq.setPersistedGameIdList(mRequest.getPersistedGameIdList());
            ggReq.setListConfigName(mRequest.getListConfigName());
            ggReq.setResponseType(Search::GET_GAMES_RESPONSE_TYPE_GAMEBROWSER);

            Blaze::Search::GetGamesResponse ggResp;
            ggResp.setGameBrowserData(mResponse.getGameData());

            return commandErrorFromBlazeError(mComponent.fetchGamesFromSearchSlaves(ggReq, ggResp));
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    DEFINE_GETGAMEDATAFROMID_CREATE();


} // namespace GameManager
} // namespace Blaze
