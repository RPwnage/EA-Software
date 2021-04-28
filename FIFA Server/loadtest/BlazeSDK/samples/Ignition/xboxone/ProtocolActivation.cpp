#include "ProtocolActivation.h"

#if defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)
#include "Ignition/GameManagement.h" // for GameManagement in checkProtocolActivation

using namespace Windows::Foundation;
using namespace Windows::Xbox::Multiplayer;
using namespace Windows::Xbox::System;
using namespace Microsoft::Xbox::Services::Multiplayer;
using namespace Microsoft::Xbox::Services;

namespace Ignition
{
    ProtocolActivationInfo::ProtocolActivationInfo() : mActivationType(NONE)
    {
    }
    // based off MS's 2014 Nov XDK HighCardDraw110 sample (see MultiplayerManager::GetMultiplayerActivationInfo).
    /*!************************************************************************************************/
    /*! \brief Called when title is protocol activated and used to parse the arguments
        - It'll parse the activation args looking for the handle and xuid
        - It'll save the parsed value into wrapper class (ProtocolActivationInfo)
        \return whether activation args needing handling were initialized
    ***************************************************************************************************/
    bool ProtocolActivationInfo::initProtocolActivation(Windows::ApplicationModel::Activation::IProtocolActivatedEventArgs^ const args)
    {
        return mProtocolActivationInfo.init(args); 
    }

    bool ProtocolActivationInfo::init(Windows::ApplicationModel::Activation::IProtocolActivatedEventArgs^ const args)
    {
        if (args != nullptr && args->Uri != nullptr)
        {
            Windows::Foundation::Uri^ url = ref new Windows::Foundation::Uri(args->Uri->RawUri);

            NetPrintf(("ProtocolActivationInfo: init url raw data: %S.", args->Uri->RawUri->Data()));

            try
            {
                // This is triggered off a game invite toast
                if (url->Host == "inviteHandleAccept")
                {
                    init(INVITE_HANDLE_ACCEPT,
                        url->QueryParsed->GetFirstValueByName("handle"),
                        url->QueryParsed->GetFirstValueByName("invitedXuid"),
                        nullptr,
                        url->QueryParsed->GetFirstValueByName("senderXuid"),
                        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
                }
                else if(url->Host == "activityHandleJoin")
                {
                    init(ACTIVITY_HANDLE_JOIN,
                        url->QueryParsed->GetFirstValueByName("handle"),
                        url->QueryParsed->GetFirstValueByName("joinerXuid"),
                        url->QueryParsed->GetFirstValueByName("joineeXuid"),
                        nullptr, nullptr, nullptr,nullptr, nullptr, nullptr, nullptr);
                }
                else if (url->Host == "partyInviteAccept")
                {
                    init(PARTY_INVITE_ACCEPT,
                        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
                }
                else if (url->Host == "team")
                {
                    init(TOURNAMENT_TEAM_VIEW,
                        nullptr, nullptr, nullptr, nullptr,
                        getQueryStringValue(url, "tournamentId"),
                        getQueryStringValue(url, "organizerId"),
                        nullptr,
                        getQueryStringValue(url, "name"),
                        getQueryStringValue(url, "templateName"),
                        getQueryStringValue(url, "scid"));
                }
                else if (url->Host == "tournament")
                {
                    init(TOURNAMENT,
                        nullptr, 
                        getQueryStringValue(url, "joinerXuid"),
                        getQueryStringValue(url, "memberId"),
                        nullptr,
                        getQueryStringValue(url, "tournamentId"),
                        getQueryStringValue(url, "organizerId"),
                        getQueryStringValue(url, "state"),
                        getQueryStringValue(url, "name"),
                        getQueryStringValue(url, "templateName"),
                        getQueryStringValue(url, "scid"));
                }
            }
            catch(Platform::Exception^ ex)
            {
                NetPrintf(("ProtocolActivationInfo: init failed: %S\n", getErrorStringForException(ex)->Data()));
                return false;
            }
        }
        return (mActivationType != NONE);
    }

    void ProtocolActivationInfo::init(ProtocolActivationType activationType, Platform::String^ const handleId,
                                      Platform::String^ const targetXuid, Platform::String^ const joineeXuid,
                                      Platform::String^ const senderXuid,
                                      Platform::String^ const tournamentId, Platform::String^ const organizerId,
                                      Platform::String^ const registrationState,
                                      Platform::String^ const sessionName, Platform::String^ const templateName, Platform::String^ const scid)
    {
        mActivationType = activationType;
        mHandleId = handleId;
        mTargetXuid = targetXuid;
        mJoineeXuid = joineeXuid;
        mSenderXuid = senderXuid;
        mTournamentId = tournamentId;
        mOrganizerId = organizerId;
        mRegistrationState = registrationState;
        mSessionName = sessionName;
        mTemplateName = templateName;
        mScid = scid;
    }

    Ignition::ProtocolActivationInfo ProtocolActivationInfo::mProtocolActivationInfo;
    /*! ************************************************************************************************/
    /*! \brief Checks whether there was a protocol activation that needs to be handled, if so handle it.
    ***************************************************************************************************/
    void ProtocolActivationInfo::checkProtocolActivation()
    {
        ProtocolActivationType type = mProtocolActivationInfo.getActivationType();
        if (type != NONE)
        {
            // Need to handle game join on startup.
            // If user not fully logged in, keep the info cached off for joining game later.
            if ((gBlazeHubUiDriver == nullptr) || (gBlazeHub == nullptr) || (gBlazeHub->getGameManagerAPI() == nullptr) || (gBlazeHub->getConnectionManager() == nullptr))
            {
                NetPrintf(("ProtocolActivationInfo: checkProtocolActivationStartup cannot run game manager operations yet as game manager api is not yet initialized."));
                return;
            }

            // If Blaze ping site data not ready (needed before game joins) wait for onQosPingSiteLatencyRetrieved first.
            if (!gBlazeHub->getConnectionManager()->initialQosPingSiteLatencyRetrieved())
                return;

            // If network adapter not yet initialized init it so we can join (i.e. we beat GMAPI's onAuthenticated to it here).
            if (!gBlazeHub->getGameManagerAPI()->getNetworkAdapter()->isInitialized())
            {
                gBlazeHub->getGameManagerAPI()->getNetworkAdapter()->initialize(gBlazeHub);
            }

            // Kick off join game process (async call).
            if ((type == ProtocolActivationInfo::ACTIVITY_HANDLE_JOIN) ||
                (type == ProtocolActivationInfo::INVITE_HANDLE_ACCEPT))
            {
                processJoinByHandle(mProtocolActivationInfo.getHandleId(), (type == ProtocolActivationInfo::INVITE_HANDLE_ACCEPT));
            }
            else if (type == ProtocolActivationInfo::PARTY_INVITE_ACCEPT)
            {
                processAcceptPartyInvite();
            }
            else if (type == ProtocolActivationInfo::TOURNAMENT_TEAM_VIEW)
            {
                processTournamentTeamView(mProtocolActivationInfo.getTournamentId(), mProtocolActivationInfo.getOrganizerId(), mProtocolActivationInfo.getSessionName(),
                    mProtocolActivationInfo.getTemplateName(), mProtocolActivationInfo.getScid());
            }
            else if (type == ProtocolActivationInfo::TOURNAMENT)
            {
                processTournamentActivation(mProtocolActivationInfo.getTargetXuid(), mProtocolActivationInfo.getJoineeXuid(), mProtocolActivationInfo.getTournamentId(),
                    mProtocolActivationInfo.getOrganizerId(), mProtocolActivationInfo.getRegistrationState(),
                    mProtocolActivationInfo.getSessionName(), mProtocolActivationInfo.getTemplateName(), mProtocolActivationInfo.getScid());
            }
            // kicked off protocol activation's handling. Clear the info to avoid doing it again.
            mProtocolActivationInfo.clear();
        }
    }


    /*! ************************************************************************************************/
    /*! \brief join game by Xbox/shell UX invite accept or 'Join' button click
    ***************************************************************************************************/
    void ProtocolActivationInfo::processJoinByHandle(Platform::String^ const handleStr, bool fromInvite)
    {
        // fetch the MPS by the activity handle in order to get the GameType
        NetPrintf(("processJoinByHandle: attempting to fetch and join session by handle: %S (type:%s)\n", handleStr->Data(), (fromInvite? "invite accept" : "join button click")));
        Microsoft::Xbox::Services::XboxLiveContext^ xblContext = ref new XboxLiveContext(Windows::Xbox::System::User::Users->GetAt(0));
    
        IAsyncOperation<Microsoft::Xbox::Services::Multiplayer::MultiplayerSession ^>^ mpsOp = 
            xblContext->MultiplayerService->GetCurrentSessionByHandleAsync(handleStr);
    
        mpsOp->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<MultiplayerSession ^>(
            [fromInvite](IAsyncOperation<MultiplayerSession ^>^ mpsOp, Windows::Foundation::AsyncStatus status)
        {
            if (status == Windows::Foundation::AsyncStatus::Completed)
            {
                Blaze::GameManager::GameId gameId = Ignition::GameManagement::getGameIdFromMps(mpsOp->GetResults());
                Platform::String^ customDataRole = Ignition::GameManagement::getCustomDataStringFromMps(mpsOp->GetResults());
                eastl::string buf;
    
                Blaze::GameManager::RoleNameToPlayerMap joiningRoles;
                Ignition::GameManagement::setDefaultRole(joiningRoles, (customDataRole->IsEmpty() ? Blaze::GameManager::PLAYER_ROLE_NAME_ANY : toEastlStr(buf, customDataRole)));

                gBlazeHubUiDriver->getGameManagement().runJoinGameById(gameId, nullptr, Blaze::GameManager::UNSPECIFIED_TEAM_INDEX,
                    Blaze::GameManager::GAME_ENTRY_TYPE_DIRECT, &joiningRoles, Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT, nullptr, nullptr,
                    (fromInvite? Blaze::GameManager::JOIN_BY_INVITES : Blaze::GameManager::JOIN_BY_PLAYER));
            }
            else if (status == Windows::Foundation::AsyncStatus::Error)
            {
                NetPrintf(("processJoinByHandle: error: %S/0x%x\n", mpsOp->ErrorCode.ToString()->Data(), mpsOp->ErrorCode.Value));
            }
            mpsOp->Close();
        });
    }

    /*! ************************************************************************************************/
    /*! \brief join game by Xbox/shell UX Party invite accept
    ***************************************************************************************************/
    void ProtocolActivationInfo::processAcceptPartyInvite()
    {
        #if defined(EA_PLATFORM_XBOXONE) && (_XDK_VER < 0x295A0401)
        IAsyncOperation<PartyView ^>^ partyOperation = Windows::Xbox::Multiplayer::Party::GetPartyViewAsync();
        partyOperation->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<PartyView ^>(
            [](IAsyncOperation<PartyView ^>^ partyOp, Windows::Foundation::AsyncStatus status)
        {
            Windows::Xbox::Multiplayer::PartyView^ const party = partyOp->GetResults();
            if ((status == Windows::Foundation::AsyncStatus::Completed) && (party != nullptr) && (party->GameSession != nullptr))
            {
                // fetch the MPS
                Microsoft::Xbox::Services::XboxLiveContext^ xblContext = ref new XboxLiveContext(Windows::Xbox::System::User::Users->GetAt(0));
                Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionReference^ mpsRef = ref new Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionReference(
                    party->GameSession->ServiceConfigurationId, party->GameSession->SessionTemplateName, party->GameSession->SessionName);

                IAsyncOperation<Microsoft::Xbox::Services::Multiplayer::MultiplayerSession ^>^ mpsOp =
                    xblContext->MultiplayerService->GetCurrentSessionAsync(mpsRef);

                mpsOp->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<MultiplayerSession ^>(
                    [](IAsyncOperation<MultiplayerSession ^>^ mpsOp, Windows::Foundation::AsyncStatus status)
                {
                    if (status == Windows::Foundation::AsyncStatus::Completed)
                    {
                        Blaze::GameManager::GameId gameId = Ignition::GameManagement::getGameIdFromMps(mpsOp->GetResults());
                        
                        Blaze::GameManager::RoleNameToPlayerMap joiningRoles;
                        Ignition::GameManagement::setDefaultRole(joiningRoles, Blaze::GameManager::PLAYER_ROLE_NAME_ANY);

                        gBlazeHubUiDriver->getGameManagement().runJoinGameById(gameId, nullptr, Blaze::GameManager::UNSPECIFIED_TEAM_INDEX,
                            Blaze::GameManager::GAME_ENTRY_TYPE_DIRECT, &joiningRoles, Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT, nullptr, nullptr,
                            Blaze::GameManager::JOIN_BY_INVITES);
                    }
                    else if (status == Windows::Foundation::AsyncStatus::Error)
                    {
                        NetPrintf(("processAcceptPartyInvite: error: %S/0x%x\n", mpsOp->ErrorCode.ToString()->Data(), mpsOp->ErrorCode.Value));
                    }
                    mpsOp->Close();
                });
            }
            partyOp->Close();
        });
        #endif
    }

    void ProtocolActivationInfo::processTournamentTeamView(Platform::String^ const tournamentId, Platform::String^ const organizerId, Platform::String^ const sessionName, Platform::String^ const templateName, Platform::String^ const scid)
    {
#if defined(EA_PLATFORM_XBOXONE) && (_XDK_VER >= __XDK_VER_2017_ARENA)
        NetPrintf(("processTournamentTeamView: activation for viewing tournament(%S), organizer(%S) team(sessionName=%S:template=%S)\n",
            tournamentId->Data(), organizerId->Data(), sessionName->Data(), templateName->Data()));

        // implement tournament organizer handling here
#endif
    }

    /*! ************************************************************************************************/
    /*! \brief handle tournament Xbox/shell UX activations
    ***************************************************************************************************/
    void ProtocolActivationInfo::processTournamentActivation(Platform::String^ const joinerXuid, Platform::String^ const registrationXuid,
        Platform::String^ const tournamentId, Platform::String^ const organizerId,
        Platform::String^ const registrationState,
        Platform::String^ const sessionName, Platform::String^ const templateName, Platform::String^ const scid)
    {
        if (joinerXuid != nullptr && !joinerXuid->IsEmpty())
        {
            processTournamentSessionJoin(joinerXuid, sessionName, templateName, scid);
        }
        else if (registrationState != nullptr && registrationState == "register")
        {
            processTournamentRegister(tournamentId, organizerId, registrationXuid, scid);
        }
        else if (registrationState != nullptr && registrationState == "withdraw")
        {
            processTournamentWithdraw(tournamentId, organizerId, registrationXuid, scid);
        }
        else if (tournamentId != nullptr && !tournamentId->IsEmpty())
        {
            processTournamentView(tournamentId, organizerId, scid);
        }
        else
        {
            NetPrintf(("ProtocolActivationInfo: processTournamentActivation unhandled activation string!\n"));
        }
    }

    void ProtocolActivationInfo::processTournamentSessionJoin(Platform::String^ const joinerXuid, Platform::String^ const sessionName, Platform::String^ const templateName, Platform::String^ const scid)
    {
        try
        {
            NetPrintf(("processTournamentSessionJoin: activation for joining user(%S) to tournament match/lobby for (sessionName=%S:template=%S)\n", joinerXuid->Data(), sessionName->Data(), templateName->Data()));

            // fetch the input Multiplayer Session
            Microsoft::Xbox::Services::XboxLiveContext^ xblContext = ref new XboxLiveContext(Windows::Xbox::System::User::Users->GetAt(0));
            Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionReference^ inputMpsRef = ref new Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionReference(scid, templateName, sessionName);

            IAsyncOperation<Microsoft::Xbox::Services::Multiplayer::MultiplayerSession ^>^ inputMpsOp = xblContext->MultiplayerService->GetCurrentSessionAsync(inputMpsRef);
            inputMpsOp->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<MultiplayerSession ^>(
                [xblContext](IAsyncOperation<MultiplayerSession ^>^ inputMpsOp, Windows::Foundation::AsyncStatus inputMpsOpStatus)
            {
                if (inputMpsOpStatus == Windows::Foundation::AsyncStatus::Completed)
                {
                    // New Tournament Hubs based Team APIs:
                    // The input MultiplayerSession is the game to join
                    
                    Blaze::GameManager::GameId gameId = Ignition::GameManagement::getGameIdFromMps(inputMpsOp->GetResults());
                    gBlazeHubUiDriver->getGameManagement().runJoinGameById(gameId, nullptr, Blaze::GameManager::UNSPECIFIED_TEAM_INDEX,
                        Blaze::GameManager::GAME_ENTRY_TYPE_CLAIM_RESERVATION, nullptr, Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT, nullptr, nullptr, Blaze::GameManager::JOIN_BY_INVITES);
                }
                else if (inputMpsOpStatus == Windows::Foundation::AsyncStatus::Error)
                {
                    NetPrintf(("processTournamentSessionJoin: error fetching team MPS: %S/0x%x\n", inputMpsOp->ErrorCode.ToString()->Data(), inputMpsOp->ErrorCode.Value));
                }
                inputMpsOp->Close();
            });
        }
        catch (Platform::Exception^ ex)
        {
            NetPrintf(("getGameMpsFrominputMpsRendezvous: failed: %S, failed to parse tournaments section\n", ProtocolActivationInfo::getErrorStringForException(ex)->Data()));
        }
    }

    void ProtocolActivationInfo::processTournamentRegister(Platform::String^ const tournamentId, Platform::String^ const organizerId, Platform::String^ const targetXuid, Platform::String^ const scid)
    {
        NetPrintf(("processTournamentRegister: activation for registering user(%s) for tournament(%S), organizer(%S)\n", targetXuid->Data(), tournamentId->Data(), organizerId->Data()));
        
        // implement tournament organizer handling here
    }

    void ProtocolActivationInfo::processTournamentWithdraw(Platform::String^ const tournamentId, Platform::String^ const organizerId, Platform::String^ const targetXuid, Platform::String^ const scid)
    {
        NetPrintf(("processTournamentWithdraw: activation for withdrawing user(%s) for tournament(%S), organizer(%S)\n", targetXuid->Data(), tournamentId->Data(), organizerId->Data()));
        
        // implement tournament organizer handling here
    }

    void ProtocolActivationInfo::processTournamentView(Platform::String^ const tournamentId, Platform::String^ const organizerId, Platform::String^ const scid)
    {
        NetPrintf(("processTournamentView: activation for viewing tournament(%S), organizer(%S)\n", tournamentId->Data(), organizerId->Data()));
        // implement tournament organizer handling here
    }

    Platform::String^ const ProtocolActivationInfo::getQueryStringValue(Windows::Foundation::Uri^ const uri, Platform::String^ const paramName)
    {
        auto parsedQuery = uri->QueryParsed;
        for (unsigned int i = 0; i < parsedQuery->Size; ++i)
        {
            auto next = parsedQuery->GetAt(i);
            if (next->Name == paramName)
            {
                return next->Value;
            }
        }
        return nullptr;
    }

    // MS type to blaze type
    const char8_t* ProtocolActivationInfo::toEastlStr(eastl::string& handleIdBuf, Platform::String^ const platformString)
    {
        char8_t charBuf[4096];
        const wchar_t* rawWchar = platformString->Data();
        wcsrtombs(charBuf, &rawWchar, sizeof(charBuf), nullptr);
        handleIdBuf = charBuf;
        return handleIdBuf.c_str();
    }

    // This helper is used for more than _DEBUG so keep it outside the ifdef
    Platform::String^ ProtocolActivationInfo::getErrorStringForException(Platform::Exception^ ex)
    {
        if (ex == nullptr || ex->Message == nullptr || ex->Message->Length() == 0)
        {
            wchar_t buf[2048];

            swprintf_s(buf, 2048, L"Unknown error: 0x%x\n", ex->HResult);

            return ref new Platform::String(buf);
        }

        return ex->Message;
    }

} // ns Ignition

#endif