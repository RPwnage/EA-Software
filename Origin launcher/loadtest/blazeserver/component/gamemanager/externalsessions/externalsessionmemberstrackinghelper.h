/*! ************************************************************************************************/
/*!
    \file externalsessionmemberstrackinghelper.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef EXTERNAL_SESSION_MEMBERS_TRACKING_HELPER_H
#define EXTERNAL_SESSION_MEMBERS_TRACKING_HELPER_H

#include "EASTL/string.h"
#include "gamemanager/tdf/gamemanager.h"

namespace Blaze
{
class ExternalSessionActivityTypeInfo;
class ExternalSessionIdentification;
class ExternalMemberInfo;
class UserIdentification;
namespace Arson { class ArsonSlaveImpl; }

namespace GameManager
{
    class GameManagerSlaveImpl;
    class GameManagerMaster;
    class GetExternalSessionInfoMasterResponse;
    class TrackExtSessionMembershipRequest;

    /*! ************************************************************************************************/
    /*! \brief Helpers for tracking external session members with GameManager
    ***************************************************************************************************/
    class ExternalSessionMembersTrackingHelper
    {
        NON_COPYABLE(ExternalSessionMembersTrackingHelper);
    public:

        ExternalSessionMembersTrackingHelper(GameManagerSlaveImpl* gameManagerSlave, ClientPlatformType activityTypePlatform, ExternalSessionActivityType activityType, ClientPlatformType crossgenClientPlatform = INVALID);
        virtual ~ExternalSessionMembersTrackingHelper() {}

        /*! ************************************************************************************************/
        /*! \brief return the platform/activity type the instance of this util updates
        ***************************************************************************************************/
        const ExternalSessionActivityTypeInfo& getExternalSessionActivityTypeInfo() const { return mExternalSessionActivityTypeInfo; }

        BlazeRpcError trackExtSessionMembershipOnMaster(GetExternalSessionInfoMasterResponse& result,
            const GetExternalSessionInfoMasterResponse& gameInfo, 
            const ExternalMemberInfo& member, 
            const ExternalSessionIdentification& extSessionToAdvertise) const;

        BlazeRpcError untrackExtSessionMembershipOnMaster(UntrackExtSessionMembershipResponse* result,
            GameId gameId,
            const ExternalMemberInfo* member,
            const ExternalSessionIdentification* extSessToUntrackAll) const;


        BlazeRpcError getGameInfoFromMaster(GetExternalSessionInfoMasterResponse& gameInfo, GameId gameId, const UserIdentification& caller) const;
        bool isAddUserToExtSessionRequired(GetExternalSessionInfoMasterResponse& gameInfo, GameId gameId, const UserIdentification& user) const;

    protected:
        GameManagerMaster* getMaster() const;
        const char8_t* logPrefix() const { return (mLogPrefix.empty() ? mLogPrefix.sprintf("[ExternalSessionMembersTrackingHelper(%s:%d)]", ClientPlatformTypeToString(getExternalSessionActivityTypeInfo().getPlatform()), getExternalSessionActivityTypeInfo().getType()).c_str() : mLogPrefix.c_str()); }

    private:
        ExternalSessionActivityTypeInfo mExternalSessionActivityTypeInfo;
        GameManagerSlaveImpl* mGameManagerSlave;
        mutable eastl::string mLogPrefix;
    #ifdef BLAZE_ARSONCOMPONENT_SLAVEIMPL_H
        friend class Blaze::Arson::ArsonSlaveImpl;
    #endif
    };

}//ns
}//Blaze

#endif
