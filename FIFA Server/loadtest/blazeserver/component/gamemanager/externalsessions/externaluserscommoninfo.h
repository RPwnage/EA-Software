/*! ************************************************************************************************/
/*!
    \file   externaluserscommoninfo.h

    \brief Utils for external user info

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_GAMEMANAGER_EXTERNAL_USERS_COMMON_INFO_H
#define BLAZE_GAMEMANAGER_EXTERNAL_USERS_COMMON_INFO_H

#include "gamemanager/tdf/gamemanager_server.h" // for UserJoinInfoList in getFirstExternalSessionJoinerInfo

namespace Blaze
{
    class UserIdentification;
    class UserInfoData;
    enum ClientPlatformType;
    
namespace GameManager
{
    const char8_t* externalUserIdentificationToString(const UserIdentification& ident, char8_t* buf, size_t bufLen);

    const char8_t* externalUserIdentificationToString(const UserIdentification &ident, eastl::string& buf);

    const Blaze::FixedString256 toExtUserLogStr(const UserIdentification &ident);

    bool isExternalUserIdentificationForPlayer(const UserIdentification& ident, const UserInfoData& playerInfo);

    bool areUserIdsEqual(const UserIdentification& id1, const UserIdentification& id2);

    /*! \brief return whether the inputs identify a potential primary platform first party user.
    ***************************************************************************************************/
    bool hasFirstPartyId(const PlatformInfo& platformInfo);

    /*! ************************************************************************************************/
    /*! \brief return whether the identification info container has at least necessary first party relevant
         fields to identify a primary platform first party user.
        \param[in] ident user's identification info. Note, depending on the platform, certain fields must
        be set in order to create external player objects in games for the user.
    ***************************************************************************************************/
    bool hasFirstPartyId(const UserIdentification& ident);
    /*! ************************************************************************************************/
    /*! \brief retrieve the user info for the player by its first party identification info, or BlazeId
        \param[in] ident contains player's first party identification info and/or BlazeId
        \return USER_ERR_USER_NOT_FOUND if user info not on the Blaze Server (or its db), ERR_OK if it is
    ***************************************************************************************************/
    BlazeRpcError lookupExternalUserByUserIdentification(const UserIdentification& ident, UserInfoPtr &userInfo);
    void userIdentificationListToUserSessionIdList(const UserIdentificationList& externalUsers, UserSessionIdList& userSessionIdList);

    /*! ************************************************************************************************/
    /*! \brief add copies of the joined reserved players info from one response to other response
    ***************************************************************************************************/
    void joinedReservedPlayerIdentificationsToOtherResponse(const JoinGameResponse& source, CreateGameResponse& target);

    /*! ************************************************************************************************/
    /*! \brief add copies of the joined reserved players info from one response to other response
    ***************************************************************************************************/
    void joinedReservedPlayerIdentificationsToOtherResponse(const CreateGameResponse& source, JoinGameResponse& target);

    /*! ************************************************************************************************/
    /*! \brief Find first user in the list, if one exists, with external session join permission. I.e. the first
        who could create/join the game's external session.
        If platform is not INVALID, only returns users whose platform matches the requested platform.

        Returns nullptr if no user is found
    ***************************************************************************************************/
    const UserSessionInfo* getFirstExternalSessionJoinerInfo(const UserJoinInfoList& usersInfo, ClientPlatformType platform = INVALID);

} // namespace GameManager
} // namespace Blaze

#endif //BLAZE_GAMEMANAGER_EXTERNAL_USERS_COMMON_INFO_H
