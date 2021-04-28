#if defined(EA_PLATFORM_GDK)

#include "PyroSDK/pyrosdk.h"
#include "GdkInvitesSample.h"
#include <xsapi-c/multiplayer_c.h>
#include <XGame.h> //for XGameGetXboxTitleId() in sendInvite()
#include "EAJson/JsonDomReader.h" //for JsonDomReader in onShellUxEventCb
#include "Ignition/GameManagement.h" //for runJoinGameById in onShellUxEventCb


namespace Ignition
{
    GDKInvitesSample::GDKInvitesSample()
    {
    }
    GDKInvitesSample::~GDKInvitesSample()
    {
        cleanup();
    }

    /*!************************************************************************************************/
    /*! \brief Called when title should initialize shell UX joins and invites handling
        \return success/error value
    ***************************************************************************************************/
    HRESULT GDKInvitesSample::initHandler()
    {
        HRESULT result = XGameInviteRegisterForEvent(mTaskQueue, nullptr, GDKInvitesSample::onShellUxEventCb, &mGameInviteEventToken);
        NetPrintf(("[GDKInvitesSample].onShellUxEventCb: init handler(%d)\n", result));
        return result;
    }
    void GDKInvitesSample::finalize()
    {
        gSampleInvites.cleanup();
    }
    void GDKInvitesSample::cleanup()
    {
        XGameInviteUnregisterForEvent(mGameInviteEventToken, true);
        NetPrintf(("[GDKInvitesSample].onShellUxEventCb: removed handler\n"));
    }

    /*!************************************************************************************************/
    /*! \brief parse type of event from url
    ***************************************************************************************************/
    bool isMpsJoinUrl(const eastl::string& eventUrlStr)
    {
        return (eventUrlStr.find("://activityHandleJoin/?") != eastl::string::npos);
    }
    bool isMpsInviteAcceptUrl(const eastl::string& eventUrlStr)
    {
        return (eventUrlStr.find("://inviteHandleAccept/?") != eastl::string::npos);
    }
    /*!************************************************************************************************/
    /*! \brief Handle shell UX join or invite-accept (triggered by local user from console UX)
    ***************************************************************************************************/
    void GDKInvitesSample::onShellUxEventCb(void* context, const char* eventUrl)
    {
        eastl::string eventUrlStr(eventUrl ? eventUrl : "<null>");
        NetPrintf(("onShellEventCb: url(%s)\n", eventUrlStr.c_str()));
        if (isMpsJoinUrl(eventUrlStr) || isMpsInviteAcceptUrl(eventUrlStr))
        {
            onShellUxEventMpsd(eventUrlStr);
        }
        //non MPSD currently unsupported
    }

    //  E.g. "ms-xbl-7a970ea1://activityHandleJoin/?joinerXuid=123&handle=d7853aa1-6500-4fa8-a744-e093e77a4ba8&joineeXuid=321"   (for join)
    //  E.g. "ms-xbl-7a970ea1://inviteHandleAccept/?invitedXuid=123&handle=d7853aa1-6500-4fa8-a744-e093e77a4ba8&senderXuid=321"  (for invite-accept)
    void GDKInvitesSample::onShellUxEventMpsd(const eastl::string& eventUrlStr)
    {
        // To get the MPS/game to join, extract handle from eventUrlStr.
        eastl::string inviteHandleId;
        if (parseParamValueFromUrl(inviteHandleId, "handle", eventUrlStr, 36).empty()) //36 based on MS samples/doc
        {
            NetPrintf(("onShellUxEventMpsd: url(%s) missing handle, unable to fetch MPS/game to join.\n", eventUrlStr.c_str()));
            return;
        }
        XblContextHandle xblContextHandle;
        if (!GDKInvitesSample::getXblContext(xblContextHandle))
            return; //logged
        auto* b = new XAsyncBlock;
        ZeroMemory(b, sizeof(XAsyncBlock));
        b->context = nullptr;
        b->callback = [](XAsyncBlock* async)
        {
            std::unique_ptr<XAsyncBlock> asyncBlockPtr{ async }; // Take over ownership of the XAsyncBlock*
            XblMultiplayerSessionHandle sessionHandle{ nullptr };
            HRESULT hr2 = XblMultiplayerGetSessionResult(async, &sessionHandle);
            if (!SUCCEEDED(hr2))
            {
                NetPrintf(("onShellUxEventMpsd: failed XblMultiplayerGetSessionResult (%d)\n", hr2));
                return;
            }
            auto constants = XblMultiplayerSessionSessionConstants(sessionHandle);
            eastl::string mpsCustomConstants(constants->CustomJson);

            EA::Json::JsonDomReader reader;
            reader.SetString(mpsCustomConstants.c_str(), mpsCustomConstants.length(), false);
            EA::Json::JsonDomDocument doc;
            auto result = reader.Build(doc);
            if (result != EA::Json::kSuccess)
            {
                NetPrintf(("onShellUxEventMpsd: Could not parse json response. error: 0x%x at index %d (%d:%d), length: %d, data:\n%s", result, (int32_t)reader.GetByteIndex(), reader.GetLineNumber(), reader.GetColumnNumber(), mpsCustomConstants.length(), mpsCustomConstants.c_str()));
                return;
            }
            auto* sessionId = doc.GetString("/externalSessionId");
            if (!sessionId || sessionId->mValue.empty())
            {
                NetPrintf(("onShellUxEventMpsd: rsp body missing its externalSessionId.\n"));
                return;
            }
            Blaze::GameManager::GameId gameId = EA::StdC::AtoU64(sessionId->mValue.c_str());
            gBlazeHubUiDriver->getGameManagement().runJoinGameById(gameId, nullptr, Blaze::GameManager::UNSPECIFIED_TEAM_INDEX,
                Blaze::GameManager::GAME_ENTRY_TYPE_DIRECT, nullptr);
        };
        HRESULT hr = XblMultiplayerGetSessionByHandleAsync(xblContextHandle, inviteHandleId.c_str(), b);
        if (!SUCCEEDED(hr))
        {
            NetPrintf(("onShellUxEventMpsd: failed XblMultiplayerGetSessionByHandleAsync (%d)\n", hr));
        }
    }






    /////////////// SENDING INVITES

    /*F*******************************************************************************/
    /*! Send invite to Game
    ********************************************************************************F*/
    void GDKInvitesSample::sendInvite(const Blaze::GameManager::Game& game, Blaze::ExternalXblAccountId toXuid)
    {
        if (game.getPresenceMode() == Blaze::PRESENCE_MODE_NONE)
        {
            NetPrintf(("sendInvite: trying to send invites to a game with PresenceMode(%s) unsupported, NOOP.\n", Blaze::PresenceModeToString(game.getPresenceMode())));
            return;
        }
        if (game.getExternalSessionName()[0] != '\0') // MPSD
        {
            sendInviteMpsd(game, toXuid);
        }
        //non MPSD currently unsupported
    }

    void GDKInvitesSample::sendInviteMpsd(const Blaze::GameManager::Game& game, Blaze::ExternalXblAccountId toXuid)
    {
        if ((game.getScid() == nullptr) || (game.getScid()[0] == '\0') || (game.getExternalSessionName() == nullptr) || (game.getExternalSessionName()[0] == '\0') || (game.getExternalSessionTemplateName() == nullptr) || (game.getExternalSessionTemplateName()[0] == '\0'))
        {
            NetPrintf(("sendInviteMpsd: Error", "sendInviteMpsd: game missing external session parameters,  MPS calls disabled\n"));
            return;
        }
        uint32_t titleId = 0;
        HRESULT hr = XGameGetXboxTitleId(&titleId);
        if (!SUCCEEDED(hr))
        {
            NetPrintf(("sendInviteMpsd: XGameGetXboxTitleId failed: %d\n", hr));
            return;
        }
        XblContextHandle xblContextHandle;
        if (!GDKInvitesSample::getXblContext(xblContextHandle))
        {
            return; //logged
        }
        XblMultiplayerSessionReference sessionReference;
        blaze_strnzcpy(sessionReference.Scid, game.getScid(), strlen(game.getScid()) + 1);
        blaze_strnzcpy(sessionReference.SessionName, game.getExternalSessionName(), strlen(game.getExternalSessionName()) + 1);
        blaze_strnzcpy(sessionReference.SessionTemplateName, game.getExternalSessionTemplateName(), strlen(game.getExternalSessionTemplateName()) + 1);

        // setup and send the invite
        auto asyncBlock = std::make_unique<XAsyncBlock>();
        asyncBlock->queue = gSampleInvites.mTaskQueue;
        asyncBlock->context = nullptr;
        asyncBlock->callback = [](XAsyncBlock* asyncBlock)
        {
            std::unique_ptr<XAsyncBlock> asyncBlockPtr{ asyncBlock }; // Take over ownership of the XAsyncBlock*
            size_t handlesCount = 1; // must = invites requested
            XblMultiplayerInviteHandle handles[1] = {};
            HRESULT hr2 = XblMultiplayerSendInvitesResult(asyncBlock, handlesCount, handles);
            if (!SUCCEEDED(hr2))
            {
                NetPrintf(("sendInviteMpsd: XblMultiplayerSendInvitesResult failed: %d\n", hr2));
                return;
            }
            NetPrintf(("sendInviteMpsd: XblMultiplayerSendInvitesResult: inviteHandle(%.*s)\n", sizeof(handles[0].Data), handles[0].Data));
        };
        uint64_t xuids[1] = { toXuid };
        const size_t xuidsCount = 1;

        hr = XblMultiplayerSendInvitesAsync(
            xblContextHandle,
            &sessionReference,
            xuids,
            xuidsCount,
            titleId,
            " ///custominvitestrings_BlazeSampleInvite", //configured
            "Ignition invite!",
            asyncBlock.get());
        if (!SUCCEEDED(hr))
        {
            NetPrintf(("sendInviteMpsd: XblMultiplayerSendInvitesAsync failed: %d\n", hr));
            return;
        }
        // The call succeeded, so release the std::unique_ptr ownership of XAsyncBlock* since the callback will take over ownership.
        // If the call fails, the std::unique_ptr will keep ownership and delete the XAsyncBlock*
        asyncBlock.release();
    }






    eastl::string& GDKInvitesSample::parseParamValueFromUrl(eastl::string& parsedValue, const eastl::string paramNameInUrl, const eastl::string& eventUrlStr, size_t valueExplicitLen /*= eastl::string::npos*/)
    {
        parsedValue.clear();
        const eastl::string paramStartPart(eastl::string::CtorSprintf(), "%s=", paramNameInUrl.c_str()); //format e.g. "..field=<value>.."
        auto pos = eventUrlStr.find(paramStartPart);
        if (pos != eastl::string::npos)
        {
            parsedValue = eventUrlStr.substr(pos + paramStartPart.length());
            pos = ((valueExplicitLen != eastl::string::npos) ? valueExplicitLen : parsedValue.find("&")); //strip any tail
            if (pos != eastl::string::npos)
                parsedValue.erase(pos);
        }
        return parsedValue;
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