// Copyright (C) 2013 Electronic Arts. All rights reserved.

#include <limits>
#include <QDateTime>
#include <QString>
#include <QDomDocument>
#include <QDomElement>
#include <QMap>
#include <QUuid>

#include "Service/XMPPService/XMPP_ServiceArea.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "chat/OriginConnection.h"
#include "chat/Message.h"
#include "chat/ConnectedUser.h"
#include "chat/Roster.h"
#include "chat/RemoteUser.h"
#include "chat/Blocklist.h"
#include "chat/Presence.h"
#include "chat/MucRoom.h"

#include "engine/login/LoginController.h"
#include "engine/content/ContentController.h"
#include "engine/social/SocialController.h"
#include "engine/social/AvatarManager.h"
#include "engine/social/SdkPresenceController.h"
#include "engine/social/ConversationEvent.h"
#include "engine/multiplayer/SessionInfo.h"
#include "engine/multiplayer/Invite.h"
#include "engine/multiplayer/InviteController.h"

#include "LSX/LSX_Handler.h"
#include "LSX/LsxConnection.h"

#include "flows/ClientFlow.h"
#include "widgets/social/source/SocialViewController.h"

#include "EbisuError.h"

#include "lsx.h"
#include "lsxreader.h"
#include "lsxwriter.h"

// Qt
#include <QMetaType>


#define LEGACY_FRIENS_SDK_VERSION_THRESHOLD 9,10,0,0

namespace Origin
{
    namespace SDK
    {
        namespace // Empty
        {
            void populateErrorSuccessResponse(Lsx::Response &response, int responseCode)
            {
				lsx::ErrorSuccessT err;

				err.Code = responseCode;       

                lsx::Write(response.document(), err);
            }

            lsx::PresenceT presenceToLsx(const Chat::Presence &presence, bool isCurrentUser = false)
            {
                if (presence.isNull())
                {
                    // Not a valid presence - pretend they're offline
                    return lsx::PRESENCE_OFFLINE;
                }

                if (!presence.gameActivity().isNull())
                {
                    // Fold our game activity in to our presence
                    if(presence.gameActivity().joinable())
                        return lsx::PRESENCE_JOINABLE;
                    else if(presence.gameActivity().joinable_invite_only() && isCurrentUser)
                        return lsx::PRESENCE_JOINABLE_INVITE_ONLY;
                    else
                        return lsx::PRESENCE_INGAME; 
                }

                // This is a lossy conversation
                // LSX doesn't support all the possible availabilities
                // Currently the Origin client can't enter them legitimately but do our best to convert the missing ones in to
                // something sane
                switch(presence.availability())
                {
                case Chat::Presence::Chat:
                case Chat::Presence::Online:
                    return lsx::PRESENCE_ONLINE;
                case Chat::Presence::Dnd:
                    return lsx::PRESENCE_BUSY;
                case Chat::Presence::Away:
                case Chat::Presence::Xa:
                    return lsx::PRESENCE_IDLE;
                case Chat::Presence::Unavailable:
                    return lsx::PRESENCE_OFFLINE;
                }

                ORIGIN_ASSERT(0);
                return lsx::PRESENCE_OFFLINE;
            }

            lsx::FriendStateT subscriptionStateToLsx(const Chat::SubscriptionState &state)
            {
                switch(state.direction())
                {
                case Chat::SubscriptionState::DirectionNone:
					if(state.isPendingContactApproval())
					{
						return lsx::FRIENDSTATE_INVITED;
					}
					else if(state.isPendingCurrentUserApproval())
					{
						return lsx::FRIENDSTATE_REQUEST;
					}
					else
					{
						return lsx::FRIENDSTATE_NONE;
					}
				case Chat::SubscriptionState::DirectionBoth:
                    return lsx::FRIENDSTATE_MUTUAL;
                default:
                    break;
                }

                ORIGIN_ASSERT(0);
                return lsx::FRIENDSTATE_NONE;
            }

            lsx::FriendT remoteUserToLsxFriend(const Chat::RemoteUser *contact)
            {
                Engine::UserRef user = Engine::LoginController::currentUser();

                lsx::FriendT ret;

                ret.UserId = contact->nucleusId();
                ret.PersonaId = contact->nucleusPersonaId();
                ret.Persona = contact->originId();

                if (user)
                {
                    Engine::Social::AvatarManager *avatarManager = user->socialControllerInstance()->smallAvatarManager();
                    QString avatarFile = avatarManager->pathForNucleusId(contact->nucleusId());

                    if (QFile::exists(avatarFile))
                    {
                        ret.AvatarId = QDir::toNativeSeparators(avatarFile);
                    }
                }

                ret.Group = "";
                ret.State = subscriptionStateToLsx(contact->subscriptionState());
                ret.Presence = presenceToLsx(contact->presence());
                ret.RichPresence = contact->presence().statusText();

                const Chat::OriginGameActivity gameActivity(contact->presence().gameActivity());

                if (!gameActivity.isNull())
                {
                    ret.Title = gameActivity.gameTitle();
                    ret.TitleId = gameActivity.productId();
                    ret.MultiplayerId = gameActivity.multiplayerId();
                    ret.GamePresence = gameActivity.gamePresence();
                }

                return ret;
            }

            Chat::OriginConnection *chatConnection()
            {
                Engine::UserRef user = Engine::LoginController::currentUser();

                if (user.isNull())
                {
                    // No user yet
                    return NULL;
                }

                return user->socialControllerInstance()->chatConnection();
            }

            Engine::Social::ConversationManager *chatConversation()
            {
                Engine::UserRef user = Engine::LoginController::currentUser();

                if (user.isNull())
                {
                    // No user yet
                    return NULL;
                }

                return user->socialControllerInstance()->conversations();
            }

            bool getFriendsResponse(lsx::QueryFriendsResponseT &resp, bool bLegacyUsersOnly)
            {
                Chat::OriginConnection *connection = chatConnection();
                Origin::Chat::ConnectedUser *user;
                Origin::Chat::Roster *roster;

                if((connection == NULL) || ((user = connection->currentUser()) == NULL) || ((roster = user->roster()) == NULL))
                    return false;

                for (const auto contact : roster->contacts())
                {
                    // Do not include blocked users
                    if(!user->blockList()->jabberIDs().contains(contact->jabberId()))
                    {
                        if (!bLegacyUsersOnly || contact->legacyUser())
                        {
                            resp.Friends.push_back(remoteUserToLsxFriend(contact));
                        }
                    }
                }
                return true;
            }

        }

        // Singleton functions
        static XMPP_ServiceArea* mpSingleton = NULL;

        XMPP_ServiceArea* XMPP_ServiceArea::create(Lsx::LSX_Handler* handler)
        {
            if (mpSingleton == NULL)
            {
                mpSingleton = new XMPP_ServiceArea(handler);
            }
            return mpSingleton;
        }

        XMPP_ServiceArea* XMPP_ServiceArea::instance()
        {
            return mpSingleton;
        }

        void XMPP_ServiceArea::destroy()
        {
            delete mpSingleton;
            mpSingleton = NULL;
        }

        XMPP_ServiceArea::~XMPP_ServiceArea() 
        {
        }

        XMPP_ServiceArea::XMPP_ServiceArea( Lsx::LSX_Handler* handler) 
            : BaseService( handler, "XMPP" )
        {
            //registerHandler("SendChatMessage"		, ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_SendChatMessage );
            registerHandler("SubscribePresence"		, ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_SubscribePresence );
            registerHandler("UnsubscribePresence"	, ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_UnsubscribePresence );

            // PRESENCE
            registerHandler("GetPresence"		, ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_GetPresence );
            registerHandler("SetPresence"		, ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_SetPresence );
            registerHandler("GetPresenceVisibility"		, ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_GetPresenceVisibility );
            registerHandler("SetPresenceVisibility"		, ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_SetPresenceVisibility );
            registerHandler("QueryPresence"		, ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_QueryPresence );
            registerHandler("QueryFriends"		, ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_QueryFriends );
            registerHandler("QueryAreFriends"	, ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_QueryAreFriends );

            // INVITE
            registerHandler("SendInvite"		, ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_SendInvite );
            registerHandler("AcceptInvite"		, ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_AcceptInvite );

            // CHAT
            registerHandler("SendChat"          , ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_SendChat );
            registerHandler("QueryChatInfo"     , ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_QueryChatInfo );
            registerHandler("QueryChatUsers"    , ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_QueryChatUsers );
            registerHandler("LeaveChat"         , ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_LeaveChat );
            registerHandler("JoinChat"          , ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_JoinChat );
            registerHandler("CreateChat"        , ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_CreateChat );
            registerHandler("AddUserToChat"     , ( BaseService::RequestHandler ) &XMPP_ServiceArea::Request_AddUserToChat );

            registerHandler("BlockUser",			(BaseService::RequestHandler)&XMPP_ServiceArea::Request_BlockUser);	
            registerHandler("UnblockUser",			(BaseService::RequestHandler)&XMPP_ServiceArea::Request_UnblockUser );	
            registerHandler("RequestFriend",			(BaseService::RequestHandler)&XMPP_ServiceArea::Request_RequestFriend);	
            registerHandler("RemoveFriend",			(BaseService::RequestHandler)&XMPP_ServiceArea::Request_RemoveFriend );	
			registerHandler("AcceptFriendInvite",			(BaseService::RequestHandler)&XMPP_ServiceArea::Request_AcceptFriendInvite );

            ORIGIN_VERIFY_CONNECT(
                Origin::Engine::LoginController::instance(), SIGNAL(userLoggedIn(Origin::Engine::UserRef)),
                this, SLOT(userLoggedIn(Origin::Engine::UserRef)));
        }

        //void XMPP_ServiceArea::Request_SendChatMessage(Lsx::Request& request, Lsx::Response &response)
        //{
        //    Chat::OriginConnection *connection = chatConnection();

        //    if (!connection)
        //    {
        //        // We used to silently allow this to succeed
        //        // Keep for compatibility
        //        populateErrorSuccessResponse(response, EBISU_SUCCESS);
        //        return;
        //    }


        //    QDomElement command = request.commandElement();
        //    const QString user = command.attribute("ToUserId");
        //    const QString threadId = command.attribute("Thread");
        //    const QString body = command.attribute("Body");

        //    // Convert the Nucleus ID in to a Jabber ID
        //    const Chat::JabberID toJid(connection->nucleusIdToJabberId(user.toULongLong()));

        //    Chat::Message outgoingMessage("chat", connection->currentUser()->jabberId(), toJid, Chat::ChatStateActive, threadId);
        //    outgoingMessage.setBody(body);

        //    connection->sendMessage(outgoingMessage);
        //    populateErrorSuccessResponse(response, EBISU_SUCCESS);
        //}

        void XMPP_ServiceArea::Request_SubscribePresence(Lsx::Request& request, Lsx::Response & response)
        {
            populateErrorSuccessResponse(response, EBISU_ERROR_NOT_IMPLEMENTED);
        }

        void XMPP_ServiceArea::Request_UnsubscribePresence(Lsx::Request& request, Lsx::Response &response)
        {
            populateErrorSuccessResponse(response, EBISU_ERROR_NOT_IMPLEMENTED);
        }

        void XMPP_ServiceArea::Request_SetPresence(Lsx::Request& request, Lsx::Response &response)
        {
            lsx::SetPresenceT req;

            if (Read(request.document(), req))
            {
                // Find and validate our user
                Engine::UserRef user = Engine::LoginController::currentUser();
                if (user.isNull() || (static_cast<uint64_t>(user->userId()) != req.UserId))
                {
                    populateErrorSuccessResponse(response, EBISU_ERROR_INVALID_USER);
                    return;
                }

                if ((req.Presence != lsx::PRESENCE_INGAME) && (req.Presence != lsx::PRESENCE_JOINABLE) && (req.Presence != lsx::PRESENCE_JOINABLE_INVITE_ONLY))
                {
                    // This isn't allowed - and SdkPresenceController doesn't have a way to set this anyway
                    CreateErrorResponse(EBISU_ERROR_INVALID_ARGUMENT, "You can only set INGAME and JOINABLE from a game.", response);
                    return;
                }

                QString gameTitle = request.connection()->gameName(); 
                QString gameId = request.connection()->productId();
                QString multiplayerId = request.connection()->multiplayerId();

                if (multiplayerId.isEmpty() && (req.Presence == lsx::PRESENCE_JOINABLE || req.Presence == lsx::PRESENCE_JOINABLE_INVITE_ONLY))
                {
                    CreateErrorResponse(EBISU_ERROR_NO_MULTIPLAYER_ID, "Cannot set presence to JOINABLE/JOINABLE_INVITE_ONLY, when no multiplayer Id is configured on an offer.", response);
                    return;
                }

                // Try to set the game ID and title from what we know from our entitlements
                // This is important because in 9.2+ we use product IDs over the wire but the SDK passes content IDs
                {
                    using namespace Origin::Engine::Content;
                    EntitlementRef entRef = request.connection()->entitlement();

                    // Initialize 'hideTheGame' with settings from EACore.ini in case entitlement is not valid
                    bool isOverridesEnabled = Services::readSetting(Services::SETTING_OverridesEnabled).toQVariant().toBool();
                    bool isEmbargoModeDisabled = isOverridesEnabled && Services::readSetting(Services::SETTING_DisableEmbargoMode);
                    bool hideTheGame = !(isOverridesEnabled && isEmbargoModeDisabled); // EACore.ini with Override

                    if (!entRef.isNull())
                    {
                        gameId = entRef->contentConfiguration()->productId();

                        hideTheGame = entRef->localContent()->shouldHide();
                        if (hideTheGame)
                        {
                            gameTitle = tr("ebisu_client_notranslate_embargomode_title");
                            req.RichPresence = ""; // clear it for safety reasons
                        }
                        else
                            gameTitle = entRef->contentConfiguration()->displayName();
                    }
                    else if (hideTheGame)
                    {
                        gameTitle = tr("ebisu_client_notranslate_embargomode_title");
                        req.RichPresence = ""; // clear it for safety reasons
                    }
                }


                // Get some stuff from our request
                QByteArray session = req.SessionId.isEmpty() ? QByteArray() : req.SessionId.toUtf8(); 
                bool joinable = req.Presence == lsx::PRESENCE_JOINABLE;
                bool joinable_invite_only = req.Presence == lsx::PRESENCE_JOINABLE_INVITE_ONLY;
                QString status = gameTitle + " " + req.RichPresence;

                // Build a game activity from all the bits the game specified
                Chat::OriginGameActivity gameActivity(true, gameId, gameTitle, joinable, joinable_invite_only, req.GamePresence, multiplayerId);

                // Update our SDK presence. This will indirectly set our actual presence
                user->socialControllerInstance()->sdkPresence()->set(gameId, gameActivity, status, session);

                // Listen for LSX hangups so we can clear our presence
                connect(request.connection(), SIGNAL(hungup(const QString &)), this, SLOT(connectionHungup(const QString &)), Qt::UniqueConnection);

                populateErrorSuccessResponse(response, EBISU_SUCCESS);
            }
            else
            {
                populateErrorSuccessResponse(response, EBISU_ERROR_LSX_INVALID_REQUEST);
            }
        }

        void XMPP_ServiceArea::connectionHungup(const QString &productId)
        {
            Engine::UserRef user = Engine::LoginController::currentUser();

            if (!user.isNull())
            {
                // Clear out any presence the game might have set
                user->socialControllerInstance()->sdkPresence()->clearIfSetBy(productId);
            }
        }

        void XMPP_ServiceArea::Request_GetPresence(Lsx::Request& request, Lsx::Response &response)
        {
            lsx::GetPresenceT req;

            if (!Read(request.document(), req))
            {
                return;
            }

            Chat::OriginConnection *connection = chatConnection();

            lsx::GetPresenceResponseT resp;

            if (connection && (connection->currentUser()->requestedVisibility() != Chat::ConnectedUser::Invisible))
            {
                const Chat::Presence presence = connection->currentUser()->requestedPresence();

                resp.Presence = presenceToLsx(presence, true);
                resp.RichPresence = presence.statusText().toUtf8();

                if (!presence.gameActivity().isNull())
                {
                    resp.GamePresence = presence.gameActivity().gamePresence().toUtf8();
                }

                Engine::UserRef user = Engine::LoginController::currentUser();

                if (!user.isNull())
                {
                    resp.SessionId = QString::fromUtf8(user->socialControllerInstance()->sdkPresence()->multiplayerSessionInfo().data());
                }
            }
            else
            {
                resp.Presence = lsx::PRESENCE_OFFLINE;
            }

            lsx::Write(response.document(), resp);
        }

        void XMPP_ServiceArea::Request_QueryPresence(Lsx::Request& request, Lsx::Response &response)
        {
            // We handle NULL inside the user loop below
            Chat::OriginConnection *connection = chatConnection();
            Origin::Chat::ConnectedUser *user = connection->currentUser();

            bool bLegacyUsersOnly = isLegacyConnection(request.connection());

            lsx::QueryPresenceT req;

            if (!Read(request.document(), req))
            {
                CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode request", response);
                return;
            }

            lsx::QueryPresenceResponseT resp;

            for(const auto userId : req.Users)
            {
                // Try to find the contact
                Chat::RemoteUser *remoteUser = NULL;

                // If we don't have a connection pretend we didn't find the user
                if (connection)
                {
                    const Chat::JabberID jid = connection->nucleusIdToJabberId(userId);
                    remoteUser = connection->remoteUserForJabberId(jid);
                }

                // When on a legacy connection, and the remote user is not a legacy user show as offline. (Even though we know his/her presence).
                if (remoteUser && (!bLegacyUsersOnly || remoteUser->legacyUser()))
                {
                    if(!user->blockList()->jabberIDs().contains(remoteUser->jabberId()))
                    {
                        resp.Friends.push_back(remoteUserToLsxFriend(remoteUser));
                    }
                    else
                    {
                        lsx::FriendT emptyFriend;

                        // This weirdness is copied from the old social code
                        emptyFriend.UserId = userId;
                        emptyFriend.Persona = "------";
                        emptyFriend.Group = "";
                        emptyFriend.State = lsx::FRIENDSTATE_NONE;
                        emptyFriend.Presence = lsx::PRESENCE_OFFLINE;

                        resp.Friends.push_back(emptyFriend);
                    }
                }
                else
                {
                    lsx::FriendT emptyFriend;

                    // This weirdness is copied from the old social code
                    emptyFriend.UserId = userId;
                    emptyFriend.Persona = "------";
                    emptyFriend.Group = "";
                    emptyFriend.State = lsx::FRIENDSTATE_NONE;
                    emptyFriend.Presence = lsx::PRESENCE_OFFLINE;

                    resp.Friends.push_back(emptyFriend);
                }
            }

            lsx::Write(response.document(), resp);
        }

        void XMPP_ServiceArea::Request_GetPresenceVisibility(Lsx::Request &request, Lsx::Response &response)
        {
            lsx::GetPresenceVisibilityT req;

            if(lsx::Read(request.document(), req))
            {
                lsx::GetPresenceVisibilityResponseT resp;

                Chat::OriginConnection *connection = chatConnection();

                if(connection)
                {
                    resp.Visible = connection->currentUser()->visibility() == Chat::ConnectedUser::Visible;

                    lsx::Write(response.document(), resp);
                }
                else
                {
                    CreateErrorResponse(ERROR_GENERAL, "Chat is not initialized", response);
                }
            }
            else
            {
                CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't understand the request.", response);
            }
        }

        void XMPP_ServiceArea::visibilityChanged( Origin::Chat::ConnectedUser::Visibility )
        {
            Chat::OriginConnection *connection = chatConnection();

            if(connection)
            {
                lsx::PresenceVisibilityEventT resp;

                resp.Visible = connection->currentUser()->visibility() == Chat::ConnectedUser::Visible;

                sendEvent(resp);
            }
        }

        void XMPP_ServiceArea::presenceChanged(const Origin::Chat::Presence &presence)
        {
            Chat::OriginConnection *connection = chatConnection();

            lsx::CurrentUserPresenceEventT resp;

            if (connection)
            {
                Engine::UserRef user = Engine::LoginController::currentUser();
                if(user)
                {
                    resp.UserId = user->userId();
                    resp.SessionId = QString::fromUtf8(user->socialControllerInstance()->sdkPresence()->multiplayerSessionInfo().data());
                }

                resp.Presence = presenceToLsx(presence);
                resp.RichPresence = presence.statusText().toUtf8();

                if (!presence.gameActivity().isNull())
                {
                    resp.GamePresence = presence.gameActivity().gamePresence().toUtf8();
                }
            }

            sendEvent(resp);

        }

        void XMPP_ServiceArea::Request_SetPresenceVisibility(Lsx::Request &request, Lsx::Response &response)
        {
            lsx::SetPresenceVisibilityT req;

            if(lsx::Read(request.document(), req))
            {
                Chat::OriginConnection *connection = chatConnection();

                if(connection)
                {
                    connection->currentUser()->requestVisibility(req.Visible ? Chat::ConnectedUser::Visible : Chat::ConnectedUser::Invisible);

                    CreateErrorResponse(EBISU_SUCCESS, "", response);
                }
                else
                {
                    CreateErrorResponse(ERROR_GENERAL, "Chat is not initialized", response);
                }
            }
            else
            {
                CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't understand the request.", response);
            }
        }

        void XMPP_ServiceArea::Request_QueryFriends(Lsx::Request& request, Lsx::Response &response)
        {
            Chat::OriginConnection *connection = chatConnection();

            if (!connection || !connection->currentUser()->roster()->hasLoaded())
            {
                // Not ready
                CreateErrorResponse(EBISU_PENDING, "Friends Services hasn't been logged into.", response);
                return;
            }

            lsx::QueryFriendsT req;

            if (!Read(request.document(), req))
            {
                return;
            }

            bool bLegacyUsersOnly = isLegacyConnection(request.connection());

            lsx::QueryFriendsResponseT resp;
            if(getFriendsResponse(resp, bLegacyUsersOnly) == false)
                return;

            lsx::Write(response.document(), resp);
        }

        void XMPP_ServiceArea::Request_QueryAreFriends(Lsx::Request& request, Lsx::Response &response)
        {
            // We handle NULL inside the user loop below
            Chat::OriginConnection *connection = chatConnection();

            lsx::QueryAreFriendsT req;

            if (!Read(request.document(), req))
            {
                return;
            }

            lsx::QueryAreFriendsResponseT resp;

            Engine::UserRef user = Engine::LoginController::currentUser(); 
            Chat::ConnectedUser *currentUser = user->socialControllerInstance()->chatConnection()->currentUser();

            bool bLegacyUsersOnly = isLegacyConnection(request.connection());

            for(const auto userId : req.Friends)
            {
                // Try to find the remote user
                Chat::RemoteUser *remoteUser = NULL;

                // If we don't have a connection pretend we didn't find the user
                if (connection)
                {
                    const Chat::JabberID jid = connection->nucleusIdToJabberId(userId);
                    remoteUser = connection->remoteUserForJabberId(jid);
                }

                if (remoteUser && (!bLegacyUsersOnly || remoteUser->legacyUser()))
                {
                    lsx::FriendStatusT friendStatus;
                    friendStatus.FriendId = userId;

                    if(!currentUser->blockList()->jabberIDs().contains(remoteUser->jabberId()))
                    {
                        friendStatus.State = subscriptionStateToLsx(remoteUser->subscriptionState());
                    }
                    else
                    {
                        friendStatus.State = lsx::FRIENDSTATE_NONE;
                    }


                    resp.Users.push_back(friendStatus);
                }
            }

            lsx::Write(response.document(), resp);
        }

        void XMPP_ServiceArea::Request_SendInvite(Lsx::Request& request, Lsx::Response &response)
        {
            lsx::SendInviteT req;

            if (!Read(request.document(), req))
            {
                populateErrorSuccessResponse(response, EBISU_ERROR_LSX_INVALID_REQUEST);
                return;
            }

            Engine::UserRef user = Engine::LoginController::currentUser();
            if (user.isNull() || (static_cast<uint64_t>(user->userId()) != req.UserId))
            {
                populateErrorSuccessResponse(response, EBISU_ERROR_INVALID_USER);
                return;
            }

            Engine::Social::SocialController *socialController = user->socialControllerInstance();
            for(uint32_t i = 0; i < req.Invitees.size(); i++)
            {
                const Chat::JabberID jid = socialController->chatConnection()->nucleusIdToJabberId(req.Invitees[i]);
                socialController->multiplayerInvites()->inviteToLocalSession(jid);
            }

            populateErrorSuccessResponse(response, EBISU_SUCCESS);
        }

        void XMPP_ServiceArea::Request_AcceptInvite(Lsx::Request& request, Lsx::Response &response)
        {
            lsx::AcceptInviteT req;

            if (!Read(request.document(), req))
            {
                populateErrorSuccessResponse(response, EBISU_ERROR_LSX_INVALID_REQUEST);
                return;
            }

            Engine::UserRef user = Engine::LoginController::currentUser();
            if (user.isNull() || (static_cast<uint64_t>(user->userId()) != req.UserId))
            {
                populateErrorSuccessResponse(response, EBISU_ERROR_INVALID_USER);
                return;
            }

            // Start the flow on the main thread
            const Chat::JabberID host = user->socialControllerInstance()->chatConnection()->nucleusIdToJabberId(req.OtherId);
            Origin::Client::ClientFlow::instance()->startJoinMultiplayerFlow(host);

            populateErrorSuccessResponse(response, EBISU_SUCCESS);
        }

        void XMPP_ServiceArea::Request_SendChat(Lsx::Request &request, Lsx::Response &response)
        {
            lsx::SendChatT req;
            if (!Read(request.document(), req))
            {
                return;
            }
            QSharedPointer<Engine::Social::Conversation> conversation = mConversations[req.ChatId];
            if (conversation)
            {
                conversation->conversable()->sendMessage(req.Body);
            }
            populateErrorSuccessResponse(response, EBISU_SUCCESS);
        }

        void XMPP_ServiceArea::Request_QueryChatInfo(Lsx::Request &request, Lsx::Response &response)
        {
            lsx::QueryChatInfoT req;

            if (!Read(request.document(), req))
            {
                return;
            }

            lsx::QueryChatInfoResponseT resp;

            QMapIterator<QString, QWeakPointer<Engine::Social::Conversation>> iter(mConversations);
            while (iter.hasNext())
            {
                iter.next();
                lsx::ChatInfoT chatInfo;
                chatInfo.ChatId = iter.key();

                /*
                switch(iter.value().data()->state())
                {
                case Engine::Social::Conversation::OneToOneState:
                    chatInfo.State = lsx::CHATSTATE_ONETOONE;
                    break;
                case Engine::Social::Conversation::MultiUserChatState:
                    chatInfo.State = lsx::CHATSTATE_MUC;
                    break;
                case Engine::Social::Conversation::FinishedState:
                    chatInfo.State = lsx::CHATSTATE_FINISHED;
                    break;
                default:
                    break;
                }
                */

                resp.Chats.push_back(chatInfo);
            }

            lsx::Write(response.document(), resp);
        }

        void XMPP_ServiceArea::Request_QueryChatUsers(Lsx::Request &request, Lsx::Response &response)
        {
            lsx::QueryChatUsersT req;

			if (!Read(request.document(), req))
            {
                return;
            }

            lsx::QueryChatUsersResponseT resp;

            QSharedPointer<Engine::Social::Conversation> conversation = mConversations[req.ChatId];

            const QSet<Chat::ConversableParticipant> participants(conversation->conversable()->participants());

            for(QSet<Chat::ConversableParticipant>::ConstIterator it = participants.constBegin();
                it != participants.constEnd();
                it++)
            {
                Chat::RemoteUser* remoteUser =  it->remoteUser();
                resp.ChatUsers.push_back(remoteUserToLsxFriend(remoteUser));
            }

            lsx::Write(response.document(), resp);
        }

        void XMPP_ServiceArea::Request_LeaveChat(Lsx::Request &request, Lsx::Response &response)
        {
            lsx::LeaveChatT req;

            if (!Read(request.document(), req))
            {
                return;
            }

            QSharedPointer<Engine::Social::Conversation> conversation = mConversations[req.ChatId];
            conversation->finish(Engine::Social::Conversation::UserFinished);
                        
            populateErrorSuccessResponse(response, EBISU_SUCCESS);
        }

        void XMPP_ServiceArea::Request_JoinChat(Lsx::Request &request, Lsx::Response &response)
        {
            lsx::JoinChatT req;

			if (!Read(request.document(), req))
            {
                return;
            }

            /*
            QSharedPointer<Engine::Social::Conversation> conversation = mConversations[req.ChatId];
            if (!conversation->incomingMucRoom().isNull())
            {
                //conversation->acceptMucRoomInvite();
            }
            */
            populateErrorSuccessResponse(response, EBISU_SUCCESS);
        }

        void XMPP_ServiceArea::Request_CreateChat(Lsx::Request &request, Lsx::Response &response)
        {
            lsx::CreateChatT req;

            if (!Read(request.document(), req))
            {
                return;
            }

            Chat::OriginConnection *connection = chatConnection();

            if (!connection)
            {
                return;
            }

            Engine::Social::ConversationManager* conversations = chatConversation();
            if (req.UserIds.size() == 1)
            {
                Chat::RemoteUser* user = connection->remoteUserForJabberId(connection->nucleusIdToJabberId(req.UserIds[0]));
                conversations->conversationForConversable(user, Engine::Social::Conversation::SDK);
            }
            else
            {
                /*
                QList<Chat::RemoteUser*> inviteeList;

                for (unsigned int i = 0; i < req.UserIds.size(); i++)
                {
                    Chat::RemoteUser* user = connection->remoteUserForJabberId(connection->nucleusIdToJabberId(req.UserIds[i]));
                    inviteeList << user;
                }

                Chat::EnterMucRoomOperation* roomOp = conversations->createMultiUserConversation(inviteeList);
                (void) roomOp;
                */
            }

            populateErrorSuccessResponse(response, EBISU_SUCCESS);
        }

        void XMPP_ServiceArea::Request_AddUserToChat(Lsx::Request &request, Lsx::Response &response)
        {
            lsx::AddUserToChatT req;

            if (!Read(request.document(), req))
            {
                return;
            }

            QSharedPointer<Engine::Social::Conversation> conversation = mConversations[req.ChatId];
            for (unsigned int i = 0; i < req.UserIds.size(); i++)
            {
                //conversation->inviteRemoteUser(chatConnection()->remoteUserForJabberId(chatConnection()->nucleusIdToJabberId(req.UserIds[i])));
            }            
        
            populateErrorSuccessResponse(response, EBISU_SUCCESS);
        }

        void XMPP_ServiceArea::userLoggedIn(Origin::Engine::UserRef user)
        {
            Engine::Social::SocialController *social = user->socialControllerInstance();

            ORIGIN_VERIFY_CONNECT(
                social->chatConnection(), SIGNAL(messageReceived(Origin::Chat::Conversable *, const Origin::Chat::Message &)),
                this, SLOT(messageReceived(Origin::Chat::Conversable *, const Origin::Chat::Message &)));

            ORIGIN_VERIFY_CONNECT(
                social->multiplayerInvites(), SIGNAL(invitedToRemoteSession(const Origin::Engine::MultiplayerInvite::Invite &)),
                this, SLOT(invitedToRemoteSession(const Origin::Engine::MultiplayerInvite::Invite &)));

            ORIGIN_VERIFY_CONNECT(
                social->multiplayerInvites(), SIGNAL(sendSessionToConnectedGame(const Origin::Engine::MultiplayerInvite::Invite &, bool)),
                this, SLOT(sendSessionToConnectedGame(const Origin::Engine::MultiplayerInvite::Invite &, bool)));

            ORIGIN_VERIFY_CONNECT(
                social->chatConnection()->currentUser(), SIGNAL(visibilityChanged(Origin::Chat::ConnectedUser::Visibility)),
                this, SLOT(visibilityChanged(Origin::Chat::ConnectedUser::Visibility)));

            ORIGIN_VERIFY_CONNECT(
                social->chatConnection()->currentUser(), SIGNAL(presenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)),
                this, SLOT(presenceChanged(const Origin::Chat::Presence &)));
        
            ORIGIN_VERIFY_CONNECT(social->conversations(),
                SIGNAL(conversationCreated(QSharedPointer<Origin::Engine::Social::Conversation>)),
                this,
                SLOT(onConversationCreated(QSharedPointer<Origin::Engine::Social::Conversation>)) );          
        
            ORIGIN_VERIFY_CONNECT(social->chatConnection()->currentUser()->roster(),
                SIGNAL(updated()),
                this,
                SLOT(onRosterUpdated()));

            ORIGIN_VERIFY_CONNECT(social->chatConnection()->currentUser()->roster(), 
                SIGNAL(contactAdded(Origin::Chat::RemoteUser*)), 
                this, 
                SLOT(subscribeToContact(Origin::Chat::RemoteUser*)));

            ORIGIN_VERIFY_CONNECT(social->chatConnection()->currentUser()->roster(), 
                SIGNAL(contactRemoved(Origin::Chat::RemoteUser*)), 
                this, 
                SLOT(unsubscribeToContact(Origin::Chat::RemoteUser*)));
        }

        void XMPP_ServiceArea::onRosterUpdated()
        {
            lsx::QueryFriendsResponseT resp;

            bool bLegacyUsersOnly = hasLegacyConnection();

            if (getFriendsResponse(resp, bLegacyUsersOnly) == false)
                return;

			sendEvent(resp);
        }

        void XMPP_ServiceArea::subscribeToContact(Origin::Chat::RemoteUser* contact)
        {
            ORIGIN_VERIFY_CONNECT(contact, 
                SIGNAL(presenceChanged(Origin::Chat::Presence, Origin::Chat::Presence)), 
                this, 
                SLOT(onContactPresenceChanged(Origin::Chat::Presence, Origin::Chat::Presence)));
        }

        void XMPP_ServiceArea::unsubscribeToContact(Origin::Chat::RemoteUser* contact)
        {
            ORIGIN_VERIFY_DISCONNECT(contact, 
                SIGNAL(presenceChanged(Origin::Chat::Presence, Origin::Chat::Presence)), 
                this, 
                SLOT(onContactPresenceChanged(Origin::Chat::Presence, Origin::Chat::Presence)));
        }

        void XMPP_ServiceArea::onContactPresenceChanged(Origin::Chat::Presence newPresence, Origin::Chat::Presence /*oldPresence*/)
        {
            if (newPresence.isNull())
                return;

            lsx::GetPresenceResponseT resp;

            bool bLegacyUsersOnly = hasLegacyConnection();
            
            Origin::Chat::RemoteUser* user = dynamic_cast<Origin::Chat::RemoteUser*>(sender());

            if (user && (!bLegacyUsersOnly || user->legacyUser()))
            {
                resp.UserId = user->nucleusId();

                resp.Presence = presenceToLsx(newPresence);
                resp.RichPresence = newPresence.statusText().toUtf8();

                if (!newPresence.gameActivity().isNull())
                {
                    resp.GamePresence = newPresence.gameActivity().gamePresence().toUtf8();
                    resp.MultiplayerId = newPresence.gameActivity().multiplayerId();
                    resp.Title = newPresence.gameActivity().gameTitle().toUtf8();
                    resp.TitleId = newPresence.gameActivity().productId();
                }

                sendEvent(resp);
            }
        }

        void XMPP_ServiceArea::messageReceived(Origin::Chat::Conversable* conversable, const Origin::Chat::Message &message)
        {
            if (message.type() == "chat" || message.type() == "groupchat")
            {
                QSharedPointer<Engine::Social::Conversation> conversation = 
                    chatConversation()->conversationForConversable(conversable, Engine::Social::Conversation::SDK);

                QString key;
                QMapIterator<QString, QWeakPointer<Engine::Social::Conversation>> iter(mConversations);
                while (iter.hasNext())
                {
                    iter.next();
                    if (!iter.value().isNull() && iter.value().data() == conversation.data())
                    {
                        key = iter.key();
                        break;
                    }
                }

                lsx::ChatMessageEventT chatMessage;

                chatMessage.ChatId = key;
                chatMessage.Body = message.body();
                chatMessage.UserId = message.from().node().toULongLong();

                sendEvent(chatMessage);
            }
        }

        void XMPP_ServiceArea::invitedToRemoteSession(const Origin::Engine::MultiplayerInvite::Invite &invite)
        {
            Chat::OriginConnection *connection = chatConnection();

            if (!connection)
            {
                return;
            }

            lsx::MultiplayerInvitePendingT pendingInvite;

            pendingInvite.from = connection->jabberIdToNucleusId(invite.from());
            pendingInvite.MultiplayerId = invite.sessionInfo().multiplayerId();

            sendEvent(pendingInvite);
            }

        void XMPP_ServiceArea::sendSessionToConnectedGame(const Origin::Engine::MultiplayerInvite::Invite &invite, bool initial)
        {
            Chat::OriginConnection *connection = chatConnection();

            if (!connection)
            {
                return;
            }

			lsx::MultiplayerInviteT multiplayerInvite;

			multiplayerInvite.initial = initial;
			multiplayerInvite.from = connection->jabberIdToNucleusId(invite.from());
            multiplayerInvite.multiplayerId = invite.sessionInfo().multiplayerId();
			multiplayerInvite.SessionInformation = invite.sessionInfo().data();

            sendEvent(multiplayerInvite, invite.sessionInfo().multiplayerId().toUtf8());
        }

        void XMPP_ServiceArea::CreateErrorResponse( int code, const char * description, Lsx::Response &response )
        {
            lsx::ErrorSuccessT resp;

            resp.Code = code;
            resp.Description = description;

            lsx::Write(response.document(), resp);
        }

        void XMPP_ServiceArea::onConversationCreated(QSharedPointer<Origin::Engine::Social::Conversation> conv)
        {
            QString uuid = QUuid::createUuid().toString();
            uuid.replace('{', "").replace('}', "").replace('-', "");

            mConversations.insert(uuid, conv.toWeakRef());
            
            lsx::ChatCreatedEventT chatCreated;

            chatCreated.ChatId = uuid;

            sendEvent(chatCreated);
            
            ORIGIN_VERIFY_CONNECT(conv.data(), SIGNAL(conversationFinished(Origin::Engine::Social::Conversation::FinishReason)), this, SLOT(onConversationFinished()));
            ORIGIN_VERIFY_CONNECT(conv.data(), SIGNAL(eventAdded(const Origin::Engine::Social::ConversationEvent*)), this, SLOT(onConversationUpdated(const Origin::Engine::Social::ConversationEvent*)));
        }

        void XMPP_ServiceArea::onConversationFinished()
        {
            Engine::Social::Conversation* conv = dynamic_cast<Engine::Social::Conversation*>(sender());

            QString key;
            QMapIterator<QString, QWeakPointer<Engine::Social::Conversation>> iter(mConversations);
            while (iter.hasNext())
            {
                iter.next();
                if (!iter.value().isNull() && iter.value().data() == conv)
                {
                    key = iter.key();
                    break;
                }
            }

            if (key.isEmpty())
                return;

            mConversations.remove(key);

            lsx::ChatFinishedEventT chatFinished;

            chatFinished.ChatId = key;

            sendEvent(chatFinished);
            }

        void XMPP_ServiceArea::onConversationUpdated(const Origin::Engine::Social::ConversationEvent* event)
        {
            if (dynamic_cast<const Engine::Social::ParticipantEnterEvent*>(event) ||
                dynamic_cast<const Engine::Social::ParticipantExitEvent*>(event))
            {
                Engine::Social::Conversation* conv = dynamic_cast<Engine::Social::Conversation*>(sender());

                QString key;
                QMapIterator<QString, QWeakPointer<Engine::Social::Conversation>> iter(mConversations);
                for (; iter.hasNext(); iter.next())
                {
                    if (!iter.value().isNull() && iter.value().data() == conv)
                    {
                        key = iter.key();
                        break;
                    }
                }

                if (key.isEmpty())
                    return;

                lsx::ChatUpdatedEventT chatUpdated;

                chatUpdated.ChatId = key;

                sendEvent(chatUpdated);
                
            }
        }

        void XMPP_ServiceArea::Request_BlockUser(Lsx::Request &request, Lsx::Response &response)
        {
            lsx::BlockUserT blockUser;

            if (Read(request.document(), blockUser))
            {
                Chat::OriginConnection *connection = chatConnection();
                Origin::Chat::ConnectedUser *user;

				if(!Services::Connection::ConnectionStatesService::isUserOnline (Engine::LoginController::instance()->currentUser()->getSession()))
				{
					CreateErrorResponse(EBISU_ERROR_NO_NETWORK, "Origin is offline",response);
					return;
				}
                if((connection == NULL) || ((user = connection->currentUser()) == NULL))
                {
                    CreateErrorResponse(ERROR_GENERAL, "BlockUser failed", response);
                    return;
                }

                Origin::Client::ClientFlow::instance()->socialViewController()->blockUser(blockUser.UserIdToBlock);

                CreateErrorResponse(EBISU_SUCCESS, "Request sent", response);
                return;
            }

            CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "Cannot decode request.", response);
            return;
        }

        void XMPP_ServiceArea::Request_UnblockUser(Lsx::Request &request, Lsx::Response &response)
        {
            lsx::UnblockUserT unblockUser;

            if (Read(request.document(), unblockUser))
            {
                Chat::OriginConnection *connection = chatConnection();
                Origin::Chat::ConnectedUser *user;

				if(!Services::Connection::ConnectionStatesService::isUserOnline (Engine::LoginController::instance()->currentUser()->getSession()))
				{
					CreateErrorResponse(EBISU_ERROR_NO_NETWORK, "Origin is offline",response);
					return;
				}
                if((connection == NULL) || ((user = connection->currentUser()) == NULL))
                {
                    CreateErrorResponse(ERROR_GENERAL, "UnblockUser failed", response);
                    return;
                }

                user->blockList()->removeJabberID(connection->nucleusIdToJabberId(unblockUser.UserIdToUnblock));

                CreateErrorResponse(EBISU_SUCCESS, "Request sent", response);
                return;
            }

            CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "Cannot decode request.", response);
            return;
        }

        void XMPP_ServiceArea::Request_RequestFriend(Lsx::Request &request, Lsx::Response &response)
        {
            lsx::RequestFriendT requestFriend;

            if (Read(request.document(), requestFriend))
            {
                Chat::OriginConnection *connection = chatConnection();
                Origin::Chat::ConnectedUser *user;
				Chat::JabberID id = connection->nucleusIdToJabberId(requestFriend.UserToAdd);

				if(!Services::Connection::ConnectionStatesService::isUserOnline (Engine::LoginController::instance()->currentUser()->getSession()))
				{
					CreateErrorResponse(EBISU_ERROR_NO_NETWORK, "Origin is offline",response);
					return;
				}
                if((connection == NULL) || ((user = connection->currentUser()) == NULL))
                {
                    CreateErrorResponse(ERROR_GENERAL, "Add Friend failed", response);
                    return;
                }

                for (const auto contact : user->roster()->contacts())
				{
					if(contact->jabberId() == id)
					{
						if(contact->subscriptionState().direction() == Chat::SubscriptionState::DirectionBoth)
						{
							CreateErrorResponse(EBISU_SUCCESS, "User is already friend", response);
							return;
						}					
						else if(contact->subscriptionState().isPendingCurrentUserApproval()) //Incoming invite
						{
							user->roster()->approveSubscriptionRequest(id);
							CreateErrorResponse(EBISU_SUCCESS, "Other user already sent request, accepting instead", response);
							return;
						}
					}
				}

                user->roster()->requestSubscription(id);

                CreateErrorResponse(EBISU_SUCCESS, "Request sent", response);
                return;
            }

            CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "Cannot decode request.", response);
            return;
        }

		void XMPP_ServiceArea::Request_AcceptFriendInvite(Lsx::Request &request, Lsx::Response &response)
		{
			lsx::AcceptFriendInviteT acceptFriendInvite;

			if (Read(request.document(), acceptFriendInvite))
			{
				Chat::OriginConnection *connection = chatConnection();
				Origin::Chat::ConnectedUser *user;
				Chat::JabberID id = connection->nucleusIdToJabberId(acceptFriendInvite.OtherId);

				if(!Services::Connection::ConnectionStatesService::isUserOnline (Engine::LoginController::instance()->currentUser()->getSession()))
				{
					CreateErrorResponse(EBISU_ERROR_NO_NETWORK, "Origin is offline",response);
					return;
				}
				if((connection == NULL) || ((user = connection->currentUser()) == NULL))
				{
					CreateErrorResponse(ERROR_GENERAL, "Accept Invite failed", response);
					return;
				}

				bool found = false;

                for (const auto contact : user->roster()->contacts())
				{
					if(contact->jabberId() == id)
					{
						if(contact->subscriptionState().direction() == Chat::SubscriptionState::DirectionBoth)
						{
							CreateErrorResponse(EBISU_SUCCESS, "User is already friend", response);
							return;
						}					
						else if(contact->subscriptionState().isPendingCurrentUserApproval()) //Incoming invite
						{
							user->roster()->approveSubscriptionRequest(id);
							found = true;
						}
						else if(contact->subscriptionState().isPendingContactApproval()) //Outgoing invite
						{
							CreateErrorResponse(EBISU_SUCCESS, "Pending approval from external friend", response);
							return;
						}
					}
				}

				if(found)
				{
					CreateErrorResponse(EBISU_SUCCESS, "Invite Accepted", response);
				}
				else
				{
					CreateErrorResponse(EBISU_ERROR_NOT_FOUND, "Couldn't find the request.", response);
				}

				return;

			}

			CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "Cannot decode request.", response);
			return;
		}

        void XMPP_ServiceArea::Request_RemoveFriend(Lsx::Request &request, Lsx::Response &response)
        {
            lsx::RemoveFriendT removeFriend;

            if (Read(request.document(), removeFriend))
            {
                Chat::OriginConnection *connection = chatConnection();
                Origin::Chat::ConnectedUser *user;

				if(!Services::Connection::ConnectionStatesService::isUserOnline (Engine::LoginController::instance()->currentUser()->getSession()))
				{
					CreateErrorResponse(EBISU_ERROR_NO_NETWORK, "Origin is offline",response);
					return;
				}
                if((connection == NULL) || ((user = connection->currentUser()) == NULL))
                {
                    CreateErrorResponse(ERROR_GENERAL, "Remove Friend failed", response);
                    return;
                }

                Chat::JabberID id = connection->nucleusIdToJabberId(removeFriend.UserToRemove);

                bool found = false;
                for (const auto contact : user->roster()->contacts())
                {
                    if(contact->jabberId() == id)
                    {
                        if(contact->subscriptionState().direction() == Chat::SubscriptionState::DirectionBoth)
                        {
                            user->roster()->removeContact(contact);
                            found = true;
                            break;
                        }
						else if(contact->subscriptionState().isPendingContactApproval()) //Outgoing invite
						{
							user->roster()->cancelSubscriptionRequest(id);
							found = true;
							break;
						}
						else if(contact->subscriptionState().isPendingCurrentUserApproval()) //Incoming invite
						{
							user->roster()->denySubscriptionRequest(id);
							found = true;
							break;
						}
                        else
                        {
                            CreateErrorResponse(EBISU_ERROR_INVALID_USER, "The user is not a mutual, invited or pending invite friend", response);
                            return;
                        }
                    }
                }

                if(found)
                {
                    CreateErrorResponse(EBISU_SUCCESS, "Request sent", response);
                }
                else
                {
                    CreateErrorResponse(EBISU_ERROR_NOT_FOUND, "Couldn't find the friend.", response);
                }

                return;
            }

            CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "Cannot decode request.", response);
            return;
        }

        bool XMPP_ServiceArea::hasLegacyConnection() const
        {
            bool legacyOnly = false;

            for (const auto connection : handler()->connections())
            {
                if (isLegacyConnection(connection))
                {
                    legacyOnly = true;
                    break;
                }
            }

            return legacyOnly;
        }

        bool XMPP_ServiceArea::isLegacyConnection(const Lsx::Connection *connection) const
        {
            // This function can become more complex if we want older games to be enabled for more than 100 friends.
            return connection->CompareSDKVersion(LEGACY_FRIENS_SDK_VERSION_THRESHOLD) < 0;
        }

    }
}


