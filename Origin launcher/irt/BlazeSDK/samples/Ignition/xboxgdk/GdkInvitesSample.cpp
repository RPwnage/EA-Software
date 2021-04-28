#if defined(EA_PLATFORM_GDK)

#include "PyroSDK/pyrosdk.h"
#include "GdkInvitesSample.h"
#include <xsapi-c/multiplayer_c.h>
#include "EAJson/JsonDomReader.h" //for JsonDomReader in onInviteCb
#include "Ignition/GameManagement.h" //for runJoinGameById in onInviteCb


namespace Ignition
{
    GDKInvitesSample::GDKInvitesSample()
    {
    }

    /*!************************************************************************************************/
    /*! \brief Called when title should initialize invites handling
        \return success/error value
    ***************************************************************************************************/
    void GDKInvitesSample::finalize()
    {
        gSampleInvites.cleanup();
    }

    HRESULT GDKInvitesSample::initHandler()
    {
        HRESULT result = XGameInviteRegisterForEvent(mTaskQueue, nullptr, GDKInvitesSample::onInviteCb, &mGameInviteEventToken);
        NetPrintf(("[GDKInvitesSample].onInviteCb: init handler(%d)\n", result));
        return result;
    }

    void GDKInvitesSample::cleanup()
    {
        XGameInviteUnregisterForEvent(mGameInviteEventToken, true);
        NetPrintf(("[GDKInvitesSample].onInviteCb: removed handler\n"));
    }

    void GDKInvitesSample::onInviteCb(void* context, const char* inviteUrl)
    {
        // Join the game session
        eastl::string inviteStr(inviteUrl ? inviteUrl : "<null>");
        NetPrintf(("[GDKInvitesSample].onInviteCb: url(%s), context(%s)\n", inviteStr.c_str(), (context ? context : "<null>")));

        // To get the MPS/game to join, extract handle from inviteUrl.
        // E.g. "ms-xbl-7a970ea1://inviteHandleAccept/?invitedXuid=2814616402722312&handle=d7853aa1-6500-4fa8-a744-e093e77a4ba8&senderXuid=2814650950488554"
        auto pos = inviteStr.find("handle=");
        auto inviteHandleId = inviteStr.substr(pos + 7, 36); //(Based on MS docs examples)  
        if (!inviteHandleId.empty())
        {
            XblContextHandle xblContextHandle;
            if (!GDKInvitesSample::getXblContext(xblContextHandle))
                return; //logged
            auto* b = new XAsyncBlock;
            ZeroMemory(b, sizeof(XAsyncBlock));
            b->context = nullptr;
            b->callback = [](XAsyncBlock* async)
            {
                XblMultiplayerSessionHandle sessionHandle{ nullptr };
                XblMultiplayerGetSessionResult(async, &sessionHandle);
                auto constants = XblMultiplayerSessionSessionConstants(sessionHandle);
                eastl::string mpsCustomConstants(constants->CustomJson);
                
                EA::Json::JsonDomReader reader;
                reader.SetString(mpsCustomConstants.c_str(), mpsCustomConstants.length(), false);
                EA::Json::JsonDomDocument doc;
                auto result = reader.Build(doc);
                if (result != EA::Json::kSuccess)
                {
                    NetPrintf(("[GDKInvitesSample].onInviteCb: Could not parse json response. error: 0x%x at index %d (%d:%d), length: %d, data:\n%s", result, (int32_t)reader.GetByteIndex(), reader.GetLineNumber(), reader.GetColumnNumber(), mpsCustomConstants.length(), mpsCustomConstants.c_str()));
                    return;
                }
                auto* sessionId = doc.GetString("/externalSessionId");
                if (!sessionId || sessionId->mValue.empty())
                {
                    NetPrintf(("[GDKInvitesSample].onInviteCb: rsp body missing its externalSessionId.\n"));
                    return;
                }
                Blaze::GameManager::GameId gameId = EA::StdC::AtoU64(sessionId->mValue.c_str());
                gBlazeHubUiDriver->getGameManagement().runJoinGameById(gameId, nullptr, Blaze::GameManager::UNSPECIFIED_TEAM_INDEX,
                    Blaze::GameManager::GAME_ENTRY_TYPE_DIRECT, nullptr, Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT, nullptr, nullptr,
                    Blaze::GameManager::JOIN_BY_INVITES);
                delete async;
            };
            HRESULT hr = XblMultiplayerGetSessionByHandleAsync(xblContextHandle, inviteHandleId.c_str(), b);
            if (!SUCCEEDED(hr))
            {
                NetPrintf(("[GDKInvitesSample].onInviteCb: failed XblMultiplayerGetSessionByHandleAsync (%d)\n", hr));
            }
        }
        else
        {
            NetPrintf(("[GDKInvitesSample].onInviteCb: url(%s) missing invite hande, unable to fetch MPS/game to join.\n", inviteStr.c_str()));
        }
    }

    /*F*******************************************************************************/
    /*! Helper to get an XblContextHandle
    ********************************************************************************F*/
    bool GDKInvitesSample::getXblContext(XblContextHandle& xblContextHandle)
    {
        XUserHandle pUser = nullptr;
        const int32_t iUserIndex = 0;
        if (NetConnStatus('xusr', iUserIndex, &pUser, sizeof(pUser)) < 0)
        {
            NetPrintf(("[Sample].getXblContext: no user at index %d.\n", iUserIndex));
            return false;
        }
        auto hr = XblContextCreateHandle(pUser, &xblContextHandle);
        if (!SUCCEEDED(hr))
        {
            NetPrintf(("[Sample].getXblContext: failed: %d\n", hr));
            return false;
        }
        return true;
    }

    Ignition::GDKInvitesSample GDKInvitesSample::gSampleInvites;


} // ns Ignition

#endif