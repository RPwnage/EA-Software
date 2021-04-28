/*! ************************************************************************************************/
/*!
    \file ProtocolActivation.h

    \brief Util for Xbox One activation protocol handling.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef MULTIPLAYER_PROTOCOL_ACTIVATION_H
#define MULTIPLAYER_PROTOCOL_ACTIVATION_H
#if defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)
#include "Ignition/xboxone/XdkVersionDefines.h"

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/shared/framework/util/blazestring.h"// for parsing in getTargetXuid
#include "BlazeSDK/component/framework/tdf/userdefines.h" // for ExternalId in getTargetXuid

namespace Ignition
{
    class ProtocolActivationInfo
    {

    public:
        static bool initProtocolActivation(Windows::ApplicationModel::Activation::IProtocolActivatedEventArgs^ const args);
        static void checkProtocolActivation();

    public:
        enum ProtocolActivationType
        {
            NONE, // no or un-handled protocol activation
            ACTIVITY_HANDLE_JOIN,
            INVITE_HANDLE_ACCEPT,
            PARTY_INVITE_ACCEPT,
            TOURNAMENT,
            TOURNAMENT_TEAM_VIEW
        };

        ProtocolActivationInfo();

        bool init(Windows::ApplicationModel::Activation::IProtocolActivatedEventArgs^ const args);
        void clear() { mActivationType = NONE; }

        ProtocolActivationType getActivationType() const { return mActivationType; }
        Platform::String^ const getHandleId() const { return mHandleId; }
        Platform::String^ const getTargetXuid() const { return mTargetXuid; }
        Platform::String^ const getJoineeXuid() const { return mJoineeXuid; }
        Platform::String^ const getSenderXuid() const { return mSenderXuid; }
        Platform::String^ const getTournamentId() const { return mTournamentId; }
        Platform::String^ const getOrganizerId() const { return mOrganizerId; }
        Platform::String^ const getRegistrationState() const { return mRegistrationState; }
        Platform::String^ const getSessionName() const { return mSessionName; }
        Platform::String^ const getTemplateName() const { return mTemplateName; }
        Platform::String^ const getScid() const { return mScid; }

        static Platform::String^ getErrorStringForException(Platform::Exception^ ex);
    // helpers
    private:
        void init(ProtocolActivationType activationType, Platform::String^ const handleId, Platform::String^ const targetXuid, Platform::String^ const joineeXuid, Platform::String^ const senderXuid, Platform::String^ const tournamentId, Platform::String^ const organizerId, Platform::String^ const registrationState, Platform::String^ const sessionName, Platform::String^ const templateName, Platform::String^ const scid);
        static Platform::String^ const getQueryStringValue(Windows::Foundation::Uri^ const uri, Platform::String^ const paramName);
        static const char8_t* toEastlStr(eastl::string& handleIdBuf, Platform::String^ const platformString);

        static void processJoinByHandle(Platform::String^ const handleStr, bool fromInvite);
        static void processAcceptPartyInvite();
        static void processTournamentActivation(Platform::String^ const joinerXuid, Platform::String^ const registrationXuid, Platform::String^ const tournamentId, Platform::String^ const organizerId, Platform::String^ const registrationState, Platform::String^ const sessionName, Platform::String^ const templateName, Platform::String^ const scid);
        static void processTournamentSessionJoin(Platform::String^ const joinerXuid, Platform::String^ const sessionName, Platform::String^ const templateName, Platform::String^ const scid);
        static void processTournamentRegister(Platform::String^ const tournamentId, Platform::String^ const organizerId, Platform::String^ const targetXuid, Platform::String^ const scid);
        static void processTournamentWithdraw(Platform::String^ const tournamentId, Platform::String^ const organizerId, Platform::String^ const targetXuid, Platform::String^ const scid);
        static void processTournamentView(Platform::String^ const tournamentId, Platform::String^ const organizerId, Platform::String^ const scid);
        static void processTournamentTeamView(Platform::String^ const tournamentId, Platform::String^ const organizerId, Platform::String^ const sessionName, Platform::String^ const templateName, Platform::String^ const scid);
    private:
        static Ignition::ProtocolActivationInfo mProtocolActivationInfo;

        ProtocolActivationType mActivationType;
        Platform::String^ mHandleId;
        Platform::String^ mTargetXuid;
        Platform::String^ mJoineeXuid;
        Platform::String^ mSenderXuid;
        Platform::String^ mTournamentId;
        Platform::String^ mOrganizerId;
        Platform::String^ mRegistrationState;
        Platform::String^ mSessionName;
        Platform::String^ mTemplateName;
        Platform::String^ mScid;
    };
   
} // ns Ignition

#endif //defined _XDK_VER >= __XDK_VER_2015_MP
#endif //MULTIPLAYER_PROTOCOL_ACTIVATION_H
